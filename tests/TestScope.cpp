#include "TestBase.h"

const char	*testVisibility = 
"// Function visibility test\r\n\
int u = 5, t1 = 2, t2 = 2;\r\n\
for({int i = 0;}; i < 4; {i++;})\r\n\
{\r\n\
  int b = 2;\r\n\
  {\r\n\
    int test(int x){ return x*b+u; }\r\n\
    t1 = test(2);\r\n\
  }\r\n\
  {\r\n\
    int test(int x){ return x*u+b; }\r\n\
    t2 = test(2);\r\n\
  }\r\n\
}\r\n\
\r\n\
return 4;";
TEST("Function visibility test", testVisibility, "4")
{
	CHECK_INT("u", 0, 5, lastFailed);
	CHECK_INT("t1", 0, 9, lastFailed);
	CHECK_INT("t2", 0, 12, lastFailed);
}

const char	*testClassMemberHide =
"class Test{\r\n\
int x, y;\r\n\
int sum(int x){ return x + y; }\r\n\
}\r\n\
Test a;\r\n\
a.x = 14;\r\n\
a.y = 15;\r\n\
return a.sum(5);";
TEST_RESULT("Member function hides members", testClassMemberHide, "20");

const char	*testVariableHiding1 =
"int a = 4;\r\n\
{\r\n\
	auto a(){ }\r\n\
	a();\r\n\
}\r\n\
return a;";
TEST_RESULT("Variable hiding by function 1", testVariableHiding1, "4");

const char	*testVariableHiding2 =
"int a = 4;\r\n\
{\r\n\
	auto a(){ }\r\n\
	auto b = a;\r\n\
	b();\r\n\
}\r\n\
return a;";
TEST_RESULT("Variable hiding by function 2", testVariableHiding2, "4");

const char	*testFunctionVisibility =
"int func()\r\n\
{\r\n\
	int test(int x)\r\n\
	{\r\n\
		return -x;\r\n\
	}\r\n\
	return test(5);\r\n\
}\r\n\
\r\n\
int test = func();\r\n\
\r\n\
return test;";
TEST_RESULT("Function visibility test", testFunctionVisibility, "-5");

const char	*testTypedefScopeFunction =
"int func1(){ typedef int[2] data; data x; x[0] = 5; x[1] = 4; return x[0] * x[1]; }\r\n\
int func2(){ typedef float[2] data; data x; x[0] = 3.6; x[1] = 0.5; return x[0] / x[1]; }\r\n\
typedef int data;\r\n\
data res = func1() + func2();\r\n\
return res;";
TEST_RESULT("typedef scoping in functions.", testTypedefScopeFunction, "27");

const char	*testTypedefScopeType =
"class TypeA\r\n\
{\r\n\
	typedef int[2] data;\r\n\
	data x;\r\n\
	int func(){ return x[0] * x[1]; }\r\n\
}\r\n\
TypeA a;\r\n\
a.x[0] = 5; a.x[1] = 4;\r\n\
class TypeB\r\n\
{\r\n\
	//typedef float[2] data;\r\n\
	/*data*/float[2] x;\r\n\
	int func(){ return x[0] / x[1]; }\r\n\
}\r\n\
TypeB b;\r\n\
b.x[0] = 3.6; b.x[1] = 0.5;\r\n\
typedef int data;\r\n\
data res = a.func() + b.func();\r\n\
return res;";
TEST_RESULT("typedef scoping in types.", testTypedefScopeType, "27");

const char	*testTypedefScopeTypeReturn =
"class TypeA\r\n\
{\r\n\
	typedef int[2] data;\r\n\
	data x;\r\n\
}\r\n\
int TypeA::func(){ data y = x; return y[0] * y[1]; }\r\n\
TypeA a;\r\n\
a.x[0] = 5; a.x[1] = 4;\r\n\
class TypeB\r\n\
{\r\n\
	typedef float[2] data;\r\n\
	data x;\r\n\
}\r\n\
int TypeB::func(){ data y = x; return y[0] / y[1]; }\r\n\
TypeB b;\r\n\
b.x[0] = 3.6; b.x[1] = 0.5;\r\n\
typedef int data;\r\n\
data res = a.func() + b.func();\r\n\
return res;";
TEST_RESULT("typedef recovery.", testTypedefScopeTypeReturn, "27");
