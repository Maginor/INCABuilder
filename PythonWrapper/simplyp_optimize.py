
import inca
from inca_calibration import *
import numpy as np
	

inca.initialize('simplyp.dll')

dataset = inca.DataSet.setup_from_parameter_and_input_files('../Applications/SimplyP/tarlandparameters.dat', '../Applications/SimplyP/tarlandinputs.dat')

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
	
initial_guess = default_initial_guess(dataset, calibration)    #NOTE: This reads the initial guess that was provided by the parameter file.
initial_guess.append(0.5)

min = [0.1 * x for x in initial_guess]
max = [10.0 * x for x in initial_guess]

constrain_min_max(dataset, calibration, min, max) #NOTE: Constrain to the min and max values recommended by the model in case we made our bounds too wide.

skiptimesteps = 50   # Skip these many of the first timesteps in the objective evaluation

objective = (log_likelyhood, 'Reach flow (daily mean, cumecs)', ['Tarland1'], 'observed Q', [], skiptimesteps)


param_est = run_optimization(dataset, min, max, initial_guess, calibration, objective, minimize=False)
#param_est = param_est[0]

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

# NOTE: Run the model one more time with the optimal parameters to get the correct values in the dataset, the print goodness of fit and plot.
dataset.run_model()
print_goodness_of_fit(dataset, objective)
plot_objective(dataset, objective, "simplyp_plots\\optimizer_MAP.png")



