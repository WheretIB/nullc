#include "TestBase.h"

const char	*testNamedArgumentCall1 = 
"int foo(int i = 4, j){ return i + j; } return foo(j: 5);";
TEST_RESULT("Function call with named arguments 1", testNamedArgumentCall1, "9");

const char	*testNamedArgumentCall2 = 
"int foo(int i, j){ return i / j; } return foo(i: 10, j: 5);";
TEST_RESULT("Function call with named arguments 2", testNamedArgumentCall2, "2");

const char	*testNamedArgumentCall3 = 
"int foo(int i, j){ return i / j; } return foo(j: 5, i: 10);";
TEST_RESULT("Function call with named arguments 3", testNamedArgumentCall3, "2");

const char	*testNamedArgumentCall4 = 
"int foo(int i, j){ return i / j; } int foo(int k, l, m){ return k + l + m; } return foo(j: 5, i: 10);";
TEST_RESULT("Function call with named arguments 4", testNamedArgumentCall4, "2");

const char	*testNamedArgumentCall5 = 
"int foo(int i, j){ return i / j; } int foo(int i, j, k){ return i + j + k; } return foo(j: 5, i: 10);";
TEST_RESULT("Function call with named arguments 5", testNamedArgumentCall5, "2");

const char	*testNamedArgumentCall6 = 
"int foo(int i = 10, j){ return i / j; } int foo(int i, j, k = 5){ return i + j + k; } return foo(j: 5);";
TEST_RESULT("Function call with named arguments 6", testNamedArgumentCall6, "2");

const char	*testNamedArgumentCall7 = 
"auto foo(generic i, j){ return i / j; } return foo(j: 5, i: 10);";
TEST_RESULT("Function call with named arguments 7", testNamedArgumentCall7, "2");

const char	*testNamedArgumentCall8 = 
"auto foo(generic i, j){ return i / j; } return foo(j: 5, i: 10) + foo(i: 20, j: 4);";
TEST_RESULT("Function call with named arguments 8", testNamedArgumentCall8, "7");

const char	*testNamedArgumentCall9 = 
"class Bar{} int operator[](Bar ref a, int x, int y){ return x / y; } Bar n; return n[x:10, y:5];";
TEST_RESULT("Function call with named arguments 9", testNamedArgumentCall9, "2");

const char	*testNamedArgumentCall10 = 
"class Bar{} int operator[](Bar ref a, int x, int y){ return x / y; } Bar n; return n[y:5, x:10];";
TEST_RESULT("Function call with named arguments 10", testNamedArgumentCall10, "2");

const char	*testNamedArgumentCall11 = 
"int foo(int i, j){ return i / j; } return foo(10, j: 5);";
TEST_RESULT("Function call with named arguments 11", testNamedArgumentCall11, "2");

LOAD_MODULE(test_importnamedcall1, "test.namedcall", "int foo(int i, j){ return i / j; }");
const char	*testNamedArgumentCall12 =
"import test.namedcall; return foo(j: 5, i: 10);";
TEST_RESULT("Function call with named arguments 12 (modules)", testNamedArgumentCall12, "2");
