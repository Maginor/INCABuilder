


static inca_data_set *
GenerateDataSet(inca_model *Model)
{
	if(!Model->Finalized)
	{
		std::cout << "ERROR: Attempted to generate a data set before the model was finalized using an EndModelDefinition call." << std::endl;
		return 0;
	}
	inca_data_set *DataSet = new inca_data_set {};
	
	DataSet->Model = Model;
	
	DataSet->IndexCounts = AllocClearedArray(index_t, Model->FirstUnusedIndexSetHandle);
	DataSet->IndexCounts[0] = 1;
	DataSet->IndexNames = AllocClearedArray(const char **, Model->FirstUnusedIndexSetHandle);
	DataSet->IndexNamesToHandle.resize(Model->FirstUnusedIndexSetHandle, {});
	
	DataSet->BranchInputs = AllocClearedArray(branch_inputs *, Model->FirstUnusedIndexSetHandle);
	
	return DataSet;
}

static void
SetupStorageStructureSpecifer(storage_structure &Structure, size_t *IndexCounts, size_t FirstUnusedHandle)
{
	size_t UnitCount = Structure.Units.size();
	Structure.TotalCountForUnit = AllocClearedArray(size_t, UnitCount);
	Structure.OffsetForUnit     = AllocClearedArray(size_t, UnitCount);
	Structure.UnitForHandle     = AllocClearedArray(size_t, FirstUnusedHandle);
	Structure.LocationOfHandleInUnit = AllocClearedArray(size_t, FirstUnusedHandle);
	Structure.TotalCount = 0;
	
	size_t UnitIndex = 0;
	size_t OffsetForUnitSoFar = 0;
	for(storage_unit_specifier &Unit : Structure.Units)
	{
		Structure.TotalCountForUnit[UnitIndex] = Unit.Handles.size();
		for(index_set IndexSet : Unit.IndexSets)
		{
			Structure.TotalCountForUnit[UnitIndex] *= IndexCounts[IndexSet.Handle];
		}
		
		size_t HandleIdx = 0;
		for(handle_t Handle : Unit.Handles)
		{
			Structure.UnitForHandle[Handle] = UnitIndex;
			Structure.LocationOfHandleInUnit[Handle] = HandleIdx;
			++HandleIdx;
		}
		
		Structure.OffsetForUnit[UnitIndex] = OffsetForUnitSoFar;
		OffsetForUnitSoFar += Structure.TotalCountForUnit[UnitIndex];
		Structure.TotalCount += Structure.TotalCountForUnit[UnitIndex];
		++UnitIndex;
	}
}

//NOTE: The following access methods are very similar, but it would incur a performance penalty to try to merge them.
//TODO: Document them properly!!!!!

inline size_t
OffsetForHandle(storage_structure &Structure, handle_t Handle)
{
	//NOTE: returns an offset to the first instance for this handle.
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	return OffsetForUnit + LocationOfHandleInUnit;
}

static size_t
OffsetForHandle(storage_structure &Structure, const index_t *CurrentIndexes, const size_t *IndexCounts, handle_t Handle)
{
	std::vector<storage_unit_specifier> &Units = Structure.Units;
	
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	storage_unit_specifier &Specifier = Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.size();
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	size_t InstanceOffset = 0;
	for(index_set IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index = CurrentIndexes[IndexSet.Handle];
		
		InstanceOffset = InstanceOffset * Count + Index;
	}
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}

static size_t
OffsetForHandle(storage_structure &Structure, const index_t *Indexes, size_t IndexesCount, const size_t *IndexCounts, handle_t Handle)
{
	std::vector<storage_unit_specifier> &Units = Structure.Units;
	
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	storage_unit_specifier &Specifier = Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.size();
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	size_t InstanceOffset = 0;
	size_t Level = 0;
	for(index_set IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index = Indexes[Level];
		
		InstanceOffset = InstanceOffset * Count + Index;
		
		++Level;
	}
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}

