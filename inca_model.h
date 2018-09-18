

/*
Important TODOs:
	- Better encapsulation of the ValueSet subsystem. Unify lookup systems for parameters, inputs, results, last_results
	- Have to figure out if the initial value equation system we have currently is good.
	- Implement stream index set specific functionality. (or not??)
	- Better logging / error handling system
	- Functionality for re-running the same DataSet, potentially with changed parameter values. (i.e. for use with MCMC)
	- Proper destructors for several structs (inca_model, inca_data_set etc) so as not to leak.
	- In the equation placement optimization, try to move entire batches to reduce the number of batch groups if possible.
	- Give warning if not all input series received values?
	- Maybe just use fscanf for reading numbers in inca_io, but it is actually a little complicated since we have to figure out the type in any case.
	
Bugs:
	- Check the dependency system with maximumnitrogenuptake in incan-classic again.. I may have misread it, but there is a potential bug there.
	
Less important:
	- Make a Contains macro? (things like (std::find(blabla.begin(), blabla.end(), it) != it) are hard to read).
	
Other TODOs (low priority):
	For better memory locality:
	- Manage the memory for all the data in the equation batch structure in such a way that it is aligned with how it will be read. (will have to not use std::vector in that case...)
*/




#if !defined(INCA_MODEL_H)

#include <stdint.h>
#include <stdlib.h>
#include <functional>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <iostream>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <cmath>

//NOTE: Does this intrinsic header exist for all compilers? Is only used for __rdtsc();
#include <x86intrin.h>


typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef size_t handle_t;
//typedef u32    index_t; //NOTE: We get an error on the linux compilation with this, and have not entirely figured out why
typedef size_t index_t;

#include "inca_util.h"


#define MODEL_ENTITY_HANDLE(Name) struct Name \
{ \
	handle_t Handle; \
}; \
bool operator==(const Name &A, const Name &B) { return A.Handle == B.Handle; } \
bool operator!=(const Name &A, const Name &B) { return A.Handle != B.Handle; } \
bool operator<(const Name &A, const Name &B) { return A.Handle < B.Handle; } \
inline bool IsValid(Name H) { return H.Handle > 0; }

MODEL_ENTITY_HANDLE(unit)

MODEL_ENTITY_HANDLE(input)
MODEL_ENTITY_HANDLE(equation)
MODEL_ENTITY_HANDLE(parameter_double)
MODEL_ENTITY_HANDLE(parameter_uint)
MODEL_ENTITY_HANDLE(parameter_bool)
MODEL_ENTITY_HANDLE(parameter_ptime)

MODEL_ENTITY_HANDLE(solver)

MODEL_ENTITY_HANDLE(index_set)

MODEL_ENTITY_HANDLE(parameter_group)

#undef MODEL_ENTITY_HANDLE


struct parameter_value
{
	union
	{
		double ValDouble;
		u64 ValUInt;
		u64 ValBool; //NOTE: Since this is a union we don't save space by making the bool smaller any way.
		s64 ValTime; //NOTE: Seconds since 1/1/1970
	};
};

enum parameter_type
{
	ParameterType_Double,
	ParameterType_UInt,
	ParameterType_Bool,
	ParameterType_Time,
};

struct parameter_spec
{
	const char *Name;
	parameter_type Type;
	parameter_value Min;
	parameter_value Max;
	parameter_value Default;
	
	unit Unit;
	
	parameter_group Group;
	std::vector<index_set> IndexSetDependencies;
};

struct unit_spec
{
	const char *Name;
	//NOTE: We don't need to put anything else here at the moment. Maybe eventually?
};


struct value_set_accessor;

typedef std::function<double(value_set_accessor *)> inca_equation;

typedef std::function<void(double *, double *)> inca_solver_equation_function;
#define INCA_SOLVER_FUNCTION(Name) void Name(double h, u32 n, double* x0, double* wk, const inca_solver_equation_function &EquationFunction)
typedef INCA_SOLVER_FUNCTION(inca_solver_function);

struct parameter_group_spec
{
	const char *Name;
	parameter_group ParentGroup;
	std::vector<parameter_group> ChildrenGroups;
	index_set IndexSet;
	std::vector<handle_t> Parameters;
};

enum index_set_type
{
	IndexSetType_Basic,
	IndexSetType_Branched,
};

struct index_set_spec
{
	const char *Name;
	index_set_type Type;
	std::vector<const char *> RequiredIndexes;
};

enum equation_type
{
	EquationType_Basic,
	EquationType_ODE,
	EquationType_InitialValue,
	EquationType_Cumulative,
};

//TODO: See if we could unionize some of the data below. Not everything is needed by every type of equation.
struct equation_spec
{
	const char *Name;
	equation_type Type;
	
	unit Unit;
	
	parameter_double InitialValue;
	double ExplicitInitialValue;
	bool HasExplicitInitialValue;
	equation InitialValueEquation;
	
	std::set<index_set> IndexSetDependencies;
	std::set<handle_t>  ParameterDependencies;
	std::set<input>     InputDependencies;
	std::set<equation>  DirectResultDependencies;
	std::set<equation>  DirectLastResultDependencies;
	bool EquationIsSet;
	
	bool TempVisited; //NOTE: For use in a graph traversal algorithm while resolving dependencies.
	bool Visited;     //NOTE: For use in a graph traversal algorithm while resolving dependencies
	
	index_set CumulatesOverIndexSet; //NOTE: Only used for Type == EquationType_Cumulative.
	equation Cumulates;              //NOTE: Only used for Type == EquationType_Cumulative.
	
	solver Solver;
};

struct solver_spec
{
	const char *Name;
	double h;
	inca_solver_function *SolverFunction;
	std::set<index_set> IndexSetDependencies;
	
