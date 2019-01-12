
//#define INCA_SOLVER_FUNCTION(Name) void Name(double h, size_t n, double* x0, double* wk, const inca_solver_equation_function &EquationFunction, const inca_solver_equation_function &JacobiFunction, double AbsErr, double RelErr)


/*

	There are a lot of different solvers int the boost::numeric::odeint package. We have not wrapped all of them here. If you want one you could ask for it to be included here, or just do it yourself, which should be pretty easy.

*/


#if !defined(BOOST_SOLVERS_H)

#include <boost/numeric/odeint.hpp>

typedef boost::numeric::ublas::vector< double > solver_vector_type;
typedef boost::numeric::ublas::matrix< double > solver_matrix_type;

struct ode_system
{
	const inca_solver_equation_function &EquationFunction;
	
	ode_system(const inca_solver_equation_function &EquationFunction) : EquationFunction(EquationFunction)
	{
	}
	
	void operator()( const solver_vector_type &X , solver_vector_type &DXDT , double /* t */ )
    {
        EquationFunction((double *)X.data().begin(), (double *)DXDT.data().begin());
    }
};

struct ode_system_jacobi
{
	const inca_solver_equation_function &JacobiFunction;
	
	ode_system_jacobi(const inca_solver_equation_function &JacobiFunction) : JacobiFunction(JacobiFunction)
	{
	}
	
	void operator()( const solver_vector_type &X, solver_matrix_type &J , const double & /* t */ , solver_vector_type /* &DFDT */ )
    {
		//NOTE: We are banking on not having to clear DFDT each time. We assume it is inputed as 0 from the solver.. However I don't know if this is documented functionality
		
		J.clear(); //NOTE: Unfortunately it seems like J contains garbage values at the start of each run unless we clear it. And we have to clear it since we only set the nonzero values in the JacobiEstimation.
		
		JacobiFunction((double *)X.data().begin(), (double *)J.data().begin());
	}
};

INCA_SOLVER_FUNCTION(BoostRosenbrock4Impl_)
{
	using namespace boost::numeric::odeint;
	
	//It is a little stupid that we have to copy the state back and forth, but it seems like we can't create a ublas vector that has an existing pointer as its data (or correct me if I'm wrong!)
	solver_vector_type X(n);
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		X[Idx] = x0[Idx];
	}

	size_t NSteps = integrate_adaptive( 
			make_controlled< rosenbrock4< double > >( AbsErr, RelErr ),
			std::make_pair( ode_system(EquationFunction), ode_system_jacobi(JacobiFunction) ),
			X, 0.0 , 1.0 , h 
			/*TODO: add an observer to handle errors? */);
			
	//std::cout << "N steps : " << NSteps << std::endl;
	
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		x0[Idx] = X[Idx];
	}
}

INCA_SOLVER_SETUP_FUNCTION(BoostRosenbrock4)
{
	SolverSpec->SolverFunction = BoostRosenbrock4Impl_;
	SolverSpec->UsesJacobian = true;
	SolverSpec->UsesErrorControl = true;
}



INCA_SOLVER_FUNCTION(BoostRK4Impl_)
{
	using namespace boost::numeric::odeint;
	solver_vector_type X(n);
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		X[Idx] = x0[Idx];
	}

	size_t NSteps = integrate_adaptive( 
			runge_kutta4<solver_vector_type>(),
			ode_system(EquationFunction),
			X, 0.0 , 1.0 , h 
			/*TODO: add an observer to handle errors? */);
			
	//std::cout << "N steps : " << NSteps << std::endl;
	
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		x0[Idx] = X[Idx];
	}
}

INCA_SOLVER_SETUP_FUNCTION(BoostRK4)
{
	SolverSpec->SolverFunction = BoostRK4Impl_;
	SolverSpec->UsesJacobian = false;
	SolverSpec->UsesErrorControl = false;
}


INCA_SOLVER_FUNCTION(BoostCashCarp54Impl_)
{
	using namespace boost::numeric::odeint;
	solver_vector_type X(n);
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		X[Idx] = x0[Idx];
	}

	size_t NSteps = integrate_adaptive( 
			controlled_runge_kutta<runge_kutta_cash_karp54<solver_vector_type>>(AbsErr, RelErr),
			ode_system(EquationFunction),
			X, 0.0 , 1.0 , h 
			/*TODO: add an observer to handle errors? */);
			
	//std::cout << "N steps : " << NSteps << std::endl;
	
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		x0[Idx] = X[Idx];
	}
}

INCA_SOLVER_SETUP_FUNCTION(BoostCashCarp54)
{
	SolverSpec->SolverFunction = BoostRK4Impl_;
	SolverSpec->UsesJacobian = false;
	SolverSpec->UsesErrorControl = true;
}

#define BOOST_SOLVERS_H
#endif