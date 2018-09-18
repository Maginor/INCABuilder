
#if !defined(SNOW_ROUTINE_H)

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
	auto WaterInput  = RegisterEquation(Model, "Water input to soil", Mm);
	
	EQUATION(Model, Diff,
		return INPUT(AirTemperature) - PARAMETER(SnowThresholdTemperature);
	)
	
	//TODO: once we have index set dependencies for inputs, these may have to be rewritten:
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
	
	EQUATION(Model, WaterInput,
		double rainfall = RESULT(Rainfall);
		double excess   = RESULT(ExcessMelt);
		if(RESULT(Snowpack) == 0.0) return rainfall;
		return excess;
	)
}

#define SNOW_MODEL_H
#endif