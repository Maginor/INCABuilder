
//NOTE: This functionality was factored out to its own file to not clutter up inca_model.cpp too much, but its workings are intimately tied with those of inca_model.cpp


static void
FindSolverBatchIndirectEquationDependencies(inca_model *Model, equation_batch &Batch, equation_h Equation, std::set<size_t> &ODEDependenciesOut)
{
	equation_spec &Spec = Model->EquationSpecs[Equation.Handle];
	for(equation_h Dependency : Spec.DirectResultDependencies)
	{
		auto It = std::find(Batch.EquationsODE.begin(), Batch.EquationsODE.end(), Dependency);   // NOTE: See if the dependency is an ODE function in the same batch.
		if(It != Batch.EquationsODE.end())
		{
			size_t Pos = (size_t)std::distance(Batch.EquationsODE.begin(), It);   //NOTE: This is just the index of the Dependency in the EquationsODE vector (yes, C++ has no standard function for this :( 
			ODEDependenciesOut.insert(Pos);
			continue;
		}
		
		auto It2 = std::find(Batch.Equations.begin(), Batch.Equations.end(), Dependency);  // NOTE: See if the dependency is a non-ODE in the same batch
		if(It2 != Batch.Equations.end())
		{
			//NOTE: We have to traverse recursively so that we get all true dependencies of the original ODE!
			FindSolverBatchIndirectEquationDependencies(Model, Batch, Dependency, ODEDependenciesOut);
		}
	}
}

static void
BuildJacobianInfo(inca_model *Model)
{
	for(equation_batch &Batch : Model->EquationBatches)
	{
		if(Batch.Type == BatchType_Solver && Model->SolverSpecs[Batch.Solver.Handle].UsesJacobian)
		{	
			size_t N = Batch.EquationsODE.size();
			Batch.ODEIsDependencyOfODE.resize(N);
			Batch.ODEIsDependencyOfNonODE.resize(N);
			
			for(size_t Idx = 0; Idx < N; ++Idx)
			{
				equation_h Equation = Batch.EquationsODE[Idx];
				
				std::set<size_t> ODEDependencies;
				
				FindSolverBatchIndirectEquationDependencies(Model, Batch, Equation, ODEDependencies);
				
				for(size_t Dep : ODEDependencies)
				{
					Batch.ODEIsDependencyOfODE[Dep].push_back(Idx);  //NOTE: Signifies that the equation with index Dep is a dependency of the equation with index Idx.
				}
			}
			
			for(equation_h Equation : Batch.Equations)
			{
				std::set<size_t> ODEDependencies;
				
				FindSolverBatchIndirectEquationDependencies(Model, Batch, Equation, ODEDependencies);
				
				for(size_t Dep : ODEDependencies)
				{
					Batch.ODEIsDependencyOfNonODE[Dep].push_back(Equation);
				}
			}
		}
	}
}


#define USE_JACOBIAN_OPTIMIZATION 1

static void
EstimateJacobian(double *X, double *J, double *FBaseVec, const inca_model *Model, value_set_accessor *ValueSet, const equation_batch &Batch)
{
	//NOTE: This is not a very numerically accurate estimation of the Jacobian, it is mostly optimized for speed. We'll see if it is good enough..

	size_t N = Batch.EquationsODE.size();
	
	for(size_t Idx = 0; Idx < N; ++Idx)
	{
		equation_h Equation = Batch.EquationsODE[Idx];
		ValueSet->CurResults[Equation.Handle] = X[Idx];
	}
	
	//NOTE: Evaluation of the ODE system at base point. TODO: We should find a way to reuse the calculation we already do at the basepoint, however it is done by a separate callback, so that is tricky..
	for(equation_h Equation : Batch.Equations)
	{
		double ResultValue = CallEquation(Model, ValueSet, Equation);
		ValueSet->CurResults[Equation.Handle] = ResultValue;
	}
	
	for(size_t Idx = 0; Idx < N; ++Idx)
	{
		equation_h EquationToCall = Batch.EquationsODE[Idx];
		FBaseVec[Idx] = CallEquation(Model, ValueSet, EquationToCall);
	}
	
#if USE_JACOBIAN_OPTIMIZATION
	double *Backup = (double *)malloc(sizeof(double) * Model->FirstUnusedEquationHandle);
#endif
	
	//TODO: This is a naive way of doing it. We should instead use pre-recorded info about which equations depend on which and skip evaluation in the case where there is no dependency.
	for(size_t Col = 0; Col < N; ++Col)
	{
		equation_h EquationToPermute = Batch.EquationsODE[Col];
		
		//Hmm. here we assume we can use the same H for all Fs which may not be a good idea?? But it makes things significantly faster because then we don't have to recompute the non-odes in all cases.
		double H = 1e-6;  //TODO: This should definitely be sensitive to the size of the base values!!
		ValueSet->CurResults[EquationToPermute.Handle] = X[Col] + H; //TODO: Do it numerically correct
		
		//TODO: We should do some optimization here too to not have to call all the non-ode equations, but in that case we have to remember to reset the values at the end.
#if USE_JACOBIAN_OPTIMIZATION		
		for(equation_h Equation : Batch.ODEIsDependencyOfNonODE[Col])
#else
		for(equation_h Equation : Batch.Equations)
#endif
		{
#if USE_JACOBIAN_OPTIMIZATION
			Backup[Equation.Handle] = ValueSet->CurResults[Equation.Handle];
#endif
			double ResultValue = CallEquation(Model, ValueSet, Equation);
			ValueSet->CurResults[Equation.Handle] = ResultValue;
		}
		
#if USE_JACOBIAN_OPTIMIZATION
		for(size_t Row : Batch.ODEIsDependencyOfODE[Col])
#else
		for(size_t Row = 0; Row < N; ++Row)
#endif
		{
			equation_h EquationToCall = Batch.EquationsODE[Row];
		
			double FBase = FBaseVec[Row]; //NOTE: The value of the EquationToCall at the base point.
			
			double FPermute = CallEquation(Model, ValueSet, EquationToCall);
			
			double Derivative = (FPermute - FBase) / H; //TODO: Numerical correctness
			
			//NOTE: Assumes row major storage (this is the default for the boost solver use case. If we need column major for another use case we have to figure out how to do that)
			J[N*Row + Col] = Derivative;
			
			//std::cout << Derivative << " ";
		}
		//std::cout << std::endl;
		
		ValueSet->CurResults[EquationToPermute.Handle] = X[Col];  //NOTE: Reset the value so that it is correct for the next column.
		
#if USE_JACOBIAN_OPTIMIZATION
		for(equation_h Equation : Batch.ODEIsDependencyOfNonODE[Col])
		{
			ValueSet->CurResults[Equation.Handle] = Backup[Equation.Handle];
		}
#endif
	}
	
#if USE_JACOBIAN_OPTIMIZATION
	free(Backup);
#endif
	//std::cout << "end matrix" << std::endl;
}