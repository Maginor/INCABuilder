@echo off
g++ persist.cpp -O2 -std=c++11 -o persist.exe ../../sqlite3/sqlite3.o -fmax-errors=5