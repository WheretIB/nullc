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
