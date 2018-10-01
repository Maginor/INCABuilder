
#if !defined(HBV_H)

static void AddSnowRoutine(inca_model *Model)
{
	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto Dimensionless     = RegisterUnit(Model);
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	auto Land = RegisterParameterGroup(Model, "Landscape units", LandscapeUnits);
	
	auto DegreeDayFactor          = RegisterParameterDouble(Model, Land, "Degree day factor", MmPerDegreePerDay, 2.5, 1.5, 4.0, "Degree day factor, rate at which the snow will melt water in snow will freeze");
	auto RefreezingCoefficient    = RegisterParameterDouble(Model, Land, "Refreezing coefficient", Dimensionless, 0.05, 0.01, 0.1, "Proportion of meltwater that can refreeze");
	auto StorageFraction          = RegisterParameterDouble(Model, Land, "Storage fraction", Dimensionless, 0.1, 0.01, 0.2, "Proportion of meltwater that can be stored in the snowpack");
	auto SnowThresholdTemperature = RegisterParameterDouble(Model, Land, "Snow threshold temperature", DegreesCelsius, 0.0, -3.0, 5.0, "Threshold temperature above which precipitation falls as rain");
	auto InitialSnowDepth         = RegisterParameterDouble(Model, Land, "Initial snow depth", Mm, 0.0, 0.0, 9999.0, "The depth of snow, expressed as water equivalents, at the start of the simulation");
	
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	auto Precipitation  = RegisterInput(Model, "Precipitation");
	
	
	auto Diff        = RegisterEquation(Model, "Diff", DegreesCelsius);
	auto Snowfall    = RegisterEquation(Model, "Snowfall", Mm);
	auto Rainfall    = RegisterEquation(Model, "Rainfall", Mm);
	auto Snowpack    = RegisterEquation(Model, "Snow pack", Mm);
	SetInitialValue(Model, Snowpack, InitialSnowDepth);
	auto MaxStorage  = RegisterEquation(Model, "Max storage", Mm);
	auto MeltWater   = RegisterEquation(Model, "Meltwater", Mm);
	auto WaterInSnow = RegisterEquation(Model, "Water in snow", Mm);
	auto ExcessMelt  = RegisterEquation(Model, "Excess melt", Mm);
	auto Refreeze    = RegisterEquation(Model, "Refreeze", Mm);
	auto WaterToSoil  = RegisterEquation(Model, "Water to soil", Mm);
	
	EQUATION(Model, Diff,
		return INPUT(AirTemperature) - PARAMETER(SnowThresholdTemperature);
	)
	
	EQUATION(Model, Snowfall,
		double precip = INPUT(Precipitation);
		return RESULT(Diff) <= 0.0 ? precip : 0.0;
	)
	
	EQUATION(Model, Rainfall,
		double precip = INPUT(Precipitation);
		return RESULT(Diff) > 0.0 ? precip : 0.0;
	)
	
	EQUATION(Model, Snowpack,
		double snowpack1 = RESULT(Snowfall) + RESULT(Refreeze);
		double snowpack2 = LAST_RESULT(Snowpack) - RESULT(MeltWater);
		return RESULT(Diff) <= 0.0 ? snowpack1 : snowpack2;
	)
	
	EQUATION(Model, MaxStorage,
		return PARAMETER(StorageFraction) * LAST_RESULT(Snowpack);
	)
	
	EQUATION(Model, MeltWater,
		double potentialmelt = PARAMETER(DegreeDayFactor) * RESULT(Diff);
		double actualmelt = Min(potentialmelt, LAST_RESULT(Snowpack));
		if(LAST_RESULT(Snowpack) <= 0.0 || RESULT(Diff) <= 0.0) actualmelt = 0.0;
		return actualmelt;
	)
	
	EQUATION(Model, WaterInSnow,
		double maxStorage = RESULT(MaxStorage);
		double meltWater  = RESULT(MeltWater);
		double refreeze   = RESULT(Refreeze);
		if(LAST_RESULT(Snowpack) == 0.0) return 0.0;
		if(RESULT(Diff) > 0.0)
		{
			return Min(maxStorage, LAST_RESULT(WaterInSnow) + meltWater);
		}
		else
		{
			return Max(0.0, LAST_RESULT(WaterInSnow) - refreeze);
		}
	)
	
	EQUATION(Model, ExcessMelt,
		double availableWater = RESULT(MeltWater) + RESULT(WaterInSnow);
		double excess = 0.0;
		if(RESULT(Diff) > 0.0 && availableWater > RESULT(MaxStorage))
		{
			excess = availableWater - RESULT(MaxStorage);
		}
		return excess;
	)
	
	EQUATION(Model, Refreeze,
		double refreeze = PARAMETER(RefreezingCoefficient) * PARAMETER(DegreeDayFactor) * -RESULT(Diff);
		refreeze = Min(refreeze, LAST_RESULT(WaterInSnow));
		if(LAST_RESULT(Snowpack) == 0.0 || RESULT(Diff) > 0.0 || LAST_RESULT(WaterInSnow) == 0.0) refreeze = 0.0;
		return refreeze;
	)
	
	EQUATION(Model, WaterToSoil,
		double rainfall = RESULT(Rainfall);
		double excess   = RESULT(ExcessMelt);
		if(RESULT(Snowpack) == 0.0) return rainfall;
		return excess;
	)
}



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


