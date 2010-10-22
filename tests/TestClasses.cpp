#include "TestBase.h"

const char	*testCmplxType1 = 
"// Complex type test (simple)\r\n\
import std.math;\r\n\
float f1;\r\n\
float2 f2;\r\n\
float3 f3;\r\n\
float4 f4;\r\n\
f1 = 1; // f1 = 1.0\r\n\
f2.x = 2.0; // f2.x = 2.0\r\n\
f2.y = 3l; // f2.y = 3.0\r\n\
f3.x = f2.y; // f3.x = 3.0\r\n\
f3.y = 4.0f; // f3.y = 4.0\r\n\
f3.z = f1*f3.x; // f3.z = 3.0\r\n\
f4.x = f3.y; // f4.x = 4.0\r\n\
f4.y = 6; // f4.y = 6.0\r\n\
f4.z = f3.z++; //f4.z = 3.0 f3.z = 4.0\r\n\
f4.w = 12; // f4.w = 12.0\r\n\
f3.x += f4.y++; // f3.x = 9.0 f4.y = 7.0\r\n\
f3.y -= f4.z--; // f3.y = 1.0 f4.z = 2.0\r\n\
f3.z++; // f3.z = 5.0\r\n\
++f4.x; // f4.x = 5.0\r\n\
f4.y--; // f4.y = 6.0\r\n\
--f4.z; // f4.z = 1.0\r\n\
f4.w *= f2.x += f3.z = 5; // f3.z = 5.0 f2.x = 7.0 f4.w = 84\r\n\
f2.x /= 0.5; // f2.x = 14.0\r\n\
f2.y **= 2.0; // f2.y = 9.0\r\n\
return 1;";

TEST("Complex type test (simple)", testCmplxType1, "1")
{
	CHECK_FLOAT("f1", 0, 1);

	CHECK_FLOAT("f2", 0, 14);
	CHECK_FLOAT("f2", 1, 9);

	CHECK_FLOAT("f3", 0, 9);
	CHECK_FLOAT("f3", 1, 1);
	CHECK_FLOAT("f3", 2, 5);

	CHECK_FLOAT("f4", 0, 5);
	CHECK_FLOAT("f4", 1, 6);
	CHECK_FLOAT("f4", 2, 1);
	CHECK_FLOAT("f4", 3, 84);
}

const char	*testCmplxType2 = 
"// Complex type test (array)\r\n\
import std.math;\r\n\
float3[10] fa;\r\n\
for(int i = 0; i < 10; i++)\r\n\
{\r\n\
	fa[i].x = i*8;\r\n\
	fa[i].y = fa[i].x++ - i*4;\r\n\
	fa[fa[(fa[i].x-1)*0.125].y*0.25].z = i+100;\r\n\
}\r\n\
return 1;";

TEST("Complex type test (complex)", testCmplxType2, "1")
{
	float values[] = { 1, 0, 100, 9, 4, 101, 17, 8, 102, 25, 12, 103, 33, 16, 104, 41, 20, 105, 49, 24, 106, 57, 28, 107, 65, 32, 108, 73, 36, 109 };
	for(int i = 0; i < 30; i++)
		CHECK_FLOAT("fa", i, values[i]);
}

const char	*testCmplx3 = 
"// Complex type test\r\n\
import std.math;\r\n\
float4x4 mat;\r\n\
mat.row1.y = 5;\r\n\
return 1;";
TEST("Complex type test", testCmplx3, "1")
{
	CHECK_FLOAT("mat", 1, 5.0f);
}

const char	*testClass1 = 
"// Class test\r\n\
import std.math;\r\n\
class One\r\n\
{\r\n\
  int a, b, c;\r\n\
  float e, f;\r\n\
}\r\n\
class Two\r\n\
{\r\n\
  One a, b;\r\n\
  float3 c;\r\n\
  int d;\r\n\
}\r\n\
One one;\r\n\
Two two;\r\n\
one.a = 3;\r\n\
one.e = 2;\r\n\
two.a.a = 14;\r\n\
two.c.x = 2;\r\n\
return 1;";
TEST("Class test", testClass1, "1")
{
	CHECK_INT("one", 0, 3);
	CHECK_FLOAT("one", 3, 2.0f);
	CHECK_INT("two", 0, 14);
	CHECK_FLOAT("two", 10, 2.0f);
}

const char	*testClass2 = 
"// Class test 2\r\n\
import std.math;\r\n\
class One\r\n\
{\r\n\
  int a, b, c;\r\n\
  float e, f;\r\n\
}\r\n\
class Two\r\n\
{\r\n\
  One a, b;\r\n\
  float3 c;\r\n\
  int d;\r\n\
}\r\n\
Two two, twonext;\r\n\
float3[2][4] fa;\r\n\
int[2][4] ia;\r\n\
double[8] da;\r\n\
char c = 66;\r\n\
short u = 15;\r\n\
long l = 45645l;\r\n\
l *= 4594454795l;\r\n\
float4x4 mat;\r\n\
\r\n\
two.a.a = 14;\r\n\
two.c.x = 2;\r\n\
two.d = 5;\r\n\
twonext = two;\r\n\
return 1;";
TEST("Class test 2", testClass2, "1")
{
	CHECK_CHAR("c", 0, 66);
	CHECK_SHORT("u", 0, 15);
	CHECK_LONG("l", 0, 45645ll * 4594454795ll);
	
	CHECK_INT("two", 0, 14);
	CHECK_FLOAT("two", 10, 2);
	CHECK_INT("two", 13, 5);

	CHECK_INT("twonext", 0, 14);
	CHECK_FLOAT("twonext", 10, 2);
	CHECK_INT("twonext", 13, 5);
}

