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
(auto(){arr1[0] = tmp = new A;\r\n\
tmp.d = new A;\r\n\
tmp.e = new A;\r\n\
tmp.f = new A;\r\n\
tmp = nullptr;})();\r\n\
arr1[1] = new A;\r\n\
arr1[1].d = new A;\r\n\
arr1[1].e = new A;\r\n\
arr1[1].f = new A;\r\n\
GC.CollectMemory();\r\n\
return GC.UsedMemory() - start;";
TEST_RESULT_SIMPLE("Garbage collection correctness.", testGarbageCollectionCorrectness, sizeof(void*) == 8 ? "544" : "272");

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
(auto(){arr1[0] = tmp = new A;\r\n\
tmp.d = new A;\r\n\
tmp.e = new A;\r\n\
tmp.f = new A;\r\n\
tmp = nullptr;\r\n\
arr1[1] = new A;\r\n\
arr1[1].d = new A;\r\n\
arr1[1].e = new A;\r\n\
arr1[1].f = new A;\r\n\
arr1[0] = nullptr;\r\n\
arr1[1] = nullptr;})();\r\n\
GC.CollectMemory();\r\n\
return GC.UsedMemory() - start;";
TEST_RESULT_SIMPLE("Garbage collection correctness 2.", testGarbageCollectionCorrectness2, sizeof(void*) == 8 ? "32" : "16");

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
(auto(){arr1[0] = tmp = new A;\r\n\
tmp.d = new A;\r\n\
tmp.e = new A;\r\n\
tmp.f = new A;\r\n\
arr1[1] = new A;\r\n\
arr1[1].d = new A;\r\n\
arr1[1].e = new A;\r\n\
arr1[1].f = new A;})();\r\n\
GC.CollectMemory();\r\n\
return GC.UsedMemory() - start;";
TEST_RESULT_SIMPLE("Garbage collection correctness 3.", testGarbageCollectionCorrectness3, sizeof(void*) == 8 ? "544" : "272");

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
A ref a; (auto(){ a = new A; })();\r\n\
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
A ref a; (auto(){ a = new A; })();\r\n\
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
A ref a; (auto(){ a = new A; })();\r\n\
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
A ref a; (auto(){ a = new A; })();\r\n\
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
A ref a; (auto(){ a = new A; })();\r\n\
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
"import old.vector;\r\n\
import std.gc;\r\n\
\r\n\
class A\r\n\
{\r\n\
	int ref a, b;\r\n\
}\r\n\
vector arr = vector(A);\r\n\
auto test = new A;\r\n\
test.a = new int;\r\n\
(auto(){*test.a = 6;\r\n\
test.b = new int;\r\n\
*test.b = 5;\r\n\
arr.push_back(test);\r\n\
test = nullptr;})();\r\n\
GC.CollectMemory();\r\n\
auto test2 = new A;\r\n\
test2.a = new int;\r\n\
*test2.a = 9;\r\n\
test2.b = new int;\r\n\
*test2.b = 4;\r\n\
test = arr.back();\r\n\
return *test.a * 10 + *test.b;";
TEST_RESULT("Garbage collection correctness 11 (auto[] type).", testGarbageCollectionCorrectness11, "65");

const char	*testGarbageCollectionCorrectness11b =
"import old.vector;\r\n\
import std.gc;\r\n\
\r\n\
class A\r\n\
{\r\n\
	long ref a, b;\r\n\
}\r\n\
vector arr = vector(A);\r\n\
auto test = new A;\r\n\
test.a = new long;\r\n\
(auto(){*test.a = 6;\r\n\
test.b = new long;\r\n\
*test.b = 5;\r\n\
arr.push_back(test);\r\n\
test = nullptr;})();\r\n\
GC.CollectMemory();\r\n\
auto test2 = new A;\r\n\
test2.a = new long;\r\n\
*test2.a = 9;\r\n\
test2.b = new long;\r\n\
*test2.b = 4;\r\n\
test = arr.back();\r\n\
return int(*test.a * 10 + *test.b);";
TEST_RESULT("Garbage collection correctness 11 (auto[] type, larger interference).", testGarbageCollectionCorrectness11b, "65");

