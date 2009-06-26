#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#pragma warning(disable: 4530)
#pragma warning(disable: 4127)
#endif

#ifndef _MSC_VER
#define __forceinline inline // TODO: NULLC_FORCEINLINE?
#endif

typedef unsigned int UINT;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;

#include "nullcdef.h"

#include <stdlib.h>

#include "SharedPtr/shared_ptr.hpp"

#include <vector>

#include <list>
#include <string>
#include <sstream>
#ifdef NULLC_LOG_FILES
	#include <fstream>
#endif
#include <math.h>
using namespace std;

#include <assert.h>
#ifdef NDEBUG
#undef assert
#define assert(expr)	((void)sizeof(!(expr)))
#endif

