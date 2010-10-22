#include "TestBase.h"

const char	*testMislead = 
"// Compiler mislead test\r\n\
import std.math;\r\n\
float2 a;\r\n\
a/*[gg]*/.x = 2;\r\n\
a.y = a/*[gg]*/.x + 3;\r\n\
// Result:\r\n\
// a.x = 2; a.y = 5\r\n\
return a.x;";
TEST("Compiler mislead test", testMislead, "2.000000")
{
	CHECK_FLOAT("a", 0, 2.0f);
	CHECK_FLOAT("a", 1, 5.0f);
}

const char	*testLogicalAnd =
"int i = 0, m = 4;\r\n\
i && (m = 3);\r\n\
return m;";
TEST_RESULT("Logical && special case", testLogicalAnd, "4");

const char	*testLogicalOr =
"int i = 1, m = 4;\r\n\
i || (m = 3);\r\n\
return m;";
TEST_RESULT("Logical || special case", testLogicalOr, "4");

const char	*testLongOrInt =
"long a = 0;int b = 0;\r\n\
return a || b;";
TEST_RESULT("Long or int", testLongOrInt, "0");

const char	*testAllInOne = 
"// Old all-in-one test\r\n\
double test(float x, float y){ return x**2*y; }\r\n\
int a=5;\r\n\
float b=1;\r\n\
float[3] c=14**2-134;\r\n\
double[10] d;\r\n\
for(int i = 0; i< 10; i++)\r\n\
d[i] = test(i*2, i-2);\r\n\
double n=1;\r\n\
while(1){ n*=2; if(n>1000) break; }\r\n\
return 2+test(2, 3)+a**b;";
TEST("Old all-in-one test", testAllInOne, "19.000000")
{
	CHECK_INT("a", 0, 5);
	CHECK_FLOAT("b", 0, 1.0f);
	CHECK_FLOAT("c", 0, 62.0f);
	CHECK_FLOAT("c", 1, 62.0f);
	CHECK_FLOAT("c", 2, 62.0f);
	double values[] = { -0.0, -4.0, 0.0, 36.0, 128.0, 300.0, 576.0, 980.0, 1536.0, 2268.0 };
	for(int i = 0; i < 10; i++)
		CHECK_DOUBLE("d", i, values[i]);
	CHECK_DOUBLE("n", 0, 1024);
}

const char	*testLongSpeed = 
"//longPow speed test\r\n\
long a = 43l, b = 10l; \r\n\
long c;\r\n\
for(int i = 0; i < 1000; i++)\r\n\
  c = a**b;\r\n\
return 0;";
TEST("longPow speed test", testLongSpeed, "0")
{
	CHECK_LONG("a", 0, 43);
	CHECK_LONG("b", 0, 10);
	CHECK_LONG("c", 0, 21611482313284249ll);
}

const char	*testOptiA = 
"int a = 12;\r\n\
int[6] res;\r\n\
res[0] = a + 0;\r\n\
res[1] = a * 0;\r\n\
res[2] = a * 1;\r\n\
res[3] = (a*1) +(a*0);\r\n\
res[4] = a*2+0;\r\n\
res[5] = a*3*1;\r\n\
return 1;";
TEST("Simple optimizations", testOptiA, "1")
{
	CHECK_INT("a", 0, 12);
	CHECK_INT("res", 0, 12);
	CHECK_INT("res", 1, 0);
	CHECK_INT("res", 2, 12);
	CHECK_INT("res", 3, 12);
	CHECK_INT("res", 4, 24);
	CHECK_INT("res", 5, 36);
}

const char	*testVarMod = 
"// Variable modify test\r\n\
int slow(int how){ for(int i = 0; i < how; i++){ how = how-1; } return 2; }\r\n\
int index = 2;\r\n\
int[10] arr = 4;\r\n\
arr[slow(/*40000000*/1000)] += 16; // 330 ms total. target - 140ms\r\n\
return 3;";
TEST("Variable modify test", testVarMod, "3")
{
	CHECK_INT("index", 0, 2);
	for(int i = 0; i < 10; i++)
	{
		if(i != 2)
		{
			CHECK_INT("arr", i, 4);
		}else{
			CHECK_INT("arr", i, 20);
		}
	}
}

