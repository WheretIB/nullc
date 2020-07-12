#include "time.h"

#include "../nullc.h"
#include "../nullbind.h"

#include <time.h>

#if defined(_WIN32)
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
#if defined(_WIN32)
		LARGE_INTEGER freq, count;
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&count);
		double temp = double(count.QuadPart) / double(freq.QuadPart);
		return temp * 1000.0;
#elif defined(__linux)
		timespec x;

		clock_gettime(CLOCK_MONOTONIC, &x);

		return x.tv_sec * 1000.0 + x.tv_nsec / 1000000.0;
#else
		return ::clock() * 1000.0 / CLOCKS_PER_SEC;
#endif
	}

#if defined(_WIN32)
	bool clockMicroReady = false;
	LARGE_INTEGER clockMicroFreq;
	LARGE_INTEGER clockMicroStart;
	double clockMicroMult = 0.0;

	void clockMicroInit()
	{
		if(clockMicroReady)
			return;

		clockMicroReady = true;

		QueryPerformanceFrequency(&clockMicroFreq);
		QueryPerformanceCounter(&clockMicroStart);

		clockMicroMult = 1.0 / clockMicroFreq.QuadPart * 1000000.0;
	}

	unsigned clockMicro()
	{
		LARGE_INTEGER count;
		QueryPerformanceCounter(&count);

		return unsigned((count.QuadPart - clockMicroStart.QuadPart) * clockMicroMult);
	}
#elif defined(__linux)
	void clockMicroInit()
	{
	}

	unsigned clockMicro()
	{
		timespec x;

		clock_gettime(CLOCK_MONOTONIC, &x);

		return x.tv_sec * 1000000 + x.tv_nsec / 1000;
	}
#else
	void clockMicroInit()
	{
	}

	unsigned clockMicro()
	{
		timespec x;

		clock_gettime(CLOCK_MONOTONIC, &x);

		return unsigned(::clock() * 1000000ll / CLOCKS_PER_SEC);
	}
#endif
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.time", NULLCTime::funcPtr, name, index)) return false;
bool	nullcInitTimeModule()
{
	REGISTER_FUNC(clock, "clock", 0);
	REGISTER_FUNC(clockPrecise, "clock_precise", 0);
	return true;
}
