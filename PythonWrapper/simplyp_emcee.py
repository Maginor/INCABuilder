

import matplotlib.pyplot as plt, seaborn as sn, emcee, corner, mpld3
import inca
from inca_calibration import *
import numpy as np
import time

from multiprocessing import Pool


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

	
	
inca.initialize('simplyp.dll')

dataset = inca.DataSet.setup_from_parameter_and_input_files('optimal_parameters.dat', '../Applications/SimplyP/tarlandinputs.dat')

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

initial_guess = default_initial_guess(dataset, calibration)    #NOTE: This reads the initial guess that was provided by the parameter file.
initial_guess.append(0.23)

min = [0.1 * x for x in initial_guess]
max = [10.0 * x for x in initial_guess]

constrain_min_max(dataset, calibration, min, max) #NOTE: Constrain to the min and max values recommended by the model in case we made our bounds too wide.

skiptimesteps = 50   # Skip these many of the first timesteps in the objective evaluation

objective = (log_likelyhood, 'Reach flow (daily mean)', ['Tarland1'], 'observed Q mm/d', [], skiptimesteps)

if __name__ == '__main__': #NOTE: This line is needed, or something goes horribly wrong with the paralellization
	samples = run_emcee(min, max, initial_guess, calibration, labels_short, objective, n_walk=20, n_steps=20000, n_burn=1000)
	
