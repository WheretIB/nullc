#include "TestBase.h"

#include <stdint.h>

bool TestBoolExtra(int, bool a)
{
	return a;
}

char TestCharExtra(int, char a)
{
	return a;
}

short TestShortExtra(int, short a)
{
	return a;
}

int TestInt(int a)
{
	return a;
}

long long TestLong(long long a)
{
	return a;

}
float TestFloat(float a)
{
	return a;
}

double TestDouble(double a)
{
	return a;
}

int TestExt2(char a, short b, int c, long long d, char e, short f, int g, long long h, char i, short j, int k, long long l)
{
	return a == 1 && b == 2 && c == 3 && d == 4 && e == 5 && f == 6 && g == 7 && h == 8 && i == 9 && j == 10 && k == 11 && l == 12;
}

int TestExt3(char a, short b, int c, long long d, char e, short f, int g, long long h, char i, short j, int k, long long l)
{
	return a == -1 && b == -2 && c == -3 && d == -4 && e == -5 && f == -6 && g == -7 && h == -8 && i == -9 && j == -10 && k == -11 && l == -12;
}

int TestExt4(char a, short b, int c, long long d, float e, double f, char g, short h, int i, long long j, float k, double l)
{
	return a == -1 && b == -2 && c == -3 && d == -4 && e == -5.0f && f == -6.0 && g == -7 && h == -8 && i == -9 && j == -10 && k == -11.0f && l == -12.0;
}

int TestExt4d(float a, double b, int c, long long d, float e, double f, float g, double h, int i, long long j, float k, double l)
{
	return a == -1.0f && b == -2.0 && c == -3 && d == -4 && e == -5.0f && f == -6.0 && g == -7.0f && h == -8.0 && i == -9 && j == -10 && k == -11.0f && l == -12.0;
}

int TestExt5(char a, short b, int c, char d, short e, int f)
{
	return a == -1 && b == -2 && c == -3 && d == -4 && e == -5 && f == -6;
}

int TestExt6(char a, short b, int c, long long d, long long e, int f)
{
	return a == -1 && b == -2 && c == -3 && d == -4 && e == -5 && f == -6;
}

int TestExt7(char a, short b, double c, double d, long long e, int f)
{
	return a == -1 && b == -2 && c == -3.0 && d == -4.0 && e == -5 && f == -6;
}

int TestExt8(float a, float b, double c, double d, float e, float f)
{
	return a == -1.0f && b == -2.0f && c == -3.0 && d == -4.0 && e == -5.0f && f == -6.0f;
}

int TestExt8_ex(float a, float b, double c, double d, float e, float f, double g, double h, float i, float j)
{
	return a == -1.0f && b == -2.0f && c == -3.0 && d == -4.0 && e == -5.0f && f == -6.0f && g == -7.0 && h == -8.0 && i == -9.0f && j == -10.0f;
}

int TestExt9(char a, short b, int c, long long d, float e, double f)
{
	return a == -1 && b == -2 && c == -3 && d == -4 && e == -5.0f && f == -6.0;
}

void* TestGetPtr(int i)
{
	return (void*)(intptr_t)(0x80000000 | i);
}

int TestExt10(void* a, int b, long long c, void* d)
{
	return ((intptr_t)a) == (intptr_t)(0x80000000u | 1) && b == -2 && c == -3 && ((intptr_t)d) == (intptr_t)(0x80000000u | 4);
}

struct TestExt11Foo{};
int TestExt11(char a, TestExt11Foo b, int c, TestExt11Foo d, float e, double f)
{
	(void)b;
	(void)d;
	return a == -1 && c == -3 && e == -5.0f && f == -6.0;
}

struct TestExt12Foo{ int x; };
int TestExt12(NULLCArray a, NULLCArray b, TestExt12Foo u)
{
	return a.len == 2 && ((int*)a.ptr)[0] == 1 && ((int*)a.ptr)[1] == 2 && b.len == 2 && ((int*)b.ptr)[0] == 3 && ((int*)b.ptr)[1] == 4 && u.x == 4;
}

