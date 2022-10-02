#include "TestBase.h"

const char	*testFunctionTypeNew =
"import std.range;\r\n\
\r\n\
auto a = { 1, 2, 3, 4, 5 };\r\n\
auto b = new int ref()[a.size];\r\n\
for(i in a, n in range(0, 4))\r\n\
	b[n] = auto(){ return a[n]; };\r\n\
\r\n\
return b[0]() + b[1]() + b[4]();";
TEST_RESULT("new call with function type", testFunctionTypeNew, "8");

const char	*testTypeOfNew =
"import std.range;\r\n\
int test(){ return 0; }\r\n\
auto a = { 1, 2, 3, 4, 5 };\r\n\
auto b = new typeof(test)[a.size];\r\n\
for(i in a, n in range(0, 4))\r\n\
	b[n] = auto(){ return a[n]; };\r\n\
\r\n\
return b[0]() + b[1]() + b[4]();";
TEST_RESULT("new call with typeof", testTypeOfNew, "8");

const char	*testConstructorAfterNew =
"void int::int(int x, y){ *this = x; }\r\n\
int ref a = new int(5, 0);\r\n\
int foo(int ref x){ return *x; }\r\n\
return *a + foo(new int(30, 0));";
TEST_RESULT("Type constructor after new.", testConstructorAfterNew, "35");

const char	*testConstructorAfterNew2 = "class Test{ int x; } void Test::Test(int x){ this.x = x; } return (new Test(4)).x;";
TEST_RESULT("Type constructor after new 2.", testConstructorAfterNew2, "4");

const char	*testConstructorAfterNew3 = "class Test{ int x; } void Test::Test(){ } return (new Test()).x;";
TEST_RESULT("Type constructor after new 3.", testConstructorAfterNew3, "0");

const char	*testTypeChangeInArrayExpression = "int[] arr = new int[(int f(){ return 5; })()]; return arr.size;";
TEST_RESULT("Type change in array size in new expression", testTypeChangeInArrayExpression, "5");

const char	*testArrayTypeConstruction1 = "auto x = new (int[]); return typeof(x) == int[] ref;";
TEST_RESULT("Array type allocation 1", testArrayTypeConstruction1, "1");

const char	*testArrayTypeConstruction2 = "auto x = new (int[])[2]; return typeof(x) == int[][] && x.size == 2;";
TEST_RESULT("Array type allocation 2", testArrayTypeConstruction2, "1");

const char	*testArrayTypeConstruction3 = "auto x = new (int[][]); return typeof(x) == int[][] ref;";
TEST_RESULT("Array type allocation 3", testArrayTypeConstruction3, "1");

const char	*testArrayTypeConstruction4 = "auto x = new (int[][])(); return typeof(x) == int[][] ref;";
TEST_RESULT("Array type allocation 4", testArrayTypeConstruction4, "1");

const char	*testArrayTypeConstruction5 = "auto x = new (int[2][2]); return typeof(x) == int[2][2] ref;";
TEST_RESULT("Array type allocation 5", testArrayTypeConstruction5, "1");

const char	*testDerivedTypeValueConstruction1 = "auto a = { 1, 2 }; auto ay = new (int[2])(a); return ay[0] + ay[1];";
TEST_RESULT("Value construction of a derived type 1", testDerivedTypeValueConstruction1, "3");

const char	*testDerivedTypeValueConstruction2 = "auto by = new (int[2])({ 3, 4 }); return by[0] + by[1];";
TEST_RESULT("Value construction of a derived type 2", testDerivedTypeValueConstruction2, "7");

const char	*testDerivedTypeValueConstruction3 = "auto cay = new (int[])({ 1, 2 }); return cay[0] + cay[1];";
TEST_RESULT("Value construction of a derived type 3", testDerivedTypeValueConstruction3, "3");

const char	*testDerivedTypeValueConstruction4 = "auto x = new int(5); auto dy = new int ref(x); return * *dy;";
TEST_RESULT("Value construction of a derived type 4", testDerivedTypeValueConstruction4, "5");

const char	*testDerivedTypeValueConstruction5 = "auto fy = new int ref()(auto(){ return 14; }); return (*fy)();";
TEST_RESULT("Value construction of a derived type 5", testDerivedTypeValueConstruction5, "14");

const char	*testDerivedTypeValueConstruction6 = "auto fy = new int ref(int)(auto(@T x){ return x + 4; }); return (*fy)(4);";
TEST_RESULT("Value construction of a derived type 6", testDerivedTypeValueConstruction6, "8");

const char	*testDerivedTypeCustomConstruction1 = "auto x = new (int ref)(){ *this = new int(5); }; return * *x;";
TEST_RESULT("Custom construction of a derived type 1", testDerivedTypeCustomConstruction1, "5");

const char	*testDerivedTypeCustomConstruction2 = "auto cy = new (int[2])(){ this[0] = 5; this[1] = 6; }; return cy[0] + cy[1];";
TEST_RESULT("Custom construction of a derived type 2", testDerivedTypeCustomConstruction2, "11");

const char	*testDerivedTypeCustomConstruction3 = "auto cby = new (int[])({ 1, 2, 3 }){ this[1] = 5; }; return cby[0] + cby[1] + cby[2];";
TEST_RESULT("Custom construction of a derived type 3", testDerivedTypeCustomConstruction3, "9");

const char	*testDerivedTypeCustomConstruction4 = "auto ccy = new (int[])({ 1, 2, 3 }){ for(i in *this) i *= 10; }; return ccy[0] + ccy[1] + ccy[2];";
TEST_RESULT("Custom construction of a derived type 4", testDerivedTypeCustomConstruction4, "60");
