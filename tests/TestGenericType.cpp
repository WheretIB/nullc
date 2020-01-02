#include "TestBase.h"

const char *testGenericType1 =
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U ref y;\r\n\
}\r\n\
\r\n\
Foo<int, int> a;\r\n\
a.y = &a.x;\r\n\
a.x = 5;\r\n\
return *a.y;";
TEST_RESULT("Generic type test (POD)", testGenericType1, "5");

const char *testGenericType2 =
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U y;\r\n\
}\r\n\
class Bar\r\n\
{\r\n\
	int bar(){ Foo<int, float> z; z.x = 6; z.y = 8.5; return z.x * z.y; }\r\n\
}\r\n\
Bar v;\r\n\
return v.bar();";
TEST_RESULT("Generic type test (POD instance inside another type)", testGenericType2, "51");

const char *testGenericType3 =
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U ref y;\r\n\
}\r\n\
\r\n\
Foo<int, int> a;\r\n\
Foo<int, float> b;\r\n\
a.y = &a.x;\r\n\
a.x = 5;\r\n\
b.y = new float(5);\r\n\
return int(a.x + *b.y);";
TEST_RESULT("Generic type test (POD with multiple instances)", testGenericType3, "10");

const char *testGenericType4 =
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U ref y;\r\n\
}\r\n\
\r\n\
Foo<int, int> a;\r\n\
Foo<int, float> b;\r\n\
Foo<int, int> c;\r\n\
a.y = &a.x;\r\n\
a.x = 1;\r\n\
b.y = new float(5);\r\n\
c.y = new int(40);\r\n\
return int(a.x + *b.y + *c.y);";
TEST_RESULT("Generic type test (POD with multiple instances of the same type)", testGenericType4, "46");

const char *testGenericType5 =
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U ref y;\r\n\
	auto foo(){ return -x - *y; }\r\n\
}\r\n\
\r\n\
Foo<int, int> a;\r\n\
Foo<int, float> b;\r\n\
Foo<int, int> c;\r\n\
a.y = &a.x;\r\n\
a.x = 1;\r\n\
b.y = new float(5);\r\n\
c.y = new int(40);\r\n\
return int(a.foo() + b.foo() + c.foo());";
TEST_RESULT("Generic type test (with function (internal definition, global instance))", testGenericType5, "-47");

const char *testGenericType6 =
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U y;\r\n\
	int foo(){ return -x; }\r\n\
}\r\n\
class Bar\r\n\
{\r\n\
	int bar(){ Foo<int, float> z; z.x = 6; z.y = 8.5; return -z.foo() * z.y; }\r\n\
}\r\n\
Bar v;\r\n\
return v.bar();";
TEST_RESULT("Generic type test (with function (internal definition, local instance))", testGenericType6, "51");

const char *testGenericType7 =
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U y;\r\n\
	int foo(){ return -x; }\r\n\
}\r\n\
class Bar\r\n\
{\r\n\
	int bar(){ Foo<int, float> z; z.x = 6; z.y = 8.5; return -z.foo() * z.y; }\r\n\
}\r\n\
Bar v;\r\n\
Foo<int, float> u;\r\n\
u.x = 6;\r\n\
return v.bar() + u.foo();";
TEST_RESULT("Generic type test (with function (internal definition, local instance)) 2", testGenericType7, "45");

const char *testGenericType8 =
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U y;\r\n\
	int foo(){ return -x; }\r\n\
}\r\n\
\r\n\
int goo()\r\n\
{\r\n\
	int bar()\r\n\
	{\r\n\
		Foo<int, float> z;\r\n\
		z.x = 6;\r\n\
		z.y = 8.5;\r\n\
		return -z.foo() * z.y;\r\n\
	}\r\n\
	return bar();\r\n\
}\r\n\
Foo<int, float> u;\r\n\
u.x = 6;\r\n\
return goo() + u.foo();";
TEST_RESULT("Generic type test (with function (internal definition, local instance)) 3", testGenericType8, "45");

const char *testGenericType9 =
"class Stack<T>\r\n\
{\r\n\
	T[] arr;\r\n\
	int size, reserved;\r\n\
\r\n\
	void grow()\r\n\
	{\r\n\
		int nReserved = reserved + (reserved >> 1) + 1;\r\n\
		T[] nArr = new T[nReserved];\r\n\
		for(i in arr, j in nArr)\r\n\
			j = i;\r\n\
		arr = nArr;\r\n\
		reserved = nReserved;\r\n\
	}\r\n\
	void push_back(T x)\r\n\
	{\r\n\
		if(size == reserved)\r\n\
			grow();\r\n\
		arr[size++] = x;\r\n\
	}\r\n\
	T pop_back()\r\n\
	{\r\n\
		size--;\r\n\
		return arr[size];\r\n\
	}\r\n\
	T ref back()\r\n\
	{\r\n\
	return &arr[size-1];\r\n\
	}\r\n\
}\r\n\
Stack<int> s;\r\n\
s.push_back(3);\r\n\
s.push_back(4.0);\r\n\
s.push_back(5);\r\n\
return s.pop_back() + s.pop_back() + (s.back() = 8);";
TEST_RESULT("Generic type test Stack type 1", testGenericType9, "17");

const char *testGenericType10 =
"class Stack<T>\r\n\
{\r\n\
	T[] arr;\r\n\
	int size, reserved;\r\n\
\r\n\
	void grow()\r\n\
	{\r\n\
		int nReserved = reserved + (reserved >> 1) + 1;\r\n\
		T[] nArr = new T[nReserved];\r\n\
		for(i in arr, j in nArr)\r\n\
			j = i;\r\n\
		arr = nArr;\r\n\
		reserved = nReserved;\r\n\
	}\r\n\
	void push_back(T x)\r\n\
	{\r\n\
		if(size == reserved)\r\n\
			grow();\r\n\
		arr[size++] = x;\r\n\
	}\r\n\
	T pop_back()\r\n\
	{\r\n\
		size--;\r\n\
		return arr[size];\r\n\
	}\r\n\
	T ref back()\r\n\
	{\r\n\
		return &arr[size-1];\r\n\
	}\r\n\
	T ref front()\r\n\
	{\r\n\
		return &arr[0];\r\n\
	}\r\n\
}\r\n\
Stack<Stack<float> ref> s;\r\n\
Stack<float> s2; Stack<float> s3;\r\n\
s.push_back(s2); s.push_back(s3);\r\n\
s2.push_back(4); s3.push_back(8);\r\n\
return int(*s.front().back() * *s.back().back());";
TEST_RESULT("Generic type test Stack type 2", testGenericType10, "32");

const char *testGenericType11 =
"class Foo<T>{ T x; }\r\n\
int Foo<int>:foo(){ return -x; }\r\n\
Foo<int> x;\r\n\
x.x = 10;\r\n\
return x.foo();";
TEST_RESULT("Generic type specialized external function definition", testGenericType11, "-10");

const char *testGenericType12 =
"class Foo<T>{ T x; }\r\n\
int Foo:foo(){ @if(typeof(this.x) == int) return x; else return -x; }\r\n\
Foo<int> x;\r\n\
Foo<float> z;\r\n\
x.x = 10;\r\n\
z.x = 5;\r\n\
return x.foo() + z.foo();";
TEST_RESULT("Generic type external member function definition", testGenericType12, "5");

const char *testGenericType13 =
"class Foo<T>{ T x; }\r\n\
\r\n\
int Foo:foo(){ return -x; }\r\n\
int Foo<int>:foo(){ return 2*x; }\r\n\
\r\n\
Foo<int> x;\r\n\
\r\n\
int Foo:bar(){ return 9; }\r\n\
\r\n\
Foo<float> z;\r\n\
x.x = 10;\r\n\
z.x = 5;\r\n\
return x.foo() + z.foo() + x.bar();";
TEST_RESULT("Generic type external member function definition after instancing", testGenericType13, "24");

const char *testGenericType14 =
"class Foo<T>{ T x; }\r\n\
\r\n\
int Foo:foo(){ return -x; }\r\n\
int Foo<int>:foo(){ return 2*x; }\r\n\
\r\n\
Foo<int> x;\r\n\
\r\n\
int Foo:bar(){ return 9; }\r\n\
\r\n\
Foo<float> z;\r\n\
x.x = 10;\r\n\
z.x = 5;\r\n\
int test()\r\n\
{\r\n\
	return x.foo() + z.foo() + x.bar();\r\n\
}\r\n\
return test();";
TEST_RESULT("Generic type external member function instancing in an incorrect scope", testGenericType14, "24");

const char *testGenericType15 =
"class Foo<T, U>{ T x; U y; }\r\n\
class Bar{ Foo<int, float> a; Foo<float, int> b; }\r\n\
Bar z;\r\n\
return typeof(z.a.x) == int && typeof(z.a.y) == float && typeof(z.b.x) == float && typeof(z.b.y) == int;";
TEST_RESULT("Generic type instance inside another type", testGenericType15, "1");

const char *testGenericType16 =
"class Foo<T, U>{ T x; U y; }\r\n\
class Bar<T, U>{ Foo<T, U> a; Foo<U, T> b; }\r\n\
Bar<int, float> z;\r\n\
return typeof(z.a.x) == int && typeof(z.a.y) == float && typeof(z.b.x) == float && typeof(z.b.y) == int;";
TEST_RESULT("Generic type instance inside another generic type", testGenericType16, "1");

LOAD_MODULE(test_generic_type1, "test.generic_type1", "class Pair<T, U>{ T x; U y; }");
const char *testGenericType17 =
"import test.generic_type1;\r\n\
Pair<int, float> z; z.x = 4; z.y = 2.5;\r\n\
return int(z.x * z.y);";
TEST_RESULT("Generic type import 1", testGenericType17, "10");

LOAD_MODULE(test_generic_type2, "test.generic_type2", "class Pair<T, U>{ T x; U y; int prod(){ return x * y; } }");
const char *testGenericType18 =
"import test.generic_type2;\r\n\
Pair<int, float> z; z.x = 4; z.y = 2.5;\r\n\
return z.prod();";
TEST_RESULT("Generic type import 2", testGenericType18, "10");

LOAD_MODULE(test_generic_type3, "test.generic_type3", "class Pair<T, U>{ T x; U y; } int Pair:prod(){ return x * y; }");
const char *testGenericType19 =
"import test.generic_type2;\r\n\
Pair<int, float> z; z.x = 4; z.y = 2.5;\r\n\
return z.prod();";
TEST_RESULT("Generic type import 3", testGenericType19, "10");

LOAD_MODULE(test_generic_type2a, "test.generic_type2a", "import test.generic_type2; Pair<int, float> x;");
LOAD_MODULE(test_generic_type2b, "test.generic_type2b", "import test.generic_type2; Pair<int, float> y;");
const char *testGenericType20 =
"import test.generic_type2a;\r\n\
import test.generic_type2b;\r\n\
x.x = 2; x.y = 2.5;\r\n\
y.x = 20; y.y = 1.5;\r\n\
return x.prod() + y.prod();";
TEST_RESULT("Generic type import 4", testGenericType20, "35");

LOAD_MODULE(test_generic_type3a, "test.generic_type3a", "import test.generic_type3; Pair<int, float> x;");
LOAD_MODULE(test_generic_type3b, "test.generic_type3b", "import test.generic_type3; Pair<int, float> y;");
const char *testGenericType21 =
"import test.generic_type3;\r\n\
import test.generic_type3a;\r\n\
import test.generic_type3b;\r\n\
x.x = 2; x.y = 2.5;\r\n\
y.x = 20; y.y = 1.5;\r\n\
return x.prod() + y.prod();";
TEST_RESULT("Generic type import 5", testGenericType21, "35");