NULLCArray TestExt13(NULLCArray a, NULLCArray b, TestExt12Foo u)
{
	bool res = a.len == 2 && ((int*)a.ptr)[0] == 1 && ((int*)a.ptr)[1] == 2 && b.len == 2 && ((int*)b.ptr)[0] == 3 && ((int*)b.ptr)[1] == 4 && u.x == 4;
	((int*)a.ptr)[1] = res;
	return a;
}

NULLCArray TestExt14(NULLCArray a)
{
	((int*)a.ptr)[1] = 1;
	return a;
}

int TestExt14e(NULLCArray a)
{
	return ((int*)a.ptr)[0] + ((int*)a.ptr)[1];
}

struct TestExtF1Foo{ char a; };
int TestExtF1(TestExtF1Foo x)
{
	return x.a == -1;
}
struct TestExtF2Foo{ short a; };
int TestExtF2(TestExtF2Foo x)
{
	return x.a == -2;
}
struct TestExtF3Foo{ int a; };
int TestExtF3(TestExtF3Foo x)
{
	return x.a == -3;
}
struct TestExtF4Foo{ long long a; };
int TestExtF4(TestExtF4Foo x)
{
	return x.a == -4;
}
struct TestExtF5Foo{ float a; };
int TestExtF5(TestExtF5Foo x)
{
	return x.a == -5.0f;
}
struct TestExtF6Foo{ double a; };
int TestExtF6(TestExtF6Foo x)
{
	return x.a == -6.0;
}
struct TestExtGFoo{ long long a, b, c; };
int TestExtG(TestExtGFoo x)
{
	return x.a == -1 && x.b == -2 && x.c == -3;
}

int TestExtC2(NULLCRef x)
{
	return x.typeID == 4 && *(int*)x.ptr == 3;
}

NULLCRef TestExtC3(NULLCRef x)
{
	*(int*)x.ptr = x.typeID == 4 && *(int*)x.ptr == 3;
	return x;
}

struct TestExtG2Foo{ char a; int b; };
int TestExtG2(TestExtG2Foo x)
{
	return x.a == -1 && x.b == -2;
}
struct TestExtG3Foo{ int a; char b; };
int TestExtG3(TestExtG3Foo x)
{
	return x.a == -1 && x.b == -2;
}
struct TestExtG4Foo{ int a; char b; short c; };
int TestExtG4(TestExtG4Foo x)
{
	return x.a == -1 && x.b == -2 && x.c == -3;
}
struct TestExtG5Foo{ int a; short b; char c; };
int TestExtG5(TestExtG5Foo x)
{
	return x.a == -1 && x.b == -2 && x.c == -3;
}
#pragma pack(push, 4)
struct TestExtG6Foo{ int a; int *b; };
#pragma pack(pop)
int TestExtG6(TestExtG6Foo x)
{
	return x.a == -1 && *x.b == -2;
}
struct TestExtG7Foo{ float a; double b; };
int TestExtG7(TestExtG7Foo x)
{
	return x.a == -1.0f && x.b == -2.0;
}

struct TestExtG2bFoo{ char a; int b; };
TestExtG2bFoo TestExtG2b(TestExtG2bFoo x)
{
	x.a = x.a == -1 && x.b == -2;
	return x;
}
struct TestExtG3bFoo{ int a; char b; };
TestExtG3bFoo TestExtG3b(TestExtG3bFoo x)
{
	x.a = x.a == -1 && x.b == -2;
	return x;
}
struct TestExtG4bFoo{ int a; char b; short c; };
TestExtG4bFoo TestExtG4b(TestExtG4bFoo x)
{
	x.a = x.a == -1 && x.b == -2 && x.c == -3;
	return x;
}
struct TestExtG5bFoo{ int a; short b; char c; };
TestExtG5bFoo TestExtG5b(TestExtG5bFoo x)
{
	x.a = x.a == -1 && x.b == -2 && x.c == -3;
	return x;
}
#pragma pack(push, 4)
struct TestExtG6bFoo{ int a; int *b; };
#pragma pack(pop)
TestExtG6bFoo TestExtG6b(TestExtG6bFoo x)
{
	x.a = x.a == -1 && *x.b == -2;
	return x;
}

