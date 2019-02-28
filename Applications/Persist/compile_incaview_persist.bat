@echo off
g++ persist.cpp -O2 -std=c++11 -fno-exceptions -o persist.exe ../../sqlite3/sqlite3.o -fmax-errors=5