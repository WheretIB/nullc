#include "TestBase.h"

const char	*testLocalClass1 =
"int foo(int x)\r\n\
{\r\n\
	class X{ int i; int bar(){ return -i; } }\r\n\
	X a;\r\n\
	a.i = x;\r\n\
	return a.bar();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Class defined locally test 1", testLocalClass1, "-5");

const char	*testLocalClass2 =
"{\r\n\
	class X{ int i; int bar(){ return -i; } }\r\n\
	X a;\r\n\
	a.i = 5;\r\n\
	return a.bar();\r\n\
}";
TEST_RESULT("Class defined locally test 2", testLocalClass2, "-5");

const char	*testLocalClass3 =
"int foo(int x)\r\n\
{\r\n\
	class X{ int i; int bar(){ return -i; } }\r\n\
	int X::foo(){ return this.i * 2; }\r\n\
	X a;\r\n\
	a.i = x;\r\n\
	return a.foo();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Class defined locally test 3", testLocalClass3, "10");

const char	*testLocalClass4 =
"class X{ int i; int bar(){ return -i; } }\r\n\
int foo(int x)\r\n\
{\r\n\
	int X::foo(){ return i * 2; }\r\n\
	X a;\r\n\
	a.i = x;\r\n\
	return a.foo();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Class defined locally test 4", testLocalClass4, "10");

const char	*testLocalClass5 =
"int foo(int x)\r\n\
{\r\n\
	class X{ int i; int bar(){ return -i; } }\r\n\
	int X::foo(){ return i * 2; }\r\n\
	X a;\r\n\
	a.i = x;\r\n\
	return a.bar();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Class defined locally test 5", testLocalClass5, "-5");

const char	*testLocalClass6 =
"int foo(int x)\r\n\
{\r\n\
	class X{ int i; int bar(){ return -i; } }\r\n\
	int X::foo(){ return i * 2; }\r\n\
	X a;\r\n\
	a.i = x;\r\n\
	return a.foo();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Class defined locally test 6", testLocalClass6, "10");
