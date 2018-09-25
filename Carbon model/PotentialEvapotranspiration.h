
#if !defined(POTENTIAL_EVAPOTRANSPIRATION_H)

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
	//NOTE: This is similar to what is used in PERSIST.
	
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




#define POTENTIAL_EVAPOTRANSPIRATION_H
#endif