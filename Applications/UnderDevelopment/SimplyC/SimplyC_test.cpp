#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 1 //0 or 1. Test for NaNs and if find print which equation, indexer and params associated with it. Slows model.
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 1

#include "../../inca.h"

#include "../../Modules/SimplyC.h"
#include "../../Modules/SimplyHydrol.h"
#include "../../Modules/SoilTemperature_simply.h"

#define READ_PARAMETER_FILE 1 //Read params from file? Or auto-generate using indexers defined below & defaults

int main()
{
	inca_model *Model = BeginModelDefinition("SimplyC", "0.0"); //Name, version
	
	auto Days 	        = RegisterUnit(Model, "days");
	auto System = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 10957);
	RegisterParameterDate(Model, System, "Start date", "1986-1-1");
	
	//Call functions declared earlier
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel(Model);
	AddSimplyCModel(Model);
	
	//Input .dat file can say whether inputs vary per reach/landscape unit/are global, etc., which affects eqns
	ReadInputDependenciesFromFile(Model, "../SimplyC/langtjerninputs.dat"); //NOTE: This has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);  //"compiles" model - order eqns, etc.
	
	inca_data_set *DataSet = GenerateDataSet(Model); //Create specific model dataset objext

#if READ_PARAMETER_FILE == 0 //If don't have param file already, auto-generate param file for you
	SetIndexes(DataSet, "Landscape units", {"Low soil carbon", "High soil carbon"});
	SetBranchIndexes(DataSet, "Reaches", {  {"Inlet", {}} }  );
	
	AllocateParameterStorage(DataSet);
	WriteParametersToFile(DataSet, "new_SimplyC_params.dat");
#else
	ReadParametersFromFile(DataSet, "SimplyC_params.dat");

	ReadInputsFromFile(DataSet, "../SimplyC/langtjerninputs.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	
	//SetParameterValue(DataSet, "Timesteps", {}, (u64)1); //for testing

	RunModel(DataSet);
#endif

	// Can access results by name and indexes. Get this from results structure
	PrintResultSeries(DataSet, "Soil water volume", {"Inlet"}, 10); //Print just first 10 values
	PrintResultSeries(DataSet, "Soil water carbon flux", {"Inlet"}, 10);

	
	//PrintResultSeries(DataSet, "Agricultural soil water EPC0", {"Tarland1"}, 1000);
	//PrintResultSeries(DataSet, "Agricultural soil labile P mass", {"Tarland1"}, 1000);
	//PrintResultSeries(DataSet, "Agricultural soil TDP mass", {"Tarland1"}, 1000);
	
	/*
	DlmWriteResultSeriesToFile(DataSet, "results.dat",
		{"Agricultural soil water volume", "Agricultural soil net P sorption", "Agricultural soil water EPC0", "Agricultural soil labile P mass", "Agricultural soil TDP mass"}, 
		{{"Tarland1"},                     {"Tarland1"},                   {"Tarland1"},                      {"Tarland1"},                       {"Tarland1"}},
		'\t'
	);
	*/
}