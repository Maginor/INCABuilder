
#include "../inca.h"


#define DLLEXPORT extern "C" __declspec(dllexport)

DLLEXPORT void
RunModel(void *DataSet)
{
	RunModel((inca_data_set *)DataSet);
}

DLLEXPORT u64
GetTimesteps(void *DataSet)
{
	return GetTimesteps((inca_data_set *)DataSet);
}

DLLEXPORT
GetResultSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *WriteTo)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	
	size_t Timesteps = GetTimesteps(DataSet);
	
	GetResultSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps);
}

DLLEXPORT
GetInputSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *WriteTo, bool AlignWithResults)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	
	size_t Timesteps = GetTimesteps(DataSet);
	if(!AlignWithResults && DataSet->InputDataHasSeparateStartDate)
	{
		Timesteps = DataSet->InputDataTimesteps;
	}
	
	GetInputSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps, AlignWithResults);
}

DLLEXPORT
SetParameterDouble(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double Val)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValDouble = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Double);
}