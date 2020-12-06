#include "TestBase.h"

const char	*testNamespace0 =
"namespace Test{}\r\n\
return 1;";
TEST_RESULT("namespace test 0", testNamespace0, "1");

const char	*testNamespace1 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 12; }\r\n\
}\r\n\
return Test.foo();";
TEST_RESULT("namespace test 1", testNamespace1, "12");

const char	*testNamespace2 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 12; }\r\n\
	int bar(){ return foo(); }\r\n\
}\r\n\
return Test.bar();";
TEST_RESULT("namespace test 2", testNamespace2, "12");

const char	*testNamespace3 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 12; }\r\n\
	namespace Nested\r\n\
	{\r\n\
		int bar(){ return foo(); }\r\n\
	}\r\n\
	int bar(){ return foo(); }\r\n\
}\r\n\
return Test.bar() + Test.Nested.bar();";
TEST_RESULT("namespace test 3", testNamespace3, "24");

const char	*testNamespace4 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 12; }\r\n\
	namespace Nested\r\n\
	{\r\n\
		int foo(){ return 3; }\r\n\
		int bar(){ return foo(); }\r\n\
	}\r\n\
	int bar(){ return foo(); }\r\n\
}\r\n\
return Test.bar() + Test.Nested.bar();";
TEST_RESULT("namespace test 4", testNamespace4, "15");

const char	*testNamespace5 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 12; }\r\n\
}\r\n\
namespace Test\r\n\
{\r\n\
	int bar(){ return 5 + foo(); }\r\n\
}\r\n\
return Test.foo() + Test.bar();";
TEST_RESULT("namespace test 5", testNamespace5, "29");

const char	*testNamespace6 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 12; }\r\n\
}\r\n\
namespace Test\r\n\
{\r\n\
	namespace Nested\r\n\
	{\r\n\
		int bar(){ return foo(); }\r\n\
	}\r\n\
}\r\n\
return Test.Nested.bar();";
TEST_RESULT("namespace test 6", testNamespace6, "12");

const char	*testNamespace7 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 12; }\r\n\
}\r\n\
namespace Test\r\n\
{\r\n\
	namespace Nested\r\n\
	{\r\n\
		int foo(){ return 0; }\r\n\
		int bar(){ return Test.foo(); }\r\n\
	}\r\n\
}\r\n\
return Test.Nested.bar();";
TEST_RESULT("namespace test 7", testNamespace7, "12");

const char	*testNamespace8 =
"namespace Test\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int a;\r\n\
	}\r\n\
}\r\n\
Test.Foo x;\r\n\
x.a = 10;\r\n\
return x.a + sizeof(Test.Foo);";
TEST_RESULT("namespace test 8", testNamespace8, "14");

const char	*testNamespace9 =
"namespace Test\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int a;\r\n\
	}\r\n\
	namespace Nested\r\n\
	{\r\n\
		class Foo\r\n\
		{\r\n\
			double z;\r\n\
		}\r\n\
	}\r\n\
}\r\n\
Test.Foo x;\r\n\
Test.Nested.Foo y;\r\n\
return sizeof(x) + sizeof(y);";
TEST_RESULT("namespace test 9", testNamespace9, "12");

const char	*testNamespace10 =
"namespace Test\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int a;\r\n\
	}\r\n\
	namespace Nested\r\n\
	{\r\n\
		class Foo\r\n\
		{\r\n\
			double z;\r\n\
		}\r\n\
	}\r\n\
}\r\n\
class Foo\r\n\
{\r\n\
	int[25] xx;\r\n\
}\r\n\
Foo x;\r\n\
Test.Foo y;\r\n\
Test.Nested.Foo z;\r\n\
return sizeof(x) + sizeof(y) + sizeof(z);";
TEST_RESULT("namespace test 10", testNamespace10, "112");

const char	*testNamespace11 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 24; }\r\n\
}\r\n\
namespace Bar\r\n\
{\r\n\
	namespace Test\r\n\
	{\r\n\
		int foo(){ return 12; }\r\n\
	}\r\n\
}\r\n\
return Test.foo();";
TEST_RESULT("namespace test 11", testNamespace11, "24");