//NOTE: Same as above, but overrides the OverrideCount last indexes
inline size_t
OffsetForHandle(storage_structure &Structure, const index_t* CurrentIndexes, const size_t *IndexCounts, const size_t *OverrideIndexes, size_t OverrideCount, handle_t Handle)
{
	std::vector<storage_unit_specifier> &Units = Structure.Units;
	
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	storage_unit_specifier &Specifier = Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.size();
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	size_t IndexSetCount = Specifier.IndexSets.size();
	size_t InstanceOffset = 0;
	size_t IndexSetLevel = 0;
	for(index_set IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index;
		if(IndexSetLevel < (IndexSetCount - OverrideCount))
		{
			Index = CurrentIndexes[IndexSet.Handle];
		}
		else
		{
			Index = OverrideIndexes[IndexSetLevel + (OverrideCount - IndexSetCount)];
		}
		
		InstanceOffset = InstanceOffset * Count + Index;
		++IndexSetLevel;
	}
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}

//NOTE: Similar to above, but instead overrides the index in a specific index set.

static size_t
OffsetForHandle(storage_structure &Structure, index_t *CurrentIndexes, size_t *IndexCounts, index_set Skip, size_t& SubsequentOffset, handle_t Handle)
{
	std::vector<storage_unit_specifier> &Units = Structure.Units;
	
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	storage_unit_specifier &Specifier = Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.size();
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	size_t InstanceOffset = 0;
	SubsequentOffset = 1;
	bool Skipped = false;
	for(index_set IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index = CurrentIndexes[IndexSet.Handle];
		if(Skipped)
		{
			SubsequentOffset *= Count;
		}
		if(IndexSet == Skip)
		{
			Index = 0;
			Skipped = true;
		}
		
		InstanceOffset = InstanceOffset * Count + Index;
	}
	SubsequentOffset *= NumHandlesInUnitInstance;
	
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}


static void
SetIndexes(inca_data_set *DataSet, const char* IndexSetName, const std::vector<const char *>& IndexNames)
{
	inca_model *Model = DataSet->Model;
	
	handle_t IndexSetHandle = GetIndexSetHandle(DataSet->Model, IndexSetName).Handle;
	index_set_spec &Spec = Model->IndexSetSpecs[IndexSetHandle];
	
	if(Spec.Type != IndexSetType_Basic)
	{
		std::cout << "ERROR: Can not use the method SetIndexes for the index set " << Spec.Name << ", use a method that is specific to the type of that index set instead." << std::endl;
		exit(0);
	}
	
	if(DataSet->IndexNames[IndexSetHandle] != 0)
	{
		std::cout << "ERROR: Tried to set the indexes for the index set " << Spec.Name << " more than once." << std::endl;
		exit(0);
	}
	
	if(IndexNames.empty())
	{
		std::cout << "ERROR: Tried to set indexes for the index set " << Spec.Name << ", but no indexes were provided" << std::endl;
		exit(0);
	}
	
	if(!Spec.RequiredIndexes.empty())
	{
		bool Correct = true;
		if(Spec.RequiredIndexes.size() > IndexNames.size()) Correct = false;
		else
		{
			for(size_t IdxIdx = 0; IdxIdx < Spec.RequiredIndexes.size(); ++IdxIdx)
			{
				if(strcmp(Spec.RequiredIndexes[IdxIdx], IndexNames[IdxIdx]) != 0)
				{
					Correct = false;
					break;
				}
			}
		}

		if(!Correct)
		{
			std::cout << "ERROR: The model requires the following indexes to be the first indexes for the index set " << Spec.Name << ":" << std::endl;
			for(const char *IndexName :  Spec.RequiredIndexes) std::cout << "\"" << IndexName << "\" ";
			std::cout << std::endl << "in that order. We got the indexes: " << std::endl;
			for(const char *IndexName : IndexNames)  std::cout << "\"" << IndexName << "\" ";
			std::cout << std::endl;
			exit(0);
		}
	}
	
	DataSet->IndexCounts[IndexSetHandle] = IndexNames.size();
	DataSet->IndexNames[IndexSetHandle] = AllocClearedArray(const char *, IndexNames.size());
	
	for(size_t IndexIndex = 0; IndexIndex < IndexNames.size(); ++IndexIndex)
	{
		const char *IndexName = CopyString(IndexNames[IndexIndex]); //NOTE: Leaks unless we free it.
		DataSet->IndexNames[IndexSetHandle][IndexIndex] = IndexName;
		DataSet->IndexNamesToHandle[IndexSetHandle][IndexName] = IndexIndex;
	}
	
	if(DataSet->IndexNamesToHandle[IndexSetHandle].size() != IndexNames.size())
	{
		std::cout << "ERROR: Got duplicate indexes for index set " << Spec.Name << std::endl;
		exit(0);
	}
	
	bool AllSet = true;
	for(size_t IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		if(DataSet->IndexCounts[IndexSetHandle] == 0)
		{
			AllSet = false;
			break;
		}
	}
	DataSet->AllIndexesHaveBeenSet = AllSet;
}

