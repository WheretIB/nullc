#include "TestBase.h"

const char	*testShortArrayDefinition =
"short[4] a = { 1, 2, 3, 4 };\r\n\
short[4] b;\r\n\
b = { 1, 2, 3, 4 };\r\n\
short[] c = { 1, 2, 3, 4 };\r\n\
int sumA = 0, sumB = 0, sumC = 0;\r\n\
for(iA in a, iB in b, iC in c)\r\n\
{\r\n\
	sumA += iA;\r\n\
	sumB += iB;\r\n\
	sumC += iC;\r\n\
}\r\n\
return 0;";
TEST("Array of type short definition", testShortArrayDefinition, "0")
{
	CHECK_INT("sumA", 0, 10, lastFailed);
	CHECK_INT("sumB", 0, 10, lastFailed);
	CHECK_INT("sumC", 0, 10, lastFailed);
}

const char	*testFloatArrayDefinition =
"float[4] a = { 3.0, 2, 4, 3 };\r\n\
float[4] b;\r\n\
b = { 3.0, 2, 4, 3 };\r\n\
float[] c = { 3.0, 2, 4, 3 };\r\n\
int sumA = 0, sumB = 0, sumC = 0;\r\n\
for(iA in a, iB in b, iC in c)\r\n\
{\r\n\
	sumA += iA;\r\n\
	sumB += iB;\r\n\
	sumC += iC;\r\n\
}\r\n\
return 0;";
TEST("Array of type float definition", testFloatArrayDefinition, "0")
{
	CHECK_INT("sumA", 0, 12, lastFailed);
	CHECK_INT("sumB", 0, 12, lastFailed);
	CHECK_INT("sumC", 0, 12, lastFailed);
}

const char	*testCharArrayDefinition =
"char[6] a = { 'h', 'e', 'l', 'l', 'o', '\\0' };\r\n\
char[6] b;\r\n\
b = { 'h', 'e', 'l', 'l', 'o', '\\0' };\r\n\
char[] c = { 'w', 'o', 'r', 'l', 'd', '\\0' };\r\n\
int sumA = 0, sumB = 0, sumC = 0;\r\n\
for(iA in a, iB in b, iC in c)\r\n\
{\r\n\
	sumA += iA;\r\n\
	sumB += iB;\r\n\
	sumC += iC;\r\n\
}\r\n\
return 0;";
TEST("Array of type char definition", testCharArrayDefinition, "0")
{
	CHECK_INT("sumA", 0, 532, lastFailed);
	CHECK_INT("sumB", 0, 532, lastFailed);
	CHECK_INT("sumC", 0, 552, lastFailed);
}

const char	*testBoolArrayDefinition = "auto x = { true, true }; return x.size;";
TEST_RESULT("Array of type bool definition", testBoolArrayDefinition, "2");

const char	*testInplaceArrayDouble =
"double[] arr = { 12.0, 14, 18.0f };\r\n\
return int(arr[0]+arr[1]+arr[2]);";
TEST_RESULT("Inplace double array with integer elements", testInplaceArrayDouble, "44");

const char	*testArrayConditionalCommonType =
"auto arr = sizeof(int) == 4 ? \"four\" : \"eight\";\r\n\
return arr.size == (\"four\").size;";
TEST_RESULT("Unsized array is selected betweeen arrays of different length", testArrayConditionalCommonType, "1");

const char	*testArrayReset =
"char[] test = \"hello\";\r\n\
test = nullptr;\r\n\
return test == nullptr;";
TEST_RESULT("Assignment of nullptr to unsized array", testArrayReset, "1");
