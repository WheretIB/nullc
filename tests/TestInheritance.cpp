#include "TestBase.h"

const char	*testInheritance1 =
"class vec2\r\n\
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
"class vec2\r\n\
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
"class Foo{ const int A=2, B, C; int y; }\r\n\
class Bar : Foo { int x; }\r\n\
\r\n\
Bar x;\r\n\
x.x = 5;\r\n\
assert(sizeof(Bar) == 8);\r\n\
assert(x.B == 3);\r\n\
return Bar.C;";
TEST_RESULT("Inheritance test 3", testInheritance3, "4");

const char	*testInheritance4 =
"enum Foo{ A = 2, B, C }\r\n\
class Bar : Foo { int x; }\r\n\
\r\n\
Bar x;\r\n\
x.x = 5;\r\n\
assert(sizeof(Bar) == 8);\r\n\
assert(int(x.B) == 3);\r\n\
return Bar.C;";
TEST_RESULT("Inheritance test 4", testInheritance4, "4");

const char	*testInheritance5 =
"class Foo<T>{ T x; }\r\n\
class Bar<T> : Foo<T>{ T y; }\r\n\
\r\n\
Bar<float> x;\r\n\
\r\n\
assert(typeof(x.x) == typeof(x.y));\r\n\
assert(typeof(x.x) == float);\r\n\
\r\n\
return 1;";
TEST_RESULT("Inheritance test 5", testInheritance5, "1");
