#include "TestBase.h"

#ifndef NULLC_ENABLE_C_TRANSLATION
#ifndef _MSC_VER
	#include <stdint.h>
#endif

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

LOAD_MODULE_BIND(test_ext1, "test.ext1", "char char_(char a); short short_(short a); int int_(int a); long long_(long a); float float_(float a); double double_(double a);")
{
	nullcBindModuleFunction("test.ext1", (void (*)())TestInt, "char_", 0);
	nullcBindModuleFunction("test.ext1", (void (*)())TestInt, "short_", 0);
	nullcBindModuleFunction("test.ext1", (void (*)())TestInt, "int_", 0);
	nullcBindModuleFunction("test.ext1", (void (*)())TestLong, "long_", 0);
	nullcBindModuleFunction("test.ext1", (void (*)())TestFloat, "float_", 0);
	nullcBindModuleFunction("test.ext1", (void (*)())TestDouble, "double_", 0);
}

const char	*testExternalCallBasic1 = "import test.ext1;\r\n	auto Char = char_;\r\n		return Char(24);";
TEST_RESULT("External function call. char type.", testExternalCallBasic1, "24");
const char	*testExternalCallBasic2 = "import test.ext1;\r\n	auto Short = short_;\r\n	return Short(57);";
TEST_RESULT("External function call. short type.", testExternalCallBasic2, "57");
const char	*testExternalCallBasic3 = "import test.ext1;\r\n		auto Int = int_;\r\n		return Int(2458);";
TEST_RESULT("External function call. int type.", testExternalCallBasic3, "2458");
const char	*testExternalCallBasic4 = "import test.ext1;\r\n	auto Long = long_;\r\n		return Long(14841324198l);";
TEST_RESULT("External function call. long type.", testExternalCallBasic4, "14841324198L");
const char	*testExternalCallBasic5 = "import test.ext1;\r\n	auto Float = float_;\r\n	return int(Float(3.0));";
TEST_RESULT("External function call. float type.", testExternalCallBasic5, "3");
const char	*testExternalCallBasic6 = "import test.ext1;\r\n	auto Double = double_;\r\n	return int(Double(2.0));";
TEST_RESULT("External function call. double type.", testExternalCallBasic6, "2");

// Tests check parameter passing through stack, so PS3 is disabled, since such external functions are unsupported
#if !defined(__CELLOS_LV2__)

LOAD_MODULE_BIND(test_ext2, "test.ext2", "int Call(char a, short b, int c, long d, char e, short f, int g, long h, char i, short j, int k, long l);")
{
	nullcBindModuleFunction("test.ext2", (void (*)())TestExt2, "Call", 0);
}
const char	*testExternalCall2 =
"import test.ext2;\r\n\
return Call(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);";
TEST_RESULT("External function call. Integer types, arguments through stack.", testExternalCall2, "1");

LOAD_MODULE_BIND(test_ext3, "test.ext3", "int Call(char a, short b, int c, long d, char e, short f, int g, long h, char i, short j, int k, long l);")
{
	nullcBindModuleFunction("test.ext3", (void (*)())TestExt3, "Call", 0);
}
const char	*testExternalCall3 =
"import test.ext3;\r\n\
return Call(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12);";
TEST_RESULT("External function call. Integer types, arguments through stack. Sign-extend.", testExternalCall3, "1");

LOAD_MODULE_BIND(test_ext4, "test.ext4", "int Call(char a, short b, int c, long d, float e, double f, char g, short h, int i, long j, float k, double l);")
{
	nullcBindModuleFunction("test.ext4", (void (*)())TestExt4, "Call", 0);
}
const char	*testExternalCall4 =
"import test.ext4;\r\n\
return Call(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12);";
TEST_RESULT("External function call. Basic types, arguments through stack. sx", testExternalCall4, "1");

LOAD_MODULE_BIND(test_ext4d, "test.ext4d", "int Call(float a, double b, int c, long d, float e, double f, float g, double h, int i, long j, float k, double l);")
{
	nullcBindModuleFunction("test.ext4d", (void (*)())TestExt4d, "Call", 0);
}
const char	*testExternalCall4d =
"import test.ext4d;\r\n\
return Call(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12);";
TEST_RESULT("External function call. Basic types (more FP), arguments through stack. sx", testExternalCall4d, "1");
#endif