void
AddGroundwaterResponseRoutine(inca_model *Model)
{
	auto Mm = RegisterUnit(Model, "mm");
	auto Days = RegisterUnit(Model, "day");
	auto PerDay = RegisterUnit(Model, "1/day");
	auto MmPerDay = RegisterUnit(Model, "mm/day");
	
	auto Reaches = RegisterIndexSetBranched(Model, "Reaches");
	auto Groundwater = RegisterParameterGroup(Model, "Groundwater", Reaches);
	
	//TODO: Find good values for parameters.
	auto FirstUpperRecessionCoefficient  = RegisterParameterDouble(Model, Groundwater, "First recession coefficent for upper groundwater storage", PerDay, 0.1);
	auto SecondUpperRecessionCoefficient = RegisterParameterDouble(Model, Groundwater, "First recession coefficent for upper groundwater storage", PerDay, 0.1);
	auto LowerRecessionCoefficient       = RegisterParameterDouble(Model, Groundwater, "Recession coefficient for lower groundwater storage", PerDay, 0.1);
	auto UpperSecondRunoffThreshold   = RegisterParameterDouble(Model, Groundwater, "Threshold for second runoff in upper storage", Mm, 10.0);
	auto PercolationRate              = RegisterParameterDouble(Model, Groundwater, "Percolation rate from upper to lower groundwater storage", MmPerDay, 0.1);
	
	auto MaxBase = RegisterParameterUInt(Model, Groundwater, "Flow routing max base", Days, 5);
	
	auto TotalGroundwaterRecharge = GetEquationHandle(Model, "Groundwater recharge"); //NOTE: From the soil moisture routine.
	
	auto GroundwaterSolver = RegisterSolver(Model, "Groundwater solver", 0.1, IncaDascru);
	
	auto UpperRunoff = RegisterEquation(Model, "Runoff from upper groundwater storage", MmPerDay);
	SetSolver(Model, UpperRunoff, GroundwaterSolver);
	auto LowerRunoff = RegisterEquation(Model, "Runoff from lower groundwater storage", MmPerDay);
	SetSolver(Model, LowerRunoff, GroundwaterSolver);
	auto Percolation = RegisterEquation(Model, "Percolation from upper to lower groundwater storage", MmPerDay);
	SetSolver(Model, Percolation, GroundwaterSolver);
	auto UpperStorage = RegisterEquationODE(Model, "Upper groundwater storage", Mm);
	SetSolver(Model, UpperStorage, GroundwaterSolver);
	//TODO: initial value
	auto LowerStorage = RegisterEquationODE(Model, "Lower groundwater storage", Mm);
	SetSolver(Model, LowerStorage, GroundwaterSolver);
	//TODO: initial value
	auto GroundwaterDischargeBeforeRouting = RegisterEquation(Model, "Groundwater discharge to reach before routing", MmPerDay);
	auto GroundwaterDischarge = RegisterEquation(Model, "Groundwater discharge to reach", MmPerDay);
	
	EQUATION(Model, UpperRunoff,
		double K1 = PARAMETER(FirstUpperRecessionCoefficient);
		double K0 = PARAMETER(SecondUpperRecessionCoefficient);
		double UZL = PARAMETER(UpperSecondRunoffThreshold);
		double runoff = RESULT(UpperStorage) * K1;
		if(RESULT(UpperStorage) > UZL) runoff += (RESULT(UpperStorage) - UZL)*K0;
		return runoff;
	)
	
	EQUATION(Model, LowerRunoff,
		return RESULT(LowerStorage) * PARAMETER(LowerRecessionCoefficient);
	)
	
	EQUATION(Model, Percolation,
		return RESULT(UpperStorage) * PARAMETER(PercolationRate);
	)
	
	EQUATION(Model, UpperStorage,
		return RESULT(TotalGroundwaterRecharge) - RESULT(Percolation) - RESULT(UpperRunoff);
	)
	
	EQUATION(Model, LowerStorage,
		return RESULT(Percolation) - RESULT(LowerRunoff);
	)
	
	EQUATION(Model, GroundwaterDischargeBeforeRouting,
		return RESULT(UpperRunoff) + RESULT(LowerRunoff);
	)
	
	//TODO: We have to test that these coefficients are correct:
	EQUATION(Model, GroundwaterDischarge,
		RESULT(GroundwaterDischargeBeforeRouting); //NOTE: To force a dependency since this is not automatic when we use EARLIER_RESULT;
	
		u64 M = PARAMETER(MaxBase);		
		double sum = 0.0;

		for(u64 I = 1; I <= M; ++I)
		{
			double a;
			u64 M2;
			if((M % 2) == 0)
			{
				double Md = (double)M * 0.5;
				a = 1.0 / (Md*(Md + 1.0));
				M2 = M / 2;
			}
			else
			{
				double Md = floor((double)M * 0.5);
				a = 2.0 / ((2.0 + 2.0*Md) * (Md + 1.0) + 1.0);
				M2 = M / 2 + 1;
			}
			double coeff;
			if(I <= M2)
			{
				coeff = a * (double)I;
			}
			else
			{
				coeff = a * (double)(2*M2 - I);
			}
			sum += coeff * EARLIER_RESULT(GroundwaterDischargeBeforeRouting, I-1);
		}
			
		return sum;
	)
	
	//TODO: Convert millimeter/day to flow for the reach routine.
}


