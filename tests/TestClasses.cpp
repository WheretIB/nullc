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
	CHECK_FLOAT("f1", 0, 1, lastFailed);

	CHECK_FLOAT("f2", 0, 14, lastFailed);
	CHECK_FLOAT("f2", 1, 9, lastFailed);

	CHECK_FLOAT("f3", 0, 9, lastFailed);
	CHECK_FLOAT("f3", 1, 1, lastFailed);
	CHECK_FLOAT("f3", 2, 5, lastFailed);

	CHECK_FLOAT("f4", 0, 5, lastFailed);
	CHECK_FLOAT("f4", 1, 6, lastFailed);
	CHECK_FLOAT("f4", 2, 1, lastFailed);
	CHECK_FLOAT("f4", 3, 84, lastFailed);
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
		CHECK_FLOAT("fa", i, values[i], lastFailed);
}

const char	*testCmplx3 = 
"// Complex type test\r\n\
import std.math;\r\n\
float4x4 mat;\r\n\
mat.row1.y = 5;\r\n\
return 1;";
TEST("Complex type test", testCmplx3, "1")
{
	CHECK_FLOAT("mat", 1, 5.0f, lastFailed);
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
	CHECK_INT("one", 0, 3, lastFailed);
	CHECK_FLOAT("one", 3, 2.0f, lastFailed);
	CHECK_INT("two", 0, 14, lastFailed);
	CHECK_FLOAT("two", 10, 2.0f, lastFailed);
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
	CHECK_CHAR("c", 0, 66, lastFailed);
	CHECK_SHORT("u", 0, 15, lastFailed);
	CHECK_LONG("l", 0, 45645ll * 4594454795ll, lastFailed);
	
	CHECK_INT("two", 0, 14, lastFailed);
	CHECK_FLOAT("two", 10, 2, lastFailed);
	CHECK_INT("two", 13, 5, lastFailed);

	CHECK_INT("twonext", 0, 14, lastFailed);
	CHECK_FLOAT("twonext", 10, 2, lastFailed);
	CHECK_INT("twonext", 13, 5, lastFailed);
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
TEST_SIMPLE("Complex types test #4", testCmplx4, "56.000000")
{
	float values[] = { 6, 3, 5, 0, 2, 3, 7, 0, 12, 4.7f, 0, 11, 3, 5, 0, 2.5f, 1.2f, 5, 6, 5.9f, 5.9f, 5.9f, 5.9f, 12, 4.7f, 0, 2 };
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("n", i, values[i], lastFailed);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("m", i, values[i+4], lastFailed);
	for(int i = 0; i < 3; i++)
		CHECK_FLOAT("k", i, values[i+8], lastFailed);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("u", i, values[i+11], lastFailed);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("v", i, values[i+15], lastFailed);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("w", i, values[i+19], lastFailed);
	for(int i = 0; i < 4; i++)
		CHECK_FLOAT("q", i, values[i+23], lastFailed);

	float values2[] = { 0.0, 1.5, 3.0, 4.5, 6.0, 7.5, 9.0, 10.5, 12.0, 13.5 };
	for(int i = 0; i < 10; i++)
		CHECK_FLOAT("arr", i, values2[i], lastFailed);
	CHECK_FLOAT("arrSum", 0, 67.5f, lastFailed);
	for(int i = 0; i < 10; i++)
		CHECK_FLOAT("iArr", i, values2[i]+1.0f, lastFailed);
	CHECK_FLOAT("iArrSum", 0, 77.5f, lastFailed);
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
Foo l = Foo(5);\r\n\
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

const char	*testClassDefaultConstructorExplicitly =
"class Foo\r\n\
{\r\n\
	int y;\r\n\
}\r\n\
auto x = new Foo();\r\n\
return x.y;";
TEST_RESULT("explicit constructor call calls default constructor", testClassDefaultConstructorExplicitly, "0");

const char	*testClassConstructorImplicitCall =
"class Foo\r\n\
{\r\n\
	int y;\r\n\
	void Foo(){ y = 5; }\
}\r\n\
auto x = new Foo;\r\n\
return x.y;";
TEST_RESULT("implicit constructor call", testClassConstructorImplicitCall, "5");

const char	*testSizedArrayAllocation =
"typedef int[4] int4;\r\n\
auto x = new int4;\r\n\
return 1;";
TEST_RESULT("sized array allocation", testSizedArrayAllocation, "1");

const char	*testUnsizedArrayAllocation =
"typedef int[] int_;\r\n\
auto x = new int_;\r\n\
return 1;";
TEST_RESULT("unsized array allocation", testUnsizedArrayAllocation, "1");

const char	*testConstructorCallOnStaticConstruction1 =
"class Foo{ int x; void Foo(int z){ x = z; } void Foo(){ x = 42; } }\r\n\
Foo m = Foo();\r\n\
Foo n = Foo(5);\r\n\
return m.x - n.x;";
TEST_RESULT("member constructor call on external static construction 1", testConstructorCallOnStaticConstruction1, "37");

const char	*testConstructorCallOnStaticConstruction2 =
"class Foo{ int x; void Foo(int z){ x = z; } void Foo(){ x = 42; } }\r\n\
auto operator+(Foo ref a, b){ return Foo(a.x + b.x); }\r\n\
Foo m = Foo();\r\n\
Foo n = Foo(5);\r\n\
return (Foo() + Foo(5)).x;";
TEST_RESULT("member constructor call on external static construction 2", testConstructorCallOnStaticConstruction2, "47");

const char	*testConstructorForArrayElements1 =
"class Foo{ int x; void Foo(){ x = 42; } }\r\n\
auto x = new Foo[32];\r\n\
return x.size;";
TEST_RESULT("constructor call for every array element 1", testConstructorForArrayElements1, "32");

const char	*testConstructorForArrayElements2 =
"int m = 10;\r\n\
class Foo\r\n\
{\r\n\
	int x;\r\n\
	void Foo()\r\n\
	{\r\n\
		x = m++;\r\n\
	}\r\n\
}\r\n\
auto x = new Foo[32];\r\n\
return x[19].x;";
TEST_RESULT_SIMPLE("constructor call for every array element 2", testConstructorForArrayElements2, "29");

const char	*testConstructorForArrayElements2b =
"class Foo{ int x; void Foo(){ x = 42; } }\r\n\
auto x = new Foo[32];\r\n\
class Bar{ int x; void Bar(int y = 42){ x = y; } }\r\n\
auto y = new Bar[32];\r\n\
Foo[4][4] z;\r\n\
class Fez{ int x; Foo y; void Fez(int a){ x = a; } }\r\n\
auto w = new Fez[4];\r\n\
return x.size + x[1].x + y[1].x + z[1][2].x + w[1].y.x;";
TEST_RESULT_SIMPLE("constructor call for every array element 2b", testConstructorForArrayElements2b, "200");

const char	*testImplicitConstructorCallForGenericType1 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	void Foo(){ x = 42; }\r\n\
}\r\n\
auto x = new Foo<int>;\r\n\
return x.x;";
TEST_RESULT("implicit constructor call for generic types 1", testImplicitConstructorCallForGenericType1, "42");

const char	*testImplicitConstructorCallForGenericType2 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
}\r\n\
void Foo:Foo(){ x = 42; }\r\n\
auto x = new Foo<int>;\r\n\
return x.x;";
TEST_RESULT("implicit constructor call for generic types 2", testImplicitConstructorCallForGenericType2, "42");