	std::vector<equation> EquationsToSolve;
	std::set<equation> DirectResultDependencies;
	bool TempVisited; //NOTE: For use in a graph traversal algorithm while resolving dependencies.
	bool Visited;     //NOTE: For use in a graph traversal algorithm while resolving dependencies
};

struct input_spec
{
	const char *Name;
	
	unit Unit; //NOTE: Not currently used.
	
	std::vector<index_set> IndexSetDependencies;
};


//TODO: Find a better name for this struct?
struct iteration_data
{
	std::vector<handle_t>   ParametersToRead;
	std::vector<input>      InputsToRead;
	std::vector<equation>   ResultsToRead;
	std::vector<equation>   LastResultsToRead;
};

enum equation_batch_type
{
	BatchType_Regular,
	BatchType_Solver,
};

struct equation_batch
{ 
	equation_batch_type Type;
	std::vector<equation> Equations;
	
	solver Solver;                      //NOTE: Only for Type==BatchType_Solver.
	std::vector<equation> EquationsODE; //NOTE: Only for Type==BatchType_Solver.
	
	std::vector<equation> InitialValueOrder; //NOTE: The initial value setup of equations happens in a different order than the execution order during model run because the intial value equations may have different dependencies than the equations they are initial values for.
};

struct equation_batch_group
{
	std::vector<index_set> IndexSets;
	std::vector<iteration_data> IterationData;
	size_t FirstBatch;
	size_t LastBatch;
};

struct storage_unit_specifier
{
	std::vector<index_set> IndexSets;
	std::vector<handle_t> Handles;
};

struct storage_structure
{
	std::vector<storage_unit_specifier> Units;
	
	size_t *TotalCountForUnit;
	size_t *OffsetForUnit;
	size_t *UnitForHandle;
	size_t *LocationOfHandleInUnit;       // UnitForHandle[H].Handles[LocationOfHandleInUnit[H]] == H;
	size_t TotalCount;
};


struct char_equals 
{  
    bool operator()(const char *x, const char *y) const  
    { return strcmp(x, y) == 0; }  
};

//TODO: Borrowed hash function from https://stackoverflow.com/questions/20649864/c-unordered-map-with-char-as-key . We should look into it more..
struct hash_function
{
    //BKDR Hash algorithm
    int operator()(const char *Str) const
    {
        int Seed = 131;//31  131 1313 13131131313 etc//
        int Hash = 0;
        while(*Str)
        {
            Hash = (Hash * Seed) + (*Str);
            ++Str;
        }
        return Hash & (0x7FFFFFFF);
    }
};

typedef std::unordered_map<const char *, handle_t, hash_function, char_equals> char_map;


struct inca_model
{
	handle_t FirstUnusedEquationHandle;
	char_map EquationNameToHandle;
	std::vector<inca_equation> Equations;
	std::vector<equation_spec> EquationSpecs;
	
	handle_t FirstUnusedInputHandle;
	char_map InputNameToHandle;
	std::vector<input_spec> InputSpecs;
	
	handle_t FirstUnusedParameterHandle;
	char_map ParameterNameToHandle;
	std::vector<parameter_spec> ParameterSpecs;
	
	handle_t FirstUnusedIndexSetHandle;
	char_map IndexSetNameToHandle;
	std::vector<index_set_spec> IndexSetSpecs;
	
	handle_t FirstUnusedParameterGroupHandle;
	char_map ParameterGroupNameToHandle;
	std::vector<parameter_group_spec> ParameterGroupSpecs;
	
	handle_t FirstUnusedSolverHandle;
	char_map SolverNameToHandle;
	std::vector<solver_spec> SolverSpecs;
	
	handle_t FirstUnusedUnitHandle;
	char_map UnitNameToHandle;
	std::vector<unit_spec> UnitSpecs;
	
	std::vector<equation_batch> EquationBatches;
	std::vector<equation_batch_group> BatchGroups;
	
	timer DefinitionTimer;
	bool Finalized;
};

#define FOR_ALL_BATCH_EQUATIONS(Batch, Do) \
for(equation Equation : Batch.Equations) { Do } \
if(Batch.Type == BatchType_Solver) { for(equation Equation : Batch.EquationsODE) { Do } }

#if !defined(INCA_EQUATION_PROFILING)
#define INCA_EQUATION_PROFILING 0
#endif

//TODO: The name "inputs" here is confusing, since there is already a different concept called input.
//TODO: Couldn't this just be a std::vector?
struct branch_inputs
{
	size_t Count;
	index_t *Inputs;
};

struct inca_data_set
{
	inca_model *Model;
	
	parameter_value *ParameterData;
	storage_structure ParameterStorageStructure;
	
	double *InputData;
	storage_structure InputStorageStructure;
	u64 InputDataTimesteps;
	
	double *ResultData;
	storage_structure ResultStorageStructure;
	
	index_t *IndexCounts;
	const char ***IndexNames;  // IndexNames[IndexSet.Handle][IndexNamesToHandle[IndexSet.Handle][IndexName]] == IndexName;
	std::vector<char_map> IndexNamesToHandle;
	bool AllIndexesHaveBeenSet;
	
	branch_inputs **BranchInputs;

	
	std::vector<parameter_value> FastParameterLookup;
	std::vector<size_t> FastInputLookup;
	std::vector<size_t> FastResultLookup;
	std::vector<size_t> FastLastResultLookup;
	
	double *x0; //NOTE: Temporary storage for use by solvers
	double *wk; //NOTE: Temporary storage for use by solvers

	bool HasBeenRun;
	u64 TimestepsLastRun;
};

