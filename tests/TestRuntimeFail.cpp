#include "TestBase.h"

#include <assert.h>

const char	*testDivZeroInt = 
"// Division by zero handling\r\n\
int a = 5, b = 0;\r\n\
return a/b;";
TEST_RUNTIME_FAIL("Division by zero handling 1 [failure handling]", testDivZeroInt, "ERROR: integer division by zero");

const char	*testDivZeroLong = 
"// Division by zero handling\r\n\
long a = 5, b = 0;\r\n\
return a / b;";
TEST_RUNTIME_FAIL("Division by zero handling 2 [failure handling]", testDivZeroLong, "ERROR: integer division by zero");

const char	*testModZeroInt = 
"// Division by zero handling\r\n\
int a = 5, b = 0;\r\n\
return a % b;";
TEST_RUNTIME_FAIL("Modulus division by zero handling 1 [failure handling]", testModZeroInt, "ERROR: integer division by zero");

const char	*testModZeroLong = 
"// Division by zero handling\r\n\
long a = 5, b = 0;\r\n\
return a % b;";
TEST_RUNTIME_FAIL("Modulus division by zero handling 2 [failure handling]", testModZeroLong, "ERROR: integer division by zero");

const char	*testFuncNoReturn = 
"// Function with no return handling\r\n\
int test(){ if(0) return 2; } // temporary\r\n\
return test();";
TEST_RUNTIME_FAIL("Function with no return handling [failure handling]", testFuncNoReturn, "ERROR: function didn't return a value");

const char	*testBounds1 = 
"// Array out of bound check \r\n\
int[4] n;\r\n\
int i = 4;\r\n\
n[i] = 3;\r\n\
return 1;";
TEST_RUNTIME_FAIL("Array out of bounds error check 1 [failure handling]", testBounds1, "ERROR: array index out of bounds");

const char	*testBounds2 = 
"// Array out of bound check 2\r\n\
int[4] n;\r\n\
int[] nn = n;\r\n\
int i = 4;\r\n\
nn[i] = 3;\r\n\
return 1;";
TEST_RUNTIME_FAIL("Array out of bounds error check 2 [failure handling]", testBounds2, "ERROR: array index out of bounds");

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
TEST_RUNTIME_FAIL("Array out of bounds error check 3 [failure handling]", testBounds3, "ERROR: array index out of bounds");

const char	*testBounds4 =
"int[10] arr; int index = -1024; return arr[index];";
TEST_RUNTIME_FAIL("Array out of bounds error check 4 [failure handling]", testBounds4, "ERROR: array index out of bounds");

const char	*testBounds5 =
"int[10] arr; int foo(){ return -1024; } int index = foo(); return arr[index];";
TEST_RUNTIME_FAIL("Array out of bounds error check 5 [failure handling]", testBounds5, "ERROR: array index out of bounds");

const char	*testInvalidFuncPtr1 = 
"int ref(int) a;\r\n\
return a(5);";
TEST_RUNTIME_FAIL("Invalid function pointer check 1 [failure handling]", testInvalidFuncPtr1, "ERROR: invalid function pointer");

const char	*testInvalidFuncPtr2 = 
"int foo(){ return 2; } int x = 0; auto a = x ? foo : nullptr; return a();";
TEST_RUNTIME_FAIL("Invalid function pointer check 2 [failure handling]", testInvalidFuncPtr2, "ERROR: invalid function pointer");

const char	*testAutoReferenceMismatch =
"int a = 17;\r\n\
auto ref d = &a;\r\n\
double ref ll = d;\r\n\
return *ll;";
TEST_RUNTIME_FAIL("Auto reference type mismatch [failure handling]", testAutoReferenceMismatch, "ERROR: cannot convert from int ref to double ref");

const char	*testFunctionIsNotACoroutine = "for(i in auto(){return 1;}){}";
TEST_RUNTIME_FAIL("Iteration over a a function that is not a coroutine [failure handling]", testFunctionIsNotACoroutine, "ERROR: function is not a coroutine");

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
TEST_RUNTIME_FAIL("Type doesn't implement method on auto ref function call [failure handling]", testTypeDoesntImplementMethod, "ERROR: type 'int' doesn't implement method 'int::test' of type 'void ref()'");

const char	*testAutoArrayOutOfBounds = "auto str = \"Hello\"; auto[] arr = str; return char(arr[-1]) - 'l';";
TEST_RUNTIME_FAIL("auto[] type underflow [failure handling]", testAutoArrayOutOfBounds, "ERROR: array index out of bounds");