const char	*testImplicitConstructorCallForGenericType3 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
}\r\n\
void Foo:Foo(){ x = 10; }\r\n\
void Foo<int>:Foo(){ x = 42; }\r\n\
auto x = new Foo<int>;\r\n\
return x.x;";
TEST_RESULT("implicit constructor call for generic types 3", testImplicitConstructorCallForGenericType3, "42");

const char	*testImplicitConstructorCallForGenericType4 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	void Foo(){ x = 42; }\r\n\
}\r\n\
auto x = new Foo<int>();\r\n\
return x.x;";
TEST_RESULT("implicit constructor call for generic types 4", testImplicitConstructorCallForGenericType4, "42");

const char	*testImplicitConstructorCallForGenericType5 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
}\r\n\
void Foo:Foo(){ x = 42; }\r\n\
auto x = new Foo<int>();\r\n\
return x.x;";
TEST_RESULT("implicit constructor call for generic types 5", testImplicitConstructorCallForGenericType5, "42");

const char	*testImplicitConstructorCallForGenericType6 =
"class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
}\r\n\
void Foo:Foo(){ x = 10; }\r\n\
void Foo<int>:Foo(){ x = 42; }\r\n\
auto x = new Foo<int>();\r\n\
return x.x;";
TEST_RESULT("implicit constructor call for generic types 6", testImplicitConstructorCallForGenericType6, "42");

const char	*testConstructorForArrayElements3 =
"int m = 10;\r\n\
class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
	void Foo(){ x = m++; }\r\n\
}\r\n\
auto x = new Foo<int>[32];\r\n\
return x[19].x;";
TEST_RESULT_SIMPLE("constructor call for every array element 3", testConstructorForArrayElements3, "29");

const char	*testConstructorForArrayElements4 =
"int m = 10;\r\n\
class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
}\r\n\
void Foo:Foo(){ x = m++; }\r\n\
auto x = new Foo<int>[32];\r\n\
return x[19].x;";
TEST_RESULT_SIMPLE("constructor call for every array element 4", testConstructorForArrayElements4, "29");

const char	*testConstructorForArrayElements5 =
"int m = 10;\r\n\
class Foo<T>\r\n\
{\r\n\
T x;\r\n\
}\r\n\
void Foo:Foo(){ x = 10; }\r\n\
void Foo<int>:Foo(){ x = m++; }\r\n\
auto x = new Foo<int>[32];\r\n\
return x[19].x;";
TEST_RESULT_SIMPLE("constructor call for every array element 5", testConstructorForArrayElements5, "29");

