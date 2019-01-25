

//NOTE: This is an adaption of
// https://github.com/LeahJB/SimplyP

//NOTE: There are still a few quirks that causes this version to produce slightly (but not that significantly) different results.


//NOTE: Only include these if you are going to use them (they cause long compile times):
//#include "../boost_solvers.h"
//#include "../mtl_solvers.h"


inline double
ConvertMmPerDayToM3PerDay(double MmPerDay, double CatchmentArea)
{
	return MmPerDay * 1000.0 * CatchmentArea;
}

inline double
ConvertM3PerSecondToMmPerDay(double M3PerSecond, double CatchmentArea)
{
	return M3PerSecond * 86400.0 / (1000.0 * CatchmentArea);
}

inline double
ConvertMmPerDayToM3PerSecond(double MmPerDay, double CatchmentArea)
{
	return MmPerDay * CatchmentArea / 86.4;
}

inline double
ConvertKgPerMmToMgPerL(double KgPerMm, double CatchmentArea)
{
	return KgPerMm / CatchmentArea;
}

inline double
ConvertMgPerLToKgPerMm(double MgPerL, double CatchmentArea)
{
	return MgPerL * CatchmentArea;
}

inline double
ConvertMmToM3(double Mm, double CatchmentArea)
{
	return Mm * 1000.0 * CatchmentArea;
}

inline double
ConvertMmToLitres(double Mm, double CatchmentArea)
{
	return Mm * 1e6 * CatchmentArea;
}

inline double
ActivationControl0(double X)
{
	return (3.0 - 2.0*X)*X*X;
}

inline double
ActivationControl(double X, double Threshold, double RelativeActivationDistance)
{
	if(X < Threshold) return 0.0;
	double Dist = Threshold * RelativeActivationDistance;
	if(X > Threshold + Dist) return 1.0;
	return ActivationControl0( (X - Threshold) / Dist );
}

inline double
LinInterp(double X, double X0, double X1, double Y0, double Y1)
{
	return Y0 + (Y1 - Y0)*(X - X0) / (X1 - X0);
}

