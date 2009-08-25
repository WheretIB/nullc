#pragma once
/*#define SUPER_CALC_ON
#include "MemoryMan/platform.h"
#include "MemoryMan/MemoryMan.h"
#pragma comment(lib, "lib/debuglib/MemoryMan.lib")*/

#include <stdlib.h>

#pragma warning(disable: 4275)

#define _HAS_EXCEPTIONS 0

#include <string>
#include <sstream>
#include <math.h>
#include <vector>
using namespace std;

#include <assert.h>

double myGetPreciseTime();