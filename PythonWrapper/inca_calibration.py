from scipy import optimize
import numpy as np
import numdifftools as nd

def set_values(dataset, values, calibration):
	#TODO: Allow for linking parameters across indexes (say you want the Time constant for soil water to be the same across all landscape units)
	for idx, cal in enumerate(calibration):
		parname, parindexes = cal
		dataset.set_parameter_double(parname, parindexes, values[idx])
		
def check_min_max(params, min, max):
	for idx, value in enumerate(params):
		if(value < min[idx] or value > max[idx]):
			return False
	return True

def run_optimization(dataset, min, max, initial_guess, calibration, objective) :
	objective_fun, simname, simindexes, obsname, obsindexes, skiptimesteps = objective
	
	#NOTE: We extract the observation series just once here so that it does not have to be extracted at every evaluation (it will not change during optimization)
	obsseries = dataset.get_input_series(obsname, obsindexes, alignwithresults=True)

	if skiptimesteps >= dataset.get_parameter_uint('Timesteps', []) :
		raise ValueError('We were told to skip more timesteps in the evaluation of the objective than the amount of timesteps we run the model for.')
	
	def eval(params) :
		# NOTE: This version of the Nelder-Mead algorithm does not allow for bounds, so we have to hack them in
		if not check_min_max(params, min, max):
			return np.inf
		return objective_fun(params, dataset, calibration, objective, obsseries)
	
	return optimize.fmin(eval, initial_guess, maxfun=10000)

	
def compute_hessian(dataset, params, calibration, objective) :
	objective_fun, simname, simindexes, obsname, obsindexes, skiptimesteps = objective
	obsseries = dataset.get_input_series(obsname, obsindexes, alignwithresults=True)
	
	#WARNING: This may be dangerous if we are close to the parameter bounds. The algorithm may evaluate outside the bounds and maybe crash the model.
	def eval(par) : return objective_fun(par, dataset, calibration, objective, obsseries)
	
	return nd.Hessian(eval)(params)
	
def default_initial_guess(dataset, calibration) :
	#NOTE: Just reads the values that were provided in the file
	return [dataset.get_parameter_double(cal[0], cal[1]) for cal in calibration]