

// NOTE NOTE NOTE This module is in development and is not finished

static void
AddINCASedModel(inca_model *Model)
{
	//NOTE: Is designed to work with PERSiST
	
	auto Dimensionless = RegisterUnit(Model);
	auto SPerM         = RegisterUnit(Model, "s/m");
	auto SPerM2        = RegisterUnit(Model, "s/m2");
	auto M3PerSPerKm2  = RegisterUnit(Model, "m3/s/km2")
	auto KgPerM2PerKm2 = RegisterUnit(Model, "kg/m2/km2");
	auto KgPerSPerKm2  = RegisterUnit(Model, "kg/s/km2");
	auto KgPerM2PerS   = RegisterUnit(Model, "kg/m2/s");
	auto PercentU      = RegisterUnit(Model, "%");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	auto KgPerKm2      = RegisterUnit(Model, "kg/km2");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	
	auto Sediment = RegisterParameterGroup(Model, "Sediments", Reach);
	
	auto SizeClass = RegisterIndexSet(Model, "Sediment size class");
	
	RequireIndex(Model, SizeClass, "Clay");
	RequireIndex(Model, SizeClass, "Silt");
	RequireIndex(Model, SizeClass, "Fine sand");
	RequireIndex(Model, SizeClass, "Medium sand");
	RequireIndex(Model, SizeClass, "Coarse sand");
	
	auto SedimentSizeClass = RegisterParameterGroup(Model, "Sediment size class", SizeClass);
	SetParentGroup(Model, SedimentSizeClass, Sediment);
	
	//TODO : Find default/min/max/description for these e.g. in the INCA-P documentation
	auto FlowErosionScalingFactor               = RegisterParameterDouble(Model, Sediment, "Flow erosion scaling factor", SPerM2, 1.0);
	auto FlowErosionDirectRunoffThreshold       = RegisterParameterDouble(Model, Sediment, "Flow erosion direct runoff threshold", M3PerSPerKm2, 1.0);
	auto FlowErosionNonlinearCoefficient        = RegisterParameterDouble(Model, Sediment, "Flow erosion nonlinear coefficient", Dimensionless, 1.0);
	auto TransportCapacityScalingFactor         = RegisterParameterDouble(Model, Sediment, "Transport capacity scaling factor", KgPerM2PerKm2, 1.0);
	auto TransportCapacityDirectRunoffThreshold = RegisterParameterDouble(Model, Sediment, "Transport capacity direct runoff threshold", M3PerSPerKm2, 1.0);
	auto TransportCapacityNonlinearCoefficient  = RegisterParameterDouble(Model, Sediment, "Transport capacity nonlinear coefficient", Dimensionless, 1.0);
	auto SplashDetachmentScalingFactor          = RegisterParameterDouble(Model, Sediment, "Splash detachment scaling factor", SPerM, 1.0);
	auto FlowErosionPotential                   = RegisterParameterDouble(Model, Sediment, "Flow erosion potential", KgPerSPerKm2, 1.0);
	auto SplashDetachmentSoilErodibility        = RegisterParameterDouble(Model, Sediment, "Splash detachment soil erodibility", KgPerM2PerS, 1.0);
	
	auto PercentageOfSedimentInGrainSizeClass   = RegisterParameterDouble(Model, SedimentSizeClass, "Percentage of sediment in grain size class", PercentU, 0.2);
	
	//Should this index over landscape units too?
	auto VegetationIndex = RegisterParameterDouble(Model, Sediment, "Vegetation index", Dimensionless, 1.0);
	
	
	auto SubcatchmentArea = GetParameterDoubleHandle(Model, "Subcatchment area");
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length");
	
	auto Rainfall = GetEquationHandle(Model, "Rainfall");
	
	auto ReachSolver = GetSolverHandle(Model, "Reach solver");
	
	
	auto SedimentMobilisedViaSplashDetachment = RegisterEquation(Model, "Sediment mobilised via splash detachment", KgPerKm2PerDay);
	auto SedimentMobilisedViaSoilErosion      = RegisterEquation(Model, "Sediment mobilised via soil erosion", KgPerKm2PerDay);
	auto FlowErosionKFactor                   = RegisterEquation(Model, "Flow erosion K factor", KgPerKm2PerDay);
	auto SedimentTransportCapacity            = RegisterEquation(Model, "Sediment transport capacity", KgPerKm2PerDay);
	
	auto SurfaceSedimentStore   = RegisterEquationODE(Model, "Surface sediment store", KgPerKm2);
	SetSolver(Model, SurfaceSedimentStore, ReachSolver);
	//SetInitialValue
	
	auto SedimentDeliveryToReach              = RegisterEquation(Model, "Sediment delivery to reach", KgPerKm2PerDay);
	
	
	//TODO: Documentation says this should use "effective precipitation". Is that the same as rainfall?
	EQUATION(Model, SedimentMobilisedViaSplashDetachment,
		double Reffq = (1e3/86400.0)*RESULT(Rainfall);
		return 86400.0 * PARAMETER(SplashDetachmentScalingFactor) * Reffq * pow(PARAMETER(SplashDetachmentSoilErodibility), 10.0 / (10.0 - PARAMETER(VegetationIndex)));
	)
	
	EQUATION(Model, FlowErosionKFactor,
		double qquick = ;
		return 86400.0 * PARAMETER(FlowErosionScalingFactor) * PARAMETER(FlowErosionPotential) 
			* pow((PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*(qquick - PARAMETER(FlowErosionDirectRunoffThreshold)), PARAMETER(FlowErosionNonlinearCoefficient));
	)
	
	EQUATION(Model, SedimentTransportCapacity,
		double qquick = ;
		return 86400.0 * PARAMETER(TransportCapacityScalingFactor) 
			* pow((PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*(qquick - PARAMETER(TransportCapacityDirectRunoffThreshold)), PARAMETER(TransportCapacityNonlinearCoefficient));
	)
	
	EQUATION(Model, SedimentMobilisedViaSoilErosion,
		return RESULT(FlowErosionKFactor) * (RESULT(SedimentTransportCapacity) - RESULT(SedimentMobilisedViaSplashDetachment)) / (RESULT(SedimentTransportCapacity) + RESULT(FlowErosionKFactor));
	)
	
	EQUATION(Model, SedimentDeliveryToReach,
		//if(RESULT(SurfaceSedimentStore) > 0 || ()
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
}


