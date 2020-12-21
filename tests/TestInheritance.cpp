#include "TestBase.h"

const char	*testInheritance1 =
"class vec2 extendable\r\n\
{\r\n\
	float x, y;\r\n\
	void vec2(float x, y){ this.x = x; this.y = y; }\r\n\
}\r\n\
class vec3 : vec2\r\n\
{\r\n\
	float z;\r\n\
	void vec3(float x, y, z){ this.x = x; this.y = y; this.z = z; }\r\n\
}\r\n\
vec3 t = vec3(1, 2, 3);\r\n\
return int(t.x + t.z);";
TEST_RESULT("Inheritance test 1", testInheritance1, "4");

const char	*testInheritance2 =
"class vec2 extendable\r\n\
{\r\n\
	typedef float K;\r\n\
	float x, y;\r\n\
	void vec2(float x, y){ this.x = x; this.y = y; }\r\n\
}\r\n\
class vec3 : vec2\r\n\
{\r\n\
	K z;\r\n\
	void vec3(K x, y, z){ this.x = x; this.y = y; this.z = z; }\r\n\
}\r\n\
vec3 t = vec3(1, 2, 3);\r\n\
return int(t.x + t.z);";
TEST_RESULT("Inheritance test 2", testInheritance2, "4");

const char	*testInheritance3 =
"class Foo extendable{ const int A=2, B, C; int y; }\r\n\
class Bar : Foo { int x; }\r\n\
\r\n\
Bar x;\r\n\
x.x = 5;\r\n\
assert(sizeof(Bar) == 12);\r\n\
assert(x.B == 3);\r\n\
return Bar.C;";
TEST_RESULT("Inheritance test 3", testInheritance3, "4");

const char	*testInheritance5 =
"class Foo<T> extendable{ T x; }\r\n\
class Bar<T> : Foo<T>{ T y; }\r\n\
\r\n\
Bar<float> x;\r\n\
\r\n\
assert(typeof(x.x) == typeof(x.y));\r\n\
assert(typeof(x.x) == float);\r\n\
\r\n\
return 1;";
TEST_RESULT("Inheritance test 5", testInheritance5, "1");

const char	*testInheritance6 =
"class Foo<T> extendable{ T x; }\r\n\
class Bar<T> : Foo<int>{ T y; }\r\n\
\r\n\
Bar<float> x;\r\n\
\r\n\
assert(typeof(x.x) != typeof(x.y));\r\n\
assert(typeof(x.x) == int);\r\n\
assert(typeof(x.y) == float);\r\n\
\r\n\
return 1;";
TEST_RESULT("Inheritance test 6", testInheritance6, "1");

const char	*testInheritance7 =
"class Foo<U> extendable{ U x; }\r\n\
class Bar<U, T> : Foo<int>{ T y; }\r\n\
\r\n\
Bar<char, float> x;\r\n\
\r\n\
assert(typeof(x.x) != typeof(x.y));\r\n\
assert(typeof(x.x) == int);\r\n\
assert(typeof(x.y) == float);\r\n\
\r\n\
return 1;";
TEST_RESULT("Inheritance test 7", testInheritance7, "1");

const char	*testInheritance8 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec2 x;\r\n\
vec3 y;\r\n\
\r\n\
vec2 ref z = &y;\r\n\
vec3 ref w = z;\r\n\
\r\n\
return 1;";
TEST_RESULT("Inheritance test 8", testInheritance8, "1");

const char	*testInheritance9 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
class vec4 : vec3{ float w; }\r\n\
\r\n\
vec2 x;\r\n\
vec4 y;\r\n\
\r\n\
vec2 ref z = &y;\r\n\
vec3 ref w = z;\r\n\
\r\n\
return 1;";
TEST_RESULT("Inheritance test 9", testInheritance9, "1");

