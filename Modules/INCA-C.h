

//NOTE NOTE NOTE: This is still in development and is not finished.

static void
AddINCACModel(inca_model *Model)
{
	//NOTE: Uses PERSiST, SoilTemperature
	
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
	auto OrganicLayer = RequireIndex(Model, Soils, "Organic layer");
	auto MineralLayer = RequireIndex(Model, Soils, "Mineral layer");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	auto SO4SoilSolution  = RegisterInput(Model, "SO4 soil solution");
	
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	
	//TODO: As always, find better default values, min/max, description..
	auto MinRateDepth                 = RegisterParameterDouble(Model, Land, "Min rate depth", Mm, 50.0, 0.0, 10000.0, "The water depth at which carbon processes in the soil are at their lowest rate");
	auto MaxRateDepth                 = RegisterParameterDouble(Model, Land, "Max rate depth", Mm, 400.0, 0.0, 10000.0, "The water depth at which carbon processes in the soil are at their highest rate");
	auto MinimalMoistureFactor        = RegisterParameterDouble(Model, Land, "Min moisture factor", Dimensionless, 0.05, 0.0, 1.0, "The rate factor from moisture on carbon processes when soil moisture is at its minimal");
	
	auto LitterFallRate                = RegisterParameterDouble(Model, Land, "Litter fall rate", GPerM2PerDay, 1.0);
	auto LitterFallStartDay            = RegisterParameterUInt(  Model, Land, "Litter fall start day", JulianDay, 300, 1, 364);
	auto LitterFallDuration            = RegisterParameterUInt(  Model, Land, "Litter fall duration", Days, 30, 0, 365);
	//auto RootBreakdownRate             = RegisterParameterDouble(Model, Land, "Root breakdown rate", GPerM2PerDay, 1.0);
	
	auto FastPoolEquilibriumFractionOrganicLayer = RegisterParameterDouble(Model, Land, "SOC fast pool equilibrium fraction in the organic layer", Dimensionless, 0.5, 0.0, 1.0, "The size that the fast SOC pool tends towards.");
	auto FastPoolEquilibriumFractionMineralLayer = RegisterParameterDouble(Model, Land, "SOC fast pool equilibrium fraction in the mineral layer", Dimensionless, 0.5, 0.0, 1.0, "The size that the fast SOC pool tends towards.");
	auto FastPoolRateConstantOrganicLayer = RegisterParameterDouble(Model, Land, "SOC fast pool rate constant in the organic layer", Dimensionless, 50.0, 0.0, 5000.0, "Constant used to determine the rate of exchange between the fast and slow pools of SOC.");
	auto FastPoolRateConstantMineralLayer = RegisterParameterDouble(Model, Land, "SOC fast pool rate constant in the mineral layer", Dimensionless, 50.0, 0.0, 5000.0, "Constant used to determine the rate of exchange between the fast and slow pools of SOC.");
	
	auto SoilTemperatureRateMultiplier = RegisterParameterDouble(Model, Land, "Soil temperature rate multiplier", Dimensionless, 1.0);
	auto SoilTemperatureRateOffset     = RegisterParameterDouble(Model, Land, "Soil temperature rate offset", DegreesCelsius, 20.0);
	
	auto DICMassTransferVelocity               = RegisterParameterDouble(Model, Land, "DIC mass transfer velocity", MPerDay, 1.0);
	auto DICSaturationConstant                 = RegisterParameterDouble(Model, Land, "DIC saturation constant", KgPerM3, 1.0);
	auto SOCMineralisationBaseRateOrganicLayer = RegisterParameterDouble(Model, Land, "SOC mineralisation base rate in organic soil layer", PerDay, 0.1);
	auto SOCMineralisationBaseRateMineralLayer = RegisterParameterDouble(Model, Land, "SOC mineralisation base rate in mineral soil layer", PerDay, 0.1);
	auto SOCDesorptionBaseRateOrganicLayer     = RegisterParameterDouble(Model, Land, "SOC desorption base rate in organic soil layer", PerDay, 0.1);
	auto SOCDesorptionBaseRateMineralLayer     = RegisterParameterDouble(Model, Land, "SOC desorption base rate in mineral soil layer", PerDay, 0.1);
	auto DOCSorptionBaseRateOrganicLayer       = RegisterParameterDouble(Model, Land, "DOC sorption base rate in organic soil layer", PerDay, 0.1);
	auto DOCSorptionBaseRateMineralLayer       = RegisterParameterDouble(Model, Land, "DOC sorption base rate in mineral soil layer", PerDay, 0.1);
	auto DOCMineralisationBaseRateOrganicLayer = RegisterParameterDouble(Model, Land, "DOC mineralisation base rate in organic soil layer", PerDay, 0.1);
	auto DOCMineralisationBaseRateMineralLayer = RegisterParameterDouble(Model, Land, "DOC mineralisation base rate in mineral soil layer", PerDay, 0.1);
	
	auto DOCMineralisationRateGroundwater      = RegisterParameterDouble(Model, Land, "DOC mineralisation rate in groundwater", PerDay, 0.1);
	
	auto LinearEffectOfSO4OnSolubilityOrganicLayer = RegisterParameterDouble(Model, Land, "Linear effect of SO4 on organic matter solubility in organic soil layer", PerDay, 0.1); //TODO: This is actually a different unit, but it depends on the unit of the SO4 timeseries, which could be user specified..
	auto LinearEffectOfSO4OnSolubilityMineralLayer = RegisterParameterDouble(Model, Land, "Linear effect of SO4 on organic matter solubility in mineral soil layer", PerDay, 0.1); //TODO: This is actually a different unit, but it depends on the unit of the SO4 timeseries, which could be user specified..
	auto ExponentialEffectOfSO4OnSolubilityOrganicLayer = RegisterParameterDouble(Model, Land, "Exponential effect of SO4 on organic matter solubility in organic soil layer", Dimensionless, 1);
	auto ExponentialEffectOfSO4OnSolubilityMineralLayer = RegisterParameterDouble(Model, Land, "Exponential effect of SO4 on organic matter solubility in mineral soil layer", Dimensionless, 1);
	
	auto IncaSolver = RegisterSolver(Model, "INCA Solver", 0.1, IncaDascru);
	
	auto TemperatureRateModifier            = RegisterEquation(Model, "Temperature rate modifier", Dimensionless);
	auto MoistureRateModifier               = RegisterEquation(Model, "Moisture rate modifier", Dimensionless);
	auto RateModifierOrganicLayer           = RegisterEquation(Model, "Rate modifier in organic layer", Dimensionless);
	auto RateModifierMineralLayer           = RegisterEquation(Model, "Rate modifier in mineral layer", Dimensionless);
	
	auto LitterFall                         = RegisterEquation(Model, "Litter fall", GPerM2PerDay);
	
	auto DirectRunoffToReachFraction        = RegisterEquation(Model, "Direct runoff to reach fraction", PerDay);
	auto DirectRunoffToOrganicLayerFraction = RegisterEquation(Model, "Direct runoff to organic layer fraction", PerDay);
	auto OrganicLayerToDirectRunoffFraction = RegisterEquation(Model, "Organic layer to direct runoff fraction", PerDay);
	auto OrganicLayerToMineralLayerFraction = RegisterEquation(Model, "Organic layer to mineral layer fraction", PerDay);
	auto OrganicLayerToReachFraction        = RegisterEquation(Model, "Organic layer to reach fraction", PerDay);
	auto MineralLayerToReachFraction        = RegisterEquation(Model, "Mineral layer to reach fraction", PerDay);
	auto MineralLayerToGroundwaterFraction  = RegisterEquation(Model, "Mineral layer to groundwater fraction", PerDay);
	auto GroundwaterToReachFraction         = RegisterEquation(Model, "Groundwater to reach fraction", PerDay);
	
	auto SOCMineralisationInOrganicLayer = RegisterEquation(Model, "SOC mineralisation in organic soil layer", PerDay);
	SetSolver(Model, SOCMineralisationInOrganicLayer, IncaSolver);
	auto SOCMineralisationInMineralLayer = RegisterEquation(Model, "SOC mineralisation in mineral soil layer", KgPerDay);
	SetSolver(Model, SOCMineralisationInMineralLayer, IncaSolver);
	auto SOCDesorptionInOrganicLayer     = RegisterEquation(Model, "SOC desorption in organic soil layer", PerDay);
	SetSolver(Model, SOCDesorptionInOrganicLayer, IncaSolver);
	auto SOCDesorptionInMineralLayer     = RegisterEquation(Model, "SOC desorption in mineral soil layer", KgPerDay);
	SetSolver(Model, SOCDesorptionInMineralLayer, IncaSolver);
	auto DOCSorptionInOrganicLayer       = RegisterEquation(Model, "DOC sorption in organic soil layer", KgPerDay);
	SetSolver(Model, DOCSorptionInOrganicLayer, IncaSolver);
	auto DOCSorptionInMineralLayer       = RegisterEquation(Model, "DOC sorption in mineral soil layer", KgPerDay);
	SetSolver(Model, DOCSorptionInMineralLayer, IncaSolver);
	auto DOCMineralisationInOrganicLayer = RegisterEquation(Model, "DOC mineralisation in organic soil layer", KgPerDay);
	SetSolver(Model, DOCMineralisationInOrganicLayer, IncaSolver);
	auto DOCMineralisationInMineralLayer = RegisterEquation(Model, "DOC mineralisation in mineral soil layer", KgPerDay);
	SetSolver(Model, DOCMineralisationInMineralLayer, IncaSolver);
	auto DICMassTransferToAtmosphere     = RegisterEquation(Model, "DIC mass transfer to atmosphere", KgPerDay);
	SetSolver(Model, DICMassTransferToAtmosphere, IncaSolver);
	auto DOCMineralisationInGroundwater  = RegisterEquation(Model, "DOC mineralisation in groundwater", KgPerDay);
	SetSolver(Model, DOCMineralisationInGroundwater, IncaSolver);
	
	auto DOCMassInDirectRunoff = RegisterEquationODE(Model, "DOC mass in direct runoff", KgPerKm2);
	SetSolver(Model, DOCMassInDirectRunoff, IncaSolver);
	//SetInitialValue()
	
	auto DICMassInDirectRunoff = RegisterEquationODE(Model, "DIC mass in direct runoff", KgPerKm2);
	SetSolver(Model, DICMassInDirectRunoff, IncaSolver);
	//SetInitialValue()
	
	auto SOCMassInOrganicLayerFastPool = RegisterEquationODE(Model, "SOC mass in organic soil layer fast pool", KgPerKm2);
	SetSolver(Model, SOCMassInOrganicLayerFastPool, IncaSolver);
	SetInitialValue(Model, SOCMassInOrganicLayerFastPool, 1.0);
	
	auto SOCMassInOrganicLayerSlowPool = RegisterEquationODE(Model, "SOC mass in organic soil layer slow pool", KgPerKm2);
	SetSolver(Model, SOCMassInOrganicLayerSlowPool, IncaSolver);
	//SetInitialValue()
	
	auto SOCFastFractionInOrganicLayer = RegisterEquation(Model, "Fraction of mass in SOC fast pool in organic layer", Dimensionless);
	SetSolver(Model, SOCFastFractionInOrganicLayer, IncaSolver);
	
	auto MassTransferFromSlowToFastInOrganicLayer = RegisterEquation(Model, "Mass transfer from slow to fast SOC pool in organic layer", KgPerDay);
	SetSolver(Model, MassTransferFromSlowToFastInOrganicLayer, IncaSolver);
	
	auto DOCMassInOrganicLayer = RegisterEquationODE(Model, "DOC mass in organic soil layer", KgPerKm2);
	SetSolver(Model, DOCMassInOrganicLayer, IncaSolver);
	//SetInitialValue()
	
	auto DICMassInOrganicLayer = RegisterEquationODE(Model, "DIC mass in organic soil layer", KgPerKm2);
	SetSolver(Model, DICMassInOrganicLayer, IncaSolver);
	//SetInitialValue()
	
	auto SOCMassInMineralLayerFastPool = RegisterEquationODE(Model, "SOC mass in mineral soil layer fast pool", KgPerKm2);
	SetSolver(Model, SOCMassInMineralLayerFastPool, IncaSolver);
	SetInitialValue(Model, SOCMassInMineralLayerFastPool, 1.0);
	
	auto SOCMassInMineralLayerSlowPool = RegisterEquationODE(Model, "SOC mass in mineral soil layer slow pool", KgPerKm2);
	SetSolver(Model, SOCMassInMineralLayerSlowPool, IncaSolver);
	//SetInitialValue()
	
	auto SOCFastFractionInMineralLayer = RegisterEquation(Model, "Fraction of mass in SOC fast pool in mineral layer", Dimensionless);
	SetSolver(Model, SOCFastFractionInMineralLayer, IncaSolver);
	
	auto MassTransferFromSlowToFastInMineralLayer = RegisterEquation(Model, "Mass transfer from slow to fast SOC pool in mineral layer", KgPerDay);
	SetSolver(Model, MassTransferFromSlowToFastInMineralLayer, IncaSolver);
	
	auto DOCMassInMineralLayer = RegisterEquationODE(Model, "DOC mass in mineral soil layer", KgPerKm2);
	SetSolver(Model, DOCMassInMineralLayer, IncaSolver);
	//SetInitialValue()
	
	auto DICMassInMineralLayer = RegisterEquationODE(Model, "DIC mass in mineral soil layer", KgPerKm2);
	SetSolver(Model, DICMassInMineralLayer, IncaSolver);
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
	auto WaterDepth3           = GetEquationHandle(Model, "Water depth 3"); //NOTE: This is right before percolation and runoff is subtracted.
	auto RunoffToReach         = GetEquationHandle(Model, "Runoff to reach");
	auto SaturationExcessInput = GetEquationHandle(Model, "Saturation excess input");
	auto PercolationInput      = GetEquationHandle(Model, "Percolation input");
	
	
	
	EQUATION(Model, LitterFall,
		//TODO: Should be possible to provide this as input timeseries
	
		double litter = PARAMETER(LitterFallRate);
		u64 start     = PARAMETER(LitterFallStartDay);
		u64 duration  = PARAMETER(LitterFallDuration);
		if( (CURRENT_DAY_OF_YEAR() < start) || (CURRENT_DAY_OF_YEAR() >= start + duration))
		{
			litter = 0.0;
		}
	
		return litter;
	)
	
	EQUATION(Model, TemperatureRateModifier,
		return pow(PARAMETER(SoilTemperatureRateMultiplier), RESULT(SoilTemperature) - PARAMETER(SoilTemperatureRateOffset));
	)
	
	EQUATION(Model, MoistureRateModifier,
		return InverseGammaResponse(RESULT(WaterDepth), PARAMETER(MinRateDepth), PARAMETER(MaxRateDepth), PARAMETER(MinimalMoistureFactor), 1.0);
	)
	
	EQUATION(Model, RateModifierOrganicLayer,
		return RESULT(TemperatureRateModifier) * RESULT(MoistureRateModifier, OrganicLayer);
	)
	
	EQUATION(Model, RateModifierMineralLayer,
		return RESULT(TemperatureRateModifier) * RESULT(MoistureRateModifier, MineralLayer);
	)
	
	
	//NOTE: The following equations make a lot of assumptions about where you can and can not have percolation or infiltration excess!
	
	EQUATION(Model, DirectRunoffToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, DirectRunoff), RESULT(WaterDepth3, DirectRunoff));
	)
	
	EQUATION(Model, DirectRunoffToOrganicLayerFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(PercolationInput, OrganicLayer), RESULT(WaterDepth3, DirectRunoff));
	)
	
	EQUATION(Model, OrganicLayerToDirectRunoffFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(SaturationExcessInput, DirectRunoff), RESULT(WaterDepth3, OrganicLayer));
	)
	
	EQUATION(Model, OrganicLayerToMineralLayerFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(PercolationInput, MineralLayer), RESULT(WaterDepth3, OrganicLayer));
	)
	
	EQUATION(Model, OrganicLayerToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, OrganicLayer), RESULT(WaterDepth3, OrganicLayer));
	)
	
	EQUATION(Model, MineralLayerToGroundwaterFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(PercolationInput, Groundwater), RESULT(WaterDepth3, MineralLayer));
	)
	
	EQUATION(Model, MineralLayerToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, MineralLayer), RESULT(WaterDepth3, MineralLayer));
	)

	EQUATION(Model, GroundwaterToReachFraction,
		CURRENT_INDEX(Reach); CURRENT_INDEX(LandscapeUnits);
		return SafeDivide(RESULT(RunoffToReach, Groundwater), RESULT(WaterDepth3, Groundwater));
	)
	
	
	EQUATION(Model, SOCMineralisationInOrganicLayer,
		return 
			  RESULT(RateModifierOrganicLayer)
			* PARAMETER(SOCMineralisationBaseRateOrganicLayer)
			* RESULT(SOCMassInOrganicLayerFastPool);
			//TODO: Also mineralisation in slow pool?
	)
	
	EQUATION(Model, SOCDesorptionInOrganicLayer,
		return
			  RESULT(RateModifierOrganicLayer)
			* (PARAMETER(SOCDesorptionBaseRateOrganicLayer) + PARAMETER(LinearEffectOfSO4OnSolubilityOrganicLayer)*pow(INPUT(SO4SoilSolution), PARAMETER(ExponentialEffectOfSO4OnSolubilityOrganicLayer)))
			* RESULT(SOCFastFractionInOrganicLayer)
			* RESULT(SOCMassInOrganicLayerFastPool);
	)
	
	EQUATION(Model, DOCSorptionInOrganicLayer,
		return 
			  RESULT(RateModifierOrganicLayer)
			* PARAMETER(DOCSorptionBaseRateOrganicLayer)
			* RESULT(DOCMassInOrganicLayer);
	)
	
	EQUATION(Model, DOCMineralisationInOrganicLayer,
		return 
			  RESULT(RateModifierOrganicLayer)
		    * PARAMETER(DOCMineralisationBaseRateOrganicLayer)
			* RESULT(DOCMassInOrganicLayer);
	)
	
	EQUATION(Model, SOCMineralisationInMineralLayer,
		return
			  RESULT(RateModifierOrganicLayer)
			* PARAMETER(SOCMineralisationBaseRateMineralLayer)
			* RESULT(SOCMassInMineralLayerFastPool);
			//TODO: Also mineralisation in slow pool?
	)
	
	EQUATION(Model, SOCDesorptionInMineralLayer,
		return
			  RESULT(RateModifierOrganicLayer)
			* (PARAMETER(SOCDesorptionBaseRateMineralLayer) + PARAMETER(LinearEffectOfSO4OnSolubilityMineralLayer)*pow(INPUT(SO4SoilSolution), PARAMETER(ExponentialEffectOfSO4OnSolubilityMineralLayer)))
			* RESULT(SOCFastFractionInMineralLayer)
			* RESULT(SOCMassInMineralLayerFastPool);
	)
	
	EQUATION(Model, DOCSorptionInMineralLayer,
		return
			  RESULT(RateModifierOrganicLayer)
			* PARAMETER(DOCSorptionBaseRateMineralLayer)
			* RESULT(DOCMassInMineralLayer);
	)
	
	EQUATION(Model, DOCMineralisationInMineralLayer,
		return
			  RESULT(RateModifierOrganicLayer)
			* PARAMETER(DOCMineralisationBaseRateMineralLayer)
			* RESULT(DOCMassInMineralLayer);
	)
	
	EQUATION(Model, DICMassTransferToAtmosphere,
		double organiclayervolume = RESULT(WaterDepth, OrganicLayer) / 1000.0 * PARAMETER(TerrestrialCatchmentArea) * 1e6 * PARAMETER(Percent) * 1e-2;  //TODO: Check if unit conversion is correct
		return PARAMETER(DICMassTransferVelocity) * (SafeDivide(RESULT(DICMassInOrganicLayer), organiclayervolume) - PARAMETER(DICSaturationConstant)); //TODO: Check if we should allow this to be negative
	)
	
	EQUATION(Model, DOCMineralisationInGroundwater,
		return PARAMETER(DOCMineralisationRateGroundwater) * RESULT(DOCMassInGroundwater);
	)
	
	
	EQUATION(Model, DOCMassInDirectRunoff,
		return
			- RESULT(DOCMassInDirectRunoff) * (RESULT(DirectRunoffToReachFraction) + RESULT(DirectRunoffToOrganicLayerFraction))
			+ RESULT(DOCMassInOrganicLayer) * RESULT(OrganicLayerToDirectRunoffFraction);
	)
	
	EQUATION(Model, DICMassInDirectRunoff,
		return
			- RESULT(DICMassInDirectRunoff) * (RESULT(DirectRunoffToReachFraction) + RESULT(DirectRunoffToOrganicLayerFraction))
			+ RESULT(DICMassInOrganicLayer) * RESULT(OrganicLayerToDirectRunoffFraction);
	)
	
	
	
	EQUATION(Model, SOCFastFractionInOrganicLayer,
		return SafeDivide(RESULT(SOCMassInOrganicLayerFastPool), RESULT(SOCMassInOrganicLayerFastPool) + RESULT(SOCMassInOrganicLayerSlowPool));
	)
	
	EQUATION(Model, MassTransferFromSlowToFastInOrganicLayer,
		double totsoc = RESULT(SOCMassInOrganicLayerFastPool) + RESULT(SOCMassInOrganicLayerSlowPool);
		double fractiondiff = (PARAMETER(FastPoolEquilibriumFractionOrganicLayer) - RESULT(SOCFastFractionInOrganicLayer));
		
		return PARAMETER(FastPoolRateConstantOrganicLayer) * fractiondiff * totsoc;
	)
	
	EQUATION(Model, SOCMassInOrganicLayerFastPool,
		return 
			  1000.0 * RESULT(LitterFall) * RESULT(SOCFastFractionInOrganicLayer)
			+ RESULT(MassTransferFromSlowToFastInOrganicLayer)
			+ RESULT(DOCSorptionInOrganicLayer)
			- RESULT(SOCDesorptionInOrganicLayer)
			- RESULT(SOCMineralisationInOrganicLayer);
	)
	
	EQUATION(Model, SOCMassInOrganicLayerSlowPool,
		return 
			  1000.0 * RESULT(LitterFall) * (1.0 - RESULT(SOCFastFractionInOrganicLayer))
			  - RESULT(MassTransferFromSlowToFastInOrganicLayer);
	)
	
	EQUATION(Model, DOCMassInOrganicLayer,
		return
			  RESULT(SOCDesorptionInOrganicLayer)
			- RESULT(DOCSorptionInOrganicLayer)
			- RESULT(DOCMineralisationInOrganicLayer)
			
			- RESULT(DOCMassInOrganicLayer) * (RESULT(OrganicLayerToDirectRunoffFraction) + RESULT(OrganicLayerToReachFraction) + RESULT(OrganicLayerToMineralLayerFraction))
			+ RESULT(DOCMassInDirectRunoff) * RESULT(DirectRunoffToOrganicLayerFraction);
	)
	
	EQUATION(Model, DICMassInOrganicLayer,
		return
			  RESULT(SOCMineralisationInOrganicLayer)
			+ RESULT(DOCMineralisationInOrganicLayer)
			- RESULT(DICMassTransferToAtmosphere)
			
			- RESULT(DICMassInOrganicLayer) * (RESULT(OrganicLayerToDirectRunoffFraction) + RESULT(OrganicLayerToReachFraction) + RESULT(OrganicLayerToMineralLayerFraction))
			+ RESULT(DICMassInDirectRunoff) * RESULT(DirectRunoffToOrganicLayerFraction);
	)
	
	
	
	EQUATION(Model, SOCFastFractionInMineralLayer,
		return SafeDivide(RESULT(SOCMassInMineralLayerFastPool), RESULT(SOCMassInMineralLayerFastPool) + RESULT(SOCMassInMineralLayerSlowPool));
	)
	
	EQUATION(Model, MassTransferFromSlowToFastInMineralLayer,
		double totsoc = RESULT(SOCMassInMineralLayerFastPool) + RESULT(SOCMassInMineralLayerSlowPool);
		double fractiondiff = (PARAMETER(FastPoolEquilibriumFractionMineralLayer) - RESULT(SOCFastFractionInMineralLayer));
		
		return PARAMETER(FastPoolRateConstantMineralLayer) * fractiondiff * totsoc;
	)
	
	EQUATION(Model, SOCMassInMineralLayerFastPool,
		return 
			+ RESULT(MassTransferFromSlowToFastInMineralLayer)
			+ RESULT(DOCSorptionInMineralLayer)
			- RESULT(SOCDesorptionInMineralLayer)
			- RESULT(SOCMineralisationInMineralLayer);
	)
	
	EQUATION(Model, SOCMassInMineralLayerSlowPool,
		return
			  - RESULT(MassTransferFromSlowToFastInMineralLayer);
	)
	
	EQUATION(Model, DOCMassInMineralLayer,
		return
			  RESULT(SOCDesorptionInMineralLayer)
			- RESULT(DOCSorptionInMineralLayer)
			- RESULT(DOCMineralisationInMineralLayer)
			
			+ RESULT(DOCMassInOrganicLayer) * RESULT(OrganicLayerToMineralLayerFraction)
			- RESULT(DOCMassInMineralLayer) * (RESULT(MineralLayerToReachFraction) + RESULT(MineralLayerToGroundwaterFraction));
	)
	
	EQUATION(Model, DICMassInMineralLayer,
		return
			  RESULT(SOCMineralisationInMineralLayer)
			+ RESULT(DOCMineralisationInMineralLayer)
			
			+ RESULT(DICMassInOrganicLayer) * RESULT(OrganicLayerToMineralLayerFraction)
			- RESULT(DICMassInMineralLayer) * (RESULT(MineralLayerToReachFraction) + RESULT(MineralLayerToGroundwaterFraction));
	)
	
	EQUATION(Model, DOCMassInGroundwater,
		return
			- RESULT(DOCMineralisationInGroundwater)
			
			+ RESULT(DOCMassInMineralLayer) * RESULT(MineralLayerToGroundwaterFraction)
			- RESULT(DOCMassInGroundwater) * RESULT(GroundwaterToReachFraction);
	)
	
	EQUATION(Model, DICMassInGroundwater,
		return
			  RESULT(DOCMineralisationInGroundwater)
			  
			+ RESULT(DICMassInMineralLayer) * RESULT(MineralLayerToGroundwaterFraction)
			- RESULT(DICMassInGroundwater) * RESULT(GroundwaterToReachFraction);
	)
	
	
	EQUATION(Model, DiffuseDOCOutput,
		double docout =
			  RESULT(DOCMassInDirectRunoff) * RESULT(DirectRunoffToReachFraction)
			+ RESULT(DOCMassInOrganicLayer) * RESULT(OrganicLayerToReachFraction)
			+ RESULT(DOCMassInMineralLayer) * RESULT(MineralLayerToReachFraction)
			+ RESULT(DOCMassInGroundwater) * RESULT(GroundwaterToReachFraction);
		return PARAMETER(Percent) / 100.0 * PARAMETER(TerrestrialCatchmentArea) * docout;
	)
	
	EQUATION(Model, DiffuseDICOutput,
		double dicout =
			  RESULT(DICMassInDirectRunoff) * RESULT(DirectRunoffToReachFraction)
			+ RESULT(DICMassInOrganicLayer) * RESULT(OrganicLayerToReachFraction)
			+ RESULT(DICMassInMineralLayer) * RESULT(MineralLayerToReachFraction)
			+ RESULT(DICMassInGroundwater) * RESULT(GroundwaterToReachFraction);
		return PARAMETER(Percent) / 100.0 * PARAMETER(TerrestrialCatchmentArea) * dicout;
	)
	

	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	
	auto DOCMineralisationSelfShadingMultiplier = RegisterParameterDouble(Model, Reaches, "Aquatic DOC mineralisation self-shading multiplier", Dimensionless, 1.0); //TODO: Not actually dimensionless
	auto DOCMineralisationOffset                = RegisterParameterDouble(Model, Reaches, "Aquatic DOC mineralisation offset", KgPerM3, 1.0);
	auto ReachDICLossRate                       = RegisterParameterDouble(Model, Reaches, "Reach DIC loss rate", PerDay, 0.1);
	auto MicrobialMineralisationBaseRate        = RegisterParameterDouble(Model, Reaches, "Aquatic DOC microbial mineralisation base rate", PerDay, 0.1);
	
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
			SafeDivide(
			PARAMETER(DOCMineralisationSelfShadingMultiplier) * INPUT(SolarRadiation), 
			(PARAMETER(DOCMineralisationOffset) + SafeDivide(RESULT(DOCMassInReach), RESULT(ReachVolume)))
			);
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
			- PARAMETER(ReachDICLossRate) * RESULT(DICMassInReach); //TODO: Should the loss rate depend on anything??
	)
	
	
	auto ReachDOCConcentration = RegisterEquation(Model, "Reach DOC concentration", KgPerM3);
	auto ReachDICConcentration = RegisterEquation(Model, "Reach DIC concentration", KgPerM3);
	
	EQUATION(Model, ReachDOCConcentration,
		return SafeDivide(RESULT(DOCMassInReach), RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachDICConcentration,
		return SafeDivide(RESULT(DICMassInReach), RESULT(ReachVolume));
	)
	
}