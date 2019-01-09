import inca
import numpy as np
from scipy import optimize
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

def sum_squares_error(params, dataset, calibration, objective, obs):	
	# NOTE: If we use a parallellized optimizer we need to make a copy of the dataset to not have several threads overwrite each other.
	# (in that case, only use the copy when setting parameter values, running the model, and extracting results below)
	# datasetcopy = dataset.copy()
	
	set_values(dataset, params, calibration)
	
	dataset.run_model()
	
	fn, simname, simindexes, obsname, obsindexes, skiptimesteps = objective
    
	sim = dataset.get_result_series(simname, simindexes)
	
	sse = np.sum((obs[skiptimesteps:] - sim[skiptimesteps:])**2)
    
	# NOTE: If we made a copy of the dataset we need to delete it so that we don't get a huge memory leak
	# datasetcopy.delete()
	
	return sse

def run_optimization(dataset, min, max, initial_guess, calibration, objective) :
	objective_fun, simname, simindexes, obsname, obsindexes, skiptimesteps = objective
	
	#NOTE: We extract the observation series just once here so that it does not have to be extracted at every evaluation (it will not change during optimization)
	obsseries = dataset.get_input_series(obsname, obsindexes)

	if skiptimesteps >= dataset.get_parameter_uint('Timesteps', []) :
		raise ValueError('We were told to skip more timesteps in the evaluation of the objective than the amount of timesteps we run the model for.')
	
	def eval(params) :
		# NOTE: This version of the Nelder-Mead algorithm does not allow for bounds, so we have to hack them in
		if not check_min_max(params, min, max):
			return np.inf
		return objective_fun(params, dataset, calibration, objective, obsseries)
	
	return optimize.fmin(eval, initial_guess)

	
def compute_hessian(dataset, params, calibration, objective) :
	objective_fun, simname, simindexes, obsname, obsindexes, skiptimesteps = objective
	obsseries = dataset.get_input_series(obsname, obsindexes)
	
	#WARNING: This may be dangerous if we are close to the parameter bounds. The algorithm may evaluate outside the bounds and maybe crash the model.
	def eval(par) : return objective_fun(par, dataset, calibration, objective, obsseries)
	
	return nd.Hessian(eval)(params)
	
def default_initial_guess(dataset, calibration) :
	#NOTE: Just reads the values that were provided in the file
	return [dataset.get_parameter_double(cal[0], cal[1]) for cal in calibration]

	

# NOTE: Example for optimizing the a and b values in the equation V = aQ^b (reach flow - velocity relationship)
# This is just a toy setup, a proper calibration run probably needs to set up more parameters to modify and maybe use a better algorithm, such as the one in dlib.

#TODO: The reach flow doesn't seem to be very sensitive to these parameters in particular, choose some better ones to illustrate this example!

dataset = inca.DataSet.setup_from_parameter_and_input_files('../Applications/IncaN/tovdalparametersPersistOnly.dat', '../Applications/IncaN/tovdalinputs.dat')

# NOTE: Example of how you can override the timesteps and start date, in case you don't want to change them in the parameter file
# dataset.set_parameter_uint('Timesteps', [], 1000)
# dataset.set_parameter_time('Start date', [], '1999-12-7')


#NOTE: The 'calibration' structure is a tuple of (indexed) parameters that we want to calibrate
calibration = [
	('a', ['Tveitvatn']),
	('b', ['Tveitvatn']),
	]

#initial_guess = default_initial_guess(dataset, calibration)    #--- oops, this starting point is not good for this setup
initial_guess = [0.5, 0.7]
min = [.001, .1]
max = [.7, .9]

skiptimesteps = 365   # Skip these many of the first timesteps in the objective evaluation

#NOTE: The 'objective' structure contains information about how to evaluate the objective.
objective = (sum_squares_error, 'Reach flow', ['Tveitvatn'], 'Discharge', ['Tveitvatn'], skiptimesteps)

#NOTE: We test the optimizer by running the model with "fake real parameters" and set that as the observation series to see if the optimizer can recover the "real" parameters.
fake_real_parameters = [0.41, 0.6]
set_values(dataset, fake_real_parameters, calibration)

dataset.run_model()
fake_discharge = dataset.get_result_series(objective[1], objective[2])
dataset.set_input_series(objective[3], objective[4], fake_discharge)    # Overwrites the existing input series with our result series as the fake real input.
	
param_est = run_optimization(dataset, min, max, initial_guess, calibration, objective)


print('\n')
for idx, cal in enumerate(calibration) :
	name, indexes = cal
	print('Fake real %s: %.2f, Estimated %s: %.2f.' % (name, fake_real_parameters[idx], name, param_est[idx]))

	
# Computing the Hessian at the optimal point:
hess = compute_hessian(dataset, param_est, calibration, objective)
print('Hessian matrix at optimal parameters:')
print(hess)


# NOTE: If you do a real optimization run, this is how you can write the optimal values back to the dataset and then generate a new parameter file that has these values.
# set_values(dataset, param_est, calibration)
# dataset.write_parameters_to_file('optimal_parameters.dat')


dataset.delete() # You can optionally do this at the end. It is not necessary if your python process closes down, because then the operating system will do it for you. Also, this will not delete the model itself, only the dataset (I have not exposed any functionality for deleting the model atm.)


