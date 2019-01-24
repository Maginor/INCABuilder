
import inca
import numpy as np
from scipy.stats import norm
from inca_calibration import *


run_optimizer_first = False

def log_likelyhood(params, dataset, calibration, objective, obs):	
	# NOTE: If we use a parallellized optimizer we need to make a copy of the dataset to not have several threads overwrite each other.
	# (in that case, only use the copy when setting parameter values, running the model, and extracting results below)
	datasetcopy = dataset.copy()
	
	set_values(datasetcopy, params, calibration)
	
	datasetcopy.run_model()
	
	fn, simname, simindexes, obsname, obsindexes, skiptimesteps = objective
    
	sim = datasetcopy.get_result_series(simname, simindexes)
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
	
	
run_emcee(dataset, min, max, param_est, calibration, objective, n_walk=20, n_steps=200, n_burn=100)



