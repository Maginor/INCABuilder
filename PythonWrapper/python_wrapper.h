
#include "../inca.h"


#define DLLEXPORT extern "C" __declspec(dllexport)

DLLEXPORT void
DllRunModel(void *DataSetPtr)
{
	RunModel((inca_data_set *)DataSetPtr);
}

DLLEXPORT void *
DllCopyDataSet(void *DataSetPtr)
{
	return (void *)CopyDataSet((inca_data_set *)DataSetPtr);
}

DLLEXPORT void
DllDeleteDataSet(void *DataSetPtr)
{
	delete (inca_data_set *)DataSetPtr;
}

DLLEXPORT u64
DllGetTimesteps(void *DataSetPtr)
{
	return GetTimesteps((inca_data_set *)DataSetPtr);
}

DLLEXPORT u64
DllGetInputTimesteps(void *DataSetPtr)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	return DataSet->InputDataTimesteps;
}

DLLEXPORT void
DllGetResultSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *WriteTo)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	
	size_t Timesteps = GetTimesteps(DataSet);
	
	GetResultSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps);
}

DLLEXPORT void
DllGetInputSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *WriteTo, bool AlignWithResults)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	
	size_t Timesteps = GetTimesteps(DataSet);
	if(!AlignWithResults)
	{
		Timesteps = DataSet->InputDataTimesteps;
	}
	
	GetInputSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps, AlignWithResults);
}

DLLEXPORT void
DllSetParameterDouble(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double Val)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValDouble = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Double);
}

DLLEXPORT void
DllSetParameterUInt(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, u64 Val)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValUInt = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_UInt);
}

DLLEXPORT void
DllSetParameterBool(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, bool Val)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValBool = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Bool);
}

DLLEXPORT void
DllSetParameterTime(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, char *Val)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValTime = ParseSecondsSinceEpoch(Val);
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Time);
}

DLLEXPORT double
DllGetParameterDouble(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	return GetParameterValue((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Double).ValDouble;
}

DLLEXPORT u64
DllGetParameterUInt(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	return GetParameterValue((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_UInt).ValUInt;
}

DLLEXPORT bool
DllGetParameterBool(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	return GetParameterValue((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Bool).ValBool;
}

DLLEXPORT void
DllGetParameterTime(void *DataSetPtr, const char *Name, char **IndexNames, u64 IndexCount, char *WriteTo)
{
	//IMPORTANT: This assumes that WriteTo is a char* buffer that is long enough (i.e. at least 11-12-ish bytes)
	//IMPORTANT: This is NOT thread safe since TimeString is not thread safe.
	
	s64 SecondsSinceEpoch = GetParameterValue((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Time).ValTime;
	char *TimeStr = TimeString(SecondsSinceEpoch);
	strcpy(WriteTo, TimeStr);
}

DLLEXPORT void
DllGetInputStartDate(void *DataSetPtr, char *WriteTo)
{
	//IMPORTANT: This assumes that WriteTo is a char* buffer that is long enough (i.e. at least 11-12-ish bytes)
	//IMPORTANT: This is NOT thread safe since TimeString is not thread safe.
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	
	if(DataSet->InputDataHasSeparateStartDate)
	{
		s64 SecondsSinceEpoch = DataSet->InputDataStartDate;
		char *TimeStr = TimeString(SecondsSinceEpoch);
		strcpy(WriteTo, TimeStr);
	}
	else
	{
		DllGetParameterTime(DataSetPtr, "Start date", 0, 0, WriteTo);
	}
}

DLLEXPORT void
DllWriteParametersToFile(void *DataSetPtr, char *Filename)
{
	WriteParametersToFile((inca_data_set *)DataSetPtr, (const char *)Filename);
}

DLLEXPORT void
DllSetInputSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *InputData, u64 InputDataLength)
{
	SetInputSeries((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, InputData, (size_t)InputDataLength);
}