#include "TestBase.h"

const char *testIntOp =
"// Integer tests\r\n\
int[33] res;\r\n\
int a = 14, b = 3, c = 0;\r\n\
res[0] = a+b; // 17\r\n\
res[1] = a-b; // 11\r\n\
res[2] = -a; // -14\r\n\
res[3] = ~b; // -4\r\n\
res[4] = a*b; // 42\r\n\
res[5] = a/b; // 4\r\n\
res[6] = a%b; // 2\r\n\
res[7] = a**b; // 2744\r\n\
res[8] = a > b; // 1\r\n\
res[9] = a < b; // 0\r\n\
res[10] = a >= b; // 1\r\n\
res[11] = a <= b; // 0\r\n\
res[12] = a == b; // 0\r\n\
res[13] = a != b; // 1\r\n\
res[14] = a << b; // 112\r\n\
res[15] = a >> b; // 1\r\n\
res[16] = a & b; // 2\r\n\
res[17] = a | b; // 15\r\n\
res[18] = a ^ b; // 13\r\n\
int o = 0, i = 1;\r\n\
res[19] = o && o;\r\n\
res[20] = o && i;\r\n\
res[21] = i && o;\r\n\
res[22] = i && i;\r\n\
res[23] = o || o;\r\n\
res[24] = o || i;\r\n\
res[25] = i || o;\r\n\
res[26] = i || i;\r\n\
res[27] = o ^^ o;\r\n\
res[28] = o ^^ i;\r\n\
res[29] = i ^^ o;\r\n\
res[30] = i ^^ i;\r\n\
res[31] = !i; // 0\r\n\
res[32] = !o; // 1\r\n\
return a >> b;";

TEST("Integer operation test", testIntOp, "1")
{
	CHECK_INT("a", 0, 14, lastFailed);
	CHECK_INT("b", 0, 3, lastFailed);
	CHECK_INT("c", 0, 0, lastFailed);
	int resExp[] = { 17, 11, -14, -4, 42, 4, 2, 2744, 1, 0, 1, 0, 0, 1, 112, 1, 2, 15, 13, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1 };
	for(int i = 0; i < 33; i++)
		CHECK_INT("res", i, resExp[i], lastFailed);
}

const char	*testDoubleOp = 
"// Floating point tests\r\n\
double[13] res;\r\n\
double a = 14.0, b = 3.0;\r\n\
res[0] = a+b;\r\n\
res[1] = a-b;\r\n\
res[2] = -a;\r\n\
res[3] = a*b;\r\n\
res[4] = a/b;\r\n\
res[5] = a%b;\r\n\
res[6] = a**b;\r\n\
res[7] = a > b;\r\n\
res[8] = a < b;\r\n\
res[9] = a >= b;\r\n\
res[10] = a <= b;\r\n\
res[11] = a == b;\r\n\
res[12] = a != b;\r\n\
return a+b;";

TEST("Double operation test", testDoubleOp, "17.000000")
{
	CHECK_DOUBLE("a", 0, 14.0, lastFailed);
	CHECK_DOUBLE("b", 0, 3.0, lastFailed);
	double resExp[] = { 17.0, 11.0, -14.0, 42.0, 14.0/3.0, 2.0, 2744.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0};
	for(int i = 0; i < 13; i++)
		CHECK_DOUBLE("res", i, resExp[i], lastFailed);
}

