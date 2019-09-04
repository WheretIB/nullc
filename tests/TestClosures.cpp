#include "TestBase.h"

#include <stdint.h>

const char	*testLocalFunc1 = 
"// Local function context test\r\n\
int g1 = 3; // global variable (24)\r\n\
int r; // (28)\r\n\
{\r\n\
  int g2 = 5; // block variable (32)\r\n\
  int test(int x, int rek)\r\n\
  {\r\n\
    int g3 = 7; // rekursive function local variable (36?, 40?)\r\n\
    int help(int x)\r\n\
    {\r\n\
      return g3*x+g2*g1;\r\n\
    }\r\n\
    if(rek)\r\n\
      return help(x) * g2 + g1; // (7*3+15)*5+3 = 183\r\n\
    return help(x) * test(g1, 1); // (7*13+15) * 183\r\n\
  }\r\n\
  r = test(13, 0);\r\n\
}\r\n\
\r\n\
return r; // 19398";
TEST("Local function context test", testLocalFunc1, "19398")
{
	CHECK_INT("g1", 0, 3, lastFailed);
	CHECK_INT("r", 0, 19398, lastFailed);
}

const char	*testLocalFunc2 = 
"// Local function context test 2\r\n\
int g1 = 3; // global variable (24)\r\n\
int r; // (28)\r\n\
void glob(){\r\n\
  // test should get pointer to this variable\r\n\
  int g2 = 5; // block variable (32)\r\n\
  int test(int x, int rek)\r\n\
  {\r\n\
    // help should get pointer to this variable\r\n\
    int g3 = 7; // recursive function local variable (36?, 40?)\r\n\
    int help(int x)\r\n\
    {\r\n\
      int hell = 0;\r\n\
      if(x > 10)\r\n\
        hell = test(2, 0);\r\n\
      return g3*x + g2*g1 + hell;\r\n\
    }\r\n\
    if(rek)\r\n\
      return help(x) * g2 + g1; // (7*3+15)*5+3 = 183\r\n\
    return help(x) * test(g1, 1); // (7*13+15+(7*2+15+0)*((7*3+15+0)*5+3)) * 183\r\n\
  }\r\n\
  r = test(13, 0);\r\n\
}\r\n\
glob();\r\n\
return r; // 990579";
TEST("Local function context test 2", testLocalFunc2, "990579")
{
	CHECK_INT("g1", 0, 3, lastFailed);
	CHECK_INT("r", 0, 990579, lastFailed);
}

const char	*testClosure = 
"// Closure test\r\n\
\r\n\
int global = 100;\r\n\
int test(int a, int c){ return global + a * c; }\r\n\
\r\n\
int proxy(int a, int c, typeof(test) foo){ return foo(a, c); }\r\n\
\r\n\
auto fp = test;\r\n\
int res1a, res1b, res1c, res1d;\r\n\
res1a = test(2, 3);\r\n\
res1b = fp(2, 4);\r\n\
res1c = proxy(2, 5, test);\r\n\
res1d = proxy(2, 6, fp);\r\n\
int res2a, res2b, res2c, res2d;\r\n\
{\r\n\
  int local = 200;\r\n\
  int localtest(int a, int c){ return local + a * c; }\r\n\
\r\n\
  auto fp2 = localtest;\r\n\
  res2a = localtest(2, 3);\r\n\
  res2b = fp2(2, 4);\r\n\
  res2c = proxy(2, 5, localtest);\r\n\
  res2d = proxy(2, 6, fp2);\r\n\
}\r\n\
\r\n\
class Del\r\n\
{\r\n\
  int classLocal;\r\n\
\r\n\
  int test(int a, int c){ return classLocal + a * c; }\r\n\
}\r\n\
Del cls;\r\n\
cls.classLocal = 300;\r\n\
auto fp3 = cls.test;\r\n\
int res3a, res3b, res3c, res3d;\r\n\
res3a = cls.test(2, 3);\r\n\
res3b = fp3(2, 4);\r\n\
res3c = proxy(2, 5, cls.test);\r\n\
res3d = proxy(2, 6, fp3);\r\n\
\r\n\
int foobar(int ref(int[3]) f) { int[3] x; x = {5, 6, 7}; return f(x); }\r\n\
int result;\r\n\
{\r\n\
  int local1 = 5, local2 = 2;\r\n\
  typeof(foobar) local3 = foobar;\r\n\
\r\n\
  int bar(int ref(int[3]) x){ return local3(x); }\r\n\
  int foo(int[3] x){ return local1 + x[local2]; }\r\n\
\r\n\
  result = bar(foo);\r\n\
}\r\n\
\r\n\
char ee;\r\n\
return 1;";
TEST("Closure test", testClosure, "1")
{
	CHECK_INT("global", 0, 100, lastFailed);
	CHECK_INT("fp", 0, 0, lastFailed);
	CHECK_INT("res1a", 0, 106, lastFailed);
	CHECK_INT("res1b", 0, 108, lastFailed);
	CHECK_INT("res1c", 0, 110, lastFailed);
	CHECK_INT("res1d", 0, 112, lastFailed);

	CHECK_INT("res2a", 0, 206, lastFailed);
	CHECK_INT("res2b", 0, 208, lastFailed);
	CHECK_INT("res2c", 0, 210, lastFailed);
	CHECK_INT("res2d", 0, 212, lastFailed);

	CHECK_INT("res3a", 0, 306, lastFailed);
	CHECK_INT("res3b", 0, 308, lastFailed);
	CHECK_INT("res3c", 0, 310, lastFailed);
	CHECK_INT("res3d", 0, 312, lastFailed);

	CHECK_INT("result", 0, 12, lastFailed);
}

