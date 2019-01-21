

#define MCMC_USE_OPENMP

#include "mcmc.hpp"
#include "de.cpp"
#include "rwmh.cpp"
#include "hmc.cpp"


#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>


#include <omp.h>


#include "../calibration.h"

enum mcmc_algorithm
{
	MCMCAlgorithm_DifferentialEvolution,
	MCMCAlgorithm_RandomWalkMetropolisHastings,
	MCMCAlgorithm_HamiltonianMonteCarlo,
};

struct mcmc_setup
{
	mcmc_algorithm Algorithm;
	
	size_t NumChains;
	size_t NumGenerations;   //Excluding burnin.
	size_t NumBurnin;
	size_t DiscardTimesteps; //Discard the N first timesteps.
	
	double DeBound;
	bool   DeJumps;
	double DeJumpGamma;
	
	double HMCStepSize;
	size_t HMCLeapSteps;
	
	std::vector<parameter_calibration> Calibration;
	
	std::vector<calibration_objective> Objectives;
	
};

struct mcmc_run_data
{
	std::vector<inca_data_set *> DataSets;
	
	std::vector<parameter_calibration> Calibration;
	
	calibration_objective Objective;
	
	size_t DiscardTimesteps;
};

struct mcmc_results
{
	double AcceptanceRate;
	arma::cube DrawsOut;
	arma::mat  DrawsOut2; //NOTE: For algs without multiple chains
};

static void
ReadMCMCSetupFromFile(mcmc_setup *Setup, const char *Filename)
{
	token_stream Stream(Filename);
	
	token *Token;
	
	while(true)
	{
		Token = Stream.PeekToken();
		
		if(Token->Type == TokenType_EOF)
		{
			break;
		}
		
		const char *Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);

		if(strcmp(Section, "algorithm") == 0)
		{
			const char *Alg = Stream.ExpectUnquotedString();
			if(strcmp(Alg, "differential_evolution") == 0)
			{
				Setup->Algorithm = MCMCAlgorithm_DifferentialEvolution;
			}
			else if(strcmp(Alg, "metropolis_hastings") == 0)
			{
				Setup->Algorithm = MCMCAlgorithm_RandomWalkMetropolisHastings;
			}
			else if(strcmp(Alg, "hamiltonian_monte_carlo") == 0)
			{
				Setup->Algorithm = MCMCAlgorithm_HamiltonianMonteCarlo;
			}
			//else if ...
			else
			{
				Stream.PrintErrorHeader();
				std::cout << "Unknown or unimplemented algorithm " << Alg << std::endl;
				exit(0);
			}
		}
		else if(strcmp(Section, "chains") == 0)
		{
			Setup->NumChains = (size_t)Stream.ExpectUInt();
		}
		else if(strcmp(Section, "generations") == 0)
		{
			Setup->NumGenerations = (size_t)Stream.ExpectUInt();
		}
		else if(strcmp(Section, "burnin") == 0)
		{
			Setup->NumBurnin = (size_t)Stream.ExpectUInt();
		}
		else if(strcmp(Section, "discard_timesteps") == 0)
		{
			Setup->DiscardTimesteps = (size_t)Stream.ExpectUInt();
		}
		else if(strcmp(Section, "de_b") == 0)
		{
			Setup->DeBound = Stream.ExpectDouble();
		}
		else if(strcmp(Section, "de_jumps") == 0)
		{
			Setup->DeJumps = Stream.ExpectBool();
		}
		else if(strcmp(Section, "de_jump_gamma") == 0)
		{
			Setup->DeJumpGamma = Stream.ExpectDouble();
		}
		else if(strcmp(Section, "hmc_step_size") == 0)
		{
			Setup->HMCStepSize = Stream.ExpectDouble();
		}
		else if(strcmp(Section, "hmc_leap_steps") == 0)
		{
			Setup->HMCLeapSteps = Stream.ExpectUInt();
		}
		else if(strcmp(Section, "parameter_calibration") == 0)
		{
			ReadParameterCalibration(Stream, Setup->Calibration, ParameterCalibrationReadInitialGuesses); //TODO: We should also read distributions here!
		}
		else if(strcmp(Section, "objectives") == 0)
		{
			ReadCalibrationObjectives(Stream, Setup->Objectives, false);
		}
		else
		{
			Stream.PrintErrorHeader();
			std::cout << "Unknown section name: " << Token->StringValue << std::endl;
			exit(0);
		}
	}
	
	//TODO: Check that the file contained enough data, i.e that most of the settings received sensible values?
}