const char	*testLongOp = 
"// Long tests\r\n\
long[34] res;\r\n\
long a = 4494967296l, b = 594967296l, c = 3;\r\n\
res[0] = a+b; // 5089934592\r\n\
res[1] = a-b; // 3900000000\r\n\
res[2] = -a; // -4494967296\r\n\
res[3] = ~a; // -4494967297\r\n\
res[4] = a*b; // 2674358537709551616\r\n\
res[5] = a/b; // 7\r\n\
res[6] = a%b; // 330196224\r\n\
res[7] = 594967**c; // 210609828468829063\r\n\
res[8] = a > b; // 1\r\n\
res[9] = a < b; // 0\r\n\
res[10] = a >= b; // 1\r\n\
res[11] = a <= b; // 0\r\n\
res[12] = a == b; // 0\r\n\
res[13] = a != b; // 1\r\n\
res[14] = a << c; // 35959738368 \r\n\
res[15] = c << 45; // 105553116266496 \r\n\
res[16] = a >> c; // 561870912\r\n\
res[17] = a & b; // 56771072\r\n\
res[18] = a | b; // 5033163520\r\n\
res[19] = a ^ b; // 4976392448\r\n\
long o = 0, i = 1;\r\n\
res[20] = o && o;\r\n\
res[21] = o && i;\r\n\
res[22] = i && o;\r\n\
res[23] = i && i;\r\n\
res[24] = o || o;\r\n\
res[25] = o || i;\r\n\
res[26] = i || o;\r\n\
res[27] = i || i;\r\n\
res[28] = o ^^ o;\r\n\
res[29] = o ^^ i;\r\n\
res[30] = i ^^ o;\r\n\
res[31] = i ^^ i;\r\n\
res[32] = !i; // 0\r\n\
res[33] = !o; // 1\r\n\
\r\n\
return 1;";

TEST("Long operation test", testLongOp, "1")
{
	CHECK_LONG("a", 0, 4494967296ll, lastFailed);
	CHECK_LONG("b", 0, 594967296ll, lastFailed);
	CHECK_LONG("c", 0, 3ll, lastFailed);
	long long resExp[] = { 5089934592ll, 3900000000ll, -4494967296ll, -4494967297ll, 2674358537709551616ll, 7, 330196224, 210609828468829063ll, 1, 0, 1, 0, 0, 1,
		35959738368ll, 105553116266496ll, 561870912, 56771072, 5033163520ll, 4976392448ll, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1 };
	for(int i = 0; i < 34; i++)
		CHECK_LONG("res", i, resExp[i], lastFailed);
}

const char	*tesIncDec =
"// Decrement and increment tests for all types\r\n\
double a1=5, b1=5, c1, d1, e1, f1;\r\n\
float a2=5, b2=5, c2, d2, e2, f2;\r\n\
long a3=5, b3=5, c3, d3, e3, f3;\r\n\
int a4=5, b4=5, c4, d4, e4, f4;\r\n\
short a5=5, b5=5, c5, d5, e5, f5;\r\n\
char a6=5, b6=5, c6, d6, e6, f6;\r\n\
c1 = a1++; a1++; e1 = ++a1; ++a1;\r\n\
d1 = b1--; b1--; f1 = --b1; --b1;\r\n\
\r\n\
c2 = a2++; a2++; e2 = ++a2; ++a2;\r\n\
d2 = b2--; b2--; f2 = --b2; --b2;\r\n\
\r\n\
c3 = a3++; a3++; e3 = ++a3; ++a3;\r\n\
d3 = b3--; b3--; f3 = --b3; --b3;\r\n\
\r\n\
c4 = a4++; a4++; e4 = ++a4; ++a4;\r\n\
d4 = b4--; b4--; f4 = --b4; --b4;\r\n\
\r\n\
c5 = a5++; a5++; e5 = ++a5; ++a5;\r\n\
d5 = b5--; b5--; f5 = --b5; --b5;\r\n\
\r\n\
c6 = a6++; a6++; e6 = ++a6; ++a6;\r\n\
d6 = b6--; b6--; f6 = --b6; --b6;\r\n\
return 1;";