const char	*testClosure2 = 
"int test(int n, int ref(int) ptr){ return ptr(n); }\r\n\
\r\n\
int n = 5;\r\n\
auto a = int lam_bda(int b){ return b + 5; };\r\n\
auto b = int lam_bda(int b){ return b + n; };\r\n\
\r\n\
int res1 = test(3, int lam_bda(int b){ return b+n; });\r\n\
\r\n\
int resA, resB, resC, resD, resE, resF;\r\n\
void rurr(){\r\n\
  int d = 7;\r\n\
  int ref(int) a0, a1;\r\n\
  a0 = int lam_bda(int b){ return b + 2; };\r\n\
  a1 = int lam_bda(int b){ return b + d; };\r\n\
\r\n\
  resA = a0(3);\r\n\
  resB = a1(3);\r\n\
\r\n\
  auto b0 = int lam_bda(int b){ return b + 2; };\r\n\
  auto b1 = int lam_bda(int b){ return b + d; };\r\n\
\r\n\
  resC = b0(4);\r\n\
  resD = b1(4);\r\n\
\r\n\
  resE = test(5, int lam_bda(int b){ return b + 2; });\r\n\
  resF = test(5, int lam_bda(int b){ return b + d; });\r\n\
}\r\n\
rurr();\r\n\
int c=0;\r\n\
\r\n\
return 1;";
TEST("Closure test 2", testClosure2, "1")
{
	CHECK_INT("n", 0, 5, lastFailed);
	CHECK_INT("a", 0, 0, lastFailed);
	CHECK_INT("b", 0, 0, lastFailed);

	CHECK_INT("res1", 0, 8, lastFailed);
	CHECK_INT("resA", 0, 5, lastFailed);
	CHECK_INT("resB", 0, 10, lastFailed);
	CHECK_INT("resC", 0, 6, lastFailed);

	CHECK_INT("resD", 0, 11, lastFailed);
	CHECK_INT("resE", 0, 7, lastFailed);
	CHECK_INT("resF", 0, 12, lastFailed);
	CHECK_INT("c", 0, 0, lastFailed);
}

const char	*testClosure3 = 
"int func()\r\n\
{\r\n\
  int test(int n, int ref(int) ptr){ return ptr(n); }\r\n\
\r\n\
  int n = 5;\r\n\
  int res1 = test(3, int lam_bda(int b){ return b+n; });\r\n\
  return res1;\r\n\
}\r\n\
\r\n\
return func();";
TEST_RESULT("Closure test 3", testClosure3, "8");

const char	*testClosure4 = 
"int a = 0;\r\n\
{\r\n\
int ff(int ref() f){ return f(); }\r\n\
a = 1 + ff(int f1(){ return 1 + ff(int f2(){ return 1; }); });\r\n\
}\r\n\
int ff(int ref() f){ return f(); }\r\n\
int b = 1 + ff(int f1(){ return 1 + ff(int f2(){ return 1 + ff(int f3(){ return 1; }); }); });\r\n\
return a+b;";
TEST_RESULT("Closure test 4", testClosure4, "7");

