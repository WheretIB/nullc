#include "runtime.h"

#include <time.h>

#ifdef WIN32
	#include <Windows.h>
	#include <MMSystem.h>
	#pragma comment(lib, "Winmm.lib")
#endif

int clock(void* unused)
{
	return ::clock() * 1000 / CLOCKS_PER_SEC;
}
double clock_precise(void* unused)
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
