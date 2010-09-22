#include "TestBase.h"

LOAD_MODULE(test_A, "test.a", "import std.math; float4 a; a.x = 2;");

const char	*testGlobalVariablePositioning =
"import test.a;\r\n\
float4 b;\r\n\
b.x = 12;\r\n\
return int(a.x);";
TEST_RESULT("Global variable positioning", testGlobalVariablePositioning, "2");

const char	*testGlobalVariablePositioning2 =
"import test.a;\r\n\
import std.math;\r\n\
float4 b;\r\n\
b.x = 12;\r\n\
return int(a.x);";
TEST_RESULT("Global variable positioning 2", testGlobalVariablePositioning2, "2");

LOAD_MODULE(test_Imporhide, "test.importhide", "char[] arr2 = \" world\";{ int r = 5; }");
const char	*testImportHidding =
"import test.importhide;\r\n\
char[] arr = \"hello\";\r\n\
char[] r = arr + arr2;\r\n\
return r.size;";
TEST_RESULT("Hidden variable exclusion from import", testImportHidding, "12");

#if defined(NULLC_AUTOBINDING) && !defined(NULLC_ENABLE_C_TRANSLATION)

NULLC_BIND int myFoo(int x){ return x + 15; }

const char	*testFunctionAutobinding =
"int myFoo(int x);\r\n\
return myFoo(5);";
TEST_RESULT("Automatic function binding", testFunctionAutobinding, "20");

LOAD_MODULE(test_Autobind, "test.autobind", "int myFoo(int x);");
const char	*testFunctionAutobinding2 =
"import test.autobind;\r\n\
return myFoo(2);";
TEST_RESULT("Automatic function binding 2", testFunctionAutobinding2, "17");

#endif