const char	*testInheritance10 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec2 x;\r\n\
\r\n\
vec2 ref z;\r\n\
vec3 ref w = z;\r\n\
\r\n\
return 1;";
TEST_RESULT("Inheritance test 10", testInheritance10, "1");

const char	*testInheritance11 =
"import std.gc;\r\n\
\r\n\
class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ int ref z; }\r\n\
\r\n\
vec2 ref a;\r\n\
\r\n\
vec3 ref foo(){ return new vec3(); }\r\n\
vec3 ref x = foo();\r\n\
\r\n\
x.z = new int;\r\n\
*x.z = 2;\r\n\
a = x;\r\n\
x = nullptr;\r\n\
\r\n\
GC.CollectMemory();\r\n\
\r\n\
int ref b = new int(3);\r\n\
x = a;\r\n\
return *x.z;";
TEST_RESULT("Inheritance test 11", testInheritance11, "2");

const char	*testInheritance13 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec2 x;\r\n\
vec3 y;\r\n\
\r\n\
vec2 ref a = &y;\r\n\
\r\n\
return typeid(*a) == vec3;";
TEST_RESULT("Inheritance test 13", testInheritance13, "1");

const char	*testInheritance14 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec3 y; y.z = 5;\r\n\
vec2 ref x = &y;\r\n\
\r\n\
int bar(vec3 ref x){ return x.z; }\r\n\
return bar(x);";
TEST_RESULT("Inheritance test 14", testInheritance14, "5");

const char	*testInheritance17 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec2 ref foo(){ auto r = new vec3; r.x = 2; r.z = 3; return r; }\r\n\
\r\n\
vec2 ref a = foo();\r\n\
vec3 ref b = a;\r\n\
\r\n\
return int(a.x + b.z);";
TEST_RESULT("Inheritance test 17", testInheritance17, "5");

const char	*testInheritance18 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec2 ref foo(){ auto r = new vec3; r.x = 2; r.z = 3; return r; }\r\n\
vec3 ref bar(){ return foo(); }\r\n\
\r\n\
vec2 ref a = foo();\r\n\
vec3 ref b = bar();\r\n\
\r\n\
return int(a.x + b.z);";
TEST_RESULT("Inheritance test 18", testInheritance18, "5");

const char	*testInheritance19 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(){ return y; } }\r\n\
\r\n\
vec3 a;\r\n\
a.x = 2; a.y = 3;\r\n\
\r\n\
vec2 ref b = &a;\r\n\
\r\n\
return b.foo();";
TEST_RESULT("Inheritance test 19", testInheritance19, "3");

const char	*testInheritance20 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(){ return y; } }\r\n\
\r\n\
vec3 a;\r\n\
a.x = 2; a.y = 3;\r\n\
\r\n\
vec2 ref foo(){ return &a; }\r\n\
\r\n\
return foo().foo();";
TEST_RESULT("Inheritance test 20", testInheritance20, "3");

const char	*testInheritance21 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(){ return y; } }\r\n\
\r\n\
vec3 a;\r\n\
a.x = 2; a.y = 3;\r\n\
\r\n\
class Test{ vec2 ref a; }\r\n\
Test t; t.a = &a;\r\n\
\r\n\
return t.a.foo() == 3;";
TEST_RESULT("Inheritance test 21", testInheritance21, "1");

const char	*testInheritance22 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec3 a;\r\n\
a.x = 2;\r\n\
\r\n\
return a.foo();";
TEST_RESULT("Inheritance test 22", testInheritance22, "2");

const char	*testInheritance23 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec3 a;\r\n\
a.x = 2;\r\n\
\r\n\
vec2 ref b = &a;\r\n\
\r\n\
return b.foo();";
TEST_RESULT("Inheritance test 23", testInheritance23, "2");

const char	*testInheritance24 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec3 a;\r\n\
a.x = 2;\r\n\
\r\n\
vec2 ref foo(){ return &a; }\r\n\
\r\n\
return foo().foo();";
TEST_RESULT("Inheritance test 24", testInheritance24, "2");

