

#define INCA_TIMESTEP_VERBOSITY 0
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 0

#include "python_wrapper.h"

#include "../Modules/Persist.h"



DLLEXPORT void *
DllSetupModel(char *ParameterFilename, char *InputFilename) {
    
	CHECK_ERROR_BEGIN
	
	inca_model *Model = BeginModelDefinition("PERSiST", "1.0");
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddPersistModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFilename);
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
	
	ReadParametersFromFile(DataSet, ParameterFilename);
	ReadInputsFromFile(DataSet, InputFilename);
	
	return (void *)DataSet;
	
	CHECK_ERROR_END
	
	return 0;
}
