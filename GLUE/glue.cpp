

#include <random>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>

enum glue_parameter_distribution
{
	GLUE_ParameterDistribution_Uniform,
};

struct glue_parameter_calibration
{
	const char *ParameterName;
	std::vector<const char *> Indexes;
	glue_parameter_distribution Distribution;
	
	parameter_value Min;
	parameter_value Max;
};

enum glue_performance_measure
{
	GLUE_PerformanceMeasure_MeanAverageError,
	GLUE_PerformanceMeasure_NashSutcliffe,
};

struct glue_objective
{
	const char *Modeled;
	std::vector<const char *> IndexesModeled;
	
	const char *Observed;
	std::vector<const char *> IndexesObserved;
	
	glue_performance_measure PerformanceMeasure;
	
	double Threshold;
	double OptimalValue;
	
	bool Maximize;
	//IsPartOfMultiObjective?
};

struct glue_setup
{
	size_t NumRuns;
	
	std::vector<glue_parameter_calibration> CalibrationSettings;
	
	std::vector<glue_objective> Objectives;
};

static parameter_value
GetParameterRandomlyFromDistribution(glue_parameter_calibration &ParSetting, std::mt19937_64 &Generator)
{
	//TODO: Other parameter types later
	double NewValue;
	//TODO: Allow other types of distributions
	if(ParSetting.Distribution == GLUE_ParameterDistribution_Uniform)
	{
		std::uniform_real_distribution<double> Distribution(ParSetting.Min.ValDouble, ParSetting.Max.ValDouble);
		NewValue = Distribution(Generator);
	}
	else
	{
		std::cout << "ERROR: (GLUE) Somehow ended up with an unrecognized distribution for the parameter " << ParSetting.ParameterName << std::endl;
		exit(0); //Oops, bad for parallellized stuff. Should use a better exit function.
	}
	
	return {NewValue};
}

static std::pair<double, double>
EvaluateObjective(inca_data_set *DataSet, glue_objective &Objective)
{
	using namespace boost::accumulators;
	
	u64 Timesteps = DataSet->TimestepsLastRun;
	
	//TODO: if we instead just hooked right into the storage, we would not need to copy the results over, but then we would have to make error handling for names and indexes..
	std::vector<double> ModeledSeries((size_t)Timesteps);
	std::vector<double> ObservedSeries((size_t)Timesteps);

	GetResultSeries(DataSet, Objective.Modeled, Objective.IndexesModeled, ModeledSeries.data(), ModeledSeries.size());
	GetInputSeries(DataSet, Objective.Observed, Objective.IndexesObserved, ObservedSeries.data(), ObservedSeries.size());
	
	std::vector<double> Residuals((size_t)Timesteps);
	
	for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
	{
		Residuals[Timestep] = ModeledSeries[Timestep] - ObservedSeries[Timestep];
	}
	
	double Performance;
	
	if(Objective.PerformanceMeasure == GLUE_PerformanceMeasure_MeanAverageError)
	{
		accumulator_set<double, features<tag::mean>> Accumulator;
		
		for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
		{
			Accumulator(std::abs(ModeledSeries[Timestep] - ObservedSeries[Timestep]));
		}
		
		Performance = mean(Accumulator);
	}
	else if(Objective.PerformanceMeasure == GLUE_PerformanceMeasure_NashSutcliffe)
	{
		accumulator_set<double, features<tag::variance>> ObsAccum;
		accumulator_set<double, features<tag::mean>> ResAccum;

		for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
		{
			ObsAccum(ObservedSeries[Timestep]);
			double Res = ModeledSeries[Timestep] - ObservedSeries[Timestep];
			ResAccum(Res*Res);
		}
		
		double SumSquaresResidual = mean(ResAccum);
		double ObservedVariance = variance(ObsAccum);
		
		Performance = 1.0 - SumSquaresResidual / ObservedVariance;
	}
	//else if
	else
	{
		std::cout << "ERROR: (GLUE) Somehow ended up with an unrecognized performance measure for the series " << Objective.Modeled << std::endl;
		exit(0); //Oops, bad for parallellized stuff. Should use a better exit function.
	}
	
	double WeightedPerformance;
	if(Objective.Maximize)
	{
		WeightedPerformance = (Performance - Objective.Threshold) / (Objective.OptimalValue - Objective.Threshold);
	}
	else
	{
		WeightedPerformance = (Objective.Threshold - Performance) / (Objective.Threshold - Objective.OptimalValue);
	}
	
	return {Performance, WeightedPerformance};
}

struct glue_run_data
{
	std::vector<parameter_value> RandomParameters;                //NOTE: Values for parameters that we want to vary only.
	std::vector<std::pair<double, double>> PerformanceMeasures;   //NOTE: One per objective
};

#if !defined(GLUE_PRINT_DEBUG_INFO)
#define GLUE_PRINT_DEBUG_INFO 0
#endif

static void
RunGLUE(inca_data_set *DataSet, glue_setup *Setup)
{
	inca_model *Model = DataSet->Model;
	
	std::mt19937_64 Generator(42);
	
	//TODO: If we want to parallelize we have to copy the dataset for each parallell unit
	
	std::vector<glue_run_data> RunData; 
	RunData.resize(Setup->NumRuns);
	for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
	{
		RunData[Run].RandomParameters.resize(Setup->CalibrationSettings.size());
		RunData[Run].PerformanceMeasures.resize(Setup->Objectives.size());
	}
	
	//NOTE: It is important that the loops are in this order so that we don't get any weird dependence between the parameter values (I think).
	for(size_t ParIdx = 0; ParIdx < Setup->CalibrationSettings.size(); ++ParIdx)
	{
		glue_parameter_calibration &ParSetting = Setup->CalibrationSettings[ParIdx];
		
		{ //TODO: Make it work with other parameter types later.
			handle_t Handle = GetParameterHandle(DataSet->Model, ParSetting.ParameterName);
			parameter_type Type = DataSet->Model->ParameterSpecs[Handle].Type;
			assert(Type == ParameterType_Double);
		}

		for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
		{	
			RunData[Run].RandomParameters[ParIdx] = GetParameterRandomlyFromDistribution(ParSetting, Generator);
		}
	}
	
	for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
	{
		std::cout << "Run number: " << Run << std::endl;
		for(size_t ParIdx = 0; ParIdx < Setup->CalibrationSettings.size(); ++ParIdx)
		{
			glue_parameter_calibration &ParSetting = Setup->CalibrationSettings[ParIdx];
			
			//TODO: Other value types later
			double NewValue = RunData[Run].RandomParameters[ParIdx].ValDouble;
#if GLUE_PRINT_DEBUG_INFO		
			std::cout << "Setting " << ParSetting.ParameterName << " to " << NewValue << std::endl;
#endif
			SetParameterValue(DataSet, ParSetting.ParameterName, ParSetting.Indexes, NewValue);
		}
		
		RunModel(DataSet);
		
		for(size_t ObjIdx = 0; ObjIdx < Setup->Objectives.size(); ++ObjIdx)
		{
			glue_objective &Objective = Setup->Objectives[ObjIdx];
			
			auto Perf = EvaluateObjective(DataSet, Objective);
#if GLUE_PRINT_DEBUG_INFO		
			std::cout << "Performance and weighted performance for " << Objective.Modeled << " vs " << Objective.Observed << " was " << std::endl << Perf.first << ", " << Perf.second << std::endl;
#endif
			RunData[Run].PerformanceMeasures[ObjIdx] = Perf;
		}
		
		std::cout << std::endl;
		
	}
}