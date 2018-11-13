
#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 0

#include "../inca.h"

#include "../ExampleModules/HBV.h"
//#include "../ExampleModules/SoilTemperatureModel.h"
//#include "../ExampleModules/WaterTemperatureModel.h"
//#include "SimplyC.h"

#include "../sqlite3/sqlite3.h"
#include "../inca_database_io.cpp"

#include "../incaview_compatibility.h"

int main(int argc, char **argv)
{
	incaview_commandline_arguments Args;
	ParseIncaviewCommandline(argc, argv, &Args);
	
	inca_model *Model = BeginModelDefinition("SimplyC", "0.0");
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddHBVModel(Model);
//	AddSoilTemperatureModel(Model);
//	AddWaterTemperatureModel(Model);
//	AddSimplyCModel(Model);
	
	EnsureModelComplianceWithIncaviewCommandline(Model, &Args);
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
	
	RunDatasetAsSpecifiedByIncaviewCommandline(DataSet, &Args);
}