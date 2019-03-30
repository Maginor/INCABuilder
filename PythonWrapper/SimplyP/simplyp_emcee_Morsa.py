

import matplotlib, matplotlib.pyplot as plt, seaborn as sn, emcee, corner, imp, pandas as pd, datetime as dt, random
import numpy as np
import time

wrapper_fpath = (r"..\inca.py")
optimize_funs_fpath = (r'..\inca_calibration.py')

wr = imp.load_source('inca', wrapper_fpath)
cf = imp.load_source('inca_calibration', optimize_funs_fpath)	

from multiprocessing import Pool


def log_prior(params, min, max) :
	"""
	params: list or array of parameter values
	min: list or array of minimum parameter values
	max: list or array of maximum parameter values
	"""
	if cf.check_min_max(params, min, max) :
		return 0
	return -np.inf

#IMPORTANT NOTE: If you want to run emcee multithreaded: The log_posterior function has to access the dataset as a global. It can not take it as an argument. This is because ctypes of pointer type can not be 'pickled',
#  which is what python does when it sends arguments to functions on separate threads. In that case, you will end up with a garbage value for the pointer.
def log_posterior(params, min, max, calibration, objective):
	"""
	params: list or array of parameter values
	min: list or array of minimum parameter values
	max: list or array of maximum parameter values
	calibration: 'calibration' structure is a list of tuples, each tuple containing (param name string, [index string])
	objective: tuple (- function to use as measure of model performance,
					  - list containing simulated and observed variables you want to compare. Each element in the list is a 4-element tuple, with    items (name of simulated result, [index result applies to], name of observation input data, [index for observed data])
					  - skiptimesteps: integer, the number of timesteps to discard from the start of the model run when comparing performance (i.e. warmup period)
	"""
	log_pri = log_prior(params, min, max)
	
	llfun = objective[0]
	
	if(np.isfinite(log_pri)):
		log_like = llfun(params, dataset, calibration, objective)
		return log_pri + log_like
	return -np.inf


	
def run_emcee(min, max, initial_guess, calibration, objective, n_walk, n_steps) :
	
	ll, comparisons, skiptimesteps = objective


	n_dim = len(initial_guess)

	starting_guesses = [initial_guess + 1e-4*np.random.randn(n_dim) for i in range(n_walk)]

	
	pool = Pool(8)
		
	sampler = emcee.EnsembleSampler(n_walk, n_dim, log_posterior, 
		#threads=8,
		pool = pool,
		args=[min, max, calibration, objective])

	start = time.time()
	pos, prob, state = sampler.run_mcmc(starting_guesses, n_steps)
	end = time.time()
	print('Time elapsed running emcee: %f' % (end - start))
	
	print('EMCEE average acceptance rate: %f' % np.mean(sampler.acceptance_fraction))

	samples = sampler.chain
	
	lnprobs = sampler.lnprobability
	
	pool.close()

	return samples, lnprobs

def chain_plot(samples, calibration, labels_long, filename):
	#NOTE: This one needs non-reshaped samples
	
	n_dim = samples.shape[2]

	fig, axes = plt.subplots(nrows=n_dim, ncols=1, figsize=(10, 20))    
	for idx, title in enumerate(labels_long):        
		axes[idx].plot(samples[:,:,idx].T, '-', color='k', alpha=0.3)
		axes[idx].set_title(title, fontsize=16) 
	plt.subplots_adjust(hspace=0.7)    
	
	fig.savefig(filename)
	
def reshape_samples(samples, lnprobs, n_burn):
	n_dim = samples.shape[2]
	s = samples[:, n_burn:, :].reshape((-1, n_dim))
	ln = lnprobs[:, n_burn:].reshape((-1))
	
	return s, ln
	
def best_sample(samplelist, lnproblist):
	#NOTE Needs reshaped samples
	index_max = np.argmax(lnproblist)
	best_sample = samplelist[index_max]
	return best_sample	
	
def triangle_plot(samplelist, labels_short, filename):
	#NOTE Needs reshaped samples
	tri = corner.corner(samplelist,
						labels=labels_short,
						#truths=truths,
						quantiles=[0.025, 0.5, 0.975],
						show_titles=True, 
						title_args={'fontsize': 24},
						label_kwargs={'fontsize': 20})

	tri.savefig(filename)
	
