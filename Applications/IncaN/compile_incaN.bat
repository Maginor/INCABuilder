@echo off
g++ IncaN.cpp -O2 -std=c++11 -fno-exceptions -o IncaN.exe ../../sqlite3/sqlite3.o -fmax-errors=5