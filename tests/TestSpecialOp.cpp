#include "TestBase.h"

const char	*testFunctionCompare =
"int ref() _f = int _self()\r\n\
{\r\n\
	return _f == _self;\r\n\
};\r\n\
return _f();";
TEST_RESULT("Function comparison", testFunctionCompare, "1");

const char	*testFunctionCompare2 =
"int ref() _f = int _self()\r\n\
{\r\n\
	return _f != _self;\r\n\
};\r\n\
return _f();";
TEST_RESULT("Function comparison 2", testFunctionCompare2, "0");

const char	*testAutoRefCompare1 =
"int sum = 0;\r\n\
auto ref a = nullptr, b = &sum;\r\n\
return a == b;";
TEST_RESULT("auto ref comparison 1", testAutoRefCompare1, "0");

const char	*testAutoRefCompare2 =
"int sum = 0;\r\n\
auto ref a = &sum, b = &sum;\r\n\
return a == b;";
TEST_RESULT("auto ref comparison 2", testAutoRefCompare2, "1");

const char	*testAutoRefCompare3 =
"int sum = 0;\r\n\
auto ref a = nullptr, b = &sum;\r\n\
return a != b;";
TEST_RESULT("auto ref comparison 3", testAutoRefCompare3, "1");

const char	*testAutoRefCompare4 =
"int sum = 0;\r\n\
auto ref a = &sum, b = &sum;\r\n\
return a != b;";
TEST_RESULT("auto ref comparison 4", testAutoRefCompare4, "0");

const char	*testAutoRefNot =
"auto ref a = nullptr;\r\n\
if(!a)\r\n\
	return 1;\r\n\
return 0;";
TEST_RESULT("unary not on auto ref 1", testAutoRefNot, "1");

const char	*testAutoRefNot2 =
"auto ref a = new int;\r\n\
if(!a)\r\n\
	return 1;\r\n\
return 0;";
TEST_RESULT("unary not on auto ref 2", testAutoRefNot2, "0");

const char	*testAutoRefCondition1 =
"auto ref a;\r\n\
while(a);\r\n\
return 1;";
TEST_RESULT("auto ref as a condition 1", testAutoRefCondition1, "1");

const char	*testAutoRefCondition2 =
"auto ref a;\r\n\
for(;a;){}\r\n\
return 1;";
TEST_RESULT("auto ref as a condition 2", testAutoRefCondition2, "1");

const char	*testAutoRefCondition3 =
"auto ref a;\r\n\
if(a){}\r\n\
return 1;";
TEST_RESULT("auto ref as a condition 3", testAutoRefCondition3, "1");

const char	*testAutoRefCondition4 =
"auto ref a;\r\n\
do{}while(a);\r\n\
return 1;";
TEST_RESULT("auto ref as a condition 4", testAutoRefCondition4, "1");

const char	*testPointerCompareNullptr1 = "int ref ref a; return a == nullptr;";
TEST_RESULT("pointer comparison with nullptr 1", testPointerCompareNullptr1, "1");

const char	*testPointerCompareNullptr2 = "int ref a = new int; return &a == nullptr;";
TEST_RESULT("pointer comparison with nullptr 2", testPointerCompareNullptr2, "0");

const char	*testPointerCompareNullptr3 = "int ref ref a; return nullptr == a;";
TEST_RESULT("pointer comparison with nullptr 3", testPointerCompareNullptr3, "1");

const char	*testPointerCompareNullptr4 = "int ref a = new int; return nullptr == &a;";
TEST_RESULT("pointer comparison with nullptr 4", testPointerCompareNullptr4, "0");

const char	*testArrayCompareNullptr1 = "int[] a; return a == nullptr;";
TEST_RESULT("array comparison with nullptr 1", testArrayCompareNullptr1, "1");

const char	*testArrayCompareNullptr2 = "int[] a = new int[2]; return a == nullptr;";
TEST_RESULT("array comparison with nullptr 2", testArrayCompareNullptr2, "0");

const char	*testArrayCompareNullptr3 = "int[] a; return nullptr == a;";
TEST_RESULT("array comparison with nullptr 3", testArrayCompareNullptr3, "1");

const char	*testArrayCompareNullptr4 = "int[] a = new int[2]; return nullptr == a;";
TEST_RESULT("array comparison with nullptr 4", testArrayCompareNullptr4, "0");

const char	*testFunctionCompareNullptr1 = "int ref() a; return a == nullptr;";
TEST_RESULT("function comparison with nullptr 1", testFunctionCompareNullptr1, "1");

const char	*testFunctionCompareNullptr2 = "int foo(){ return 1; } int ref() a = foo; return a == nullptr;";
TEST_RESULT("function comparison with nullptr 2", testFunctionCompareNullptr2, "0");

const char	*testFunctionCompareNullptr3 = "int ref() a; return nullptr == a;";
TEST_RESULT("function comparison with nullptr 3", testFunctionCompareNullptr3, "1");

const char	*testFunctionCompareNullptr4 = "int foo(){ return 1; } int ref() a = foo; return nullptr == a;";
TEST_RESULT("function comparison with nullptr 4", testFunctionCompareNullptr4, "0");

const char	*testFunctionAssignNullptr = 
"int foo(int x){ return -x; }\r\n\
auto yz = foo;\r\n\
yz = nullptr;\r\n\
return yz == nullptr;";
TEST_RESULT("function assignment with nullptr", testFunctionAssignNullptr, "1");
