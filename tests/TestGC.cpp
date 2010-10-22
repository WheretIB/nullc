#include "TestBase.h"

const char	*testGCArrayFail2 =
"import std.gc;\r\n\
class Foo{ Foo[] arr; }\r\n\
Foo[] x = new Foo[2];\r\n\
Foo y;\r\n\
y.arr = x;\r\n\
x[0] = y;\r\n\
GC.CollectMemory();\r\n\
return 0;";
TEST_RESULT("GC recursion using arrays with implicit size, placed on the heap", testGCArrayFail2, "0");

const char	*testGCArrayFail1 =
"import std.gc;\r\n\
class Foo{ Foo[] arr; }\r\n\
Foo[2] fuck;\r\n\
Foo[] x = fuck;\r\n\
Foo y;\r\n\
y.arr = x;\r\n\
x[0] = y;\r\n\
GC.CollectMemory();\r\n\
return 0;";
TEST_RESULT("GC recursion using arrays with implicit size, placed on the stack", testGCArrayFail1, "0");

const char	*testGarbageCollectionCorrectness =
"import std.gc;\r\n\
int start = GC.UsedMemory();\r\n\
class A\r\n\
{\r\n\
	int a, b, c;\r\n\
	A ref d, e, f;\r\n\
}\r\n\
A ref[] arr1 = new A ref[2];\r\n\
A ref tmp;\r\n\
arr1[0] = tmp = new A;\r\n\
tmp.d = new A;\r\n\
tmp.e = new A;\r\n\
tmp.f = new A;\r\n\
tmp = nullptr;\r\n\
arr1[1] = new A;\r\n\
arr1[1].d = new A;\r\n\
arr1[1].e = new A;\r\n\
arr1[1].f = new A;\r\n\
GC.CollectMemory();\r\n\
return GC.UsedMemory() - start;";
TEST_RESULT("Garbage collection correctness.", testGarbageCollectionCorrectness, sizeof(void*) == 8 ? "544" : "272");

const char	*testGarbageCollectionCorrectness2 =
"import std.gc;\r\n\
int start = GC.UsedMemory();\r\n\
class A\r\n\
{\r\n\
	int a, b, c;\r\n\
	A ref d, e, f;\r\n\
}\r\n\
A ref[] arr1 = new A ref[2];\r\n\
A ref tmp;\r\n\
arr1[0] = tmp = new A;\r\n\
tmp.d = new A;\r\n\
tmp.e = new A;\r\n\
tmp.f = new A;\r\n\
tmp = nullptr;\r\n\
arr1[1] = new A;\r\n\
arr1[1].d = new A;\r\n\
arr1[1].e = new A;\r\n\
arr1[1].f = new A;\r\n\
arr1[0] = nullptr;\r\n\
arr1[1] = nullptr;\r\n\
GC.CollectMemory();\r\n\
return GC.UsedMemory() - start;";
TEST_RESULT("Garbage collection correctness 2.", testGarbageCollectionCorrectness2, sizeof(void*) == 8 ? "32" : "16");

const char	*testGarbageCollectionCorrectness3 =
"import std.gc;\r\n\
int start = GC.UsedMemory();\r\n\
class A\r\n\
{\r\n\
	int a, b, c;\r\n\
	A ref d, e, f;\r\n\
}\r\n\
A ref[] arr1 = new A ref[2];\r\n\
A ref tmp;\r\n\
arr1[0] = tmp = new A;\r\n\
tmp.d = new A;\r\n\
tmp.e = new A;\r\n\
tmp.f = new A;\r\n\
arr1[1] = new A;\r\n\
arr1[1].d = new A;\r\n\
arr1[1].e = new A;\r\n\
arr1[1].f = new A;\r\n\
GC.CollectMemory();\r\n\
return GC.UsedMemory() - start;";
TEST_RESULT("Garbage collection correctness 3.", testGarbageCollectionCorrectness3, sizeof(void*) == 8 ? "544" : "272");

const char	*testStackFrameSizeX64 =
"void test()\r\n\
{\r\n\
	auto d = new int;\r\n\
	*d = 4;\r\n\
	auto e = new int[1024*1024];\r\n\
	auto f = new int;\r\n\
	*f = 5;\r\n\
	assert(*d == 4);\r\n\
	assert(*f == 5);\r\n\
	assert(d != f);\r\n\
}\r\n\
void help(int a, b, c)\r\n\
{\r\n\
	test();\r\n\
}\r\n\
help(2, 3, 4);\r\n\
return 0;";
TEST_RESULT("Stack frame size calculation in GC under x64.", testStackFrameSizeX64, "0");