static void
AddPotentialEvapotranspirationModuleV1(inca_model *Model)
{
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	
	auto PotentialEvapotranspirationIn = RegisterInput(Model, "Potential evapotranspiration");
	auto PotentialEvapotranspiration = RegisterEquation(Model, "Potential evapotranspiration", MmPerDay);
	
	EQUATION(Model, PotentialEvapotranspiration,
		return INPUT(PotentialEvapotranspirationIn);
	)
}

static void
AddPotentialEvapotranspirationModuleV2(inca_model *Model)
{
	//NOTE: This is similar to what is used in PERSiST.
	
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	
	auto GrowingDegreeThreshold = RegisterParameterDouble(Model, Land, "Growing degree threshold",    DegreesCelsius,    0.0, -4.0,    4.0, "The temperature at or above which plant growth and hence evapotranspiration are assumed to occur");
	auto DegreeDayEvapotranspiration = RegisterParameterDouble(Model, Land, "Degree day evapotranspiration", MmPerDegreePerDay, 0.12, 0.05,   0.2, "Describes the baseline dependency of evapotranspiration on air temperature. The parameter represents the number of millimetres water evapotranspired per degree celcius above the growing degree threshold when evapotranspiration rates are not soil moisture limited");
	
	auto PotentialEvapotranspiration = RegisterEquation(Model, "Potential evapotranspiration", MmPerDay);
	
	EQUATION(Model, PotentialEvapotranspiration,
		return Max(0.0, INPUT(AirTemperature) - PARAMETER(GrowingDegreeThreshold)) * PARAMETER(DegreeDayEvapotranspiration);
	)
}

static void
AddPotentialEvapotranspirationModuleV3(inca_model *Model)
{
	//NOTE: I don't know if this one makes much sense. Somebody with the right expertise should probably take a look at it. -MDN.
	
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto Dimensionless     = RegisterUnit(Model);
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	
	auto EvapotranspirationConstant    = RegisterParameterDouble(Model, Land, "Evapotranspiration constant", Dimensionless, 0.1, 0.0, 0.3, "Linear parameter for the calculation of potential evapotranspiration");
	auto EvapotranspirationExponent    = RegisterParameterDouble(Model, Land, "Evapotranspiration exponent", Dimensionless, 1.0, 1.0, 3.0, "Power parameter for the calculation of potential evapotranspiration");
	
	auto PotentialEvapotranspiration = RegisterEquation(Model, "Potential evapotraspiration", MmPerDay);
	
	EQUATION(Model, PotentialEvapotranspiration,
		double etp = PARAMETER(EvapotranspirationConstant) * pow(INPUT(AirTemperature), PARAMETER(EvapotranspirationExponent));
		etp = etp < 0.0 ? 0.0 : etp;
		return etp;
	)
}

#define HBV_H
#endif