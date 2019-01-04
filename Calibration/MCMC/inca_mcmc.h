

#define MCMC_USE_OPENMP

#include "mcmc.hpp"
#include "de.cpp"
#include "rwmh.cpp"


#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/sum.hpp>


#include <omp.h>


#include "../calibration_setup.h"

enum mcmc_algorithm
{
	MCMCAlgorithm_DifferentialEvolution,
	MCMCAlgorithm_RandomWalkMetropolisHastings,
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
	
	std::vector<parameter_calibration> Calibration;
	
	const char *ResultName;
	std::vector<const char *> ResultIndexes;
	
	const char *ObservedName;
	std::vector<const char *> ObservedIndexes;
	
};

struct mcmc_run_data
{
	std::vector<inca_data_set *> DataSets;
	
	std::vector<parameter_calibration> Calibration;
	
	const char *ResultName;
	std::vector<const char *> ResultIndexes;
	
	std::vector<double> Observations;
	
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
	
	int Mode = -1;
	
	bool StartLink = false;
	
	while(true)
	{
		Token = Stream.ReadToken();
		
		if(Token->Type == TokenType_EOF)
		{
			break;
		}
		if(Token->Type == TokenType_UnquotedString)
		{
			if(strcmp(Token->StringValue, "algorithm") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				Token = Stream.ExpectToken(TokenType_UnquotedString);
				if(strcmp(Token->StringValue, "differential_evolution") == 0)
				{
					Setup->Algorithm = MCMCAlgorithm_DifferentialEvolution;
				}
				else if(strcmp(Token->StringValue, "metropolis_hastings") == 0)
				{
					Setup->Algorithm = MCMCAlgorithm_RandomWalkMetropolisHastings;
				}
				//else if ...
				else
				{
					Stream.PrintErrorHeader();
					std::cout << "Unknown or unimplemented algorithm " << Token->StringValue << std::endl;
					exit(0);
				}
			}
			else if(strcmp(Token->StringValue, "chains") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				Setup->NumChains = (size_t)Stream.ExpectUInt();
			}
			else if(strcmp(Token->StringValue, "generations") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				Setup->NumGenerations = (size_t)Stream.ExpectUInt();
			}
			else if(strcmp(Token->StringValue, "burnin") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				Setup->NumBurnin = (size_t)Stream.ExpectUInt();
			}
			else if(strcmp(Token->StringValue, "discard_timesteps") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				Setup->DiscardTimesteps = (size_t)Stream.ExpectUInt();
			}
			else if(strcmp(Token->StringValue, "de_b") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				Setup->DeBound = Stream.ExpectDouble();
			}
			else if(strcmp(Token->StringValue, "de_jumps") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				Setup->DeJumps = Stream.ExpectBool();
			}
			else if(strcmp(Token->StringValue, "de_jump_gamma") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				Setup->DeJumpGamma = Stream.ExpectDouble();
			}
			else if(strcmp(Token->StringValue, "parameter_calibration") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				
				ReadParameterCalibration(Stream, Setup->Calibration, ParameterCalibrationReadInitialGuesses); //TODO: We should also read distributions here!
			}
			else if(strcmp(Token->StringValue, "objective") == 0)
			{
				Stream.ExpectToken(TokenType_Colon);
				Mode = 1;
			}
			else
			{
				Stream.PrintErrorHeader();
				std::cout << "Unknown command word " << Token->StringValue << std::endl;
				exit(0);
			}
		}
		else if(Mode == 1)
		{
			if(Token->Type != TokenType_QuotedString)
			{
				Stream.PrintErrorHeader();
				std::cout << "Expected the quoted name of a result series." << std::endl;
				exit(0);
			}
			Setup->ResultName = CopyString(Token->StringValue); //NOTE: Leaks!
			Stream.ExpectToken(TokenType_OpenBrace);
			while(true)
			{
				Token = Stream.ReadToken();
				if(Token->Type == TokenType_CloseBrace)
				{
					break;
				}
				else if(Token->Type == TokenType_QuotedString)
				{
					Setup->ResultIndexes.push_back(CopyString(Token->StringValue)); //NOTE: Leaks!
				}
				else
				{
					Stream.PrintErrorHeader();
					std::cout << "Expected the quoted name of an index" << std::endl;
					exit(0);
				}
			}
			Token = Stream.ExpectToken(TokenType_QuotedString);
			Setup->ObservedName = CopyString(Token->StringValue); //NOTE: Leaks!
			Stream.ExpectToken(TokenType_OpenBrace);
			while(true)
			{
				Token = Stream.ReadToken();
				if(Token->Type == TokenType_CloseBrace)
				{
					break;
				}
				else if(Token->Type == TokenType_QuotedString)
				{
					Setup->ObservedIndexes.push_back(CopyString(Token->StringValue)); //NOTE: Leaks!
				}
				else
				{
					Stream.PrintErrorHeader();
					std::cout << "Expected the quoted name of an index" << std::endl;
					exit(0);
				}
			}
		}
		else
		{
			Stream.PrintErrorHeader();
			std::cout << "Unexpected token." << std::endl;
			exit(0);
		}
	}
	
	//TODO: Check that the file contained enough data?
}

