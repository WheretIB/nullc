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

#ifndef NULLC_ENABLE_C_TRANSLATION

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
TEST_RESULT("Function pointer select", testFunctionPointerSelect2, "10");

#endif

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
sort(arr, <l, r>{ int(l) < int(r); });\r\n\
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