struct value_set_accessor
{
	bool Running;
	inca_model *Model;
	inca_data_set *DataSet;
	
	u64 DayOfYear;
	u64 DaysThisYear;
	s64 Timestep; //NOTE: We make this an s64 so that the initial value step can be recorded as timestep=-1. This is not a good way to do it though.

	//NOTE: A trained eye probably recognize this class as one that should be inherited by two subclasses. The problem with doing that is that it would slow down model execution a lot if you can't do direct access of member data.
	union
	{
		struct //NOTE: For use during model execution
		{
			parameter_value *CurParameters;
			double          *CurInputs;
			double          *CurResults;
			double          *LastResults;
			
			index_t *CurrentIndexes; //NOTE: Contains the current index of each index set during execution.	
			
			double *AllCurResultsBase;
			double *AllLastResultsBase;
			
			double *AllCurInputsBase;
			
			double *AtResult;
			double *AtLastResult;
			
			
			parameter_value *AtParameterLookup;
			size_t *AtInputLookup;
			size_t *AtResultLookup;
			size_t *AtLastResultLookup;
		};
		
		struct //NOTE: For use during dependency registration
		{
			size_t *ParameterDependency;
			size_t *InputDependency;
			size_t *ResultDependency;
			size_t *LastResultDependency;
			size_t *ResultCrossIndexDependency;
			size_t *DirectIndexSetDependency;
		};
	};
	
#if INCA_EQUATION_PROFILING
	size_t *EquationHits;
	u64 *EquationTotalCycles;
#endif
	
	//NOTE: For dependency registration run:
	value_set_accessor(inca_model *Model)
	{
		Running = false;
		DataSet = 0;
		this->Model = Model;
		ParameterDependency         = AllocClearedArray(size_t, Model->FirstUnusedParameterHandle);
		InputDependency             = AllocClearedArray(size_t, Model->FirstUnusedInputHandle);
		ResultDependency            = AllocClearedArray(size_t, Model->FirstUnusedEquationHandle);
		LastResultDependency        = AllocClearedArray(size_t, Model->FirstUnusedEquationHandle);
		ResultCrossIndexDependency  = AllocClearedArray(size_t, Model->FirstUnusedEquationHandle);
		DirectIndexSetDependency    = AllocClearedArray(size_t, Model->FirstUnusedIndexSetHandle);
	}
	
	//NOTE: For proper run:
	value_set_accessor(inca_data_set *DataSet)
	{
		Running = true;
		this->DataSet = DataSet;
		Model = DataSet->Model;

		CurInputs      = AllocClearedArray(double, Model->FirstUnusedInputHandle);
		CurParameters  = AllocClearedArray(parameter_value, Model->FirstUnusedParameterHandle);
		CurResults     = AllocClearedArray(double, Model->FirstUnusedEquationHandle);
		LastResults    = AllocClearedArray(double, Model->FirstUnusedEquationHandle);
		CurrentIndexes = AllocClearedArray(index_t, Model->FirstUnusedIndexSetHandle);
		
		DayOfYear = 0;
		DaysThisYear = 365;
		Timestep = 0;
	}
	
	~value_set_accessor()
	{
		if(Running)
		{
			free(CurParameters);
			free(CurInputs);
			free(CurResults);
			free(LastResults);
			free(CurrentIndexes);

		}
		else
		{
			free(ParameterDependency);
			free(InputDependency);
			free(ResultDependency);
			free(LastResultDependency);
			free(ResultCrossIndexDependency);
			free(DirectIndexSetDependency);
		}
		
	}
	
	void Clear()
	{
		if(Running)
		{
			memset(CurParameters,  0, sizeof(parameter_value)*Model->FirstUnusedParameterHandle);
			memset(CurInputs,      0, sizeof(double)*Model->FirstUnusedInputHandle);
			memset(CurResults,     0, sizeof(double)*Model->FirstUnusedEquationHandle);
			memset(LastResults,    0, sizeof(double)*Model->FirstUnusedEquationHandle);
			memset(CurrentIndexes, 0, sizeof(index_t)*Model->FirstUnusedIndexSetHandle);
		}
		else
		{
			memset(ParameterDependency, 0, sizeof(size_t)*Model->FirstUnusedParameterHandle);
			memset(InputDependency, 0, sizeof(size_t)*Model->FirstUnusedInputHandle);
			memset(ResultDependency, 0, sizeof(size_t)*Model->FirstUnusedEquationHandle);
			memset(LastResultDependency, 0, sizeof(size_t)*Model->FirstUnusedEquationHandle);
			memset(ResultCrossIndexDependency, 0, sizeof(size_t)*Model->FirstUnusedEquationHandle);
			memset(DirectIndexSetDependency, 0, sizeof(size_t)*Model->FirstUnusedIndexSetHandle);
		}
	}
};


#define GET_ENTITY_NAME(Type, NType) \
inline const char * GetName(inca_model *Model, Type H) \
{ \
	return Model->NType##Specs[H.Handle].Name; \
}

GET_ENTITY_NAME(equation, Equation)
GET_ENTITY_NAME(input, Input)
GET_ENTITY_NAME(parameter_double, Parameter)
GET_ENTITY_NAME(parameter_uint, Parameter)
GET_ENTITY_NAME(parameter_bool, Parameter)
GET_ENTITY_NAME(parameter_ptime, Parameter)
GET_ENTITY_NAME(index_set, IndexSet)
GET_ENTITY_NAME(parameter_group, ParameterGroup)
GET_ENTITY_NAME(solver, Solver)

#undef GET_ENTITY_NAME