struct TestExtG7bFoo{ int a; };
TestExtG7bFoo TestExtG7b(NULLCRef x)
{
	TestExtG7bFoo ret;
	ret.a = x.typeID == 4 && *(int*)x.ptr == 3;
	return ret;
}

int TestExtK(int* a, int* b)
{
	return *a == 1 && *b == 2;
}

int TestExtK2(NULLCRef x, int* b)
{
	return x.typeID == 4 && *(int*)x.ptr == 3 && *b == 2;
}

NULLCRef TestExtK3(NULLCRef x, int* b)
{
	*b = (x.typeID == 4 && *(int*)x.ptr == 3 && *b == 2) ? 20 : 0;
	x.typeID = 4;
	*(int*)x.ptr = 30;
	return x;
}

int TestExtK4(NULLCRef x, NULLCRef y, NULLCRef z, NULLCRef w, NULLCRef a, NULLCRef b, NULLCRef c)
{
	return *(int*)x.ptr == 1 && *(int*)y.ptr == 2 && *(int*)z.ptr == 3 &&
			*(int*)w.ptr == 4 && *(int*)a.ptr == 5 && *(int*)b.ptr == 6 && *(int*)c.ptr == 7;
}

int TestExtK5(NULLCArray x, NULLCArray y, NULLCArray z, int w)
{
	return *(int*)x.ptr == 10 && *(int*)y.ptr == 20 && *(int*)z.ptr == 30 &&
			w == 4 && x.len == 1 && y.len == 2 && z.len == 3;
}

int TestExtK6(NULLCArray x, int w, NULLCArray y, NULLCArray z)
{
	return *(int*)x.ptr == 10 && *(int*)y.ptr == 20 && *(int*)z.ptr == 30 &&
			w == 4 && x.len == 1 && y.len == 2 && z.len == 3;
}

void TestExtL(int* x)
{
	*x = 1;
}

int TestExtM1(double x1, double x2, double x3, double x4)
{
#define fpart(x) (x - floor(x))
	double xgap = fpart(x1 + 0.5);
	return int(xgap * 10.0) == 1 && x2 == 2.0 && x3 == 3.0 && x4 == 4.0;
#undef fpart
}

int TestExtM2(double x1, double x2, double x3, double x4, int y)
{
#define fpart(x) (x - floor(x))
	double xgap = fpart(x1 + 0.5);
	return int(xgap * 10.0) == 1 && y && x2 == 2.0 && x3 == 3.0 && x4 == 4.0;
#undef fpart
}

int TestExtM3(double x1, double x2, double x3, double x4, int* y)
{
#define fpart(x) (x - floor(x))
	double xgap = fpart(x1 + 0.5);
	return int(xgap * 10.0) == 1 && y && x2 == 2.0 && x3 == 3.0 && x4 == 4.0;
#undef fpart
}

int TestExtM4(double x1, double x2, double x3, double x4, int y, int z)
{
#define fpart(x) (x - floor(x))
	double xgap = fpart(x1 + 0.5);
	return int(xgap * 10.0) == 1 && y && x2 == 2.0 && x3 == 3.0 && x4 == 4.0 && z;
#undef fpart
}

struct TestExtG8Foo
{
	float a; float b; int c;
};
int TestExtG8(TestExtG8Foo x)
{
	return x.a == -1.0f && x.b == -2.0f && x.c == -3;
}

struct TestExtG9Foo
{
	long long a; float b; float c;
};
int TestExtG9(TestExtG9Foo x)
{
	return x.a == -1 && x.b == -2.0f && x.c == -3.0f;
}

struct TestExtG10FooSub
{
	int a; int b; int c;
};
struct TestExtG10Foo
{
	TestExtG10FooSub a; float b;
};
int TestExtG10(TestExtG10Foo x)
{
	return x.a.a == -1 && x.a.b == -2 && x.a.c == -3 && x.b == -4.0f;
}