const char	*testNamespace12 =
"namespace Test\r\n\
{\r\n\
	class Foo{ int x; }\r\n\
	namespace Nested\r\n\
	{\r\n\
		class Foo{ int y, z; }\r\n\
	}\r\n\
}\r\n\
typeid a = Test.Foo;\r\n\
return a != Test.Nested.Foo;";
TEST_RESULT("namespace test 12", testNamespace12, "1");

const char	*testNamespace13 =
"namespace Test\r\n\
{\r\n\
	class Foo{ int z; }\r\n\
\r\n\
	Foo x;\r\n\
}\r\n\
Test.x.z = 5;\r\n\
\r\n\
return Test.x.z;";
TEST_RESULT("namespace test 13", testNamespace13, "5");

const char	*testNamespace14 =
"namespace Test\r\n\
{\r\n\
	class Foo{ int z; }\r\n\
\r\n\
	Foo x;\r\n\
	x.z = 5;\r\n\
}\r\n\
return Test.x.z;";
TEST_RESULT("namespace test 14", testNamespace14, "5");

const char	*testNamespace15 =
"namespace Test\r\n\
{\r\n\
	class Foo{ int z; void Foo(){ z = 14; } }\r\n\
\r\n\
	Foo x;\r\n\
}\r\n\
return Test.x.z;";
TEST_RESULT("namespace test 15", testNamespace15, "14");

const char	*testNamespace16 =
"namespace Test\r\n\
{\r\n\
	class Foo{ int z; void Foo(){ z = 14; } }\r\n\
}\r\n\
Test.Foo x;\r\n\
return x.z;";
TEST_RESULT("namespace test 16", testNamespace16, "14");

const char	*testNamespace17 =
"namespace Test\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int a;\r\n\
	}\r\n\
	int operator+(Foo a, b){ return a.a + b.a; }\r\n\
	Foo z, w;\r\n\
	z.a = 300;\r\n\
	w.a = 7;\r\n\
}\r\n\
Test.Foo x, y; x.a = 10; y.a = 20;\r\n\
return (x + y) + (Test.z + Test.w);";
TEST_RESULT("namespace test 17", testNamespace17, "337");

const char	*testNamespace18 =
"namespace Test\r\n\
{\r\n\
	class Foo{ int z; void Foo(int x){ z = x; } }\r\n\
\r\n\
	Foo x;\r\n\
}\r\n\
Test.Foo a = Test.Foo(4);\r\n\
return a.z;";
TEST_RESULT("namespace test 18", testNamespace18, "4");

const char	*testNamespace19 =
"namespace Test\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int x;\r\n\
		void Foo(){ x = 3; }\r\n\
		int bar(int y){ return x + y; }\r\n\
	}\r\n\
}\r\n\
Test.Foo a;\r\n\
return a.bar(3);";
TEST_RESULT("namespace test 19", testNamespace19, "6");

const char	*testNamespace20 =
"namespace Test\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int x;\r\n\
		void Foo(){ x = 3; }\r\n\
		void Foo(int y){ x = y; }\r\n\
		int bar(int y){ return x + y; }\r\n\
	}\r\n\
}\r\n\
Test.Foo a, b = Test.Foo(6);\r\n\
return a.bar(3) + b.bar(30);";
TEST_RESULT("namespace test 20", testNamespace20, "42");

const char	*testNamespace21 =
"namespace Test\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int x;\r\n\
		void Foo(){ x = 3; }\r\n\
		void Foo(int y){ x = y; }\r\n\
		int bar(int y){ return x + y; }\r\n\
	}\r\n\
}\r\n\
Test.Foo ref a = new Test.Foo, b = new Test.Foo(), c = new Test.Foo(100);\r\n\
return a.bar(3) + b.bar(30) + c.bar(400);";
TEST_RESULT("namespace test 21", testNamespace21, "539");