const char	*testAutoArrayOutOfBounds2 = "auto str = \"Hello\"; auto[] arr = str; return char(arr[7]) - 'l';";
TEST_RUNTIME_FAIL("auto[] type overflow 2 [failure handling]", testAutoArrayOutOfBounds2, "ERROR: array index out of bounds");

const char	*testAutoArrayConversionFail3 = "auto str = \"Hello\"; auto[] arr = str; char[7] str2 = arr; return 0;";
TEST_RUNTIME_FAIL("auto[] type conversion mismatch 2 [failure handling]", testAutoArrayConversionFail3, "ERROR: cannot convert from 'auto[]' (actual type 'char[6]') to 'char[7]'");

const char	*testAutoArrayConversionFail4 = "auto str = \"Hello\"; auto[] arr = str; int[] str2 = arr; return 0;";
TEST_RUNTIME_FAIL("auto[] type conversion mismatch 3 [failure handling]", testAutoArrayConversionFail4, "ERROR: cannot convert from 'auto[]' (actual type 'char[6]') to 'int[]'");

const char	*testAutoRefFail = "class X{} auto ref x = 5; X a = X(x); return 1;";
TEST_RUNTIME_FAIL("Auto reference type mismatch 2 [failure handling]", testAutoRefFail, "ERROR: cannot convert from int ref to X ref");

const char	*testInvalidPointer = 
"class Test{ int a, b; }\r\n\
Test ref x;\r\n\
return x.b;";
TEST_HARD_RUNTIME_FAIL("Invalid pointer check [failure handling]", testInvalidPointer, "ERROR: null pointer access");

const char	*testInvalidPointerLoadOps =
"import std.error;\r\n\
\r\n\
auto a01(char ref b){ return *b; }\r\n\
auto a02(short ref b){ return *b; }\r\n\
auto a03(int ref b){ return *b; }\r\n\
auto a04(long ref b){ return *b; }\r\n\
auto a05(float ref b){ return *b; }\r\n\
auto a06(double ref b){ return *b; }\r\n\
\r\n\
try(<> a01(nullptr));\r\n\
try(<> a02(nullptr));\r\n\
try(<> a03(nullptr));\r\n\
try(<> a04(nullptr));\r\n\
try(<> a05(nullptr));\r\n\
try(<> a06(nullptr));\r\n\
\r\n\
return a01(nullptr);";
TEST_HARD_RUNTIME_FAIL("Invalid pointer check coverage (load) [failure handling]", testInvalidPointerLoadOps, "ERROR: null pointer access");

const char	*testInvalidPointerStoreOps =
"import std.error;\r\n\
\r\n\
auto a01(char a, char ref b){ return *b = a; }\r\n\
auto a02(char a, short ref b){ return *b = a; }\r\n\
auto a03(char a, int ref b){ return *b = a; }\r\n\
auto a04(char a, long ref b){ return *b = a; }\r\n\
auto a05(char a, float ref b){ return *b = a; }\r\n\
auto a06(char a, double ref b){ return *b = a; }\r\n\
\r\n\
try(<> a01(2, nullptr));\r\n\
try(<> a02(2, nullptr));\r\n\
try(<> a03(2, nullptr));\r\n\
try(<> a04(2, nullptr));\r\n\
try(<> a05(2, nullptr));\r\n\
try(<> a06(2, nullptr));\r\n\
\r\n\
return a01(2, nullptr);";
TEST_HARD_RUNTIME_FAIL("Invalid pointer check coverage (store) [failure handling]", testInvalidPointerStoreOps, "ERROR: null pointer access");