struct TestExtG11Foo
{
	NULLCArray a;
};
int TestExtG11(TestExtG11Foo x)
{
	return x.a.len == 2 && ((int*)x.a.ptr)[0] == 1 && ((int*)x.a.ptr)[1] == 2;
}

struct TestExtG12Foo
{
	double a; double b;
};
int TestExtG12(float x, float y, float z, TestExtG12Foo a, TestExtG12Foo b, TestExtG12Foo c)
{
	return x == 1.0 && y == 2.0 && z == 3.0 && a.a == 4.0 && a.b == 5.0 && b.a == 6.0 && b.b == 7.0 && c.a == 8.0 && c.b == 9.0;
}

struct ReturnBig1
{
	int a, b, c, d;
};
ReturnBig1 TestReturnBig1()
{
	ReturnBig1 x = { 1, 2, 3, 4 };
	return x;
}

struct ReturnBig2
{
	float a, b, c;
};
ReturnBig2 TestReturnBig2()
{
	ReturnBig2 x = { 1, 2, 3 };
	return x;
}

struct ReturnBig3
{
	float a; double b;
};
ReturnBig3 TestReturnBig3()
{
	ReturnBig3 x = { 1, 2 };
	return x;
}

struct ReturnBig4
{
	int a; float b;
};
ReturnBig4 TestReturnBig4()
{
	ReturnBig4 x = { 1, 2 };
	return x;
}

struct ReturnBig5
{
	int a; double b;
};
ReturnBig5 TestReturnBig5()
{
	ReturnBig5 x = { 1, 2 };
	return x;
}

struct ReturnBig6
{
	double a; int b;
};
ReturnBig6 TestReturnBig6()
{
	ReturnBig6 x = { 4, 8 };
	return x;
}

struct ReturnBig7
{
	float a; int b;
};
ReturnBig7 TestReturnBig7()
{
	ReturnBig7 x = { 2, 16 };
	return x;
}

struct ReturnBig8
{
	float a; float b; int c;
};
ReturnBig8 TestReturnBig8()
{
	ReturnBig8 x = { 4, 8, 16 };
	return x;
}

struct ReturnSmall1
{
	float a;
};
ReturnSmall1 TestReturnSmall1()
{
	ReturnSmall1 x = { 4 };
	return x;
}

struct ReturnSmall2
{
	double a;
};
ReturnSmall2 TestReturnSmall2()
{
	ReturnSmall2 x = { 6 };
	return x;
}

struct ReturnSmall3
{
};
ReturnSmall3 TestReturnSmall3()
{
	ReturnSmall3 x;
	return x;
}

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)

#define LOAD_MODULE_BIND_(id, name, code) LOAD_MODULE_BIND(id##_Raw, name, code)
#define TEST_CODE(x) x##_Raw
#define BIND_FUNCTION(moduleId, function, name, index) nullcBindModuleFunction(moduleId, (void (*)())function, name, index);
#define TEST_RESULT_(name, id, result) TEST_RESULT(name " [raw]", id##_Raw, result)
#define MODULE_SUFFIX ""
#define ALL_EXTERNAL_CALLS 0

#include "TestExternalCallInt.h"

#undef LOAD_MODULE_BIND_
#undef TEST_CODE
#undef BIND_FUNCTION
#undef TEST_RESULT_
#undef MODULE_SUFFIX
#undef ALL_EXTERNAL_CALLS

#endif

#define LOAD_MODULE_BIND_(id, name, code) LOAD_MODULE_BIND(id##_Wrap, name ".wrap", code)
#define TEST_CODE(x) x##_Wrap
#define BIND_FUNCTION(moduleId, function, name, index) nullcBindModuleFunctionHelper(moduleId ".wrap", function, name, index);
#define TEST_RESULT_(name, id, result) TEST_RESULT(name " [wrap]", id##_Wrap, result)
#define MODULE_SUFFIX ".wrap"
#define ALL_EXTERNAL_CALLS 1

#include "TestExternalCallInt.h"

#undef LOAD_MODULE_BIND_
#undef TEST_CODE
#undef BIND_FUNCTION
#undef TEST_RESULT_
#undef MODULE_SUFFIX
#undef ALL_EXTERNAL_CALLS