const char	*testInheritance25 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec3 a;\r\n\
a.x = 2;\r\n\
\r\n\
class Test{ vec2 ref a; }\r\n\
Test t; t.a = &a;\r\n\
\r\n\
return t.a.foo();";
TEST_RESULT("Inheritance test 25", testInheritance25, "2");

const char	*testInheritance26 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(){ return y; } }\r\n\
\r\n\
vec3 a;\r\n\
a.x = 2; a.y = 3;\r\n\
\r\n\
vec2 ref aa = &a;\r\n\
auto ref b = aa;\r\n\
\r\n\
return b.foo();";
TEST_RESULT("Inheritance test 26", testInheritance26, "3");

const char	*testInheritance27 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(){ return y; } }\r\n\
\r\n\
vec3 ref a = (auto(){ return new vec3; })();\r\n\
a.x = 2; a.y = 3;\r\n\
\r\n\
vec2 ref aa = a;\r\n\
a = nullptr;\r\n\
\r\n\
auto x = aa.foo;\r\n\
\r\n\
return x();";
TEST_RESULT("Inheritance test 27", testInheritance27, "3");

const char	*testInheritance28 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(){ return y; } }\r\n\
\r\n\
vec3 ref a = (auto(){ return new vec3; })();\r\n\
a.x = 2; a.y = 3;\r\n\
\r\n\
vec2 ref aa = a;\r\n\
a = nullptr;\r\n\
\r\n\
auto ref b = aa;\r\n\
\r\n\
auto x = b.foo;\r\n\
\r\n\
return x();";
TEST_RESULT("Inheritance test 28", testInheritance28, "3");

const char	*testInheritance29 =
"class vec2 extendable{ float x, y; int foo(int a){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(int a){ return y; } }\r\n\
\r\n\
vec3 a;\r\n\
a.x = 2; a.y = 3;\r\n\
\r\n\
vec2 ref b = &a;\r\n\
\r\n\
return b.foo(3);";
TEST_RESULT("Inheritance test 29", testInheritance29, "3");

const char	*testInheritance30 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(){ return y; } }\r\n\
\r\n\
vec3 ref a = (auto(){ return new vec3; })();\r\n\
a.x = 2; a.y = 3;\r\n\
\r\n\
vec2 ref aa = a;\r\n\
a = nullptr;\r\n\
\r\n\
vec2 ref bar(){ return aa; }\r\n\
\r\n\
auto x = bar().foo;\r\n\
\r\n\
return x();";
TEST_RESULT("Inheritance test 30", testInheritance30, "3");

const char	*testInheritance31 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(){ return y; } }\r\n\
\r\n\
vec3 ref a = (auto(){ return new vec3; })();\r\n\
a.x = 2; a.y = 3;\r\n\
\r\n\
vec2 ref aa = a;\r\n\
a = nullptr;\r\n\
\r\n\
auto ref bar(){ return aa; }\r\n\
\r\n\
auto x = bar().foo;\r\n\
\r\n\
return x();";
TEST_RESULT("Inheritance test 31", testInheritance31, "3");

const char	*testInheritance32 =
"class vec2 extendable{ float x, y; int foo(double a){ return x; } }\r\n\
class distraction{ int foo(float x){ return x; } }\r\n\
\r\n\
vec2 ref a = new vec2;\r\n\
a.x = 2;\r\n\
\r\n\
return a.foo(0);";
TEST_RESULT("Inheritance test 32", testInheritance32, "2");

const char	*testInheritance33 =
"class vec2 extendable{ float x, y; int foo(double a){ return x; } }\r\n\
class distraction{ int foo(float x){ return x; } }\r\n\
\r\n\
vec2 ref a = new vec2;\r\n\
a.x = 2;\r\n\
\r\n\
auto x = a.foo;\r\n\
return x(0);";
TEST_RESULT("Inheritance test 33", testInheritance33, "2");

