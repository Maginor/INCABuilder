
#if !defined(OPTIMIZER_H)

#include "../calibration.h"

#include "dlib/optimization.h"
#include "dlib/global_optimization.h"

#include "dlib/threads/thread_pool_extension.cpp"
#include "dlib/global_optimization/global_function_search.cpp"


typedef dlib::matrix<double,0,1> column_vector;

struct optimization_setup
{
	size_t MaxFunctionCalls;
	size_t DiscardTimesteps;
	std::vector<parameter_calibration> Calibration;
	std::vector<calibration_objective> Objectives;
};

static void
ReadOptimizationSetup(optimization_setup *Setup, const char *Filename)
{
	token_stream Stream(Filename);
	
	token *Token;
	while(true)
	{
		Token = Stream.PeekToken();
		if(Token->Type == TokenType_EOF)
			break;
		
		const char *Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);
		if(strcmp(Section, "max_function_calls") == 0)
		{
			Setup->MaxFunctionCalls = (size_t)Stream.ExpectUInt();
		}
		else if(strcmp(Section, "discard_timesteps") == 0)
		{
			Setup->DiscardTimesteps = (size_t)Stream.ExpectUInt();
		}
		else if(strcmp(Section, "parameter_calibration") == 0)
		{
			ReadParameterCalibration(Stream, Setup->Calibration);
		}
		else if(strcmp(Section, "objectives") == 0)
		{
			ReadCalibrationObjectives(Stream, Setup->Objectives);
		}
	}
}

class optimization_model
{
	inca_data_set *DataSet;
	optimization_setup *Setup;
	
public:
	optimization_model(inca_data_set *DataSet, optimization_setup *Setup)
	{
		this->DataSet = DataSet;
		this->Setup = Setup;
		
		if(Setup->Objectives.size() != 1)
		{
			INCA_FATAL_ERROR("ERROR: At the moment we only support having a single optimization objective." << std::endl);
		}
	}
	
	double operator()(const column_vector& Par)
	{
		//TODO: Allow multiple objectives
		calibration_objective &Objective = Setup->Objectives[0];
		
		double Performance = EvaluateObjective(DataSet, Setup->Calibration, Objective, Par.begin(), Setup->DiscardTimesteps);
		
		return ShouldMaximize(Objective.PerformanceMeasure) ? -Performance : Performance;
	}
};

static void
PrintOptimizationResult(optimization_setup *Setup, dlib::function_evaluation& Result)
{
	std::cout << "Optimal values: " << std::endl << std::endl;
	for(size_t CalIdx = 0; CalIdx < Setup->Calibration.size(); ++CalIdx)
	{
		PrintParameterCalibration(Setup->Calibration[CalIdx]);
		std::cout << " " << Result.x(CalIdx) << std::endl << std::endl;
	}
}

static void
WriteOptimalParametersToDataSet(inca_data_set *DataSet, optimization_setup *Setup, dlib::function_evaluation &Result)
{
	size_t Dimensions = Setup->Calibration.size();
	
	for(size_t CalIdx = 0; CalIdx < Dimensions; ++ CalIdx)
	{
		double Value = Result.x(CalIdx);
		parameter_calibration &Cal = Setup->Calibration[CalIdx];
		
		for(size_t ParIdx = 0; ParIdx < Cal.ParameterNames.size(); ++ParIdx)
		{
			SetParameterValue(DataSet, Cal.ParameterNames[ParIdx], Cal.ParameterIndexes[ParIdx], Value);
		}
	}
}

static dlib::function_evaluation
RunOptimizer(inca_data_set *DataSet, optimization_setup *Setup)
{
	optimization_model Optim(DataSet, Setup);
	
	size_t Dimensions = Setup->Calibration.size();
	column_vector MinBound(Dimensions);
	column_vector MaxBound(Dimensions);
	size_t CalIdx = 0;
	for(size_t CalIdx = 0; CalIdx < Dimensions; ++CalIdx)
	{
		parameter_calibration &Cal = Setup->Calibration[CalIdx];
		
		MinBound(CalIdx) = Cal.Min;
		MaxBound(CalIdx) = Cal.Max;
	}
	
	std::cout << "Running optimization problem with " << Dimensions << " free variables. Max function calls: " << Setup->MaxFunctionCalls << "." << std::endl;
	
	auto Result = dlib::find_min_global(Optim, MinBound, MaxBound, dlib::max_function_calls(Setup->MaxFunctionCalls));
	
	return Result;
}

#define OPTIMIZER_H
#endif
