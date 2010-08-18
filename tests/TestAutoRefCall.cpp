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
TEST_RESULT("auto ref type function call 1", testAutoRefCall1, "32");

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
list shapes = list();\r\n\
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
