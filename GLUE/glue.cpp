

#include <random>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_extended_p_square.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>


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

typedef boost::accumulators::accumulator_set<double, boost::accumulators::stats<boost::accumulators::tag::weighted_extended_p_square>, double> quantile_accumulator;

static std::pair<double, double>
EvaluateObjective(inca_data_set *DataSet, glue_objective &Objective, std::vector<quantile_accumulator>& QuantileAccumulators)
{
	using namespace boost::accumulators;
	
	u64 Timesteps = DataSet->TimestepsLastRun;
	
	//TODO: if we instead just hooked right into the storage, we would not need to copy the results over, but then we would have to do error handling for names and indexes (in case these were typed in incorrectly) because GetResultSeries would not do it for us in that case..
	std::vector<double> ModeledSeries((size_t)Timesteps);
	std::vector<double> ObservedSeries((size_t)Timesteps);

	GetResultSeries(DataSet, Objective.Modeled, Objective.IndexesModeled, ModeledSeries.data(), ModeledSeries.size());
	GetInputSeries(DataSet, Objective.Observed, Objective.IndexesObserved, ObservedSeries.data(), ObservedSeries.size(), true); //NOTE: The 'true' says that we extract the input series from the start of the modelrun rather than the start of the input series (which could be different).
	
	std::vector<double> Residuals((size_t)Timesteps);
	
	for(size_t Timestep = 0; Timestep < Timesteps; ++Timestep)
	{
		Residuals[Timestep] = ModeledSeries[Timestep] - ObservedSeries[Timestep];
	}
	
	double Performance;
	
	if(Objective.PerformanceMeasure == GLUE_PerformanceMeasure_MeanAverageError)
	{
		accumulator_set<double, features<tag::mean>> Accumulator;
		
		for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
		{
			if(!std::isnan(Residuals[Timestep]))
				Accumulator(std::abs(Residuals[Timestep]));
		}
		
		Performance = mean(Accumulator);
	}
	else if(Objective.PerformanceMeasure == GLUE_PerformanceMeasure_NashSutcliffe)
	{
		accumulator_set<double, features<tag::variance>> ObsAccum;
		accumulator_set<double, features<tag::mean>> ResAccum;

		for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
		{
			double Obs = ObservedSeries[Timestep];
			double Res = Residuals[Timestep];
			if(!std::isnan(Res) && !std::isnan(Obs))
			{
				ObsAccum(Obs);	
				ResAccum(Res*Res);
			}
		}
		
		double MeanSquaresResidual = mean(ResAccum);
		double ObservedVariance = variance(ObsAccum);
		
		Performance = 1.0 - MeanSquaresResidual / ObservedVariance;
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
	
	for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
	{
		QuantileAccumulators[Timestep](ModeledSeries[Timestep], weight = WeightedPerformance);
	}
	
	return {Performance, WeightedPerformance};
}


#if !defined(GLUE_PRINT_DEBUG_INFO)
#define GLUE_PRINT_DEBUG_INFO 0
#endif

static void
RunGLUE(inca_data_set *DataSet, glue_setup *Setup, glue_results *Results)
{
	inca_model *Model = DataSet->Model;
	
	std::mt19937_64 Generator(42);
	
	//TODO: If we want to parallelize we have to copy the dataset for each parallell unit
	
	if(Setup->Objectives.size() != 1)
	{
		std::cout << "Sorry, we only support having a single objective at the moment." << std::endl;
		exit(0);
	}
	
	Results->RunData.resize(Setup->NumRuns);
	for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
	{
		Results->RunData[Run].RandomParameters.resize(Setup->CalibrationSettings.size());
		Results->RunData[Run].PerformanceMeasures.resize(Setup->Objectives.size());
	}
	
	//NOTE: It is important that the loops are in this order so that we don't get any weird dependence between the parameter values (I think).
	for(size_t ParIdx = 0; ParIdx < Setup->CalibrationSettings.size(); ++ParIdx)
	{
		glue_parameter_calibration &ParSetting = Setup->CalibrationSettings[ParIdx];
		
		{ //TODO: Make it work with other parameter types later?
			handle_t Handle = GetParameterHandle(DataSet->Model, ParSetting.ParameterName);
			parameter_type Type = DataSet->Model->ParameterSpecs[Handle].Type;
			if(Type != ParameterType_Double)
			{
				std::cout << "Sorry, we only support calibrating parameters of type double at the moment. " << ParSetting.ParameterName << " is of another type." << std::endl;
				exit(0);
			}
		}

		for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
		{	
			Results->RunData[Run].RandomParameters[ParIdx] = GetParameterRandomlyFromDistribution(ParSetting, Generator);
		}
	}
	
	u64 NumTimesteps = GetTimesteps(DataSet);
	
	std::vector<quantile_accumulator> QuantileAccumulators;
	QuantileAccumulators.reserve(NumTimesteps);
	
	for(size_t Timestep = 0; Timestep < NumTimesteps; ++Timestep)
	{
		QuantileAccumulators.push_back(quantile_accumulator(boost::accumulators::tag::weighted_extended_p_square::probabilities = Setup->Quantiles));
	}
	
	for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
	{
#if GLUE_PRINT_DEBUG_INFO
		std::cout << "Run number: " << Run << std::endl;
#endif
		for(size_t ParIdx = 0; ParIdx < Setup->CalibrationSettings.size(); ++ParIdx)
		{
			glue_parameter_calibration &ParSetting = Setup->CalibrationSettings[ParIdx];
			
			//TODO: Other value types later
			double NewValue = Results->RunData[Run].RandomParameters[ParIdx].ValDouble;
#if GLUE_PRINT_DEBUG_INFO		
			std::cout << "Setting " << ParSetting.ParameterName << " to " << NewValue << std::endl;
#endif
			SetParameterValue(DataSet, ParSetting.ParameterName, ParSetting.Indexes, NewValue);
		}
		
		RunModel(DataSet);
		
		for(size_t ObjIdx = 0; ObjIdx < Setup->Objectives.size(); ++ObjIdx)
		{
			glue_objective &Objective = Setup->Objectives[ObjIdx];
			
			auto Perf = EvaluateObjective(DataSet, Objective, QuantileAccumulators);
#if GLUE_PRINT_DEBUG_INFO		
			std::cout << "Performance and weighted performance for " << Objective.Modeled << " vs " << Objective.Observed << " was " << std::endl << Perf.first << ", " << Perf.second << std::endl;
#endif
			Results->RunData[Run].PerformanceMeasures[ObjIdx] = Perf;
		}
#if GLUE_PRINT_DEBUG_INFO	
		std::cout << std::endl;
#endif
	}
	
	Results->PostDistribution.resize(Setup->Quantiles.size());
	
	for(size_t QuantileIdx = 0; QuantileIdx < Setup->Quantiles.size(); ++QuantileIdx)
	{
		Results->PostDistribution[QuantileIdx].resize(NumTimesteps);
		
		for(size_t Timestep = 0; Timestep < NumTimesteps; ++Timestep)
		{
			using namespace boost::accumulators;
			Results->PostDistribution[QuantileIdx][Timestep] = weighted_extended_p_square(QuantileAccumulators[Timestep])[QuantileIdx];
		}
	}
}