TEST("Decrement and increment tests for all types", tesIncDec, "1")
{
	const char *name[] = { "a", "b", "c", "d", "e", "f" };
	int	value[] = { 9, 1, 5, 5, 8, 2 };

	int num = 1;
	for(int i = 0; i < 6; i++)
		CHECK_DOUBLE(Tests::Format("%s%d", name[i], num), 0, value[i], lastFailed);
	num = 2;
	for(int i = 0; i < 6; i++)
		CHECK_FLOAT(Tests::Format("%s%d", name[i], num), 0, (float)value[i], lastFailed);
	num = 3;
	for(int i = 0; i < 6; i++)
		CHECK_LONG(Tests::Format("%s%d", name[i], num), 0, value[i], lastFailed);
	num = 4;
	for(int i = 0; i < 6; i++)
		CHECK_INT(Tests::Format("%s%d", name[i], num), 0, value[i], lastFailed);
	num = 5;
	for(int i = 0; i < 6; i++)
		CHECK_SHORT(Tests::Format("%s%d", name[i], num), 0, (short)value[i], lastFailed);
	num = 6;
	for(int i = 0; i < 6; i++)
		CHECK_CHAR(Tests::Format("%s%d", name[i], num), 0, (char)value[i], lastFailed);
}

const char	*testTypeConv = 
"// Type conversions\r\n\
int ia=3, ib, ic;\r\n\
double da=5.0, db, dc;\r\n\
long la=4l, lb, lc;\r\n\
ib = da;\r\n\
ic = la;\r\n\
db = ia;\r\n\
dc = la;\r\n\
lb = ia;\r\n\
lc = da;\r\n\
return 1;";
TEST("Type conversions", testTypeConv, "1")
{
	CHECK_INT("ia", 0, 3, lastFailed);
	CHECK_INT("ib", 0, 5, lastFailed);
	CHECK_INT("ic", 0, 4, lastFailed);
	CHECK_DOUBLE("da", 0, 5.0, lastFailed);
	CHECK_DOUBLE("db", 0, 3.0, lastFailed);
	CHECK_DOUBLE("dc", 0, 4.0, lastFailed);
	CHECK_LONG("la", 0, 4, lastFailed);
	CHECK_LONG("lb", 0, 3, lastFailed);
	CHECK_LONG("lc", 0, 5, lastFailed);
}

const char	*testDoublePow = 
"double a = 0.9;\r\n\
return a**2.0;";
TEST("Double power", testDoublePow, "0.810000")
{
	CHECK_DOUBLE("a", 0, 0.9, lastFailed);
}

const char	*testHexConst = 
"//Hexadecimal constants\r\n\
auto a = 0xdeadbeef;\r\n\
auto b = 0xcafe;\r\n\
auto c = 0x7fffffffffffffff;\r\n\
return a;";
TEST("Hexadecimal constants", testHexConst, "3735928559L")
{
	CHECK_LONG("a", 0, 3735928559ll, lastFailed);
	CHECK_INT("b", 0, 51966, lastFailed);
	CHECK_LONG("c", 0, 9223372036854775807ll, lastFailed);
}

const char	*testPriority = 
"int a = 13, b = 17, c = 14;\r\n\
int[10] res;\r\n\
res[0] = a + b * c;\r\n\
res[1] = a + b ** (c-10) * a;\r\n\
return 1;";
TEST("Operation priority test", testPriority, "1")
{
	CHECK_INT("a", 0, 13, lastFailed);
	CHECK_INT("b", 0, 17, lastFailed);
	CHECK_INT("c", 0, 14, lastFailed);
	int resExp[] = { 251, 1085786, -14, -4, 42, 4, 2, 2744, 1, 0, 1, 0, 0, 1, 112, 1, 2, 15, 13, 1, 1, 0, 0, 1, 1, 0, 1 };
	for(int i = 0; i < 2; i++)
		CHECK_INT("res", i, resExp[i], lastFailed);
}

const char	*testLongIncDec =
"long count = 0xfffffffff;\r\n\
count--;\r\n\
assert(count == 0xffffffffe);\r\n\
count++;\r\n\
assert(count == 0xfffffffff);\r\n\
count++;\r\n\
assert(count == 0x1000000000);\r\n\
return count;";
TEST_RESULT("Long increment and decrement extra tests", testLongIncDec, "68719476736L");