double
TargetLogKernel(const arma::vec& Par, void* Data, size_t ChainIdx = 0)
{
	
	mcmc_run_data *RunData = (mcmc_run_data *)Data;
	
	inca_data_set *DataSet = RunData->DataSets[ChainIdx];
	
	double LogLikelyhood = EvaluateObjective(DataSet, RunData->Calibration, RunData->Objective, Par.memptr(), RunData->DiscardTimesteps);
	
	//TODO: When we have bounds turned on, it looks like de algorithm adds a log_jacobian for the priors. Find out what that is for!
	double LogPriors = 0.0; //NOTE: This assumes uniformly distributed priors and that the MCMC driving algorithm discards draws outside the parameter min-max boundaries on its own.
	//TODO: implement other priors.
	
	return LogPriors + LogLikelyhood;
}

double
TargetLogKernelWithGradient(const arma::vec &Par, arma::vec* GradientOut, void *Data, size_t ChainIdx)
{
	mcmc_run_data *RunData = (mcmc_run_data *)Data;
	
	inca_data_set *DataSet = RunData->DataSets[ChainIdx];

	double LogLikelyhood;
	if(GradientOut)
	{
		LogLikelyhood = EvaluateObjectiveAndGradientSingleForwardDifference(DataSet, RunData->Calibration, RunData->Objective, Par.memptr(), RunData->DiscardTimesteps, GradientOut->memptr());
	}
	else
	{
		LogLikelyhood = EvaluateObjective(DataSet, RunData->Calibration, RunData->Objective, Par.memptr(), RunData->DiscardTimesteps);
	}
	
	double LogPriors = 0.0;  //NOTE: See above!
	
	return LogPriors + LogLikelyhood;
}


