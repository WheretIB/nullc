#include "TestBase.h"

const char	*testFuncOverload = 
"int fa(int i){ return i*2; }\r\n\
int fa(int i, double c){ return i*c; }\r\n\
int fa(float i){ return i*3.0f; }\r\n\
return fa(5.0f) * fa(2, 3.0);";
TEST_RESULT("Function overload test", testFuncOverload, "90");

const char	*testOverloadedOperator1 =
"import std.math;\r\n\
\r\n\
void operator= (float4 ref a, float b){ a.x = a.y = a.y = a.z = b; }\r\n\
float4 a, b = 16;\r\n\
a = 12;\r\n\
return int(a.x + b.z);";
TEST_RESULT("Overloaded operator =", testOverloadedOperator1, "28");

const char	*testOverloadedOperator2 =
"class string{ int len; }\r\n\
void operator=(string ref a, char[] str){ a.len = str.size; }\r\n\
string b = \"assa\";\r\n\
class funcholder{ int ref(int) ptr; }\r\n\
void operator=(funcholder ref a, int ref(int) func){ a.ptr = func; }\r\n\
int test(int a){ return -a; }\r\n\
funcholder c = test;\r\n\
return (c.ptr)(12);";
TEST("Overloaded operator = with arrays and functions", testOverloadedOperator2, "-12")
{
	CHECK_INT("b", 0, 5, lastFailed);
}

const char	*testOverloadedOperator3 =
"import std.math;\r\n\
\r\n\
void operator= (float4 ref a, float b){ a.x = a.y = a.y = a.z = b; }\r\n\
void operator+= (float4 ref a, float b){ a.x += b; a.y += b; a.y += b; a.z += b; }\r\n\
float4 a, b = 16;\r\n\
a = 12;\r\n\
a += 3;\r\n\
return int(a.x + b.z);";
TEST_RESULT("Overloaded operator =", testOverloadedOperator3, "31");

const char	*testUnaryOverloads =
"int operator ~(int v){ return v * 4; }\r\n\
int operator !(int v){ return v * 3; }\r\n\
int operator +(int v){ return v * 2; }\r\n\
int operator -(int v){ return v * -1; }\r\n\
int[4] arr = { 13, 17, 21, 25 };\r\n\
arr[0] = ~arr[0];\r\n\
arr[1] = !arr[1];\r\n\
arr[2] = +arr[2];\r\n\
arr[3] = -arr[3];\r\n\
return arr[0]*arr[1]*arr[2]*arr[3];";
TEST_RESULT("Unary operator overloading 1", testUnaryOverloads, "-2784600");

const char	*testUnaryOverloads2 =
"class Foo{ int x; void Foo(int x){ this.x = x; } }\r\n\
int operator-(Foo x){ return -x.x; }\r\n\
Foo operator+(Foo x){ return Foo(x.x + 1); }\r\n\
return - - + +Foo(5);";
TEST_RESULT("Unary operator overloading 2", testUnaryOverloads2, "7");

const char	*testArrayIndexOverloadPointers =
"auto arr = { 100, 200, 300, 400 };\r\n\
int[4] ref u = &arr;\r\n\
int[] arr2 = arr;\r\n\
int[] ref u2 = &arr2;\r\n\
int operator[](int[] ref arr, int index){ return 5; }\r\n\
return u2[0] + arr2[1] + u[2] + arr[3];";
TEST_RESULT("Array index overload call for pointer to array type", testArrayIndexOverloadPointers, "20");

const char	*testArrayIndexOverloadPointers2 =
"import std.vector;\r\n\
vector<int> v;\r\n\
auto vv = &v;\r\n\
vv.push_back(5);\r\n\
v.push_back(7);\r\n\
return vv[0] + vv[1];";
TEST_RESULT("Array index overload call for pointer to class type", testArrayIndexOverloadPointers2, "12");

const char	*testLocalOperators =
"int funcA(int a, b)\r\n\
{\r\n\
	int operator+(int x, y){ return x * y; }\r\n\
	return a + b;\r\n\
}\r\n\
int funcB(int a, b)\r\n\
{\r\n\
	int operator+(int x, y){ return x - y; }\r\n\
	return a + b;\r\n\
}\r\n\
int funcC(int a, b)\r\n\
{\r\n\
	return a + b;\r\n\
}\r\n\
int u = funcA(4, 8);\r\n\
int v = funcB(4, 8);\r\n\
int w = funcC(4, 8);\r\n\
return u + v + w;";
TEST("Local operator definition", testLocalOperators, "40")
{
	CHECK_INT("u", 0, 32, lastFailed);
	CHECK_INT("v", 0, -4, lastFailed);
	CHECK_INT("w", 0, 12, lastFailed);
}

const char	*testClassOperators =
"int operator+(int a, b){ return a - b; }\r\n\
class Foo\r\n\
{\r\n\
	int operator+(int a, b){ return a * b; }\r\n\
	\r\n\
	int x, y;\r\n\
	int func()\r\n\
	{\r\n\
		return x + y;\r\n\
	}\r\n\
}\r\n\
int funcA(int a, b)\r\n\
{\r\n\
	return a + b;\r\n\
}\r\n\
\r\n\
Foo test;\r\n\
test.x = 5;\r\n\
test.y = 7;\r\n\
\r\n\
int u = test.func();\r\n\
int v = funcA(test.x, test.y);\r\n\
\r\n\
return u + v;";
TEST("Class operator definition", testClassOperators, "37")
{
	CHECK_INT("u", 0, 35, lastFailed);
	CHECK_INT("v", 0, -2, lastFailed);
}