const char	*testConstructorCallOnStaticConstructionOfGenericType1 =
"class Foo<T>{ T curr; }\r\n\
auto Foo:Foo(int start)\r\n\
{\r\n\
	curr = start;\r\n\
}\r\n\
Foo<int> a = Foo<int>(5);\r\n\
return a.curr;";
TEST_RESULT("member constructor call on external static construction of a generic type", testConstructorCallOnStaticConstructionOfGenericType1, "5");

const char	*testCorrectTypeAliasesInAGenericTypeConstructor =
"class Bar<T>{ Bar<T> ref x; }\r\n\
class Foo<T>{ T curr; }\r\n\
auto Foo:Foo(Bar<T> ref start){ curr = 5; }\r\n\
auto a = Foo<int>(new Bar<int>);\r\n\
return a.curr;";
TEST_RESULT("correct type alias in a generic type constructor", testCorrectTypeAliasesInAGenericTypeConstructor, "5");

const char	*testDefaultStaticConstructor1 = "class Foo{ int x; } Foo a = Foo(); return 1;";
TEST_RESULT("default static constructor 1", testDefaultStaticConstructor1, "1");

const char	*testDefaultStaticConstructor2 = "class Foo{ int x; } auto bar(Foo m){ return sizeof(m); } return bar(Foo());";
TEST_RESULT("default static constructor 2", testDefaultStaticConstructor2, "4");

const char	*testDefaultStaticConstructor3 =
"align(4) class Foo<T>{ T x; int a; }\r\n\
auto foo(Foo<@T> m){ return sizeof(m); }\r\n\
return foo(Foo<Foo<double>>());";
TEST_RESULT("default static constructor 3", testDefaultStaticConstructor3, "16");

const char	*testCustomConstructor1 =
"class Foo{ int t; }\r\n\
\r\n\
auto x = new Foo(){ t = 5; };\r\n\
assert(x.t == 5);\r\n\
\r\n\
auto y = new Foo{ t = 10; };\r\n\
assert(y.t == 10);\r\n\
\r\n\
\r\n\
class Bar{ int x, y; void Bar(){ x = 4; } }\r\n\
\r\n\
auto z = new Bar(){ y = 6; };\r\n\
assert(z.x == 4);\r\n\
assert(z.y == 6);\r\n\
\r\n\
auto w = new Bar{ y = 9; };\r\n\
assert(w.x == 4);\r\n\
assert(w.y == 9);\r\n\
\r\n\
return 1;";
TEST_RESULT("Custom constructor 1", testCustomConstructor1, "1");

const char	*testCustomConstructor2 =
"{\r\n\
	class Ken{ int x, y; void Ken(int x){ this.x = x; } }\r\n\
\r\n\
	auto z = new Ken(2){ y = 6; };\r\n\
	assert(z.x == 2);\r\n\
	assert(z.y == 6);\r\n\
\r\n\
	auto w = new Ken{ y = 9; };\r\n\
	assert(w.x == 0);\r\n\
	assert(w.y == 9);\r\n\
}\r\n\
{\r\n\
	class Ben{ int x, y; void Ben(int x){ this.x = x; } void Ben(){ x = 4; } }\r\n\
\r\n\
	auto z = new Ben(2){ y = 6; };\r\n\
	assert(z.x == 2);\r\n\
	assert(z.y == 6);\r\n\
\r\n\
	auto w = new Ben{ y = 9; };\r\n\
	assert(w.x == 4);\r\n\
	assert(w.y == 9);\r\n\
}\r\n\
return 1;";
TEST_RESULT("Custom constructor 2", testCustomConstructor2, "1");

const char	*testConstructorForArrayElements6 =
"class Bar{ int x, y; void Bar(){ x = 4; } }\r\n\
\r\n\
Bar[16] z;\r\n\
assert(z[0].x == 4);\r\n\
assert(z[7].x == 4);\r\n\
assert(z[15].x == 4);\r\n\
\r\n\
Bar[16][4] w;\r\n\
assert(w[0][0].x == 4);\r\n\
assert(w[0][3].x == 4);\r\n\
assert(w[7][1].x == 4);\r\n\
assert(w[15][0].x == 4);\r\n\
assert(w[15][3].x == 4);\r\n\
\r\n\
Bar[] p;\r\n\
Bar[4][] p2;\r\n\
Bar[][4] p3;\r\n\
\r\n\
return 1;";
TEST_RESULT("Default constructor call for array elements of an unsized array", testConstructorForArrayElements6, "1");

const char	*testClassConstants1 =
"class Foo\r\n\
{\r\n\
	int a;\r\n\
	const int b = 6;\r\n\
	const int c = 7, d = 8;\r\n\
	const auto e = 9, f = 10;\r\n\
	const auto g = 11, h;\r\n\
}\r\n\
\r\n\
assert(Foo.b == 6);\r\n\
\r\n\
assert(Foo.c == 7);\r\n\
assert(Foo.d == 8);\r\n\
\r\n\
assert(Foo.e == 9);\r\n\
assert(Foo.f == 10);\r\n\
\r\n\
assert(Foo.g == 11);\r\n\
assert(Foo.h == 12);\r\n\
\r\n\
assert(sizeof(Foo) == 4);\r\n\
\r\n\
return 1;";
TEST_RESULT("Class constants", testClassConstants1, "1");

