import inca
import numpy as np
from scipy import optimize
import pandas as pd
import datetime as dt



# NOTE: Example for optimizing the a and b values in the equation V = aQ^b (reach flow - velocity relationship)
# This is just a toy setup, a proper calibration run probably needs to set up more parameters to modify and maybe use a better algorithm.

dataset = inca.DataSet.setup_from_parameter_and_input_files('../IncaN/tovdalparametersPersistOnly.dat', '../IncaN/tovdalinputs.dat')

# NOTE: Example of how you can override the timesteps and start date, in case you don't want to change them in the parameter file
# dataset.set_parameter_uint('Timesteps', [], 1000)
# dataset.set_parameter_time('Start date', [], '1999-12-7')

def plot_flow_vs_discharge(ds):
	# NOTE: This function has not been tested yet..

	discharge = ds.get_input_series('Discharge', ['Tveitvatn'], alignwithresults=True)
	flow      = ds.get_result_series('Reach flow', ['Tveitvatn'])
	start_date = dt.datetime.strptime(ds.get_parameter_time('Start date', []),'%Y-%m-%d')
	timesteps = ds.get_parameter_uint('Timesteps', [])
	
	date_idx = np.array(pd.date_range(start_date, periods=timesteps))
	df = pd.DataFrame({'date' : date_idx, 'Discharge (obs)' :  discharge, 'flow (sim)' : flow })
	df.set_index('date',inplace=True)

	ax = df.plot(subplots=True)


def sum_squares_error(params, obs, min, max):
	a, b = params
	
	# NOTE: This version of the Nelder-Mead algorithm does not allow for bounds, so we have to hack them in
	if a < min[0] or a > max[0] or b < min[1] or b > max[1] :
		return np.inf
	
	# NOTE: If we use a parallellized optimizer we need to make a copy of the dataset to not have several threads overwrite each other.
	# (in that case, only use the copy when setting parameter values, running the model, and extracting results below)
	# datasetcopy = dataset.copy()
	
	dataset.set_parameter_double('a', ['Tveitvatn'], a)
	dataset.set_parameter_double('b', ['Tveitvatn'], b)
	
	dataset.run_model()
    
	sim = dataset.get_result_series('Reach flow', ['Tveitvatn'])
    
	skiptimesteps = 365 #NOTE: Skip these many of the first timesteps when evaluating the objective
	
	sse = np.sum((obs[skiptimesteps:] - sim[skiptimesteps:])**2)
    
	# NOTE: If we made a copy of the dataset we need to delete it so that we don't get a huge memory leak
	# datasetcopy.delete()
	
	return sse

	
	

	
obs = dataset.get_input_series('Discharge', ['Tveitvatn'], alignwithresults=True)

initial_guess = [dataset.get_parameter_double('a', ['Tveitvatn']), dataset.get_parameter_double('b', ['Tveitvatn'])]
min = [.001, .1]
max = [.7, .9]

# res = optimize.minimize(sum_squares_error, initial_guess, args=(obs, min, max), method='Nelder-Mead')
# param_est = res.x

res = optimize.fmin(sum_squares_error, initial_guess, args=(obs, min, max))
param_est = res

print('\n')
print('Estimated a: %.2f.' % param_est[0])
print('Estimated b: %.2f.' % param_est[1])

# NOTE: Write the optimal values back to the dataset (the values that end up in the dataset may not be the optimal values, depending on the algorithm)
dataset.set_parameter_double('a', ['Tveitvatn'], param_est[0])
dataset.set_parameter_double('b', ['Tveitvatn'], param_est[1])

dataset.write_parameters_to_file('optimal_parameters.dat')

dataset.delete() # You can optionally do this at the end. It is not necessary if your python process closes down, because then the operating system will do it for you. Also, this will not delete the model itself, only the dataset (I have not exposed any functionality for deleting the model atm.)



