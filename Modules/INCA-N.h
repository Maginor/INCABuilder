
#if !defined INCAN_MODEL_H

static void
AddIncaNModel(inca_model *Model)
{
	//NOTE: Uses Persist, SoilTemperature, WaterTemperature
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	
	auto Dimensionless      = RegisterUnit(Model);
	auto JulianDay          = RegisterUnit(Model, "Julian day");
	auto MetresPerDay       = RegisterUnit(Model, "m/day");
	auto KgPerHectarePerDay = RegisterUnit(Model, "kg/Ha/day");
	auto KgPerHectarePerYear= RegisterUnit(Model, "kg/Ha/year");
	auto KgPerHectare       = RegisterUnit(Model, "kg/Ha");
	auto Metres             = RegisterUnit(Model, "m");
	auto MilliMetres        = RegisterUnit(Model, "mm");
	auto Hectares           = RegisterUnit(Model, "Ha");
	auto PerDay             = RegisterUnit(Model, "/day");
	auto MgPerL             = RegisterUnit(Model, "mg/l");
	auto M3PerKm2           = RegisterUnit(Model, "m3/km2");
	auto CumecsPerKm2       = RegisterUnit(Model, "m3/km2/s");
	auto Kg                 = RegisterUnit(Model, "kg");
	auto KgPerKm2PerDay     = RegisterUnit(Model, "kg/km2/day");
	auto KgPerDay           = RegisterUnit(Model, "kg/day");
	auto KgPerKm2           = RegisterUnit(Model, "kg/km2");
	auto KgPerM3            = RegisterUnit(Model, "kg/m3");
	auto DegreesCelsius     = RegisterUnit(Model, "°C");
	
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	
	auto DirectRunoffInitialNitrateConcentration      = RegisterParameterDouble(Model, Land, "Direct runoff initial nitrate concentration", MgPerL, 10.0, 0.0, 10.0);
	auto DirectRunoffInitialAmmoniumConcentration     = RegisterParameterDouble(Model, Land, "Direct runoff initial ammonium concentration", MgPerL, 2.0, 0.0, 2.0);
	auto SoilwaterInitialNitrateConcentration         = RegisterParameterDouble(Model, Land, "Soil water initial nitrate concentration", MgPerL, 10.0);
	auto SoilwaterInitialAmmoniumConcentration        = RegisterParameterDouble(Model, Land, "Soil water initial ammonium concentration", MgPerL, 10.0);
	
	auto GrowthCurveOffset              = RegisterParameterDouble(Model, Land, "Growth curve offset", Dimensionless, 20.0);
	auto GrowthCurveAmplitude           = RegisterParameterDouble(Model, Land, "Growth curve amplitude", Dimensionless, 20.0);
	auto PlantGrowthStartDay            = RegisterParameterUInt(Model, Land, "Plant growth start day", JulianDay, 20, 1, 365, "Day of year when plant growth begins");
	auto PlantGrowthPeriod              = RegisterParameterUInt(Model, Land, "Plant growth period", JulianDay, 20, 0, 365, "Length of plant growth period in days");
	auto NitratePlantUptakeRate         = RegisterParameterDouble(Model, Land, "Nitrate plant uptake rate", MetresPerDay, 20.0, 0.0, 162.0, "Rate at which NO3 is taken up by plants.");
	auto SoilwaterDenitrificationRate   = RegisterParameterDouble(Model, Land, "Soil water denitrification rate", MetresPerDay, 20.0);
	auto AmmoniumNitrificationRate      = RegisterParameterDouble(Model, Land, "Ammonium nitrification rate", MetresPerDay, 20.0);
	auto NitrogenFixationRate           = RegisterParameterDouble(Model, Land, "Nitrogen fixation rate", KgPerHectarePerDay, 20.0);
	auto MaximumNitrogenUptake          = RegisterParameterDouble(Model, Land, "Maximum nitrogen uptake", KgPerHectarePerYear, 20.0);
	auto FertilizerAdditionStartDay     = RegisterParameterUInt(Model, Land, "Fertilizer addition start day", JulianDay, 20, 1, 365, "Day of year when fertiliser application begins.");
	auto FertilizerAdditionPeriod       = RegisterParameterUInt(Model, Land, "Fertilizer addition period", JulianDay, 20, 0, 365, "Length of fertiliser addition period in days.");
	auto FertilizerNitrateAdditionRate  = RegisterParameterDouble(Model, Land, "Fertilizer nitrate addition rate", KgPerHectarePerDay, 20.0, 0.0, 100.0, "Amount of nitrate added as fertiliser on each day of fertiliser addition period.");
	auto FertilizerAmmoniumAdditionRate = RegisterParameterDouble(Model, Land, "Fertilizer ammonium addition rate", KgPerHectarePerDay, 20.0, 0.0, 100.0, "Amount of ammonium added as fertiliser on each day of fertiliser addition period.");
	auto AmmoniumPlantUptakeRate        = RegisterParameterDouble(Model, Land, "Ammonium plant uptake rate", MetresPerDay, 20.0, 0.0, 162.0, "Rate at which NH4 is taken up by plants.");
	auto AmmoniumImmobilisationRate     = RegisterParameterDouble(Model, Land, "Ammonium immobilisation rate", MetresPerDay, 20.0);
	auto AmmoniumMineralisationRate     = RegisterParameterDouble(Model, Land, "Ammonium mineralisation rate", KgPerHectarePerDay, 20.0);
	auto ZeroRateDepth                  = RegisterParameterDouble(Model, Land, "Zero rate depth", MilliMetres, 20.0);
	auto MaxRateDepth                   = RegisterParameterDouble(Model, Land, "Max rate depth", MilliMetres, 100.0);
	auto ResponseToA10DegreeChange      = RegisterParameterDouble(Model, Land, "Response to a 10° soil temperature change", Dimensionless, 2.0, 0.0, 10.0, "Response to a 10° soil temperature change for each process.");
	auto BaseTemperature                = RegisterParameterDouble(Model, Land, "Base temperature at which response is 1", DegreesCelsius, 20.0, 0.0, 50.0, "Base temperature for each process at which the response is 1.");
    

	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	
	auto GroundwaterInitialNitrateConcentration  = RegisterParameterDouble(Model, Reaches, "Groundwater initial nitrate concentration", MgPerL, 10.0);
	auto GroundwaterInitialAmmoniumConcentration = RegisterParameterDouble(Model, Reaches, "Groundwater initial ammonium concentration", MgPerL, 0.0);
	auto GroundwaterDenitrificationRate          = RegisterParameterDouble(Model, Reaches, "Groundwater denitrification rate", MetresPerDay, 20.0);
	auto NitrateDryDeposition                    = RegisterParameterDouble(Model, Reaches, "Nitrate dry deposition", KgPerHectarePerDay, 20.0);
	auto NitrateWetDeposition                    = RegisterParameterDouble(Model, Reaches, "Nitrate wet deposition", KgPerHectarePerDay, 20.0);
	auto AmmoniumDryDeposition                   = RegisterParameterDouble(Model, Reaches, "Ammonium dry deposition", KgPerHectarePerDay, 20.0);
	auto AmmoniumWetDeposition                   = RegisterParameterDouble(Model, Reaches, "Ammonium wet deposition", KgPerHectarePerDay, 20.0);
	auto ReachDenitrificationRate                = RegisterParameterDouble(Model, Reaches, "Reach denitrification rate", PerDay, 20.0);
	auto ReachNitrificationRate                  = RegisterParameterDouble(Model, Reaches, "Reach nitrification rate", PerDay, 20.0);
	auto ReachEffluentNitrateConcentration       = RegisterParameterDouble(Model, Reaches, "Reach effluent nitrate concentration", MgPerL, 0.0);
	auto ReachEffluentAmmoniumConcentration      = RegisterParameterDouble(Model, Reaches, "Reach effluent ammonium concentration", MgPerL, 0.0);
	
	auto InitialStreamNitrateConcentration = RegisterParameterDouble(Model, Reaches, "Initial stream nitrate concentration", MgPerL, 10.0);
	auto InitialStreamAmmoniumConcentration = RegisterParameterDouble(Model, Reaches, "Initial stream ammonium concentration", MgPerL, 10.0);
	
	
	auto IncaSolver = RegisterSolver(Model, "Inca solver", 0.1, IncaDascru);
	
	auto Soils = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	//NOTE: These are from PERSiST:
	auto WaterDepth3           = GetEquationHandle(Model, "Water depth 3"); //NOTE: This is right before percolation is subtracted.
	auto WaterDepth4           = GetEquationHandle(Model, "Water depth 4"); //NOTE: This is right before runoff is subtracted.
	auto WaterDepth            = GetEquationHandle(Model, "Water depth");   //NOTE: This is after everything is subtracted.
	auto RunoffToReach         = GetEquationHandle(Model, "Runoff to reach");
	auto SaturationExcessInput = GetEquationHandle(Model, "Saturation excess input");
	auto SoilTemperature       = GetEquationHandle(Model, "Soil temperature");
	auto PercolationInput      = GetEquationHandle(Model, "Percolation input");
	auto PercolationOut        = GetEquationHandle(Model, "Percolation out");
	
	
	auto SoilwaterVolume             = RegisterEquation(Model, "Soil water volume", M3PerKm2);
	auto GroundwaterVolume           = RegisterEquation(Model, "Groundwater volume", M3PerKm2);
	auto DirectRunoffToReachFraction = RegisterEquation(Model, "Direct runoff to reach fraction", PerDay);
	auto DirectRunoffToSoilFraction  = RegisterEquation(Model, "Direct runoff to soil fraction", PerDay);
	auto SoilToDirectRunoffFraction  = RegisterEquation(Model, "Soil to direct runoff fraction", PerDay);
	auto SoilToGroundwaterFraction   = RegisterEquation(Model, "Soil to groundwater fraction", PerDay);
	auto SoilToReachFraction         = RegisterEquation(Model, "Soil to reach fraction", PerDay);
	auto GroundwaterToReachFraction  = RegisterEquation(Model, "Groundwater to reach fraction", PerDay);
	
	
	auto DirectRunoffInitialNitrate = RegisterEquationInitialValue(Model, "Direct runoff initial nitrate", KgPerKm2);
	auto DirectRunoffNitrate        = RegisterEquationODE(Model, "Direct runoff nitrate", KgPerKm2);
	SetSolver(Model, DirectRunoffNitrate, IncaSolver);
	SetInitialValue(Model, DirectRunoffNitrate, DirectRunoffInitialNitrate);
	auto DirectRunoffInitialAmmonium = RegisterEquationInitialValue(Model, "Direct runoff initial ammonium", KgPerKm2);
	auto DirectRunoffAmmonium       = RegisterEquationODE(Model, "Direct runoff ammonium", KgPerKm2);
	SetSolver(Model, DirectRunoffAmmonium, IncaSolver);
	SetInitialValue(Model, DirectRunoffAmmonium, DirectRunoffInitialAmmonium);
	auto DrynessFactor = RegisterEquation(Model, "Dryness factor", Dimensionless);
	auto SeasonalGrowthFactor = RegisterEquation(Model, "Seasonal growth factor", Dimensionless);
	auto TemperatureFactor    = RegisterEquation(Model, "Temperature factor", Dimensionless);
	auto YearlyAccumulatedNitrogenUptake = RegisterEquation(Model, "Yearly accumulated nitrogen uptake", KgPerHectare);
	auto NitrateUptake = RegisterEquation(Model, "Nitrate uptake", KgPerKm2PerDay);
	SetSolver(Model, NitrateUptake, IncaSolver);
	auto Denitrification = RegisterEquation(Model, "Denitrification", KgPerKm2PerDay);
	SetSolver(Model, Denitrification, IncaSolver);
	auto Nitrification = RegisterEquation(Model, "Nitrification", KgPerKm2PerDay);
	SetSolver(Model, Nitrification, IncaSolver);
	auto Fixation = RegisterEquation(Model, "Fixation", KgPerKm2PerDay);
	SetSolver(Model, Fixation, IncaSolver);
	auto SoilwaterNitrateInput = RegisterEquation(Model, "Soil water nitrate input", KgPerKm2PerDay);
	auto SoilwaterInitialNitrate = RegisterEquationInitialValue(Model, "Soil water initial nitrate", KgPerKm2);
	auto SoilwaterNitrate = RegisterEquationODE(Model, "Soil water nitrate", KgPerKm2);
	SetSolver(Model, SoilwaterNitrate, IncaSolver);
	SetInitialValue(Model, SoilwaterNitrate, SoilwaterInitialNitrate);
	auto AmmoniumUptake = RegisterEquation(Model, "Ammonium uptake", KgPerKm2PerDay);
	SetSolver(Model, AmmoniumUptake, IncaSolver);
	auto Immobilisation = RegisterEquation(Model, "Immobilisation", KgPerKm2PerDay);
	SetSolver(Model, Immobilisation, IncaSolver);
	auto Mineralisation = RegisterEquation(Model, "Mineralisation", KgPerKm2PerDay);
	SetSolver(Model, Mineralisation, IncaSolver);
	auto SoilwaterAmmoniumInput  = RegisterEquation(Model, "Soil water ammonium input", KgPerKm2PerDay);
	auto SoilwaterInitialAmmonium = RegisterEquationInitialValue(Model, "Soil water initial ammmonium", KgPerKm2);
	auto SoilwaterAmmonium = RegisterEquationODE(Model, "Soil water ammonium", KgPerKm2);
	SetSolver(Model, SoilwaterAmmonium, IncaSolver);
	SetInitialValue(Model, SoilwaterAmmonium, SoilwaterInitialAmmonium);
	auto GroundwaterDenitrification = RegisterEquation(Model, "Groundwater denitrification", KgPerKm2PerDay);
	SetSolver(Model, GroundwaterDenitrification, IncaSolver);
	auto GroundwaterInitialNitrate = RegisterEquationInitialValue(Model, "GroundwaterInitialNitrate", KgPerKm2);
	auto GroundwaterNitrate = RegisterEquationODE(Model, "Groundwater nitrate", KgPerKm2);
	SetSolver(Model, GroundwaterNitrate, IncaSolver);
	SetInitialValue(Model, GroundwaterNitrate, GroundwaterInitialNitrate);
	auto GroundwaterInitialAmmonium = RegisterEquationInitialValue(Model, "Groundwater initial ammmonium", KgPerKm2);
	auto GroundwaterAmmonium = RegisterEquationODE(Model, "Groundwater ammonium", KgPerKm2);
	SetSolver(Model, GroundwaterAmmonium, IncaSolver);
	SetInitialValue(Model, GroundwaterAmmonium, GroundwaterInitialAmmonium);
	
	auto SoilwaterNitrateConcentration = RegisterEquation(Model, "Soil water nitrate concentration", MgPerL);
	auto GroundwaterNitrateConcentration = RegisterEquation(Model, "Groundwater nitrate concentration", MgPerL);
	
	
	EQUATION(Model, DirectRunoffInitialNitrate,
		return PARAMETER(DirectRunoffInitialNitrateConcentration) * RESULT(WaterDepth, DirectRunoff);
	)
	
	EQUATION(Model, DirectRunoffInitialAmmonium,
		return PARAMETER(DirectRunoffInitialAmmoniumConcentration)* RESULT(WaterDepth, DirectRunoff);
	)
	
	EQUATION(Model, SoilwaterInitialNitrate,
		return PARAMETER(SoilwaterInitialNitrateConcentration)* RESULT(WaterDepth, Soilwater);
	)
	
	EQUATION(Model, SoilwaterInitialAmmonium,
		return PARAMETER(SoilwaterInitialAmmoniumConcentration) * RESULT(WaterDepth, Soilwater);
	)
	
	EQUATION(Model, GroundwaterInitialNitrate,
		return PARAMETER(GroundwaterInitialNitrateConcentration) * RESULT(WaterDepth, Groundwater);
	)
	
	EQUATION(Model, GroundwaterInitialAmmonium,
		return PARAMETER(GroundwaterInitialAmmoniumConcentration) * RESULT(WaterDepth, Groundwater);
	)
	
	
	
	EQUATION(Model, SoilwaterVolume,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return RESULT(WaterDepth, Soilwater) * 1000.0;
	)
	
	EQUATION(Model, GroundwaterVolume,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return RESULT(WaterDepth, Groundwater) * 1000.0;
	)
	
	EQUATION(Model, SoilwaterNitrateConcentration,
		return SafeDivide(RESULT(SoilwaterNitrate), RESULT(SoilwaterVolume)) * 1000.0;
	)
	
	EQUATION(Model, GroundwaterNitrateConcentration,
		return SafeDivide(RESULT(GroundwaterNitrate), RESULT(GroundwaterVolume)) * 1000.0;
	)
	
	
	EQUATION(Model, DirectRunoffToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, DirectRunoff), RESULT(WaterDepth3, DirectRunoff));
	)
	
	EQUATION(Model, DirectRunoffToSoilFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(PercolationInput, Soilwater), RESULT(WaterDepth3, DirectRunoff));
	)
	
	EQUATION(Model, SoilToDirectRunoffFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(SaturationExcessInput, DirectRunoff), RESULT(WaterDepth3, Soilwater));
	)
	
	EQUATION(Model, SoilToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, Soilwater), RESULT(WaterDepth3, Soilwater));
	)
	
	EQUATION(Model, SoilToGroundwaterFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(PercolationInput, Groundwater), RESULT(WaterDepth3, Soilwater));
	)

	EQUATION(Model, GroundwaterToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, Groundwater), RESULT(WaterDepth3, Groundwater));
	)
	
	
	
	EQUATION(Model, DrynessFactor,
		CURRENT_INDEX(Reach); //NOTE: Has to be here until we make some more fixes to the dependency system
		double depth = RESULT(WaterDepth, Soilwater);
		double maxratedepth = PARAMETER(MaxRateDepth);
		double zeroratedepth = PARAMETER(ZeroRateDepth);
	
		if(depth > maxratedepth ) return 1.0;
		
		if(depth > zeroratedepth )
		{
			return (depth - zeroratedepth) / (maxratedepth - zeroratedepth);
		}
		
		return 0.0;
	)

	EQUATION(Model, SeasonalGrowthFactor,
		double currentday = (double)CURRENT_DAY_OF_YEAR();
		double startday = (double)PARAMETER(PlantGrowthStartDay);
		double daysthisyear = (double)DAYS_THIS_YEAR();
		return PARAMETER(GrowthCurveOffset) + PARAMETER(GrowthCurveAmplitude) * sin(2.0 * Pi * (currentday - startday) / daysthisyear );
	)
	
	EQUATION(Model, TemperatureFactor,
		return pow(PARAMETER(ResponseToA10DegreeChange), (RESULT(SoilTemperature) - PARAMETER(BaseTemperature)) * 0.1);
	)
	
	

	EQUATION(Model, DirectRunoffNitrate,
		return 
			  RESULT(SoilwaterNitrate) * RESULT(SoilToDirectRunoffFraction)
			- RESULT(DirectRunoffNitrate) * (RESULT(DirectRunoffToSoilFraction) + RESULT(DirectRunoffToReachFraction));
	)
	
	EQUATION(Model, DirectRunoffAmmonium,
		return
			  RESULT(SoilwaterAmmonium) * RESULT(SoilToDirectRunoffFraction)
			- RESULT(DirectRunoffAmmonium) * (RESULT(DirectRunoffToSoilFraction) + RESULT(DirectRunoffToReachFraction));
	)
	
	EQUATION(Model, YearlyAccumulatedNitrogenUptake,
		double accumulated = LAST_RESULT(YearlyAccumulatedNitrogenUptake) + (LAST_RESULT(NitrateUptake) + LAST_RESULT(AmmoniumUptake)) / 100.0; //NOTE convert 1/km2 to 1/Ha
		if(CURRENT_DAY_OF_YEAR() == 1) accumulated = 0.0;
		return accumulated;
	)
	
	EQUATION(Model, NitrateUptake,
		double nitrateuptake = 
			  PARAMETER(NitratePlantUptakeRate) 
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterNitrate)
			* RESULT(DrynessFactor)
			* RESULT(SeasonalGrowthFactor)
			/ RESULT(SoilwaterVolume)
			* 1000000.0;
		double plantGrowthStartDay = (double)PARAMETER(PlantGrowthStartDay);
		double plantGrowthEndDay   = plantGrowthStartDay + (double)PARAMETER(PlantGrowthPeriod);
		
		if(RESULT(YearlyAccumulatedNitrogenUptake) > PARAMETER(MaximumNitrogenUptake)) return 0.0;
		if((double)CURRENT_DAY_OF_YEAR() < plantGrowthStartDay || (double)CURRENT_DAY_OF_YEAR() > plantGrowthEndDay) return 0.0;
		
		return nitrateuptake;
	)

	EQUATION(Model, Denitrification,
		return
			  PARAMETER(SoilwaterDenitrificationRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterNitrate)
			* RESULT(DrynessFactor)
			/ RESULT(SoilwaterVolume)
			* 1000000.0;
	)
	
	EQUATION(Model, Nitrification,
		return
			  PARAMETER(AmmoniumNitrificationRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterAmmonium)
			* RESULT(DrynessFactor)
			/ RESULT(SoilwaterVolume)
			* 1000000.0;
	)
  
	EQUATION(Model, Fixation,
		return PARAMETER(NitrogenFixationRate) * RESULT(TemperatureFactor) * 100.0;
	)

	EQUATION(Model, SoilwaterNitrateInput,
		double nitrateInput = 0.0;
		double startday = (double)PARAMETER(FertilizerAdditionStartDay);
		double endday   = startday + (double)PARAMETER(FertilizerAdditionPeriod);
		double additionrate = PARAMETER(FertilizerNitrateAdditionRate);
		
		if((double)CURRENT_DAY_OF_YEAR() >= startday && (double)CURRENT_DAY_OF_YEAR() <= endday)
		{
			nitrateInput += additionrate;
		}
		
		nitrateInput += PARAMETER(NitrateDryDeposition);
		nitrateInput += PARAMETER(NitrateWetDeposition);
		
		return nitrateInput * 100.0;
	)
    
	EQUATION(Model, SoilwaterNitrate,
		return
			  RESULT(SoilwaterNitrateInput)
			- RESULT(NitrateUptake)
			- RESULT(Denitrification)
			+ RESULT(Nitrification)
			+ RESULT(Fixation)
			
			- RESULT(SoilwaterNitrate) * (RESULT(SoilToDirectRunoffFraction) + RESULT(SoilToGroundwaterFraction) + RESULT(SoilToReachFraction))
			+ RESULT(DirectRunoffNitrate) * RESULT(DirectRunoffToSoilFraction);
	)
	
	EQUATION(Model, AmmoniumUptake,
		double uptake =
			  PARAMETER(AmmoniumPlantUptakeRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterAmmonium)
			* RESULT(DrynessFactor)
			* RESULT(SeasonalGrowthFactor)
			/ RESULT(SoilwaterVolume)
			* 1000000.0;
		if(RESULT(YearlyAccumulatedNitrogenUptake) > PARAMETER(MaximumNitrogenUptake)) uptake = 0.0;
		return uptake;
	)
	
	EQUATION(Model, Immobilisation,
		return
			  PARAMETER(AmmoniumImmobilisationRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterAmmonium)
			* RESULT(DrynessFactor)
			/ RESULT(SoilwaterVolume)
			* 1000000.0;
	)

	EQUATION(Model, Mineralisation,
		return PARAMETER(AmmoniumMineralisationRate) * RESULT(TemperatureFactor) * RESULT(DrynessFactor) * 100.0;
	)

	EQUATION(Model, SoilwaterAmmoniumInput,
		double nitrateInput = 0.0;
		double startday = (double)PARAMETER(FertilizerAdditionStartDay);
		double endday   = startday + (double)PARAMETER(FertilizerAdditionPeriod);
		double additionrate = PARAMETER(FertilizerAmmoniumAdditionRate);
		if((double)CURRENT_DAY_OF_YEAR() >= startday && (double)CURRENT_DAY_OF_YEAR() <= endday)
		{
			nitrateInput += additionrate;
		}
		nitrateInput += PARAMETER(AmmoniumDryDeposition);
		nitrateInput += PARAMETER(AmmoniumWetDeposition);
		
		return nitrateInput * 100.0;
	)

	EQUATION(Model, SoilwaterAmmonium,
		return
			  RESULT(SoilwaterAmmoniumInput)
			- RESULT(AmmoniumUptake)
			- RESULT(Nitrification)
			- RESULT(Immobilisation)
			+ RESULT(Mineralisation)
			
			- RESULT(SoilwaterAmmonium) * (RESULT(SoilToDirectRunoffFraction) + RESULT(SoilToGroundwaterFraction) + RESULT(SoilToReachFraction))
			+ RESULT(DirectRunoffAmmonium) * RESULT(DirectRunoffToSoilFraction);
	)
	
	EQUATION(Model, GroundwaterDenitrification,
		return SafeDivide(RESULT(GroundwaterNitrate) * PARAMETER(GroundwaterDenitrificationRate) * RESULT(TemperatureFactor), RESULT(GroundwaterVolume)) * 1000000.0;
	)

	
	EQUATION(Model, GroundwaterNitrate,
		return
			  RESULT(SoilwaterNitrate) * RESULT(SoilToGroundwaterFraction)
			- RESULT(GroundwaterNitrate) * RESULT(GroundwaterToReachFraction)
			- RESULT(GroundwaterDenitrification);
	)
	
	
	EQUATION(Model, GroundwaterAmmonium,
		return
			  RESULT(SoilwaterAmmonium) * RESULT(SoilToGroundwaterFraction)
			- RESULT(GroundwaterAmmonium) * RESULT(GroundwaterToReachFraction);
	)
	
	
	auto TotalNitrateToStream = RegisterEquation(Model, "Total nitrate to stream", KgPerKm2PerDay);
	SetSolver(Model, TotalNitrateToStream, IncaSolver);
	auto DiffuseNitrate = RegisterEquation(Model, "Diffuse nitrate", KgPerDay);
	auto TotalDiffuseNitrateOutput = RegisterEquationCumulative(Model, "Total diffuse nitrate output", DiffuseNitrate, LandscapeUnits);
	auto TotalAmmoniumToStream = RegisterEquation(Model, "Total ammonium to stream", KgPerKm2PerDay);
	SetSolver(Model, TotalAmmoniumToStream, IncaSolver);
	auto DiffuseAmmonium = RegisterEquation(Model, "Diffuse ammonium", KgPerDay);
	auto TotalDiffuseAmmoniumOutput = RegisterEquationCumulative(Model, "Total diffuse ammonium output", DiffuseAmmonium, LandscapeUnits);
	
	auto TerrestrialCatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area"); //NOTE: From persist
	auto Percent                  = GetParameterDoubleHandle(Model, "%");                          //NOTE: From persist
	
	EQUATION(Model, TotalNitrateToStream,
		return
			RESULT(DirectRunoffNitrate) * RESULT(DirectRunoffToReachFraction)
		  + RESULT(SoilwaterNitrate) * RESULT(SoilToReachFraction)
		  + RESULT(GroundwaterNitrate) * RESULT(GroundwaterToReachFraction); 
	)
	
	EQUATION(Model, DiffuseNitrate,
		return RESULT(TotalNitrateToStream) * PARAMETER(TerrestrialCatchmentArea) * PARAMETER(Percent) / 100.0;
	)
	
	EQUATION(Model, TotalAmmoniumToStream,
		return 
		  RESULT(DirectRunoffAmmonium) * RESULT(DirectRunoffToReachFraction)
		+ RESULT(SoilwaterAmmonium) * RESULT(SoilToReachFraction)
		+ RESULT(GroundwaterAmmonium) * RESULT(GroundwaterToReachFraction);
	)
	
	EQUATION(Model, DiffuseAmmonium,
		return RESULT(TotalAmmoniumToStream) * PARAMETER(TerrestrialCatchmentArea) * PARAMETER(Percent) / 100.0;
	)
	
	
	
	auto ReachSolver = GetSolverHandle(Model, "Reach solver"); //NOTE: from persist
	auto ReachFlow   = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume = GetEquationHandle(Model, "Reach volume");
	
	auto ReachNitrateOutput = RegisterEquation(Model, "Reach nitrate output", KgPerDay);
	SetSolver(Model, ReachNitrateOutput, ReachSolver);
	auto WaterTemperatureFactor = RegisterEquation(Model, "Water temperature factor", Dimensionless);
	auto ReachDenitrification = RegisterEquation(Model, "Reach denitrification", KgPerDay);
	SetSolver(Model, ReachDenitrification, ReachSolver);
	auto ReachNitrification = RegisterEquation(Model, "Reach nitrification", KgPerDay);
	SetSolver(Model, ReachNitrification, ReachSolver);
	auto ReachUpstreamNitrate = RegisterEquation(Model, "Reach upstream nitrate", KgPerDay);
	auto ReachEffluentNitrate = RegisterEquation(Model, "Reach effluent nitrate", KgPerDay);
	auto ReachTotalNitrateInput = RegisterEquation(Model, "Reach total nitrate input", KgPerDay);
	auto ReachNitrateInitialValue = RegisterEquationInitialValue(Model, "Reach nitrate initial value", Kg);
	auto ReachNitrate = RegisterEquationODE(Model, "Reach nitrate", Kg);
	SetSolver(Model, ReachNitrate, ReachSolver);
	SetInitialValue(Model, ReachNitrate, ReachNitrateInitialValue);
	auto ReachUpstreamAmmonium = RegisterEquation(Model, "Reach upstream ammonium", KgPerDay);
	auto ReachEffluentAmmonium = RegisterEquation(Model, "Reach effluent ammonium", KgPerDay);
	auto ReachTotalAmmoniumInput = RegisterEquation(Model, "Reach total ammonium input", KgPerDay);
	auto ReachAmmoniumOutput = RegisterEquation(Model, "Reach ammonium output", KgPerDay);
	SetSolver(Model, ReachAmmoniumOutput, ReachSolver);
	auto ReachAmmoniumInitialValue = RegisterEquationInitialValue(Model, "Reach ammmonium initial value", Kg);
	auto ReachAmmonium = RegisterEquationODE(Model, "Reach ammonium", Kg);
	SetSolver(Model, ReachAmmonium, ReachSolver);
	SetInitialValue(Model, ReachAmmonium, ReachAmmoniumInitialValue);
	
	EQUATION(Model, ReachNitrateOutput,
		return RESULT(ReachNitrate) * SafeDivide(RESULT(ReachFlow) * 86400.0, RESULT(ReachVolume));
	)
	
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature");
	
	EQUATION(Model, WaterTemperatureFactor,
		return pow(1.047, RESULT(WaterTemperature) - 20.0);
	)
	
	EQUATION(Model, ReachDenitrification,
		return
			  PARAMETER(ReachDenitrificationRate)
			* RESULT(ReachNitrate)
			* RESULT(WaterTemperatureFactor);
	)
	
	EQUATION(Model, ReachNitrification,
		return
			  PARAMETER(ReachNitrificationRate)
			* RESULT(ReachAmmonium)
			* RESULT(WaterTemperatureFactor);
	)
	
	EQUATION(Model, ReachUpstreamNitrate,
		double reachInput = 0.0;
		FOREACH_INPUT(Reach,
			reachInput += RESULT(ReachNitrateOutput, *Input);
		)
		return reachInput;
	)
	
	auto EffluentFlow = GetParameterDoubleHandle(Model, "Effluent flow");
	auto ReachHasEffluentInput = GetParameterBoolHandle(Model, "Reach has effluent input");
	
	EQUATION(Model, ReachEffluentNitrate,
		double effluentnitrate = PARAMETER(EffluentFlow) * PARAMETER(ReachEffluentNitrateConcentration) * 86.4;
		if(!PARAMETER(ReachHasEffluentInput)) effluentnitrate = 0.0;
		return effluentnitrate;
	)
	
	EQUATION(Model, ReachTotalNitrateInput,
		return RESULT(ReachUpstreamNitrate) + RESULT(TotalDiffuseNitrateOutput) + RESULT(ReachEffluentNitrate);
	)
	
	EQUATION(Model, ReachNitrateInitialValue,
		return PARAMETER(InitialStreamNitrateConcentration) * RESULT(ReachVolume) / 1000.0;
	)
	
	EQUATION(Model, ReachNitrate,
		return
			  RESULT(ReachTotalNitrateInput)
			- RESULT(ReachNitrateOutput)
			- RESULT(ReachDenitrification)
			+ RESULT(ReachNitrification);
			//-RESULT(ReachNitrateAbstraction);
	)
	
	EQUATION(Model, ReachUpstreamAmmonium,
		double reachInput = 0.0;
		FOREACH_INPUT(Reach,
			reachInput += RESULT(ReachAmmoniumOutput, *Input);
		)
		return reachInput;
	)
	
	EQUATION(Model, ReachEffluentAmmonium,
		double effluentammonium = PARAMETER(EffluentFlow) * PARAMETER(ReachEffluentAmmoniumConcentration) * 86.4;
		if(!PARAMETER(ReachHasEffluentInput)) effluentammonium = 0.0;
		return effluentammonium;
	)
	
	EQUATION(Model, ReachTotalAmmoniumInput,
		return RESULT(ReachUpstreamAmmonium) + RESULT(TotalDiffuseAmmoniumOutput) + RESULT(ReachEffluentAmmonium);
	)
	
	EQUATION(Model, ReachAmmoniumOutput,
		return RESULT(ReachAmmonium) * SafeDivide(RESULT(ReachFlow) * 86400.0, RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachAmmoniumInitialValue,
		return PARAMETER(InitialStreamAmmoniumConcentration) * RESULT(ReachVolume) / 1000.0;
	)
	
	EQUATION(Model, ReachAmmonium,
		return RESULT(ReachTotalAmmoniumInput) - RESULT(ReachAmmoniumOutput) - RESULT(ReachNitrification); //-RESULT(ReachAmmoniumAbstraction);
	)
	
	//NOTE: Added this for easier calibration - MDN
	auto ReachNitrateConcentration = RegisterEquation(Model, "Reach nitrate concentration", MgPerL);
	auto ReachAmmoniumConcentration = RegisterEquation(Model, "Reach ammonium concentration", MgPerL);
	
	EQUATION(Model, ReachNitrateConcentration,
		return SafeDivide(RESULT(ReachNitrate), RESULT(ReachVolume)) * 1000.0;
	)
	
	EQUATION(Model, ReachAmmoniumConcentration,
		return SafeDivide(RESULT(ReachAmmonium), RESULT(ReachVolume)) * 1000.0;
	)
}

#define INCAN_MODEL_H
#endif
