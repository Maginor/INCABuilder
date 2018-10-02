
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
	auto InitialSOCInUpperSoil = RegisterParameterDouble(Model, Land, "Initial solid organic carbon in upper soil box", KgPerM2, 2.0);
	auto InitialSOCInLowerSoil = RegisterParameterDouble(Model, Land, "Initial solid organic carbon in lower soil box", KgPerM2, 0.5);
	auto LitterFall = RegisterParameterDouble(Model, Land, "Litter fall", KgPerM2, 2.0);
	
	auto SoilsLand = GetParameterGroupHandle(Model, "Soils land");
	auto SorptionRate = RegisterParameterDouble(Model, SoilsLand, "Sorption rate", PerDay, 0.1);
	auto DesorptionRate = RegisterParameterDouble(Model, SoilsLand, "Desorption rate", PerDay, 0.01);
	auto BaseMineralisationRate = RegisterParameterDouble(Model, SoilsLand, "Base mineralisation rate", PerDay, 0.1);
	
	auto System = GetParameterGroupHandle(Model, "System");
	auto DegasVelocity = RegisterParameterDouble(Model, System, "Degas velocity", MetresPerDay, 15.0);
	auto DICConcentrationAtSaturation = RegisterParameterDouble(Model, System, "DIC concentration at saturation", KgPerM3, 0.018);
	auto MineralisationResponseToTemperature = RegisterParameterDouble(Model, System, "Mineralisation response to soil temperature", Dimensionless, 1.3);
	
	
	auto SoilSolver = RegisterSolver(Model, "Soil solver", 0.1, IncaDascru);

	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	auto Percent       = GetParameterDoubleHandle(Model, "%");
	auto FieldCapacity = GetParameterDoubleHandle(Model, "Field capacity");
	
	//auto SolarRadiation = RegisterInput(Model, "Solar radiation");
	
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature");
	auto RunoffFromBox   = GetEquationHandle(Model, "Runoff from box");
	auto SoilMoisture    = GetEquationHandle(Model, "Soil moisture");
	auto WaterInputToBox = GetEquationHandle(Model, "Water input to box");
	
	auto MineralisationRateInUpperSoilBox = RegisterEquation(Model, "Mineralisation rate in upper soil box", PerDay);
	auto MineralisationRateInLowerSoilBox = RegisterEquation(Model, "Mineralisation rate in lower soil box", PerDay);
	
	auto SOCInUpperSoilBox = RegisterEquationODE(Model, "SOC in upper soil box", Kg);
	SetSolver(Model, SOCInUpperSoilBox, SoilSolver);
	SetInitialValue(Model, SOCInUpperSoilBox, InitialSOCInUpperSoil);
	auto DOCInUpperSoilBox = RegisterEquationODE(Model, "DOC in upper soil box", Kg);
	SetSolver(Model, DOCInUpperSoilBox, SoilSolver);
	auto DICInUpperSoilBox = RegisterEquationODE(Model, "DIC in upper soil box", Kg);
	SetSolver(Model, DICInUpperSoilBox, SoilSolver);
	
	auto SOCInLowerSoilBox = RegisterEquationODE(Model, "SOC in lower soil box", Kg);
	SetSolver(Model, SOCInLowerSoilBox, SoilSolver);
	SetInitialValue(Model, SOCInLowerSoilBox, InitialSOCInLowerSoil);
	auto DOCInLowerSoilBox = RegisterEquationODE(Model, "DOC in lower soil box", Kg);
	SetSolver(Model, DOCInLowerSoilBox, SoilSolver);
	auto DICInLowerSoilBox = RegisterEquationODE(Model, "DIC in lower soil box", Kg);
	SetSolver(Model, DICInLowerSoilBox, SoilSolver);
	
	auto DOCFromLandscapeUnitToGroundwater = RegisterEquation(Model, "DOC from landscape unit to groundwater", KgPerDay);
	auto DICFromLandscapeUnitToGroundwater = RegisterEquation(Model, "DIC from landscape unit to groundwater", KgPerDay);
	
	
	EQUATION(Model, MineralisationRateInUpperSoilBox,
		return 
			PARAMETER(BaseMineralisationRate, UpperBox) 
		  * pow(PARAMETER(MineralisationResponseToTemperature), RESULT(SoilTemperature))
		  * (RESULT(SoilMoisture, UpperBox) / PARAMETER(FieldCapacity, UpperBox));
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
		  - RESULT(DOCInUpperSoilBox) * RESULT(RunoffFromBox, UpperBox) / RESULT(SoilMoisture, UpperBox);  // Outflow.
	)
	
	EQUATION(Model, DICInUpperSoilBox,
		double watervolumeinbox = (RESULT(SoilMoisture, UpperBox) / 1000.0) * (PARAMETER(CatchmentArea) * 1e6) * (PARAMETER(Percent) / 100.0);
		double dicconcentration = RESULT(DICInUpperSoilBox) / watervolumeinbox;
		double degas = PARAMETER(DegasVelocity) * (dicconcentration - PARAMETER(DICConcentrationAtSaturation));
		if(degas < 0.0) degas = 0.0;
		return
			RESULT(MineralisationRateInUpperSoilBox) * RESULT(DOCInUpperSoilBox)
		  - degas;
		  - RESULT(DICInUpperSoilBox) * RESULT(RunoffFromBox, UpperBox) / RESULT(SoilMoisture, UpperBox);  //Outflow	
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
	
	EQUATION(Model, DOCInLowerSoilBox,
		return
			- PARAMETER(SorptionRate, LowerBox) * RESULT(DOCInLowerSoilBox) 
			+ PARAMETER(DesorptionRate, LowerBox) * RESULT(SOCInLowerSoilBox)
			- RESULT(MineralisationRateInLowerSoilBox) * RESULT(DOCInLowerSoilBox)
			+ RESULT(DOCInUpperSoilBox) * RESULT(WaterInputToBox, LowerBox) / RESULT(SoilMoisture, UpperBox); //NOTE: Assumes all the input to the lower box comes from the upper box.
			- RESULT(DOCInLowerSoilBox) * RESULT(RunoffFromBox, LowerBox) / RESULT(SoilMoisture, LowerBox);
	)
	
	EQUATION(Model, DICInLowerSoilBox,
		return
			  RESULT(MineralisationRateInLowerSoilBox) * RESULT(DOCInLowerSoilBox)
			+ RESULT(DICInUpperSoilBox) * RESULT(WaterInputToBox, LowerBox) / RESULT(SoilMoisture, UpperBox); //NOTE: Assumes all the input to the lower box comes from the upper box.
			- RESULT(DICInLowerSoilBox) * RESULT(RunoffFromBox, LowerBox) / RESULT(SoilMoisture, LowerBox);
	)
	
	EQUATION(Model, DOCFromLandscapeUnitToGroundwater,
		//TODO: If some of this flow is diverted directly to the river we have to take that into account here
		return RESULT(DOCInLowerSoilBox) * RESULT(RunoffFromBox, LowerBox) / RESULT(SoilMoisture, LowerBox);
	)
	
	EQUATION(Model, DICFromLandscapeUnitToGroundwater,
		//TODO: If some of this flow is diverted directly to the river we have to take that into account here
		return RESULT(DICInLowerSoilBox) * RESULT(RunoffFromBox, LowerBox) / RESULT(SoilMoisture, LowerBox);
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
	
	auto DOCFromGroundwaterToReachBeforeRouting = RegisterEquation(Model, "DOC from groundwater to reach before routing", KgPerDay);
	auto DICFromGroundwaterToReachBeforeRouting = RegisterEquation(Model, "DIC from groundwater to reach before routing", KgPerDay);
	
	auto DOCFromGroundwaterToReach = RegisterEquation(Model, "DOC from groundwater to reach", KgPerDay);
	auto DICFromGroundwaterToReach = RegisterEquation(Model, "DIC from groundwater to reach", KgPerDay);

	
	EQUATION(Model, DOCInUpperGroundwaterStorage,
		return
			  RESULT(DOCToGroundwater)
			- RESULT(DOCInUpperGroundwaterStorage) * (RESULT(Percolation) + RESULT(UpperRunoff)) / RESULT(UpperStorage);
	)
	
	EQUATION(Model, DICInUpperGroundwaterStorage,
		return
			  RESULT(DICToGroundwater)
			- RESULT(DICInUpperGroundwaterStorage) * (RESULT(Percolation) + RESULT(UpperRunoff)) / RESULT(UpperStorage);
	)
	
	EQUATION(Model, DOCInLowerGroundwaterStorage,
		return
			  RESULT(DOCInUpperGroundwaterStorage) * RESULT(Percolation) / RESULT(UpperStorage)
			- RESULT(DOCInLowerGroundwaterStorage) * RESULT(LowerRunoff) / RESULT(LowerStorage);
	)
	
	EQUATION(Model, DICInLowerGroundwaterStorage,
		return
			  RESULT(DICInUpperGroundwaterStorage) * RESULT(Percolation) / RESULT(UpperStorage)
			- RESULT(DICInLowerGroundwaterStorage) * RESULT(LowerRunoff) / RESULT(LowerStorage);
	)
	
	EQUATION(Model, DOCFromGroundwaterToReachBeforeRouting,
		return
			  RESULT(DOCInUpperGroundwaterStorage) * RESULT(UpperRunoff) / RESULT(UpperStorage)
			+ RESULT(DOCInLowerGroundwaterStorage) * RESULT(LowerRunoff) / RESULT(LowerStorage);
	)
	
	EQUATION(Model, DICFromGroundwaterToReachBeforeRouting,
		return
			  RESULT(DICInUpperGroundwaterStorage) * RESULT(UpperRunoff) / RESULT(UpperStorage)
			+ RESULT(DICInLowerGroundwaterStorage) * RESULT(LowerRunoff) / RESULT(LowerStorage);
	)
	
	EQUATION(Model, DOCFromGroundwaterToReach,
		RESULT(DOCFromGroundwaterToReachBeforeRouting); //NOTE: To force a dependency since this is not automatic when we use EARLIER_RESULT;
	
		u64 M = PARAMETER(MaxBase);		
		double sum = 0.0;

		for(u64 I = 1; I <= M; ++I)
		{
			sum += RoutingCoefficient(M, I) * EARLIER_RESULT(DOCFromGroundwaterToReachBeforeRouting, I-1);
		}
			
		return sum;
	)
	
	EQUATION(Model, DICFromGroundwaterToReach,
		RESULT(DICFromGroundwaterToReachBeforeRouting); //NOTE: To force a dependency since this is not automatic when we use EARLIER_RESULT;
	
		u64 M = PARAMETER(MaxBase);		
		double sum = 0.0;

		for(u64 I = 1; I <= M; ++I)
		{
			sum += RoutingCoefficient(M, I) * EARLIER_RESULT(DICFromGroundwaterToReachBeforeRouting, I-1);
		}
			
		return sum;
	)
	
}

#define CARBON_MODEL_H
#endif