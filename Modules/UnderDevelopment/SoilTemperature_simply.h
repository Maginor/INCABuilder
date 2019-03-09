
#if !defined(SOIL_TEMPERATURE_MODEL_H)

static void
AddSoilTemperatureModel(inca_model *Model)
{
	auto WattsPerMetrePerDegreeCelsius				= RegisterUnit(Model, "W/m/°C");
	auto MegaJoulesPerCubicMetrePerDegreeCelsius	= RegisterUnit(Model, "MJ/m3/°C");
	auto Dimensionless 								= RegisterUnit(Model);
	auto PerCm										= RegisterUnit(Model, "/cm");
	auto Metres										= RegisterUnit(Model, "m");
	auto Cm											= RegisterUnit(Model, "cm");
	auto Seconds									= RegisterUnit(Model, "s");
	auto DegreesCelsius								= RegisterUnit(Model, "°C");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units"); //NOTE: This must then be declared elsewhere
	
	// Soil temp params which are constant over land use or soil type
	auto SoilTempParamsGlobal = RegisterParameterGroup(Model, "Global soil temperature parameters");
	
	auto SnowDepthSoilTemperatureFactor	    = RegisterParameterDouble(Model, SoilTempParamsGlobal, "Snow depth / soil temperature factor", PerCm, -0.2, -3.0, -0.001, "Defines empirical relationship between snow depth and its insulating effect on soils, incorporating the conversion from snow depth in water equivalent to snow depth");
	// Note: Compared to orginal Rankinen et al. 2004 model, have grouped together the SnowDepthSoilTemperatureFactor together with
	// the snow water equivalent factor (as these two are just multiplied anyway, and one is a tuning param, so may as well incorporate the other)
	// Have adjusted the default, min and max param values accordingly.
	
	auto SoilDepth							= RegisterParameterDouble(Model, SoilTempParamsGlobal, "Soil depth", Metres, 0.2);
	auto InitialSoilTemperature		    	= RegisterParameterDouble(Model, SoilTempParamsGlobal, "Initial soil temperature", DegreesCelsius, 10.0);
	
	// Soil temp params which may vary with land use or soil type
	
	// To do: replace ThermalConductivity and SpecificHeatCapacity with a single param, one divided by other, to reduce non-identifiability
	// Also - explore whether extra FreezeThaw parameter adds any explanatory power. Remove?
	auto SoilTempParamsLand = RegisterParameterGroup(Model, "Soil temperature parameters (varying by soil or land class)", LandscapeUnits);
	
	auto ThermalConductivitySoil 			= RegisterParameterDouble(Model, SoilTempParamsLand, "Thermal conductivity of soil", WattsPerMetrePerDegreeCelsius,	0.7);
	auto SpecificHeatCapacityFreezeThaw	    = RegisterParameterDouble(Model, SoilTempParamsLand, "Specific heat capacity due to freeze/thaw", MegaJoulesPerCubicMetrePerDegreeCelsius, 6.6, 1.0, 200.0, "Controls the energy released when water is frozen and energy consumed under melting");
	auto SpecificHeatCapacitySoil			= RegisterParameterDouble(Model, SoilTempParamsLand, "Specific heat capacity of soil", MegaJoulesPerCubicMetrePerDegreeCelsius, 1.1);
	
	// Inputs
	auto AirTemperature = GetInputHandle(Model, "Air temperature");  //NOTE: This should be declared by the main model
	
	// Equations
	auto Da                  = RegisterEquation(Model, "Da",                    Dimensionless);
	auto COUPSoilTemperature = RegisterEquation(Model, "COUP soil temperature", DegreesCelsius);
	SetInitialValue(Model, COUPSoilTemperature, InitialSoilTemperature);
	auto SoilTemperature     = RegisterEquation(Model, "Soil temperature",      DegreesCelsius);
	auto SnowAsWaterEquivalent = GetEquationHandle(Model, "Snow depth as water equivalent");
	
	EQUATION(Model, Da,
		if ( LAST_RESULT(COUPSoilTemperature) > 0.0)
		{
			return PARAMETER(ThermalConductivitySoil)
					/ ( 1000000.0 * PARAMETER(SpecificHeatCapacitySoil) );
		}

		return PARAMETER(ThermalConductivitySoil)
				/ ( 1000000.0 * ( PARAMETER(SpecificHeatCapacitySoil) + PARAMETER(SpecificHeatCapacityFreezeThaw) ) );
	)
	
	EQUATION(Model, COUPSoilTemperature,
		return LAST_RESULT(COUPSoilTemperature)
			+ 86400.0
				* RESULT(Da) / Square( 2.0 * PARAMETER(SoilDepth))
				* ( INPUT(AirTemperature) - LAST_RESULT(COUPSoilTemperature) );
	)
	
	EQUATION(Model, SoilTemperature,
		return RESULT(COUPSoilTemperature)
			* std::exp(PARAMETER(SnowDepthSoilTemperatureFactor) * (RESULT(SnowAsWaterEquivalent)/10.0));
	)

}

#define SOIL_TEMPERATURE_MODEL_H
#endif