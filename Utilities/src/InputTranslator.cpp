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
                    boost::algorithm::trim_if(line,boost::algorithm::is_any_of(" "));
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
    data.start_date = settings["start_date"][0];
    if (settings.find("index_set_dependencies") != settings.end())
    {
       for (auto&i: settings["index_set_dependencies"])
       {
           std::string parameter = findBetween(i,"^",":");
           boost::algorithm::trim_if(parameter, boost::algorithm::is_any_of("\" "));
           std::vector<std::string> indexers;
           std::string str = findBetween(i,"{","}");
           boost::split(indexers, str, boost::is_any_of(","));
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
        for (auto zi : zip( data.index_set_dependencies[parameter], indices))
        {
            idxidx += "[" + (zi.get<0>()) + "," + (zi.get<1>()) + "]";
        }
        std::cout << idxidx << " " << key << std::endl;
        if (std::find(data.additional_timeseries.begin(), //Means it's not a sparse timeseries
                      data.additional_timeseries.end(),
                      "parameter") == data.additional_timeseries.end())
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
//                data.inputs[idxidx].push_back(boost::lexical_cast<double>(j)); 
            }
          
        }    
    }   
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
        boost::algorithm::trim_if(res,boost::algorithm::is_any_of(" "));
        return res;
    }
    else return str;
}


