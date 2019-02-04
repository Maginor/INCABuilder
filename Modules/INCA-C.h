

//NOTE NOTE NOTE: This is still in development and is not finished.

static void
AddINCACModel(inca_model *Model)
{
	//NOTE: Uses PERSiST, SoilTemperature, WaterTemperature
	
	auto Dimensionless  = RegisterUnit(Model);
	auto Mm             = RegisterUnit(Model, "mm");
	auto KgPerKm2       = RegisterUnit(Model, "kg/km^2");
	auto GPerM2PerDay   = RegisterUnit(Model, "g/m^2/day");
	auto JulianDay      = RegisterUnit(Model, "Julian day");
	auto Days           = RegisterUnit(Model, "day");
	auto kWPerM2        = RegisterUnit(Model, "kW/m^2");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto KgPerM3        = RegisterUnit(Model, "kg/m^3");
	auto MPerDay        = RegisterUnit(Model, "m/day");
	
	auto Soils = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto UpperSoil    = RequireIndex(Model, Soils, "Upper soil water");
	auto LowerSoil    = RequireIndex(Model, Soils, "Lower soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	
	//TODO: As always, find better default values, min/max, description..
	auto SMDMax                        = RegisterParameterDouble(Model, .., "SMD max", Mm, 300.0);
	
	auto LitterFallRate                = RegisterParameterDouble(Model, .., "Litter fall rate", GPerM2PerDay, 1.0);
	auto LitterFallStartDay            = RegisterParameterUInt(Model, ..,   "Litter fall start day", JulianDay, 300, 1, 355);
	auto LitterFallDuration            = RegisterParameterUInt(Model, ..,   "Litter fall duration", Days, 30, 0, 356);
	auto RootBreakdownRate             = RegisterParameterDouble(Model, .., "Root breakdown rate", GPerM2PerDay, 1.0);
	
	auto SoilTemperatureRateMultiplier = RegisterParameterDouble(Model, .., "Soil temperature rate multiplier", Dimensionless);
	auto SoilTemperatureRateOffset     = RegisterParameterDouble(Model, .., "Soil temperature rate offset", DegreesCelsius);
	
	auto DICMassTransferVelocity       = RegisterParameterDouble(Model, .., "DIC mass transfer velocity", MPerDay, 1.0);
	auto DICSaturationConstant         = RegisterParameterDouble(Model, .., "DIC saturation constant", KgPerM3, 1.0);
	
	
	auto MaximumCapacity = GetParameterDoubleHandle(Model, "Maximum capacity");
	auto WaterDepth      = GetEquationHandle(Model, "Water depth");
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature");
	
	
	
	auto SoilMoistureDeficit = RegisterEquation(Model, "Soil moisture deficit", Mm);
	auto SoilMoistureDeficitFraction = RegisterEquation(Model, "Soil moisture deficit fraction", Dimensionless);
	
	EQUATION(Model, SoilMoistureDeficit,
		return PARAMETER(MaximumCapacity) - RESULT(WaterDepth);
	)
	
	EQUATION(Model, SoilProcessRateModifier,
		return 
		pow(PARAMETER(SoilTemperatureRateMultiplier), RESULT(SoilTemperature) - PARAMETER(SoilTemperatureRateOffset)) *
		(Max(PARAMETER(SMDMax) - RESULT(SoilMoistureDeficit), 0.0)) / PARAMETER(SMDMax); //TODO: Have a smoother curve for this..
	)
	
	
	
	auto IncaSolver = RegisterSolver(Model, "INCA Solver", 0.1, IncaDascru);
	
	auto LitterFall = RegisterEquation(Model, "Litter fall", GPerM2PerDay)
	
	auto SOCMassInUpperSoilLayer = RegisterEquationODE(Model, "SOC mass in upper soil layer", KgPerKm2);
	SetSolver(Model, SOCMassInUpperSoilLayer, IncaDascru);
	//SetInitialValue()
	
	auto DOCMassInUpperSoilLayer = RegisterEquationODE(Model, "DOC mass in upper soil layer", KgPerKm2);
	SetSolver(Model, DOCMassInUpperSoilLayer, IncaDascru);
	//SetInitialValue()
	
	auto DICMassInUpperSoilLayer = RegisterEquationODE(Model, "DIC mass in upper soil layer", KgPerKm2);
	SetSolver(Model, DICMassInUpperSoilLayer, IncaDascru);
	//SetInitialValue()
	
	auto SOCMassInLowerSoilLayer = RegisterEquationODE(Model, "SOC mass in lower soil layer", KgPerKm2);
	SetSolver(Model, SOCMassInLowerSoilLayer, IncaDascru);
	//SetInitialValue()
	
	auto DOCMassInLowerSoilLayer = RegisterEquationODE(Model, "DOC mass in lower soil layer", KgPerKm2);
	SetSolver(Model, DOCMassInUpperSoilLayer, IncaDascru);
	//SetInitialValue()
	
	auto DICMassInLowerSoilLayer = RegisterEquationODE(Model, "DIC mass in lower soil layer", KgPerKm2);
	SetSolver(Model, DICMassInLowerSoilLayer, IncaDascru);
	//SetInitialValue()
	
	
	EQUATION(LitterFall,
		double litter = PARAMETER(LitterFallRate);
		u64 start     = PARAMETER(LitterFallStartDay);
		u64 duration  = PARAMETER(LitterFallDuration);
		if( (CURRENT_DAY_OF_YEAR() < start) || (CURRENT_DAY_OF_YEAR() >= start + duration))
		{
			litter = 0.0;
		}
	
		return litter;
	)
	
	EQUATION(Model, DOCFromDirectRunoffToReach)
	
	EQUATION(Model, DICFromDirectRunoffToReach)
	
	EQUATION(Model, DOCFromUpperBoxToDirectRunoff)
	
	EQUATION(Model, DICFromUpperBoxToDirectRunoff)
	
	EQUATION(Model, DOCFromUpperBoxToLowerBox)
	
	EQUATION(Model, DICFromUpperBoxToLowerBox)
	
	EQUATION(Model, DOCFromUpperBoxToReach)
	
	EQUATION(Model, DICFromUpperBoxToReach)
	
	EQUATION(Model, DOCFromLowerBoxToReach)
	
	EQUATION(Model, DICFromLowerBoxToReach)
	
	EQUATION(Model, DOCFromLowerBoxToGroundwater)
	
	EQUATION(Model, DICFromLowerBoxToGroundwater)
	
	EQUATION(Model, DOCFromGroundwaterToReach)
	
	EQUATION(Model, DICFromGroundwaterToReach)
	
	
	EQUATION(Model, SOCMineralisationInUpperSoil,
		return RESULT(SoilProcessRateModifier) * PARAMETER(SOCMineralisationBaseRateUpperSoil) * RESULT(SOCMassInUpperSoilLayer);
	)
	
	EQUATION(Model, SOCDesorptionInUpperSoil,
		return RESULT(SoilProcessRateModifier) * PARAMETER(SOCDesorptionBaseRateUpperSoil) * RESULT(SOCMassInUpperSoilLayer);
	)
	
	EQUATION(Model, DOCSorptionInUpperSoil,
		return RESULT(SoilProcessRateModifier) * PARAMETER(DOCSorptionBaseRateUpperSoil) * RESULT(DOCMassInUpperSoilLayer);
	)
	
	EQUATION(Model, DOCMineralisationInUpperSoil,
		return RESULT(SoilProcessRateModifier) * PARAMETER(DOCMineralisationBaseRateUpperSoil) * RESULT(DOCMassInUpperSoilLayer);
	)
	
	EQUATION(Model, SOCMineralisationInLowerSoil,
		return RESULT(SoilProcessRateModifier) * PARAMETER(SOCMineralisationBaseRateLowerSoil) * RESULT(SOCMassInLowerSoilLayer);
	)
	
	EQUATION(Model, SOCDesorptionInLowerSoil,
		return RESULT(SoilProcessRateModifier) * PARAMETER(SOCDesorptionBaseRateLowerSoil) * RESULT(SOCMassInLowerSoilLayer);
	)
	
	EQUATION(Model, DOCSorptionInLowerSoil,
		return RESULT(SoilProcessRateModifier) * PARAMETER(DOCSorptionBaseRateLowerSoil) * RESULT(DOCMassInLowerSoilLayer);
	)
	
	EQUATION(Model, DOCMineralisationInLowerSoil,
		return RESULT(SoilProcessRateModifier) * PARAMETER(DOCMineralisationBaseRateLowerSoil) * RESULT(DOCMassInLowerSoilLayer);
	)
	
	EQUATION(Model, DICMassTransferToAtmosphere,
		double upperlayervolume = RESULT(WaterDepth, UpperSoil) * 1000.0;  //TODO: Check if unit conversion is correct
		return PARAMETER(DICMassTransferVelocity) * (SafeDivide(RESULT(DICMassInUpperSoilLayer), upperlayervolume) - PARAMETER(DICSaturationConstant));
	)
	
	
	EQUATION(Model, DOCMassInDirectRunoff,
		return RESULT(DOCFromUpperBoxToDirectRunoff) - RESULT(DOCFromDirectRunoffToReach);
	)
	
	EQUATION(Model, DICMassInDirectRunoff,
		return RESULT(DICFromUpperBoxToDirectRunoff) - RESULT(DICFromDirectRunoffToReach);
	)
	
	EQUATION(Model, SOCMassInUpperSoilLayer,
		return 
			  1000.0 * (RESULT(LitterFall) + PARAMETER(RootBreakdownRate)) 
			+ RESULT(DOCSorptionInUpperSoil)
			- RESULT(SOCMineralisationInUpperSoil)
			- RESULT(SOCDesorptionInUpperSoil);
	)
	
	EQUATION(Model, DOCMassInUpperSoilLayer,
		return
			  RESULT(SOCDesorptionInUpperSoil)
			- RESULT(DOCSorptionInUpperSoil)
			- RESULT(DOCMineralisationInUpperSoil)
			
			- RESULT(DOCFromUpperBoxToDirectRunoff)
			- RESULT(DOCFromUpperBoxToReach)
			- RESULT(DOCFromUpperBoxToLowerBox);
	)
	
	EQUATION(Model, DICMassInUpperSoilLayer,
		return
			  RESULT(SOCMineralisationInUpperSoil)
			+ RESULT(DOCMineralisationInUpperSoil)
			- RESULT(DICMassTransferToAtmosphere)
			
			- RESULT(DICFromUpperBoxToDirectRunoff)
			- RESULT(DICFromUpperBoxToReach)
			- RESULT(DICFromUpperBoxToLowerBox);
	)
	
	EQUATION(Model, SOCMassInLowerSoilLayer,
		return
			+ RESULT(DOCSorptionInLowerSoil)
			- RESULT(SOCMineralisationInLowerSoil)
			- RESULT(SOCDesorptionInLowerSoil);
	)
	
	EQUATION(Model, DOCMassInLowerSoilLayer,
		return
			  RESULT(SOCDesorptionInLowerSoil)
			- RESULT(DOCSorptionInLowerSoil)
			- RESULT(DOCMineralisationInLowerSoil)
			
			+ RESULT(DOCFromUpperBoxToLowerBox)
			- RESULT(DOCFromLowerBoxToReach)
			- RESULT(DOCFromLowerBoxToGroundwater);
	)
	
	EQUATION(Model, DICMassInLowerSoilLayer,
		return
			  RESULT(SOCMineralisationInLowerSoil)
			+ RESULT(DOCMineralisationInLowerSoil)
			
			+ RESULT(DICFromUpperBoxToLowerBox)
			- RESULT(DICFromLowerBoxToReach)
			- RESULT(DICFromLowerBoxToGroundwater);
	)
	
	EQUATION(Model, DOCMassInGroundwater,
		return RESULT(DOCFromLowerBoxToGroundwater) - RESULT(DOCFromGroundwaterToReach);
	)
	
	EQUATION(Model, DICMassInGroundwater,
		return RESULT(DICFromLowerBoxToGroundwater) - RESULT(DICFromGroundwaterToReach);
	)
	
	
	EQUATION(Model, DiffuseDOCOutput,
		return PARAMETER(Percent)/100.0 * PARAMETER(TerrestrialCatchmentArea) * (RESULT(DOCFromDirectRunoffToReach) + RESULT(DOCFromUpperBoxToReach) + RESULT(DOCFromLowerBoxToReach) + RESULT(DOCFromGroundwaterToReach));
	)
	
	EQUATION(Model, DiffuseDICOutput,
		return PARAMETER(Percent)/100.0 * PARAMETER(TerrestrialCatchmentArea) * (RESULT(DICFromDirectRunoffToReach) + RESULT(DICFromUpperBoxToReach) + RESULT(DICFromLowerBoxToReach) + RESULT(DICFromGroundwaterToReach));
	)
	
	// TODO: sum diffuse up in total
	
	
	
	
	auto DOCMineralisationSelfShadingMultiplier = RegisterParameterDouble(Model, .., "Aquatic DOC mineralisation self-shading multiplier", Dimensionless, 1.0);
	auto DOCMineralisationOffset                = RegisterParameterDouble(Model, .., "Aquatic DOC mineralisation offset", Dimensionless, 1.0);
	
	
	auto SolarRadiation = RegisterInput(Model, "Solar radiation");
	
	
	EQUATION(Model, ReachDOCInput,
		double upstreamdoc = 0.0;
		FOREACH_INPUT(Reach,
			upstreamdoc += RESULT(ReachDOCOutput, *Input);
		)
		
		return upstreamdoc + RESULT(TotalDiffuseDOCOutput); //+effluent?
	)
	
	EQUATION(Model, ReachDOCOutput,
		return 86400.0 * SafeDivide(RESULT(DOCMassInReach) * RESULT(ReachFlow), RESULT(ReachVolume)); //TODO: Check unit conversion
	)
	
	EQUATION(Model, PhotoMineralisationRate,
		return
			PARAMETER(DOCMineralisationSelfShadingMultiplier) * INPUT(SolarRadiation) / 
			(PARAMETER(DOCMineralisationOffset) + SafeDivide(RESULT(DOCMassInReach), RESULT(ReachVolume)));
	)
	
	EQUATION(Model, MicrobialMineralisationRate,
		return PARAMETER(MicrobialMineralisationBaseRate); //TODO: Dependence on water temperature??
	)
	
	EQUATION(Model, DOCMassInReach,
		return
			  RESULT(ReachDOCInput)
			- RESULT(ReachDOCOutput)
			- (RESULT(PhotoMineralisationRate) + RESULT(MicrobialMineralisationRate)) * RESULT(DOCMassInReach);
	)
	
	
	EQUATION(Model, ReachDICInput,
		double upstreamdic = 0.0;
		FOREACH_INPUT(Reach,
			upstreamdic += RESULT(ReachDICOutput, *Input);
		)
		
		return upstreamdic + RESULT(TotalDiffuseDICOutput); // + effluent?
	)
	
	EQUATION(Model, ReachDICOutput,
		return 86400.0 * SafeDivide(RESULT(DICMassInReach) * RESULT(ReachFlow), RESULT(ReachVolume)); //TODO: Check unit conversion
	)
	
	EQUATION(Model, DICMassInReach,
		return
			  RESULT(ReachDICInput)
			- RESULT(ReachDICOutput)
			+ (RESULT(PhotoMineralisationRate) + RESULT(MicrobialMineralisationRate)) * RESULT(DOCMassInReach)
			- PARAMETER(ReachDICLossRate) * RESULT(DICMassInReach);                          //TODO: Should the loss rate depend on anything??
	)
	
}