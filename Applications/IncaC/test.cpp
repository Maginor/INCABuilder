#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 1
#define INCA_PRINT_TIMING_INFO 1

#include "../../inca.h"

#include "../../Modules/Persist.h"
#include "../../Modules/SoilTemperature.h"
//#include "../../Modules/WaterTemperature.h"
#include "../../Modules/INCA-C.h"


#include "../../inca_json_io.cpp"


int main()
{
    const char *InputFile     = "langtjerninputs.dat";
	const char *ParameterFile = "langtjernparameters.dat";
	
	
	inca_model *Model = BeginModelDefinition("INCA-Sed", "0.0");
	
	auto Days   = RegisterUnit(Model, "days");
	auto System = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 10957);
	RegisterParameterDate(Model, System, "Start date", "1981-1-1");
	
	AddPersistModel(Model);
	AddSoilTemperatureModel(Model);
	AddINCACModel(Model);
	
	//ReadInputDependenciesFromFile(Model, InputFile);
	ReadInputDependenciesFromJson(Model, "pretty.json");
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
    
	//ReadInputsFromFile(DataSet, InputFile);
	ReadInputsFromJson(DataSet, "pretty.json");
	
	WriteParametersToFile(DataSet, "newparams.dat");
	
	WriteInputsToJson(DataSet, "pretty.json");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	
	//SetParameterValue(DataSet, "Timesteps", {}, (u64)100);
	//SetParameterValue(DataSet, "Start date", {}, "1996-5-1"); //No water input in january, so difficult to see any effect of erosion or splash detachment
    
	RunModel(DataSet);
}