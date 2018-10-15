



// NOTE: This is an example of a GLUE setup for HBV.


#define INCA_TIMESTEP_VERBOSITY 0
#define INCA_TEST_FOR_NAN 1
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 0

#include "../inca.h"

#include "../SimplyC/HBV.h"
//#include "../ExampleModules/SoilTemperatureModel.h"
//#include "../ExampleModules/WaterTemperatureModel.h"
//#include "../SimplyC/SimplyC.h"

#define GLUE_PRINT_DEBUG_INFO 1

#include "glue.cpp"
#include "glue_io.cpp"

int main()
{
	inca_model *Model = BeginModelDefinition("HBV", "0.0");
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddHBVModel(Model);
	//AddSoilTemperatureModel(Model);
	//AddWaterTemperatureModel(Model);
	//AddSimplyCModel(Model);
	
	ReadInputDependenciesFromFile(Model, "langtjerninputs.dat");
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);

	ReadParametersFromFile(DataSet, "langtjernparametersHBVonly.dat");
	ReadInputsFromFile(DataSet, "langtjerninputs.dat");
	
	glue_setup Setup;
	
	ReadSetupFromFile(&Setup, "GLUE_setup.dat");
	
	RunGLUE(DataSet, &Setup);
}