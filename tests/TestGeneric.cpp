#include "TestBase.h"

const char *testGeneric1 =
"auto test(generic a){ return -a; }\r\n\
int a = test(5);\r\n\
double b = test(5.5);\r\n\
return test(5l) + test(3);";
TEST("Generic function test (one-argument)", testGeneric1, "-8L")
{
	CHECK_INT("a", 0, -5, lastFailed);
	CHECK_DOUBLE("b", 0, -5.5, lastFailed);
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
return auto(typeof(function2).argument.last arg2){ return function2(arg1, arg2); };\r\n\
}\r\n\
int sum(int a, b){ return a + b; }\r\n\
auto x = bind_first(5, sum);\r\n\
return x(3);";
TEST_RESULT("Generic function test (local function return)", testGeneric4, "8");

const char *testGeneric5 =
"auto bind_first(generic arg1, generic function2)\r\n\
{\r\n\
return auto(typeof(function2).argument.last arg2){ return function2(arg1, arg2); };\r\n\
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

const char *testGeneric8a =
"auto sum(generic ref a, generic b, typeof(a).target ref(typeof(a).target, typeof(b)) function){ return function(*a, b); }\r\n\
int a = 5; return sum(a, 11, <a, b>{ a + b; });";
TEST_RESULT("Generic function test (short inline function) 2", testGeneric8a, "16");

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
	CHECK_INT("a", 0, 10, lastFailed);
	CHECK_DOUBLE("b", 0, 7, lastFailed);
	CHECK_LONG("c", 0, 14, lastFailed);
}

const char *testGeneric10a =
"class Foo\r\n\
{\r\n\
	int x;\r\n\
	auto foo(generic a){ return a * x; }\r\n\
}\r\n\
Foo x;\r\n\
x.x = 2;\r\n\
auto a = x.foo(5);\r\n\
auto b = x.foo(3.5);\r\n\
auto c = x.foo(7l);\r\n\
\r\n\
return int(a * 100 + b * 10 + c);";
TEST("Generic member function test 2 ", testGeneric10a, "1084")
{
	CHECK_INT("a", 0, 10, lastFailed);
	CHECK_DOUBLE("b", 0, 7, lastFailed);
	CHECK_LONG("c", 0, 14, lastFailed);
}

