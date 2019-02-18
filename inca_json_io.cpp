
#include "json/json.hpp"    //NOTE: download at https://github.com/nlohmann/json


static void
WriteInputToJsonRecursive(inca_data_set *DataSet, nlohmann::json &Json, input_h Input, index_t *CurrentIndexes, u64 Timesteps, s32 Level = -1)
{
	using nlohmann::json;
	
	const inca_model *Model = DataSet->Model;
	
	const input_spec &Spec = Model->InputSpecs[Input.Handle];
	
	std::string Name = Spec.Name;
	
	size_t Count = Spec.IndexSetDependencies.size();
	if(Level + 1 == (s32)Count)
	{
		std::vector<double> Values((size_t)Timesteps);
		
		std::vector<const char *> Indices(Count);
		
		for(size_t IdxIdx = 0; IdxIdx < Count; ++IdxIdx)
		{
			Indices[IdxIdx] = DataSet->IndexNames[Spec.IndexSetDependencies[IdxIdx].Handle][CurrentIndexes[IdxIdx]];
		}
		
	   
		GetInputSeries(DataSet, Name.c_str(), Indices, Values.data(), Values.size());
	   
		json JObj
		{
			{"indices", Indices},
			{"values", Values},
		};
		
		Json["data"][Name].push_back(JObj);
	}
	else
	{
		index_set_h IndexSetAtLevel = Spec.IndexSetDependencies[Level + 1];
		
		for(index_t Index = 0; Index < DataSet->IndexCounts[IndexSetAtLevel.Handle]; ++Index)
		{
			CurrentIndexes[Level + 1] = Index;
			WriteInputToJsonRecursive(DataSet, Json, Input, CurrentIndexes, Level + 1);
		}
		
	}
}


static void
WriteInputsToJson(inca_data_set *DataSet, const char *Filename)
{
	using nlohmann::json;
	
	const inca_model *Model = DataSet->Model;
	
	std::vector<std::string> AdditionalTimeseries;
	std::map<std::string, std::vector<std::string>> IndexSetDependencies;
	
	for(entity_handle InputHandle = 1; InputHandle < Model->FirstUnusedInputHandle; ++InputHandle)
	{
		const input_spec &Spec = Model->InputSpecs[InputHandle];
		
		if(Spec.IsAdditional)
		{
			AdditionalTimeseries.push_back(GetName(Model, input_h {InputHandle}));
		}
		
		std::vector<std::string> Dep;
		for(index_set_h IndexSet : Spec.IndexSetDependencies)
		{
			Dep.push_back(GetName(Model, IndexSet));
		}
		
		IndexSetDependencies[Spec.Name] = Dep;	
	}
	
	
	auto T = std::time(nullptr);
	auto TM = *std::localtime(&T);
	std::stringstream Oss;
	Oss << std::put_time(&TM, "%Y-%m-%d %H:%M:%S");
	
	std::string CreationDate = Oss.str();
	
	u64 Timesteps = DataSet->InputDataTimesteps;
	
	
	s64 StartDateS;
	if(DataSet->InputDataHasSeparateStartDate)
	{
		StartDateS = DataSet->InputDataStartDate;
	}
	else
	{
		StartDateS = GetStartDate(DataSet);
	}
	
	std::string StartDate = TimeString(StartDateS);
    
	json Json = {
				{"creation_date", CreationDate},
				{"start_date", StartDate},
				{"timesteps", Timesteps},
				{"index_set_dependencies", IndexSetDependencies},
				{"additional_timeseries", AdditionalTimeseries},
				{"data", nullptr},
			};
   
	index_t CurrentIndexes[256];
	for(entity_handle InputHandle = 1; InputHandle < Model->FirstUnusedInputHandle; ++InputHandle)
	{
		WriteInputToJsonRecursive(DataSet, Json, input_h {InputHandle}, CurrentIndexes, Timesteps);
	}
  
	std::ofstream Out(Filename);
	Out << Json.dump(1,'\t') << std::endl;
}


static void
ReadInputDependenciesFromJson(inca_model *Model, const char *Filename)
{
	std::ifstream Ifs(Filename);
	nlohmann::json JData;
	Ifs >> JData;
	
	if (JData.find("additional_timeseries") != JData.end())
    {
        std::vector<std::string> AdditionalTimeseries = JData["additional_timeseries"].get<std::vector<std::string>>();
		
		for(std::string &Str : AdditionalTimeseries)
		{
			const char *InputName = CopyString(Str.c_str());
			RegisterInput(Model, InputName, true);
		}
    }
    
    if (JData.find("index_set_dependencies") != JData.end())
    {
        std::map<std::string, std::vector<std::string>> Dep = JData["index_set_dependencies"].get<std::map<std::string,std::vector<std::string>>>();
		for(auto &D : Dep)
		{
			const std::string &Name = D.first;
			std::vector<std::string> &IndexSets = D.second;
			
			input_h Input = GetInputHandle(Model, Name.c_str());
			
			for(std::string &IndexSet : IndexSets)
			{
				index_set_h IndexSetH = GetIndexSetHandle(Model, IndexSet.c_str());
				AddInputIndexSetDependency(Model, Input, IndexSetH);
			}
		}
    }
}


