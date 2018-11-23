

static void
AddWaterTemperatureModel(inca_model *Model)
{
	auto Dimensionless 				= RegisterUnit(Model);
	auto DegreesCelsius 			= RegisterUnit(Model, "°C");
	
	auto Reach = RegisterIndexSetBranched(Model, "Reaches");
	
	auto Streams = RegisterParameterGroup(Model, "Streams", Reach);
	
	
	auto InitialWaterTemperature     = RegisterParameterDouble(Model, Streams, "Initial water temperature",    DegreesCelsius, 20.0);
	auto MinimumWaterTemperature     = RegisterParameterDouble(Model, Streams, "Minimum water temperature",    DegreesCelsius,  0.0);
	auto WaterTemperatureLagFactor   = RegisterParameterDouble(Model, Streams, "Water temperature lag factor", Dimensionless,   3.0);
	
	auto AirTemperature = GetInputHandle(Model, "Air temperature"); //NOTE: Is registered in the snow melt model
	
	auto WaterTemperature = RegisterEquation(Model, "Water temperature", DegreesCelsius);
	SetInitialValue(Model, WaterTemperature, InitialWaterTemperature);
	
	EQUATION(Model, WaterTemperature,
	
		return ( (PARAMETER(WaterTemperatureLagFactor) - 1.0) / PARAMETER(WaterTemperatureLagFactor) ) 
			* LAST_RESULT(WaterTemperature) + (1.0 / PARAMETER(WaterTemperatureLagFactor) )
			* Max( PARAMETER(MinimumWaterTemperature), INPUT(AirTemperature) );
	)
}