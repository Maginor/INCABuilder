


//NOTE: This is designed to be built as a unity build, i.e. in only a single compilation unit. We may split things up into separate compilation units with proper headers later.


/*
TODOs:
	- PrintPartialDependencyTrace gives incorrect information sometimes when a solver is involved twice.
	- Better encapsulation of the ValueSet subsystem. Unify lookup systems for parameters, inputs, results, last_results
	- Have to figure out if the initial value equation system we have currently is good.
	- Implement stream index set specific functionality. (or not??)
	- Give warning if not all input series received values?
	- Clean up the input tokenizer. Maybe just use fscanf for reading numbers, but it is actually a little complicated since we have to figure out the type in any case.
	- Register units with inputs too? They are after all expected to be in a certain unit.
	- Allow the dependency system to be able to understand explicitly indexed lookups of results better.
	- Standardize the input format. Includes finding a better format for dates.
	- Add in pre-processing options: Tests on parameter values. Pre-compute parameter values.
	- Allow certain values to either be a parameter, input series or result, depending on model settings?
	- Remove units as model entities entirely and only store / input them as strings? They seem like an unnecessary step right now.
	- Manage the memory for all the data in the equation batch structure in such a way that it is aligned with how it will be read. (will have to not use std::vector in that case...)
*/


#ifndef INCA_H
#define INCA_H

#include <stdint.h>
#include <stdlib.h>
#include <functional>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <cmath>
#include <sstream>
#include <iomanip>

//TODO: Does this intrinsic header exist for all compilers? We only use it for __rdtsc();
#include <x86intrin.h>


typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef size_t entity_handle;
//typedef u32    index_t; //NOTE: We get an error on the 64 bit compilation with this, and I have not entirely figured out why yet
typedef size_t index_t;


//NOTE: We allow the error handling to be replaced by the application. This is for instance useful for the python wrapper.
#if !defined(INCA_PARTIAL_ERROR)
	#define INCA_PARTIAL_ERROR(Msg) \
		std::cerr << Msg;
#endif

#if !defined(INCA_FATAL_ERROR)	
	#define INCA_FATAL_ERROR(Msg) \
	{INCA_PARTIAL_ERROR(Msg) \
	exit(1);}
#endif


#include "inca_util.h"
#include "inca_model.h"
#include "inca_data_set.cpp"
#include "jacobian.cpp"
#include "inca_model.cpp"
#include "lexer.cpp"
#include "inca_io.cpp"
#include "inca_solvers.h"


#endif