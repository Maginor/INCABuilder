
//TODO: This file needs more documentation and error handling.
//NOTE: Is adaptation of https://github.com/LeahJB/SimplyP/blob/Hydrology_Model/Current_Release/v0-2A/simplyP/inputs.py

static void
AnnualThornthwaite(const std::vector<double> &MonthlyMeanT, const std::vector<double> &MonthlyMeanDLH, s32 Year, std::vector<double> &PETOut)
{
	PETOut.resize(12);
	//NOTE: Calculate heat index.
	double I = 0.0;
	for(double Tai : MonthlyMeanT)
	{
		if(Tai >= 0.0) I += pow(Tai / 5.0, 1.514);
	}
	
	double a = (6.75e-07 * I * I * I) - (7.71e-05 * I * I) + (1.792e-02 * I) + 0.49239;
	
	for(int M = 0; M < 12; ++M)
	{
		double T = MonthlyMeanT[M];
		T = T >= 0.0 ? T : 0.0;
		double L = MonthlyMeanDLH[M];
		double N = (double)MonthLength(Year, M);
		PETOut[M] = 1.6 * (L / 12.0) * (N / 30.0) * (10.0 * pow(10.0 * T / I, a));
	}
}

static void
MonthlyMeanDLH(double Latitude, s32 Year, std::vector<double> &DLHOut)
{
	DLHOut.resize(12);
	int DayOfYear = 1;
	double Ylen = 365.0 + (double)IsLeapYear(Year);
	
	for(int M = 0; M < 12; ++M)
	{
		double Dlh = 0.0;
		int Len = MonthLength(Year, M);
		for(int Day = 1; Day <= Len; ++Day)
		{
			double SD = 0.409 * sin((2.0 * Pi / Ylen)*(double)DayOfYear - 1.39);
			double CosSha = -tan(Latitude) * tan(SD);
			double Sha = acos(Min(Max(CosSha, -1.0), 1.0)); //Sunset hour angle
			Dlh += (24.0 / Pi) * Sha;
			++DayOfYear;
		}
		DLHOut[M] = Dlh / (double)Len;
	}
}


static void
ComputeThornthwaitePET(inca_data_set *DataSet)
{
	//This looks for a timeseries called "Air temperature" and computes a corresponding "Potential evapotranspiration" timeseries IF it was not provided externally from an input file.
	auto PETHandle = GetInputHandle(DataSet->Model, "Potential evapotranspiration");
	size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, PETHandle.Handle); //TODO: This is just the first instance. Should iterate over multiple instances if they exist.
	if(DataSet->InputTimeseriesWasProvided[Offset]) return; //NOTE: A PET timeseries was provided, so we don't have to compute it.
	
	double Latitude = GetParameterDouble(DataSet, "Latitude", {});
	Latitude = Latitude * Pi / 180.0; //Convert degrees to radians
	
	u64 Timesteps = DataSet->InputDataTimesteps;
	
	s32 Year, Month, Day;
	s64 Date = GetInputStartDate(DataSet);
	
	YearMonthDay(Date, &Year, &Month, &Day);
	if(Month != 1 && Day != 1)
	{
		INCA_FATAL_ERROR("ERROR: To use the Thornthwaite PET module, the input data has to start at Jan. 1st." << std::endl);
	}
	s32 StartYear = Year;
	
	s64 Date2 = Date + Timesteps*86400;
	YearMonthDay(Date2, &Year, &Month, &Day);
	if(Month != 12 && Day != 31)
	{
		INCA_FATAL_ERROR("ERROR: To use the Thornthwaite PET module, the input data has to end at Dec. 31st. It ends on " << Year << "-" << Month << "-" << Day << std::endl);
	}
	s32 EndYear = Year;
	
	std::vector<double> AirTemperature(Timesteps);
	
	GetInputSeries(DataSet, "Air temperature", {}, AirTemperature.data(), AirTemperature.size()); //TODO Should instead of being hard coded to a global value, iterate over all instances
	
	
	std::vector<double> MonthlyPET;
	
	size_t Timestep = 0;
	for(s32 Year = StartYear; Year <= EndYear; ++Year)
	{
		std::vector<double> MonthlyMeanT(12);
		for(int M = 0; M < 12; ++M)
		{
			double AirTSum = 0.0;
			int MonthLen = MonthLength(Year, M);
			for(int Day = 0; Day < MonthLen; ++Day)
			{
				AirTSum += AirTemperature[Timestep];
				++Timestep;
			}
			MonthlyMeanT[M] = AirTSum / (double)MonthLen;
		}
		
		std::vector<double> DLH;
		MonthlyMeanDLH(Latitude, Year, DLH);
		
		std::vector<double> PET;
		AnnualThornthwaite(MonthlyMeanT, DLH, Year, PET);
		
		for(int M = 0; M < 12; ++M)
		{
			int MonthLen = MonthLength(Year, M);
			PET[M] /= (double)MonthLen;         //Turn average monthly into average daily.
		}
		
		MonthlyPET.insert(MonthlyPET.begin(), PET.begin(), PET.end());
	}
	
	std::vector<double> PET(Timesteps);
	
	int MonthIndex = 0;
	Timestep = 0;
	for(s32 Year = StartYear; Year <= EndYear; ++Year)
	{
		std::vector<double> MonthlyMeanT(12);
		for(int M = 0; M < 12; ++M)
		{
			double PrevMonthValue = MonthlyPET[MonthIndex];
			if(Year >= StartYear || M > 0) PrevMonthValue = MonthlyPET[MonthIndex - 1];
			double MonthValue = MonthlyPET[MonthIndex];
			double NextMonthValue = MonthlyPET[MonthIndex];
			if(Year <= EndYear || M < 11) NextMonthValue = MonthlyPET[MonthIndex + 1];
			
			int MonthLen = MonthLength(Year, M);
			for(int Day = 0; Day < MonthLen; ++Day)
			{
				double Value;
				int Midpoint = MonthLen / 2;
				if(Day < Midpoint)
				{
					double t = (double)(Midpoint - Day) / (double)Midpoint;
					Value = 0.5*t*(PrevMonthValue + MonthValue) + (1.0 - t)*MonthValue;
				}
				else
				{
					double t = (double)(MonthLen - 1 - Day) / (double)(MonthLen - Midpoint - 1);
					Value = t * MonthValue + 0.5*(1.0 - t)*(NextMonthValue + MonthValue);
				}
				PET[Timestep] = Value;
				
				++Timestep;
			}
			
			++MonthIndex;
		}
	}
	
	SetInputSeries(DataSet, "Potential evapotranspiration", {}, PET.data(), PET.size());
}