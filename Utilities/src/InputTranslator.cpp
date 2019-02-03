/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   InputTranslator.cpp
 * Author: jose-luis
 * 
 * Created on 30 January 2019, 10:04
 */

#include "InputTranslator.h"


InputTranslator::InputTranslator(const std::string _filename) : filename(_filename) {
    getFormat();
    parse();
 }

InputTranslator::InputTranslator() {
 }

InputTranslator::InputTranslator(const InputTranslator& orig) {
}

InputTranslator::~InputTranslator() {
}

void InputTranslator::getFormat(){
    std::string extension = filename.substr(filename.find_last_of('.') + 1);
    if (extension == "yaml" || extension  == "yml") fileFormat = format::yaml;
    else if (extension == "json") fileFormat = format::json;
    else if (extension == "dat") fileFormat = format::magnus;
    else 
    {
         std::cout << "Unrecognized file format in " << filename << std::endl;
         throw;
    }   
}

void InputTranslator::parse()
{
    switch(fileFormat)
    {
        case magnus:
            parseMagnus();
            break;
        case yaml:
            break;
        case json:
            break;
    }
}

void InputTranslator::parseMagnus()
{
    boost::regex header("^[^\"].+:");
    boost::regex empty("^\\s*$");
    boost::regex comment("^[!#]");
    boost::regex input("^inputs");
    std::map<std::string,std::vector<std::string>> settings,inputs;
    std::map<std::string,std::vector<std::string>>* mapPnt;    
    std::ifstream infile(filename);
    std::string line,key;
    bool inputStart = false;
    mapPnt = &settings;
    // First pass
    if (infile.is_open())
    {
       while (std::getline(infile, line))
        {
            std::istringstream iss(line);
            if (boost::regex_search(line,header))
            {
                if (!inputStart)
                {
                    inputStart = boost::regex_search(line,input);
                    if (inputStart) header=":";
                    mapPnt = &settings;
                }
                else mapPnt = &inputs;
                key = findBetween(line,"^",":");
                std::string after = findBetween(line,":","$");
                if (after != "") (*mapPnt)[key].push_back(after);                
            }
            else
            {
                if (!boost::regex_search(line,comment) && !boost::regex_search(line,empty))
                {
                    boost::algorithm::trim(line);
                    (*mapPnt)[key].push_back(line);
                }
            }
        }
    }
    else
    {
        std::cout << "Couldn't open file: " << filename << std::endl;
        throw;
    }
    //Assigning values to data structure
    data.timesteps = boost::lexical_cast<size_t>(settings["timesteps"][0]);
    
    //timestamp as posix ptime
    std::string dateStr = settings["start_date"][0];
    boost::algorithm::trim_if(dateStr, boost::algorithm::is_any_of("\""));
    std::istringstream is(dateStr);
    is.imbue(formats[3]);
    boost::posix_time::ptime t;
    try
    {
        is >> t;
    }
    catch(...)
    {
        INCA_FATAL_ERROR("Unrecognized date format \"" << dateStr << "\". Supported format: Y-m-d" << std::endl);
    }
    
    data.start_date = t;
    
    if (settings.find("index_set_dependencies") != settings.end())
    {
       for (auto&i: settings["index_set_dependencies"])
       {
           std::string parameter = findBetween(i,"^",":");
           boost::algorithm::trim_if(parameter, boost::algorithm::is_any_of("\" "));
           std::vector<std::string> indexers;
           std::string str = findBetween(i,"{","}");
           boost::split(indexers, str, boost::is_any_of(","));
           for (auto&j: indexers) boost::algorithm::trim_if(j,boost::algorithm::is_any_of("\""));
           data.index_set_dependencies[parameter]=indexers;
        }
    }
    if (settings.find("additional_timeseries") != settings.end())
    {
        for(auto&i: settings["additional_timeseries"])
        {
            boost::algorithm::trim_if(i, boost::algorithm::is_any_of("\" "));
            data.additional_timeseries.push_back(i);
        }
    }     
    for(auto&i: inputs)
    {
        std::string key = i.first;
        std::string parameter = findBetween(key,"^\"","\"");
        std::vector<std::string> indices;
        std::string str = findBetween(key,"{","}");
        boost::algorithm::trim_if(str, boost::algorithm::is_any_of("\" "));
        boost::split(indices, str, boost::is_any_of(","));
        std::string idxidx = parameter;
        //This is dangerous. Should check vectors exist and are the same length
        for (auto zi : zip( data.index_set_dependencies[parameter], indices)) 
        {
            idxidx += "[" + (zi.get<0>()) + "," + (zi.get<1>()) + "]";
        }
        std::cout << idxidx << " " << key << std::endl;
        if (*(inputs[key].rbegin()) != "end_timeseries")
        {
            for (auto&j : i.second)
            { 
                try
                {
                    data.inputs[idxidx].push_back(boost::lexical_cast<double>(j));
                }
                catch(...)
                {
                    std::cout << "could not convert <" << j << "> to double" << std::endl; 
                    throw;
                }
            }          
        }
        else
        {
            i.second.pop_back(); //Might want to add a check to see if it's "end_timeseeries"
            for (auto&j : i.second)
            {
                std::string date = findBetween(j,"^\"","\"");
                std::string value = findBetween(j,"[0-9]\"","$");
                try
                {
                    data.sparseInputs[idxidx].push_back(
                            std::make_pair<std::string&,double>(date,
                                                               boost::lexical_cast<double>(value)
                                                               )
                            );
                }
                catch(...)
                {
                    std::cout << "could not convert <" << j << "> to sparse input" << std::endl; 
                    throw;
                }
            }
        }
    }
    saveJson();
    saveYaml();
}