#if !defined(ANDROID)

const char	*testFile = 
"// File and something else test\r\n\
import std.file;\r\n\
int test(char[] eh, int u){ int b = 0; for(int i=0;i<u;i++)b+=eh[i]; return b; }\r\n\
\r\n\
auto uh = \"ehhhe\";\r\n\
int k = 5464321;\r\n\
File n = File(\"" FILE_PATH "haha.txt\", \"wb\");\r\n\
auto text = \"Hello file!!!\";\r\n\
n.Write(text);\r\n\
\r\n\
n.Write(k);\r\n\
n.Close();\r\n\
return test(uh, 3);";
TEST("File and something else test", testFile, "309")
{
	CHECK_STR("uh", 0, "ehhhe", lastFailed);
	CHECK_STR("text", 0, "Hello file!!!", lastFailed);

	CHECK_INT("k", 0, 5464321, lastFailed);

	remove(FILE_PATH "haha.txt");
}

const char	*testFile2 = 
"//File test\r\n\
import std.file;\r\n\
auto name = \"" FILE_PATH "extern.bin\";\r\n\
auto acc = \"wb\", acc2 = \"rb\";\r\n\
\r\n\
// Perform to write\r\n\
auto text = \"Hello again\";\r\n\
char ch = 45;\r\n\
short sh = 4*256+45;\r\n\
int num = 12568;\r\n\
long lnum = 4586564;\r\n\
\r\n\
// Write to file\r\n\
File test = File(name, acc);\r\n\
test.Write(text);\r\n\
test.Write(ch);\r\n\
test.Write(sh);\r\n\
test.Write(num);\r\n\
test.Write(lnum);\r\n\
test.Write(ch);\r\n\
test.Write(sh);\r\n\
test.Write(num);\r\n\
test.Write(lnum);\r\n\
test.Close();\r\n\
\r\n\
// Perform to read\r\n\
char[12] textR1;\r\n\
char chR1, chR2;\r\n\
short shR1, shR2;\r\n\
int numR1, numR2;\r\n\
long lnumR1, lnumR2;\r\n\
\r\n\
test.Open(name, acc2);\r\n\
test.Read(textR1);\r\n\
test.Read(&chR1);\r\n\
test.Read(&shR1);\r\n\
test.Read(&numR1);\r\n\
test.Read(&lnumR1);\r\n\
test.Read(&chR2);\r\n\
test.Read(&shR2);\r\n\
test.Read(&numR2);\r\n\
test.Read(&lnumR2);\r\n\
test.Close();\r\n\
return 1;";
TEST("File test 2", testFile2, "1")
{
	CHECK_STR("name", 0, FILE_PATH "extern.bin", lastFailed);
	CHECK_STR("acc", 0, "wb", lastFailed);
	CHECK_STR("acc2", 0, "rb", lastFailed);
	CHECK_STR("text", 0, "Hello again", lastFailed);
	CHECK_STR("textR1", 0, "Hello again", lastFailed);

	CHECK_CHAR("ch", 0, 45, lastFailed);
	CHECK_SHORT("sh", 0, 1069, lastFailed);
	CHECK_INT("num", 0, 12568, lastFailed);
	CHECK_LONG("lnum", 0, 4586564, lastFailed);

	CHECK_CHAR("chR1", 0, 45, lastFailed);
	CHECK_SHORT("shR1", 0, 1069, lastFailed);
	CHECK_INT("numR1", 0, 12568, lastFailed);
	CHECK_LONG("lnumR1", 0, 4586564, lastFailed);

	CHECK_CHAR("chR2", 0, 45, lastFailed);
	CHECK_SHORT("shR2", 0, 1069, lastFailed);
	CHECK_INT("numR2", 0, 12568, lastFailed);
	CHECK_LONG("lnumR2", 0, 4586564, lastFailed);

	remove(FILE_PATH "extern.bin");
}

#endif