LOAD_MODULE_BIND(test_ext5, "test.ext5", "int Call(char a, short b, int c, char d, short e, int f);")
{
	nullcBindModuleFunction("test.ext5", (void (*)())TestExt5, "Call", 0);
}
const char	*testExternalCall5 =
"import test.ext5;\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT("External function call. Integer types (-long), arguments in registers. sx", testExternalCall5, "1");

LOAD_MODULE_BIND(test_ext6, "test.ext6", "int Call(char a, short b, int c, long d, long e, int f);")
{
	nullcBindModuleFunction("test.ext6", (void (*)())TestExt6, "Call", 0);
}
const char	*testExternalCall6 =
"import test.ext6;\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT("External function call. Integer types, arguments in registers. sx", testExternalCall6, "1");

LOAD_MODULE_BIND(test_ext7, "test.ext7", "int Call(char a, short b, double c, double d, long e, int f);")
{
	nullcBindModuleFunction("test.ext7", (void (*)())TestExt7, "Call", 0);
}
const char	*testExternalCall7 =
"import test.ext7;\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT("External function call. Integer types and double, arguments in registers. sx", testExternalCall7, "1");

LOAD_MODULE_BIND(test_ext8, "test.ext8", "int Call(float a, float b, double c, double d, float e, float f);")
{
	nullcBindModuleFunction("test.ext8", (void (*)())TestExt8, "Call", 0);
}
const char	*testExternalCall8 =
"import test.ext8;\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT("External function call. float and double, arguments in registers. sx", testExternalCall8, "1");

LOAD_MODULE_BIND(test_ext8ex, "test.ext8ex", "int Call(float a, b, double c, d, float e, f, double g, h, float i, j);")
{
	nullcBindModuleFunction("test.ext8ex", (void (*)())TestExt8_ex, "Call", 0);
}
const char	*testExternalCall8Ex =
"import test.ext8ex;\r\n\
return Call(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10);";
TEST_RESULT("External function call. more float and double, registers. sx", testExternalCall8Ex, "1");

LOAD_MODULE_BIND(test_ext9, "test.ext9", "int Call(char a, short b, int c, long d, float e, double f);")
{
	nullcBindModuleFunction("test.ext9", (void (*)())TestExt9, "Call", 0);
}
const char	*testExternalCall9 =
"import test.ext9;\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT("External function call. Basic types, arguments in registers. sx", testExternalCall9, "1");

LOAD_MODULE_BIND(test_extA, "test.extA", "void ref GetPtr(int i); int Call(void ref a, int b, long c, void ref d);")
{
	nullcBindModuleFunction("test.extA", (void (*)())TestGetPtr, "GetPtr", 0);
	nullcBindModuleFunction("test.extA", (void (*)())TestExt10, "Call", 0);
}
const char	*testExternalCallA =
"import test.extA;\r\n\
return Call(GetPtr(1), -2, -3, GetPtr(4));";
TEST_RESULT("External function call. Pointer w/o sign extend, arguments in registers. sx", testExternalCallA, "1");

LOAD_MODULE_BIND(test_extB, "test.extB", "class Foo{} int Call(char a, Foo b, int c, Foo d, float e, double f);")
{
	nullcBindModuleFunction("test.extB", (void (*)())TestExt11, "Call", 0);
}
const char	*testExternalCallB =
"import test.extB;\r\n\
Foo a, b;\r\n\
return Call(-1, a, -3, b, -5, -6);";
TEST_RESULT("External function call. Class types sizeof() == 0, arguments in registers. sx", testExternalCallB, "1");

LOAD_MODULE_BIND(test_exte, "test.exte", "int Call(int[] a);")
{
	nullcBindModuleFunction("test.exte", (void (*)())TestExt14e, "Call", 0);
}
const char	*testExternalCalle =
"import test.exte;\r\n\
int[2] arr = { 1, 2 };\r\n\
return Call(arr);";
TEST_RESULT("External function call. int[] argument in registers.", testExternalCalle, "3");

