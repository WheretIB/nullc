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
	CHECK_INT("a", 0, 17);
	CHECK_DOUBLE("b", 0, 14.0);

	CHECK_INT("p1", 0, 1);
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

