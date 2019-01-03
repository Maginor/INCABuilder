

//NOTE: So far this is just a hard coded test. Will make it general with a config file later.


#include "../../inca.h"
#include "../../Modules/PersistModel.h"

#include "Dlib/optimization.h"
#include "Dlib/global_optimization.h"

#include "Dlib/threads/thread_pool_extension.cpp"
#include "Dlib/global_optimization/global_function_search.cpp"


typedef dlib::matrix<double,0,1> column_vector;

class optimization_model
{
	inca_data_set *DataSet;
	u64 Timesteps;
	
	std::vector<double> Discharge;
	
public:
	optimization_model(inca_data_set *DataSet)
	{
		this->DataSet = DataSet;
		this->Timesteps = GetTimesteps(DataSet);
		Discharge.resize((size_t)Timesteps);
		
		GetInputSeries(DataSet, "Discharge", {"Tveitvatn"}, Discharge.data(), Discharge.size(), true);
	}
	
	double operator()(const column_vector& Par)
	{
		SetParameterValue(DataSet, "a", {"Tveitvatn"}, Par(0));
		SetParameterValue(DataSet, "b", {"Tveitvatn"}, Par(1));
		
		RunModel(DataSet);
		
		std::vector<double> Flow((size_t)Timesteps);
		GetResultSeries(DataSet, "Reach flow", {"Tveitvatn"}, Flow.data(), Flow.size());
		
		double SSE = 0.0;
		for(size_t Timestep = 0; Timestep < Timesteps; ++Timestep)
		{
			//TODO: Use accumulator to not lose precision here:
			double E = Discharge[Timestep] - Flow[Timestep];
			SSE += E*E;
		}
		
		return SSE;
	}
};

int main()
{
	const char *ParameterFile = "../../Applications/IncaN/tovdalparametersPersistOnly.dat";
	const char *InputFile     = "../../Applications/IncaN/tovdalinputs.dat";
	
	inca_model *Model = BeginModelDefinition();
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddPersistModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	inca_data_set *DataSet = GenerateDataSet(Model);
	
	//NOTE: For model structure as well as parameter values that we are not going to change:
	ReadParametersFromFile(DataSet, ParameterFile);
	ReadInputsFromFile(DataSet, InputFile);
	
	optimization_model Optim(DataSet);
	
	auto Result = dlib::find_min_global(Optim, {.001, .1}, {.7, .9}, dlib::max_function_calls(500));
	
	std::cout << "best a: " << Result.x(0);
	std::cout << "best b: " << Result.x(1);
}