const char	*testOverloadedOperatorFunctionCall =
"int x = 4;\r\n\
int operator()(int ref x){ return 2 * *x; }\r\n\
int operator()(int ref x, int y){ return y * *x; }\r\n\
int operator()(int ref x, int y, z){ return y * *x + z; }\r\n\
return x() + x(10) + x(24, 4);";
TEST_RESULT("Overloaded function call operator", testOverloadedOperatorFunctionCall, "148");

const char	*testOverloadOfAnForwardDeclaredFunction =
"int f(int a);\r\n\
int a = f(1);\r\n\
float f(int a){ return a; }\r\n\
int f(int a){ return 1; }\r\n\
return a;";
TEST_RESULT("Overload of a forward-declared function", testOverloadOfAnForwardDeclaredFunction, "1");

const char	*testArrayIndexOverloadZeroMultiple =
"class Foo\r\n\
{\r\n\
	int[16] arr;\r\n\
}\r\n\
auto operator[](Foo ref a){ return 42; }\r\n\
auto operator[](Foo ref a, int i, j){ return &a.arr[i * 4 + j]; }\r\n\
Foo x;\r\n\
\r\n\
x[1, 3] = 5;\r\n\
return -x[1, 3] + x[];";
TEST_RESULT("Array index overload with zero or multiple arguments", testArrayIndexOverloadZeroMultiple, "37");

const char	*testArrayIndexOverloadZeroMultiple2 =
"class Foo\r\n\
{\r\n\
	int[16] arr;\r\n\
}\r\n\
auto operator[](Foo ref a){ return 42; }\r\n\
auto operator[](Foo ref a, int i, j){ return &a.arr[i * 4 + j]; }\r\n\
Foo x;\r\n\
Foo ref y = &x;\r\n\
y[1, 3] = 5;\r\n\
return (&y)[1, 3];";
TEST_RESULT("Array index overload, check if pointer to array is derefenced", testArrayIndexOverloadZeroMultiple2, "5");

const char	*testDefaultArrayComparisonShouldntBreakUserDefined =
"char[] a = \"hello\", b = \"hello\";\r\n\
return a == b;";
TEST_RESULT("Default array comparison function shouldn't break user defined functions", testDefaultArrayComparisonShouldntBreakUserDefined, "1");

const char	*testLogOrAndLogAndOperatorOverload1 =
"class Foo{ int x; }\r\n\
\r\n\
int operator||(Foo a, Foo ref() b)\r\n\
{\r\n\
	return a.x || b().x;\r\n\
}\r\n\
int operator&&(Foo a, Foo ref() b)\r\n\
{\r\n\
	return a.x && b().x;\r\n\
}\r\n\
Foo a, b;\r\n\
a.x = 1;\r\n\
b.x = 0;\r\n\
\r\n\
Foo k(){ assert(0); return Foo(); } // never called!\r\n\
\r\n\
assert(a || k());\r\n\
assert(!(b && k()));\r\n\
\r\n\
return 1;";
TEST_RESULT("overloaded || and && operators do not break short-circuiting", testLogOrAndLogAndOperatorOverload1, "1");

const char	*testLogAndOperatorFunctionWrapping1 =
"class Foo{ int x; }\r\n\
\r\n\
bool operator&&(Foo a, Foo ref() b)\r\n\
{\r\n\
	return a.x && b().x;\r\n\
}\r\n\
\r\n\
Foo get(int x)\r\n\
{\r\n\
	Foo result;\r\n\
	result.x = x;\r\n\
	return result;\r\n\
}\r\n\
\r\n\
bool wrap(int a, int b)\r\n\
{\r\n\
	return get(a) && get(b);\r\n\
}\r\n\
\r\n\
return wrap(3, 4);";
TEST_RESULT("function wrapping for overloaded operator && or || might create a function with external access", testLogAndOperatorFunctionWrapping1, "1");

const char	*testLogAndOperatorFunctionWrapping2 =
"class Wrap{ int x; }\r\n\
bool bool(Wrap str){ return true; }\r\n\
auto test(Wrap name, b){ return name && (&name != &b); }\r\n\
return test(Wrap(), Wrap());";
TEST_RESULT("syntax function wrapping must always be used for overloaded operator && or ||", testLogAndOperatorFunctionWrapping2, "1");

const char	*testInOverload1 =
"import std.vector;\r\n\
\r\n\
vector<int> arr;\r\n\
arr.push_back(1);\r\n\
arr.push_back(2);\r\n\
arr.push_back(3);\r\n\
arr.push_back(4);\r\n\
\r\n\
bool operator in(@T val, vector<@T> arr)\r\n\
{\r\n\
	for(i in arr)\r\n\
		if(val == i)\r\n\
			return true;\r\n\
	return false;\r\n\
}\r\n\
auto x = 3 in arr;\r\n\
auto y = 6 in arr;\r\n\
\r\n\
return x == true && y == false;";
TEST_RESULT("overloaded in operator", testInOverload1, "1");

const char	*testOverloadedOperatorInDefinition =
"auto x = { 1, 2, 3, 4 };\r\n\
auto ref y = &x;\r\n\
auto[] z = y;\r\n\
return int(z[2]);";
TEST_RESULT("overloaded operator in variable definition is called before implicit conversions", testOverloadedOperatorInDefinition, "3");
