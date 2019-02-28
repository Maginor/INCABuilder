@echo off
g++ incaviewsimplyp.cpp -O2 -std=c++11 -fno-exceptions -o incaviewsimplyp.exe ../../sqlite3/sqlite3.o -fmax-errors=5