std::string InputTranslator::findBetween(std::string str,std::string start, std::string finish)
{
    start = "(?<=" + start + ")";
    finish = "(?=" + finish + ")";
    std::string val = start + "(.*?)" + finish;
    boost::regex re(val);
    boost::smatch match;
    if (boost::regex_search(str,match,re))
    {
        //Should probably add a smarter way to process this in case there is more than one match
        std::string res = match[1];
        //Getting rid of quotes and spaces
        boost::algorithm::trim(res);
        return res;
    }
    else return str;
}


void InputTranslator::saveJson()
{
    using nlohmann::json;
    
    json ats(data.additional_timeseries);
    auto t = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(t);
    
    json j = {
                {"creation_date",strtok(std::ctime(&tt), "\n")},
                {"start_date", boost::posix_time::to_simple_string(data.start_date.get())},
                {"timesteps", data.timesteps.get()},
                {"additional_timeseries", ats},
                {"data", nullptr},
            };
    
    std::vector<json> jVec;
    for (auto&i : data.inputs)
    {
        //Separating list of indexers/indices
        std::string parameter = findBetween(i.first,"^","\\[");
        std::string dummy = findBetween(i.first,parameter.c_str(),"$");
        std::vector<std::string> idxvec; 
        boost::algorithm::trim_if(dummy, boost::algorithm::is_any_of("[]"));
        boost::split(idxvec, dummy, boost::is_any_of("[]"), boost::token_compress_on);
        
        json idxi,idxj;
        for (auto& j: idxvec)
        {
            idxi.emplace_back(findBetween(j,"^",","));
            idxj.emplace_back(findBetween(j,",","$"));
        }
        
        json v(i.second);
        
        json jobj
        {
            {"indexers", idxi},
            {"indices", idxj},
            {"values", v},
        };
        
        j["data"][parameter].push_back(jobj);
    }
    
    for (auto&i : data.sparseInputs)
    {
        //Separating list of indexers/indices
        std::string parameter = findBetween(i.first,"^","\\[");
        std::string dummy = findBetween(i.first,parameter.c_str(),"$");
        std::vector<std::string> idxvec; 
        boost::algorithm::trim_if(dummy, boost::algorithm::is_any_of("[]"));
        boost::split(idxvec, dummy, boost::is_any_of("[]"), boost::token_compress_on);
        
        json idxi,idxj;
        for (auto& j: idxvec)
        {
            idxi.emplace_back(findBetween(j,"^",","));
            idxj.emplace_back(findBetween(j,",","$"));
        }
        
        json v(i.second);
        
        json jobj
        {
            {"indexers", idxi},
            {"indices", idxj},
            {"values", v},
        };
        
        j["data"][parameter].push_back(jobj);
    }
    //
    //auto tt = ctime(&t);
    //Checking unique index_set_dependencies
    
    
    std::ofstream out("pretty.json");
    out << std::setw(1) << j << std::endl;
}