const char	*testClosure5 = 
"int r1, r2, r3, r4, r5;\r\n\
{\r\n\
int ff(int ref() f){ return f(); }\r\n\
int ref() a1 = int f1(){ return 1 + ff(int f2(){ return 1; }); };\r\n\
int ref() a2;\r\n\
a2 = int f3(){ return 1 + ff(int f4(){ return 1; }); };\r\n\
r1 = ff(int f5(){ return 1 + ff(int f6(){ return 1; }); });\r\n\
r2 = ff(a1);\r\n\
r3 = ff(a2);\r\n\
r4 = a1();\r\n\
r5 = a2();\r\n\
}\r\n\
return 1;";
TEST("Closure test 5", testClosure5, "1")
{
	CHECK_INT("r1", 0, 2, lastFailed);
	CHECK_INT("r2", 0, 2, lastFailed);
	CHECK_INT("r3", 0, 2, lastFailed);
	CHECK_INT("r4", 0, 2, lastFailed);
	CHECK_INT("r5", 0, 2, lastFailed);
}

const char	*testUpvalues1 =
"auto generator(int i)\r\n\
{\r\n\
	return auto(){ return i++; };\r\n\
}\r\n\
auto a = generator(3);\r\n\
int i1 = a();	// 3\r\n\
auto b = generator(3);\r\n\
int i2 = a();	// 4\r\n\
int i3 = b();	// 3\r\n\
int i4 = a();	// 5\r\n\
return b();		// 4";
TEST("Closure with upvalues test 1", testUpvalues1, "4")
{
	CHECK_INT("i1", 0, 3, lastFailed);
	CHECK_INT("i2", 0, 4, lastFailed);
	CHECK_INT("i3", 0, 3, lastFailed);
	CHECK_INT("i4", 0, 5, lastFailed);
}

const char	*testUpvalues2 =
"auto binder(int x, int ref(int, int) func)\r\n\
{\r\n\
	return auto(int y){ return func(x, y); };\r\n\
}\r\n\
\r\n\
int adder(int x, y){ return x + y; }\r\n\
auto add3 = binder(3, adder);\r\n\
auto add13 = binder(13, adder);\r\n\
\r\n\
int i1 = add3(5);	// 8\r\n\
int i2 = add13(7);	// 20\r\n\
return add3(add13(4));";
TEST("Closure with upvalues test 2", testUpvalues2, "20")
{
	CHECK_INT("i1", 0, 8, lastFailed);
	CHECK_INT("i2", 0, 20, lastFailed);
}

// Test checks if values of recursive function are captured correctly and that at the end of recursive function, it closes only upvalues that target it's stack frame
const char	*testUpvalues3 =
"typedef int ref(int ref) myFunc;\r\n\
myFunc f1, f2;\r\n\
\r\n\
int k1, k2, k3 = 0;\r\n\
int dh;\r\n\
\r\n\
int func(int k)\r\n\
{\r\n\
	int d = 2;\r\n\
	int func1(int r)\r\n\
	{\r\n\
		int b = k += 3;\r\n\
		\r\n\
		int func2(int ref dr)\r\n\
		{\r\n\
			d += 300;\r\n\
			b += 2;\r\n\
			*dr = d;\r\n\
			return b;\r\n\
		}\r\n\
		if(r)\r\n\
		{\r\n\
			k1 = func1(r-1);\r\n\
			f2 = func2;\r\n\
		}else{\r\n\
			f1 = func2;\r\n\
		}\r\n\
		k3 += func2(&dh);\r\n\
		return b;\r\n\
	}\r\n\
	k2 = func1(1);\r\n\
	return d;\r\n\
}\r\n\
int ddd = func(5);\r\n\
int aa, bb;\r\n\
int a = f1(&aa);\r\n\
int b = f2(&bb);\r\n\
return 1;";
TEST("Closure with upvalues test 3", testUpvalues3, "1")
{
	CHECK_INT("k1", 0, 13, lastFailed);
	CHECK_INT("k2", 0, 10, lastFailed);
	CHECK_INT("k3", 0, 23, lastFailed);
	CHECK_INT("dh", 0, 602, lastFailed);
	CHECK_INT("ddd", 0, 602, lastFailed);
	CHECK_INT("aa", 0, 902, lastFailed);
	CHECK_INT("bb", 0, 902, lastFailed);
	CHECK_INT("a", 0, 15, lastFailed);
	CHECK_INT("b", 0, 12, lastFailed);
}

