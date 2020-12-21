#include "TestBase.h"

const char	*testAutoRefCall1 =
"int sum = 0;\r\n\
class Foo{ int i; float u; }\r\n\
void Foo:add(int u)\r\n\
{\r\n\
	sum += i * u;\r\n\
}\r\n\
void int:add(int u)\r\n\
{\r\n\
	sum += *this + u;\r\n\
}\r\n\
\r\n\
void Foo:add()\r\n\
{\r\n\
	sum -= i;\r\n\
}\r\n\
void int:add()\r\n\
{\r\n\
	sum *= *this;\r\n\
}\r\n\
\r\n\
Foo test;\r\n\
test.i = 5;\r\n\
\r\n\
typedef auto ref object;\r\n\
\r\n\
object[2] objs;\r\n\
objs[0] = &test;\r\n\
objs[1] = &test.i;\r\n\
for(i in objs)\r\n\
{\r\n\
	i.add();\r\n\
	i.add(2);\r\n\
}\r\n\
\r\n\
return sum;";
TEST_RESULT_SIMPLE("auto ref type function call 1", testAutoRefCall1, "32");

const char	*testAutoRefCall2 =
"import std.list;\r\n\
import std.math;\r\n\
void int:rgba(int r, g, b, a)\r\n\
{\r\n\
	*this = (a << 24) | (r << 16) | (g << 8) | b;\r\n\
}\r\n\
int int.r(){ return (*this >> 16) & 0xff; }\r\n\
int int.g(){ return (*this >> 8) & 0xff; }\r\n\
int int.b(){ return *this & 0xff; }\r\n\
int int.a(){ return (*this >> 24) & 0xff; }\r\n\
\r\n\
// 2D point\r\n\
class Point\r\n\
{\r\n\
	int x, y;\r\n\
}\r\n\
Point Point(int x, y)	// Constructor\r\n\
{\r\n\
	Point ret;\r\n\
	ret.x = x;\r\n\
	ret.y = y;\r\n\
	return ret;\r\n\
}\r\n\
\r\n\
// Shapes\r\n\
class Line\r\n\
{\r\n\
	Point a, b;\r\n\
	int color;\r\n\
	void manhettan(int ref p)\r\n\
	{\r\n\
		*p += abs(a.x - b.x) + abs(a.y - b.y);\r\n\
	}\r\n\
}\r\n\
class Triangle\r\n\
{\r\n\
	Point a, b, c;\r\n\
	int color;\r\n\
	void manhettan(int ref p)\r\n\
	{\r\n\
		*p += abs(a.x - b.x) + abs(a.y - b.y);\r\n\
		*p += abs(b.x - c.x) + abs(b.y - c.y);\r\n\
		*p += abs(a.x - c.x) + abs(a.y - c.y);\r\n\
	}\r\n\
}\r\n\
class Polygon\r\n\
{\r\n\
	Point[] points;\r\n\
	int color;\r\n\
	void manhettan(int ref p)\r\n\
	{\r\n\
		if(points.size < 2)\r\n\
			return;\r\n\
		for(int i = 0; i < points.size - 1; i++)\r\n\
			*p += abs(points[i].x - points[i+1].x) + abs(points[i].y - points[i+1].y);\r\n\
		*p += abs(points[0].x - points[points.size-1].x) + abs(points[0].y - points[points.size-1].y);\r\n\
	}\r\n\
}\r\n\
\r\n\
// Create line\r\n\
Line l;\r\n\
l.a = Point(20, 20);\r\n\
l.b = Point(200, 40);\r\n\
l.color.rgba(255, 0, 0, 255);\r\n\
\r\n\
// Create triangle\r\n\
Triangle t;\r\n\
t.a = Point(150, 100);\r\n\
t.b = Point(200, 400);\r\n\
t.c = Point(100, 400);\r\n\
t.color.rgba(128, 255, 0, 255);\r\n\
\r\n\
// Create polygon\r\n\
Polygon p;\r\n\
p.points = { Point(40, 50), Point(30, 20), Point(40, 80), Point(140, 300), Point(600, 20) };\r\n\
p.color.rgba(255, 50, 128, 255);\r\n\
\r\n\
// Create a list of shapes\r\n\
list<auto ref> shapes;\r\n\
shapes.push_back(l);\r\n\
shapes.push_back(t);\r\n\
shapes.push_back(p);\r\n\
\r\n\
int mp = 0;\r\n\
for(i in shapes)\r\n\
	i.manhettan(&mp);\r\n\
\r\n\
return mp;";
TEST_RESULT("auto ref type function call 2", testAutoRefCall2, "2760");

const char	*testFunctionCallThroughAutoRefInMemberFunction =
"int int:foo(){ return -*this; }\r\n\
class Foo\r\n\
{\r\n\
	auto()\r\n\
	{\r\n\
		auto ref x;\r\n\
		x.foo();\r\n\
	}\r\n\
}\r\n\
return 1;";
TEST_RESULT("Function call through 'auto ref' in member function", testFunctionCallThroughAutoRefInMemberFunction, "1");

