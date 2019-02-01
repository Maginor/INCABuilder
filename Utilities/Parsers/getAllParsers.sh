#! /bin/bash

START=$PWD

#Getting yaml parser
git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp
mkdir build
cd build
cmake .. -DBUILD_SHARED_LIBS=ON
make
cp -P *.so ../../lib
cd ..
mkdir buildStatic
cd buildStatic
cmake ..
make

cd $START
cp *.a ../lib/
rm -rf ../include/yaml-cpp/
cp -rf yaml-cpp/include/yaml-cpp ../include/yaml-cpp/

git clone https://github.com/nlohmann/json.git
cd json
mkdir build 
cd build
cmake ..
make

cd $START
rm -rf ../include/nlohmann
mkdir ../include/nlohmann
cp -rf json/include/nlohmann/ ../include/





