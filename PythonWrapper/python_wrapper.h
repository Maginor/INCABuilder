
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
DllGetParameterDoubleMinMax(void *DataSetPtr, char *Name, double *MinOut, double *MaxOut)
{
	//TODO: We don't check that the requested parameter was of type double!
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	if(Spec.Type != ParameterType_Double)
	{
		std::cout << "ERROR: Requested the min and max values of " << Name << " using DllGetParameterDoubleMinMax, but it is not of type double." << std::endl;
		exit(0);
	}
	*MinOut = Spec.Min.ValDouble;
	*MaxOut = Spec.Max.ValDouble;
}

DLLEXPORT void
DllGetParameterUIntMinMax(void *DataSetPtr, char *Name, u64 *MinOut, u64 *MaxOut)
{
	//TODO: We don't check that the requested parameter was of type uint!
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	if(Spec.Type != ParameterType_UInt)
	{
		std::cout << "ERROR: Requested the min and max values of " << Name << " using DllGetParameterUIntMinMax, but it is not of type uint." << std::endl;
		exit(0);
	}
	*MinOut = Spec.Min.ValUInt;
	*MaxOut = Spec.Max.ValUInt;
}

DLLEXPORT const char *
DllGetParameterDescription(void *DataSetPtr, char *Name)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	return Spec.Description;
}

DLLEXPORT const char *
DllGetParameterUnit(void *DataSetPtr, char *Name)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	return GetName(DataSet->Model, Spec.Unit);
}

DLLEXPORT const char *
DllGetResultUnit(void *DataSetPtr, char *Name)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	equation_h Equation = GetEquationHandle(DataSet->Model, Name);
	const equation_spec &Spec = DataSet->Model->EquationSpecs[Equation.Handle];
	return GetName(DataSet->Model, Spec.Unit);
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


//TODO: Some of the following should be wrapped into accessors in inca_data_set.cpp, because we are creating too many dependencies on implementation details here:

DLLEXPORT u64
DllGetIndexSetsCount(void *DataSetPtr)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	return DataSet->Model->FirstUnusedIndexSetHandle - 1;
}

DLLEXPORT void
DllGetIndexSets(void *DataSetPtr, const char **NamesOut)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	const inca_model *Model = DataSet->Model;
	for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		NamesOut[IndexSetHandle - 1] = GetName(Model, index_set_h {IndexSetHandle});
	}
}

DLLEXPORT u64
DllGetIndexCount(void *DataSetPtr, char *IndexSetName)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	return DataSet->IndexCounts[IndexSet.Handle];
}

DLLEXPORT void
DllGetIndexes(void *DataSetPtr, char *IndexSetName, const char **NamesOut)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	for(size_t IdxIdx = 0; IdxIdx < DataSet->IndexCounts[IndexSet.Handle]; ++IdxIdx)
	{
		NamesOut[IdxIdx] = DataSet->IndexNames[IndexSet.Handle][IdxIdx];
	}
}

DLLEXPORT u64
DllGetParameterIndexSetsCount(void *DataSetPtr, char *ParameterName)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle ParameterHandle = GetParameterHandle(DataSet->Model, ParameterName);
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	return DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets.size();
}

DLLEXPORT void
DllGetParameterIndexSets(void *DataSetPtr, char *ParameterName, const char **NamesOut)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle ParameterHandle = GetParameterHandle(DataSet->Model, ParameterName);
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
}

DLLEXPORT u64
DllGetResultIndexSetsCount(void *DataSetPtr, char *ResultName)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	equation_h Equation = GetEquationHandle(DataSet->Model, ResultName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	return DataSet->ResultStorageStructure.Units[UnitIndex].IndexSets.size();
}

DLLEXPORT void
DllGetResultIndexSets(void *DataSetPtr, char *ResultName, const char **NamesOut)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	equation_h Equation = GetEquationHandle(DataSet->Model, ResultName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->ResultStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
}

DLLEXPORT u64
DllGetInputIndexSetsCount(void *DataSetPtr, char *InputName)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	input_h Input = GetInputHandle(DataSet->Model, InputName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Input.Handle];
	return DataSet->InputStorageStructure.Units[UnitIndex].IndexSets.size();
}

DLLEXPORT void
DllGetInputIndexSets(void *DataSetPtr, char *InputName, const char **NamesOut)
{
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	input_h Input = GetInputHandle(DataSet->Model, InputName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Input.Handle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->InputStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
}
