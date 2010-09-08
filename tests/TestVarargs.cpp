#include "TestBase.h"

const char	*testVarargs1 =
"int sum(auto ref[] args)\r\n\
{\r\n\
	int res = 0;\r\n\
	for(i in args)\r\n\
		res += int(i);\r\n\
	return res;\r\n\
}\r\n\
int a = 3;\r\n\
int b = 4;\r\n\
return sum(4, a, b);";
TEST_RESULT("Function with variable argument count (numbers)", testVarargs1, "11");

const char	*testVarargs2 =
"int algo(int num, auto ref[] funcs)\r\n\
{\r\n\
	int res = num;\r\n\
	for(int i = 0; i < funcs.size; i++)\r\n\
	{\r\n\
		int ref(int) ref f = funcs[i];\r\n\
		res = (*f)(res);\r\n\
	}\r\n\
	return res;\r\n\
}\r\n\
return algo(15, auto(int a){ return a + 5; }, auto(int a){ return a / 4; });";
TEST_RESULT("Function with variable argument count (functions)", testVarargs2, "5");

const char	*testVarargs3 =
"char[] print(auto ref[] args)\r\n\
{\r\n\
	char[] res = \"\";\r\n\
	for(i in args)\r\n\
	{\r\n\
		if(typeid(i) == int)\r\n\
			res += int(i).str();\r\n\
		if(typeid(i) == char[])\r\n\
			res += char[](i);\r\n\
	}\r\n\
	return res;\r\n\
}\r\n\
auto e = print(12, \" \", 14, \" \", 5);\r\n\
char[8] str;\r\n\
for(int i = 0; i < e.size; i++)\r\n\
	str[i] = e[i];\r\n\
return e.size;";
TEST("Function with variable argument count (print)", testVarargs3, "8")
{
	CHECK_STR("str", 0, "12 14 5");
}

const char	*testVarargs4 =
"int sum(int a, auto ref[] args)\r\n\
{\r\n\
	int res = a;\r\n\
	for(i in args)\r\n\
		res += int[](i)[0] + int[](i)[1];\r\n\
	return res;\r\n\
}\r\n\
return sum(1, {10, 100}, {20, 2});";
TEST_RESULT("Function with variable argument count (bug test)", testVarargs4, "133");

const char	*testVarargs5 =
"char[] print(auto ref[] args)\r\n\
{\r\n\
	char[] res = \"\";\r\n\
	for(i in args)\r\n\
	{\r\n\
	switch(typeid(i))\r\n\
		{\r\n\
		case int:\r\n\
			res += int(i).str();\r\n\
			break;\r\n\
		case char[]:\r\n\
			res += char[](i);\r\n\
			break;\r\n\
		}\r\n\
	}\r\n\
	return res;\r\n\
}\r\n\
auto e = print(12, \" \", 14, \" \", 5);\r\n\
char[8] str;\r\n\
for(int i = 0; i < e.size; i++)\r\n\
	str[i] = e[i];\r\n\
return e.size;";
TEST("Function with variable argument count (print)", testVarargs5, "8")
{
	CHECK_STR("str", 0, "12 14 5");
}

const char	*testVarargs6 =
"int sum(int a, auto ref[] args)\r\n\
{\r\n\
	int res = a;\r\n\
	for(i in args)\r\n\
		res += int[](i)[0] + int[](i)[1];\r\n\
	return res;\r\n\
}\r\n\
auto x = sum;\r\n\
return x(1, {10, 100}, {20, 2});";
TEST_RESULT("Function with variable argument count (bug test)", testVarargs6, "133");

const char	*testVarargs7 =
"int sum(auto ref[] args)\r\n\
{\r\n\
	int r = 0;\r\n\
	for(i in args)\r\n\
		r += int(i);\r\n\
	return r;\r\n\
}\r\n\
int wrap(auto ref[] args){ return sum(args); }\r\n\
auto foo = sum;\r\n\
\r\n\
int a = sum(1);\r\n\
int b = wrap(1);\r\n\
foo = sum; int c = foo(1);\r\n\
foo = wrap; int d = foo(1);\r\n\
int e = sum(1, 2, 3);\r\n\
int f = wrap(1, 2, 3);\r\n\
foo = sum; int g = foo(1, 2, 3);\r\n\
foo = wrap; int h = foo(1, 2, 3);\r\n\
return a + b*10 + c*100 + d*1000 + e*10000 + f*100000 + g*1000000 + h*10000000;";
TEST_RESULT("Variable argument count passthrough", testVarargs7, "66661111");

const char	*testVarargs8 =
"int sum(auto ref[] args)\r\n\
{\r\n\
	int r = 0;\r\n\
	for(i in args)\r\n\
		r += int(i);\r\n\
	return r;\r\n\
}\r\n\
int sum(int s, auto ref[] args)\r\n\
{\r\n\
	int r = s - 2;\r\n\
	for(i in args)\r\n\
		r += int(i);\r\n\
	return r;\r\n\
}\r\n\
return sum(5, 10, 300);";
TEST_RESULT("Variable argument function selection should match the function with most correct arguments", testVarargs8, "313");

const char	*testVarargs9 =
"int sum(int s, auto ref[] args)\r\n\
{\r\n\
	int r = s;\r\n\
	for(i in args)\r\n\
		r += int(i);\r\n\
	return r;\r\n\
}\r\n\
return sum(5);";
TEST_RESULT("Variable argument function with 0 arguments through var_args", testVarargs9, "5");

const char	*testVarargs10 =
"int sum(int s, auto ref[] args)\r\n\
{\r\n\
	int r = s;\r\n\
	for(i in args)\r\n\
		r += int(i);\r\n\
	return r;\r\n\
}\r\n\
auto x = sum;\r\n\
return x(7);";
TEST_RESULT("Variable argument function pointer with 0 arguments through var_args", testVarargs10, "7");
