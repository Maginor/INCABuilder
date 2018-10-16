
#if !defined(CARBON_MODEL_H)


//NOTE: This is far from finished

static void
AddCarbonInSoilModule(inca_model *Model)
{
	auto Km2     = RegisterUnit(Model, "km2");
	auto M3      = RegisterUnit(Model, "m3");
	auto KgPerM2 = RegisterUnit(Model, "kg/m2");
	auto KgPerM2PerDay = RegisterUnit(Model, "kg/m2/day");
	auto PerDay  = RegisterUnit(Model, "/day");
	auto Kg      = RegisterUnit(Model, "kg");
	auto MetresPerDay = RegisterUnit(Model, "m/day");
	auto KgPerM3 = RegisterUnit(Model, "kg/m3");
	auto Dimensionless = RegisterUnit(Model);
	auto M3PerDay = RegisterUnit(Model, "m3/day");
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	
	auto SoilBoxes = GetIndexSetHandle(Model, "Soil boxes");
	auto UpperBox  = RequireIndex(Model, SoilBoxes, "Upper box");
	auto LowerBox  = RequireIndex(Model, SoilBoxes, "Lower box");
	
	auto Land   = GetParameterGroupHandle(Model, "Landscape units");
	auto SoilsLand  = GetParameterGroupHandle(Model, "Soils land");
	auto InitialSOCInSoilPerArea = RegisterParameterDouble(Model, SoilsLand, "Initial SOC in soil box", KgPerM2, 2.0);
	auto LitterFall = RegisterParameterDouble(Model, Land, "Litter fall", KgPerM2PerDay, 0.02, 0.0, 10.0, "Litter fall from the canopy to the upper soil layer");
	
	auto Soils        = GetParameterGroupHandle(Model, "Soils");
	auto SorptionRate = RegisterParameterDouble(Model, Soils, "Sorption rate", PerDay, 0.1, 0.0, 1.0, "Rate coefficient for DOC sorption (DOC to SOC)");
	auto DesorptionRate = RegisterParameterDouble(Model, Soils, "Desorption rate", PerDay, 1e-4, 0.0, 1.0, "Rate coefficient for SOC desorption (SOC to DOC)");
	auto BaseMineralisationRate = RegisterParameterDouble(Model, Soils, "Base mineralisation rate in soil", PerDay, 0.1);
	
	auto System = GetParameterGroupHandle(Model, "System");
	auto DegasVelocity = RegisterParameterDouble(Model, System, "Degas velocity", MetresPerDay, 15.0, 0.0, 100.0, "DIC mass transfer velocity from upper soil layer/river to the atmosphere");
	auto DICConcentrationAtSaturation = RegisterParameterDouble(Model, System, "DIC concentration at saturation", KgPerM3, 0.018, 0.0, 0.1, "If DIC concentration is higher than this degas sets in");
	auto MineralisationResponseToTemperature = RegisterParameterDouble(Model, System, "Mineralisation response to temperature", Dimensionless, 1.3);
	
	
	auto SoilSolver = RegisterSolver(Model, "Soil solver", 0.1, IncaDascru);

	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	auto Percent       = GetParameterDoubleHandle(Model, "%");
	auto FieldCapacity = GetParameterDoubleHandle(Model, "Field capacity");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature");
	auto RunoffFromBox   = GetEquationHandle(Model, "Runoff from box");
	auto PercolationFromBox = GetEquationHandle(Model, "Percolation from box");
	auto SoilMoisture2   = GetEquationHandle(Model, "Soil moisture (partial calculation) 2");
	auto SoilMoisture    = GetEquationHandle(Model, "Soil moisture");
	auto WaterInputToBox = GetEquationHandle(Model, "Water input to box");
	
	auto MineralisationRateInUpperSoilBox = RegisterEquation(Model, "Mineralisation rate in upper soil box", PerDay);
	auto MineralisationRateInLowerSoilBox = RegisterEquation(Model, "Mineralisation rate in lower soil box", PerDay);
	
	auto InitialSOCInUpperSoil = RegisterEquationInitialValue(Model, "Initial SOC in upper soil box", Kg);
	auto InitialSOCInLowerSoil = RegisterEquationInitialValue(Model, "Initial SOC in lower soil box", Kg);
	
	auto SOCInUpperSoilBox = RegisterEquationODE(Model, "SOC in upper soil box", Kg);
	SetSolver(Model, SOCInUpperSoilBox, SoilSolver);
	SetInitialValue(Model, SOCInUpperSoilBox, InitialSOCInUpperSoil); //TODO: Has to be converted from kg/m2 to kg
	auto DOCInUpperSoilBox = RegisterEquationODE(Model, "DOC in upper soil box", Kg);
	SetSolver(Model, DOCInUpperSoilBox, SoilSolver);
	auto DICInUpperSoilBox = RegisterEquationODE(Model, "DIC in upper soil box", Kg);
	SetSolver(Model, DICInUpperSoilBox, SoilSolver);
	
	auto SOCInLowerSoilBox = RegisterEquationODE(Model, "SOC in lower soil box", Kg);
	SetSolver(Model, SOCInLowerSoilBox, SoilSolver);
	SetInitialValue(Model, SOCInLowerSoilBox, InitialSOCInLowerSoil); //TODO: Has to be converted from kg/m2 to kg
	auto DOCInLowerSoilBox = RegisterEquationODE(Model, "DOC in lower soil box", Kg);
	SetSolver(Model, DOCInLowerSoilBox, SoilSolver);
	auto DICInLowerSoilBox = RegisterEquationODE(Model, "DIC in lower soil box", Kg);
	SetSolver(Model, DICInLowerSoilBox, SoilSolver);
	
	auto DOCFromLandscapeUnitToGroundwater = RegisterEquation(Model, "DOC from landscape unit to groundwater", KgPerDay);
	SetSolver(Model, DOCFromLandscapeUnitToGroundwater, SoilSolver);
	auto DICFromLandscapeUnitToGroundwater = RegisterEquation(Model, "DIC from landscape unit to groundwater", KgPerDay);
	SetSolver(Model, DICFromLandscapeUnitToGroundwater, SoilSolver);
	
	auto DOCFromRunoffInLandscapeUnit = RegisterEquation(Model, "DOC from runoff in landscape unit", KgPerDay);
	auto DICFromRunoffInLandscapeUnit = RegisterEquation(Model, "DIC from runoff in landscape unit", KgPerDay);
	
	auto DOCFromRunoffToRouting = RegisterEquationCumulative(Model, "DOC from runoff to routing", DOCFromRunoffInLandscapeUnit, LandscapeUnits);
	auto DICFromRunoffToRouting = RegisterEquationCumulative(Model, "DIC from runoff to routing", DICFromRunoffInLandscapeUnit, LandscapeUnits);
	
	EQUATION(Model, InitialSOCInUpperSoil,
		return PARAMETER(InitialSOCInSoilPerArea, UpperBox) * (PARAMETER(CatchmentArea) * 1e6) * (PARAMETER(Percent) / 100.0);
	)
	
	EQUATION(Model, InitialSOCInLowerSoil,
		return PARAMETER(InitialSOCInSoilPerArea, LowerBox) * (PARAMETER(CatchmentArea) * 1e6) * (PARAMETER(Percent) / 100.0);
	)
	
	EQUATION(Model, MineralisationRateInUpperSoilBox,
		return 
			PARAMETER(BaseMineralisationRate, UpperBox) 
		  * pow(PARAMETER(MineralisationResponseToTemperature), RESULT(SoilTemperature))
		  * (RESULT(SoilMoisture, UpperBox) / PARAMETER(FieldCapacity, UpperBox));       //TODO: We should think about whether we want to use LAST_RESULT(SoilMoisture) here.
	)
	
	EQUATION(Model, SOCInUpperSoilBox,
		return 
			  PARAMETER(LitterFall) * (PARAMETER(CatchmentArea)*1e6) * (PARAMETER(Percent)/100.0)
			+ PARAMETER(SorptionRate, UpperBox) * RESULT(DOCInUpperSoilBox) 
			- PARAMETER(DesorptionRate, UpperBox) * RESULT(SOCInUpperSoilBox);
	)
	
	EQUATION(Model, DOCInUpperSoilBox,
		return 
			PARAMETER(DesorptionRate, UpperBox) * RESULT(SOCInUpperSoilBox)           //Desorption
		  - PARAMETER(SorptionRate, UpperBox) * RESULT(DOCInUpperSoilBox)             //Sorption
		  - RESULT(DOCInUpperSoilBox) * RESULT(MineralisationRateInUpperSoilBox);                          //Mineralisation  
		  - RESULT(DOCInUpperSoilBox) * DivideIfNotZero(RESULT(PercolationFromBox, UpperBox), RESULT(SoilMoisture2, UpperBox));
	)
	
	EQUATION(Model, DICInUpperSoilBox,
		double watervolumeinbox = (RESULT(SoilMoisture, UpperBox) / 1000.0) * (PARAMETER(CatchmentArea) * 1e6) * (PARAMETER(Percent) / 100.0); //TODO: We should think about whether we want to use LAST_RESULT(SoilMoisture) here.
		double degas = 0.0;
		double degasvelocity = PARAMETER(DegasVelocity);
		double dicconcentrationatsaturation = PARAMETER(DICConcentrationAtSaturation);
		if(watervolumeinbox > 0.0)
		{
			double dicconcentration = RESULT(DICInUpperSoilBox) / watervolumeinbox;
			degas = degasvelocity * (dicconcentration - dicconcentrationatsaturation);
			degas = Min(degas, RESULT(DICInUpperSoilBox));
		}		
		return
			RESULT(MineralisationRateInUpperSoilBox) * RESULT(DOCInUpperSoilBox)
		  - degas;
		  - RESULT(DICInUpperSoilBox) * DivideIfNotZero(RESULT(PercolationFromBox, UpperBox), RESULT(SoilMoisture2, UpperBox));
	)
	
	EQUATION(Model, MineralisationRateInLowerSoilBox,
		return 
			PARAMETER(BaseMineralisationRate, UpperBox) 
		  * pow(PARAMETER(MineralisationResponseToTemperature), RESULT(SoilTemperature));
	)
	
	EQUATION(Model, SOCInLowerSoilBox,
		return
			  PARAMETER(SorptionRate, LowerBox) * RESULT(DOCInLowerSoilBox) 
			- PARAMETER(DesorptionRate, LowerBox) * RESULT(SOCInLowerSoilBox);
	)
	
	EQUATION(Model, DOCFromLandscapeUnitToGroundwater,
		return RESULT(DOCInLowerSoilBox) * DivideIfNotZero(RESULT(PercolationFromBox, LowerBox), RESULT(SoilMoisture2, LowerBox));
	)
	
	EQUATION(Model, DICFromLandscapeUnitToGroundwater,
		return RESULT(DICInLowerSoilBox) * DivideIfNotZero(RESULT(PercolationFromBox, LowerBox), RESULT(SoilMoisture2, LowerBox));
	)
	
	EQUATION(Model, DOCInLowerSoilBox,
		return
			- PARAMETER(SorptionRate, LowerBox) * RESULT(DOCInLowerSoilBox) 
			+ PARAMETER(DesorptionRate, LowerBox) * RESULT(SOCInLowerSoilBox)
			- RESULT(MineralisationRateInLowerSoilBox) * RESULT(DOCInLowerSoilBox)
			+ RESULT(DOCInUpperSoilBox) * DivideIfNotZero(RESULT(PercolationFromBox, UpperBox) - RESULT(RunoffFromBox, LowerBox), RESULT(SoilMoisture2, UpperBox)); //NOTE: The runoff from the lower box is just water that comes from the upper box and can not enter the lower box because it is too full.
			- RESULT(DOCFromLandscapeUnitToGroundwater);
	)
	
	EQUATION(Model, DICInLowerSoilBox,
		return
			  RESULT(MineralisationRateInLowerSoilBox) * RESULT(DOCInLowerSoilBox)
			+ + RESULT(DICInUpperSoilBox) * DivideIfNotZero(RESULT(PercolationFromBox, UpperBox) - RESULT(RunoffFromBox, LowerBox), RESULT(SoilMoisture2, UpperBox)); //NOTE: The runoff from the lower box is just water that comes from the upper box and can not enter the lower box because it is too full.
			- RESULT(DICFromLandscapeUnitToGroundwater);
	)
	
	EQUATION(Model, DOCFromRunoffInLandscapeUnit,
		return RESULT(DOCInUpperSoilBox) * DivideIfNotZero(RESULT(RunoffFromBox, LowerBox), RESULT(SoilMoisture2, UpperBox)); //This is as intended. The runoff from the lower box is water that comes from the upper box but is diverted before entering the lower box.
	)
	
	EQUATION(Model, DICFromRunoffInLandscapeUnit,
		return RESULT(DICInUpperSoilBox) * DivideIfNotZero(RESULT(RunoffFromBox, LowerBox), RESULT(SoilMoisture2, UpperBox)); //This is as intended. The runoff from the lower box is water that comes from the upper box but is diverted before entering the lower box.
	)
}