const char *testGeneric10c =
"class Foo\r\n\
{\r\n\
	auto foo(generic a){ return a * 2; }\r\n\
}\r\n\
Foo x;\r\n\
\r\n\
auto a = x.foo(5);\r\n\
auto b = x.foo(3.5);\r\n\
auto c = x.foo(7000000000l);\r\n\
\r\n\
return long(a * 100 + b * 10 + c);";
TEST("Generic member function test 3", testGeneric10c, "14000001070L")
{
	CHECK_INT("a", 0, 10, lastFailed);
	CHECK_DOUBLE("b", 0, 7, lastFailed);
	CHECK_LONG("c", 0, 14000000000ll, lastFailed);
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
	CHECK_INT("a", 0, 8, lastFailed);
	CHECK_DOUBLE("b", 0, 13, lastFailed);
	CHECK_LONG("c", 0, 6, lastFailed);
	CHECK_INT("d", 0, 16, lastFailed);
	CHECK_DOUBLE("e", 0, 15, lastFailed);
	CHECK_LONG("f", 0, 14, lastFailed);
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
	CHECK_INT("a", 0, 32, lastFailed);
	CHECK_DOUBLE("b", 0, 5, lastFailed);
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
	CHECK_INT("a", 0, /*48*/32, lastFailed);
	CHECK_DOUBLE("b", 0, /*10.25*/5, lastFailed);
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
auto z = sum(3.5, 4, 2.0);\r\n\
return 0;";
TEST("Generic function import 3", testGeneric20, "0")
{
	CHECK_DOUBLE("x", 0, 12, lastFailed);
	CHECK_INT("y", 0, 11, lastFailed);
	CHECK_DOUBLE("z", 0, 11.5, lastFailed);
}

LOAD_MODULE(test_generic_export3, "test.generic_export3", "import std.math; auto foo(generic u, v){ float2 x = float2(u, v); return typeof(u*v)(dot(x, x)); }");
const char *testGeneric21 =
"import test.generic_export3;\r\n\
import std.math;\r\n\
auto a = foo(4, 8);\r\n\
auto b = foo(2, 2.5);\r\n\
return 1;";
TEST("Generic function import 4", testGeneric21, "1")
{
	CHECK_INT("a", 0, 80, lastFailed);
	CHECK_DOUBLE("b", 0, 10.25, lastFailed);
}

const char *testGeneric22 =
"auto foo(generic a){}\r\n\
foo(1);\r\n\
foo(5.0);\r\n\
return 1;";
TEST_RESULT("Generic function with no code inside", testGeneric22, "1");

const char *testGeneric23 =
"class list_node\r\n\
{\r\n\
	list_node ref next;\r\n\
	int value;\r\n\
}\r\n\
\r\n\
auto range(list_node ref c)\r\n\
{\r\n\
	return coroutine auto(){ while(c){ yield c.value; c = c.next; } return 0; };\r\n\
}\r\n\
\r\n\
auto prod(generic f)\r\n\
{\r\n\
	int product = 1;\r\n\
	for(i in f)\r\n\
		product *= i;\r\n\
	return product;\r\n\
}\r\n\
\r\n\
// Create a list of two numbers\r\n\
list_node list;\r\n\
list.value = 2;\r\n\
list.next = new list_node;\r\n\
list.next.value = 5;\r\n\
\r\n\
auto a = prod(range(list));\r\n\
auto b = prod(coroutine auto(){ list_node ref c = &list; while(c){ yield c.value; c = c.next; } return 0; });\r\n\
return a + b;";
TEST_RESULT("Generic function accepting a coroutine", testGeneric23, "20");

const char *testGeneric24 =
"void swap(generic ref a, generic ref b){ typeof(a).target tmp = *a; *a = *b; *b = tmp; }\r\n\
int a = 4, b = 7;\r\n\
swap(3, &b);\r\n\
swap(&a, 8);\r\n\
swap(&a, &b);\r\n\
return a * 10 + b;";
TEST_RESULT("Generic function test (generic ref construct) 2", testGeneric24, "38");

const char *testGeneric25 =
"auto foldl(generic array, typeof(array).target ref(typeof(array).target, typeof(array).target) f)\r\n\
{\r\n\
	auto tmp = array[0];\r\n\
	for(int i = 1; i < array.size; i++)\r\n\
		tmp = f(tmp, array[i]);\r\n\
	return tmp;\r\n\
}\r\n\
auto arr = { 360, 4, 5 };\r\n\
return foldl(arr, <i,j>{ i/j; });";
TEST_RESULT("Generic function test (correct type index in linker)", testGeneric25, "18");

const char *testGeneric26 =
"auto foldl(generic array, typeof(array).target ref(typeof(array).target, typeof(array).target) f)\r\n\
{\r\n\
	auto tmp = array[0];\r\n\
	for(int i = 1; i < array.size; i++)\r\n\
		tmp = f(tmp, array[i]);\r\n\
	return tmp;\r\n\
}\r\n\
\r\n\
auto arr1 = { 350, 4, 5 };\r\n\
auto x1 = foldl(arr1, <i,j>{ i/j; });\r\n\
\r\n\
auto arr2 = { 360.0, 4.0, 5.0 };\r\n\
auto x2 = foldl(arr2, <i,j>{ i/j; });\r\n\
\r\n\
return int(x1 + x2);";
TEST_RESULT("Generic function test (short inline function select with better inference)", testGeneric26, "35");

const char *testGeneric27 =
"auto div(generic a, generic b)\r\n\
{\r\n\
	@if(typeof(a) == int && typeof(b) == int)\r\n\
	{\r\n\
		if(b == 0)\r\n\
			return 0;\r\n\
	}\r\n\
	return a/b;\r\n\
}\r\n\
div(4.0, 2.0);\r\n\
return div(4, 0);";
TEST_RESULT("Generic function test (using a static if)", testGeneric27, "0");

const char *testGeneric28 =
"int y = 4;\r\n\
auto foo(generic a)\r\n\
{\r\n\
	return -y;\r\n\
}\r\n\
{\r\n\
	int y = 2;\r\n\
	return foo(4);\r\n\
}";
TEST_RESULT("Generic function test (taking variable from the correct scope)", testGeneric28, "-4");

const char *testGeneric29 =
"int y = 4;\r\n\
auto foo(generic a)\r\n\
{\r\n\
return -y;\r\n\
}\r\n\
int bar(int y)\r\n\
{\r\n\
	return foo(4);\r\n\
}\r\n\
return bar(1);";
TEST_RESULT("Generic function test (taking variable from the correct scope) 2", testGeneric29, "-4");

const char *testGeneric30 =
"int bar(int y)\r\n\
{\r\n\
	auto foo(generic a)\r\n\
	{\r\n\
		return -y;\r\n\
	}\r\n\
	{\r\n\
		int y = 8;\r\n\
		return foo(4);\r\n\
	}\r\n\
}\r\n\
return bar(4);";
TEST_RESULT("Generic function test (taking variable from the correct scope) 3", testGeneric30, "-4");

const char *testGeneric31 =
"auto foo(generic a)\r\n\
{\r\n\
	return -a;\r\n\
}\r\n\
int ref(int) f1;\r\n\
auto xx(int ref(int) a){ return a; }\r\n\
void bar()\r\n\
{\r\n\
	foo(4);\r\n\
	f1 = xx(foo);\r\n\
}\r\n\
bar();\r\n\
foo(4);\r\n\
int ref(int) f2 = xx(foo);\r\n\
return f1 == f2;";
TEST_RESULT("Generic function test (locally instanced functions should not go out of scope)", testGeneric31, "1");

const char *testGeneric32 =
"import std.math;\r\n\
auto foo(generic i){ return i * math.pi; }\r\n\
return int(foo(3.));";
TEST_RESULT("Generic function test (module variables should be in scope)", testGeneric32, "9");

const char *testGeneric33 =
"coroutine int rand(generic eqn)\r\n\
{\r\n\
	int current = 1;\r\n\
	while(1)\r\n\
		yield current++;\r\n\
}\r\n\
int[8] array;\r\n\
auto main_func()\r\n\
{\r\n\
	for(i in array)\r\n\
		i = rand(1);\r\n\
} \r\n\
main_func();\r\n\
int diff = 0;\r\n\
for(i in array, j in { 1, 2, 3, 4, 5, 6, 7, 8 })\r\n\
	diff += i-j;\r\n\
return diff;";
TEST_RESULT("Generic function test (locally instanced coroutine context placement)", testGeneric33, "0");

const char *testGeneric34 =
"int j = 1;\r\n\
coroutine int rand(generic eqn)\r\n\
{\r\n\
	int current = 1;\r\n\
	while(1)\r\n\
		yield j + current++;\r\n\
}\r\n\
int[8] array;\r\n\
auto main_func()\r\n\
{\r\n\
	int j = 5;\r\n\
	for(i in array)\r\n\
		i = rand(1);\r\n\
} \r\n\
main_func();\r\n\
int diff = 0;\r\n\
for(i in array, j in { 2, 3, 4, 5, 6, 7, 8, 9 })\r\n\
	diff += i-j;\r\n\
return diff;";
TEST_RESULT("Generic function test (locally instanced coroutine context placement) 2", testGeneric34, "0");

const char *testGeneric35 =
"int[8] array;\r\n\
auto main_func()\r\n\
{\r\n\
	coroutine int rand(generic eqn)\r\n\
	{\r\n\
		int current = 1;\r\n\
		while(1)\r\n\
			yield current++;\r\n\
	}\r\n\
	for(i in array)\r\n\
		i = rand(1);\r\n\
} \r\n\
main_func();\r\n\
int diff = 0;\r\n\
for(i in array, j in { 1, 2, 3, 4, 5, 6, 7, 8 })\r\n\
	diff += i-j;\r\n\
return diff;";
TEST_RESULT("Generic function test (locally instanced coroutine context placement) 3", testGeneric35, "0");

const char *testGeneric36 =
"int bar(int y)\r\n\
{\r\n\
	auto foo(generic a)\r\n\
	{\r\n\
		return -y;\r\n\
	}\r\n\
	{\r\n\
		int y = 8;\r\n\
		foo(4);\r\n\
	}\r\n\
	y = -y;\r\n\
	return foo(4);\r\n\
}\r\n\
return bar(4);";
TEST_RESULT("Generic function test (locally instanced local function context placement)", testGeneric36, "4");

const char *testGeneric37 =
"int bar(int y)\r\n\
{\r\n\
	auto foo(generic a)\r\n\
	{\r\n\
		return -y;\r\n\
	}\r\n\
	{\r\n\
		int y = 8;\r\n\
		foo(4);\r\n\
	}\r\n\
	int z = 4;\r\n\
	return foo(4);\r\n\
}\r\n\
return bar(4);";
TEST_RESULT("Generic function test (locally instanced local function context placement) 2", testGeneric37, "-4");

const char *testGeneric38 =
"int bar(int y)\r\n\
{\r\n\
	auto foo(generic a)\r\n\
	{\r\n\
		return -y;\r\n\
	}\r\n\
	{\r\n\
		int k = 3;\r\n\
		{\r\n\
			int y = 8;\r\n\
			foo(4);\r\n\
		}\r\n\
	}\r\n\
	int z = 4;\r\n\
	return foo(4);\r\n\
}\r\n\
return bar(4);";
TEST_RESULT("Generic function test (locally instanced local function context placement) 3", testGeneric38, "-4");

const char *testGeneric39 =
"int a = 4;\r\n\
{\r\n\
	int b = 5;\r\n\
	coroutine int foo(generic x)\r\n\
	{\r\n\
		yield a + x;\r\n\
		return b + x;\r\n\
	}\r\n\
	int c = 6;\r\n\
	int bar()\r\n\
	{\r\n\
		return foo(1) + foo(1);\r\n\
	}\r\n\
	int d = foo(3);\r\n\
	return bar();\r\n\
}";
TEST("Generic function test (locally instanced local function context placement) 4", testGeneric39, "11")
{
	CHECK_INT("a", 0, 4, lastFailed);
	CHECK_INT("b", 0, 5, lastFailed);
	CHECK_INT("c", 0, 6, lastFailed);
	CHECK_INT("d", 0, 7, lastFailed);
}

const char *testGeneric40 =
"int z = 0;\r\n\
int bar(int y)\r\n\
{\r\n\
	auto foo(generic a)\r\n\
	{\r\n\
		return -y;\r\n\
	}\r\n\
	{\r\n\
		int k = 3;\r\n\
		{\r\n\
			int y = 8;\r\n\
			int hell()\r\n\
			{\r\n\
				return foo(4);\r\n\
			}\r\n\
			z = hell();\r\n\
		}\r\n\
	}\r\n\
	int z = 4;\r\n\
	return foo(4);\r\n\
}\r\n\
return bar(4) + z;";
TEST_RESULT("Generic function test (locally instanced local function context placement) 5", testGeneric40, "-8");

const char *testGeneric41 =
"coroutine int rand()\r\n\
{\r\n\
	int current = 0;\r\n\
	auto clamp(generic arg)\r\n\
	{\r\n\
		return ((auto (int v) { return v; })(current) >> 16) & 32767;\r\n\
	}\r\n\
	{\r\n\
		int current = 1;\r\n\
		while(1) \r\n\
		{ \r\n\
			current = 1;\r\n\
			yield clamp(current);\r\n\
		}\r\n\
	}\r\n\
	return current;\r\n\
} \r\n\
int[8] array;\r\n\
for(i in array)\r\n\
	i = rand();\r\n\
int sum = 0;\r\n\
for(i in array)\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("Generic function test (temp variable placement doesn't look like coroutine parameter) 1", testGeneric41, "0");

const char *testGeneric42 =
"coroutine int rand(int ref (int) eqn)\r\n\
{\r\n\
	int current = 0;\r\n\
	auto clamp(generic arg)\r\n\
	{\r\n\
		return ((auto (int v) { return v; })(current) >> 16) & 32767;\r\n\
	}\r\n\
	{\r\n\
		int current = 1;\r\n\
		while(1)\r\n\
		{\r\n\
			current = eqn(current);\r\n\
			yield clamp(current);\r\n\
		}\r\n\
	}\r\n\
	return current;\r\n\
}\r\n\
int[8] array;\r\n\
for(i in array)\r\n\
	i = rand(auto (int x) { return x * 1103515245 + 12345; });\r\n\
int sum = 0;\r\n\
for(i in array)\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("Generic function test (temp variable placement doesn't look like coroutine parameter) 2", testGeneric42, "0");

const char *testGeneric43 =
"coroutine int rand(generic eqn)\r\n\
{\r\n\
	int current = 0;\r\n\
	auto clamp(generic arg)\r\n\
	{\r\n\
		return ((auto (int v) { return v; })(current) >> 16) & 32767;\r\n\
	}\r\n\
	{\r\n\
		int current = 1;\r\n\
		while(1)\r\n\
		{\r\n\
			current = eqn(current);\r\n\
			yield clamp(current);\r\n\
		}\r\n\
	}\r\n\
	return current;\r\n\
}\r\n\
int[8] array;\r\n\
for(i in array)\r\n\
	i = rand(auto (int x) { return x * 1103515245 + 12345; });\r\n\
int sum = 0;\r\n\
for(i in array)\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("Generic function test (temp variable placement doesn't look like coroutine parameter) 3", testGeneric43, "0");

const char *testGeneric44 =
"coroutine int rand(generic eqn)\r\n\
{\r\n\
	int current = 0;\r\n\
	auto clamp(generic arg)\r\n\
	{\r\n\
		return ((auto (int v) { return v; })(current) >> 16) & 32767;\r\n\
	}\r\n\
	yield clamp(current);\r\n\
	{\r\n\
		int current = 1;\r\n\
		while(1)\r\n\
		{\r\n\
			current = eqn(current);\r\n\
			yield clamp(current);\r\n\
		}\r\n\
	}\r\n\
	return current;\r\n\
}\r\n\
int[8] array;\r\n\
for(i in array)\r\n\
	i = rand(auto (int x) { return x * 1103515245 + 12345; });\r\n\
int sum = 0;\r\n\
for(i in array)\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("Generic function test (temp variable placement doesn't look like coroutine parameter) 4", testGeneric44, "0");

const char *testGeneric45 =
"auto foo(generic a)\r\n\
{\r\n\
	auto fact(generic x){ if(x < 1) return typeof(x)(1); return x * fact(x-1); }\r\n\
	return fact(a);\r\n\
}\r\n\
return foo(6);";
TEST_RESULT("Generic function test (recursion in local generic function)", testGeneric45, "720");

const char *testGeneric46 =
"auto fact(generic x){ if(x < 1) return typeof(x)(1); return x * fact(x-1); }\r\n\
auto foo(generic a)\r\n\
{\r\n\
	return fact(a);\r\n\
}\r\n\
return foo(6);";
TEST_RESULT("Generic function test (recursion in a delayed instance)", testGeneric46, "720");

const char *testGeneric47 =
"coroutine int bar(int v)\r\n\
{\r\n\
	int u = v;\r\n\
	coroutine int foo(generic x, y)\r\n\
	{\r\n\
		return u + x + y;\r\n\
	}\r\n\
	return foo(1, 8);\r\n\
}\r\n\
return bar(5);";
TEST_RESULT("Generic function test (coroutine inside a coroutine)", testGeneric47, "14");

const char *testGeneric48 =
"coroutine int bar(generic v)\r\n\
{\r\n\
	int u = 0;\r\n\
	coroutine int foo(generic x)\r\n\
	{\r\n\
		u += x;\r\n\
		for(x++; x <= 4; x++)\r\n\
		{\r\n\
			foo(x);\r\n\
		}\r\n\
		return u;\r\n\
	}\r\n\
	for(int a = 0; a < 4; a++)\r\n\
	{\r\n\
		u = 0;\r\n\
		yield foo(a);\r\n\
	}\r\n\
	return 0;\r\n\
}\r\n\
auto a = bar(1);\r\n\
return bar(1);";
TEST("Generic function test (coroutine inside a coroutine) 2", testGeneric48, "25")
{
	CHECK_INT("a", 0, 49, lastFailed);
}

LOAD_MODULE(test_generic_export4, "test.generic_export4", "coroutine auto foo(generic a){ yield -a; return a+a; }");
const char *testGeneric49 =
"import test.generic_export4;\r\n\
auto a = foo(4);\r\n\
auto b = foo(4);\r\n\
auto c = foo(4.0);\r\n\
auto d = foo(4.0);\r\n\
return 1;";
TEST("Generic coroutine import", testGeneric49, "1")
{
	CHECK_INT("a", 0, -4, lastFailed);
	CHECK_INT("b", 0, 8, lastFailed);
	CHECK_DOUBLE("c", 0, -4.0, lastFailed);
	CHECK_DOUBLE("d", 0, 8.0, lastFailed);
}

const char *testGeneric50 =
"import test.generic_export4;\r\n\
int bar1(int a){ return foo(a); }\r\n\
double bar2(double a){ return foo(a); }\r\n\
auto a = bar1(4);\r\n\
auto b = bar1(4);\r\n\
auto c = bar2(4.0);\r\n\
auto d = bar2(4.0);\r\n\
return 1;";
TEST("Generic coroutine import 2", testGeneric50, "1")
{
	CHECK_INT("a", 0, -4, lastFailed);
	CHECK_INT("b", 0, 8, lastFailed);
	CHECK_DOUBLE("c", 0, -4.0, lastFailed);
	CHECK_DOUBLE("d", 0, 8.0, lastFailed);
}

const char *testGeneric51 =
"auto cons(generic car, generic cdr)\r\n\
{\r\n\
	return auto(void ref(typeof(car) ref, typeof(cdr) ref) f){ return f(car, cdr); };\r\n\
}\r\n\
auto car(generic cell)\r\n\
{\r\n\
	int result;\r\n\
	cell(<x, _>{ result = *x; });\r\n\
	return result;\r\n\
}\r\n\
auto x = cons(5, cons(6, 7));\r\n\
return car(x);";
TEST_RESULT("Short inline function takes type from function pointer variable", testGeneric51, "5");

const char *testGeneric52 =
"auto cons(generic car, generic cdr)\r\n\
{\r\n\
	return auto (typeof(car) ref(typeof(car)) fcar, typeof(cdr) ref(typeof(cdr)) fcdr) { car = fcar(car); cdr = fcdr(cdr); };\r\n\
}\r\n\
auto car(generic cell)\r\n\
{\r\n\
	typeof(cell).argument[0].argument[0] result;\r\n\
	cell(<x>{ result = x; }, <x>{ x; });\r\n\
	return result;\r\n\
}\r\n\
auto cdr(generic cell)\r\n\
{\r\n\
	typeof(cell).argument[1].argument[0] result;\r\n\
	cell(<x>{ x; }, <x>{ result = x; });\r\n\
	return result;\r\n\
}\r\n\
auto setcar(generic cell, generic car)\r\n\
{\r\n\
	cell(<_>{ car; }, <x>{ x; });\r\n\
}\r\n\
auto setcdr(generic cell, generic cdr)\r\n\
{\r\n\
	cell(<x>{ x; }, <_>{ cdr; });\r\n\
}\r\n\
auto T(generic e0, generic e1) { return cons(e0, e1); }\r\n\
auto T(generic e0, generic e1, generic e2) { return cons(e0, T(e1, e2)); }\r\n\
auto T(generic e0, generic e1, generic e2, generic e3) { return cons(e0, T(e1, e2, e3)); }\r\n\
int match(generic v, generic e0) { *e0 = v; return 1; }\r\n\
int match(generic v, generic e0, generic e1) { *e0 = car(v); return match(cdr(v), e1); }\r\n\
int match(generic v, generic e0, generic e1, generic e2) { *e0 = car(v); return match(cdr(v), e1, e2); }\r\n\
int match(generic v, generic e0, generic e1, generic e2, generic e3) { *e0 = car(v); return match(cdr(v), e1, e2, e3); }\r\n\
auto x = T(1, 2, 3, 4);\r\n\
int x0, x1, x2, x3;\r\n\
if (match(x, &x0, &x1, &x2, &x3))\r\n\
	return x0 * 1000 + x1 * 100 + x2 * 10 + x3;\r\n\
return 0;";
TEST_RESULT("Complex generic extra test (short inline function, chained typeof)", testGeneric52, "1234");

const char *testGeneric53 =
"auto foo(generic a, generic b)\r\n\
{\r\n\
	@if(!(typeof(a) == int && typeof(b) == double))\r\n\
		!\"unsatisfying arguments\";\r\n\
	return int(a + b);\r\n\
}\r\n\
return foo(1, 3.0);";
TEST_RESULT("Function type constrains", testGeneric53, "4");

const char *testGeneric54 =
"auto foo(generic a, generic b)\r\n\
{\r\n\
	@if(!(typeof(a) == int && typeof(b) == double))\r\n\
		!\"unsatisfying arguments\";\r\n\
	return int(a + b);\r\n\
}\r\n\
\r\n\
int bar()\r\n\
{\r\n\
	return foo(1, 3.0);\r\n\
}\r\n\
\r\n\
return bar();";
TEST_RESULT("Function type constrains (delayed instance)", testGeneric54, "4");

const char *testGeneric55 =
"auto foo(generic a)\r\n\
{\r\n\
	@if(!(typeof(a).argument.size == 2))\r\n\
		!\"unsatisfying arguments\";\r\n\
	return a(3, 4);\r\n\
}\r\n\
return foo(auto(int a, b){ return a + b; });";
TEST_RESULT("Function type constrains (extended typeof expressions)", testGeneric55, "7");

const char *testGeneric56 =
"auto foo(generic ref a, generic b)\r\n\
{\r\n\
	@if(!(typeof(a).isReference && typeof(b) == typeof(a).target))\r\n\
		!\"unsatisfying arguments\";\r\n\
	*a = b;\r\n\
}\r\n\
int a = 5;\r\n\
foo(a, 4);\r\n\
return a;";
TEST_RESULT("Function type constrains (extended typeof expressions) 2", testGeneric56, "4");

const char *testGeneric57 =
"auto foo(generic a)\r\n\
{\r\n\
	@if(!(typeof(a).isArray == 1))\r\n\
		!\"unsatisfying arguments\";\r\n\
	return a[1] - 3;\r\n\
}\r\n\
auto a = { 5, 7, 9 };\r\n\
return foo(a);";
TEST_RESULT("Function type constrains (extended typeof expressions) 3", testGeneric57, "4");

const char *testGeneric58 =
"auto foo(generic a)\r\n\
{\r\n\
	@if(!(typeof(a).isFunction && typeof(a).argument.size == 2))\r\n\
		!\"unsatisfying arguments\";\r\n\
	return a(3, 4);\r\n\
}\r\n\
return foo(auto(int a, b){ return a + b; });";
TEST_RESULT("Function type constrains (extended typeof expressions) 4", testGeneric58, "7");

const char *testGeneric59 =
"auto foo(generic a)\r\n\
{\r\n\
	@if(typeof(a).isReference)\r\n\
		return *a * 2;\r\n\
	else\r\n\
		return -a;\r\n\
}\r\n\
int a = 4;\r\n\
auto b = foo(&a); // 8\r\n\
auto c = foo(a); // -4\r\n\
return b * 10 + -c;";
TEST_RESULT("Function type constrains 2", testGeneric59, "84");

const char *testGeneric60 =
"auto bind_first(generic f, generic v)\r\n\
{\r\n\
	@if(typeof(f).argument.size == 1)\r\n\
		return auto(){ return f(v); };\r\n\
	else if(typeof(f).argument.size == 2)\r\n\
		return auto(typeof(f).argument[1] x){ return f(v, x); };\r\n\
	else if(typeof(f).argument.size == 3)\r\n\
		return auto(typeof(f).argument[1] x, typeof(f).argument[2] y){ return f(v, x, y); };\r\n\
	else\r\n\
		!\"unsupported argument count\";\r\n\
}\r\n\
\r\n\
int foo(){ return 5; }\r\n\
int bar(int x){ return -x; }\r\n\
int ken(int x, y){ return x + y; }\r\n\
int joe(int x, y, z){ return x * y + z; }\r\n\
\r\n\
auto y = bind_first(bar, 7);\r\n\
auto z = bind_first(ken, 3);\r\n\
auto w = bind_first(joe, 1);\r\n\
\r\n\
return y() + z(9) + w(7, 100);";
TEST_RESULT("Function type constrains 3", testGeneric60, "112");

const char *testGeneric61 =
"auto foo(generic ref l, int ref(typeof(l), typeof(l)) f){ int a = 4, b = 5; return f(&a, &b); }\r\n\
\r\n\
int k = 3;\r\n\
return foo(&k, <i, j>{ *i + *j; });";
TEST_RESULT("Short inline function in a place where 'generic ref' is used", testGeneric61, "9");

const char *testGeneric62 =
"auto foo(generic a, typeof(a) ref x){ return *x = a; }\r\n\
auto foo(generic a, typeof(a) ref(int) f){ return f(a); }\r\n\
\r\n\
int i = 10;\r\n\
auto x = foo(5, &i);\r\n\
auto y = foo(4, auto(int i){ return -i; });\r\n\
\r\n\
return x - y;";
TEST_RESULT("Generic function overloads", testGeneric62, "9");

const char *testGeneric63 =
"int foo(int a, generic x){ return a + x; }\r\n\
return foo(3, 5);";
TEST_RESULT("Generic function with a non-generic argument", testGeneric63, "8");

const char *testGeneric64 =
"class ph{}\r\n\
auto operator()(ph a, generic i){ return -i; }\r\n\
ph x;\r\n\
return x(5);";
TEST_RESULT("Generic operator", testGeneric64, "-5");

const char *testGeneric65 =
"auto lazy(generic f){ return auto(){ return f(); }; }\r\n\
auto lazy(generic f, a0){ return auto(){ return f(a0); }; }\r\n\
auto lazy(generic f, a0, a1){ return auto(){ return f(a0, a1); }; }\r\n\
auto lazy(generic f, a0, a1, a2){ return auto(){ return f(a0, a1, a2); }; }\r\n\
auto lazy(generic f, a0, a1, a2, a3){ return auto(){ return f(a0, a1, a2, a3); }; }\r\n\
\r\n\
int foo(){ return 5; }\r\n\
int bar(int x){ return -x; }\r\n\
int ken(int x, y){ return x - y; }\r\n\
int joe(int x, y, z){ return x * y + z; }\r\n\
\r\n\
auto x0 = lazy(foo);\r\n\
auto x1 = lazy(bar, 4);\r\n\
auto x2 = lazy(ken, 2, 3);\r\n\
auto x3 = lazy(joe, 10, 5, 3);\r\n\
\r\n\
auto y0 = x0();\r\n\
auto y1 = x1();\r\n\
auto y2 = x2();\r\n\
auto y3 = x3();\r\n\
\r\n\
return y0+y1+y2+y3;";
TEST_RESULT("Lazy function evaluation", testGeneric65, "53");

const char *testGeneric66 =
"int ref(int) x = auto(generic y){ return -y; };\r\n\
return x(5);";
TEST_RESULT("Generic function instance inference 1", testGeneric66, "-5");

const char *testGeneric67a =
"int foo(int ref(int) f){ return f(5); }\r\n\
return foo(auto(generic y){ return -y; });";
TEST_RESULT("Generic function instance inference 2a", testGeneric67a, "-5");

const char *testGeneric67b =
"int foo(@T ref(int) f){ return f(5); }\r\n\
\r\n\
return foo(auto(generic y){ return -y; });";
TEST_RESULT("Generic function instance inference 2b", testGeneric67b, "-5");

const char *testGeneric68 =
"int bar()\r\n\
{\r\n\
	int ref(int) x = auto(generic y){ return -y; };\r\n\
	return x(5);\r\n\
}\r\n\
return bar();";
TEST_RESULT("Generic function instance inference 3", testGeneric68, "-5");

const char *testGeneric69 =
"int bar()\r\n\
{\r\n\
	int foo(int ref(int) f){ return f(5); }\r\n\
	return foo(auto(generic y){ return -y; });\r\n\
}\r\n\
return bar();";
TEST_RESULT("Generic function instance inference 4", testGeneric69, "-5");

const char *testGeneric70 =
"int bar()\r\n\
{\r\n\
	int ref(int) x = coroutine auto(generic y){ yield 2; return -y; };\r\n\
	return x(5) + x(5);\r\n\
}\r\n\
return bar();";
TEST_RESULT("Generic function instance inference 5", testGeneric70, "-3");

const char *testGeneric71 =
"int bar()\r\n\
{\r\n\
	int foo(int ref(int) f){ return f(5) + f(5); }\r\n\
	return foo(coroutine auto(generic y){ yield 2; return -y; });\r\n\
}\r\n\
return bar();";
TEST_RESULT("Generic function instance inference 6", testGeneric71, "-3");

const char *testGeneric72 =
"int bar(int x){ return 6; }\r\n\
auto foo(generic x, typeof(bar(bar(5))) y){ return x + y; }\r\n\
return foo(3, 4);";
TEST_RESULT("Function calls in generic function argument list", testGeneric72, "7");

const char *testGeneric73 =
"auto bar(generic y){ return -y; }\r\n\
int test()\r\n\
{\r\n\
	auto foo(generic x, typeof(bar(5)) y){ return x + y; }\r\n\
	return foo(3, 4);\r\n\
}\r\n\
return test() + bar(5);";
TEST_RESULT("Generic function instance in typeof expression", testGeneric73, "2");

const char *testGeneric74 =
"int ref(int[4]) x = auto foo(generic x){ return x[0] + x[3]; };\r\n\
return x({1, 2, 3, 4});";
TEST_RESULT("Generic type inference to a type with sized array arguments", testGeneric74, "5");

const char *testGeneric75 =
"import std.algorithm;\r\n\
int foo(int a, b){ return a + b; }\r\n\
return bind_last(bind_first(foo, 5), 3)();";
TEST_RESULT("Multiple generic function import", testGeneric75, "8");

LOAD_MODULE(test_generic_export5, "test.generic_export5", "auto foo(generic a, b){ return a + b; }");
LOAD_MODULE(test_generic_export5_a, "test.generic_export5a", "import test.generic_export5; auto x = foo(5, 6);");
LOAD_MODULE(test_generic_export5_b, "test.generic_export5b", "import test.generic_export5; auto y = foo(10.0, 7.0);");
const char *testGeneric76 =
"import test.generic_export5;\r\n\
import test.generic_export5a;\r\n\
import test.generic_export5b;\r\n\
return int(x * y + foo(2l, 3l));";
TEST_RESULT("Generic function import complex", testGeneric76, "192");

LOAD_MODULE(test_generic_export6, "test.generic_export6", "auto foo(generic a, b){ return a + b; }");
LOAD_MODULE(test_generic_export6_a, "test.generic_export6a", "import test.generic_export6; auto x = foo(5, 6);");
LOAD_MODULE(test_generic_export6_b, "test.generic_export6b", "import test.generic_export6; auto y = foo(10, 7);");
const char *testGeneric77 =
"import test.generic_export6;\r\n\
import test.generic_export6a;\r\n\
import test.generic_export6b;\r\n\
return x * y + foo(2, 3);";
TEST_RESULT("Generic function collision on import", testGeneric77, "192");

const char *testGeneric78 =
"auto foo(auto[] arr, generic x, typeof(x) ref(typeof(x)) f){ return f(x); }\r\n\
return foo({ 0 }, 5, <i>{ -i; });";
TEST_RESULT("Generic function with array type in arguments", testGeneric78, "-5");

LOAD_MODULE(test_generic_export7, "test.generic_export7", "auto foo(generic a, int ref(typeof(a)) f){ return a + f(a); }");
const char *testGeneric79 =
"import test.generic_export7;\r\n\
return foo(2, <i>{ i*8; });";
TEST_RESULT("Generic function import, imported function depends on generic", testGeneric79, "18");

LOAD_MODULE(test_generic_export8, "test.generic_export8", "auto foo(generic a, typeof(a)[] f){ return a + f[a]; }");
const char *testGeneric80 =
"import test.generic_export8;\r\n\
return foo(1, { 3, 4 });";
TEST_RESULT("Generic function import, imported array depends on generic", testGeneric80, "5");

LOAD_MODULE(test_generic_export9, "test.generic_export9", "auto foo(generic a, typeof(a) ref f){ return a + *f; }");
const char *testGeneric81 =
"import test.generic_export9;\r\n\
int z = 5; return foo(1, &z);";
TEST_RESULT("Generic function import, imported reference depends on generic", testGeneric81, "6");

const char *testGeneric82 =
"void foo(generic arr, int ref(typeof(arr).target ref, typeof(arr).target ref) f){}\r\n\
int bar(int ref x, y){ return 0; }\r\n\
int[10] arr;\r\n\
foo(arr, bar);\r\n\
foo(arr, bar);\r\n\
return 1;";
TEST_RESULT("Generic function is instanced only once", testGeneric82, "1");

const char *testGeneric83 =
"int a = 5;\r\n\
class Foo\r\n\
{\r\n\
	int x;\r\n\
	auto foo(generic t){ return a + t + x; }\r\n\
}\r\n\
Foo z;\r\n\
z.x = 10;\r\n\
return z.foo(6);";
TEST_RESULT_SIMPLE("Generic member function accesses member variables", testGeneric83, "21");

const char *testGeneric84 =
"int a = 5;\r\n\
class Foo\r\n\
{\r\n\
	int x;\r\n\
	auto foo(generic t){ return a + t + x; }\r\n\
}\r\n\
Foo z;\r\n\
z.x = 10;\r\n\
class Bar\r\n\
{\r\n\
	int y;\r\n\
	auto bar(generic t){ return z.foo(6) + t + y; }\r\n\
}\r\n\
Bar w;\r\n\
w.y = 2000;\r\n\
return w.bar(100);";
TEST_RESULT_SIMPLE("Generic member function instance while generic function is intanced", testGeneric84, "2121");

const char *testGeneric85 =
"class Foo{ auto foo(generic a){ return -a; } }\r\n\
Foo y;\r\n\
auto x = y.foo(5);\r\n\
x += y.foo(12);\r\n\
return x;";
TEST_RESULT("Generic member function has correct name and isn't instanced multiple times", testGeneric85, "-17");

const char *testGeneric86 =
"class Foo\r\n\
{\r\n\
	auto foo(generic a){ return -a; }\r\n\
}\r\n\
Foo y;\r\n\
y.foo(5);\r\n\
class Bar\r\n\
{\r\n\
	Foo x;\r\n\
	auto bar()\r\n\
	{\r\n\
		return x.foo(5);\r\n\
	}\r\n\
}\r\n\
Bar z;\r\n\
return z.bar();";
TEST_RESULT("Generic member function continues type definition while other type is defined", testGeneric86, "-5");

const char *testGeneric87 =
"class Foo\r\n\
{\r\n\
	auto foo(generic a){ return -a; }\r\n\
}\r\n\
class Bar\r\n\
{\r\n\
	Foo x;\r\n\
	auto bar()\r\n\
	{\r\n\
		return x.foo(5);\r\n\
	}\r\n\
}\r\n\
Bar z;\r\n\
return z.bar();";
TEST_RESULT("Generic member function continues type definition while other type is defined 2", testGeneric87, "-5");

const char *testGeneric88 =
"class Foo\r\n\
{\r\n\
	auto foo(generic a){ return -a; }\r\n\
}\r\n\
class Bar\r\n\
{\r\n\
	Foo x;\r\n\
	auto bar(generic z)\r\n\
	{\r\n\
		return x.foo(5) + z;\r\n\
	}\r\n\
}\r\n\
Bar z;\r\n\
return z.bar(2);";
TEST_RESULT("Generic member function continues type definition while other type is defined 3", testGeneric88, "-3");

const char *testGeneric89 =
"auto foo(generic x, int ref(int) f){ return f(x); }\r\n\
int bar(int z)\r\n\
{\r\n\
	return foo(z, auto(int x){ return -x; });\r\n\
}\r\n\
return bar(5);";
TEST_RESULT("Parenthesis in a generic delayed-instance function parameter list", testGeneric89, "-5");

const char *testGeneric90 =
"auto foo(generic x){}\r\n\
int bar(){ foo(5); return 1; }\r\n\
return bar();";
TEST_RESULT("Empty function body in a generic delayed-instance function", testGeneric90, "1");

const char *testGeneric91 =
"auto foo(generic x)\r\n\
{\r\n\
	typedef typeof(x) T;\r\n\
	T a = x * 2;\r\n\
	return a;\r\n\
}\r\n\
int bar(){ return foo(5); }\r\n\
return bar();";
TEST_RESULT("typedef in a generic function", testGeneric91, "10");

const char *testGeneric92 =
"class Foo\r\n\
{\r\n\
	int y;\r\n\
	int boo(generic x){ return x * y; }\r\n\
\r\n\
	void Foo(int n){ y = n; }\r\n\
}\r\n\
auto ref[2] arr;\r\n\
arr[0] = new Foo(20);\r\n\
arr[1] = new Foo(4);\r\n\
\r\n\
int sum = 0;\r\n\
for(i in arr)\r\n\
	sum += i.boo(4); // 4 * 20 + 4 * 4 = 96\r\n\
int sum2 = 0;\r\n\
for(i in arr)\r\n\
	sum2 += i.boo(2.5); // 2.5 * 20 + 2.5 * 4 = 60\r\n\
	return sum * 100 + sum2;";
TEST_RESULT("member generic function call through 'auto ref'", testGeneric92, "9660");

const char *testGeneric93 =
"auto average(generic ref(int) f)\r\n\
{\r\n\
	return f(6);\r\n\
}\r\n\
auto f(int x){ return x * 1.5; }\r\n\
return int(average(f));";
TEST_RESULT("specialization for function pointer", testGeneric93, "9");

const char *testGeneric94 =
"auto average(generic ref(int) f)\r\n\
{\r\n\
	return f(6);\r\n\
}\r\n\
return int(average(<i>{ i * 1.5; }));";
TEST_RESULT("specialization for function pointer with short inline function", testGeneric94, "9");

const char *testGeneric95 =
"auto average(generic ref(int, generic, int) f)\r\n\
{\r\n\
	return f(8, 2.5, 3);\r\n\
}\r\n\
auto f1(int x, y, z){ return x * y + z; }\r\n\
auto f2(int x, float y, int z){ return x * y + z; }\r\n\
return average(f1) + int(average(f2));";
TEST_RESULT("specialization for function pointer 2", testGeneric95, "42");

const char *testGeneric96 =
"auto average(generic ref(int, generic, int) f)\r\n\
{\r\n\
	return f(8, 2.5, 3);\r\n\
}\r\n\
return int(average(<x, float y, z>{ x * y + z; }));";
TEST_RESULT("specialization for function pointer with short inline function 2", testGeneric96, "23");

const char *testGeneric97 =
"auto average(generic ref(int, generic, int) f)\r\n\
{\r\n\
return f(8, 2.5, 3);\r\n\
}\r\n\
return int(average(<x, int y, z>{ x * y + z; }));";
TEST_RESULT("specialization for function pointer with short inline function 3", testGeneric97, "19");

const char *testGeneric98 =
"auto average(generic ref(generic, int, generic) f)\r\n\
{\r\n\
	return f(8.5, 2, 3.5);\r\n\
}\r\n\
auto f1(int x, y, z){ return x * y + z; }\r\n\
auto f2(float x, int y, float z){ return x * y + z; }\r\n\
return average(f1) + int(average(f2));";
TEST_RESULT("specialization for function pointer 3", testGeneric98, "39");

const char *testGeneric99 =
"auto average(generic ref(int, int ref(int, int), int) f)\r\n\
{\r\n\
	return f(8, <i, j>{ i + j; }, 3);\r\n\
}\r\n\
auto f1(int x, int ref(int, int) y, int z){ return x * y(2, 4) + z; }\r\n\
return int(average(f1));";
TEST_RESULT("specialization for function pointer 3", testGeneric99, "51");

const char *testGeneric100 =
"auto average(generic ref(int, int ref(int, int), int) f)\r\n\
{\r\n\
	return f(8, <i, j>{ i + j; }, 3);\r\n\
}\r\n\
return average(<x, y, z>{ x * y(2, 4) + z; });";
TEST_RESULT("specialization for function pointer with short inline function 4", testGeneric100, "51");

const char *testGeneric101 =
"int average(int ref(generic) f, generic m)\r\n\
{\r\n\
	return f(m);\r\n\
}\r\n\
int f1(int x){ return x * 5; }\r\n\
int f2(float x){ return x * 2.5; }\r\n\
return average(f1, 40) + average(f2, 4);";
TEST_RESULT("specialization for function pointer 4 (non generic return type)", testGeneric101, "210");

const char *testGeneric102 =
"int average(int ref(int ref(generic, int)) f)\r\n\
{\r\n\
	return f(<i, j>{ i + j; });\r\n\
}\r\n\
auto f1(int ref(int, int) x){ return x(1, 2) * 5; }\r\n\
return average(f1);";
TEST_RESULT("specialization for function pointer 4 (non generic return type, nested specialized type)", testGeneric102, "15");

const char *testGeneric103 =
"auto average(double ref(generic ref(generic, int)) f)\r\n\
{\r\n\
	return f(<i, j>{ i + j; });\r\n\
}\r\n\
auto f1(int ref(int, int) x){ return x(1, 2) * 5.0; }\r\n\
auto x = average(f1);\r\n\
return x == 15 && typeof(x) == double;";
TEST_RESULT("specialization for function pointer 5 (generic return type in nested specialized type)", testGeneric103, "1");

const char *testGeneric104 =
"int average(int a, int ref(int ref(generic, int)) f)\r\n\
{\r\n\
	return f(<i, j>{ i + j; }) + a;\r\n\
}\r\n\
auto f1(int ref(int, int) x){ return x(1, 2) * 5; }\r\n\
return average(5, f1);";
TEST_RESULT("specialization for function pointer 6 (non generic return type, nested specialized type)", testGeneric104, "20");

const char *testGeneric105 =
"auto average(int a, double ref(generic ref(generic, int)) f)\r\n\
{\r\n\
	return a + f(<i, j>{ i + j; });\r\n\
}\r\n\
auto f1(int ref(int, int) x){ return x(1, 2) * 5.0; }\r\n\
auto x = average(5, f1);\r\n\
return x == 20 && typeof(x) == double;";
TEST_RESULT("specialization for function pointer 7 (generic return type in nested specialized type)", testGeneric105, "1");

const char *testGeneric106 =
"int average(int ref(int, int ref(generic, int)) f)\r\n\
{\r\n\
	return f(5, <i, j>{ i + j; });\r\n\
}\r\n\
auto f1(int a, int ref(int, int) x){ return a + x(1, 2) * 5; }\r\n\
return average(f1);";
TEST_RESULT("specialization for function pointer 8 (non generic return type, nested specialized type)", testGeneric106, "20");

const char *testGeneric107 =
"auto average(double ref(int, generic ref(generic, int)) f)\r\n\
{\r\n\
	return f(5, <i, j>{ i + j; });\r\n\
}\r\n\
auto f1(int a, int ref(int, int) x){ return a + x(1, 2) * 5.0; }\r\n\
auto x = average(f1);\r\n\
return x == 20 && typeof(x) == double;";
TEST_RESULT("specialization for function pointer 9 (generic return type in nested specialized type)", testGeneric107, "1");

const char *testGeneric108 =
"class hashmap{ int ref(int) c; }\r\n\
hashmap map;\r\n\
\r\n\
auto foo(hashmap m, generic key){}\r\n\
foo(map, \"aaa\");\r\n\
\r\n\
return map.c == nullptr;";
TEST_RESULT("test for function pointer corruption", testGeneric108, "1");

const char *testGeneric109 =
"auto foo(@T x)\r\n\
{\r\n\
	T m;\r\n\
	return -x;\r\n\
}\r\n\
auto a = foo(4);\r\n\
auto b = foo(5.0);\r\n\
assert(a == -4);\r\n\
assert(b == -5.0);\r\n\
assert(typeof(a) == int);\r\n\
assert(typeof(b) == double);\r\n\
return 1;";
TEST_RESULT("generic type alias for a regular argument", testGeneric109, "1");

const char *testGeneric110 =
"{\r\n\
	auto foo(generic x, int a = 5){ return x * a; }\r\n\
\r\n\
	assert(foo(4) == 20);\r\n\
	assert(foo(4, 3) == 12);\r\n\
	assert(foo(5) == 25);\r\n\
	assert(int(foo(2.2) + 0.5) == 11);\r\n\
	assert(int(foo(1.5, 6) + 0.5) == 9);\r\n\
	assert(int(foo(2.4) + 0.5) == 12);\r\n\
}\r\n\
{\r\n\
	auto foo(generic x, int a = 5){ return x * a; }\r\n\
\r\n\
	assert(foo(4, 3) == 12);\r\n\
	assert(foo(5) == 25);\r\n\
	assert(int(foo(1.5, 6) + 0.5) == 9);\r\n\
	assert(int(foo(2.4) + 0.5) == 12);\r\n\
}\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values", testGeneric110, "1");

const char *testGeneric111 =
"int foo(int a){ return -a; }\r\n\
\r\n\
{\r\n\
	auto bar(generic x, int ref(int) f = foo){ return f(x); }\r\n\
	assert(bar(4) == -4);\r\n\
}\r\n\
{\r\n\
	auto bar(generic x, int ref(int) f = foo){ return f(x); }\r\n\
	assert(bar(4, <i>{ 2*i; }) == 8);\r\n\
}\r\n\
{\r\n\
	auto bar(generic x = 10, int ref(int) f = foo){ return f(x); }\r\n\
	assert(bar(4, <i>{ 2*i; }) == 8);\r\n\
	assert(bar() == -10);\r\n\
}\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values 2", testGeneric111, "1");

const char *testGeneric112 =
"auto foo(generic x = 4){ return -x; }\r\n\
\r\n\
assert(typeof(foo(4.0)) == double);\r\n\
assert(foo(4.0) == -4.0);\r\n\
assert(foo() == -4);\r\n\
assert(typeof(foo()) == int);\r\n\
assert(typeof(foo(4.0)) == double);\r\n\
assert(foo(4.0) == -4.0);\r\n\
\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values 3", testGeneric112, "1");

LOAD_MODULE(test_generic_export10, "test.generic_export10", "auto foo(generic x, int a = 5){ return x * a; }");
const char *testGeneric113 =
"import test.generic_export10;\r\n\
assert(foo(4) == 20);\r\n\
assert(foo(4, 3) == 12);\r\n\
assert(foo(5) == 25);\r\n\
assert(int(foo(2.2) + 0.5) == 11);\r\n\
assert(int(foo(1.5, 6) + 0.5) == 9);\r\n\
assert(int(foo(2.4) + 0.5) == 12);\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values 4 (import)", testGeneric113, "1");

const char *testGeneric114 =
"import test.generic_export10;\r\n\
assert(foo(4, 3) == 12);\r\n\
assert(foo(5) == 25);\r\n\
assert(int(foo(1.5, 6) + 0.5) == 9);\r\n\
assert(int(foo(2.4) + 0.5) == 12);\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values 5 (import)", testGeneric114, "1");