double
TargetLogKernel(const arma::vec& CalibrationIn, void* Data, size_t ChainIdx = 0)
{
	mcmc_run_data *RunData = (mcmc_run_data *)Data;
	
	inca_data_set *DataSet = RunData->DataSets[ChainIdx];
	
	size_t Dimensions = RunData->Calibration.size();
	
	//Write the new parameter set sample to the dataset
	for(size_t CalIdx = 0; CalIdx < Dimensions; ++CalIdx)
	{
		double ParamVal = CalibrationIn[CalIdx];
		parameter_calibration &Cal = RunData->Calibration[CalIdx];
		
		SetCalibrationValue(DataSet, Cal, ParamVal);
	}
	
	double M = CalibrationIn[Dimensions]; //NOTE: The last parameter estimates the random perturbation.
	
	timer RunModelTimer = BeginTimer();
	RunModel(DataSet);
	
	std::cout << "Time to run a single model: " << GetTimerMilliseconds(&RunModelTimer) << " milliseconds." << std::endl;
	
	u64 Timesteps = DataSet->TimestepsLastRun;
	
	std::vector<double> Simulated(Timesteps);
	std::vector<double> &Observations = RunData->Observations;
	
	//NOTE: This copies the result series into a vector. This could be made faster if we made some iterator scheme for the dataset that lets us read out the data directly.
	GetResultSeries(DataSet, RunData->ResultName, RunData->ResultIndexes, Simulated.data(), Simulated.size());
	
	//Evaluate log likelyhood of simulated vs. observed.
	//NOTE: This is the log likelyhood described here: http://nbviewer.jupyter.org/github/JamesSample/enviro_mod_notes/blob/master/notebooks/06_Beyond_Metropolis.ipynb	
	
	using namespace boost::accumulators;
	accumulator_set<double, stats<tag::sum>> LogLikelyhoodAccum;
	
	double LogLikelyhood = 0.0; //TODO: Use boost::accumulator for summing since that is more accurate.
	for(size_t Timestep = RunData->DiscardTimesteps; Timestep < Timesteps; ++Timestep)
	{
		double Obs = Observations[Timestep];
		double Sim = Simulated[Timestep];
		if(!std::isnan(Obs) && !std::isnan(Sim)) //TODO: We should do something else upon NaN in the sim (return -inf?). NaN in obs just means that we don't have a value for that timestep, and so we skip it.
		{
			double Sigma = M*Sim;
			double Like = -0.5*std::log(2.0*Pi*Sigma*Sigma) - (Obs - Sim)*(Obs - Sim) / (2*Sigma*Sigma);
			LogLikelyhoodAccum(Like);
		}
	}
	
	//TODO: When we have bounds turned on, it looks like de returns adds a log_jacobian for the priors. Find out what that is for!
	double LogPriors = 0.0; //NOTE: This assumes normal distributed priors and that the MCMC driving algorithm discards draws outside the parameter min-max boundaries on its own.
	//TODO: implement other priors.
	
	return LogPriors + sum(LogLikelyhoodAccum);
	//Do logarithm stuff (and random perturbations?).
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
	//TODO: Don't hard code these:
	InitialGuess[Dimensions] = 0.5;
	LowerBounds [Dimensions] = 0.0;
	UpperBounds [Dimensions] = 1.0;
	
	mcmc::algo_settings_t Settings;
	
	Settings.vals_bound = true;				//Alternatively we could actually allow people to not have bounds here...
	Settings.lower_bounds = LowerBounds;
	Settings.upper_bounds = UpperBounds;
	
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
	
	//NOTE: Make one copy of the dataset for each chain (so that they don't overwrite each other).
	RunData.DataSets.resize(Setup->NumChains);
	RunData.DataSets[0] = DataSet;
	for(size_t ChainIdx = 1; ChainIdx < Setup->NumChains; ++ChainIdx)
	{
		RunData.DataSets[ChainIdx] = CopyDataSet(DataSet);
	}
	
	RunData.ResultName    = Setup->ResultName;
	RunData.ResultIndexes = Setup->ResultIndexes;
	
	u64 Timesteps = GetTimesteps(DataSet);
	RunData.Observations.resize(Timesteps);
	GetInputSeries(DataSet, Setup->ObservedName, Setup->ObservedIndexes, RunData.Observations.data(), RunData.Observations.size(), true); //NOTE: the 'true' signifies that it should get the series from the start of the modelrun rather than from the start of the input timeseries (which could potentially be different).
	
	RunData.DiscardTimesteps = Setup->DiscardTimesteps;
	
	if(RunData.DiscardTimesteps >= Timesteps)
	{
		std::cout << "ERROR: (MCMC) We are told to discard the first " << RunData.DiscardTimesteps << " timesteps when evaluating the objective, but we only run the model for " << Timesteps << " timesteps." << std::endl;
		exit(0);
	}
	
	omp_set_num_threads(Setup->NumChains); //TODO: This may disturb some of the post-processing in metropolis-hastings
	
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
	
	
	
	//NOTE: We delete every DataSet that we allocated above. We don't delete the one that was passed in since the caller may want to keep it.
	for(size_t ChainIdx = 1; ChainIdx < Setup->NumChains; ++ChainIdx)
	{
		delete RunData.DataSets[ChainIdx];
	}
}