const char *testDiv0ConstanFold = "double a = 1.0 / 0.0; return 10;";
TEST_RESULT("Double division by zero during constant folding.", testDiv0ConstanFold, "10");

const char *testDoubleMod = "double a = 800000000000000000000.0; return (a % 5.5) < 5.5;";
TEST_RESULT("Double modulus division.", testDoubleMod, "1");

const char *testNumberParse1 = "return 0x0000000000000000000000000f;";
TEST_RESULT("Leading zero skip (hex)", testNumberParse1, "15");
const char *testNumberParse2 = "return 0x1eadbeef;";
TEST_RESULT("int type constant (hex)", testNumberParse2, "514703087");
const char *testNumberParse3 = "return 0xffffffffffffffff;";
TEST_RESULT("negative int type constant (hex)", testNumberParse3, "-1");
const char *testNumberParse4 = "return 0xdeadbeefdeadbeef;";
TEST_RESULT("negative long type constant (hex)", testNumberParse4, "-2401053088876216593L");

const char *testNumberParse5 = "return 020000000000;";
TEST_RESULT("long type constant 1 (oct)", testNumberParse5, "2147483648L");
const char *testNumberParse6 = "return 017777777777;";
TEST_RESULT("int type constant (oct)", testNumberParse6, "2147483647");
const char *testNumberParse7 = "return 0000000000000000000000000000000176666;";
TEST_RESULT("Leading zero skip (oct)", testNumberParse7, "64950");
const char *testNumberParse8 = "return 0777777777777777777777;";
TEST_RESULT("long type constant 2 (oct)", testNumberParse8, "9223372036854775807L");
const char *testNumberParse9 = "return 01777777777777777777777;";
TEST_RESULT("negative int type constant (oct)", testNumberParse9, "-1");
const char *testNumberParse10 = "return 01777700000000000000000;";
TEST_RESULT("negative long type constant (oct)", testNumberParse10, "-2251799813685248L");

const char *testNumberParse11 = "return 01111111111111111111111111111111b;";
TEST_RESULT("int type constant (bin)", testNumberParse11, "2147483647");
const char *testNumberParse12 = "return 11111111111111111111111111111111b;";
TEST_RESULT("long type constant (bin)", testNumberParse12, "4294967295L");
const char *testNumberParse13 = "return 000000000000000000000000111111111111111111111111111111111111111111111111111111111111111b;";
TEST_RESULT("Leading zero skip (bin)", testNumberParse13, "9223372036854775807L");
const char *testNumberParse14 = "return 01111111111111111111111111111111111111111111111111111111111111111b;";
TEST_RESULT("negative int type constant (bin)", testNumberParse14, "-1");
const char *testNumberParse15 = "return 01111111111111111111111111111000000000000000000000000000000000000b;";
TEST_RESULT("negative long type constant (bin)", testNumberParse15, "-68719476736L");

const char *testIntegerPow = "int a = 1; return a*100**a;";
TEST_RESULT("Integer power test", testIntegerPow, "100");

const char *testIntegerPowCF = "return 100**1;";
TEST_RESULT("Integer power test. Constant folding", testIntegerPowCF, "100");

const char	*testVariableModifyAssignment1 =
"int[11] res = 12; // 1100\r\n\
res[0] += 5; assert(res[0] == 17);\r\n\
res[1] -= 5; assert(res[1] == 7);\r\n\
res[2] *= 5; assert(res[2] == 60);\r\n\
res[3] /= 4; assert(res[3] == 3);\r\n\
res[4] **= 2; assert(res[4] == 144);\r\n\
res[5] %= 5; assert(res[5] == 2);\r\n\
res[6] <<= 2; assert(res[6] == 48);\r\n\
res[7] >>= 1; assert(res[7] == 6);\r\n\
res[8] &= 5; assert(res[8] == 4);\r\n\
res[9] |= 5; assert(res[9] == 13);\r\n\
res[10] ^= 5; assert(res[10] == 9);\r\n\
return 1;";
TEST_RESULT("Integer modify assignment operator test.", testVariableModifyAssignment1, "1");

