#include "TestBase.h"

const char	*testListComprehension =
"import std.range;\r\n\
\r\n\
double[] xd = { for(i in range(1,30)) yield double(i); };\r\n\
float[] xf = { for(i in range(1,30)) yield float(i); };\r\n\
long[] xl = { for(i in range(1,30)) yield long(i); };\r\n\
int[] xi = { for(i in range(1,30)) yield int(i); };\r\n\
short[] xs = { for(i in range(1,30)) yield short(i); };\r\n\
char[] xc = { for(i in range(1,30)) yield char(i); };\r\n\
class Int\r\n\
{\r\n\
	int i;\r\n\
}\r\n\
Int Int(int x){ Int a; a.i = x; return a; }\r\n\
Int[] xI = { for(i in range(1,30)) yield Int(i); };\r\n\
class BigInt\r\n\
{\r\n\
	int i;\r\n\
	float x, y;\r\n\
}\r\n\
BigInt BigInt(int x){ BigInt a; a.i = x; return a; }\r\n\
BigInt[] xBI = { for(i in range(1,30)) yield BigInt(i); };\r\n\
int good = 1;\r\n\
void CHECK(double a, int b){ if(a != b) good = 0; }\r\n\
void CHECK(float a, int b){ if(a != b) good = 0; }\r\n\
void CHECK(long a, int b){ if(a != b) good = 0; }\r\n\
void CHECK(int a, int b){ if(a != b) good = 0; }\r\n\
void CHECK(short a, int b){ if(a != b) good = 0; }\r\n\
void CHECK(char a, int b){ if(a != b) good = 0; }\r\n\
for(int i = 0; i < 30; i++)\r\n\
{\r\n\
	CHECK(xd[i], i+1);\r\n\
	CHECK(xf[i], i+1);\r\n\
	CHECK(xl[i], i+1);\r\n\
	CHECK(xi[i], i+1);\r\n\
	CHECK(xs[i], i+1);\r\n\
	CHECK(xc[i], i+1);\r\n\
	CHECK(xI[i].i, i+1);\r\n\
	CHECK(xBI[i].i, i+1);\r\n\
}\r\n\
return good;";
TEST_RESULT("List comprehension test", testListComprehension, "1");

const char *testListComprehensionEmpty1 = "import std.range; class Empty{} auto fail = { for(i in range(1,5)){ Empty e; yield e; } }; return 1;";
TEST_RESULT("List comprehension test with empty class 1", testListComprehensionEmpty1, "1");

const char *testListComprehensionEmpty2 = "import std.range; class Empty{} auto fail = { for(i in range(1,5)){ Empty e; int a; yield e; } }; return 1;";
TEST_RESULT("List comprehension test with empty class 2", testListComprehensionEmpty2, "1");

const char *testListComprehensionEmpty3 = "coroutine void fail(){while(0){}return;} fail(); return 1;";
TEST_RESULT("List comprehension test with empty class 3", testListComprehensionEmpty3, "1");

LOAD_MODULE(test_list_comp1, "test.list_comp1", "import std.range; auto a = { for(i in range(4, 10)) yield i; };");
LOAD_MODULE(test_list_comp2, "test.list_comp2", "import std.range; auto b = { for(i in range(2, 4)) yield i; };");
const char	*testListComprehensionCollision =
"import test.list_comp1;\r\n\
import test.list_comp2;\r\n\
int sum = 0;\r\n\
for(i in a, j in b) sum += i * j;\r\n\
return sum;";
TEST_RESULT("List comprehension helper function collision resolve", testListComprehensionCollision, "47");

const char	*testListComprehensionReturn =
"int i = 10; auto x = { for(;0;){} if(--i) return 1; yield 15; }; return x.size;";
TEST_RESULT("List comprehension with explicit return", testListComprehensionReturn, "0");

const char	*testListComprehensionRef =
"class Foo{ int a; }\r\n\
void Foo::Foo(int x){ a = x; }\r\n\
auto x = { for(int i=0;i<4;i++) yield new Foo(10+i); };\r\n\
int z = 0; for(i in x) z += i.a;\r\n\
return z;";
TEST_RESULT("List comprehension for an array of references", testListComprehensionRef, "46");
