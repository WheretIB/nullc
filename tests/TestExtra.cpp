#include "TestBase.h"

#ifndef _MSC_VER
	#include <stdint.h>
#endif

const char	*testMissingTests = 
"long a1 = 01, a2 = 0377, a3 = 01777777, a4 = 017777777777777777;\r\n\
long b1 = 0b, b2 = 1b, b3 = 1111111111100001010100101b, b4 = 101010101101010101011010101010011101011010010101110101010101001b;\r\n\
\r\n\
double c1 = 1e-3, c2 = 1e6, c3=123e2, c4=0.121e-4;\r\n\
\r\n\
char[] d1 = \"\\\\\\\'\\0\\\"\";\r\n\
int d1size = d1.size;\r\n\
\r\n\
int d2 = !4, d3 = ~5, d4 = -12;\r\n\
float e2 = -1.0f;\r\n\
double e3 = -3.0;\r\n\
long e4 = !324324234324234423l, e5 = ~89435763476541l, e6 = -1687313675313735l;\r\n\
int f1 = 2 << 4;\r\n\
long f2 = 3l << 12l;\r\n\
\r\n\
int f4 = 0 - f1;\r\n\
double f5 = 2 * 3.0, f6 = f1 - 0.0;\r\n\
\r\n\
return 1;";
TEST("Group of tests", testMissingTests, "1")
{
	CHECK_LONG("a1", 0, 1);

	CHECK_LONG("a2", 0, 255);
	CHECK_LONG("a3", 0, 524287);
	CHECK_LONG("a4", 0, 562949953421311ll);

	CHECK_LONG("b1", 0, 0);
	CHECK_LONG("b2", 0, 1);
	CHECK_LONG("b3", 0, 33538725);
	CHECK_LONG("b4", 0, 6154922420991617705ll);

	CHECK_DOUBLE("c1", 0, 1e-3);
	CHECK_DOUBLE("c2", 0, 1e6);
	CHECK_DOUBLE("c3", 0, 123e2);
	CHECK_DOUBLE("c4", 0, 0.121e-4);

	CHECK_INT("d1size", 0, 5);

	CHECK_CHAR("$temp1", 0, '\\');
	CHECK_CHAR("$temp1", 1, '\'');
	CHECK_CHAR("$temp1", 2, 0);
	CHECK_CHAR("$temp1", 3, '\"');
	CHECK_CHAR("$temp1", 4, 0);

	CHECK_INT("d2", 0, !4);
	CHECK_INT("d3", 0, ~5);
	CHECK_INT("d4", 0, -12);

	CHECK_FLOAT("e2", 0, -1.0f);
	CHECK_DOUBLE("e3", 0, -3.0);

	CHECK_LONG("e4", 0, !324324234324234423ll);
	CHECK_LONG("e5", 0, ~89435763476541ll);
	CHECK_LONG("e6", 0, -1687313675313735ll);

	CHECK_INT("f1", 0, 2 << 4);
	CHECK_LONG("f2", 0, 3ll << 12ll);

	CHECK_INT("f4", 0, -(2 << 4));
	CHECK_DOUBLE("f5", 0, 6.0);
	CHECK_DOUBLE("f6", 0, (2 << 4));
}

const char	*testMissingTests2 =
"auto a = {1,2,3,4};\r\n\
int[] b = a;\r\n\
int[] ref c = &b;\r\n\
int d = c[2];\r\n\
int e = a.size;\r\n\
\r\n\
int[] f = *c;\r\n\
int[4] ref g = &a;\r\n\
int[] h = *g;\r\n\
int j = g.size;\r\n\
\r\n\
auto n1 = auto(int a){ return -a; };\r\n\
auto n2 = auto(){}, n4 = int ff(){ return 0; };\r\n\
int ref(int) n1_ = n1;\r\n\
void ref() n2_ = n2;\r\n\
int ref(int) n11 = int n11f(int a){ return ~a; }, n33;\r\n\
void ref() n22 = void n22f(){}, n44;\r\n\
int k1 = n1(5), k2 = n1_(12), k3 = n11(7);\r\n\
n33 = auto(int n){ return n*1024; };\r\n\
n44 = auto(){};\r\n\
int k4 = n33(4);\r\n\
n44();\r\n\
return 1;";
TEST("Group of tests 2", testMissingTests2, "1")
{
	CHECK_INT("d", 0, 3);
	CHECK_INT("e", 0, 4);

	CHECK_INT("k1", 0, -5);
	CHECK_INT("k2", 0, -12);
	CHECK_INT("k3", 0, ~7);
	CHECK_INT("k4", 0, 4096);
}

