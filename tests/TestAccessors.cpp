#include "TestBase.h"

const char	*testAccessors =
"class Test{\r\n\
int x, y;\r\n\
int sum{ get{ return x + y; } };\r\n\
int[2] xy{ get{ return {x, y}; } set{ x = r[0]; y = r[1]; } };\r\n\
double doubleX{ get{ return x; } set(value){ x = value; return y; } };\r\n\
}\r\n\
Test a;\r\n\
a.x = 14;\r\n\
a.y = 15;\r\n\
int c = a.sum;\r\n\
a.xy = { 5, 1 };\r\n\
double b;\r\n\
b = a.doubleX = 5.0;\r\n\
return a.sum;";
TEST("Accessors", testAccessors, "6")
{
	CHECK_INT("c", 0, 29);
	CHECK_DOUBLE("b", 0, 1.0);
}

const char	*testPropertyAfterFunctionCall =
"import std.math;\r\n\
import std.typeinfo;\r\n\
float4 u = float4(1, 2, 3, 4);\r\n\
if(sizeof(u) == 16) u.x = 5; else u.x = 6;\r\n\
return typeid(u.xyz).size;";
TEST_RESULT("Property access after function call", testPropertyAfterFunctionCall, "12");

