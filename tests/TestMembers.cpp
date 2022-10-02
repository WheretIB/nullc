#include "TestBase.h"

const char	*testClassMethod = 
"// Class method test\r\n\
import std.math;\r\n\
class vec3\r\n\
{\r\n\
float x, y, z;\r\n\
void vec3(float xx, float yy, float zz){ x = xx; y = yy; z = zz; }\r\n\
\r\n\
float length(){ return sqrt(x*x+y*y+z*z); }\r\n\
float dot(vec3 ref v){ return x*v.x+y*v.y+z*v.z; }\r\n\
\r\n\
float w;\r\n\
}\r\n\
\r\n\
return 1;";
TEST_RESULT("Class method test", testClassMethod, "1");

const char	*testClassExternalMethodInt =
"auto int::toString()\r\n\
{\r\n\
	int copy = *this, len = 0;\r\n\
	while(copy)\r\n\
	{\r\n\
		len++;\r\n\
		copy /= 10;\r\n\
	}\r\n\
	char[] val = new char[len+1];\r\n\
	copy = *this;\r\n\
	while(copy)\r\n\
	{\r\n\
		val[--len] = copy % 10 + '0';\r\n\
		copy /= 10;\r\n\
	}\r\n\
	return val;\r\n\
}\r\n\
int n = 19;\r\n\
auto nv = n.toString();\r\n\
return nv[0] + nv[1];";
TEST_RESULT("Class externally defined method (int)", testClassExternalMethodInt, "106");

const char	*testClassMethodString =
"typedef char[] string;\r\n\
string string::reverse()\r\n\
{\r\n\
	for(int i = 0; i < (this.size-1) / 2; i++)\r\n\
	{\r\n\
		char tmp = this[i];\r\n\
		this[i] = this[this.size-i-2];\r\n\
		this[this.size-i-2] = tmp;\r\n\
	}\r\n\
	return *this;\r\n\
}\r\n\
string a = \"hello\";\r\n\
string b = a.reverse();\r\n\
return b[0] - 'o';";
TEST_RESULT("Class externally defined method (char[])", testClassMethodString, "0");

const char	*testClassExternalMethod =
"class Foo\r\n\
{\r\n\
	int bar;\r\n\
}\r\n\
\r\n\
int Foo::GetBar(){ return bar; }\r\n\
Foo a;\r\n\
a.bar = 14;\r\n\
\r\n\
return a.GetBar();";
TEST_RESULT("Class externally defined method (custom)", testClassExternalMethod, "14");

const char	*testInternalMemberFunctionCall =
"class Test\r\n\
{\r\n\
	int i;\r\n\
	int foo(){ return i; }\r\n\
	auto bar(){ return foo(); }\r\n\
}\r\n\
Test a;\r\n\
a.i = 5;\r\n\
auto k = a.bar();\r\n\
return k;";
TEST_RESULT("Internal member function call", testInternalMemberFunctionCall, "5");

const char	*testFunctionWithArgumentsMember =
"class Test{\r\n\
int x, y;\r\n\
int sum(int x){ return x + y; }\r\n\
int sum(int x, ny){ y = ny; return sum(x); }\r\n\
}\r\n\
Test a;\r\n\
a.x = 14;\r\n\
a.y = 15;\r\n\
return a.sum(5, 12);";
TEST_RESULT("Member function with arguments call from member function", testFunctionWithArgumentsMember, "17");

const char	*testMemberFunctionPointerCall1 =
"class Test\r\n\
{\r\n\
	int ref(int) func;\r\n\
}\r\n\
Test a;\r\n\
a.func = auto(int x){ return -x; };\r\n\
\r\n\
return a.func(5);";
TEST_RESULT("Member function pointer call 1", testMemberFunctionPointerCall1, "-5");

const char	*testMemberFunctionPointerCall2 =
"class Test\r\n\
{\r\n\
	int ref(int) func;\r\n\
}\r\n\
auto a = new Test;\r\n\
a.func = auto(int x){ return -x; };\r\n\
\r\n\
return a.func(5);";
TEST_RESULT("Member function pointer call 2", testMemberFunctionPointerCall2, "-5");