static void
SetBranchIndexes(inca_data_set *DataSet, const char *IndexSetName, const std::vector<std::pair<const char *, std::vector<const char *>>>& Inputs)
{
	inca_model *Model = DataSet->Model;
	
	handle_t IndexSetHandle = GetIndexSetHandle(Model, IndexSetName).Handle;
	index_set_spec &Spec = Model->IndexSetSpecs[IndexSetHandle];
	
	if(Spec.Type != IndexSetType_Branched)
	{
		std::cout << "ERROR: Can not use the method SetBranchIndexes for the index set " << IndexSetName << ", use a method that is specific to the type of that index set instead." << std::endl;
		exit(0);
	}
	
	if(DataSet->IndexNames[IndexSetHandle] != 0)
	{
		std::cout << "ERROR: Tried to set the indexes for the index set " << Spec.Name << " more than once." << std::endl;
	}
	
	if(Inputs.empty())
	{
		std::cout << "ERROR: Tried to set indexes for the index set " << Spec.Name << ", but no indexes were provided" << std::endl;
		exit(0);
	}
	
	DataSet->IndexCounts[IndexSetHandle] = Inputs.size();
	DataSet->IndexNames[IndexSetHandle] = AllocClearedArray(const char *, Inputs.size());

	DataSet->BranchInputs[IndexSetHandle] = AllocClearedArray(branch_inputs, Inputs.size());
	index_t IndexIndex = 0;
	for(const auto &Data : Inputs)
	{
		const char *IndexName = CopyString(Data.first); //NOTE: Leaks unless we free it.

		const std::vector<const char *> &InputNames = Data.second;
		if(DataSet->IndexNamesToHandle[IndexSetHandle].find(IndexName) != DataSet->IndexNamesToHandle[IndexSetHandle].end())
		{
			
			std::cout << "ERROR: Got duplicate indexes for index set " << IndexSetName << std::endl;
			exit(0);
		}
		
		DataSet->IndexNamesToHandle[IndexSetHandle][IndexName] = IndexIndex;
		DataSet->IndexNames[IndexSetHandle][IndexIndex] = IndexName;
		
		DataSet->BranchInputs[IndexSetHandle][IndexIndex].Count = InputNames.size();
		DataSet->BranchInputs[IndexSetHandle][IndexIndex].Inputs = AllocClearedArray(index_t, InputNames.size());
		
		index_t InputIdxIdx = 0;
		for(const char *InputName : InputNames)
		{
			auto Find = DataSet->IndexNamesToHandle[IndexSetHandle].find(InputName);
			if(Find == DataSet->IndexNamesToHandle[IndexSetHandle].end())
			{
				std::cout << "ERROR: The index \"" << InputName << "\" appears an input to the index \"" << IndexName << "\", in the index set " << IndexSetName << ", before it itself is declared." << std::endl;
				exit(0);
			}
			index_t InputIndex = Find->second;
			DataSet->BranchInputs[IndexSetHandle][IndexIndex].Inputs[InputIdxIdx] = InputIndex;
			++InputIdxIdx;
		}
		
		++IndexIndex;
	}
	
	bool AllSet = true;
	for(size_t IndexSetHandle = 1; IndexSetHandle < DataSet->Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		if(DataSet->IndexCounts[IndexSetHandle] == 0)
		{
			AllSet = false;
			break;
		}
	}
	DataSet->AllIndexesHaveBeenSet = AllSet;
}