const char	*testSpeed = 
"// Speed tests\r\n\
import std.math;\r\n\
float4x4 mat;\r\n\
class Float{ float x; }\r\n\
Float f;\r\n\
float2 f2;\r\n\
float3 f3;\r\n\
float4 f4;\r\n\
float4x4 test(float4x4 p){ p.row1.y *= 2.0f; return p; }\r\n\
Float test(Float p){ p.x *= 2.0f; return p; }\r\n\
float2 test(float2 p){ p.y *= 2.0f; return p; }\r\n\
float3 test(float3 p){ p.y *= 2.0f; return p; }\r\n\
float4 test(float4 p){ p.y *= 2.0f; return p; }\r\n\
for(int i = 0; i < 100; i++)\r\n\
{\r\n\
  mat = test(mat);\r\n\
  f = test(f);\r\n\
  f2 = test(f2);\r\n\
  f3 = test(f3);\r\n\
  f4 = test(f4);\r\n\
}\r\n\
return int(mat.row1.y);";
TEST_RESULT("Speed tests", testSpeed, "0");

const char	*testFunctionPrototypes =
"int func1();\r\n\
int func2(){ return func1(); }\r\n\
int func1(){ return 12; }\r\n\
return func2();";
TEST_RESULT("Function prototypes", testFunctionPrototypes, "12");

const char	*testSingleArrayIndexCalculation =
"int a = 0, b = 0;\r\n\
int func(int ref v, int index){ *v += 5; return index; }\r\n\
int[2] arr;\r\n\
arr[func(&a, 0)] += 4;\r\n\
arr[func(&b, 1)]++;\r\n\
return 0;";
TEST("Single array index calculation", testSingleArrayIndexCalculation, "0")
{
	CHECK_INT("a", 0, 5);
	CHECK_INT("b", 0, 5);

	CHECK_INT("arr", 0, 4);
	CHECK_INT("arr", 1, 1);
}

const char *testBytecodeNoGlobal = "int func(){ return 0; }";
TEST_RESULT("Bytecode with no global code", testBytecodeNoGlobal, "no return value");

const char	*testRange =
"import std.range;\r\n\
int sum = 0;\r\n\
for(i in range(1, 5))\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("std.range test", testRange, "15");

const char *testRawString = "auto x = @\"\\r\\n\\thello\\0\"; return x.size + (x[1] == 'r');";
TEST_RESULT("Unescaped string literal", testRawString, "15");

const char *testCharEscapeEnd = "return '\\\\';";
TEST_RESULT("Escaping at the end of the character", testCharEscapeEnd, "92");

const char *testStringEscapeEnd = "return \"\\\\\"[0];";
TEST_RESULT("Escaping at the end of the string", testStringEscapeEnd, "92");

const char *testCorrectCall = "int x = 8; int y = duplicate(x); return x + y;";
TEST_RESULT("Correct duplicate call", testCorrectCall, "16");

const char *testIfElseCompileTime1 = "if(1) return 2; else return 5;";
TEST_RESULT("if(){}else{} compile time resolve 1", testIfElseCompileTime1, "2");

const char *testIfElseCompileTime2 = "if(0) return 2; else return 5;";
TEST_RESULT("if(){}else{} compile time resolve 2", testIfElseCompileTime2, "5");

const char *testIfElseCompileTime3 = "return 1 ? 2 : 5;";
TEST_RESULT("if(){}else{} compile time resolve 3", testIfElseCompileTime3, "2");

const char *testIfElseCompileTime4 = "return 0 ? 2 : 5;";
TEST_RESULT("if(){}else{} compile time resolve 4", testIfElseCompileTime4, "5");

const char *testIfElseCompileTime5 = "if(1) return 2; return 5;";
TEST_RESULT("if(){}else{} compile time resolve 5", testIfElseCompileTime5, "2");

const char *testIfElseCompileTime6 = "if(0) return 2; return 5;";
TEST_RESULT("if(){}else{} compile time resolve 6", testIfElseCompileTime6, "5");