// Partial upvalue list closure
const char	*testUpvalues4 =
"int ref()[4] arr;\r\n\
\r\n\
int func()\r\n\
{\r\n\
	int m = 8;\r\n\
	for(int i = 0; i < 2; i++)\r\n\
	{\r\n\
		int a = 5 * (i + 1);\r\n\
		arr[i*2+0] = auto(){ return a + m; };\r\n\
		int b = i - 3;\r\n\
		arr[i*2+1] = auto(){ return b + m; };\r\n\
	}\r\n\
	m = 12;\r\n\
	return 0;\r\n\
}\r\n\
int clear()\r\n\
{\r\n\
	int[20] clr = 0;\r\n\
	int ref a = &clr[0];\r\n\
	return *a;\r\n\
}\r\n\
func();\r\n\
clear();\r\n\
int i1 = arr[0]();\r\n\
int i2 = arr[1]();\r\n\
int i3 = arr[2]();\r\n\
int i4 = arr[3]();\r\n\
return i1 * 1000000 + i2 * 10000 + i3 * 100 + i4;";
TEST_RESULT("Closure with upvalues test 4", testUpvalues4, "17092210");

const char	*testUpvalues4b =
"int ref()[4] arr;\r\n\
\r\n\
int func()\r\n\
{\r\n\
	int m = 8;\r\n\
	for(int i = 0; i < 2; i++)\r\n\
	{\r\n\
		arr[i*2+0] = auto(){ return 5 * (i + 1) + m; };\r\n\
		arr[i*2+1] = auto(){ return i - 3 + m; };\r\n\
	}\r\n\
	m = 12;\r\n\
	return 0;\r\n\
}\r\n\
int clear()\r\n\
{\r\n\
	int[20] clr = 0;\r\n\
	int ref a = &clr[0];\r\n\
	return *a;\r\n\
}\r\n\
func();\r\n\
clear();\r\n\
int i1 = arr[0]();\r\n\
int i2 = arr[1]();\r\n\
int i3 = arr[2]();\r\n\
int i4 = arr[3]();\r\n\
return i1 * 1000000 + i2 * 10000 + i3 * 100 + i4;";
TEST_RESULT("Closure with upvalues test 4b", testUpvalues4b, "17092210");

const char	*testUpvalues5 =
"int func()\r\n\
{\r\n\
	auto f = auto init() { return 1; };\r\n\
	for (int i = 0; i < 10; ++i)\r\n\
	{\r\n\
		auto temp = f;\r\n\
		f = auto next() { return 1 + temp();  };\r\n\
	}\r\n\
	auto g = f();\r\n\
	return g;\r\n\
}\r\n\
return func();";
TEST_RESULT("Closure with upvalues test 5", testUpvalues5, "11");

const char	*testClassMemberCaptureInLocalFunction1 =
"class A\r\n\
{\r\n\
	int x, y;\r\n\
	auto sum(){ return auto(){ return this.x + y; }; }\r\n\
}\r\n\
auto a = new A;\r\n\
a.x = 4;\r\n\
a.y = 9;\r\n\
auto f = a.sum();\r\n\
return f();";
TEST_RESULT("Class member capture in local functions", testClassMemberCaptureInLocalFunction1, "13");

const char	*testClassMemberCaptureInLocalFunction2 =
"class A\r\n\
{\r\n\
	int x, y;\r\n\
	auto sum(){ return auto(){ return x + y; }; }\r\n\
}\r\n\
auto a = new A;\r\n\
a.x = 4;\r\n\
a.y = 9;\r\n\
auto f = a.sum();\r\n\
return f();";
TEST_RESULT("Class member capture in local functions 2", testClassMemberCaptureInLocalFunction2, "13");

const char	*testExternalAutoDerefVariable =
"int rand(int ref() f) \r\n\
{ \r\n\
	return f();\r\n\
} \r\n\
int[8] array = 4;\r\n\
auto main_func()\r\n\
{\r\n\
	for(i in array)\r\n\
		i = rand(auto(){ return i * 2; });\r\n\
}\r\n\
main_func();\r\n\
return array[2];";
TEST_RESULT("External variable that should be dereferenced automatically.", testExternalAutoDerefVariable, "8");

