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
auto main()\r\n\
{\r\n\
	for(i in array)\r\n\
		i = rand(1);\r\n\
} \r\n\
main();\r\n\
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
auto main()\r\n\
{\r\n\
	int j = 5;\r\n\
	for(i in array)\r\n\
		i = rand(1);\r\n\
} \r\n\
main();\r\n\
int diff = 0;\r\n\
for(i in array, j in { 2, 3, 4, 5, 6, 7, 8, 9 })\r\n\
	diff += i-j;\r\n\
return diff;";
TEST_RESULT("Generic function test (locally instanced coroutine context placement) 2", testGeneric34, "0");

const char *testGeneric35 =
"int[8] array;\r\n\
auto main()\r\n\
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
main();\r\n\
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
	CHECK_INT("a", 0, 4);
	CHECK_INT("b", 0, 5);
	CHECK_INT("c", 0, 6);
	CHECK_INT("d", 0, 7);
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
