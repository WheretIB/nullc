#include "TestBase.h"

const char	*testAuto = 
"//Auto type tests\r\n\
import std.math;\r\n\
float lengthSqr(float3 ref f){ return f.x*f.x+f.y*f.y+f.z*f.z; }\r\n\
float[10] tenArr(float n){ float[10] arr; for(int i = 0; i < 10; i++) arr[i] = n; return arr; }\r\n\
auto b = 15;\r\n\
auto c = 2*2.0;\r\n\
float3 m;\r\n\
m.x = 3.0;\r\n\
m.y = 0.0;\r\n\
m.z = 1;\r\n\
auto n = m, nd = lengthSqr(&n);\r\n\
auto k = c = 12;\r\n\
auto ar = tenArr(3.0);\r\n\
\r\n\
auto u = &b;\r\n\
return *u;";
TEST("Auto type tests", testAuto, "15")
{
	CHECK_INT("b", 0, 15);
	CHECK_DOUBLE("c", 0, 12.0);

	CHECK_FLOAT("m", 0, 3.0f);
	CHECK_FLOAT("m", 1, 0.0f);
	CHECK_FLOAT("m", 2, 1.0f);

	CHECK_FLOAT("n", 0, 3.0f);
	CHECK_FLOAT("m", 1, 0.0f);
	CHECK_FLOAT("n", 2, 1.0f);

	CHECK_FLOAT("nd", 0, 10.0f);
	CHECK_DOUBLE("k", 0, 12.0f);

	for(int i = 0; i < 10; i++)
		CHECK_FLOAT("ar", i, 3.0);
}

const char	*testSizeof = 
"//sizeof tests\r\n\
import std.math;\r\n\
int t1 = sizeof(int); // 4\r\n\
int t2 = sizeof(float4); // 16\r\n\
int t3 = sizeof({4,5,5}); // 12\r\n\
int t4 = sizeof(t3); // 4\r\n\
int t5 = sizeof(t2*0.5); // 8\r\n\
return 1;";
TEST("sizeof tests", testSizeof, "1")
{
	CHECK_INT("t1", 0, 4);
	CHECK_INT("t2", 0, 16);
	CHECK_INT("t3", 0, 12);
	CHECK_INT("t4", 0, 4);
	CHECK_INT("t5", 0, 8);
}

const char	*testTypeof = 
"//typeof tests\r\n\
int test(float a, double b){ return a+b; }\r\n\
int funcref(float a, double b, typeof(test) op){ return op(a, b); }\r\n\
\r\n\
int i=1;\r\n\
auto m = test; // int ref(float, double)\r\n\
typeof(i) i2 = 4.0f; // int\r\n\
typeof(m) n = m; // int ref(float, double)\r\n\
typeof(i*0.2) d = 3; // double\r\n\
typeof(funcref) fr = funcref; // int ref(float, double, int ref(float, double))\r\n\
int rr = sizeof(typeof(fr));\r\n\
\r\n\
return 1;";
TEST("typeof tests", testTypeof, "1")
{
	CHECK_INT("i", 0, 1);
	CHECK_INT("m", 0, 0);
	CHECK_INT("i2", 0, 4);
	CHECK_INT("n", 0, 0);

	CHECK_DOUBLE("d", 0, 3.0f);
	CHECK_INT("fr", 0, 0);
	CHECK_INT("rr", 0, 4 + NULLC_PTR_SIZE);
}

const char	*testAutoReturn = 
"auto f1(){ }\r\n\
auto f2(){ return 3; }\r\n\
return f2();";
TEST_RESULT("Auto return type tests", testAutoReturn, "3");

const char	*testFunctionPointerSelect = 
"int foo(int a){ return -a; }\r\n\
int foo(double a){ return a*2; }\r\n\
\r\n\
int bar(int ref(double) f, double y){ return f(y); }\r\n\
\r\n\
return bar(foo, 5);";
TEST_RESULT("Function pointer select", testFunctionPointerSelect, "10");

const char	*testFunctionPointerSelect2 = 
"int foo(int a){ return -a; }\r\n\
int foo(double a){ return a*2; }\r\n\
\r\n\
int bar(int ref(char) f, double y){ return f(y); }\r\n\
int bar(int ref(double) f, double y){ return f(y); }\r\n\
\r\n\
return bar(foo, 5);";
TEST_RESULT("Function pointer select 2", testFunctionPointerSelect2, "10");

const char	*testFunctionPointerSelect3 = 
"{\r\n\
	int foo(int a, b){ return a + b; }\r\n\
}\r\n\
int caller(int x, y, int ref(int, int) f){ return f(x, y); }\r\n\
\r\n\
int foo(int a, b){ return a * b; }\r\n\
int foo(double a, b){ return a - b; }\r\n\
\r\n\
return caller(4, 5, foo);";
TEST_RESULT("Function pointer select 3", testFunctionPointerSelect3, "20");

