
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
	
	auto WaterVolumeInSoilBox = RegisterEquation(Model, "Water volume in soil box", M3);
	auto FlowFromBox          = RegisterEquation(Model, "Flow from soil box", M3PerDay);
	auto MineralisationRateInUpperSoilBox = RegisterEquation(Model, "Mineralisation rate in upper soil box", PerDay);
	
	auto SOCInUpperSoilBox = RegisterEquationODE(Model, "SOC in upper soil box", Kg);
	SetSolver(Model, SOCInUpperSoilBox, SoilSolver);
	SetInitialValue(Model, SOCInUpperSoilBox, InitialSOCInUpperSoil);
	auto DOCInUpperSoilBox = RegisterEquationODE(Model, "DOC in upper soil box", Kg);
	SetSolver(Model, DOCInUpperSoilBox, SoilSolver);
	auto DICInUpperSoilBox = RegisterEquationODE(Model, "DIC in upper soil box", Kg);
	SetSolver(Model, DICInUpperSoilBox, SoilSolver);
	
	EQUATION(Model, WaterVolumeInSoilBox,
		return 
			(RESULT(SoilMoisture)/1000.0) *    //NOTE: Convert mm to m
			(PARAMETER(CatchmentArea)*1e6) *   //NOTE: Convert km2 to m2
			(PARAMETER(Percent)/100.0);
	)
	
	EQUATION(Model, FlowFromBox,
		return 
			(RESULT(RunoffFromBox) / 1000.0) *    //NOTE: Convert mm/day to m/day
			(PARAMETER(CatchmentArea)*1e6) *      //NOTE: Convert km2 to m2
			(PARAMETER(Percent)/100.0);
	)
	
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
		  - RESULT(MineralisationRateInUpperSoilBox) * RESULT(DOCInUpperSoilBox); //Mineralisation
		  
		  - RESULT(FlowFromBox, UpperBox) * RESULT(DOCInUpperSoilBox) / RESULT(WaterVolumeInSoilBox, UpperBox);  // Outflow.
	)
	
	EQUATION(Model, DICInUpperSoilBox,
		double dicconcentration = RESULT(DICInUpperSoilBox) / RESULT(WaterVolumeInSoilBox, UpperBox);
		double degas = PARAMETER(DegasVelocity) * (dicconcentration - PARAMETER(DICConcentrationAtSaturation));
		if(degas < 0.0) degas = 0.0;
		return
			RESULT(MineralisationRateInUpperSoilBox) * RESULT(DOCInUpperSoilBox)
		  - degas;
		  - RESULT(FlowFromBox, UpperBox) * RESULT(DICInUpperSoilBox) / RESULT(WaterVolumeInSoilBox, UpperBox);  //Outflow	
	)
}


#define CARBON_MODEL_H
#endif