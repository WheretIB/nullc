#include "NULLC/nullc.h"

#include "UnitTests.h"

#if defined(_MSC_VER)
#include <Windows.h>
double myGetPreciseTime()
{
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	double temp = double(count.QuadPart) / double(freq.QuadPart);
	return temp*1000.0;
}
#endif

namespace Tests
{
	extern const char *testMatch;
}

int main(int argc, char** argv)
{
	(void)argv;

	return RunTests(argc == 2, 0, argc == 3);
}