const char	*testNamespace22 =
"namespace Test\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int x;\r\n\
		void Foo(){ x = 3; }\r\n\
		void Foo(int y){ x = y; }\r\n\
		int bar(int y){ return x + y; }\r\n\
	}\r\n\
	Foo ref a = new Foo, b = new Foo(), c = new Foo(100);\r\n\
}\r\n\
return Test.a.bar(3) + Test.b.bar(30) + Test.c.bar(400);";
TEST_RESULT("namespace test 22", testNamespace22, "539");

const char	*testNamespace23 =
"namespace Test\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int x;\r\n\
		void Foo(){ x = 3; }\r\n\
		void Foo(int y){ x = y; }\r\n\
		int bar(int y){ return x + y; }\r\n\
	}\r\n\
	Foo a, b = Foo(), c = Foo(100);\r\n\
}\r\n\
return Test.a.bar(3) + Test.b.bar(30) + Test.c.bar(400);";
TEST_RESULT("namespace test 23", testNamespace23, "539");

const char	*testNamespace24 =
"namespace Test\r\n\
{\r\n\
	auto foo(generic x){ return -x; }\r\n\
\r\n\
	int y = foo(5);\r\n\
	auto z = foo(4.5);\r\n\
}\r\n\
return int(Test.y * Test.z);";
TEST_RESULT("namespace test 24", testNamespace24, "22");

const char	*testNamespace25 =
"namespace Test\r\n\
{\r\n\
	auto foo(generic x){ return -x; }\r\n\
}\r\n\
return Test.foo(5);";
TEST_RESULT("namespace test 25", testNamespace25, "-5");

const char	*testNamespace26 =
"namespace Test\r\n\
{\r\n\
	auto foo(generic v, x, y){ return v + x + y; }\r\n\
	int a, b;\r\n\
	a = foo(a, 0, 1023);\r\n\
	b = foo(b, 0, 1023);\r\n\
}\r\n\
return Test.a + Test.b;";
TEST_RESULT("namespace test 26", testNamespace26, "2046");

const char	*testNamespace27 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 5; }\r\n\
	class Bar\r\n\
	{\r\n\
		int foo(){ return 10; }\r\n\
		int bar(){ return Test.foo(); }\r\n\
	}\r\n\
}\r\n\
Test.Bar x;\r\n\
return x.bar();";
TEST_RESULT("namespace test 27", testNamespace27, "5");

LOAD_MODULE(test_namespace, "test.namespace1", "namespace Test{ int foo(int a){ return 3 * a; } }");
const char	*testNamespace28 =
"import test.namespace1;\r\n\
return Test.foo(5);";
TEST_RESULT("namespace test 28", testNamespace28, "15");

const char	*testNamespace29 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 12; }\r\n\
	class Foo\r\n\
	{\r\n\
		int bar(){ return foo(); }\r\n\
	}\r\n\
	Foo x;\r\n\
	int a = x.bar();\r\n\
}\r\n\
Test.Foo y;\r\n\
return Test.a + y.bar();";
TEST_RESULT("namespace test 29", testNamespace29, "24");

const char	*testNamespace30 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 5; }\r\n\
}\r\n\
auto x = Test.foo;\r\n\
return x();";
TEST_RESULT("namespace test 30", testNamespace30, "5");

const char	*testNamespace31 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 5; }\r\n\
	auto x = foo;\r\n\
}\r\n\
return Test.x();";
TEST_RESULT("namespace test 31", testNamespace31, "5");

const char	*testNamespace32 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 5; }\r\n\
	auto x = foo;\r\n\
	auto y = x();\r\n\
}\r\n\
return Test.y;";
TEST_RESULT("namespace test 32", testNamespace32, "5");

const char	*testNamespace33 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 5; }\r\n\
	int foo(int x){ return 5 + x; }\r\n\
}\r\n\
int ref(int) x = Test.foo;\r\n\
return x(2);";
TEST_RESULT("namespace test 33", testNamespace33, "7");

const char	*testNamespace34 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 5; }\r\n\
	int foo(int x){ return 5 + x; }\r\n\
	int ref(int) x = foo;\r\n\
}\r\n\
return Test.x(2);";
TEST_RESULT("namespace test 34", testNamespace34, "7");