static void
AddSimplyPHydrologyModule(inca_model *Model)
{
	auto Dimensionless     = RegisterUnit(Model);
	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto Days              = RegisterUnit(Model, "days");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto PerM3             = RegisterUnit(Model, "/m3");
	auto M3PerSecond       = RegisterUnit(Model, "m3/s");
	auto Km2               = RegisterUnit(Model, "km2");
	auto M                 = RegisterUnit(Model, "m");
	
	auto Reach = RegisterIndexSetBranched(Model, "Reaches");
	
	auto Snow = RegisterParameterGroup(Model, "Snow", Reach);
	
	auto InitialSnowDepth        = RegisterParameterDouble(Model, Snow, "Initial snow depth", Mm, 0.0, 0.0, 50000.0);
	auto DegreeDayFactorSnowmelt = RegisterParameterDouble(Model, Snow, "Degree-day factor for snowmelt", MmPerDegreePerDay, 2.74, 0.0, 5.0);
	
	auto Hydrology = RegisterParameterGroup(Model, "Hydrology", Reach);
	
	auto ProportionToQuickFlow   = RegisterParameterDouble(Model, Hydrology, "Proportion of precipitation that contributes to quick flow", Dimensionless, 0.020, 0.0, 1.0);
	auto PETReductionFactor      = RegisterParameterDouble(Model, Hydrology, "PET reduction factor", Dimensionless, 1.0, 0.0, 1.0);
	auto SoilFieldCapacity       = RegisterParameterDouble(Model, Hydrology, "Soil field capacity", Mm, 290.0, 0.0, 5000.0);
	auto BaseflowIndex           = RegisterParameterDouble(Model, Hydrology, "Baseflow index", Dimensionless, 0.70, 0.0, 1.0);
	auto GroundwaterTimeConstant = RegisterParameterDouble(Model, Hydrology, "Groundwater time constant", Days, 65.0, 0.5, 400.0);
	auto MinimumGroundwaterFlow  = RegisterParameterDouble(Model, Hydrology, "Minimum groundwater flow", MmPerDay, 0.40, 0.0, 10.0);
	auto A                       = RegisterParameterDouble(Model, Hydrology, "Gradient of stream velocity-discharge relationship", PerM3, 0.5, 0.00001, 0.99, "The a in V = aQ^b");
	auto B                       = RegisterParameterDouble(Model, Hydrology, "Exponent of stream velocity-discharge relationship", Dimensionless, 0.42, 0.1, 0.99, "The b in V = aQ^b");
	
	auto Reaches = RegisterParameterGroup(Model, "Reaches", Reach);
	
	auto CatchmentArea           = RegisterParameterDouble(Model, Reaches, "Catchment area", Km2, 51.7, 0.0, 10000.0);
	auto ReachLength             = RegisterParameterDouble(Model, Reaches, "Reach length", M, 10000.0, 0.0, 10000000.0);
	auto InitialInStreamFlow     = RegisterParameterDouble(Model, Reaches, "Initial in-stream flow", M3PerSecond, 1.0, 0.0, 1000000.0, "This parameter is only used by reaches that don't have other reaches as inputs.");
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	auto Arable             = RequireIndex(Model, LandscapeUnits, "Arable");
	auto Seminatural        = RequireIndex(Model, LandscapeUnits, "Semi-natural");
	auto ImprovedGrassland  = RequireIndex(Model, LandscapeUnits, "Improved grassland");
	
	auto HydrologyLand = RegisterParameterGroup(Model, "Hydrology land", LandscapeUnits);
	
	auto SoilWaterTimeConstant   = RegisterParameterDouble(Model, HydrologyLand, "Soil water time constant", Days, 2.0, 0.05, 40.0);
	
	auto LandUseSubcatchment = RegisterParameterGroup(Model, "Land use for subcatchment", LandscapeUnits);
	SetParentGroup(Model, LandUseSubcatchment, Reaches);
	
	auto LandUseProportions   = RegisterParameterDouble(Model, LandUseSubcatchment, "Land use proportions", Dimensionless, 0.5, 0.0, 1.0);
	
	auto Precipitation  = RegisterInput(Model, "Precipitation");
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	
	auto PrecipitationFallingAsSnow = RegisterEquation(Model, "Precipitation falling as snow", MmPerDay);
	auto PrecipitationFallingAsRain = RegisterEquation(Model, "Precipitation falling as rain", MmPerDay);
	auto PotentialDailySnowmelt     = RegisterEquation(Model, "Potential daily snowmelt", MmPerDay);
	auto SnowMelt                   = RegisterEquation(Model, "Snow melt", MmPerDay);
	auto SnowDepth                  = RegisterEquation(Model, "Snow depth", Mm);
	SetInitialValue(Model, SnowDepth, InitialSnowDepth);
	auto HydrologicalInputToSoilBox = RegisterEquation(Model, "Hydrological input to soil box", MmPerDay);
	
	EQUATION(Model, PrecipitationFallingAsSnow,
		double precip = INPUT(Precipitation);
		return (INPUT(AirTemperature) < 0) ? precip : 0.0;
	)
	
	EQUATION(Model, PrecipitationFallingAsRain,
		double precip = INPUT(Precipitation);
		return (INPUT(AirTemperature) > 0) ? precip : 0.0;
	)
	
	EQUATION(Model, PotentialDailySnowmelt,
		return Max(0.0, PARAMETER(DegreeDayFactorSnowmelt) * INPUT(AirTemperature));
	)
	
	EQUATION(Model, SnowMelt,
		return Min(LAST_RESULT(SnowDepth), RESULT(PotentialDailySnowmelt));
	)
	
	EQUATION(Model, SnowDepth,
		return LAST_RESULT(SnowDepth) + RESULT(PrecipitationFallingAsSnow) - RESULT(SnowMelt);
	)
	
	EQUATION(Model, HydrologicalInputToSoilBox,
		return RESULT(SnowMelt) + RESULT(PrecipitationFallingAsRain);
	)
	
	auto PotentialEvapoTranspiration = RegisterInput(Model, "Potential evapotranspiration");
	
	auto InfiltrationExcess = RegisterEquation(Model, "Infiltration excess", MmPerDay);
	auto Infiltration       = RegisterEquation(Model, "Infiltration", MmPerDay);
	
	EQUATION(Model, InfiltrationExcess,
		return PARAMETER(ProportionToQuickFlow) * RESULT(HydrologicalInputToSoilBox);
	)
	
	EQUATION(Model, Infiltration,
		return (1.0 - PARAMETER(ProportionToQuickFlow)) * RESULT(HydrologicalInputToSoilBox);
	)
	
	auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.01 /* 1.0/20000.0 */, IncaDascru);
	//auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.001, BoostRK4);
	//auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.001, BoostCashCarp54, 1e-6, 1e-6);
	//auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.001, BoostRosenbrock4, 1e-3, 1e-3);
	//auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.0025, Mtl4ImplicitEuler);   //NOTE: Being a first order method, this one is not that good..
	
	//NOTE: Ideally we would want the soil water volume equations to just be one equation that is autoindexed over landscape units, but that would create a difficulty when merging outflow from the landscape units to the reach as we could not do that inside the same solver (currently). Also, we could not let one instance of the calculation span both Arable and Improved-grassland as is done with Agricultural here.
	

	auto AgriculturalSoilWaterFlow = RegisterEquation(Model, "Agricultural soil water flow", MmPerDay);
	SetSolver(Model, AgriculturalSoilWaterFlow, SimplyPSolver);
	
	auto AgriculturalSoilWaterVolume = RegisterEquationODE(Model, "Agricultural soil water volume", Mm);
	SetInitialValue(Model, AgriculturalSoilWaterVolume, SoilFieldCapacity);
	SetSolver(Model, AgriculturalSoilWaterVolume, SimplyPSolver);
	
	EQUATION(Model, AgriculturalSoilWaterFlow,
		double smd = PARAMETER(SoilFieldCapacity) - RESULT(AgriculturalSoilWaterVolume);
		//return - smd / (PARAMETER(SoilWaterTimeConstant, Arable) * (1.0 + exp(smd)));
		return -smd * ActivationControl(RESULT(AgriculturalSoilWaterVolume), PARAMETER(SoilFieldCapacity), 0.01) / PARAMETER(SoilWaterTimeConstant, Arable);
	)
	
	EQUATION(Model, AgriculturalSoilWaterVolume,
		// mu = -np.log(0.01)/p['fc']
		//P*(1-f_quick) - alpha*E*(1 - np.exp(-mu*VsA_i)) - QsA_i
		return
			  RESULT(Infiltration)
			- PARAMETER(PETReductionFactor) * INPUT(PotentialEvapoTranspiration) * (1.0 - exp(log(0.01) * RESULT(AgriculturalSoilWaterVolume) / PARAMETER(SoilFieldCapacity))) //NOTE: Should 0.01 be a parameter?
			- RESULT(AgriculturalSoilWaterFlow);	
	)
	
	
	
	auto SeminaturalSoilWaterFlow = RegisterEquation(Model, "Semi-natural soil water flow", MmPerDay);
	SetSolver(Model, SeminaturalSoilWaterFlow, SimplyPSolver);
	
	auto SeminaturalSoilWaterVolume = RegisterEquationODE(Model, "Semi-natural soil water volume", Mm);
	SetInitialValue(Model, SeminaturalSoilWaterVolume, SoilFieldCapacity);
	SetSolver(Model, SeminaturalSoilWaterVolume, SimplyPSolver);
	
	EQUATION(Model, SeminaturalSoilWaterFlow,
		double smd = PARAMETER(SoilFieldCapacity) - RESULT(SeminaturalSoilWaterVolume);
		//return - smd / (PARAMETER(SoilWaterTimeConstant, Seminatural) * (1.0 + exp(smd)));
		return - smd * ActivationControl(RESULT(SeminaturalSoilWaterVolume), PARAMETER(SoilFieldCapacity), 0.01) / PARAMETER(SoilWaterTimeConstant, Seminatural);
	)
	
	EQUATION(Model, SeminaturalSoilWaterVolume,
		// mu = -np.log(0.01)/p['fc']
		//P*(1-f_quick) - alpha*E*(1 - np.exp(-mu*VsA_i)) - QsA_i
		return
			  RESULT(Infiltration)
			- PARAMETER(PETReductionFactor) * INPUT(PotentialEvapoTranspiration) * (1.0 - exp(log(0.01) * RESULT(AgriculturalSoilWaterVolume) / PARAMETER(SoilFieldCapacity))) //NOTE: Should 0.01 be a parameter?
			- RESULT(SeminaturalSoilWaterFlow);	
	)
	
	
	
	auto TotalSoilWaterFlow       = RegisterEquation(Model, "Total soil water flow", MmPerDay);
	SetSolver(Model, TotalSoilWaterFlow, SimplyPSolver);
	
	EQUATION(Model, TotalSoilWaterFlow,
		double f_A = PARAMETER(LandUseProportions, Arable) + PARAMETER(LandUseProportions, ImprovedGrassland);
		double f_S  = PARAMETER(LandUseProportions, Seminatural);
		return f_A * RESULT(AgriculturalSoilWaterFlow) + f_S * RESULT(SeminaturalSoilWaterFlow);
	)
	

	auto InitialGroundwaterVolume = RegisterEquationInitialValue(Model, "Initial groundwater volume", Mm);
	auto GroundwaterVolume        = RegisterEquationODE(Model, "Groundwater volume", Mm);
	SetInitialValue(Model, GroundwaterVolume, InitialGroundwaterVolume);
	SetSolver(Model, GroundwaterVolume, SimplyPSolver);
	
	auto GroundwaterFlow          = RegisterEquation(Model, "Groundwater flow", MmPerDay);
	SetSolver(Model, GroundwaterFlow, SimplyPSolver);
	
	EQUATION(Model, GroundwaterFlow,
		double flow0   = RESULT(GroundwaterVolume) / PARAMETER(GroundwaterTimeConstant);
		double flowmin = PARAMETER(MinimumGroundwaterFlow);
		double t = ActivationControl(flow0, flowmin, 0.01);
		return (1.0 - t)*flowmin + t*flow0;
	)
	
	EQUATION(Model, GroundwaterVolume,
		// f_A = f_IG + f_Ar
		// dVg_dt = beta*(f_A*QsA_i + f_S*QsS_i) - Qg_i
		
		return PARAMETER(BaseflowIndex) * RESULT(TotalSoilWaterFlow)
			- RESULT(GroundwaterFlow);
	)
	
	EQUATION(Model, InitialGroundwaterVolume,
		//Vg0 = Qg0 *p['T_g']     # Groundwater vol (mm)
		double initialflow = PARAMETER(BaseflowIndex) * ConvertM3PerSecondToMmPerDay(PARAMETER(InitialInStreamFlow), PARAMETER(CatchmentArea));
		return initialflow * PARAMETER(GroundwaterTimeConstant);
	)
	
	auto Control = RegisterEquation(Model, "Control", Dimensionless);
	
	EQUATION(Model, Control,
		//NOTE: We create this equation to put in the code that allow us to "hack" certain values.
		// The return value of this equation does not mean anything.
		
		double volume = RESULT(GroundwaterFlow)*PARAMETER(GroundwaterTimeConstant);  //Wow, somehow this does not register index sets correctly if it is passed directly inside the macro below! May want to debug that.
		SET_RESULT(GroundwaterVolume, volume);
		
		return 0.0;
	)
	
	auto ReachFlowInput    = RegisterEquation(Model, "Reach flow input", MmPerDay);
	SetSolver(Model, ReachFlowInput, SimplyPSolver);
	
	auto InitialReachVolume = RegisterEquationInitialValue(Model, "Initial reach volume", Mm); 
	auto ReachVolume        = RegisterEquationODE(Model, "Reach volume", Mm);
	SetInitialValue(Model, ReachVolume, InitialReachVolume);
	SetSolver(Model, ReachVolume, SimplyPSolver);
	
	auto InitialReachFlow   = RegisterEquationInitialValue(Model, "Initial reach flow", MmPerDay);
	auto ReachFlow          = RegisterEquationODE(Model, "Reach flow", MmPerDay);
	SetInitialValue(Model, ReachFlow, InitialReachFlow);
	SetSolver(Model, ReachFlow, SimplyPSolver);
	
	auto DailyMeanReachFlow = RegisterEquationODE(Model, "Daily mean reach flow", MmPerDay);
	SetInitialValue(Model, DailyMeanReachFlow, 0.0);
	SetSolver(Model, DailyMeanReachFlow, SimplyPSolver);
	ResetEveryTimestep(Model, DailyMeanReachFlow);
	
	EQUATION(Model, ReachFlowInput,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(DailyMeanReachFlow, *Input) * PARAMETER(CatchmentArea, *Input) / PARAMETER(CatchmentArea);
		)
		
		return upstreamflow + RESULT(InfiltrationExcess) + (1.0 - PARAMETER(BaseflowIndex)) * RESULT(TotalSoilWaterFlow) + RESULT(GroundwaterFlow);
	)
	
	EQUATION(Model, InitialReachFlow,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(ReachFlow, *Input) * PARAMETER(CatchmentArea, *Input) / PARAMETER(CatchmentArea);
		)
		double initflow = ConvertM3PerSecondToMmPerDay(PARAMETER(InitialInStreamFlow), PARAMETER(CatchmentArea));

		if(INPUT_COUNT(Reach) == 0) return initflow;
		else return upstreamflow;
	)
	
	EQUATION(Model, ReachFlow,
		//dQr_dt = ((Qq_i + (1-beta)*(f_A*QsA_i + f_S*QsS_i) + Qg_i + Qr_US_i - Qr_i) # Fluxes (mm/d)
              //*a_Q*(Qr_i**b_Q)*(8.64*10**4)/((1-b_Q)*(L_reach)))
		return
			(RESULT(ReachFlowInput) - RESULT(ReachFlow))
			* PARAMETER(A) * pow(RESULT(ReachFlow), PARAMETER(B))*86400.0 / ((1.0-PARAMETER(B))*PARAMETER(ReachLength));
	)
	
	EQUATION(Model, InitialReachVolume,
		//Tr0 = ((p_SC.ix['L_reach',SC])/
                   //(p['a_Q']*(Qr0**p['b_Q'])*(8.64*10**4))) # Reach time constant (days); T=L/aQ^b
        //Vr0 = Qr0*Tr0 # Reach volume (V=QT) (mm)
		double initialreachtimeconstant = PARAMETER(ReachLength) / (PARAMETER(A) * pow(RESULT(ReachFlow), PARAMETER(B)) * 86400.0);
		
		return RESULT(ReachFlow) * initialreachtimeconstant;
	)
	
	EQUATION(Model, ReachVolume,
		//dVr_dt = Qq_i + (1-beta)*(f_A*QsA_i + f_S*QsS_i) + Qg_i + Qr_US_i - Qr_i
		return RESULT(ReachFlowInput) - RESULT(ReachFlow);
	)
	
	EQUATION(Model, DailyMeanReachFlow,
		//NOTE: Since DailyMeanReachFlow is reset to start at 0 every timestep and since its derivative is the reach flow, its value becomes the integral of the reach flow over the timestep, i.e. the daily mean value.
		return RESULT(ReachFlow);
	)
}


