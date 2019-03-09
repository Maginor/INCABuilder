// Simply Hydrology module

#include "Preprocessing/ThornthwaitePET.h"

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

static void
AddSimplyHydrologyModule(inca_model *Model)
{
	auto Degrees = RegisterUnit(Model, "°");
	auto System = GetParameterGroupHandle(Model, "System");
	RegisterParameterDouble(Model, System, "Latitude", Degrees, 60.0, -90.0, 90.0, "Used in PET calculation if no PET timeseries was provided in the input data");
	
	AddPreprocessingStep(Model, ComputeThornthwaitePET); //NOTE: The preprocessing step is called at the start of each model run.
	
	auto Dimensionless     = RegisterUnit(Model);
	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto Days              = RegisterUnit(Model, "days");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto PerM3             = RegisterUnit(Model, "m^{-3}");
	auto M3PerSecond       = RegisterUnit(Model, "m^3/s");
	auto Km2               = RegisterUnit(Model, "km^2");
	auto M                 = RegisterUnit(Model, "m");
	
	// Set up indexers
	auto Reach = RegisterIndexSetBranched(Model, "Reaches");
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	// Global snow parameters
	auto Snow = RegisterParameterGroup(Model, "Snow");
	
	auto InitialSnowDepth        = RegisterParameterDouble(Model, Snow, "Initial snow depth as water equivalent", Mm, 0.0, 0.0, 50000.0);
	auto DegreeDayFactorSnowmelt = RegisterParameterDouble(Model, Snow, "Degree-day factor for snowmelt", MmPerDegreePerDay, 2.74, 0.0, 5.0);
	
	// Hydrology parameters that don't vary by sub-catchment or reach
	auto Hydrology = RegisterParameterGroup(Model, "Hydrology");
	
	auto ProportionToQuickFlow   = RegisterParameterDouble(Model, Hydrology, "Proportion of precipitation that contributes to quick flow", Dimensionless, 0.020, 0.0, 1.0);
	auto PETReductionFactor      = RegisterParameterDouble(Model, Hydrology, "PET reduction factor", Dimensionless, 1.0, 0.0, 1.0);
	auto SoilFieldCapacity       = RegisterParameterDouble(Model, Hydrology, "Soil field capacity", Mm, 290.0, 0.0, 5000.0);
	auto BaseflowIndex           = RegisterParameterDouble(Model, Hydrology, "Baseflow index", Dimensionless, 0.70, 0.0, 1.0);
	auto GroundwaterTimeConstant = RegisterParameterDouble(Model, Hydrology, "Groundwater time constant", Days, 65.0, 0.5, 400.0);
	auto MinimumGroundwaterFlow  = RegisterParameterDouble(Model, Hydrology, "Minimum groundwater flow", MmPerDay, 0.40, 0.0, 10.0);
	auto A                       = RegisterParameterDouble(Model, Hydrology, "Gradient of stream velocity-discharge relationship", PerM3, 0.5, 0.00001, 0.99, "The a in V = aQ^b");
	auto B                       = RegisterParameterDouble(Model, Hydrology, "Exponent of stream velocity-discharge relationship", Dimensionless, 0.42, 0.1, 0.99, "The b in V = aQ^b");
	
	// General parameters that vary by reach or sub-catchment
	auto ReachParams = RegisterParameterGroup(Model, "General subcatchment and reach parameters", Reach);
	
	auto CatchmentArea           = RegisterParameterDouble(Model, ReachParams, "Catchment area", Km2, 51.7, 0.0, 10000.0);
	auto ReachLength             = RegisterParameterDouble(Model, ReachParams, "Reach length", M, 10000.0, 0.0, 10000000.0);
	
	// Instream hydrology parameters that vary by reach
	auto HydrologyReach = RegisterParameterGroup(Model, "Hydrology reach", Reach);
	
	auto InitialInStreamFlow     = RegisterParameterDouble(Model, HydrologyReach, "Initial in-stream flow", M3PerSecond, 1.0, 0.0, 1000000.0, "This parameter is only used by reaches that don't have other reaches as inputs.");
	
	// Terrestrial hydrology parameters that vary by land class
	auto HydrologyLand = RegisterParameterGroup(Model, "Hydrology land", LandscapeUnits);
	
	auto SoilWaterTimeConstant   = RegisterParameterDouble(Model, HydrologyLand, "Soil water time constant", Days, 2.0, 0.05, 40.0);
	
	// General parameters that vary by land class and reach
	auto SubcatchmentGeneral = RegisterParameterGroup(Model, "Subcatchment characteristics by land class", LandscapeUnits);
	SetParentGroup(Model, SubcatchmentGeneral, ReachParams);
	
	auto LandUseProportions   = RegisterParameterDouble(Model, SubcatchmentGeneral, "Land use proportions", Dimensionless, 0.5, 0.0, 1.0);
	
	// Inputs
	auto Precipitation  = RegisterInput(Model, "Precipitation");
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	
	// Start equations
	
	// Non-ODE equations
	auto PrecipitationFallingAsSnow = RegisterEquation(Model, "Precipitation falling as snow", MmPerDay);
	auto PrecipitationFallingAsRain = RegisterEquation(Model, "Precipitation falling as rain", MmPerDay);
	auto PotentialDailySnowmelt     = RegisterEquation(Model, "Potential daily snowmelt", MmPerDay);
	auto SnowMelt                   = RegisterEquation(Model, "Snow melt", MmPerDay);
	auto SnowDepth                  = RegisterEquation(Model, "Snow depth as water equivalent", Mm);
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
	
	// ODE equations
	
	auto SimplySolver = RegisterSolver(Model, "Simply solver", 0.01 /* 1.0/20000.0 */, IncaDascru);
	//auto SimplySolver = RegisterSolver(Model, "SimplyP solver", 0.001, BoostRK4);
	//auto SimplySolver = RegisterSolver(Model, "SimplyP solver", 0.001, BoostCashCarp54, 1e-6, 1e-6);
	//auto SimplySolver = RegisterSolver(Model, "SimplyP solver", 0.001, BoostRosenbrock4, 1e-3, 1e-3);
	//auto SimplySolver = RegisterSolver(Model, "SimplyP solver", 0.0025, Mtl4ImplicitEuler);   //NOTE: Being a first order method, this one is not that good..
	
	auto SoilWaterFlow = RegisterEquation(Model, "Soil water flow", MmPerDay);
	SetSolver(Model, SoilWaterFlow, SimplySolver);
	
	auto SoilWaterVolume = RegisterEquationODE(Model, "Soil water volume", Mm);
	SetInitialValue(Model, SoilWaterVolume, SoilFieldCapacity);
	SetSolver(Model, SoilWaterVolume, SimplySolver);
	
	EQUATION(Model, SoilWaterFlow,
		double smd = PARAMETER(SoilFieldCapacity) - RESULT(SoilWaterVolume);
		//return - smd / (PARAMETER(SoilWaterTimeConstant, Arable) * (1.0 + exp(smd)));
		return -smd * ActivationControl(RESULT(SoilWaterVolume), PARAMETER(SoilFieldCapacity), 0.01) / PARAMETER(SoilWaterTimeConstant);
	)
	
	EQUATION(Model, SoilWaterVolume,
		// mu = -np.log(0.01)/p['fc']
		//P*(1-f_quick) - alpha*E*(1 - np.exp(-mu*VsA_i)) - QsA_i
		return
			  RESULT(Infiltration)
			- PARAMETER(PETReductionFactor) * INPUT(PotentialEvapoTranspiration) * (1.0 - exp(log(0.01) * RESULT(SoilWaterVolume) / PARAMETER(SoilFieldCapacity))) //NOTE: Should 0.01 be a parameter?
			- RESULT(SoilWaterFlow);	
	)
	
	auto InitialGroundwaterVolume = RegisterEquationInitialValue(Model, "Initial groundwater volume", Mm);
	auto GroundwaterVolume        = RegisterEquationODE(Model, "Groundwater volume", Mm);
	SetInitialValue(Model, GroundwaterVolume, InitialGroundwaterVolume);
	SetSolver(Model, GroundwaterVolume, SimplySolver);
	
	auto GroundwaterFlow          = RegisterEquation(Model, "Groundwater flow", MmPerDay);
	SetSolver(Model, GroundwaterFlow, SimplySolver);
	
	EQUATION(Model, GroundwaterFlow,
		double flow0   = RESULT(GroundwaterVolume) / PARAMETER(GroundwaterTimeConstant);
		double flowmin = PARAMETER(MinimumGroundwaterFlow);
		double t = ActivationControl(flow0, flowmin, 0.01);
		return (1.0 - t)*flowmin + t*flow0;
	)
	
	EQUATION(Model, GroundwaterVolume,
		return PARAMETER(BaseflowIndex) * RESULT(SoilWaterFlow)
			- RESULT(GroundwaterFlow);
	)
	
	EQUATION(Model, InitialGroundwaterVolume,
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
	
	auto SoilWaterFlowToReach = RegisterEquation(Model, "Soil water flow to reach", M3PerSecond); //Units m3/s so that can sum over LU classes
	SetSolver(Model, SoilWaterFlowToReach, SimplySolver);
	
	auto TotalSoilwaterFlowToReach = RegisterEquationCumulative(Model, "Total soilwater runoff to reach", SoilWaterFlowToReach, LandscapeUnits); //Sum over LU
		
	auto ReachFlowInput    = RegisterEquation(Model, "Reach flow input", MmPerDay);
	SetSolver(Model, ReachFlowInput, SimplySolver);
	
	auto InitialReachVolume = RegisterEquationInitialValue(Model, "Initial reach volume", Mm); 
	auto ReachVolume        = RegisterEquationODE(Model, "Reach volume", Mm);
	SetInitialValue(Model, ReachVolume, InitialReachVolume);
	SetSolver(Model, ReachVolume, SimplySolver);
	
	auto InitialReachFlow   = RegisterEquationInitialValue(Model, "Initial reach flow", MmPerDay);
	auto ReachFlow          = RegisterEquationODE(Model, "Reach flow (end-of-day)", MmPerDay);
	SetInitialValue(Model, ReachFlow, InitialReachFlow);
	SetSolver(Model, ReachFlow, SimplySolver);
	
	auto DailyMeanReachFlow = RegisterEquationODE(Model, "Reach flow (daily mean, mm/day)", MmPerDay);
	SetInitialValue(Model, DailyMeanReachFlow, 0.0);
	SetSolver(Model, DailyMeanReachFlow, SimplySolver);
	ResetEveryTimestep(Model, DailyMeanReachFlow);
	
	EQUATION(Model, SoilWaterFlowToReach,
		return ConvertMmPerDayToM3PerSecond((1.0-PARAMETER(BaseflowIndex))*RESULT(SoilWaterFlow), PARAMETER(CatchmentArea)*PARAMETER(LandUseProportions));
	)
	
	EQUATION(Model, ReachFlowInput,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(DailyMeanReachFlow, *Input) * PARAMETER(CatchmentArea, *Input) / PARAMETER(CatchmentArea);
		)
		double soilwaterFlow = ConvertM3PerSecondToMmPerDay(RESULT(TotalSoilwaterFlowToReach), PARAMETER(CatchmentArea));
		return upstreamflow + RESULT(InfiltrationExcess)+ soilwaterFlow + RESULT(GroundwaterFlow);
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

	// Post-processing
	auto DailyMeanReachFlowCumecs = RegisterEquation(Model, "Reach flow (daily mean, cumecs)", M3PerSecond);
	
	EQUATION(Model, DailyMeanReachFlowCumecs,
		return ConvertMmPerDayToM3PerSecond(RESULT(DailyMeanReachFlow), PARAMETER(CatchmentArea));
	)
}
