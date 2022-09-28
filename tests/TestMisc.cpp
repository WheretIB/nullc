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
	CHECK_FLOAT("a", 0, 2.0f, lastFailed);
	CHECK_FLOAT("a", 1, 5.0f, lastFailed);
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
	CHECK_INT("a", 0, 5, lastFailed);
	CHECK_FLOAT("b", 0, 1.0f, lastFailed);
	CHECK_FLOAT("c", 0, 62.0f, lastFailed);
	CHECK_FLOAT("c", 1, 62.0f, lastFailed);
	CHECK_FLOAT("c", 2, 62.0f, lastFailed);
	double values[] = { -0.0, -4.0, 0.0, 36.0, 128.0, 300.0, 576.0, 980.0, 1536.0, 2268.0 };
	for(int i = 0; i < 10; i++)
		CHECK_DOUBLE("d", i, values[i], lastFailed);
	CHECK_DOUBLE("n", 0, 1024, lastFailed);
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
	CHECK_LONG("a", 0, 43, lastFailed);
	CHECK_LONG("b", 0, 10, lastFailed);
	CHECK_LONG("c", 0, 21611482313284249ll, lastFailed);
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
	CHECK_INT("a", 0, 12, lastFailed);
	CHECK_INT("res", 0, 12, lastFailed);
	CHECK_INT("res", 1, 0, lastFailed);
	CHECK_INT("res", 2, 12, lastFailed);
	CHECK_INT("res", 3, 12, lastFailed);
	CHECK_INT("res", 4, 24, lastFailed);
	CHECK_INT("res", 5, 36, lastFailed);
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
	CHECK_INT("index", 0, 2, lastFailed);
	for(int i = 0; i < 10; i++)
	{
		if(i != 2)
		{
			CHECK_INT("arr", i, 4, lastFailed);
		}else{
			CHECK_INT("arr", i, 20, lastFailed);
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

const char	*testFunctionPrototypes2 =
"void foo();\r\n\
auto foo(){}\r\n\
foo();\r\n\
return 1;";
TEST_RESULT("Function prototype is implemented by a function with auto return type", testFunctionPrototypes2, "1");

const char	*testFunctionPrototypes3 =
"int foo(int x);\r\n\
\r\n\
int foo(int x)\r\n\
{\r\n\
	return x ? x + foo(x - 1) : 1;\r\n\
}\r\n\
\r\n\
return foo(5);";
TEST_RESULT("Function prototype is implemented at a correct time", testFunctionPrototypes3, "16");

const char	*testFunctionPrototypes4 =
"class Test{ int f(); }\r\n\
int Test::f(){ return 2; }\r\n\
return Test().f();";
TEST_RESULT("Member function prototype implementation in a different scope 1", testFunctionPrototypes4, "2");

const char	*testFunctionPrototypes5 =
"class Test{ int f(int a); }\r\n\
int Test::f(int a){ if(a) return f(0); return 10; }\r\n\
return Test().f(1);";
TEST_RESULT("Member function prototype implementation in a different scope 2", testFunctionPrototypes5, "10");

const char	*testSingleArrayIndexCalculation =
"int a = 0, b = 0;\r\n\
int func(int ref v, int index){ *v += 5; return index; }\r\n\
int[2] arr;\r\n\
arr[func(&a, 0)] += 4;\r\n\
arr[func(&b, 1)]++;\r\n\
return 0;";
TEST("Single array index calculation", testSingleArrayIndexCalculation, "0")
{
	CHECK_INT("a", 0, 5, lastFailed);
	CHECK_INT("b", 0, 5, lastFailed);

	CHECK_INT("arr", 0, 4, lastFailed);
	CHECK_INT("arr", 1, 1, lastFailed);
}

const char *testBytecodeNoGlobal = "int func(){ return 0; }";
TEST_RESULT_SIMPLE("Bytecode with no global code", testBytecodeNoGlobal, "no return value");

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

const char *testTernaryTypeResolve1 = "int x = 2; auto a = !x ? nullptr : &x; return *a;";
TEST_RESULT("ternary operator result type resolve 1", testTernaryTypeResolve1, "2");

const char *testTernaryTypeResolve2 = "int x = 2; auto a = x ? &x : nullptr; return *a;";
TEST_RESULT("ternary operator result type resolve 2", testTernaryTypeResolve2, "2");

const char *testTernaryTypeResolve3 = "int foo(){ return 2; } int x = 1; auto a = !x ? nullptr : foo; return a();";
TEST_RESULT("ternary operator result type resolve 3", testTernaryTypeResolve3, "2");

const char *testTernaryTypeResolve4 = "int foo(){ return 2; } int x = 1; auto a = x ? foo : nullptr; return a();";
TEST_RESULT("ternary operator result type resolve 4", testTernaryTypeResolve4, "2");

const char *testTypesAsConditions1 = "void ref() x; if(x) return 0; else return 1;";
TEST_RESULT("Build-in types as conditions", testTypesAsConditions1, "1");

const char *testTypesAsConditions2 = "int[] x; if(x) return 0; else return 1;";
TEST_RESULT("Build-in types as conditions 2", testTypesAsConditions2, "1");

const char *testTypesAsConditions3 = "void ref() x; for(;x;) return 0; return 1;";
TEST_RESULT("Build-in types as conditions 3", testTypesAsConditions3, "1");

const char *testTypesAsConditions4 = "int[] x; for(;x;) return 0; return 1;";
TEST_RESULT("Build-in types as conditions 4", testTypesAsConditions4, "1");

const char *testTypesAsConditions5 = "void ref() x; while(x) return 0; return 1;";
TEST_RESULT("Build-in types as conditions 5", testTypesAsConditions5, "1");

const char *testTypesAsConditions6 = "int[] x; while(x) return 0; return 1;";
TEST_RESULT("Build-in types as conditions 6", testTypesAsConditions6, "1");

const char *testTypesAsConditions7 = "void ref() x; do{ return 1; }while(x);";
TEST_RESULT("Build-in types as conditions 7", testTypesAsConditions7, "1");

const char *testTypesAsConditions8 = "int[] x; do{ return 1; }while(x);";
TEST_RESULT("Build-in types as conditions 8", testTypesAsConditions8, "1");

const char *testTypesAsConditions9 = "void ref() x; if(x) return 0; return 1;";
TEST_RESULT("Build-in types as conditions 9", testTypesAsConditions9, "1");

const char *testTypesAsConditions10 = "int[] x; if(x) return 0; return 1;";
TEST_RESULT("Build-in types as conditions 10", testTypesAsConditions10, "1");

const char *testTypesAsConditions11 = "void ref() x; return x ? 0 : 1;";
TEST_RESULT("Build-in types as conditions 11", testTypesAsConditions11, "1");

const char *testTypesAsConditions12 = "int[] x; return x ? 0 : 1;";
TEST_RESULT("Build-in types as conditions 12", testTypesAsConditions12, "1");

const char *testTypesAsConditions13 = "void ref() x; return x && 1 ? 0 : 1;";
TEST_RESULT("Build-in types as conditions 13", testTypesAsConditions13, "1");

const char *testTypesAsConditions14 = "int[] x; return x && 1 ? 0 : 1;";
TEST_RESULT("Build-in types as conditions 14", testTypesAsConditions14, "1");

const char *testTypesAsConditions15 = "void ref() x; return x || 1 ? 1 : 0;";
TEST_RESULT("Build-in types as conditions 15", testTypesAsConditions15, "1");

const char *testTypesAsConditions16 = "int[] x; return x || 1 ? 1 : 0;";
TEST_RESULT("Build-in types as conditions 16", testTypesAsConditions16, "1");

const char *testTypesAsConditions17 = "void ref() x; return 1 && x ? 0 : 1;";
TEST_RESULT("Build-in types as conditions 17", testTypesAsConditions17, "1");

const char *testTypesAsConditions18 = "int[] x; return 1 && x ? 0 : 1;";
TEST_RESULT("Build-in types as conditions 18", testTypesAsConditions18, "1");

const char *testStringConversion1 = "int i = 0; return i.str() == \"0\" && int(i.str()) == i;";
TEST_RESULT("Test string conversion 1", testStringConversion1, "1");

const char *testStringConversion2 = "int i = 25; return i.str() == \"25\" && int(i.str()) == i;";
TEST_RESULT("Test string conversion 2", testStringConversion2, "1");

const char *testStringConversion3 = "int i = -25; return i.str() == \"-25\" && int(i.str()) == i;";
TEST_RESULT("Test string conversion 3", testStringConversion3, "1");

const char *testStringConversion4 = "int i = 2147483647; return i.str() == \"2147483647\" && int(i.str()) == i;";
TEST_RESULT("Test string conversion 4", testStringConversion4, "1");

const char *testStringConversion5 = "int i = -2147483647 - 1; return i.str() == \"-2147483648\" && int(i.str()) == i;";
TEST_RESULT("Test string conversion 5", testStringConversion5, "1");

const char *testStringConversion6 = "long i = 0; return i.str() == \"0\" && long(i.str()) == i;";
TEST_RESULT("Test string conversion 6", testStringConversion6, "1");

const char *testStringConversion7 = "long i = 25; return i.str() == \"25\" && long(i.str()) == i;";
TEST_RESULT("Test string conversion 7", testStringConversion7, "1");

const char *testStringConversion8 = "long i = -25; return i.str() == \"-25\" && long(i.str()) == i;";
TEST_RESULT("Test string conversion 8", testStringConversion8, "1");

const char *testStringConversion9 = "long i = 2147483647; return i.str() == \"2147483647\" && long(i.str()) == i;";
TEST_RESULT("Test string conversion 9", testStringConversion9, "1");

const char *testStringConversion10 = "long i = -2147483647 - 1; return i.str() == \"-2147483648\" && long(i.str()) == i;";
TEST_RESULT("Test string conversion 10", testStringConversion10, "1");

const char *testStringConversion11 = "long i = 9223372036854775807l; return i.str() == \"9223372036854775807\" && long(i.str()) == i;";
TEST_RESULT("Test string conversion 11", testStringConversion11, "1");

const char *testStringConversion12 = "long i = -9223372036854775807l - 1; return i.str() == \"-9223372036854775808\" && long(i.str()) == i;";
TEST_RESULT("Test string conversion 12", testStringConversion12, "1");

const char *testStringConversion13 = "return short(\"120000\") == 120000;";
TEST_RESULT("Test string conversion 13", testStringConversion13, "0");

const char *testStringConversion14 = "short i = 32767; return i.str() == \"32767\" && short(i.str()) == i;";
TEST_RESULT("Test string conversion 14", testStringConversion14, "1");

const char *testStringConversion15 = "short i = -32768; return i.str() == \"-32768\" && short(i.str()) == i;";
TEST_RESULT("Test string conversion 15", testStringConversion15, "1");

const char	*testInitialization1 =
"int foo()\r\n\
{\r\n\
	int sum = 0;\r\n\
	for(int i = 0; i < 10; i++)\r\n\
	{\r\n\
		int k;\r\n\
		sum += k;\r\n\
		k += i;\r\n\
	}\r\n\
	return sum;\r\n\
}\r\n\
return foo();";
TEST_RESULT("Test variable initialization 1", testInitialization1, "0");

const char	*testInitialization2 =
"class Nested{ int x, y; }\r\n\
int bar()\r\n\
{\r\n\
	int sum = 0;\r\n\
\r\n\
	for(int i = 0; i < 10; i++)\r\n\
	{\r\n\
		int y;\r\n\
		int[1024] z;\r\n\
		Nested w;\r\n\
\r\n\
		sum += y + z[55] + w.x;\r\n\
		y += i;\r\n\
		z[55] += i;\r\n\
		w.x += i;\r\n\
	}\r\n\
\r\n\
	return sum;\r\n\
}\r\n\
\r\n\
return bar();";
TEST_RESULT("Test variable initialization 2", testInitialization2, "0");

const char	*testInitialization3 =
"class Nested{ int x, y; }\r\n\
class Bar\r\n\
{\r\n\
	int x = 2;\r\n\
	int y; \r\n\
	int[128] z;\r\n\
	Nested w;\r\n\
}\r\n\
\r\n\
int bar()\r\n\
{\r\n\
	int sum = 0;\r\n\
\r\n\
	for(int i = 0; i < 10; i++)\r\n\
	{\r\n\
		Bar k;\r\n\
		sum += k.y + k.z[55] + k.w.x;\r\n\
		k.y += i;\r\n\
		k.z[55] += i;\r\n\
		k.w.x += i;\r\n\
	}\r\n\
\r\n\
	return sum;\r\n\
}\r\n\
\r\n\
return bar();";
TEST_RESULT("Test variable initialization 3", testInitialization3, "0");

const char	*testInitialization4 =
"class Nested{ int x, y; }\r\n\
class Bar\r\n\
{\r\n\
	void Bar()\r\n\
	{\r\n\
		x = 2;\r\n\
	}\r\n\
	\r\n\
	int x;\r\n\
	int y;\r\n\
	int[1024] z;\r\n\
	Nested w;\r\n\
}\r\n\
\r\n\
int bar()\r\n\
{\r\n\
	int sum = 0;\r\n\
	\r\n\
	for(int i = 0; i < 10; i++)\r\n\
	{\r\n\
		Bar k;\r\n\
		\r\n\
		sum += k.y + k.z[55] + k.w.x;\r\n\
		k.y += i;\r\n\
		k.z[55] += i;\r\n\
		k.w.x += i;\r\n\
	}\r\n\
	\r\n\
	return sum;\r\n\
}\r\n\
\r\n\
return bar();";
TEST_RESULT("Test variable initialization 4", testInitialization4, "0");

const char	*testInitialization5 =
"coroutine int foo()\r\n\
{\r\n\
	int sum = 0;\r\n\
\r\n\
	for(int i = 0; i < 10; i++)\r\n\
	{\r\n\
		int k;\r\n\
\r\n\
		sum += k;\r\n\
		k += i;\r\n\
	}\r\n\
\r\n\
	return sum;\r\n\
}\r\n\
\r\n\
class Nested{ int x, y; }\r\n\
class Bar\r\n\
{\r\n\
	int x = 2;\r\n\
	int y;\r\n\
	int[1024] z;\r\n\
	Nested w;\r\n\
}\r\n\
\r\n\
coroutine int bar()\r\n\
{\r\n\
	int sum = 0;\r\n\
\r\n\
	for(int i = 0; i < 10; i++)\r\n\
	{\r\n\
		Bar k;\r\n\
		int[1024] z;\r\n\
		Nested w;\r\n\
\r\n\
		sum += k.y + k.z[55] + k.w.x + z[55] + w.x;\r\n\
		k.y += i;\r\n\
		k.z[55] += i;\r\n\
		k.w.x += i;\r\n\
		z[55] += i;\r\n\
		w.x += i;\r\n\
	}\r\n\
\r\n\
	return sum;\r\n\
}\r\n\
\r\n\
return foo() + bar();";
TEST_RESULT("Test variable initialization 5", testInitialization5, "0");

const char	*testInitialization6 =
"coroutine int foo(){ int k; yield k++; yield k++; yield k++; return 0; }\r\n\
int sum; for(i in foo) sum += i; for(i in foo) sum += i;\r\n\
return sum;";
TEST_RESULT("Test variable initialization 6", testInitialization6, "6");

const char	*testInitialization7 =
"class Foo\r\n\
{\r\n\
	void Foo()\r\n\
	{\r\n\
		y = x;\r\n\
		x = 2;\r\n\
	}\r\n\
\r\n\
	int x, y;\r\n\
}\r\n\
\r\n\
int foo()\r\n\
{\r\n\
	int sum = 0;\r\n\
\r\n\
	for(int i = 0; i < 10; i++)\r\n\
	{\r\n\
		Foo f;\r\n\
\r\n\
		sum += f.y;\r\n\
	}\r\n\
\r\n\
	return sum;\r\n\
}\r\n\
\r\n\
return foo();";
TEST_RESULT("Test variable initialization 7", testInitialization7, "0");
