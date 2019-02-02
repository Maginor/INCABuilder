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
#include "inca.h"
#include "boost/lexical_cast.hpp"
#include "boost/iterator/transform_iterator.hpp"
#include "boost/algorithm/string.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>
#include "boost/regex.hpp"
#include <chrono>
#include <ctime>
#include "nlohmann/json.hpp"
#include "yaml-cpp/yaml.h"

#ifndef INPUTTRANSLATOR_H
#define INPUTTRANSLATOR_H


//Helper function that allow to iterate over two containers simultaneously

// See https://stackoverflow.com/a/8513803/2706707
template <typename... T>
auto zip(T&&... containers) -> boost::iterator_range<boost::zip_iterator<decltype(boost::make_tuple(std::begin(containers)...))>>
{
    auto zip_begin = boost::make_zip_iterator(boost::make_tuple(std::begin(containers)...));
    auto zip_end = boost::make_zip_iterator(boost::make_tuple(std::end(containers)...));
    return boost::make_iterator_range(zip_begin, zip_end);
} //This doesn't do any checks and is bound to fail horribly


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
    struct modelData
    {
        std::string start_date;
        size_t timesteps;
        std::vector<std::string> additional_timeseries;
        std::map<std::string,std::vector<std::string>> index_set_dependencies;
        std::map< std::string , std::vector<double> > inputs;
        std::map< std::string, std::vector<std::pair<std::string,double> > > sparseInputs;
                
    } data ;
    
    std::string findBetween(std::string,std::string,std::string);
       
    void parse();
    void parseMagnus();
    
    void saveJson();
    void saveYaml();
    
};

#endif /* INPUTTRANSLATOR_H */