static void
AddCarbonInGroundwaterModule(inca_model *Model)
{
	auto Kg       = RegisterUnit(Model, "kg");
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	
	auto GroundwaterSolver = GetSolverHandle(Model, "Groundwater solver");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	
	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	auto MaxBase       = GetParameterUIntHandle(Model, "Flow routing max base");
	
	auto DOCFromLandscapeUnitToGroundwater = GetEquationHandle(Model, "DOC from landscape unit to groundwater");
	auto DICFromLandscapeUnitToGroundwater = GetEquationHandle(Model, "DIC from landscape unit to groundwater");
	auto Percolation                       = GetEquationHandle(Model, "Percolation from upper to lower groundwater storage");
	auto UpperRunoff                       = GetEquationHandle(Model, "Runoff from upper groundwater storage");
	auto LowerRunoff                       = GetEquationHandle(Model, "Runoff from lower groundwater storage");
	auto UpperStorage                      = GetEquationHandle(Model, "Upper groundwater storage");
	auto LowerStorage                      = GetEquationHandle(Model, "Lower groundwater storage");
	
	auto DOCToGroundwater = RegisterEquationCumulative(Model, "DOC to Groundwater", DOCFromLandscapeUnitToGroundwater, LandscapeUnits);
	auto DICToGroundwater = RegisterEquationCumulative(Model, "DIC to Groundwater", DICFromLandscapeUnitToGroundwater, LandscapeUnits);
	
	auto DOCInUpperGroundwaterStorage = RegisterEquationODE(Model, "DOC in upper groundwater storage", Kg);
	SetSolver(Model, DOCInUpperGroundwaterStorage, GroundwaterSolver);
	auto DICInUpperGroundwaterStorage = RegisterEquationODE(Model, "DIC in upper groundwater storage", Kg);
	SetSolver(Model, DICInUpperGroundwaterStorage, GroundwaterSolver);
	auto DOCInLowerGroundwaterStorage = RegisterEquationODE(Model, "DOC in lower groundwater storage", Kg);
	SetSolver(Model, DOCInLowerGroundwaterStorage, GroundwaterSolver);
	auto DICInLowerGroundwaterStorage = RegisterEquationODE(Model, "DIC in lower groundwater storage", Kg);
	SetSolver(Model, DICInLowerGroundwaterStorage, GroundwaterSolver);
	
	auto DOCFromGroundwaterToRouting = RegisterEquation(Model, "DOC from groundwater to routing routine", KgPerDay);
	auto DICFromGroundwaterToRouting = RegisterEquation(Model, "DIC from groundwater to routing routine", KgPerDay);

	
	EQUATION(Model, DOCInUpperGroundwaterStorage,
		
		return
			  RESULT(DOCToGroundwater)
			- DivideIfNotZero(RESULT(DOCInUpperGroundwaterStorage) * (RESULT(Percolation) + RESULT(UpperRunoff)), RESULT(UpperStorage));
	)
	
	EQUATION(Model, DICInUpperGroundwaterStorage,
		return
			  RESULT(DICToGroundwater)
			- DivideIfNotZero(RESULT(DICInUpperGroundwaterStorage) * (RESULT(Percolation) + RESULT(UpperRunoff)), RESULT(UpperStorage));
	)
	
	EQUATION(Model, DOCInLowerGroundwaterStorage,
		return
			  DivideIfNotZero(RESULT(DOCInUpperGroundwaterStorage) * RESULT(Percolation), RESULT(UpperStorage))
			- DivideIfNotZero(RESULT(DOCInLowerGroundwaterStorage) * RESULT(LowerRunoff), RESULT(LowerStorage));
	)
	
	EQUATION(Model, DICInLowerGroundwaterStorage,
		return
			  DivideIfNotZero(RESULT(DICInUpperGroundwaterStorage) * RESULT(Percolation), RESULT(UpperStorage))
			- DivideIfNotZero(RESULT(DICInLowerGroundwaterStorage) * RESULT(LowerRunoff), RESULT(LowerStorage));
	)
	
	EQUATION(Model, DOCFromGroundwaterToRouting,
		return
			  DivideIfNotZero(RESULT(DOCInUpperGroundwaterStorage) * RESULT(UpperRunoff), RESULT(UpperStorage))
			+ DivideIfNotZero(RESULT(DOCInLowerGroundwaterStorage) * RESULT(LowerRunoff), RESULT(LowerStorage));
	)
	
	EQUATION(Model, DICFromGroundwaterToRouting,
		return
			  DivideIfNotZero(RESULT(DICInUpperGroundwaterStorage) * RESULT(UpperRunoff), RESULT(UpperStorage))
			+ DivideIfNotZero(RESULT(DICInLowerGroundwaterStorage) * RESULT(LowerRunoff), RESULT(LowerStorage));
	)	
}

