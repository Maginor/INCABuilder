

// NOTE NOTE NOTE This module is in development and is not finished!!!!

//#include "../boost_solvers.h"

static void
AddINCAMicroplasticsModel(inca_model *Model)
{
	//NOTE: Is designed to work with PERSiST
	
	auto Dimensionless = RegisterUnit(Model);
	auto SPerM         = RegisterUnit(Model, "s/m");
	auto MPerS         = RegisterUnit(Model, "m/s");
	auto SPerM2        = RegisterUnit(Model, "s/m2");
	auto M3PerSPerKm2  = RegisterUnit(Model, "m3/s/km2");
	auto Kg            = RegisterUnit(Model, "kg");
	auto KgPerM3       = RegisterUnit(Model, "kg/m3");
	auto KgPerM2PerKm2 = RegisterUnit(Model, "kg/m2/km2");
	auto KgPerSPerKm2  = RegisterUnit(Model, "kg/s/km2");
	auto KgPerM2PerS   = RegisterUnit(Model, "kg/m2/s");
	auto PercentU      = RegisterUnit(Model, "%");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto KgPerKm2      = RegisterUnit(Model, "kg/km2");
	auto KgPerM2       = RegisterUnit(Model, "kg/m2");
	auto KgPerM2PerDay = RegisterUnit(Model, "kg/m2/day");
	auto JPerSPerM2    = RegisterUnit(Model, "J/s/m2");
	auto MgPerL        = RegisterUnit(Model, "mg/L");
	auto Metres        = RegisterUnit(Model, "m");
	auto KgPerM2PerM3SPerDay = RegisterUnit(Model, "kg/m2/m3 s/day");
	auto S2PerKg       = RegisterUnit(Model, "s2/kg");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto SoilBoxes = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, SoilBoxes, "Direct runoff");
	
	auto SedimentReach = RegisterParameterGroup(Model, "Sediment reach", Reach);
	
	//TODO : Find default/min/max/description for these
	
	//TODO: Some of these should probably also be per grain class
	auto FlowErosionScalingFactor               = RegisterParameterDouble(Model, SedimentReach, "Flow erosion scaling factor", SPerM2, 1.0);
	auto FlowErosionDirectRunoffThreshold       = RegisterParameterDouble(Model, SedimentReach, "Flow erosion direct runoff threshold", M3PerSPerKm2, 0.001);
	auto FlowErosionNonlinearCoefficient        = RegisterParameterDouble(Model, SedimentReach, "Flow erosion non-linear coefficient", Dimensionless, 1.0);
	auto TransportCapacityScalingFactor         = RegisterParameterDouble(Model, SedimentReach, "Transport capacity scaling factor", KgPerM2PerKm2, 1.0);
	auto TransportCapacityDirectRunoffThreshold = RegisterParameterDouble(Model, SedimentReach, "Transport capacity direct runoff threshold", M3PerSPerKm2, 0.001);
	auto TransportCapacityNonlinearCoefficient  = RegisterParameterDouble(Model, SedimentReach, "Transport capacity non-linear coefficient", Dimensionless, 1.0);
	
	
	auto SubcatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length");
	auto Percent          = GetParameterDoubleHandle(Model, "%");
	
	auto Rainfall = GetEquationHandle(Model, "Rainfall");
	auto RunoffToReach = GetEquationHandle(Model, "Runoff to reach");
	
	auto Class = RegisterIndexSet(Model, "Grain class");
	auto GrainClass = RegisterParameterGroup(Model, "Grain class", Class);
	
	auto SmallestDiameterOfClass        = RegisterParameterDouble(Model, GrainClass, "Smallest diameter of grain in class", Metres, 0.0, 0.0, 2e3);
	auto LargestDiameterOfClass         = RegisterParameterDouble(Model, GrainClass, "Largest diameter of grain in class", Metres, 2e-6);
	auto DensityOfClass                 = RegisterParameterDouble(Model, GrainClass, "Density of grain class", KgPerM3, 1000.0);
	
	auto SedimentLand = RegisterParameterGroup(Model, "Sediment land", LandscapeUnits);
	auto VegetationIndex                        = RegisterParameterDouble(Model, SedimentLand, "Vegetation index", Dimensionless, 1.0); //Could this be the same as the canopy interception though?
	
	auto Erosion = RegisterParameterGroup(Model, "Erosion", LandscapeUnits);
	SetParentGroup(Model, Erosion, GrainClass);
	auto SplashDetachmentScalingFactor          = RegisterParameterDouble(Model, Erosion, "Splash detachment scaling factor", SPerM, 0.001);
	auto FlowErosionPotential                   = RegisterParameterDouble(Model, Erosion, "Flow erosion potential", KgPerSPerKm2, 0.074);
	auto SplashDetachmentSoilErodibility        = RegisterParameterDouble(Model, Erosion, "Splash detachment soil erodibility", KgPerM2PerS, 1.0);
	
	auto Store = RegisterParameterGroup(Model, "Grain store", Reach);
	SetParentGroup(Model, Store, GrainClass);
	auto InitialSurfaceStore                    = RegisterParameterDouble(Model, Store, "Initial surface grain store", KgPerKm2, 100.0);
	auto InitialImmobileStore                   = RegisterParameterDouble(Model, Store, "Initial immobile grain store", KgPerKm2, 100.0);
	auto GrainInput                             = RegisterParameterDouble(Model, Store, "Grain input", KgPerKm2PerDay, 0.0);
	
	
	auto TransferMatrix = RegisterParameterGroup(Model, "Transfer matrix", Class);
	SetParentGroup(Model, TransferMatrix, GrainClass);
	auto LandMassTransferRateBetweenClasses = RegisterParameterDouble(Model, TransferMatrix, "Mass transfer rate between classes on land", Dimensionless, 0.0, 0.0, 1.0);
	
	///////////// Erosion and transport ////////////////
	
	auto GrainInputTimeseries = RegisterInput(Model, "Grain input", KgPerKm2PerDay);
	
	auto SurfaceTransportCapacity     = RegisterEquation(Model, "Land surface transport capacity", KgPerKm2PerDay);
	auto ImmobileGrainStoreBeforeMobilisation = RegisterEquation(Model, "Immobile grain store before mobilisation", KgPerKm2);
	auto MobilisedViaSplashDetachment = RegisterEquation(Model, "Grain mass mobilised via splash detachment", KgPerKm2PerDay);
	auto FlowErosionKFactor           = RegisterEquation(Model, "Flow erosion K factor", KgPerKm2PerDay);
	auto SurfaceGrainStoreBeforeTransport = RegisterEquation(Model, "Surface grain store before transport", KgPerKm2);
	auto TransportBeforeFlowErosion   = RegisterEquation(Model, "Transport before flow erosion", KgPerKm2PerDay);
	auto PotentiallyMobilisedViaFlowErosion = RegisterEquation(Model, "Grain mass potentially mobilised via flow erosion", KgPerKm2PerDay);
	auto MobilisedViaFlowErosion      = RegisterEquation(Model, "Grain mass mobilised via flow erosion", KgPerKm2PerDay);
	
	auto SurfaceGrainStoreAfterAllTransport = RegisterEquation(Model, "Surface grain store after transport", KgPerKm2);
	auto SurfaceGrainStore   = RegisterEquation(Model, "Surface grain store", KgPerKm2);
	SetInitialValue(Model, SurfaceGrainStore, InitialSurfaceStore);
	
	auto ImmobileGrainStoreAfterMobilisation = RegisterEquation(Model, "Immobile grain store after mobilisation", KgPerKm2);
	auto ImmobileGrainStore     = RegisterEquation(Model, "Immobile grain store", KgPerKm2);
	SetInitialValue(Model, ImmobileGrainStore, InitialImmobileStore);
	
	auto GrainDeliveryToReach              = RegisterEquation(Model, "Grain delivery to reach", KgPerKm2PerDay);

	auto AreaScaledGrainDeliveryToReach    = RegisterEquation(Model, "Area scaled grain delivery to reach", KgPerDay);
	
	auto TotalGrainDeliveryToReach         = RegisterEquationCumulative(Model, "Total grain delivery to reach", AreaScaledGrainDeliveryToReach, LandscapeUnits);
	
	EQUATION(Model, SurfaceTransportCapacity,
		double qquick = RESULT(RunoffToReach, DirectRunoff) / 86.4;
		double flow = Max(0.0, qquick - PARAMETER(TransportCapacityDirectRunoffThreshold));
		return 86400.0 * PARAMETER(TransportCapacityScalingFactor) 
			* pow((PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(TransportCapacityNonlinearCoefficient));
	)
	
	EQUATION(Model, ImmobileGrainStoreBeforeMobilisation,
		return LAST_RESULT(ImmobileGrainStore) + IF_INPUT_ELSE_PARAMETER(GrainInputTimeseries, GrainInput);
	)
	
	//TODO: Documentation says this should use Reffq = "effective precipitation". Is that the same as rainfall? Or rainfall + snowmelt?
	EQUATION(Model, MobilisedViaSplashDetachment,
		double Reffq = RESULT(Rainfall) / 86.4;
		double SSD = 86400.0 * PARAMETER(SplashDetachmentScalingFactor) * Reffq * pow(PARAMETER(SplashDetachmentSoilErodibility), 10.0 / (10.0 - PARAMETER(VegetationIndex)));
		return Min(SSD, RESULT(ImmobileGrainStoreBeforeMobilisation));
	)
	
	EQUATION(Model, SurfaceGrainStoreBeforeTransport,
		//TODO: Should we also allow inputs to the surface store?
		return LAST_RESULT(SurfaceGrainStore) + RESULT(MobilisedViaSplashDetachment);
	)
	
	auto TotalSurfaceGrainstoreBeforeTransport = RegisterEquationCumulative(Model, "Total surface grain store before transport", SurfaceGrainStoreBeforeTransport, Class);
	
	EQUATION(Model, TransportBeforeFlowErosion,
		double potential = RESULT(SurfaceGrainStoreBeforeTransport);
		double capacity  = RESULT(SurfaceTransportCapacity);
		double totalpotential = RESULT(TotalSurfaceGrainstoreBeforeTransport);
		if(totalpotential > capacity) return SafeDivide(potential, totalpotential) * capacity;
		return potential;
	)
	
	EQUATION(Model, FlowErosionKFactor,
		double qquick = RESULT(RunoffToReach, DirectRunoff) / 86.4;
		double flow = Max(0.0, qquick - PARAMETER(FlowErosionDirectRunoffThreshold));
		return 86400.0 * PARAMETER(FlowErosionScalingFactor) * PARAMETER(FlowErosionPotential) 
			* pow( (1e-3 * PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(FlowErosionNonlinearCoefficient));
	)
	
	EQUATION(Model, PotentiallyMobilisedViaFlowErosion,
		double TC = RESULT(SurfaceTransportCapacity);
	
		double SFE = RESULT(FlowErosionKFactor)
				* SafeDivide(TC - RESULT(MobilisedViaSplashDetachment),
							TC + RESULT(FlowErosionKFactor));
		SFE = Max(0.0, SFE);
		
		double SSD = RESULT(MobilisedViaSplashDetachment);
		
		double surfacestorebeforeerosion = RESULT(SurfaceGrainStoreBeforeTransport) - RESULT(TransportBeforeFlowErosion);
		
		if(surfacestorebeforeerosion > 0.0) return 0.0; //NOTE: If there is any surface sediment left we know that we already exceeded our transport capacity, and so we can not mobilise any more.
		
		return Min(SFE, RESULT(ImmobileGrainStoreBeforeMobilisation) - RESULT(MobilisedViaSplashDetachment));
	)
	
	auto TotalPotentiallyMobilisedViaFlowErosion = RegisterEquationCumulative(Model, "Total potentially mobilised via flow erosion", PotentiallyMobilisedViaFlowErosion, Class);
	
	EQUATION(Model, MobilisedViaFlowErosion,
		double potential = RESULT(PotentiallyMobilisedViaFlowErosion);
		double totalpotential = RESULT(TotalPotentiallyMobilisedViaFlowErosion);
		double capacity = RESULT(SurfaceTransportCapacity) - RESULT(TransportBeforeFlowErosion);
		
		if(totalpotential > capacity) return SafeDivide(potential, totalpotential) * capacity;
		return potential;
	)
	
	EQUATION(Model, GrainDeliveryToReach,
		return RESULT(TransportBeforeFlowErosion) + RESULT(MobilisedViaFlowErosion);
	)
	
	EQUATION(Model, SurfaceGrainStoreAfterAllTransport,
		return RESULT(SurfaceGrainStoreBeforeTransport) - RESULT(TransportBeforeFlowErosion); //NOTE: Flow erosion is removed from the immobile store directly.
	)
	
	EQUATION(Model, SurfaceGrainStore,
		
		double aftertransport   = RESULT(SurfaceGrainStoreAfterAllTransport);
		
		double fromotherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			double otherclassmass = RESULT(SurfaceGrainStoreAfterAllTransport, OtherClass);
			fromotherclasses += PARAMETER(LandMassTransferRateBetweenClasses, OtherClass, CURRENT_INDEX(Class)) * otherclassmass;
		}
		
		double tootherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			tootherclasses += PARAMETER(LandMassTransferRateBetweenClasses, CURRENT_INDEX(Class), OtherClass) * aftertransport;
		}
		
		return aftertransport + fromotherclasses - tootherclasses;
	)
	
	EQUATION(Model, ImmobileGrainStoreAfterMobilisation,
		//TODO: Inputs. And the inputs should probably be added BEFORE mobilisation.
		return RESULT(ImmobileGrainStoreBeforeMobilisation) - RESULT(MobilisedViaSplashDetachment) - RESULT(MobilisedViaFlowErosion);
	)
	
	EQUATION(Model, ImmobileGrainStore,
		double aftermob   = RESULT(ImmobileGrainStoreAfterMobilisation);
		
		double fromotherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			double otherclassmass = RESULT(ImmobileGrainStoreAfterMobilisation, OtherClass);
			fromotherclasses += PARAMETER(LandMassTransferRateBetweenClasses, OtherClass, CURRENT_INDEX(Class)) * otherclassmass;
		}
		
		double tootherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			tootherclasses += PARAMETER(LandMassTransferRateBetweenClasses, CURRENT_INDEX(Class), OtherClass) * aftermob;
		}
		
		return aftermob + fromotherclasses - tootherclasses;
	)
	
	EQUATION(Model, AreaScaledGrainDeliveryToReach,
		return PARAMETER(SubcatchmentArea) * PARAMETER(Percent) / 100.0 * RESULT(GrainDeliveryToReach);
	)

	
	
