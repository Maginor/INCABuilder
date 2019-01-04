
#if !defined(INCA_GLUE_H)


#include "../calibration_setup.h"

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
	
	size_t NumThreads;
	
	std::vector<parameter_calibration> Calibration;
	
	std::vector<glue_objective> Objectives;
	
	std::vector<double> Quantiles;
};

struct glue_run_data
{
	std::vector<double> RandomParameters;                         //NOTE: Values for parameters that we want to vary only.
	std::vector<std::pair<double, double>> PerformanceMeasures;   //NOTE: One per objective.
};

struct glue_results
{
	std::vector<glue_run_data> RunData;
	std::vector<std::vector<double>> PostDistribution;
};


#include "../../sqlite3/sqlite3.h"

#include "glue_io.cpp"
#include "glue.cpp"




#define INCA_GLUE_H
#endif