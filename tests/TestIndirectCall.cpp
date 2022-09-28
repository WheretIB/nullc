#include "TestBase.h"

const char	*testIndirectPointers =
"int ref(int)[2] farr;\r\n\
\r\n\
farr[0] = auto(int c){ return -c; };\r\n\
farr[1] = auto(int d){ return 2*d; };\r\n\
\r\n\
int ref(int) func(int i){ return farr[i]; }\r\n\
typeof(func) farr2 = func;\r\n\
auto getarr(){ return { farr[1], farr[0] }; }\r\n\
\r\n\
int a1 = farr[0](12);\r\n\
int a2 = farr[1](12);\r\n\
int b1 = func(0)(12);\r\n\
int b2 = func(1)(12);\r\n\
int c1 = getarr()[0](12);\r\n\
int c2 = getarr()[1](12);\r\n\
int d1 = farr2(0)(12);\r\n\
int d2 = farr2(1)(12);\r\n\
\r\n\
class Del\r\n\
{\r\n\
  int classLocal;\r\n\
\r\n\
  int test(int a){ return classLocal + a; }\r\n\
}\r\n\
Del cls;\r\n\
cls.classLocal = 300;\r\n\
farr[0] = cls.test;\r\n\
\r\n\
return (farr[0])(12);";
TEST("Indirect function pointers", testIndirectPointers, "312")
{
	CHECK_INT("a1", 0, -12, lastFailed);
	CHECK_INT("a2", 0, 24, lastFailed);
	CHECK_INT("b1", 0, -12, lastFailed);
	CHECK_INT("b2", 0, 24, lastFailed);
	CHECK_INT("c1", 0, 24, lastFailed);
	CHECK_INT("c2", 0, -12, lastFailed);
	CHECK_INT("d1", 0, -12, lastFailed);
	CHECK_INT("d2", 0, 24, lastFailed);
}

const char	*testMemberFuncCallPostExpr =
"import std.math;\r\n\
class foo\r\n\
{\r\n\
	float2 v;\r\n\
	int[3] arr;\r\n\
	auto init(){ v.x = 4; v.y = 9; arr = { 12, 14, 17 }; }\r\n\
	auto vec(){ return v; }\r\n\
	auto array(){ return arr; }\r\n\
	auto func(){ return auto(){ return *this; }; }\r\n\
	auto func2(){ return auto(){ return this; }; }\r\n\
}\r\n\
foo f;\r\n\
f.init();\r\n\
float f1 = f.vec().x;\r\n\
float f2 = f.func()().v.y;\r\n\
int i1 = f.array()[0];\r\n\
int i2 = f.func()().arr[1];\r\n\
int i3 = f.func()().func()().arr[2];\r\n\
\r\n\
float f3 = f.func2()().v.y;\r\n\
int i4 = f.func2()().arr[0];\r\n\
int i5 = f.func2()().func2()().arr[1];\r\n\
int i6 = f.func()().func2()().arr[2];\r\n\
return 1;";
TEST("Member function call post expressions", testMemberFuncCallPostExpr, "1")
{
	CHECK_FLOAT("f1", 0, 4, lastFailed);
	CHECK_FLOAT("f2", 0, 9, lastFailed);
	CHECK_FLOAT("f3", 0, 9, lastFailed);
	CHECK_INT("i1", 0, 12, lastFailed);
	CHECK_INT("i2", 0, 14, lastFailed);
	CHECK_INT("i3", 0, 17, lastFailed);
	CHECK_INT("i4", 0, 12, lastFailed);
	CHECK_INT("i5", 0, 14, lastFailed);
	CHECK_INT("i6", 0, 17, lastFailed);
}

const char	*testClassFuncReturn =
"class Test\r\n\
{\r\n\
	int i;\r\n\
	int foo(){ return i; }\r\n\
	auto bar(){ return foo; }\r\n\
}\r\n\
Test a;\r\n\
a.i = 5;\r\n\
auto k = a.bar()();\r\n\
return 1;";
TEST("Class function return", testClassFuncReturn, "1")
{
	CHECK_INT("k", 0, 5, lastFailed);
}

const char	*testMemberFuncCallRef =
"typedef char[] string;\r\n\
\r\n\
void string::set(int a)\r\n\
{\r\n\
	this[0] = a;\r\n\
}\r\n\
\r\n\
string a = \"hello\";\r\n\
auto b = &a;\r\n\
b.set('m');\r\n\
return b[0];";
TEST_RESULT("Member function call of a reference to a class", testMemberFuncCallRef, "109");

const char	*testIndirectCallInMemberFunction2 =
"int foo(int x){ return -x; }\r\n\
auto bar(){ return foo; }\r\n\
class Foo { int fail(){ return bar()(5); } }\r\n\
Foo x;\r\n\
return x.fail();";
TEST_RESULT("Indirect function call in member function 2", testIndirectCallInMemberFunction2, "-5");

const char	*testFunctionTypeMemberSelfcall =
"int foo(int x){ return -x; }\r\n\
typedef int ref(int) omg;\r\n\
int omg::call(int x){ return (*this)(x); }\r\n\
auto ref x = duplicate(foo);\r\n\
return x.call(5);";
TEST_RESULT("Function type member function that calls itself", testFunctionTypeMemberSelfcall, "-5");
