#include "TestBase.h"

const char	*testStackResize =
"class RefHold\r\n\
{\r\n\
	int ref c;\r\n\
}\r\n\
int a = 12;\r\n\
int ref b = &a;\r\n\
auto h = new RefHold;\r\n\
h.c = b;\r\n\
auto ref c = &a;\r\n\
int ref[2] d;\r\n\
int ref[] e = d, e0;\r\n\
int[2][2] u;\r\n\
RefHold[] u2 = { *h, *h, *h };\r\n\
e[0] = &a;\r\n\
auto func()\r\n\
{\r\n\
	auto f2()\r\n\
	{\r\n\
		int[4096] arr;\r\n\
	}\r\n\
	f2();\r\n\
	return b;\r\n\
}\r\n\
int ref res = func();\r\n\
int ref v = c;\r\n\
a = 10;\r\n\
return *res + *h.c + *v + *e[0];";
TEST_RELOCATE("Parameter stack resize", testStackResize, "40");

const char	*testStackRelocationFrameSizeX64 =
"int a = 1;\r\n\
void test()\r\n\
{\r\n\
	int[1024*1024] e = 0;\r\n\
	a = 6;\r\n\
}\r\n\
void help()\r\n\
{\r\n\
	auto e = &a;\r\n\
	test();\r\n\
	assert(*e == a);\r\n\
	assert(*e == 6);\r\n\
}\r\n\
void help2(int a_, b, c)\r\n\
{\r\n\
	help();\r\n\
}\r\n\
help2(2, 3, 4);\r\n\
return 0;";
TEST_RELOCATE("Stack frame size calculation in VM stack relocation under x64", testStackRelocationFrameSizeX64, "0");

const char	*testStackRelocationFrameSizeX86 =
"int a = 1;\r\n\
void test()\r\n\
{\r\n\
	int[1024*1024] e = 0;\r\n\
	a = 6;\r\n\
}\r\n\
void help()\r\n\
{\r\n\
	auto e = &a;\r\n\
	test();\r\n\
	assert(*e == a);\r\n\
	assert(*e == 6);\r\n\
}\r\n\
void help2(int a_, b, c, d)\r\n\
{\r\n\
	help();\r\n\
}\r\n\
help2(2, 3, 4, 5);\r\n\
return 0;";
TEST_RELOCATE("Stack frame size calculation in VM stack relocation under x86", testStackRelocationFrameSizeX86, "0");

const char	*testStackRelocationFrameSizeX64_2 =
"int a = 1;\r\n\
void corrupt()\r\n\
{\r\n\
	int[1024*1024] e = 0;\r\n\
	a = 6;\r\n\
}\r\n\
void test()\r\n\
{\r\n\
	auto e = &a;\r\n\
	corrupt();\r\n\
	assert(*e == a);\r\n\
	assert(*e == 6);\r\n\
}\r\n\
auto rurr()\r\n\
{\r\n\
	// function local state (x64):\r\n\
	// $context at base + 0 (sizeof 8)\r\n\
	// d at base + 8 (sizeof 4)\r\n\
	// pad at base + 12 (sizeof 4)\r\n\
	// b1 at base + 24 (sizeof 12)\r\n\
	// $lamda_27_ext at base + 16 (sizeof 8)\r\n\
	// stack frame end is at 36, but incorrect calculation will return 24\r\n\
	// stack frame alignment will translate this to 48 and 32, creating an error\r\n\
	int d = 5;\r\n\
	int pad = 8;\r\n\
	auto b1 = int lambda(int b){ return b + d; };\r\n\
	test();\r\n\
	d = 7;\r\n\
	return b1(4);\r\n\
}\r\n\
return rurr();";
TEST_RELOCATE("Stack frame size calculation in VM stack relocation under x64 2", testStackRelocationFrameSizeX64_2, "11");

const char	*testStackRelocationFrameSizeX86_2 =
"int a = 1;\r\n\
void corrupt()\r\n\
{\r\n\
	int[1024*1024] e = 0;\r\n\
	a = 6;\r\n\
}\r\n\
void test()\r\n\
{\r\n\
	auto e = &a;\r\n\
	corrupt();\r\n\
	assert(*e == a);\r\n\
	assert(*e == 6);\r\n\
}\r\n\
auto rurr()\r\n\
{\r\n\
	// function local state (x86):\r\n\
	// $context at base + 0 (sizeof 4)\r\n\
	// d at base + 4 (sizeof 4)\r\n\
	// b1 at base + 12 (sizeof 8)\r\n\
	// $lamda_27_ext at base + 8 (sizeof 4)\r\n\
	// stack frame end is at 20, but incorrect calculation will return 12\r\n\
	// stack frame alignment will translate this to 32 and 16, creating an error\r\n\
	int d = 5;\r\n\
	auto b1 = int lambda(int b){ return b + d; };\r\n\
	test();\r\n\
	d = 7;\r\n\
	return b1(4);\r\n\
}\r\n\
return rurr();";
TEST_RELOCATE("Stack frame size calculation in VM stack relocation under x86 2", testStackRelocationFrameSizeX86_2, "11");