NULLCRef NullcCallReturnTest(NULLCRef function)
{
	NULLCFuncPtr functionValue = *(NULLCFuncPtr*)function.ptr;

	nullcCallFunction(functionValue);

	return nullcGetResultObject();
}

LOAD_MODULE_BIND(test_call_return_values, "test.call_return_values", "auto ref call(auto ref function);")
{
	nullcBindModuleFunctionHelper("test.call_return_values", NullcCallReturnTest, "call", 0);
}

const char	*testCallReturnValues =
"import test.call_return_values;\r\n\
\r\n\
class Test{ int a = 1, b = 2; }\r\n\
enum Test2{ A, B, C, D }\r\n\
\r\n\
auto ref x0 = call(auto(){});\r\n\
assert(x0.type == void);\r\n\
auto ref x1 = call(auto(){ return true; });\r\n\
assert(x1.type == bool);\r\n\
bool y1 = x1;\r\n\
assert(y1 == true);\r\n\
auto ref x2 = call(auto(){ return 'a'; });\r\n\
assert(x2.type == char);\r\n\
char y2 = x2;\r\n\
assert(y2 == 'a');\r\n\
auto ref x3 = call(auto(){ return short(2); });\r\n\
assert(x3.type == short);\r\n\
short y3 = x3;\r\n\
assert(y3 == 2);\r\n\
auto ref x4 = call(auto(){ return 3; });\r\n\
assert(x4.type == int);\r\n\
int y4 = x4;\r\n\
assert(y4 == 3);\r\n\
auto ref x5 = call(auto(){ return 4l; });\r\n\
assert(x5.type == long);\r\n\
long y5 = x5;\r\n\
assert(y5 == 4);\r\n\
auto ref x6 = call(auto(){ return 5.0f; });\r\n\
assert(x6.type == float);\r\n\
float y6 = x6;\r\n\
assert(y6 == 5.0f);\r\n\
auto ref x7 = call(auto(){ return 6.0; });\r\n\
assert(x7.type == double);\r\n\
double y7 = x7;\r\n\
assert(y7 == 6.0);\r\n\
auto ref x8 = call(auto(){ return short; });\r\n\
assert(x8.type == typeid);\r\n\
typeid y8 = x8;\r\n\
assert(y8 == short);\r\n\
auto ref x9 = call(auto(){ return nullptr; });\r\n\
assert(x9.type == __nullptr);\r\n\
__nullptr y9 = x9;\r\n\
assert(y9 == nullptr);\r\n\
auto ref x10 = call(auto(){ auto ref r = 7; return r; });\r\n\
assert(x10.type == int);\r\n\
int y10 = x10;\r\n\
assert(y10 == 7);\r\n\
auto ref x11 = call(auto(){ auto[] r = { 1, 2, 3 }; return r; });\r\n\
assert(x11.type == auto[]);\r\n\
auto[] y11 = x11;\r\n\
assert(int(y11[0]) == 1 && int(y11[1]) == 2);\r\n\
auto ref x12 = call(auto(){ return \"hello\"; });\r\n\
assert(x12.type == char[6]);\r\n\
char[6] y12 = x12;\r\n\
assert(y12 == \"hello\");\r\n\
auto ref x13 = call(auto(){ return char[](\"hello\"); });\r\n\
assert(x13.type == char[]);\r\n\
char[] y13 = x13;\r\n\
assert(y13 == \"hello\");\r\n\
auto ref x14 = call(auto(){ return new int(8); });\r\n\
assert(x14.type == int ref);\r\n\
int ref ref y14 = x14;\r\n\
assert(* *y14 == 8);\r\n\
auto ref x15 = call(auto(){ return Test(); });\r\n\
assert(x15.type == Test);\r\n\
Test y15 = x15;\r\n\
assert(y15.a == 1 && y15.b == 2);\r\n\
auto ref x16 = call(auto(){ return Test2.C; });\r\n\
assert(x16.type == Test2);\r\n\
Test2 y16 = x16;\r\n\
assert(y16 == Test2.C);\r\n\
\r\n\
return 1;";
TEST_RESULT("Return of different types from nullc function calls", testCallReturnValues, "1");
