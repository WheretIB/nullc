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

const char	*testInheritance12 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec2 x;\r\n\
vec3 y;\r\n\
y.x = 2; y.y = 3;\r\n\
x = y;\r\n\
\r\n\
return int(x.x + x.y);";
TEST_RESULT("Inheritance test 12", testInheritance12, "5");

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

const char	*testInheritance15 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec2 x;\r\n\
vec3 y; y.x = 5;\r\n\
\r\n\
int bar(vec2 x){ return x.x; }\r\n\
return bar(y);";
TEST_RESULT("Inheritance test 15", testInheritance15, "5");

const char	*testInheritance16 =
"class vec2 extendable{ float x, y; }\r\n\
class vec3 : vec2{ float z; }\r\n\
\r\n\
vec2 x;\r\n\
vec3 y;\r\n\
x = y;\r\n\
\r\n\
return typeid(&y) == vec3;";
TEST_RESULT("Inheritance test 16", testInheritance16, "1");

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