const char	*testNamespace35 =
"namespace Test\r\n\
{\r\n\
	int foo(){ return 5; }\r\n\
	int foo(int x){ return 5 + x; }\r\n\
	int ref(int) x = foo;\r\n\
	auto y = x(2);\r\n\
}\r\n\
return Test.y;";
TEST_RESULT("namespace test 35", testNamespace35, "7");

const char	*testNamespace36 =
"namespace Test\r\n\
{\r\n\
	int foo(int ref(int, int) f){ return f(2, 3); }\r\n\
}\r\n\
return Test.foo(<i, j>{ i + j; });";
TEST_RESULT("namespace test 36", testNamespace36, "5");

const char	*testNamespace37 =
"namespace Test\r\n\
{\r\n\
	int foo(int ref(int, int) f){ return f(2, 3); }\r\n\
	auto x = foo(<i, j>{ i + j; });\r\n\
}\r\n\
return Test.x;";
TEST_RESULT("namespace test 37", testNamespace37, "5");

const char	*testNamespace38 =
"namespace Test\r\n\
{\r\n\
	int foo(int ref(int, int) f){ return f(2, 3); }\r\n\
	auto x = foo;\r\n\
}\r\n\
return Test.x(<i, j>{ i + j; });";
TEST_RESULT("namespace test 38", testNamespace38, "5");

const char	*testNamespace39 =
"namespace Test\r\n\
{\r\n\
	auto ref x = 5;\r\n\
	int y = int(x);\r\n\
}\r\n\
return Test.y;";
TEST_RESULT("namespace test 39", testNamespace39, "5");

const char	*testNamespace40 =
"namespace Test\r\n\
{\r\n\
	class Foo{ int x; void Foo(int y){ x = y; } }\r\n\
	auto ref x = Foo(5);\r\n\
	int y = Foo(x).x;\r\n\
}\r\n\
return Test.y;";
TEST_RESULT("namespace test 40", testNamespace40, "5");

const char	*testNamespace41 =
"namespace Test\r\n\
{\r\n\
	class Foo{ int x; void Foo(int y){ x = y; } }\r\n\
	auto ref x = Foo(5);\r\n\
}\r\n\
return Test.Foo(Test.x).x;";
TEST_RESULT("namespace test 41", testNamespace41, "5");

const char	*testNamespace42 =
"namespace Test\r\n\
{\r\n\
	class Foo{ int x; }\r\n\
	int foo(typeid x){ return x == Foo; }\r\n\
}\r\n\
return Test.foo(Test.Foo);";
TEST_RESULT("namespace test 42", testNamespace42, "1");

const char	*testNamespace43 =
"namespace Test\r\n\
{\r\n\
	int foo(int x){ return 3 * x; }\r\n\
}\r\n\
return Test.foo(Test.foo(5));";
TEST_RESULT("namespace test 43", testNamespace43, "45");

const char	*testNamespace44 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	Foo<T> ref next;\r\n\
}\r\n\
namespace Test\r\n\
{\r\n\
	Foo<bool> h;\r\n\
	return h.x;\r\n\
}";
TEST_RESULT("namespace test 44", testNamespace44, "0");

const char	*testNamespace45 =
"class Foo<T>{ T x; }\r\n\
auto operator[](Foo<@T> ref x, int a){ return &x.x; }\r\n\
namespace Test\r\n\
{\r\n\
	Foo<Foo<bool>> h;\r\n\
	h[5][4] = true;\r\n\
}\r\n\
return Test.h[5][4];";
TEST_RESULT("namespace test 45 (global generic operator instance inside a namespace)", testNamespace45, "1");

const char	*testNamespace46 =
"auto foo(generic x){ return -x; }\r\n\
namespace Test\r\n\
{\r\n\
	int ref(int) a = foo;\r\n\
}\r\n\
int ref(int) a = foo;\r\n\
return Test.a == a;";
TEST_RESULT("namespace test 46 (generic function should be instanced in a correct namespace)", testNamespace46, "1");

