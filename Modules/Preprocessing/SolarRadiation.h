

//NÒTE: These calculations are based on the formulas given in http://www.fao.org/3/X0490E/x0490e07.htm


inline double
ShortWaveRadiationOnAClearSkyDay(double Elevation, double ExtraterrestrialRadiation)
{
	return (0.75 + 2e-5*Elevation)*ExtraterrestrialRadiation;
}

inline double
DailyExtraTerrestrialRadiation(double Latitude, s32 DayOfYear)
{
	const double SolarConstant = 0.0820;
	
	double InverseRelativeDistanceEarthSun = 1.0 + 0.033*cos(2.0*Pi*(double)DayOfYear / 365.0);
	
	double SolarDecimation = 0.409*sin(2.0*Pi*(double)DayOfYear / 365.0 - 1.39);
	
	double LatitudeRad = Latitude * Pi / 180.0;
	
	double SunsetHourAngle = acos(-tan(LatitudeRad)*tan(SolarDecimation));
	
	return (24.0 * 60.0 / Pi) * SolarConstant * InverseRelativeDistanceEarthSun * (SunsetHourAngle * sin(LatitudeRad) * sin(SolarDecimation) + cos(LatitudeRad) * cos(SolarDecimation) * sin(SunsetHourAngle));
}

static void
ComputeSolarRadiation(inca_data_set *DataSet)
{
	//NOTE: It would be a little nonsensical to have more than one solar radiation timeseries unless you have a REeeeaAAAaaaaaLLLllYyYyyYYY long river going north-south (or opposite). OR if you have actual data in which case you don't use this precomputation.
	// We will only allow for one Latitude and one Elevation for now. If this is a problem, contact us...
	
	//NOTE: It is not strictly necessary to have this as a preprocessing step. It could be done in-model. Lets see what works out best.
	
	bool AnyNeedProcessing = false;
	ForeachInputInstance(DataSet, "Solar radiation", 
		[DataSet, &AnyNeedProcessing](const char * const *IndexNames, size_t IndexesCount)
		{
			if(!InputSeriesWasProvided(DataSet, "Solar radiation", IndexNames, IndexesCount))
			{
				AnyNeedProcessing = true;
			}
		}
	);
	
	if(!AnyNeedProcessing) return;
	
	double Latitude = GetParameterDouble(DataSet, "Latitude", {});
	double Elevation = GetParameterDouble(DataSet, "Elevation", {});
	
	u64 Timesteps = DataSet->InputDataTimesteps;
	datetime StartDate = GetInputStartDate(DataSet);
	
	std::vector<double> SolarRad(Timesteps);
	
	datetime Date = StartDate;
	
	//NOTE: This could probably be optimized by reusing computations for one single year, but it probably does not matter that much.
	for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
	{
		s32 Year, DOY;
		Date.DayOfYear(&DOY, &Year);
		
		double DETR = DailyExtraTerrestrialRadiation(Latitude, DOY);
		double SWR  = ShortWaveRadiationOnAClearSkyDay(Elevation, DETR);
		
		SolarRad[Timestep] = SWR * 11.5740741;  //NOTE: Converting MJ/m2/day to W/m2
		
		Date.AdvanceDays(1);
	}
	
	ForeachInputInstance(DataSet, "Solar radiation",
		[DataSet, &SolarRad](const char * const *IndexNames, size_t IndexesCount)
		{
			if(!InputSeriesWasProvided(DataSet, "Solar radiation", IndexNames, IndexesCount))
			{
				SetInputSeries(DataSet, "Solar radiation", IndexNames, IndexesCount, SolarRad.data(), SolarRad.size());
			}
		}
	);
}