LOAD_MODULE(test_constant_export, "test.constant_export",
"class Foo\r\n\
{\r\n\
	int a;\r\n\
	const int b = 6;\r\n\
	const int c = 7, d = 8;\r\n\
	const auto e = 9, f = 10;\r\n\
	const auto g = 11, h;\r\n\
}");
const char	*testClassConstantsImport1 =
"import test.constant_export;\r\n\
\r\n\
assert(Foo.b == 6);\r\n\
\r\n\
assert(Foo.c == 7);\r\n\
assert(Foo.d == 8);\r\n\
\r\n\
assert(Foo.e == 9);\r\n\
assert(Foo.f == 10);\r\n\
\r\n\
assert(Foo.g == 11);\r\n\
assert(Foo.h == 12);\r\n\
\r\n\
assert(sizeof(Foo) == 4);\r\n\
\r\n\
return 1;";
TEST_RESULT("Class constants import 1", testClassConstantsImport1, "1");

LOAD_MODULE(test_constant_export2, "test.constant_export2",
"class Foo\r\n\
{\r\n\
int a;\r\n\
const double b = 6;\r\n\
int c;\r\n\
}");
const char	*testClassConstantsImport2 =
"import test.constant_export2;\r\n\
\r\n\
assert(Foo.a == int);\r\n\
assert(Foo.b == 6);\r\n\
assert(Foo.c == int);\r\n\
\r\n\
assert(sizeof(Foo) == 8);\r\n\
\r\n\
return 1;";
TEST_RESULT("Class constants import 2", testClassConstantsImport2, "1");

const char	*testClassConstants2 =
"class Foo\r\n\
{\r\n\
	const double v = 3;\r\n\
}\r\n\
auto x = Foo.v;\r\n\
return typeof(x) == double;";
TEST_RESULT("Class constants 2", testClassConstants2, "1");

const char	*testClassConstants3 =
"class Foo\r\n\
{\r\n\
	const int a = 5;\r\n\
}\r\n\
int Foo:foo(){ return a; }\r\n\
Foo m;\r\n\
return m.foo();";
TEST_RESULT("Class constants 3", testClassConstants3, "5");

const char	*testClassConstants4 =
"class Foo\r\n\
{\r\n\
	const int a = 5;\r\n\
	const int b = a + 5;\r\n\
}\r\n\
int Foo:foo(){ return a * b; }\r\n\
Foo m;\r\n\
return m.foo();";
TEST_RESULT("Class constants 4", testClassConstants4, "50");

const char	*testClassConstants5 =
"class Foo\r\n\
{\r\n\
	const int c = 2, d = c * 2, e;\r\n\
}\r\n\
assert(Foo.c == 2);\r\n\
assert(Foo.d == 4);\r\n\
assert(Foo.e == 5);\r\n\
return 1;";
TEST_RESULT("Class constants 5", testClassConstants5, "1");

const char	*testClassForwardDeclaration1 =
"class Foo;\r\n\
Foo ref a;\r\n\
class Foo\r\n\
{\r\n\
	int a, b;\r\n\
}\r\n\
a = new Foo;\r\n\
a.a = 5;\r\n\
return -a.a;";
TEST_RESULT("Class forward declaration 1", testClassForwardDeclaration1, "-5");

const char	*testClassForwardDeclaration2 =
"class Node;\r\n\
\r\n\
class List\r\n\
{\r\n\
	Node ref start;\r\n\
}\r\n\
class Node\r\n\
{\r\n\
	Node ref next;\r\n\
	List ref parent;\r\n\
}\r\n\
Node a;\r\n\
List b;\r\n\
\r\n\
return 1;";
TEST_RESULT("Class forward declaration 2", testClassForwardDeclaration2, "1");

const char	*testClassForwardDeclaration3 =
"class Node;\r\n\
class Node;\r\n\
class Node;\r\n\
class Node;\r\n\
class Node{ int a; }\r\n\
\r\n\
Node b;\r\n\
b.a = 10;\r\n\
return b.a;";
TEST_RESULT("Class forward declaration 3", testClassForwardDeclaration3, "10");

const char	*testClassForwardDeclaration4 =
"class Node;\r\n\
\r\n\
int Node:foo(){ return 12; }\r\n\
\r\n\
class Node\r\n\
{\r\n\
	int a;\r\n\
}\r\n\
\r\n\
Node f;\r\n\
return f.foo();";
TEST_RESULT("Class forward declaration 4", testClassForwardDeclaration4, "12");

const char	*testClassConstant6 =
"class DIR{ const auto LEFT = 0, RIGHT, UP, DOWN; }\r\n\
switch(1){ case DIR.UP: case DIR.DOWN: }\r\n\
int foo(int a){ return -a; } auto x = foo; return foo(DIR.UP);";
TEST_RESULT("Class constant 6 (test for a bug with constant wrap into a list)", testClassConstant6, "-2");