const char	*testStackFrameSizeX86 =
"void test()\r\n\
{\r\n\
	auto d = new int;\r\n\
	*d = 4;\r\n\
	auto e = new int[1024*1024];\r\n\
	auto f = new int;\r\n\
	*f = 5;\r\n\
	assert(*d == 4);\r\n\
	assert(*f == 5);\r\n\
	assert(d != f);\r\n\
}\r\n\
void help(int a, b, c, d)\r\n\
{\r\n\
	test();\r\n\
}\r\n\
help(2, 3, 4, 5);\r\n\
return 0;";
TEST_RESULT("Stack frame size calculation in GC under x86.", testStackFrameSizeX86, "0");

const char	*testGarbageCollectionCorrectness4 =
"import std.gc;\r\n\
class A\r\n\
{\r\n\
	int x, y;\r\n\
	int sum(){ return x + y; }\r\n\
}\r\n\
auto a = new A;\r\n\
a.x = 4;\r\n\
a.y = 9;\r\n\
auto f = a.sum;\r\n\
a = nullptr;\r\n\
GC.CollectMemory();\r\n\
auto b = new int[2];\r\n\
for(i in b)\r\n\
	i = 0;\r\n\
return f();";
TEST_RESULT("Garbage collection correctness 4 (member function pointers).", testGarbageCollectionCorrectness4, "13");

const char	*testGarbageCollectionCorrectness5 =
"import std.gc;\r\n\
class A\r\n\
{\r\n\
	int x;\r\n\
	int ref y;\r\n\
	int sum(){ return x + *y; }\r\n\
}\r\n\
auto a = new A;\r\n\
a.x = 4;\r\n\
a.y = new int;\r\n\
*a.y = 9;\r\n\
auto f = a.sum;\r\n\
a = nullptr;\r\n\
GC.CollectMemory();\r\n\
auto b = new A;\r\n\
b.x = 0;\r\n\
b.y = new int;\r\n\
*b.y = 0;\r\n\
return f();";
TEST_RESULT("Garbage collection correctness 5 (member function pointers).", testGarbageCollectionCorrectness5, "13");

const char	*testGarbageCollectionCorrectness6 =
"import std.gc;\r\n\
auto func(int i)\r\n\
{\r\n\
	return auto(){ return i; };\r\n\
}\r\n\
auto f = func(5);\r\n\
GC.CollectMemory();\r\n\
auto f2 = func(8);\r\n\
return f();";
TEST_RESULT("Garbage collection correctness 6 (local function context).", testGarbageCollectionCorrectness6, "5");

const char	*testGarbageCollectionCorrectness7 =
"import std.gc;\r\n\
auto func(int i)\r\n\
{\r\n\
	int ref a = new int;\r\n\
	*a = i * 10;\r\n\
	return auto(){ return i + *a; };\r\n\
}\r\n\
auto f = func(5);\r\n\
GC.CollectMemory();\r\n\
auto f2 = func(8);\r\n\
return f();";
TEST_RESULT("Garbage collection correctness 7 (local function context).", testGarbageCollectionCorrectness7, "55");

const char	*testGarbageCollectionCorrectness8 =
"import std.gc;\r\n\
class A\r\n\
{\r\n\
	int x, y;\r\n\
	auto sum(){ return auto(){ return this.x + y; }; }\r\n\
}\r\n\
auto a = new A;\r\n\
a.x = 4;\r\n\
a.y = 9;\r\n\
auto f = a.sum();\r\n\
a = nullptr;\r\n\
GC.CollectMemory();\r\n\
auto b = new A;\r\n\
b.x = 0;\r\n\
b.y = 0;\r\n\
return f();";
TEST_RESULT("Garbage collection correctness 8 (local member function context).", testGarbageCollectionCorrectness8, "13");

const char	*testGarbageCollectionCorrectness9 =
"import std.gc;\r\n\
class A\r\n\
{\r\n\
	int x, y;\r\n\
	auto sum(){ return auto(){ return x + y; }; }\r\n\
}\r\n\
auto a = new A;\r\n\
a.x = 4;\r\n\
a.y = 9;\r\n\
auto f = a.sum();\r\n\
a = nullptr;\r\n\
GC.CollectMemory();\r\n\
auto b = new A;\r\n\
b.x = 0;\r\n\
b.y = 0;\r\n\
return f();";
TEST_RESULT("Garbage collection correctness 9 (local member function context).", testGarbageCollectionCorrectness9, "13");

