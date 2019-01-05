

#include <random>
#include <thread>
#include <mutex>


#include <boost/accumulators/statistics/weighted_extended_p_square.hpp>


static double
GetParameterRandomlyFromDistribution(parameter_calibration &ParSetting, std::mt19937_64 &Generator)
{
	//TODO: Other parameter types later
	double NewValue;
	//TODO: Allow other types of distributions
	if(ParSetting.Distribution == ParameterDistribution_Uniform)
	{
		std::uniform_real_distribution<double> Distribution(ParSetting.Min, ParSetting.Max);
		NewValue = Distribution(Generator);
	}
	// else if..
	
	return NewValue;
}

typedef boost::accumulators::accumulator_set<double, boost::accumulators::stats<boost::accumulators::tag::weighted_extended_p_square>, double> quantile_accumulator;


#if !defined(GLUE_MULTITHREAD)
#define GLUE_MULTITHREAD 1
#endif

#if GLUE_MULTITHREAD
std::mutex AccumulatorLock;
#endif

static std::pair<double, double>
ComputeWeightedPerformance(inca_data_set *DataSet, double Performance, calibration_objective &Objective, std::vector<quantile_accumulator>& QuantileAccumulators, size_t DiscardTimesteps)
{
	using namespace boost::accumulators;
	
	double WeightedPerformance;
	if(ShouldMaximize(Objective.PerformanceMeasure))
	{
		WeightedPerformance = (Performance - Objective.Threshold) / (Objective.OptimalValue - Objective.Threshold);
	}
	else
	{
		WeightedPerformance = (Objective.Threshold - Performance) / (Objective.Threshold - Objective.OptimalValue);
	}
	
	size_t Timesteps = (size_t)DataSet->TimestepsLastRun;
	
	//TODO: The other EvaluateObjective we call before this already extracts this series, so it is kind of stupid to do it twice. Maybe we could make an optional version of it that gives us back the modeled series?
	std::vector<double> ModeledSeries(Timesteps);
	GetResultSeries(DataSet, Objective.ModeledName, Objective.ModeledIndexes, ModeledSeries.data(), ModeledSeries.size());
	
#if GLUE_MULTITHREAD
	std::lock_guard<std::mutex> Lock(AccumulatorLock); //NOTE: I don't know if it is safe for several threads to write to the same accumulator without locking it, so I do this to be safe. If it is not a problem, just remove the mutex.
#endif
	
	//TODO! TODO! We should maybe also discard timesteps here too (but has to take care to do it correctly!)
	for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
	{
		QuantileAccumulators[Timestep](ModeledSeries[Timestep], weight = WeightedPerformance);
	}
	
	return {Performance, WeightedPerformance};
}

static void
RunGLUE(inca_data_set *DataSet, glue_setup *Setup, glue_results *Results)
{
	const inca_model *Model = DataSet->Model;
	
	std::mt19937_64 Generator(42);
	
	//TODO: If we want to parallelize we have to copy the dataset for each parallell unit
	
	if(Setup->Objectives.size() != 1)
	{
		std::cout << "Sorry, we only support having a single objective at the moment." << std::endl;
		exit(0);
	}
	
	if(Setup->Quantiles.empty())
	{
		std::cout << "ERROR: GLUE requires at least 1 quantile" << std::endl;
		exit(0);
	}
	
	Results->RunData.resize(Setup->NumRuns);
	for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
	{
		Results->RunData[Run].RandomParameters.resize(Setup->Calibration.size());
		Results->RunData[Run].PerformanceMeasures.resize(1);
	}
	
	//NOTE: It is important that the loops are in this order so that we don't get any weird dependence between the parameter values (I think).
	//TODO: We should probably have a better generation scheme for parameter values (such as Latin Cube?).
	for(size_t ParIdx = 0; ParIdx < Setup->Calibration.size(); ++ParIdx)
	{
		parameter_calibration &ParSetting = Setup->Calibration[ParIdx];

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
	
#if GLUE_MULTITHREAD
	auto RunFunc =
		[&] (size_t RunID, inca_data_set *DataSet)
		{
			/*
			for(size_t ParIdx = 0; ParIdx < Setup->Calibration.size(); ++ParIdx)
			{
				glue_parameter_calibration &ParSetting = Setup->Calibration[ParIdx];
				
				//TODO: Other value types later
				double NewValue = Results->RunData[RunID].RandomParameters[ParIdx];

				SetParameterValue(DataSet, ParSetting.ParameterName, ParSetting.Indexes, NewValue);
			}
			
			RunModel(DataSet);
			*/
			calibration_objective &Objective = Setup->Objectives[0];
			
			const double *ParValues = Results->RunData[RunID].RandomParameters.data();
			
			double Performance = EvaluateObjective(DataSet, Setup->Calibration, Objective, ParValues, Setup->DiscardTimesteps);

			auto Perf = ComputeWeightedPerformance(DataSet, Performance, Objective, QuantileAccumulators, Setup->DiscardTimesteps);

			Results->RunData[RunID].PerformanceMeasures[0] = Perf;

		};
	
	size_t NumThreads = Setup->NumThreads;
	size_t RunsPerThread = Setup->NumRuns / NumThreads;
	if(Setup->NumRuns % NumThreads != 0) RunsPerThread++;

	std::vector<std::thread> Threads;
	Threads.reserve(NumThreads);
	
	//NOTE: Each thread has to work on its own dataset (otherwise they would overwrite each others results).
	std::vector<inca_data_set *> DataSets(NumThreads);
	DataSets[0] = DataSet;
	for(size_t ThreadId = 1; ThreadId < NumThreads; ++ThreadId)
	{
		DataSets[ThreadId] = CopyDataSet(DataSet);
	}
	
	//TODO: This is an inefficient way to do it, instead we should have a batch system.
	for(size_t Run = 0; Run < RunsPerThread; ++Run)
	{
		size_t RunIDBase = Run * NumThreads;
		
		for(size_t ThreadIdx = 0; ThreadIdx < NumThreads; ++ThreadIdx)
		{
			size_t RunID = RunIDBase + ThreadIdx;
			if(RunID < Setup->NumRuns)
			{
				Threads.push_back(std::thread(RunFunc, RunID, DataSets[ThreadIdx]));
			}
		}
		
		for(auto &Thread : Threads)
		{
			Thread.join();
		}
		
		Threads.clear();
	}
	
	//NOTE: We leave it up to the caller to delete the original dataset if they want to.
	for(size_t ThreadId = 1; ThreadId < NumThreads; ++ThreadId)
	{
		delete DataSets[ThreadId];
	}
	
#else
	
	for(size_t RunID = 0; RunID < Setup->NumRuns; ++RunID)
	{
#if CALIBRATION_PRINT_DEBUG_INFO
		std::cout << "Run number: " << RunID << std::endl;
#endif
		calibration_objective &Objective = Setup->Objectives[0];

		const double *ParValues = Results->RunData[RunID].RandomParameters.data();
			
		double Performance = EvaluateObjective(DataSet, Setup->Calibration, Objective, ParValues, Setup->DiscardTimesteps);

		auto Perf = ComputeWeightedPerformance(DataSet, Performance, Objective, QuantileAccumulators, Setup->DiscardTimesteps);

		Results->RunData[RunID].PerformanceMeasures[0] = Perf;

#if CALIBRATION_PRINT_DEBUG_INFO		
		std::cout << "Performance and weighted performance for " << Objective.ModeledName << " vs " << Objective.ObservedName << " was " << std::endl << Perf.first << ", " << Perf.second << std::endl;
#endif
	}
#endif
	
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