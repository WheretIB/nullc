#include "TestBase.h"

#ifndef _DEBUG
	#define FAILURE_TEST
#endif

#ifdef FAILURE_TEST

const char	*testDivZeroInt = 
"// Division by zero handling\r\n\
int a = 5, b = 0;\r\n\
return a/b;";
TEST_RUNTIME_FAIL("Division by zero handling 1", testDivZeroInt, "ERROR: integer division by zero");

const char	*testDivZeroLong = 
"// Division by zero handling\r\n\
long a = 5, b = 0;\r\n\
return a / b;";
TEST_RUNTIME_FAIL("Division by zero handling 2", testDivZeroLong, "ERROR: integer division by zero");

const char	*testModZeroInt = 
"// Division by zero handling\r\n\
int a = 5, b = 0;\r\n\
return a % b;";
TEST_RUNTIME_FAIL("Modulus division by zero handling 1", testModZeroInt, "ERROR: integer division by zero");

const char	*testModZeroLong = 
"// Division by zero handling\r\n\
long a = 5, b = 0;\r\n\
return a % b;";
TEST_RUNTIME_FAIL("Modulus division by zero handling 2", testModZeroLong, "ERROR: integer division by zero");

const char	*testFuncNoReturn = 
"// Function with no return handling\r\n\
int test(){ 1; } // temporary\r\n\
return test();";
TEST_RUNTIME_FAIL("Function with no return handling", testFuncNoReturn, "ERROR: function didn't return a value");

const char	*testBounds1 = 
"// Array out of bound check \r\n\
int[4] n;\r\n\
int i = 4;\r\n\
n[i] = 3;\r\n\
return 1;";
TEST_RUNTIME_FAIL("Array out of bounds error check 1", testBounds1, "ERROR: array index out of bounds");

const char	*testBounds2 = 
"// Array out of bound check 2\r\n\
int[4] n;\r\n\
int[] nn = n;\r\n\
int i = 4;\r\n\
nn[i] = 3;\r\n\
return 1;";
TEST_RUNTIME_FAIL("Array out of bounds error check 2", testBounds2, "ERROR: array index out of bounds");

const char	*testBounds3 = 
"// Array out of bound check 3\r\n\
auto x0 = { 1, 2 };\r\n\
auto x1 = { 1, 2, 3 };\r\n\
auto x2 = { 1, 2, 3, 4 };\r\n\
int[3][] x;\r\n\
x[0] = x0;\r\n\
x[1] = x1;\r\n\
x[2] = x2;\r\n\
int[][] xr = x;\r\n\
return xr[1][3];";
TEST_RUNTIME_FAIL("Array out of bounds error check 3", testBounds3, "ERROR: array index out of bounds");

const char	*testInvalidFuncPtr = 
"int ref(int) a;\r\n\
return a(5);";
TEST_RUNTIME_FAIL("Invalid function pointer check", testInvalidFuncPtr, "ERROR: invalid function pointer");

const char	*testAutoReferenceMismatch =
"int a = 17;\r\n\
auto ref d = &a;\r\n\
double ref ll = d;\r\n\
return *ll;";
TEST_RUNTIME_FAIL("Auto reference type mismatch", testAutoReferenceMismatch, "ERROR: cannot convert from int ref to double ref");

const char	*testFunctionIsNotACoroutine = "for(i in auto(){return 1;}){}";
TEST_RUNTIME_FAIL("Iteration overa a function that is not a coroutine", testFunctionIsNotACoroutine, "ERROR: function is not a coroutine");

const char	*testTypeDoesntImplementMethod =
"class Foo{ int i; }\r\n\
void Foo:test(){ assert(i); }\r\n\
Foo test; test.i = 5;\r\n\
auto ref[2] objs;\r\n\
objs[0] = &test;\r\n\
objs[1] = &test.i;\r\n\
for(i in objs)\r\n\
	i.test();\r\n\
return 0;";
TEST_RUNTIME_FAIL("Type doesn't implement method on auto ref function call", testTypeDoesntImplementMethod, "ERROR: type 'int' doesn't implement method 'int::test' of type 'void ref()'");

const char	*testAutoArrayConversionFail = "int x = 12; auto[] arr = x; return 0;";
TEST_RUNTIME_FAIL("Array to auto[] type conversion fail", testAutoArrayConversionFail, "ERROR: cannot convert from 'int' to 'auto[]'");