const char	*testStackRelocationFunction =
"class A\r\n\
{\r\n\
	int x, y;\r\n\
	int sum(){ return x + y; }\r\n\
}\r\n\
A a;\r\n\
a.x = 1;\r\n\
a.y = 2;\r\n\
auto f = a.sum;\r\n\
void corrupt()\r\n\
{\r\n\
	int[32*1024] e = 0;\r\n\
}\r\n\
corrupt();\r\n\
a.x = 4;\r\n\
a.y = 9;\r\n\
return f();";
TEST_RELOCATE("VM stack relocation function check", testStackRelocationFunction, "13");

const char	*testStackRelocationFunction2 =
"void corrupt()\r\n\
{\r\n\
	int[32*1024] e = 0;\r\n\
}\r\n\
auto foo()\r\n\
{\r\n\
	int i = 3;\r\n\
	auto help()\r\n\
	{\r\n\
		int ref func()\r\n\
		{\r\n\
			return &i;\r\n\
		}\r\n\
		return func;\r\n\
	}\r\n\
	auto f = help();\r\n\
	corrupt();\r\n\
	i = 8;\r\n\
	int ref data = f();\r\n\
	return *data;\r\n\
}\r\n\
return foo();";
TEST_RELOCATE("VM stack relocation function check 2", testStackRelocationFunction2, "8");

const char	*testStackRelocationFunction3 =
"void corrupt()\r\n\
{\r\n\
	int[32*1024] e = 0;\r\n\
}\r\n\
auto foo()\r\n\
{\r\n\
	int i = 3;\r\n\
	auto help()\r\n\
	{\r\n\
		int ref func()\r\n\
		{\r\n\
			return &i;\r\n\
		}\r\n\
		return func;\r\n\
	}\r\n\
	int ref ref()[1] f;\r\n\
	f[0] = help();\r\n\
	corrupt();\r\n\
	i = 8;\r\n\
	int ref data = f[0]();\r\n\
	return *data;\r\n\
}\r\n\
return foo();";
TEST_RELOCATE("VM stack relocation function check 3", testStackRelocationFunction3, "8");

const char	*testVMRelocateArrayFail =
"class Foo{ Foo[] arr; int x; }\r\n\
Foo[2] fuck;\r\n\
fuck[0].x = 2;\r\n\
fuck[1].x = 5;\r\n\
Foo[] x = fuck;\r\n\
Foo y;\r\n\
y.arr = x;\r\n\
x[0] = y;\r\n\
void corrupt()\r\n\
{\r\n\
	int[1024*1024] e = 0;\r\n\
}\r\n\
corrupt();\r\n\
fuck[0].x = 12;\r\n\
fuck[1].x = 25;\r\n\
return x[0].x + x[1].x;";
TEST_RELOCATE("VM stack relocation test with possible infinite recursion", testVMRelocateArrayFail, "37");

const char	*testVMRelocateArrayFail2 =
"class Foo{ Foo[] arr; int x; }\r\n\
Foo[] x = new Foo[2];\r\n\
x[0].x = 2;\r\n\
x[1].x = 5;\r\n\
Foo y;\r\n\
y.arr = x;\r\n\
x[0] = y;\r\n\
void corrupt()\r\n\
{\r\n\
	int[1024*1024] e = 0;\r\n\
}\r\n\
corrupt();\r\n\
x[0].x = 12;\r\n\
x[1].x = 25;\r\n\
return x[0].x + x[1].x;";
TEST_RELOCATE("VM stack relocation test with possible infinite recursion 2", testVMRelocateArrayFail2, "37");

const char	*testStackRelocationClassAlignedMembers =
"void corrupt()\r\n\
{\r\n\
	int[32*1024] e = 0;\r\n\
}\r\n\
int z = 5;\r\n\
class Test{ char a; int c; int ref b; }\r\n\
Test x;\r\n\
x.a = 5;\r\n\
x.b = &z;\r\n\
corrupt();\r\n\
z = 15;\r\n\
return *x.b;";
TEST_RELOCATE("VM stack relocation with aligned class members", testStackRelocationClassAlignedMembers, "15");

const char	*testAutoArrayRelocation =
"import std.vector;\r\n\
\r\n\
class A\r\n\
{\r\n\
	int ref a, b;\r\n\
}\r\n\
int av = 7, bv = 30;\r\n\
vector arr = vector(A);\r\n\
auto test = new A;\r\n\
test.a = &av;\r\n\
test.b = &bv;\r\n\
arr.push_back(test);\r\n\
test = nullptr;\r\n\
void corrupt()\r\n\
{\r\n\
	int[32*1024] e = 0;\r\n\
}\r\n\
corrupt();\r\n\
test = arr.back();\r\n\
av = 4;\r\n\
bv = 60;\r\n\
return *test.a + *test.b;";
TEST_RELOCATE("VM stack relocation (auto[] type)", testAutoArrayRelocation, "64");

const char	*testAutoArrayRelocation2 =
"import std.vector;\r\n\
vector arr = vector(int);\r\n\
void corrupt()\r\n\
{\r\n\
	int[32*1024] e = 0;\r\n\
}\r\n\
corrupt();\r\n\
return 1;";
TEST_RELOCATE("VM stack relocation (uninitialized auto[] type)", testAutoArrayRelocation2, "1");