static void
AllocateParameterStorage(inca_data_set *DataSet)
{
	inca_model *Model = DataSet->Model;
	
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		std::cout << "ERROR: Tried to allocate parameter storage before all index sets were filled." << std::endl;
		exit(0);
	}
	
	if(DataSet->ParameterData)
	{
		std::cout << "ERROR: Tried to allocate parameter storage twice." << std::endl;
		exit(0);
	}
	
	std::map<std::vector<index_set>, std::vector<handle_t>> TransposedParameterDependencies;
	for(handle_t ParameterHandle = 1; ParameterHandle < Model->FirstUnusedParameterHandle; ++ParameterHandle)
	{
		std::vector<index_set> Dependencies = Model->ParameterSpecs[ParameterHandle].IndexSetDependencies;
		TransposedParameterDependencies[Dependencies].push_back(ParameterHandle);
	}
	size_t ParameterStorageUnitCount = TransposedParameterDependencies.size();
	std::vector<storage_unit_specifier> &Units = DataSet->ParameterStorageStructure.Units;
	Units.resize(ParameterStorageUnitCount);
	size_t UnitIndex = 0;
	for(auto& Structure : TransposedParameterDependencies)
	{
		Units[UnitIndex].IndexSets = Structure.first; //NOTE: vector copy.
		Units[UnitIndex].Handles   = Structure.second; //NOTE: vector copy.
		
		++UnitIndex;
	}
	SetupStorageStructureSpecifer(DataSet->ParameterStorageStructure, DataSet->IndexCounts, Model->FirstUnusedParameterHandle);
	
	DataSet->ParameterData = AllocClearedArray(parameter_value, DataSet->ParameterStorageStructure.TotalCount);
	
	//NOTE: Setting up default values.
	UnitIndex = 0;
	for(storage_unit_specifier& Unit : Units)
	{
		size_t HandlesInInstance = Unit.Handles.size();
		size_t TotalHandlesForUnit = DataSet->ParameterStorageStructure.TotalCountForUnit[UnitIndex];
		
		size_t ParameterIndex = 0;
		for(handle_t ParameterHandle : Unit.Handles)
		{
			parameter_value DefaultValue = Model->ParameterSpecs[ParameterHandle].Default;
			size_t At = OffsetForHandle(DataSet->ParameterStorageStructure, ParameterHandle);
			for(size_t Instance = 0; Instance < TotalHandlesForUnit; Instance += HandlesInInstance)
			{
				DataSet->ParameterData[At] = DefaultValue;
				At += HandlesInInstance;
			}
			++ParameterIndex;
		}
		
		++UnitIndex;
	}
}

static void
AllocateInputStorage(inca_data_set *DataSet, u64 Timesteps)
{
	inca_model *Model = DataSet->Model;
	
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		std::cout << "ERROR: Tried to allocate input storage before all index sets were filled." << std::endl;
		exit(0);
	}
	
	if(DataSet->InputData)
	{
		std::cout << "ERROR: Tried to allocate input storage twice." << std::endl;
		exit(0);
	}

	std::map<std::vector<index_set>, std::vector<handle_t>> TransposedInputDependencies;
	
	for(handle_t InputHandle = 1; InputHandle < Model->FirstUnusedInputHandle; ++InputHandle)
	{
		input_spec &Spec = Model->InputSpecs[InputHandle];
		TransposedInputDependencies[Spec.IndexSetDependencies].push_back(InputHandle);
	}
	
	size_t InputStorageUnitCount = TransposedInputDependencies.size();
	std::vector<storage_unit_specifier> &Units = DataSet->InputStorageStructure.Units;
	Units.resize(InputStorageUnitCount);
	size_t UnitIndex = 0;
	for(auto& Structure : TransposedInputDependencies)
	{
		Units[UnitIndex].IndexSets = Structure.first; //NOTE: vector copy.
		Units[UnitIndex].Handles   = Structure.second; //NOTE: vector copy.
		
		++UnitIndex;
	}
	SetupStorageStructureSpecifer(DataSet->InputStorageStructure, DataSet->IndexCounts, Model->FirstUnusedInputHandle);

	
	DataSet->InputData = AllocClearedArray(double, DataSet->InputStorageStructure.TotalCount * Timesteps);
	DataSet->InputDataTimesteps = Timesteps;
}

