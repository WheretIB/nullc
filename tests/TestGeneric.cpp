#include "TestBase.h"

const char *testGeneric1 =
"auto test(generic a){ return -a; }\r\n\
int a = test(5);\r\n\
double b = test(5.5);\r\n\
return test(5l) + test(3);";
TEST("Generic function test (one-argument)", testGeneric1, "-8L")
{
	CHECK_INT("a", 0, -5);
	CHECK_DOUBLE("b", 0, -5.5);
}

const char *testGeneric2 =
"auto two(generic a, generic b)\r\n\
{\r\n\
	return a + b;\r\n\
}\r\n\
return int(two(2, 4.5) * 10);";
TEST_RESULT("Generic function test (two arguments)", testGeneric2, "65");

const char *testGeneric3 =
"auto two(generic a, typeof(a) b)\r\n\
{\r\n\
return a + b;\r\n\
}\r\n\
return two(2, 4.4);";
TEST_RESULT("Generic function test (typeof used on generic argument)", testGeneric3, "6");

const char *testGeneric4 =
"auto bind_first(generic arg1, generic function2)\r\n\
{\r\n\
return auto(typeof(function2).arguments.last arg2){ return function2(arg1, arg2); };\r\n\
}\r\n\
int sum(int a, b){ return a + b; }\r\n\
auto x = bind_first(5, sum);\r\n\
return x(3);";
TEST_RESULT("Generic function test (local function return)", testGeneric4, "8");

const char *testGeneric5 =
"auto bind_first(generic arg1, generic function2)\r\n\
{\r\n\
return auto(typeof(function2).arguments.last arg2){ return function2(arg1, arg2); };\r\n\
}\r\n\
auto x = bind_first(5, auto(int a, b){ return a + b; });\r\n\
return x(3);";
TEST_RESULT("Generic function test (local function return)", testGeneric5, "8");

const char *testGeneric6 =
"void foo(generic a){ a[0] = 1; }\r\n\
auto x = { 32, 43, 54 };\r\n\
foo(x);\r\n\
return x[0];";
TEST_RESULT("Generic function test (accepts arrays as [])", testGeneric6, "1");

const char *testGeneric6a =
"void foo(generic a, int b){ a[0] = b; }\r\n\
auto x = { 32, 43, 54 };\r\n\
foo(x, 8);\r\n\
return x[0];";
TEST_RESULT("Generic function test (accepts arrays as []) 2", testGeneric6a, "8");

const char *testGeneric7 =
"auto sum(generic a, generic function, generic b){ return function(a, b); }\r\n\
return sum(5, auto(int a, b){ return a + b; }, 11);";
TEST_RESULT("Generic function test (function to pointer in arguments)", testGeneric7, "16");

const char *testGeneric8 =
"auto sum(generic a, generic b, typeof(a) ref(typeof(a), typeof(b)) function){ return function(a, b); }\r\n\
return sum(5, 11, <a, b>{ a + b; });";
TEST_RESULT("Generic function test (short inline function)", testGeneric8, "16");

const char *testGeneric9 =
"void swap(generic ref a, generic ref b){ typeof(a).target tmp = *a; *a = *b; *b = tmp; }\r\n\
int a = 4, b = 7;\r\n\
swap(3, b);\r\n\
swap(a, 8);\r\n\
swap(a, b);\r\n\
return a * 10 + b;";
TEST_RESULT("Generic function test (generic ref construct)", testGeneric9, "38");

const char *testGeneric10 =
"class Foo\r\n\
{\r\n\
	auto foo(generic a){ return a * 2; }\r\n\
}\r\n\
Foo x;\r\n\
\r\n\
auto a = x.foo(5);\r\n\
auto b = x.foo(3.5);\r\n\
auto c = x.foo(7l);\r\n\
\r\n\
return int(a * 100 + b * 10 + c);";
TEST("Generic member function test", testGeneric10, "1084")
{
	CHECK_INT("a", 0, 10);
	CHECK_DOUBLE("b", 0, 7);
	CHECK_LONG("c", 0, 14);
}

const char *testGeneric11 =
"int foo(int x)\r\n\
{\r\n\
	auto bar(generic b){ return -b * x; }\r\n\
	auto a = bar(4);\r\n\
	auto b = bar(3.5);\r\n\
	auto c = bar(7);\r\n\
	return a * 100 + b * 10 + c;\r\n\
}\r\n\
return foo(-2);";
TEST_RESULT("Generic local function test", testGeneric11, "884");

const char *testGeneric12 =
"auto fact(generic a){ if(a < 1) return typeof(a)(1); return a * fact(a-1); }\r\n\
return int(fact(3l) * fact(4.5));";
TEST_RESULT("Recursion in generic function test", testGeneric12, "354");

const char *testGeneric13 =
"coroutine auto foo(generic a){ while(1) yield a * typeof(a)(2); }\r\n\
auto a = foo(4);\r\n\
auto b = foo(6.5);\r\n\
auto c = foo(3l);\r\n\
auto d = foo(8);\r\n\
auto e = foo(7.5);\r\n\
auto f = foo(7l);\r\n\
return 1;";
TEST("Generic coroutine function test", testGeneric13, "1")
{
	CHECK_INT("a", 0, 8);
	CHECK_DOUBLE("b", 0, 13);
	CHECK_LONG("c", 0, 6);
	CHECK_INT("d", 0, 16);
	CHECK_DOUBLE("e", 0, 15);
	CHECK_LONG("f", 0, 14);
}