const char	*testInvalidPointerIntOps =
"import std.error;\r\n\
\r\n\
int a01(int a, int ref b){ return a + *b; }\r\n\
int a02(int a, int ref b){ return a - *b; }\r\n\
int a03(int a, int ref b){ return a * *b; }\r\n\
int a04(int a, int ref b){ return a / *b; }\r\n\
int a05(int a, int ref b){ return a % *b; }\r\n\
int a06(int a, int ref b){ return a ** *b; }\r\n\
int a07(int a, int ref b){ return a < *b; }\r\n\
int a08(int a, int ref b){ return a > *b; }\r\n\
int a09(int a, int ref b){ return a <= *b; }\r\n\
int a10(int a, int ref b){ return a >= *b; }\r\n\
int a11(int a, int ref b){ return a == *b; }\r\n\
int a12(int a, int ref b){ return a != *b; }\r\n\
int a13(int a, int ref b){ return a << *b; }\r\n\
int a14(int a, int ref b){ return a >> *b; }\r\n\
int a15(int a, int ref b){ return a & *b; }\r\n\
int a16(int a, int ref b){ return a | *b; }\r\n\
int a17(int a, int ref b){ return a ^ *b; }\r\n\
\r\n\
try(<> a01(2, nullptr));\r\n\
try(<> a02(2, nullptr));\r\n\
try(<> a03(2, nullptr));\r\n\
try(<> a04(2, nullptr));\r\n\
try(<> a05(2, nullptr));\r\n\
try(<> a06(2, nullptr));\r\n\
try(<> a07(2, nullptr));\r\n\
try(<> a08(2, nullptr));\r\n\
try(<> a09(2, nullptr));\r\n\
try(<> a10(2, nullptr));\r\n\
try(<> a11(2, nullptr));\r\n\
try(<> a12(2, nullptr));\r\n\
try(<> a13(2, nullptr));\r\n\
try(<> a14(2, nullptr));\r\n\
try(<> a15(2, nullptr));\r\n\
try(<> a16(2, nullptr));\r\n\
try(<> a17(2, nullptr));\r\n\
\r\n\
return a01(2, nullptr);";
TEST_HARD_RUNTIME_FAIL("Invalid pointer check coverage (int) [failure handling]", testInvalidPointerIntOps, "ERROR: null pointer access");

const char	*testInvalidPointerLongOps =
"import std.error;\r\n\
\r\n\
int a01(long a, long ref b){ return a + *b; }\r\n\
int a02(long a, long ref b){ return a - *b; }\r\n\
int a03(long a, long ref b){ return a * *b; }\r\n\
int a04(long a, long ref b){ return a / *b; }\r\n\
int a05(long a, long ref b){ return a % *b; }\r\n\
int a06(long a, long ref b){ return a ** *b; }\r\n\
int a07(long a, long ref b){ return a < *b; }\r\n\
int a08(long a, long ref b){ return a > *b; }\r\n\
int a09(long a, long ref b){ return a <= *b; }\r\n\
int a10(long a, long ref b){ return a >= *b; }\r\n\
int a11(long a, long ref b){ return a == *b; }\r\n\
int a12(long a, long ref b){ return a != *b; }\r\n\
int a13(long a, long ref b){ return a << *b; }\r\n\
int a14(long a, long ref b){ return a >> *b; }\r\n\
int a15(long a, long ref b){ return a & *b; }\r\n\
int a16(long a, long ref b){ return a | *b; }\r\n\
int a17(long a, long ref b){ return a ^ *b; }\r\n\
\r\n\
try(<> a01(2, nullptr));\r\n\
try(<> a02(2, nullptr));\r\n\
try(<> a03(2, nullptr));\r\n\
try(<> a04(2, nullptr));\r\n\
try(<> a05(2, nullptr));\r\n\
try(<> a06(2, nullptr));\r\n\
try(<> a07(2, nullptr));\r\n\
try(<> a08(2, nullptr));\r\n\
try(<> a09(2, nullptr));\r\n\
try(<> a10(2, nullptr));\r\n\
try(<> a11(2, nullptr));\r\n\
try(<> a12(2, nullptr));\r\n\
try(<> a13(2, nullptr));\r\n\
try(<> a14(2, nullptr));\r\n\
try(<> a15(2, nullptr));\r\n\
try(<> a16(2, nullptr));\r\n\
try(<> a17(2, nullptr));\r\n\
\r\n\
return a01(2, nullptr);";
TEST_HARD_RUNTIME_FAIL("Invalid pointer check coverage (long) [failure handling]", testInvalidPointerLongOps, "ERROR: null pointer access");

const char	*testInvalidPointerDoubleOps =
"import std.error;\r\n\
\r\n\
int a01(double a, double ref b){ return a + *b; }\r\n\
int a02(double a, double ref b){ return a - *b; }\r\n\
int a03(double a, double ref b){ return a * *b; }\r\n\
int a04(double a, double ref b){ return a / *b; }\r\n\
int a05(double a, double ref b){ return a % *b; }\r\n\
int a06(double a, double ref b){ return a ** *b; }\r\n\
int a07(double a, double ref b){ return a < *b; }\r\n\
int a08(double a, double ref b){ return a > *b; }\r\n\
int a09(double a, double ref b){ return a <= *b; }\r\n\
int a10(double a, double ref b){ return a >= *b; }\r\n\
int a11(double a, double ref b){ return a == *b; }\r\n\
int a12(double a, double ref b){ return a != *b; }\r\n\
\r\n\
try(<> a01(2, nullptr));\r\n\
try(<> a02(2, nullptr));\r\n\
try(<> a03(2, nullptr));\r\n\
try(<> a04(2, nullptr));\r\n\
try(<> a05(2, nullptr));\r\n\
try(<> a06(2, nullptr));\r\n\
try(<> a07(2, nullptr));\r\n\
try(<> a08(2, nullptr));\r\n\
try(<> a09(2, nullptr));\r\n\
try(<> a10(2, nullptr));\r\n\
try(<> a11(2, nullptr));\r\n\
try(<> a12(2, nullptr));\r\n\
\r\n\
return a01(2, nullptr);";
TEST_HARD_RUNTIME_FAIL("Invalid pointer check coverage (double) [failure handling]", testInvalidPointerDoubleOps, "ERROR: null pointer access");