static void
AddSimplyPSedimentModule(inca_model *Model)
{
	auto Dimensionless = RegisterUnit(Model);
	auto Kg            = RegisterUnit(Model, "kg");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto KgPerMm       = RegisterUnit(Model, "kg/mm");
	auto JulianDay     = RegisterUnit(Model, "Julian day");
	auto Degrees       = RegisterUnit(Model, "°");
	auto MgPerL        = RegisterUnit(Model, "mg/l");
	
	
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	
	auto Arable             = RequireIndex(Model, LandscapeUnits, "Arable");
	auto Seminatural        = RequireIndex(Model, LandscapeUnits, "Semi-natural");
	auto ImprovedGrassland  = RequireIndex(Model, LandscapeUnits, "Improved grassland");
	
	auto Sediment = RegisterParameterGroup(Model, "Sediment", Reach);
	auto ReachSedimentInputScalingFactor         = RegisterParameterDouble(Model, Sediment, "Reach sediment input scaling factor", KgPerMm, 1500.0, 0.0, 100000.0);
	auto InstreamEntrainmentNonlinearCoefficient = RegisterParameterDouble(Model, Sediment, "Instream entrainment non-linear coefficient", Dimensionless, 2.0, 0.1, 5.0); 
	auto DayOfYearWhenSoilErodibilityIsMaxSpring = RegisterParameterUInt(Model, Sediment, "Day of year when soil erodibility is at its max for spring-grown crops", JulianDay, 60, 30, 335);
	auto DayOfYearWhenSoilErodibilityIsMaxAutumn = RegisterParameterUInt(Model, Sediment, "Day of year when soil erodibility is at its max for autumn-grown crops", JulianDay, 304, 30, 335);
	
	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	auto ReachSlope                              = RegisterParameterDouble(Model, Reaches, "Reach slope", Degrees, 0.8, 0.0, 90.0);
	auto ProportionOfSpringGrownCrops            = RegisterParameterDouble(Model, Reaches, "Proportion of spring grown crops", Dimensionless, 0.65, 0.0, 1.0, "Proportion spring-sown crops to make total arable land area (assume rest is autumn-sown)");
	
	auto LandUseSubcatchment = GetParameterGroupHandle(Model, "Land use for subcatchment");
	auto MeanSlopeOfLand                         = RegisterParameterDouble(Model, LandUseSubcatchment, "Mean slope of land in the subcatchment", Degrees, 4.0, 0.0, 90.0);
	
	auto SedimentLand = RegisterParameterGroup(Model, "Sediment land", LandscapeUnits);
	auto VegetationCoverFactor                   = RegisterParameterDouble(Model, SedimentLand, "Vegetation cover factor", Dimensionless, 0.2, 0.0, 1.0, "Vegetation cover factor, describing ratio between long-term erosion under the land use class, compared to under bare soil of the same soil type, slope, etc. Sourced from (R)USLE literature.");
	auto ReductionOfLoadInSediment               = RegisterParameterDouble(Model, SedimentLand, "Reduction of load in sediment", Dimensionless, 0.0, 0.0, 1.0, "Reduction in load of sediment delivered to the stream due to management measures, e.g. buffer strips, filter fences, conservation tillage practices, etc.");
	
	auto DynamicOptions = GetParameterGroupHandle(Model, "Dynamic options");
	auto DynamicErodibility                      = RegisterParameterBool(Model, DynamicOptions, "Dynamic erodibility", true);
	
	auto LandUseProportions = GetParameterDoubleHandle(Model, "Land use proportions");
	
	
	
	
	auto SimplyPSolver = GetSolverHandle(Model, "SimplyP solver");
	
	auto ReachFlow          = GetEquationHandle(Model, "Reach flow");
	auto DailyMeanReachFlow = GetEquationHandle(Model, "Daily mean reach flow");
	auto ReachVolume        = GetEquationHandle(Model, "Reach volume");
	
	auto TimeDependentVegetationCoverFactor = RegisterEquation(Model, "Time dependent vegetation cover factor", Dimensionless);
	auto ReachSedimentInputFromLand         = RegisterEquation(Model, "Reach sediment input from land", KgPerDay);
	auto TotalReachSedimentInputFromLand    = RegisterEquationCumulative(Model, "Total reach sediment input from land", ReachSedimentInputFromLand, LandscapeUnits);
	
	auto SuspendedSedimentFlux = RegisterEquation(Model, "Suspended sediment flux", KgPerDay);
	SetSolver(Model, SuspendedSedimentFlux, SimplyPSolver);
	
	auto SuspendedSedimentMass = RegisterEquationODE(Model, "Suspended sediment mass", Kg);
	SetInitialValue(Model, SuspendedSedimentMass, 0.0);
	SetSolver(Model, SuspendedSedimentMass, SimplyPSolver);
	
	auto DailyMeanSuspendedSedimentFlux = RegisterEquationODE(Model, "Daily mean suspended sediment flux", KgPerDay);
	SetInitialValue(Model, DailyMeanSuspendedSedimentFlux, 0.0);
	SetSolver(Model, DailyMeanSuspendedSedimentFlux, SimplyPSolver);
	ResetEveryTimestep(Model, DailyMeanSuspendedSedimentFlux);
	
	auto SuspendedSedimentConcentration = RegisterEquation(Model, "Suspended sediment concentration (volume weighted daily mean)", MgPerL);
	
	EQUATION(Model, TimeDependentVegetationCoverFactor,
		
		/*
		if LU == 'A':
			# If arable land, work out a dynamic crop cover factor, to account for the variation
			# in erodibility through the year due to harvesting and planting practices.
			if dynamic_options['Dynamic_erodibility'] == 'y':
				
				# Using a sine wave to simulate the annual change in erodibility
#                         C_spr_t = p_LU[LU]['C_cover']*(np.cos((2*np.pi/365)*(dayNo-p['d_maxE_spr']))+1)
#                         C_aut_t = p_LU[LU]['C_cover']*(np.cos((2*np.pi/365)*(dayNo-p['d_maxE_aut']))+1)
#                         C_cover = (p_SC.ix['f_spr',SC]*C_spr_t +(1-p_SC.ix['f_spr',SC])*C_aut_t)
				
				# Using a triangular wave
				C_cov_dict = {} # Dict for storing spring & autumn results in
				for s in ['spr','aut']:  # Loop through seasons
					d = Erisk_dict # Defined above; dict defining high erosion risk period
					d_start, d_end, d_mid = d[s]['start'], d[s]['end'], d[s]['mid']
					if dayNo in np.arange(d_start, d_end):
						if dayNo < d_mid: # If within high risk period, before mid-point
							C_season = lin_interp(dayNo, x0=d_start, x1=d_mid,
											 y0=p_LU[LU]['C_cover'], y1=1.0)
						else: # If in high risk period, after mid-point
							C_season = lin_interp(dayNo, x0=d_mid, x1=d_end,
											 y0=1.0, y1=p_LU[LU]['C_cover'])
					else: # Otherwise, outside high risk period
						C_season = (p_LU[LU]['C_cover']-(E_risk_period*(1-p_LU[LU]['C_cover'])
								   /(2*(365-E_risk_period))))
					C_cov_dict[s] = C_season
				# Average the dynamic factor over spring and autumn-sown crops
				C_cover = (p_SC.ix['f_spr',SC]*C_cov_dict['spr']
						   + (1-p_SC.ix['f_spr',SC])*C_cov_dict['aut'])  
				
			else:  # If not calculating a dynamic crop cover, then just assign user parameter
				C_cover = p_LU[LU]['C_cover']
				
			C_cover_A = C_cover  # Store this for arable land, for checking
		
		else:  # For non-arable LU, the cover factor is always constant throughout the year
			C_cover = p_LU[LU]['C_cover']
			*/
			
		double C_cover = PARAMETER(VegetationCoverFactor);
		
		double d_maxE[2];
		d_maxE[0] = (double)PARAMETER(DayOfYearWhenSoilErodibilityIsMaxSpring);
		d_maxE[1] = (double)PARAMETER(DayOfYearWhenSoilErodibilityIsMaxAutumn);
		
		double coverproportion[2];
		coverproportion[0] = PARAMETER(ProportionOfSpringGrownCrops);
		coverproportion[1] = 1.0 - coverproportion[0];
		
		//NOTE: Sine wave not implemented, only triangular.
		if(PARAMETER(DynamicErodibility) && (CURRENT_INDEX(LandscapeUnits) == Arable))
		{
			double C_cov = 0.0;
			
			double E_risk_period = 60.0;
			for(size_t Season = 0; Season < 2; ++Season)
			{
				double d_start = d_maxE[Season] - E_risk_period / 2.0;
				double d_end   = d_maxE[Season] + E_risk_period / 2.0;
				double d_mid   = d_maxE[Season];
				
				double dayNo = (double)CURRENT_DAY_OF_YEAR();
				
				double C_season;
				if(dayNo >= d_start && dayNo <= d_end)
				{
					if(dayNo < d_mid) C_season = LinInterp(dayNo, d_start, d_mid, C_cover, 1.0);
					else              C_season = LinInterp(dayNo, d_mid,   d_end, 1.0, C_cover);
				}
				else C_season = C_cover - E_risk_period*(1.0 - C_cover)/(2.0*(DAYS_THIS_YEAR()-E_risk_period));
				
				C_cov += C_season * coverproportion[Season];
			}
			
			return C_cov;
		}

		return C_cover;
	)
	
	EQUATION(Model, ReachSedimentInputFromLand,
		//# Reach sed input coefficient per land use class (kg/d). See documentation for rationale/source
        //        # The factor of 100 is just to reduce the input parameter value by 100 so it is a smaller number
        //        Esus_i[LU] = (p['E_M'] * p_SC.ix['S_reach',SC]
        //                      * slope_dict[LU]
        //                      *C_cover
        //                      *(1-p_LU[LU]['C_measures']))
		double Esus_i =
			  PARAMETER(ReachSedimentInputScalingFactor)
			* PARAMETER(ReachSlope)
			* PARAMETER(MeanSlopeOfLand)
			* RESULT(TimeDependentVegetationCoverFactor)
			* (1.0 - PARAMETER(ReductionOfLoadInSediment));
		
		return Esus_i  
			* PARAMETER(LandUseProportions); //NOTE: This is to make them sum up correctly for use later
	)
	
	EQUATION(Model, SuspendedSedimentFlux,
		//Qr_i*Msus_i/Vr_i  # Daily flux of SS
		return RESULT(SuspendedSedimentMass) * RESULT(ReachFlow) / RESULT(ReachVolume);
	)
	
	auto ReachSedimentExternalInput = RegisterEquation(Model, "Reach sediment external input", KgPerDay);
	SetSolver(Model, ReachSedimentExternalInput, SimplyPSolver);
	
	EQUATION(Model, ReachSedimentExternalInput,
		// Msus_in_i = Esus_i * Qr_i**k_M
		return RESULT(TotalReachSedimentInputFromLand) * pow(RESULT(ReachFlow), PARAMETER(InstreamEntrainmentNonlinearCoefficient));
	)
	
	EQUATION(Model, SuspendedSedimentMass,
		//dMsus_dt = (f_Ar*Msus_in_i['A'] + f_IG*Msus_in_i['IG'] + f_S* Msus_in_i['S']  # External inputs (kg/day)
        //        + Msus_US_i                                                       # Inputs from upstream
        //        - (Msus_i/Vr_i)*Qr_i)                                             # Outflow from the reach;(kg/mm)*(mm/day)
		
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanSuspendedSedimentFlux, *Input);
		)
		
		return RESULT(ReachSedimentExternalInput) + upstreamflux - RESULT(SuspendedSedimentFlux);
	)
	
	EQUATION(Model, DailyMeanSuspendedSedimentFlux,
		return RESULT(SuspendedSedimentFlux);
	)
	
	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	
	EQUATION(Model, SuspendedSedimentConcentration,
		return ConvertKgPerMmToMgPerL(RESULT(DailyMeanSuspendedSedimentFlux) / RESULT(DailyMeanReachFlow), PARAMETER(CatchmentArea));
	)
}