LOAD_MODULE(test_generic_type3c, "test.generic_type3c", "import test.generic_type3; Pair<int, float> x; x.x = 2; x.y = 2.5; auto k = x.prod();");
LOAD_MODULE(test_generic_type3d, "test.generic_type3d", "import test.generic_type3; Pair<int, float> y; y.x = 20; y.y = 1.5; auto l = y.prod();");
const char *testGenericType22 =
"import test.generic_type3;\r\n\
import test.generic_type3c;\r\n\
import test.generic_type3d;\r\n\
Pair<int, float> z; z.x = 100; z.y = 1.5; auto m = z.prod();\r\n\
return k + l + m;";
TEST_RESULT("Generic type import 6", testGenericType22, "185");

const char *testGenericType23 =
"class Foo<T>{ T x; }\r\n\
auto foo(Foo<generic> a){ return -a.x; }\r\n\
Foo<int> b; Foo<float> c;\r\n\
b.x = 6; c.x = 2;\r\n\
return int(foo(b) + foo(c));";
TEST_RESULT("Function that accepts generic type 1", testGenericType23, "-8");

const char *testGenericType24 =
"class Foo<T>{ T x; }\r\n\
auto foo(int x, Foo<generic> a){ return x - a.x; }\r\n\
Foo<int> b; Foo<float> c;\r\n\
b.x = 6; c.x = 2;\r\n\
return int(foo(1, b) + foo(1, c));";
TEST_RESULT("Function that accepts generic type 2", testGenericType24, "-6");

const char *testGenericType25 =
"class Foo<T>{ T x; }\r\n\
auto Foo:foo(){ T y = 7; return x + y; }\r\n\
Foo<int> a; a.x = 4;\r\n\
return a.foo();";
TEST_RESULT("Generic type external member function aliases restore", testGenericType25, "11");

const char *testGenericType26 =
"class Foo<T, U>{ T x; U y; }\r\n\
auto foo(Foo<int, generic> a){ return a.x * a.y; }\r\n\
Foo<int, int> b;\r\n\
Foo<int, float> c;\r\n\
b.x = 6; b.y = 1; c.x = 2; c.y = 1.5;\r\n\
return int(foo(b) + foo(c));";
TEST_RESULT("Generic function complex specialization for generic type", testGenericType26, "9");

const char *testGenericType27 =
"class Foo<T, U>{ T x; U y; }\r\n\
auto foo(Foo<generic, generic> a){ return a.x * a.y; }\r\n\
Foo<int, int> b;\r\n\
Foo<float, float> c;\r\n\
b.x = 6; b.y = 1; c.x = 2; c.y = 1.5;\r\n\
return int(foo(b) + foo(c));";
TEST_RESULT("Generic function complex specialization for generic type 2", testGenericType27, "9");

const char *testGenericType28 =
"class Foo<T, U>{ T x; U y; }\r\n\
auto foo(int y, Foo<generic, generic> a){ return y+a.x * a.y; }\r\n\
Foo<int, int> b;\r\n\
Foo<float, float> c;\r\n\
b.x = 6; b.y = 1; c.x = 2; c.y = 1.5;\r\n\
return int(foo(1,b) + foo(2,c));";
TEST_RESULT("Generic function complex specialization for generic type 3", testGenericType28, "12");

const char *testGenericType29 =
"class Foo<T>{ T x; }\r\n\
auto operator +(Foo<generic> ref x, y){ return x.x + y.x; }\r\n\
Foo<int> b; Foo<float> c;\r\n\
b.x = 6; c.x = 2;\r\n\
return int(b + c);";
TEST_RESULT("Generic function complex specialization (fail possibility 1)", testGenericType29, "8");

const char *testGenericType30 =
"class complex<T>{ T re, im; }\r\n\
\r\n\
double complex(double re, im){ return 1; }\r\n\
auto operator * (complex<generic> a, b){ return 1; }\r\n\
\r\n\
int foo(complex<double> a, b)\r\n\
{\r\n\
	return complex(typeof(a.re)(a.re * b.re), 1.0);\r\n\
}\r\n\
complex<double> a, b;\r\n\
return foo(a, b);";
TEST_RESULT("Compiler current type change during constructor call", testGenericType30, "1");

const char *testGenericType31 =
"class Foo<T>{ T x; }\r\n\
auto foo(Foo<@T> x)\r\n\
{\r\n\
	T y = x.x;\r\n\
	return y + x.x;\r\n\
}\r\n\
Foo<int> a;\r\n\
a.x = 5;\r\n\
return foo(a);";
TEST_RESULT("Generic function specialization for generic type with alias", testGenericType31, "10");

const char *testGenericType32 =
"class Foo<T>{ T t; }\r\n\
Foo<Foo<int>> b; b.t.t = 5;\r\n\
auto foo(Foo<Foo<int>> z){ return z.t.t; }\r\n\
return foo(b);";
TEST_RESULT("Nested generic type definition '>>' resolve 1", testGenericType32, "5");

const char *testGenericType33 =
"class Foo<T>{ T t; }\r\n\
class Bar<T, U>{ T t; U s; }\r\n\
auto bar(Bar<@T, Foo<int>> z){ return z.s.t; }\r\n\
Bar<int, Foo<int>> b; b.s.t = 5;\r\n\
return bar(b);";
TEST_RESULT("Nested generic type definition '>>' resolve 2", testGenericType33, "5");

const char *testGenericType34 =
"class Foo<T>{ T x; }\r\n\
Foo<int> a; a.x = 5;\r\n\
auto foo(Foo<generic> ref m){ return -m.x; }\r\n\
return foo(&a);";
TEST_RESULT("Generic type specialization to reference type", testGenericType34, "-5");

const char *testGenericType35 =
"class Foo<T>{ T x; }\r\n\
Foo<int> a; a.x = 5;\r\n\
auto foo(Foo<generic> ref m){ return -m.x; }\r\n\
return foo(a);";
TEST_RESULT("Generic type specialization to reference type 2", testGenericType35, "-5");

const char *testGenericType36 =
"class Foo<T>{ T a; }\r\n\
auto Foo:foo(){ return 2*a; }\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo;\r\n\
auto z = s.foo;\r\n\
return int(y() + z());";
TEST_RESULT("Taking pointer to a generic type member function", testGenericType36, "88");

const char *testGenericType37 =
"class Foo<T>{ T a; }\r\n\
auto Foo:foo(){ return 2*a; }\r\n\
int Foo<double>:foo(){ return -a; }\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo;\r\n\
auto z = s.foo;\r\n\
return int(y() + z());";
TEST_RESULT("Taking pointer to a generic type member function (with a specialized option)", testGenericType37, "-32");

const char *testGenericType38 =
"class Foo<T>\r\n\
{\r\n\
	T a;\r\n\
	auto foo(){ return -a; }\r\n\
}\r\n\
int Foo<double>:foo(){ return 2*a; }\r\n\
\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo;\r\n\
int ref() z = s.foo;\r\n\
return int(y() + z());";
TEST_RESULT("Taking pointer to a generic type member function (overload resolve)", testGenericType38, "76");

const char *testGenericType39 =
"class Foo<T>\r\n\
{\r\n\
	T a;\r\n\
\r\n\
	auto bar(int x){ return -x; }\r\n\
	auto bar(float x){ return 2*x; }\r\n\
}\r\n\
int Foo:foo()\r\n\
{\r\n\
	int ref(int) m = bar;\r\n\
	return m(a);\r\n\
}\r\n\
\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo;\r\n\
auto z = s.foo;\r\n\
return int(y() + z());";
TEST_RESULT("Function call by pointer in a member function", testGenericType39, "-44");

const char *testGenericType40 =
"class Foo<T>{ T x; }\r\n\
void Foo:set(T x){ assert(typeof(x) == double); this.x = x; }\r\n\
Foo<double> m;\r\n\
m.set(4);\r\n\
return int(m.x * 1.5);";
TEST_RESULT("Generic type aliases are available in external unspecialized member function argument list", testGenericType40, "6");

const char *testGenericType41 =
"class Factorial<T>{}\r\n\
auto Factorial<int[1] ref>:get(){ T arr; return arr; }\r\n\
auto Factorial:get()\r\n\
{\r\n\
	Factorial<int[typeof(T).target.arraySize - 1] ref> x;\r\n\
	int[typeof(x.get()).target.arraySize] ref m; // arraysize > 1 ? arraysize - 1 : 1;\r\n\
	typeof(T).target.target[typeof(T).target.arraySize * typeof(m).target.arraySize] ref arr;\r\n\
	return arr;\r\n\
}\r\n\
Factorial<int[8] ref> fact;\r\n\
return typeof(fact.get()).target.arraySize;";
TEST_RESULT("Generic type compile-time factorial", testGenericType41, "40320");

const char *testGenericType42 =
"class Natural<T>\r\n\
{\r\n\
	T x;\r\n\
}\r\n\
auto operator+(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).arraySize + typeof(y.x).arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator-(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).arraySize - typeof(y.x).arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator*(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).arraySize * typeof(y.x).arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator/(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).arraySize / typeof(y.x).arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto factorial(Natural<@T> x)\r\n\
{\r\n\
	Natural<int[1]> one;\r\n\
	@if(typeof(x.x).arraySize > 1)\r\n\
		return x * factorial(x - one);\r\n\
	else\r\n\
		return one;\r\n\
}\r\n\
\r\n\
Natural<int[8]> a;\r\n\
Natural<int[3]> b;\r\n\
Natural<int[4]> c;\r\n\
auto val1 = typeof(((a * b + a) / c).x).arraySize; // 8\r\n\
\r\n\
Natural<int[5]> f;\r\n\
auto fact4 = typeof(factorial(f).x).arraySize; // 120\r\n\
\r\n\
return val1 + fact4;";
TEST_RESULT("Generic type compile-time 2", testGenericType42, "128");

const char *testGenericType43 =
"class Natural<T>\r\n\
{\r\n\
	T ref x;\r\n\
}\r\n\
auto operator+(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).target.arraySize + typeof(y.x).target.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator-(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).target.arraySize - typeof(y.x).target.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator*(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).target.arraySize * typeof(y.x).target.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator/(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).target.arraySize / typeof(y.x).target.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto factorial(Natural<@T> x)\r\n\
{\r\n\
	Natural<int[1]> one;\r\n\
	@if(typeof(x.x).target.arraySize > 1)\r\n\
		return x * factorial(x - one);\r\n\
	else\r\n\
		return one;\r\n\
}\r\n\
\r\n\
Natural<int[8]> a;\r\n\
Natural<int[3]> b;\r\n\
Natural<int[4]> c;\r\n\
auto val1 = typeof(*((a * b + a) / c).x).arraySize; // 8\r\n\
\r\n\
Natural<int[10]> f;\r\n\
auto fact4 = typeof(*factorial(f).x).arraySize; // 3628800\r\n\
\r\n\
return val1 + fact4;";
TEST_RESULT("Generic type compile-time 3", testGenericType43, "3628808");

const char *testGenericType44 =
"class Foo<T>{ T x; }\r\n\
auto foo(Foo<@T> x, Foo<@U> y)\r\n\
{\r\n\
	T a; U b;\r\n\
	return sizeof(a) + sizeof(b);\r\n\
}\r\n\
Foo<int> a; Foo<double> b;\r\n\
return foo(a, b);";
TEST_RESULT("Generic function specialization for generic types with aliases", testGenericType44, "12");

