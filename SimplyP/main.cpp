
#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 1

#include "../inca.h"

#include "SimplyP.h"

#define READ_PARAMETER_FILE 1

int main()
{
	inca_model *Model = BeginModelDefinition("Simply P", "0.0");
	
	auto Days 	        = RegisterUnit(Model, "days");
	auto DynamicOptions = RegisterParameterGroup(Model, "Dynamic options");
	RegisterParameterUInt(Model, DynamicOptions, "Timesteps", Days, 10957);
	RegisterParameterDate(Model, DynamicOptions, "Start date", "1981-1-1");
	
	AddSimplyPHydrologyModule(Model);
	AddSimplyPSedimentModule(Model);
	AddSimplyPPhosphorusModule(Model);
	
	ReadInputDependenciesFromFile(Model, "tarlandinputs.dat"); //NOTE: This has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);

#if READ_PARAMETER_FILE == 0
	SetIndexes(DataSet, "Landscape units", {"Arable", "Semi-natural", "Improved grassland"});
	SetBranchIndexes(DataSet, "Reaches", {  {"Tarland1", {}} }  );
	
	AllocateParameterStorage(DataSet);
	WriteParametersToFile(DataSet, "newparams.dat");
#else
	ReadParametersFromFile(DataSet, "tarlandparameters.dat");
#endif

	ReadInputsFromFile(DataSet, "tarlandinputs.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);

	RunModel(DataSet);
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