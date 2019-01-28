
#if !defined(INCAVIEW_COMPATIBILITY_H)


#if !defined(INCAVIEW_INCLUDE_OPTIMIZER)
#define INCAVIEW_INCLUDE_OPTIMIZER 0
#endif

#if INCAVIEW_INCLUDE_OPTIMIZER
#include "Calibration\Optimizer\optimizer.h"
#endif

enum incaview_run_mode
{
	IncaviewRunMode_Run,
	IncaviewRunMode_CreateParameterDatabase,
	IncaviewRunMode_ExportParameters,
	IncaviewRunMode_FillParameterFile,
	IncaviewRunMode_RunOptimization,
};

struct incaview_commandline_arguments
{
	incaview_run_mode Mode;
	const char *InputFileName;
	const char *ParameterInFileName;
	const char *ParameterOutFileName;
	const char *CalibrationScriptName;
	const char *Exename;
};

static void
ParseIncaviewCommandline(int argc, char **argv, incaview_commandline_arguments *Args)
{
	bool CorrectUse = false;
	char *Exename = argv[0];
	
	//NOTE: Very rudimentary attempt to trim off any leading directories. In case somebody called the exe from another directory while creating the database.
	int LastSlashPos = -1;
	int Pos = 0;
	for(char *C = Exename; *C != 0; ++C)
	{
		if(*C == '\\' || *C == '/') LastSlashPos = Pos;
		++Pos;
	}
	Exename = Exename + (LastSlashPos + 1);
	
	Args->Exename = Exename;
	if(argc == 4)
	{
		if(strcmp(argv[1], "run") == 0)
		{
			Args->Mode = IncaviewRunMode_Run;
			Args->InputFileName = argv[2];
			Args->ParameterInFileName = argv[3];
			CorrectUse = true;
		}
		else if(strcmp(argv[1], "create_parameter_database") == 0)
		{
			Args->Mode = IncaviewRunMode_CreateParameterDatabase;
			Args->ParameterInFileName    = argv[2];
			Args->ParameterOutFileName   = argv[3];
			CorrectUse = true;
		}
		else if(strcmp(argv[1], "export_parameters") == 0)
		{
			Args->Mode = IncaviewRunMode_ExportParameters;
			Args->ParameterInFileName   = argv[2];
			Args->ParameterOutFileName  = argv[3];
			CorrectUse = true;
		}
		else if(strcmp(argv[1], "fill_parameter_file") == 0)
		{
			Args->Mode = IncaviewRunMode_FillParameterFile;
			Args->ParameterInFileName  = argv[2];
			Args->ParameterOutFileName = argv[3];
			CorrectUse = true;
		}
	}
	else if(argc == 6)
	{
#if INCAVIEW_INCLUDE_OPTIMIZER
		if(strcmp(argv[1], "run_optimizer") == 0)
		{
			Args->Mode = IncaviewRunMode_RunOptimization;
			Args->InputFileName       = argv[2];
			Args->ParameterInFileName = argv[3];
			Args->CalibrationScriptName = argv[4];
			Args->ParameterOutFileName = argv[5];
			CorrectUse = true;
		}
#endif
	}
	
	if(!CorrectUse)
	{
		std::cout << "Incorrect use of the executable. Correct use is one of: " << std::endl;
		std::cout << " <exename> run <inputfile(.dat)> <parameterfile(.db or .dat)>" << std::endl;
		std::cout << " <exename> create_parameter_database <parameterfile(.dat)> <parameterfile(.db)>" << std::endl;
		std::cout << " <exename> export_parameters <parameterfile(.db)> <parameterfile(.dat)>" << std::endl;
		std::cout << " <exename> fill_parameter_file <parameterfilein(.dat)> <parameterfileout(.dat)>" << std::endl;
#if INCAVIEW_INCLUDE_OPTIMIZER
		std::cout << " <exename> run_optimizer <inputfile.dat> <parameterfile(.db or .dat)> <calibrationscript(.dat)> <parameterfileout(.dat or .db)>" << std::endl;
#endif
		exit(0);
	}
}

