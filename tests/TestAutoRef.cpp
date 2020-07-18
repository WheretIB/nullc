#include "TestBase.h"

const char	*testAutoReference1 =
"int a = 17;\r\n\
double b = 14.0;\r\n\
auto ref c = &a, d = &a;\r\n\
auto m = &a;\r\n\
auto n = &b;\r\n\
c = &b;\r\n\
int ref l = d;\r\n\
int p1 = l == &a;\r\n\
return *l;";
TEST("Auto reference type", testAutoReference1, "17")
{
	CHECK_INT("a", 0, 17, lastFailed);
	CHECK_DOUBLE("b", 0, 14.0, lastFailed);

	CHECK_INT("p1", 0, 1, lastFailed);
}

const char	*testAutoReference2 =
"typedef auto ref object;\r\n\
\r\n\
void func(object p){ int ref a = p; *a = 9; }\r\n\
int k = 4;\r\n\
\r\n\
func(&k);\r\n\
\r\n\
return k;";
TEST_RESULT("Auto reference type 2", testAutoReference2, "9");

const char	*testBoxUnbox1 =
"auto ref foo(){ return 12; }\r\n\
int a = foo();\r\n\
return int(foo()) * a;";
TEST_RESULT("Auto reference type boxing and unboxing", testBoxUnbox1, "144");

const char	*testBoxUnbox2 =
"auto ref x = 4;\r\n\
int a = x;\r\n\
return int(x) * a;";
TEST_RESULT("Auto reference type boxing and unboxing 2", testBoxUnbox2, "16");

const char	*testAutoRefAssign =
"auto ref x = 4;\r\n\
*x = 3;\r\n\
int y = x;\r\n\
return y;";
TEST_RESULT("Auto reference immediate assignment", testAutoRefAssign, "3");

const char	*testAutoRefNoArrayAssign1 = 
"char[6] a = { 1, 2, 3, 4, 5, 6 };\r\n\
auto ref b = a;\r\n\
char[6] c = b;\r\n\
return c[2];";
TEST_RESULT("Auto reference assignment to an array 1", testAutoRefNoArrayAssign1, "3");

const char	*testAutoRefNoArrayAssign2 =
"class Test\r\n\
{\r\n\
	char[6] a = { 1, 2, 3, 4, 5, 6 };\r\n\
	auto ref b = a;\r\n\
	char[6] c = b;\r\n\
}\r\n\
Test t;\r\n\
return t.c[2];";
TEST_RESULT("Auto reference assignment to an array 2", testAutoRefNoArrayAssign2, "3");
