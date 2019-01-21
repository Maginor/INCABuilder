
import inca
import numpy as np
from scipy.stats import norm
from inca_calibration import *


def log_likelyhood(params, dataset, calibration, objective, obs):	
	# NOTE: If we use a parallellized optimizer we need to make a copy of the dataset to not have several threads overwrite each other.
	# (in that case, only use the copy when setting parameter values, running the model, and extracting results below)
	# datasetcopy = dataset.copy()
	
	set_values(dataset, params, calibration)
	
	dataset.run_model()
	
	fn, simname, simindexes, obsname, obsindexes, skiptimesteps = objective
    
	sim = dataset.get_result_series(simname, simindexes)
	sim2 = sim[skiptimesteps:]
	obs2 = obs[skiptimesteps:]
	
	#TODO: sigma_e = M*sim2
	sigma_e = sim2
	
	likes = norm(sim2, sigma_e).logpdf(obs2)
	
	like = np.nansum(likes)
    
	# NOTE: If we made a copy of the dataset we need to delete it so that we don't get a huge memory leak
	# datasetcopy.delete()
	
	#print ('single evaluation. result: %f' % like)
	
	return like
	
def neg_likelyhood(params, dataset, calibration, objective, obs) :
	return -log_likelyhood(params, dataset, calibration, objective, obs)



inca.initialize('simplyp.dll')

dataset = inca.DataSet.setup_from_parameter_and_input_files('../Applications/SimplyP/tarlandparameters.dat', '../Applications/SimplyP/tarlandinputs.dat')


#NOTE: The 'calibration' structure is a tuple of (indexed) parameters that we want to calibrate
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

min = [0.5 * x for x in initial_guess]
max = [2.0 * x for x in initial_guess]

skiptimesteps = 50   # Skip these many of the first timesteps in the objective evaluation
objective = (neg_likelyhood, 'Reach flow', ['Tarland1'], 'observed Q', [], skiptimesteps)

param_est = run_optimization(dataset, min, max, initial_guess, calibration, objective)

print('\n')
for idx, cal in enumerate(calibration) :
	name, indexes = cal
	print('Estimated %s: %.2f.' %  (name, param_est[idx]))

	
# Computing the Hessian at the optimal point:
hess = compute_hessian(dataset, param_est, calibration, objective)
print('Hessian matrix at optimal parameters:')
print(hess)


# NOTE: Write the optimal values back to the dataset and then generate a new parameter file that has these values.
set_values(dataset, param_est, calibration)
dataset.write_parameters_to_file('optimal_parameters.dat')