LOAD_MODULE_BIND(test_extE, "test.extE", "int[] Call(int[] a);")
{
	nullcBindModuleFunction("test.extE", (void (*)())TestExt14, "Call", 0);
}
const char	*testExternalCallE =
"import test.extE;\r\n\
int[2] arr = { 1, 0 };\r\n\
return Call(arr)[1];";
TEST_RESULT("External function call. Complex return, arguments in registers.", testExternalCallE, "1");

LOAD_MODULE_BIND(test_extC, "test.extC", "int Call(int[] a, int[] b, typeid u);")
{
	nullcBindModuleFunction("test.extC", (void (*)())TestExt12, "Call", 0);
}
const char	*testExternalCallC =
"import test.extC;\r\n\
int[] arr = new int[2];\r\n\
arr[0] = 1; arr[1] = 2;\r\n\
return Call(arr, {3, 4}, int);";
TEST_RESULT("External function call. Complex built-ins, arguments in registers.", testExternalCallC, "1");

LOAD_MODULE_BIND(test_extC2, "test.extC2", "int Call(auto ref x);")
{
	nullcBindModuleFunction("test.extC2", (void (*)())TestExtC2, "Call", 0);
}
const char *testExternalCallC2 = "import test.extC2; return Call(3);";
TEST_RESULT("External function call. auto ref argument.", testExternalCallC2, "1");

LOAD_MODULE_BIND(test_extC3, "test.extC3", "auto ref Call(auto ref x);")
{
	nullcBindModuleFunction("test.extC3", (void (*)())TestExtC3, "Call", 0);
}
const char *testExternalCallC3 = "import test.extC3; return int(Call(3));";
TEST_RESULT("External function call. auto ref return.", testExternalCallC3, "1");

LOAD_MODULE_BIND(test_extD, "test.extD", "int[] Call(int[] a, int[] b, typeid u);")
{
	nullcBindModuleFunction("test.extD", (void (*)())TestExt13, "Call", 0);
}
const char	*testExternalCallD =
"import test.extD;\r\n\
int[] arr = new int[2];\r\n\
arr[0] = 1; arr[1] = 2;\r\n\
return Call(arr, {3, 4}, int)[1];";
TEST_RESULT("External function call. Complex built-in return, arguments in registers.", testExternalCallD, "1");

LOAD_MODULE_BIND(test_extF, "test.extF",
"class Char{ char a; } int Call(Char a);\r\n\
class Short{ short a; } int Call(Short a);\r\n\
class Int{ int a; } int Call(Int a);\r\n\
class Long{ long a; } int Call(Long a);\r\n\
class Float{ float a; } int Call(Float a);\r\n\
class Double{ double a; } int Call(Double a);")
{
	nullcBindModuleFunction("test.extF", (void (*)())TestExtF1, "Call", 0);
	nullcBindModuleFunction("test.extF", (void (*)())TestExtF2, "Call", 1);
	nullcBindModuleFunction("test.extF", (void (*)())TestExtF3, "Call", 2);
	nullcBindModuleFunction("test.extF", (void (*)())TestExtF4, "Call", 3);
	nullcBindModuleFunction("test.extF", (void (*)())TestExtF5, "Call", 4);
	nullcBindModuleFunction("test.extF", (void (*)())TestExtF6, "Call", 5);
}
const char *testExternalCallClass1 = "import test.extF; Char x; x.a = -1; return Call(x);";
TEST_RESULT("External function call. char inside a class, argument in registers.", testExternalCallClass1, "1");
const char *testExternalCallClass2 = "import test.extF; Short x; x.a = -2; return Call(x);";
TEST_RESULT("External function call. short inside a class, argument in registers.", testExternalCallClass2, "1");
const char *testExternalCallClass3 = "import test.extF; Int x; x.a = -3; return Call(x);";
TEST_RESULT("External function call. int inside a class, argument in registers.", testExternalCallClass3, "1");
const char *testExternalCallClass4 = "import test.extF; Long x; x.a = -4; return Call(x);";
TEST_RESULT("External function call. long inside a class, argument in registers.", testExternalCallClass4, "1");
const char *testExternalCallClass5 = "import test.extF; Float x; x.a = -5; return Call(x);";
TEST_RESULT("External function call. float inside a class, argument in registers.", testExternalCallClass5, "1");
const char *testExternalCallClass6 = "import test.extF; Double x; x.a = -6; return Call(x);";
TEST_RESULT("External function call. double inside a class, argument in registers.", testExternalCallClass6, "1");