static void
AllocateResultStorage(inca_data_set *DataSet, u64 Timesteps)
{
	inca_model *Model = DataSet->Model;
	
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		std::cout << "ERROR: Tried to allocate result storage before all index sets were filled." << std::endl;
		exit(0);
	}
	
	if(DataSet->ResultData)
	{
		std::cout << "ERROR: Tried to allocate result storage twice." << std::endl;
		exit(0);
	}
	
	//NOTE: We set up a storage structure for results that mirrors the equation batch group structure. This simplifies things a lot in other code.
	
	size_t ResultStorageUnitCount = Model->BatchGroups.size();
	std::vector<storage_unit_specifier> &Units = DataSet->ResultStorageStructure.Units;
	Units.resize(ResultStorageUnitCount);
	for(size_t UnitIndex = 0; UnitIndex < ResultStorageUnitCount; ++UnitIndex)
	{
		equation_batch_group &BatchGroup = Model->BatchGroups[UnitIndex];
		Units[UnitIndex].IndexSets = BatchGroup.IndexSets;
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			equation_batch &Batch = Model->EquationBatches[BatchIdx];
			FOR_ALL_BATCH_EQUATIONS(Batch,
				Units[UnitIndex].Handles.push_back(Equation.Handle);
			)
		}
	}
	
	SetupStorageStructureSpecifer(DataSet->ResultStorageStructure, DataSet->IndexCounts, Model->FirstUnusedEquationHandle);
	
	DataSet->ResultData = AllocClearedArray(double, DataSet->ResultStorageStructure.TotalCount * (Timesteps + 1)); //NOTE: We add one to timesteps since we also need space for the initial values.
}




inline index_t
GetIndex(inca_data_set *DataSet, index_set IndexSet, const char *IndexName)
{
	auto &IndexMap = DataSet->IndexNamesToHandle[IndexSet.Handle];
	auto Find = IndexMap.find(IndexName);
	if(Find != IndexMap.end())
	{
		return Find->second;
	}
	else
	{
		std::cout << "ERROR: Tried the index name " << IndexName << " with the index set " << GetName(DataSet->Model, IndexSet) << ", but that index set does not contain that index." << std::endl;
		exit(0);
	}
	return 0;
}

static void
SetParameterValue(inca_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, parameter_value Value, parameter_type Type)
{
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		std::cout << "ERROR: Tried to set a parameter value before all index sets have been filled with indexes" << std::endl;
		exit(0);
	}
	if(DataSet->ParameterData == 0)
	{
		AllocateParameterStorage(DataSet);
	}
	
	inca_model *Model = DataSet->Model;
	handle_t ParameterHandle = GetParameterHandle(Model, Name);
	if(!ParameterHandle) return;
	
	if(Model->ParameterSpecs[ParameterHandle].Type != Type)
	{
		std::cout << "WARNING: Tried to set the value of the parameter " << Name << " with a value that was of the wrong type. This can lead to undefined behaviour." << std::endl;
	}
	
	//TODO: Check that the value is in the Min-Max range. (issue warning only)
	
	size_t StorageUnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	std::vector<storage_unit_specifier> &Units = DataSet->ParameterStorageStructure.Units;
	std::vector<index_set> &IndexSetStack = Units[StorageUnitIndex].IndexSets;
	
	size_t StackSize = IndexSetStack.size();
	if(Indexes.size() != StackSize)
	{
		std::cout << "ERROR; Tried to set the value of the parameter " << Name << ", but an incorrect number of indexes were provided." << std::endl;
		exit(0);
	}

	//TODO: This crashes if somebody have more than 256 index sets for a parameter, but that is highly unlikely. Still, this is not clean code...
	index_t IndexValues[256];
	for(size_t Level = 0; Level < Indexes.size(); ++Level)
	{
		IndexValues[Level] = GetIndex(DataSet, IndexSetStack[Level], Indexes[Level]);
	}
	
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, IndexValues, Indexes.size(), DataSet->IndexCounts, ParameterHandle);
	DataSet->ParameterData[Offset] = Value;
}

inline void
SetParameterValue(inca_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, double Value)
{
	parameter_value Val;
	Val.ValDouble = Value;
	SetParameterValue(DataSet, Name, Indexes, Val, ParameterType_Double);
}

inline void
SetParameterValue(inca_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, u64 Value)
{
	parameter_value Val;
	Val.ValUInt = Value;
	SetParameterValue(DataSet, Name, Indexes, Val, ParameterType_UInt);
}

inline void
SetParameterValue(inca_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, bool Value)
{
	parameter_value Val;
	Val.ValBool = Value;
	SetParameterValue(DataSet, Name, Indexes, Val, ParameterType_Bool);
}

inline void
SetParameterValue(inca_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, const char *TimeValue)
{
	parameter_value Val;
	Val.ValTime = ParseSecondsSinceEpoch(TimeValue);
	SetParameterValue(DataSet, Name, Indexes, Val, ParameterType_Time);
}

