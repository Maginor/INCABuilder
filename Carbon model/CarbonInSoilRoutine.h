
#if !defined(CARBON_IN_SOIL_H)

static void
AddCarbonInSoilModule(inca_model *Model)
{
	auto KgPerM2 = RegisterUnit(Model, "kg/m2");
	auto KgPerM2PerDay = RegisterUnit(Model, "kg/m2/day");
	auto PerDay  = RegisterUnit(Model, "/day");
	auto Kg      = RegisterUnit(Model, "kg");
	auto MetresPerDay = RegisterUnit(Model, "m/day");
	auto KgPerM3 = RegisterUnit(Model, "kg/m3");
	
	auto Land   = GetParameterGroupHandle(Model, "Landscape units");
	auto InitialSOCInSoil = RegisterParameterDouble(Model, Land, "Initial solid organic carbon in soil.", KgPerM2, 2.0);
	auto LitterFall = RegisterParameterDouble(Model, Land, "Litter fall", KgPerM2, 2.0);
	
	auto SoilsLand = GetParameterGroupHandle(Model, "Soils land");
	auto SorptionRate = RegisterParameterDouble(Model, SoilsLand, "Sorption rate", PerDay, 0.1);
	auto DesorptionRate = RegisterParameterDouble(Model, SoilsLand, "Desorption rate", PerDay, 0.01);
	auto BaseMineralisationRate = RegisterParameterDouble(Model, SoilsLand, "Base mineralisation rate", PerDay, 0.1);
	
	auto DegasVelocity = RegisterParameterDouble(Model, System, "Degas velocity", 15.0);
	auto DICConcentrationAtSaturation = RegisterParameterDouble(Model, System, "DIC concentration at saturation", KgPerM3, 0.018);
	auto MineralisationResponseToTemperature = RegisterParameterDouble(Model, System, "Mineralisation response to soil temperature", Dimensionless, 1.3);
	
	auto SolarRadiation = RegisterInput(Model, "Solar radiation");
	
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature");
	
	auto MineralisationRateInUpperSoilBox = RegisterEquation(Model, "Mineralisation rate in upper soil box", PerDay);
	auto SOCInUpperSoilBox = RegisterEquationODE(Model, "SOC in upper soil box", Kg);
	auto DOCInUpperSoilBox = RegisterEquationODE(Model, "DOC in upper soil box", Kg);
	auto DICInUpperSoilBox = RegisterEquationODE(Model, "DIC in upper soil box", Kg);
	
	EQUATION(Model, MineralisationRateInUpperSoilBox,
		return 
			PARAMETER(BaseMineralisationRate, UpperBox) 
		  * pow(RESULT(SoilTemperature), PARAMETER(MineralisationResponseToTemperature))
		  * (PARAMETER(FieldCapacity, UpperBox) - RESULT(SoilMoisture, UpperBox)) / PARAMETER(FieldCapacity, UpperBox);
	)
	
	EQUATION(Model, SOCInUpperSoilBox,
		return PARAMETER(LitterFall) + PARAMETER(Sorption, UpperBox) * RESULT(DOCInUpperSoilBox) - PARAMETER(Desorption, UpperBox) * RESULT(SOCInUpperSoilBox);
	)
	
	EQUATION(Model, DOCInUpperSoilBox,
		return 
			PARAMETER(Desorption, UpperBox) * RESULT(SOCInUpperSoilBox) 
		  - PARAMETER(Sorption, UpperBox) * RESULT(DOCInUpperSoilBox)
		  - RESULT(MineralisationRateInUpperSoilBox) * RESULT(DOCInUpperSoilBox);
		  // - outflow.
	)
	
	EQUATION(Model, DICInUpperSoilBox,
		return
			RESULT(MineralisationRateInUpperSoilBox) * RESULT(DOCInUpperSoilBox)
		  - PARAMETER(DegasVelocity) * (RESULT(DICInUpperSoilBox) / VOLUME!!! - PARAMETER(DICConcentrationAtSaturation));
		  // - outflow
			
	)
}


#define CARBON_IN_SOIL_H
#endif