const char	*testInheritance34 =
"class vec2 extendable{ float x, y; int foo(int a = 5){ return x + a; } }\r\n\
\r\n\
vec2 ref a = new vec2;\r\n\
a.x = 2;\r\n\
\r\n\
return int(a.foo());";
TEST_RESULT("Inheritance test 34", testInheritance34, "7");

const char	*testInheritance35 =
"class vec2 extendable{ float x, y; int foo(){ return x; } }\r\n\
class vec3 : vec2{ float z; int foo(){ return y; } }\r\n\
\r\n\
vec3 ref a = new vec3;\r\n\
a.x = 2;\r\n\
\r\n\
int foo(vec2 ref x){ return x.x; }\r\n\
\r\n\
auto ref b = a;\r\n\
\r\n\
return foo(vec2 ref(b));";
TEST_RESULT("Inheritance test 35", testInheritance35, "2");

const char	*testInheritance36 =
"class vec2 extendable{ float x, y; int foo(int a = 2){ return a; } }\r\n\
class vec3 : vec2{ float z; int foo(int a = 3){ return a; } }\r\n\
\r\n\
vec3 ref a = new vec3;\r\n\
vec2 ref b = a;\r\n\
\r\n\
return (a.foo() == 3) + (b.foo() == 2);";
TEST_RESULT("Inheritance test 36", testInheritance36, "2");

const char	*testInheritance37 =
"class vec2 extendable{ float x, y; int foo(int i, j){ return i / j; } }\r\n\
class vec3 : vec2{ float z; int foo(int j, i){ return i / j; } }\r\n\
\r\n\
vec3 ref a = new vec3;\r\n\
vec3 ref b = a;\r\n\
return a.foo(i: 6, j: 3) + b.foo(i: 6, j: 3);";
TEST_RESULT("Inheritance test 37", testInheritance37, "4");

const char	*testInheritance38 =
"class vec2 extendable\r\n\
{\r\n\
	float x, y;\r\n\
	auto foo(generic i){ return -i; }\r\n\
}\r\n\
class vec3 : vec2\r\n\
{\r\n\
	float z;\r\n\
	int foo(int i){ return i; }\r\n\
	int foo(int j, i){ return i / j; }\r\n\
}\r\n\
\r\n\
vec3 ref a = new vec3;\r\n\
vec2 ref b = a;\r\n\
return b.foo(i: 6) + a.foo(i: 6, j: 3);";
TEST_RESULT("Inheritance test 38", testInheritance38, "8");

const char	*testInheritance39 =
"class vec2 extendable\r\n\
{\r\n\
	float x, y;\r\n\
	auto foo(generic i){ return -i; }\r\n\
}\r\n\
class vec3 : vec2\r\n\
{\r\n\
	float z;\r\n\
	auto foo(generic i){ return i * 2; }\r\n\
}\r\n\
\r\n\
vec3 ref a = new vec3;\r\n\
vec2 ref b = new vec2;\r\n\
\r\n\
b.foo(6);\r\n\
\r\n\
return a.foo(6);";
TEST_RESULT("Inheritance test 39 (generic member function)", testInheritance39, "12");

const char	*testInheritance40 =
"class vec2 extendable{ int x, y; }\r\n\
class vec3 : vec2{ int z; }\r\n\
class vec4 : vec3{ int w; }\r\n\
\r\n\
vec2 x;\r\n\
vec3 y;\r\n\
vec4 z;\r\n\
\r\n\
vec2 ref xr = &z;\r\n\
vec3 ref yr = xr;\r\n\
vec4 ref zr = yr;\r\n\
xr.x = 1;\r\n\
xr.y = 20;\r\n\
yr.z = 300;\r\n\
zr.w = 4000;\r\n\
\r\n\
return z.x + z.y + z.z + z.w;";
TEST_RESULT("Inheritance test 40", testInheritance40, "4321");

