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
    std::regex header("^[^\"].+:");
    std::regex empty("^\\s*$");
    std::regex comment("^[!#]");
    std::regex input("^inputs");
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
            if (std::regex_search(line,header))
            {
                if (!inputStart)
                {
                    inputStart = std::regex_search(line,input);
                    if (inputStart) header=":";
                    mapPnt = &settings;
                }
                else mapPnt = &inputs;
                key = findBetween(line,"^\\s*","\\s*:");
                //std:: cout << "key ->" << key << std::endl;
                std::string after = findBetween(line,":\\s*","\\s*$");
                if (after != "")
                {
                    std::string trim = findBetween(after,"\"" , "\"");
                    if ( trim != "")
                    {
                        (*mapPnt)[key].push_back(trim);
                    } 
                    else (*mapPnt)[key].push_back(after);
                }
                
            }
            else
            {
                if (!std::regex_search(line,comment) && !std::regex_search(line,empty))
                {
                    (*mapPnt)[key].push_back(line);
//                     std::cout << line << std::endl;
                }
                
            }
            
            // process pair (a,b)
        }
    }
    else
    {
        std::cout << "Couldn't open file: " << filename << std::endl;
        throw;
    }
    //Second pass, storing in data structure
    for(auto &i:inputs) std::cout << i.first << std::endl;
    std::cout << "lololo" << std::endl;
    for(auto &i:settings) std::cout << i.first << std::endl;
    
}

std::string InputTranslator::findBetween(std::string str,std::string start, std::string finish)
{
    start = "(?:" + start + ")";
    finish = "(?:" + finish + ")";
    std::regex re(start + "(.*)" + finish);
    std::smatch match;
    if (std::regex_search(str,match,re))
    {
        //Should probably add a smarter way to process this in case there is more than one match
        return match[1];
    }
    else return "";
    
}