const char	*testAutoRefRecursionRelocate =
"auto ref y = &y;\r\n\
void corrupt()\r\n\
{\r\n\
	int[32*1024] e = 0;\r\n\
}\r\n\
corrupt();\r\n\
return 1;";
TEST_RELOCATE("VM stack relocation (auto ref recursion)", testAutoRefRecursionRelocate, "1");

const char	*testExtraArgumentRelocate =
"class Test{ int x; }\r\n\
int ref Test:getX(){ int[32*1024] make_stack_reallocation; return &x; }\r\n\
Test a;\r\n\
a.x = 5;\r\n\
auto y = a.getX();\r\n\
a.x = 8;\r\n\
return *y;";
TEST_RELOCATE("VM stack relocation (extra function argument)", testExtraArgumentRelocate, "8");

const char	*testCoroutineNoLocalStackRelocate =
"void fuckup(){ int[128] i = 0xfeeefeee; } fuckup();\r\n\
void corrupt(){ int[32*1024] e = 0; }\r\n\
coroutine auto test()\r\n\
{\r\n\
	auto ref x, y, z, w, k, l, m; int i = 3;\r\n\
	corrupt();\r\n\
	yield 2; return 3;\r\n\
}\r\n\
test(); return test();";
TEST_RELOCATE("VM stack relocation (coroutine without stack for locals)", testCoroutineNoLocalStackRelocate, "3");

struct TestEval : TestQueue
{
	virtual void Run()
	{
		nullcTerminate();
		nullcInit(MODULE_PATH);
		nullcInitDynamicModule();

		const char	*testEval =
"import std.dynamic;\r\n\
\r\n\
int a = 5;\r\n\
\r\n\
for(int i = 0; i < 200; i++)\r\n\
	eval(\"a = 3 * \" + i.str() + \";\");\r\n\
\r\n\
return a;";
		double evalStart = myGetPreciseTime();
		testsCount[0]++;
		if(Tests::RunCode(testEval, NULLC_VM, "597", "Dynamic code. eval()"))
			testsPassed[0]++;
		printf("Eval test finished in %f\r\n", myGetPreciseTime() - evalStart);

		nullcTerminate();
		nullcInit(MODULE_PATH);
		nullcInitDynamicModule();
	}
};
TestEval testEval;

const char	*testCoroutineAndEval =
"import std.dynamic;\r\n\
coroutine int foo()\r\n\
{\r\n\
	int i = 0;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	return i;\r\n\
}\r\n\
foo();\r\n\
int x = foo();\r\n\
int a = 5;\r\n\
\r\n\
for(int i = 0; i < 200; i++)\r\n\
	eval(\"a = 3 * \" + i.str() + \";\");\r\n\
int y = foo();\r\n\
return x * 10 + y;";
TEST_VM("Coroutine and dynamic code. eval()", testCoroutineAndEval, "12")
{
}

struct TestVariableImportCorrectness : TestQueue
{
	virtual void Run()
	{
		nullcTerminate();
		nullcInit(MODULE_PATH);
		nullcInitDynamicModule();

		const char	*testVariableImportCorrectness =
"import std.dynamic;\r\n\
int x = 2;\r\n\
eval(\"x = 4;\");\r\n\
{ int n, m; }\r\n\
int y = 3;\r\n\
eval(\"y = 8;\");\r\n\
return x * 10 + y;";
		testsCount[0]++;
		if(Tests::RunCode(testVariableImportCorrectness, NULLC_VM, "48", "Variable import correctness"))
			testsPassed[0]++;
	}
};
TestVariableImportCorrectness testVariableImportCorrectness;

struct TestGCGlobalLimit : TestQueue
{
	virtual void Run()
	{
		nullcTerminate();
		nullcInit(MODULE_PATH);
		nullcInitGCModule();
		nullcSetGlobalMemoryLimit(1024 * 1024);

		const char	*testGCGlobalLimit =
		"import std.gc;\r\n\
		int[] arr1 = new int[200000];\r\n\
		arr1 = nullptr;\r\n\
		int[] arr2 = new int[200000];\r\n\
		return arr2.size;";
		for(int t = 0; t < 2; t++)
		{
			testsCount[t]++;
			if(Tests::RunCode(testGCGlobalLimit, t, "200000", "GC collection before global limit exceeded error"))
				testsPassed[t]++;
		}
	}
};
TestGCGlobalLimit testGCGlobalLimit;

struct TestRestore : TestQueue
{
	virtual void Run()
	{
		nullcTerminate();
		nullcInit(MODULE_PATH);

		nullcInitTypeinfoModule();
		nullcInitFileModule();
		nullcInitMathModule();
		nullcInitVectorModule();
		nullcInitRandomModule();
		nullcInitDynamicModule();
		nullcInitGCModule();
		nullcInitIOModule();
		nullcInitCanvasModule();
#if defined(_MSC_VER)
		nullcInitWindowModule();
#endif
	}
};
TestRestore testRestore;