const char	*testNamespace47 =
"namespace Foo\r\n\
{\r\n\
	auto foo(generic x){ return -x; }\r\n\
}\r\n\
namespace Test\r\n\
{\r\n\
	int ref(int) a = Foo.foo;\r\n\
}\r\n\
int ref(int) a = Foo.foo;\r\n\
return Test.a == a;";
TEST_RESULT("namespace test 47 (generic function should be instanced in a correct namespace)", testNamespace47, "1");

const char	*testNamespace48 =
"namespace Foo\r\n\
{\r\n\
	int bar(int x){ return -x; }\r\n\
	auto foo(generic x){ return bar(x); }\r\n\
}\r\n\
namespace Test\r\n\
{\r\n\
	int bar(int x){ return 1; }\r\n\
	int ref(int) a = Foo.foo;\r\n\
}\r\n\
return Test.a(2);";
TEST_RESULT("namespace test 48", testNamespace48, "-2");

const char	*testNamespace49 =
"namespace Foo\r\n\
{\r\n\
	int bar(int x){ return -x; }\r\n\
	auto foo(generic x){ return bar(x); }\r\n\
}\r\n\
namespace Test\r\n\
{\r\n\
	int bar(int x){ return 1; }\r\n\
	int ref(int) a;\r\n\
	{\r\n\
		int ref(int) b = Foo.foo;\r\n\
		a = b;\r\n\
	}\r\n\
}\r\n\
return Test.a(2);";
TEST_RESULT("namespace test 49", testNamespace49, "-2");

const char	*testNamespace50 =
"namespace Test\r\n\
{\r\n\
	class Foo<T>{ T x; }\r\n\
	int bar(int x){ return x * 2; }\r\n\
	int Foo:foo(){ return bar(5); }\r\n\
}\r\n\
namespace Bar\r\n\
{\r\n\
	Test.Foo<int> a;\r\n\
}\r\n\
return Bar.a.foo();";
TEST_RESULT("namespace test 50", testNamespace50, "10");

const char	*testNamespace51 =
"namespace Test\r\n\
{\r\n\
	int bar(int x){ return x * 2; }\r\n\
	class Foo<T>\r\n\
	{\r\n\
		T x;\r\n\
		int foo(){ return bar(5); }\r\n\
	}\r\n\
}\r\n\
namespace Bar\r\n\
{\r\n\
	Test.Foo<int> a;\r\n\
}\r\n\
return Bar.a.foo();";
TEST_RESULT("namespace test 51", testNamespace51, "10");

LOAD_MODULE(test_namespace2, "test.namespace2", "namespace Foo{ auto foo(generic x){ return -x; } }");
const char	*testNamespace52 =
"import test.namespace2;\r\n\
namespace Test\r\n\
{\r\n\
	int ref(int) a = Foo.foo;\r\n\
}\r\n\
int ref(int) a = Foo.foo;\r\n\
return Test.a == a;";
TEST_RESULT("namespace test 52", testNamespace52, "1");

LOAD_MODULE(test_namespace3, "test.namespace3", "namespace Test{ int bar(int x){ return x * 2; } class Foo<T>{ T x; int foo(){ return bar(5); } } }");
const char	*testNamespace53 =
"import test.namespace3;\r\n\
namespace Bar\r\n\
{\r\n\
	Test.Foo<int> a;\r\n\
}\r\n\
return Bar.a.foo();";
TEST_RESULT("namespace test 53", testNamespace53, "10");

const char	*testNamespace54 =
"namespace Test\r\n\
{\r\n\
	int g1 = 3;\r\n\
	int r;\r\n\
	void glob()\r\n\
	{\r\n\
		int g2 = 5;\r\n\
		int test(int x, int rek)\r\n\
		{\r\n\
			int g3 = 7;\r\n\
			int help(int x)\r\n\
			{\r\n\
				int hell = 0;\r\n\
				if(x > 10)\r\n\
					hell = test(2, 0);\r\n\
				return g3*x + g2*g1 + hell;\r\n\
			}\r\n\
			if(rek)\r\n\
				return help(x) * g2 + g1;\r\n\
			return help(x) * test(g1, 1);\r\n\
		}\r\n\
		r = test(13, 0);\r\n\
	}\r\n\
	glob();\r\n\
}\r\n\
return Test.r;";
TEST_RESULT("namespace test 54", testNamespace54, "990579");

