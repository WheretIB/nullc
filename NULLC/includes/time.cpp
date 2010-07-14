#include "time.h"
#include "../nullc.h"
#include <time.h>

#ifdef WIN32
	#include <Windows.h>
	#include <MMSystem.h>
	#pragma comment(lib, "Winmm.lib")
#endif

namespace NULLCTime
{
	int clock()
	{
		return ::clock() * 1000 / CLOCKS_PER_SEC;
	}

	double clockPrecise()
	{
#ifdef WIN32
		LARGE_INTEGER freq, count;
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&count);
		double temp = double(count.QuadPart) / double(freq.QuadPart);
		return temp*1000.0;
#else
		return ::clock() * 1000 / CLOCKS_PER_SEC;
#endif
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("std.time", (void(*)())NULLCTime::funcPtr, name, index)) return false;
bool	nullcInitTimeModule()
{
	REGISTER_FUNC(clock, "clock", 0);
	REGISTER_FUNC(clockPrecise, "clock_precise", 0);
	return true;
}
