

// NOTE NOTE NOTE This module is in development and is not finished

static void
AddINCASedModel(inca_model *Model)
{
	//NOTE: Is designed to work with PERSiST
	
	auto Dimensionless = RegisterUnit(Model);
	auto SPerM         = RegisterUnit(Model, "s/m");
	auto SPerM2        = RegisterUnit(Model, "s/m2");
	auto M3PerSPerKm2  = RegisterUnit(Model, "m3/s/km2");
	auto KgPerM2PerKm2 = RegisterUnit(Model, "kg/m2/km2");
	auto KgPerSPerKm2  = RegisterUnit(Model, "kg/s/km2");
	auto KgPerM2PerS   = RegisterUnit(Model, "kg/m2/s");
	auto PercentU      = RegisterUnit(Model, "%");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto KgPerKm2      = RegisterUnit(Model, "kg/km2");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto SoilBoxes = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, SoilBoxes, "Direct runoff");
	
	auto Sediment = RegisterParameterGroup(Model, "Sediments", LandscapeUnits);
	
	//TODO : Find default/min/max/description for these e.g. in the INCA-P documentation
	//TODO : Find out which of these should index over LandscapeUnits (in addition to or instead of Reach)
	auto FlowErosionScalingFactor               = RegisterParameterDouble(Model, Sediment, "Flow erosion scaling factor", SPerM2, 1.0);
	auto FlowErosionDirectRunoffThreshold       = RegisterParameterDouble(Model, Sediment, "Flow erosion direct runoff threshold", M3PerSPerKm2, 1.0);
	auto FlowErosionNonlinearCoefficient        = RegisterParameterDouble(Model, Sediment, "Flow erosion nonlinear coefficient", Dimensionless, 1.0);
	auto TransportCapacityScalingFactor         = RegisterParameterDouble(Model, Sediment, "Transport capacity scaling factor", KgPerM2PerKm2, 1.0);
	auto TransportCapacityDirectRunoffThreshold = RegisterParameterDouble(Model, Sediment, "Transport capacity direct runoff threshold", M3PerSPerKm2, 1.0);
	auto TransportCapacityNonlinearCoefficient  = RegisterParameterDouble(Model, Sediment, "Transport capacity nonlinear coefficient", Dimensionless, 1.0);
	auto SplashDetachmentScalingFactor          = RegisterParameterDouble(Model, Sediment, "Splash detachment scaling factor", SPerM, 1.0);
	auto FlowErosionPotential                   = RegisterParameterDouble(Model, Sediment, "Flow erosion potential", KgPerSPerKm2, 0.074);
	auto SplashDetachmentSoilErodibility        = RegisterParameterDouble(Model, Sediment, "Splash detachment soil erodibility", KgPerM2PerS, 1.0);
	auto VegetationIndex                        = RegisterParameterDouble(Model, Sediment, "Vegetation index", Dimensionless, 1.0);
	
	auto InitialSurfaceSedimentStore            = RegisterParameterDouble(Model, Sediment, "Initial surface sediment store", KgPerKm2, 100.0);
	
	
	auto SubcatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length");
	auto Percent          = GetParameterDoubleHandle(Model, "%");
	
	auto Rainfall = GetEquationHandle(Model, "Rainfall");
	auto RunoffToReach = GetEquationHandle(Model, "Runoff to reach");
	
	//auto ReachSolver = GetSolverHandle(Model, "Reach solver");
	
	auto IncaSolver = RegisterSolver(Model, "Inca solver", 0.1, IncaDascru);
	
	
	auto SedimentMobilisedViaSplashDetachment = RegisterEquation(Model, "Sediment mobilised via splash detachment", KgPerKm2PerDay);
	auto SedimentMobilisedViaFlowErosion      = RegisterEquation(Model, "Sediment mobilised via flow erosion", KgPerKm2PerDay);
	auto FlowErosionKFactor                   = RegisterEquation(Model, "Flow erosion K factor", KgPerKm2PerDay);
	auto SedimentTransportCapacity            = RegisterEquation(Model, "Sediment transport capacity", KgPerKm2PerDay);
	
	auto SurfaceSedimentStore   = RegisterEquationODE(Model, "Surface sediment store", KgPerKm2);
	SetSolver(Model, SurfaceSedimentStore, IncaSolver);
	SetInitialValue(Model, SurfaceSedimentStore, InitialSurfaceSedimentStore);
	
	auto SedimentDeliveryToReach              = RegisterEquation(Model, "Sediment delivery to reach", KgPerKm2PerDay);
	SetSolver(Model, SedimentDeliveryToReach, IncaSolver);
	auto AreaScaledSedimentDeliveryToReach    = RegisterEquation(Model, "Area scaled sediment delivery to reach", KgPerDay);
	
	auto TotalSedimentDeliveryToReach         = RegisterEquationCumulative(Model, "Total sediment delivery to reach", AreaScaledSedimentDeliveryToReach, LandscapeUnits);
	
	
	//TODO: Documentation says this should use Reffq = "effective precipitation". Is that the same as rainfall?
	EQUATION(Model, SedimentMobilisedViaSplashDetachment,
		double Reffq = RESULT(Rainfall) / 86.4;
		return 86400.0 * PARAMETER(SplashDetachmentScalingFactor) * Reffq * pow(PARAMETER(SplashDetachmentSoilErodibility), 10.0 / (10.0 - PARAMETER(VegetationIndex)));
	)
	
	EQUATION(Model, FlowErosionKFactor,
		double qquick = RESULT(RunoffToReach, DirectRunoff) / 86.4;
		double flow = Max(0.0, qquick - PARAMETER(FlowErosionDirectRunoffThreshold));
		return 86400.0 * PARAMETER(FlowErosionScalingFactor) * PARAMETER(FlowErosionPotential) 
			* pow((PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(FlowErosionNonlinearCoefficient));
	)
	
	EQUATION(Model, SedimentTransportCapacity,
		double qquick = RESULT(RunoffToReach, DirectRunoff) / 86.4;
		double flow = Max(0.0, qquick - PARAMETER(TransportCapacityDirectRunoffThreshold));
		return 86400.0 * PARAMETER(TransportCapacityScalingFactor) 
			* pow((PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(TransportCapacityNonlinearCoefficient));
	)
	
	EQUATION(Model, SedimentMobilisedViaFlowErosion,
		double SFE = RESULT(FlowErosionKFactor) * (RESULT(SedimentTransportCapacity) - RESULT(SedimentMobilisedViaSplashDetachment)) / (RESULT(SedimentTransportCapacity) + RESULT(FlowErosionKFactor));
		if(RESULT(SedimentTransportCapacity) + RESULT(FlowErosionKFactor) == 0) return 0.0;
		return SFE;
	)
	
	EQUATION(Model, SedimentDeliveryToReach,
		double SSD = RESULT(SedimentMobilisedViaSplashDetachment);
		double SFE = RESULT(SedimentMobilisedViaFlowErosion);
		double STC = RESULT(SedimentTransportCapacity);
		if(RESULT(SurfaceSedimentStore) > 0 || SSD  > STC)
		{
			return STC;
		}
		return SSD + SFE;
	)
	
	EQUATION(Model, AreaScaledSedimentDeliveryToReach,
		return PARAMETER(SubcatchmentArea) * 1e6 * PARAMETER(Percent) / 100.0 * RESULT(SedimentDeliveryToReach);
	)
	
	EQUATION(Model, SurfaceSedimentStore,
		double SSD = RESULT(SedimentMobilisedViaSplashDetachment);
		double STC = RESULT(SedimentTransportCapacity);
		double Mland = RESULT(SedimentDeliveryToReach);
		if(RESULT(SurfaceSedimentStore) > 0 || SSD > STC)
		{
			return SSD - Mland;
		}
		return 0.0;
	)
	
	
	///////////////// IN - STREAM ////////////////////////////////
	
	auto SizeClass = RegisterIndexSet(Model, "Sediment size class");
	
	RequireIndex(Model, SizeClass, "Clay");
	RequireIndex(Model, SizeClass, "Silt");
	RequireIndex(Model, SizeClass, "Fine sand");
	RequireIndex(Model, SizeClass, "Medium sand");
	RequireIndex(Model, SizeClass, "Coarse sand");
	
	auto SedimentSizeClass = RegisterParameterGroup(Model, "Sediment size class", SizeClass);
	SetParentGroup(Model, SedimentSizeClass, Sediment);
	
	auto PercentageOfSedimentInGrainSizeClass   = RegisterParameterDouble(Model, SedimentSizeClass, "Percentage of sediment in grain size class", PercentU, 0.2);
}