const char	*testVariableModifyAssignment2 =
"double[6] res = 12;\r\n\
res[0] += 5; assert(res[0] == 17);\r\n\
res[1] -= 5; assert(res[1] == 7);\r\n\
res[2] *= 5; assert(res[2] == 60);\r\n\
res[3] /= 4; assert(res[3] == 3);\r\n\
res[4] **= 2; assert(res[4] == 144);\r\n\
res[5] %= 5; assert(res[5] == 2);\r\n\
return 1;";
TEST_RESULT("Double modify assignment operator test.", testVariableModifyAssignment2, "1");

const char	*testBoolType =
"assert(true);\r\n\
assert(!false);\r\n\
\r\n\
bool a, b;\r\n\
auto x = a == b;\r\n\
\r\n\
assert(typeof(x) == bool);\r\n\
assert(x == true);\r\n\
\r\n\
bool t = true;\r\n\
bool f = false;\r\n\
\r\n\
assert(t && t);\r\n\
assert(!(t && f));\r\n\
assert(t || f);\r\n\
assert(f || t);\r\n\
assert(t || t);\r\n\
assert(!(f || f));\r\n\
\r\n\
{\r\n\
	bool d = 5;\r\n\
	assert(d == true);\r\n\
	bool e = 5.0;\r\n\
	assert(e == true);\r\n\
	bool f = 6l;\r\n\
	assert(f == true);\r\n\
}\r\n\
{\r\n\
	bool d = 0;\r\n\
	assert(d == false);\r\n\
	bool e = 0.0;\r\n\
	assert(e == false);\r\n\
	bool f = 0l;\r\n\
	assert(f == false);\r\n\
}\r\n\
{\r\n\
	bool d = true, e = d;\r\n\
	assert(d + e == 2);\r\n\
	assert(typeof(d + e) == int);\r\n\
	bool f = d + e;\r\n\
	assert(f);\r\n\
	assert(f == true);\r\n\
	bool g = d - e;\r\n\
	assert(g == false);\r\n\
	assert(!g);\r\n\
}\r\n\
{\r\n\
	bool a = true, b = false;\r\n\
	assert(a | b);\r\n\
	assert(a || b);\r\n\
	assert(typeof(a | b) == bool);\r\n\
	assert(typeof(a || b) == bool);\r\n\
\r\n\
	if(true)\r\n\
		a = false;\r\n\
	assert(!a);\r\n\
	if(false)\r\n\
		a = true;\r\n\
	assert(!a);\r\n\
	a = b ? false : true;\r\n\
	assert(a);\r\n\
}\r\n\
{\r\n\
	bool a = true, b = true;\r\n\
	a += b;\r\n\
	assert(a == true);\r\n\
\r\n\
	a = true;\r\n\
	b = true;\r\n\
	a -= b;\r\n\
	assert(a == false);\r\n\
\r\n\
	a = false;\r\n\
	b = true;\r\n\
	a |= b;\r\n\
	assert(a == true);\r\n\
\r\n\
	a = true;\r\n\
	a *= 10;\r\n\
	assert(a == true);\r\n\
\r\n\
	a *= 0;\r\n\
	assert(a == false);\r\n\
}\r\n\
{\r\n\
	int x = 5;\r\n\
	assert(typeof(!x) == bool);\r\n\
	\r\n\
	assert(typeof(!5) == bool);\r\n\
	assert(!0 == true);\r\n\
	\r\n\
	assert(typeof(5 == 3) == bool);\r\n\
	\r\n\
	assert(typeof(4l > 5l) == bool);\r\n\
	assert(typeof(4.0 == 5.0) == bool);\r\n\
}\r\n\
{\r\n\
	int a = 5;\r\n\
	double b = 5.0;\r\n\
	long c = 6l;\r\n\
	bool d = a;\r\n\
	assert(d == true);\r\n\
	bool e = b;\r\n\
	assert(e == true);\r\n\
	bool f = c;\r\n\
	assert(f == true);\r\n\
}\r\n\
{\r\n\
	bool t = true;\r\n\
	bool f = false;\r\n\
	assert(typeof(t || f) == bool);\r\n\
	assert(typeof(t | f) == bool);\r\n\
\r\n\
	assert(typeof(true | false) == bool);\r\n\
}\r\n\
return 1;";
TEST_RESULT("bool type tests", testBoolType, "1");

