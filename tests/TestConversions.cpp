#include "TestBase.h"

const char	*testImplicitToRef =
"class float2\r\n\
{\r\n\
float x, y;\r\n\
}\r\n\
float2 float2(float x, y){ float2 ret; ret.x = x; ret.y = y; return ret; }\r\n\
\r\n\
float2 operator+(float2 ref a, float2 ref b){ return float2(a.x+b.x, a.y+b.y); }\r\n\
float2 ref operator+=(float2 ref a, float2 ref b){ a.x += b.x; a.y += b.y; return a; }\r\n\
float2 ref operator=(float2 ref a, float2 ref b){ a.x = b.x; a.y = b.y; return a; }\r\n\
float2 a, b, c, d;\r\n\
\r\n\
a = float2(1, 3);\r\n\
b = float2(5, 10);\r\n\
c = a + b;\r\n\
d = c + float2(100, 14);\r\n\
\r\n\
float2 aa;\r\n\
aa += float2(1, 1);\r\n\
float2 bb = float2(3, 4) += float2(1, 4);\r\n\
return 0;";
TEST("Implicit type to type ref conversions", testImplicitToRef, "0")
{
	CHECK_FLOAT("a", 0, 1, lastFailed);
	CHECK_FLOAT("a", 1, 3, lastFailed);
	CHECK_FLOAT("b", 0, 5, lastFailed);
	CHECK_FLOAT("b", 1, 10, lastFailed);
	CHECK_FLOAT("c", 0, 6, lastFailed);
	CHECK_FLOAT("c", 1, 13, lastFailed);
	CHECK_FLOAT("d", 0, 106, lastFailed);
	CHECK_FLOAT("d", 1, 27, lastFailed);
	CHECK_FLOAT("aa", 0, 1, lastFailed);
	CHECK_FLOAT("aa", 1, 1, lastFailed);
	CHECK_FLOAT("bb", 0, 4, lastFailed);
	CHECK_FLOAT("bb", 1, 8, lastFailed);
}

const char	*testAutoRefToValue =
"int b = 9;\r\n\
auto ref a = &b;\r\n\
int float(auto ref b){ return 5; }\r\n\
return int(a) + float(a);";
TEST_RESULT("Auto ref to value conversion", testAutoRefToValue, "14");

const char	*testImplicitConversionOnReturn =
"class array{ int[10] arr; int size; }\r\n\
void array::push_back(auto ref a){ arr[size++] = int(a); }\r\n\
auto ref operator[](array ref arr, int index){ assert(index<arr.size); return &arr.arr[index]; }\r\n\
\r\n\
array a;\r\n\
a.push_back(1); a.push_back(5);\r\n\
int[] test(){ return { 1, 2 }; }\r\n\
return int(a[0]) + int(a[1]) + test()[0];";
TEST_RESULT("Implicit conversion on return from function", testImplicitConversionOnReturn, "7");

const char	*testImplicitAutoRefDereference =
"int i = 5;\r\n\
auto ref u = &i;\r\n\
int k = u;\r\n\
return k;";
TEST_RESULT("auto ref type implicit dereference in an unambiguous situation", testImplicitAutoRefDereference, "5");

const char	*testFunctionCallConstantConvertion =
"int funcA(int a, float b)\r\n\
{\r\n\
	return a + b * 2;\r\n\
}\r\n\
return funcA(5, 6.6);";
TEST_RESULT("Constant number type conversions in function call.", testFunctionCallConstantConvertion, "18");

const char	*testNullptrToArray = "int[] arr = new int[20]; arr = nullptr; return arr.size;";
TEST_RESULT("nullptr to type[] conversion", testNullptrToArray, "0");

const char	*testAutoRefConversion =
"int foo(int ref x){ return *x; }\r\n\
int x = 5;\r\n\
auto ref y = &x;\r\n\
return foo(int ref(y));";
TEST_RESULT("Auto ref explicit conversion to pointer type.", testAutoRefConversion, "5");

const char	*testDoubleToInt =
"int a = (auto(){ return 0.1; })();\r\n\
int b = (auto(){ return 0.5; })();\r\n\
int c = (auto(){ return 0.9; })();\r\n\
int d = (auto(){ return 1.0; })();\r\n\
int e = (auto(){ return -0.1; })();\r\n\
int f = (auto(){ return -0.5; })();\r\n\
int g = (auto(){ return -0.9; })();\r\n\
int h = (auto(){ return -1.0; })();\r\n\
return (a == 0) && (b == 0) && (c == 0) && (d == 1) && (e == 0) && (f == 0) && (g == 0) && (h == -1);";
TEST_RESULT("Double to int rounding check", testDoubleToInt, "1");

const char	*testDoubleToLong =
"long a = (auto(){ return 0.1; })();\r\n\
long b = (auto(){ return 0.5; })();\r\n\
long c = (auto(){ return 0.9; })();\r\n\
long d = (auto(){ return 1.0; })();\r\n\
long e = (auto(){ return -0.1; })();\r\n\
long f = (auto(){ return -0.5; })();\r\n\
long g = (auto(){ return -0.9; })();\r\n\
long h = (auto(){ return -1.0; })();\r\n\
return (a == 0) && (b == 0) && (c == 0) && (d == 1) && (e == 0) && (f == 0) && (g == 0) && (h == -1);";
TEST_RESULT("Double to long rounding check", testDoubleToLong, "1");

const char *testCompileTimeConversion = "return double(2) < 2.2;";
TEST_RESULT("Compile-time conversion check", testCompileTimeConversion, "1");

const char *testNullptrArgument1 = "int foo(int ref x){ return !x; } return foo(nullptr);";
TEST_RESULT("nullptr in function arguments 1", testNullptrArgument1, "1");

const char *testNullptrArgument2 = "int foo(int[] x){ return x == nullptr; } return foo(nullptr);";
TEST_RESULT("nullptr in function arguments 2", testNullptrArgument2, "1");

const char *testNullptrArgument3 = "int foo(auto ref x){ return !x; } return foo(nullptr);";
TEST_RESULT("nullptr in function arguments 3", testNullptrArgument3, "1");

const char *testNullptrArgument4 = "int foo(void ref() x){ return x == nullptr; } return foo(nullptr);";
TEST_RESULT("nullptr in function arguments 4", testNullptrArgument4, "1");

const char *testNullptrArgument5 = "int foo(auto[] x){ return x.size + 1; } return foo(nullptr) + foo(nullptr);";
TEST_RESULT("nullptr in function arguments 5", testNullptrArgument5, "2");
