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
TEST_RESULT("Generic type specialized extenal function definition", testGenericType11, "-10");

const char *testGenericType12 =
"class Foo<T>{ T x; }\r\n\
int Foo:foo(){ @if(typeof(this.x) == int) return x; else return -x; }\r\n\
Foo<int> x;\r\n\
Foo<float> z;\r\n\
x.x = 10;\r\n\
z.x = 5;\r\n\
return x.foo() + z.foo();";
TEST_RESULT("Generic type extenal member function definition", testGenericType12, "5");

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