static void
AddSimplyPPhosphorusModule(inca_model *Model)
{
	auto Dimensionless  = RegisterUnit(Model);
	auto Kg             = RegisterUnit(Model, "kg");
	auto Mm             = RegisterUnit(Model, "mm");
	auto MmPerKg        = RegisterUnit(Model, "mm/kg");
	auto KgPerM2        = RegisterUnit(Model, "kg/m2");
	auto MgPerL         = RegisterUnit(Model, "mg/l");
	auto MgPerKg        = RegisterUnit(Model, "mg/kg");
	auto KgPerHaPerYear = RegisterUnit(Model, "kg/ha/year");
	auto KgPerDay       = RegisterUnit(Model, "kg/day");
	auto KgPerMm        = RegisterUnit(Model, "kg/mm");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Arable             = RequireIndex(Model, LandscapeUnits, "Arable");
	auto Seminatural        = RequireIndex(Model, LandscapeUnits, "Semi-natural");
	auto ImprovedGrassland  = RequireIndex(Model, LandscapeUnits, "Improved grassland");
	
	auto Phosphorous = RegisterParameterGroup(Model, "Phosphorous", Reach);
	
	auto MSoilPerM2                     = RegisterParameterDouble(Model, Phosphorous, "Soil mass per m2", KgPerM2, 95.0, 0.0, 200.0);
	auto PhosphorousSorptionCoefficient = RegisterParameterDouble(Model, Phosphorous, "Phosphorous sorption coefficient", MmPerKg, 1.13e-4, 0.0, 0.1, "gradient of linear relationship between labile P and TDP concentration");
	auto NetAnnualPInputAgricultural    = RegisterParameterDouble(Model, Phosphorous, "Net annual P input to agricultural soil", KgPerHaPerYear, 10.0, -100.0, 100.0);
	auto NetAnnualPInputNewlyConverted  = RegisterParameterDouble(Model, Phosphorous, "Net annual P input to newly-converted soil", KgPerHaPerYear, -5.0, -100.0, 100.0);
	auto GroundwaterTDPConcentration    = RegisterParameterDouble(Model, Phosphorous, "Groundwater TDP concentration", MgPerL, 0.02, 0.0, 10.0);
	auto PPEnrichmentFactor             = RegisterParameterDouble(Model, Phosphorous, "Particulate P enrichment factor", Dimensionless, 1.6, 0.5, 2.0, "P content of eroded material compared to P content of bulk soils"); //NOTE: min-max values are pulled from thin air. Should be set by somebody who know better.
	auto EffluentTDP                    = RegisterParameterDouble(Model, Phosphorous, "Reach effluent TDP inputs", KgPerDay, 0.1, 0.0, 10.0);
	auto SRPFraction                    = RegisterParameterDouble(Model, Phosphorous, "SRP fraction", Dimensionless, 0.7, 0.0, 1.0, "Factor to multiply TDP by to estimate instream SRP concentration");
	
	auto PhosphorousLand = RegisterParameterGroup(Model, "Phosphorous land", LandscapeUnits);
	SetParentGroup(Model, PhosphorousLand, Phosphorous);
	
	auto InitialEPC0                    = RegisterParameterDouble(Model, PhosphorousLand, "Initial soil water EPC0",      MgPerL, 0.1, 0.0, 10.0);
	auto InitialSoilPConcentration      = RegisterParameterDouble(Model, PhosphorousLand, "Initial total soil P content", MgPerKg, 1458, 0.0, 10000.0);
	
	auto DynamicOptions = GetParameterGroupHandle(Model, "Dynamic options");
	auto DynamicEPC0                    = RegisterParameterBool(Model, DynamicOptions, "Dynamic EPC0", true, "Calculate a dynamic soil water EPC0 (the equilibrium P concentration of zero sorption), so that it varies with labile P content");
	
	auto LandUseProportionsNC           = RegisterParameterDouble(Model, PhosphorousLand, "Land use proportions from newly-converted", Dimensionless, 0.0, 0.0, 1.0);
	auto NCType                         = RegisterParameterUInt(Model, Phosphorous, "Newly-converted type", Dimensionless, 2, 0, 2, "0=Agricultural (from semi-natural), 1=Semi-natural (from agricultural), 2=None");
	
	
	auto SimplyPSolver               = GetSolverHandle(Model, "SimplyP solver");
	auto CatchmentArea               = GetParameterDoubleHandle(Model, "Catchment area");
	auto InstreamEntrainmentNonlinearCoefficient = GetParameterDoubleHandle(Model, "Instream entrainment non-linear coefficient");
	auto LandUseProportions          = GetParameterDoubleHandle(Model, "Land use proportions");
	auto BaseflowIndex               = GetParameterDoubleHandle(Model, "Baseflow index");
	auto AgriculturalSoilWaterVolume = GetEquationHandle(Model, "Agricultural soil water volume");
	auto SeminaturalSoilWaterVolume  = GetEquationHandle(Model, "Semi-natural soil water volume");
	auto InfiltrationExcess          = GetEquationHandle(Model, "Infiltration excess");
	auto AgriculturalSoilWaterFlow   = GetEquationHandle(Model, "Agricultural soil water flow");
	auto SeminaturalSoilWaterFlow    = GetEquationHandle(Model, "Semi-natural soil water flow");
	auto ReachVolume                 = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow                   = GetEquationHandle(Model, "Reach flow");
	auto DailyMeanReachFlow          = GetEquationHandle(Model, "Daily mean reach flow");
	auto GroundwaterFlow             = GetEquationHandle(Model, "Groundwater flow");
	auto ReachSedimentInputFromLand  = GetEquationHandle(Model, "Reach sediment input from land");
	
	auto InitialAgriculturalSoilWaterEPC0 = RegisterEquationInitialValue(Model, "Initial agricultural soil water EPC0", KgPerMm);
	auto AgriculturalSoilWaterEPC0   = RegisterEquation(Model, "Agricultural soil water EPC0", KgPerMm);
	SetInitialValue(Model, AgriculturalSoilWaterEPC0, InitialAgriculturalSoilWaterEPC0);
	
#define DISCRETISE_SOIL_P 1
	
#if DISCRETISE_SOIL_P
	auto InitialAgriculturalSoilTDPMass     = RegisterEquationInitialValue(Model, "Initial agricultural soil TDP mass", Kg);
	auto AgriculturalSoilTDPMass            = RegisterEquation(Model, "Agricultural soil TDP mass", Kg);
	SetInitialValue(Model, AgriculturalSoilTDPMass, InitialAgriculturalSoilTDPMass);
	
	auto InitialAgriculturalSoilLabilePMass = RegisterEquationInitialValue(Model, "Initial agricultural soil labile P mass", Kg);
	auto AgriculturalSoilLabilePMass = RegisterEquation(Model, "Agricultural soil labile P mass", Kg);
	SetInitialValue(Model, AgriculturalSoilLabilePMass, InitialAgriculturalSoilLabilePMass);
	
	EQUATION(Model, InitialAgriculturalSoilTDPMass,
		return RESULT(AgriculturalSoilWaterEPC0) * RESULT(AgriculturalSoilWaterVolume);
	)
	
	EQUATION(Model, AgriculturalSoilTDPMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b = (PARAMETER(PhosphorousSorptionCoefficient) * Msoil + LAST_RESULT(AgriculturalSoilWaterFlow) + RESULT(InfiltrationExcess)) / LAST_RESULT(AgriculturalSoilWaterVolume);
		double a = PARAMETER(NetAnnualPInputAgricultural) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + PARAMETER(PhosphorousSorptionCoefficient) * Msoil * RESULT(AgriculturalSoilWaterEPC0);
		double value = a / b + (LAST_RESULT(AgriculturalSoilTDPMass) - a / b) * exp(-b);
		
		if(!PARAMETER(DynamicEPC0)) return LAST_RESULT(AgriculturalSoilTDPMass);
		
		return value;
	)
	
	EQUATION(Model, InitialAgriculturalSoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		return 1e-6 * (PARAMETER(InitialSoilPConcentration, Arable) - PARAMETER(InitialSoilPConcentration, Seminatural)) * Msoil;
	)
	
	EQUATION(Model, AgriculturalSoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b0 = PARAMETER(PhosphorousSorptionCoefficient) * Msoil + LAST_RESULT(AgriculturalSoilWaterFlow) + RESULT(InfiltrationExcess);
		double b = b0 / LAST_RESULT(AgriculturalSoilWaterVolume);
		double a = PARAMETER(NetAnnualPInputAgricultural) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + PARAMETER(PhosphorousSorptionCoefficient) * Msoil * RESULT(AgriculturalSoilWaterEPC0);
		//TODO: factor out calculations of b0, a? Would probably not matter that much to speed though.
	
		double sorp = PARAMETER(PhosphorousSorptionCoefficient) * Msoil * (a / b0 - RESULT(AgriculturalSoilWaterEPC0) + (LAST_RESULT(AgriculturalSoilTDPMass)/LAST_RESULT(AgriculturalSoilWaterVolume) - a/b0)*(1.0 - exp(-b))/b);
		
		if(!PARAMETER(DynamicEPC0)) sorp = 0.0;
	
		return LAST_RESULT(AgriculturalSoilLabilePMass) + sorp;
	)
	
#else
	auto AgriculturalSoilNetPSorption = RegisterEquation(Model, "Agricultural soil net P sorption", KgPerDay);
	SetSolver(Model, AgriculturalSoilNetPSorption, SimplyPSolver);
	
	auto AgriculturalSoilTDPFlux = RegisterEquation(Model, "Agricultural soil TDP flux", KgPerDay);
	SetSolver(Model, AgriculturalSoilTDPFlux, SimplyPSolver);
	
	auto InitialAgriculturalSoilLabilePMass = RegisterEquationInitialValue(Model, "Initial agricultural soil labile P mass", Kg);
	auto AgriculturalSoilLabilePMass = RegisterEquationODE(Model, "Agricultural soil labile P mass", Kg);
	SetInitialValue(Model, AgriculturalSoilLabilePMass, InitialAgriculturalSoilLabilePMass);
	SetSolver(Model, AgriculturalSoilLabilePMass, SimplyPSolver);
	
	auto InitialAgriculturalSoilTDPMass = RegisterEquationInitialValue(Model, "Initial agricultural soil TDP mass", Kg);
	auto AgriculturalSoilTDPMass     = RegisterEquationODE(Model, "Agricultural soil TDP mass", Kg);
	SetInitialValue(Model, AgriculturalSoilTDPMass, InitialAgriculturalSoilTDPMass);
	SetSolver(Model, AgriculturalSoilTDPMass, SimplyPSolver);
	
	
	EQUATION(Model, AgriculturalSoilNetPSorption,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		//dPlabA_dt = Kf*Msoil*((TDPsA_i/VsA_i)-EPC0_A)  # Net sorption
		double sorption = PARAMETER(PhosphorousSorptionCoefficient) * Msoil * (RESULT(AgriculturalSoilTDPMass) / RESULT(AgriculturalSoilWaterVolume) - RESULT(AgriculturalSoilWaterEPC0) );
		
		return sorption;
	)
	
	EQUATION(Model, InitialAgriculturalSoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		return 1e-6 * (PARAMETER(InitialSoilPConcentration, Arable) - PARAMETER(InitialSoilPConcentration, Seminatural)) * Msoil;
	)
	
	EQUATION(Model, AgriculturalSoilLabilePMass,
		return RESULT(AgriculturalSoilNetPSorption);
		//return 0;
	)
	
	EQUATION(Model, InitialAgriculturalSoilTDPMass,
		return RESULT(AgriculturalSoilWaterEPC0) * RESULT(AgriculturalSoilWaterVolume);
	)
	
	EQUATION(Model, AgriculturalSoilTDPFlux,
		return
			  RESULT(AgriculturalSoilTDPMass) 
			  * (RESULT(AgriculturalSoilWaterFlow) + RESULT(InfiltrationExcess)) / RESULT(AgriculturalSoilWaterVolume);
	)
	
	EQUATION(Model, AgriculturalSoilTDPMass,
		//dTDPsA_dt = ((P_netInput['A']*100*A_catch/365)    # Net inputs (fert+manure-uptake) (kg/ha/yr)
                 //- Kf*Msoil*((TDPsA_i/VsA_i)-EPC0_A)  # Net sorpn (kg/day) (could be alt above)
                 //- (QsA_i*TDPsA_i/VsA_i)              # Outflow via soil water flow (kg/day)
                 //- (Qq_i*TDPsA_i/VsA_i))              # Outflow via quick flow (kg/day)
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		return
			  PARAMETER(NetAnnualPInputAgricultural) * 100.0 * PARAMETER(CatchmentArea) / 365.0
			- RESULT(AgriculturalSoilNetPSorption)
			- RESULT(AgriculturalSoilTDPFlux);
	)
#endif

	EQUATION(Model, InitialAgriculturalSoilWaterEPC0,
		 //p_LU.ix['EPC0_0',LU] = UC_Cinv(p_LU[LU]['EPC0_init_mgl'], p_SC.ix['A_catch',SC])
		 return ConvertMgPerLToKgPerMm(PARAMETER(InitialEPC0, Arable), PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, AgriculturalSoilWaterEPC0,
		/*
		if dynamic_options['Dynamic_EPC0'] == 'y':
			EPC0_A_i = Plab0_A/(Kf*Msoil) # Agricultural EPC0; equals EPC0_0 on the 1st timestep
		else:
			EPC0_A_i = p_LU['A']['EPC0_0']
		*/
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double Kf = PARAMETER(PhosphorousSorptionCoefficient);
		double Plab_A = LAST_RESULT(AgriculturalSoilLabilePMass);
		
		if(PARAMETER(DynamicEPC0)) return Plab_A / (Kf * Msoil);
		
		return LAST_RESULT(AgriculturalSoilWaterEPC0);
	)
	
	
	
	auto SoilFieldCapacity = GetParameterDoubleHandle(Model, "Soil field capacity");

	auto NewlyConvertedSoilWaterVolume = RegisterEquation(Model, "Newly-converted soil water volume", Mm);
	SetInitialValue(Model, NewlyConvertedSoilWaterVolume, SoilFieldCapacity); //NOTE: This is needed for the Labile P + TDP computations in the first timestep!
	SetSolver(Model, NewlyConvertedSoilWaterVolume, SimplyPSolver);
	
	auto NewlyConvertedSoilWaterFlow   = RegisterEquation(Model, "Newly-converted soil water flow", Mm);
	SetSolver(Model, NewlyConvertedSoilWaterFlow, SimplyPSolver);
	
	auto InitialNewlyConvertedSoilWaterEPC0 = RegisterEquationInitialValue(Model, "Initial newly-converted soil water EPC0", KgPerMm);
	auto NewlyConvertedSoilWaterEPC0   = RegisterEquation(Model, "Newly-converted soil water EPC0", KgPerMm);
	SetInitialValue(Model, NewlyConvertedSoilWaterEPC0, InitialNewlyConvertedSoilWaterEPC0);
	
#if DISCRETISE_SOIL_P
	auto InitialNewlyConvertedSoilTDPMass     = RegisterEquationInitialValue(Model, "Initial newly-converted soil TDP mass", Kg);
	auto NewlyConvertedSoilTDPMass            = RegisterEquation(Model, "Newly-converted soil TDP mass", Kg);
	SetInitialValue(Model, NewlyConvertedSoilTDPMass, InitialNewlyConvertedSoilTDPMass);
	
	auto InitialNewlyConvertedSoilLabilePMass = RegisterEquationInitialValue(Model, "Initial newly-converted soil labile P mass", Kg);
	auto NewlyConvertedSoilLabilePMass = RegisterEquation(Model, "Newly-converted soil labile P mass", Kg);
	SetInitialValue(Model, NewlyConvertedSoilLabilePMass, InitialNewlyConvertedSoilLabilePMass);
	
	EQUATION(Model, InitialNewlyConvertedSoilTDPMass,
		return RESULT(AgriculturalSoilWaterEPC0) * RESULT(AgriculturalSoilWaterVolume);
	)
	
	EQUATION(Model, NewlyConvertedSoilTDPMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b = (PARAMETER(PhosphorousSorptionCoefficient) * Msoil + LAST_RESULT(NewlyConvertedSoilWaterFlow) + RESULT(InfiltrationExcess)) / LAST_RESULT(NewlyConvertedSoilWaterVolume);
		double a = PARAMETER(NetAnnualPInputNewlyConverted) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + PARAMETER(PhosphorousSorptionCoefficient) * Msoil * RESULT(NewlyConvertedSoilWaterEPC0);
		
		if(!PARAMETER(DynamicEPC0)) return LAST_RESULT(NewlyConvertedSoilTDPMass);
		
		return a / b + (LAST_RESULT(NewlyConvertedSoilTDPMass) - a / b) * exp(-b);
	)
	
	EQUATION(Model, InitialNewlyConvertedSoilLabilePMass,
		double ag = RESULT(AgriculturalSoilLabilePMass);
		if(PARAMETER(NCType) == Seminatural) return ag;
		return 0.0;
	)
	
	EQUATION(Model, NewlyConvertedSoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b0 = PARAMETER(PhosphorousSorptionCoefficient) * Msoil + LAST_RESULT(NewlyConvertedSoilWaterFlow) + RESULT(InfiltrationExcess);
		double b = b0 / LAST_RESULT(NewlyConvertedSoilWaterVolume);
		double a = PARAMETER(NetAnnualPInputNewlyConverted) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + PARAMETER(PhosphorousSorptionCoefficient) * Msoil * RESULT(NewlyConvertedSoilWaterEPC0);
		//TODO: factor out calculations of b0, a? Would probably not matter that much to speed though.
	
		double sorp = PARAMETER(PhosphorousSorptionCoefficient) * Msoil * (a / b0 - RESULT(NewlyConvertedSoilWaterEPC0) + (LAST_RESULT(NewlyConvertedSoilTDPMass)/LAST_RESULT(NewlyConvertedSoilWaterVolume) - a/b0)*(1.0 - exp(-b))/b);
		
		if(!PARAMETER(DynamicEPC0)) sorp = 0.0;
	
		return LAST_RESULT(NewlyConvertedSoilLabilePMass) + sorp;
	)

#else
	
	auto NewlyConvertedSoilNetPSorption = RegisterEquation(Model, "Newly-converted soil net P sorption", KgPerDay);
	SetSolver(Model, NewlyConvertedSoilNetPSorption, SimplyPSolver);
	
	auto NewlyConvertedSoilTDPFlux = RegisterEquation(Model, "Newly-converted soil TDP flux", KgPerDay);
	SetSolver(Model, NewlyConvertedSoilTDPFlux, SimplyPSolver);
	
	auto InitialNewlyConvertedSoilLabilePMass = RegisterEquationInitialValue(Model, "Initial newly-converted soil labile P mass", Kg);
	auto NewlyConvertedSoilLabilePMass = RegisterEquationODE(Model, "Newly-converted soil labile P mass", Kg);
	SetInitialValue(Model, NewlyConvertedSoilLabilePMass, InitialNewlyConvertedSoilLabilePMass);
	SetSolver(Model, NewlyConvertedSoilLabilePMass, SimplyPSolver);
	
	auto InitialNewlyConvertedSoilTDPMass = RegisterEquationInitialValue(Model, "Initial newly-converted soil TDP mass", Kg);
	auto NewlyConvertedSoilTDPMass     = RegisterEquationODE(Model, "Newly-converted soil TDP mass", Kg);
	SetInitialValue(Model, NewlyConvertedSoilTDPMass, InitialNewlyConvertedSoilTDPMass);
	SetSolver(Model, NewlyConvertedSoilTDPMass, SimplyPSolver);
	
	EQUATION(Model, NewlyConvertedSoilNetPSorption,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		//dPlabA_dt = Kf*Msoil*((TDPsA_i/VsA_i)-EPC0_A)  # Net sorption
		double sorption = PARAMETER(PhosphorousSorptionCoefficient) * Msoil * (RESULT(NewlyConvertedSoilTDPMass) / RESULT(NewlyConvertedSoilWaterVolume) - RESULT(NewlyConvertedSoilWaterEPC0) );
		
		return sorption;
	)
	
	EQUATION(Model, InitialNewlyConvertedSoilLabilePMass,
		double ag = RESULT(AgriculturalSoilLabilePMass);
		if(PARAMETER(NCType) == Seminatural) return ag;
		return 0.0;
	)
	
	EQUATION(Model, NewlyConvertedSoilLabilePMass,
		return RESULT(NewlyConvertedSoilNetPSorption);
	)
	
	EQUATION(Model, InitialNewlyConvertedSoilTDPMass,
		return RESULT(NewlyConvertedSoilWaterEPC0) * RESULT(NewlyConvertedSoilWaterVolume);
	)
	
	EQUATION(Model, NewlyConvertedSoilTDPFlux,
		return
			  RESULT(NewlyConvertedSoilTDPMass) 
			  * (RESULT(NewlyConvertedSoilWaterFlow) + RESULT(InfiltrationExcess)) / RESULT(NewlyConvertedSoilWaterVolume);
	)
	
	EQUATION(Model, NewlyConvertedSoilTDPMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		return
			  PARAMETER(NetAnnualPInputNewlyConverted) * 100.0 * PARAMETER(CatchmentArea) / 365.0
			- RESULT(NewlyConvertedSoilNetPSorption)
			- RESULT(NewlyConvertedSoilTDPFlux);
	)
#endif
	
	EQUATION(Model, NewlyConvertedSoilWaterVolume,
		u64 nctype = PARAMETER(NCType);
		double ag = RESULT(AgriculturalSoilWaterVolume);
		double sn = RESULT(SeminaturalSoilWaterVolume);
		if(nctype == Arable) return ag;
		return sn;
	)
	
	EQUATION(Model, NewlyConvertedSoilWaterFlow,
		u64 nctype = PARAMETER(NCType);
		double ag = RESULT(AgriculturalSoilWaterFlow);
		double sn = RESULT(SeminaturalSoilWaterFlow);
		if(nctype == Arable) return ag;
		return sn;
	)
	
	
	EQUATION(Model, InitialNewlyConvertedSoilWaterEPC0,
		u64 nctype = PARAMETER(NCType);
		return ConvertMgPerLToKgPerMm(PARAMETER(InitialEPC0, nctype), PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, NewlyConvertedSoilWaterEPC0,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double Kf = PARAMETER(PhosphorousSorptionCoefficient);
		double Plab_A = LAST_RESULT(NewlyConvertedSoilLabilePMass);
		
		if(PARAMETER(DynamicEPC0)) return Plab_A / (Kf * Msoil);
		
		return LAST_RESULT(NewlyConvertedSoilWaterEPC0);
	)
	
	
	
	auto StreamTDPFlux = RegisterEquation(Model, "Stream TDP flux", KgPerDay);
	SetSolver(Model, StreamTDPFlux, SimplyPSolver);
	
	auto StreamPPFlux  = RegisterEquation(Model, "Stream PP flux", KgPerDay);
	SetSolver(Model, StreamPPFlux, SimplyPSolver);
	
	auto StreamTDPMass = RegisterEquationODE(Model, "Stream TDP mass", Kg);
	SetInitialValue(Model, StreamTDPMass, 0.0);
	SetSolver(Model, StreamTDPMass, SimplyPSolver);
	
	auto StreamPPMass  = RegisterEquationODE(Model, "Stream PP mass", Kg);
	SetInitialValue(Model, StreamPPMass, 0.0);
	SetSolver(Model, StreamPPMass, SimplyPSolver);
	
	auto DailyMeanStreamTDPFlux = RegisterEquationODE(Model, "Daily mean stream TDP flux", KgPerDay);
	SetInitialValue(Model, DailyMeanStreamTDPFlux, 0.0);
	SetSolver(Model, DailyMeanStreamTDPFlux, SimplyPSolver);
	ResetEveryTimestep(Model, DailyMeanStreamTDPFlux);
	
	auto DailyMeanStreamPPFlux = RegisterEquationODE(Model, "Daily mean stream PP flux", KgPerDay);
	SetInitialValue(Model, DailyMeanStreamPPFlux, 0.0);
	SetSolver(Model, DailyMeanStreamPPFlux, SimplyPSolver);
	ResetEveryTimestep(Model, DailyMeanStreamPPFlux);
	
	EQUATION(Model, StreamTDPFlux,
		//dTDPr_out_dt = Qr_i*TDPr_i/Vr_i
		return RESULT(StreamTDPMass) * RESULT(ReachFlow) / RESULT(ReachVolume);
	)
	
	EQUATION(Model, DailyMeanStreamTDPFlux,
		return RESULT(StreamTDPFlux);
	)
	
	EQUATION(Model, StreamPPFlux,
		return RESULT(StreamPPMass) * RESULT(ReachFlow) / RESULT(ReachVolume);
	)
	
	EQUATION(Model, DailyMeanStreamPPFlux,
		return RESULT(StreamPPFlux);
	)
	
	EQUATION(Model, StreamTDPMass,
		/*
		dTDPr_dt = ((1-beta)*f_A*QsA_i*(TDPsA_i/VsA_i)          # Soil input, old agri. Units:(mm/d)(kg/mm)
					+ (1-beta)*f_NC_A*QsNC_i*(TDPsNC_i/VsNC_i)  # Soil input, new agri land
					+ (1-beta)*f_NC_S*QsNC_i*(TDPsNC_i/VsNC_i)  # Soil input, new SN land
					+ f_A*Qq_i*(TDPsA_i/VsA_i)                  # Quick input, old agri. Units:(mm/d)(kg/mm)
					+ f_NC_A*Qq_i*(TDPsNC_i/VsNC_i)             # Quick input, newly-converted agri
					+ f_NC_S*Qq_i*(TDPsNC_i/VsNC_i)             # Quick inputs, newly-converted SN
					+ Qg_i*UC_Cinv(TDPg,A_catch)                # Groundwater input. Units: (mm/d)(kg/mm)
					+ TDPeff                                    # Effluent input (kg/day)
					+ TDPr_US_i                                 # Inputs from upstream 
					- Qr_i*(TDPr_i/Vr_i))                       # Reach outflow. Units: (mm/d)(kg/mm)
		*/
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamTDPFlux, *Input);
		)
		
		double f_A = PARAMETER(LandUseProportions, Arable) + PARAMETER(LandUseProportions, ImprovedGrassland);
		double f_S = PARAMETER(LandUseProportions, Seminatural);
		double f_NC_A = PARAMETER(LandUseProportionsNC, Arable) + PARAMETER(LandUseProportionsNC, ImprovedGrassland);
		double f_NC_S = PARAMETER(LandUseProportionsNC, Seminatural);
		
		double fromAgriculturalSoil =
			    f_A * (1.0 - f_NC_A)
			  * (RESULT(AgriculturalSoilTDPMass)  / RESULT(AgriculturalSoilWaterVolume))
			  * ((1.0-PARAMETER(BaseflowIndex)) * RESULT(AgriculturalSoilWaterFlow) + RESULT(InfiltrationExcess));
		
		double fromNewlyConvertedSoil = 0.0;
				(f_A * f_NC_A + f_S * f_NC_S)      //NOTE: at least one of f_NC_A or f_NC_S will always be 0.
			  * (RESULT(NewlyConvertedSoilTDPMass) / RESULT(NewlyConvertedSoilWaterVolume))
			  * ((1.0-PARAMETER(BaseflowIndex)) * RESULT(NewlyConvertedSoilWaterFlow) + RESULT(InfiltrationExcess));
		
		return
			  fromAgriculturalSoil
			+ fromNewlyConvertedSoil
			+ RESULT(GroundwaterFlow) * ConvertMgPerLToKgPerMm(PARAMETER(GroundwaterTDPConcentration), PARAMETER(CatchmentArea))
			+ PARAMETER(EffluentTDP)
			+ upstreamflux
			- RESULT(StreamTDPFlux);
	)
	
	EQUATION(Model, StreamPPMass,
		/*
		dPPr_dt = (E_PP *
               (f_Ar*Msus_in_i['A']*(PlabA_i+P_inactive)/Msoil       # Old arable land
                + f_IG*Msus_in_i['IG']*(PlabA_i+P_inactive)/Msoil    # Old improved grassland
                + f_S*Msus_in_i['S']*P_inactive/Msoil)               # Semi-natural land
               + f_NC_Ar*Msus_in_i['A']*(PlabNC_i+P_inactive)/Msoil  # Newly-converted arable
               + f_NC_IG*Msus_in_i['IG']*(PlabNC_i+P_inactive)/Msoil # Newly-converted IG
               + f_NC_S*Msus_in_i['S']*(PlabNC_i+P_inactive)/Msoil   # New semi-natural
               + PPr_US_i                                            # Inputs from upstream 
               - Qr_i*(PPr_i/Vr_i))                                  # Reach outflow (mm/d)(kg/mm)
		*/
		//P_inactive = 10**-6*p_LU['S']['SoilPconc']*Msoil
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double P_inactive = 1e-6*PARAMETER(InitialSoilPConcentration, Seminatural)*Msoil;
		double E_PP  = PARAMETER(PPEnrichmentFactor);
		double Ag = (RESULT(AgriculturalSoilLabilePMass)+P_inactive) / Msoil;
		double Sn = P_inactive / Msoil;
		
		double f_NC_Ar = PARAMETER(LandUseProportionsNC, Arable);
		double f_NC_IG = PARAMETER(LandUseProportionsNC, ImprovedGrassland);
		double f_NC_S  = PARAMETER(LandUseProportionsNC, Seminatural);
		
		double Nc = (RESULT(NewlyConvertedSoilLabilePMass) + P_inactive) / Msoil;
		
		//NOTE: These are already multiplied with f_X. I.e. Esus_in_Ar is actually Esus_in_i['Ar']*f_Ar
		double Esus_in_Ar = RESULT(ReachSedimentInputFromLand, Arable);
		double Esus_in_IG = RESULT(ReachSedimentInputFromLand, ImprovedGrassland);
		double Esus_in_S  = RESULT(ReachSedimentInputFromLand, Seminatural);
		
		//NOTE: RESULT(ReachSedimentInputFromLand, Arable) = Esus_i['A']*f_A   etc.
		// Msus_in_i = Esus_i * Qr_i**k_M
		double coeff = pow(RESULT(ReachFlow), PARAMETER(InstreamEntrainmentNonlinearCoefficient));
		double sedimentinput =
			E_PP * (
			  (Esus_in_Ar*(1.0 - f_NC_Ar) + Esus_in_IG*(1.0 - f_NC_IG))*Ag
		     + Esus_in_S * (1.0 - f_NC_S) * Sn
			 + (Esus_in_Ar * f_NC_Ar + Esus_in_IG * f_NC_IG + Esus_in_S * f_NC_S) * Nc
			) * coeff;
		
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamPPFlux, *Input);
		)
		return
			  sedimentinput
			+ upstreamflux
			- RESULT(StreamPPFlux);
	)
	
	auto TDPConcentration = RegisterEquation(Model, "TDP concentration (volume weighted daily mean)", MgPerL);
	auto PPConcentration  = RegisterEquation(Model, "PP concentration (volume weighted daily mean)", MgPerL);
	auto DailyMeanStreamTPFlux = RegisterEquation(Model, "Daily mean stream TP flux", KgPerDay);
	auto TPConcentration  = RegisterEquation(Model, "TP concentration (volume weighted daily mean)", MgPerL);
	auto DailyMeanStreamSRPFlux = RegisterEquation(Model, "Daily mean stream SRP flux", KgPerDay);
	auto SRPConcentration = RegisterEquation(Model, "SRP concentration (volume weighted daily mean)", MgPerL);
	
	EQUATION(Model, TDPConcentration,
		return ConvertKgPerMmToMgPerL(RESULT(DailyMeanStreamTDPFlux) / RESULT(DailyMeanReachFlow), PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, PPConcentration,
		return ConvertKgPerMmToMgPerL(RESULT(DailyMeanStreamPPFlux) / RESULT(DailyMeanReachFlow), PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, DailyMeanStreamTPFlux,
		return RESULT(DailyMeanStreamTDPFlux) + RESULT(DailyMeanStreamPPFlux);
	)
	
	EQUATION(Model, TPConcentration,
		return RESULT(TDPConcentration) + RESULT(PPConcentration);
	)
	
	EQUATION(Model, DailyMeanStreamSRPFlux,
		return RESULT(DailyMeanStreamTDPFlux) * PARAMETER(SRPFraction);
	)
	
	EQUATION(Model, SRPConcentration,
		return RESULT(TDPConcentration) * PARAMETER(SRPFraction);
	)
}

static void
AddSimplyPInputToWaterBodyModule(inca_model *Model)
{
	auto M3PerSecond = RegisterUnit(Model, "m3/s");
	auto KgPerDay    = RegisterUnit(Model, "kg/day");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	
	auto WaterBody = RegisterParameterGroup(Model, "Input to water body", Reach);
	
	auto IsInputToWaterBody = RegisterParameterBool(Model, WaterBody, "Is input to water body", false, "Whether or not the flow and various fluxes from this reach should be summed up in the calculation of inputs to a water body or lake");
	
	auto DailyMeanReachFlow = GetEquationHandle(Model, "Daily mean reach flow");
	auto DailyMeanSuspendedSedimentFlux = GetEquationHandle(Model, "Daily mean suspended sediment flux");
	auto DailyMeanTDPFlux = GetEquationHandle(Model, "Daily mean stream TDP flux");
	auto DailyMeanPPFlux  = GetEquationHandle(Model, "Daily mean stream PP flux");
	auto DailyMeanTPFlux  = GetEquationHandle(Model, "Daily mean stream TP flux");
	auto DailyMeanSRPFlux = GetEquationHandle(Model, "Daily mean stream SRP flux");
	
	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	
	auto FlowToWaterBody     = RegisterEquation(Model, "Flow to water body", M3PerSecond);
	auto SSFluxToWaterBody   = RegisterEquation(Model, "SS flux to water body", KgPerDay);
	auto TDPFluxToWaterBody  = RegisterEquation(Model, "TDP flux to water body", KgPerDay);
	auto PPFluxToWaterBody   = RegisterEquation(Model, "PP flux to water body", KgPerDay);
	auto TPFluxToWaterBody   = RegisterEquation(Model, "TP flux to water body", KgPerDay);
	auto SRPFluxToWaterBody  = RegisterEquation(Model, "SRP flux to water body", KgPerDay);
	
	//TODO: We should maybe have a shorthand for this kind of cumulative equation?
	
	EQUATION(Model, FlowToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = 0; ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double ca = PARAMETER(CatchmentArea, ReachIndex);
			double q  = RESULT(DailyMeanReachFlow, ReachIndex); //NOTE: This is in mm/day
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += ConvertMmPerDayToM3PerSecond(q, ca);
			}
		}
		return sum;
	)
	
	EQUATION(Model, SSFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = 0; ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double ss = RESULT(DailyMeanSuspendedSedimentFlux, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += ss;
			}
		}
		return sum;
	)
	
	EQUATION(Model, TDPFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = 0; ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double tdp = RESULT(DailyMeanTDPFlux, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += tdp;
			}
		}
		return sum;
	)
	
	EQUATION(Model, PPFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = 0; ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double pp = RESULT(DailyMeanPPFlux, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += pp;
			}
		}
		return sum;
	)
	
	EQUATION(Model, TPFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = 0; ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double tp = RESULT(DailyMeanTPFlux, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += tp;
			}
		}
		return sum;
	)
	
	EQUATION(Model, SRPFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = 0; ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double srp = RESULT(DailyMeanSRPFlux, ReachIndex); //NOTE: This is in mm/day
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += srp;
			}
		}
		return sum;
	)
	
}




