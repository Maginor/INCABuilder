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
//           std::cout << line << std::endl;
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
                    boost::algorithm::trim_if(line,boost::algorithm::is_any_of("\" "));
                    (*mapPnt)[key].push_back(line);
//                     std::cout << line << std::endl;
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
        std::string parameter = findBetween(i,"\"","\"");
        std::vector<std::string> indices;
        std::string str = findBetween(i,"{","}");
        boost::split(indices, str, boost::is_any_of(","));
        
        
    }
    
    
    
}

std::string InputTranslator::findBetween(std::string str,std::string start, std::string finish)
{
    start = "(?<=" + start + ")";
    finish = "(?=" + finish + ")";
    std::string val = start + "(.*)" + finish;
    boost::regex re(val);
    boost::smatch match;
    if (boost::regex_search(str,match,re))
    {
        //Should probably add a smarter way to process this in case there is more than one match
        std::string res = match[1];
        //Getting rid of quotes and spaces
        boost::algorithm::trim_if(res,boost::algorithm::is_any_of("\" "));
        return res;
    }
    else return str;
}
std::stringstream InputTranslator::ss;

std::size_t InputTranslator::myHash::operator ()(const Key& k) const
{
    ss.clear();
    ss << k.first;
    ss << '[';
    std::copy(boost::make_transform_iterator(k.second.begin(),k.one),
              boost::make_transform_iterator(k.second.end(), k.one),
              std::ostream_iterator<std::string>(ss,","));
    ss << ']';
    ss << '{';
    std::copy(boost::make_transform_iterator(k.second.begin(),k.two),
              boost::make_transform_iterator(k.second.end(), k.two),
              std::ostream_iterator<std::string>(ss,","));
    ss << '}';
    return std::hash<std::string>()(ss.str());
}

