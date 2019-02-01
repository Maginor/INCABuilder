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
    struct modelData
    {
        std::string start_date;
        size_t timesteps;
        std::vector<std::string> additional_timeseries;
        std::map<std::string,std::vector<std::string>> index_set_dependencies;
        std::map< size_t , std::vector<double> > inputs;
        std::map< size_t, std::vector<std::pair<std::string,double> > > sparseInputs;
                
    } data ;
    
    typedef std::pair<std::string,std::string> idxidx;
    
    
    typedef boost::tuple<
    std::vector<double>::const_iterator,
    std::vector<double>::const_iterator
    > the_iterator_tuple;

  typedef boost::zip_iterator<
    the_iterator_tuple
    > the_zip_iterator;

  typedef boost::transform_iterator<
    [](return void;),
    the_zip_iterator
    > the_transform_iterator;

  the_transform_iterator it_begin(
    the_zip_iterator(
      the_iterator_tuple(
        vect_1.begin(),
        vect_2.begin()
        )
      ),
    tuple_multiplies<double>()
    );

  the_transform_iterator it_end(
    the_zip_iterator(
      the_iterator_tuple(
        vect_1.end(),
        vect_2.end()
        )
      ),
    tuple_multiplies<double>()
    );
    
    
   //std::function<std::string(> 
    
    
    struct Key
    {
      std::string& first;
      std::vector<idxidx>& second;
      std::function<std::string(idxidx&)> one = [](idxidx& i){return i.first;};
      std::function<std::string(idxidx&)> two = [](idxidx& i){return i.second;};
      bool operator==(const Key &other) const
      { return (first == other.first
                && second == other.second
                );
      }
    };
    
    
    struct myHash
    {
        std::size_t operator()(const Key& k) const;
    };
    
    std::string findBetween(std::string,std::string,std::string);
    static std::stringstream ss;
    
    void parse();
    void parseMagnus();
    
    
    
       
           
};

#endif /* INPUTTRANSLATOR_H */

