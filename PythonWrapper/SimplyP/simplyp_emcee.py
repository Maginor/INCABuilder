

import matplotlib.pyplot as plt, seaborn as sn, emcee, corner, imp
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

def run_emcee(min, max, initial_guess, calibration, labels_short, objective, n_walk, n_steps, n_burn) :
	
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

	param_labels = ['%s [%s] (%s)' % (cal[0], ', '.join(cal[1]), dataset.get_parameter_unit(cal[0])) 
					for cal in calibration]
	param_labels.append('M')

	fig, axes = plt.subplots(nrows=n_dim, ncols=1, figsize=(10, 20))    
	for idx, title in enumerate(param_labels):        
		axes[idx].plot(sampler.chain[:,:,idx].T, '-', color='k', alpha=0.3)
		axes[idx].set_title(title, fontsize=16) 
	plt.subplots_adjust(hspace=0.7)    
	fig.savefig('simplyp_plots\\chains.png')

	samples = sampler.chain[:, n_burn:, :].reshape((-1, n_dim))

	# Triangle plot
	tri = corner.corner(samples,
						labels=labels_short,
						#truths=truths,
						quantiles=[0.025, 0.5, 0.975],
						show_titles=True, 
						title_args={'fontsize': 24},
						label_kwargs={'fontsize': 20})

	tri.savefig("simplyp_plots\\triangle_plot.png")
	
	pool.close()

	return samples	

	
	
wr.initialize('simplyp.dll')

dataset = wr.DataSet.setup_from_parameter_and_input_files('../../Applications/SimplyP/Tarland/TarlandParameters.dat', '../../Applications/SimplyP/Tarland/TarlandInputs.dat')

#NOTE: The 'calibration' structure is a list of (indexed) parameters that we want to calibrate
calibration = [
	('Proportion of precipitation that contributes to quick flow', []),
	('Baseflow index',                                             []),
	('Groundwater time constant',                                  []),
	('Gradient of stream velocity-discharge relationship',         []),
	('Exponent of stream velocity-discharge relationship',         []),
	('Soil water time constant',                                   ['Arable']),
	('Soil water time constant',                                   ['Semi-natural']),
	]

labels_short = [r'$f_{quick}$', r'$\beta$', r'$T_g$', r'$a$', r'$b$', r'$T_s[A]$', r'$T_s[S]$', r'M']

initial_guess = cf.default_initial_guess(dataset, calibration)    #NOTE: This reads the initial guess that was provided by the parameter file.
initial_guess.append(0.23)

min = [0.1 * x for x in initial_guess]
max = [10.0 * x for x in initial_guess]

cf.constrain_min_max(dataset, calibration, min, max) #NOTE: Constrain to the min and max values recommended by the model in case we made our bounds too wide.

skiptimesteps = 50   # Skip these many of the first timesteps in the objective evaluation

comparisons = [
	('Reach flow (daily mean, cumecs)', ['Tarland1'], 'observed Q', []),
	#Put more here!
	]

objective = (cf.log_likelyhood, comparisons, skiptimesteps)


if __name__ == '__main__': #NOTE: This line is needed, or something goes horribly wrong with the paralellization
	samples = run_emcee(min, max, initial_guess, calibration, labels_short, objective, n_walk=20, n_steps=10, n_burn=5)
	

	
	
