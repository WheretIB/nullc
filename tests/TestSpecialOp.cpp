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