#if 0
	///////////////// Suspended sediment ////////////////////////////////
	
	auto InstreamSedimentSolver = RegisterSolver(Model, "In-stream sediment solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	
	auto SedimentReach = RegisterParameterGroup(Model, "Sediment reach", Reach);
	SetParentGroup(Model, SedimentReach, GrainClass);
	
	auto EffluentSedimentConcentration   = RegisterParameterDouble(Model, SedimentReach, "Effluent sediment concentration", MgPerL, 0.0);
	
	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	
	auto BankErosionScalingFactor        = RegisterParameterDouble(Model, Reaches, "Bank erosion scaling factor", KgPerM2PerM3SPerDay, 1.0);
	auto BankErosionNonlinearCoefficient = RegisterParameterDouble(Model, Reaches, "Bank erosion non-linear coefficient", Dimensionless, 1.0);
	auto ShearVelocityCoefficient        = RegisterParameterDouble(Model, Reaches, "Shear velocity coefficient", Dimensionless, 1.0);
	auto MeanChannelSlope                = RegisterParameterDouble(Model, Reaches, "Mean channel slope", Dimensionless, 2.0);
	auto EntrainmentCoefficient          = RegisterParameterDouble(Model, Reaches, "Entrainment coefficient", S2PerKg, 1.0);
	auto InitialMassOfBedSedimentPerUnitArea = RegisterParameterDouble(Model, SedimentReach, "Initial mass of bed sediment per unit area", KgPerM2, 10);
	
	auto InitialSuspendedSedimentMass    = RegisterParameterDouble(Model, SedimentReach, "Initial suspended sediment mass", Kg, 1e2);
	
	auto SedimentOfSizeClassDeliveredToReach = RegisterEquation(Model, "Sediment of size class delivered to reach", KgPerDay);
	auto ReachUpstreamSuspendedSediment     = RegisterEquation(Model, "Reach upstream suspended sediment", KgPerDay);
	auto ClayReleaseFromChannelBanks        = RegisterEquation(Model, "Clay release from channel banks", KgPerM2PerDay);
	auto ReachFrictionFactor                = RegisterEquation(Model, "Reach friction factor", Dimensionless);
	auto ReachShearVelocity                 = RegisterEquation(Model, "Reach shear velocity", MPerS);
	auto ProportionOfSedimentThatCanBeEntrained = RegisterEquation(Model, "Proportion of sediment that can be entrained", Dimensionless);
	auto StreamPower                        = RegisterEquation(Model, "Stream power", JPerSPerM2);
	
	auto SedimentEntrainment                = RegisterEquation(Model, "Sediment entrainment", KgPerM2PerDay);
	SetSolver(Model, SedimentEntrainment, InstreamSedimentSolver);
	
	auto SedimentDeposition                 = RegisterEquation(Model, "Sediment deposition", KgPerM2PerDay);
	SetSolver(Model, SedimentDeposition, InstreamSedimentSolver);
	
	auto ReachSuspendedSedimentOutput       = RegisterEquation(Model, "Reach suspended sediment output", KgPerDay);
	SetSolver(Model, ReachSuspendedSedimentOutput, InstreamSedimentSolver);
	
	auto MassOfBedSedimentPerUnitArea       = RegisterEquationODE(Model, "Mass of bed sediment per unit area", KgPerM2);
	SetInitialValue(Model, MassOfBedSedimentPerUnitArea, InitialMassOfBedSedimentPerUnitArea);
	SetSolver(Model, MassOfBedSedimentPerUnitArea, InstreamSedimentSolver);
	
	auto SuspendedSedimentMass = RegisterEquationODE(Model, "Suspended sediment mass", Kg);
	SetInitialValue(Model, SuspendedSedimentMass, InitialSuspendedSedimentMass);
	SetSolver(Model, SuspendedSedimentMass, InstreamSedimentSolver);
	
	
	
	auto ReachWidth = GetParameterDoubleHandle(Model, "Reach width");
	auto EffluentFlow = GetParameterDoubleHandle(Model, "Effluent flow");
	auto ReachDepth = GetEquationHandle(Model, "Reach depth");
	auto ReachFlow  = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume = GetEquationHandle(Model, "Reach volume");
	auto ReachVelocity = GetEquationHandle(Model, "Reach velocity");
	
	
	
	EQUATION(Model, SedimentOfSizeClassDeliveredToReach,
		return RESULT(TotalSedimentDeliveryToReach) * PARAMETER(PercentageOfSedimentInGrainSizeClass) / 100.0;
	)
	
	EQUATION(Model, ReachUpstreamSuspendedSediment,
		//CURRENT_INDEX(SizeClass); //TODO: Has to be here until we fix the dependency system some more..
		double sum = 0.0;
		FOREACH_INPUT(Reach,
			sum += RESULT(ReachSuspendedSedimentOutput, *Input);
		)
		return sum;
	)
	
	EQUATION(Model, ReachSuspendedSedimentOutput,
		return 86400.0 * RESULT(SuspendedSedimentMass) * RESULT(ReachFlow) / RESULT(ReachVolume);
	)
	
	EQUATION(Model, ClayReleaseFromChannelBanks,
		return PARAMETER(BankErosionScalingFactor) * pow(RESULT(ReachFlow), PARAMETER(BankErosionNonlinearCoefficient));
	)
	
	EQUATION(Model, ReachFrictionFactor,
		return 4.0 * RESULT(ReachDepth) / (2.0 * RESULT(ReachDepth) + PARAMETER(ReachWidth));
	)
	
	EQUATION(Model, ReachShearVelocity,
		double earthsurfacegravity = 9.807;
		return sqrt(earthsurfacegravity * RESULT(ReachDepth) * PARAMETER(ShearVelocityCoefficient) * PARAMETER(MeanChannelSlope));
	)
	
	EQUATION(Model, ProportionOfSedimentThatCanBeEntrained,
		double Dmax = 9.99 * pow(RESULT(ReachShearVelocity), 2.52);
		double Dlow = PARAMETER(SmallestDiameterOfSedimentClass);
		double Dupp = PARAMETER(LargestDiameterOfSedimentClass);
		if(Dmax < Dlow) return 0.0;
		if(Dmax > Dupp) return 1.0;
		return (Dmax - Dlow) / (Dupp - Dlow);
	)
	
	EQUATION(Model, StreamPower,
		double waterdensity = 1000.0;
		double earthsurfacegravity = 9.807;
		return waterdensity * earthsurfacegravity * PARAMETER(MeanChannelSlope) * RESULT(ReachVelocity) * RESULT(ReachDepth);
	)
	
	EQUATION(Model, SedimentEntrainment,
		double value = 86400.0 * PARAMETER(EntrainmentCoefficient) * RESULT(MassOfBedSedimentPerUnitArea) * RESULT(ProportionOfSedimentThatCanBeEntrained) * RESULT(StreamPower) * RESULT(ReachFrictionFactor);
		return value;
	)
	
	EQUATION(Model, SedimentDeposition,
		double mediangrainsize = (PARAMETER(SmallestDiameterOfSedimentClass) + PARAMETER(LargestDiameterOfSedimentClass)) / 2.0;
		double sedimentdensity = 2650.0;
		double waterdensity    = 1000.0;
		double earthsurfacegravity = 9.807;
		double fluidviscosity = 0.001;
		double terminalsettlingvelocity = (sedimentdensity - waterdensity) * earthsurfacegravity * Square(mediangrainsize) / (18.0 * fluidviscosity);
		
		double value = 86400.0 * terminalsettlingvelocity * RESULT(SuspendedSedimentMass) / RESULT(ReachVolume);
		
		return value;
	)
	
	EQUATION(Model, MassOfBedSedimentPerUnitArea,
		return RESULT(SedimentDeposition) - RESULT(SedimentEntrainment);
	)
	
	EQUATION(Model, SuspendedSedimentMass,
		double clayrelease = RESULT(ClayReleaseFromChannelBanks);
		if(CURRENT_INDEX(SizeClass) != 0) clayrelease = 0.0;      //NOTE: This assumes that index 0 is always clay though...
		
		return 
			  RESULT(SedimentOfSizeClassDeliveredToReach) 
			+ PARAMETER(EffluentSedimentConcentration) * PARAMETER(EffluentFlow)
			+ RESULT(ReachUpstreamSuspendedSediment) 
			- RESULT(ReachSuspendedSedimentOutput) 
			+ PARAMETER(ReachLength) * PARAMETER(ReachWidth) * (RESULT(SedimentEntrainment) + clayrelease - RESULT(SedimentDeposition));
	)
#endif
}


