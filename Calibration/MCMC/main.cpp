
#include "../../inca.h"
#include "../../Modules/PersistModel.h"

#include "inca_mcmc.h"

int main()
{
	const char *ParameterFile = "../../Applications/IncaN/tovdalparametersPersistOnly.dat";
	const char *InputFile     = "../../Applications/IncaN/tovdalinputs.dat";
	
	inca_model *Model = BeginModelDefinition();
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddPersistModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
	
	//NOTE: For model structure as well as parameter values that we are not going to change:
	ReadParametersFromFile(DataSet, ParameterFile);
	ReadInputsFromFile(DataSet, InputFile);
	
	mcmc_setup Setup = {};
	
	ReadMCMCSetupFromFile(&Setup, "mcmc_setup.dat");

	mcmc_results Results;
	
	timer MCMCTimer = BeginTimer();
	RunMCMC(DataSet, &Setup, &Results);
	
	u64 Ms = GetTimerMilliseconds(&MCMCTimer);
	
	std::cout << "Total MCMC run time : " << Ms << " milliseconds." << std::endl;
	
	std::cout << "Acceptance rate: " << Results.AcceptanceRate << std::endl;
	//TODO: post-processing / store results
	
	if(Setup.Algorithm == MCMCAlgorithm_DifferentialEvolution)
	{
		arma::cube& Draws = Results.DrawsOut;
	
		Draws.slice(Draws.n_slices - 1).print();
		Draws.save("mcmc_results.dat", arma::arma_ascii);
	}
	else
	{
		arma::mat& Draws2 = Results.DrawsOut2;
	
		Draws2.row(Draws2.n_rows - 1).print();
		Draws2.save("mcmc_results.dat", arma::arma_ascii);
	}
}