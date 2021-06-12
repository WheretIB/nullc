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

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)

int myFoo(int x){ return x + 15; }
void* myFunctionLookup(const char* name){ return strcmp(name, "myFoo") == 0 ? (void*)myFoo : NULL; }

const char	*testFunctionAutobinding =
"int myFoo(int x);\r\n\
return myFoo(5);";
TEST_SIMPLE_WITH_SETUP("Automatic function binding [skip_c]", testFunctionAutobinding, "20")
{
	if(before)
		nullcSetMissingFunctionLookup(myFunctionLookup);
	else
		nullcSetMissingFunctionLookup(NULL);
}

LOAD_MODULE(test_Autobind, "test.autobind", "int myFoo(int x);");
const char	*testFunctionAutobinding2 =
"import test.autobind;\r\n\
return myFoo(2);";
TEST_SIMPLE_WITH_SETUP("Automatic function binding 2 [skip_c]", testFunctionAutobinding2, "17")
{
	if(before)
		nullcSetMissingFunctionLookup(myFunctionLookup);
	else
		nullcSetMissingFunctionLookup(NULL);
}

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

LOAD_MODULE(test_fwddecl1a, "test.fwddecl1a", "class Node; Node ref a;");
LOAD_MODULE(test_fwddecl1b, "test.fwddecl1b", "import test.fwddecl1a; class Node{ int x = 2; int y = 3; int sum(){ return x + y; } }");
const char	*testForwardDecl1 =
"import test.fwddecl1a;\r\n\
import test.fwddecl1b;\r\n\
import std.typeinfo;\r\n\
a = new Node();\r\n\
a.y = 10;\r\n\
return sizeof(*a) + a.y + typeid(a).memberCount() == 20;";
TEST_RESULT("Forward declaration import 1", testForwardDecl1, "1");

LOAD_MODULE(test_fwddecl1c, "test.fwddecl1c", "import test.fwddecl1a; import test.fwddecl1b; a = new Node(); a.y = 10;");
const char	*testForwardDecl2 =
"import test.fwddecl1c;\r\n\
auto b = new Node();\r\n\
b.y = 20;\r\n\
return sizeof(*a) + a.y == 18 && sizeof(*b) + b.y == 28;";
TEST_RESULT("Forward declaration import 2", testForwardDecl2, "1");

LOAD_MODULE(test_fwddecl2a, "test.fwddecl2a", "class Node; Node ref a;");
LOAD_MODULE(test_fwddecl2b, "test.fwddecl2b", "import test.fwddecl2a; class Node extendable { int x = 2; int y = 3; int sum(){ return x + y; } }");
LOAD_MODULE(test_fwddecl2c, "test.fwddecl2c", "import test.fwddecl2a; import test.fwddecl2b; a = new Node(); a.y = 10;");
const char	*testTransitiveImport1 =
"import test.fwddecl2c;\r\n\
auto b = new Node();\r\n\
b.y = 20; \r\n\
return a.sum() == 12 && b.sum() == 22 && typeid(b) == Node;";
TEST_RESULT("Transitive module import 1", testTransitiveImport1, "1");

const char	*testTransitiveImport2 =
"import test.fwddecl2c;\r\n\
class Node2 : Node\r\n\
{\r\n\
	int z = 400;\r\n\
	int sum(){ return x + y + z; }\r\n\
}\r\n\
return Node2().sum() == 405;";
TEST_RESULT("Transitive module import 2", testTransitiveImport2, "1");

LOAD_MODULE(test_typedef_import, "test.typedef_import", "class Instance<T>{ T a; } void operator=(Instance ref a, int b){ a.a = b; } class Test{ typedef Instance<int> A; A x = 2; } Test t;");
const char	*testClassTypedefImportOrder1 =
"import test.typedef_import;\r\n\
Test x;\r\n\
return t.x.a + x.x.a;";
TEST_RESULT("Class typedef import order test 1", testClassTypedefImportOrder1, "4");