static void
EnsureModelComplianceWithIncaviewCommandline(inca_model *Model, incaview_commandline_arguments *Args)
{
	if(Args->Mode == IncaviewRunMode_Run || Args->Mode == IncaviewRunMode_RunOptimization)
	{
		ReadInputDependenciesFromFile(Model, Args->InputFileName);
	}
}

static int
IncaviewParseFileType(const char *Filename)
{
	int Len = strlen(Filename);
	int At = Len - 1;
	while(At >= 0)
	{
		char C = Filename[At];
		if(C == '.') break;
		At--;
	}
	//NOTE: At now points to the last '.' in the file name
	const char *Extension = Filename + At + 1;
	
	if(strcmp(Extension, "db") == 0) return 0;
	if(strcmp(Extension, "dat") == 0) return 1;
	
	std::cout << "ERROR: Unsupported file extension: " << Extension << " for file " << Filename << std::endl;
	exit(0);
}

static void
RunDatasetAsSpecifiedByIncaviewCommandline(inca_data_set *DataSet, incaview_commandline_arguments *Args)
{
	if(Args->Mode == IncaviewRunMode_Run)
	{
		//TODO: would it be better to provide these as arguments too?
		const char *ResultDbFileName    = "results.db";
		const char *InputDbFileName     = "inputs.db"; //NOTE: This is only for writing inputs TO so that they can be read by INCAView. Inputs are always read in from the provided .dat file.
		
		int Type = IncaviewParseFileType(Args->ParameterInFileName);
		
		if(Type == 0)
			ReadParametersFromDatabase(DataSet, Args->ParameterInFileName);
		else
			ReadParametersFromFile(DataSet, Args->ParameterInFileName);
		ReadInputsFromFile(DataSet, Args->InputFileName);
		
		RunModel(DataSet);
		
		std::cout << "Model run finished. Writing result data to database." << std::endl;
		
		//TODO: Delete existing databases if they exist? (right now it is handled by incaview, but it could be confusing if somebody runs the exe manually)
		WriteResultsToDatabase(DataSet, ResultDbFileName);
		WriteInputsToDatabase(DataSet, InputDbFileName);
	}
#if INCAVIEW_INCLUDE_OPTIMIZER
	else if(Args->Mode == IncaviewRunMode_RunOptimization)
	{
		int Type = IncaviewParseFileType(Args->ParameterInFileName);
		
		if(Type == 0)
			ReadParametersFromDatabase(DataSet, Args->ParameterInFileName);
		else
			ReadParametersFromFile(DataSet, Args->ParameterInFileName);
		
		ReadInputsFromFile(DataSet, Args->InputFileName);
		
		optimization_setup Setup;
	
		ReadOptimizationSetup(&Setup, Args->CalibrationScriptName);
		
		auto Result = RunOptimizer(DataSet, &Setup);
		
		std::cout << std::endl;
		PrintOptimizationResult(&Setup, Result);
		
		WriteOptimalParametersToDataSet(DataSet, &Setup, Result);
		
		int Type2 = IncaviewParseFileType(Args->ParameterOutFileName);
		if(Type2 == 0)
			CreateParameterDatabase(DataSet, Args->ParameterOutFileName, Args->Exename);
		else
			WriteParametersToFile(DataSet, Args->ParameterOutFileName);
		
	}
#endif
	else if(Args->Mode == IncaviewRunMode_CreateParameterDatabase)
	{
		//TODO: Check right file types?
		ReadParametersFromFile(DataSet, Args->ParameterInFileName);
		//TODO: Delete existing database if it exists? (right now it is handled by incaview, but it could be confusing if somebody runs the exe manually)
		CreateParameterDatabase(DataSet, Args->ParameterOutFileName, Args->Exename);
	}
	else if(Args->Mode == IncaviewRunMode_ExportParameters)
	{
		//TODO: Check right file types?
		ReadParametersFromDatabase(DataSet, Args->ParameterInFileName);
		WriteParametersToFile(DataSet, Args->ParameterOutFileName);
	}
	else if(Args->Mode == IncaviewRunMode_FillParameterFile)
	{
		ReadParametersFromFile(DataSet, Args->ParameterInFileName);
		WriteParametersToFile(DataSet, Args->ParameterOutFileName);
	}
}

#define INCAVIEW_COMPATIBILITY_H
#endif