static void
AddCarbonRoutingRoutine(inca_model *Model)
{
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	
	auto DOCToRouting = RegisterEquation(Model, "DOC to routing", KgPerDay);
	auto DICToRouting = RegisterEquation(Model, "DIC to routing", KgPerDay);
	
	auto DOCFromRoutingToReach = RegisterEquation(Model, "DOC from routing to reach", KgPerDay);
	auto DICFromRoutingToReach = RegisterEquation(Model, "DIC from routing to reach", KgPerDay);
	
	
	auto DOCFromGroundwaterToRouting = GetEquationHandle(Model, "DOC from groundwater to routing routine");
	auto DICFromGroundwaterToRouting = GetEquationHandle(Model, "DIC from groundwater to routing routine");
	auto DOCFromRunoffToRouting = GetEquationHandle(Model, "DOC from runoff to routing");
	auto DICFromRunoffToRouting = GetEquationHandle(Model, "DIC from runoff to routing");
	
	auto MaxBase = GetParameterUIntHandle(Model, "Flow routing max base");
	
	EQUATION(Model, DOCToRouting,
		return
			  RESULT(DOCFromGroundwaterToRouting)
			+ RESULT(DOCFromRunoffToRouting);
	)
	
	EQUATION(Model, DICToRouting,
		return
			  RESULT(DICFromGroundwaterToRouting)
			+ RESULT(DICFromRunoffToRouting);
	)
	
	EQUATION(Model, DOCFromRoutingToReach,
		RESULT(DOCToRouting); //NOTE: To force a dependency since this is not automatic when we use EARLIER_RESULT;
	
		u64 M = PARAMETER(MaxBase);		
		double sum = 0.0;

		for(u64 I = 1; I <= M; ++I)
		{
			sum += RoutingCoefficient(M, I) * EARLIER_RESULT(DOCToRouting, I-1);
		}
			
		return sum;
	)
	
	EQUATION(Model, DICFromRoutingToReach,
		RESULT(DICToRouting); //NOTE: To force a dependency since this is not automatic when we use EARLIER_RESULT;
	
		u64 M = PARAMETER(MaxBase);		
		double sum = 0.0;

		for(u64 I = 1; I <= M; ++I)
		{
			sum += RoutingCoefficient(M, I) * EARLIER_RESULT(DICToRouting, I-1);
		}
			
		return sum;
	)
}

