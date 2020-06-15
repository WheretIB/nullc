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

#if defined(NULLC_AUTOBINDING)

NULLC_BIND int myFoo(int x){ return x + 15; }

const char	*testFunctionAutobinding =
"int myFoo(int x);\r\n\
return myFoo(5);";
TEST_RESULT_SIMPLE("Automatic function binding [skip_c]", testFunctionAutobinding, "20");

LOAD_MODULE(test_Autobind, "test.autobind", "int myFoo(int x);");
const char	*testFunctionAutobinding2 =
"import test.autobind;\r\n\
return myFoo(2);";
TEST_RESULT("Automatic function binding 2 [skip_c]", testFunctionAutobinding2, "17");

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

LOAD_MODULE(test_importenum, "test.importenum", "enum Test{ A, B, C, D }");
const char	*testImportEnum =
"import test.importenum;\r\n\
Test a = Test.C;\r\n\
return int(a);";
TEST_RESULT("Enum import", testImportEnum, "2");

LOAD_MODULE(test_importenum2, "test.importenum2", "import std.file; enum Test{ WORLD, VIEW, PROJ } void foo( Test type ){ }");
const char	*testImportEnum2 =
"import std.file;\r\n\
import test.importenum2;\r\n\
foo(Test.PROJ);\r\n\
return 1;";
TEST_RESULT("Enum import 2", testImportEnum2, "1");

LOAD_MODULE(test_importenum3a, "test.importenum3a", "enum Seek1{ A, B, C }"); // old
LOAD_MODULE(test_importenum3b, "test.importenum3b", "enum Seek2{ A, B, C }"); // new
LOAD_MODULE(test_importenum3c, "test.importenum3c", "enum Seek3{ A, B, C }"); // old
LOAD_MODULE(test_importenum3d, "test.importenum3d", "import test.importenum3a; import test.importenum3b; import test.importenum3c; void foo( Seek2 type ){ }");
const char	*testImportEnum3 =
"import test.importenum3a;\r\n\
import test.importenum3c;\r\n\
import test.importenum3d;\r\n\
foo(Seek2.B);\r\n\
return 1;";
TEST_RESULT("Enum import 3", testImportEnum3, "1");

LOAD_MODULE(test_importnamespacecoroutine, "test.importnamespacecoroutine", "namespace Foo{coroutine int foo(){return 0;}}");
const char	*testImportNamespaceCoroutine =
"import test.importnamespacecoroutine;\r\n\
for(i in Foo.foo){}\r\n\
return 1;";
TEST_RESULT("Import coroutine from a namespace", testImportNamespaceCoroutine, "1");

LOAD_MODULE(test_importnamespacecoroutine2, "test.importnamespacecoroutine2", "import std.range; namespace Foo{coroutine int foo(){return 5;}}");
const char	*testImportNamespaceCoroutine2 =
"import test.importnamespacecoroutine2;\r\n\
auto y = Foo.foo;\r\n\
return y();";
TEST_RESULT("Import coroutine from a namespace 2", testImportNamespaceCoroutine2, "5");

LOAD_MODULE(test_importindirect1, "test.importindirect1", "class vector<T>{ T x; }");
LOAD_MODULE(test_importindirect2, "test.importindirect2", "import test.importindirect1; int a = 4;");
const char	*testImportInidrect1 =
"import test.importindirect2;\r\n\
vector<int> x;\r\n\
return 1;";
TEST_RESULT("Indirect generic type import", testImportInidrect1, "1");

LOAD_MODULE(test_variablecollision1, "test.variablecollision1", "int a = 1;");
LOAD_MODULE(test_variablecollision2, "test.variablecollision2", "import test.variablecollision1; a = 2;");
LOAD_MODULE(test_variablecollision3, "test.variablecollision3", "import test.variablecollision1; a = 3;");
const char	*testVariableCollsion1 =
"import test.variablecollision1;\r\n\
import test.variablecollision2;\r\n\
import test.variablecollision3;\r\n\
return a;";
TEST_RESULT("Variable collision 1", testVariableCollsion1, "3");