const char	*testClassAssignmentOperator1 =
"class Foo\r\n\
{\r\n\
	int ref a;\r\n\
}\r\n\
auto operator=(Foo ref a, Foo b){ a.a = new int(*b.a); return a; }\r\n\
\r\n\
Foo a;\r\n\
a.a = new int(4);\r\n\
Foo b = a;\r\n\
*b.a = 10;\r\n\
\r\n\
assert(a.a != b.a);\r\n\
assert(*a.a == 4);\r\n\
assert(*b.a == 10);\r\n\
\r\n\
class Bar1\r\n\
{\r\n\
	int x;\r\n\
	Foo y;\r\n\
}\r\n\
Bar1 m1, n1;\r\n\
m1.x = 7;\r\n\
m1.y.a = &m1.x;\r\n\
\r\n\
n1 = m1;\r\n\
*n1.y.a = 14;\r\n\
\r\n\
assert(m1.y.a != n1.y.a);\r\n\
assert(*m1.y.a == 7);\r\n\
assert(*n1.y.a == 14);\r\n\
\r\n\
class Bar2\r\n\
{\r\n\
	Bar1 a;\r\n\
}\r\n\
Bar2 m2, n2;\r\n\
m2.a.x = 7;\r\n\
m2.a.y.a = &m2.a.x;\r\n\
\r\n\
n2 = m2;\r\n\
*n2.a.y.a = 14;\r\n\
\r\n\
assert(m2.a.y.a != n2.a.y.a);\r\n\
assert(*m2.a.y.a == 7);\r\n\
assert(*n2.a.y.a == 14);\r\n\
\r\n\
return 1;";
TEST_RESULT("A default custom assignment operator is generated for classes that have members with a custom assignment operators 1", testClassAssignmentOperator1, "1");

const char	*testClassAssignmentOperator2 =
"namespace A\r\n\
{\r\n\
	class Foo\r\n\
	{\r\n\
		int ref a;\r\n\
	}\r\n\
	auto operator=(Foo ref a, Foo b){ a.a = new int(*b.a); return a; }\r\n\
\r\n\
	class Bar1\r\n\
	{\r\n\
		int x;\r\n\
		Foo y;\r\n\
	}\r\n\
}\r\n\
\r\n\
A.Foo a;\r\n\
a.a = new int(4);\r\n\
A.Foo b = a;\r\n\
*b.a = 10;\r\n\
\r\n\
assert(a.a != b.a);\r\n\
assert(*a.a == 4);\r\n\
assert(*b.a == 10);\r\n\
\r\n\
A.Bar1 m1, n1;\r\n\
m1.x = 7;\r\n\
m1.y.a = &m1.x;\r\n\
\r\n\
n1 = m1;\r\n\
*n1.y.a = 14;\r\n\
\r\n\
assert(m1.y.a != n1.y.a);\r\n\
assert(*m1.y.a == 7);\r\n\
assert(*n1.y.a == 14);\r\n\
\r\n\
return 1;";
TEST_RESULT("A default custom assignment operator is generated for classes that have members with a custom assignment operators 2", testClassAssignmentOperator2, "1");

const char	*testClassAssignmentOperator3 =
"class Custom\r\n\
{\r\n\
	int a = 4;\r\n\
}\r\n\
\r\n\
Custom ref operator=(Custom ref lhs, rhs)\r\n\
{\r\n\
	lhs.a = rhs.a * 2;\r\n\
\r\n\
	return lhs;\r\n\
}\r\n\
\r\n\
class A\r\n\
{\r\n\
	int x, y, z;\r\n\
}\r\n\
\r\n\
class B\r\n\
{\r\n\
	Custom x;\r\n\
\r\n\
	A a, b;\r\n\
}\r\n\
\r\n\
B b;\r\n\
\r\n\
b.a.y = 10;\r\n\
\r\n\
B b2 = b;\r\n\
\r\n\
return b2.a.y + b2.x.a;";
TEST_RESULT("Default custom assignment operator should not skip members without a custom/default assignment operator", testClassAssignmentOperator3, "18");

const char	*testEnumeration1 =
"enum Foo\r\n\
{\r\n\
	LEFT = 0, RIGHT, UP, DOWN\r\n\
}\r\n\
\r\n\
assert(typeof(Foo.LEFT) == Foo);\r\n\
int b = int(Foo.UP);\r\n\
assert(b == 2);\r\n\
auto c = Foo.UP;\r\n\
assert(typeof(c) == Foo);\r\n\
assert(c == Foo.UP);\r\n\
\r\n\
return 1;";
TEST_RESULT("Enumeration test 1", testEnumeration1, "1");

const char	*testEnumeration2 =
"enum Foo { y = 1 << 1, z = 1 << 2 } \r\n\
\r\n\
Foo d = Foo.y | Foo.z;\r\n\
int i = int(Foo.y);\r\n\
Foo e = Foo(34);\r\n\
\r\n\
assert(int(d) == 6);\r\n\
assert(i == 2);\r\n\
assert(int(e) == 34);\r\n\
\r\n\
return 1;";
TEST_RESULT("Enumeration test 2", testEnumeration2, "1");

const char	*testClassDefaultConstructor1 =
"class Foo{ int z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo y;\r\n\
	void aabb(int a){ x = a; }\r\n\
}\r\n\
aabb m;\r\n\
return m.y.z;";
TEST_RESULT("A default custom constructor is generated for classes that have members with a custom default constructor", testClassDefaultConstructor1, "5");

