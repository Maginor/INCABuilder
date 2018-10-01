
#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0

#include "../inca.h"

#include "HBV.h"


#define READ_PARAMETER_FILE 0

int main()
{
	inca_model *Model = BeginModelDefinition();
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddSnowRoutine(Model);
	AddPotentialEvapotranspirationModuleV2(Model);
	AddSoilMoistureRoutine(Model);
	AddGroundwaterResponseRoutine(Model);
	
	ReadInputDependenciesFromFile(Model, "testinput.dat"); //NOTE: Unfortunately this has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);

#if READ_PARAMETER_FILE == 0
	SetIndexes(DataSet, "Landscape units", {"Forest", "Peatland"});
	SetIndexes(DataSet, "Soil boxes", {"First box", "Second box"});
	SetBranchIndexes(DataSet, "Reaches", {{"R1", {}}, {"R2", {"R1"}}});
	
	SetParameterValue(DataSet, "%", {"R1", "Forest"}, 50.0);
	SetParameterValue(DataSet, "%", {"R1", "Peatland"}, 50.0);
	
	SetParameterValue(DataSet, "%", {"R2", "Forest"}, 50.0);
	SetParameterValue(DataSet, "%", {"R2", "Peatland"}, 50.0);
#else
	ReadParametersFromFile(DataSet, "testparameters.dat");
#endif
	ReadInputsFromFile(DataSet, "testinput.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);

	RunModel(DataSet);
	
	WriteParametersToFile(DataSet, "testparameters.dat");
	
	PrintResultSeries(DataSet, "Evapotranspiration", {"R1", "Forest", "First box"}, 100);
	PrintResultSeries(DataSet, "Evapotranspiration", {"R1", "Forest", "Second box"}, 100);
	
	//PrintResultSeries(DataSet, "Snowfall", {"First reach", "Forest"}, 100);
	//PrintResultSeries(DataSet, "Snowfall", {"Second reach", "Forest"}, 100);
	//PrintResultSeries(DataSet, "Rainfall", {"First reach", "Forest"}, 100);
	//PrintResultSeries(DataSet, "Rainfall", {"Second reach", "Forest"}, 100);
	/*
	PrintResultSeries(DataSet, "Water input to soil", {"First reach", "Forest"}, 100);
	
	PrintResultSeries(DataSet, "Evaporation", {"First reach", "Forest", "Box 1"}, 100);
	PrintResultSeries(DataSet, "Evaporation", {"First reach", "Forest", "Box 2"}, 100);
	PrintResultSeries(DataSet, "Runoff from box", {"First reach", "Forest", "Box 1"}, 100);
	PrintResultSeries(DataSet, "Runoff from box", {"First reach", "Forest", "Box 2"}, 100);
	PrintResultSeries(DataSet, "Soil moisture", {"First reach", "Forest", "Box 1"}, 100);
	PrintResultSeries(DataSet, "Soil moisture", {"First reach", "Forest", "Box 2"}, 100);
	
	PrintResultSeries(DataSet, "Groundwater recharge from box", {"First reach", "Forest", "Box 2"}, 100);
	PrintResultSeries(DataSet, "Total groundwater recharge", {"First reach"}, 100);
	PrintResultSeries(DataSet, "Groundwater runoff to reach before routing", {"First reach"}, 100);
	PrintResultSeries(DataSet, "Groundwater discharge to reach", {"First reach"}, 100);
	*/
}