const char	*testShortInlineFunction1 = 
"int bar(int ref(double) f, double y){ return f(y); }\r\n\
return bar(<x>{ return -x; }, 5);";
TEST_RESULT("Short inline function definition 1", testShortInlineFunction1, "-5");

const char	*testShortInlineFunction2 = 
"int bar(int ref(double) f, double y){ return f(y); }\r\n\
return bar(<x>{ -x; }, 5);";
TEST_RESULT("Short inline function definition with implicit return", testShortInlineFunction2, "-5");

const char	*testShortInlineFunction3 = 
"int bar(int ref(auto ref) f, double y){ return f(&y); }\r\n\
return bar(<double ref x>{ -*x; }, 5);";
TEST_RESULT("Short inline function with argument conversion", testShortInlineFunction3, "-5");

const char	*testShortInlineFunction4 = 
"int bar(int ref(auto ref, auto ref) f, double y, z){ return f(y, z); }\r\n\
return bar(<double x, double y>{ x > y; }, 3, 4);";
TEST_RESULT("Short inline function with argument conversion2", testShortInlineFunction4, "0");

const char	*testShortInlineFunction5 = 
"import std.algorithm;\r\n\
int[] arr = { 7, 5, 3, 9, 3, 2, 1, 0, 15 };\r\n\
int[] arr2 = { 0, 1, 2, 3, 3, 5, 7, 9, 15 };\r\n\
sort(arr, <l, r>{ *l < *r; });\r\n\
int diff = 0; for(i in arr, j in arr2) diff += i - j;\r\n\
return diff;";
TEST_RESULT("Short inline funciton as a comparator", testShortInlineFunction5, "0");

const char	*testShortInlineFunction6 = 
"import std.algorithm;\r\n\
int[] arr = { 7, 5, 3, 9, 3, 2, 1, 0, 15 };\r\n\
int[] arr2 = { 0, 1, 2, 3, 3, 5, 7, 9, 15 };\r\n\
sort(arr, <int l, int r>{ l < r; });\r\n\
int diff = 0; for(i in arr, j in arr2) diff += i - j;\r\n\
return diff;";
TEST_RESULT("Short inline funciton as a comparator with argument conversion", testShortInlineFunction6, "0");

const char	*testShortInlineFunction7 = 
"import std.algorithm;\r\n\
int[] arr = { 2, 6, 2, 3, 5, 8, 1, 0, 5, 4 };\r\n\
sort(arr, <int a, int b>{ a > b; });\r\n\
map(arr, <int ref x>{ *x = *x * 2; });\r\n\
arr = filter(arr, <int x>{ x < 8; });\r\n\
int sum = 0;\r\n\
for(i in arr) sum += i;\r\n\
return sum;";
TEST_RESULT("Short inline funcitons passed to std.algorithm", testShortInlineFunction7, "16");

const char	*testTypeofOnFunctionArgument = "auto foo(int a, typeof(a) b){ return a + b; } return foo(5, 7);";
TEST_RESULT("typeof on argument of a function in definition", testTypeofOnFunctionArgument, "12");

const char	*testTypeofPostExpression1 = "double foo(int a, double b, long c){ return 0; } return int == typeof(foo).argument.first;";
TEST_RESULT("typeof .argument .first", testTypeofPostExpression1, "1");

const char	*testTypeofPostExpression2 = "double foo(int a, double b, long c){ return 0; } return long == typeof(foo).argument.last;";
TEST_RESULT("typeof .argument .last", testTypeofPostExpression2, "1");

const char	*testTypeofPostExpression3 = "double foo(int a, double b, long c){ return 0; } return double == typeof(foo).argument[1];";
TEST_RESULT("typeof .argument []", testTypeofPostExpression3, "1");

const char	*testTypeofPostExpression4 = "double foo(int a, double b, long c){ return 0; } return double == typeof(foo).return;";
TEST_RESULT("typeof .return", testTypeofPostExpression4, "1");

const char	*testTypeofPostExpression5 = "int ref a; return int == typeof(a).target;";
TEST_RESULT("typeof .target", testTypeofPostExpression5, "1");

const char	*testTypeofPostExpression6 = "char[] a; return char == typeof(a).target;";
TEST_RESULT("typeof .target 2", testTypeofPostExpression6, "1");

const char	*testTypeofPostExpression7 = "float[4] a; return float == typeof(a).target;";
TEST_RESULT("typeof .target 3", testTypeofPostExpression7, "1");

const char	*testShortFunctionArgumentResolve = 
"auto f1(int[] a, int ref(int) b){ a[0] = b(a[1]); }\r\n\
auto f1(double[] a, double ref(double) b){ a[0] = b(a[1]); }\r\n\
int[] arr1 = { 2, 3 };\r\n\
f1(arr1, <i>{ i; });\r\n\
return arr1[0];";
TEST_RESULT("Resolve of short inline function type in a partially ambiguous situation", testShortFunctionArgumentResolve, "3");