const char *testGeneric14 =
"import std.random;\r\n\
void map1(auto[] arr, void ref(auto ref) f){for(int i = 0; i < arr.size; i++) f(arr[i]);}\r\n\
void map2(generic arr, generic f){for(int i = 0; i < arr.size; i++) arr[i] = f(arr[i]);}\r\n\
void map3(generic arr, generic f){for(int i = 0; i < arr.size; i++) f(arr[i]);}\r\n\
int[] arr = new int[1024];\r\n\
for(i in arr) i = rand();\r\n\
map1(arr, <int ref x>{ *x = -*x; });\r\n\
map2(arr, auto(int x){ return -x; });\r\n\
map3(arr, auto(int ref x){ *x = -*x; });\r\n\
return 1;";
TEST_RESULT("Generic functions in a presence of modules", testGeneric14, "1");

LOAD_MODULE(test_generic_export, "test.generic_export", "auto foo(generic u, v){ return u * v; }");
const char *testGeneric15 =
"import test.generic_export;\r\n\
auto a = foo(4, 8);\r\n\
auto b = foo(2, 2.5);\r\n\
return 1;";
TEST("Generic function import", testGeneric15, "1")
{
	CHECK_INT("a", 0, 32);
	CHECK_DOUBLE("b", 0, 5);
}

const char *testGeneric16 =
"import std.random;\r\n\
void map(generic arr, typeof(arr).target ref(typeof(arr).target) f){for(int i = 0; i < arr.size; i++) arr[i] = f(arr[i]);}\r\n\
int[] arr = new int[1024];\r\n\
for(i in arr) i = rand();\r\n\
map(arr, <x>{ -x; });\r\n\
return 1;";
TEST_RESULT("Generic functions and short inline functions using typeof", testGeneric16, "1");

LOAD_MODULE(test_generic_export2, "test.generic_export2", "import std.math; auto foo(generic u, v){ return u * v;/*float2 x = float2(u, v); return dot(x, x);*/ }");
const char *testGeneric17 =
"import test.generic_export2;\r\n\
auto a = foo(4, 8);\r\n\
auto b = foo(2, 2.5);\r\n\
return 1;";
TEST("Generic function import 2", testGeneric17, "1")
{
	CHECK_INT("a", 0, /*48*/32);
	CHECK_DOUBLE("b", 0, /*10.25*/5);
}

const char *testGeneric18 =
"void bubble_sort(generic arr, int ref(typeof(arr).target ref, typeof(arr).target ref) comp)\r\n\
{\r\n\
	for(int i = 0; i < arr.size; i++)\r\n\
	{\r\n\
		for(int j = i + 1; j < arr.size; j++)\r\n\
		{\r\n\
			if(comp(arr[i], arr[j]))\r\n\
			{\r\n\
				typeof(arr).target x = arr[i];\r\n\
				arr[i] = arr[j];\r\n\
				arr[j] = x;\r\n\
			}\r\n\
		}\r\n\
	}\r\n\
}\r\n\
auto arr = { 1, 3, 7, 2, 4, 8, 0 };\r\n\
bubble_sort(arr, <l, r>{ *l > *r; });\r\n\
auto cmp = { 8, 7, 4, 3, 2, 1, 0 };\r\n\
int diff = 0; for(i in arr, j in cmp) diff += i-j;\r\n\
return diff;";
TEST_RESULT("Test for correct marking of dependent argument", testGeneric18, "0");

const char *testGeneric19 =
"auto func(generic a, b, typeof(a*b) ref(typeof(a), typeof(b)) f){ return f(a, b); }\r\n\
return int(10 * func(3, 4.5, auto(int a, double b){ return a * b; }));";
TEST_RESULT("Test for complex typeof usage on generics", testGeneric19, "135");

const char *testGeneric20 =
"auto sum(generic a, b, typeof(a*b) c){ return a + b * c; }\r\n\
auto x = sum(3, 4.5, 2);\r\n\
auto y = sum(3, 4, 2);\r\n\
auto z = sum(3.5, 4, 2);\r\n\
return 0;";
TEST("Generic function import 2", testGeneric20, "0")
{
	CHECK_DOUBLE("x", 0, 12);
	CHECK_INT("y", 0, 11);
	CHECK_DOUBLE("z", 0, 11.5);
}

LOAD_MODULE(test_generic_export3, "test.generic_export3", "import std.math; auto foo(generic u, v){ float2 x = float2(u, v); return typeof(u*v)(dot(x, x)); }");
const char *testGeneric21 =
"import test.generic_export3;\r\n\
import std.math;\r\n\
auto a = foo(4, 8);\r\n\
auto b = foo(2, 2.5);\r\n\
return 1;";
TEST("Generic function import 3", testGeneric21, "1")
{
	CHECK_INT("a", 0, 80);
	CHECK_DOUBLE("b", 0, 10.25);
}