const char	*testInheritance41 =
"class vec2 extendable{ int x, y; }\r\n\
class vec3 : vec2{ int z; }\r\n\
class vec4 : vec3{ int w; }\r\n\
return sizeof(vec4);";
TEST_RESULT("Inheritance test 41", testInheritance41, "20");

const char	*testInheritance42 =
"class vec2 extendable{ int x, y; }\r\n\
class vec3 extendable : vec2{ int z; }\r\n\
class vec4 : vec3{ int w; }\r\n\
return sizeof(vec4);";
TEST_RESULT("Inheritance test 42", testInheritance42, "20");

const char	*testInheritance43 =
"class Test extendable{ int x; }\r\n\
int x = 1;\r\n\
class Bug:Test{}\r\n\
int y = 2;\r\n\
\r\n\
return x;";
TEST_RESULT("Inheritance test 43 - incorrect variable stack size after extended class definition", testInheritance43, "1");

const char	*testInheritance44 =
"class int2 extendable{ int x, y; void int2(){ x = 10; y = 20; } }\r\n\
class int3 : int2{ int z; void int3(){ x = 1; y = 2; z = 3; } }\r\n\
\r\n\
int3 a;\r\n\
auto b = int2 ref(&a);\r\n\
auto c = int3 ref(b);\r\n\
\r\n\
int2 a2;\r\n\
auto b2 = int2 ref(&a2);\r\n\
\r\n\
return b.x + c.z + b2.y;";
TEST_RESULT("Inheritance test 44 - manual type casts", testInheritance44, "24");

const char	*testInheritance45 =
"class int2 extendable{ int x, y; void int2(){ x = 10; y = 20; } }\r\n\
class int3 : int2{ int z; void int3(){ x = 1; y = 2; z = 3; } }\r\n\
\r\n\
int2 ref a = new int3;\r\n\
int3 ref b = int3 ref(a);\r\n\
return b.z;";
TEST_RESULT("Inheritance test 45 - manual type casts", testInheritance45, "3");

const char	*testInheritance46 =
"class Test extendable\r\n\
{\r\n\
	int a;\r\n\
	int b;\r\n\
	int c;\r\n\
}\r\n\
\r\n\
void Test:Test()\r\n\
{\r\n\
	a = 1;\r\n\
	b = 2;\r\n\
	c = 3;\r\n\
}\r\n\
\r\n\
class Test2 : Test\r\n\
{\r\n\
	int d;\r\n\
}\r\n\
\r\n\
void Test2 : Test2()\r\n\
{\r\n\
	d = 10;\r\n\
}\r\n\
\r\n\
Test2 t;\r\n\
\r\n\
return t.a + t.b + t.c + t.d;";
TEST_RESULT("Inheritance test 46 - base class default constructors", testInheritance46, "16");

const char	*testInheritance47 =
"class Test extendable\r\n\
{\r\n\
	int a = 1;\r\n\
	int b = 2;\r\n\
	int c = 3;\r\n\
}\r\n\
\r\n\
class Test2 : Test\r\n\
{\r\n\
	int d = 10;\r\n\
}\r\n\
\r\n\
Test2 t;\r\n\
\r\n\
return t.a + t.b + t.c + t.d;";
TEST_RESULT("Inheritance test 47 - member initializers", testInheritance47, "16");

const char	*testInheritance48 =
"class Test\r\n\
{\r\n\
	int a = 1;\r\n\
	int b = 2;\r\n\
\r\n\
	int foo()\r\n\
	{\r\n\
		Test x;\r\n\
\r\n\
		return x.a + x.b;\r\n\
	}\r\n\
}\r\n\
\r\n\
Test t;\r\n\
return t.foo();";
TEST_RESULT("Inheritance test 48 - constructor availability", testInheritance48, "3");