const char	*testGarbageCollectionCorrectness10 =
"import std.gc;\r\n\
class A\r\n\
{\r\n\
	int x, y;\r\n\
	auto sum(){ return auto(){ return x + y; }; }\r\n\
}\r\n\
auto a = new A;\r\n\
a.x = 4;\r\n\
a.y = 9;\r\n\
int ref()[1] f;\r\n\
f[0] = a.sum();\r\n\
a = nullptr;\r\n\
GC.CollectMemory();\r\n\
auto b = new A;\r\n\
b.x = 0;\r\n\
b.y = 0;\r\n\
return f[0]();";
TEST_RESULT("Garbage collection correctness 10 (local member function context).", testGarbageCollectionCorrectness10, "13");

const char	*testGarbageCollectionCorrectness11 =
"import std.vector;\r\n\
import std.gc;\r\n\
\r\n\
class A\r\n\
{\r\n\
	int ref a, b;\r\n\
}\r\n\
vector arr = vector(A);\r\n\
auto test = new A;\r\n\
test.a = new int;\r\n\
*test.a = 6;\r\n\
test.b = new int;\r\n\
*test.b = 5;\r\n\
arr.push_back(test);\r\n\
test = nullptr;\r\n\
GC.CollectMemory();\r\n\
auto test2 = new A;\r\n\
test2.a = new int;\r\n\
*test2.a = 9;\r\n\
test2.b = new int;\r\n\
*test2.b = 4;\r\n\
test = arr.back();\r\n\
return *test.a * 10 + *test.b;";
TEST_RESULT("Garbage collection correctness 11 (auto[] type).", testGarbageCollectionCorrectness11, "65");

const char	*testGarbageCollectionCorrectness12 =
"import std.gc;\r\n\
auto ref y = &y;\r\n\
GC.CollectMemory();\r\n\
return 1;";
TEST_RESULT("Garbage collection correctness 12 (auto ref recursion).", testGarbageCollectionCorrectness12, "1");

const char	*testGarbageCollectionCorrectness13 =
"import std.vector;\r\n\
import std.gc;\r\n\
vector arr = vector(int);\r\n\
GC.CollectMemory();\r\n\
return 1;";
TEST_RESULT("Garbage collection correctness 13 (uninitialized auto[] type).", testGarbageCollectionCorrectness13, "1");

const char	*testGarbageCollectionCorrectness14 =
"import std.gc;\r\n\
class Test{ int x; }\r\n\
int ref Test:getX(){ GC.CollectMemory(); return &x; }\r\n\
auto y = (new Test).getX();\r\n\
*y = 8;\r\n\
auto m = new Test;\r\n\
m.x = 2;\r\n\
return *y;";
TEST_RESULT("Garbage collection correctness 14 (extra function argument check).", testGarbageCollectionCorrectness14, "8");

const char	*testGarbageCollectionCorrectness15 =
"import std.vector;\r\n\
import std.list;\r\n\
import std.gc;\r\n\
auto a = new list;\r\n\
a.list(int);\r\n\
a.push_back(6);\r\n\
int ref y;\r\n\
auto x = a.begin;\r\n\
a = nullptr;\r\n\
GC.CollectMemory();\r\n\
y = new int(4);\r\n\
return int(x().value);";
TEST_RESULT("Garbage collection correctness 15 (extra function argument type fixup).", testGarbageCollectionCorrectness15, "6");

const char	*testGCFullArrayCapture =
"import std.range;\r\n\
int func()\r\n\
{\r\n\
	int[32*1024] k = 5;\r\n\
	auto getTest(int i)\r\n\
	{\r\n\
		k[0]++;\r\n\
		int test()\r\n\
		{\r\n\
			int x = i;//arr[i];\r\n\
			k[0]++;\r\n\
			return x;\r\n\
		}\r\n\
		return test;\r\n\
	}\r\n\
	int ref()[10] fArr;\r\n\
	for(f in fArr, i in range(0, 1024))\r\n\
		f = getTest(i);\r\n\
	return fArr[2]() * fArr[3]();\r\n\
}\r\n\
return func();";
TEST_RESULT("Stack reallocation when it is empty", testGCFullArrayCapture, "6");

const char	*testCoroutineNoLocalStackGC =
"import std.gc;\r\n\
void fuckup(){ int[128] i = 0xfeeefeee; } fuckup();\r\n\
coroutine auto test()\r\n\
{\r\n\
	auto ref x, y; int i = 3;\r\n\
	GC.CollectMemory();\r\n\
	yield 2; return 3;\r\n\
}\r\n\
test(); return test();";
TEST_RESULT("Coroutine without stack frame for locals GC test", testCoroutineNoLocalStackGC, "3");