const char	*testInvalidPointerFloatOps =
"import std.error;\r\n\
\r\n\
int a01(float a, float ref b){ return a + *b; }\r\n\
int a02(float a, float ref b){ return a - *b; }\r\n\
int a03(float a, float ref b){ return a * *b; }\r\n\
int a04(float a, float ref b){ return a / *b; }\r\n\
\r\n\
try(<> a01(2, nullptr));\r\n\
try(<> a02(2, nullptr));\r\n\
try(<> a03(2, nullptr));\r\n\
try(<> a04(2, nullptr));\r\n\
\r\n\
return a01(2, nullptr);";
TEST_HARD_RUNTIME_FAIL("Invalid pointer check coverage (float) [failure handling]", testInvalidPointerFloatOps, "ERROR: null pointer access");

const char	*testArrayAllocationFail = 
"int[] f = new int[1024*1024*1024];\r\n\
f[5] = 0;\r\n\
return 0;";
TEST_RUNTIME_FAIL("Array allocation failure [failure handling]", testArrayAllocationFail, "ERROR: can't allocate array with 1073741824 elements of size 4");

const char	*testBaseToDerivedFail1 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec2 ref x = new vec2;\r\n\
vec3 ref y = x;\r\n\
return 0;";
TEST_RUNTIME_FAIL("Base to derived type pointer conversion failure [failure handling]", testBaseToDerivedFail1, "ERROR: cannot convert from 'vec2' to 'vec3'");

const char	*testBaseToDerivedFail2 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
vec2 x;\r\n\
int bar(vec3 ref x){ return x.z; }\r\n\
return bar(&x);";
TEST_RUNTIME_FAIL("Base to derived type pointer conversion failure 2 [failure handling]", testBaseToDerivedFail2, "ERROR: cannot convert from 'vec2' to 'vec3'");

const char	*testAssertionFail1 =
"assert(0, \"%s%s%s%s%s%s%s\");";
TEST_RUNTIME_FAIL("Assertion fail correctly handles formatting string [failure handling]", testAssertionFail1, "%s%s%s%s%s%s%s");

const char	*testAssertionFail2 =
"char[4] a = 'a'; char[1024] b = 'b'; assert(0, a);";
TEST_RUNTIME_FAIL("Assertion fail correctly handles string length [failure handling]", testAssertionFail2, "aaaa");

void RecallerTransition(int x)
{
	(void)nullcRunFunction("inside", x);
}

