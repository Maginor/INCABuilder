import inca
import numpy as np
from scipy import optimize



#NOTE: Example for optimizing the a and b values in the equation Q = aV^b (reach flow - volume relationship)

dataset = inca.SetupModel('../IncaN/tovdalparametersPersistOnly.dat', '../IncaN/tovdalinputs.dat')

#NOTE: Example of how you can overwrite the timesteps and start date, in case you don't want to change the parameter file
#inca.SetParameterUInt(dataset, 'Timesteps', [], 1000)
#inca.SetParameterTime(dataset, 'Start date', [], '1999-12-7')

def sum_squares_error(params, obs, min, max):
	
	n = len(obs)
    
	a, b = params
	
	# NOTE: This version of the Nelder-Mead algorithm does not allow for bounds, so we have to hack them in
	if a < min[0] or a > max[0] or b < min[1] or b > max[1] :
		return np.inf
	
	#NOTE: If we use a parallellized optimizer we need to make a copy of the dataset to not have several threads overwrite each other.
	# (in that case, only use the copy when setting parameter values, running the model, and extracting results below)
	# datasetcopy = inca.CopyDataSet(dataset)
	
	inca.SetParameterDouble(dataset, 'a', ['Tveitvatn'], a)
	inca.SetParameterDouble(dataset, 'b', ['Tveitvatn'], b)
	
	inca.RunModel(dataset)
    
	sim = inca.GetResultSeries(dataset, 'Reach flow', ['Tveitvatn'])
    
	skiptimesteps = 365 #NOTE: Skip these many of the first timesteps when evaluating the objective
	
	sse = np.sum(((obs[skiptimesteps:] - sim[skiptimesteps:])**2))
    
	#NOTE: If we made a copy of the dataset we need to delete it so that we don't get a huge memory leak
	# inca.DeleteDataSet(datasetcopy)
	
	return sse

	
obs = inca.GetInputSeries(dataset, 'Discharge', ['Tveitvatn'], alignwithresults=True)

initial_guess = [.1, .2]
min = [.001, .1]
max = [.7, .9]

#res = optimize.minimize(sum_squares_error, initial_guess, args=(obs, min, max), method='Nelder-Mead')
#param_est = res.x

res = optimize.fmin(sum_squares_error, initial_guess, args=(obs, min, max))
param_est = res

print('\n')
print('Estimated a: %.2f.' % param_est[0])
print('Estimated b: %.2f.' % param_est[1])




