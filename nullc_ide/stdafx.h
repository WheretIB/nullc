#pragma once
#pragma warning(disable: 4275)
#pragma warning(disable: 4005)

#include <stdlib.h>

#define _HAS_EXCEPTIONS 0

#include <string>
#include <sstream>
#include <math.h>
#include <vector>

#include <assert.h>
#include <time.h>

double myGetPreciseTime();
size_t safeprintf(char* dst, size_t size, const char* src, ...);