inline const char *
GetParameterName(inca_model *Model, handle_t ParameterHandle) //NOTE: In case we don't know the type of the parameter and just want the name.
{
	return Model->ParameterSpecs[ParameterHandle].Name;
}

#define GET_ENTITY_HANDLE(Type, NType, NType2) \
inline Type Get##NType2##Handle(inca_model *Model, const char *Name) \
{ \
	handle_t Handle = 0; \
	auto Find = Model->NType##NameToHandle.find(Name); \
	if(Find != Model->NType##NameToHandle.end()) \
	{ \
		Handle = Find->second; \
	} \
	else \
	{ \
		std::cout << "ERROR: Tried to look up the handle of the " << #NType << " \"" << Name << "\", but it was not registered with the model." << std::endl; \
		exit(0);\
	} \
	return { Handle }; \
}

GET_ENTITY_HANDLE(equation, Equation, Equation)
GET_ENTITY_HANDLE(input, Input, Input)
GET_ENTITY_HANDLE(parameter_double, Parameter, ParameterDouble)
GET_ENTITY_HANDLE(parameter_uint, Parameter, ParameterUInt)
GET_ENTITY_HANDLE(parameter_bool, Parameter, ParameterBool)
GET_ENTITY_HANDLE(parameter_ptime, Parameter, ParameterTime)
GET_ENTITY_HANDLE(index_set, IndexSet, IndexSet)
GET_ENTITY_HANDLE(parameter_group, ParameterGroup, ParameterGroup)
GET_ENTITY_HANDLE(solver, Solver, Solver)

#undef GET_ENTITY_HANDLE

inline handle_t
GetParameterHandle(inca_model *Model, const char *Name) //NOTE: In case we don't know the type of the parameter and just want the handle.
{
	handle_t Handle = 0;
	auto Find = Model->ParameterNameToHandle.find(Name);
	if(Find != Model->ParameterNameToHandle.end())
	{
		Handle = Find->second;
	}
	else
	{
		std::cout << "ERROR: Tried to find the Parameter \"" << Name << "\", but it was not registered with the model." << std::endl;
		exit(0);
	}
	return Handle;
}


#define REGISTRATION_BLOCK(Model) \
if(Model->Finalized) \
{ \
	std::cout << "ERROR: You can not call the function " << __func__ << " on the model after it has been finalized using EndModelDefinition." << std::endl; \
	exit(0); \
}

#define REGISTER_MODEL_ENTITY(Model, NType, Name) \
auto Find = Model->NType##NameToHandle.find(Name); \
if(Find != Model->NType##NameToHandle.end()) \
{ \
	NType.Handle = Find->second; \
} \
else \
{ \
	NType.Handle = Model->FirstUnused##NType##Handle++; \
	Model->NType##NameToHandle[Name] = NType.Handle; \
} \
if(Model->NType##Specs.size() <= NType.Handle) \
{ \
	Model->NType##Specs.resize(NType.Handle + 1, {}); \
} \
Model->NType##Specs[NType.Handle].Name = Name;

inline unit 
RegisterUnit(inca_model *Model, const char *Name = "dimensionless")
{
	REGISTRATION_BLOCK(Model)
	
	unit Unit = {};
	
	REGISTER_MODEL_ENTITY(Model, Unit, Name);
	
	return Unit;
}

inline index_set
RegisterIndexSet(inca_model *Model, const char *Name, index_set_type Type = IndexSetType_Basic)
{
	REGISTRATION_BLOCK(Model)

	index_set IndexSet = {};
	REGISTER_MODEL_ENTITY(Model, IndexSet, Name);
	
	Model->IndexSetSpecs[IndexSet.Handle].Type = Type;
	
	return IndexSet;
}

inline index_set
RegisterIndexSetBranched(inca_model *Model, const char *Name)
{
	REGISTRATION_BLOCK(Model)
	
	index_set IndexSet = RegisterIndexSet(Model, Name, IndexSetType_Branched);
	
	return IndexSet;
}

inline index_t
RequireIndex(inca_model *Model, index_set IndexSet, const char *IndexName)
{
	REGISTRATION_BLOCK(Model)
	
	index_set_spec &Spec = Model->IndexSetSpecs[IndexSet.Handle];
	if(Spec.Type != IndexSetType_Basic)
	{
		//TODO: Get rid of this requirement? However that may lead to issues with index order in branched index sets later.
		std::cout << "ERROR: We only allow requiring indexes for basic index sets, " << Spec.Name << " is of a different type." << std::endl;
	}
	auto Find = std::find(Spec.RequiredIndexes.begin(), Spec.RequiredIndexes.end(), IndexName);
	if(Find != Spec.RequiredIndexes.end())
	{
		return (index_t)std::distance(Spec.RequiredIndexes.begin(), Find);
	}
	else
	{
		Spec.RequiredIndexes.push_back(IndexName);
		return (index_t)(Spec.RequiredIndexes.size() - 1);
	}
}


inline parameter_group
RegisterParameterGroup(inca_model *Model, const char *Name, index_set IndexSet = {0})
{
	REGISTRATION_BLOCK(Model)
	
	parameter_group ParameterGroup = {};
	REGISTER_MODEL_ENTITY(Model, ParameterGroup, Name)
	
	Model->ParameterGroupSpecs[ParameterGroup.Handle].IndexSet = IndexSet;
	
	return ParameterGroup;
}

inline void
SetParentGroup(inca_model *Model, parameter_group Child, parameter_group Parent)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_group_spec &ChildSpec = Model->ParameterGroupSpecs[Child.Handle];
	parameter_group_spec &ParentSpec = Model->ParameterGroupSpecs[Parent.Handle];
	if(IsValid(ChildSpec.ParentGroup) && ChildSpec.ParentGroup.Handle != Parent.Handle)
	{
		std::cout << "WARNING: Setting a parent group for the parameter group " << GetName(Model, Child) << ", but it already has a different parent group.";
	}
	ChildSpec.ParentGroup = Parent;
	ParentSpec.ChildrenGroups.push_back(Child);
}

inline input
RegisterInput(inca_model *Model, const char *Name)
{
	REGISTRATION_BLOCK(Model)
	
	input Input = {};
	REGISTER_MODEL_ENTITY(Model, Input, Name)
	
	return Input;
}

//TODO: store descriptions:

inline parameter_double
RegisterParameterDouble(inca_model *Model, parameter_group Group, const char *Name, unit Unit, double Default, double Min = -DBL_MAX, double Max = DBL_MAX, const char *Description = 0)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_double Parameter = {};
	REGISTER_MODEL_ENTITY(Model, Parameter, Name)
	
	parameter_spec &Spec = Model->ParameterSpecs[Parameter.Handle];
	Spec.Type = ParameterType_Double;
	Spec.Default.ValDouble = Default;
	Spec.Min.ValDouble = Min;
	Spec.Max.ValDouble = Max;
	Spec.Group = Group;
	Spec.Unit = Unit;
	
	Model->ParameterGroupSpecs[Group.Handle].Parameters.push_back(Parameter.Handle);

	return Parameter;
}

inline parameter_uint
RegisterParameterUInt(inca_model *Model, parameter_group Group, const char *Name, unit Unit, u64 Default, u64 Min = 0, u64 Max = 0xffffffffffffffff, const char *Description = 0)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_uint Parameter = {};
	REGISTER_MODEL_ENTITY(Model, Parameter, Name)
	
	parameter_spec &Spec = Model->ParameterSpecs[Parameter.Handle];
	Spec.Type = ParameterType_UInt;
	Spec.Default.ValUInt = Default;
	Spec.Min.ValUInt = Min;
	Spec.Max.ValUInt = Max;
	Spec.Group = Group;
	Spec.Unit = Unit;
	
	Model->ParameterGroupSpecs[Group.Handle].Parameters.push_back(Parameter.Handle);
	
	return Parameter;
}