const char	*testClassDefaultConstructor2 =
"class Foo{ int z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo y;\r\n\
	void aabb(int a){ x = a; }\r\n\
}\r\n\
auto m = new aabb();\r\n\
return m.y.z;";
TEST_RESULT("A default custom constructor test 2", testClassDefaultConstructor2, "5");

const char	*testClassDefaultConstructor3 =
"class Foo{ int z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo y;\r\n\
	void aabb(int a){ x = a; }\r\n\
}\r\n\
auto m = new aabb;\r\n\
return m.y.z;";
TEST_RESULT("A default custom constructor test 3", testClassDefaultConstructor3, "5");

const char	*testClassDefaultConstructor4 =
"class Foo{ int z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo y;\r\n\
	void aabb(int a){ x = a; }\r\n\
}\r\n\
aabb[4] m;\r\n\
\r\n\
return m[3].y.z;";
TEST_RESULT("A default custom constructor test 4", testClassDefaultConstructor4, "5");

const char	*testClassDefaultConstructor5 =
"class Foo{ int z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo y;\r\n\
	void aabb(int a){ x = a; }\r\n\
}\r\n\
auto m = new aabb[8];\r\n\
return m[4].y.z;";
TEST_RESULT("A default custom constructor test 5", testClassDefaultConstructor5, "5");

const char	*testClassDefaultConstructor6 =
"class Foo{ int z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo y;\r\n\
	void aabb(int ref ref y){ x = 3; }\r\n\
}\r\n\
aabb[4] m;\r\n\
return m[3].y.z;";
TEST_RESULT("A default custom constructor test 6", testClassDefaultConstructor6, "5");

const char	*testClassDefaultConstructor7 =
"class Foo{ int z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo y;\r\n\
	void aabb(){ x = 3; }\r\n\
}\r\n\
aabb[4] m;\r\n\
return m[3].y.z * m[2].x;";
TEST_RESULT("A default custom constructor test 7", testClassDefaultConstructor7, "15");

const char	*testClassDefaultConstructor8 =
"class Foo<T>{ T z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo<int> y;\r\n\
}\r\n\
aabb[4] m;\r\n\
return m[3].y.z;";
TEST_RESULT("A default custom constructor test 8", testClassDefaultConstructor8, "5");

const char	*testClassDefaultConstructor9 =
"class Foo<T>{ T z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo<int> y;\r\n\
	void aabb(){ x = 4; }\r\n\
}\r\n\
aabb[4] m;\r\n\
return m[3].y.z;";
TEST_RESULT("A default custom constructor test 9", testClassDefaultConstructor9, "5");

const char	*testClassDefaultConstructor10 =
"class Foo{ int z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo[4] y;\r\n\
}\r\n\
aabb m;\r\n\
return m.y[2].z;";
TEST_RESULT("A default custom constructor test 10", testClassDefaultConstructor10, "5");

const char	*testClassDefaultConstructor11 =
"class Foo<T>{ T z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo<int>[5] y;\r\n\
}\r\n\
aabb[4] m;\r\n\
return m[3].y[2].z;";
TEST_RESULT("A default custom constructor test 11", testClassDefaultConstructor11, "5");

const char	*testClassDefaultConstructor12 =
"class Foo<T>{ T z; void Foo(){ z = 5; } }\r\n\
class aabb\r\n\
{\r\n\
	int x;\r\n\
	Foo<int>[5] y;\r\n\
	void aabb(){ x = 4; }\r\n\
}\r\n\
aabb[4] m;\r\n\
return m[3].y[2].z * m[2].x;";
TEST_RESULT("A default custom constructor test 12", testClassDefaultConstructor12, "20");