const char	*testAutoArrayOutOfBounds = "auto str = \"Hello\"; auto[] arr = str; return char(arr[-1]) - 'l';";
TEST_RUNTIME_FAIL("auto[] type underflow", testAutoArrayOutOfBounds, "ERROR: array index out of bounds");

const char	*testAutoArrayOutOfBounds2 = "auto str = \"Hello\"; auto[] arr = str; return char(arr[7]) - 'l';";
TEST_RUNTIME_FAIL("auto[] type overflow 2", testAutoArrayOutOfBounds2, "ERROR: array index out of bounds");

const char	*testAutoArrayConversionFail2 = "auto str = \"Hello\"; auto[] arr = str; int str2 = arr; return 0;";
TEST_RUNTIME_FAIL("auto[] type conversion mismatch 1", testAutoArrayConversionFail2, "ERROR: cannot convert from 'auto[]' to 'int'");

const char	*testAutoArrayConversionFail3 = "auto str = \"Hello\"; auto[] arr = str; char[7] str2 = arr; return 0;";
TEST_RUNTIME_FAIL("auto[] type conversion mismatch 2", testAutoArrayConversionFail3, "ERROR: cannot convert from 'auto[]' (actual type 'char[6]') to 'char[7]'");

const char	*testAutoArrayConversionFail4 = "auto str = \"Hello\"; auto[] arr = str; int[] str2 = arr; return 0;";
TEST_RUNTIME_FAIL("auto[] type conversion mismatch 3", testAutoArrayConversionFail4, "ERROR: cannot convert from 'auto[]' (actual type 'char[6]') to 'int[]'");

const char	*testInvalidPointer = 
"class Test{ int a, b; }\r\n\
Test ref x;\r\n\
return x.b;";
TEST_RUNTIME_FAIL("Invalid pointer check", testInvalidPointer, "ERROR: null pointer access");

void RecallerTransition(int x)
{
	nullcRunFunction("inside", x);
}

LOAD_MODULE_BIND(func_testX, "func.testX", "void recall(int x);")
{
	nullcBindModuleFunction("func.testX", (void(*)())RecallerTransition, "recall", 0);
}
const char	*testCallStackWhenVariousTransitions =
"import func.testX;\r\n\
void inside(int x)\r\n\
{\r\n\
	assert(x);\r\n\
	recall(x-1);\r\n\
}\r\n\
recall(2);\r\n\
return 0;";
const char *error = "Assertion failed\r\n\
Call stack:\r\n\
global scope (at recall(2);)\r\n\
inside (at recall(x-1);)\r\n\
 param 0: int x (at base+0 size 4)\r\n\
inside (at recall(x-1);)\r\n\
 param 0: int x (at base+0 size 4)\r\n\
inside (at assert(x);)\r\n\
 param 0: int x (at base+0 size 4)\r\n";
struct Test_testMultipleTransiotions : TestQueue
{
	virtual void Run()
	{
		if(Tests::messageVerbose)
			printf("Call stack when there are various transitions between NULLC and C\r\n");
		for(int t = 0; t < TEST_COUNT; t++)
		{
			testsCount[t]++;
			nullcSetExecutor(testTarget[t]);
			nullres good = nullcBuild(testCallStackWhenVariousTransitions);
			if(!good)
			{
				if(!Tests::messageVerbose)
					printf("Call stack when there are various transitions between NULLC and C\r\n");
				printf("Compilation failed: %s\r\n", nullcGetLastError());
				break;
			}
			good = nullcRun();
			if(!good)
			{
				if(strcmp(error, nullcGetLastError()) != 0)
				{
					if(!Tests::messageVerbose)
						printf("Call stack when there are various transitions between NULLC and C\r\n");
					printf("%s failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", testTarget[t] == NULLC_VM ? "VM " : "X86", nullcGetLastError(), error);
				}else{
					testsPassed[t]++;
				}
			}else{
				if(!Tests::messageVerbose)
					printf("Call stack when there are various transitions between NULLC and C\r\n");
				printf("Test should have failed.\r\n");
			}
		}
	}
};
Test_testMultipleTransiotions test_testMultipleTransiotions;

#endif

#if defined(FAILURE_TEST) && defined(NULLC_BUILD_X86_JIT)

