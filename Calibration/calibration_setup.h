
//NOTE: This file is for common functionality between all calibration/uncertainty analysis

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/sum.hpp>

enum parameter_distribution_type
{
	ParameterDistribution_Uniform,
	//TODO: Support a few more!!!!
};

//NOTE: Specification of calibration of one (or several linked) parameter(s).
struct parameter_calibration
{
	std::vector<const char *> ParameterNames;
	std::vector<std::vector<const char *>> ParameterIndexes;
	parameter_distribution_type Distribution;
	double Min;
	double Max;
	double InitialGuess;
};

enum performance_measure_type
{
	PerformanceMeasure_MeanAbsoluteError,
	PerformanceMeasure_MeanSquareError,
	PerformanceMeasure_NashSutcliffe,
	PerformanceMeasure_LogLikelyhood_ProportionalNormal,
};

//IMPORTANT: Remember to keep the following functions updated as new objectives are added in!
inline bool
ShouldMaximize(performance_measure_type Type) //Otherwise it should be minimized
{
	return Type == PerformanceMeasure_NashSutcliffe;
}

inline bool
IsLogLikelyhoodMeasure(performance_measure_type Type)
{
	return Type == PerformanceMeasure_LogLikelyhood_ProportionalNormal;
}



struct calibration_objective
{
	performance_measure_type PerformanceMeasure;
	
	const char *ModeledName;
	std::vector<const char *> ModeledIndexes;
	const char *ObservedName;
	std::vector<const char *> ObservedIndexes;
	
	//NOTE: These are only used if the calibration system is weighting the performance measure:
	double Threshold;
	double OptimalValue;
};

const u64 ParameterCalibrationReadDistribution   = 0x1;
const u64 ParameterCalibrationReadInitialGuesses = 0x2;

static void
ReadParameterCalibration(token_stream &Stream, std::vector<parameter_calibration> &CalibOut, u64 Flags = 0)
{
	//NOTE: This function assumes that the Stream has been opened and has already consumed the tokens 'parameter_calibration' and ':'
	
	while(true)
	{
		bool WeAreInLink = false;
		parameter_calibration Calib = {};
		
		while(true)
		{
			token *Token = Stream.PeekToken();
			if(Token->Type == TokenType_QuotedString)
			{
				const char *ParameterName = CopyString(Token->StringValue); //NOTE: The copied string leaks unless somebody frees it later
				
				Calib.ParameterNames.push_back(ParameterName);  
				Stream.ReadToken(); //NOTE: Consumes the token we peeked.
				
				std::vector<const char *> Indexes;
				ReadQuotedStringList(Stream, Indexes, true); //NOTE: The copied strings leak unless we free them later
				Calib.ParameterIndexes.push_back(Indexes);
				
				if(!WeAreInLink) break;
			}
			else if(Token->Type == TokenType_UnquotedString)
			{
				if(WeAreInLink)
				{
					Stream.PrintErrorHeader();
					std::cout << "Unexpected token inside link." << std::endl;
					exit(0);
				}
				
				if(strcmp(Token->StringValue, "link") == 0)
				{
					WeAreInLink = true;
					Stream.ReadToken(); //NOTE: Consumes the token we peeked.
					Stream.ExpectToken(TokenType_OpenBrace);
				}
				else
				{
					//NOTE: We hit another code word that signifies a new section of the file.
					//NOTE: Here we should not consume the peeked token since the caller needs to be able to read it.
					return;
				}
			}
			else if(WeAreInLink && Token->Type == TokenType_CloseBrace)
			{
				Stream.ReadToken(); //NOTE: Consumes the token we peeked.
				break;
			}
			else if(Token->Type == TokenType_EOF)
			{
				if(WeAreInLink)
				{
					Stream.PrintErrorHeader();
					std::cout << "File ended unexpectedly." << std::endl;
					exit(0);
				}
				
				return;
			}
			else
			{
				Stream.PrintErrorHeader();
				std::cout << "Unexpected token." << std::endl;
				exit(0);
			}
		}
		WeAreInLink = false;
		
		if(Flags & ParameterCalibrationReadDistribution)
		{
			const char *DistrName = Stream.ExpectUnquotedString();
			if(strcmp(DistrName, "uniform") == 0)
			{
				Calib.Distribution = ParameterDistribution_Uniform;
			}
			//else if ...
			else
			{
				Stream.PrintErrorHeader();
				std::cout << "Unsupported distribution: " << DistrName << std::endl;
				exit(0);
			}
		}
		
		Calib.Min = Stream.ExpectDouble();
		Calib.Max = Stream.ExpectDouble();
		if(Flags & ParameterCalibrationReadInitialGuesses)
		{
			Calib.InitialGuess = Stream.ExpectDouble();
		}
		//TODO: Optional distribution?
		
		CalibOut.push_back(Calib);
		
	}
}

