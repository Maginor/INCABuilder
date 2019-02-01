#! /bin/bash
rm -rf boost*
wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2
tar -xf boost*
rm *.tar.bz2
cd boost_1_69_0
./bootstrap.sh
./b2 --with-regex
cd ..
rm -rf include/boost/
mkdir include/boost/
mv boost_1_69_0/boost/ include/
cp boost_1_69_0/stage/lib/* lib/Unix
rm -r boost*