const char *testGenericType45 =
"class Foo<T>{ T x; }\r\n\
auto foo(Foo<@T> x, Foo<@U> y)\r\n\
{\r\n\
	T a; U b;\r\n\
	return sizeof(a) + sizeof(b);\r\n\
}\r\n\
auto bar(Foo<@T> x, Foo<@U> y)\r\n\
{\r\n\
	Foo<int> a; Foo<double> b;\r\n\
	return foo(a, b);\r\n\
}\r\n\
Foo<long> c; Foo<double> d;\r\n\
return bar(c, d);";
TEST_RESULT("Generic function specialization. aliases are taken from function prototype", testGenericType45, "12");

const char *testGenericType46 =
"class Natural<T>\r\n\
{\r\n\
	T ref x;\r\n\
}\r\n\
auto operator+(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[T.arraySize + U.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator-(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[T.arraySize - U.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator*(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[T.arraySize * U.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator/(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[T.arraySize / U.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto factorial(Natural<@T> x)\r\n\
{\r\n\
	Natural<int[1]> one;\r\n\
	@if(T.arraySize > 1)\r\n\
		return x * factorial(x - one);\r\n\
	else\r\n\
		return one;\r\n\
}\r\n\
\r\n\
Natural<int[8]> a;\r\n\
Natural<int[3]> b;\r\n\
Natural<int[4]> c;\r\n\
auto val1 = typeof((a * b + a) / c).x.target.arraySize; // 8\r\n\
\r\n\
Natural<int[10]> f;\r\n\
auto fact4 = typeof(factorial(f)).x.target.arraySize; // 3628800\r\n\
\r\n\
return val1 + fact4;";
TEST_RESULT("Generic type compile-time 4", testGenericType46, "3628808");

const char *testGenericType47 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	void Foo(int x = 4){ this.x = x; }\r\n\
}\r\n\
auto x = new Foo<int>(8);\r\n\
return x.x;";
TEST_RESULT("Constructor call after new with generic type instance", testGenericType47, "8");

const char *testGenericType48 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	void Foo(T x = 4.5){ this.x = x; }\r\n\
}\r\n\
auto x = new Foo<int>(8);\r\n\
auto y = new Foo<double>();\r\n\
return int(x.x * y.x);";
TEST_RESULT("Constructor call after new with generic type instance 2", testGenericType48, "36");

const char *testGenericType49 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	T y;\r\n\
}\r\n\
void foo(Foo<@T> ref x)\r\n\
{\r\n\
	x.x = 5;\r\n\
}\r\n\
Foo<int> a;\r\n\
foo(a);\r\n\
return a.x;";
TEST_RESULT("Generic function is specialized for reference type but a non-reference is passed", testGenericType49, "5");

const char *testGenericType50 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
}\r\n\
void Foo:Foo(){ x = 4; }\r\n\
Foo<int> a;\r\n\
return a.x;";
TEST_RESULT("automatic constructor call for generic class instance on stack", testGenericType50, "4");

const char *testGenericType51 =
"class Foo<T>{ T x; }\r\n\
auto Foo:bar(generic z){ return x - z; }\r\n\
Foo<int> h;\r\n\
h.x = 3;\r\n\
h.bar(3);\r\n\
auto m = h.bar(4.0);\r\n\
return typeof(m) == double;";
TEST_RESULT("generic type generic member function is instanced for different, but close types", testGenericType51, "1");

const char *testGenericType52 =
"class Foo<T>{ T x; }\r\n\
Foo<int> h;\r\n\
h.x = 3;\r\n\
int Foo:bar(){ return 5; }\r\n\
h.bar();\r\n\
int Foo:bar(int z){ return x - z; }\r\n\
return h.bar(2);";
TEST_RESULT("generic type member function overload defined after first overload instantion is instanced", testGenericType52, "1");

const char *testGenericType53 =
"class Foo<T>{ T x; }\r\n\
int Foo:bar(){ return 5; }\r\n\
int Foo:bar(int z){ return x - z; }\r\n\
Foo<int> h;\r\n\
h.x = 3;\r\n\
return h.bar() + h.bar(1);";
TEST_RESULT("generic type member function overloads are instanced", testGenericType53, "7");

const char *testGenericType54 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	T y{ get{ return -x; } set(t){ x = -t; } };\r\n\
}\r\n\
Foo<int> g;\r\n\
g.y = 5;\r\n\
return g.y + g.x;";
TEST_RESULT("generic type accessor is instanced", testGenericType54, "0");

const char *testGenericType55 =
"class Foo<T>{ T x; }\r\n\
auto Foo.y(){ return -x; }\r\n\
auto Foo.y(T t){ x = -t; }\r\n\
Foo<int> g;\r\n\
\r\n\
g.y = 5;\r\n\
return g.y + g.x;";
TEST_RESULT("generic type accessor is instanced 2", testGenericType55, "0");

const char *testGenericType56 =
"class Foo{ typedef int T; T x; }\r\n\
auto Foo:bar(T z, generic y){ return x - z + y; }\r\n\
Foo h; h.x = 3;\r\n\
return h.bar(5, 3);";
TEST_RESULT("class typedef in a generic member function", testGenericType56, "1");

const char *testGenericType57 =
"class Foo{ typedef int T; T x; }\r\n\
auto Foo:bar(generic z, T y){ return x - z + y; }\r\n\
Foo h; h.x = 3;\r\n\
return h.bar(5, 3);";
TEST_RESULT("class typedef in a generic member function 2", testGenericType57, "1");

const char *testGenericType58 =
"class Foo<T>\r\n\
{\r\n\
	T y;\r\n\
	int boo(int x){ return x * y; }\r\n\
\r\n\
	void Foo(T n){ y = n; }\r\n\
}\r\n\
auto ref[2] arr;\r\n\
arr[0] = new Foo<int>(3);\r\n\
arr[1] = new Foo<double>(1.5);\r\n\
\r\n\
int sum = 0;\r\n\
for(i in arr)\r\n\
sum += i.boo(4);\r\n\
return sum;";
TEST_RESULT("generic type member function call through 'auto ref'", testGenericType58, "18");

const char *testGenericType58b =
"class Foo<T>\r\n\
{\r\n\
	T y;\r\n\
}\r\n\
int Foo:boo(int x){ return x * y; }\r\n\
void Foo:Foo(T n){ y = n; }\r\n\
auto ref[2] arr;\r\n\
arr[0] = new Foo<int>(3);\r\n\
arr[1] = new Foo<double>(1.5);\r\n\
\r\n\
int sum = 0;\r\n\
for(i in arr)\r\n\
sum += i.boo(4);\r\n\
return sum;";
TEST_RESULT("generic type member function call through 'auto ref' 2", testGenericType58b, "18");

const char *testGenericType59 =
"class Foo<T>\r\n\
{\r\n\
	T y;\r\n\
	int boo(int x){ return x * y; }\r\n\
\r\n\
	void Foo(T n){ y = n; }\r\n\
}\r\n\
auto x = new Foo<int>(3);\r\n\
auto x2 = new Foo<int>(6);\r\n\
auto y = new Foo<double>(1.5);\r\n\
auto y2 = new Foo<double>(3.5);\r\n\
\r\n\
auto xM = x.boo;\r\n\
auto yM = y.boo;\r\n\
auto x2M = x2.boo;\r\n\
auto y2M = y2.boo;\r\n\
\r\n\
return xM(1) + x2M(1) + yM(2) + y2M(4);";
TEST_RESULT("generic type member function pointers", testGenericType59, "26");

const char *testGenericType60 =
"class Foo<T>\r\n\
{\r\n\
	T y;\r\n\
	int boo(int x){ return x * y; }\r\n\
	int boo(float x){ return x * y; }\r\n\
\r\n\
	void Foo(T n){ y = n; }\r\n\
}\r\n\
auto x = new Foo<int>(3);\r\n\
auto x2 = new Foo<int>(6);\r\n\
auto y = new Foo<double>(1.5);\r\n\
auto y2 = new Foo<double>(3.5);\r\n\
\r\n\
int ref(int) xM = x.boo;\r\n\
int ref(int) yM = y.boo;\r\n\
int ref(float) x2M = x2.boo;\r\n\
int ref(float) y2M = y2.boo;\r\n\
\r\n\
return xM(1) + x2M(1.5) + yM(2) + y2M(4.7);";
TEST_RESULT("generic type member function pointers (overloads available)", testGenericType60, "31");

const char *testGenericType61 =
"class Foo<T>\r\n\
{\r\n\
	int y;\r\n\
}\r\n\
auto x = new Foo<int>;\r\n\
x.y = 6;\r\n\
auto y = new Foo<double>[10];\r\n\
return x.y + y.size;";
TEST_RESULT("generic type allocation without constructor call", testGenericType61, "16");

const char *testGenericType62 =
"class Foo<T>\r\n\
{\r\n\
	int boo(auto ref[] x){ return x.size; }\r\n\
}\r\n\
auto ref x = new Foo<int>;\r\n\
return x.boo(1, 2, 3) + x.boo(1, 2);";
TEST_RESULT("Function call through 'auto ref' for generic type, selection of a variable argument function", testGenericType62, "5");

const char *testGenericType62b =
"class Foo<T>\r\n\
{\r\n\
}\r\n\
int Foo:boo(auto ref[] x){ return x.size; }\r\n\
auto ref x = new Foo<int>;\r\n\
return x.boo(1, 2, 3) + x.boo(1, 2);";
TEST_RESULT("Function call through 'auto ref' for generic type, selection of a variable argument function 2", testGenericType62b, "5");

//////////////////////////////////////////////////////////////////////////
LOAD_MODULE(test_generic_type63, "test.generic_type63",
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U ref y;\r\n\
}");
const char *testGenericType63 =
"import test.generic_type63;\r\n\
Foo<int, int> a;\r\n\
a.y = &a.x;\r\n\
a.x = 5;\r\n\
return *a.y;";
TEST_RESULT("Generic type test (POD)", testGenericType63, "5");

LOAD_MODULE(test_generic_type64, "test.generic_type64",
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U y;\r\n\
}");
const char *testGenericType64 =
"import test.generic_type64;\r\n\
class Bar\r\n\
{\r\n\
	int bar(){ Foo<int, float> z; z.x = 6; z.y = 8.5; return z.x * z.y; }\r\n\
}\r\n\
Bar v;\r\n\
return v.bar();";
TEST_RESULT("Generic type test (POD instance inside another type)", testGenericType64, "51");

LOAD_MODULE(test_generic_type65, "test.generic_type65",
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U ref y;\r\n\
}");
const char *testGenericType65 =
"import test.generic_type65;\r\n\
Foo<int, int> a;\r\n\
Foo<int, float> b;\r\n\
a.y = &a.x;\r\n\
a.x = 5;\r\n\
b.y = new float(5);\r\n\
return int(a.x + *b.y);";
TEST_RESULT("Generic type test (POD with multiple instances)", testGenericType65, "10");

LOAD_MODULE(test_generic_type66, "test.generic_type66",
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U ref y;\r\n\
}");
const char *testGenericType66 =
"import test.generic_type66;\r\n\
Foo<int, int> a;\r\n\
Foo<int, float> b;\r\n\
Foo<int, int> c;\r\n\
a.y = &a.x;\r\n\
a.x = 1;\r\n\
b.y = new float(5);\r\n\
c.y = new int(40);\r\n\
return int(a.x + *b.y + *c.y);";
TEST_RESULT("Generic type test (POD with multiple instances of the same type)", testGenericType66, "46");

LOAD_MODULE(test_generic_type67, "test.generic_type67",
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U ref y;\r\n\
	auto foo(){ return -x - *y; }\r\n\
}");
const char *testGenericType67 =
"import test.generic_type67;\r\n\
Foo<int, int> a;\r\n\
Foo<int, float> b;\r\n\
Foo<int, int> c;\r\n\
a.y = &a.x;\r\n\
a.x = 1;\r\n\
b.y = new float(5);\r\n\
c.y = new int(40);\r\n\
return int(a.foo() + b.foo() + c.foo());";
TEST_RESULT("Generic type test (with function (internal definition, global instance))", testGenericType67, "-47");