static void
ReadCalibrationObjectives(token_stream &Stream, std::vector<calibration_objective> &ObjectivesOut, bool ReadWeightingInfo = false)
{
	//NOTE: This function assumes that the Stream has been opened and has already consumed the tokens 'parameter_calibration' and ':'
	
	while(true)
	{
		calibration_objective Objective = {};
		
		token *Token = Stream.PeekToken();
		if(Token->Type == TokenType_QuotedString)
		{
			Objective.ModeledName = CopyString(Stream.ExpectQuotedString());
			ReadQuotedStringList(Stream, Objective.ModeledIndexes, true);
			
			Objective.ObservedName = CopyString(Stream.ExpectQuotedString());
			ReadQuotedStringList(Stream, Objective.ObservedIndexes, true);
			
			const char *PerformanceMeasure = Stream.ExpectUnquotedString();
			if(strcmp(PerformanceMeasure, "mean_absolute_error") == 0)
			{
				Objective.PerformanceMeasure = PerformanceMeasure_MeanAbsoluteError;
			}
			else if(strcmp(PerformanceMeasure, "mean_square_error") == 0)
			{
				Objective.PerformanceMeasure = PerformanceMeasure_MeanSquareError;
			}
			else if(strcmp(PerformanceMeasure, "nash_sutcliffe") == 0)
			{
				Objective.PerformanceMeasure = PerformanceMeasure_NashSutcliffe;
			}
			else if(strcmp(PerformanceMeasure, "ll_proportional_normal") == 0)
			{
				Objective.PerformanceMeasure = PerformanceMeasure_LogLikelyhood_ProportionalNormal;
			}
			else assert(0);
			
			if(ReadWeightingInfo)
			{
				Objective.Threshold = Stream.ExpectDouble();
				Objective.OptimalValue = Stream.ExpectDouble();
			}
			
			ObjectivesOut.push_back(Objective);
		}
		else break;
	}
}

static double
EvaluateObjective(inca_data_set *DataSet, calibration_objective &Objective, size_t DiscardTimesteps = 0, double M = 0.0)
{
	size_t Timesteps = (size_t)GetTimesteps(DataSet);
	std::vector<double> ModeledSeries(Timesteps);
	std::vector<double> ObservedSeries(Timesteps);
	
	GetResultSeries(DataSet, Objective.ModeledName, Objective.ModeledIndexes, ModeledSeries.data(), ModeledSeries.size());
	GetInputSeries(DataSet, Objective.ObservedName, Objective.ObservedIndexes, ObservedSeries.data(), ObservedSeries.size(), true); //NOTE: It is a little wasteful reading a copy of this at every evaluation since it will be the same every time...
	
	std::vector<double> Residuals(Timesteps);
	
	for(size_t Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
	{
		Residuals[Timestep] = ModeledSeries[Timestep] - ObservedSeries[Timestep];
	}
	
	double Performance;
	
	using namespace boost::accumulators;
	
	if(Objective.PerformanceMeasure == PerformanceMeasure_MeanAbsoluteError)
	{
		accumulator_set<double, features<tag::mean>> Accumulator;
		
		for(u64 Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
		{
			if(!std::isnan(Residuals[Timestep]))
				Accumulator(std::abs(Residuals[Timestep]));
		}
		
		Performance = mean(Accumulator);
	}
	else if(Objective.PerformanceMeasure == PerformanceMeasure_MeanSquareError)
	{
		accumulator_set<double, features<tag::mean>> Accumulator;
		
		for(u64 Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
		{
			double Res = Residuals[Timestep];
			if(!std::isnan(Res))
				Accumulator(Res*Res);
		}
		
		Performance = mean(Accumulator);
	}
	else if(Objective.PerformanceMeasure == PerformanceMeasure_NashSutcliffe)
	{
		accumulator_set<double, features<tag::variance>> ObsAccum;
		accumulator_set<double, features<tag::mean>> ResAccum;

		for(u64 Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
		{
			double Obs = ObservedSeries[Timestep];
			double Res = Residuals[Timestep];
			if(!std::isnan(Res))
			{
				ObsAccum(Obs);	
				ResAccum(Res*Res);
			}
		}
		
		double MeanSquaresResidual = mean(ResAccum);
		double ObservedVariance = variance(ObsAccum);
		
		Performance = 1.0 - MeanSquaresResidual / ObservedVariance;
	}
	else if(Objective.PerformanceMeasure == PerformanceMeasure_LogLikelyhood_ProportionalNormal)
	{
		accumulator_set<double, stats<tag::sum>> LogLikelyhoodAccum;
	
		for(size_t Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
		{
			double Sim = ModeledSeries[Timestep];
			double Res = Residuals[Timestep];
			if(!std::isnan(Res)) //TODO: We should do something else upon NaN in the sim (return -inf?). NaN in obs just means that we don't have a value for that timestep, and so we skip it.
			{
				double Sigma = M*Sim;
				double Like = -0.5*std::log(2.0*Pi*Sigma*Sigma) - Res*Res / (2*Sigma*Sigma);
				LogLikelyhoodAccum(Like);
			}
		}
		
		Performance = sum(LogLikelyhoodAccum);
	}
	else assert(0);
	
	return Performance;
}

inline void
SetCalibrationValue(inca_data_set *DataSet, parameter_calibration &Calibration, double Value)
{
	for(size_t ParIdx = 0; ParIdx < Calibration.ParameterNames.size(); ++ParIdx)
	{
		SetParameterValue(DataSet, Calibration.ParameterNames[ParIdx], Calibration.ParameterIndexes[ParIdx], Value);
	}
}