inline parameter_bool
RegisterParameterBool(inca_model *Model, parameter_group Group, const char *Name, bool Default, const char *Description = 0)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_bool Parameter = {};
	REGISTER_MODEL_ENTITY(Model, Parameter, Name)
	
	parameter_spec &Spec = Model->ParameterSpecs[Parameter.Handle];
	Spec.Type = ParameterType_Bool;
	Spec.Default.ValBool = Default;
	Spec.Min.ValBool = false;
	Spec.Max.ValBool = true;
	Spec.Group = Group;
	
	Model->ParameterGroupSpecs[Group.Handle].Parameters.push_back(Parameter.Handle);
	
	return Parameter;
}

inline parameter_ptime
RegisterParameterDate(inca_model *Model, parameter_group Group, const char *Name, const char *Default, const char *Min = "1900-1-1", const char *Max = "2200-12-31", const char *Description = 0)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_ptime Parameter = {};
	REGISTER_MODEL_ENTITY(Model, Parameter, Name)
	
	parameter_spec &Spec = Model->ParameterSpecs[Parameter.Handle];
	Spec.Type = ParameterType_Time;
	Spec.Default.ValTime = ParseSecondsSinceEpoch(Default);
	Spec.Min.ValTime = ParseSecondsSinceEpoch(Min);
	Spec.Max.ValTime = ParseSecondsSinceEpoch(Max);
	Spec.Group = Group;
	
	Model->ParameterGroupSpecs[Group.Handle].Parameters.push_back(Parameter.Handle);

	return Parameter;
}

inline void
SetEquation(inca_model *Model, equation Equation, inca_equation EquationBody, bool Override = false)
{
	//REGISTRATION_BLOCK(Model) //NOTE: We can't use REGISTRATION_BLOCK since the user don't call the SetEquation explicitly, it is called through the macro EQUATION, and so they would not understand the error message.
	if(Model->Finalized)
	{
		std::cout << "ERROR: You can not define an EQUATION for the model after it has been finalized using EndModelDefinition." << std::endl;
		exit(0);
	}
	
	if(!Override && Model->EquationSpecs[Equation.Handle].EquationIsSet)
	{
		std::cout << "ERROR: The equation body for " << GetName(Model, Equation) << " is already defined. It can not be defined twice unless it is explicitly overridden." << std::endl;
		exit(0);
	}
	
	Model->Equations[Equation.Handle] = EquationBody;

	Model->EquationSpecs[Equation.Handle].EquationIsSet = true;
}

static equation
RegisterEquation(inca_model *Model, const char *Name, unit Unit, equation_type Type = EquationType_Basic)
{
	REGISTRATION_BLOCK(Model)
	
	equation Equation = {};
	REGISTER_MODEL_ENTITY(Model, Equation, Name)
	
	if(Model->Equations.size() <= Equation.Handle)
	{
		Model->Equations.resize(Equation.Handle + 1, {});
	}
	
	Model->EquationSpecs[Equation.Handle].Type = Type;
	Model->EquationSpecs[Equation.Handle].Unit = Unit;
	
	return Equation;
}

inline equation
RegisterEquationODE(inca_model *Model, const char *Name, unit Unit)
{
	REGISTRATION_BLOCK(Model)
	
	return RegisterEquation(Model, Name, Unit, EquationType_ODE);
}

inline equation
RegisterEquationInitialValue(inca_model *Model, const char *Name, unit Unit)
{
	REGISTRATION_BLOCK(Model)
	
	return RegisterEquation(Model, Name, Unit, EquationType_InitialValue);
}