def plot_n_random_samples(dataset, samplelist, lnproblist, calibration, objective, n_random_samples, filename) :

	llfun, comparisons, skiptimesteps = objective
	
	comparisontolookat = comparisons[0] #TODO: Do all comparisons in multiplot?
	simname, simindexes, obsname, obsindexes = comparisontolookat
	
	fig, ax = plt.subplots()
	
	start_date = dt.datetime.strptime(dataset.get_parameter_time('Start date', []),'%Y-%m-%d')
	timesteps = dataset.get_parameter_uint('Timesteps', [])
	date_idx = np.array(pd.date_range(start_date, periods=timesteps))
	
	for it in range(1, n_random_samples) :       #TODO: Should be paralellized, really..
		
		random_index = random.randint(0, len(samplelist)-1)
		#print(random_index)
		random_sample = samplelist[random_index]
		#print(random_sample)
		
		cf.set_values(dataset, random_sample, calibration)
		dataset.run_model()
	
		sim = dataset.get_result_series(simname, simindexes)
	
		a = max(1/256, 1/n_random_samples)
		#a = 0.01
		ax.plot(date_idx, sim, color='black', alpha=a, label='_nolegend_') #,linewidth=1)
		
	obs = dataset.get_input_series(obsname, obsindexes, True)
	pobs = ax.plot(date_idx, obs, color = 'orange', label = 'observed')
	
	best = best_sample(samplelist, lnproblist)
	cf.set_values(dataset, best, calibration)
	dataset.run_model()
	
	sim = dataset.get_result_series(simname, simindexes)
	
	psim = ax.plot(date_idx, sim, color = 'blue', label='best simulated')
	
	ax.legend()
	
	ax.set_xlabel('Date')
	ax.set_ylabel('%s $%s$' % (obsname, dataset.get_result_unit(simname)))
	
	fig.savefig(filename)


###############################################################################################

wr.initialize('simplyp.dll')

dataset = wr.DataSet.setup_from_parameter_and_input_files('../../Applications/SimplyP/Morsa/MorsaParameters.dat', '../../Applications/SimplyP/Morsa/MorsaInputs.dat')


if __name__ == '__main__': #NOTE: This line is needed, or something goes horribly wrong with the paralellization

	# List of simulated and observed variables to include in likelihood
	comparisons = [('Reach flow (daily mean, cumecs)', ['Kure'],               'Observed Q', []),
                   ('Reach TP concentration', ['Kure'],
				   'Observed TP at Kure', [])]
				   
	n_comparisons = len(comparisons)

	# Read in csv with parameters to vary, short names and param ranges
	fpath = r'C:\Data\GitHub\INCABuilder\PythonWrapper\SimplyP\SimplyP_calParams_ranges.csv'
	param_df = pd.read_csv(fpath)
	
	# Make calibration variable (list of (param, index) tuples to calibrate)
	calibration = [('Degree-day factor for snowmelt', []),
	 ('Proportion of precipitation that contributes to quick flow', []),
	 ('PET multiplication factor', []),
	 ('Soil field capacity', []),
	 ('Baseflow index', []),
	 ('Groundwater time constant', []),
	 ('Gradient of reach velocity-discharge relationship', []),
	 ('Soil water time constant', ['Semi-natural']),
	 ('Reach sediment input scaling factor', []),
	 ('Sediment input non-linear coefficient', []),
	 ('Groundwater TDP concentration', []),
	 ('Particulate P enrichment factor', []),
	 ('Reach effluent TDP inputs', ['Kure']),
	 ('Initial soil water TDP concentration and EPC0', ['Arable']),
	 ('M_Q', []),
	 ('M_TP', [])]
	
	# Extract inputs
	minval = param_df['Min']
	maxval = param_df['Max']
	labels_short = param_df['ShortName']
	
	labels_long  = ['%s, [%s]' % (cal[0], cal[1]) 
                    for cal in calibration[:-n_comparisons]]
	labels_long.append('M_Q')
	labels_long.append('M_TP')
	
	
	initial_guess = cf.default_initial_guess(dataset, calibration[:-n_comparisons])    #NOTE: This reads the initial guess that was provided by the parameter file, excluding any error term parameters

	# Initial guesses for the residual error terms
	initial_guess.append(0.5)
	initial_guess.append(0.05)
	
	skiptimesteps = 50   # Skip this many first timesteps in the objective evaluation
		
	objective = (cf.log_likelyhood, comparisons, skiptimesteps)
	
	
	# emcee setup params
	n_walk = 32
	n_steps = 200
	n_burn = 50

	samp, lnprob = run_emcee(minval, maxval, initial_guess, calibration, objective, n_walk=n_walk, n_steps=n_steps)
	
	chain_plot(samp, calibration, labels_long, "simplyp_plots\\chains.png")
	
	samplelist, lnproblist = reshape_samples(samp, lnprob, n_burn)
	
	triangle_plot(samplelist, labels_short, "simplyp_plots\\triangle_plot.png")
	
	plot_n_random_samples(dataset, samplelist, lnproblist, calibration, objective, 1000, "simplyp_plots\\random_samples.png")

	plt.show()
	
	
