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
#include <boost/regex.hpp>
#include <boost/optional.hpp>
#include "boost/date_time/posix_time/posix_time.hpp" 
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

//Formats of dates that boost::posix time can parse
const std::locale formats[] = 
{
    std::locale(std::locale::classic(),new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S")),
    std::locale(std::locale::classic(),new boost::posix_time::time_input_facet("%Y/%m/%d %H:%M:%S")),
    std::locale(std::locale::classic(),new boost::posix_time::time_input_facet("%d.%m.%Y %H:%M:%S")),
    std::locale(std::locale::classic(),new boost::posix_time::time_input_facet("%Y-%m-%d"))
};


struct ParameterAttributes
{
    std::string name;
    std::vector<std::string> indexers;
    std::vector<std::string> indices;
    
    ParameterAttributes(const std::string& name_) : name(name_) {};
    ParameterAttributes(){};
    

    bool operator==(const ParameterAttributes &other) const
    { return (name == other.name
              && indexers == other.indexers
              && indices == other.indices);
    }

};    

struct KeyHasher
{
    std::size_t operator()(const ParameterAttributes& k) const
    {
        std::stringstream ss;
        ss << k.name;
        for(auto&i : k.indexers) ss << i;
        return (std::hash<std::string>()(ss.str()));
    }
};


/*The way this class works is to read in inputs into a custom struct/map that can 
 * then be broadcast to the different type formats
 */
class InputTranslator {
public:
    InputTranslator(const std::string filename_);
    InputTranslator();
    InputTranslator(const InputTranslator& orig);
    virtual ~InputTranslator();
    
    void setDataset(inca_data_set*);
    
private:
    //Allowable formats for Inputs
    enum format{
        magnus,
        yaml,
        json
    };
    
    format fileFormat;
    
    //Dataset where the inputs will be placed
    inca_data_set* DataSet;
    
    //File from which the data will be read
    const std::string filename;
    
    //Determining file type
    void getFormat();
    
    struct modelData
    {
        boost::optional<boost::posix_time::ptime> start_date;
        boost::optional<size_t> timesteps;
        std::vector<std::string> additional_timeseries;
        std::map<std::string,std::vector<std::string>> index_set_dependencies;
//        std::map< std::string , std::vector<double> > inputs;
//        std::map< std::string, std::vector<std::pair<std::string,double> > > sparseInputs;

        std::unordered_map<ParameterAttributes, std::vector<double>, KeyHasher > inputs;
        std::unordered_map<ParameterAttributes, std::vector<std::pair<std::string,double> >, KeyHasher > sparseInputs;
        

        
        
        bool isAny(){
            return !(start_date  &&
                    timesteps  &&
                    additional_timeseries.empty() &&
                    inputs.empty() &&
                    sparseInputs.empty()
                    );
        }
                
    } data ;
    
    std::string findBetween(std::string,std::string,std::string);
       
    void parse();
    void parseMagnus();
    void parseJson();
    void parseYaml();
    
    
    template<typename T>
    std::vector<T> getSequence(YAML::Node& node)
    {
        std::vector<T> vec;
        assert(node.IsSequence());
        for(auto i = 0; i < node.size(); ++i)
        {
            vec.push_back(node[i].as<T>());
        }
        return vec;
    };
   
    
    template<typename T>
    void prettyInsert(YAML::Emitter& out,const std::vector<T>& vec) 
    {
        if (vec.size() > 10)  out << YAML::Flow;
        out << YAML::BeginSeq;
        for(auto const &i: vec) out << i;
        out << YAML::EndSeq;        
    };
    
    
    
    void saveJson();
    void saveYaml();
    
    
    void putInDataset();
    
};

template<>
std::vector<std::pair<std::string,double>> InputTranslator::getSequence(YAML::Node& node)
{
    std::vector<std::pair<std::string,double>> vec;
    assert(node.IsSequence());
    for (auto i = 0; i< node.size(); ++ i)
    {
        vec.emplace_back(std::make_pair(
                   node[i][0].as<std::string>(),
                   node[i][1].as<double>()
                                        )
                        );
    }
    return vec;
}


#endif /* INPUTTRANSLATOR_H */

