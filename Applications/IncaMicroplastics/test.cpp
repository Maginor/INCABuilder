
#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 1

#include "../../inca.h"

#include "../../Modules/Persist.h"
#include "../../Modules/INCA-Microplastics.h"

int main()
{
    const char *InputFile = "testinputs.dat";
	const char *ParameterFile = "testparameters.dat";
	
	inca_model *Model = BeginModelDefinition("INCA-Microplastics", "0.0");
	
	auto Days   = RegisterUnit(Model, "days");
	auto System = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 10957);
	RegisterParameterDate(Model, System, "Start date", "1981-1-1");
	
	AddPersistModel(Model);
	AddINCAMicroplasticsModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
    
	ReadInputsFromFile(DataSet, InputFile);
	
	//WriteParametersToFile(DataSet, "newparams.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	//RunModel(DataSet);
}