const char	*testGarbageCollectionCorrectness12 =
"import std.gc;\r\n\
auto ref y = &y;\r\n\
GC.CollectMemory();\r\n\
return 1;";
TEST_RESULT("Garbage collection correctness 12 (auto ref recursion).", testGarbageCollectionCorrectness12, "1");

const char	*testGarbageCollectionCorrectness13 =
"import old.vector;\r\n\
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
"import old.vector;\r\n\
import old.list;\r\n\
import std.gc;\r\n\
list ref a; (auto(){ a = new list; })();\r\n\
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

const char	*testArrayOfArraysGC =
"import std.gc;\r\n\
class Foo{ int ref y; }\r\n\
Foo[1][4] arr;\r\n\
arr[0][0].y = new int(2); arr[0][1].y = new int(30); arr[0][2].y = new int(400); arr[0][3].y = new int(5000);\r\n\
GC.CollectMemory();\r\n\
new int(0);new int(0);new int(0);new int(0);\r\n\
return *arr[0][0].y + *arr[0][1].y + *arr[0][2].y + *arr[0][3].y;";
TEST_RESULT("Array of arrays GC test", testArrayOfArraysGC, "5432");

const char	*testUnusedUpvaluesGC =
"import std.gc;\r\n\
int ref() k;\r\n\
auto foo(int x)\r\n\
{\r\n\
	int ref() m;\r\n\
	for(int y = 0; y < 10; y++)\r\n\
	{\r\n\
		m = auto(){ return y + x; };\r\n\
		if(y == 6)\r\n\
			k = m;\r\n\
	}\r\n\
	GC.CollectMemory();\r\n\
	return m;\r\n\
}\r\n\
int res = foo(5)();\r\n\
GC.CollectMemory();\r\n\
res += k();\r\n\
return res;";
TEST_RESULT("Unused upvalues GC test", testUnusedUpvaluesGC, "25");

const char	*testUnusedUpvaluesGC2 =
"import std.gc;\r\n\
int start = GC.UsedMemory();\r\n\
int ref() k;\r\n\
auto foo(int x)\r\n\
{\r\n\
	int a; int ref() m;\r\n\
	for(int y = 0; y < 10; y++)\r\n\
	{\r\n\
		m = auto(){ return y + x + a; };\r\n\
		if(y == 6)\r\n\
			k = m;\r\n\
	}\r\n\
	GC.CollectMemory();\r\n\
	return m;\r\n\
}\r\n\
foo(5)() + k();\r\n\
GC.CollectMemory();\r\n\
return GC.UsedMemory() - start;";
TEST_RESULT_SIMPLE("Unused upvalues GC test 2", testUnusedUpvaluesGC2, sizeof(void*) == 8 ? "128" : "64");

const char	*testDoubleMemoryRemovalGC2 =
"import std.gc;\r\n\
import std.range;\r\n\
int ref() k;\r\n\
auto foo(int x)\r\n\
{\r\n\
	int ref() m;\r\n\
	for(int y = 0; y < 10; y++)\r\n\
	{\r\n\
		m = auto(){ return y + x; };\r\n\
		if(y == 6)\r\n\
			k = m;\r\n\
	}\r\n\
	GC.CollectMemory();\r\n\
	return m;\r\n\
}\r\n\
int zzz = foo(5)() + k();\r\n\
GC.CollectMemory();\r\n\
class Foo{ int[4] arr; int x; void Foo(int y){ x = y; } }\r\n\
Foo ref[20] fArr;\r\n\
for(i in fArr, j in range(2, 1000, 2))\r\n\
	i = new Foo(j);\r\n\
for(i in fArr, j in range(2, 1000, 2))\r\n\
	assert(i.x == j);\r\n\
	return zzz;";
TEST_RESULT("Prevention of double memory removal 2", testDoubleMemoryRemovalGC2, "25");

