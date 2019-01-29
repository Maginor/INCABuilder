
#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 1
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 1

#include "../../inca.h"

#include "../../Modules/PersistModel.h"
#include "../../Modules/INCA-Sed.h"

int main()
{
	const char *InputFile = "tovdalinputs.dat";
	const char *ParameterFile = "tarlandparameters.dat";
	
	
	inca_model *Model = BeginModelDefinition("INCA-Sed", "0.0");
	
	auto Days 	        = RegisterUnit(Model, "days");
	auto System = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 10957);
	RegisterParameterDate(Model, System, "Start date", "1981-1-1");
	
	AddPersistModel(Model);
	AddINCASedModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);

	ReadParametersFromFile(DataSet, ParameterFile);

	ReadInputsFromFile(DataSet, InputFile);
	
	//WriteParametersToFile(DataSet, "newparams.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	
	SetParameterValue(DataSet, "Timesteps", {}, (u64)100);
	SetParameterValue(DataSet, "Start date", {}, "1996-5-1"); //No water input in january, so difficult to see any effect of erosion or splash detachment

	RunModel(DataSet);
	
	PrintResultSeries(DataSet, "Runoff to reach", {"Blackmill", "Arable", "Direct runoff"}, 10);
	PrintResultSeries(DataSet, "Sediment mobilised via splash detachment", {"Blackmill", "Arable"}, 10);
	PrintResultSeries(DataSet, "Flow erosion K factor", {"Blackmill", "Arable"}, 10);
	PrintResultSeries(DataSet, "Sediment transport capacity", {"Blackmill", "Arable"}, 10);
	PrintResultSeries(DataSet, "Sediment mobilised via flow erosion", {"Blackmill", "Arable"}, 10);
	PrintResultSeries(DataSet, "Surface sediment store", {"Blackmill", "Arable"}, 10);
	PrintResultSeries(DataSet, "Sediment delivery to reach", {"Blackmill", "Arable"}, 10);
	
	PrintResultSeries(DataSet, "Total sediment delivery to reach", {"Blackmill"}, 10);
	PrintResultSeries(DataSet, "Suspended sediment mass", {"Blackmill", "Clay"}, 100);
}