const char	*testCmplx4 = 
"//Complex types test #3\r\n\
import std.math;\r\n\
float test(float4 a, float4 b){ return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }\r\n\
float4 test2(float4 u){ u.x += 5.0; return u; }\r\n\
float4 float4(float all){ float4 ret; ret.x = ret.y = ret.z = ret.w = all; return ret; }\r\n\
float sum(float[10] u){ float res = 0; for(int i = 0; i < 10; i++) res += u[i]; return res; }\r\n\
float[10] inc(float[10] v){ float[10] res; for(int i = 0; i < 10; i++) res[i] = v[i]+1.0f; return res; }\r\n\
float4 n, m;\r\n\
n.x = 6.0f;\r\n\
n.y = 3.0f;\r\n\
n.z = 5.0f;\r\n\
n.w = 0.0f;\r\n\
\r\n\
m.x = 2.0f;\r\n\
m.y = 3.0f;\r\n\
m.z = 7.0f;\r\n\
m.w = 0.0f;\r\n\
float3 k;\r\n\
k.x = 12.0;\r\n\
k.y = 4.7;\r\n\
k.z = 0;\r\n\
float4 u = test2(n), v = float4(2.5, 1.2, 5, 6.0), w = float4(5.9), q = float4(k, 2.0);\r\n\
float[10] arr;\r\n\
for(int i = 0; i < 10; i++)\r\n\
  arr[i] = i*1.5f;\r\n\
float arrSum = sum(arr);\r\n\
float[10] iArr = inc(arr);\r\n\
float iArrSum = sum(iArr);\r\n\
return test(n, m); // 56.0";
TEST("Complex types test #4", testCmplx4, "56.000000")
{
	float values[] = { 6, 3, 5, 0, 2, 3, 7, 0, 12, 4.7f, 0, 11, 3, 5, 0, 2.5f, 1.2f, 5, 6, 5.9f, 5.9f, 5.9f, 5.9f, 12, 4.7f, 0, 2 };
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("n", i, values[i]);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("m", i, values[i+4]);
	for(int i = 0; i < 3; i++)
		CHECK_FLOAT("k", i, values[i+8]);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("u", i, values[i+11]);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("v", i, values[i+15]);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("w", i, values[i+19]);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("q", i, values[i+23]);

	float values2[] = { 0.0, 1.5, 3.0, 4.5, 6.0, 7.5, 9.0, 10.5, 12.0, 13.5 };
	for(int i = 0; i < 10; i++)
		CHECK_FLOAT("arr", i, values2[i]);
	CHECK_FLOAT("arrSum", 0, 67.5f);
	for(int i = 0; i < 10; i++)
		CHECK_FLOAT("iArr", i, values2[i]+1.0f);
	CHECK_FLOAT("iArrSum", 0, 77.5f);
}

const char	*testSmallClass =
"class TestS{ short a; }\r\n\
TestS b;\r\n\
b.a = 6;\r\n\
class TestC{ char a; }\r\n\
TestC c;\r\n\
c.a = 3;\r\n\
int func(TestS a){ return a.a; }\r\n\
int func(TestC a){ return a.a; }\r\n\
return func(b) + func(c);";
TEST_RESULT("Class with size smaller that 4 bytes", testSmallClass, "9");

const char	*testExtraSmallClass =
"class Test{ }\r\n\
Test b;\r\n\
int func(Test a, int b){ return b; }\r\n\
return func(b, 4);";
TEST_RESULT("Class with size of 0 bytes", testExtraSmallClass, "4");

const char	*testClassAutoConstructor =
"class Foo\r\n\
{\r\n\
	int y;\r\n\
	void Foo(){ y = 9; }\r\n\
}\r\n\
Foo l;\r\n\
return l.y;";
TEST_RESULT("automatic constructor call for class instance on stack", testClassAutoConstructor, "9");

const char	*testClassConstructorStack =
"class Foo\r\n\
{\r\n\
	int y;\r\n\
	void Foo(int n){ y = n; }\r\n\
}\r\n\
Foo l!(5);\r\n\
return l.y;";
TEST_RESULT("manual constructor call for class instance on stack", testClassConstructorStack, "5");

LOAD_MODULE(test_class_typedef, "test.class_typedef",
"class Foo\r\n\
{\r\n\
	typedef int T;\r\n\
}");
const char *testClassTypeAliasImport1 =
"import test.class_typedef;\r\n\
void Foo:foo(){ T y; }\r\n\
return 1;";
TEST_RESULT("Import of class local aliases", testClassTypeAliasImport1, "1");