const char	*testNamespace55 =
"namespace A.B.C\r\n\
{\r\n\
	int x = 10;\r\n\
}\r\n\
int y = 5;\r\n\
return A.B.C.x + y;";
TEST_RESULT("namespace test 55 (short-hand nested namespace definition)", testNamespace55, "15");

const char	*testNamespace56 =
"class Canvas{ int x; }\r\n\
namespace Test\r\n\
{\r\n\
	class Device{ Canvas ref canvas; }\r\n\
	void Device:Device( Canvas ref canvas )\r\n\
	{\r\n\
		this.canvas = canvas;\r\n\
	}\r\n\
}\r\n\
Canvas c;\r\n\
Test.Device d = Test.Device(c);\r\n\
return d.canvas != nullptr;";
TEST_RESULT("namespace test 56", testNamespace56, "1");

const char	*testNamespace57 =
"namespace Test\r\n\
{\r\n\
	int foo(int x){ return x * 5; }\r\n\
}\r\n\
return Test.foo(-1);";
TEST_RESULT("namespace test 57", testNamespace57, "-5");

const char	*testNamespace58 =
"namespace Test\r\n\
{\r\n\
	int foo(int ref(int, int) x, y){ return x(2, 3) + y(20, 30); }\r\n\
}\r\n\
return Test.foo(<i, j>{ i + j; }, <i, j>{ i + j; });";
TEST_RESULT("namespace test 58", testNamespace58, "55");

const char	*testNamespace59 =
"namespace Foo\r\n\
{\r\n\
	coroutine int foo()\r\n\
	{\r\n\
		return 10;\r\n\
	}\r\n\
}\r\n\
auto x = Foo.foo;\r\n\
return x();";
TEST_RESULT("namespace test 59 (coroutine)", testNamespace59, "10");

LOAD_MODULE(test_namespace60, "test.namespace60", "namespace A.B{ void foo(){} }");
const char	*testNamespace60 =
"import test.namespace60;\r\n\
A.B.foo();\r\n\
return 1;";
TEST_RESULT("namespace test 60", testNamespace60, "1");

const char	*testNamespace61 =
"class X{}\r\n\
int X.pos(){ return 10; }\r\n\
\r\n\
namespace B\r\n\
{\r\n\
	int bar(int ref x){ return -*x; }\r\n\
}\r\n\
X c;\r\n\
return B.bar(c.pos);";
TEST_RESULT("namespace test 61", testNamespace61, "-10");

const char	*testNamespace62 =
"class X{}\r\n\
int X:pos(){ return 10; }\r\n\
\r\n\
namespace B\r\n\
{\r\n\
	int bar(int ref x){ return -*x; }\r\n\
}\r\n\
X c;\r\n\
return B.bar(c.pos());";
TEST_RESULT("namespace test 62", testNamespace62, "-10");

const char	*testNamespace63 =
"class Bar{}\r\n\
int operator+(Bar a, b){ return 12; }\r\n\
namespace X\r\n\
{\r\n\
	int foo(int x){ return -x; }\r\n\
}\r\n\
Bar a, b;\r\n\
return X.foo(a + b);";
TEST_RESULT("namespace test 63", testNamespace63, "-12");

const char	*testNamespace64 =
"namespace X{ void foo(char[] x){} }\r\n\
X.foo({\"brgrg\", \"brgrg_\"}[0]);\r\n\
return 1;";
TEST_RESULT("namespace test 64", testNamespace64, "1");

const char	*testNamespace65 =
"namespace Bar.Test\r\n\
{\r\n\
	auto foo(generic x){ return -x; }\r\n\
\r\n\
	int y = foo(5);\r\n\
	auto z = foo(4.5);\r\n\
}\r\n\
return int(Bar.Test.y * Bar.Test.z);";
TEST_RESULT("namespace test 65", testNamespace65, "22");

