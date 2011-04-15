#include "TestBase.h"

const char *testJiTError1 = 
"void median3(int first, middle, last)\r\n\
{\r\n\
	assert(first < 1000);\r\n\
	assert(middle < 1000);\r\n\
	assert(last < 1000);\r\n\
}\r\n\
void median(int first, middle, last)\r\n\
{\r\n\
	int step = (last - first + 1) / 8;\r\n\
	median3(first, first + step, first + 2 * step);\r\n\
}\r\n\
median(588, 794, 999);\r\n\
return 1;";
TEST_RESULT("Test for JiT error 1", testJiTError1, "1");

const char *testJiTError2 = 
"void Input(int ref a){ *a = 3; }\r\n\
int a, b, c;\r\n\
Input(a);\r\n\
b = 5 / a;\r\n\
c = 5 % a;\r\n\
return b + c;";
TEST_RESULT("Test for JiT error 2", testJiTError2, "3");

const char *testJiTError3 = 
"char a = 'a';\r\n\
char b = a;\r\n\
return 1;";
TEST_RESULT("Test for JiT error 3", testJiTError3, "1");

const char *testJiTError4 = 
"int ref x = new int(2);\r\n\
*x *= *x ** *x;\r\n\
return *x;";
TEST_RESULT("Test for JiT error 4", testJiTError4, "8");

const char *testJiTError5 = 
"class Big{ int x, y, z, w; }\r\n\
int foo(int ref(Big, Big) x){ Big a, b; return x(a, b); }\r\n\
return foo(<x, y>{ 5; });";
TEST_RESULT("Test for JiT error 5", testJiTError5, "5");
