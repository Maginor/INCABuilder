
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
	
	//auto LandscapeUnits = GetIndexerHandle(Model, "Landscape units"); //NOTE: This is declared in the snow melt model
	auto Land = GetParameterGroupHandle(Model, "Landscape units");    //NOTE: This is declared in the snow melt model
	
	auto ThermalConductivitySoil 			= RegisterParameterDouble(Model, Land, "Thermal conductivity of soil", 				WattsPerMetrePerDegreeCelsius,			 0.7);
	auto SpecificHeatCapacityFreezeThaw	    = RegisterParameterDouble(Model, Land, "Specific heat capacity due to freeze/thaw", MegaJoulesPerCubicMetrePerDegreeCelsius, 6.6);
	
	auto SnowDepthSoilTemperatureFactor	    = RegisterParameterDouble(Model, Land, "Snow depth / soil temperature factor", 		PerCm, 								    -0.02);
	auto SpecificHeatCapacitySoil			= RegisterParameterDouble(Model, Land, "Specific heat capacity of soil",			MegaJoulesPerCubicMetrePerDegreeCelsius, 1.1);
	auto SoilDepth							= RegisterParameterDouble(Model, Land, "Soil depth",								Metres,									 0.2);
	auto InitialSoilTemperature		    	= RegisterParameterDouble(Model, Land, "Initial soil temperature",					DegreesCelsius,							20.0);

	auto AirTemperature = GetInputHandle(Model, "Air temperature");  //NOTE: This should be declared by the main model
	auto SnowDepth = GetEquationHandle(Model, "Snow depth");
	
	auto Da                  = RegisterEquation(Model, "Da",                    Dimensionless);
	auto COUPSoilTemperature = RegisterEquation(Model, "COUP soil temperature", DegreesCelsius);
	SetInitialValue(Model, COUPSoilTemperature, InitialSoilTemperature);
	auto SoilTemperature     = RegisterEquation(Model, "Soil temperature",      DegreesCelsius);
	
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
			* std::exp(PARAMETER(SnowDepthSoilTemperatureFactor) * RESULT(SnowDepth));
	)
}

#define SOIL_TEMPERATURE_MODEL_H
#endif