LOAD_MODULE_BIND(func_testX, "func.testX", "void recall(int x);")
{
	nullcBindModuleFunctionHelper("func.testX", RecallerTransition, "recall", 0);
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
#ifdef NULLC_STACK_TRACE_WITH_LOCALS
const char *error = "Assertion failed\r\n\
Call stack:\r\n\
global scope (line 7: at recall(2);)\r\n\
inside (line 5: at recall(x-1);)\r\n\
 param 0: int x (at base+0 size 4)\r\n\
inside (line 5: at recall(x-1);)\r\n\
 param 0: int x (at base+0 size 4)\r\n\
inside (line 4: at assert(x);)\r\n\
 param 0: int x (at base+0 size 4)\r\n";
#else
const char *error = "Assertion failed\r\n\
Call stack:\r\n\
global scope (line 7: at recall(2);)\r\n\
inside (line 5: at recall(x-1);)\r\n\
inside (line 5: at recall(x-1);)\r\n\
inside (line 4: at assert(x);)\r\n";
#endif
struct Test_testMultipleTransitions : TestQueue
{
	virtual void Run()
	{
		if(Tests::messageVerbose)
			printf("Call stack when there are various transitions between NULLC and C\r\n");
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testFailureExecutor[t])
				continue;
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

					const char *targetName = testTarget[t] == NULLC_X86 ? "X86" : (testTarget[t] == NULLC_LLVM ? "LLVM" : "REGVM");

					printf("%s failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", targetName, nullcGetLastError(), error);
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
Test_testMultipleTransitions test_testMultipleTransitions;

#if defined(NULLC_BUILD_X86_JIT)

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
		nullcClean();
		nullcSetExecutorStackSize(16 * 1024);

		if(Tests::messageVerbose)
			printf("Call depth test\r\n");

		if(Tests::testHardFailureExecutor[TEST_TYPE_X86])
		{
			testsCount[TEST_TYPE_X86]++;
			nullcSetExecutor(NULLC_X86);
			nullres good = nullcBuild(testDepthOverflow);
			assert(good);
			good = nullcRun();
			if(!good)
			{
				const char *expected = "ERROR: stack overflow";
				char buf[512];

				if(const char *pos = strstr(nullcGetLastError(), "ERROR:"))
					strncpy(buf, pos, 511);
				else
					strncpy(buf, nullcGetLastError(), 511);

				buf[511] = 0;

				if(char *lineEnd = strchr(buf, '\r'))
					*lineEnd = 0;
				if(strcmp(expected, buf) != 0)
				{
					if(!Tests::messageVerbose)
						printf("Call depth test\r\n");
					printf("X86 failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", buf, expected);
				}else{
					testsPassed[TEST_TYPE_X86]++;
				}
			}else{
				if(!Tests::messageVerbose)
					printf("Call depth test\r\n");
				printf("Test should have failed.\r\n");
			}
		}

		nullcClean();
		nullcSetExecutorStackSize(Tests::testStackSize);
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
double[5400] res;\r\n\
return clamp(abs(-1.5), 0.0, 1.0);";
struct Test_testGlobalOverflow : TestQueue
{
	virtual void Run()
	{
		nullcClean();
		nullcSetExecutorStackSize(32 * 1024);

		if(Tests::messageVerbose)
			printf("Global overflow test\r\n");

		if(Tests::testFailureExecutor[TEST_TYPE_X86])
		{
			testsCount[TEST_TYPE_X86]++;
			nullcSetExecutor(NULLC_X86);
			nullres good = nullcBuild(testGlobalOverflow);
			assert(good);
			good = nullcRun();
			if(!good)
			{
				const char *expected = "ERROR: allocated stack overflow";
				char buf[512];

				if(const char *pos = strstr(nullcGetLastError(), "ERROR:"))
					strncpy(buf, pos, 511);
				else
					strncpy(buf, nullcGetLastError(), 511);

				buf[511] = 0;

				if(char *lineEnd = strchr(buf, '\r'))
					*lineEnd = 0;
				if(strcmp(expected, buf) != 0)
				{
					if(!Tests::messageVerbose)
						printf("Global overflow test\r\n");
					printf("X86 failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", buf, expected);
				}else{
					testsPassed[TEST_TYPE_X86]++;
				}
			}else{
				if(!Tests::messageVerbose)
					printf("Global overflow test\r\n");
				printf("Test should have failed.\r\n");
			}
		}

		nullcClean();
		nullcSetExecutorStackSize(Tests::testStackSize);
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
		nullcClean();
		nullcSetExecutorStackSize(1024 * 1024);

		if(Tests::messageVerbose)
			printf("Depth overflow in unmanaged memory\r\n");
		if(Tests::testHardFailureExecutor[TEST_TYPE_X86])
		{
			testsCount[TEST_TYPE_X86]++;
			nullcSetExecutor(NULLC_X86);
			nullres good = nullcBuild(testDepthOverflowUnmanaged);
			assert(good);
			good = nullcRun();
			if(!good)
			{
				const char *expected = "ERROR: stack overflow";
				char buf[512];

				if(const char *pos = strstr(nullcGetLastError(), "ERROR:"))
					strncpy(buf, pos, 511);
				else
					strncpy(buf, nullcGetLastError(), 511);

				buf[511] = 0;

				if(char *lineEnd = strchr(buf, '\r'))
					*lineEnd = 0;
				if(strcmp(expected, buf) != 0)
				{
					if(Tests::messageVerbose)
						printf("Depth overflow in unmanaged memory\r\n");
					printf("X86 failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", buf, expected);
				}else{
					testsPassed[TEST_TYPE_X86]++;
				}
			}else{
				if(Tests::messageVerbose)
					printf("Depth overflow in unmanaged memory\r\n");
				printf("Test should have failed.\r\n");
			}
		}

		nullcClean();
		nullcSetExecutorStackSize(Tests::testStackSize);
	}
};
Test_testDepthOverflowUnmanaged test_testDepthOverflowUnmanaged;

#endif
