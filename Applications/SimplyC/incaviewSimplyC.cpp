#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 0

//NOTE: You should probably not turn the following flags on unless you are Magnus or Magnus has shown you how to set up the libraries you need for correct compilation.
#define INCAVIEW_INCLUDE_OPTIMIZER 0
#define INCAVIEW_INCLUDE_GLUE 0
#define INCAVIEW_INCLUDE_MCMC 0

#define CALIBRATION_PRINT_DEBUG_INFO 0

#include "../../inca.h"

#include "../../Modules/UnitConversions.h"
#include "../../Modules/SimplyC.h"
#include "../../Modules/SimplyHydrol_noGW.h"
#include "../../Modules/SoilTemperature_simply.h"

#include "../../incaview_compatibility.h"

int main(int argc, char **argv)
{
	incaview_commandline_arguments Args;
	ParseIncaviewCommandline(argc, argv, &Args);
	
	inca_model *Model = BeginModelDefinition("SimplyC", "0.1");
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1991-1-1");
	
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel(Model);
	AddSimplyCModel(Model);
	
	EnsureModelComplianceWithIncaviewCommandline(Model, &Args);
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
	
	RunDatasetAsSpecifiedByIncaviewCommandline(DataSet, &Args);
}