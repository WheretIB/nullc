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
