#include "TestBase.h"

const char	*testPostExpressions =
"typedef char[] string;\r\n\
\r\n\
int string:find(char a)\r\n\
{\r\n\
	int i = 0;\r\n\
	while(i < this.size && this[i] != a)\r\n\
		i++;\r\n\
	if(i == this.size)\r\n\
		i = -1;\r\n\
	return i;\r\n\
}\r\n\
\r\n\
int a = (\"hello\").size + \"me\".size;\r\n\
int b = (\"hi\" + \"me\").size;\r\n\
\r\n\
int l = (\"Pota\" + \"to\").find('a');\r\n\
int l2 = (\"Potato\").find('t');\r\n\
\r\n\
auto str = \"hello\";\r\n\
int l3 = str.find('o');\r\n\
char[] str2 = \"helloworld\";\r\n\
int l4 = str.find('3');\r\n\
\r\n\
int a2 = ({1, 2, 3}).size;\r\n\
int a3 = {1, 2, 3}.size;\r\n\
int a4 = \"as\".size;\r\n\
\r\n\
return 0;";
TEST("Post expressions on arrays and strings", testPostExpressions, "0")
{
	CHECK_INT("a", 0, 9, lastFailed);
	CHECK_INT("b", 0, 5, lastFailed);
	CHECK_INT("l", 0, 3, lastFailed);
	CHECK_INT("l2", 0, 2, lastFailed);
	CHECK_INT("l3", 0, 4, lastFailed);
	CHECK_INT("l4", 0, -1, lastFailed);
	CHECK_INT("a2", 0, 3, lastFailed);
	CHECK_INT("a3", 0, 3, lastFailed);
	CHECK_INT("a4", 0, 3, lastFailed);
}

const char	*testPreAndPostOnGroupA =
"int x = 1, y = 9, z = 101, w = 1001;\r\n\
int ref xr = &x, yr = &y, zr = &z, wr = &w;\r\n\
(*xr)++; ++(*yr); (*zr)--; --(*wr);\r\n\
return x + y + z + w;";
TEST_RESULT("Prefix and postfix expressions on expression in parentheses 1.", testPreAndPostOnGroupA, "1112");

const char	*testPreAndPostOnGroupB =
"int x = 1, y = 9, z = 101, w = 1001;\r\n\
int ref[] r = { &x, &y, &z, &w };\r\n\
(*r[0])++; ++(*r[1]); (*r[2])--; --(*r[3]);\r\n\
return x + y + z + w;";
TEST_RESULT("Prefix and postfix expressions on expression in parentheses 2.", testPreAndPostOnGroupB, "1112");

const char	*testFunctionResultDereference =
"int ref test(auto ref x){ return int ref(x); }\r\n\
return -*test(5);";
TEST_RESULT("Dereference of function result.", testFunctionResultDereference, "-5");
