#include "TestBase.h"

const char *testGenericExplicit1 =
"auto bar<@T, @U>()\r\n\
{\r\n\
	return typeof(T) == typeof(U);\r\n\
}\r\n\
\r\n\
int b = bar with<int, int>();\r\n\
int c = bar with<int, int>();\r\n\
int d = bar with<int, float>();\r\n\
int e = bar with<int, float>();\r\n\
return b * 1000 + c * 100 + d * 10 + e;";
TEST_RESULT("Generic function test with explicit template arguments (multiple instantiations)", testGenericExplicit1, "1100");

const char *testGenericExplicit2 =
"auto bar<@T>(@T a){ return a; }\r\n\
return typeof(bar with<int>(2l)) == int && bar with<int>(2l) == 2;";
TEST_RESULT("Generic function test with explicit template arguments (single type enforcement)", testGenericExplicit2, "1");

const char *testGenericExplicit3 =
"auto bar<@T>(@T a, @T ref b){ return typeof(a) == int && typeof(b) == int ref; }\r\n\
return bar with<int>(2l, 2);";
TEST_RESULT("Generic function test with explicit template arguments (partial type enforcement)", testGenericExplicit3, "1");

const char *testGenericExplicit4 =
"auto bar<@T, @U>(@T x, @U y){ return typeof(x) == int && typeof(y) == long; }\r\n\
return bar with<int>(2l, 2l);";
TEST_RESULT("Generic function test with explicit template arguments (partial type enforcement) 2", testGenericExplicit4, "1");

const char *testGenericExplicit5 =
"auto bar<@T, @U>(@T x, @U y){ return typeof(x) == int && typeof(y) == float; }\r\n\
return bar with<int, float>(2l, 2l);";
TEST_RESULT("Generic function test with explicit template arguments (multiple type enforcement)", testGenericExplicit5, "1");

const char *testGenericExplicit6 =
"class Foo<T>{ T x; }\r\n\
auto foo<@T>()\r\n\
{\r\n\
	return new T(){ x = T(5); };\r\n\
}\r\n\
return typeof(foo with<Foo<long>>().x) == long;";
TEST_RESULT("Generic function test with explicit template arguments (generic explicit type)", testGenericExplicit6, "1");

const char *testGenericExplicit7 =
"class Foo<T>{ T x; }\r\n\
auto foo<@T>()\r\n\
{\r\n\
	return new T(){ x = T(5); };\r\n\
}\r\n\
return foo with<Foo<int>>().x;";
TEST_RESULT("Generic function test with explicit template arguments (generic explicit type) 2", testGenericExplicit7, "5");

const char *testGenericExplicit8 =
"class Foo\r\n\
{\r\n\
	auto bar<@T>(@T x){ return x; }\r\n\
	auto bar<@T>(@T x, @U y){ return x + y; }\r\n\
}\r\n\
Foo x;\r\n\
return typeof(x.bar with<int>(4l, 3)) == int && x.bar with<int>(4l, 3) == 7;";
TEST_RESULT("Generic function test with explicit template arguments (overload resoultion)", testGenericExplicit8, "1");

const char *testGenericExplicit9 =
"auto bar<@T>(@T x){ return x; }\r\n\
return typeof(bar with<int>(bar with<long>(3l))) == int;";
TEST_RESULT("Generic function test with explicit template arguments (nested calls with explicit lists)", testGenericExplicit9, "1");

const char *testGenericExplicit10 =
"class Foo<T>{ T x; }\r\n\
auto foo<@T>()\r\n\
{\r\n\
	return new T(){ x = *new T(5){ *this = 4l; }; };\r\n\
}\r\n\
return foo with<Foo<int>>().x;";
TEST_RESULT("Generic function test with explicit template arguments (correct function explicit list tracking)", testGenericExplicit10, "4");

LOAD_MODULE(test_explicit_type_a, "test.explicit.type.a", "auto foo<@T>(){ return T(4); }");
const char *testGenericExplicit11 =
"import test.explicit.type.a;\r\n\
return typeof(foo with<int>()) == int && foo with<int>() == 4;";
TEST_RESULT("Generic function test with explicit template arguments (import)", testGenericExplicit11, "1");

LOAD_MODULE(test_explicit_type_b, "test.explicit.type.b", "auto foo<@T>(){ return T(4); } foo with<int>();");
const char *testGenericExplicit12 =
"import test.explicit.type.b;\r\n\
return typeof(foo with<int>()) == int && foo with<int>() == 4;";
TEST_RESULT("Generic function test with explicit template arguments (import with instance)", testGenericExplicit12, "1");

LOAD_MODULE(test_explicit_type_c, "test.explicit.type.c", "auto foo<@T>(){ return T(4); } foo with<long>();");
const char *testGenericExplicit13 =
"import test.explicit.type.c;\r\n\
return typeof(foo with<int>()) == int && foo with<int>() == 4;";
TEST_RESULT("Generic function test with explicit template arguments (import with instance) 2", testGenericExplicit13, "1");

LOAD_MODULE(test_explicit_type_d1, "test.explicit.type.d1", "auto foo<@T>(){ return T(4); }");
LOAD_MODULE(test_explicit_type_d2, "test.explicit.type.d2", "import test.explicit.type.d1; foo with<int>();");
const char *testGenericExplicit14 =
"import test.explicit.type.d2;\r\n\
return typeof(foo with<int>()) == int && foo with<int>() == 4;";
TEST_RESULT("Generic function test with explicit template arguments (import with import)", testGenericExplicit14, "1");

LOAD_MODULE(test_explicit_type_e1, "test.explicit.type.e1", "auto foo<@T>(){ return T(4); }");
LOAD_MODULE(test_explicit_type_e2, "test.explicit.type.e2", "import test.explicit.type.e1; foo with<long>();");
const char *testGenericExplicit15 =
"import test.explicit.type.e1;\r\n\
import test.explicit.type.e2;\r\n\
return typeof(foo with<int>()) == int && foo with<int>() == 4;";
TEST_RESULT("Generic function test with explicit template arguments (import with import) 2", testGenericExplicit15, "1");

LOAD_MODULE(test_explicit_type_f1, "test.explicit.type.f1", "auto foo<@T>(){ return T(4); }");
LOAD_MODULE(test_explicit_type_f2, "test.explicit.type.f2", "import test.explicit.type.f1; foo with<int>();");
const char *testGenericExplicit16 =
"import test.explicit.type.f2;\r\n\
return typeof(foo with<int>()) == int && foo with<int>() == 4;";
TEST_RESULT("Generic function test with explicit template arguments (import with import) 3", testGenericExplicit16, "1");
