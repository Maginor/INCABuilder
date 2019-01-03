



// NOTE: This is an example of a GLUE setup for HBV.


#define INCA_TIMESTEP_VERBOSITY 0
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 0

#include "../../inca.h"

#include "../../Modules/HBV.h"

#define GLUE_PRINT_DEBUG_INFO 0
#define GLUE_MULTITHREAD 1

#include "glue.h"

int main()
{
	inca_model *Model = BeginModelDefinition("HBV", "0.0");
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddHBVModel(Model);
	
	ReadInputDependenciesFromFile(Model, "langtjerninputs.dat");
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);

	ReadParametersFromFile(DataSet, "langtjernparametersHBVonly.dat");
	ReadInputsFromFile(DataSet, "langtjerninputs.dat");
	
	glue_setup Setup;
	glue_results Results;
	
	ReadSetupFromFile(&Setup, "GLUE_setup.dat");
	
	timer RunGlueTimer = BeginTimer();
	RunGLUE(DataSet, &Setup, &Results);
	u64 Ms = GetTimerMilliseconds(&RunGlueTimer);
	
	std::cout << "GLUE finished. Running the model " << Setup.NumRuns << " times with " << Setup.NumThreads << " threads took " << Ms << " milliseconds." << std::endl;
	
	WriteGLUEResultsToDatabase("GLUE_results.db", &Setup, &Results, DataSet);
}