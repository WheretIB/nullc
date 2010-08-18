#include "TestBase.h"

const char	*testLeftValueExtends =
"int v = 0;\r\n\
int v2 = 12, v4 = 15;\r\n\
auto k = { 3, 4, 9 };\r\n\
int ref f(){ return &v; }\r\n\
auto g(){ return &k; }\r\n\
f() = 5;\r\n\
g()[0] = 12;\r\n\
g()[1] = 18;\r\n\
(v2 > 5 ? &v2 : &v4) = 5;\r\n\
return 1;";
TEST("L-value extended cases", testLeftValueExtends, "1")
{
	CHECK_INT("v", 0, 5);
	CHECK_INT("k", 0, 12);
	CHECK_INT("k", 1, 18);
	CHECK_INT("v2", 0, 5);
	CHECK_INT("v4", 0, 15);
}

