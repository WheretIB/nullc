#include "TestBase.h"

const char	*testFunctionTypeNew =
"import std.range;\r\n\
\r\n\
auto a = { 1, 2, 3, 4, 5 };\r\n\
auto b = new int ref()[a.size];\r\n\
for(i in a, n in range(0, 4))\r\n\
	b[n] = auto(){ return a[n]; };\r\n\
\r\n\
return b[0]() + b[1]() + b[4]();";
TEST_RESULT("new call with function type", testFunctionTypeNew, "15");

const char	*testTypeOfNew =
"import std.range;\r\n\
int test(){ return 0; }\r\n\
auto a = { 1, 2, 3, 4, 5 };\r\n\
auto b = new typeof(test)[a.size];\r\n\
for(i in a, n in range(0, 4))\r\n\
	b[n] = auto(){ return a[n]; };\r\n\
\r\n\
return b[0]() + b[1]() + b[4]();";
TEST_RESULT("new call with typeof", testTypeOfNew, "15");

const char	*testConstructorAfterNew =
"void int:int(int x, y){ *this = x; }\r\n\
int ref a = new int(5, 0);\r\n\
int foo(int ref x){ return *x; }\r\n\
return *a + foo(new int(30, 0));";
TEST_RESULT("Type constructor after new.", testConstructorAfterNew, "35");

const char	*testConstructorAfterNew2 = "class Test{ int x; } void Test:Test(int x){ this.x = x; } return (new Test(4)).x;";
TEST_RESULT("Type constructor after new 2.", testConstructorAfterNew2, "4");

const char	*testConstructorAfterNew3 = "class Test{ int x; } void Test:Test(){ } return (new Test()).x;";
TEST_RESULT("Type constructor after new 3.", testConstructorAfterNew3, "0");