const char	*testConstantFoldExtra =
"class Foo\r\n\
{\r\n\
	const char a = 5;\r\n\
	const short b = 7;\r\n\
}\r\n\
char a = 5;\r\n\
short b = 7;\r\n\
assert(typeof(a + b) == short);\r\n\
assert(typeof(b + a) == short);\r\n\
assert(typeof((Foo.a + Foo.b)) == short);\r\n\
assert(typeof((Foo.b + Foo.a)) == short);\r\n\
\r\n\
return 1;";
TEST_RESULT("Constant fold test with non-stack types", testConstantFoldExtra, "1");

const char	*testBoolType2 =
"int foo(bool a){ return a; }\r\n\
\r\n\
int x = 12;\r\n\
assert(1 == foo(5));\r\n\
assert(1 == foo(x));\r\n\
float y = 5.0f;\r\n\
assert(1 == foo(y));\r\n\
\r\n\
return 1;";
TEST_RESULT("bool as a function argument", testBoolType2, "1");

const char	*testBoolType3 =
"assert(true == bool(5));\r\n\
assert(true == bool(6.0));\r\n\
assert(true == bool(5.0f));\r\n\
assert(false == bool('\\0'));\r\n\
\r\n\
auto x1 = 5;\r\n\
auto x2 = 6.0;\r\n\
auto x3 = 5.0f;\r\n\
auto x4 = '\\0';\r\n\
assert(true == bool(x1));\r\n\
assert(true == bool(x2));\r\n\
assert(true == bool(x3));\r\n\
assert(false == bool(x4));\r\n\
\r\n\
auto a = new bool(5);\r\n\
assert(*a == true);\r\n\
\r\n\
a = new bool(x1);\r\n\
assert(*a == true);\r\n\
\r\n\
return 1;";
TEST_RESULT("bool type constructors", testBoolType3, "1");

const char	*testBoolType4 =
"bool bar(){ return 5; }\r\n\
bool foo(int x){ return x; }\r\n\
\r\n\
assert(bar() == true);\r\n\
assert(foo(45) == true);\r\n\
\r\n\
return 1;";
TEST_RESULT("bool as a function return type", testBoolType4, "1");

const char	*testNestedComment = "return /* /* */ */1;";
TEST_RESULT("nested comments", testNestedComment, "1");

const char	*testNestedComment2 = "return /* \"/*\" */1;";
TEST_RESULT("nested comments 2", testNestedComment2, "1");

const char	*testNestedComment3 = "return 1; /*aa\"bb";
TEST_RESULT_SIMPLE("nested comments 3", testNestedComment3, "1");

const char	*testIntegerConstantFolding1 = "class X{ const short A = 2; } return -X.A;";
TEST_RESULT("Integer type constant folding 1", testIntegerConstantFolding1, "-2");

const char	*testIntegerConstantFolding2 = "return -' ';";
TEST_RESULT("Integer type constant folding 2", testIntegerConstantFolding2, "-32");

const char	*testIntegerConstantFolding3 = "class X{ const short A = 2; } return ~X.A;";
TEST_RESULT("Integer type constant folding 3", testIntegerConstantFolding3, "-3");

const char	*testIntegerConstantFolding4 = "return ~' ';";
TEST_RESULT("Integer type constant folding 4", testIntegerConstantFolding4, "-33");

