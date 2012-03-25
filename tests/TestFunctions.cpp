#include "TestBase.h"

const char	*testFuncCall1 = 
"int test(int x, int y, int z){return x*y+z;}\r\n\
return 1+test(2, 3, 4);	// 11";
TEST_RESULT("Function call test 1", testFuncCall1, "11");

const char	*testFuncCall2 = 
"int test(int x, int y, int z){return x*y+z;}\r\n\
int b = 1;\r\n\
if(7>5)\r\n\
b = 3;\r\n\
return b+test(2, 3, 4);";
TEST_RESULT("Function call test 2", testFuncCall2, "13");

const char	*testFuncCall3 = 
"int fib(int n){ if(n<3) return 5; return 10; }\r\n\
return fib(1);";
TEST_RESULT("Function call test 3", testFuncCall3, "5");

const char	*testRecursion = 
"int fib(int n){ if(n<3) return 1; return fib(n-2)+fib(n-1); }\r\n\
return fib(4);";
TEST_RESULT("Recursion test", testRecursion, "3");

const char	*testBuildinFunc = 
"// Build-In function checks\r\n\
import std.math;\r\n\
double Pi = 3.1415926583;\r\n\
double[27] res;\r\n\
res[0] = cos(0); // 1.0\r\n\
res[1] = cos(Pi/3.0); // 0.5\r\n\
res[2] = cos(Pi); // -1.0\r\n\
\r\n\
res[3] = sin(0); // 0.0\r\n\
res[4] = sin(Pi/6.0); // 0.5\r\n\
res[5] = sin(Pi); // 0.0\r\n\
\r\n\
res[6] = ceil(1.5); // 2.0\r\n\
res[7] = floor(1.5); // 1.0\r\n\
res[8] = ceil(-1.5); // -1.0\r\n\
res[9] = floor(-1.5); // -2.0\r\n\
\r\n\
res[10] = tan(0); // 0.0\r\n\
res[11] = tan(Pi/4.0); // 1.0\r\n\
res[12] = tan(Pi/2.0); // +inf\r\n\
\r\n\
res[13] = ctg(0); // +inf\r\n\
res[14] = ctg(Pi/4.0); // 1.0\r\n\
res[15] = ctg(Pi/2.0); // 0.0\r\n\
\r\n\
res[16] = sqrt(1.0); // 1.0\r\n\
res[17] = sqrt(0.0); // 0.0\r\n\
res[18] = sqrt(9.0); // 3.0\r\n\
\r\n\
res[19] = cosh(1.0); // \r\n\
res[20] = sinh(1.0); // \r\n\
res[21] = tanh(1.0); // \r\n\
\r\n\
res[22] = acos(0.5); // Pi/3.0\r\n\
res[23] = asin(0.5); // Pi/6.0\r\n\
res[24] = atan(1.0); // Pi/4.0\r\n\
\r\n\
res[25] = exp(1.0); // E\r\n\
res[26] = log(2.7182818284590452353602874713527); // 1.0\r\n\
\r\n\
return (\"hello\" == \"hello\") + (\"world\" != \"World\") + (\"world\" != \"worl\");";
TEST("Build-In function checks", testBuildinFunc, "3")
{
	double resExp[] = { 1.0, 0.5, -1.0, 0.0, 0.5, 0.0, 2.0, 1.0, -1.0, -2.0,
						0.0, 1.0, 0,0, 1.0, 0.0, 1.0, 0.0, 3.0, 
						1.5430806348152437784779056207571, 1.1752011936438014568823818505956, 0.76159415595576488811945828260479,
						1.0471975511965977461542144610932, 0.52359877559829887307710723054658, 0.78539816339744830961566084581988,
						2.7182818284590452353602874713527, 1.0 };
	for(int i = 0; i < 27; i++)
		if(i != 12 && i != 13)
			CHECK_DOUBLE("res", i, resExp[i]);
}

const char	*testFuncCall4 = 
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
return clamp(abs(-1.5), 0.0, 1.0);";
TEST_RESULT("Function call test 4", testFuncCall4, "1.000000");

