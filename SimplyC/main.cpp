
#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 1
#define INCA_EQUATION_PROFILING 0

#include "../inca.h"

#include "HBV.h"
#include "../ExampleModules/SoilTemperatureModel.h"
#include "../ExampleModules/WaterTemperatureModel.h"
#include "SimplyC.h"

#define READ_PARAMETER_FILE 1

static void
AddHBVModel(inca_model *Model)
{
	AddSnowRoutine(Model);
	AddPotentialEvapotranspirationModuleV2(Model);
	AddSoilMoistureRoutine(Model);
	AddGroundwaterResponseRoutine(Model);
	AddWaterRoutingRoutine(Model);
	AddReachFlowRoutine(Model);
}

static void
AddCarbonModel(inca_model *Model)
{
	AddCarbonInSoilModule(Model);
	AddCarbonInGroundwaterModule(Model);
	AddCarbonRoutingRoutine(Model);
	AddCarbonInReachModule(Model);
}

int main()
{
	inca_model *Model = BeginModelDefinition("Carbon model", "0.0");
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddHBVModel(Model);
	AddSoilTemperatureModel(Model);
	AddWaterTemperatureModel(Model);
	AddCarbonModel(Model);
	
	ReadInputDependenciesFromFile(Model, "langtjerninputs.dat"); //NOTE: Unfortunately this has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);

#if READ_PARAMETER_FILE == 0
	SetIndexes(DataSet, "Landscape units", {"Forest", "Peatland"});
	SetIndexes(DataSet, "Soil boxes", {"Upper box", "Lower box"});
	SetBranchIndexes(DataSet, "Reaches", {{"R1", {}}, {"R2", {"R1"}}});
	
	SetParameterValue(DataSet, "%", {"R1", "Forest"}, 50.0);
	SetParameterValue(DataSet, "%", {"R1", "Peatland"}, 50.0);
	
	SetParameterValue(DataSet, "%", {"R2", "Forest"}, 50.0);
	SetParameterValue(DataSet, "%", {"R2", "Peatland"}, 50.0);
#else
	ReadParametersFromFile(DataSet, "testparameters.dat");
#endif
	ReadInputsFromFile(DataSet, "langtjerninputs.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);

	RunModel(DataSet);
	
	WriteParametersToFile(DataSet, "newparams.dat");
	
	//PrintResultSeries(DataSet, "DOC to routing", {"R2"}, 100);
	
	PrintResultSeries(DataSet, "DOC output from reach", {"R2"}, 100);
	PrintResultSeries(DataSet, "DIC output from reach", {"R2"}, 100);
	
	//PrintResultSeries(DataSet, "Evapotranspiration", {"R1", "Forest", "Upper box"}, 100);
	//PrintResultSeries(DataSet, "Evapotranspiration", {"R1", "Forest", "Lower box"}, 100);
	
	//PrintResultSeries(DataSet, "SOC in upper soil box", {"R1", "Forest"}, 100);
	//PrintResultSeries(DataSet, "DOC in upper soil box", {"R1", "Forest"}, 100);
	//PrintResultSeries(DataSet, "DIC in upper soil box", {"R1", "Forest"}, 100);
	
	//PrintResultSeries(DataSet, "Flow to routing routine", {"R1"}, 100);
	//PrintResultSeries(DataSet, "Flow from routing routine to reach", {"R1"}, 100);
	//PrintResultSeries(DataSet, "Reach flow input", {"R1"}, 100);
	
	//PrintResultSeries(DataSet, "Reach flow", {"R1"}, 100);
	//PrintResultSeries(DataSet, "Reach volume", {"R1"}, 100);
	//PrintResultSeries(DataSet, "Reach flow", {"R2"}, 100);
	
	//PrintResultSeries(DataSet, "DOC from groundwater to reach", {"R1"}, 100);
	//PrintResultSeries(DataSet, "DIC from groundwater to reach", {"R1"}, 100);
	
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