LOAD_MODULE(test_generic_type68, "test.generic_type68",
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U y;\r\n\
	int foo(){ return -x; }\r\n\
}");
const char *testGenericType68 =
"import test.generic_type68;\r\n\
class Bar\r\n\
{\r\n\
	int bar(){ Foo<int, float> z; z.x = 6; z.y = 8.5; return -z.foo() * z.y; }\r\n\
}\r\n\
Bar v;\r\n\
return v.bar();";
TEST_RESULT("Generic type test (with function (internal definition, local instance))", testGenericType68, "51");

LOAD_MODULE(test_generic_type69, "test.generic_type69",
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U y;\r\n\
	int foo(){ return -x; }\r\n\
}");
const char *testGenericType69 =
"import test.generic_type69;\r\n\
class Bar\r\n\
{\r\n\
	int bar(){ Foo<int, float> z; z.x = 6; z.y = 8.5; return -z.foo() * z.y; }\r\n\
}\r\n\
Bar v;\r\n\
Foo<int, float> u;\r\n\
u.x = 6;\r\n\
return v.bar() + u.foo();";
TEST_RESULT("Generic type test (with function (internal definition, local instance)) 2", testGenericType69, "45");

LOAD_MODULE(test_generic_type70, "test.generic_type70",
"class Foo<T, U>\r\n\
{\r\n\
	T x;\r\n\
	U y;\r\n\
	int foo(){ return -x; }\r\n\
}");
const char *testGenericType70 =
"import test.generic_type70;\r\n\
int goo()\r\n\
{\r\n\
	int bar()\r\n\
	{\r\n\
		Foo<int, float> z;\r\n\
		z.x = 6;\r\n\
		z.y = 8.5;\r\n\
		return -z.foo() * z.y;\r\n\
	}\r\n\
	return bar();\r\n\
}\r\n\
Foo<int, float> u;\r\n\
u.x = 6;\r\n\
return goo() + u.foo();";
TEST_RESULT("Generic type test (with function (internal definition, local instance)) 3", testGenericType70, "45");

LOAD_MODULE(test_generic_type71, "test.generic_type71",
"class Stack<T>\r\n\
{\r\n\
	T[] arr;\r\n\
	int size, reserved;\r\n\
\r\n\
	void grow()\r\n\
	{\r\n\
		int nReserved = reserved + (reserved >> 1) + 1;\r\n\
		T[] nArr = new T[nReserved];\r\n\
		for(i in arr, j in nArr)\r\n\
			j = i;\r\n\
		arr = nArr;\r\n\
		reserved = nReserved;\r\n\
	}\r\n\
	void push_back(T x)\r\n\
	{\r\n\
		if(size == reserved)\r\n\
			grow();\r\n\
		arr[size++] = x;\r\n\
	}\r\n\
	T pop_back()\r\n\
	{\r\n\
		size--;\r\n\
		return arr[size];\r\n\
	}\r\n\
	T ref back()\r\n\
	{\r\n\
	return &arr[size-1];\r\n\
	}\r\n\
}");
const char *testGenericType71 =
"import test.generic_type71;\r\n\
Stack<int> s;\r\n\
s.push_back(3);\r\n\
s.push_back(4.0);\r\n\
s.push_back(5);\r\n\
return s.pop_back() + s.pop_back() + (s.back() = 8);";
TEST_RESULT("Generic type test Stack type 2", testGenericType71, "17");

LOAD_MODULE(test_generic_type72, "test.generic_type72",
"class Stack<T>\r\n\
{\r\n\
	T[] arr;\r\n\
	int size, reserved;\r\n\
\r\n\
	void grow()\r\n\
	{\r\n\
		int nReserved = reserved + (reserved >> 1) + 1;\r\n\
		T[] nArr = new T[nReserved];\r\n\
		for(i in arr, j in nArr)\r\n\
			j = i;\r\n\
		arr = nArr;\r\n\
		reserved = nReserved;\r\n\
	}\r\n\
	void push_back(T x)\r\n\
	{\r\n\
		if(size == reserved)\r\n\
			grow();\r\n\
		arr[size++] = x;\r\n\
	}\r\n\
	T pop_back()\r\n\
	{\r\n\
		size--;\r\n\
		return arr[size];\r\n\
	}\r\n\
	T ref back()\r\n\
	{\r\n\
		return &arr[size-1];\r\n\
	}\r\n\
	T ref front()\r\n\
	{\r\n\
		return &arr[0];\r\n\
	}\r\n\
}");
const char *testGenericType72 =
"import test.generic_type72;\r\n\
Stack<Stack<float> ref> s;\r\n\
Stack<float> s2; Stack<float> s3;\r\n\
s.push_back(s2); s.push_back(s3);\r\n\
s2.push_back(4); s3.push_back(8);\r\n\
return int(*s.front().back() * *s.back().back());";
TEST_RESULT("Generic type test Stack type 2", testGenericType72, "32");

LOAD_MODULE(test_generic_type73, "test.generic_type73",
"class Foo<T>{ T x; }");
const char *testGenericType73 =
"import test.generic_type73;\r\n\
int Foo<int>:foo(){ return -x; }\r\n\
Foo<int> x;\r\n\
x.x = 10;\r\n\
return x.foo();";
TEST_RESULT("Generic type specialized external function definition", testGenericType73, "-10");

const char *testGenericType74 =
"import test.generic_type73;\r\n\
int Foo:foo(){ @if(typeof(this.x) == int) return x; else return -x; }\r\n\
Foo<int> x;\r\n\
Foo<float> z;\r\n\
x.x = 10;\r\n\
z.x = 5;\r\n\
return x.foo() + z.foo();";
TEST_RESULT("Generic type external member function definition", testGenericType74, "5");

const char *testGenericType75 =
"import test.generic_type73;\r\n\
int Foo:foo(){ return -x; }\r\n\
int Foo<int>:foo(){ return 2*x; }\r\n\
\r\n\
Foo<int> x;\r\n\
\r\n\
int Foo:bar(){ return 9; }\r\n\
\r\n\
Foo<float> z;\r\n\
x.x = 10;\r\n\
z.x = 5;\r\n\
return x.foo() + z.foo() + x.bar();";
TEST_RESULT("Generic type external member function definition after instancing", testGenericType75, "24");

const char *testGenericType76 =
"import test.generic_type73;\r\n\
int Foo:foo(){ return -x; }\r\n\
int Foo<int>:foo(){ return 2*x; }\r\n\
\r\n\
Foo<int> x;\r\n\
\r\n\
int Foo:bar(){ return 9; }\r\n\
\r\n\
Foo<float> z;\r\n\
x.x = 10;\r\n\
z.x = 5;\r\n\
int test()\r\n\
{\r\n\
	return x.foo() + z.foo() + x.bar();\r\n\
}\r\n\
return test();";
TEST_RESULT("Generic type external member function instancing in an incorrect scope", testGenericType76, "24");

LOAD_MODULE(test_generic_type77, "test.generic_type77",
"class Foo<T, U>{ T x; U y; }");
const char *testGenericType77 =
"import test.generic_type77;\r\n\
class Bar{ Foo<int, float> a; Foo<float, int> b; }\r\n\
Bar z;\r\n\
return typeof(z.a.x) == int && typeof(z.a.y) == float && typeof(z.b.x) == float && typeof(z.b.y) == int;";
TEST_RESULT("Generic type instance inside another type", testGenericType77, "1");

LOAD_MODULE(test_generic_type78, "test.generic_type78",
"class Foo<T, U>{ T x; U y; }\r\n\
class Bar<T, U>{ Foo<T, U> a; Foo<U, T> b; }");
const char *testGenericType78 =
"import test.generic_type78;\r\n\
Bar<int, float> z;\r\n\
return typeof(z.a.x) == int && typeof(z.a.y) == float && typeof(z.b.x) == float && typeof(z.b.y) == int;";
TEST_RESULT("Generic type instance inside another generic type", testGenericType78, "1");

//////////////////////////////////////////////////////////////////////////
const char *testGenericType79 =
"import test.generic_type73;\r\n\
\r\n\
auto foo(Foo<generic> a){ return -a.x; }\r\n\
Foo<int> b; Foo<float> c;\r\n\
b.x = 6; c.x = 2;\r\n\
return int(foo(b) + foo(c));";
TEST_RESULT("Function that accepts generic type 1", testGenericType79, "-8");

const char *testGenericType80 =
"import test.generic_type73;\r\n\
\r\n\
auto foo(int x, Foo<generic> a){ return x - a.x; }\r\n\
Foo<int> b; Foo<float> c;\r\n\
b.x = 6; c.x = 2;\r\n\
return int(foo(1, b) + foo(1, c));";
TEST_RESULT("Function that accepts generic type 2", testGenericType80, "-6");

const char *testGenericType81 =
"import test.generic_type73;\r\n\
\r\n\
auto Foo:foo(){ T y = 7; return x + y; }\r\n\
Foo<int> a; a.x = 4;\r\n\
return a.foo();";
TEST_RESULT("Generic type external member function aliases restore", testGenericType81, "11");

const char *testGenericType82 =
"import test.generic_type77;\r\n\
\r\n\
auto foo(Foo<int, generic> a){ return a.x * a.y; }\r\n\
Foo<int, int> b;\r\n\
Foo<int, float> c;\r\n\
b.x = 6; b.y = 1; c.x = 2; c.y = 1.5;\r\n\
return int(foo(b) + foo(c));";
TEST_RESULT("Generic function complex specialization for generic type", testGenericType82, "9");

const char *testGenericType83 =
"import test.generic_type77;\r\n\
\r\n\
auto foo(Foo<generic, generic> a){ return a.x * a.y; }\r\n\
Foo<int, int> b;\r\n\
Foo<float, float> c;\r\n\
b.x = 6; b.y = 1; c.x = 2; c.y = 1.5;\r\n\
return int(foo(b) + foo(c));";
TEST_RESULT("Generic function complex specialization for generic type 2", testGenericType83, "9");

const char *testGenericType84 =
"import test.generic_type77;\r\n\
\r\n\
auto foo(int y, Foo<generic, generic> a){ return y+a.x * a.y; }\r\n\
Foo<int, int> b;\r\n\
Foo<float, float> c;\r\n\
b.x = 6; b.y = 1; c.x = 2; c.y = 1.5;\r\n\
return int(foo(1,b) + foo(2,c));";
TEST_RESULT("Generic function complex specialization for generic type 3", testGenericType84, "12");

const char *testGenericType85 =
"import test.generic_type73;\r\n\
\r\n\
auto operator +(Foo<generic> ref x, y){ return x.x + y.x; }\r\n\
Foo<int> b; Foo<float> c;\r\n\
b.x = 6; c.x = 2;\r\n\
return int(b + c);";
TEST_RESULT("Generic function complex specialization (fail possibility 1)", testGenericType85, "8");

LOAD_MODULE(test_generic_type86, "test.generic_type86",
"class complex<T>{ T re, im; }");
const char *testGenericType86 =
"import test.generic_type86;\r\n\
\r\n\
\r\n\
double complex(double re, im){ return 1; }\r\n\
auto operator * (complex<generic> a, b){ return 1; }\r\n\
\r\n\
int foo(complex<double> a, b)\r\n\
{\r\n\
	return complex(typeof(a.re)(a.re * b.re), 1.0);\r\n\
}\r\n\
complex<double> a, b;\r\n\
return foo(a, b);";
TEST_RESULT("Compiler current type change during constructor call", testGenericType86, "1");