static double CumulateResult(inca_data_set *DataSet, equation Result, index_set CumulateOverIndexSet, index_t *CurrentIndexes, double *LookupBase);

inline equation
RegisterEquationCumulative(inca_model *Model, const char *Name, equation Cumulates, index_set CumulatesOverIndexSet)
{
	REGISTRATION_BLOCK(Model)
	
	//TODO: Should we restrict what kind of equations we should be able to cumulate? For instance, should not be able to cumulate initialvalueequations?
	
	unit Unit = Model->EquationSpecs[Cumulates.Handle].Unit;
	equation Equation = RegisterEquation(Model, Name, Unit, EquationType_Cumulative);
	Model->EquationSpecs[Equation.Handle].CumulatesOverIndexSet = CumulatesOverIndexSet;
	Model->EquationSpecs[Equation.Handle].Cumulates = Cumulates;
	
	SetEquation(Model, Equation,
		[Cumulates, CumulatesOverIndexSet] (value_set_accessor *ValueSet) -> double
		{
			return CumulateResult(ValueSet->DataSet, Cumulates, CumulatesOverIndexSet, ValueSet->CurrentIndexes, ValueSet->AllCurResultsBase);
		}
	);
	
	return Equation;
}

//TODO: Give warnings or errors when setting a initial value on an equation that already has one.
inline void
SetInitialValue(inca_model *Model, equation Equation, parameter_double InitialValue)
{
	REGISTRATION_BLOCK(Model)
	
	Model->EquationSpecs[Equation.Handle].InitialValue = InitialValue;
	
	if(Model->EquationSpecs[Equation.Handle].Unit != Model->ParameterSpecs[InitialValue.Handle].Unit)
	{
		std::cout << "WARNING: The equation " << GetName(Model, Equation) << " was registered with a different unit than its initial value parameter " << GetName(Model, InitialValue) << std::endl;
	}
}

inline void
SetInitialValue(inca_model *Model, equation Equation, double Value)
{
	REGISTRATION_BLOCK(Model)
	
	Model->EquationSpecs[Equation.Handle].ExplicitInitialValue = Value;
	Model->EquationSpecs[Equation.Handle].HasExplicitInitialValue = true;
}

inline void
SetInitialValue(inca_model *Model, equation Equation, equation InitialValueEquation)
{
	REGISTRATION_BLOCK(Model)
	if(Model->EquationSpecs[InitialValueEquation.Handle].Type != EquationType_InitialValue)
	{
		std::cout << "ERROR: Tried to set the equation " << GetName(Model, InitialValueEquation) << " as an initial value of another equation, but it was not registered as an equation of type EquationInitialValue." << std::endl;
		exit(0);
	}
	
	Model->EquationSpecs[Equation.Handle].InitialValueEquation = InitialValueEquation;
	
	if(Model->EquationSpecs[Equation.Handle].Unit != Model->EquationSpecs[InitialValueEquation.Handle].Unit)
	{
		std::cout << "WARNING: The equation " << GetName(Model, Equation) << " was registered with a different unit than its initial value equation " << GetName(Model, InitialValueEquation) << std::endl;
	}
}

static solver
RegisterSolver(inca_model *Model, const char *Name, double h, inca_solver_function *SolverFunction)
{
	REGISTRATION_BLOCK(Model)
	
	solver Solver = {};
	REGISTER_MODEL_ENTITY(Model, Solver, Name)
	
	if(h <= 0.0 || h > 1.0)
	{
		std::cout << "ERROR: The timestep of the solver " << Name << " can not be smaller than 0.0 or larger than 1.0" << std::endl;
		exit(0);
	}
	
	Model->SolverSpecs[Solver.Handle].h = h;
	Model->SolverSpecs[Solver.Handle].SolverFunction = SolverFunction;
	
	return Solver;
}

static void
SetSolver(inca_model *Model, equation Equation, solver Solver)
{
	REGISTRATION_BLOCK(Model)
	
	equation_type Type = Model->EquationSpecs[Equation.Handle].Type;
	if(Type != EquationType_Basic && Type != EquationType_ODE)
	{
		std::cout << "ERROR: Tried to set a solver for the equation " << GetName(Model, Equation) << ", but it is not a basic equation or ODE equation, and so can not be given a solver." << std::endl;
		exit(0);
	}
	Model->EquationSpecs[Equation.Handle].Solver = Solver;
}

inline void
AddInputIndexSetDependency(inca_model *Model, input Input, index_set IndexSet)
{
	REGISTRATION_BLOCK(Model)
	
	input_spec &Spec = Model->InputSpecs[Input.Handle];
	Spec.IndexSetDependencies.push_back(IndexSet);
}


#undef REGISTER_MODEL_ENTITY
#undef REGISTRATION_BLOCK










/////////// IMPORTANT NOTE: ////////////////////
// ##__VA_ARGS__ with double hashes to eliminate superfluous commas may not be supported on all compilers. The alternative __VA_OPT__(,) is not supported until C++20
// Try to detect if this does not work so that the user does not get a barrage of unintelligible error messages?
////////////////////////////////////////////////

////////// IMPORTANT NOTE: /////////////////////
// It may actually be pretty bad to return unsigned ints to the user because it is very easy to get integer underflows when using them. Is there any
// reason not to use signed integers as a parameter type instead of unsigned? (One could just set the min value to 0 if one wants to).
////////////////////////////////////////////////