const char	*testClassAsBoolAndInSwitch =
"class Foo\r\n\
{\r\n\
	int x, y;\r\n\
	void Foo(int x, y){ this.x = x; this.y = y; }\r\n\
}\r\n\
Foo a = Foo(2, 4), b = Foo(3, -3);\r\n\
bool bool(Foo a){ return a.x + a.y; }\r\n\
\r\n\
{\r\n\
	int x = 0;\r\n\
	if(a)\r\n\
		x = 1;\r\n\
	assert(x == 1);\r\n\
}\r\n\
{\r\n\
	int x = 0;\r\n\
	if(b)\r\n\
		x = 1;\r\n\
	assert(x == 0);\r\n\
}\r\n\
\r\n\
{\r\n\
	int x = 0;\r\n\
	if(a)\r\n\
		x = 1;\r\n\
	else\r\n\
		x = 2;\r\n\
	assert(x == 1);\r\n\
}\r\n\
{\r\n\
	int x = 0;\r\n\
	if(b)\r\n\
		x = 1;\r\n\
	else\r\n\
		x = 2;\r\n\
	assert(x == 2);\r\n\
}\r\n\
{\r\n\
	int x = a ? 1 : 2;\r\n\
	assert(x == 1);\r\n\
}\r\n\
{\r\n\
	int x = b ? 1 : 2;\r\n\
	assert(x == 2);\r\n\
}\r\n\
{\r\n\
	int x = 0;\r\n\
	Foo c = Foo(4, 3);\r\n\
	for(; c; c.y--)\r\n\
		x++;\r\n\
	assert(x == 7);\r\n\
}\r\n\
{\r\n\
	int x = 0;\r\n\
	Foo c = Foo(4, 3);\r\n\
	while(c)\r\n\
	{\r\n\
		c.y--;\r\n\
		x++;\r\n\
	}\r\n\
	assert(x == 7);\r\n\
}\r\n\
{\r\n\
	int x = 0;\r\n\
	Foo c = Foo(4, 3);\r\n\
	do\r\n\
	{\r\n\
		c.y--;\r\n\
		x++;\r\n\
	}while(c);\r\n\
	assert(x == 7);\r\n\
}\r\n\
bool operator==(Foo a, b){ return a.x == b.x && a.y == b.y; }\r\n\
{\r\n\
	int x = 0;\r\n\
	Foo c = Foo(4, 3);\r\n\
	switch(c)\r\n\
	{\r\n\
	case Foo(0, 4):\r\n\
		x = 1;\r\n\
		break;\r\n\
	case Foo(4, 3):\r\n\
		x = 3;\r\n\
		break;\r\n\
	default:\r\n\
		x = 4;\r\n\
	}\r\n\
	assert(x == 3);\r\n\
}\r\n\
return 2;";
TEST_RESULT("Usage of class value inside if, for, while, do while and switch statements", testClassAsBoolAndInSwitch, "2");

const char	*testClassConstructorInMemberFunction =
"class Foo{ int x; void Foo(int y){ x = y; } }\r\n\
int Foo:xx()\r\n\
{\r\n\
	Foo a = Foo(5);\r\n\
	return 12;\r\n\
}\r\n\
Foo a;\r\n\
return a.xx();";
TEST_RESULT("Class constructor call inside a member function", testClassConstructorInMemberFunction, "12");

const char	*testClassCustomConstructorInsideAMemberFunction1 =
"class Foo\r\n\
{\r\n\
	int x;\r\n\
}\r\n\
class Bar\r\n\
{\r\n\
	Foo ref x;\r\n\
	void Bar(){ x = new Foo{ x = 5; }; }\r\n\
}\r\n\
return (new Bar()).x.x;";
TEST_RESULT("Class custom construction inside a member function", testClassCustomConstructorInsideAMemberFunction1, "5");

const char	*testClassCustomConstructorInsideAMemberFunction2 =
"class Foo\r\n\
{\r\n\
	int x;\r\n\
}\r\n\
class Bar\r\n\
{\r\n\
	Foo ref x;\r\n\
}\r\n\
void Bar:Bar(){ x = new Foo{ x = 5; }; }\r\n\
return (new Bar()).x.x;";
TEST_RESULT("Class custom construction inside a member function", testClassCustomConstructorInsideAMemberFunction2, "5");

const char	*testEnumeration3 =
"namespace Foo{ enum Bar{ C, D } }\r\n\
Foo.Bar y = Foo.Bar(1);\r\n\
return y;";
TEST_RESULT("Enumeration test 3", testEnumeration3, "1");

const char	*testEnumeration4 =
"namespace Foo{ enum Bar{ C, D } }\r\n\
Foo.Bar y = Foo.Bar(1);\r\n\
return int(y);";
TEST_RESULT("Enumeration test 4", testEnumeration4, "1");

const char	*testEnumeration5 =
"enum x{ C, D }\r\n\
x y = x(1);\r\n\
return int(y);";
TEST_RESULT("Enumeration test 5", testEnumeration5, "1");

const char	*testEnumeration6 =
"enum X{ a = 1, b = 2, c = a, d }\r\n\
return X.c + X.d;";
TEST_RESULT("Enumeration test 6", testEnumeration6, "3");

const char	*testMemberFunctionCallFromLocalFunctionInsideMemberFunction =
"class Foo{}\r\n\
auto Foo:foo(){ return 1; }\r\n\
auto Foo:bar(){ return coroutine auto(){ return foo(); }; }\r\n\
Foo x; return x.bar()();";
TEST_RESULT("Member function call from local function inside a member function", testMemberFunctionCallFromLocalFunctionInsideMemberFunction, "1");

const char	*testMemberAccessFromLocalFunctionInsideMemberFunction =
"class X\r\n\
{\r\n\
	int a, b, c;\r\n\
	auto Members()\r\n\
	{\r\n\
		return coroutine auto()\r\n\
		{\r\n\
			yield a;\r\n\
			yield b;\r\n\
			yield c;\r\n\
			return 0;\r\n\
		};\r\n\
	}\r\n\
}\r\n\
X x;\r\n\
x.a = 4;\r\n\
x.b = 30;\r\n\
x.c = 200;\r\n\
int s;\r\n\
for(i in x.Members())\r\n\
	s += i;\r\n\
return s;";
TEST_RESULT("Member access from local function inside a member function", testMemberAccessFromLocalFunctionInsideMemberFunction, "234");

