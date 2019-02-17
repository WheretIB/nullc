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

#if !defined(SANITIZE_FUZZER)

int main(int argc, char** argv)
{
	(void)argv;

	return RunTests(argc == 2, 0, argc == 3);
}

#else

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

bool testInit = false;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	char *code = new char[size + 1];
	memcpy(code, data, size);
	code[size] = 0;

	if(!testInit)
		nullcInit("Modules/");

	if(nullcCompile(code))
	{
		char exprResult[256];
		bool exprDone = false;
		if(nullcTestEvaluateExpressionTree(exprResult, 256))
		{
			exprDone = true;
		}

		char instResult[256];
		if(nullcTestEvaluateInstructionTree(instResult, 256))
		{
			if(exprDone)
				assert(strcmp(exprResult, instResult) == 0);
		}
	}

	nullcClean();

	delete[] code;

	return 0;  // Non-zero return values are reserved for future use.
}

#endif