LOAD_MODULE(test_generic_export11, "test.generic_export11", "auto foo(generic x, int a = 5){ return x * a; } assert(foo(4) == 20);");
const char *testGeneric115 =
"import test.generic_export11;\r\n\
assert(foo(4) == 20);\r\n\
assert(foo(4, 3) == 12);\r\n\
assert(foo(5) == 25);\r\n\
assert(int(foo(2.2) + 0.5) == 11);\r\n\
assert(int(foo(1.5, 6) + 0.5) == 9);\r\n\
assert(int(foo(2.4) + 0.5) == 12);\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values 6 (import)", testGeneric115, "1");

const char *testGeneric116 =
"import test.generic_export11;\r\n\
assert(foo(4, 3) == 12);\r\n\
assert(foo(5) == 25);\r\n\
assert(int(foo(1.5, 6) + 0.5) == 9);\r\n\
assert(int(foo(2.4) + 0.5) == 12);\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values 7 (import)", testGeneric116, "1");

LOAD_MODULE(test_generic_export12, "test.generic_export12", "int foo(int a){ return -a; } auto bar(generic x = 10, int ref(int) f = foo){ return f(x); }");
const char *testGeneric117 =
"import test.generic_export12;\r\n\
assert(bar(4) == -4);\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values 8 (import)", testGeneric117, "1");

