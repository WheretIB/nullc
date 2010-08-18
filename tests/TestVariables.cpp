#include "TestBase.h"

const char	*testVarGetSet1 = 
"import std.math;\r\n\
int[10] a=4;\r\n\
int[] b = a;\r\n\
float4 c;\r\n\
c.y = 5.0f;\r\n\
c.z = c.w;\r\n\
b[8] = 4;\r\n\
int t1 = b[8];\r\n\
int t2 = b.size;\r\n\
\r\n\
for(int i = 0; i < 3; i++)\r\n\
  b[5+i] += 3;\r\n\
b[3]++;\r\n\
b[6] = ++b[9];\r\n\
b[2] = b[8]++;\r\n\
b[b[8]]++;\r\n\
b[b[2]] = b[b[8]]++;\r\n\
b[4]--;\r\n\
return b[1];";
TEST("Variable get and set", testVarGetSet1, "4")
{
	int val[] = { 4, 4, 4, 5, 7, 9, 5, 7, 5, 5, };
	for(int i = 0; i < 10; i++)
		CHECK_INT("a", i, val[i]);
	CHECK_FLOAT("c", 1, 5.0f);

	CHECK_INT("t1", 0, 4);
	CHECK_INT("t2", 0, 10);
}

const char	*testGetSet = 
"// New get and set functions test\r\n\
import std.math;\r\n\
auto s1 = 5;\r\n\
auto f1 = &s1;\r\n\
int d1 = 3;\r\n\
int ref df1 = &d1;\r\n\
\r\n\
auto k = { 1,4,8,64,7};\r\n\
auto kr = { &k[4], &k[3], &k[2], &k[1], &k[0] };\r\n\
auto krr = &kr;\r\n\
auto t1 = 5;\r\n\
float4 t2;\r\n\
t2.x = 6;\r\n\
t2.y = 4;\r\n\
float4[4] t3;\r\n\
t3[1].x = 5;\r\n\
t3[2].z = 14.0;\r\n\
auto t4 = &t1;\r\n\
auto t5 = &t2;\r\n\
auto t6 = &t3;\r\n\
float4 ref[4] t7;\r\n\
t7[0] = &t3[1];\r\n\
class Test{ int ref x, y; }\r\n\
Test t8;\r\n\
t8.x = &s1;\r\n\
t8.y = &t1;\r\n\
Test[3] t9;\r\n\
t9[0].x = &s1;t9[0].y = &s1;t9[1].x = &s1;t9[1].y = &s1;t9[2].x = &s1;\r\n\
t9[2].y = &t1;\r\n\
auto t10 = &t8;\r\n\
\r\n\
float er2, er1 = 2 * (er2 = t2.y);\r\n\
//gets\r\n\
int r1 = t1;	// 5\r\n\
int r2 = k[2];	// 8\r\n\
float r3s, r3 = r3s = t2.y;	// 4\r\n\
float r4 = t3[2].z;	// 14\r\n\
int r5 = *t4;	// 5\r\n\
int r6 = *kr[2];	// 8\r\n\
float r7 = t5.y;	// 4\r\n\
int r8 = *t8.y;	// 5\r\n\
int r9 = *t9[2].y;	// 5\r\n\
//float r10 = (*t7)[2].z;\r\n\
int r11 = *t10.y; // 5\r\n\
float4 t11 = *t7[0];\r\n\
\r\n\
//sets in reverse\r\n\
t1 = r1;\r\n\
k[2] = r2;\r\n\
t2.y = r3;\r\n\
t3[2].z = r4;\r\n\
*t4 = r5;\r\n\
*kr[2] = r6;\r\n\
t5.y = r7;\r\n\
*t8.y = r8;\r\n\
*t9[2].y = r9;\r\n\
*t10.y = r11;\r\n\
*t7[0] = t11;\r\n\
\r\n\
int i = 1, j = 2;\r\n\
auto spec = {{1,2,3},{4,5,6},{7,8,9}};\r\n\
int r2a = spec[2][1]; // 8\r\n\
int r2b = spec[j][1]; // 8\r\n\
int r2c = spec[1][j]; // 6\r\n\
int r2d = spec[i][j]; // 6\r\n\
{\r\n\
auto d2 = *f1;\r\n\
auto d3 = f1;\r\n\
d1 = d2;\r\n\
df1 = d3;\r\n\
}\r\n\
return 1;";
TEST("New get and set functions test", testGetSet, "1")
{
	CHECK_INT("s1", 0, 5);
	CHECK_INT("d1", 0, 5);

	CHECK_INT("k", 0, 1);
	CHECK_INT("k", 1, 4);
	CHECK_INT("k", 2, 8);
	CHECK_INT("k", 3, 64);
	CHECK_INT("k", 4, 7);

	CHECK_INT("t1", 0, 5);

	CHECK_FLOAT("t2", 0, 6.0f);
	CHECK_FLOAT("t2", 1, 4.0f);

	CHECK_FLOAT("t3", 4, 5.0f);
	CHECK_FLOAT("t3", 10, 14.0f);

	CHECK_FLOAT("er2", 0, 4.0f);
	CHECK_FLOAT("er1", 0, 8.0f);

	CHECK_INT("r1", 0, 5);
	CHECK_INT("r2", 0, 8);

	CHECK_FLOAT("r3s", 0, 4.0f);
	CHECK_FLOAT("r3", 0, 4.0f);
	CHECK_FLOAT("r4", 0, 14.0f);

	CHECK_INT("r5", 0, 5);
	CHECK_INT("r6", 0, 8);

	CHECK_FLOAT("r7", 0, 4.0f);

	CHECK_INT("r8", 0, 5);
	CHECK_INT("r9", 0, 5);
	CHECK_INT("r11", 0, 5);

	CHECK_FLOAT("t11", 0, 5.0f);

	CHECK_INT("i", 0, 1);
	CHECK_INT("j", 0, 2);

	CHECK_INT("spec", 0, 1);
	CHECK_INT("spec", 1, 2);
	CHECK_INT("spec", 2, 3);
	CHECK_INT("spec", 3, 4);
	CHECK_INT("spec", 4, 5);
	CHECK_INT("spec", 5, 6);
	CHECK_INT("spec", 6, 7);
	CHECK_INT("spec", 7, 8);
	CHECK_INT("spec", 8, 9);

	CHECK_INT("r2a", 0, 8);
	CHECK_INT("r2b", 0, 8);
	CHECK_INT("r2c", 0, 6);
	CHECK_INT("r2d", 0, 6);
}

const char	*testArrayMemberAfterCall =
"import std.math;\r\n\
float2[2] f(){ return { float2(12,13), float2(14,15) }; }\r\n\
int x = (f())[0].x;\r\n\
int y = float2(45, 98).y;\r\n\
return int(f()[1].y);";
TEST("Array and class member access after function call", testArrayMemberAfterCall, "15")
{
	CHECK_INT("x", 0, 12);
	CHECK_INT("y", 0, 98);
}

