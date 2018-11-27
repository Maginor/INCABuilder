
#include "../inca.h"
#include "../SimplyP/SimplyP.h"

#include "inca_mcmc.h"

int main()
{
	const char *ParameterFile = "../SimplyP/tarlandparameters.dat";
	const char *InputFile     = "../SimplyP/tarlandinputs.dat";
	
	inca_model *Model = BeginModelDefinition();
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "Dynamic options");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddSimplyPHydrologyModule(Model);
	AddSimplyPSedimentModule(Model);
	AddSimplyPPhosphorusModule(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
	
	//NOTE: For model structure as well as parameter values that we are not going to change:
	ReadParametersFromFile(DataSet, ParameterFile);
	ReadInputsFromFile(DataSet, InputFile);
	
	mcmc_setup Setup;
	
	ReadMCMCSetupFromFile(&Setup, "mcmc_setup.dat");

	mcmc_results Results;
	
	timer MCMCTimer = BeginTimer();
	RunMCMC(DataSet, &Setup, &Results);
	
	u64 Ms = GetTimerMilliseconds(&MCMCTimer);
	
	std::cout << "Total MCMC run time : " << Ms << " milliseconds." << std::endl;
	
	std::cout << "Acceptance rate: " << Results.AcceptanceRate << std::endl;
	//TODO: post-processing / store results
}