LOAD_MODULE_BIND(test_extG, "test.extG", "class Zomg{ long x,y,z; } int Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG", (void (*)())TestExtG, "Call", 0);
}
const char	*testExternalCallG =
"import test.extG;\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
return Call(z);";
TEST_RESULT("External function call. Complex argument (24 bytes) in registers.", testExternalCallG, "1");

LOAD_MODULE_BIND(test_extG2, "test.extG2", "class Zomg{ char x; int y; } int Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG2", (void (*)())TestExtG2, "Call", 0);
}
const char	*testExternalCallG2 =
"import test.extG2;\r\n\
Zomg z; z.x = -1; z.y = -2;\r\n\
return Call(z);";
TEST_RESULT("External function call. { char; int; } in argument.", testExternalCallG2, "1");

LOAD_MODULE_BIND(test_extG3, "test.extG3", "class Zomg{ int x; char y; } int Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG3", (void (*)())TestExtG3, "Call", 0);
}
const char	*testExternalCallG3 =
"import test.extG3;\r\n\
Zomg z; z.x = -1; z.y = -2;\r\n\
return Call(z);";
TEST_RESULT("External function call. { int; char; } in argument.", testExternalCallG3, "1");

LOAD_MODULE_BIND(test_extG4, "test.extG4", "class Zomg{ int x; char y; short z; } int Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG4", (void (*)())TestExtG4, "Call", 0);
}
const char	*testExternalCallG4 =
"import test.extG4;\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
return Call(z);";
TEST_RESULT("External function call. { int; char; short; } in argument.", testExternalCallG4, "1");

LOAD_MODULE_BIND(test_extG5, "test.extG5", "class Zomg{ int x; short y; char z; } int Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG5", (void (*)())TestExtG5, "Call", 0);
}
const char	*testExternalCallG5 =
"import test.extG5;\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
return Call(z);";
TEST_RESULT("External function call. { int; short; char; } in argument.", testExternalCallG5, "1");

LOAD_MODULE_BIND(test_extG6, "test.extG6", "class Zomg{ int x; int ref y; } int Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG6", (void (*)())TestExtG6, "Call", 0);
}
const char	*testExternalCallG6 =
"import test.extG6;\r\n\
int u = -2;\r\n\
Zomg z; z.x = -1; z.y = &u;\r\n\
return Call(z);";
TEST_RESULT("External function call. { int; int ref; } in argument.", testExternalCallG6, "1");

LOAD_MODULE_BIND(test_extG2b, "test.extG2b", "class Zomg{ char x; int y; } Zomg Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG2b", (void (*)())TestExtG2b, "Call", 0);
}
const char	*testExternalCallG2b =
"import test.extG2b;\r\n\
Zomg z; z.x = -1; z.y = -2;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT("External function call. { char; int; } returned.", testExternalCallG2b, "1");

LOAD_MODULE_BIND(test_extG3b, "test.extG3b", "class Zomg{ int x; char y; } Zomg Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG3b", (void (*)())TestExtG3b, "Call", 0);
}
const char	*testExternalCallG3b =
"import test.extG3b;\r\n\
Zomg z; z.x = -1; z.y = -2;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT("External function call. { int; char; } returned.", testExternalCallG3b, "1");

LOAD_MODULE_BIND(test_extG4b, "test.extG4b", "class Zomg{ int x; char y; short z; } Zomg Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG4b", (void (*)())TestExtG4b, "Call", 0);
}
const char	*testExternalCallG4b =
"import test.extG4b;\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT("External function call. { int; char; short; } returned.", testExternalCallG4b, "1");

LOAD_MODULE_BIND(test_extG5b, "test.extG5b", "class Zomg{ int x; short y; char z; } Zomg Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG5b", (void (*)())TestExtG5b, "Call", 0);
}
const char	*testExternalCallG5b =
"import test.extG5b;\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT("External function call. { int; short; char; } returned.", testExternalCallG5b, "1");

