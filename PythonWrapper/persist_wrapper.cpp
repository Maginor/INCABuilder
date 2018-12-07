

#define INCA_TIMESTEP_VERBOSITY 0
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 0

#include "python_wrapper.h"

#include "../ExampleModules/PersistModel.h"


inca_model *GlobalModel;
inca_data_set *GlobalDataSet;

DLLEXPORT void *
SetupModel(char *ParameterFilename, char *InputFilename) {
    
	GlobalModel = BeginModelDefinition("PERSiST", "1.0");
	
	auto Days 	      = RegisterUnit(GlobalModel, "days");
	auto System       = RegisterParameterGroup(GlobalModel, "System");
	RegisterParameterUInt(GlobalModel, System, "Timesteps", Days, 100);
	RegisterParameterDate(GlobalModel, System, "Start date", "1999-1-1");
	
	AddPersistModel(GlobalModel);
	
	ReadInputDependenciesFromFile(GlobalModel, InputFilename);
	
	EndModelDefinition(GlobalModel);
	
	GlobalDataSet = GenerateDataSet(GlobalModel);
	
	ReadParametersFromFile(GlobalDataSet, ParameterFilename);
	ReadInputsFromFile(GlobalDataSet, InputFilename);
	
	return (void *)GlobalDataSet;
}
