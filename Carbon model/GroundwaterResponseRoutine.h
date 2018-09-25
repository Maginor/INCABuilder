#if !defined GROUNDWATER_RESPONSE_ROUTINE_H

void
AddGroundwaterResponseRoutine(inca_model *Model)
{
	auto Mm = RegisterUnit(Model, "mm");
	auto Days = RegisterUnit(Model, "day");
	auto PerDay = RegisterUnit(Model, "1/day");
	auto MmPerDay = RegisterUnit(Model, "mm/day");
	
	auto Reaches = RegisterIndexSetBranched(Model, "Reaches");
	auto Groundwater = RegisterParameterGroup(Model, "Groundwater", Reaches);
	
	//TODO: Find good values for parameters.
	auto FirstUpperRecessionCoefficient  = RegisterParameterDouble(Model, Groundwater, "First recession coefficent for upper groundwater storage", PerDay, 0.1);
	auto SecondUpperRecessionCoefficient = RegisterParameterDouble(Model, Groundwater, "First recession coefficent for upper groundwater storage", PerDay, 0.1);
	auto LowerRecessionCoefficient       = RegisterParameterDouble(Model, Groundwater, "Recession coefficient for lower groundwater storage", PerDay, 0.1);
	auto UpperSecondRunoffThreshold   = RegisterParameterDouble(Model, Groundwater, "Threshold for second runoff in upper storage", Mm, 10.0);
	auto PercolationRate              = RegisterParameterDouble(Model, Groundwater, "Percolation rate from upper to lower groundwater storage", MmPerDay, 0.1);
	
	auto MaxBase = RegisterParameterUInt(Model, Groundwater, "Flow routing max base", Days, 5);
	
	auto TotalGroundwaterRecharge = GetEquationHandle(Model, "Groundwater recharge"); //NOTE: From the soil moisture routine.
	
	auto GroundwaterSolver = RegisterSolver(Model, "Groundwater solver", 0.1, IncaDascru);
	
	auto UpperRunoff = RegisterEquation(Model, "Runoff from upper groundwater storage", MmPerDay);
	SetSolver(Model, UpperRunoff, GroundwaterSolver);
	auto LowerRunoff = RegisterEquation(Model, "Runoff from lower groundwater storage", MmPerDay);
	SetSolver(Model, LowerRunoff, GroundwaterSolver);
	auto Percolation = RegisterEquation(Model, "Percolation from upper to lower groundwater storage", MmPerDay);
	SetSolver(Model, Percolation, GroundwaterSolver);
	auto UpperStorage = RegisterEquationODE(Model, "Upper groundwater storage", Mm);
	SetSolver(Model, UpperStorage, GroundwaterSolver);
	//TODO: initial value
	auto LowerStorage = RegisterEquationODE(Model, "Lower groundwater storage", Mm);
	SetSolver(Model, LowerStorage, GroundwaterSolver);
	//TODO: initial value
	auto GroundwaterDischargeBeforeRouting = RegisterEquation(Model, "Groundwater discharge to reach before routing", MmPerDay);
	auto GroundwaterDischarge = RegisterEquation(Model, "Groundwater discharge to reach", MmPerDay);
	
	EQUATION(Model, UpperRunoff,
		double K1 = PARAMETER(FirstUpperRecessionCoefficient);
		double K0 = PARAMETER(SecondUpperRecessionCoefficient);
		double UZL = PARAMETER(UpperSecondRunoffThreshold);
		double runoff = RESULT(UpperStorage) * K1;
		if(RESULT(UpperStorage) > UZL) runoff += (RESULT(UpperStorage) - UZL)*K0;
		return runoff;
	)
	
	EQUATION(Model, LowerRunoff,
		return RESULT(LowerStorage) * PARAMETER(LowerRecessionCoefficient);
	)
	
	EQUATION(Model, Percolation,
		return RESULT(UpperStorage) * PARAMETER(PercolationRate);
	)
	
	EQUATION(Model, UpperStorage,
		return RESULT(TotalGroundwaterRecharge) - RESULT(Percolation) - RESULT(UpperRunoff);
	)
	
	EQUATION(Model, LowerStorage,
		return RESULT(Percolation) - RESULT(LowerRunoff);
	)
	
	EQUATION(Model, GroundwaterDischargeBeforeRouting,
		return RESULT(UpperRunoff) + RESULT(LowerRunoff);
	)
	
	//TODO: We have to test that these coefficients are correct:
	EQUATION(Model, GroundwaterDischarge,
		RESULT(GroundwaterDischargeBeforeRouting); //NOTE: To force a dependency since this is not automatic when we use EARLIER_RESULT;
	
		u64 M = PARAMETER(MaxBase);		
		double sum = 0.0;

		for(u64 I = 1; I <= M; ++I)
		{
			double a;
			u64 M2;
			if((M % 2) == 0)
			{
				double Md = (double)M * 0.5;
				a = 1.0 / (Md*(Md + 1.0));
				M2 = M / 2;
			}
			else
			{
				double Md = floor((double)M * 0.5);
				a = 2.0 / ((2.0 + 2.0*Md) * (Md + 1.0) + 1.0);
				M2 = M / 2 + 1;
			}
			double coeff;
			if(I <= M2)
			{
				coeff = a * (double)I;
			}
			else
			{
				coeff = a * (double)(2*M2 - I);
			}
			sum += coeff * EARLIER_RESULT(GroundwaterDischargeBeforeRouting, I-1);
		}
			
		return sum;
	)
	
	//TODO: Convert millimeter/day to flow for the reach routine.
}

#define GROUNDWATER_RESPONSE_ROUTINE_H
#endif