const char	*testLocalFunctionPrototype =
"int foo()\r\n\
{\r\n\
	int bar();\r\n\
	return bar();\r\n\
	int bar()\r\n\
	{\r\n\
		return 5;\r\n\
	}\r\n\
}\r\n\
return foo();";
TEST_RESULT("Local function prototype", testLocalFunctionPrototype, "5");

const char	*testLocalFunctionPrototype2 =
"int foo(int x)\r\n\
{\r\n\
	int bar();\r\n\
	auto y = bar();\r\n\
	int bar()\r\n\
	{\r\n\
		return -x;\r\n\
	}\r\n\
	return y;\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Local function prototype 2", testLocalFunctionPrototype2, "-5");

const char	*testLocalFunctionPrototype3 =
"int foo(int x)\r\n\
{\r\n\
	int bar();\r\n\
	auto y = bar();\r\n\
	int bar()\r\n\
	{\r\n\
		return -x;\r\n\
	}\r\n\
	return y + bar();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Local function prototype 3", testLocalFunctionPrototype3, "-10");

const char	*testLocalFunctionPrototype4 =
"int foo(int x)\r\n\
{\r\n\
	int bar();\r\n\
	auto y = bar;\r\n\
	int z = y();\r\n\
	int bar()\r\n\
	{\r\n\
		return -x;\r\n\
	}\r\n\
	return z + y() + bar();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Local function prototype 4", testLocalFunctionPrototype4, "-15");

const char	*testLocalFunctionPrototype5 =
"int foo(int x)\r\n\
{\r\n\
	int bar();\r\n\
	auto y = bar;\r\n\
	int bar()\r\n\
	{\r\n\
		return -x;\r\n\
	}\r\n\
	auto z = bar;\r\n\
	return y() + z();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Local function prototype 5", testLocalFunctionPrototype5, "-10");

const char	*testLocalFunctionPrototype6 =
"int foo(int x)\r\n\
{\r\n\
	int bar();\r\n\
	int hell()\r\n\
	{\r\n\
		return bar();\r\n\
	}\r\n\
	auto m = hell();\r\n\
	int bar()\r\n\
	{\r\n\
		return -x;\r\n\
	}\r\n\
	auto z = bar;\r\n\
	return m + z();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Local function prototype 6", testLocalFunctionPrototype6, "-10");

const char	*testLocalFunctionPrototype7 =
"int foo(int x)\r\n\
{\r\n\
	int bar();\r\n\
	int hell()\r\n\
	{\r\n\
		auto y = bar;\r\n\
		return y();\r\n\
	}\r\n\
	auto m = hell();\r\n\
	int bar()\r\n\
	{\r\n\
		return -x;\r\n\
	}\r\n\
	auto z = bar;\r\n\
	return m + z();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Local function prototype 7", testLocalFunctionPrototype7, "-10");

const char	*testLocalFunctionPrototype8 =
"int foo(int x)\r\n\
{\r\n\
	int bar();\r\n\
	int hell()\r\n\
	{\r\n\
		auto y = bar;\r\n\
		return y();\r\n\
	}\r\n\
	auto m = bar();\r\n\
	int bar()\r\n\
	{\r\n\
		auto y = bar;\r\n\
		return -2;\r\n\
	}\r\n\
	auto z = bar;\r\n\
	return m + z();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Local function prototype 8", testLocalFunctionPrototype8, "-4");

const char	*testLocalFunctionPrototype9 =
"int foo(int x)\r\n\
{\r\n\
	int bar();\r\n\
	int hell()\r\n\
	{\r\n\
		auto y = bar;\r\n\
		return y();\r\n\
	}\r\n\
	auto m = bar();\r\n\
	int bar()\r\n\
	{\r\n\
		auto y1 = bar;\r\n\
		auto y2 = bar;\r\n\
		auto y3 = bar;\r\n\
		auto y4 = bar;\r\n\
		auto y5 = bar;\r\n\
		return -2;\r\n\
	}\r\n\
	auto z = bar;\r\n\
	return m + z();\r\n\
}\r\n\
return foo(5);";
TEST_RESULT("Local function prototype 9", testLocalFunctionPrototype9, "-4");

