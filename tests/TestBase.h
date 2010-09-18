#pragma once

#include "../NULLC/nullc.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#if defined(__CELLOS_LV2__)
#	define FILE_PATH "/app_home/"
#else
#	define FILE_PATH ""
#endif

#define MODULE_PATH FILE_PATH "Modules/"

#define TEST_COUNT 3

#define TEST_FAILURE_INDEX 4
#define TEST_EXTRA_INDEX 3
#define TEST_TRANSLATION_INDEX 5

struct TestQueue
{
	TestQueue()
	{
		this->next = 0;
		if(!head)
		{
			head = tail = this;
		}else{
			tail->next = this;
			tail = this;
		}
	}

	void RunTests()
	{
		TestQueue *curr = head;
		while(curr)
		{
			curr->Run();
			curr = curr->next;
		}
	}
	virtual void Run(){}

	static TestQueue *head, *tail;
	TestQueue *next;
};

extern int testsPassed[];
extern int testsCount[];
extern unsigned int	testTarget[];

namespace Tests
{
	extern bool messageVerbose;
	extern const char *lastMessage;

	extern double timeCompile;
	extern double timeGetListing;
	extern double timeGetBytecode;
	extern double timeClean;
	extern double timeLinkCode;
	extern double timeRun;

	extern const char		*varData;
	extern unsigned int		variableCount;
	extern void				*varInfo;
	extern const char		*symbols;

	extern bool doTranslation;

	extern bool	testExecutor[TEST_COUNT];

	void*	FindVar(const char* name);
	bool	RunCode(const char *code, unsigned int executor, const char* expected, const char* message = 0, bool execShouldFail = false);
	char*	Format(const char *str, ...);
}


#define TEST_IMPL(name, code, result, count)	\
struct Test_##code : TestQueue {	\
	virtual void Run(){	\
		for(int t = 0; t < count; t++)	\
		{	\
			if(!Tests::testExecutor[t])	\
				continue;	\
			testsCount[t]++;	\
			lastFailed = false;	\
			if(!Tests::RunCode(code, t, result, name))	\
			{	\
				lastFailed = true;	\
				return;	\
			}else{	\
				RunTest();	\
			}	\
			if(!lastFailed)	\
				testsPassed[t]++;	\
		}	\
	}	\
	bool lastFailed;	\
	void RunTest();	\
};	\
Test_##code test_##code;	\
void Test_##code::RunTest()

#define TEST_VM(name, code, result) TEST_IMPL(name, code, result, 1)
#define TEST(name, code, result) TEST_IMPL(name, code, result, TEST_COUNT)

#define TEST_RESULT(name, code, result)	\
struct Test_##code : TestQueue {	\
	virtual void Run(){	\
		for(int t = 0; t < TEST_COUNT; t++)	\
		{	\
			if(!Tests::testExecutor[t])	\
				continue;	\
			testsCount[t]++;	\
			if(Tests::RunCode(code, t, result, name))	\
				testsPassed[t]++;	\
		}	\
	}	\
};	\
Test_##code test_##code;

#define LOAD_MODULE(id, name, code)	\
struct Test_##id : TestQueue {	\
	virtual void Run(){	\
		testsCount[TEST_EXTRA_INDEX]++;	\
		if(nullcLoadModuleBySource(name, code))	\
			testsPassed[TEST_EXTRA_INDEX]++;	\
		else	\
			printf("Test "name" failed\n");	\
	}	\
};	\
Test_##id test_##id;

#define LOAD_MODULE_BIND(id, name, code)	\
struct Test_##id : TestQueue {	\
	virtual void Run(){	\
		testsCount[TEST_EXTRA_INDEX]++;	\
		if(nullcLoadModuleBySource(name, code))	\
		{	\
			testsPassed[TEST_EXTRA_INDEX]++;	\
			RunTest();	\
		}else{	\
			printf("Test "name" failed\n");	\
		}	\
	}	\
	bool lastFailed;	\
	void RunTest();	\
};	\
Test_##id test_##id;	\
void Test_##id::RunTest()