const char *testGeneric118 =
"import test.generic_export12;\r\n\
assert(bar(4, <i>{ 2*i; }) == 8);\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values 8 (import)", testGeneric118, "1");

const char *testGeneric119 =
"import test.generic_export12;\r\n\
assert(bar(4, <i>{ 2*i; }) == 8);\r\n\
assert(bar() == -10);\r\n\
return 1;";
TEST_RESULT("generic functions with default argument values 8 (import)", testGeneric119, "1");

const char *testGeneric120 =
"int foo(@T[] arr){ return arr.size; }\r\n\
return foo(\"hello\");";
TEST_RESULT("generic function with alias specialization", testGeneric120, "6");

const char *testGeneric121 =
"int foo(@T[] arr){ return arr.size; }\r\n\
int foo(@T ref arr){ return 500; }\r\n\
return foo(\"hello\") + foo(new int);";
TEST_RESULT("generic function with alias specialization 2", testGeneric121, "506");

const char *testGeneric122 = "int foo(@T ref() arr){ return 1; } return foo(auto(){});";
TEST_RESULT("generic function with alias specialization 3", testGeneric122, "1");

const char *testGeneric123 =
"int bar(){ return 5; }\r\n\
int foo(@T ref()[] arr){ return arr[1](); } return foo({ bar, bar });";
TEST_RESULT("generic function with alias specialization 4", testGeneric123, "5");

