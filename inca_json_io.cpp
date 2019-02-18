
#include "json/json.hpp"

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
	
   
/*   
    for (auto&i : data.sparseInputs)
    {
        json idxi(i.first.indexers),
             idxj(i.first.indices),
             v(i.second);        
        
        json jobj
        {
            {"indexers", idxi},
            {"indices", idxj},
            {"values", v},
        };
        
        j["data"][i.first.name].push_back(jobj);
    }
*/
    
    std::ofstream out(Filename);
    out << Json.dump(1,'\t') << std::endl;
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
		//TODO: Error
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

//NOTE: These have to be done in a separate pass..	
/*
    if (JData.find("additional_timeseries") != JData.end())
    {
        std::vector<std::sctring>> AdditionalTimeseries = jData["additional_timeseries"].get<std::vector<std::string>>();
		
		for()
    }
    
    if (jData.find("index_set_dependencies")!=jData.end())
    {
        data.index_set_dependencies = jData["index_set_dependencies"].get<std::map<std::string,std::vector<std::string>>>();
    }
*/
    
    //Populating data
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
				
                //attributes.indexers = (*itit)["indexers"].get<std::vector<std::string>>();
                //attributes.indices = (*itit)["indices"].get<std::vector<std::string>>();
                if ( (*Itit)["values"][0].is_number_float() )
                {
                    std::vector<double> Values = (*Itit)["values"].get<std::vector<double>>();
					
					SetInputSeries(DataSet, Name.c_str(), Indices2, Values.data(), Values.size());
                }
                //else
                //{
                //    data.sparseInputs[attributes] 
                //            = (*itit)["values"].get<std::vector<std::pair<std::string,double>>>();
                //}
            
            }
        }        
    }
}