const char	*testClassTypedefImportOrder2 =
"import test.typedef_import;\r\n\
int Test.y(){ A b = x; return b.a * 2; }\r\n\
Test x;\r\n\
return t.y + x.y;";
TEST_RESULT("Class typedef import order test 2", testClassTypedefImportOrder2, "8");

LOAD_MODULE(test_nested_nested_a, "test.nested.nested_a", "int foo(int x){ return -x; }");
LOAD_MODULE(test_nested_nested_b, "test.nested.nested_b", "import std.vector; import nested_a; int bar(int x){ return foo(x) * 2; }");
const char	*testNestedModuleSearch1 =
"import test.nested.nested_b;\r\n\
return bar(4) == -8;";
TEST_RESULT("Nested module search 1", testNestedModuleSearch1, "1");

LOAD_MODULE(test_nested_a, "test.nested_a", "assert(false, \"wasn't imported\"); int foo(int x){ return x; }");
const char	*testNestedModuleSearch2 =
"import test.nested.nested_b;\r\n\
return bar(4) == -8;";
TEST_RESULT("Nested module search 2 (unrelated file in outer folder)", testNestedModuleSearch2, "1");

LOAD_MODULE(test_nested_deep_nested_a, "test.nested.deep.nested_a", "int foo(int x){ return -x; }");
LOAD_MODULE(test_nested_deep_nested_b, "test.nested.deep.nested_b", "import nested_a; int bar(int x){ return foo(x) * 2; }");
LOAD_MODULE(test_nested_nested_c, "test.nested.nested_c", "import deep.nested_b; int test(int x){ return bar(-x) * 2; }");
LOAD_MODULE(test_nested_nested_d, "test.nested.nested_d", "import nested_c; int test2(int x){ return test(x * 10); }");
const char	*testNestedModuleSearch3 =
"import test.nested.nested_d;\r\n\
return test2(4) == 160;";
TEST_RESULT("Nested module search 3 (multiple levels)", testNestedModuleSearch3, "1");

