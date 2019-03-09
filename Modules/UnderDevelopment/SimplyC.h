

inline double
ConvertMgPerLToKgPerMm(double MgPerL, double CatchmentArea)
{
	return MgPerL * CatchmentArea;
}

inline double
ConvertKgPerMmToMgPerL(double KgPerMm, double CatchmentArea)
{
	return KgPerMm / CatchmentArea;
}

static void
AddSimplyCModel(inca_model *Model)
{
	
	// Inputs
	auto AirTemperature = GetInputHandle(Model, "Air temperature");

	// Solver
	auto SimplySolver = GetSolverHandle(Model, "Simply solver");

	// Units
	auto Kg			= RegisterUnit(Model, "kg");
	auto KgPerDay	= RegisterUnit(Model, "kg/day");
	auto Dimensionless = RegisterUnit(Model);
	auto MgPerL		 = RegisterUnit(Model, "mg/l");

	// Set up indexers
	auto Reach          = GetIndexSetHandle(Model, "Reaches"); //Defined in SimplyHydrol.h
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto LowCarbon   = RequireIndex(Model, LandscapeUnits, "Low soil carbon");
	auto HighCarbon  = RequireIndex(Model, LandscapeUnits, "High soil carbon");

	// PARAMS
	
	// Params defined in hydrol model
	auto CatchmentArea               = GetParameterDoubleHandle(Model, "Catchment area");
	auto BaseflowIndex               = GetParameterDoubleHandle(Model, "Baseflow index");

	// Carbon params that don't vary with land class, sub-catchment/reach
	auto CarbonParamsGlobal = RegisterParameterGroup(Model, "Carbon global");
	auto SoilDOCcoefficient = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil water DOC coefficient", Dimensionless, 1.0);
	auto DeepSoilDOCConcentration = RegisterParameterDouble(Model, CarbonParamsGlobal, "Mineral soil/groundwater DOC concentration", MgPerL, 0.0);

	// Carbon params that vary with land class
	auto CarbonParamsLand = RegisterParameterGroup(Model, "Carbon land", LandscapeUnits);
	auto BaselineSoilDOCConcentration = RegisterParameterDouble(Model, CarbonParamsLand, "Baseline soil water DOC concentratoin", MgPerL, 10.0);

	// EQUATIONS

	// Equations defined in hydrology module required here
	auto SoilWaterVolume = GetEquationHandle(Model, "Soil water volume");
	auto InfiltrationExcess          = GetEquationHandle(Model, "Infiltration excess");
	auto SoilWaterFlow   = GetEquationHandle(Model, "Soil water flow");
	auto GroundwaterFlow             = GetEquationHandle(Model, "Groundwater flow");
	auto ReachVolume                 = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow                   = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto DailyMeanReachFlow          = GetEquationHandle(Model, "Reach flow (daily mean, mm/day)");

	// Equation from soil temperature module
	auto SoilTemperature       = GetEquationHandle(Model, "Soil temperature");

	// Terrestrial carbon equations

	auto SoilwaterCarbonFluxToReach = RegisterEquation(Model, "Soil water carbon flux", KgPerDay);
	SetSolver(Model, SoilwaterCarbonFluxToReach, SimplySolver);

	auto SoilwaterCarbonMass = RegisterEquationODE(Model, "Soil water carbon mass", Kg);
	SetInitialValue(Model, SoilwaterCarbonMass, 0.0); //Default 0  if don't provide this
	SetSolver(Model, SoilwaterCarbonMass, SimplySolver);

	EQUATION(Model, SoilwaterCarbonFluxToReach,
		return
			(RESULT(SoilwaterCarbonMass)/ RESULT(SoilWaterVolume))
			* (1.0-PARAMETER(BaseflowIndex))*RESULT(SoilWaterFlow) + RESULT(InfiltrationExcess) ;
			)

	EQUATION(Model, SoilwaterCarbonMass,
		return
			PARAMETER(SoilDOCcoefficient) * RESULT(SoilTemperature) * PARAMETER(BaselineSoilDOCConcentration)
			- RESULT(SoilwaterCarbonFluxToReach);
			)

	// Instream equations

	auto StreamDOCMass = RegisterEquationODE(Model, "Stream DOC mass", Kg);
	SetInitialValue(Model, StreamDOCMass, 0.0);
	SetSolver(Model, StreamDOCMass, SimplySolver);

	auto StreamDOCFlux = RegisterEquation(Model, "Stream DOC flux", KgPerDay);
	SetSolver(Model, StreamDOCFlux, SimplySolver);

	auto DailyMeanStreamDOCFlux = RegisterEquationODE(Model, "Daily mean stream DOC flux", KgPerDay);
	SetInitialValue(Model, DailyMeanStreamDOCFlux, 0.0);
	SetSolver(Model, DailyMeanStreamDOCFlux, SimplySolver);
	ResetEveryTimestep(Model, DailyMeanStreamDOCFlux);

	auto DOCConcentration = RegisterEquation(Model, "DOC concentration (volume weighted daily mean)", MgPerL);

	EQUATION(Model, StreamDOCMass,
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamDOCFlux, *Input);
		)
		return
			RESULT(SoilwaterCarbonFluxToReach)
			+ RESULT(GroundwaterFlow) * ConvertMgPerLToKgPerMm(PARAMETER(DeepSoilDOCConcentration), PARAMETER(CatchmentArea))
			+ upstreamflux
			- RESULT(StreamDOCFlux);		
	)
		
	EQUATION(Model, StreamDOCFlux,
		return RESULT(StreamDOCMass) * RESULT(ReachFlow) / RESULT(ReachVolume);
	)

	EQUATION(Model, DailyMeanStreamDOCFlux,
		return RESULT(StreamDOCFlux);
	)

	EQUATION(Model, DOCConcentration,
		return ConvertKgPerMmToMgPerL(RESULT(DailyMeanStreamDOCFlux) / RESULT(DailyMeanReachFlow), PARAMETER(CatchmentArea));
	)
	
	
}

