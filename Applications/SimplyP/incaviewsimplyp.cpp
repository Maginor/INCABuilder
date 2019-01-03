#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 0

#include "../../inca.h"

#include "SimplyP.h"

#include "../../sqlite3/sqlite3.h"
#include "../../inca_database_io.cpp"

#include "../../incaview_compatibility.h"

int main(int argc, char **argv)
{
	incaview_commandline_arguments Args;
	ParseIncaviewCommandline(argc, argv, &Args);
	
	inca_model *Model = BeginModelDefinition("Simply P", "0.0");
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto DynamicOptions       = RegisterParameterGroup(Model, "Dynamic options");
	RegisterParameterUInt(Model, DynamicOptions, "Timesteps", Days, 100);
	RegisterParameterDate(Model, DynamicOptions, "Start date", "1999-1-1");
	
	AddSimplyPHydrologyModule(Model);
	AddSimplyPSedimentModule(Model);
	AddSimplyPPhosphorusModule(Model);
	
	EnsureModelComplianceWithIncaviewCommandline(Model, &Args);
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
	
	RunDatasetAsSpecifiedByIncaviewCommandline(DataSet, &Args);
}