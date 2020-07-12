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
	CHECK_INT("b", 0, 15, lastFailed);
	CHECK_DOUBLE("c", 0, 12.0, lastFailed);

	CHECK_FLOAT("m", 0, 3.0f, lastFailed);
	CHECK_FLOAT("m", 1, 0.0f, lastFailed);
	CHECK_FLOAT("m", 2, 1.0f, lastFailed);

	CHECK_FLOAT("n", 0, 3.0f, lastFailed);
	CHECK_FLOAT("m", 1, 0.0f, lastFailed);
	CHECK_FLOAT("n", 2, 1.0f, lastFailed);

	CHECK_FLOAT("nd", 0, 10.0f, lastFailed);
	CHECK_DOUBLE("k", 0, 12.0f, lastFailed);

	for(int i = 0; i < 10; i++)
		CHECK_FLOAT("ar", i, 3.0, lastFailed);
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
	CHECK_INT("t1", 0, 4, lastFailed);
	CHECK_INT("t2", 0, 16, lastFailed);
	CHECK_INT("t3", 0, 12, lastFailed);
	CHECK_INT("t4", 0, 4, lastFailed);
	CHECK_INT("t5", 0, 8, lastFailed);
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
	CHECK_INT("i", 0, 1, lastFailed);
	CHECK_INT("m", 0, 0, lastFailed);
	CHECK_INT("i2", 0, 4, lastFailed);
	CHECK_INT("n", 0, 0, lastFailed);

	CHECK_DOUBLE("d", 0, 3.0f, lastFailed);
	CHECK_INT("fr", 0, 0, lastFailed);
	CHECK_INT("rr", 0, 4 + NULLC_PTR_SIZE, lastFailed);
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

const char	*testFunctionPointerSelect4 = 
"int foo(int a){ return -a; }\r\n\
int foo(double a){ return a*2; }\r\n\
\r\n\
int bar(int ref(char, float) f, double y){ return f(y, 2); }\r\n\
int bar(int ref(double) f, double y){ return f(y); }\r\n\
\r\n\
return bar(foo, 5);";
TEST_RESULT("Function pointer select 4", testFunctionPointerSelect4, "10");

const char	*testFunctionPointerSelect5 = 
"int foo(int x){ return x * 4; }\r\n\
float foo(int x){ return x - 5.5f; }\r\n\
\r\n\
int ref(int) x = foo;\r\n\
\r\n\
return x(2);";
TEST_RESULT("Function pointer select 5 (return type overloads)", testFunctionPointerSelect5, "8");

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

const char	*testShortInlineFunction8 = 
"auto curry(generic ref(@A, @B) f, int ref(int, int) x = nullptr, y = nullptr)\r\n\
{\r\n\
	return auto(A a)\r\n\
	{\r\n\
		return auto(B b){ return f(a, b); };\r\n\
	};\r\n\
}\r\n\
int sum(int a, b){ return a + b; }\r\n\
curry(sum);\r\n\
auto curried = curry(sum, <x, y>{ x + y; });\r\n\
return curried(4)(5);";
TEST_RESULT("Short inline funciton incorrect child alias list handling", testShortInlineFunction8, "9");

const char	*testShortInlineFunction9 = 
"auto curry(generic ref(@A, @B) f, int ref(int, int) x = nullptr, y = nullptr)\r\n\
{\r\n\
	return auto(A a)\r\n\
	{\r\n\
		return auto(B b){ return f(a, b); };\r\n\
	};\r\n\
}\r\n\
int sum(int a, b){ return a + b; }\r\n\
curry(sum, <x, y>{ x + y; });\r\n\
curry(sum, <x, y>{ x + y; });\r\n\
auto curried = curry(sum, <x, y>{ x + y; });\r\n\
return curried(4)(5);";
TEST_RESULT("Short inline funciton incorrect child alias list handling 2", testShortInlineFunction9, "9");

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

const char	*testFunctionTypeInference7 = 
"class Foo{ int x; double y; typeof(x*y) m; }\r\n\
Foo m;\r\n\
return typeof(m.m) == double;";
TEST_RESULT("typeof expression in a class", testFunctionTypeInference7, "1");

const char	*testFunctionTypeInference8 = 
"class Foo{ int x; int[!typeof(x).isReference] m; }\r\n\
Foo m;\r\n\
return m.m.size;";
TEST_RESULT("extended typeof expressions in a class", testFunctionTypeInference8, "1");

