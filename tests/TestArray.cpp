#include "TestBase.h"

const char	*testCycle = 
"int a=5;\r\n\
double[10] d=0.0;\r\n\
for(int i = 0; i < a; i++)\r\n\
d[i] = i*2 + i-2;\r\n\
return d[5];";
TEST("Array test", testCycle, "0.000000")
{
	CHECK_INT("a", 0, 5);
	double values[] = { -2.0, 1.0, 4.0, 7.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	for(int i = 0; i < 10; i++)
		CHECK_DOUBLE("d", i, values[i]);
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
	CHECK_INT("res", 0, 1);
	CHECK_INT("res", 1, 13);
	CHECK_INT("res", 2, 3);
	CHECK_INT("res", 3, 4);
	CHECK_INT("res", 4, 12);
}

const char	*testArrayFill = 
"// Array fill test\r\n\
int[10] a=5;\r\n\
return 1;";
TEST("Array fill test", testArrayFill, "1")
{
	for(int i = 0; i < 10; i++)
		CHECK_INT("a", i, 5);
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
	CHECK_INT("test", 0, 7);
	CHECK_INT("test", 1, 7);
	CHECK_INT("test", 2, 7);
	CHECK_INT("test", 3, 7);
	CHECK_INT("test", 4, 7);
}

const char	*testCharArr = 
"// Char array test\r\n\
auto str1 = \"\", str2 = \"a\", str3 = \"ab\";\r\n\
auto str4 = \"abc\", str5 = \"abcd\", string = \"Hello World!\";\r\n\
return 1;";
TEST("Char array test", testCharArr, "1")
{
	CHECK_STR("str1", 0, "");
	CHECK_STR("str2", 0, "a");
	CHECK_STR("str3", 0, "ab");
	CHECK_STR("str4", 0, "abc");
	CHECK_STR("str5", 0, "abcd");
	CHECK_STR("string", 0, "Hello World!");
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
	CHECK_STR("arr", 0, "hehe");
	CHECK_STR("arr", 8, "haha");

	CHECK_STR("un", 0, "buggy");

	CHECK_STR("kl", 4, "tyty");
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
	CHECK_STR("name", 0, "09\n");
}

const char	*testPrint = 
"auto ts = \"Hello World!\\r\\noh\\toh\\r\\n\";\r\n\
char[] mm = \"hello\";\r\n\
char[] mn = \"world\";\r\n\
auto mo = \"!!!\";\r\n\
return 1;";
TEST("Print test", testPrint, "1")
{
	CHECK_STR("ts", 0, "Hello World!\r\noh\toh\r\n");
	CHECK_HEAP_STR("$temp1", 0, "hello");
	CHECK_HEAP_STR("$temp2", 0, "world");
	CHECK_STR("mo", 0, "!!!");
}

const char	*testMultiCtor = 
"// Multidimensional array constructor test\r\n\
import std.math;\r\n\
float3 float3(int[3] comp){ float3 ret; ret.x = comp[0]; ret.y = comp[1]; ret.z = comp[2]; return ret; }\r\n\
float3 float3(float[3] comp){ float3 ret; ret.x = comp[0]; ret.y = comp[1]; ret.z = comp[2]; return ret; }\r\n\
float3 float3(double[3] comp){ float3 ret; ret.x = comp[0]; ret.y = comp[1]; ret.z = comp[2]; return ret; }\r\n\
float3 g = float3({4.0, 6.0, 8.0}), h = float3({3.0f, 5.0f, 7.0f}), j = float3({11, 12, 13});\r\n\
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
return 1;";
TEST("Multidimensional array constructor test", testMultiCtor, "1")
{
	CHECK_FLOAT("g", 0, 4.0f);
	CHECK_FLOAT("g", 1, 6.0f);
	CHECK_FLOAT("g", 2, 8.0f);

	CHECK_FLOAT("h", 0, 3.0f);
	CHECK_FLOAT("h", 1, 5.0f);
	CHECK_FLOAT("h", 2, 7.0f);

	CHECK_FLOAT("j", 0, 11.0f);
	CHECK_FLOAT("j", 1, 12.0f);
	CHECK_FLOAT("j", 2, 13.0f);

	CHECK_INT("a", 0, 3);
	CHECK_INT("a", 1, 4);
	CHECK_INT("a", 2, 5);

	CHECK_INT("b", 0, 11);
	CHECK_INT("b", 1, 12);
	CHECK_INT("b", 2, 14);
	CHECK_INT("b", 3, 15);

	CHECK_FLOAT("c", 0, 4.0f);
	CHECK_FLOAT("c", 1, 6.0f);
	CHECK_FLOAT("c", 2, 8.0f);
	CHECK_FLOAT("c", 3, 3.0f);
	CHECK_FLOAT("c", 4, 5.0f);
	CHECK_FLOAT("c", 5, 7.0f);
	CHECK_FLOAT("c", 6, 11.0f);
	CHECK_FLOAT("c", 7, 12.0f);
	CHECK_FLOAT("c", 8, 13.0f);

	CHECK_FLOAT("d", 0, 4.0f);
	CHECK_FLOAT("d", 1, 6.0f);
	CHECK_FLOAT("d", 2, 8.0f);
	CHECK_FLOAT("d", 3, 3.0f);
	CHECK_FLOAT("d", 4, 5.0f);
	CHECK_FLOAT("d", 5, 7.0f);
	CHECK_FLOAT("d", 6, 3.0f);
	CHECK_FLOAT("d", 7, 5.0f);
	CHECK_FLOAT("d", 8, 7.0f);
	CHECK_FLOAT("d", 9, 11.0f);
	CHECK_FLOAT("d", 10, 12.0f);
	CHECK_FLOAT("d", 11, 13.0f);

	CHECK_DOUBLE("f", 0, 1.0);
	CHECK_DOUBLE("f", 1, 2.0);
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
