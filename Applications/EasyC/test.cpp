#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 1
#define INCA_EQUATION_PROFILING 0

#include "../../inca.h"

#include "../../Modules/HBV.h"
#include "../../Modules/SoilTemperature.h"
#include "../../Modules/WaterTemperature.h"
#include "../../Modules/EasyC.h"

#define READ_PARAMETER_FILE 1

int main()
{
	inca_model *Model = BeginModelDefinition("Carbon Model", "0.0");

	auto Days = RegisterUnit(Model, "days");
	auto System = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");

	AddHBVModel(Model);
	AddSoilTemperatureModel(Model);
	AddWaterTemperatureModel(Model);
	AddEasyCModel(Model);

	ReadInputDependenciesFromFile(Model, "langtjerninputs.dat"); //NOTE: Unfortunately this has to happen here before EndModelDefinition

	EndModelDefinition(Model);

	inca_data_set *DataSet = GenerateDataSet(Model);

#if READ_PARAMETER_FILE == 0
	SetIndexes(DataSet, "Landscape units", {"Forest", "Peatland"});
	SetBranchIndexes(DataSet, "Reaches", { {"R1", {}} });

	SetParameterValue(DataSet, "%", {"R1", "Forest"}, 50.0);
	SetParameterValue(DataSet, "%", {"R1", "Peatland"}, 50.0);
#else
	ReadParametersFromFile(DataSet, "langtjernparameters.dat");
#endif
	ReadInputsFromFile(DataSet, "langtjerninputs.dat");

	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);

	RunModel(DataSet);

	WriteParametersToFile(DataSet, "newparams.dat");

	//PrintResultSeries(DataSet, "DOC to routing", {"R2"}, 100);

	//PrintResultSeries(DataSet, "DOC output from reach", { "R2" }, 100);
	//PrintResultSeries(DataSet, "DIC output from reach", { "R2" }, 100);

}