const char	*testLocalFunctionMultilevelReturn =
"auto foo()\r\n\
{\r\n\
	int a = 3;\r\n\
	{\r\n\
		int b = 4;\r\n\
		return auto(){ return a + b; };\r\n\
	}\r\n\
}\r\n\
auto x = foo();\r\n\
return x();";
TEST_RESULT("Local function return through nested blocks", testLocalFunctionMultilevelReturn, "7");

const char	*testLocalFunctionMultilevelReturn2 =
"auto foo()\
{\
int ref() m;\
{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{\
int a = 4; int test(){ return a; } m = test;\
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\
return m;\
}\
return foo()();";
TEST_RESULT("Local function return through nested blocks 2", testLocalFunctionMultilevelReturn2, "4");

const char	*testClosureListCloseIssue =
"int ref() foo(int c)\r\n\
{\r\n\
	int x = 8;\r\n\
	{\r\n\
		int y = 4;\r\n\
\r\n\
		auto bar(){ return x + y; }\r\n\
\r\n\
		y = 20;\r\n\
\r\n\
		if(c)\r\n\
			y = foo(0)();\r\n\
\r\n\
		return bar;\r\n\
	}\r\n\
}\r\n\
\r\n\
return foo(1)();";
TEST_RESULT("Closure list close issue", testClosureListCloseIssue, "36");

const char	*testClosureOnImplicitReturn1 =
"int ref() f;\r\n\
void foo()\r\n\
{\r\n\
	int x = 4;\r\n\
	f = int test(){ return x; };\r\n\
}\r\n\
foo();\r\n\
return f();";
TEST_RESULT("Closure on implicit return 1", testClosureOnImplicitReturn1, "4");

const char	*testClosureOnImplicitReturn2 =
"class Foo\r\n\
{\r\n\
	int y;\r\n\
	int z;\r\n\
	\r\n\
	void ref() f;\r\n\
	\r\n\
	void add(int x)\r\n\
	{\r\n\
		y += x;\r\n\
		f();\r\n\
	}\r\n\
}\r\n\
\r\n\
class Bar\r\n\
{\r\n\
	Foo ref x;\r\n\
	\r\n\
	int lastY;\r\n\
	\r\n\
	void Bar(Foo ref obj)\r\n\
	{\r\n\
		obj.f = void test(){ lastY = 5; };\r\n\
	}\r\n\
}\r\n\
\r\n\
auto foo = new Foo();\r\n\
auto bar = new Bar(foo);\r\n\
\r\n\
foo.add(4);\r\n\
foo.add(5);\r\n\
\r\n\
return foo.y == 9 && foo.z == 0 && bar.lastY == 5;";
TEST_RESULT("Closure on implicit return 2", testClosureOnImplicitReturn2, "1");

const char	*testClosureAlignment1 =
"import std.gc;\r\n\
\r\n\
auto foo(char z, int x)\r\n\
{\r\n\
	auto bar(int ref y)\r\n\
	{\r\n\
		return auto(){ return z + *y; };\r\n\
	}\r\n\
	\r\n\
	int ref p = new int;\r\n\
	*p = 5;\r\n\
	\r\n\
	auto t = bar(p);\r\n\
	p = nullptr;\r\n\
	\r\n\
	GC.CollectMemory();\r\n\
	\r\n\
	new int(20);\r\n\
	\r\n\
	return t;\r\n\
}\r\n\
\r\n\
auto t = foo(100, 2);\r\n\
\r\n\
return t();";
TEST_RESULT("Closure alignment test 1", testClosureAlignment1, "105");

int CheckAlignmentClosure(NULLCRef ptr, int alignment)
{
	intptr_t asInt = (intptr_t)ptr.ptr;
	return asInt % alignment == 0;
}

LOAD_MODULE_BIND(test_alignment_closure, "test.alignment.closure", "int CheckAlignment(auto ref ptr, int alignment);")
{
	nullcBindModuleFunctionHelper("test.alignment.closure", CheckAlignmentClosure, "CheckAlignment", 0);
}

const char	*testClosureAlignment2 =
"import test.alignment.closure;\r\n\
auto x = coroutine auto(){ char x = 1; int y = 2; return CheckAlignment(y, 4); };\r\n\
return x();";
TEST_RESULT("Closure alignment test 2", testClosureAlignment2, "1");