const char	*testFuncCall5 = 
"int test(int x, int y, int z){return x*y+z;}\r\n\
int res;\r\n\
{\r\n\
int x = 2;\r\n\
{\r\n\
res = 1+test(x, 3, 4);\r\n\
}\r\n\
}\r\n\
return res;";
TEST("Function Call test 5", testFuncCall5, "11")
{
	CHECK_INT("res", 0, 11);
}

const char	*testFuncCall6 = 
"double abs(double x)\r\n\
{\r\n\
  {\r\n\
    if(x < 0.0)\r\n\
      return -x;\r\n\
  }\r\n\
  return x;\r\n\
}\r\n\
return abs(-0.5);";
TEST_RESULT("Function call test 6", testFuncCall6, "0.500000");

const char	*testCalls = 
"int calltest = 0;\r\n\
int fib(int n, int ref calls)\r\n\
{\r\n\
	(*calls)++;\r\n\
	calltest++;\r\n\
	if(n < 3)\r\n\
		return 1;\r\n\
	return fib(n-2, calls) + fib(n-1, calls);\r\n\
}\r\n\
int calls = 0;\r\n\
return fib(15, &calls);";
TEST("Call number test", testCalls, "610")
{
	CHECK_INT("calltest", 0, 1219);
	CHECK_INT("calls", 0, 1219);
}

const char	*testNegate = 
"double neg(double a){ return -a; }\r\n\
double x = 5.0, nx;\r\n\
for(int i = 0; i < 1000; i++)\r\n\
nx = neg(x);\r\n\
return nx;";
TEST("Negate test", testNegate, "-5.000000")
{
	CHECK_DOUBLE("x", 0, 5.0);
	CHECK_DOUBLE("nx", 0, -5.0);
}

const char	*testParametersExtraordinaire =
"char func(char a, b, c){ return a+b+c; }\r\n\
auto u = func;\r\n\
int i = u(1, 7, 18);\r\n\
return func(1,7,18);";
TEST("Function parameters with different stack type", testParametersExtraordinaire, "26")
{
	CHECK_INT("i", 0, 26);
}

const char	*testPrototypePointer =
"void foo();\r\n\
auto a = foo;\r\n\
void foo(){}\r\n\
auto b = foo;\r\n\
return a == b;";
TEST_RESULT("Function prototype matches implementation", testPrototypePointer, "1");

const char	*testFunctionHiddenByVariable =
"int foo(int a){ return -a; } int bar(int ref(int) foo){ return foo(-5); } return bar(<i>{ i; });";
TEST_RESULT("Function that is hidden by variable with the same name", testFunctionHiddenByVariable, "-5");

const char	*testFunctionReturnVoid =
"void foo()\r\n\
{\r\n\
	return;\r\n\
}\r\n\
void bar()\r\n\
{\r\n\
	return foo();\r\n\
}\r\n\
bar();\r\n\
return 1;";
TEST_RESULT("Function implicitly returns void", testFunctionReturnVoid, "1");

const char	*testCompileTimeFunctionEvaluationBug3 = "char foo(char a, b){ return a + b; } return foo(1, 3);";
TEST_RESULT("Compile-time function evaluation bug 3", testCompileTimeFunctionEvaluationBug3, "4");

const char	*testCompileTimeFunctionEvaluationBug4 =
"int bar1(int k){ return k; }\r\n\
int bar2(int k){ return k; }\r\n\
\r\n\
int foo(int a)\r\n\
{\r\n\
	int[16] arr;\r\n\
	int sum = 0;\r\n\
	for(int i = 0; i < 16; i++)\r\n\
	{\r\n\
		arr[i] = i * 5 + 3;\r\n\
		sum += arr[i];\r\n\
	}\r\n\
\r\n\
	bar1(a); bar2(a);\r\n\
\r\n\
	int sum2 = 0;\r\n\
	for(int i = 0; i < 16; i++)\r\n\
		sum2 += arr[i];\r\n\
\r\n\
	return sum - sum2 + a;\r\n\
}\r\n\
return foo(10);";
TEST_RESULT("Compile-time function evaluation bug 4", testCompileTimeFunctionEvaluationBug4, "10");