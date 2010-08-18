#include "TestBase.h"

struct TestLongVariable : TestQueue
{
	virtual void Run()
	{
		char code[8192];
		char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
		for(unsigned int i = 0; i < NULLC_MAX_VARIABLE_NAME_LENGTH; i++)
			name[i] = 'a';
		name[NULLC_MAX_VARIABLE_NAME_LENGTH - 1] = 0;
		SafeSprintf(code, 8192, "int %s = 12; return %s;", name, name);
		for(int t = 0; t < 2; t++)
		{
			testsCount[t]++;
			if(Tests::RunCode(code, t, "12", "Long variable name."))
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
		SafeSprintf(code, 8192, "void foo(int bar){ int %s(){ return bar; } int %s(int u){ return bar + u; } } return 1;", name, name);
		for(int t = 0; t < 2; t++)
		{
			testsCount[t]++;
			if(Tests::RunCode(code, t, "1", "Long function name."))
				testsPassed[t]++;
		}
	}
};
TestLongFunction testLongFunction;
