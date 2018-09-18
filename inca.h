


//NOTE: This is designed to be built as a unity build, i.e. in only a single compilation unit. We may split things up into separate compilation units with proper headers later.

#if !defined(INCA_H)

#include "inca_model.h"
#include "inca_data_set.cpp"
#include "inca_model.cpp"
#include "inca_io.cpp"
#include "inca_solvers.h"
#include "inca_math.h"

#define INCA_H
#endif