LOAD_MODULE_BIND(test_extG6b, "test.extG6b", "class Zomg{ int x; int ref y; } Zomg Call(Zomg a);")
{
	nullcBindModuleFunction("test.extG6b", (void (*)())TestExtG6b, "Call", 0);
}
const char	*testExternalCallG6b =
"import test.extG6b;\r\n\
int u = -2;\r\n\
Zomg z; z.x = -1; z.y = &u;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT("External function call. { int; int ref; } returned.", testExternalCallG6b, "1");

LOAD_MODULE_BIND(test_extK, "test.extK", "int Call(int ref a, b);")
{
	nullcBindModuleFunction("test.extK", (void (*)())TestExtK, "Call", 0);
}
const char	*testExternalCallK = "import test.extK;\r\n int a = 1, b = 2;\r\n	return Call(&a, &b);";
TEST_RESULT("External function call. References.", testExternalCallK, "1");

LOAD_MODULE_BIND(test_extK2, "test.extK2", "int Call(auto ref a, int ref b);")
{
	nullcBindModuleFunction("test.extK2", (void (*)())TestExtK2, "Call", 0);
}
const char	*testExternalCallK2 = "import test.extK2;\r\n int a = 3, b = 2;\r\n	return Call(a, &b);";
TEST_RESULT("External function call. auto ref and int ref.", testExternalCallK2, "1");

LOAD_MODULE_BIND(test_extK3, "test.extK3", "auto ref Call(auto ref a, int ref b);")
{
	nullcBindModuleFunction("test.extK3", (void (*)())TestExtK3, "Call", 0);
}
const char	*testExternalCallK3 = "import test.extK3;\r\n int a = 3, b = 2;\r\nauto ref u = Call(a, &b); return int(u) == 30 && b == 20;";
TEST_RESULT("External function call. auto ref and int ref, auto ref return.", testExternalCallK3, "1");

#if !defined(__CELLOS_LV2__)
LOAD_MODULE_BIND(test_extK4, "test.extK4", "int Call(auto ref x, y, z, w, a, b, c);")
{
	nullcBindModuleFunction("test.extK4", (void (*)())TestExtK4, "Call", 0);
}
const char	*testExternalCallK4 = "import test.extK4;\r\n return Call(1, 2, 3, 4, 5, 6, 7);";
TEST_RESULT("External function call. a lot of auto ref parameters.", testExternalCallK4, "1");

LOAD_MODULE_BIND(test_extK5, "test.extK5", "int Call(int[] x, y, z, int w);")
{
	nullcBindModuleFunction("test.extK5", (void (*)())TestExtK5, "Call", 0);
}
const char	*testExternalCallK5 = "import test.extK5;\r\n return Call({10}, {20,2}, {30,1,2}, 4);";
TEST_RESULT("External function call. int[] fills registers and int on stack", testExternalCallK5, "1");

LOAD_MODULE_BIND(test_extK6, "test.extK6", "int Call(int[] x, int w, int[] y, z);")
{
	nullcBindModuleFunction("test.extK6", (void (*)())TestExtK6, "Call", 0);
}
const char	*testExternalCallK6 = "import test.extK6;\r\n return Call({10}, 4, {20,2}, {30,1,2});";
TEST_RESULT("External function call. Class divided between reg/stack (amd64)", testExternalCallK6, "1");
#endif

LOAD_MODULE_BIND(test_extL, "test.extL", "void Call(int ref a);")
{
	nullcBindModuleFunction("test.extL", (void (*)())TestExtL, "Call", 0);
}
const char	*testExternalCallL = "import test.extL;\r\n int a = 2;\r\nCall(&a); return a;";
TEST_RESULT("External function call. void return type.", testExternalCallL, "1");

LOAD_MODULE_BIND(test_extM1, "test.extM1", "int Call(double a, b, c, d);")
{
	nullcBindModuleFunction("test.extM1", (void (*)())TestExtM1, "Call", 0);
}
const char	*testExternalCallM1 = "import test.extM1;\r\n double a = 1.6; return Call(a, 2.0, 3.0, 4.0);";
TEST_RESULT("External function call. alignment test 1.", testExternalCallM1, "1");