const char	*testFunctionTypeInference9 = 
"class Foo{ int x; double y; int[!typeof(y).isReference] m; }\r\n\
Foo m;\r\n\
return typeof(m.m).target == int;";
TEST_RESULT("extended typeof expressions in a class 2", testFunctionTypeInference9, "1");

const char	*testFunctionTypeInference10 = 
"typeof(int) a;\r\n\
return typeof(a) == int;";
TEST_RESULT("typeof type is type, not typeid", testFunctionTypeInference10, "1");

const char	*testFunctionTypeInference11 = 
"double[10] y;\r\n\
int[typeof(y).arraySize] a;\r\n\
return typeof(a).target == int;";
TEST_RESULT("typeof shouldn't change array type", testFunctionTypeInference11, "1");

const char	*testFunctionTypeInference12 = 
"class Foo<T, N>\r\n\
{\r\n\
	N _;\r\n\
	T[typeof(_).argument.size] x;\r\n\
}\r\n\
Foo<int, int ref(int, int)> a;\r\n\
return a.x.size;";
TEST_RESULT("extended typeof expressions in a class 3", testFunctionTypeInference12, "2");

const char	*testFunctionTypeInference13 = 
"class vector<T>\r\n\
{\r\n\
	typeof(T).target[typeof(T).arraySize] arr;\r\n\
}\r\n\
vector<float[3]> a;\r\n\
return a.arr.size;";
TEST_RESULT("extended typeof expressions in a class 4", testFunctionTypeInference13, "3");

const char	*testTypePostExpression1 = 
"typedef int[5] ref T;\r\n\
return T.target.target == int;";
TEST_RESULT("extended typeof expressions after type", testTypePostExpression1, "1");

const char	*testTypePostExpression2 = 
"typedef int[5] ref T;\r\n\
return T.target.arraySize;";
TEST_RESULT("extended typeof expressions after type 2", testTypePostExpression2, "5");

const char	*testTypePostExpression3 = 
"class Foo\r\n\
{\r\n\
	int[5] arr;\r\n\
}\r\n\
return Foo.arr.arraySize;";
TEST_RESULT("extended typeof expressions after type 3", testTypePostExpression3, "5");

const char	*testTypeofPostExpression17 = 
"class Foo\r\n\
{\r\n\
	int k;\r\n\
}\r\n\
auto foo(generic x, typeof(x).k l)\r\n\
{\r\n\
	return typeof(l) == int;\r\n\
}\r\n\
Foo m;\r\n\
return foo(m, 3.0);";
TEST_RESULT("extended typeof expressions (members) 17", testTypeofPostExpression17, "1");

const char	*testSizeofInExternalFunction = 
"class Foo\r\n\
{\r\n\
	int k;\r\n\
}\r\n\
auto Foo:s(){ return sizeof(Foo); }\r\n\
Foo m;\r\n\
return m.s();";
TEST_RESULT("sizeof in external member function", testSizeofInExternalFunction, "4");

const char	*testShortFunctionInMemberFunctionCall = 
"class Foo{}\r\n\
auto Foo:average(generic ref(int) f){ return f(5); }\r\n\
Foo m;\r\n\
return m.average(<x>{ 5+x; });";
TEST_RESULT("short function in a member function call", testShortFunctionInMemberFunctionCall, "10");

const char	*testShortFunctionInMemberFunctionCall2 = 
"class Foo<T>{}\r\n\
auto Foo:average(generic ref(int) f){ return f(5); }\r\n\
Foo<int> m;\r\n\
return m.average(<x>{ 5+x; });";
TEST_RESULT("short function in a generic type member function call", testShortFunctionInMemberFunctionCall2, "10");

const char	*testShortFunctionInMemberFunctionCall3 = 
"class Foo<T>{}\r\n\
auto Foo:average(generic ref(int, T) f){ return f(5, 4); }\r\n\
Foo<int> m;\r\n\
return m.average(<x, y>{ 5+x+y; });";
TEST_RESULT("short function in a generic type member function call with generic type aliases", testShortFunctionInMemberFunctionCall3, "14");

const char	*testShortFunctionInMemberFunctionCall4 = 
"class Foo\r\n\
{\r\n\
	auto average(generic ref(int) f){ return f(5); }\r\n\
	auto foo(){ return average(<i>{ -i; }); }\r\n\
}\r\n\
Foo k;\r\n\
return k.foo();";
TEST_RESULT("short function in a member function call in a member function", testShortFunctionInMemberFunctionCall4, "-5");