static void 
ReadInputsFromJson(inca_data_set *DataSet, const char *Filename)
{
	std::ifstream Ifs(Filename);
	nlohmann::json JData;
	Ifs >> JData;
 
	if (JData.find("timesteps") != JData.end())
	{
		//data.timesteps = jData["timesteps"];
		DataSet->InputDataTimesteps = (u64)JData["timesteps"].get<u64>();
	}
	else
	{
		INCA_FATAL_ERROR("Input file " << Filename << " does not declare the number of timesteps for the input data." << std::endl);
	}
    
	if (JData.find("start_date") != JData.end())
	{
		std::string DateStr = JData["start_date"];
		
		s64 SecondsSinceEpoch;
		bool Success = ParseSecondsSinceEpoch(DateStr.c_str(), &SecondsSinceEpoch);
		
		if(!Success)
		{
			INCA_FATAL_ERROR("Unrecognized date format \"" << DateStr << "\". Supported format: Y-m-d" << std::endl);
		}
		
		DataSet->InputDataHasSeparateStartDate = true;
		DataSet->InputDataStartDate = SecondsSinceEpoch;
	}

	
	if (JData.find("data") != JData.end())
	{
		for (nlohmann::json::iterator It = JData["data"].begin(); It != JData["data"].end(); ++It)
		{
			std::string Name = It.key();
			
			for(auto Itit = It->begin(); Itit != It->end(); ++Itit)
			{
				std::vector<std::string> Indices = (*Itit)["indices"].get<std::vector<std::string>>();
				std::vector<const char *> Indices2;
				for(std::string &Str : Indices) Indices2.push_back(Str.c_str());
				
				auto &Val = (*Itit)["values"];
				std::vector<double> Values;
				Values.reserve(Val.size());
				for(auto &V : Val)
				{
					if(V.is_number_float())
					{
						double X = V;
						Values.push_back(X);
					}
					else Values.push_back(std::numeric_limits<double>::quiet_NaN());
				}

				SetInputSeries(DataSet, Name.c_str(), Indices2, Values.data(), Values.size());
			}
		}
	}
}


static void
WriteParametersToJson(inca_data_set *DataSet, const char *Filename)
{
	using nlohmann::json;
	
	const inca_model *Model = DataSet->Model;
	
	json Json
	{
		{"index_sets", nullptr},
		{"parameters", nullptr},
	};
	
	for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		const index_set_spec &Spec = Model->IndexSetSpecs[IndexSetHandle];
		if(Spec.Type == IndexSetType_Basic)
		{
			for(index_t Index = 0; Index < DataSet->IndexCounts[IndexSetHandle]; ++Index)
			{
				std::string IndexName = DataSet->IndexNames[IndexSetHandle][Index];
				Json["index_sets"][Spec.Name].push_back(IndexName);
			}
		}
		else if(Spec.Type == IndexSetType_Branched)
		{
			std::map<std::string, std::vector<std::string>> BranchInputs;
			for(index_t Index = 0; Index < DataSet->IndexCounts[IndexSetHandle]; ++Index)
			{
				std::string IndexName = DataSet->IndexNames[IndexSetHandle][Index];
				
				std::vector<std::string> Branches;
				for(size_t In = 0; In < DataSet->BranchInputs[IndexSetHandle][Index].Count; ++In)
				{
					index_t InIndex = DataSet->BranchInputs[IndexSetHandle][Index].Inputs[In];
					
					std::string InName = DataSet->IndexNames[IndexSetHandle][InIndex];
					Branches.push_back(InName);
				}
				
				BranchInputs[IndexName] = Branches;
				
				Json["index_sets"][Spec.Name] = BranchInputs;
			}
		}
	}
	
	for(entity_handle ParameterHandle = 1; ParameterHandle < Model->FirstUnusedParameterHandle; ++ParameterHandle)
	{
		const parameter_spec &Spec = Model->ParameterSpecs[ParameterHandle];
		
		//NOTE: This is pretty horrible. It should be wrapped up somehow instead.
		size_t UnitIdx = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
		storage_unit_specifier &Unit = DataSet->ParameterStorageStructure.Units[UnitIdx];
		size_t ParametersInUnit = Unit.Handles.size();
		size_t TotalValuesInUnit = DataSet->ParameterStorageStructure.TotalCountForUnit[UnitIdx];
		size_t ValuesPerPar = TotalValuesInUnit / ParametersInUnit;
		
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ParameterHandle);
		
		if(Spec.Type == ParameterType_Double)
		{
			std::vector<double> Values;
			
			parameter_value *At = DataSet->ParameterData + Offset;
			for(size_t Idx = 0; Idx < ValuesPerPar; ++Idx)
			{
				Values.push_back((*At).ValDouble);
				At += ParametersInUnit;
			}
			
			Json["parameters"][Spec.Name] = Values;
		}
		else if(Spec.Type == ParameterType_UInt)
		{
			std::vector<u64> Values;
			
			parameter_value *At = DataSet->ParameterData + Offset;
			for(size_t Idx = 0; Idx < ValuesPerPar; ++Idx)
			{
				Values.push_back((*At).ValUInt);
				At += ParametersInUnit;
			}
			
			Json["parameters"][Spec.Name] = Values;
		}
		else if(Spec.Type == ParameterType_Bool)
		{
			std::vector<bool> Values;
			
			parameter_value *At = DataSet->ParameterData + Offset;
			for(size_t Idx = 0; Idx < ValuesPerPar; ++Idx)
			{
				Values.push_back((*At).ValBool);
				At += ParametersInUnit;
			}
			
			Json["parameters"][Spec.Name] = Values;
		}
		else if(Spec.Type == ParameterType_Time)
		{
			std::vector<std::string> Values;
			
			parameter_value *At = DataSet->ParameterData + Offset;
			for(size_t Idx = 0; Idx < ValuesPerPar; ++Idx)
			{
				s64 SecondsSinceEpoch = (*At).ValTime;
				std::string Date = TimeString(SecondsSinceEpoch);
				Values.push_back(Date);
				At += ParametersInUnit;
			}
			
			Json["parameters"][Spec.Name] = Values;
		}
	}
	
	std::ofstream Out(Filename);
	Out << Json.dump(1,'\t') << std::endl;
}