const char	*testCompileTimeFunctionEvaluationBug1 = "int foo(){ int x = 1; x++; return x; } return foo();";
TEST_RESULT("Compile-time function evaluation bug 1", testCompileTimeFunctionEvaluationBug1, "2");

const char	*testCompileTimeFunctionEvaluationBug2 = "int foo(){ int x = 1; return --x; } return foo();";
TEST_RESULT("Compile-time function evaluation bug 2", testCompileTimeFunctionEvaluationBug2, "0");

const char	*testTernaryExpressionCommonTypeResolve = "int x; return typeof(x < 1 ? 2 : 3.0) == double && int(x < 1 ? 2 : 3.0) == 2;";
TEST_RESULT("Ternary expression common type resolve when different types are placed in true and false parts", testTernaryExpressionCommonTypeResolve, "1");

const char	*testExponentiationCornerCasesInt1 =
"auto x1 = 1, y1 = -1;\r\n\
auto x2 = 1, y2 = -127;\r\n\
auto x3 = 4, y3 = -1;\r\n\
auto x4 = 4, y4 = -5;\r\n\
auto x5 = 4, y5 = -127;\r\n\
auto x6 = 0, y6 = -3;\r\n\
\r\n\
return ((x1 ** y1) == 1) + ((x2 ** y2) == 1) * 10 + ((x3 ** y3) == 0) * 100 + ((x4 ** y4) == 0) * 1000 + ((x5 ** y5) == 0) * 10000 + ((x6 ** y6) == 0) * 100000;";
TEST_RESULT("Exponentiation corner cases 1 (int)", testExponentiationCornerCasesInt1, "111111");

const char	*testExponentiationCornerCasesInt2 =
"auto x1 = -1, y1 = -1;\r\n\
auto x2 = -1, y2 = -126;\r\n\
auto x3 = -4, y3 = -1;\r\n\
auto x4 = -4, y4 = -5;\r\n\
auto x5 = -4, y5 = -126;\r\n\
\r\n\
return ((x1 ** y1) == -1) + ((x2 ** y2) == 1) * 10 + ((x3 ** y3) == 0) * 100 + ((x4 ** y4) == 0) * 1000 + ((x5 ** y5) == 0) * 10000;";
TEST_RESULT("Exponentiation corner cases 2 (int)", testExponentiationCornerCasesInt2, "11111");

const char	*testExponentiationCornerCasesLong1 =
"auto x1 = 1l, y1 = -1l;\r\n\
auto x2 = 1l, y2 = -127l;\r\n\
auto x3 = 4l, y3 = -1l;\r\n\
auto x4 = 4l, y4 = -5l;\r\n\
auto x5 = 4l, y5 = -127l;\r\n\
auto x6 = 0, y6 = -3;\r\n\
\r\n\
return ((x1 ** y1) == 1) + ((x2 ** y2) == 1) * 10 + ((x3 ** y3) == 0) * 100 + ((x4 ** y4) == 0) * 1000 + ((x5 ** y5) == 0) * 10000 + ((x6 ** y6) == 0) * 100000;";
TEST_RESULT("Exponentiation corner cases 1 (long)", testExponentiationCornerCasesLong1, "111111");

const char	*testExponentiationCornerCasesLong2 =
"auto x1 = -1l, y1 = -1l;\r\n\
auto x2 = -1l, y2 = -126l;\r\n\
auto x3 = -4l, y3 = -1l;\r\n\
auto x4 = -4l, y4 = -5l;\r\n\
auto x5 = -4l, y5 = -126l;\r\n\
\r\n\
return ((x1 ** y1) == -1) + ((x2 ** y2) == 1) * 10 + ((x3 ** y3) == 0) * 100 + ((x4 ** y4) == 0) * 1000 + ((x5 ** y5) == 0) * 10000;";
TEST_RESULT("Exponentiation corner cases 2 (long)", testExponentiationCornerCasesLong2, "11111");