const char	*testShortFunctionInMemberFunctionCall5 = 
"class Foo\r\n\
{\r\n\
	typedef int T;\r\n\
	auto average(generic ref(T) f){ return f(5); }\r\n\
	auto foo(){ return average(<i>{ -i; }); }\r\n\
}\r\n\
Foo k;\r\n\
return k.foo();";
TEST_RESULT("short function in a member function call in a member function with aliases", testShortFunctionInMemberFunctionCall5, "-5");

const char	*testShortFunctionInMemberFunctionCall6 = 
"class Foo\r\n\
{\r\n\
	typedef int T;\r\n\
	auto average(generic ref(T) f){ return f(5); }\r\n\
}\r\n\
auto Foo:foo(){ return average(<i>{ -i; }); }\r\n\
Foo k;\r\n\
return k.foo();";
TEST_RESULT("short function in a member function call in a member function with aliases", testShortFunctionInMemberFunctionCall6, "-5");

const char	*testShortFunctionInMemberFunctionCall7 = 
"auto average(float ref(float) f){ return 10; }\r\n\
class Foo\r\n\
{\r\n\
	typedef int T;\r\n\
	auto average(int ref(T) f){ return f(5); }\r\n\
}\r\n\
auto Foo:foo(){ return average(<i>{ -i; }); }\r\n\
Foo k;\r\n\
return k.foo();";
TEST_RESULT("short function in a member function call in a member function correct selection", testShortFunctionInMemberFunctionCall7, "-5");

const char	*testShortFunctionInGenericFunctionCall = 
"auto average(generic ref(int) f)\r\n\
{\r\n\
	return f(6);\r\n\
}\r\n\
return int(average(<i>{ i += 4; i * 1.5; }));";
TEST_RESULT("short function in a generic function call", testShortFunctionInGenericFunctionCall, "15");

const char	*testTypePostExpression4 = 
"class Foo{ int x; typedef int myInt; }\r\n\
return int == Foo.myInt;";
TEST_RESULT("extended typeof expressions after type 4 (child aliases)", testTypePostExpression4, "1");

const char	*testTypePostExpression5 = 
"class Foo{ int x; typedef double myDouble; typedef int myInt; }\r\n\
return double == Foo.myDouble;";
TEST_RESULT("extended typeof expressions after type 5 (child aliases)", testTypePostExpression5, "1");

const char	*testGenericTypePointerResolve1 = 
"auto foo(generic x){ return -x; }\r\n\
\r\n\
int ref(int) a = foo;\r\n\
double ref(double) b = foo;\r\n\
\r\n\
assert(a(4) == -4);\r\n\
assert(typeof(a(4)) == int);\r\n\
\r\n\
assert(b(5) == -5.0);\r\n\
assert(typeof(b(5)) == double);\r\n\
\r\n\
int ref(int) c = foo;\r\n\
double ref(double) d = foo;\r\n\
\r\n\
return a == c && b == d;";
TEST_RESULT("generic function pointer resolve 1", testGenericTypePointerResolve1, "1");

const char	*testGenericTypePointerResolve2 = 
"class Foo{ int y; int boo(generic x){ return x * y; } int boo(int x){ return x * y; } }\r\n\
Foo x; x.y = 6;\r\n\
int ref(double) m = x.boo;\r\n\
return m(2.5);";
TEST_RESULT("generic function pointer resolve 2", testGenericTypePointerResolve2, "15");

const char	*testGenericTypePointerResolve3 = 
"class Foo\r\n\
{\r\n\
	int y;\r\n\
	int boo(generic x){ return x * y; }\r\n\
	int boo(int x){ return x * y; }\r\n\
}\r\n\
Foo x; x.y = 6;\r\n\
auto Foo:foo()\r\n\
{\r\n\
	int ref(double) m = x.boo;\r\n\
	return m(2.5);\r\n\
}\r\n\
return x.foo();";
TEST_RESULT_SIMPLE("generic function pointer resolve 3", testGenericTypePointerResolve3, "15");

const char	*testGenericTypePointerResolve4 = 
"class Foo\r\n\
{\r\n\
	int y;\r\n\
	int boo(generic x){ return x * y; }\r\n\
}\r\n\
Foo x; x.y = 6;\r\n\
auto Foo:foo()\r\n\
{\r\n\
	int ref(double) m = x.boo;\r\n\
	return m(2.5);\r\n\
}\r\n\
return x.foo();";
TEST_RESULT_SIMPLE("generic function pointer resolve 4", testGenericTypePointerResolve4, "15");