const char *testGenericType87 =
"import test.generic_type73;\r\n\
\r\n\
auto foo(Foo<@T> x)\r\n\
{\r\n\
	T y = x.x;\r\n\
	return y + x.x;\r\n\
}\r\n\
Foo<int> a;\r\n\
a.x = 5;\r\n\
return foo(a);";
TEST_RESULT("Generic function specialization for generic type with alias", testGenericType87, "10");

LOAD_MODULE(test_generic_type88, "test.generic_type88",
"class Foo<T>{ T t; }");
const char *testGenericType88 =
"import test.generic_type88;\r\n\
\r\n\
Foo<Foo<int>> b; b.t.t = 5;\r\n\
auto foo(Foo<Foo<int>> z){ return z.t.t; }\r\n\
return foo(b);";
TEST_RESULT("Nested generic type definition '>>' resolve 1", testGenericType88, "5");

LOAD_MODULE(test_generic_type89, "test.generic_type89",
"class Foo<T>{ T t; }\r\n\
class Bar<T, U>{ T t; U s; }");
const char *testGenericType89 =
"import test.generic_type89;\r\n\
\r\n\
auto bar(Bar<@T, Foo<int>> z){ return z.s.t; }\r\n\
Bar<int, Foo<int>> b; b.s.t = 5;\r\n\
return bar(b);";
TEST_RESULT("Nested generic type definition '>>' resolve 2", testGenericType89, "5");

const char *testGenericType90 =
"import test.generic_type73;\r\n\
\r\n\
Foo<int> a; a.x = 5;\r\n\
auto foo(Foo<generic> ref m){ return -m.x; }\r\n\
return foo(&a);";
TEST_RESULT("Generic type specialization to reference type", testGenericType90, "-5");

const char *testGenericType91 =
"import test.generic_type73;\r\n\
\r\n\
Foo<int> a; a.x = 5;\r\n\
auto foo(Foo<generic> ref m){ return -m.x; }\r\n\
return foo(a);";
TEST_RESULT("Generic type specialization to reference type 2", testGenericType91, "-5");

LOAD_MODULE(test_generic_type92, "test.generic_type92",
"class Foo<T>{ T a; }");
const char *testGenericType92 =
"import test.generic_type92;\r\n\
\r\n\
auto Foo:foo(){ return 2*a; }\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo;\r\n\
auto z = s.foo;\r\n\
return int(y() + z());";
TEST_RESULT("Taking pointer to a generic type member function", testGenericType92, "88");

LOAD_MODULE(test_generic_type93, "test.generic_type93",
"class Foo<T>{ T a; }");
const char *testGenericType93 =
"import test.generic_type93;\r\n\
\r\n\
auto Foo:foo(){ return 2*a; }\r\n\
int Foo<double>:foo(){ return -a; }\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo;\r\n\
auto z = s.foo;\r\n\
return int(y() + z());";
TEST_RESULT("Taking pointer to a generic type member function (with a specialized option)", testGenericType93, "-32");

LOAD_MODULE(test_generic_type94, "test.generic_type94",
"class Foo<T>\r\n\
{\r\n\
	T a;\r\n\
	auto foo(){ return -a; }\r\n\
}");
const char *testGenericType94 =
"import test.generic_type94;\r\n\
\r\n\
int Foo<double>:foo(){ return 2*a; }\r\n\
\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo;\r\n\
int ref() z = s.foo;\r\n\
return int(y() + z());";
TEST_RESULT("Taking pointer to a generic type member function (overload resolve)", testGenericType94, "76");

LOAD_MODULE(test_generic_type95, "test.generic_type95",
"class Foo<T>\r\n\
{\r\n\
	T a;\r\n\
\r\n\
	auto bar(int x){ return -x; }\r\n\
	auto bar(float x){ return 2*x; }\r\n\
}");
const char *testGenericType95 =
"import test.generic_type95;\r\n\
\r\n\
int Foo:foo()\r\n\
{\r\n\
	int ref(int) m = bar;\r\n\
	return m(a);\r\n\
}\r\n\
\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo;\r\n\
auto z = s.foo;\r\n\
return int(y() + z());";
TEST_RESULT("Function call by pointer in a member function", testGenericType95, "-44");

const char *testGenericType96 =
"import test.generic_type73;\r\n\
\r\n\
void Foo:set(T x){ assert(typeof(x) == double); this.x = x; }\r\n\
Foo<double> m;\r\n\
m.set(4);\r\n\
return int(m.x * 1.5);";
TEST_RESULT("Generic type aliases are available in external unspecialized member function argument list", testGenericType96, "6");

LOAD_MODULE(test_generic_type97, "test.generic_type97",
"class Factorial<T>{}");
const char *testGenericType97 =
"import test.generic_type97;\r\n\
\r\n\
auto Factorial<int[1] ref>:get(){ T arr; return arr; }\r\n\
auto Factorial:get()\r\n\
{\r\n\
	Factorial<int[typeof(T).target.arraySize - 1] ref> x;\r\n\
	int[typeof(x.get()).target.arraySize] ref m; // arraysize > 1 ? arraysize - 1 : 1;\r\n\
	typeof(T).target.target[typeof(T).target.arraySize * typeof(m).target.arraySize] ref arr;\r\n\
	return arr;\r\n\
}\r\n\
Factorial<int[8] ref> fact;\r\n\
return typeof(fact.get()).target.arraySize;";
TEST_RESULT("Generic type compile-time factorial", testGenericType97, "40320");

LOAD_MODULE(test_generic_type98, "test.generic_type98",
"class Natural<T>\r\n\
{\r\n\
	T x;\r\n\
}");
const char *testGenericType98 =
"import test.generic_type98;\r\n\
\r\n\
auto operator+(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).arraySize + typeof(y.x).arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator-(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).arraySize - typeof(y.x).arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator*(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).arraySize * typeof(y.x).arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator/(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).arraySize / typeof(y.x).arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto factorial(Natural<@T> x)\r\n\
{\r\n\
	Natural<int[1]> one;\r\n\
	@if(typeof(x.x).arraySize > 1)\r\n\
		return x * factorial(x - one);\r\n\
	else\r\n\
		return one;\r\n\
}\r\n\
\r\n\
Natural<int[8]> a;\r\n\
Natural<int[3]> b;\r\n\
Natural<int[4]> c;\r\n\
auto val1 = typeof(((a * b + a) / c).x).arraySize; // 8\r\n\
\r\n\
Natural<int[5]> f;\r\n\
auto fact4 = typeof(factorial(f).x).arraySize; // 120\r\n\
\r\n\
return val1 + fact4;";
TEST_RESULT("Generic type compile-time 2", testGenericType98, "128");

LOAD_MODULE(test_generic_type99, "test.generic_type99",
"class Natural<T>\r\n\
{\r\n\
	T ref x;\r\n\
}");
const char *testGenericType99 =
"import test.generic_type99;\r\n\
\r\n\
auto operator+(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).target.arraySize + typeof(y.x).target.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator-(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).target.arraySize - typeof(y.x).target.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator*(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).target.arraySize * typeof(y.x).target.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator/(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[typeof(x.x).target.arraySize / typeof(y.x).target.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto factorial(Natural<@T> x)\r\n\
{\r\n\
	Natural<int[1]> one;\r\n\
	@if(typeof(x.x).target.arraySize > 1)\r\n\
		return x * factorial(x - one);\r\n\
	else\r\n\
		return one;\r\n\
}\r\n\
\r\n\
Natural<int[8]> a;\r\n\
Natural<int[3]> b;\r\n\
Natural<int[4]> c;\r\n\
auto val1 = typeof(*((a * b + a) / c).x).arraySize; // 8\r\n\
\r\n\
Natural<int[10]> f;\r\n\
auto fact4 = typeof(*factorial(f).x).arraySize; // 3628800\r\n\
\r\n\
return val1 + fact4;";
TEST_RESULT("Generic type compile-time 3", testGenericType99, "3628808");

const char *testGenericType100 =
"import test.generic_type73;\r\n\
\r\n\
auto foo(Foo<@T> x, Foo<@U> y)\r\n\
{\r\n\
	T a; U b;\r\n\
	return sizeof(a) + sizeof(b);\r\n\
}\r\n\
Foo<int> a; Foo<double> b;\r\n\
return foo(a, b);";
TEST_RESULT("Generic function specialization for generic types with aliases", testGenericType100, "12");

const char *testGenericType101 =
"import test.generic_type73;\r\n\
\r\n\
auto foo(Foo<@T> x, Foo<@U> y)\r\n\
{\r\n\
	T a; U b;\r\n\
	return sizeof(a) + sizeof(b);\r\n\
}\r\n\
auto bar(Foo<@T> x, Foo<@U> y)\r\n\
{\r\n\
	Foo<int> a; Foo<double> b;\r\n\
	return foo(a, b);\r\n\
}\r\n\
Foo<long> c; Foo<double> d;\r\n\
return bar(c, d);";
TEST_RESULT("Generic function specialization. aliases are taken from function prototype", testGenericType101, "12");

LOAD_MODULE(test_generic_type102, "test.generic_type102",
"class Natural<T>\r\n\
{\r\n\
	T ref x;\r\n\
}");
const char *testGenericType102 =
"import test.generic_type102;\r\n\
\r\n\
auto operator+(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[T.arraySize + U.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator-(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[T.arraySize - U.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator*(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[T.arraySize * U.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto operator/(Natural<@T> x, Natural<@U> y)\r\n\
{\r\n\
	int[T.arraySize / U.arraySize] value;\r\n\
	Natural<typeof(value)> result;\r\n\
	return result;\r\n\
}\r\n\
auto factorial(Natural<@T> x)\r\n\
{\r\n\
	Natural<int[1]> one;\r\n\
	@if(T.arraySize > 1)\r\n\
		return x * factorial(x - one);\r\n\
	else\r\n\
		return one;\r\n\
}\r\n\
\r\n\
Natural<int[8]> a;\r\n\
Natural<int[3]> b;\r\n\
Natural<int[4]> c;\r\n\
auto val1 = typeof((a * b + a) / c).x.target.arraySize; // 8\r\n\
\r\n\
Natural<int[10]> f;\r\n\
auto fact4 = typeof(factorial(f)).x.target.arraySize; // 3628800\r\n\
\r\n\
return val1 + fact4;";
TEST_RESULT("Generic type compile-time 4", testGenericType102, "3628808");

LOAD_MODULE(test_generic_type103, "test.generic_type103",
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	void Foo(int x = 4){ this.x = x; }\r\n\
}");
const char *testGenericType103 =
"import test.generic_type103;\r\n\
\r\n\
auto x = new Foo<int>(8);\r\n\
return x.x;";
TEST_RESULT("Constructor call after new with generic type instance", testGenericType103, "8");

LOAD_MODULE(test_generic_type104, "test.generic_type104",
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	void Foo(T x = 4.5){ this.x = x; }\r\n\
}");
const char *testGenericType104 =
"import test.generic_type104;\r\n\
\r\n\
auto x = new Foo<int>(8);\r\n\
auto y = new Foo<double>();\r\n\
return int(x.x * y.x);";
TEST_RESULT("Constructor call after new with generic type instance 2", testGenericType104, "36");

LOAD_MODULE(test_generic_type105, "test.generic_type105",
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	T y;\r\n\
}");
const char *testGenericType105 =
"import test.generic_type105;\r\n\
\r\n\
void foo(Foo<@T> ref x)\r\n\
{\r\n\
	x.x = 5;\r\n\
}\r\n\
Foo<int> a;\r\n\
foo(a);\r\n\
return a.x;";
TEST_RESULT("Generic function is specialized for reference type but a non-reference is passed", testGenericType105, "5");