const char	*testStackVariablesGC =
"import std.gc;\r\n\
int foo(int ref x, y, z, int[] arr)\r\n\
{\r\n\
	int ref s = new int(0), t = new int(0), r = new int(0);\r\n\
	return *x + *y + *z;\r\n\
}\r\n\
auto bar(){ GC.CollectMemory(); return new int[32]; }\r\n\
return foo(new int(4), new int(6), new int(40), bar());";
TEST_RESULT("Checking of temporary variable stack in GC test", testStackVariablesGC, "50");

const char *testEmptyObjectArrayGC =
"import std.gc;\r\n\
class Foo{}\r\n\
\r\n\
Foo[] x = new Foo[4];\r\n\
GC.CollectMemory();\r\n\
Foo[] y = new Foo[4];\r\n\
\r\n\
return x == y;";
TEST_RESULT("Check for bug in GC on an array of empty objects", testEmptyObjectArrayGC, "0");

const char *testGCOnAllocatedArrayPointer =
"import std.gc;\r\n\
\r\n\
int next = 1;\r\n\
\r\n\
class Foo\r\n\
{\r\n\
	int ref x;\r\n\
	void Foo(){ x = new int(next++); }\r\n\
}\r\n\
\r\n\
Foo[4] ref a, b;\r\n\
\r\n\
auto foo(generic x, generic y, generic z){ a = x; b = z; return y; }\r\n\
\r\n\
auto omm()\r\n\
{\r\n\
	auto x = new (Foo[4]);\r\n\
	*x = { Foo(), Foo(), Foo(), Foo() };\r\n\
	return x;\r\n\
}\r\n\
\r\n\
auto omg(){ GC.CollectMemory(); return 1; }\r\n\
\r\n\
foo(omm(), omg(), omm());\r\n\
\r\n\
int sum = 0;\r\n\
for(i in *a, j in *b) sum += *i.x + *j.x;\r\n\
\r\n\
return sum;";
TEST_RESULT_SIMPLE("Check for bug in GC with allocating pointers to arrays", testGCOnAllocatedArrayPointer, "36");

int RecallerGC1()
{
	if(nullcRunFunction("first"))
		return nullcGetResultInt();

	return -1;
}
int RecallerGC2()
{
	if(nullcRunFunction("second"))
		return nullcGetResultInt();
	return -1;
}
int RecallerGC3()
{
	if(nullcRunFunction("runGC"))
		return nullcGetResultInt();
	return -1;
}

LOAD_MODULE_BIND(test_gctransition1, "func.gctransition1", "int Recaller1(); int Recaller2(); int Recaller3();")
{
	nullcBindModuleFunctionHelper("func.gctransition1", RecallerGC1, "Recaller1", 0);
	nullcBindModuleFunctionHelper("func.gctransition1", RecallerGC2, "Recaller2", 0);
	nullcBindModuleFunctionHelper("func.gctransition1", RecallerGC3, "Recaller3", 0);
}

const char	*testGCWhenTransitions =
"import func.gctransition1;\r\n\
import std.gc;\r\n\
int ref a = new int(5);\r\n\
int first()\r\n\
{\r\n\
	int ref b = new int(6);\r\n\
	int k = Recaller2();\r\n\
	assert(*b == 6);\r\n\
	assert(k == 7);\r\n\
	return *b;\r\n\
}\r\n\
int second()\r\n\
{\r\n\
	int ref c = new int(7);\r\n\
	int l = Recaller3();\r\n\
	assert(*c == 7);\r\n\
	assert(l == 8);\r\n\
	return *c;\r\n\
}\r\n\
int runGC()\r\n\
{\r\n\
	int ref d = new int(8);\r\n\
	GC.CollectMemory();\r\n\
	new int(1);\r\n\
	new int(2);\r\n\
	new int(3);\r\n\
	new int(4);\r\n\
	assert(*d == 8);\r\n\
	return *d;\r\n\
}\r\n\
int m = Recaller1();\r\n\
assert(*a == 5);\r\n\
assert(m == 6);\r\n\
return 1;";
TEST_RESULT_SIMPLE("GC execution when callstack is full of NULLC->C transitions", testGCWhenTransitions, "1");
