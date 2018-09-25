
#if !defined(SOIL_MOISTURE_ROUTINE_H)

void
AddSoilMoistureRoutine(inca_model *Model)
{
	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto Dimensionless     = RegisterUnit(Model);
	
	auto LandscapeUnits    = RegisterIndexSet(Model, "Landscape units");
	auto SoilBoxes         = RegisterIndexSet(Model, "Soil boxes");
	
	auto FirstBox  = RequireIndex(Model, SoilBoxes, "First box");
	auto SecondBox = RequireIndex(Model, SoilBoxes, "Second box");
	
	auto SoilsLand         = RegisterParameterGroup(Model, "Soils land", LandscapeUnits);
	auto Soils             = RegisterParameterGroup(Model, "Soils", SoilBoxes);
	SetParentGroup(Model, Soils, SoilsLand);
	
	auto Reaches = RegisterIndexSetBranched(Model, "Reaches");
	auto ReachParameters = RegisterParameterGroup(Model, "Reach parameters", Reaches);
	auto LandscapePercentages = RegisterParameterGroup(Model, "Landscape percentages", LandscapeUnits);
	SetParentGroup(Model, LandscapePercentages, ReachParameters);
	
	auto Percent = RegisterParameterDouble(Model, LandscapePercentages, "%", Dimensionless, 20.0, 0.0, 100.0);
	
	//TODO: find good default (and min/max) values for these:
	auto SoilMoistureEvapotranspirationMax  = RegisterParameterDouble(Model, Soils, "Fraction of field capacity where evapotranspiration reaches its maximal", Dimensionless, 0.7);
	auto FieldCapacity = RegisterParameterDouble(Model, Soils, "Field capacity", Mm, 150.0, 100.0, 300.0, "Maximum soil moisture storage");
	auto RelativeRunoffExponent = RegisterParameterDouble(Model, Soils, "Relative runoff exponent", Dimensionless, 2.0, 1.0, 10.0, "Power parameter that determines the relative contribution to runoff");
	auto InitialSoilMoisture = RegisterParameterDouble(Model, Soils, "Initial soil moisture", Mm, 70.0); //TODO: Have an equation for this instead?
	
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	
	auto WaterToSoil = GetEquationHandle(Model, "Water to soil"); //NOTE: from the snow model.
	
	auto PotentialEvapotranspiration = GetEquationHandle(Model, "Potential evapotranspiration"); //NOTE: from the potentialevapotranspiration module.
	
	auto EvapotranspirationFraction  = RegisterEquation(Model, "Evapotranspiration fraction", Dimensionless);
	auto RunoffFraction              = RegisterEquation(Model, "Runoff fraction", Dimensionless);
	auto Evapotranspiration          = RegisterEquation(Model, "Evapotranspiration", MmPerDay);
	auto RunoffFromBox               = RegisterEquation(Model, "Runoff from box", MmPerDay);
	auto SoilMoisture                = RegisterEquation(Model, "Soil moisture", Mm);
	SetInitialValue(Model, SoilMoisture, InitialSoilMoisture);
	auto WaterInputToBox             = RegisterEquation(Model, "Water input to box", MmPerDay);
	auto GroundwaterRechargeFromLandscapeUnit = RegisterEquation(Model, "Groundwater recharge from landscape unit", MmPerDay);
	auto GroundwaterRecharge    = RegisterEquationCumulative(Model, "Groundwater recharge", GroundwaterRechargeFromLandscapeUnit, LandscapeUnits);
	
	auto Dummy                       = RegisterEquationCumulative(Model, "Dummy", SoilMoisture, SoilBoxes); //NOTE: stupid hack used to force GroundwaterRechargeFromUnit into the right place.
	
	EQUATION(Model, EvapotranspirationFraction,
		double smmax = PARAMETER(SoilMoistureEvapotranspirationMax) * PARAMETER(FieldCapacity);
		double potentialetpfraction = Min(LAST_RESULT(SoilMoisture), smmax) / smmax;
		if(CURRENT_INDEX(SoilBoxes) == FirstBox) return potentialetpfraction;
		return 0.0;
	)
	
	EQUATION(Model, RunoffFraction,
		return pow(LAST_RESULT(SoilMoisture) / PARAMETER(FieldCapacity), PARAMETER(RelativeRunoffExponent));
	)
	
	EQUATION(Model, Evapotranspiration,
		return RESULT(PotentialEvapotranspiration) * RESULT(EvapotranspirationFraction);
	)
	
	EQUATION(Model, WaterInputToBox,
		double fromPrecipitationOrSnow = RESULT(WaterToSoil);
		double fromBoxAbove = RESULT(RunoffFromBox, FirstBox);
		if(CURRENT_INDEX(SoilBoxes) == FirstBox) return fromPrecipitationOrSnow;
		return fromBoxAbove;
	)
	
	EQUATION(Model, RunoffFromBox,
		return RESULT(WaterInputToBox) * RESULT(RunoffFraction);
	)
	
	//TODO: overland flow if Soil moisture > field capacity.
	
	EQUATION(Model, SoilMoisture,
		return LAST_RESULT(SoilMoisture) - RESULT(Evapotranspiration) + RESULT(WaterInputToBox) * (1.0 - RESULT(RunoffFraction));
	)

	
	EQUATION(Model, GroundwaterRechargeFromLandscapeUnit,
		RESULT(Dummy); //TODO: Without this the equation is misplaced in the run tree since it falls outside the current dependency system. We need to fix the dependency system!!
		
		return RESULT(RunoffFromBox, SecondBox) * PARAMETER(Percent) / 100.0;
	)
}

#define SOIL_MOISTURE_MODEL_H
#endif