const char *testGenericType106 =
"import test.generic_type73;\r\n\
\r\n\
void Foo:Foo(){ x = 4; }\r\n\
Foo<int> a;\r\n\
return a.x;";
TEST_RESULT("automatic constructor call for generic class instance on stack", testGenericType106, "4");

const char *testGenericType107 =
"import test.generic_type73;\r\n\
\r\n\
auto Foo:bar(generic z){ return x - z; }\r\n\
Foo<int> h;\r\n\
h.x = 3;\r\n\
h.bar(3);\r\n\
auto m = h.bar(4.0);\r\n\
return typeof(m) == double;";
TEST_RESULT("generic type generic member function is instanced for different, but close types", testGenericType107, "1");

const char *testGenericType108 =
"import test.generic_type73;\r\n\
\r\n\
Foo<int> h;\r\n\
h.x = 3;\r\n\
int Foo:bar(){ return 5; }\r\n\
h.bar();\r\n\
int Foo:bar(int z){ return x - z; }\r\n\
return h.bar(2);";
TEST_RESULT("generic type member function overload defined after first overload instantion is instanced", testGenericType108, "1");

const char *testGenericType109 =
"import test.generic_type73;\r\n\
\r\n\
int Foo:bar(){ return 5; }\r\n\
int Foo:bar(int z){ return x - z; }\r\n\
Foo<int> h;\r\n\
h.x = 3;\r\n\
return h.bar() + h.bar(1);";
TEST_RESULT("generic type member function overloads are instanced", testGenericType109, "7");

LOAD_MODULE(test_generic_type110, "test.generic_type110",
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	T y{ get{ return -x; } set(t){ x = -t; } };\r\n\
}");
const char *testGenericType110 =
"import test.generic_type110;\r\n\
\r\n\
Foo<int> g;\r\n\
g.y = 5;\r\n\
return g.y + g.x;";
TEST_RESULT("generic type accessor is instanced", testGenericType110, "0");

const char *testGenericType111 =
"import test.generic_type73;\r\n\
\r\n\
auto Foo.y(){ return -x; }\r\n\
auto Foo.y(T t){ x = -t; }\r\n\
Foo<int> g;\r\n\
\r\n\
g.y = 5;\r\n\
return g.y + g.x;";
TEST_RESULT("generic type accessor is instanced 2", testGenericType111, "0");

LOAD_MODULE(test_generic_type114, "test.generic_type114",
"class Foo<T>\r\n\
{\r\n\
	T y;\r\n\
	int boo(int x){ return x * y; }\r\n\
\r\n\
	void Foo(T n){ y = n; }\r\n\
}");
const char *testGenericType114 =
"import test.generic_type114;\r\n\
\r\n\
auto ref[2] arr;\r\n\
arr[0] = new Foo<int>(3);\r\n\
arr[1] = new Foo<double>(1.5);\r\n\
\r\n\
int sum = 0;\r\n\
for(i in arr)\r\n\
sum += i.boo(4);\r\n\
return sum;";
TEST_RESULT("generic type member function call through 'auto ref'", testGenericType114, "18");

LOAD_MODULE(test_generic_type114b, "test.generic_type114b",
"class Foo<T>{ T y; }\r\n\
int Foo:boo(int x){ return x * y; }\r\n\
void Foo:Foo(T n){ y = n; }");
const char *testGenericType114b =
"import test.generic_type114b;\r\n\
\r\n\
auto ref[2] arr;\r\n\
arr[0] = new Foo<int>(3);\r\n\
arr[1] = new Foo<double>(1.5);\r\n\
\r\n\
int sum = 0;\r\n\
for(i in arr)\r\n\
sum += i.boo(4);\r\n\
return sum;";
TEST_RESULT("generic type member function call through 'auto ref' 2", testGenericType114b, "18");

LOAD_MODULE(test_generic_type115, "test.generic_type115",
"class Foo<T>\r\n\
{\r\n\
	T y;\r\n\
	int boo(int x){ return x * y; }\r\n\
\r\n\
	void Foo(T n){ y = n; }\r\n\
}");
const char *testGenericType115 =
"import test.generic_type115;\r\n\
\r\n\
auto x = new Foo<int>(3);\r\n\
auto x2 = new Foo<int>(6);\r\n\
auto y = new Foo<double>(1.5);\r\n\
auto y2 = new Foo<double>(3.5);\r\n\
\r\n\
auto xM = x.boo;\r\n\
auto yM = y.boo;\r\n\
auto x2M = x2.boo;\r\n\
auto y2M = y2.boo;\r\n\
\r\n\
return xM(1) + x2M(1) + yM(2) + y2M(4);";
TEST_RESULT("generic type member function pointers", testGenericType115, "26");

LOAD_MODULE(test_generic_type116, "test.generic_type116",
"class Foo<T>\r\n\
{\r\n\
	T y;\r\n\
	int boo(int x){ return x * y; }\r\n\
	int boo(float x){ return x * y; }\r\n\
\r\n\
	void Foo(T n){ y = n; }\r\n\
}");
const char *testGenericType116 =
"import test.generic_type116;\r\n\
\r\n\
auto x = new Foo<int>(3);\r\n\
auto x2 = new Foo<int>(6);\r\n\
auto y = new Foo<double>(1.5);\r\n\
auto y2 = new Foo<double>(3.5);\r\n\
\r\n\
int ref(int) xM = x.boo;\r\n\
int ref(int) yM = y.boo;\r\n\
int ref(float) x2M = x2.boo;\r\n\
int ref(float) y2M = y2.boo;\r\n\
\r\n\
return xM(1) + x2M(1.5) + yM(2) + y2M(4.7);";
TEST_RESULT("generic type member function pointers (overloads available)", testGenericType116, "31");

LOAD_MODULE(test_generic_type117, "test.generic_type117",
"class Foo<T>{ int y; }");
const char *testGenericType117 =
"import test.generic_type117;\r\n\
\r\n\
auto x = new Foo<int>;\r\n\
x.y = 6;\r\n\
auto y = new Foo<double>[10];\r\n\
return x.y + y.size;";
TEST_RESULT("generic type allocation without constructor call", testGenericType117, "16");

LOAD_MODULE(test_generic_type118, "test.generic_type118",
"class Foo<T>\r\n\
{\r\n\
	int boo(auto ref[] x){ return x.size; }\r\n\
}");
const char *testGenericType118 =
"import test.generic_type118;\r\n\
\r\n\
auto ref x = new Foo<int>;\r\n\
return x.boo(1, 2, 3) + x.boo(1, 2);";
TEST_RESULT("Function call through 'auto ref' for generic type, selection of a variable argument function", testGenericType118, "5");

LOAD_MODULE(test_generic_type118b, "test.generic_type118b",
"class Foo<T>{}\r\n\
int Foo:boo(auto ref[] x){ return x.size; }");
const char *testGenericType118b =
"import test.generic_type118b;\r\n\
\r\n\
auto ref x = new Foo<int>;\r\n\
return x.boo(1, 2, 3) + x.boo(1, 2);";
TEST_RESULT("Function call through 'auto ref' for generic type, selection of a variable argument function 2", testGenericType118b, "5");

const char *testGenericType119 =
"class Foo<T>{ T x; }\r\n\
auto Foo:sum(generic ref(T) f){ return f(10); }\r\n\
auto Foo:average(generic ref(T) f){ return sum(f) / 2; }\r\n\
Foo<int> m;\r\n\
return m.average(<i>{ -i; });";
TEST_RESULT("generic member function instancing in a generic member function", testGenericType119, "-5");

const char *testGenericType120 =
"import test.generic_type73;\r\n\
auto Foo:sum(generic ref(T) f){ auto o = x; return f(10); }\r\n\
auto Foo:average(generic ref(T) f){ return sum(f) / x; }\r\n\
Foo<int> m; m.x = 2;\r\n\
return m.average(<i>{ -i; });";
TEST_RESULT("generic member function instancing in a generic member function (imported class)", testGenericType120, "-5");

LOAD_MODULE(test_generic_type121, "test.generic_type121",
"class Foo<T>{ T x; }\r\n\
auto Foo:sum(generic ref(T) f){ auto o = x; return f(10); }\r\n\
auto Foo:average(generic ref(T) f){ return sum(f) / x; }");
const char *testGenericType121 =
"import test.generic_type121;\r\n\
Foo<int> m; m.x = 2;\r\n\
return m.average(<i>{ -i; });";
TEST_RESULT("generic member function instancing in a generic member function (imported class and functions)", testGenericType121, "-5");

const char *testGenericType122 =
"class Foo<T>\r\n\
{\r\n\
	T i;\r\n\
}\r\n\
auto foo(generic a, generic ref(Foo<typeof(a)>) m)\r\n\
{\r\n\
	Foo<typeof(a)> x;\r\n\
	x.i = 5;\r\n\
	return a + m(x);\r\n\
}\r\n\
return foo(1, <x>{ -x.i; });";
TEST_RESULT("short inline function in argument with function with generic type depending on generic argument", testGenericType122, "-4");

const char *testGenericType123 =
"class Foo<T>\r\n\
{\r\n\
	T i;\r\n\
}\r\n\
auto foo(generic a, int ref(Foo<typeof(a)>) m)\r\n\
{\r\n\
	Foo<typeof(a)> x;\r\n\
	x.i = 5;\r\n\
	return a + m(x);\r\n\
}\r\n\
return foo(1, <x>{ x.i; });";
TEST_RESULT("short inline function in argument with function with generic type depending on generic argument 2", testGenericType123, "6");

const char *testGenericType124 =
"class Foo<T>{}\r\n\
int Foo:foo(T ref x){ return *x; }\r\n\
Foo<int> x;\r\n\
return x.foo(1) + x.foo(2);";
TEST_RESULT("when generic type member function is instanced, aliases are from instanced type", testGenericType124, "3");

const char *testGenericType125 =
"class Node<T>{}\r\n\
class Foo{}\r\n\
auto Node:first(int hash)\r\n\
{\r\n\
	Foo ref curr;\r\n\
	return coroutine auto generator()\r\n\
	{\r\n\
		while(curr) break;\r\n\
	};\r\n\
}\r\n\
Node<int> a;\r\n\
a.first(1);\r\n\
return 1;";
TEST_RESULT("local function is local", testGenericType125, "1");

const char *testGenericType126 =
"class vector<T>{}\r\n\
class foo<T>{ T i; }\r\n\
\r\n\
auto hash_value(vector<@T> v){ return sizeof(T); }\r\n\
\r\n\
vector<int> a;\r\n\
vector<foo<int>> b;\r\n\
vector<foo<double>> c;\r\n\
\r\n\
assert(hash_value(a) == 4);\r\n\
assert(hash_value(b) == 4);\r\n\
assert(hash_value(c) == 8);\r\n\
return 1;";
TEST_RESULT("Additional specialization test", testGenericType126, "1");

const char *testGenericType127 =
"class Foo<T>{ T x; }\r\n\
auto foo(Foo<@T> x){ return -x.x; }\r\n\
\r\n\
Foo<int> a; a.x = 4;\r\n\
Foo<double> b; b.x = 5.0;\r\n\
\r\n\
auto ax = foo(a);\r\n\
auto bx = foo(b);\r\n\
assert(ax == -4);\r\n\
assert(bx == -5.0);\r\n\
assert(typeof(ax) == int);\r\n\
assert(typeof(bx) == double);\r\n\
\r\n\
auto bar(Foo<Foo<@T>> x){ return -x.x.x; }\r\n\
\r\n\
Foo<Foo<int>> c; c.x.x = 3;\r\n\
\r\n\
auto cx = bar(c);\r\n\
assert(cx == -3);\r\n\
assert(typeof(cx) == int);\r\n\
\r\n\
return 1;";
TEST_RESULT("Additional specialization test 2 (nested generic types)", testGenericType127, "1");