const char *testGeneric124 =
"int bar(){ return 5; }\r\n\
int foo(generic ref()[] arr){ return arr[1](); } return foo({ bar, bar });";
TEST_RESULT("generic function specialization 5", testGeneric124, "5");

const char *testGeneric125 = "int bar(generic[] arr){ return arr[1]; } return bar({3, 4});";
TEST_RESULT("generic function specialization 6", testGeneric125, "4");

const char *testGeneric126 =
"int bar(int ref(generic ref) x){ return typeof(x).argument.first.isReference; }\r\n\
return bar(auto(int ref x){ return -*x; });";
TEST_RESULT("generic function specialization 7", testGeneric126, "1");

const char *testGeneric128 =
"auto foo(generic ref x){ typeid y = typeof(x); return y; }\r\n\
int x;\r\n\
assert(foo(4) == int ref);\r\n\
assert(foo(x) == int ref);\r\n\
assert(foo(&x) == int ref);\r\n\
return 1;";
TEST_RESULT("generic function specialization 9", testGeneric128, "1");

const char *testGeneric129 =
"int foo(generic ref(generic, int) x){ return x(4, 5); }\r\n\
int bar(int x, y){ return x + y; }\r\n\
int bar(int x){ return -x; }\r\n\
return foo(bar);";
TEST_RESULT("function inference for a generic argument", testGeneric129, "9");

