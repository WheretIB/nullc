#include "TestBase.h"

#include "../NULLC/StrAlgo.h"

struct TestLongVariable : TestQueue
{
	virtual void Run()
	{
		char code[8192];
		char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
		for(unsigned int i = 0; i < NULLC_MAX_VARIABLE_NAME_LENGTH; i++)
			name[i] = 'a';
		name[NULLC_MAX_VARIABLE_NAME_LENGTH - 1] = 0;
		NULLC::SafeSprintf(code, 8192, "int %s = 12; return %s;", name, name);
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;

			testsCount[t]++;
			if(Tests::RunCode(code, testTarget[t], "12", "Long variable name."))
				testsPassed[t]++;
		}
	}
};
TestLongVariable testLongVariable;

struct TestLongFunction : TestQueue
{
	virtual void Run()
	{
		char code[8192];
		char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
		for(unsigned int i = 0; i < NULLC_MAX_VARIABLE_NAME_LENGTH; i++)
			name[i] = 'a';
		name[NULLC_MAX_VARIABLE_NAME_LENGTH - 1] = 0;
		NULLC::SafeSprintf(code, 8192, "void foo(int bar){ int %s(){ return bar; } int %s(int u){ return bar + u; } } return 1;", name, name);
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;

			testsCount[t]++;
			if(Tests::RunCode(code, testTarget[t], "1", "Long function name."))
				testsPassed[t]++;
		}
	}
};
TestLongFunction testLongFunction;
