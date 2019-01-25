
import inca
import numpy as np
import time
from scipy.stats import norm
from inca_calibration import *

from multiprocessing import Pool

run_optimizer_first = False






def log_likelyhood(params, dataset, calibration, objective):	
	# NOTE: If we use a parallellized optimizer we need to make a copy of the dataset to not have several threads overwrite each other.
	# (in that case, only use the copy when setting parameter values, running the model, and extracting results below)
	datasetcopy = dataset.copy()
	
	set_values(datasetcopy, params, calibration)
	
	datasetcopy.run_model()
	
	fn, simname, simindexes, obsname, obsindexes, skiptimesteps = objective
    
	sim = datasetcopy.get_result_series(simname, simindexes)
	obs = datasetcopy.get_input_series(obsname, obsindexes, alignwithresults=True)
	
	sim2 = sim[skiptimesteps:]
	obs2 = obs[skiptimesteps:]
	
	M = params[len(calibration)]
	sigma_e = M*sim2
	
	likes = norm(sim2, sigma_e).logpdf(obs2)
	
	like = np.nansum(likes)
    
	# NOTE: If we made a copy of the dataset we need to delete it so that we don't get a huge memory leak
	datasetcopy.delete()
	
	#print ('single evaluation. result: %f' % like)
	
	return like	
	

	

inca.initialize('simplyp.dll')

dataset = None

if run_optimizer_first :
	dataset = inca.DataSet.setup_from_parameter_and_input_files('../Applications/SimplyP/tarlandparameters.dat', '../Applications/SimplyP/tarlandinputs.dat')
else :
	dataset = inca.DataSet.setup_from_parameter_and_input_files('optimal_parameters.dat', '../Applications/SimplyP/tarlandinputs.dat')

#NOTE: The 'calibration' structure is a list of (indexed) parameters that we want to calibrate
calibration = [
	('Proportion of precipitation that contributes to quick flow', ['Tarland1']),
	('Baseflow index',                                             ['Tarland1']),
	('Groundwater time constant',                                  ['Tarland1']),
	('Gradient of stream velocity-discharge relationship',         ['Tarland1']),
	('Exponent of stream velocity-discharge relationship',         ['Tarland1']),
	('Soil water time constant',                                   ['Arable']),
	('Soil water time constant',                                   ['Semi-natural']),
	]
	
labels_short = [r'$f_{quick}$', r'$\beta$', r'$T_g$', r'$a$', r'$b$', r'$T_s[A]$', r'$T_s[S]$']

initial_guess = default_initial_guess(dataset, calibration)    #NOTE: This reads the initial guess that was provided by the parameter file.
initial_guess.append(0.5)

min = [0.25 * x for x in initial_guess]
max = [4.0 * x for x in initial_guess]

constrain_min_max(dataset, calibration, min, max) #NOTE: Constrain to the min and max values recommended by the model in case we made our bounds too wide.

skiptimesteps = 50   # Skip these many of the first timesteps in the objective evaluation

objective = (log_likelyhood, 'Daily mean reach flow', ['Tarland1'], 'observed Q mm/d', [], skiptimesteps)

param_est = None
if run_optimizer_first :
	param_est = run_optimization(dataset, min, max, initial_guess, calibration, objective, minimize=False)

	print('\n')
	for idx, cal in enumerate(calibration) :
		name, indexes = cal
		print('Estimated %-60s %-20s %5.2f (range [%5.2f, %5.2f])' %  (name, ', '.join(indexes), param_est[idx], min[idx], max[idx]))
	if len(param_est) > len(calibration) :
		print('M: %f' % param_est[len(calibration)])

	# Computing the Hessian at the optimal point:
	hess = compute_hessian(dataset, param_est, calibration, objective)
	print('\nHessian matrix at optimal parameters:')
	print_matrix(hess)
	inv_hess = np.linalg.inv(hess)
	print('\nInverse Hessian:')
	print_matrix(inv_hess)
	
	# NOTE: Write the optimal values back to the dataset and then generate a new parameter file that has these values.
	set_values(dataset, param_est, calibration)
	dataset.write_parameters_to_file('optimal_parameters.dat')
else :
	param_est = initial_guess

	
def log_prior(params, min, max) :
	if check_min_max(params, min, max) :
		return 0
	return -np.inf

#IMPORTANT NOTE: If you want to run emcee multithreaded: The log_posterior function has to access the dataset as a global. It can not take it as an argument. This is because ctypes of pointer type can not be 'pickled',
#  which is what python does when it sends arguments to functions on separate threads. In that case, you will end up with a garbage value for the pointer.
def log_posterior(params, min, max, calibration, objective):
	log_pri = log_prior(params, min, max)
	
	llfun = objective[0]
	
	if(np.isfinite(log_pri)):
		log_like = llfun(params, dataset, calibration, objective)
		return log_pri + log_like
	return -np.inf

def run_emcee(min, max, initial_guess, calibration, labels_short, objective, n_walk, n_steps, n_burn) :
	
	ll, simname, simindexes, obsname, obsindexes, skiptimesteps = objective


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
	print('Time elapsed: %f' % (end - start))
	
	print('\nAverage acceptance rate: %f' % np.mean(sampler.acceptance_fraction))

	labels_short.append(r'M')

	param_labels = ['%s [%s] (%s)' % (cal[0], ', '.join(cal[1]), dataset.get_parameter_unit(cal[0])) 
					for cal in calibration]
	param_labels.append('M')

	fig, axes = plt.subplots(nrows=n_dim, ncols=1, figsize=(10, 20))    
	for idx, title in enumerate(param_labels):        
		axes[idx].plot(sampler.chain[:,:,idx].T, '-', color='k', alpha=0.3)
		axes[idx].set_title(title, fontsize=16) 
	plt.subplots_adjust(hspace=0.7)    
	fig.savefig('emcee_output\\chains.png')

	samples = sampler.chain[:, n_burn:, :].reshape((-1, n_dim))

	# Triangle plot
	tri = corner.corner(samples,
						labels=labels_short,
						#truths=truths,
						quantiles=[0.025, 0.5, 0.975],
						show_titles=True, 
						title_args={'fontsize': 24},
						label_kwargs={'fontsize': 20})

	tri.savefig("emcee_output\\triangle_plot.png")
	
	pool.close()

	return samples	
	

if __name__ == '__main__': #NOTE: This line is needed, or something goes horribly wrong with the paralellization
	samples = run_emcee(min, max, param_est, calibration, labels_short, objective, n_walk=20, n_steps=200, n_burn=100)


