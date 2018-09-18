#define INCA_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define INCA_TEST_FOR_NAN 0
#define INCA_EQUATION_PROFILING 0
#define INCA_PRINT_TIMING_INFO 0

#include "../inca.h"

#include "../ExampleModules/SnowMeltModel.h"
#include "../ExampleModules/SoilTemperatureModel.h"
#include "../ExampleModules/WaterTemperatureModel.h"
#include "../ExampleModules/INCA-N_ClassicModel.h"

#include "../sqlite3/sqlite3.h"
#include "../inca_database_io.cpp"

int main(int argc, char **argv)
{
	int Mode = 0;
	
	const char *File;
	
	bool CorrectUse = true;
	
	if(argc == 3)
	{
		//NOTE: argv[0] is the exe name.
		const char *Command = argv[1];
		File    = argv[2];
		
		if(strcmp(Command, "create_parameter_database") == 0)
		{
			Mode = 0;
		}
		else if(strcmp(Command, "run") == 0)
		{
			Mode = 1;
		}
		else
		{
			CorrectUse = false;
		}
	}
	else if(argc == 1)
	{
		Mode = 1;
		File = "incaNtestinput.dat";
	}
	else
	{
		CorrectUse = false;
	}
	
	if(!CorrectUse)
	{
		std::cout << "ERROR: unknown use of incan.exe: Proper use is one of:" << std::endl;
		std::cout << "incan" << std::endl;
		std::cout << "incan create_parameter_database <parameterfile.dat>" << std::endl;
		std::cout << "incan run <inputfile.dat>" << std::endl;
		exit(0);
	}
	
	const char *Parameterdb = "parameters.db";
	const char *Resultdb    = "results.db";
	const char *Inputdb     = "inputs.db"; //NOTE: This is only for writing inputs TO so that they can be read by INCAView. Inputs are always read in from the provided .dat file.
	
	inca_model *Model = BeginModelDefinition();
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddSnowMeltModel(Model);
	AddSoilTemperatureModel(Model);
	AddWaterTemperatureModel(Model);
	AddINCANClassicModel(Model);
	
	if(Mode == 1)
	{
		ReadInputDependenciesFromFile(Model, File);
	}
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
	
	if(Mode == 0)
	{
		ReadParametersFromFile(DataSet, File);
		//TODO: Delete existing database if it exists?
		CreateParameterDatabase(DataSet, Parameterdb);
	}
	else if(Mode == 1)
	{
		ReadParametersFromDatabase(DataSet, Parameterdb);
		ReadInputsFromFile(DataSet, File);
		
		RunModel(DataSet);
		
		std::cout << "Model run finished. Writing result data to database." << std::endl;
		
		//TODO: Delete existing databases if they exist?
		WriteResultsToDatabase(DataSet, Resultdb);
		WriteInputsToDatabase(DataSet, Inputdb);
	}
	
}