const char	*testNamespace66 =
"namespace A\r\n\
{\r\n\
	namespace A{ int x = 1; }\r\n\
	int x = 2;\r\n\
	int a = A.A.x;\r\n\
}\r\n\
return A.a;";
TEST_RESULT("namespace test 66", testNamespace66, "1");

const char	*testNamespace67 =
"namespace A\r\n\
{\r\n\
	namespace A\r\n\
	{\r\n\
		int x = 2;\r\n\
		\r\n\
		int u1 = x;\r\n\
		int u2 = A.x;\r\n\
		int u3 = A.A.x;\r\n\
		\r\n\
		x += 4;\r\n\
		A.x += 20;\r\n\
		A.A.x += 500;\r\n\
	}\r\n\
	\r\n\
	A.x += 3000;\r\n\
	A.A.x += 60000;\r\n\
}\r\n\
\r\n\
A.A.x += 10000;\r\n\
return A.A.x;";
TEST_RESULT("namespace test 67", testNamespace67, "73526");

const char	*testNamespace68 =
"namespace A\r\n\
{\r\n\
	namespace B\r\n\
	{\r\n\
		namespace C\r\n\
		{\r\n\
			int x = 2;\r\n\
			\r\n\
			int u1 = x;\r\n\
			int u2 = C.x;\r\n\
			int u3 = B.C.x;\r\n\
			int u4 = A.B.C.x;\r\n\
			\r\n\
			x += 1;\r\n\
			C.x += 30;\r\n\
			B.C.x += 200;\r\n\
			A.B.C.x += 1000;\r\n\
		}\r\n\
		\r\n\
		C.x += 4;\r\n\
		B.C.x += 20;\r\n\
		A.B.C.x += 500;\r\n\
	}\r\n\
	\r\n\
	B.C.x += 3000;\r\n\
	A.B.C.x += 60000;\r\n\
}\r\n\
\r\n\
A.B.C.x += 10000;\r\n\
return A.B.C.x;";
TEST_RESULT("namespace test 68", testNamespace68, "74757");

const char	*testNamespace69 =
"namespace Runner{\r\n\
	int x = 10;\r\n\
\r\n\
	int foo()\r\n\
	{\r\n\
		int x = 5;\r\n\
		return x;\r\n\
	}\r\n\
\r\n\
	return foo();\r\n\
}";
TEST_RESULT("namespace test 69 (local scope priority)", testNamespace69, "5");

const char	*testNamespace70 =
"int x = 10;\r\n\
\r\n\
namespace A\r\n\
{\r\n\
	int x = 5;\r\n\
\r\n\
	namespace B\r\n\
	{\r\n\
		return x;\r\n\
	}\r\n\
}";
TEST_RESULT("namespace test 70 (namespace scope priority)", testNamespace70, "5");

const char	*testNamespace71 =
"int x = 10;\r\n\
\r\n\
namespace A\r\n\
{\r\n\
	int x = 5;\r\n\
\r\n\
	int foo()\r\n\
	{\r\n\
		int x = 2;\r\n\
		return x;\r\n\
	}\r\n\
\r\n\
	return foo();\r\n\
}";
TEST_RESULT("namespace test 71 (local scope priority 2)", testNamespace71, "2");

LOAD_MODULE(test_namespace72, "test.namespace72", "namespace detail{ int get(){ return 5; } } class Test<T>{ int test(){ return detail.get(); } }");
const char	*testNamespace72 =
"import test.namespace72;\r\n\
class Foo{ int x; }\r\n\
Test<Foo> f;\r\n\
return f.test();";
TEST_RESULT("namespace test 72 (generic type import parse in correct namespace)", testNamespace72, "5");

LOAD_MODULE(test_namespace73, "test.namespace73", "namespace detail{ int get(){ return 5; } } auto gen(generic a){ return a + detail.get(); }");
const char	*testNamespace73 =
"import test.namespace73;\r\n\
return gen(5);";
TEST_RESULT("namespace test 73 (generic function import parse in correct namespace)", testNamespace73, "10");
