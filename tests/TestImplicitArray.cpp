#include "TestBase.h"

const char	*testArrPtr = 
"// Implicit pointers to arrays\r\n\
int sum(int[] arr){ int res=0; for(int i=0;i<arr.size;i++) res+=arr[i]; return res; }\r\n\
void print(char[] str){ 1; } \r\n\
\r\n\
int[7] u=0;\r\n\
int[] k = u;\r\n\
int i = 3;\r\n\
\r\n\
u[1] = 5;\r\n\
k[2] = 2;\r\n\
k[i] = 5;\r\n\
\r\n\
int[7][3] uu;\r\n\
uu[2][1] = 100;\r\n\
int[][3] kk = uu;\r\n\
\r\n\
k[4] = kk[2][1];\r\n\
\r\n\
auto name = \"omfg\";\r\n\
print(name);\r\n\
print(\"does work\");\r\n\
\r\n\
return sum(u);";
TEST("Implicit pointers to arrays", testArrPtr, "112")
{
	CHECK_INT("u", 1, 5);
	CHECK_INT("u", 2, 2);
	CHECK_INT("u", 3, 5);
	CHECK_INT("u", 4, 100);

	CHECK_INT("uu", 7, 100);

	CHECK_STR("name", 0, "omfg");
	CHECK_STR("$temp1", 0, "does work");
}

const char	*testArrays = 
"import std.math;\r\n\
int test(int a, int b, int c){ return a*b+c; }\r\n\
float test2(float4 b){ return b.x-b.y; }\r\n\
int test3(char[] ch){ return ch.size; }\r\n\
\r\n\
int kl = test(2, 3, 4);\r\n\
float4 kl2; kl2.x = 5.0; kl2.y = 3.0;\r\n\
float res2 = test2(kl2);\r\n\
auto kl4 = \"kjskadjaskd\";\r\n\
int kl5 = test3(kl4);\r\n\
return 1;";
TEST("Arrays test", testArrays, "1")
{
	CHECK_INT("kl", 0, 10);
	CHECK_FLOAT("kl2", 0, 5.0f);
	CHECK_FLOAT("kl2", 1, 3.0f);

	CHECK_FLOAT("res2", 0, 2.0f);

	CHECK_INT("kl5", 0, 12);
	CHECK_STR("kl4", 0, "kjskadjaskd");
}

const char	*testArrays2 = 
"int test(int a, int b, int c){ return a*b+c; }\r\n\
void test4(float b){ return; }\r\n\
float test6(){ return 12; }\r\n\
\r\n\
int sum(int[] arr)\r\n\
{\r\n\
  int res = 0;\r\n\
  for(int i = 0; i < arr.size; i++)\r\n\
    res += arr[i];\r\n\
  return res;\r\n\
}\r\n\
short sh(int a){ return a; }\r\n\
auto n = { 10, 12, 11, 156 };\r\n\
auto m = \"hello?\";\r\n\
auto l = { sh(1), sh(2), sh(3) };\r\n\
\r\n\
int ns = sum(n), ts = sum({10, 12, 14, 16});\r\n\
return 1;";
TEST("Array test 2", testArrays2, "1")
{
	CHECK_INT("n", 0, 10);
	CHECK_INT("n", 1, 12);
	CHECK_INT("n", 2, 11);
	CHECK_INT("n", 3, 156);

	CHECK_STR("m", 0, "hello?");

	CHECK_INT("l", 0, 1);
	CHECK_INT("l", 1, 2);
	CHECK_INT("l", 2, 3);

	CHECK_INT("ns", 0, 189);
	CHECK_INT("ts", 0, 52);

	CHECK_INT("$temp1", 0, 10);
	CHECK_INT("$temp1", 1, 12);
	CHECK_INT("$temp1", 2, 14);
	CHECK_INT("$temp1", 3, 16);
}

const char	*testStrings = 
"int test(char[] text){ return 2; }\r\n\
int[10] arr;\r\n\
auto hm = \"World\\r\\n\";\r\n\
char[] nm = hm;\r\n\
char[] um = nm;\r\n\
arr[test(\"hello\\r\\n\")] += 3;\r\n\
return 1;";
TEST("Strings test", testStrings, "1")
{
	CHECK_STR("hm", 0, "World\r\n");
	CHECK_STR("$temp1", 0, "hello\r\n");
}

const char	*testInplaceArraysWithArrayElements =
"auto arr = { \"one\", \"two\", \"three\" };\r\n\
auto arr2 = {\r\n\
	{ 57 },\r\n\
	{ 12, 34 },\r\n\
	{ 34, 48, 56 }\r\n\
};\r\n\
return arr2[1][1] + arr2[0][0] + arr2[2][2] + arr[1][1];";
TEST_RESULT("Inplace arrays with array elements of different size", testInplaceArraysWithArrayElements, "266");