const char *testGeneric130 =
"int foo(generic ref(generic, int) x){ return x(4, 5); }\r\n\
int bar(int x){ return -x; }\r\n\
int bar(int x, y){ return x + y; }\r\n\
return foo(bar);";
TEST_RESULT("function inference for a generic argument 2", testGeneric130, "9");

const char *testGeneric131 =
"int foo(int ref(generic, int) x){ return x(4, 5); }\r\n\
int bar(int a, generic b){ return a + b; }\r\n\
return foo(bar);";
TEST_RESULT("generic function inference for a generic argument 1", testGeneric131, "9");

const char *testGeneric132 =
"int foo(int ref(generic, int) x){ return x(4, 5); }\r\n\
return foo(int bar(int a, generic b){ return a + b; });";
TEST_RESULT("generic function inference for a generic argument 2", testGeneric132, "9");

const char *testGeneric133 =
"int foo(int ref(int) x){ return x(4); }\r\n\
int bar(generic b){ return b; }\r\n\
return foo(bar);";
TEST_RESULT("generic function inference for a generic argument 5", testGeneric133, "4");

const char *testGeneric134 =
"int foo(int ref(int) x){ return x(4); }\r\n\
return foo(int bar(generic b){ return b; });";
TEST_RESULT("generic function inference for a generic argument 6", testGeneric134, "4");