const char	*testDefaultConstructCallFromExplicitConstructor =
"class Foo\r\n\
{\r\n\
	int a;\r\n\
	void Foo()\r\n\
	{\r\n\
		a = 4;\r\n\
	}\r\n\
}\r\n\
\r\n\
class Bar\r\n\
{\r\n\
	Foo x;\r\n\
}\r\n\
class X\r\n\
{\r\n\
	Bar y;\r\n\
	void X()\r\n\
	{\r\n\
	}\r\n\
}\r\n\
X m = X();\r\n\
return m.y.x.a;";
TEST_RESULT("Default constructor call from explicit constructor", testDefaultConstructCallFromExplicitConstructor, "4");

const char	*testMemberFunctionsWorkWithCompleteClass =
"class Test\r\n\
{\r\n\
	int x, y;\r\n\
	\r\n\
	int sum(Test t, Test u)\r\n\
	{\r\n\
		return t.x + t.y + u.x + u.y;\r\n\
	}\r\n\
	\r\n\
	int z, w;\r\n\
}\r\n\
\r\n\
Test a;\r\n\
a.x = 1; a.y = 2; a.z = 3; a.w = 4;\r\n\
\r\n\
Test b;\r\n\
b.x = 10; b.y = 20; b.z = 30; b.w = 40;\r\n\
\r\n\
return a.sum(a, b);";
TEST_RESULT("Member functions are analyzed when the class is complete", testMemberFunctionsWorkWithCompleteClass, "33");

const char	*testClassMemberInitializers1 =
"class Test\r\n\
{\r\n\
	int a = 1;\r\n\
	int b = 2;\r\n\
	int c = 3;\r\n\
}\r\n\
\r\n\
Test t;\r\n\
\r\n\
return t.a + t.b + t.c;";
TEST_RESULT("Class member initializers 1", testClassMemberInitializers1, "6");

const char	*testClassMemberInitializers2 =
"class Test\r\n\
{\r\n\
	int a = 1;\r\n\
	int b = 2;\r\n\
	int c = 3;\r\n\
\r\n\
	void Test()\r\n\
	{\r\n\
		b = 10;\r\n\
	}\r\n\
}\r\n\
\r\n\
Test t;\r\n\
\r\n\
return t.a + t.b + t.c;";
TEST_RESULT("Class member initializers 2", testClassMemberInitializers2, "14");

const char	*testClassMemberInitializers3 =
"class Test\r\n\
{\r\n\
	int[16] a = 1;\r\n\
	int[8] b = 2;\r\n\
	int[4] c = 3;\r\n\
}\r\n\
\r\n\
Test t;\r\n\
\r\n\
return t.a[10] + t.b[3] + t.c[1];";
TEST_RESULT("Class member initializers 3", testClassMemberInitializers3, "6");

const char	*testClassMemberInitializers4 =
"class Test\r\n\
{\r\n\
	int b = 10;\r\n\
	int c = b * 2;\r\n\
}\r\n\
\r\n\
Test t;\r\n\
\r\n\
return t.b + t.c;";
TEST_RESULT("Class member initializers 4", testClassMemberInitializers4, "30");

const char	*testClassMemberInitializers5 =
"class Test\r\n\
{\r\n\
	int ref(int) a = auto(int x){ return b + x * 2; };\r\n\
	int b = 10;\r\n\
}\r\n\
\r\n\
Test t;\r\n\
\r\n\
return t.a(3);";
TEST_RESULT("Class member initializers 5", testClassMemberInitializers5, "16");

const char	*testClassMemberInitializers6 =
"class Test\r\n\
{\r\n\
	int[] a = { 1, 2, 3, 4 };\r\n\
	int[] b = { for(int i = 1; i <= 10; i++) yield i; };\r\n\
}\r\n\
\r\n\
Test t;\r\n\
\r\n\
return t.a[2] + t.b[8];";
TEST_RESULT("Class member initializers 6", testClassMemberInitializers6, "12");

const char	*testClassStaticIf1 =
"class Test\r\n\
{\r\n\
	@if(1)\r\n\
	{\r\n\
		int x = 1, y = 2;\r\n\
	}\r\n\
\r\n\
	int s()\r\n\
	{\r\n\
		return x + y;\r\n\
	}\r\n\
}\r\n\
\r\n\
Test t;\r\n\
return t.s();";
TEST_RESULT("Class static if 1", testClassStaticIf1, "3");

const char	*testClassStaticIf2 =
"class Test\r\n\
{\r\n\
	int a = 1, b = 2;\r\n\
\r\n\
	@if(1){ int c, d; }\r\n\
}\r\n\
\r\n\
Test a;\r\n\
\r\n\
return sizeof(Test) + a.c;";
TEST_RESULT("Class static if 2", testClassStaticIf2, "16");

const char	*testClassConstantVisibility =
"class Test\r\n\
{\r\n\
	const int a = 1, b = 2;\r\n\
\r\n\
	int s()\r\n\
	{\r\n\
		return a + b;\r\n\
	}\r\n\
}\r\n\
\r\n\
Test t;\r\n\
return t.s();";
TEST_RESULT("Class constant visibility", testClassConstantVisibility, "3");
