#include "TestBase.h"

const char	*testLocalReturn1 =
"int ref boo(){ int a; return &a; }\r\n\
void clear(int a, b, c){}\r\n\
int ref x = boo();\r\n\
*x = 5;\r\n\
clear(0, 0, 0);\r\n\
return *x;";
TEST_RESULT("Returning pointer to local (obvious)", testLocalReturn1, "5");

const char	*testLocalReturn2 =
"int ref foo(int ref x){ return x; }\r\n\
int ref boo(){ int a; return foo(&a); }\r\n\
void clear(int a, b, c){}\r\n\
int ref x = boo();\r\n\
*x = 5;\r\n\
clear(0, 0, 0);\r\n\
return *x;";
TEST_RESULT("Returning pointer to local (not obvious)", testLocalReturn2, "5");

const char	*testLocalReturn3 = 
"void clear(int a, b, c){}\r\n\
auto foo2(int x){ return &x; }\r\n\
int ref x = foo2(6);\r\n\
*x = 5;\r\n\
clear(0, 0, 0);\r\n\
return *x;";
TEST_RESULT("Returning pointer to local 3", testLocalReturn3, "5");

const char	*testLocalReturn4 = 
"void clear(int[10] a){}\r\n\
auto foo3(){ int[3] bar; int i = 2; return &bar[i]; }\r\n\
int ref x = foo3();\r\n\
*x = 5;\r\n\
clear({0,0,0,0,0,0,0,0,0,0});\r\n\
return *x;";
TEST_RESULT("Returning pointer to local 4", testLocalReturn4, "5");

const char	*testLocalReturn5 = 
"import std.math;\r\n\
void clear(int[16] a){}\r\n\
auto foo4(){ float4[3] bar; int i = 2; return &bar[i].x; }\r\n\
float ref x = foo4();\r\n\
*x = 5;\r\n\
clear({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0});\r\n\
return int(*x);";
TEST_RESULT("Returning pointer to local 5", testLocalReturn5, "5");

const char	*testLocalReturn6 = 
"import std.math;\r\n\
void clear(int[32] a){}\r\n\
class Foo{ int[3] arr; float4[3] arr2;}\r\n\
auto foo5(){ Foo[3] bar; int i = 2; return &bar[i].arr[2]; }\r\n\
int ref x = foo5();\r\n\
*x = 5;\r\n\
clear({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0});\r\n\
return *x;";
TEST_RESULT("Returning pointer to local 6", testLocalReturn6, "5");

const char	*testLocalReturn7 = 
"import std.math;\r\n\
void clear(int[64] a){}\r\n\
class Foo{ int[3] arr; float4[3] arr2;}\r\n\
auto foo6(){ Foo[3] bar; int i = 2; return &bar[i].arr2[2].w; }\r\n\
float ref x = foo6();\r\n\
*x = 5;\r\n\
clear({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0});\r\n\
return int(*x);";
TEST_RESULT("Returning pointer to local 7", testLocalReturn7, "5");

const char	*testLocalArrayReturn1 = 
"char[] foo()\r\n\
{\r\n\
	auto x = \"hello\";\r\n\
	char[] y = x;\r\n\
	return y;\r\n\
}\r\n\
void test(int a, b, c){}\r\n\
char[] res = foo();\r\n\
test(0,0,0);\r\n\
return res[0]-'g' + res[1]-'d';";
TEST_RESULT("Returning pointer to local array 1", testLocalArrayReturn1, "2");