#define PARAMETER(ParH, ...) (ValueSet__->Running ? GetCurrentParameter(ValueSet__, ParH, ##__VA_ARGS__) : RegisterParameterDependency(ValueSet__, ParH, ##__VA_ARGS__))
#define INPUT(InputH) (ValueSet__->Running ? GetCurrentInput(ValueSet__, InputH) : RegisterInputDependency(ValueSet__, InputH))
#define RESULT(ResultH, ...) (ValueSet__->Running ? GetCurrentResult(ValueSet__, ResultH, ##__VA_ARGS__) : RegisterResultDependency(ValueSet__, ResultH, ##__VA_ARGS__))
#define LAST_RESULT(ResultH, ...) (ValueSet__->Running ? GetLastResult(ValueSet__, ResultH, ##__VA_ARGS__) : RegisterLastResultDependency(ValueSet__, ResultH, ##__VA_ARGS__))
#define EARLIER_RESULT(ResultH, StepBack, ...) (ValueSet__->Running ? GetEarlierResult(ValueSet__, ResultH, (StepBack), ##__VA_ARGS__) : RegisterLastResultDependency(ValueSet__, ResultH, ##__VA_ARGS__)) 

#define CURRENT_DAY_OF_YEAR() (ValueSet__->DayOfYear)
#define DAYS_THIS_YEAR() (ValueSet__->DaysThisYear)

#define EQUATION(Model, ResultH, Def) \
SetEquation(Model, ResultH, \
 [=] (value_set_accessor *ValueSet__) { \
 Def \
 } \
);

#define EQUATION_OVERRIDE(Model, ResultH, Def) \
SetEquation(Model, ResultH, \
 [=] (value_set_accessor *ValueSet__) { \
 Def \
 } \
 , true \
);


//NOTE: These inline functions are used for type safety, which we don't get from macros.
//NOTE: We don't provide direct access to Time parameters since their storage is implemetation dependent. Instead we have accessor macros like CURRENT_DAY_OF_YEAR.
inline double
GetCurrentParameter(value_set_accessor *ValueSet, parameter_double Parameter)
{
	return ValueSet->CurParameters[Parameter.Handle].ValDouble;
}

inline u64
GetCurrentParameter(value_set_accessor *ValueSet, parameter_uint Parameter)
{
	return ValueSet->CurParameters[Parameter.Handle].ValUInt;
}

inline bool
GetCurrentParameter(value_set_accessor *ValueSet, parameter_bool Parameter)
{
	return ValueSet->CurParameters[Parameter.Handle].ValBool;
}

//NOTE: This does NOT do error checking to see if we provided too many override indexes or if they were out of bounds! We could maybe add an option to do that
// that is compile-out-able?
size_t OffsetForHandle(storage_structure &Structure, const index_t* CurrentIndexes, const size_t *IndexCounts, const size_t *OverrideIndexes, size_t OverrideCount, handle_t Handle);


template<typename... T> double
GetCurrentParameter(value_set_accessor *ValueSet, parameter_double Parameter, T... Indexes)
{
	inca_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Parameter.Handle);
	return DataSet->ParameterData[Offset].ValDouble;
}

template<typename... T> u64
GetCurrentParameter(value_set_accessor *ValueSet, parameter_uint Parameter, T... Indexes)
{
	inca_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Parameter.Handle);
	return DataSet->ParameterData[Offset].ValUInt;
}

template<typename... T> bool
GetCurrentParameter(value_set_accessor *ValueSet, parameter_bool Parameter, T... Indexes)
{
	inca_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Parameter.Handle);
	return DataSet->ParameterData[Offset].ValBool;
}


inline double
GetCurrentResult(value_set_accessor *ValueSet, equation Result)
{
	return ValueSet->CurResults[Result.Handle];
}

inline double 
GetLastResult(value_set_accessor *ValueSet, equation LastResult)
{
	return ValueSet->LastResults[LastResult.Handle];
}

template<typename... T> double
GetCurrentResult(value_set_accessor *ValueSet, equation Result, T... Indexes)
{
	inca_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result.Handle);
	return ValueSet->AllCurResultsBase[Offset];
}

template<typename... T> double
GetLastResult(value_set_accessor *ValueSet, equation Result, T... Indexes)
{
	inca_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};

	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result.Handle);
	return ValueSet->AllLastResultsBase[Offset];
}

template<typename... T> double
GetEarlierResult(value_set_accessor *ValueSet, equation Result, u64 StepBack, T...Indexes)
{
	inca_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result.Handle);
	
	//TODO: Make proper accessor for this that belongs to inca_data_set.cpp
	double *Initial = DataSet->ResultData + Offset;
	//NOTE: Initial points to the initial value (adding TotalCount once gives us timestep 0)
	if(StepBack > ValueSet->Timestep)
	{
		return *Initial;
	}
	return *(Initial + ( (ValueSet->Timestep+1) - StepBack)*(ValueSet->DataSet->ResultStorageStructure.TotalCount));
}

inline double
GetCurrentInput(value_set_accessor *ValueSet, input Input)
{
	return ValueSet->CurInputs[Input.Handle];
}




inline double
RegisterParameterDependency(value_set_accessor *ValueSet, parameter_double Parameter)
{
	ValueSet->ParameterDependency[Parameter.Handle]++;
	return 0.0;
}

template<typename... T> double
RegisterParameterDependency(value_set_accessor *ValueSet, parameter_double Parameter, T... Indexes)
{
	//TODO: @ImplementMe!
	
	return 0.0;
}

inline u64
RegisterParameterDependency(value_set_accessor *ValueSet, parameter_uint Parameter)
{
	ValueSet->ParameterDependency[Parameter.Handle]++;
	return 0;
}

template<typename... T> double
RegisterParameterDependency(value_set_accessor *ValueSet, parameter_uint Parameter, T... Indexes)
{
	//TODO: @ImplementMe!
	
	return 0.0;
}