void InputTranslator::saveYaml()
{
    auto t = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(t);
    
    YAML::Emitter out;
    out.SetDoublePrecision(6);
    
    out << YAML::BeginDoc << YAML::BeginMap;
    out << YAML::Key << "creation_date" << YAML::Value << strtok(std::ctime(&tt), "\n");
    out << YAML::Key << "start_date" << YAML::Value << boost::posix_time::to_simple_string(data.start_date.get());
    out << YAML::Key << "timesteps" << YAML::Value << data.timesteps.get();
    out << YAML::Key << "additional_timeseries" << YAML::Value << data.additional_timeseries;
    out << YAML::Key << "index_set_dependencies" << YAML::Value << data.index_set_dependencies;
    out << YAML::Key << "data" << YAML::BeginSeq;
       
    for (auto&i : data.inputs)
    {
        //Separating list of indexers/indices
        std::string parameter = findBetween(i.first,"^","\\[");
        std::string dummy = findBetween(i.first,parameter.c_str(),"$");
        std::vector<std::string> idxvec; 
        boost::algorithm::trim_if(dummy, boost::algorithm::is_any_of("[]"));
        boost::split(idxvec, dummy, boost::is_any_of("[]"), boost::token_compress_on);
        
        std::vector<std::string> idxi,idxj;
        for (auto& j: idxvec)
        {
            idxi.emplace_back(findBetween(j,"^",","));
            idxj.emplace_back(findBetween(j,",","$"));
        }        
        
        out << YAML::BeginMap;
        out << YAML::Key << "parameter" << YAML::Value << parameter;
        out << YAML::Key << "indexers" << YAML::Flow;
        out << YAML::BeginSeq << idxi << YAML::EndSeq;
        out << YAML::Key << "indices" << YAML::Flow;
        out << YAML::BeginSeq << idxj << YAML::EndSeq;
        out << YAML::Key << "values" << YAML::Value;
        out << YAML::BeginSeq << YAML::Flow << i.second << YAML::EndSeq;
        out << YAML::EndMap;
        
    }

    for (auto&i : data.sparseInputs)
    {
        //Separating list of indexers/indices
        std::string parameter = findBetween(i.first,"^","\\[");
        std::string dummy = findBetween(i.first,parameter.c_str(),"$");
        std::vector<std::string> idxvec; 
        boost::algorithm::trim_if(dummy, boost::algorithm::is_any_of("[]"));
        boost::split(idxvec, dummy, boost::is_any_of("[]"), boost::token_compress_on);
        
        std::vector<std::string> idxi,idxj;
        for (auto& j: idxvec)
        {
            idxi.emplace_back(findBetween(j,"^",","));
            idxj.emplace_back(findBetween(j,",","$"));
        }
        
        out << YAML::BeginMap;
        out << YAML::Key << "parameter" << YAML::Value << parameter;
        out << YAML::Key << "indexers" << YAML::Flow;
        out << YAML::BeginSeq << idxi << YAML::EndSeq;
        out << YAML::Key << "indices" << YAML::Flow;
        out << YAML::BeginSeq << idxj << YAML::EndSeq;
        out << YAML::Key << "values" << YAML::Flow;
        out << YAML::BeginSeq << YAML::Flow;
        for(auto&j: i.second)
        {
            out << YAML::Flow << YAML::BeginSeq  << j.first << j.second << YAML::EndSeq;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
        
    }
    
    out << YAML::EndSeq << YAML::EndMap << YAML::EndDoc;
    
    std::ofstream(ofs)("pretty.yaml");
    ofs << out.c_str() << std::endl;
}


void InputTranslator::putInDataset()
{
    const inca_model* Model = DataSet->Model;
    u64 Timesteps = 0;
    bool FoundTimesteps = false;
    
    if ( !data.isAny() )
    {
        INCA_FATAL_ERROR("Expected one of the code words timesteps, start_date, inputs, additional_timeseries or index_set_dependencies" << std::endl);
    }
    
    if (data.timesteps)
    {
        if (*data.timesteps == 0)
        {
            INCA_FATAL_ERROR("ERROR: Timesteps in the input file " << filename << " is set to 0." << std::endl);
        }
        AllocateInputStorage(DataSet, *data.timesteps);
        FoundTimesteps = true;
    }
    if (data.start_date)
    {
        boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
        boost::posix_time::time_duration diff = *data.start_date - epoch;
        s64 secondsSinceEpoch = diff.ticks()/boost::posix_time::time_duration::rep_type::ticks_per_second;
        
        DataSet->InputDataStartDate = secondsSinceEpoch;
	DataSet->InputDataHasSeparateStartDate = true;     
    }
    
    for(auto&i: data.inputs)
    {
        std::string inputName = findBetween(i.first,"\"","\"");
        input_h Input = GetInputHandle(Model, inputName.c_str());
        std::vector<std::string> indexNames;
        std::string idx = findBetween(i.first,inputName,"$");
        
    }
    
    
    
    
    
    
}