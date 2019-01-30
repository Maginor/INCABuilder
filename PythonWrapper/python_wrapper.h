
#include <sstream>

static int Dll_GlobalErrorCode = 0;
std::stringstream _Errstream;

#define INCA_PARTIAL_ERROR(Msg) \
	_Errstream << Msg; \
	Dll_GlobalErrorCode = 1;
	
#define INCA_FATAL_ERROR(Msg) \
	INCA_PARTIAL_ERROR(Msg) \
	throw 0;

#define CHECK_ERROR_BEGIN \
try {

#define CHECK_ERROR_END \
} catch(int Errcode) { \
}	
	

#include "../inca.h"


#define DLLEXPORT extern "C" __declspec(dllexport)

DLLEXPORT int
DllEncounteredError(char *ErrmsgOut)
{
	std::string ErrStr = _Errstream.str();
	strcpy(ErrmsgOut, ErrStr.data());
	
	int Code = Dll_GlobalErrorCode;
	
	//NOTE: Since Jupyter does not seem to reload the dll when you restart it (normally), we have to clear this.
	Dll_GlobalErrorCode = 0;
	_Errstream.clear();
	
	return Code;
}

DLLEXPORT void
DllRunModel(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	RunModel((inca_data_set *)DataSetPtr);
	
	CHECK_ERROR_END
}

DLLEXPORT void *
DllCopyDataSet(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	return (void *)CopyDataSet((inca_data_set *)DataSetPtr);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllDeleteDataSet(void *DataSetPtr)
{
	delete (inca_data_set *)DataSetPtr;
}

DLLEXPORT u64
DllGetTimesteps(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	return GetTimesteps((inca_data_set *)DataSetPtr);
	
	CHECK_ERROR_END
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
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	
	size_t Timesteps = GetTimesteps(DataSet);
	
	GetResultSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetInputSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *WriteTo, bool AlignWithResults)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	
	size_t Timesteps = GetTimesteps(DataSet);
	if(!AlignWithResults)
	{
		Timesteps = DataSet->InputDataTimesteps;
	}
	
	GetInputSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps, AlignWithResults);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterDouble(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double Val)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValDouble = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Double);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterUInt(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, u64 Val)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValUInt = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_UInt);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterBool(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, bool Val)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValBool = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Bool);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterTime(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, char *Val)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	parameter_value Value;
	bool ParseSuccess = ParseSecondsSinceEpoch(Val, &Value.ValTime);
	if(!ParseSuccess)
	{
		INCA_FATAL_ERROR("ERROR: Unrecognized date format provided for the value of the parameter " << Name << std::endl)
	}
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Time);
	
	CHECK_ERROR_END
}

DLLEXPORT double
DllGetParameterDouble(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return GetParameterValue((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Double).ValDouble;
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetParameterUInt(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return GetParameterValue((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_UInt).ValUInt;
	
	CHECK_ERROR_END
}

DLLEXPORT bool
DllGetParameterBool(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return GetParameterValue((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Bool).ValBool;
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetParameterTime(void *DataSetPtr, const char *Name, char **IndexNames, u64 IndexCount, char *WriteTo)
{
	//IMPORTANT: This assumes that WriteTo is a char* buffer that is long enough (i.e. at least 11-12-ish bytes)
	//IMPORTANT: This is NOT thread safe since TimeString is not thread safe.
	CHECK_ERROR_BEGIN
	
	s64 SecondsSinceEpoch = GetParameterValue((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Time).ValTime;
	char *TimeStr = TimeString(SecondsSinceEpoch);
	strcpy(WriteTo, TimeStr);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetParameterDoubleMinMax(void *DataSetPtr, char *Name, double *MinOut, double *MaxOut)
{
	CHECK_ERROR_BEGIN
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
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetParameterUIntMinMax(void *DataSetPtr, char *Name, u64 *MinOut, u64 *MaxOut)
{
	CHECK_ERROR_BEGIN
	
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
	
	CHECK_ERROR_END
}

DLLEXPORT const char *
DllGetParameterDescription(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	return Spec.Description;
	
	CHECK_ERROR_END
}

DLLEXPORT const char *
DllGetParameterUnit(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	return GetName(DataSet->Model, Spec.Unit);
	
	CHECK_ERROR_END
}

DLLEXPORT const char *
DllGetResultUnit(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	equation_h Equation = GetEquationHandle(DataSet->Model, Name);
	const equation_spec &Spec = DataSet->Model->EquationSpecs[Equation.Handle];
	return GetName(DataSet->Model, Spec.Unit);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetInputStartDate(void *DataSetPtr, char *WriteTo)
{
	CHECK_ERROR_BEGIN
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
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllWriteParametersToFile(void *DataSetPtr, char *Filename)
{
	CHECK_ERROR_BEGIN
	
	WriteParametersToFile((inca_data_set *)DataSetPtr, (const char *)Filename);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetInputSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *InputData, u64 InputDataLength)
{
	CHECK_ERROR_BEGIN
	
	SetInputSeries((inca_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, InputData, (size_t)InputDataLength);
	
	CHECK_ERROR_END
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
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	const inca_model *Model = DataSet->Model;
	for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		NamesOut[IndexSetHandle - 1] = GetName(Model, index_set_h {IndexSetHandle});
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetIndexCount(void *DataSetPtr, char *IndexSetName)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	return DataSet->IndexCounts[IndexSet.Handle];
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetIndexes(void *DataSetPtr, char *IndexSetName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	for(size_t IdxIdx = 0; IdxIdx < DataSet->IndexCounts[IndexSet.Handle]; ++IdxIdx)
	{
		NamesOut[IdxIdx] = DataSet->IndexNames[IndexSet.Handle][IdxIdx];
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetParameterIndexSetsCount(void *DataSetPtr, char *ParameterName)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle ParameterHandle = GetParameterHandle(DataSet->Model, ParameterName);
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	return DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets.size();
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetParameterIndexSets(void *DataSetPtr, char *ParameterName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	entity_handle ParameterHandle = GetParameterHandle(DataSet->Model, ParameterName);
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetResultIndexSetsCount(void *DataSetPtr, char *ResultName)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	equation_h Equation = GetEquationHandle(DataSet->Model, ResultName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	return DataSet->ResultStorageStructure.Units[UnitIndex].IndexSets.size();
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetResultIndexSets(void *DataSetPtr, char *ResultName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	equation_h Equation = GetEquationHandle(DataSet->Model, ResultName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->ResultStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetInputIndexSetsCount(void *DataSetPtr, char *InputName)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	input_h Input = GetInputHandle(DataSet->Model, InputName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Input.Handle];
	return DataSet->InputStorageStructure.Units[UnitIndex].IndexSets.size();
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetInputIndexSets(void *DataSetPtr, char *InputName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	input_h Input = GetInputHandle(DataSet->Model, InputName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Input.Handle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->InputStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetAllParametersCount(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	return (u64)(DataSet->Model->FirstUnusedParameterHandle - 1);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetAllParameters(void *DataSetPtr, const char **NamesOut, const char **TypesOut)
{
	CHECK_ERROR_BEGIN
	
	inca_data_set *DataSet = (inca_data_set *)DataSetPtr;
	for(size_t Idx = 0; Idx < DataSet->Model->FirstUnusedParameterHandle - 1; ++Idx)
	{
		entity_handle Handle = Idx + 1;
		const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
		NamesOut[Idx] = Spec.Name;
		TypesOut[Idx] = GetParameterTypeName(Spec.Type);
	}
	
	CHECK_ERROR_END
}