const char	*testMissingTests3 =
"int max(int a, b, c){ return a > b ? (a > c ? a : c) : (b > c ? b : c); }\r\n\
int max(int a, b){ if(a > b) return a; else return b; }\r\n\
int max2(int a, b, c){ if(a > b) return max(a, c); else return max(b, c); }\r\n\
int a1 = max(1, 2, 3), a2 = max(1, 0, 2), a3 = max(7, 6, 5);\r\n\
int b1 = max2(1, 2, 3), b2 = max2(1, 0, 2), b3 = max2(7, 6, 5);\r\n\
\r\n\
int c1 = 1;\r\n\
do{\r\n\
c1 *= 2;\r\n\
}while(c1 < 500);\r\n\
\r\n\
class A{ char x, y; }\r\n\
\r\n\
int opt1 = ((12 / 3) < 8) != ((5 >= 7) == (5 <= 3)) + (18 >> 2) % (4 & 4 ^ 0 | 0xffff) + (5 && 1 || 3 ^^ 2);\r\n\
long opt2 = 18l >> 2l % 5l ^ 9l | 12l & 13l;\r\n\
long opt3 = 1l && 1l || 1l ^^ 1l;\r\n\
double opt4 = 8.0 % 3.0;\r\n\
return 1;";
TEST("Group of tests 3", testMissingTests3, "1")
{
	CHECK_INT("a1", 0, 3);
	CHECK_INT("a2", 0, 2);
	CHECK_INT("a3", 0, 7);

	CHECK_INT("b1", 0, 3);
	CHECK_INT("b2", 0, 2);
	CHECK_INT("b3", 0, 7);

	CHECK_INT("c1", 0, 512);

	CHECK_INT("opt1", 0, (1 != 1 + (18 >> 2) % (4 & 4 ^ 0 | 0xffff) + 1) ? 1 : 0);
	CHECK_LONG("opt2", 0, 18l >> 2l % 5l ^ 9l | 12l & 13l);
	CHECK_LONG("opt3", 0, 1);

	CHECK_DOUBLE("opt4", 0, 2.0);
}

const char	*testMissingTests4 =
"class matrix\r\n\
{\r\n\
    float[] arr;\r\n\
    int width, height;\r\n\
}\r\n\
matrix matrix(int width, height){ matrix ret; ret.arr = new float[width * height]; ret.width = width; ret.height = height; return ret; }\r\n\
class row\r\n\
{\r\n\
    matrix ref parent;\r\n\
    int x;\r\n\
}\r\n\
row operator[](matrix ref a, int x){ row ret; ret.x = x; ret.parent = a; return ret; }\r\n\
float ref operator[](row a, int y){ return &a.parent.arr[y * a.parent.width + a.x]; }\r\n\
\r\n\
matrix m = matrix(4, 4);\r\n\
m[1][3] = 4;\r\n\
\r\n\
int operator+(float a, int n){ return int(a) + n; }\r\n\
\r\n\
int min(int a, b){ return a < b ? a : b; }\r\n\
int ref rsize = new int;\r\n\
int[] operator+(int[] a, b){ int[] res = new int[min(a.size, b.size)]; for(int i = 0; i < res.size; i++) res[i] = a[i] + b[i]; *rsize = res.size; return res; }\r\n\
auto k = { 1, 2, 3, 4 } + { 4, 2, 5, 3 };\r\n\
int[] pass(int[] a){ return a; }\r\n\
auto i = pass({ 4, 7 });\r\n\
int ksize = k.size; int isize = i.size;\r\n\
int k1 = k[0], k2 = k[1], k3 = k[2], k4 = k[3];\r\n\
return m[1][3] + 2;";
TEST("Group of tests 4", testMissingTests4, "6")
{
	CHECK_INT("$temp1", 0, 1);
	CHECK_INT("$temp1", 1, 2);
	CHECK_INT("$temp1", 2, 3);
	CHECK_INT("$temp1", 3, 4);

	CHECK_INT("$temp2", 0, 4);
	CHECK_INT("$temp2", 1, 2);
	CHECK_INT("$temp2", 2, 5);
	CHECK_INT("$temp2", 3, 3);

	CHECK_INT("$temp3", 0, 4);
	CHECK_INT("$temp3", 1, 7);

	CHECK_INT("ksize", 0, 4);
	CHECK_INT("isize", 0, 2);

	CHECK_INT("k1", 0, 5);
	CHECK_INT("k2", 0, 4);
	CHECK_INT("k3", 0, 8);
	CHECK_INT("k4", 0, 7);
}