const char	*testIndirectCallInMemberFunction =
"int foo(){ return -1; }\r\n\
auto bar(){ return foo; }\r\n\
class Foo\r\n\
{\r\n\
	auto()\r\n\
	{\r\n\
		bar()();\r\n\
	}\r\n\
}\r\n\
return 1;";
TEST_RESULT("Indirect function call in member function", testIndirectCallInMemberFunction, "1");

LOAD_MODULE(test_autorefcall, "test.autorefcall", "class Proxy{ int foo(int x){ return x; } } int rc(auto ref z){ return z.foo(5); }");
const char *testIndirectCallModules =
"import test.autorefcall;\r\n\
class Bar{ int u; }\r\n\
int Bar:foo(int x){ return x + u; }\r\n\
Bar m; m.u = 10;\r\n\
return rc(m);";
TEST_RESULT("Function call through 'auto ref', more types defined later", testIndirectCallModules, "15");

const char	*testAutorefSelectionImprovement =
"class Foo\r\n\
{\r\n\
	int boo(auto ref[] x){ return x.size; }\r\n\
}\r\n\
auto ref x = new Foo;\r\n\
return x.boo(1, 2, 3) + x.boo(1, 2);";
TEST_RESULT("Function call through 'auto ref', selection of a variable argument function", testAutorefSelectionImprovement, "5");

const char	*testAutorefCallIssue1 =
"class Test\r\n\
{\r\n\
	int a(int x){ return -x; }\r\n\
	int b(int x, int y){ return x + y; }\r\n\
	void c(){}\r\n\
	int d(int x, int y, int z){ return x + y * z; }\r\n\
}\r\n\
Test x;\r\n\
auto ref u = x;\r\n\
u.a(2);\r\n\
u.b(2, 3);\r\n\
u.c();\r\n\
return u.d(2, 3, 4);";
TEST_RESULT("Function call through 'auto ref', issue with incorrect vtbl search", testAutorefCallIssue1, "14");

const char	*testAutorefPtr1 =
"class Foo{ int foo(int a){ return -a; } }\r\n\
class Bar{}\r\n\
auto x = new Foo;\r\n\
auto ref y = x;\r\n\
auto z = y.foo;\r\n\
return z != nullptr ? z(5) : 3;";
TEST_RESULT("Getting function pointer from 'auto ref'", testAutorefPtr1, "-5");

const char	*testAutorefPtr2 =
"class Foo{ int foo(int a){ return -a; } }\r\n\
class Bar{}\r\n\
auto x = new Bar;\r\n\
auto ref y = x;\r\n\
auto z = y.foo;\r\n\
return z != nullptr ? z(5) : 3;";
TEST_RESULT("Getting function pointer from 'auto ref' 2", testAutorefPtr2, "3");

const char	*testAutorefCallIssue2 =
"class Test{ auto foo(generic i){ return -i; } }\r\n\
class Bar{ auto foo(generic i){ return i * 2; } }\r\n\
\r\n\
Test a;\r\n\
a.foo(4);\r\n\
\r\n\
auto ref x = new Bar;\r\n\
return x.foo(4);";
TEST_RESULT("Getting function call through 'auto ref' issue", testAutorefCallIssue2, "8");

const char	*testAutorefCallIssue3 =
"void foo()\r\n\
{\r\n\
	class Foo{}\r\n\
	int Foo:F(){ return 1; }\r\n\
}\r\n\
\r\n\
auto bar()\r\n\
{\r\n\
	class Foo{}\r\n\
	auto Foo:F(){ return 123; }\r\n\
	\r\n\
	Foo x;\r\n\
	auto ref y = x;\r\n\
	return y.F();\r\n\
}\r\n\
\r\n\
return bar();";
TEST_RESULT("Getting function call through 'auto ref' issue", testAutorefCallIssue3, "123");

const char	*testAutorefCallIssue4 =
"class F\r\n\
{\r\n\
	int f(int ref() x)\r\n\
	{\r\n\
		return -x();\r\n\
	}\r\n\
}\r\n\
auto ref x = F();\r\n\
return x.f(<>{ 2; });";
TEST_RESULT("Short inline function in 'auto ref' call", testAutorefCallIssue4, "-2");

LOAD_MODULE(test_autorefmerge1, "test.autorefmerge1", "class Test{ int f(int a, b){ return a + b; } }");
LOAD_MODULE(test_autorefmerge2, "test.autorefmerge2", "class Temp{ int f(int a, b){ return 0; } } int call(auto ref x){ return x.f(1, 2); }");
const char	*testAutorefIntroduceTypeToFunction =
"import test.autorefmerge1;\r\n\
import test.autorefmerge2;\r\n\
Test t;\r\n\
return call(t);";
TEST_RESULT("Imported type gets registered for imported 'auto ref' function call", testAutorefIntroduceTypeToFunction, "3");


LOAD_MODULE(test_autoreforder1, "test.autoreforder1", "class Test extendable{ int f(int a, b){ return a + b; } }");
LOAD_MODULE(test_autoreforder2, "test.autoreforder2", "import test.autoreforder1; class Test2 : Test{ int f(int a, b){ return a - b; } }");
const char	*testAutorefInitOrder =
"import test.autoreforder2;\r\n\
Test2 t;\r\n\
auto ref x = t;\r\n\
return t.f(4, 2); ";
TEST_RESULT("Order of 'auto ref' table setup on implicit import", testAutorefInitOrder, "2");