static void RunMCMC(inca_data_set *DataSet, mcmc_setup *Setup, mcmc_results *Results)
{
	mcmc_run_data RunData = {};
	
	RunData.Calibration = Setup->Calibration;
	
	size_t Dimensions = RunData.Calibration.size();
	
	if(Dimensions == 0)
	{
		std::cout << "ERROR: (MCMC) Need at least one parameter to calibrate." << std::endl;
		exit(0);
	}
	
	arma::vec InitialGuess(Dimensions + 1);
	arma::vec LowerBounds(Dimensions + 1);
	arma::vec UpperBounds(Dimensions + 1);
	
	for(size_t CalIdx = 0; CalIdx < Dimensions; ++CalIdx)
	{
		parameter_calibration &Cal = RunData.Calibration[CalIdx];
		InitialGuess[CalIdx] = Cal.InitialGuess;
		LowerBounds [CalIdx] = Cal.Min;
		UpperBounds [CalIdx] = Cal.Max;
	}
	
	//NOTE: The final parameter is the parameter for random perturbation.
	//TODO: Don't hard code these, they should be in the setup file!
	InitialGuess[Dimensions] = 0.5;
	LowerBounds [Dimensions] = 0.0;
	UpperBounds [Dimensions] = 1.0;
	
	mcmc::algo_settings_t Settings;
	
	Settings.vals_bound = true;				//Alternatively we could actually allow people to not have bounds here...
	Settings.lower_bounds = LowerBounds;
	Settings.upper_bounds = UpperBounds;
	
	Setup->NumChains = 1;
	
	if(Setup->Algorithm == MCMCAlgorithm_DifferentialEvolution)
	{
		Settings.de_n_pop = Setup->NumChains;
		Settings.de_n_gen = Setup->NumGenerations;
		Settings.de_n_burnin = Setup->NumBurnin;
		
		Settings.de_initial_lb = LowerBounds;
		Settings.de_initial_ub = UpperBounds;

		Settings.de_par_b = Setup->DeBound;
		
		Settings.de_jumps = Setup->DeJumps;
		Settings.de_par_gamma_jump = Setup->DeJumpGamma;
	}
	else if(Setup->Algorithm == MCMCAlgorithm_RandomWalkMetropolisHastings)
	{
		Settings.rwmh_n_draws = Setup->NumGenerations;
		Settings.rwmh_n_burnin = Setup->NumBurnin;
		Settings.rwmh_par_scale = 1.0;                 //TODO: See if we need to do anything with this.
		//arma::mat rwmh_cov_mat;
		
		//Ouch, rwmh does not support multiple chains, and so no parallelisation...
		if(Setup->NumChains > 1)
		{
			std::cout << "WARNING (MCMC): Random Walk Metropolis Hastings does not support having more than one chain." << std::endl;
		}
		Setup->NumChains = 1; // :(
	}
	else if (Setup->Algorithm = MCMCAlgorithm_HamiltonianMonteCarlo)
	{
		Settings.hmc_n_draws = Setup->NumGenerations;
		Settings.hmc_step_size = Setup->HMCStepSize;
		Settings.hmc_leap_steps = Setup->HMCLeapSteps;
		Settings.hmc_n_burnin = Setup->NumBurnin;
		//hmc_precond_mat
		
		if(Setup->NumChains > 1)
		{
			std::cout << "WARNING (MCMC): Hamiltonian Monte Carlo does not support having more than one chain." << std::endl;
		}
		Setup->NumChains = 1;
	}
	
	//NOTE: Make one copy of the dataset for each chain (so that they don't overwrite each other).
	RunData.DataSets.resize(Setup->NumChains);
	RunData.DataSets[0] = DataSet;
	for(size_t ChainIdx = 1; ChainIdx < Setup->NumChains; ++ChainIdx)
	{
		RunData.DataSets[ChainIdx] = CopyDataSet(DataSet);
	}
	
	if(Setup->Objectives.size() != 1)
	{
		std::cout << "ERROR: (MCMC) We currently support having only one objective." << std::endl;
		exit(0);
	}
	
	RunData.Objective = Setup->Objectives[0];
	
	if(!IsLogLikelyhoodMeasure(RunData.Objective.PerformanceMeasure))
	{
		std::cout << "ERROR: (MCMC) A performance measure was selected that was not a log likelyhood measure." << std::endl;
		exit(0);
	}
	
	RunData.DiscardTimesteps = Setup->DiscardTimesteps;
	
	u64 Timesteps = GetTimesteps(DataSet);
	if(RunData.DiscardTimesteps >= Timesteps)
	{
		std::cout << "ERROR: (MCMC) We are told to discard the first " << RunData.DiscardTimesteps << " timesteps when evaluating the objective, but we only run the model for " << Timesteps << " timesteps." << std::endl;
		exit(0);
	}
	
	if(Setup->NumChains > 1)
	{
		omp_set_num_threads(Setup->NumChains);
	}
	
	if(Setup->Algorithm == MCMCAlgorithm_DifferentialEvolution)
	{
		mcmc::de(InitialGuess, Results->DrawsOut, TargetLogKernel, &RunData, Settings); //NOTE: we had to make a modification to the library so that it passes the Chain index to the TargetLogKernel.
	
		Results->AcceptanceRate = Settings.de_accept_rate;
	}
	else if(Setup->Algorithm == MCMCAlgorithm_RandomWalkMetropolisHastings)
	{
		mcmc::rwmh(InitialGuess, Results->DrawsOut2, 
		[](const arma::vec& CalibrationIn, void* Data){return TargetLogKernel(CalibrationIn, Data, 0);}, 
		&RunData, Settings);
		
		Results->AcceptanceRate = Settings.rwmh_accept_rate;
	}
	else if(Setup->Algorithm = MCMCAlgorithm_HamiltonianMonteCarlo)
	{
		mcmc::hmc(InitialGuess, Results->DrawsOut2,
		[](const arma::vec& CalibrationIn, arma::vec* GradOut, void* Data){return TargetLogKernelWithGradient(CalibrationIn, GradOut, Data, 0);},
		&RunData, Settings);
		
		Results->AcceptanceRate = Settings.hmc_accept_rate;
	}
	
	
	//NOTE: We delete every DataSet that we allocated above. We don't delete the one that was passed in since the caller may want to keep it.
	for(size_t ChainIdx = 1; ChainIdx < Setup->NumChains; ++ChainIdx)
	{
		delete RunData.DataSets[ChainIdx];
	}
}