static double
CumulateResult(inca_data_set *DataSet, equation Equation, index_set CumulateOverIndexSet, index_t *CurrentIndexes, double *LookupBase)
{
	double Total = 0.0;
	
	size_t SubsequentOffset;
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, CurrentIndexes, DataSet->IndexCounts, CumulateOverIndexSet, SubsequentOffset, Equation.Handle);
	
	double *Lookup = LookupBase + Offset;
	for(index_t Index = 0; Index < DataSet->IndexCounts[CumulateOverIndexSet.Handle]; ++Index)
	{
		Total += *Lookup;
		Lookup += SubsequentOffset;
	}
	
	return Total;
}

static void
GetResultSeries(inca_data_set *DataSet, const char *Name, const std::vector<const char*> &IndexNames, double *WriteTo, size_t WriteSize)
{	
	if(DataSet->HasBeenRun && DataSet->ResultData)
	{
		inca_model *Model = DataSet->Model;
		
		u64 NumToWrite = Min(WriteSize, DataSet->TimestepsLastRun);
		
		equation Equation = GetEquationHandle(Model, Name);
		
		size_t StorageUnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
		std::vector<storage_unit_specifier> &Units = DataSet->ResultStorageStructure.Units;
		std::vector<index_set> &IndexSets = Units[StorageUnitIndex].IndexSets;

		if(IndexNames.size() != IndexSets.size())
		{
			std::cout << "WARNING: Got the wrong amount of indexes when getting the result series for " << GetName(Model, Equation) << std::endl;
			return;
		}
		index_t Indexes[256];
		for(size_t IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
		{
			Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);
		}

		size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, Indexes, IndexNames.size(), DataSet->IndexCounts, Equation.Handle);
		double *Lookup = DataSet->ResultData + Offset;
		
		for(size_t Idx = 0; Idx < NumToWrite; ++Idx)
		{
			Lookup += DataSet->ResultStorageStructure.TotalCount; //NOTE: We do one initial time advancement before the lookup since the first set of values are just the initial values, which we want to skip.
			WriteTo[Idx] = *Lookup;
		}
	}
	else
	{
		std::cout << "WARNING: Tried to extract result series before the model was run at least once." << std::endl;
	}
}

static void
PrintResultSeries(inca_data_set *DataSet, const char *Name, const std::vector<const char*> &Indexes, size_t WriteSize)
{
	double *ResultSeries = AllocClearedArray(double, WriteSize);
	GetResultSeries(DataSet, Name, Indexes, ResultSeries, WriteSize);
	
	std::cout << "Result series for " << Name << " ";
	for(const char *Index : Indexes) std::cout << "[" << Index << "]";
	std::cout << ":" << std::endl;
	for(size_t Idx = 0; Idx < WriteSize; ++Idx)
	{
		std::cout << ResultSeries[Idx] << " ";
	}
	std::cout << std::endl;
	
	free(ResultSeries);
}

static void
PrintIndexes(inca_data_set *DataSet, const char *IndexSetName)
{
	//TODO: checks
	inca_model *Model = DataSet->Model;
	handle_t IndexSetHandle = GetIndexSetHandle(Model, IndexSetName).Handle;
	std::cout << "Indexes for " << IndexSetName << ": ";
	index_set_spec &Spec = Model->IndexSetSpecs[IndexSetHandle];
	if(Spec.Type == IndexSetType_Basic)
	{
		for(size_t IndexIndex = 0; IndexIndex < DataSet->IndexCounts[IndexSetHandle]; ++IndexIndex)
		{
			std::cout << DataSet->IndexNames[IndexSetHandle][IndexIndex] << " ";
		}
	}
	else if(Spec.Type == IndexSetType_Branched)
	{
		for(size_t IndexIndex = 0; IndexIndex < DataSet->IndexCounts[IndexSetHandle]; ++IndexIndex)
		{
			std::cout <<  "(" << DataSet->IndexNames[IndexSetHandle][IndexIndex] << ": ";
			for(size_t InputIndexIndex = 0; InputIndexIndex < DataSet->BranchInputs[IndexSetHandle][IndexIndex].Count; ++InputIndexIndex)
			{
				size_t InputIndex = DataSet->BranchInputs[IndexSetHandle][IndexIndex].Inputs[InputIndexIndex];
				std::cout << DataSet->IndexNames[IndexSetHandle][InputIndex] << " ";
			}
			std::cout << ") ";
		}
	}
	std::cout << std::endl;
}