int CheckAlignment(NULLCRef ptr, int alignment)
{
	intptr_t asInt = (intptr_t)ptr.ptr;
	return asInt % alignment == 0;
}

LOAD_MODULE_BIND(test_alignment, "test.alignment", "int CheckAlignment(auto ref ptr, int alignment);")
{
	nullcBindModuleFunction("test.alignment", (void(*)())CheckAlignment, "CheckAlignment", 0);
}

const char	*testMissingTests5a =
"import test.alignment;\r\n\
long a = 3, a2 = 1;\r\n\
long b1 = a ** 27;\r\n\
long b2 = a2 ** 16884;\r\n\
long b3 = a ** -0;\r\n\
long b4 = a ** -1;\r\n\
char f;\r\n\
int k(){ align(16) char a; return CheckAlignment(&a, 16); }\r\n\
int r(){ align(16) char a; return k(); }\r\n\
int good = r();\r\n\
int k2(){ align(16) char a; return CheckAlignment(&a, 16); }\r\n\
int r2(){ align(16) char a; return k2(); }\r\n\
good += r2();\r\n\
return good;";
TEST("Group of tests 5", testMissingTests5a, "2")
{
	CHECK_LONG("b1", 0, 7625597484987ll);
	CHECK_LONG("b2", 0, 1);
	CHECK_LONG("b3", 0, 1);
	CHECK_LONG("b4", 0, 0);
}

const char	*testStaticIf1 =
"int a = 4;\r\n\
@if(typeof(a) != int)\r\n\
{\r\n\
	int[-1] f;\r\n\
}\r\n\
return a;";
TEST_RESULT("Static if test 1", testStaticIf1, "4");

const char	*testStaticIf2 =
"int a = 4;\r\n\
@if(typeof(a) == int)\r\n\
{\r\n\
	a = 5;\r\n\
}else{\r\n\
	int[-1] f;\r\n\
}\r\n\
return a;";
TEST_RESULT("Static if test 2", testStaticIf2, "5");

const char	*testStaticIf3 =
"int a = 4;\r\n\
@if(typeof(a) != int)\r\n\
{\r\n\
	int[-1] f;\r\n\
}else{\r\n\
	 a = 5;\r\n\
}\r\n\
return a;";
TEST_RESULT("Static if test 3", testStaticIf3, "5");

const char	*testStaticIf4 =
"auto foo(generic a)\r\n\
{\r\n\
	@if(typeof(a) == int)\r\n\
		return 1;\r\n\
	return 0;\r\n\
}\r\n\
\r\n\
auto bar(generic a)\r\n\
{\r\n\
	@if(typeof(a) == int)\r\n\
		return 1;\r\n\
	else\r\n\
		return 0;\r\n\
}\r\n\
\r\n\
int a = foo(4);\r\n\
int b = foo(4.0f);\r\n\
int c = bar(3);\r\n\
int d = bar(4l);\r\n\
return 1;";
TEST("Static if test 4", testStaticIf4, "1")
{
	CHECK_INT("a", 0, 1);
	CHECK_INT("b", 0, 0);
	CHECK_INT("c", 0, 1);
	CHECK_INT("d", 0, 0);
}
