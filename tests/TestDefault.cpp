#include "TestBase.h"

const char	*testDefaultFuncVars1 =
"int def = 4;\r\n\
int func(int a = 5.0, b = def){ return a + b; }\r\n\
\r\n\
int a = func();\r\n\
int b = func(7);\r\n\
def = 12;\r\n\
int a2 = func();\r\n\
int b2 = func(7);\r\n\
int c = func(12, 8);\r\n\
\r\n\
return a;";
TEST("Default function parameter values", testDefaultFuncVars1, "9")
{
	CHECK_INT("a", 0, 9, lastFailed);
	CHECK_INT("b", 0, 11, lastFailed);
	CHECK_INT("a2", 0, 17, lastFailed);
	CHECK_INT("b2", 0, 19, lastFailed);
	CHECK_INT("c", 0, 20, lastFailed);
}

const char	*testDefaultFuncVars2 =
"auto generator(int from = 0){ return auto(){ return from++; }; }\r\n\
\r\n\
auto genEater(auto gen = generator(5))\r\n\
{\r\n\
	int ret = 0;\r\n\
	for(int i = 0; i < 3; i++)\r\n\
		ret += gen();\r\n\
	return ret;\r\n\
}\r\n\
auto u = genEater();\r\n\
auto v = genEater(generator());\r\n\
auto w = genEater(generator(8));\r\n\
\r\n\
return 0;";
TEST("Default function parameter values 2", testDefaultFuncVars2, "0")
{
	CHECK_INT("u", 0, 18, lastFailed);
	CHECK_INT("v", 0, 3, lastFailed);
	CHECK_INT("w", 0, 27, lastFailed);
}

const char	*testDefaultFuncVars3 =
"int test(auto a = auto(int i){ return i++; }, int b = 5){ return a(3) + b; }\r\n\
return test() + test(auto(int l){ return l * 2; });";
TEST_RESULT("Default function parameter values 3", testDefaultFuncVars3, "19");

LOAD_MODULE(test_defargs, "test.defargs", "int func(int a, b = 6){ return a * b; }");
const char	*testDefaultFunctionArgumentExport =
"import test.defargs;\r\n\
return func(5) - func(10, 2);";
TEST_RESULT("Default function argument export and import", testDefaultFunctionArgumentExport, "10");

LOAD_MODULE(test_defargs2, "test.defargs2", "int func(int a, b = 6){ return a * b; } int func(int d, c, a, b = 4){ return d * c + a + b; }");
const char	*testDefaultFunctionArgumentExport2 =
"import test.defargs2;\r\n\
return func(5) - func(10, 2) + func(-1, 2, 3);";
TEST_RESULT("Default function argument export and import 2", testDefaultFunctionArgumentExport2, "15");

LOAD_MODULE(test_defargs3, "test.defargs3", "class Test{ int func(int a, b = 6){ return a * b; } }");
const char	*testDefaultFunctionArgumentExport3 =
"import test.defargs3;\r\n\
Test test;\r\n\
return test.func(5) - test.func(10, 2);";
TEST_RESULT("Default function argument export and import (class function)", testDefaultFunctionArgumentExport3, "10");

int TestDefaultArgs(int a, int b)
{
	return a * b;
}

LOAD_MODULE_BIND(test_defargs4, "test.defargs4", "class Test{ int func(int a, b = 6); }")
{
	nullcBindModuleFunctionHelper("test.defargs4", TestDefaultArgs, "Test::func", 0);
}
const char	*testDefaultFunctionArgumentExport4 =
"import test.defargs4;\r\n\
Test test;\r\n\
return test.func(5) - test.func(10, 2);";
TEST_RESULT("Default function argument export and import (class function external)", testDefaultFunctionArgumentExport4, "10");

LOAD_MODULE(test_defargs5, "test.defargs5", "int foo(int x, char[] a = \"xx\", int y = 0){return x + a[0] + a[1];}");
const char	*testDefaultFunctionArgumentExport5 =
"import test.defargs5;\r\n\
return foo(0) + foo(0, \"cc\");";
TEST_RESULT("Default function argument export and import 5", testDefaultFunctionArgumentExport5, "438");

LOAD_MODULE(test_defargs6, "test.defargs6", "int x = 5; int foo(auto ref a = &x){return int(a);}");
const char	*testDefaultFunctionArgumentExport6 =
"import test.defargs6;\r\n\
return foo() + foo(4);";
TEST_RESULT("Default function argument export and import 6", testDefaultFunctionArgumentExport6, "9");

const char	*testDefaultFunctionConversion1 = "int foo(char[] a = \"xx\"){return a[0]+a[1];} return foo() + foo(\"cc\");";
TEST_RESULT("Function default argument conversion 1", testDefaultFunctionConversion1, "438"); // 2*0x78 + 2*0x63

const char	*testDefaultFunctionConversion2 = "int x = 5; int foo(auto ref a = &x){return int(a);} return foo() + foo(4);";
TEST_RESULT("Function default argument conversion 2", testDefaultFunctionConversion2, "9");

const char	*testDefaultFunctionArgumentsInPrototype1 =
"int test(int a, int b = 10);\r\n\
int test(int a, int b){ return a + b; }\r\n\
return test(4);";
TEST_RESULT("Default function arguments from prototype 1", testDefaultFunctionArgumentsInPrototype1, "14");

const char	*testDefaultFunctionArgumentsInPrototype2 =
"class Foo\r\n\
{\r\n\
	int test(int a, int b = 10);\r\n\
}\r\n\
int Foo::test(int a, int b){ return a + b; }\r\n\
Foo a;\r\n\
return a.test(4);";
TEST_RESULT("Default function arguments from prototype 2", testDefaultFunctionArgumentsInPrototype2, "14");

LOAD_MODULE(test_defargs7, "test.defargs7", "int test(int a, int b = 10); int test(int a, int b){ return a + b; }");
const char	*testDefaultFunctionArgumentExport7 =
"import test.defargs7;\r\n\
return test(4);";
TEST_RESULT("Default function argument export and import 7", testDefaultFunctionArgumentExport7, "14");

LOAD_MODULE(test_defargs8, "test.defargs8", "class Foo{ int test(int a, int b = 10); } int Foo::test(int a, int b){ return a + b; }");
const char	*testDefaultFunctionArgumentExport8 =
"import test.defargs8;\r\n\
Foo a;\r\n\
return a.test(4);";
TEST_RESULT("Default function argument export and import 8", testDefaultFunctionArgumentExport8, "14");