LOAD_MODULE_BIND(test_extM2, "test.extM2", "int Call(double a, b, c, d, int e);")
{
	nullcBindModuleFunction("test.extM2", (void (*)())TestExtM2, "Call", 0);
}
const char	*testExternalCallM2 = "import test.extM2;\r\n double a = 1.6; return Call(a, 2.0, 3.0, 4.0, 5);";
TEST_RESULT("External function call. alignment test 2.", testExternalCallM2, "1");

LOAD_MODULE_BIND(test_extM3, "test.extM3", "class Foo{ int x; } int Foo:Call(double a, b, c, d);")
{
	nullcBindModuleFunction("test.extM3", (void (*)())TestExtM3, "Foo::Call", 0);
}
const char	*testExternalCallM3 = "import test.extM3;\r\n Foo m; m.x = 5; double a = 1.6; return m.Call(a, 2.0, 3.0, 4.0);";
TEST_RESULT("External function call. alignment test 3.", testExternalCallM3, "1");

LOAD_MODULE_BIND(test_extM4, "test.extM4", "int Call(double a, b, c, d, int e, f);")
{
	nullcBindModuleFunction("test.extM4", (void (*)())TestExtM4, "Call", 0);
}
const char	*testExternalCallM4 = "import test.extM4;\r\n double a = 1.6; return Call(a, 2.0, 3.0, 4.0, 5, 6);";
TEST_RESULT("External function call. alignment test 4.", testExternalCallM4, "1");

// big argument tests
// big arguments with int and float/double

#ifdef NULLC_COMPLEX_RETURN

struct ReturnBig1{ int a, b, c, d; };
ReturnBig1 TestReturnBig1()
{
	ReturnBig1 x = { 1, 2, 3, 4 };
	return x;
}
LOAD_MODULE_BIND(test_big1, "test.big1", "class X{ int a, b, c, d; } X Call();")
{
	nullcBindModuleFunction("test.big1", (void (*)())TestReturnBig1, "Call", 0);
}
const char	*testBigReturnType1 = "import test.big1;\r\n X x; x = Call(); return x.a == 1 && x.b == 2 && x.c == 3 && x.d == 4;";
TEST_RESULT("External function call. Big return type 1.", testBigReturnType1, "1");

struct ReturnBig2{ float a, b, c; };
ReturnBig2 TestReturnBig2()
{
	ReturnBig2 x = { 1, 2, 3 };
	return x;
}
LOAD_MODULE_BIND(test_big2, "test.big2", "class X{ float a, b, c; } X Call();")
{
	nullcBindModuleFunction("test.big2", (void (*)())TestReturnBig2, "Call", 0);
}
const char	*testBigReturnType2 = "import test.big2;\r\n X x; x = Call(); return x.a == 1.0f && x.b == 2.0f && x.c == 3.0f;";
TEST_RESULT("External function call. Big return type 2.", testBigReturnType2, "1");

#endif

#endif

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
	CHECK_STR("uh", 0, "ehhhe");
	CHECK_STR("$temp1", 0, FILE_PATH "haha.txt");
	CHECK_STR("$temp2", 0, "wb");
	CHECK_STR("text", 0, "Hello file!!!");

	CHECK_INT("k", 0, 5464321);
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
	CHECK_STR("name", 0, FILE_PATH "extern.bin");
	CHECK_STR("acc", 0, "wb");
	CHECK_STR("acc2", 0, "rb");
	CHECK_STR("text", 0, "Hello again");
	CHECK_STR("textR1", 0, "Hello again");

	CHECK_CHAR("ch", 0, 45);
	CHECK_SHORT("sh", 0, 1069);
	CHECK_INT("num", 0, 12568);
	CHECK_LONG("lnum", 0, 4586564);

	CHECK_CHAR("chR1", 0, 45);
	CHECK_SHORT("shR1", 0, 1069);
	CHECK_INT("numR1", 0, 12568);
	CHECK_LONG("lnumR1", 0, 4586564);

	CHECK_CHAR("chR2", 0, 45);
	CHECK_SHORT("shR2", 0, 1069);
	CHECK_INT("numR2", 0, 12568);
	CHECK_LONG("lnumR2", 0, 4586564);
}