LOAD_MODULE(test_variablecollision1b, "test.variablecollision1b", "class Foo{ int foo(){ return 12; } } if(0){ auto ref x; x.foo(); }");
LOAD_MODULE(test_variablecollision2b, "test.variablecollision2b", "import test.variablecollision1b; if(0){ auto ref x; x.foo(); }");
LOAD_MODULE(test_variablecollision3b, "test.variablecollision3b", "import test.variablecollision1b; if(0){ auto ref x; x.foo(); }");
const char	*testVariableCollsion2 =
"import test.variablecollision1b;\r\n\
import test.variablecollision2b;\r\n\
import test.variablecollision3b;\r\n\
auto ref x = new Foo();\r\n\
return x.foo();";
TEST_RESULT("Variable collision 2", testVariableCollsion2, "12");

LOAD_MODULE(test_variablecollision1c, "test.variablecollision1c", "class Foo{ int foo(){ return 12; } }");
LOAD_MODULE(test_variablecollision2c, "test.variablecollision2c", "import test.variablecollision1c; if(0){ auto ref x; x.foo(); }");
LOAD_MODULE(test_variablecollision3c, "test.variablecollision3c", "import test.variablecollision1c; if(0){ auto ref x; x.foo(); }");
const char	*testVariableCollsion3 =
"import test.variablecollision1c;\r\n\
import test.variablecollision2c;\r\n\
import test.variablecollision3c;\r\n\
auto ref x = new Foo();\r\n\
return x.foo();";
TEST_RESULT("Variable collision 3", testVariableCollsion3, "12");

LOAD_MODULE(test_Imporhide2, "test.importhide2", "auto x = auto lam_bda(int x){ return x * 2; };");
const char	*testImportHidding2 =
"import test.importhide2;\r\n\
auto y = auto lam_bda(int x){ return x + 2; };\r\n\
return x(10) + y(4);";
TEST_RESULT("Hidden local function excludion from import", testImportHidding2, "26");

LOAD_MODULE(test_derivedtypes1, "test.derivedtypes1", "class int2 extendable{ int x, y; void int2(){ x = 10; y = 20; } } class int3 : int2{ int z; void int3(int a, int b, int2 ref c, int d){ this.z = b; } } class int4 : int2{ int z, w; void int4(){} }");
const char	*testDerivedTypeImport1 =
"import test.derivedtypes1;\r\n\
auto x = new int3(1, 2, new int4, 4);\r\n\
return x.z;";
TEST_RESULT("Derived type import preserves base class information 1", testDerivedTypeImport1, "2");

LOAD_MODULE(test_derivedtypes2, "test.derivedtypes2", "class int2<T> extendable{ T x, y; void int2(){ x = 10; y = 20; } } class int3<U> : int2<U>{ U z; void int3(U a, U b, int2<U> ref c, U d){ z = b; } } class int4<U> : int2<U>{ U z, w; void int4(){} }");
const char	*testDerivedTypeImport2 =
"import test.derivedtypes2;\r\n\
auto x = new int3<int>(1, 2, new int4<int>, 4);\r\n\
return x.z;";
TEST_RESULT("Derived type import preserves base class information 2", testDerivedTypeImport2, "2");

LOAD_MODULE(test_derivedtypes3, "test.derivedtypes3", "class int2 extendable{ int x, y; void int2(){ x = 10; y = 20; } }");

const char	*testDerivedTypeImport3 =
"import test.derivedtypes3;\r\n\
class int3 : int2{ int z; void int3(int a, int b, int2 ref c, int d){ z = b; } }\r\n\
class int4 : int2{ int z, w; void int4(){} }\r\n\
auto x = new int3(1, 2, new int4, 4);\r\n\
return x.z;";
TEST_RESULT("Derived type import preserves base class information 3", testDerivedTypeImport3, "2");