const char *testGenericType128 =
"class Bar<T, U>{ T x; U y; }\r\n\
auto foo(Bar<generic, Bar<int, generic>> m){ return 1; }\r\n\
auto foo(Bar<int, int> n){ return 2; }\r\n\
\r\n\
Bar<int, int> a;\r\n\
return foo(a);";
TEST_RESULT("Additional specialization test 3 (nested generic types)", testGenericType128, "2");

const char *testGenericType129 =
"class Bar<T, U>{ T x; U y; }\r\n\
auto foo(Bar<int, generic> ref(int, int) m){ auto n = m(2, 3); return n.x + n.y; }\r\n\
\r\n\
Bar<int, int> bar(int a, b){ Bar<int, int> r; r.x = a; r.y = b; return r; }\r\n\
return foo(bar);";
TEST_RESULT("Additional specialization test 4 (specialization for a function with specialized generic type as a return type)", testGenericType129, "5");

const char *testGenericType130 =
"class Bar<T, U>{ T x; U y; }\r\n\
auto foo(Bar<int, generic> ref(int, int) m){ auto n = m(2, 3); return n.x + n.y; }\r\n\
auto foo(Bar<int, generic> ref(int, int)[] m){ return 10; }\r\n\
\r\n\
Bar<int, int> bar(int a, b){ Bar<int, int> r; r.x = a; r.y = b; return r; }\r\n\
typeof(bar)[4] ken;\r\n\
return foo(ken) + foo(bar);";
TEST_RESULT("Additional specialization test 5 (specialization for a function with specialized generic type as a return type)", testGenericType130, "15");

const char *testGenericType131 =
"class Bar<T, U>{ T x; U y; }\r\n\
auto foo(Bar<int, generic> ref(int, generic) m){ auto n = m(2, 3); return n.x + n.y; }\r\n\
auto foo(Bar<int, int> ref(int, int) m){ return 10; }\r\n\
\r\n\
Bar<int, int> bar(int a, b){ Bar<int, int> r; r.x = a; r.y = b; return r; }\r\n\
Bar<int, int> ken(int a, float b){ Bar<int, int> r; r.x = a; r.y = b; return r; }\r\n\
return foo(ken) * 10 + foo(bar);";
TEST_RESULT("Additional specialization test 6 (specialization for a function with specialized generic type as a return type) and overloads", testGenericType131, "60");

const char *testGenericType132 =
"class Bar<T, U>{ T x; U y; }\r\n\
auto foo(Bar<int, @T> ref(int, @U) m){ assert(T == int); assert(U == float); auto n = m(2, 3); return n.x + n.y; }\r\n\
Bar<int, int> ken(int a, float b){ Bar<int, int> r; r.x = a; r.y = b; return r; }\r\n\
return foo(ken);";
TEST_RESULT("Additional specialization test 7 (generic type alias in function type) and overloads", testGenericType132, "5");

const char *testGenericType133 =
"class Foo<T>{ T x; }\r\n\
\r\n\
int foo(Foo<Foo<@T>> a, int x){ return 1; }\r\n\
int foo(Foo<@T> a, double x){ return 2; }\r\n\
\r\n\
Foo<int> a;\r\n\
assert(foo(a, 2) == 2);\r\n\
Foo<Foo<int>> b;\r\n\
assert(foo(b, 2) == 1);\r\n\
\r\n\
return 1;";
TEST_RESULT("Additional specialization test 7 (multiple specializations for the same type)", testGenericType133, "1");

const char *testGenericType134 =
"class Tuple<T, U>{ T x; U y; void Tuple(T a, U b){ x = a; y = b; } }\r\n\
auto operator==(Tuple<@T, @U> ref a, b){ return a.x == b.x && a.y == b.y; }\r\n\
auto operator+=(Tuple<@T, @U> ref a, b){ a.x += b.x; a.y += b.y; return a; }\r\n\
auto operator-=(Tuple<@T, @U> ref a, b){ a.x -= b.x; a.y -= b.y; return a; }\r\n\
auto operator*=(Tuple<@T, @U> ref a, b){ a.x *= b.x; a.y *= b.y; return a; }\r\n\
auto operator/=(Tuple<@T, @U> ref a, b){ a.x /= b.x; a.y /= b.y; return a; }\r\n\
auto operator**=(Tuple<@T, @U> ref a, b){ a.x **= b.x; a.y **= b.y; return a; }\r\n\
auto operator%=(Tuple<@T, @U> ref a, b){ a.x %= b.x; a.y %= b.y; return a; }\r\n\
auto operator<<=(Tuple<@T, @U> ref a, b){ a.x <<= b.x; a.y <<= b.y; return a; }\r\n\
auto operator>>=(Tuple<@T, @U> ref a, b){ a.x >>= b.x; a.y >>= b.y; return a; }\r\n\
auto operator&=(Tuple<@T, @U> ref a, b){ a.x &= b.x; a.y &= b.y; return a; }\r\n\
auto operator|=(Tuple<@T, @U> ref a, b){ a.x |= b.x; a.y |= b.y; return a; }\r\n\
auto operator^=(Tuple<@T, @U> ref a, b){ a.x ^= b.x; a.y ^= b.y; return a; }\r\n\
\r\n\
Tuple<int, int>[11] res;\r\n\
for(i in res){ i = Tuple<int, int>(12, 37); } // 1100, 100101\r\n\
res[0] += Tuple<int, int>(5, 17); assert(res[0] == Tuple<int, int>(17, 54));\r\n\
res[1] -= Tuple<int, int>(5, 17); assert(res[1] == Tuple<int, int>(7, 20));\r\n\
res[2] *= Tuple<int, int>(5, 17); assert(res[2] == Tuple<int, int>(60, 17*37));\r\n\
res[3] /= Tuple<int, int>(4, 5); assert(res[3] == Tuple<int, int>(3, 7));\r\n\
res[4] **= Tuple<int, int>(3, 2); assert(res[4] == Tuple<int, int>(12**3, 37*37));\r\n\
res[5] %= Tuple<int, int>(5, 10); assert(res[5] == Tuple<int, int>(2, 7));\r\n\
res[6] <<= Tuple<int, int>(2, 3); assert(res[6] == Tuple<int, int>(48, 296));\r\n\
res[7] >>= Tuple<int, int>(1, 2); assert(res[7] == Tuple<int, int>(6, 9));\r\n\
res[8] &= Tuple<int, int>(5, 10); assert(res[8] == Tuple<int, int>(4, 0));\r\n\
res[9] |= Tuple<int, int>(5, 10); assert(res[9] == Tuple<int, int>(13, 47));\r\n\
res[10] ^= Tuple<int, int>(5, 10); assert(res[10] == Tuple<int, int>(9, 47));\r\n\
return 1;";
TEST_RESULT("generic type and modify-assignment operator overload", testGenericType134, "1");

const char *testGenericType135 =
"class Foo<T>{ T x; }\r\n\
auto foo(Foo<@T> x, int ref(int, int) y){ return x.x * y(1, 2); }\r\n\
Foo<int> a; a.x = 2;\r\n\
return foo(a, <i, j>{ i+j; });";
TEST_RESULT("short inline function definition in a funciton with specializations", testGenericType135, "6");

const char *testGenericType136 =
"class Foo<T>{ T x; }\r\n\
auto foo(Foo<@T> x, @T y){ return x.x * y; }\r\n\
auto foo(Foo<@T> x, double y){ return x.x * y; }\r\n\
\r\n\
Foo<int> a; a.x = 2;\r\n\
assert(9 == int(foo(a, 4.5)));\r\n\
assert(double == typeof(foo(a, 4.5)));\r\n\
\r\n\
return foo(a, 4);";
TEST_RESULT("function with equal alias names is instanced only if alias types are equal", testGenericType136, "8");

const char *testGenericType137 =
"class Foo<T, U>{ T x; U y; }\r\n\
auto foo(Foo<@T, @T> x){ return 2; }\r\n\
Foo<int, int> a;\r\n\
return foo(a);";
TEST_RESULT("function with equal alias names is instanced only if alias types are equal 2", testGenericType137, "2");

const char *testGenericType138 =
"auto bar(auto ref x){ return x; }\r\n\
auto bar(generic x){ @if(typeof(x).isReference) return x; else return &x; }\r\n\
\r\n\
class Foo<T>{}\r\n\
\r\n\
typeid Foo:foo(typeof(bar(T())) e){ return typeof(e); }\r\n\
\r\n\
Foo<int> test1;\r\n\
assert(test1.foo(4) == int ref);\r\n\
\r\n\
Foo<int ref> test2; int t;\r\n\
assert(test2.foo(&t) == int ref);\r\n\
\r\n\
Foo<auto ref> test3;\r\n\
assert(test3.foo(4) == auto ref);\r\n\
\r\n\
return 1;";
TEST_RESULT("improved error handling in typeof", testGenericType138, "1");

const char *testLogOrAndLogAndOperatorOverload2 =
"class Foo<T>{ T x; }\r\n\
\r\n\
int operator||(Foo<@T> a, Foo<@T> ref() b)\r\n\
{\r\n\
	return int(a.x) || int(b().x);\r\n\
}\r\n\
int operator&&(Foo<@T> a, Foo<@T> ref() b)\r\n\
{\r\n\
	return int(a.x) && int(b().x);\r\n\
}\r\n\
Foo<int> a, b;\r\n\
a.x = 1;\r\n\
b.x = 0;\r\n\
\r\n\
Foo<int> k(){ assert(0); return Foo<int>(); } // never called!\r\n\
\r\n\
assert(a || k());\r\n\
assert(!(b && k()));\r\n\
\r\n\
Foo<double> c, d;\r\n\
c.x = 1;\r\n\
d.x = 0;\r\n\
\r\n\
Foo<double> j(){ assert(0); return Foo<double>(); } // never called!\r\n\
\r\n\
assert(c || j());\r\n\
assert(!(d && j()));\r\n\
\r\n\
return 1;";
TEST_RESULT("overloaded || and && operators do not break short-circuiting 2", testLogOrAndLogAndOperatorOverload2, "1");

const char *testFunctionPrototypeProblemTest =
"import std.vector;\r\n\
\r\n\
class Edge{ int x; }\r\n\
vector<Edge> edges;\r\n\
edges.push_back(Edge());\r\n\
\r\n\
{\r\n\
	edges[0];\r\n\
	edges[0];\r\n\
	edges[0];\r\n\
	edges[0];\r\n\
	edges[0];\r\n\
	edges[0];\r\n\
\r\n\
	int ClusterVertices(int c)\r\n\
	{\r\n\
		return 0;\r\n\
	}\r\n\
}\r\n\
int foo()\r\n\
{\r\n\
	void rek(int nSoFar){ }\r\n\
	rek(1);\r\n\
	return 0;\r\n\
}\r\n\
foo();\r\n\
return 1;";
TEST_RESULT("Function prototype problem test", testFunctionPrototypeProblemTest, "1");

const char *testOperatorInstanceCorrectness =
"class Foo<T>{ }\r\n\
auto operator+(Foo<generic> ref v){ }\r\n\
\r\n\
void ref(Foo<int> ref) x = @+;\r\n\
void ref(Foo<int> ref) y = @+;\r\n\
\r\n\
assert(x == y); //  Assertion Failed\r\n\
\r\n\
return 1;";
TEST_RESULT("Operator instancing test", testOperatorInstanceCorrectness, "1");