LOAD_MODULE(test_deep_nestring_a, "test.deep_nestring_a", "auto a0(char[] x){ return auto(int y){ return x; }; } auto a1(char[] x){ return auto(int y){ return x; }; } auto a2(char[] x){ return auto(int y){ return x; }; } auto a3(char[] x){ return auto(int y){ return x; }; } auto a4(char[] x){ return auto(int y){ return x; }; } auto a5(char[] x){ return auto(int y){ return x; }; } auto a6(char[] x){ return auto(int y){ return x; }; } auto a7(char[] x){ return auto(int y){ return x; }; } auto a8(char[] x){ return auto(int y){ return x; }; } auto a9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_b, "test.deep_nestring_b", "auto b0(char[] x){ return auto(int y){ return x; }; } auto b1(char[] x){ return auto(int y){ return x; }; } auto b2(char[] x){ return auto(int y){ return x; }; } auto b3(char[] x){ return auto(int y){ return x; }; } auto b4(char[] x){ return auto(int y){ return x; }; } auto b5(char[] x){ return auto(int y){ return x; }; } auto b6(char[] x){ return auto(int y){ return x; }; } auto b7(char[] x){ return auto(int y){ return x; }; } auto b8(char[] x){ return auto(int y){ return x; }; } auto b9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_c, "test.deep_nestring_c", "import test.deep_nestring_a; auto c0(char[] x){ return auto(int y){ return x; }; } auto c1(char[] x){ return auto(int y){ return x; }; } auto c2(char[] x){ return auto(int y){ return x; }; } auto c3(char[] x){ return auto(int y){ return x; }; } auto c4(char[] x){ return auto(int y){ return x; }; } auto c5(char[] x){ return auto(int y){ return x; }; } auto c6(char[] x){ return auto(int y){ return x; }; } auto c7(char[] x){ return auto(int y){ return x; }; } auto c8(char[] x){ return auto(int y){ return x; }; } auto c9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_d, "test.deep_nestring_d", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; auto d0(char[] x){ return auto(int y){ return x; }; } auto d1(char[] x){ return auto(int y){ return x; }; } auto d2(char[] x){ return auto(int y){ return x; }; } auto d3(char[] x){ return auto(int y){ return x; }; } auto d4(char[] x){ return auto(int y){ return x; }; } auto d5(char[] x){ return auto(int y){ return x; }; } auto d6(char[] x){ return auto(int y){ return x; }; } auto d7(char[] x){ return auto(int y){ return x; }; } auto d8(char[] x){ return auto(int y){ return x; }; } auto d9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_e, "test.deep_nestring_e", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; auto e0(char[] x){ return auto(int y){ return x; }; } auto e1(char[] x){ return auto(int y){ return x; }; } auto e2(char[] x){ return auto(int y){ return x; }; } auto e3(char[] x){ return auto(int y){ return x; }; } auto e4(char[] x){ return auto(int y){ return x; }; } auto e5(char[] x){ return auto(int y){ return x; }; } auto e6(char[] x){ return auto(int y){ return x; }; } auto e7(char[] x){ return auto(int y){ return x; }; } auto e8(char[] x){ return auto(int y){ return x; }; } auto e9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_f, "test.deep_nestring_f", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; auto f0(char[] x){ return auto(int y){ return x; }; } auto f1(char[] x){ return auto(int y){ return x; }; } auto f2(char[] x){ return auto(int y){ return x; }; } auto f3(char[] x){ return auto(int y){ return x; }; } auto f4(char[] x){ return auto(int y){ return x; }; } auto f5(char[] x){ return auto(int y){ return x; }; } auto f6(char[] x){ return auto(int y){ return x; }; } auto f7(char[] x){ return auto(int y){ return x; }; } auto f8(char[] x){ return auto(int y){ return x; }; } auto f9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_g, "test.deep_nestring_g", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; auto g0(char[] x){ return auto(int y){ return x; }; } auto g1(char[] x){ return auto(int y){ return x; }; } auto g2(char[] x){ return auto(int y){ return x; }; } auto g3(char[] x){ return auto(int y){ return x; }; } auto g4(char[] x){ return auto(int y){ return x; }; } auto g5(char[] x){ return auto(int y){ return x; }; } auto g6(char[] x){ return auto(int y){ return x; }; } auto g7(char[] x){ return auto(int y){ return x; }; } auto g8(char[] x){ return auto(int y){ return x; }; } auto g9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_h, "test.deep_nestring_h", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; import test.deep_nestring_f; import test.deep_nestring_g; auto h0(char[] x){ return auto(int y){ return x; }; } auto h1(char[] x){ return auto(int y){ return x; }; } auto h2(char[] x){ return auto(int y){ return x; }; } auto h3(char[] x){ return auto(int y){ return x; }; } auto h4(char[] x){ return auto(int y){ return x; }; } auto h5(char[] x){ return auto(int y){ return x; }; } auto h6(char[] x){ return auto(int y){ return x; }; } auto h7(char[] x){ return auto(int y){ return x; }; } auto h8(char[] x){ return auto(int y){ return x; }; } auto h9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_i, "test.deep_nestring_i", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; import test.deep_nestring_f; import test.deep_nestring_g; auto i0(char[] x){ return auto(int y){ return x; }; } auto i1(char[] x){ return auto(int y){ return x; }; } auto i2(char[] x){ return auto(int y){ return x; }; } auto i3(char[] x){ return auto(int y){ return x; }; } auto i4(char[] x){ return auto(int y){ return x; }; } auto i5(char[] x){ return auto(int y){ return x; }; } auto i6(char[] x){ return auto(int y){ return x; }; } auto i7(char[] x){ return auto(int y){ return x; }; } auto i8(char[] x){ return auto(int y){ return x; }; } auto i9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_j, "test.deep_nestring_j", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; import test.deep_nestring_f; import test.deep_nestring_g; import test.deep_nestring_h; import test.deep_nestring_i; auto j0(char[] x){ return auto(int y){ return x; }; } auto j1(char[] x){ return auto(int y){ return x; }; } auto j2(char[] x){ return auto(int y){ return x; }; } auto j3(char[] x){ return auto(int y){ return x; }; } auto j4(char[] x){ return auto(int y){ return x; }; } auto j5(char[] x){ return auto(int y){ return x; }; } auto j6(char[] x){ return auto(int y){ return x; }; } auto j7(char[] x){ return auto(int y){ return x; }; } auto j8(char[] x){ return auto(int y){ return x; }; } auto j9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_k, "test.deep_nestring_k", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; import test.deep_nestring_f; import test.deep_nestring_g; import test.deep_nestring_h; import test.deep_nestring_i; auto k0(char[] x){ return auto(int y){ return x; }; } auto k1(char[] x){ return auto(int y){ return x; }; } auto k2(char[] x){ return auto(int y){ return x; }; } auto k3(char[] x){ return auto(int y){ return x; }; } auto k4(char[] x){ return auto(int y){ return x; }; } auto k5(char[] x){ return auto(int y){ return x; }; } auto k6(char[] x){ return auto(int y){ return x; }; } auto k7(char[] x){ return auto(int y){ return x; }; } auto k8(char[] x){ return auto(int y){ return x; }; } auto k9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_l, "test.deep_nestring_l", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; import test.deep_nestring_f; import test.deep_nestring_g; import test.deep_nestring_h; import test.deep_nestring_i; import test.deep_nestring_j; import test.deep_nestring_k; auto l0(char[] x){ return auto(int y){ return x; }; } auto l1(char[] x){ return auto(int y){ return x; }; } auto l2(char[] x){ return auto(int y){ return x; }; } auto l3(char[] x){ return auto(int y){ return x; }; } auto l4(char[] x){ return auto(int y){ return x; }; } auto l5(char[] x){ return auto(int y){ return x; }; } auto l6(char[] x){ return auto(int y){ return x; }; } auto l7(char[] x){ return auto(int y){ return x; }; } auto l8(char[] x){ return auto(int y){ return x; }; } auto l9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_m, "test.deep_nestring_m", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; import test.deep_nestring_f; import test.deep_nestring_g; import test.deep_nestring_h; import test.deep_nestring_i; import test.deep_nestring_j; import test.deep_nestring_k; auto m0(char[] x){ return auto(int y){ return x; }; } auto m1(char[] x){ return auto(int y){ return x; }; } auto m2(char[] x){ return auto(int y){ return x; }; } auto m3(char[] x){ return auto(int y){ return x; }; } auto m4(char[] x){ return auto(int y){ return x; }; } auto m5(char[] x){ return auto(int y){ return x; }; } auto m6(char[] x){ return auto(int y){ return x; }; } auto m7(char[] x){ return auto(int y){ return x; }; } auto m8(char[] x){ return auto(int y){ return x; }; } auto m9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_n, "test.deep_nestring_n", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; import test.deep_nestring_f; import test.deep_nestring_g; import test.deep_nestring_h; import test.deep_nestring_i; import test.deep_nestring_j; import test.deep_nestring_k; import test.deep_nestring_l; import test.deep_nestring_m; auto n0(char[] x){ return auto(int y){ return x; }; } auto n1(char[] x){ return auto(int y){ return x; }; } auto n2(char[] x){ return auto(int y){ return x; }; } auto n3(char[] x){ return auto(int y){ return x; }; } auto n4(char[] x){ return auto(int y){ return x; }; } auto tn5(char[] x){ return auto(int y){ return x; }; } auto n6(char[] x){ return auto(int y){ return x; }; } auto n7(char[] x){ return auto(int y){ return x; }; } auto n8(char[] x){ return auto(int y){ return x; }; } auto n9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_o, "test.deep_nestring_o", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; import test.deep_nestring_f; import test.deep_nestring_g; import test.deep_nestring_h; import test.deep_nestring_i; import test.deep_nestring_j; import test.deep_nestring_k; import test.deep_nestring_l; import test.deep_nestring_m; auto o0(char[] x){ return auto(int y){ return x; }; } auto o1(char[] x){ return auto(int y){ return x; }; } auto o2(char[] x){ return auto(int y){ return x; }; } auto o3(char[] x){ return auto(int y){ return x; }; } auto o4(char[] x){ return auto(int y){ return x; }; } auto o5(char[] x){ return auto(int y){ return x; }; } auto o6(char[] x){ return auto(int y){ return x; }; } auto o7(char[] x){ return auto(int y){ return x; }; } auto o8(char[] x){ return auto(int y){ return x; }; } auto o9(char[] x){ return auto(int y){ return x; }; }");
LOAD_MODULE(test_deep_nestring_p, "test.deep_nestring_p", "import test.deep_nestring_a; import test.deep_nestring_b; import test.deep_nestring_c; import test.deep_nestring_d; import test.deep_nestring_e; import test.deep_nestring_f; import test.deep_nestring_g; import test.deep_nestring_h; import test.deep_nestring_i; import test.deep_nestring_j; import test.deep_nestring_k; import test.deep_nestring_l; import test.deep_nestring_m; import test.deep_nestring_n; import test.deep_nestring_o; auto p0(char[] x){ return auto(int y){ return x; }; } auto p1(char[] x){ return auto(int y){ return x; }; } auto p2(char[] x){ return auto(int y){ return x; }; } auto p3(char[] x){ return auto(int y){ return x; }; } auto p4(char[] x){ return auto(int y){ return x; }; } auto p5(char[] x){ return auto(int y){ return x; }; } auto p6(char[] x){ return auto(int y){ return x; }; } auto p7(char[] x){ return auto(int y){ return x; }; } auto p8(char[] x){ return auto(int y){ return x; }; } auto p9(char[] x){ return auto(int y){ return x; }; }");
const char	*testDeepModuleNesting =
"import test.deep_nestring_p;\r\n\
return a0(\"hi\")(5)[1] == 'i';";
TEST_RESULT("Deep nesting modules", testDeepModuleNesting, "1");

