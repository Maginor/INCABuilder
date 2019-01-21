
//NOTE: This file contains common functionality between all calibration/uncertainty analysis algorithms that want to work with our models.

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/sum.hpp>



#if !defined(CALIBRATION_PRINT_DEBUG_INFO)
#define CALIBRATION_PRINT_DEBUG_INFO 0
#endif



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
	//TODO: Mean, StandardDeviation etc. in case of other distributions.
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
EvaluateObjective(inca_data_set *DataSet, std::vector<parameter_calibration> &Calibrations, calibration_objective &Objective, const double *ParameterValues, size_t DiscardTimesteps = 0)
{
	//TODO: Evaluate multiple objectives?
	
#if CALIBRATION_PRINT_DEBUG_INFO
	std::cout << "Starting an objective evaluation" << std::endl;
#endif
	
	size_t Dimensions = Calibrations.size();
	
	for(size_t CalibIdx = 0; CalibIdx < Dimensions; ++ CalibIdx)
	{
		double Value = ParameterValues[CalibIdx];
		parameter_calibration &Calibration = Calibrations[CalibIdx];
		
		for(size_t ParIdx = 0; ParIdx < Calibration.ParameterNames.size(); ++ParIdx)
		{
			SetParameterValue(DataSet, Calibration.ParameterNames[ParIdx], Calibration.ParameterIndexes[ParIdx], Value);
		}
		
#if CALIBRATION_PRINT_DEBUG_INFO
		std::cout << "Setting " << Calibration.ParameterNames[0] << " to " << Value << std::endl; //TODO: This does not print enough info when we are setting a linked set of parameters
#endif
	}

#if CALIBRATION_PRINT_DEBUG_INFO
	timer Timer = BeginTimer();
	RunModel(DataSet);
	u64 Ms = GetTimerMilliseconds(&Timer);
	std::cout << "Running the model took " << Ms << " milliseconds" << std::endl;
#else
	RunModel(DataSet);
#endif
	
	size_t Timesteps = (size_t)DataSet->TimestepsLastRun;
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
		//NOTE: M is an extra parameter that is not a model parameter, so it is placed at the end of the ParameterValues vector. It is important that the caller of the function sets this up correctly..
		double M = ParameterValues[Dimensions];
#if CALIBRATION_PRINT_DEBUG_INFO
		std::cout << "M was set to " << M << std::endl;
#endif
		
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
	
#if CALIBRATION_PRINT_DEBUG_INFO
	std::cout << "Performance: " << Performance << std::endl << std::endl;
#endif
	
	return Performance;
}


static double
EvaluateObjectiveAndGradientSingleForwardDifference(inca_data_set *DataSet, std::vector<parameter_calibration> &Calibrations, calibration_objective &Objective, const double *ParameterValues, size_t DiscardTimesteps, double *GradientOut)
{
	//NOTE: This is a very cheap and probably not that good estimation of the gradient. It should only be used if you need the estimation to be very fast (such as if you are going to use it for each step of an MCMC run).
	size_t Dimensions = Calibrations.size();
	
	double F0 = EvaluateObjective(DataSet, Calibrations, Objective, ParameterValues, DiscardTimesteps);
	
	double *XD = (double *)malloc(sizeof(double) * Dimensions);
	
	const double Epsilon = 1e-6;
	
	for(size_t Dim = 0; Dim < Dimensions; ++Dim)
	{	
		memcpy(XD, ParameterValues, sizeof(double) * Dimensions);
		
		double H0;
		if(abs(XD[Dim]) > 1e-6)
			H0 = sqrt(XD[Dim])*Epsilon;
		else
			H0 = 1e-9; //TODO: This was just completely arbitrary, it should be done properly
		
		volatile double Temp = XD[Dim] + H0;  //Volatile so that the compiler does not optimize it away
		double H = Temp - XD[Dim];
		
		XD[Dim] += H;
		
		double FD = EvaluateObjective(DataSet, Calibrations, Objective, XD, DiscardTimesteps);
		
		GradientOut[Dim] = (FD - F0) / H;
	}
	
	free(XD);
	
	return F0;
}