const char *testGeneric135 =
"int foo(int ref(int) x){ return x(4); }\r\n\
return foo(auto bar(generic b){ return b; });";
TEST_RESULT("generic function inference for a generic argument 7", testGeneric135, "4");

const char *testGeneric136 =
"int foo(generic ref(generic, int) x){ return x(4, 5); }\r\n\
return foo(int bar(int a, generic b){ return a + b; });";
TEST_RESULT("generic function inference for a generic argument 8", testGeneric136, "9");

const char *testGeneric137 =
"int foo(generic ref(generic, int) x){ return x(4, 5); }\r\n\
auto bar(int a, generic b){ return a + b; }\r\n\
return foo(bar);";
TEST_RESULT("generic function inference for a generic argument 9", testGeneric137, "9");

const char *testGeneric138 =
"auto foo(generic ref(generic, int) x){ return x(4, 5); }\r\n\
return foo(auto(int a, generic b){ return a + b; });";
TEST_RESULT("generic function inference for a generic argument 10", testGeneric138, "9");

const char *testGeneric139 =
"int foo(int _, int ref(generic, int) x){ return _ + x(4, 5); }\r\n\
int bar(int a, generic b){ return a + b; }\r\n\
return foo(2, bar);";
TEST_RESULT("generic function inference for a generic argument 11", testGeneric139, "11");

const char *testGeneric140 =
"int foo(generic ref(generic, int) x, y){ return x(4, 5) + y(40, 50); }\r\n\
auto bar(int a, generic b){ return a + b; }\r\n\
return foo(bar, bar);";
TEST_RESULT("generic function inference for a generic argument 12", testGeneric140, "99");

const char *testGeneric141 =
"auto foo(generic x, auto ref[] y)\r\n\
{\r\n\
	return x + typeof(x)(y[0]) + typeof(x)(y[1]);\r\n\
}\r\n\
return foo(1, 2, 3);";
TEST_RESULT("generic function with variable argument count", testGeneric141, "6");

const char *testGeneric142 =
"auto wrap(@T ref(@U) func, auto ref[] args)\r\n\
{\r\n\
	return func(U(args[0]));\r\n\
}\r\n\
\r\n\
auto wrap(@T ref(@U, @V) func, auto ref[] args)\r\n\
{\r\n\
	return func(U(args[0]), V(args[1]));\r\n\
}\r\n\
\r\n\
int foo(int x){ return -x; }\r\n\
int bar(int x, y){ return x + y; }\r\n\
\r\n\
return wrap(foo, 1) + wrap(bar, 2, 3);";
TEST_RESULT("generic function with variable argument count 2", testGeneric142, "4");

const char *testGeneric143 =
"auto foo(generic x, int a = 0, int b = 0){ return x + a + b; }\r\n\
\r\n\
auto x = auto(){ return foo(1, 1); };\r\n\
auto y = foo(1);\r\n\
\r\n\
return x() - y;";
TEST_RESULT("local generic function instantiation with default function argument values", testGeneric143, "1");