const char	*testGenericTypePointerResolve5 = 
"{\r\n\
	class Foo\r\n\
	{\r\n\
		auto foo(generic x){ return -x; }\r\n\
	}\r\n\
	Foo z;\r\n\
	int ref(int) y = z.foo;\r\n\
	return y(5);\r\n\
}";
TEST_RESULT("generic function pointer resolve 5", testGenericTypePointerResolve5, "-5");

const char	*testGenericTypePointerResolve6 = 
"{\r\n\
	class Foo\r\n\
	{\r\n\
		auto foo(int x){ return -x; }\r\n\
		auto foo(generic x){ return -x; }\r\n\
		int ref(int) bar(){ return foo; }\r\n\
	}\r\n\
	Foo z;\r\n\
	auto y = z.bar();\r\n\
	return y(5);\r\n\
}";
TEST_RESULT("generic function pointer resolve 6", testGenericTypePointerResolve6, "-5");

const char	*testGenericTypePointerResolve7 = 
"class vector<T>{}\r\n\
class foo<T>{ T i; }\r\n\
auto hash_value(vector<foo<@T>> v){ return sizeof(T); }\r\n\
int ref(vector<foo<int>>) a = hash_value;\r\n\
vector<foo<int>> n;\r\n\
return a(n);";
TEST_RESULT("generic function pointer resolve 7", testGenericTypePointerResolve7, "4");

const char	*testTypeofExtenendedExpr1 = 
"class Foo\r\n\
{\r\n\
	int a;\r\n\
	const int c = 7;\r\n\
}\r\n\
assert(typeof(Foo.a.isReference) == bool);\r\n\
assert(typeof(Foo.c) == int);\r\n\
return 1;";
TEST_RESULT("typeof on extended typeof expressions", testTypeofExtenendedExpr1, "1");

const char	*testTypeofExtenendedExpr2 = 
"class Foo\r\n\
{\r\n\
	const char a = 5;\r\n\
	const short b = 7;\r\n\
}\r\n\
assert(typeof(Foo.a + Foo.b) == short);\r\n\
assert(typeof(Foo.b + Foo.a) == short);\r\n\
return 1;";
TEST_RESULT("typeof on extended typeof expressions", testTypeofExtenendedExpr2, "1");

const char	*testTypeofExtenendedExpr3 = 
"class Foo\r\n\
{\r\n\
	int a;\r\n\
	const int b = 7;\r\n\
}\r\n\
assert(Foo.hasMember(a) == true);\r\n\
assert(Foo.hasMember(b) == false);\r\n\
assert(Foo.hasMember(c) == false);\r\n\
return 1;";
TEST_RESULT("typeof on extended typeof expressions (hasMember)", testTypeofExtenendedExpr3, "1");

const char	*testTypeofExtenendedExpr4 = 
"class X{ int first, second; float arraySize; }\r\n\
\r\n\
auto x = X.first;\r\n\
assert(x == int);\r\n\
auto y = X.arraySize;\r\n\
assert(y == float);\r\n\
return 1;";
TEST_RESULT("typeof on extended typeof expressions (members)", testTypeofExtenendedExpr4, "1");

const char	*testTypeofExtenendedExpr5 = 
"class X{ const int first = 3; }\r\n\
\r\n\
return X.first;";
TEST_RESULT("typeof on extended typeof expressions (constants)", testTypeofExtenendedExpr5, "3");

const char	*testShortInlineFunctionNoArguments = 
"int foo(int ref() f){ return f(); }\r\n\
return foo(<>{ 5; });";
TEST_RESULT("short inline function with no arguments", testShortInlineFunctionNoArguments, "5");

const char	*testShortInlineFunctionOverloads =
"float foo(float ref(float) f, int ref x){ return f(1.0f); }\r\n\
int foo(int ref(int) f)\r\n\
{\r\n\
	if(f(1) >= 10)\r\n\
		return 10;\r\n\
\r\n\
	return f(foo(<x>{ return f(x + 1); }));\r\n\
}\r\n\
\r\n\
int bar(int x){ return x + 2; }\r\n\
return foo(bar);";
TEST_RESULT("short inline function produces a selection between overloads with each having an expternal context", testShortInlineFunctionOverloads, "45");
