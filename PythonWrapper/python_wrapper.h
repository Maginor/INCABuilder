
#include "../inca.h"


#define DLLEXPORT extern "C" __declspec(dllexport)

DLLEXPORT void
RunModel(void *DataSetPtr)
{
	RunModel((inca_data_set *)DataSetPtr);
}

DLLEXPORT void *
CopyDataSet(void *DataSetPtr)
{
	return (void *)CopyDataSet((inca_data_set *)DataSetPtr);
}

DLLEXPORT void
DeleteDataSet(void *DataSetPtr)
{
	delete (inca_data_set *)DataSetPtr;
}

DLLEXPORT u64
GetTimesteps(void *DataSetPtr)
{
	return GetTimesteps((inca_data_set *)DataSetPtr);
}

DLLEXPORT u64
GetInputTimesteps(void *DataSetPtr)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	return DataSet->InputDataTimesteps;
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
	if(!AlignWithResults)
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

DLLEXPORT
SetParameterUInt(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, u64 Val)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValUInt = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_UInt);
}

DLLEXPORT
SetParameterBool(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, bool Val)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValBool = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Bool);
}


DLLEXPORT
SetParameterTime(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, char *Val)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValTime = ParseSecondsSinceEpoch(Val);
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Time);
}