static void
AddCarbonInReachModule(inca_model *Model)
{
	auto PerDay   = RegisterUnit(Model, "/day");
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	auto Kg       = RegisterUnit(Model, "kg");
	auto Dimensionless = RegisterUnit(Model);
	auto KgPerM3  = RegisterUnit(Model, "kg/m3");
	
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature");
	auto ReachVolume      = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow        = GetEquationHandle(Model, "Reach flow");
	
	auto MineralisationResponseToTemperature = GetParameterDoubleHandle(Model, "Mineralisation response to temperature");
	auto DegasVelocity = GetParameterDoubleHandle(Model, "Degas velocity");
	auto DICConcentrationAtSaturation = GetParameterDoubleHandle(Model, "DIC concentration at saturation");
	
	auto DOCFromRoutingToReach = GetEquationHandle(Model, "DOC from routing to reach");
	auto DICFromRoutingToReach = GetEquationHandle(Model, "DIC from routing to reach");
	
	auto Reach           = GetIndexSetHandle(Model, "Reaches");
	auto ReachParameters = GetParameterGroupHandle(Model, "Reach parameters");
	
	auto ReachSolver     = GetSolverHandle(Model, "Reach solver");
	
	auto BaseMineralisationRateInReach = RegisterParameterDouble(Model, ReachParameters, "Base mineralisation rate in reach", PerDay, 1e-5);
	auto Sigma1                        = RegisterParameterDouble(Model, ReachParameters, "Sigma 1", Dimensionless, 1e-3, 1e-4, 1e-2, "Parameter used for determining the rate of photo-oxidation in reach");   //TODO: Figure out dimension later
	auto Sigma2                        = RegisterParameterDouble(Model, ReachParameters, "Sigma 2", Dimensionless, 1e-4, 1e-5, 1e-3, "Parameter used for determining the rate of photo-oxidation in reach");   //TODO: Figure out dimension later
	
	auto SolarRadiation = RegisterInput(Model, "Solar radiation");
	
	auto MineralisationRateInReach = RegisterEquation(Model, "Mineralisation rate in reach", PerDay);
	auto PhotoOxidationRateInReach = RegisterEquation(Model, "Photo-oxidation rate in reach", PerDay);
	SetSolver(Model, PhotoOxidationRateInReach, ReachSolver);
	auto DOCInputToReach           = RegisterEquation(Model, "DOC input to reach", KgPerDay);
	auto DICInputToReach           = RegisterEquation(Model, "DIC input to reach", KgPerDay);
	
	auto DOCOutputFromReach        = RegisterEquation(Model, "DOC output from reach", KgPerDay);
	SetSolver(Model, DOCOutputFromReach, ReachSolver);
	auto DICOutputFromReach        = RegisterEquation(Model, "DIC output from reach", KgPerDay);
	SetSolver(Model, DICOutputFromReach, ReachSolver);
	
	auto DOCInReach                = RegisterEquationODE(Model, "DOC in reach", Kg);
	SetSolver(Model, DOCInReach, ReachSolver);
	//TODO: Initial value?
	auto DICInReach                = RegisterEquationODE(Model, "DIC in reach", Kg);
	SetSolver(Model, DICInReach, ReachSolver);
	//TODO: Initial value?
	
	auto DOCConcentrationInReach = RegisterEquation(Model, "DOC concentration in reach", KgPerM3);
	SetSolver(Model, DOCConcentrationInReach, ReachSolver);
	auto DICConcentrationInReach = RegisterEquation(Model, "DIC concentration in reach", KgPerM3);
	SetSolver(Model, DICConcentrationInReach, ReachSolver);
	
	EQUATION(Model, MineralisationRateInReach,
		return 
			  PARAMETER(BaseMineralisationRateInReach)
			* pow(RESULT(WaterTemperature), PARAMETER(MineralisationResponseToTemperature));
	)
	
	EQUATION(Model, PhotoOxidationRateInReach,
		return (PARAMETER(Sigma1) * INPUT(SolarRadiation)) / (PARAMETER(Sigma2) + RESULT(DOCConcentrationInReach));
	)
	
	EQUATION(Model, DOCInputToReach,
		double docinput = RESULT(DOCFromRoutingToReach);
		// TODO: Effluent input?
		FOREACH_INPUT(Reach,
			docinput += RESULT(DOCOutputFromReach, *Input);
		)
		
		return docinput;
	)
	
	EQUATION(Model, DICInputToReach,
		double dicinput = RESULT(DICFromRoutingToReach);
		// TODO: Effluent input?
		FOREACH_INPUT(Reach,
			dicinput += RESULT(DICOutputFromReach, *Input);
		)
		
		return dicinput;
	)
	
	EQUATION(Model, DOCOutputFromReach,
		return RESULT(DOCInReach) * DivideIfNotZero(RESULT(ReachFlow), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DICOutputFromReach,
		return RESULT(DICInReach) * DivideIfNotZero(RESULT(ReachFlow), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DOCInReach,
		return
			  RESULT(DOCInputToReach)
			- RESULT(DOCOutputFromReach)
			- RESULT(DOCInReach) * (RESULT(PhotoOxidationRateInReach) + RESULT(MineralisationRateInReach));
	)
	
	EQUATION(Model, DOCConcentrationInReach,
		return DivideIfNotZero(RESULT(DOCInReach), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DICInReach,
		return
			  RESULT(DICInputToReach)
			- RESULT(DICOutputFromReach)
			+ RESULT(DOCInReach) * (RESULT(PhotoOxidationRateInReach) + RESULT(MineralisationRateInReach));
			- PARAMETER(DegasVelocity) * (RESULT(DICInReach) / RESULT(ReachVolume) - PARAMETER(DICConcentrationAtSaturation));
	)
	
	EQUATION(Model, DICConcentrationInReach,
		return DivideIfNotZero(RESULT(DICInReach), RESULT(ReachVolume));
	)
}

static void
AddSimplyCModel(inca_model *Model)
{
	AddCarbonInSoilModule(Model);
	AddCarbonInGroundwaterModule(Model);
	AddCarbonRoutingRoutine(Model);
	AddCarbonInReachModule(Model);
}

#define CARBON_MODEL_H
#endif