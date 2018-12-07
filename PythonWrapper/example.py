import inca
import numpy as np
from scipy import optimize



#NOTE: Example for optimizing the a and b values in the equation Q = aV^b (reach flow - volume relationship)

dataset = inca.SetupModel('../IncaN/tovdalparametersPersistOnly.dat', '../IncaN/tovdalinputs.dat')

def sum_squares_error(params, obs, min, max):
	
	n = len(obs)
    
	a, b = params
	
	# NOTE: This version of the Nelder-Mead algorithm does not allow for bounds, so we have to hack them in
	if a < min[0] or a > max[0] or b < min[1] or b > max[1] :
		return np.inf
	
	#TODO: If we use a paralellized optimizer we need to make a copy of the dataset (using an exposed function from the dll) whenever we modify and run it to not have several threads overwrite each other.
	
	inca.SetParameterDouble(dataset, 'a', ['Tveitvatn'], a)
	inca.SetParameterDouble(dataset, 'b', ['Tveitvatn'], b)
	
	inca.RunModel(dataset)
    
	sim = inca.GetResultSeries(dataset, 'Reach flow', ['Tveitvatn'])
    
	skiptimesteps = 365 #NOTE: Skip these many of the first timesteps when evaluating the objective
	
	sse = np.sum(((obs[skiptimesteps:] - sim[skiptimesteps:])**2))
    
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




