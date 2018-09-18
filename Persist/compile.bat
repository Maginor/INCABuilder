@echo off

C:\Mingw\bin\g++ persist.cpp -O2 -fno-exceptions -Werror=return-type -o persist.exe ../sqlite3/sqlite3.o -fmax-errors=5