const char	*testDepthOverflow = 
"int fib(int n)\r\n\
{\r\n\
	if(!n)\r\n\
		return 0;\r\n\
	return fib(n-1);\r\n\
}\r\n\
return fib(3500);";
struct Test_testDepthOverflow : TestQueue
{
	virtual void Run()
	{
		char *stackMem = new char[32*1024];
		nullcSetJiTStack(stackMem, stackMem + 32*1024, true);
		if(Tests::messageVerbose)
			printf("Call depth test\r\n");
		{
			testsCount[1]++;
			nullcSetExecutor(NULLC_X86);
			nullres good = nullcBuild(testDepthOverflow);
			assert(good);
			good = nullcRun();
			if(!good)
			{
				const char *error = "ERROR: allocated stack overflow";
				char buf[512];\
				strcpy(buf, strstr(nullcGetLastError(), "ERROR:"));
				if(char *lineEnd = strchr(buf, '\r'))
					*lineEnd = 0;
				if(strcmp(error, buf) != 0)
				{
					if(!Tests::messageVerbose)
						printf("Call depth test\r\n");
					printf("X86 failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", buf, error);
				}else{
					testsPassed[1]++;
				}
			}else{
				if(!Tests::messageVerbose)
					printf("Call depth test\r\n");
				printf("Test should have failed.\r\n");
			}
		}
		delete[] stackMem;
	}
};
Test_testDepthOverflow test_testDepthOverflow;

const char	*testGlobalOverflow = 
"double clamp(double a, double min, double max)\r\n\
{\r\n\
  if(a < min)\r\n\
    return min;\r\n\
  if(a > max)\r\n\
    return max;\r\n\
  return a;\r\n\
}\r\n\
double abs(double x)\r\n\
{\r\n\
  if(x < 0.0)\r\n\
    return -x;\r\n\
  return x;\r\n\
}\r\n\
double[2700] res;\r\n\
return clamp(abs(-1.5), 0.0, 1.0);";
struct Test_testGlobalOverflow : TestQueue
{
	virtual void Run()
	{
		char *stackMem = new char[32*1024];
		if(Tests::messageVerbose)
			printf("Global overflow test\r\n");
		{
			testsCount[1]++;
			nullcSetExecutor(NULLC_X86);
			nullres good = nullcBuild(testGlobalOverflow);
			assert(good);
			good = nullcRun();
			if(!good)
			{
				const char *error = "ERROR: allocated stack overflow";
				char buf[512];\
				strcpy(buf, strstr(nullcGetLastError(), "ERROR:"));
				if(char *lineEnd = strchr(buf, '\r'))
					*lineEnd = 0;
				if(strcmp(error, buf) != 0)
				{
					if(!Tests::messageVerbose)
						printf("Global overflow test\r\n");
					printf("X86 failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", buf, error);
				}else{
					testsPassed[1]++;
				}
			}else{
				if(!Tests::messageVerbose)
					printf("Global overflow test\r\n");
				printf("Test should have failed.\r\n");
			}
		}
		delete[] stackMem;
	}
};
Test_testGlobalOverflow test_testGlobalOverflow;

const char	*testDepthOverflowUnmanaged = 
"int fib(int n)\r\n\
{\r\n\
	int[1024] arr;\r\n\
	if(!n)\r\n\
		return 0;\r\n\
	return fib(n-1);\r\n\
}\r\n\
return fib(3500);";
struct Test_testDepthOverflowUnmanaged : TestQueue
{
	virtual void Run()
	{
		nullcSetJiTStack((void*)0x20000000, (void*)(0x20000000 + 1024*1024), false);
		if(Tests::messageVerbose)
			printf("Depth overflow in unmanaged memory\r\n");
		{
			testsCount[1]++;
			nullcSetExecutor(NULLC_X86);
			nullres good = nullcBuild(testDepthOverflowUnmanaged);
			assert(good);
			good = nullcRun();
			if(!good)
			{
				const char *error = "ERROR: failed to reserve new stack memory";
				char buf[512];\
				strcpy(buf, strstr(nullcGetLastError(), "ERROR:"));
				if(char *lineEnd = strchr(buf, '\r'))
					*lineEnd = 0;
				if(strcmp(error, buf) != 0)
				{
					if(Tests::messageVerbose)
						printf("Depth overflow in unmanaged memory\r\n");
					printf("X86 failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", buf, error);
				}else{
					testsPassed[1]++;
				}
			}else{
				if(Tests::messageVerbose)
					printf("Depth overflow in unmanaged memory\r\n");
				printf("Test should have failed.\r\n");
			}
		}

		nullcSetJiTStack((void*)0x20000000, NULL, false);
	}
};
Test_testDepthOverflowUnmanaged test_testDepthOverflowUnmanaged;

#endif