static void
ReadParametersFromJson(inca_data_set *DataSet, const char *Filename)
{
	const inca_model *Model = DataSet->Model;
	
	std::ifstream Ifs(Filename);
	nlohmann::json JData;
	Ifs >> JData;
	
	if(JData.find("index_sets") != JData.end())
	{
		for (nlohmann::json::iterator It = JData["index_sets"].begin(); It != JData["index_sets"].end(); ++It)
		{
			std::string IndexSetName = It.key();
			
			index_set_h IndexSet = GetIndexSetHandle(Model, IndexSetName.c_str());
			const index_set_spec &Spec = Model->IndexSetSpecs[IndexSet.Handle];
			
			if(Spec.Type == IndexSetType_Basic)
			{
				std::vector<std::string> IndexNames = It->get<std::vector<std::string>>();
				std::vector<token_string> IndexNames2;
				for(std::string &Str : IndexNames) IndexNames2.push_back(Str.c_str());
				
				SetIndexes(DataSet, IndexSetName.c_str(), IndexNames2);
			}
			else if(Spec.Type == IndexSetType_Branched)
			{
				std::map<std::string, std::vector<std::string>> Indexes = It->get<std::map<std::string, std::vector<std::string>>>();
				
				std::vector<std::pair<token_string, std::vector<token_string>>> Inputs;
				
				for(auto &Branch : Indexes)
				{
					std::vector<token_string> IndexNames2;
					for(std::string &Str : Branch.second) IndexNames2.push_back(Str.c_str());
					Inputs.push_back({Branch.first.c_str(), IndexNames2});
				}
				
				SetBranchIndexes(DataSet, IndexSetName.c_str(), Inputs);
			}
		}
	}
	
	//TODO: Find crash bug here..
	if(JData.find("parameters") != JData.end())
	{
		for (nlohmann::json::iterator It = JData["parameters"].begin(); It != JData["parameters"].end(); ++It)
		{
			std::string ParameterName = It.key();
			entity_handle ParameterHandle = GetParameterHandle(Model, ParameterName.c_str());
			std::vector<parameter_value> Values;
			const parameter_spec &Spec = Model->ParameterSpecs[ParameterHandle];
			if(Spec.Type == ParameterType_Double)
			{
				std::vector<double> Val = It->get<std::vector<double>>();
				for(double D : Val)
				{
					parameter_value ParVal; ParVal.ValDouble = D;
					Values.push_back(ParVal);
				}
			}
			else if(Spec.Type == ParameterType_UInt)
			{
				std::vector<u64> Val = It->get<std::vector<u64>>();
				for(u64 D : Val)
				{
					parameter_value ParVal; ParVal.ValUInt = D;
					Values.push_back(ParVal);
				}
			}
			else if(Spec.Type == ParameterType_Bool)
			{
				std::vector<bool> Val = It->get<std::vector<bool>>();
				for(bool D : Val)
				{
					parameter_value ParVal; ParVal.ValBool = D;
					Values.push_back(ParVal);
				}
			}
			else if(Spec.Type == ParameterType_Time)
			{
				std::vector<std::string> Val = It->get<std::vector<std::string>>();
				for(std::string &D : Val)
				{
					s64 SecondsSinceEpoch;
					bool Success = ParseSecondsSinceEpoch(D.c_str(), &SecondsSinceEpoch);
					//TODO: On failure..
					parameter_value ParVal; ParVal.ValTime = SecondsSinceEpoch;
					Values.push_back(ParVal);
				}
			}
			
			SetMultipleValuesForParameter(DataSet, ParameterHandle, Values.data(), Values.size());
		}
	}
}


