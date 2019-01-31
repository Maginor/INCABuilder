/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   InputTranslator.h
 * Author: jose-luis
 *
 * Created on 30 January 2019, 10:04
 */
#pragma once
#include "../inca.h"
#include "nlohmann/json.hpp"
#include "yaml-cpp/yaml.h"

#ifndef INPUTTRANSLATOR_H
#define INPUTTRANSLATOR_H



/*The way this class works is to read in inputs into a custom struct/map that can 
 * then be broadcast to the different type formats
 */
class InputTranslator {
public:
    InputTranslator(const std::string filename_);
    InputTranslator();
    InputTranslator(const InputTranslator& orig);
    virtual ~InputTranslator();
    
private:
    //Allowable formats for Inputs
    enum format{
        magnus,
        yaml,
        json
    };
    
    format fileFormat;
    
    //Dataset where the inputs will be placed
    inca_data_set* Dataset;
    
    //File from which the data will be read
    const std::string filename;
    
    //Determining file type
    void getFormat();
    
    //Internal format for file conversion
    struct data
    {
        std::string start_date;
        size_t timesteps;
        std::vector<std::string> additional_timeseries;
        std::map< std::pair<std::string, std::vector<std::string>>, std::vector<double> > inputs;
        std::map< std::pair<std::string, std::vector<std::string>>, std::vector<std::pair<size_t,double> > > sparseInputs;
                
    };
    
    std::string findBetween(std::string,std::string,std::string);
    
    void parse();
    void parseMagnus();
    
    
    
       
           
};

#endif /* INPUTTRANSLATOR_H */

