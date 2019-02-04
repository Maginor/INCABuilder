

//NOTE NOTE NOTE: This is still in development and is not finished.


inline double
SCurve(double X, double Threshold1, double Threshold2)
{
	if(X < Threshold1) return 0.0;
	if(X > Threshold2) return 1.0;
	double Y = (X - Threshold1) / (Threshold2 - Threshold1);
	return (3.0 - 2.0*Y)*Y*Y;
}

static void
AddINCACModel(inca_model *Model)
{
	//NOTE: Uses PERSiST, SoilTemperature (, WaterTemperature??)
	
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
	auto KgPerDay       = RegisterUnit(Model, "kg/day");
	auto PerDay         = RegisterUnit(Model, "/day");
	auto Kg             = RegisterUnit(Model, "kg");
	
	auto Soils          = GetIndexSetHandle(Model, "Soils");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto UpperSoil    = RequireIndex(Model, Soils, "Upper soil water");
	auto LowerSoil    = RequireIndex(Model, Soils, "Lower soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	
	//TODO: As always, find better default values, min/max, description..
	auto SoilMoistureThreshold         = RegisterParameterDouble(Model, Land, "Soil moisture threshold", Mm, 50.0, 0.0, 10000.0, "The minimal water depth at which carbon processes in the soil occur");
	
	auto LitterFallRate                = RegisterParameterDouble(Model, Land, "Litter fall rate", GPerM2PerDay, 1.0);
	auto LitterFallStartDay            = RegisterParameterUInt(  Model, Land, "Litter fall start day", JulianDay, 300, 1, 364);
	auto LitterFallDuration            = RegisterParameterUInt(  Model, Land, "Litter fall duration", Days, 30, 0, 365);
	auto RootBreakdownRate             = RegisterParameterDouble(Model, Land, "Root breakdown rate", GPerM2PerDay, 1.0);
	
	auto SoilTemperatureRateMultiplier = RegisterParameterDouble(Model, Land, "Soil temperature rate multiplier", Dimensionless, 1.0);
	auto SoilTemperatureRateOffset     = RegisterParameterDouble(Model, Land, "Soil temperature rate offset", DegreesCelsius, 20.0);
	
	auto DICMassTransferVelocity            = RegisterParameterDouble(Model, Land, "DIC mass transfer velocity", MPerDay, 1.0);
	auto DICSaturationConstant              = RegisterParameterDouble(Model, Land, "DIC saturation constant", KgPerM3, 1.0);
	auto SOCMineralisationBaseRateUpperSoil = RegisterParameterDouble(Model, Land, "SOC mineralisation base rate in upper soil layer", PerDay, 0.1);
	auto SOCMineralisationBaseRateLowerSoil = RegisterParameterDouble(Model, Land, "SOC mineralisation base rate in lower soil layer", PerDay, 0.1);
	auto SOCDesorptionBaseRateUpperSoil     = RegisterParameterDouble(Model, Land, "SOC desorption base rate in upper soil layer", PerDay, 0.1);
	auto SOCDesorptionBaseRateLowerSoil     = RegisterParameterDouble(Model, Land, "SOC desorption base rate in lower soil layer", PerDay, 0.1);
	auto DOCSorptionBaseRateUpperSoil       = RegisterParameterDouble(Model, Land, "DOC sorption base rate in upper soil layer", PerDay, 0.1);
	auto DOCSorptionBaseRateLowerSoil       = RegisterParameterDouble(Model, Land, "DOC sorption base rate in lower soil layer", PerDay, 0.1);
	auto DOCMineralisationBaseRateUpperSoil = RegisterParameterDouble(Model, Land, "DOC mineralisation base rate in upper soil layer", PerDay, 0.1);
	auto DOCMineralisationBaseRateLowerSoil = RegisterParameterDouble(Model, Land, "DOC mineralisation base rate in lower soil layer", PerDay, 0.1);
	
	auto IncaSolver = RegisterSolver(Model, "INCA Solver", 0.1, IncaDascru);
	
	auto SoilProcessRateModifier          = RegisterEquation(Model, "Soil process rate modifier", Dimensionless);
	auto LitterFall                       = RegisterEquation(Model, "Litter fall", GPerM2PerDay);
	auto DirectRunoffToReachFraction      = RegisterEquation(Model, "Direct runoff to reach fraction", Dimensionless);
	auto DirectRunoffToUpperLayerFraction = RegisterEquation(Model, "Direct runoff to upper layer fraction", Dimensionless);
	auto UpperLayerToDirectRunoffFraction = RegisterEquation(Model, "Upper layer to direct runoff fraction", Dimensionless);
	auto UpperLayerToLowerLayerFraction   = RegisterEquation(Model, "Upper layer to lower layer fraction", Dimensionless);
	auto UpperLayerToReachFraction        = RegisterEquation(Model, "Upper layer to reach fraction", Dimensionless);
	auto LowerLayerToReachFraction        = RegisterEquation(Model, "Lower layer to reach fraction", Dimensionless);
	auto LowerLayerToGroundwaterFraction  = RegisterEquation(Model, "Lower layer to groundwater fraction", Dimensionless);
	auto GroundwaterToReachFraction       = RegisterEquation(Model, "Groundwater to reach fraction", Dimensionless);
	
	auto SOCMineralisationInUpperSoil = RegisterEquation(Model, "SOC mineralisation in upper soil layer", KgPerDay);
	SetSolver(Model, SOCMineralisationInUpperSoil, IncaSolver);
	auto SOCMineralisationInLowerSoil = RegisterEquation(Model, "SOC mineralisation in lower soil layer", KgPerDay);
	SetSolver(Model, SOCMineralisationInLowerSoil, IncaSolver);
	auto SOCDesorptionInUpperSoil     = RegisterEquation(Model, "SOC desorption in upper soil layer", KgPerDay);
	SetSolver(Model, SOCDesorptionInUpperSoil, IncaSolver);
	auto SOCDesorptionInLowerSoil     = RegisterEquation(Model, "SOC desorption in lower soil layer", KgPerDay);
	SetSolver(Model, SOCDesorptionInLowerSoil, IncaSolver);
	auto DOCSorptionInUpperSoil       = RegisterEquation(Model, "DOC sorption in upper soil layer", KgPerDay);
	SetSolver(Model, DOCSorptionInUpperSoil, IncaSolver);
	auto DOCSorptionInLowerSoil       = RegisterEquation(Model, "DOC sorption in lower soil layer", KgPerDay);
	SetSolver(Model, DOCSorptionInLowerSoil, IncaSolver);
	auto DOCMineralisationInUpperSoil = RegisterEquation(Model, "DOC mineralisation in upper soil layer", KgPerDay);
	SetSolver(Model, DOCMineralisationInUpperSoil, IncaSolver);
	auto DOCMineralisationInLowerSoil = RegisterEquation(Model, "DOC mineralisation in lower soil layer", KgPerDay);
	SetSolver(Model, DOCMineralisationInLowerSoil, IncaSolver);
	auto DICMassTransferToAtmosphere  = RegisterEquation(Model, "DIC mass transfer to atmosphere", KgPerDay);
	SetSolver(Model, DICMassTransferToAtmosphere, IncaSolver);
	
	auto DOCMassInDirectRunoff   = RegisterEquationODE(Model, "DOC mass in direct runoff", KgPerKm2);
	SetSolver(Model, DOCMassInDirectRunoff, IncaSolver);
	//SetInitialValue()
	
	auto DICMassInDirectRunoff   = RegisterEquationODE(Model, "DIC mass in direct runoff", KgPerKm2);
	SetSolver(Model, DICMassInDirectRunoff, IncaSolver);
	//SetInitialValue()
	
	auto SOCMassInUpperSoilLayer = RegisterEquationODE(Model, "SOC mass in upper soil layer", KgPerKm2);
	SetSolver(Model, SOCMassInUpperSoilLayer, IncaSolver);
	//SetInitialValue()
	
	auto DOCMassInUpperSoilLayer = RegisterEquationODE(Model, "DOC mass in upper soil layer", KgPerKm2);
	SetSolver(Model, DOCMassInUpperSoilLayer, IncaSolver);
	//SetInitialValue()
	
	auto DICMassInUpperSoilLayer = RegisterEquationODE(Model, "DIC mass in upper soil layer", KgPerKm2);
	SetSolver(Model, DICMassInUpperSoilLayer, IncaSolver);
	//SetInitialValue()
	
	auto SOCMassInLowerSoilLayer = RegisterEquationODE(Model, "SOC mass in lower soil layer", KgPerKm2);
	SetSolver(Model, SOCMassInLowerSoilLayer, IncaSolver);
	//SetInitialValue()
	
	auto DOCMassInLowerSoilLayer = RegisterEquationODE(Model, "DOC mass in lower soil layer", KgPerKm2);
	SetSolver(Model, DOCMassInLowerSoilLayer, IncaSolver);
	//SetInitialValue()
	
	auto DICMassInLowerSoilLayer = RegisterEquationODE(Model, "DIC mass in lower soil layer", KgPerKm2);
	SetSolver(Model, DICMassInLowerSoilLayer, IncaSolver);
	//SetInitialValue()
	
	auto DOCMassInGroundwater   = RegisterEquationODE(Model, "DOC mass in groundwater", KgPerKm2);
	SetSolver(Model, DOCMassInGroundwater, IncaSolver);
	//SetInitialValue()
	
	auto DICMassInGroundwater   = RegisterEquationODE(Model, "DIC mass in groundwater", KgPerKm2);
	SetSolver(Model, DICMassInGroundwater, IncaSolver);
	//SetInitialValue()
	
	
	auto DiffuseDOCOutput = RegisterEquation(Model, "Diffuse DOC output", KgPerDay);
	auto DiffuseDICOutput = RegisterEquation(Model, "Diffuse DIC output", KgPerDay);
	
	auto TotalDiffuseDOCOutput = RegisterEquationCumulative(Model, "Total diffuse DOC output", DiffuseDOCOutput, LandscapeUnits);
	auto TotalDiffuseDICOutput = RegisterEquationCumulative(Model, "Total diffuse DIC output", DiffuseDICOutput, LandscapeUnits);
	
	
	auto MaximumCapacity          = GetParameterDoubleHandle(Model, "Maximum capacity");
	auto Percent                  = GetParameterDoubleHandle(Model, "%");
	auto TerrestrialCatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	
	auto SoilTemperature       = GetEquationHandle(Model, "Soil temperature");
	auto WaterDepth            = GetEquationHandle(Model, "Water depth");
	auto RunoffToReach         = GetEquationHandle(Model, "Runoff to reach");
	auto SaturationExcessInput = GetEquationHandle(Model, "Saturation excess input");
	auto PercolationInput      = GetEquationHandle(Model, "Percolation input");
	
	
	
	EQUATION(Model, LitterFall,
		double litter = PARAMETER(LitterFallRate);
		u64 start     = PARAMETER(LitterFallStartDay);
		u64 duration  = PARAMETER(LitterFallDuration);
		if( (CURRENT_DAY_OF_YEAR() < start) || (CURRENT_DAY_OF_YEAR() >= start + duration))
		{
			litter = 0.0;
		}
	
		return litter;
	)
	
	EQUATION(Model, SoilProcessRateModifier,
		return 
		pow(PARAMETER(SoilTemperatureRateMultiplier), RESULT(SoilTemperature) - PARAMETER(SoilTemperatureRateOffset)) *
		SCurve(RESULT(WaterDepth), PARAMETER(SoilMoistureThreshold), PARAMETER(MaximumCapacity));
	)
	
	//NOTE: The following equations make a lot of assumptions about where you can and can not have percolation or infiltration excess!
	//TODO: Check if we should not use some of the intermediate water levels instead?
	
	EQUATION(Model, DirectRunoffToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, DirectRunoff), RESULT(WaterDepth, DirectRunoff));
	)
	
	EQUATION(Model, DirectRunoffToUpperLayerFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(PercolationInput, UpperSoil), RESULT(WaterDepth, DirectRunoff));
	)
	
	EQUATION(Model, UpperLayerToDirectRunoffFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(SaturationExcessInput, DirectRunoff), RESULT(WaterDepth, DirectRunoff));
	)
	
	EQUATION(Model, UpperLayerToLowerLayerFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(PercolationInput, LowerSoil), RESULT(WaterDepth, UpperSoil));
	)
	
	EQUATION(Model, UpperLayerToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, UpperSoil), RESULT(WaterDepth, UpperSoil));
	)
	
	EQUATION(Model, LowerLayerToGroundwaterFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(PercolationInput, Groundwater), RESULT(WaterDepth, LowerSoil));
	)
	
	EQUATION(Model, LowerLayerToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, LowerSoil), RESULT(WaterDepth, LowerSoil));
	)

	EQUATION(Model, GroundwaterToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, Groundwater), RESULT(WaterDepth, Groundwater));
	)
	
	
	EQUATION(Model, SOCMineralisationInUpperSoil,
		return RESULT(SoilProcessRateModifier, UpperSoil) * PARAMETER(SOCMineralisationBaseRateUpperSoil) * RESULT(SOCMassInUpperSoilLayer);
	)
	
	EQUATION(Model, SOCDesorptionInUpperSoil,
		return RESULT(SoilProcessRateModifier, UpperSoil) * PARAMETER(SOCDesorptionBaseRateUpperSoil) * RESULT(SOCMassInUpperSoilLayer);
	)
	
	EQUATION(Model, DOCSorptionInUpperSoil,
		return RESULT(SoilProcessRateModifier, UpperSoil) * PARAMETER(DOCSorptionBaseRateUpperSoil) * RESULT(DOCMassInUpperSoilLayer);
	)
	
	EQUATION(Model, DOCMineralisationInUpperSoil,
		return RESULT(SoilProcessRateModifier, UpperSoil) * PARAMETER(DOCMineralisationBaseRateUpperSoil) * RESULT(DOCMassInUpperSoilLayer);
	)
	
	EQUATION(Model, SOCMineralisationInLowerSoil,
		return RESULT(SoilProcessRateModifier, LowerSoil) * PARAMETER(SOCMineralisationBaseRateLowerSoil) * RESULT(SOCMassInLowerSoilLayer);
	)
	
	EQUATION(Model, SOCDesorptionInLowerSoil,
		return RESULT(SoilProcessRateModifier, LowerSoil) * PARAMETER(SOCDesorptionBaseRateLowerSoil) * RESULT(SOCMassInLowerSoilLayer);
	)
	
	EQUATION(Model, DOCSorptionInLowerSoil,
		return RESULT(SoilProcessRateModifier, LowerSoil) * PARAMETER(DOCSorptionBaseRateLowerSoil) * RESULT(DOCMassInLowerSoilLayer);
	)
	
	EQUATION(Model, DOCMineralisationInLowerSoil,
		return RESULT(SoilProcessRateModifier, LowerSoil) * PARAMETER(DOCMineralisationBaseRateLowerSoil) * RESULT(DOCMassInLowerSoilLayer);
	)
	
	EQUATION(Model, DICMassTransferToAtmosphere,
		double upperlayervolume = RESULT(WaterDepth, UpperSoil) * 1000.0;  //TODO: Check if unit conversion is correct
		return PARAMETER(DICMassTransferVelocity) * (SafeDivide(RESULT(DICMassInUpperSoilLayer), upperlayervolume) - PARAMETER(DICSaturationConstant));
	)
	
	
	EQUATION(Model, DOCMassInDirectRunoff,
		return
			- RESULT(DOCMassInDirectRunoff) * (RESULT(DirectRunoffToReachFraction) + RESULT(DirectRunoffToUpperLayerFraction))
			+ RESULT(DOCMassInUpperSoilLayer) * RESULT(UpperLayerToDirectRunoffFraction);
	)
	
	EQUATION(Model, DICMassInDirectRunoff,
		return
			- RESULT(DICMassInDirectRunoff) * (RESULT(DirectRunoffToReachFraction) + RESULT(DirectRunoffToUpperLayerFraction))
			+ RESULT(DICMassInUpperSoilLayer) * RESULT(UpperLayerToDirectRunoffFraction);
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
			
			- RESULT(DOCMassInUpperSoilLayer) * (RESULT(UpperLayerToDirectRunoffFraction) + RESULT(UpperLayerToReachFraction) + RESULT(UpperLayerToLowerLayerFraction))
			+ RESULT(DOCMassInDirectRunoff) * RESULT(DirectRunoffToUpperLayerFraction);
	)
	
	EQUATION(Model, DICMassInUpperSoilLayer,
		return
			  RESULT(SOCMineralisationInUpperSoil)
			+ RESULT(DOCMineralisationInUpperSoil)
			- RESULT(DICMassTransferToAtmosphere)
			
			- RESULT(DICMassInUpperSoilLayer) * (RESULT(UpperLayerToDirectRunoffFraction) + RESULT(UpperLayerToReachFraction) + RESULT(UpperLayerToLowerLayerFraction))
			+ RESULT(DICMassInDirectRunoff) * RESULT(DirectRunoffToUpperLayerFraction);
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
			
			+ RESULT(DOCMassInUpperSoilLayer) * RESULT(UpperLayerToLowerLayerFraction)
			- RESULT(DOCMassInLowerSoilLayer) * (RESULT(LowerLayerToReachFraction) + RESULT(LowerLayerToGroundwaterFraction));
	)
	
	EQUATION(Model, DICMassInLowerSoilLayer,
		return
			  RESULT(SOCMineralisationInLowerSoil)
			+ RESULT(DOCMineralisationInLowerSoil)
			
			+ RESULT(DICMassInUpperSoilLayer) * RESULT(UpperLayerToLowerLayerFraction)
			- RESULT(DICMassInLowerSoilLayer) * (RESULT(LowerLayerToReachFraction) + RESULT(LowerLayerToGroundwaterFraction));
	)
	
	EQUATION(Model, DOCMassInGroundwater,
		return
			  RESULT(DOCMassInLowerSoilLayer) * RESULT(LowerLayerToGroundwaterFraction)
			- RESULT(DOCMassInGroundwater) * RESULT(GroundwaterToReachFraction);
	)
	
	EQUATION(Model, DICMassInGroundwater,
		return
			  RESULT(DOCMassInLowerSoilLayer) * RESULT(LowerLayerToGroundwaterFraction)
			- RESULT(DOCMassInGroundwater) * RESULT(GroundwaterToReachFraction);
	)
	
	
	EQUATION(Model, DiffuseDOCOutput,
		double docout =
			  RESULT(DOCMassInDirectRunoff) * RESULT(DirectRunoffToReachFraction)
			+ RESULT(DOCMassInUpperSoilLayer) * RESULT(UpperLayerToReachFraction)
			+ RESULT(DOCMassInLowerSoilLayer) * RESULT(LowerLayerToReachFraction)
			+ RESULT(DOCMassInGroundwater) * RESULT(GroundwaterToReachFraction);
		return PARAMETER(Percent) / 100.0 * PARAMETER(TerrestrialCatchmentArea) * docout;
	)
	
	EQUATION(Model, DiffuseDICOutput,
		double dicout =
			  RESULT(DICMassInDirectRunoff) * RESULT(DirectRunoffToReachFraction)
			+ RESULT(DICMassInUpperSoilLayer) * RESULT(UpperLayerToReachFraction)
			+ RESULT(DICMassInLowerSoilLayer) * RESULT(LowerLayerToReachFraction)
			+ RESULT(DICMassInGroundwater) * RESULT(GroundwaterToReachFraction);
		return PARAMETER(Percent) / 100.0 * PARAMETER(TerrestrialCatchmentArea) * dicout;
	)
	

	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	
	auto DOCMineralisationSelfShadingMultiplier = RegisterParameterDouble(Model, Reaches, "Aquatic DOC mineralisation self-shading multiplier", Dimensionless, 1.0);
	auto DOCMineralisationOffset                = RegisterParameterDouble(Model, Reaches, "Aquatic DOC mineralisation offset", Dimensionless, 1.0);
	auto ReachDICLossRate                       = RegisterParameterDouble(Model, Reaches, "Reach DIC loss rate", PerDay, 0.1);
	auto MicrobialMineralisationBaseRate        = RegisterParameterDouble(Model, Reaches, "Microbial mineralisation base rate", PerDay, 0.1);
	
	auto SolarRadiation = RegisterInput(Model, "Solar radiation");
	
	auto ReachSolver = GetSolverHandle(Model, "Reach solver"); //NOTE: Defined in PERSiST
	
	auto ReachDOCInput = RegisterEquation(Model, "Reach DOC input", KgPerDay);
	auto ReachDICInput = RegisterEquation(Model, "Reach DIC input", KgPerDay);
	
	auto ReachDOCOutput = RegisterEquation(Model, "Reach DOC output", KgPerDay);
	SetSolver(Model, ReachDOCOutput, ReachSolver);
	auto ReachDICOutput = RegisterEquation(Model, "Reach DIC output", KgPerDay);
	SetSolver(Model, ReachDICOutput, ReachSolver);
	auto DOCMassInReach = RegisterEquationODE(Model, "DOC mass in reach", Kg);
	SetSolver(Model, DOCMassInReach, ReachSolver);
	//SetInitialValue
	auto DICMassInReach = RegisterEquationODE(Model, "DIC mass in reach", Kg);
	SetSolver(Model, DICMassInReach, ReachSolver);
	//SetInitialValue
	auto PhotoMineralisationRate = RegisterEquation(Model, "Photomineralisation rate", KgPerDay);
	SetSolver(Model, PhotoMineralisationRate, ReachSolver);
	auto MicrobialMineralisationRate = RegisterEquation(Model, "Microbial mineralisation rate", KgPerDay);
	SetSolver(Model, MicrobialMineralisationRate, ReachSolver);
	
	
	
	auto ReachVolume = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow   = GetEquationHandle(Model, "Reach flow");
	
	
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