const char *testGeneric144 =
"auto func(generic x, y, typeof(!y) ref(int, float) z){ return z(x, y); }\r\n\
auto func(generic x, y, int ref(int, float) z){ return z(x, y); }\r\n\
return func(1, 3.0f, auto(int a, float b){ return 4; });";
TEST_RESULT("SFINAE", testGeneric144, "4");

const char *testGeneric145 =
"auto foo(generic x, auto ref[] y){ return x + y.size; }\r\n\
auto bar(int a, generic x, auto ref[] y){ return x + y.size; }\r\n\
\r\n\
return foo(1) + foo(1, 1, 1, 1, 1) + bar(1, 1);";
TEST_RESULT("generic function with variable argument count 3", testGeneric145, "7");

const char *testGeneric146 =
"int foo(@T ref(int, int) f){ return f(2, 3); }\r\n\
auto x = foo(<i, j>{ i + j; });\r\n\
return x;";
TEST_RESULT("short inline function type inference improvement 1", testGeneric146, "5");

const char *testGeneric147 =
"int foo(int ref(generic, int) f){ return f(2, 3); }\r\n\
auto x = foo(<int i, j>{ i + j; });\r\n\
return x;";
TEST_RESULT("short inline function type inference improvement 2", testGeneric147, "5");

const char *testGeneric148 =
"auto foo(int ref(@T, int) f, f2){ return f(2, 3) + f2(1, 2); }\r\n\
auto x = foo(<int i, j>{ i + j; }, <i, j>{ i + j; });\r\n\
return x;";
TEST_RESULT("short inline function type inference improvement 3", testGeneric148, "8");

const char *testGeneric149 =
"auto foo(@T x, int ref(@T, @T) f){ return x * f(2, 3); }\r\n\
return foo(5, <x, y>{ x + y; });";
TEST_RESULT("short inline function type inference improvement 4", testGeneric149, "25");

const char *testGeneric150 =
"class Foo<T, U>{ T x; U y; void Foo(T x, U y){ this.x = x; this.y = y; } }\r\n\
auto make_Foo(@T x, @U y){ return Foo<T, U>(x, y); }\r\n\
auto foo(Foo<@T, @U> a, b, typeof(a) ref(Foo<@T, @U>, Foo<@T, @U>) f){ return f(a, b); }\r\n\
auto x = foo(make_Foo(2, 4.0f), make_Foo(5, 8.0f), <x, y>{ return make_Foo(x.x + y.x, x.y + y.y); });\r\n\
return int(x.x + x.y);";
TEST_RESULT("short inline function type inference improvement 5", testGeneric150, "19");

const char *testGeneric151 =
"class Foo<T>{ T x; void Foo(T x){ this.x = x; } }\r\n\
auto foo(int ref(int ref(generic)) f){ return f(<x>{ -x; }); }\r\n\
auto x = foo(<int ref(float) x>{ x(5.0f); });\r\n\
return x;";
TEST_RESULT("short inline function type inference improvement 6", testGeneric151, "-5");

const char *testGeneric152 =
"class Foo<T>{ T x; void Foo(T x){ this.x = x; } }\r\n\
auto foo(Foo<@T> a, Foo<@T> b, int ref(Foo<generic>, Foo<@T>) f){ return f(a, b); }\r\n\
auto x = foo(Foo<int>(2), Foo<int>(5), <Foo<int> x, y>{ x.x + y.x; });\r\n\
return x;";
TEST_RESULT("short inline function type inference improvement 7", testGeneric152, "7");

const char *testGeneric153 =
"import test.generic_export4;\r\n\
int bar1(int a){ return foo(a); }\r\n\
double bar2(double a){ return foo(a); }\r\n\
auto a = bar1(4);\r\n\
auto b = foo(4);\r\n\
auto c = bar2(4.0);\r\n\
auto d = foo(4.0);\r\n\
return 1;";
TEST("Generic coroutine import (same instantitaion in local scopes)", testGeneric153, "1")
{
	CHECK_INT("a", 0, -4, lastFailed);
	CHECK_INT("b", 0, 8, lastFailed);
	CHECK_DOUBLE("c", 0, -4.0, lastFailed);
	CHECK_DOUBLE("d", 0, 8.0, lastFailed);
}

const char *testGeneric154 =
"class F{}\r\n\
int F:f(generic x){ return 2; }\r\n\
float F:f(generic x){ return 2; }\r\n\
F a;\r\n\
int operator+(int a, int ref(int) b){ return a + b(1); }\r\n\
return 1 + a.f;";
TEST_RESULT("Class generic function pointer access selection (return type match)", testGeneric154, "3");

const char *testGeneric155 =
"auto foo(@T x){ return new T(){ this.x = 2 * x; }; }\r\n\
\r\n\
class A{ int x = 10; }\r\n\
class B{ float x = 2.5; }\r\n\
\r\n\
return int(foo(A()).x + foo(B()).x + 0.5);";
TEST_RESULT("Local functions must be analyzed again in a generic function instantiation", testGeneric155, "25");

const char *testGeneric156 =
"auto foo(generic ref(generic ref(generic)) bar)\r\n\
{\r\n\
	return bar(<float x>{ 2 * x; });\r\n\
}\r\n\
\r\n\
int a1 = 2;\r\n\
auto a = foo(int aa(int ref(int) x){ return x(a1); });\r\n\
\r\n\
float b1 = 3.5;\r\n\
auto b = foo(int bb(int ref(float) x){ return x(b1); });\r\n\
\r\n\
return a + b;";
TEST_RESULT("Same short function instance request with different types", testGeneric156, "11");

const char *testGeneric157 =
"auto foo(generic a, int ref(int ref(int)) bar)\r\n\
{\r\n\
	return bar(<x>{ a * x; });\r\n\
}\r\n\
\r\n\
auto a = foo(2, int aa(int ref(int) x){ return x(5); });\r\n\
auto b = foo(3.2, int aa(int ref(int) x){ return x(5); });\r\n\
\r\n\
return a + b;";
TEST_RESULT("Same short function instance request with different closure types 1", testGeneric157, "26");

const char *testGeneric158 =
"auto foo(int a, int ref(int ref(int)) bar, generic c)\r\n\
{\r\n\
	return bar(<x>{ a * x; });\r\n\
}\r\n\
\r\n\
auto a = foo(2, int aa(int ref(int) x){ return x(5); }, 1);\r\n\
auto b = foo(3, int aa(int ref(int) x){ return x(5); }, 2.5);\r\n\
\r\n\
return a + b;";
TEST_RESULT("Same short function instance request with different closure types 2", testGeneric158, "25");

const char *testGeneric159 =
"auto foo<@T>(int a, int ref(int ref(int)) bar)\r\n\
{\r\n\
	return bar(<x>{ a * x; });\r\n\
}\r\n\
\r\n\
auto a = foo with<int>(2, int aa(int ref(int) x){ return x(5); });\r\n\
auto b = foo with<float>(3, int aa(int ref(int) x){ return x(5); });\r\n\
\r\n\
return a + b;";
TEST_RESULT("Same short function instance request with different closure types 3", testGeneric159, "25");

const char	*testDefaultGenericFuncVars =
"auto test(generic c, auto a = auto(int i){ return i++; }, int b = 5){ return a(3) + c * b; }\r\n\
return test(1) + test(2, auto(int l){ return l * 2; });";
TEST_RESULT("Generic function with default parameter values", testDefaultGenericFuncVars, "24");

const char	*testExplicitFunctionArgumentGeneric1 =
"int foo(explicit int ref(int) b){ return b(4); }\r\n\
return foo(auto(generic x){ return x * 3; });";
TEST_RESULT("Generic function passed into a function with an explicit argument 1", testExplicitFunctionArgumentGeneric1, "12");

const char	*testExplicitFunctionArgumentGeneric2 =
"int foo(explicit int ref(int) b){ return b(4); }\r\n\
auto bar(generic x){ return x * 3; }\r\n\
return foo(bar);";
TEST_RESULT("Generic function passed into a function with an explicit argument 2", testExplicitFunctionArgumentGeneric2, "12");

const char	*testGenericFunctionWithAutoArg =
"auto foo(generic x, auto y = nullptr){ return x; }\r\n\
return foo(1);";
TEST_RESULT("Generic function with 'auto' argument type", testGenericFunctionWithAutoArg, "1");

const char *testGenericClassOperatorInANamespace =
"namespace Test\r\n\
{\r\n\
	class Foo{ int a; void Foo(int x){ a = x; } }\r\n\
	\r\n\
	int operator+(Foo a, generic b){return a.a + b.a; }\r\n\
}\r\n\
auto x = Test.Foo(1), y = Test.Foo(20);\r\n\
return x + y;";
TEST_RESULT("Generic operator of a class defined in a namespace", testGenericClassOperatorInANamespace, "21");
