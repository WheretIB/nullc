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

LOAD_MODULE(test_importtypedef, "test.importtypedef", "class Foo_{ int bar; } typedef Foo_ ref Foo; Foo Foo(int y){ auto x = new Foo_; x.bar = y; return x; }");
const char	*testImportTypedef =
"import test.importtypedef;\r\n\
Foo x = Foo(23);\r\n\
return x.bar;";
TEST_RESULT("Typedef import", testImportTypedef, "23");

LOAD_MODULE(test_genericX1, "test.genericX1", "auto foo(generic x){ return -x; }");
LOAD_MODULE(test_genericX2, "test.genericX2", "import test.genericX1; double foo(double a){ return 0.0; } auto bar(generic x){ foo(1.0); return 2*x; }");
const char	*testGenericImportX =
"import test.genericX1;\r\n\
import test.genericX2;\r\n\
return bar(3) * foo(4) + foo(4) * bar(3);";
TEST_RESULT("Typedef import", testGenericImportX, "-48");

LOAD_MODULE(test_importprototype, "test.importprototype", "int foo(); int a = foo(); int foo(){ return 10; }");
const char	*testImportPrototype =
"import test.importprototype;\r\n\
return a + foo();";
TEST_RESULT("Prototype import", testImportPrototype, "20");

LOAD_MODULE(test_importconst, "test.importconst", "class Foo{ const int a = 1, b, c, d, e; } int x = sizeof(Foo);");
LOAD_MODULE(test_importconst2, "test.importconst2", "import test.importconst; int y = x + sizeof(Foo);");
const char	*testImportConst =
"import test.importconst2;\r\n\
return sizeof(Foo) + y;";
TEST_RESULT("Constant import", testImportConst, "0");

LOAD_MODULE(test_importgeneric, "test.importgeneric", "class Rect<T>{ T x; } class Font{ Rect<float>[256] symbolRects; }");
const char	*testImportGeneric =
"import test.importgeneric;\r\n\
return sizeof(Font);";
TEST_RESULT("Constant generic", testImportGeneric, "1024");

LOAD_MODULE(test_importconst3, "test.importconst3", "class Rect<T>{ T x; } int foo(Rect<float>[2] symbolRects){ return 5; }");
const char	*testImportConst2 =
"import test.importconst3;\r\n\
auto x = foo;\r\n\
Rect<float>[2] m;\r\n\
return x(m);";
TEST_RESULT("Constant import 2", testImportConst2, "5");