const char *testConstantsInAGenericType =
"class Foo<T>\r\n\
{\r\n\
	int a;\r\n\
\r\n\
	const int b = 6;\r\n\
	const int c = 7, d = 8;\r\n\
	const auto e = 9, f = 10;\r\n\
	const auto g = sizeof(T), h;\r\n\
}\r\n\
\r\n\
assert(Foo<int>.b == 6);\r\n\
\r\n\
assert(Foo<int>.c == 7);\r\n\
assert(Foo<int>.d == 8);\r\n\
\r\n\
assert(Foo<int>.e == 9);\r\n\
assert(Foo<int>.f == 10);\r\n\
\r\n\
assert(Foo<int>.g == 4);\r\n\
assert(Foo<int>.h == 5);\r\n\
\r\n\
assert(Foo<double>.g == 8);\r\n\
assert(Foo<double>.h == 9);\r\n\
\r\n\
assert(sizeof(Foo<int>) == 4);\r\n\
assert(sizeof(Foo<double>) == 4);\r\n\
\r\n\
return 1;";
TEST_RESULT("Constants in a generic type test", testConstantsInAGenericType, "1");

LOAD_MODULE(test_constant_export3, "test.constant_export3",
"class Foo<T>\r\n\
{\r\n\
	int a;\r\n\
\r\n\
	const int b = 6;\r\n\
	const int c = 7, d = 8;\r\n\
	const auto e = 9, f = 10;\r\n\
	const auto g = sizeof(T), h;\r\n\
}");
const char	*testConstantsInAGenericType2 =
"import test.constant_export3;\r\n\
\r\n\
assert(Foo<int>.b == 6);\r\n\
\r\n\
assert(Foo<int>.c == 7);\r\n\
assert(Foo<int>.d == 8);\r\n\
\r\n\
assert(Foo<int>.e == 9);\r\n\
assert(Foo<int>.f == 10);\r\n\
\r\n\
assert(Foo<int>.g == 4);\r\n\
assert(Foo<int>.h == 5);\r\n\
\r\n\
assert(Foo<double>.g == 8);\r\n\
assert(Foo<double>.h == 9);\r\n\
\r\n\
assert(sizeof(Foo<int>) == 4);\r\n\
assert(sizeof(Foo<double>) == 4);\r\n\
\r\n\
return 1;";
TEST_RESULT("Constants in a generic type test (import)", testConstantsInAGenericType2, "1");

const char	*testAccessorAccessInsideAMemberFunctionGeneric =
"class Foo<T>{ T _x; }\r\n\
\r\n\
auto Foo.x(){ return _x; }\r\n\
void Foo.x(T r){ _x = r / 2; }\r\n\
\r\n\
void Foo:Foo(){ x = 4; }\r\n\
\r\n\
Foo<int> a;\r\n\
return a.x;";
TEST_RESULT("Accessor access inside a generic type member function", testAccessorAccessInsideAMemberFunctionGeneric, "2");

const char	*testLocalClassOperatorsScope =
"class Rect<T>\r\n\
{\r\n\
	T x;\r\n\
	auto operator=( Rect<int> a, Rect<float> b ){ }\r\n\
}\r\n\
\r\n\
Rect<float> a;\r\n\
return 1;";
TEST_RESULT("Local class operators go out of scope before default assignment operator is created", testLocalClassOperatorsScope, "1");

const char	*testLocalClassAliasParent =
"class Foo<T>\r\n\
{\r\n\
	typedef T ref X;\r\n\
}\r\n\
int foo()\r\n\
{\r\n\
	Foo<int> a;\r\n\
	return typeof(a).X == int ref;\r\n\
}\r\n\
return foo();";
TEST_RESULT("If a class is defined locally, its aliases still go to class alias list, not parent function alias list", testLocalClassAliasParent, "1");

const char	*testGenericTypeDefaultConstructor =
"class hashmap_node<Key, Value>{ Value value; }\r\n\
class hashmap<Key, Value>\r\n\
{\r\n\
	typedef hashmap_node<Key, Value> Node;\r\n\
\r\n\
	Node ref	entry;\r\n\
	int x;\r\n\
}\r\n\
void hashmap:hashmap()\r\n\
{\r\n\
	x = 5;\r\n\
}\r\n\
void hashmap:foo()\r\n\
{\r\n\
	entry = new Node;\r\n\
}\r\n\
hashmap<bool, hashmap<int, float>> h;\r\n\
h.foo();\r\n\
return h.entry.value.x;";
TEST_RESULT("Default constructor generation for a generic type", testGenericTypeDefaultConstructor, "5");

const char *testGenericType139 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	T y{ get{ return -x; } set(t){ x = -t; } };\r\n\
\r\n\
	void Foo(){ x = 4; }\r\n\
}\r\n\
Foo<int> g(){ return Foo<int>(); }\r\n\
\r\n\
return g().y;";
TEST_RESULT("generic type accessor is instanced (after function call)", testGenericType139, "-4");

const char *testGenericType140 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	auto getX(generic y){ return x + y; }\r\n\
\r\n\
	void Foo(){ x = 4; }\r\n\
}\r\n\
Foo<int> g(){ return Foo<int>(); }\r\n\
\r\n\
int ref(int) a = g().getX;\r\n\
return a(3);";
TEST_RESULT("generic type generic function retrieval (after function call)", testGenericType140, "7");

const char *testGenericType141 =
"class Foo<X>\r\n\
{\r\n\
	@if(X == int)\r\n\
	{\r\n\
		int a;\r\n\
	}else if(X == char){\r\n\
		float a;\r\n\
	}\r\n\
}\r\n\
Foo<int> a;\r\n\
Foo<double> b;\r\n\
Foo<char> c;\r\n\
return typeof(a).hasMember(a) && typeof(a.a) == int && !typeof(b).hasMember(a) && typeof(c).hasMember(a) && typeof(c.a) == float;";
TEST_RESULT("static if in type body 1", testGenericType141, "1");

const char *testGenericType142 =
"class Foo<X>\r\n\
{\r\n\
	@if(X == int)\r\n\
	{\r\n\
		int a;\r\n\
	}else{\r\n\
		float a;\r\n\
	}\r\n\
}\r\n\
Foo<int> a;\r\n\
Foo<double> b;\r\n\
Foo<char> c;\r\n\
return typeof(a).hasMember(a) && typeof(a.a) == int && typeof(b).hasMember(a) && typeof(b.a) == float && typeof(c).hasMember(a) && typeof(c.a) == float;";
TEST_RESULT("static if in type body 2", testGenericType142, "1");

const char *testGenericType143 =
"class Foo<X>\r\n\
{\r\n\
	@if(X == int)\r\n\
		int a;\r\n\
	else if(X == char)\r\n\
		float a;\r\n\
}\r\n\
Foo<int> a;\r\n\
Foo<double> b;\r\n\
Foo<char> c;\r\n\
return typeof(a).hasMember(a) && typeof(a.a) == int && !typeof(b).hasMember(a) && typeof(c).hasMember(a) && typeof(c.a) == float;";
TEST_RESULT("static if in type body 3", testGenericType143, "1");

const char *testGenericType144 =
"class Foo<X>\r\n\
{\r\n\
	@if(X == int)\r\n\
		int a;\r\n\
	else\r\n\
		float a;\r\n\
}\r\n\
Foo<int> a;\r\n\
Foo<double> b;\r\n\
Foo<char> c;\r\n\
return typeof(a).hasMember(a) && typeof(a.a) == int && typeof(b).hasMember(a) && typeof(b.a) == float && typeof(c).hasMember(a) && typeof(c.a) == float;";
TEST_RESULT("static if in type body 4", testGenericType144, "1");

const char *testGenericType145 =
"class Foo<X>\r\n\
{\r\n\
	@if(X == int){\r\n\
		int a;\r\n\
	}\r\n\
}\r\n\
Foo<int> a;\r\n\
Foo<double> b;\r\n\
Foo<char> c;\r\n\
return typeof(a).hasMember(a) && typeof(a.a) == int && !typeof(b).hasMember(a) && !typeof(c).hasMember(a);";
TEST_RESULT("static if in type body 5", testGenericType145, "1");

const char *testGenericType146 =
"class Foo<X>\r\n\
{\r\n\
	@if(X == int)\r\n\
		int a;\r\n\
}\r\n\
Foo<int> a;\r\n\
Foo<double> b;\r\n\
Foo<char> c;\r\n\
return typeof(a).hasMember(a) && typeof(a.a) == int && !typeof(b).hasMember(a) && !typeof(c).hasMember(a);";
TEST_RESULT("static if in type body 6", testGenericType146, "1");

const char *testGenericType147 =
"class Foo<T>{ T x; }\r\n\
\r\n\
Foo a = Foo<int>();\r\n\
a.x = 5;\r\n\
\r\n\
int foo(Foo a)\r\n\
{\r\n\
	return a.x;\r\n\
}\r\n\
\r\n\
return foo(a);";
TEST_RESULT("Generic class name can be used if the generic arguments can be deduced", testGenericType147, "5");

const char *testGenericType148 =
"class F<T>{}\r\n\
int F:f(){ return 2; }\r\n\
F<int> a;\r\n\
int operator+(int a, int ref() b){ return a + b(); }\r\n\
return 1 + a.f;";
TEST_RESULT("Generic class function pointer access performs instantiation", testGenericType148, "3");

const char *testGenericType149 =
"class F<T>{}\r\n\
int F:f(){ return 2; }\r\n\
float F:f(){ return 2; }\r\n\
double F:f(){ return 2; }\r\n\
F<int> a;\r\n\
int operator+(int a, int ref() b){ return a + b(); }\r\n\
return 1 + a.f;";
TEST_RESULT("Generic class function pointer access performs instantiation", testGenericType149, "3");

const char *testGenericType150 =
"class Foo<T>{}\r\n\
auto Foo:foo(){ return sizeof(T); }\r\n\
\r\n\
Foo<int> a;\r\n\
Foo<char> b;\r\n\
\r\n\
auto ref x = a;\r\n\
auto ref y = b;\r\n\
\r\n\
return x.foo() + y.foo();";
TEST_RESULT("Generic class external function definition and auto ref calls", testGenericType150, "5");

const char *testGenericType151 =
"class Foo<T>{}\r\n\
auto Foo:foo(){ return sizeof(T); }\r\n\
auto Foo:foo(int x){ return sizeof(T) + x; }\r\n\
\r\n\
Foo<int> a;\r\n\
Foo<char> b;\r\n\
\r\n\
auto ref x = a;\r\n\
auto ref y = b;\r\n\
\r\n\
return x.foo() + y.foo(5) + x.foo(10) + y.foo();";
TEST_RESULT("Generic class external function definition and auto ref calls (overloads)", testGenericType151, "25");

const char *testGenericType152 =
"class Foo<T>{}\r\n\
auto Foo:foo(generic x){ return sizeof(T) * 10 + sizeof(x); }\r\n\
\r\n\
Foo<int> a;\r\n\
Foo<char> b;\r\n\
\r\n\
auto ref x = a;\r\n\
auto ref y = b;\r\n\
\r\n\
return x.foo('a') + y.foo(2l);";
TEST_RESULT("Generic class external function definition and auto ref calls (generic function)", testGenericType152, "59");

const char *testGenericType153 =
"class Foo<T>{}\r\n\
auto Foo:foo(generic x){ return sizeof(T) * 10 + sizeof(x); }\r\n\
\r\n\
class Bar{}\r\n\
auto Bar:foo(generic x){ return 100; }\r\n\
\r\n\
Foo<int> a;\r\n\
Bar b;\r\n\
\r\n\
auto ref x = a;\r\n\
auto ref y = b;\r\n\
\r\n\
return x.foo('a') + y.foo(2l);";
TEST_RESULT("Generic class external function definition and auto ref calls (generic function, multiple instances)", testGenericType153, "141");
