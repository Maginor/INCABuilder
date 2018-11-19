
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
	
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	
	//TODO: In case one wants these to be initial concentrations instead one has to make an initial value equation that convert them..
	auto DirectRunoffInitialNitrateConcentration      = RegisterParameterDouble(Model, Land, "Direct runoff initial nitrate concentration", MgPerL, 10.0);
	auto DirectRunoffInitialAmmoniumConcentration     = RegisterParameterDouble(Model, Land, "Direct runoff initial ammonium concentration", MgPerL, 10.0);
	auto SoilwaterInitialNitrateConcentration         = RegisterParameterDouble(Model, Land, "Soil water initial nitrate concentration", MgPerL, 10.0);
	auto SoilwaterInitialAmmoniumConcentration        = RegisterParameterDouble(Model, Land, "Soil water initial ammonium concentration", MgPerL, 10.0);
	
	auto GrowthCurveOffset              = RegisterParameterDouble(Model, Land, "Growth curve offset", Dimensionless, 20.0);
	auto GrowthCurveAmplitude           = RegisterParameterDouble(Model, Land, "Growth curve amplitude", Dimensionless, 20.0);
	auto PlantGrowthStartDay            = RegisterParameterUInt(Model, Land, "Plant growth start day", JulianDay, 20);
	auto PlantGrowthPeriod              = RegisterParameterUInt(Model, Land, "Plant growth period", JulianDay, 20);
	auto NitratePlantUptakeRate         = RegisterParameterDouble(Model, Land, "Nitrate plant uptake rate", MetresPerDay, 20.0);
	auto SoilwaterDenitrificationRate   = RegisterParameterDouble(Model, Land, "Soil water denitrification rate", MetresPerDay, 20.0);
	auto AmmoniumNitrificationRate      = RegisterParameterDouble(Model, Land, "Ammonium nitrification rate", MetresPerDay, 20.0);
	auto NitrogenFixationRate           = RegisterParameterDouble(Model, Land, "Nitrogen fixation rate", KgPerHectarePerDay, 20.0);
	auto MaximumNitrogenUptakeRate      = RegisterParameterDouble(Model, Land, "Maximum nitrogen uptake rate", KgPerHectarePerDay, 20.0);
	auto FertilizerAdditionStartDay     = RegisterParameterUInt(Model, Land, "Fertilizer addition start day", JulianDay, 20);
	auto FertilizerAdditionPeriod       = RegisterParameterUInt(Model, Land, "Fertilizer addition period", JulianDay, 20);
	auto FertilizerNitrateAdditionRate  = RegisterParameterDouble(Model, Land, "Fertilizer nitrate addition rate", KgPerHectarePerDay, 20.0);
	auto FertilizerAmmoniumAdditionRate = RegisterParameterDouble(Model, Land, "Fertilizer ammonium addition rate", KgPerHectarePerDay, 20.0);
	auto AmmoniumPlantUptakeRate        = RegisterParameterDouble(Model, Land, "Ammonium plant uptake rate", MetresPerDay, 20.0);
	auto AmmoniumImmobilisationRate     = RegisterParameterDouble(Model, Land, "Ammonium immobilisation rate", MetresPerDay, 20.0);
	auto AmmoniumMineralisationRate     = RegisterParameterDouble(Model, Land, "Ammonium mineralisation rate", KgPerHectarePerDay, 20.0);
	auto ZeroRateDepth                  = RegisterParameterDouble(Model, Land, "Zero rate depth", MilliMetres, 20.0);
	auto MaxRateDepth                   = RegisterParameterDouble(Model, Land, "Max rate depth", MilliMetres, 100.0);
    

	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	
	auto GroundwaterInitialNitrateConcentration  = RegisterParameterDouble(Model, Reaches, "Groundwater initial nitrate concentration", MgPerL, 10.0);
	auto GroundwaterInitialAmmoniumConcentration = RegisterParameterDouble(Model, Reaches, "Groundwater initial ammonium concentration", MgPerL, 0.0);
	auto GroundwaterDenitrificationRate          = RegisterParameterDouble(Model, Reaches, "Groundwater denitrification rate", MetresPerDay, 20.0);
	auto NitrateDryDeposition                    = RegisterParameterDouble(Model, Reaches, "Nitrate dry deposition", KgPerHectarePerDay, 20.0);
	auto NitrateWetDeposition                    = RegisterParameterDouble(Model, Reaches, "Nitrate wet deposition", KgPerHectarePerDay, 20.0);
	auto AmmoniumDryDeposition                   = RegisterParameterDouble(Model, Reaches, "Ammonium dry deposition", KgPerHectarePerDay, 20.0);
	auto AmmoniumWetDeposition                   = RegisterParameterDouble(Model, Reaches, "Ammonium wet deposition", KgPerHectarePerDay, 20.0);
	auto BaseFlowIndex                           = RegisterParameterDouble(Model, Reaches, "Base flow index", Dimensionless, 0.9);
	auto ReachDenitrificationRate                = RegisterParameterDouble(Model, Reaches, "Reach denitrification rate", PerDay, 20.0);
	auto ReachNitrificationRate                  = RegisterParameterDouble(Model, Reaches, "Reach nitrification rate", PerDay, 20.0);
	auto ReachEffluentNitrateConcentration       = RegisterParameterDouble(Model, Reaches, "Reach effluent nitrate concentration", MgPerL, 0.0);
	auto ReachEffluentAmmoniumConcentration      = RegisterParameterDouble(Model, Reaches, "Reach effluent ammonium concentration", MgPerL, 0.0);

	auto Streams = GetParameterGroupHandle(Model, "Streams");
	
	auto InitialStreamNitrateConcentration = RegisterParameterDouble(Model, Streams, "Initial stream nitrate concentration", MgPerL, 10.0);
	auto InitialStreamAmmoniumConcentration = RegisterParameterDouble(Model, Streams, "Initial stream ammonium concentration", MgPerL, 10.0);
	
	
	auto IncaSolver = RegisterSolver(Model, "Inca solver", 0.1, IncaDascru);
	
	auto Soils = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	auto WaterDepth     = GetEquationHandle(Model, "Water depth");
	auto RunoffToReach = GetEquationHandle(Model, "Runoff to reach");
	auto SaturationExcessInput = GetEquationHandle(Model, "Saturation excess input");
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature");
	
	auto DirectRunoffVolume = RegisterEquation(Model, "Direct runoff volume", M3PerKm2);
	auto SoilwaterVolume    = RegisterEquation(Model, "Soil water volume", M3PerKm2);
	auto GroundwaterVolume  = RegisterEquation(Model, "Groundwater volume", M3PerKm2);
	auto DirectRunoffFlow   = RegisterEquation(Model, "Direct runoff flow", CumecsPerKm2);
	auto SoilwaterFlow      = RegisterEquation(Model, "Soil water flow", CumecsPerKm2);
	auto GroundwaterFlow    = RegisterEquation(Model, "Groundwater flow", CumecsPerKm2);
	auto DirectRunoffNitrateOutput  = RegisterEquation(Model, "Direct runoff nitrate output", KgPerKm2PerDay);
	SetSolver(Model, DirectRunoffNitrateOutput, IncaSolver);
	auto DirectRunoffInitialNitrate = RegisterEquationInitialValue(Model, "Direct runoff initial nitrate", KgPerKm2);
	auto DirectRunoffNitrate        = RegisterEquationODE(Model, "Direct runoff nitrate", KgPerKm2);
	SetSolver(Model, DirectRunoffNitrate, IncaSolver);
	SetInitialValue(Model, DirectRunoffNitrate, DirectRunoffInitialNitrate);
	auto DirectRunoffAmmoniumOutput = RegisterEquation(Model, "Direct runoff ammonium output", KgPerKm2PerDay);
	SetSolver(Model, DirectRunoffAmmoniumOutput, IncaSolver);
	auto DirectRunoffInitialAmmonium = RegisterEquationInitialValue(Model, "Direct runoff initial ammonium", KgPerKm2);
	auto DirectRunoffAmmonium       = RegisterEquationODE(Model, "Direct runoff ammonium", KgPerKm2);
	SetSolver(Model, DirectRunoffAmmonium, IncaSolver);
	SetInitialValue(Model, DirectRunoffAmmonium, DirectRunoffInitialAmmonium);
	auto DrynessFactor = RegisterEquation(Model, "Dryness factor", Dimensionless);
	auto SeasonalGrowthFactor = RegisterEquation(Model, "Seasonal growth factor", Dimensionless);
	auto TemperatureFactor    = RegisterEquation(Model, "Temperature factor", Dimensionless);
	auto MaximumNitrogenUptake = RegisterEquation(Model, "Maximum nitrogen uptake", KgPerKm2PerDay);
	//SetSolver(Model, MaximumNitrogenUptake, IncaSolver);
	auto NitrateUptake = RegisterEquation(Model, "Nitrate uptake", KgPerKm2PerDay);
	SetSolver(Model, NitrateUptake, IncaSolver);
	auto Denitrification = RegisterEquation(Model, "Denitrification", KgPerKm2PerDay);
	SetSolver(Model, Denitrification, IncaSolver);
	auto Nitrification = RegisterEquation(Model, "Nitrification", KgPerKm2PerDay);
	SetSolver(Model, Nitrification, IncaSolver);
	auto Fixation = RegisterEquation(Model, "Fixation", KgPerKm2PerDay);
	SetSolver(Model, Fixation, IncaSolver);
	auto SoilwaterNitrateOutput = RegisterEquation(Model, "Soil water nitrate output", KgPerKm2PerDay);
	SetSolver(Model, SoilwaterNitrateOutput, IncaSolver);
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
	auto SoilwaterAmmoniumOutput = RegisterEquation(Model, "Soil water ammonium output", KgPerKm2PerDay);
	SetSolver(Model, SoilwaterAmmoniumOutput, IncaSolver);
	auto SoilwaterAmmoniumInput  = RegisterEquation(Model, "Soil water ammonium input", KgPerKm2PerDay);
	auto SoilwaterInitialAmmonium = RegisterEquationInitialValue(Model, "Soil water initial ammmonium", KgPerKm2);
	auto SoilwaterAmmonium = RegisterEquationODE(Model, "Soil water ammonium", KgPerKm2);
	SetSolver(Model, SoilwaterAmmonium, IncaSolver);
	SetInitialValue(Model, SoilwaterAmmonium, SoilwaterInitialAmmonium);
	auto GroundwaterDenitrification = RegisterEquation(Model, "Groundwater denitrification", KgPerKm2PerDay);
	SetSolver(Model, GroundwaterDenitrification, IncaSolver);
	auto GroundwaterNitrateOutput = RegisterEquation(Model, "Groundwater nitrate output", KgPerKm2PerDay);
	SetSolver(Model, GroundwaterNitrateOutput, IncaSolver);
	auto GroundwaterInitialNitrate = RegisterEquationInitialValue(Model, "GroundwaterInitialNitrate", KgPerKm2);
	auto GroundwaterNitrate = RegisterEquationODE(Model, "Groundwater nitrate", KgPerKm2);
	SetSolver(Model, GroundwaterNitrate, IncaSolver);
	SetInitialValue(Model, GroundwaterNitrate, GroundwaterInitialNitrate);
	auto GroundwaterAmmoniumOuput = RegisterEquation(Model, "Groundwater ammonium output", KgPerKm2PerDay);
	SetSolver(Model, GroundwaterAmmoniumOuput, IncaSolver);
	auto GroundwaterInitialAmmonium = RegisterEquationInitialValue(Model, "Groundwater initial ammmonium", KgPerKm2);
	auto GroundwaterAmmonium = RegisterEquationODE(Model, "Groundwater ammonium", KgPerKm2);
	SetSolver(Model, GroundwaterAmmonium, IncaSolver);
	SetInitialValue(Model, GroundwaterAmmonium, GroundwaterInitialAmmonium);
	
	
	EQUATION(Model, DirectRunoffInitialNitrate,
		return PARAMETER(DirectRunoffInitialNitrateConcentration) * RESULT(DirectRunoffVolume) / 1000.0;
	)
	
	EQUATION(Model, DirectRunoffInitialAmmonium,
		return PARAMETER(DirectRunoffInitialAmmoniumConcentration)* RESULT(DirectRunoffVolume) / 1000.0;
	)
	
	EQUATION(Model, SoilwaterInitialNitrate,
		return PARAMETER(SoilwaterInitialNitrateConcentration)* RESULT(SoilwaterVolume) / 1000.0;
	)
	
	EQUATION(Model, SoilwaterInitialAmmonium,
		return PARAMETER(SoilwaterInitialAmmoniumConcentration) * RESULT(SoilwaterVolume) / 1000.0;
	)
	
	EQUATION(Model, GroundwaterInitialNitrate,
		return PARAMETER(GroundwaterInitialNitrateConcentration) * RESULT(GroundwaterVolume) / 1000.0;
	)
	
	EQUATION(Model, GroundwaterInitialAmmonium,
		return PARAMETER(GroundwaterInitialAmmoniumConcentration) * RESULT(GroundwaterVolume) / 1000.0;
	)
	
	
	EQUATION(Model, DirectRunoffVolume,
		CURRENT_INDEX(LandscapeUnits); CURRENT_INDEX(Reach); //TODO TODO TODO: Improve dependency system to get rid of this
		return RESULT(WaterDepth, DirectRunoff) * 1000.0;
	)
	
	EQUATION(Model, SoilwaterVolume,
		CURRENT_INDEX(LandscapeUnits); CURRENT_INDEX(Reach); //TODO TODO TODO: Improve dependency system to get rid of this
		return RESULT(WaterDepth, Soilwater) * 1000.0;
	)
	
	EQUATION(Model, GroundwaterVolume,
		CURRENT_INDEX(LandscapeUnits); CURRENT_INDEX(Reach); //TODO TODO TODO: Improve dependency system to get rid of this
		return RESULT(WaterDepth, Groundwater) * 1000.0;
	)
	
	EQUATION(Model, DirectRunoffFlow,
		CURRENT_INDEX(LandscapeUnits); CURRENT_INDEX(Reach); //TODO TODO TODO: Improve dependency system to get rid of this
		return RESULT(RunoffToReach, DirectRunoff) * 1000.0 / 86400.0;
	)
	
	EQUATION(Model, SoilwaterFlow,
		CURRENT_INDEX(LandscapeUnits); CURRENT_INDEX(Reach); //TODO TODO TODO: Improve dependency system to get rid of this
		return RESULT(RunoffToReach, Soilwater) * 1000.0 / 86400.0;
	)
	
	EQUATION(Model, GroundwaterFlow,
		CURRENT_INDEX(LandscapeUnits); CURRENT_INDEX(Reach); //TODO TODO TODO: Improve dependency system to get rid of this
		return RESULT(RunoffToReach, Groundwater) * 1000.0 / 86400.0;
	)

	EQUATION(Model, DirectRunoffNitrateOutput,
		double output = RESULT(DirectRunoffNitrate) * RESULT(DirectRunoffFlow) * 86400.0 / RESULT(DirectRunoffVolume);
		if(RESULT(DirectRunoffVolume) == 0.0) return 0.0; //Hmm, we should perhaps have a tolerance check here instead..
		return output;
	)

	EQUATION(Model, DirectRunoffNitrate,
		return RESULT(SaturationExcessInput, DirectRunoff) * RESULT(SoilwaterNitrate) * 86400.0 / RESULT(SoilwaterVolume) - RESULT(DirectRunoffNitrateOutput);
	)
	
	EQUATION(Model, DirectRunoffAmmoniumOutput,
		double output = RESULT(DirectRunoffAmmonium) * RESULT(DirectRunoffFlow) * 86400.0 / RESULT(DirectRunoffVolume);
		if(RESULT(DirectRunoffVolume) == 0.0) return 0.0; //Hmm, we should perhaps have a tolerance check here instead..
		return output;
	)

	EQUATION(Model, DirectRunoffAmmonium,
		return RESULT(SaturationExcessInput, DirectRunoff) * RESULT(SoilwaterAmmonium) * 86400.0 / RESULT(SoilwaterVolume) - RESULT(DirectRunoffAmmoniumOutput);
	)
	
	EQUATION(Model, DrynessFactor,
		double maxratedepth = PARAMETER(MaxRateDepth);
		double zeroratedepth = PARAMETER(ZeroRateDepth);
	
		if(RESULT(WaterDepth, Soilwater) > maxratedepth ) return 1.0;
		
		if(RESULT(WaterDepth, Soilwater) > zeroratedepth )
		{
			return ( RESULT(WaterDepth, Soilwater) - zeroratedepth) / (maxratedepth - zeroratedepth);
		}
		
		return 0.0;
	)

	EQUATION(Model, SeasonalGrowthFactor,
		return PARAMETER(GrowthCurveOffset) + PARAMETER(GrowthCurveAmplitude) * sin(2.0 * Pi * ((double)CURRENT_DAY_OF_YEAR() - (double)PARAMETER(PlantGrowthStartDay) ) / (double) DAYS_THIS_YEAR() );
	)
	
	EQUATION(Model, TemperatureFactor,
		double soiltemperature = RESULT(SoilTemperature);
		return pow(1.047, (soiltemperature - 20.0) );
	)
	
	EQUATION(Model, MaximumNitrogenUptake,
		return LAST_RESULT(NitrateUptake) + LAST_RESULT(AmmoniumUptake); //NOTE: Is RESULT in original, but that messes up execution order.
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
		
		if(RESULT(MaximumNitrogenUptake) > PARAMETER(MaximumNitrogenUptakeRate)) return 0.0;
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

	EQUATION(Model, SoilwaterNitrateOutput,
		double output = RESULT(SoilwaterNitrate) * RESULT(SoilwaterFlow) * 86400.0 / RESULT(SoilwaterVolume);
		if(RESULT(SoilwaterVolume) == 0.0) return 0.0; //Hmm, we should perhaps have a tolerance check here instead..
		return output;
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
			- RESULT(SoilwaterNitrateOutput)
			- RESULT(NitrateUptake)
			- RESULT(Denitrification)
			+ RESULT(Nitrification)
			+ RESULT(Fixation);
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
		if(RESULT(MaximumNitrogenUptake) > PARAMETER(MaximumNitrogenUptakeRate)) uptake = 0.0;
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

	EQUATION(Model, SoilwaterAmmoniumOutput,
		double output = RESULT(SoilwaterAmmonium) * RESULT(SoilwaterFlow) * 86400.0 / RESULT(SoilwaterVolume);
		if(RESULT(SoilwaterVolume) == 0.0) return 0.0; //Hmm, we should perhaps have a tolerance check here instead..
		return output;
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
			- RESULT(SoilwaterAmmoniumOutput)
			- RESULT(AmmoniumUptake)
			- RESULT(Nitrification)
			- RESULT(Immobilisation)
			+ RESULT(Mineralisation);
	)
	
	EQUATION(Model, GroundwaterDenitrification,
		return RESULT(GroundwaterNitrate) * PARAMETER(GroundwaterDenitrificationRate) * RESULT(TemperatureFactor) / RESULT(GroundwaterVolume) * 1000000.0;
	)

	EQUATION(Model, GroundwaterNitrateOutput,
		double output = RESULT(GroundwaterNitrate) * RESULT(GroundwaterFlow) * 86400.0 / RESULT(GroundwaterVolume);
		if(RESULT(GroundwaterVolume) == 0.0) return 0.0; //Hmm, we should perhaps have a tolerance check here instead..
		return output;
	)
	
	EQUATION(Model, GroundwaterNitrate,
		return PARAMETER(BaseFlowIndex) * RESULT(SoilwaterNitrateOutput) - RESULT(GroundwaterNitrateOutput) - RESULT(GroundwaterDenitrification);
	)
	
	EQUATION(Model, GroundwaterAmmoniumOuput,
		double output = RESULT(GroundwaterAmmonium) * RESULT(GroundwaterFlow) * 86400.0 / RESULT(GroundwaterVolume);
		if(RESULT(GroundwaterVolume) == 0.0) return 0.0; //Hmm, we should perhaps have a tolerance check here instead..
		return output;
	)
	
	EQUATION(Model, GroundwaterAmmonium,
		return PARAMETER(BaseFlowIndex) * RESULT(SoilwaterAmmoniumOutput) - RESULT(GroundwaterAmmoniumOuput);
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
		return RESULT(SoilwaterNitrateOutput) + RESULT(GroundwaterNitrateOutput) + RESULT(DirectRunoffNitrateOutput);
	)
	
	EQUATION(Model, DiffuseNitrate,
		return RESULT(TotalNitrateToStream) * PARAMETER(TerrestrialCatchmentArea) * PARAMETER(Percent) / 100.0;
	)
	
	EQUATION(Model, TotalAmmoniumToStream,
		return RESULT(SoilwaterAmmoniumOutput) + RESULT(GroundwaterAmmoniumOuput) + RESULT(DirectRunoffAmmoniumOutput);
	)
	
	EQUATION(Model, DiffuseAmmonium,
		return RESULT(TotalAmmoniumToStream) * PARAMETER(TerrestrialCatchmentArea) * PARAMETER(Percent) / 100.0;
	)
	
	
	
	auto ReachSolver = GetSolverHandle(Model, "Reach solver"); //NOTE: from persist
	auto ReachFlow   = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume = GetEquationHandle(Model, "Reach volume");
	
	auto ConvertMassToConcentration = RegisterEquation(Model, "Convert mass to concentration", Dimensionless);
	SetSolver(Model, ConvertMassToConcentration, ReachSolver);
	auto ConvertConcentrationToMass = RegisterEquation(Model, "Convert concentration to mass", Dimensionless);
	SetSolver(Model, ConvertConcentrationToMass, ReachSolver);
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
	
	EQUATION(Model, ConvertMassToConcentration,
		return 1000.0 / RESULT(ReachVolume);
	)
	
	EQUATION(Model, ConvertConcentrationToMass,
		return RESULT(ReachVolume) / 1000.0;
	)
	
	EQUATION(Model, ReachNitrateOutput,
		return RESULT(ReachNitrate) * RESULT(ReachFlow) * 86400.0 / RESULT(ReachVolume);
	)
	
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature");
	
	EQUATION(Model, WaterTemperatureFactor,
		return pow(1.047, RESULT(WaterTemperature) - 20.0);
	)
	
	EQUATION(Model, ReachDenitrification,
		return
			  PARAMETER(ReachDenitrificationRate)
			* RESULT(ReachNitrate) * RESULT(ConvertMassToConcentration)
			* RESULT(WaterTemperatureFactor)
			* RESULT(ReachVolume)
			/ 1000.0;
	)
	
	EQUATION(Model, ReachNitrification,
		return
			  PARAMETER(ReachNitrificationRate)
			* RESULT(ReachAmmonium) * RESULT(ConvertMassToConcentration)
			* RESULT(WaterTemperatureFactor)
			* RESULT(ReachVolume)
			/ 1000.0;
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
		return PARAMETER(InitialStreamNitrateConcentration) * RESULT(ConvertConcentrationToMass);
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
		return RESULT(ReachAmmonium) * RESULT(ReachFlow) * 86400.0 / RESULT(ReachVolume);
	)
	
	EQUATION(Model, ReachAmmoniumInitialValue,
		return PARAMETER(InitialStreamAmmoniumConcentration) * RESULT(ConvertConcentrationToMass);
	)
	
	EQUATION(Model, ReachAmmonium,
		return RESULT(ReachTotalAmmoniumInput) - RESULT(ReachAmmoniumOutput) - RESULT(ReachNitrification); //-RESULT(ReachAmmoniumAbstraction);
	)
	
	//NOTE: Added this for easier calibration - MDN
	auto ReachNitrateConcentration = RegisterEquation(Model, "Reach nitrate concentration", KgPerM3);
	EQUATION(Model, ReachNitrateConcentration,
		return RESULT(ReachNitrate) / RESULT(ReachVolume);
	)
}

#define INCAN_MODEL_H
#endif