const char	*testInheritance49 =
"class vec2 extendable\r\n\
{\r\n\
	int x = 1, y = 2;\r\n\
}\r\n\
\r\n\
class vec3 : vec2\r\n\
{\r\n\
	vec3 f()\r\n\
	{\r\n\
		vec3 x;\r\n\
		return x;\r\n\
	}\r\n\
}\r\n\
\r\n\
vec3 a;\r\n\
a = a.f();\r\n\
auto ref x = a;\r\n\
vec3 b = x;\r\n\
return b.x;";
TEST_RESULT("Inheritance test 49 - constructor availability", testInheritance49, "1");

const char	*testInheritance50 =
"class A extendable{}\r\n\
class B: A{}\r\n\
class C: B{}\r\n\
\r\n\
auto a = new A();\r\n\
auto b = new B();\r\n\
auto c = new C();\r\n\
\r\n\
bool t1 = a != b;\r\n\
bool t2 = b != c;\r\n\
\r\n\
a = c;\r\n\
b = c;\r\n\
\r\n\
bool t3 = a == b;\r\n\
bool t4 = b == c;\r\n\
\r\n\
return t1 && t2 && t3 && t4;";
TEST_RESULT("Inheritance test 50 - pointer comparison between classes in the same hierarchy", testInheritance50, "1");

const char	*testInheritance51 =
"class A extendable{}\r\n\
class B: A{}\r\n\
\r\n\
auto a = new A();\r\n\
auto b = new B();\r\n\
\r\n\
return (a == b ? a : b) == b && (a != b ? b : a) == b;";
TEST_RESULT("Inheritance test 51 - common type between base/derived classes in conditional expression 1", testInheritance51, "1");

const char	*testInheritance52 =
"class A extendable{}\r\n\
class B: A{}\r\n\
\r\n\
auto a = new A();\r\n\
auto b = new B();\r\n\
\r\n\
return typeof(a == b ? a : b) == A ref && typeof(a != b ? b : a) == A ref;";
TEST_RESULT("Inheritance test 51 - common type between base/derived classes in conditional expression 2", testInheritance52, "1");

const char	*testInheritance53 =
"int x;\r\n\
class Counter{ void Counter(){ x++; } }\r\n\
class Test extendable{ Counter a; }\r\n\
class Test2 : Test{}\r\n\
Test2 t;\r\n\
return x;";
TEST_RESULT_SIMPLE("Inheritance test 53 - class members should be initialized only once", testInheritance53, "1");

const char	*testInheritance54 =
"class Base extendable{ align(8) long x = 1; int y = 2; }\r\n\
class Derived: Base{ int z = 3; }\r\n\
class Derived2: Derived{ int w = 4; }\r\n\
Derived2 t;\r\n\
int s = t.x * 1000 + t.y * 100 + t.z * 10 + t.w;\r\n\
t.x = 4; t.y = 3; t.z = 2; t.w = 1;\r\n\
int s2 = t.x * 1000 + t.y * 100 + t.z * 10 + t.w;\r\n\
return s + s2;";
TEST_RESULT_SIMPLE("Inheritance test 54 - base class tail padding 1", testInheritance54, "5555");

const char	*testInheritance55 =
"class Base extendable{ align(8) int x = 1; int y = 2; }\r\n\
class Derived: Base{ int z = 3; }\r\n\
class Derived2: Derived{ int w = 4; }\r\n\
class Derived3: Derived2{ int a = 5; }\r\n\
Derived3 t;\r\n\
int s = t.x * 10000 + t.y * 1000 + t.z * 100 + t.w * 10 + t.a;\r\n\
t.x = 5; t.y = 4; t.z = 3; t.w = 2; t.a = 1;\r\n\
int s2 = t.x * 10000 + t.y * 1000 + t.z * 100 + t.w * 10 + t.a;\r\n\
return s + s2;";
TEST_RESULT_SIMPLE("Inheritance test 54 - base class tail padding 2", testInheritance55, "66666");