const char	*testTypeofPostExpression8 = "int i; return typeof(&i).isReference;";
TEST_RESULT("typeof .isReference 1", testTypeofPostExpression8, "1");

const char	*testTypeofPostExpression9 = "int i; return typeof(i).isReference;";
TEST_RESULT("typeof .isReference 2", testTypeofPostExpression9, "0");

const char	*testTypeofPostExpression10 = "int[6] i; return typeof(i).isArray;";
TEST_RESULT("typeof .isArray 1", testTypeofPostExpression10, "1");

const char	*testTypeofPostExpression11 = "int[6] i; return typeof(i[4]).isArray;";
TEST_RESULT("typeof .isArray 2", testTypeofPostExpression11, "0");

const char	*testTypeofPostExpression12 = "int ref() i; return typeof(i).isFunction;";
TEST_RESULT("typeof .isFunction 1", testTypeofPostExpression12, "1");

const char	*testTypeofPostExpression13 = "int ref() i; return typeof(i()).isFunction;";
TEST_RESULT("typeof .isFunction 2", testTypeofPostExpression13, "0");

const char	*testTypeofPostExpression14 = "int[4] i; return typeof(i).arraySize;";
TEST_RESULT("typeof .arraySize 1", testTypeofPostExpression14, "4");

const char	*testTypeofPostExpression15 = "int[] i; return typeof(i).arraySize;";
TEST_RESULT("typeof .arraySize 2", testTypeofPostExpression15, "-1");

const char	*testTypeofPostExpression16 = "int ref(int, float) i; return typeof(i).argument.size;";
TEST_RESULT("typeof .argument .size", testTypeofPostExpression16, "2");

const char	*testFunctionTypeInference1 =
"int foo(int x){ return -x; }\r\n\
int foo(float x){ return x * 2.0f; }\r\n\
int ref(int) bar(){ return foo; }\r\n\
return bar()(5);";
TEST_RESULT("Function overload type inference from the return type", testFunctionTypeInference1, "-5");

const char	*testFunctionTypeInference2 =
"int foo(int x){ return -x; }\r\n\
int foo(float x){ return x * 2.0f; }\r\n\
int ref(int) bar = foo;\r\n\
return bar(5);";
TEST_RESULT("Function overload type inference from assignment type 1", testFunctionTypeInference2, "-5");

const char	*testFunctionTypeInference3 =
"int foo(int x){ return -x; }\r\n\
int foo(float x){ return x * 2.0f; }\r\n\
int ref(int) bar;\r\n\
bar = foo;\r\n\
return bar(5);";
TEST_RESULT("Function overload type inference from assignment type 2", testFunctionTypeInference3, "-5");

const char	*testFunctionTypeInference4 =
"int foo(int x){ return -x; }\r\n\
int foo(float x){ return x * 2.0f; }\r\n\
int bar(int ref(int) f = foo){ return foo(5); }\r\n\
return bar();";
TEST_RESULT("Function overload type inference in default function parameters", testFunctionTypeInference4, "-5");

const char	*testFunctionTypeInference5 =
"auto operator*(int ref(int) f1, f2){ return auto(float z){ return f1(f2(z)); }; }\r\n\
\r\n\
int foo(int x){ return -x; }\r\n\
int foo(float x){ return x * 2; }\r\n\
\r\n\
int bar(int x){ return x - 5; }\r\n\
\r\n\
int test(int ref(float) f){ return f(10); }\r\n\
\r\n\
return test(foo * bar);";
TEST_RESULT("Function overload type inference in function arguments", testFunctionTypeInference5, "-5");

const char	*testOperatorInference =
"int operator+(int a, b){ return a + b; }\r\n\
int foo(int ref(int, int) f){ return f(3, 4); }\r\n\
return foo(@+);";
TEST_RESULT("Taking pointer to an overloaded operator", testOperatorInference, "7");

const char	*testShortFunctionArgumentConversion = 
"auto foo(int[] a, int ref(int ref) b){ a[0] = b(&a[1]); }\r\n\
int[] arr1 = { 2, 3 };\r\n\
foo(arr1, <int i>{ -i; });\r\n\
return arr1[0];";
TEST_RESULT("Conversion from reference to type in a short inline function", testShortFunctionArgumentConversion, "-3");

const char	*testFunctionTypeInference6 = 
"class Foo\r\n\
{\r\n\
	int foo(int x){ return -x; }\r\n\
	int foo(double x){ return 2*x; }\r\n\
}\r\n\
Foo bar(){ Foo r; return r; }\r\n\
int ref(int) z = bar().foo;\r\n\
return z(5);";
TEST_RESULT("Member function overload type inference after function that returned non-reference type", testFunctionTypeInference6, "-5");
