
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
	
	auto SoilsLand         = RegisterParameterGroup(Model, "Soils land", LandscapeUnits);
	auto Soils             = RegisterParameterGroup(Model, "Soils", SoilBoxes);
	SetParentGroup(Model, Soils, SoilsLand);
	
	auto Reaches = RegisterIndexSetBranched(Model, "Reaches");
	auto ReachParameters = RegisterParameterGroup(Model, "Reach parameters", Reaches);
	auto LandscapePercentages = RegisterParameterGroup(Model, "Landscape percentages", LandscapeUnits);
	SetParentGroup(Model, LandscapePercentages, ReachParameters);
	
	auto Percent = RegisterParameterDouble(Model, LandscapePercentages, "%", Dimensionless, 20.0, 0.0, 100.0);
	
	auto EvaporationConstant    = RegisterParameterDouble(Model, Soils, "Evaporation constant", Dimensionless, 0.1, 0.0, 0.3, "Linear parameter for the calculation of potential evapotranspiration");
	auto EvaporationExponent    = RegisterParameterDouble(Model, Soils, "Evaporation exponent", Dimensionless, 1.0, 1.0, 3.0, "Power parameter for the calculation of potential evapotranspiration");
	//TODO: find good default (and min/max) values for these:
	auto SoilMoistureEvaporationMax  = RegisterParameterDouble(Model, Soils, "Fraction of field capacity where evaporation reaches its maximal", Dimensionless, 0.7);
	auto FieldCapacity = RegisterParameterDouble(Model, Soils, "Field capacity", Mm, 150.0, 100.0, 300.0, "Maximum soil moisture storage");
	auto RelativeRunoffExponent = RegisterParameterDouble(Model, Soils, "Relative runoff exponent", Dimensionless, 2.0, 1.0, 10.0, "Power parameter that determines the relative contribution to runoff");
	auto InitialSoilMoisture = RegisterParameterDouble(Model, Soils, "Initial soil moisture", Mm, 70.0); //TODO: Have an equation for this instead?
	
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	
	auto WaterInput = GetEquationHandle(Model, "Water input to soil"); //NOTE: from the snow model.
	
	auto PotentialEvaporation = RegisterEquation(Model, "Potential evaporation", MmPerDay);
	auto EvaporationFraction  = RegisterEquation(Model, "Evaporation fraction", Dimensionless);
	auto RunoffFraction       = RegisterEquation(Model, "Runoff fraction", Dimensionless);
	auto Evaporation          = RegisterEquation(Model, "Evaporation", MmPerDay);
	auto RunoffFromBox        = RegisterEquation(Model, "Runoff from box", MmPerDay);
	auto SoilMoisture         = RegisterEquation(Model, "Soil moisture", Mm);
	SetInitialValue(Model, SoilMoisture, InitialSoilMoisture);
	auto WaterInputToBox      = RegisterEquation(Model, "Water input to box", MmPerDay);
	auto GroundwaterRechargeFromBox  = RegisterEquation(Model, "Groundwater recharge from box", MmPerDay);
	auto GroundwaterRechargeFromUnit = RegisterEquationCumulative(Model, "Groundwater recharge from landscape unit", GroundwaterRechargeFromBox, SoilBoxes);
	auto PercentAdjustedGroundwaterRechargeFromUnit = RegisterEquation(Model, "Percent adjusted groundwater recharge from landscape unit", MmPerDay);
	auto TotalGroundwaterRecharge = RegisterEquationCumulative(Model, "Total groundwater recharge", PercentAdjustedGroundwaterRechargeFromUnit, LandscapeUnits);
	
	
	//TODO: Potentialetp as input. Make a separate routine for potential evapotranspiration
	
	EQUATION(Model, PotentialEvaporation,
		double etp = PARAMETER(EvaporationConstant) * INPUT(AirTemperature) * pow(4.5, PARAMETER(EvaporationExponent)); //TODO: Where does 4.5 come from? Should be parameter?
		etp = etp < 0.0 ? 0.0 : etp;
		return etp;
	)
	
	EQUATION(Model, EvaporationFraction,
		double smmax = PARAMETER(SoilMoistureEvaporationMax) * PARAMETER(FieldCapacity);
		return Min(LAST_RESULT(SoilMoisture), smmax) / smmax;
	)
	
	EQUATION(Model, RunoffFraction,
		return pow(LAST_RESULT(SoilMoisture) / PARAMETER(FieldCapacity), PARAMETER(RelativeRunoffExponent));
	)
	
	EQUATION(Model, Evaporation,
		return RESULT(PotentialEvaporation) * RESULT(EvaporationFraction);
	)
	
	EQUATION(Model, WaterInputToBox,
		double fromPrecipitationOrSnow = RESULT(WaterInput);
		double fromBoxAbove = RESULT(RunoffFromBox, PREVIOUS_INDEX(SoilBoxes));
		if(CURRENT_INDEX(SoilBoxes) == 0) return fromPrecipitationOrSnow;
		return fromBoxAbove;
	)
	
	EQUATION(Model, RunoffFromBox,
		return RESULT(WaterInputToBox) * RESULT(RunoffFraction);
	)
	
	//TODO: overland flow if Soil moisture > field capacity.
	
	EQUATION(Model, SoilMoisture,
		return LAST_RESULT(SoilMoisture) - RESULT(Evaporation) + RESULT(WaterInputToBox) * (1.0 - RESULT(RunoffFraction));
	)
	
	EQUATION(Model, GroundwaterRechargeFromBox,
		double runoff = RESULT(RunoffFromBox);
		if(CURRENT_INDEX(SoilBoxes) == FINAL_INDEX(SoilBoxes))
		{
			return runoff;
		}
		return 0.0;
	)
	
	EQUATION(Model, PercentAdjustedGroundwaterRechargeFromUnit,
		return RESULT(GroundwaterRechargeFromUnit) * PARAMETER(Percent) / 100.0;
	)
}

#define SOIL_MOISTURE_MODEL_H
#endif