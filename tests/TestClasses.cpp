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
TEST_RESULT("constructor call for every array element 2", testConstructorForArrayElements2, "29");

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
TEST_RESULT("constructor call for every array element 3", testConstructorForArrayElements3, "29");

const char	*testConstructorForArrayElements4 =
"int m = 10;\r\n\
class Foo<T>\r\n\
{\r\n\
	T x;\r\n\
}\r\n\
void Foo:Foo(){ x = m++; }\r\n\
auto x = new Foo<int>[32];\r\n\
return x[19].x;";
TEST_RESULT("constructor call for every array element 4", testConstructorForArrayElements4, "29");

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
TEST_RESULT("constructor call for every array element 5", testConstructorForArrayElements5, "29");

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
"class Foo<T>{ T x; int a; }\r\n\
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

const char	*testClassAssignmentOperator =
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
class Bar\r\n\
{\r\n\
	int x;\r\n\
	Foo y;\r\n\
}\r\n\
Bar m, n;\r\n\
m.x = 7;\r\n\
m.y.a = &m.x;\r\n\
\r\n\
n = m;\r\n\
*n.y.a = 14;\r\n\
\r\n\
assert(m.y.a != n.y.a);\r\n\
assert(*m.y.a == 7);\r\n\
assert(*n.y.a == 14);\r\n\
\r\n\
return 1;";
TEST_RESULT("A default custom assignment operator is generated for classes that have members with a custom assignment operators", testClassAssignmentOperator, "1");

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