LOAD_MODULE(test_local_function_visibility, "test.local_function_visibility", "int f1(){ auto n1(int x){ return -x; } return n1(10); }");
const char	*testLocalFunctionVisibility =
"import test.local_function_visibility;\r\n\
int f2(){ auto n1(int x){ return x * 100; } return n1(4); }\r\n\
return f1() + f2();";
TEST_RESULT("Visiblity of imported local function", testLocalFunctionVisibility, "390");

LOAD_MODULE(test_class_member_alignment1, "test.class_member_alignment1", "class Test extendable{ bool a; float b; } float x(Test ref a){ return a.b; }");
const char	*testClassMemberAlignmentImport1 =
"import test.class_member_alignment1;\r\n\
class Test2 : Test{ int c; }\r\n\
Test2 b;\r\n\
b.b = 123456789;\r\n\
return int(x(&b) - b.b);";
TEST_RESULT("Import of class member alignment value 1", testClassMemberAlignmentImport1, "0");

LOAD_MODULE(test_class_member_alignment2, "test.class_member_alignment2", "class Test extendable{ bool a; align(16) float b; } float x(Test ref a){ return a.b; }");
const char	*testClassMemberAlignmentImport2 =
"import test.class_member_alignment2;\r\n\
class Test2 : Test{ int c; }\r\n\
Test2 b;\r\n\
b.b = 123456789;\r\n\
return int(x(&b) - b.b);";
TEST_RESULT("Import of class member alignment value 2", testClassMemberAlignmentImport2, "0");
