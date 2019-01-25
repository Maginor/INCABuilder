
from scipy import optimize
import numpy as np
import numdifftools as nd
from scipy.stats import norm

#from emcee.utils import MPIPool

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

def run_optimization(dataset, min, max, initial_guess, calibration, objective, minimize=True) :
	objective_fun = objective[0]

	#if skiptimesteps >= dataset.get_parameter_uint('Timesteps', []) :
	#	raise ValueError('We were told to skip more timesteps in the evaluation of the objective than the amount of timesteps we run the model for.')
	
	if minimize :
		def eval(params) :
			# NOTE: This version of the Nelder-Mead algorithm does not allow for bounds, so we have to hack them in
			if not check_min_max(params, min, max):
				return np.inf
			return objective_fun(params, dataset, calibration, objective) 
		
		return optimize.fmin(eval, initial_guess, maxfun=10000)
	else :
		def eval(params) :
			if not check_min_max(params, min, max):
				return np.inf
			return -objective_fun(params, dataset, calibration, objective) 
		
		return optimize.fmin(eval, initial_guess, maxfun=10000)

def compute_hessian(dataset, params, calibration, objective) :
	objective_fun = objective[0]
	
	def eval(par) : return objective_fun(par, dataset, calibration, objective)
	
	#WARNING: The estimation seems to be extremely sensitive to the step size! How to choose the best one?
	#steps = [1e-4 * x for x in params]
	steps = 1e-3
	return nd.Hessian(eval, step=steps)(params)
	
def print_matrix(matrix) :
	s = [['%9.4f' % e for e in row] for row in matrix]
	lens = [max(map(len, col)) for col in zip(*s)]
	fmt = '\t'.join('{{:{}}}'.format(x) for x in lens)
	table = [fmt.format(*row) for row in s]
	print('\n'.join(table))
	
def default_initial_guess(dataset, calibration) :
	#NOTE: Just reads the values that were provided in the file
	return [dataset.get_parameter_double(cal[0], cal[1]) for cal in calibration]
	
def constrain_min_max(dataset, calibration, minvec, maxvec) :
	'''
		Constrain the min and max values in the provided vectors to the recommended min and max values set for these parameters by the model.
	'''
	for idx, cal in enumerate(calibration) :
		min, max = dataset.get_parameter_double_min_max(cal[0])
		if minvec[idx] < min : minvec[idx] = min
		if maxvec[idx] > max : maxvec[idx] = max

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
		
		
	