inline bool
RegisterParameterDependency(value_set_accessor *ValueSet, parameter_bool Parameter)
{
	ValueSet->ParameterDependency[Parameter.Handle]++;
	return false;
}

template<typename... T> double
RegisterParameterDependency(value_set_accessor *ValueSet, parameter_bool Parameter, T... Indexes)
{
	//TODO: @ImplementMe!
	
	return 0.0;
}

inline double
RegisterInputDependency(value_set_accessor *ValueSet, input Input)
{
	ValueSet->InputDependency[Input.Handle]++;
	return 0.0;
}

inline double
RegisterResultDependency(value_set_accessor *ValueSet, equation Result)
{
	ValueSet->ResultDependency[Result.Handle]++;
	return 0.0;
}

template<typename... T> double
RegisterResultDependency(value_set_accessor *ValueSet, equation Result, T... Indexes)
{
	//TODO: May need to do more stuff here
	ValueSet->ResultCrossIndexDependency[Result.Handle]++;
	
	return 0.0;
}

inline double
RegisterLastResultDependency(value_set_accessor *ValueSet, equation Result)
{
	ValueSet->LastResultDependency[Result.Handle]++;
	return 0.0;
}

template<typename... T> double
RegisterLastResultDependency(value_set_accessor *ValueSet, equation Result, T... Indexes)
{
	//TODO: @ImplementMe!
	
	return 0.0;
}

//TODO: SET_RESULT is not that nice, and can interfere with how the dependency system works if used incorrectly. It was only included to get Persist to work. However, Persist should probably be rewritten to remove that necessity.
#define SET_RESULT(ResultH, Value, ...) {if(ValueSet__->Running){SetResult(ValueSet__, Value, ResultH, ##__VA_ARGS__);}}

template<typename... T> void
SetResult(value_set_accessor *ValueSet, double Value, equation Result, T... Indexes)
{
	inca_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result.Handle);
	ValueSet->AllCurResultsBase[Offset] = Value;
}


#define INDEX_COUNT(IndexSetH) (ValueSet__->Running ? (ValueSet__->DataSet->IndexCounts[IndexSetH.Handle]) : 1)
#define CURRENT_INDEX(IndexSetH) (ValueSet__->Running ? GetCurrentIndex(ValueSet__, IndexSetH) : RegisterIndexSetDependency(ValueSet__, IndexSetH))
#define PREVIOUS_INDEX(IndexSetH) (ValueSet__->Running ? GetPreviousIndex(ValueSet__, IndexSetH) : RegisterIndexSetDependency(ValueSet__, IndexSetH))
#define FINAL_INDEX(IndexSetH) (INDEX_COUNT(IndexSetH)-1)
#define INPUT_COUNT(IndexSetH) (ValueSet__->Running ? InputCount(ValueSet__, IndexSetH) : 0)

inline index_t
RegisterIndexSetDependency(value_set_accessor *ValueSet, index_set IndexSet)
{
	ValueSet->DirectIndexSetDependency[IndexSet.Handle]++;
	return 0;
}

inline index_t
GetCurrentIndex(value_set_accessor *ValueSet, index_set IndexSet)
{
	return ValueSet->CurrentIndexes[IndexSet.Handle];
}

inline index_t
GetPreviousIndex(value_set_accessor *ValueSet, index_set IndexSet)
{
	index_t CurrentIndex = GetCurrentIndex(ValueSet, IndexSet);
	if(CurrentIndex == 0) return 0;
	return CurrentIndex - 1;
}

inline size_t
InputCount(value_set_accessor *ValueSet, index_set IndexSet)
{
	index_t Current = GetCurrentIndex(ValueSet, IndexSet);
	return ValueSet->DataSet->BranchInputs[IndexSet.Handle][Current].Count;
}

inline size_t
BranchInputIteratorEnd(value_set_accessor *ValueSet, index_set IndexSet, index_t Branch)
{
	return ValueSet->Running ? ValueSet->DataSet->BranchInputs[IndexSet.Handle][Branch].Count : 1; //NOTE: The count is always one greater than the largest index, so it is guaranteed to be invalid.
}

struct branch_input_iterator
{
	index_set IndexSet;
	index_t *InputIndexes;
	size_t CurrentInputIndexIndex;
	
	void operator++() { CurrentInputIndexIndex++; }
	
	const index_t& operator*(){ return InputIndexes[CurrentInputIndexIndex]; }

	bool operator!=(const size_t& Idx) { return CurrentInputIndexIndex != Idx; }
	
};

index_t DummyData = 0; //TODO: Ugh! get rid of this global

inline branch_input_iterator
BranchInputIteratorBegin(value_set_accessor *ValueSet, index_set IndexSet, index_t Branch)
{
	branch_input_iterator Iterator;
	Iterator.IndexSet = IndexSet;
	Iterator.InputIndexes = ValueSet->Running ? ValueSet->DataSet->BranchInputs[IndexSet.Handle][Branch].Inputs : &DummyData;
	Iterator.CurrentInputIndexIndex = 0;
	return Iterator;
}

#define BRANCH_INPUT_BEGIN(IndexSetH) (BranchInputIteratorBegin(ValueSet__, IndexSetH, CURRENT_INDEX(IndexSetH)))
#define BRANCH_INPUT_END(IndexSetH) (BranchInputIteratorEnd(ValueSet__, IndexSetH, CURRENT_INDEX(IndexSetH)))

#define FOREACH_INPUT(IndexSetH, Body) \
for(auto Input = BRANCH_INPUT_BEGIN(IndexSetH); Input != BRANCH_INPUT_END(IndexSetH); ++Input) \
{ \
Body \
}

#define INCA_MODEL_H
#endif