const char	*testClosureAlignment3 =
"import test.alignment.closure;\r\n\
auto x = coroutine auto(){ int x = 1; align(8) int y = 2; return CheckAlignment(y, 8); };\r\n\
auto y = coroutine auto(){ int x = 1; align(16) int y = 2; return CheckAlignment(y, 16); };\r\n\
return x() + y();";
TEST_RESULT("Closure alignment test 3", testClosureAlignment3, "2");

const char	*testClosureAlignment4 =
"import test.alignment.closure;\r\n\
auto x = coroutine auto(){ int[1024] x; align(8) int y = 2; return CheckAlignment(y, 8); };\r\n\
int m = 0;\r\n\
for(int i = 0; i < 128; i++)\r\n\
	m += (coroutine auto(){ int[1024] x; align(16) int y = 2; return CheckAlignment(y, 16); })();\r\n\
return x() + m;";
TEST_RESULT("Closure alignment test 4", testClosureAlignment4, "129");

const char	*testBreakClosure =
"auto foo()\r\n\
{\r\n\
	int ref() f; \r\n\
	for(int k = 0; k < 5; k++)\r\n\
	{\r\n\
		for(int i = 0; i < 5; i++)\r\n\
		{\r\n\
			if(k == 0)\r\n\
				f = auto() { return k * 100 + i; };\r\n\
			if(k == 0 && i == 0)\r\n\
			{ \r\n\
				k = 3;\r\n\
				i = 1;\r\n\
				break 2;\r\n\
			}\r\n\
		}\r\n\
	}\r\n\
	return f;\r\n\
}\r\n\
return foo()();";
TEST_RESULT("Closure on loop break", testBreakClosure, "301");

const char	*testContinueClosure =
"auto foo()\r\n\
{\r\n\
	int ref() f; \r\n\
	for(int k = 0; k < 5; k++)\r\n\
	{\r\n\
		for(int i = 0; i < 5; i++)\r\n\
		{\r\n\
			if(k == 0)\r\n\
				f = auto() { return k * 100 + i; };\r\n\
			if(k == 0 && i == 0)\r\n\
			{ \r\n\
				k = 3;\r\n\
				i = 1;\r\n\
				continue 2;\r\n\
			}\r\n\
		}\r\n\
	}\r\n\
	return f;\r\n\
}\r\n\
return foo()();";
TEST_RESULT("Closure on loop continue", testContinueClosure, "301");

const char	*testFunctionPointerUpvalueClosure =
"auto runner()\r\n\
{\r\n\
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
		return m;\r\n\
	}\r\n\
	return foo(5)() + k();\r\n\
}\r\n\
int result = runner();\r\n\
\r\n\
return result;";
TEST_RESULT("Closure of function pointer upvalue", testFunctionPointerUpvalueClosure, "25");

const char	*testBlockClosuresGlobalCode =
"int ref()[10] arr;\r\n\
\r\n\
for(int i = 0; i < 10; i++)\r\n\
	arr[i] = auto(){ return i * 2; };\r\n\
\r\n\
return arr[3]() + arr[7]();";
TEST_RESULT("Closure of local variables in global code", testBlockClosuresGlobalCode, "20");

const char	*testBreakClosureGlobalCode =
"int ref() f; \r\n\
for(int k = 0; k < 5; k++)\r\n\
{\r\n\
	for(int i = 0; i < 5; i++)\r\n\
	{\r\n\
		if(k == 0)\r\n\
			f = auto() { return k * 100 + i; };\r\n\
		if(k == 0 && i == 0)\r\n\
		{ \r\n\
			k = 3;\r\n\
			i = 1;\r\n\
			break 2;\r\n\
		}\r\n\
	}\r\n\
}\r\n\
return f();";
TEST_RESULT("Closure on loop break in global code", testBreakClosureGlobalCode, "301");

const char	*testContinueClosureGlobalCode =
"int ref() f; \r\n\
for(int k = 0; k < 5; k++)\r\n\
{\r\n\
	for(int i = 0; i < 5; i++)\r\n\
	{\r\n\
		if(k == 0)\r\n\
			f = auto() { return k * 100 + i; };\r\n\
		if(k == 0 && i == 0)\r\n\
		{ \r\n\
			k = 3;\r\n\
			i = 1;\r\n\
			continue 2;\r\n\
		}\r\n\
	}\r\n\
}\r\n\
return f();";
TEST_RESULT("Closure on loop continue in global code", testContinueClosureGlobalCode, "301");
