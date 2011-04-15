#include "TestBase.h"

const char	*testPointers = 
"// Pointers!\r\n\
int testA(int ref v){ return *v * 5; }\r\n\
void testB(int ref v){ *v += 5; }\r\n\
int a = 5;\r\n\
int ref b = &a;\r\n\
int c = 2;\r\n\
c = *b;\r\n\
*b = 14;\r\n\
(*b)++;\r\n\
*b *= 4;\r\n\
testB(b);\r\n\
return testA(&a);";
TEST("Pointers", testPointers, "325")
{
	CHECK_INT("a", 0, 65);
	CHECK_INT("c", 0, 5);
}

const char	*testPointersCmplx = 
"// Pointers on complex!\r\n\
import std.math;\r\n\
double length(float4 ref v)\r\n\
{\r\n\
	return sqrt((v.x*v.x)+(v.y*v.y)+(v.z*v.z));\r\n\
}\r\n\
void normalize(float4 ref v)\r\n\
{\r\n\
	double len = length(v);\r\n\
	v.x /= len; v.y /= len; v.z /= len;\r\n\
}\r\n\
float4 a;\r\n\
a.x = 12.0;\r\n\
a.y = 4.0;\r\n\
a.z = 3.0;\r\n\
a.w = 1.0;\r\n\
float4 ref b = &a;\r\n\
normalize(&a);\r\n\
return length(b);";
TEST("Pointers on complex", testPointersCmplx, "1.000000")
{
	CHECK_FLOAT("a", 0, 12.0f/13.0f);
	CHECK_FLOAT("a", 1, 4.0f/13.0f);
	CHECK_FLOAT("a", 2, 3.0f/13.0f);
	CHECK_FLOAT("a", 3, 1.0f);
}

const char	*testPointersCmplx2 = 
"import std.math;\r\n\
float4 a;\r\n\
float4 ref b = &a;\r\n\
b.x = 5.0f;\r\n\
return b.x;";
TEST("Pointers on complex 2", testPointersCmplx2, "5.000000")
{
	CHECK_FLOAT("a", 0, 5.0);
}

const char	*testPointers2 = 
"import std.math;\r\n\
double testA(float4 ref v){ return v.x; }\r\n\
float4 a;\r\n\
float4 ref b = &a;\r\n\
a.x = 5.0f;\r\n\
return testA(b);";
TEST("Pointers 2", testPointers2, "5.000000")
{
	CHECK_FLOAT("a", 0, 5.0);
}

const char	*testPointers3 = 
"import std.math;\r\n\
int[5] arr;\r\n\
float4[4] arrF;\r\n\
int ref a = &arr[3];\r\n\
*a = 55;\r\n\
float4 ref b = &arrF[1];\r\n\
b.x = 85;\r\n\
float ref c = &arrF[1].y;\r\n\
*c = 125;\r\n\
{\r\n\
	int ref a = &arr[1];\r\n\
	*a = 5;\r\n\
	float4 ref b = &arrF[2];\r\n\
	b.x = 8;\r\n\
	float ref c = &arrF[2].y;\r\n\
	*c = 12;\r\n\
}\r\n\
return 1;";
TEST("Pointers test 3", testPointers3, "1")
{
	CHECK_INT("arr", 1, 5);
	CHECK_INT("arr", 3, 55);

	CHECK_FLOAT("arrF", 4, 85.0);
	CHECK_FLOAT("arrF", 5, 125.0);

	CHECK_FLOAT("arrF", 8, 8.0);
	CHECK_FLOAT("arrF", 9, 12.0);
}

const char *testCorrectDereference =
"int ref a = new int(6);\r\n\
int ref ref x = &a;\r\n\
int ref y = *x;\r\n\
return *y;";
TEST_RESULT("Correct dereference test", testCorrectDereference, "6");

const char *testCorrectDereference2 =
"import std.vector;\r\n\
vector<int ref> goals;\r\n\
goals.push_back(new int(5));\r\n\
auto x = goals.back();\r\n\
auto y = *x;\r\n\
auto z = *goals.back();\r\n\
return y == z;";
TEST_RESULT("Correct dereference test 2", testCorrectDereference2, "1");

const char *testCorrectDereference3 =
"import std.vector;\r\n\
vector<int ref> goals;\r\n\
goals.push_back(new int(5));\r\n\
auto x = goals.back();\r\n\
auto y = *x;\r\n\
auto z = *(goals.back());\r\n\
return y == z;";
TEST_RESULT("Correct dereference test 3", testCorrectDereference3, "1");

const char *testSideEffectPreservation =
"class X{} X x;\r\n\
int i;\r\n\
X ref foo(){ i = 5; return &x; }\r\n\
X a = *foo();\r\n\
return i;";
TEST_RESULT("Preservation of side effects when dereferencing a 0-byte class pointer", testSideEffectPreservation, "5");
