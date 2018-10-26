
#if !defined(INCAVIEW_COMPATIBILITY_H)

enum incaview_run_mode
{
	IncaviewRunMode_Run,
	IncaviewRunMode_CreateParameterDatabase,
	IncaviewRunMode_ExportParameters,
};

struct incaview_commandline_arguments
{
	incaview_run_mode Mode;
	const char *InputFileName;
	const char *ParameterDbFileName;
	const char *ParameterTextFileName;
};

static void
ParseIncaviewCommandline(int argc, char **argv, incaview_commandline_arguments *Args)
{
	bool CorrectUse = false;
	//NOTE: argv[0] is the exe name.
	
	if(argc == 4)
	{
		if(strcmp(argv[1], "run") == 0)
		{
			Args->Mode = IncaviewRunMode_Run;
			Args->InputFileName = argv[2];
			Args->ParameterDbFileName = argv[3];
			CorrectUse = true;
		}
		else if(strcmp(argv[1], "create_parameter_database") == 0)
		{
			Args->Mode = IncaviewRunMode_CreateParameterDatabase;
			Args->ParameterTextFileName = argv[2];
			Args->ParameterDbFileName   = argv[3];
			CorrectUse = true;
		}
		else if(strcmp(argv[1], "export_parameters") == 0)
		{
			Args->Mode = IncaviewRunMode_ExportParameters;
			Args->ParameterDbFileName   = argv[2];
			Args->ParameterTextFileName = argv[3];
			CorrectUse = true;
		}
	}
	
	if(!CorrectUse)
	{
		std::cout << "Incorrect use of the executable. Correct use is one of: " << std::endl;
		std::cout << " <exename> run <inputfile> <parameterdatabase>" << std::endl;
		std::cout << " <exename> create_parameter_database <parametertextfile> <parameterdatabase>" << std::endl;
		std::cout << " <exename> export_parameters <parameterdatabase> <parametertextfile>" << std::endl;
		exit(0);
	}
}

static void
EnsureModelComplianceWithIncaviewCommandline(inca_model *Model, incaview_commandline_arguments *Args)
{
	if(Args->Mode == IncaviewRunMode_Run)
	{
		ReadInputDependenciesFromFile(Model, Args->InputFileName);
	}
}

static void
RunDatasetAsSpecifiedByIncaviewCommandline(inca_data_set *DataSet, incaview_commandline_arguments *Args)
{
	if(Args->Mode == IncaviewRunMode_Run)
	{
		//TODO: would it be better to provide these as arguments too?
		const char *ResultDbFileName    = "results.db";
		const char *InputDbFileName     = "inputs.db"; //NOTE: This is only for writing inputs TO so that they can be read by INCAView. Inputs are always read in from the provided .dat file.
		
		ReadParametersFromDatabase(DataSet, Args->ParameterDbFileName);
		ReadInputsFromFile(DataSet, Args->InputFileName);
		
		RunModel(DataSet);
		
		std::cout << "Model run finished. Writing result data to database." << std::endl;
		
		//TODO: Delete existing databases if they exist? (right now it is handled by incaview, but it could be confusing if somebody runs the exe manually)
		WriteResultsToDatabase(DataSet, ResultDbFileName);
		WriteInputsToDatabase(DataSet, InputDbFileName);
	}
	else if(Args->Mode == IncaviewRunMode_CreateParameterDatabase)
	{
		ReadParametersFromFile(DataSet, Args->ParameterTextFileName);
		//TODO: Delete existing database if it exists? (right now it is handled by incaview, but it could be confusing if somebody runs the exe manually)
		CreateParameterDatabase(DataSet, Args->ParameterDbFileName);
	}
	else if(Args->Mode == IncaviewRunMode_ExportParameters)
	{
		ReadParametersFromDatabase(DataSet, Args->ParameterDbFileName);
		WriteParametersToFile(DataSet, Args->ParameterTextFileName);
	}
}

#define INCAVIEW_COMPATIBILITY_H
#endif