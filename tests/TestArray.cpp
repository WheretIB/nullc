#include "TestBase.h"

const char	*testCycle = 
"int a=5;\r\n\
double[10] d=0.0;\r\n\
for(int i = 0; i < a; i++)\r\n\
d[i] = i*2 + i-2;\r\n\
return d[5];";
TEST("Array test", testCycle, "0.000000")
{
	CHECK_INT("a", 0, 5, lastFailed);
	double values[] = { -2.0, 1.0, 4.0, 7.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	for(int i = 0; i < 10; i++)
		CHECK_DOUBLE("d", i, values[i], lastFailed);
}

const char	*testIndirection = 
"// Array indirection and optimization test\r\n\
int[5] res=1;\r\n\
res[1] = 13;\r\n\
res[2] = 3;\r\n\
res[res[2]] = 4;\r\n\
res[res[res[2]]] = 12;\r\n\
return 5;";
TEST("Array indirection and optimization test", testIndirection, "5")
{
	CHECK_INT("res", 0, 1, lastFailed);
	CHECK_INT("res", 1, 13, lastFailed);
	CHECK_INT("res", 2, 3, lastFailed);
	CHECK_INT("res", 3, 4, lastFailed);
	CHECK_INT("res", 4, 12, lastFailed);
}

const char	*testArrayFill = 
"// Array fill test\r\n\
int[10] a=5;\r\n\
return 1;";
TEST("Array fill test", testArrayFill, "1")
{
	for(int i = 0; i < 10; i++)
		CHECK_INT("a", i, 5, lastFailed);
}

const char	*testIncDec = 
"int[5] test=0;\r\n\
for(int i = 0; i < 5; i++)\r\n\
{\r\n\
  test[i] = 1;\r\n\
  test[i] += 5;\r\n\
  test[i] = test[i]++;\r\n\
  test[i] = ++test[i];\r\n\
}\r\n\
return 1;";
TEST("Inc dec test", testIncDec, "1")
{
	CHECK_INT("test", 0, 7, lastFailed);
	CHECK_INT("test", 1, 7, lastFailed);
	CHECK_INT("test", 2, 7, lastFailed);
	CHECK_INT("test", 3, 7, lastFailed);
	CHECK_INT("test", 4, 7, lastFailed);
}

const char	*testCharArr = 
"// Char array test\r\n\
auto str1 = \"\", str2 = \"a\", str3 = \"ab\";\r\n\
auto str4 = \"abc\", str5 = \"abcd\", string = \"Hello World!\";\r\n\
return 1;";
TEST("Char array test", testCharArr, "1")
{
	CHECK_STR("str1", 0, "", lastFailed);
	CHECK_STR("str2", 0, "a", lastFailed);
	CHECK_STR("str3", 0, "ab", lastFailed);
	CHECK_STR("str4", 0, "abc", lastFailed);
	CHECK_STR("str5", 0, "abcd", lastFailed);
	CHECK_STR("string", 0, "Hello World!", lastFailed);
}

const char	*testCharArr2 = 
"char[2][5] arr;\r\n\
arr[0] = \"hehe\";\r\n\
arr[1] = \"haha\";\r\n\
auto un = \"buggy\";\r\n\
align(8) class sss{ int a; char[5] uhu; int bb; }\r\n\
sss kl;\r\n\
char p;\r\n\
noalign double c, d;\r\n\
double f;\r\n\
kl.uhu = \"tyty\";\r\n\
return 0;";
TEST("Char array test 2", testCharArr2, "0")
{
	CHECK_STR("arr", 0, "hehe", lastFailed);
	CHECK_STR("arr", 8, "haha", lastFailed);

	CHECK_STR("un", 0, "buggy", lastFailed);

	CHECK_STR("kl", 4, "tyty", lastFailed);
}

const char	*testEscape = 
"//Escape sequences\r\n\
auto name = \"01n\";\r\n\
for(int i = 0; i < 10; i++)\r\n\
{\r\n\
  name[1] = i+'0';\r\n\
  name[2] = '\\n';\r\n\
}\r\n\
\r\n\
return 'n';";
TEST("Escape sequences", testEscape, "110")
{
	CHECK_STR("name", 0, "09\n", lastFailed);
}

const char	*testPrint = 
"auto ts = \"Hello World!\\r\\noh\\toh\\r\\n\";\r\n\
char[] mm = \"hello\";\r\n\
char[] mn = \"world\";\r\n\
auto mo = \"!!!\";\r\n\
return 1;";
TEST("Print test", testPrint, "1")
{
	CHECK_STR("ts", 0, "Hello World!\r\noh\toh\r\n", lastFailed);
	CHECK_ARRAY_STR("mm", 0, "hello", lastFailed);
	CHECK_ARRAY_STR("mn", 0, "world", lastFailed);
	CHECK_STR("mo", 0, "!!!", lastFailed);
}

const char	*testMultiCtor = 
"// Multidimensional array constructor test\r\n\
import std.math;\r\n\
float3 float3(int[3] comp)\r\n\
{\r\n\
float3 ret;\r\n\
ret.x = comp[0];\r\n\
ret.y = comp[1];\r\n\
ret.z = comp[2];\r\n\
return ret;\r\n\
}\r\n\
float3 float3(float[3] comp)\r\n\
{\r\n\
float3 ret;\r\n\
ret.x = comp[0];\r\n\
ret.y = comp[1];\r\n\
ret.z = comp[2];\r\n\
return ret;\r\n\
}\r\n\
float3 float3(double[3] comp)\r\n\
{\r\n\
float3 ret;\r\n\
ret.x = comp[0];\r\n\
ret.y = comp[1];\r\n\
ret.z = comp[2];\r\n\
return ret;\r\n\
}\r\n\
float3 g = float3({4.0, 6.0, 8.0});\r\n\
float3 h = float3({3.0f, 5.0f, 7.0f});\r\n\
float3 j = float3({11, 12, 13});\r\n\
\r\n\
auto a = { 3, 4, 5 };\r\n\
auto b = { {11, 12}, {14, 15} };\r\n\
\r\n\
auto c = { g, h, j };\r\n\
auto d = { { {g}, {h} }, { {h}, {j} } };\r\n\
\r\n\
auto f = { 1.0f, 2.0f };\r\n\
int[] k;\r\n\
k = { 1, 2, 3, 4 };\r\n\
return int(g.x + 0.5) == 4 && int(h.y + 0.5) == 5 && int(j.z + 0.5) == 13 && a[1] == 4 && b[1][1] == 15 && int(c[1].y + 0.5) == 5 && int(d[1][0][0].z + 0.5) == 7 && int(f[1] + 0.5) == 2;";
TEST("Multidimensional array constructor test", testMultiCtor, "1")
{
	CHECK_FLOAT("g", 0, 4.0f, lastFailed);
	CHECK_FLOAT("g", 1, 6.0f, lastFailed);
	CHECK_FLOAT("g", 2, 8.0f, lastFailed);

	CHECK_FLOAT("h", 0, 3.0f, lastFailed);
	CHECK_FLOAT("h", 1, 5.0f, lastFailed);
	CHECK_FLOAT("h", 2, 7.0f, lastFailed);

	CHECK_FLOAT("j", 0, 11.0f, lastFailed);
	CHECK_FLOAT("j", 1, 12.0f, lastFailed);
	CHECK_FLOAT("j", 2, 13.0f, lastFailed);

	CHECK_INT("a", 0, 3, lastFailed);
	CHECK_INT("a", 1, 4, lastFailed);
	CHECK_INT("a", 2, 5, lastFailed);

	CHECK_INT("b", 0, 11, lastFailed);
	CHECK_INT("b", 1, 12, lastFailed);
	CHECK_INT("b", 2, 14, lastFailed);
	CHECK_INT("b", 3, 15, lastFailed);

	CHECK_FLOAT("c", 0, 4.0f, lastFailed);
	CHECK_FLOAT("c", 1, 6.0f, lastFailed);
	CHECK_FLOAT("c", 2, 8.0f, lastFailed);
	CHECK_FLOAT("c", 3, 3.0f, lastFailed);
	CHECK_FLOAT("c", 4, 5.0f, lastFailed);
	CHECK_FLOAT("c", 5, 7.0f, lastFailed);
	CHECK_FLOAT("c", 6, 11.0f, lastFailed);
	CHECK_FLOAT("c", 7, 12.0f, lastFailed);
	CHECK_FLOAT("c", 8, 13.0f, lastFailed);

	CHECK_FLOAT("d", 0, 4.0f, lastFailed);
	CHECK_FLOAT("d", 1, 6.0f, lastFailed);
	CHECK_FLOAT("d", 2, 8.0f, lastFailed);
	CHECK_FLOAT("d", 3, 3.0f, lastFailed);
	CHECK_FLOAT("d", 4, 5.0f, lastFailed);
	CHECK_FLOAT("d", 5, 7.0f, lastFailed);
	CHECK_FLOAT("d", 6, 3.0f, lastFailed);
	CHECK_FLOAT("d", 7, 5.0f, lastFailed);
	CHECK_FLOAT("d", 8, 7.0f, lastFailed);
	CHECK_FLOAT("d", 9, 11.0f, lastFailed);
	CHECK_FLOAT("d", 10, 12.0f, lastFailed);
	CHECK_FLOAT("d", 11, 13.0f, lastFailed);

	CHECK_FLOAT("f", 0, 1.0, lastFailed);
	CHECK_FLOAT("f", 1, 2.0, lastFailed);
}

const char	*testTemporaryHeapArray1 = 
"char[] a;\r\n\
\r\n\
auto foo()\r\n\
{\r\n\
	a = \"hello there\";\r\n\
}\r\n\
auto bar(){ }\r\n\
foo();\r\n\
bar();\r\n\
\r\n\
return (a[0] == 'h');";
TEST_RESULT("Creation of temporary array on heap 1", testTemporaryHeapArray1, "1");

const char	*testTemporaryHeapArray2 = 
"char[] a;\r\n\
\r\n\
auto foo()\r\n\
{\r\n\
	char[] b = \"hello there\";\r\n\
	a = b;\r\n\
}\r\n\
auto bar(){ }\r\n\
foo();\r\n\
bar();\r\n\
\r\n\
return (a[0] == 'h');";
TEST_RESULT("Creation of temporary array on heap 2", testTemporaryHeapArray2, "1");

const char	*testTemporaryHeapArray3 = 
"char[] a;\r\n\
\r\n\
auto foo(char[] b)\r\n\
{\r\n\
	a = b;\r\n\
}\r\n\
auto bar(){ }\r\n\
auto t = \"hello there\";\r\n\
foo(t);\r\n\
bar();\r\n\
\r\n\
return (a[0] == 'h');";
TEST_RESULT("Creation of temporary array on heap 3", testTemporaryHeapArray3, "1");

const char	*testArrayInit = 
"char[4] x1 = 1;\r\n\
int y1;\r\n\
short[4] x2 = 2;\r\n\
int y2;\r\n\
int[4] x3 = 3;\r\n\
int y3;\r\n\
long[4] x4 = 4;\r\n\
int y4;\r\n\
float[4] x5 = 5;\r\n\
int y5;\r\n\
double[4] x6 = 6;\r\n\
int y6;\r\n\
\r\n\
return 91 == (x1[0] * x1[3] + y1 + x2[0] * x2[3] + y2 + x3[0] * x3[3] + y3 + x4[0] * x4[3] + y4 + x5[0] * x5[3] + y5 + x6[0] * x6[3] + y6);";
TEST_RESULT("Array initialization test", testArrayInit, "1");