#define TEST_RELOCATE(name, code, result)	\
struct Test_##code : TestQueue {	\
	virtual void Run(){	\
		testsCount[0]++;	\
		nullcTerminate();	\
		nullcInit(MODULE_PATH);	\
		nullcInitTypeinfoModule();	\
		nullcInitVectorModule();	\
		if(Tests::RunCode(code, 0, result, name))	\
			testsPassed[0]++;	\
	}	\
};	\
Test_##code test_##code;

#define TEST_NAME() if(Tests::lastMessage) printf("%s\r\n", Tests::lastMessage);
#define CHECK_DOUBLE(var, index, expected) if(fabs(((double*)Tests::FindVar(var))[index] - (expected)) > 1e-6){ TEST_NAME(); printf(" Failed %s[%d] == %f (got %f)\r\n", #var, index, (double)expected, ((double*)Tests::FindVar(var))[index]); lastFailed = true; }
#define CHECK_FLOAT(var, index, expected) if(((float*)Tests::FindVar(var))[index] != (expected)){ TEST_NAME(); printf(" Failed %s[%d] == %f (got %f)\r\n", #var, index, (double)expected, (double)((float*)Tests::FindVar(var))[index]); lastFailed = true; }
#define CHECK_LONG(var, index, expected) if(((long long*)Tests::FindVar(var))[index] != (expected)){ TEST_NAME(); printf(" Failed %s[%d] == %lld (got %lld)\r\n", #var, index, (long long)expected, ((long long*)Tests::FindVar(var))[index]); lastFailed = true; }
#define CHECK_INT(var, index, expected) if(((int*)Tests::FindVar(var))[index] != (expected)){ TEST_NAME(); printf(" Failed %s[%d] == %d (got %d)\r\n", #var, index, expected, ((int*)Tests::FindVar(var))[index]); lastFailed = true; }
#define CHECK_SHORT(var, index, expected) if(((short*)Tests::FindVar(var))[index] != (expected)){ TEST_NAME(); printf(" Failed %s[%d] == %d (got %d)\r\n", #var, index, expected, ((short*)Tests::FindVar(var))[index]); lastFailed = true; }
#define CHECK_CHAR(var, index, expected) if(((char*)Tests::FindVar(var))[index] != (expected)){ TEST_NAME(); printf(" Failed %s[%d] == %d (got %d)\r\n", #var, index, expected, ((char*)Tests::FindVar(var))[index]); lastFailed = true; }
#define CHECK_STR(var, index, expected) if(strcmp(((char*)Tests::FindVar(var)+index), (expected)) != 0){ TEST_NAME(); printf(" Failed %s[%d] == %s (got %s)\r\n", #var, index, expected, ((char*)Tests::FindVar(var))+index); lastFailed = true; }

#define TEST_RUNTIME_FAIL(name, code, result)	\
struct Test_##code : TestQueue {	\
	virtual void Run(){	\
		for(int t = 0; t < 2; t++)	\
		{	\
			testsCount[t]++;	\
			if(Tests::RunCode(code, t, result, name, true))	\
				testsPassed[t]++;	\
		}	\
	}	\
};	\
Test_##code test_##code;

#define TEST_FOR_FAIL(name, str, error)\
{\
	testsCount[TEST_FAILURE_INDEX]++;\
	nullres good = nullcCompile(str);\
	if(!good)\
	{\
		char buf[512];\
		strcpy(buf, strstr(nullcGetLastError(), "ERROR:"));\
		if(char *lineEnd = strchr(buf, '\r'))\
			*lineEnd = 0;\
		if(strcmp(error, buf) != 0)\
		{\
			printf("Failed %s but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", name, buf, error);\
		}else{\
			testsPassed[TEST_FAILURE_INDEX]++;\
		}\
	}else{\
		printf("Test \"%s\" failed to fail.\r\n", name);\
	}\
}
