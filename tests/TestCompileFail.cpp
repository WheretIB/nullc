#include "TestCompileFail.h"

#include "TestBase.h"

void TEST_FOR_FAIL_FULL(const char* name, const char* str, const char* error)
{
	testsCount[TEST_TYPE_FAILURE]++;
	nullres good = nullcCompile(str);
	if(!good)
	{
		char buf[4096];
		strncpy(buf, nullcGetLastError(), 4095); buf[4095] = 0;
		if(strcmp(error, buf) != 0)
		{
			printf("Failed %s but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", name, buf, error);
		}else{
			testsPassed[TEST_TYPE_FAILURE]++;
		}
	}else{
		printf("Test \"%s\" failed to fail.\r\n", name);
	}
}


void RunCompileFailTests()
{
	TEST_FOR_FAIL("Number not allowed in this base", "return 08;", "ERROR: digit 8 is not allowed in base 8");
	TEST_FOR_FAIL("Unknown escape sequence", "return '\\p';", "ERROR: unknown escape sequence");
	TEST_FOR_FAIL("Wrong alignment", "align(32) int a; return 0;", "ERROR: alignment must be less than 16 bytes");
	TEST_FOR_FAIL("Wrong alignment", "align(13) int a; return 0;", "ERROR: alignment must be power of two");
	TEST_FOR_FAIL("Change of immutable value", "int i; return *i = 5;", "ERROR: cannot dereference type 'int' that is not a pointer");
	TEST_FOR_FAIL("Hex overflow", "return 0xbeefbeefbeefbeefb;", "ERROR: overflow in hexadecimal constant");
	TEST_FOR_FAIL("Oct overflow", "return 03333333333333333333333;", "ERROR: overflow in octal constant");
	TEST_FOR_FAIL("Bin overflow", "return 10000000000000000000000000000000000000000000000000000000000000000b;", "ERROR: overflow in binary constant");
	TEST_FOR_FAIL("Int overflow 1", "return 2147483648;", "ERROR: overflow in integer constant");
	TEST_FOR_FAIL("Int overflow 2", "return -2147483648;", "ERROR: overflow in integer constant");
	TEST_FOR_FAIL("Long overflow 1", "return 9223372036854775808l;", "ERROR: overflow in integer constant");
	TEST_FOR_FAIL("Long overflow 2", "return 100000000000000000000000000000000000l;", "ERROR: overflow in integer constant");
	TEST_FOR_FAIL("Logical not on double", "return !0.5;", "ERROR: unary operation '!' is not supported on 'double'");
	TEST_FOR_FAIL("Binary not on double", "return ~0.4;", "ERROR: unary operation '~' is not supported on 'double'");

	TEST_FOR_FAIL("No << on float", "return 1.0 << 2.0;", "ERROR: operation << is not supported on 'double' and 'double'");
	TEST_FOR_FAIL("No >> on float", "return 1.0 >> 2.0;", "ERROR: operation >> is not supported on 'double' and 'double'");
	TEST_FOR_FAIL("No | on float", "return 1.0 | 2.0;", "ERROR: operation | is not supported on 'double' and 'double'");
	TEST_FOR_FAIL("No & on float", "return 1.0 & 2.0;", "ERROR: operation & is not supported on 'double' and 'double'");
	TEST_FOR_FAIL("No ^ on float", "return 1.0 ^ 2.0;", "ERROR: operation ^ is not supported on 'double' and 'double'");

	TEST_FOR_FAIL("Wrong return", "int a(){ return {1,2};} return 1;", "ERROR: cannot convert 'int[2]' to 'int'");
	TEST_FOR_FAIL("Shouldn't return anything", "void a(){ return 1; } return 1;", "ERROR: 'void' function returning a value");
	TEST_FOR_FAIL("Should return something", "int a(){ return; } return 1;", "ERROR: function must return a value of type 'int'");
	TEST_FOR_FAIL("Global return doesn't accept void", "void a(){} return a();", "ERROR: global return cannot accept 'void'");
	TEST_FOR_FAIL("Global return doesn't accept complex types", "void a(){} return a;", "ERROR: global return cannot accept 'void ref()'");

	TEST_FOR_FAIL("Break followed by trash", "int a; break a; return 1;", "ERROR: break statement must be followed by ';' or a constant");
	TEST_FOR_FAIL("Break with depth 0", "break 0; return 1;", "ERROR: break level can't be negative or zero");
	TEST_FOR_FAIL("Break with depth too big", "while(1){ break 2; } return 1;", "ERROR: break level is greater that loop depth");

	TEST_FOR_FAIL("continue followed by trash", "int a; continue a; return 1;", "ERROR: continue statement must be followed by ';' or a constant");
	TEST_FOR_FAIL("continue with depth 0", "continue 0; return 1;", "ERROR: continue level can't be negative or zero");
	TEST_FOR_FAIL("continue with depth too big", "while(1){ continue 2; } return 1;", "ERROR: continue level is greater that loop depth");

	TEST_FOR_FAIL("Variable redefinition", "int a, a; return 1;", "ERROR: name 'a' is already taken for a variable in current scope");
	TEST_FOR_FAIL("Variable hides function", "void a(){} int a; return 1;", "ERROR: name 'a' is already taken for a function");

	TEST_FOR_FAIL("Uninit auto", "auto a; return 1;", "ERROR: auto variable must be initialized in place of definition");
	TEST_FOR_FAIL("Array of auto", "auto[4] a; return 1;", "ERROR: cannot specify array size for auto");
	TEST_FOR_FAIL("sizeof auto", "return sizeof(auto);", "ERROR: sizeof auto type is illegal");

	TEST_FOR_FAIL("Unknown function", "return b;", "ERROR: unknown identifier 'b'");
	TEST_FOR_FAIL("Unclear decision", "void a(int b){} void a(float b){} return a;", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("Variable of unknown type used", "auto a = a + 1; return a;", "ERROR: variable 'a' is being used while its type is unknown");
	TEST_FOR_FAIL("Variable of unknown type used", "auto x = x(<y>{ 3; });", "ERROR: variable 'x' is being used while its type is unknown");
	
	TEST_FOR_FAIL("Indexing not an array", "int a; return a[5];", "ERROR: type 'int' is not an array");
	TEST_FOR_FAIL("Array underflow", "int[4] a; a[-1] = 2; return 1;", "ERROR: array index cannot be negative");
	TEST_FOR_FAIL("Array overflow", "int[4] a; a[5] = 1; return 1;", "ERROR: array index out of bounds");

	TEST_FOR_FAIL("No matching function", "int f(int a, b){ return 0; } int f(int a, long b){ return 0; } return f(1);", "ERROR: can't find function 'f' with following arguments:");
	TEST_FOR_FAIL("No clear decision", "int f(int a, b){ return 0; } int f(int a, long b){ return 0; } int f(){ return 0; } return f(1, 3.0);", "ERROR: ambiguity, there is more than one overloaded function available for the call:");

	TEST_FOR_FAIL("Array without member", "int[4] a; return a.m;", "ERROR: member variable or function 'm' is not defined in class 'int[4]'");
	TEST_FOR_FAIL("No methods", "int[4] i; return i.ok();", "ERROR: member variable or function 'ok' is not defined in class 'int[4]'");
	TEST_FOR_FAIL("void array", "void f(){} return { f(), f() };", "ERROR: array cannot be constructed from void type elements");
	TEST_FOR_FAIL("Name taken", "int a; void a(){} return 1;", "ERROR: name 'a' is already taken for a variable in current scope");
	TEST_FOR_FAIL("Auto argument", "auto(auto a){} return 1;", "ERROR: function argument cannot be an auto type");
	TEST_FOR_FAIL("Auto argument 2", "int func(auto a, int i){ return 0; } return 0;", "ERROR: function argument cannot be an auto type");
	TEST_FOR_FAIL("Auto argument 3", "auto foo(@T t, auto a){ } foo(1);", "ERROR: function argument cannot be an auto type");
	TEST_FOR_FAIL("Auto argument 4", "auto foo(auto a, @T t){ } foo(1);", "ERROR: function argument cannot be an auto type");
	TEST_FOR_FAIL("Function redefine", "int a(int b){ return 0; } int a(int c){ return 0; } return 1;", "ERROR: function 'a' is being defined with the same set of arguments");
	TEST_FOR_FAIL("Wrong overload", "int operator*(int a){} return 1;", "ERROR: operator '*' definition must accept exactly two arguments");
	TEST_FOR_FAIL("No member function", "int a; return a.ok();", "ERROR: member variable or function 'ok' is not defined in class 'int'");
	TEST_FOR_FAIL("Unclear decision - member function", "class test{ void a(int b){} void a(float b){} } test t; return t.a;", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("No function", "return k();", "ERROR: unknown identifier 'k'");

	TEST_FOR_FAIL("void condition", "void f(){} if(f()){} return 1;", "ERROR: condition type cannot be 'void' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("void condition", "void f(){} if(f()){}else{} return 1;", "ERROR: condition type cannot be 'void' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("void condition", "void f(){} return f() ? 1 : 0;", "ERROR: condition type cannot be 'void' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("void condition", "void f(){} for(int i = 0; f(); i++){} return 1;", "ERROR: condition type cannot be 'void' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("void condition", "void f(){} while(f()){} return 1;", "ERROR: condition type cannot be 'void' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("void condition", "void f(){} do{}while(f()); return 1;", "ERROR: condition type cannot be 'void' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("complex condition", "import std.math; float4 f(){ return float4(1,1,1,1); } if(f()){} return 1;", "ERROR: condition type cannot be 'float4' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("complex condition", "import std.math; float4 f(){ return float4(1,1,1,1); } if(f()){}else{} return 1;", "ERROR: condition type cannot be 'float4' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("complex condition", "import std.math; float4 f(){ return float4(1,1,1,1); } return f() ? 1 : 0;", "ERROR: condition type cannot be 'float4' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("complex condition", "import std.math; float4 f(){ return float4(1,1,1,1); } for(int i = 0; f(); i++){} return 1;", "ERROR: condition type cannot be 'float4' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("complex condition", "import std.math; float4 f(){ return float4(1,1,1,1); } while(f()){} return 1;", "ERROR: condition type cannot be 'float4' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("complex condition", "import std.math; float4 f(){ return float4(1,1,1,1); } do{}while(f()); return 1;", "ERROR: condition type cannot be 'float4' and function for conversion to bool is undefined");
	TEST_FOR_FAIL("void switch", "void f(){} switch(f()){ case 1: break; } return 1;", "ERROR: condition type cannot be 'void'");
	TEST_FOR_FAIL("complex switch", "import std.math; float4 f(){ return float4(1,1,1,1); } switch(f()){ case 1: break; } return 1;", "ERROR: can't find function '==' with following arguments:");
	TEST_FOR_FAIL("complex switch", "class Foo{ int x, y; } float operator==(Foo a, b){ return 1.0f; } Foo c; switch(c){ case Foo(): break; } return 2;", "ERROR: '==' operator result type must be bool, char, short or int");
	TEST_FOR_FAIL("void case", "void f(){} switch(1){ case f(): break; } return 1;", "ERROR: case value type cannot be 'void'");
	TEST_FOR_FAIL("void array init 1", "void[4] x = 1;", "ERROR: cannot specify array size for void");
	TEST_FOR_FAIL("void array init 2", "void[] x = 1;", "ERROR: cannot specify array size for void");

	TEST_FOR_FAIL("class wrong alignment", "align(13) class test{int a;} return 1;", "ERROR: alignment must be power of two");
	TEST_FOR_FAIL("class wrong alignment", "align(32) class test{int a;} return 1;", "ERROR: alignment must be less than 16 bytes");
	TEST_FOR_FAIL("class member auto", "class test{ auto i; } return 1;", "ERROR: member variable type cannot be 'auto'");
	TEST_FOR_FAIL("class is too big", "class nobiggy{ int[128][128][4] a; } return 1;", "ERROR: class size cannot exceed 65535 bytes");

	TEST_FOR_FAIL("array size not const", "import std.math; int[cos(12) * 16] a; return a[0];", "ERROR: array size cannot be evaluated");
	TEST_FOR_FAIL("array size not positive", "int[-16] a; return a[0];", "ERROR: array size can't be negative or zero");

	TEST_FOR_FAIL("function argument cannot be a void type", "int f(void a){ return 0; } return 1;", "ERROR: function argument cannot be a void type");
	TEST_FOR_FAIL("function prototype with unresolved return type", "auto f(); return 1;", "ERROR: function prototype with unresolved return type");

	TEST_FOR_FAIL("Variable as a function", "int a = 5; return a(4);", "ERROR: operator '()' accepting 1 argument(s) is undefined for a class 'int'");

	TEST_FOR_FAIL("Function pointer call with wrong argument count", "int f(int a){ return -a; } auto foo = f; auto b = foo(); return foo(1, 2);", "ERROR: function expects 1 argument(s), while 0 are supplied");
	TEST_FOR_FAIL("Function pointer call with wrong argument types", "import std.math; int f(int a){ return -a; } auto foo = f; float4 v; return foo(v);", "ERROR: cannot convert 'float4' to 'int'");

	TEST_FOR_FAIL("Indirect function pointer call with wrong argument count", "int f(int a){ return -a; } typeof(f)[2] foo = { f, f }; auto b = foo[0](); return foo(1, 2);", "ERROR: function expects 1 argument(s), while 0 are supplied");
	TEST_FOR_FAIL("Indirect function pointer call with wrong argument types", "import std.math; int f(int a){ return -a; } typeof(f)[2] foo = { f, f }; float4 v; return foo[0](v);", "ERROR: cannot convert 'float4' to 'int'");

	TEST_FOR_FAIL("Array element type mismatch", "import std.math;\r\nauto err = { 1, float2(2, 3), 4 };\r\nreturn 1;", "ERROR: array element 2 type 'float2' doesn't match 'int'");
	TEST_FOR_FAIL("Ternary operator complex type mismatch", "import std.math;\r\nint x = 1; auto err = x ? 1 : float2(2, 3);\r\nreturn 1;", "ERROR: can't find common type between 'int' and 'float2'");

	TEST_FOR_FAIL("Indexing value that is not an array 2", "return (1)[1];", "ERROR: type 'int' is not an array");
	TEST_FOR_FAIL("Illegal conversion from type[] ref to type[]", "int[] b = { 1, 2, 3 };int[] ref c = &b;int[] d = c;return 1;", "ERROR: cannot convert 'int[] ref' to 'int[]'");
	TEST_FOR_FAIL("Type redefinition", "class int{ int a, b; } return 1;", "ERROR: 'int' is being redefined");
	TEST_FOR_FAIL("Type redefinition 2", "typedef int uint; class uint{ int x; } return 1;", "ERROR: 'uint' is being redefined");

	TEST_FOR_FAIL("Illegal conversion 1", "import std.math; float3 a; a = 12.0; return 1;", "ERROR: cannot convert 'double' to 'float3'");
	TEST_FOR_FAIL("Illegal conversion 2", "import std.math; float3 a; float4 b; b = a; return 1;", "ERROR: cannot convert 'float3' to 'float4'");

	TEST_FOR_FAIL("For scope", "for(int i = 0; i < 1000; i++) i += 5; return i;", "ERROR: unknown identifier 'i'");

	TEST_FOR_FAIL("Class function return unclear 1", "class Test{int i;int foo(){ return i; }int foo(int k){ return i; }auto bar(){ return foo; }}return 1;", "ERROR: ambiguity, there is more than one overloaded function available:");

	TEST_FOR_FAIL("Class externally defined method 1", "int dontexist:don(){ return 0; } return 1;", "ERROR: 'dontexist' is not a known type name");
	TEST_FOR_FAIL("Class externally defined method 2", "int int:(){ return *this; } return 1;", "ERROR: function name expected after ':' or '.'");

	TEST_FOR_FAIL("Member variable or function is not found", "int a; a.b; return 1;", "ERROR: member variable or function 'b' is not defined in class 'int'");

	TEST_FOR_FAIL("Inplace array element type mismatch", "auto a = { 12, 15.0 };", "ERROR: array element 2 type 'double' doesn't match 'int'");
	TEST_FOR_FAIL("Ternary operator void return type", "void f(){} int a = 1; return a ? f() : 0.0;", "ERROR: can't find common type between 'void' and 'double'");
	TEST_FOR_FAIL("Ternary operator return type difference", "import std.math; int a = 1; return a ? 12 : float2(3, 4);", "ERROR: can't find common type between 'int' and 'float2'");

	TEST_FOR_FAIL("Variable type is unknown 1", "int test(int a, typeof(test) ptr){ return ptr(a, ptr); }", "ERROR: unknown identifier 'test'");
	TEST_FOR_FAIL("Variable type is unknown 2", "auto foo(@int x, auto y = typeof(foo)){ int y = x; return y; }", "ERROR: unknown identifier 'foo'");
	TEST_FOR_FAIL("Variable type is unknown 3", "void foo(){} auto foo(@int x, auto y = typeof(foo)){ int y = x; return y; } foo(1);", "ERROR: ambiguity, there is more than one overloaded function available:");

	TEST_FOR_FAIL("Illegal pointer operation 1", "int ref a; a += a;", "ERROR: can't find function '+' with following arguments:");
	TEST_FOR_FAIL("Illegal pointer operation 2", "int ref a; a++;", "ERROR: increment is not supported on 'int ref'");
	TEST_FOR_FAIL("Illegal pointer operation 3", "int ref a; a = a * 5;", "ERROR: can't find function '*' with following arguments:");
	TEST_FOR_FAIL("Illegal pointer operation 4", "-new char;", "ERROR: unary operation '-' is not supported on 'char ref'");
	TEST_FOR_FAIL("Illegal class operation", "import std.math; float2 v; v = ~v;", "ERROR: unary operation '~' is not supported on 'float2'");

	TEST_FOR_FAIL("Default function argument type mismatch", "import std.math;int f(int v = float3(3, 4, 5)){ return v; }return f();", "ERROR: cannot convert 'float3' to 'int'");
	TEST_FOR_FAIL("Default function argument type mismatch 2", "void func(){} int test(int a = func()){ return a; } return 0;", "ERROR: cannot convert 'void' to 'int'");
	TEST_FOR_FAIL("Default function argument of void type", "void func(){} int test(auto a = func()){ return a; } return 0;", "ERROR: r-value type is 'void'");

	TEST_FOR_FAIL("Undefined function call in function arguments", "int func(int a = func()){ return 0; } return 0;", "ERROR: unknown identifier 'func'");
	TEST_FOR_FAIL("Property set function is missing", "int int.test(){ return *this; } int a; a.test = 5; return a.test;", "ERROR: cannot change immutable value of type int");
	TEST_FOR_FAIL("Illegal comparison", "return \"hello\" > 12;", "ERROR: can't find function '>' with following arguments:");
	TEST_FOR_FAIL("Illegal array element", "auto a = { {15, 12 }, 14, {18, 48} };", "ERROR: cannot convert 'int' to 'int[]'");
	TEST_FOR_FAIL("Wrong return type", "int ref a(){ float b=5; return &b; } return 9;", "ERROR: cannot convert 'float ref' to 'int ref'");

	TEST_FOR_FAIL("Wrong return type", "auto foo(int ref() f){ return f(); } foo(<>{ auto x = {1, 2, 3}; });", "ERROR: function must return a value of type 'int'");
	TEST_FOR_FAIL("Wrong return type", "auto foo(int ref() f){ return f(); } auto x = foo(<>{ void u; u; });", "ERROR: function must return a value of type 'int'");

	TEST_FOR_FAIL("Global variable size limit", "char[32*1024*1024] arr;", "ERROR: variable size limit exceeded");
	TEST_FOR_FAIL("Unsized array initialization", "char[] arr = 1;", "ERROR: cannot convert 'int' to 'char[]'");
	
	TEST_FOR_FAIL("Invalid array index type A", "int[100] arr; void func(){} arr[func()] = 5;", "ERROR: cannot convert 'void' to 'int'");
	TEST_FOR_FAIL("Invalid array index type B", "import std.math; float2 a; int[100] arr; arr[a] = 7;", "ERROR: cannot convert 'float2' to 'int'");

	TEST_FOR_FAIL("None of the types implement method", "int i = 0; auto ref u = &i; return u.value();", "ERROR: function 'value' is undefined in any of existing classes");
	TEST_FOR_FAIL("None of the types implement correct method", "int i = 0; int int:value(){ return *this; } auto ref u = &i; return u.value(15);", "ERROR: can't find function 'int::value' with following arguments:");

	TEST_FOR_FAIL("Operator overload with no arguments", "int operator+(){ return 5; }", "ERROR: operator '+' definition must accept one or two arguments");

	TEST_FOR_FAIL("new auto;", "auto a = new auto;", "ERROR: can't allocate objects of type 'auto'");
	TEST_FOR_FAIL("new void;", "auto a = new void;", "ERROR: can't allocate objects of type 'void'");
	TEST_FOR_FAIL("new void[];", "auto a = new void[8];", "ERROR: can't allocate objects of type 'void'");
	TEST_FOR_FAIL("new array", "int a = 10; new (typeof(1)[a])(1);", "ERROR: can't provide constructor arguments to array allocation");
	TEST_FOR_FAIL("new array", "int a = 10; new (typeof(1)[a]){ *this = 1; };", "ERROR: can't provide custom construction code for array allocation");

	TEST_FOR_FAIL("Array underflow 2", "int[7][3] uu; uu[2][1] = 100; int[][3] kk = uu; return kk[2][-1000000];", "ERROR: array index cannot be negative");
	TEST_FOR_FAIL("Array overflow 2", "int[7][3] uu; uu[2][1] = 100; int[][3] kk = uu; return kk[2][1000000];", "ERROR: array index out of bounds");

	TEST_FOR_FAIL("Invalid conversion", "int a; int[] arr = new int[10]; arr = &a; return 0;", "ERROR: cannot convert 'int ref' to 'int[]'");

	TEST_FOR_FAIL("Usage of an undefined class", "class Foo{ Foo a; int i; } Foo a; return 1;", "ERROR: type 'Foo' is not fully defined");

	TEST_FOR_FAIL("Can't yield if not a coroutine", "int test(){ yield 4; } return test();", "ERROR: yield can only be used inside a coroutine");

	TEST_FOR_FAIL("Operation unsupported on reference 1", "int x; int ref y = &x; return *(y++);", "ERROR: increment is not supported on 'int ref'");
	TEST_FOR_FAIL("Operation unsupported on reference 2", "int x; int ref y = &x; return *++y;", "ERROR: increment is not supported on 'int ref'");
	TEST_FOR_FAIL("Operation unsupported on reference 3", "int x; int ref y = &x; return *(y--);", "ERROR: decrement is not supported on 'int ref'");
	TEST_FOR_FAIL("Operation unsupported on reference 4", "int x; int ref y = &x; return *--y;", "ERROR: decrement is not supported on 'int ref'");

	TEST_FOR_FAIL("Constructor returns a value", "auto int:int(int x, y){ *this = x; return this; } return *new int(4, 8);", "ERROR: type constructor return type must be 'void'");

const char	*testCompileTimeNoReturn =
"int foo(int m)\r\n\
{\r\n\
	if(m == 0)\r\n\
		return 1;\r\n\
}\r\n\
int[foo(3)] arr;";
	TEST_FOR_FAIL("Compile time function evaluation doesn't return.", testCompileTimeNoReturn, "ERROR: function didn't return a value");

	TEST_FOR_FAIL("Inline function with wrong type", "int foo(int ref(int) x){ return x(4); } foo(auto(){});", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("Function argument already defined", "int foo(int x, x){ return x + x; } return foo(5, 4);", "ERROR: name 'x' is already taken for a variable in current scope");

	TEST_FOR_FAIL("Read-only member", "int[] arr; arr.size = 10; return arr.size;", "ERROR: cannot change immutable value of type int");
	TEST_FOR_FAIL("Read-only member", "auto ref x; x.type = int; return 1;", "ERROR: cannot change immutable value of type typeid");
	TEST_FOR_FAIL("Read-only member", "auto ref x, y; x.ptr = y.ptr; return 1;", "ERROR: cannot convert from void ref to void");
	TEST_FOR_FAIL("Read-only member", "auto[] x; x.type = int; return 1;", "ERROR: cannot change immutable value of type typeid");
	TEST_FOR_FAIL("Read-only member", "auto[] x; x.size = 10; return 1;", "ERROR: cannot change immutable value of type int");
	TEST_FOR_FAIL("Read-only member", "auto[] x, y; x.ptr = y.ptr; return 1;", "ERROR: cannot convert from void ref to void");
	TEST_FOR_FAIL("Read-only member", "int[] x = new int[2]; (&x.size)++; return x.size;", "ERROR: cannot get address of the expression");
	TEST_FOR_FAIL("Read-only member", "int[] x = new int[2]; (&x.size)--; return x.size;", "ERROR: cannot get address of the expression");
	TEST_FOR_FAIL("Read-only member", "auto[] x = new int[2]; (&x.size)++; return x.size;", "ERROR: cannot get address of the expression");
	TEST_FOR_FAIL("Read-only member", "auto[] x = new int[2]; (&x.size)--; return x.size;", "ERROR: cannot get address of the expression");
	TEST_FOR_FAIL("Read-only member", "int[] x = new int[2]; (auto(int ref x){ return x; })(&x.size) = 56; return x.size;", "ERROR: cannot get address of the expression");
	TEST_FOR_FAIL("Read-only member", "int[] x = { 1, 2 }; typedef int[] wrap; void wrap:rewrite(int x){ size = x; } x.rewrite(2048); return x[1000];", "ERROR: cannot change immutable value of type int");
	TEST_FOR_FAIL("Read-only member", "auto[] x = { 1, 2 }; typedef auto[] wrap; void wrap:rewrite(){ type = long; } x.rewrite(); return long(x[0]);", "ERROR: cannot change immutable value of type typeid");

	nullcLoadModuleBySource("test.redefinitionPartA", "int foo(int x){ return -x; }");
	nullcLoadModuleBySource("test.redefinitionPartB", "int foo(int x){ return ~x; }");
	TEST_FOR_FAIL("Module function redefinition", "import test.redefinitionPartA; import test.redefinitionPartB; return foo();", "ERROR: function foo (type int ref(int)) is already defined. While importing test/redefinitionPartB.nc");

	TEST_FOR_FAIL("number constant overflow (hex)", "return 0x1deadbeefdeadbeef;", "ERROR: overflow in hexadecimal constant");
	TEST_FOR_FAIL("number constant overflow (oct)", "return 02777777777777777777777;", "ERROR: overflow in octal constant");
	TEST_FOR_FAIL("number constant overflow (bin)", "return 011111111111111111111111111111111111111111111111111111111111111111b;", "ERROR: overflow in binary constant");

	TEST_FOR_FAIL("variable with class name", "class Test{} int Test = 5; return Test;", "ERROR: name 'Test' is already taken for a type");

	TEST_FOR_FAIL("List comprehension of void type", "auto fail = { for(;0;){ yield; } };", "ERROR: cannot generate an array of 'void' element type");
	TEST_FOR_FAIL("List comprehension of unknown type", "auto fail = { for(;0;){} };", "ERROR: not a single element is generated, and an array element type is unknown");

	TEST_FOR_FAIL("Non-coroutine as an iterator 1", "int foo(){ return 1; } for(int i in foo) return 1;", "ERROR: function is not a coroutine");
	TEST_FOR_FAIL("Non-coroutine as an iterator 2", "auto omg(int z){ int foo(){ return z; } for(i in foo) return 1; } omg(1);", "ERROR: function is not a coroutine");

	TEST_FOR_FAIL("Dereferencing non-pointer", "int a = 5; return *a;", "ERROR: cannot dereference type 'int' that is not a pointer");

	TEST_FOR_FAIL("Short function outside argument list", "return <x>{ return 5; };", "ERROR: cannot infer type for inline function outside of the function call");
	
	TEST_FOR_FAIL("Short function outside argument list",
"void bar(void ref() x){ x(); }\r\n\
return bar(auto(){ int a = <x>{ return 5; }; });", "ERROR: cannot infer type for inline function outside of the function call");
	
	TEST_FOR_FAIL("Parent function not found", "void bar(void ref() x){ x(); } return bar(<x>{ return -x; }, 5);", "ERROR: cannot find function which accepts a function with 1 argument(s) as an argument #1");

	TEST_FOR_FAIL("Argument is not a function",
"int bar(int x, int y){ return x + y; }\r\n\
return bar(<x>{ return -x; }, 5);", "ERROR: cannot find function which accepts a function with 1 argument(s) as an argument #1");
	
	TEST_FOR_FAIL("Wrong argument count",
"int bar(int ref(double) f, double y){ return f(y); }\r\n\
return bar(<>{ return -x; }, 5);", "ERROR: cannot find function which accepts a function with 0 argument(s) as an argument #1");

	TEST_FOR_FAIL("No expression list in short inline function", "int caller(int ref(int) f){ return f(5); } return caller(<x>{});", "ERROR: function must return a value of type 'int'");
	TEST_FOR_FAIL("One expression in short function if not a pop node", "int caller(int ref(int) f){ return f(5); } return caller(<x>{ if(x){} });", "ERROR: function must return a value of type 'int'");
	TEST_FOR_FAIL("Multiple expressions in short function without a pop node", "int caller(int ref(int) f){ return f(5); } return caller(<x>{ if(x){} if(x){} });", "ERROR: function must return a value of type 'int'");

	TEST_FOR_FAIL("Using an argument of a function in definition in expression", "auto foo(int a, int b = a){ return a + b; } return foo(5, 7);", "ERROR: unknown identifier 'a'");

	TEST_FOR_FAIL("Short inline function at generic argument position", "auto foo(generic a, b, generic f){ return f(a, b); } return foo(5, 4, <x,y>{ x * y; });", "ERROR: cannot find function which accepts a function with 2 argument(s) as an argument #3");

	TEST_FOR_FAIL("typeof from a combination of generic arguments", "auto sum(generic a, b, typeof(a*b) c){ return a + b; } return sum(3, 4.5, double);", "ERROR: can't find function 'sum' with following arguments:");

	TEST_FOR_FAIL("coroutine cannot be a member function", "class Foo{} coroutine int Foo:bar(){ yield 1; return 0; } return 1;", "ERROR: coroutine cannot be a member function");

	TEST_FOR_FAIL("error in generic function body", "auto sum(generic a, b, c){ return a + b + ; } return sum(3, 4.5, 5l);", "ERROR: expression not found after binary operation");
	TEST_FOR_FAIL_GENERIC("genric function accessing variable that is defined later", "int y = 4; auto foo(generic a){ return i + y; } int i = 2; return foo(4);", "ERROR: unknown identifier 'i'", "while instantiating generic function foo(generic)");

	TEST_FOR_FAIL("multiple function prototypes", "int foo(); int foo(); int foo(){ return 1; } return 1;", "ERROR: function is already defined");
	TEST_FOR_FAIL("function prototype after definition", "int foo(){ return 1; } int foo(); return 1;", "ERROR: function 'foo' is being defined with the same set of arguments");
	TEST_FOR_FAIL("function prototype coroutine mismatch", "int foo(); coroutine int foo(){ return 2; } return foo();", "ERROR: function prototype was not a coroutine");
	TEST_FOR_FAIL("function prototype coroutine mismatch", "coroutine int foo(); int foo(){ return 2; } return foo();", "ERROR: function prototype was a coroutine");

	TEST_FOR_FAIL("unimplemented local function", "int foo(){ int bar(); return bar(); } return foo();", "ERROR: local function 'bar' went out of scope unimplemented");

	TEST_FOR_FAIL("wrong implementation scope", "int foo(int x){ int bar(); int y = bar(); int help(){ int bar(){ return -x; } return bar(); } help(); return y; } return foo(5);", "ERROR: function 'bar' is being defined with the same set of arguments");

	TEST_FOR_FAIL("typeid of auto", "typeid a = auto;", "ERROR: cannot take typeid from auto type");
	TEST_FOR_FAIL("typeid of auto", "typeof(auto);", "ERROR: cannot take typeid from auto type");
	TEST_FOR_FAIL("conversion from void to basic type", "int foo(int a){ return -a; } void bar(){} return foo(bar());", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("buffer overrun prevention", "int foo(generic a){ return -; /* %s %s %s %s %s %s %s %s %s %s %s %s %s %s */ } return foo(1);", "ERROR: expression not found after '-'");
	TEST_FOR_FAIL("Infinite instantiation recursion", "auto foo(generic a){ typeof(a) ref x; return foo(x); } return foo(1);", "ERROR: reached maximum generic function instance depth (64)");
	TEST_FOR_FAIL("Infinite instantiation recursion 2", "auto foo(generic a){ typeof(a) ref(typeof(a) ref, typeof(a) ref, typeof(a) ref) x; return foo(x); } return foo(1);", "ERROR: generated function type name exceeds maximum type length '8192'");
	TEST_FOR_FAIL("Infinite instantiation recursion 3", "class Foo<T>: Foo<T ref>{} Foo<int> a;", "ERROR: reached maximum generic type instance depth (64)");
	TEST_FOR_FAIL("auto resolved to void", "void foo(){} auto x = foo();", "ERROR: r-value type is 'void'");
	TEST_FOR_FAIL("unclear decision at return", "int foo(int x){ return -x; } int foo(float x){ return x * 2.0f; } auto bar(){ return foo; }", "ERROR: ambiguity, there is more than one overloaded function available:");

	TEST_FOR_FAIL("unclear function overload resolution", "auto foo(generic x){ return x + foo; } foo(1);", "ERROR: can't find function '+' with following arguments:");

	TEST_FOR_FAIL_FULL("unclear target function",
"int foo(int x){ return -x; }\r\n\
int foo(float x){ return x * 2; }\r\n\
int bar(int ref(int) f){ return f(5); }\r\n\
int bar(int ref(float) f){ return f(10); }\r\n\
return bar(foo);",
"ERROR: ambiguity, there is more than one overloaded function available for the call:\n\
  bar(int ref(float) or int ref(int))\n\
 candidates are:\n\
  int bar(int ref(float))\n\
  int bar(int ref(int))\n\
\n\
  at line 5: 'return bar(foo);'\n\
                        ^\n");

	TEST_FOR_FAIL_FULL("no target function",
"int foo(double x){ return -x; }\r\n\
int foo(float x){ return x * 2; }\r\n\
int bar(int ref(int) f){ return f(5); }\r\n\
return bar(foo);",
"ERROR: can't find function 'bar' with following arguments:\n\
  bar(int ref(float) or int ref(double))\n\
 the only available are:\n\
  int bar(int ref(int))\n\
\n\
  at line 4: 'return bar(foo);'\n\
                        ^\n");

	TEST_FOR_FAIL("generic function instance type unknown", "auto y = auto(generic y){ return -y; };", "ERROR: cannot instantiate generic function, because target type is not known");

	TEST_FOR_FAIL_FULL("cannot instance function in argument list", "int foo(int f){ return f; }\r\nreturn foo(auto(generic y){ return -y; });",
"ERROR: can't find function 'foo' with following arguments:\n\
  foo(auto ref(generic))\n\
 the only available are:\n\
  int foo(int)\n\
\n\
  at line 2: 'return foo(auto(generic y){ return -y; });'\n\
                        ^\n");

	TEST_FOR_FAIL("cannot instance function because target type is not a function", "int x = auto(generic y){ return -y; }; return x;", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("cannot select function overload", "int foo(int x){ return -x; } int foo(float x){ return x * 2; } int x = foo;", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("Instanced inline function is wrong", "class X{} X x; int foo(int ref(int, X) f){ return f(5, x); } return foo(auto(generic y, double z){ return -y; });", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("no finalizable objects on stack", "class Foo{int a;} void Foo:finalize(){} Foo x;", "ERROR: cannot create 'Foo' that implements 'finalize' on stack");
	TEST_FOR_FAIL("no finalizable objects on stack 2", "class Foo{int a;} void Foo:finalize(){} auto x = *(new Foo);", "ERROR: cannot create 'Foo' that implements 'finalize' on stack");
	TEST_FOR_FAIL("no finalizable objects on stack 3", "class Foo{int a;} void Foo:finalize(){} Foo[10] arr;", "ERROR: class 'Foo' implements 'finalize' so only an unsized array type can be created");
	TEST_FOR_FAIL("no finalizable objects on stack 4", "class Foo{int a;} void Foo:finalize(){} class Bar{ Foo z; }", "ERROR: cannot create 'Foo' that implements 'finalize' on stack");

	if(!nullcLoadModuleBySource("test.import_typedef1a", "class Foo{ int bar; }"))
		printf("Failed to create module test.import_typedef1a\n");
	if(!nullcLoadModuleBySource("test.import_typedef1b", "typedef int Foo;"))
		printf("Failed to create module test.import_typedef1b\n");
	TEST_FOR_FAIL("type alias collision with class", "import test.import_typedef1a; import test.import_typedef1b; return 1;", "ERROR: type 'int' alias 'Foo' is equal to previously imported class");
	
	if(!nullcLoadModuleBySource("test.import_typedef2a", "typedef float Foo;"))
		printf("Failed to create module test.import_typedef2a\n");
	if(!nullcLoadModuleBySource("test.import_typedef2b", "typedef int Foo;"))
		printf("Failed to create module test.import_typedef2b\n");
	TEST_FOR_FAIL("type alias collision with class", "import test.import_typedef2a; import test.import_typedef2b; return 1;", "ERROR: type 'int' alias 'Foo' is equal to previously imported alias");

	TEST_FOR_FAIL("generic type too many arguments", "class Foo<T>{ T a; } Foo<int, float, int> a;", "ERROR: type has only '1' generic argument(s) while '3' specified");
	TEST_FOR_FAIL("generic type wrong argument count", "class Foo<T, U>{ T a; } Foo<int> a;", "ERROR: there where only '1' argument(s) to a generic type that expects '2'");
	TEST_FOR_FAIL("generic instance type invisible after instance", "class Foo<T>{ T x, y, z; } Foo<int> x; T a; return a;", "ERROR: 'T' is not a known type name");

	TEST_FOR_FAIL("generic type used type in definition 1", "class Foo<T>{ T x; Bar<float> y; } class Bar<T>{ Foo<T ref> x; } Foo<Bar<int> > a; return 0;", "ERROR: 'Bar' is not a known type name");
	TEST_FOR_FAIL("generic type used type in definition 2", "class Foo<T>{ T x; Bar<T ref> y; } class Bar<T>{ Foo<T ref> x; } Foo<Bar<int> > a; return 0;", "ERROR: 'Bar' is not a known type name");

	TEST_FOR_FAIL_GENERIC("generic type function scope",
"class Foo<T>\r\n\
{\r\n\
	T x, y, z;\r\n\
	int foo()\r\n\
	{\r\n\
		return c;\r\n\
	}\r\n\
}\r\n\
int c = 10;\r\n\
Foo<int> x;\r\n\
return x.foo();", "ERROR: unknown identifier 'c'", "while instantiating generic type Foo<int>");

	TEST_FOR_FAIL_GENERIC("generic type function scope 2",
"class Foo<T>\r\n\
{\r\n\
	T x, y, z;\r\n\
	int foo()\r\n\
	{\r\n\
		return c;\r\n\
	}\r\n\
}\r\n\
int bar()\r\n\
{\r\n\
	int ken()\r\n\
	{\r\n\
		int c = 5;\r\n\
		Foo<int> x;\r\n\
		return x.foo();\r\n\
	}\r\n\
	return ken();\r\n\
}\r\n\
return bar();", "ERROR: unknown identifier 'c'", "while instantiating generic type Foo<int>");

	TEST_FOR_FAIL_GENERIC("generic type function scope 3",
"class Foo<T>\r\n\
{\r\n\
	T x, y, z;\r\n\
}\r\n\
int Foo:foo()\r\n\
{\r\n\
	return c;\r\n\
}\r\n\
int c = 10;\r\n\
Foo<int> x;\r\n\
return x.foo();", "ERROR: unknown identifier 'c'", "while instantiating generic function Foo::foo()");

	TEST_FOR_FAIL("generic type name is too long", 
"class Pair<T, U>{ T i; U j; }\r\n\
Pair<int ref(int ref(int, int), int ref(int, int)), int ref(int ref(int, int), int ref(int, int))> x;\r\n\
Pair<typeof(x), typeof(x)> x1;\r\n\
Pair<typeof(x1), typeof(x1)> x2;\r\n\
Pair<typeof(x2), typeof(x2)> x3;\r\n\
Pair<typeof(x3), typeof(x3)> x4;\r\n\
Pair<typeof(x4), typeof(x4)> x5;\r\n\
Pair<typeof(x5), typeof(x5)> x6;\r\n\
Pair<typeof(x6), typeof(x6)> x7;\r\n\
return sizeof(x7);", "ERROR: generated type name exceeds maximum type length '8192'");

	TEST_FOR_FAIL("generic type member function doesn't exist", "class Foo<T>{ T a; void foo(){} } Foo<int> x; x.food();", "ERROR: member variable or function 'food' is not defined in class 'Foo<int>'");

	TEST_FOR_FAIL_FULL("complex function specialization fail",
"class Foo<T, U>{ T x; U y; }\r\n\
auto foo(Foo<int, generic> a){ return a.x * a.y; }\r\n\
Foo<int, int> b;\r\n\
Foo<float, float> c;\r\n\
b.x = 6; b.y = 1; c.x = 2; c.y = 1.5;\r\n\
return int(foo(b) + foo(c));",
"ERROR: can't find function 'foo' with following arguments:\n\
  foo(Foo<float,float>)\n\
 the only available are:\n\
  int foo(Foo<int,int>)\n\
  auto foo(Foo<int,generic>) (wasn't instanced here)\n\
\n\
  at line 6: 'return int(foo(b) + foo(c));'\n\
                                     ^\n");

	TEST_FOR_FAIL("generic function specialization fail", "class Bar{ typedef int ref iref; } class Foo<T>{ T x; } auto foo(Foo<generic> a){ return a.x; } Bar z; return foo(z);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("generic function specialization fail 2", "class Foo<T>{ T x; } auto foo(Foo<generic> a){ return a.x; } return foo(5);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("generic function specialization fail 3", "class Bar<T>{ T ref ref y; } class Foo<T>{ T x; } auto foo(Foo<generic> a){ return a.x; } Bar<float> z; return foo(z);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("generic function specialization fail 4", "class Foo<T>{ T x; }auto foo(Foo<generic, int> a){ return a.x; }Foo<float> z;return foo(z);", "ERROR: type has only '1' generic argument(s) while '2' specified");
	TEST_FOR_FAIL("generic function specialization fail 5", "class Foo<T, U>{ T x; } auto foo(Foo<generic> a){ return a.x; } Foo<int, int> z; return foo(z);", "ERROR: there where only '1' argument(s) to a generic type that expects '2'");

	TEST_FOR_FAIL("generic function specialization alias double", "class Foo<T, U>{ T x; } auto foo(Foo<@T, @T> x){ T y = x.x; return y + x.x; } Foo<int, float> a; return foo(a);", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL(">> after a non-nested generic type name", "class Foo<T>{ T t; } int c = 1; int a = Foo<int>> c;", "ERROR: unknown identifier 'Foo'");

	TEST_FOR_FAIL("generic instance type invisible after instance 2", "class Foo<T>{ T x; } Foo<int> x; Foo<int> y; T a;", "ERROR: 'T' is not a known type name");

	TEST_FOR_FAIL("generic function specialization fail 6", "class Foo<T>{ T x; } Foo<int> a; a.x = 5; auto foo(Foo<generic> m){ return -m.x; } return foo(&a);", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("external function definition syntax inside a type 1", "class Foo{ void Foo:foo(){} }", "ERROR: class name repeated inside the definition of class");
	TEST_FOR_FAIL("external function definition syntax inside a type 2", "class Bar{} class Foo{ void Bar:foo(){} }", "ERROR: cannot define class 'Bar' function inside the scope of class 'Foo'");

	TEST_FOR_FAIL_FULL("function pointer selection fail", "void foo(int a){} void foo(double a){}\r\nauto a = foo;\r\nreturn 1;",
"ERROR: ambiguity, there is more than one overloaded function available:\n\
 candidates are:\n\
  void foo(double)\n\
  void foo(int)\n\
\n\
  at line 2: 'auto a = foo;'\n\
                   ^\n");

TEST_FOR_FAIL_FULL("function pointer selection fail 2",
"class Foo<T>{ T a; auto foo(){ return -a; } }\r\n\
int Foo<double>:foo(){ return -a; }\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo; auto z = s.foo;\r\n\
return int(y() + z());",
"ERROR: ambiguity, there is more than one overloaded function available:\n\
 candidates are:\n\
  int Foo<double>::foo()\n\
  double Foo<double>::foo()\n\
\n\
  at line 4: 'auto y = x.foo; auto z = s.foo;'\n\
                                   ^\n");

	TEST_FOR_FAIL("short inline function fail in variable 1", "int foo(int x){ return 2 * x; } int bar(int a, int ref(double) y){ return y(5.0); } auto x = bar; return x(foo, <x>{ -x; });", "ERROR: cannot convert 'int ref(int)' to 'int'");
	TEST_FOR_FAIL("short inline function fail in variable 2", "int bar(int ref(double) y){ return y(5.0); } auto x = bar; int foo(int x){ return x(<x>{ -x; }); }", "ERROR: cannot find function which accepts a function with 1 argument(s) as an argument #1");
	TEST_FOR_FAIL("short inline function fail in variable 3", "int bar(int ref(double) y){ return y(5.0); } auto x = bar; int foo(){ return x(1, <x>{ -x; }); }", "ERROR: cannot find function which accepts a function with 1 argument(s) as an argument #2");

	TEST_FOR_FAIL("generic type member function doesn't exist", "class Foo<T>{ T x; } Foo<int> y; auto z = y.bar; return 1;", "ERROR: member variable or function 'bar' is not defined in class 'Foo<int>'");

	TEST_FOR_FAIL("typedef dies after a generic function instance in an incorrect scope", "auto foo(generic x){ typedef typeof(x) T; T a = x * 2; return a; } int bar(){ int x = foo(5); T y; return x; } return bar();", "ERROR: 'T' is not a known type name");

	TEST_FOR_FAIL("alias redefinition in generic type definition", "class Foo<T, T>{ T x; } Foo<int, double> m;", "ERROR: there is already a type or an alias with the same name");

	TEST_FOR_FAIL("generic instance type invisible after instance 3", "class Foo<T>{} void foo(Foo<@T> x){ T y; } Foo<int> a; foo(a); T x; return sizeof(x);", "ERROR: 'T' is not a known type name");

	TEST_FOR_FAIL("generic instance type invisible after instance 3", "class Foo<T>{ T x; } void foo(Foo<@T>[] x){ x.x = 5; } Foo<int> a; foo(a); return a.x;", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("wrong array index argument count 1", "int[16] arr; return arr[];", "ERROR: can't find function '[]' with following arguments:");
	TEST_FOR_FAIL("wrong array index argument count 2", "int[16] arr; return arr[2, 3];", "ERROR: can't find function '[]' with following arguments:");

	TEST_FOR_FAIL("foreach restores cycle depth", "int[4] arr; for(i in arr){} break;", "ERROR: break level is greater that loop depth");

	TEST_FOR_FAIL("short inline function in a place where function accepts generic function type specialization and one of arguments is generic", "auto average(generic ref(int, generic, int) f){ return f(8, 2.5, 3); } return average(<x, y, z>{ x * y + z; });", "ERROR: can't find function 'average' with following arguments:");

	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(generic ref(int, float) f){ return f(8, 3); } auto f1(int x, int z){ return x * z; } return average(f1);", "ERROR: can't find function 'average' with following arguments:");
	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(generic ref(int, float) f){ return f(8, 3); } auto f1(int x){ return x; } return average(f1);", "ERROR: can't find function 'average' with following arguments:");
	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(generic ref(int) f){ return f(8, 3); } auto f1(int x, int z){ return x + z; } return average(f1);", "ERROR: can't find function 'average' with following arguments:");

	TEST_FOR_FAIL("cannot find suitable function", "class Foo<T>{} auto Foo:average(generic ref(int, T) f){ return f(5, 4); } Foo<int> m; return m.average(<x, y>{ 5+x+y; }, 4);", "ERROR: can't find function 'Foo::average' with following arguments:");

	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(generic ref(generic, int) f){ return f(4.0, 2); } auto f2(float x, y){ return x * 2.5; } return average(f2);", "ERROR: can't find function 'average' with following arguments:");
	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(int ref(generic, int) f){ return f(2, 4); } auto f2(float x, y){ return x * 2.5; } return average(f2);", "ERROR: can't find function 'average' with following arguments:");

	TEST_FOR_FAIL("generic type constructor call without generic arguments", "class Foo<T>{ T curr; } auto Foo:Foo(int start){ curr = start; } Foo<int> a = Foo(5);", "ERROR: generic type arguments in <> are not found after constructor name");
	TEST_FOR_FAIL_GENERIC("generic type constructor call without generic arguments 2", "class list<T>{} class list_iterator<T>{} auto list_iterator:list_iterator(){} auto list:sum(){for(i in list_iterator()){}} list<int> arr; arr.sum();", "ERROR: generic type arguments in <> are not found after constructor name", "while instantiating generic function list::sum()");
	
	TEST_FOR_FAIL_GENERIC("generic type member function local function is local", "class Foo<T>{ T x; } auto Foo:foo(){ auto bar(){ return x; } return this.bar(); } Foo<int> m; return m.foo();", "ERROR: member variable or function 'bar' is not defined in class 'Foo<int>'", "while instantiating generic function Foo::foo()");

	TEST_FOR_FAIL_GENERIC("type aliases are taken from instance", "class Foo<T>{} auto Foo:foo(T key){ bar(10); } auto Foo:bar(T key){} Foo<char[]> map; map.foo(\"foo\"); return 1;", "ERROR: can't find function 'Foo::bar' with following arguments:", "while instantiating generic function Foo::foo(@T)");

	TEST_FOR_FAIL("wrong function return type", "int foo(int ref(generic) f){ return f(4); } auto f2(float x){ return x * 2.5; } return foo(f2);", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("unable to select function overload", "auto foo(generic x){ return -x; } int ref(int, double) a = foo;", "ERROR: can't resolve generic type 'auto ref(generic)' instance for 'int ref(int,double)'");

	TEST_FOR_FAIL("binary modify operation on floating point", "double a = 1; a <<= 2;", "ERROR: operation << is not supported on 'double' and 'int'");
	TEST_FOR_FAIL("binary modify operation on floating point", "float a = 1; a >>= 1;", "ERROR: operation >> is not supported on 'float' and 'int'");
	TEST_FOR_FAIL("binary modify operation on floating point", "double a = 1; a &= 5;", "ERROR: operation & is not supported on 'double' and 'int'");
	TEST_FOR_FAIL("binary modify operation on floating point", "float a = 1; a |= 5;", "ERROR: operation | is not supported on 'float' and 'int'");
	TEST_FOR_FAIL("binary modify operation on floating point", "double a = 1; a ^= 5;", "ERROR: operation ^ is not supported on 'double' and 'int'");

	TEST_FOR_FAIL("argument mistaken for generic", "auto foo(generic x, typeof(x) a, b){ } class Foo{} Foo a; foo(4, 4, a);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("argument mistaken for generic", "class Tuple<T, U>{ T x; U y; } auto operator==(Tuple<@T, @U> ref a, b){ } Tuple<int, int> a; return a == 5;", "ERROR: can't find function '==' with following arguments:");

	TEST_FOR_FAIL("multiple aliases with different type in argument list", "class Foo<T>{ void Foo(){} } auto foo(Foo<@T> a, b){} foo(Foo<int>(), Foo<double>());", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("typedef dies after a generic function instance 2", "class Foo<T>{ T x; } auto foo(Foo<@T> x, int ref(int, int) y){ return x.x * y(1, 2); } Foo<int> a; a.x = 2; assert(6 == foo(a, <i, j>{ i+j; })); T x; return x;", "ERROR: 'T' is not a known type name");

	TEST_FOR_FAIL("generic in an illegal context", "auto foo(generic x){} class Bar<T>{ T x; } auto Bar:bar(generic y = foo(Bar<T>())){} return 1;", "ERROR: can't cast to a generic type");
	TEST_FOR_FAIL("generic in an illegal context", "auto foo(generic x){} class Bar<T>{ T x; } auto Bar:bar(generic y = foo(T())){} return 1;", "ERROR: can't cast to a generic type");

	TEST_FOR_FAIL("operator with short-circuit requirement", "int operator||(int a, b){ return 0; }", "ERROR: operator '||' definition must accept a function returning desired type as the second argument");

	TEST_FOR_FAIL("constant couldn't be evaluated at compilation time", "int a = 4; class Foo{ const int b = a; }", "ERROR: expression didn't evaluate to a constant number");
	TEST_FOR_FAIL("name occupied", "class Foo{ const int a = 1, a = 3; }", "ERROR: name 'a' is already taken");

	TEST_FOR_FAIL("bool is non-negatable", "bool a = true; return -a;", "ERROR: unary operation '-' is not supported on 'bool'");
	TEST_FOR_FAIL("no bit not on bool", "bool a = true; return ~a;", "ERROR: unary operation '~' is not supported on 'bool'");
	TEST_FOR_FAIL("bool is non-negatable", "return -true;", "ERROR: unary operation '-' is not supported on 'bool'");
	TEST_FOR_FAIL("no bit not on bool", "return ~true;", "ERROR: unary operation '~' is not supported on 'bool'");

	TEST_FOR_FAIL("class prototype", "class Foo; Foo x;", "ERROR: type 'Foo' is not fully defined");
	TEST_FOR_FAIL("class prototype", "class Foo; Foo ref a = new Foo;", "ERROR: type 'Foo' is not fully defined");
	TEST_FOR_FAIL("class prototype", "class Foo; Foo ref a; auto x = *a;", "ERROR: type 'Foo' is not fully defined");
	TEST_FOR_FAIL("class prototype", "class Foo; return 1;", "ERROR: type 'Foo' is not fully defined");

	TEST_FOR_FAIL("class undefined", "class bar{ bar[12] arr; }", "ERROR: type 'bar' is not fully defined");
	TEST_FOR_FAIL("class prototype", "class foo; foo[1] f;", "ERROR: type 'foo' is not fully defined");

	TEST_FOR_FAIL("class prototype", "class foo; foo bar() { foo ref x; return *x; }", "ERROR: type 'foo' is not fully defined");
	TEST_FOR_FAIL("class prototype", "class foo; auto bar() { foo ref x; return *x; }", "ERROR: type 'foo' is not fully defined");

	TEST_FOR_FAIL_FULL("No single () that fits", "class Foo{} void operator()(int a){} void operator()(float a){} Foo a; a('a');",
"ERROR: can't find function '()' with following arguments:\n\
  ()(Foo, char)\n\
 the only available are:\n\
  void ()(float)\n\
  void ()(int)\n\
\n\
  at line 1: 'class Foo{} void operator()(int a){} void operator()(float a){} Foo a; a('a');'\n\
                                                                                      ^\n");

	TEST_FOR_FAIL_FULL("Unknown escape sequence", "auto x = \":xxx \\x\";",
"ERROR: unknown escape sequence\n\
  at line 1: 'auto x = \":xxx \\x\";'\n\
                             ^\n");

	TEST_FOR_FAIL("No constructor", "auto std() { return 1; } auto main() { return typeof(std)(1.0f); }", "ERROR: cannot convert 'float' to 'int ref()'");
	TEST_FOR_FAIL("Prototype is redeclared as generic", "class Foo; Foo ref a; class Foo<T, U>{ T x; U y; }", "ERROR: type 'Foo' was forward declared as a non-generic type");
	TEST_FOR_FAIL("Generic type redefinition", "class Foo<T>{} class Foo<T>{} return 1;", "ERROR: 'Foo' is being redefined");

	TEST_FOR_FAIL("restricted enum", "enum x { y = 54, z } int a = x.y;", "ERROR: cannot convert 'x' to 'int'");
	TEST_FOR_FAIL("restricted enum", "enum x { y = 54, z } x b = 67;", "ERROR: cannot convert 'int' to 'x'");
	TEST_FOR_FAIL("restricted enum", "enum x { y = 54, z } x c = x.y * 25;", "ERROR: can't find function '*' with following arguments:");
	TEST_FOR_FAIL("restricted enum", "enum x { y = 54, z } x foo(){ return 15; }", "ERROR: cannot convert 'int' to 'x'");
	TEST_FOR_FAIL("restricted enum", "enum x { y = 54, z } x c = x.y; c *= 25;", "ERROR: can't find function '*' with following arguments:");
	TEST_FOR_FAIL("restricted enum", "enum x { y = 54, z } x c = x.y; c++;", "ERROR: increment is not supported on 'x'");

	TEST_FOR_FAIL("no incorrect optimization for classes", "class Foo{ } Foo a = 0 + Foo(); return 1;", "ERROR: can't find function '+' with following arguments:");

	TEST_FOR_FAIL("test for bug in function call", "int foo(void ref() f, char[] x = \"x\", int a = 2){ return 5; } return foo(\"f\");", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("namespace error", "namespace Test{} class Test{}", "ERROR: name 'Test' is already taken for a namespace");
	TEST_FOR_FAIL("namespace error", "class Test{} namespace Test{}", "ERROR: name 'Test' is already taken for a type");
	TEST_FOR_FAIL("namespace error", "namespace Test{} class Test;", "ERROR: name 'Test' is already taken for a namespace");
	TEST_FOR_FAIL("namespace error", "class Test; namespace Test{}", "ERROR: name 'Test' is already taken for a type");
	TEST_FOR_FAIL("namespace error", "namespace Test{} int Test;", "ERROR: name 'Test' is already taken for a namespace");
	TEST_FOR_FAIL("namespace error", "int Test; namespace Test{}", "ERROR: name 'Test' is already taken for a variable in current scope");
	TEST_FOR_FAIL("namespace error", "namespace Test{ int foo(){ return 12; } } return foo();", "ERROR: unknown identifier 'foo'");
	TEST_FOR_FAIL("namespace error", "namespace Test{ namespace Nested{ int x; } } return Nested.x;", "ERROR: unknown identifier 'Nested'");

	TEST_FOR_FAIL("enum error", "namespace Test{} enum Test{ A }", "ERROR: name 'Test' is already taken for a namespace");
	TEST_FOR_FAIL("enum error", "enum Test{ A } namespace Test{}", "ERROR: name 'Test' is already taken for a type");
	TEST_FOR_FAIL("enum error", "enum bool{ True, False }", "ERROR: 'bool' is being redefined");

	TEST_FOR_FAIL("no biggy", "int[1024 * 1024 * 1024] f; f[3] = 0;", "ERROR: variable size limit exceeded");

	TEST_FOR_FAIL("namespace error", "int foo(){ namespace Test{ int x; } return Test.x; }", "ERROR: a namespace definition must appear either at file scope or immediately within another namespace definition");

	TEST_FOR_FAIL("reserved function", "int __newS(){ return 4; }", "ERROR: function '__newS' is reserved");
	TEST_FOR_FAIL("reserved function", "int __newA(){ return 4; }", "ERROR: function '__newA' is reserved");
	TEST_FOR_FAIL("reserved function", "auto x = __newS(1023, 23);", "ERROR: unknown identifier '__newS'");
	TEST_FOR_FAIL("reserved function", "auto x = __newA(5, 5, 5);", "ERROR: unknown identifier '__newA'");

	TEST_FOR_FAIL("restricted enum", "enum X{ Y, Z } int a(int x){ return x; } return a(X.Y);", "ERROR: can't find function 'a' with following arguments:");

	TEST_FOR_FAIL("void operation", "void foo(){} do{}while(!foo());", "ERROR: unary operation '!' is not supported on 'void'");
	TEST_FOR_FAIL("void operation", "void foo(){} void x = foo() + foo(); return 1;", "ERROR: first operand type is 'void'");
	TEST_FOR_FAIL("void operation", "void foo(){} void x = 5 + foo(); return 1;", "ERROR: second operand type is 'void'");

	TEST_FOR_FAIL("unresolved type", "auto foo(){ foo.a(); }", "ERROR: function 'foo' type is unresolved at this point");
	TEST_FOR_FAIL("unresolved type", "auto foo(){ foo.a; }", "ERROR: function 'foo' type is unresolved at this point");
	TEST_FOR_FAIL("unresolved type", "auto foo(){ &foo; }", "ERROR: function 'foo' type is unresolved at this point");
	TEST_FOR_FAIL("unresolved type", "class Foo{ } auto Foo:foo(){ auto m = this.foo; return m(); }", "ERROR: function 'Foo::foo' type is unresolved at this point");

	TEST_FOR_FAIL("no function", "int foo(@T[] arr){ return 1; } return foo(new int);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("no function", "class Bar<T>{} int foo(Bar<@T>[] arr){ return 1; } Bar<int> ref x; return foo(x);", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("clean-up after generic function instance failure",
"class Bar<T>{} class Foo<T>{}\r\n\
int foo(generic b, Bar<@T> a){ return 1; }\r\n\
int foo(generic b, Foo<@T> a){ return 2; }\r\n\
foo(5, Foo<int>());\r\n\
return b;", "ERROR: unknown identifier 'b'");

	TEST_FOR_FAIL("clean-up after generic function instance failure",
"class Bar<T>{} class Foo<T>{}\r\n\
int foo(@U b, Bar<@T> a){ return 1; }\r\n\
int foo(generic b, Foo<@T> a){ return 2; }\r\n\
foo(5, Foo<int>());\r\n\
U b; return b;", "ERROR: 'U' is not a known type name");

	TEST_FOR_FAIL("clean-up after generic function instance failure",
"class Bar<T>{} class Foo<T>{}\r\n\
class Test\r\n\
{\r\n\
	typedef int U;\r\n\
	int foo(generic b, Bar<@T> a){ return 1; }\r\n\
	int foo(generic b, Foo<@T> a){ return 2; }\r\n\
}\r\n\
Test x; x.foo(5, Foo<int>());\r\n\
U b; return b;", "ERROR: 'U' is not a known type name");

TEST_FOR_FAIL("clean-up after generic function instance failure",
"class Foo<T>{ int x; }\r\n\
auto foo(Foo<@T> x, @T y){ return x.x * y; }\r\n\
auto foo(Foo<@T> x, double y){ return x.x * y; }\r\n\
foo(Foo<int>(), 5.0);\r\n\
return y;", "ERROR: unknown identifier 'y'");

	TEST_FOR_FAIL_FULL("ambiguity",
"class A{ void run(char x){} }\r\n\
class B{ void run(float x){} }\r\n\
A a; B b; auto ref[2] arr; arr[0] = &a; arr[1] = &b;\r\n\
for(i in arr) i.run(5.0);\r\n\
return 0;",
"ERROR: ambiguity, there is more than one overloaded function available for the call:\n\
  A::run(double)\n\
 candidates are:\n\
  void A::run(char)\n\
  void B::run(float)\n\
\n\
  at line 4: 'for(i in arr) i.run(5.0);'\n\
                                 ^\n");

	TEST_FOR_FAIL("incorrect template type", "class Foo<T>{} Foo<auto> x;", "ERROR: 'auto' type cannot be used as template argument");

	TEST_FOR_FAIL("auto as base class", "class Foo : auto{}", "ERROR: type 'auto' is not extendable");
	TEST_FOR_FAIL("unfinished type as base class", "class Foo : Foo { }", "ERROR: 'Foo' is not a known type name");

	TEST_FOR_FAIL("constant fold unsafe", "int ref b = nullptr + 0x0808f00d; return *b;", "ERROR: can't find function '+' with following arguments:");
	TEST_FOR_FAIL("constant fold unsafe", "enum Foo{ A, B, C, D } enum Bar{ A, B, C, D } auto x = 1 + Bar.C;", "ERROR: can't find function '+' with following arguments:");

	TEST_FOR_FAIL("unknown instance type", "auto foo(generic x){} auto bar(generic x){} foo(bar);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("unknown instance type", "auto foo(generic x){} foo(auto(generic x){});", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("?: error message", "void foo(int x){} void bar(float x){} int a = 5; auto x = a ? foo : bar;", "ERROR: can't find common type between 'void ref(int)' and 'void ref(float)'");

	TEST_FOR_FAIL("?: exploit", "int foo = 0xdeadbeef; void ref bar = !foo ? nullptr : foo; int ref error = bar; *error;", "ERROR: cannot convert '__nullptr' to 'int'");

	TEST_FOR_FAIL("this exploit", "class X{ int a; } auto X:foo(){ auto bar(){ int this; return a; } return bar(); } X x; x.a = 4; return x.foo();", "ERROR: 'this' is a reserved keyword");

	TEST_FOR_FAIL("error with a generic function returning auto printout", "int foo(generic ref(generic, int) x){ return x(4, 5); } auto bar(int a, generic b){ return a + b; } return foo(bar, bar);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("function selection error printout failure", "auto x = duplicate;", "ERROR: ambiguity, there is more than one overloaded function available:");

	TEST_FOR_FAIL_FULL("better error description",
"void foo(generic x){ a.x; }\r\n\
foo(void bar(){});\r\n\
return 1;",
"ERROR: unknown identifier 'a'\n\
  at line 1: 'void foo(generic x){ a.x; }'\n\
                                   ^\n\
while instantiating generic function foo(generic)\n\
  using argument(s) (void ref())\n\
  at line 2: 'foo(void bar(){});'\n\
                 ^\n");

	TEST_FOR_FAIL("named function arguments", "int foo(int i, j){ return i / j; } int foo(int i, j, k = 3){ return i + j + k; } return foo(j: 5, i: 10);", "ERROR: ambiguity, there is more than one overloaded function available for the call:");
	TEST_FOR_FAIL("named function arguments", "int foo(int i, j){ return i / j; } return foo(i: 1, i: 2, i: 3, i: 4, j: 5, j: 6, j: 7, j: 8);", "ERROR: argument 'i' is already set");
	TEST_FOR_FAIL("named function arguments", "int foo(int i, j){ return i / j; } auto x = foo; return x(j: 5, i: 10);", "ERROR: function argument names are unknown at this point");
	TEST_FOR_FAIL("named function arguments", "int foo(int i, j){ return i / j; } return foo(10, i: 5);", "ERROR: argument 'i' is already set");

	TEST_FOR_FAIL("named function arguments", "int[10] arr; return arr[x: 1];", "ERROR: can't find function '[]' with following arguments:");

	TEST_FOR_FAIL("incorrect typeid", "return typeid(auto) == typeid(1);", "ERROR: cannot take typeid from auto type");

	TEST_FOR_FAIL("ambiguity (generic, named)", "class Test{ auto foo(generic i){ return -i; } auto foo(generic i, int j = 2){ return i + j; } } auto ref x = new Test; return x.foo(1);", "ERROR: ambiguity, there is more than one overloaded function available for the call:");

	TEST_FOR_FAIL("not extendable", "class A{ int x; } class B : A{ int y; }", "ERROR: type 'A' is not extendable");

	TEST_FOR_FAIL("unknown type for operation", "return -nullptr;", "ERROR: unary operation '-' is not supported on '__nullptr'");
	TEST_FOR_FAIL("unknown type for operation", "return ~nullptr;", "ERROR: unary operation '~' is not supported on '__nullptr'");
	TEST_FOR_FAIL("unknown type for operation", "return !nullptr;", "ERROR: unary operation '!' is not supported on '__nullptr'");

	TEST_FOR_FAIL("unknown type for operation", "auto ref x; return -x;", "ERROR: unary operation '-' is not supported on 'auto ref'");
	TEST_FOR_FAIL("unknown type for operation", "auto ref x; return ~x;", "ERROR: unary operation '~' is not supported on 'auto ref'");

	TEST_FOR_FAIL("fake nullptr", "void ref b; int ref a = b;", "ERROR: cannot convert 'void ref' to 'int ref'");
	TEST_FOR_FAIL("void value", "void ref b; return *b;", "ERROR: cannot dereference type 'void ref'");

	TEST_FOR_FAIL("explicit generic function types", "return foo with int();", "ERROR: '<' not found before explicit generic type alias list");
	TEST_FOR_FAIL("explicit generic function types", "return foo with<int,>();", "ERROR: type name is expected after ','");
	TEST_FOR_FAIL("explicit generic function types", "return foo with<>();", "ERROR: type name is expected after 'with'");
	TEST_FOR_FAIL("explicit generic function types", "return foo with<int;", "ERROR: '>' not found after explicit generic type alias list");
	TEST_FOR_FAIL("explicit generic function types", "return foo with<int>;", "ERROR: '(' is expected at this point");
	TEST_FOR_FAIL("explicit generic function types", "void foo<@T, U>(){}", "ERROR: '@' is expected after ',' in explicit generic type alias list");
	TEST_FOR_FAIL("explicit generic function types", "void foo<@>(){}", "ERROR: explicit generic type alias is expected after '@'");
	TEST_FOR_FAIL("explicit generic function types", "void foo<@T, @T>(){}", "ERROR: there is already an alias with the same name");
	TEST_FOR_FAIL("explicit generic function types", "void foo<@T, @U(){}", "ERROR: '>' expected after generic type alias list");
	TEST_FOR_FAIL("explicit generic function types", "void foo<@T, @U>{}", "ERROR: '(' expected after function name");

	TEST_FOR_FAIL("hasMember extended typeof", "int.hasMember x;", "ERROR: ';' not found after expression");
	TEST_FOR_FAIL("hasMember extended typeof", "int.hasMember();", "ERROR: member variable or function 'hasMember' is not defined in class 'typeid'");
	TEST_FOR_FAIL("hasMember extended typeof", "int.hasMember(x;", "ERROR: ')' not found after function argument list");

	TEST_FOR_FAIL("any type allocation", "new (int[]();", "ERROR: matching ')' not found after '('");
	TEST_FOR_FAIL("any type allocation", "new int ref(1, 2);", "ERROR: function 'int ref::int ref' that accepts 2 arguments is undefined");
	TEST_FOR_FAIL("any type allocation", "auto by_e = new (int[2])({1, 2, 3});", "ERROR: cannot convert 'int[3]' to 'int[2]'");
	TEST_FOR_FAIL("any type allocation", "new int ref(0);", "ERROR: cannot convert 'int' to 'int ref'");

	TEST_FOR_FAIL_FULL("correct instancing info",
"auto bar(@T x){}\r\n\
bar(1);\r\n\
bar(5.0);\r\n\
auto m = bar;",
"ERROR: ambiguity, there is more than one overloaded function available:\n\
 candidates are:\n\
  void bar(double)\n\
  void bar(int)\n\
  auto bar(@T)\n\
\n\
  at line 4: 'auto m = bar;'\n\
                   ^\n");

	TEST_FOR_FAIL("incorrect switch type combination", "switch(4){ case int: return 3; break; case float: return 1; } return 2;", "ERROR: can't find function '==' with following arguments:");

	TEST_FOR_FAIL("__function type", "__function x; x = 1024; return 1;", "ERROR: cannot convert 'int' to '__function'");

	TEST_FOR_FAIL("invalid index type", "int[nullptr] x;", "ERROR: cannot convert '__nullptr' to 'long'");

	TEST_FOR_FAIL("invalid allocation count type", "void foo(){} auto x = new int[foo];", "ERROR: cannot convert 'void ref()' to 'int'");

	TEST_FOR_FAIL("overloaded function misuse 1", "void f(int x){} void f(long x){} return 2 + f;", "ERROR: can't find function '+' with following arguments:");
	TEST_FOR_FAIL("overloaded function misuse 2", "void f(int x){} void f(long x){} auto x = { f };", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("overloaded function misuse 3", "void f(int x){} void f(long x){} return sizeof(f);", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("overloaded function misuse 4", "void f(int x){} void f(long x){} return typeof(f);", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("overloaded function misuse 5", "void f(int x){} void f(long x){} if(f){}", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("overloaded function misuse 6", "void f(int x){} void f(long x){} for(; f; ){}", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("overloaded function misuse 7", "void f(int x){} void f(long x){} while(f){}", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("overloaded function misuse 8", "void f(int x){} void f(long x){} do{}while(f);", "ERROR: ambiguity, there is more than one overloaded function available:");

	TEST_FOR_FAIL("incorrect function prototype implementation", "void foo(); { auto a = foo; void foo(){} auto b = foo; return a == b; }", "ERROR: function 'foo' is being defined with the same set of arguments");

	TEST_FOR_FAIL("generic function misuse 1", "return 2 + auto(@T x){};", "ERROR: can't find function '+' with following arguments:");
	TEST_FOR_FAIL("generic function misuse 2", "auto x = { auto(@T x){} };", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("generic function misuse 3", "return sizeof((auto(@T x){}));", "ERROR: cannot instantiate generic function, because target type is not known");
	TEST_FOR_FAIL("generic function misuse 4", "return typeof(auto(@T x){});", "ERROR: cannot instantiate generic function, because target type is not known");
	TEST_FOR_FAIL("generic function misuse 5", "if(auto(@T x){}){}", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("generic function misuse 6", "for(; auto(@T x){}; ){}", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("generic function misuse 7", "while(auto(@T x){}){}", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("generic function misuse 8", "do{}while(auto(@T x){});", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("generic function misuse 9", "for(i in auto(@T x){}){}", "ERROR: ambiguity, the expression is a generic function");
	
	TEST_FOR_FAIL("fuzzy test 1", "typedef auto Foo;", "ERROR: can't alias 'auto' type");
	TEST_FOR_FAIL("fuzzy test 2", "\"test\"[];", "ERROR: can't find function '[]' with following arguments:");
	TEST_FOR_FAIL("fuzzy test 3", "auto[sizeof(4)];", "ERROR: cannot specify array size for auto");
	TEST_FOR_FAIL("fuzzy test 4", "yield 1;", "ERROR: global yield is not allowed");
	TEST_FOR_FAIL("fuzzy test 5", "0in+2122;", "ERROR: can't find function 'in' with following arguments:");

	TEST_FOR_FAIL("Array to auto[] type conversion fail", "int x = 12; auto[] arr = x; return 0;", "ERROR: cannot convert 'int' to 'auto[]'");
	TEST_FOR_FAIL("auto[] type conversion mismatch 1", "auto str = \"Hello\"; auto[] arr = str; int str2 = arr; return 0;", "ERROR: cannot convert 'auto[]' to 'int'");

	TEST_FOR_FAIL("instantiation failure during short inline function argument resolve", "auto foo(generic ref(int) f, int ref(int) x){ return f(x(1)); } return foo(int, <x>{ -x; });", "ERROR: cannot find function which accepts a function with 1 argument(s) as an argument #2");
	TEST_FOR_FAIL("short inline function fail 1", "class Foo<T>{} auto foo(generic a, generic ref(Foo<generic>) m){} return foo(1, <x>{ -4; });", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("explicit function arguments", "int foo(explicit int a, b){ return a + b; } return foo(3, 4.0);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("explicit function arguments", "int foo(int a, explicit float b){ return a + b; } return foo(3, 4.0);", "ERROR: can't find function 'foo' with following arguments:");

	TEST_FOR_FAIL("function selection", "class Foo<T>{} auto foo(Foo<@T> x){} return foo(foo);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("function selection", "class Foo<T>{} +auto operator+(Foo<generic> ref v){ };", "ERROR: ambiguity, the expression is a generic function");

	TEST_FOR_FAIL("incorrect auto", "int foo(int ref(int, int) f){ return f(2, 3); } auto x = foo(<auto i, j>{ i + j; }); return x;", "ERROR: function argument cannot be an auto type");
	TEST_FOR_FAIL("incorrect auto", "auto bar<@T>(){ return 1; } bar with<auto>();", "ERROR: explicit generic argument type can't be auto");

	TEST_FOR_FAIL("virtual type 1", "void foo(int a){} void foo(int a, int b){} foo[0];", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("virtual type 1", "(auto(generic a){})[0];", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("virtual type 2", "void foo(int a){} void foo(int a, int b){} foo.a;", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("virtual type 2", "(auto(generic a){}).a;", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("virtual type 3", "void foo(int a){} void foo(int a, int b){} typeof(foo) a;", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("virtual type 3", "typeof(auto(generic a){}) a;", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("virtual type 4", "void foo(int a){} void foo(int a, int b){} typeid(foo);", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("virtual type 5", "typeid(auto(generic a){});", "ERROR: ambiguity, the expression is a generic function");

	TEST_FOR_FAIL("invalid generic type use 1", "auto x = new generic; auto y = *x;", "ERROR: generic type is not allowed");
	TEST_FOR_FAIL("invalid generic type use 2", "auto x = new @T; auto y = *x;", "ERROR: generic type is not allowed");
	TEST_FOR_FAIL("invalid generic type use 3", "class Foo<T>{} auto x = new Foo; *x;", "ERROR: generic type is not allowed");
	TEST_FOR_FAIL("invalid generic type use 4", "return sizeof(generic);", "ERROR: sizeof generic type is illegal");
	TEST_FOR_FAIL("invalid generic type use 5", "return sizeof(@T);", "ERROR: sizeof generic type is illegal");
	TEST_FOR_FAIL("invalid generic type use 6", "class Foo<T>{} return sizeof(Foo);", "ERROR: sizeof generic type is illegal");
	TEST_FOR_FAIL("invalid generic type use 7", "assert(generic);", "ERROR: cannot take typeid from generic type");
	TEST_FOR_FAIL("invalid generic type use 8", "assert(@T);", "ERROR: cannot take typeid from generic type");
	TEST_FOR_FAIL("invalid generic type use 9", "class Foo<T>{} assert(Foo);", "ERROR: cannot take typeid from generic type");
	TEST_FOR_FAIL("invalid generic type use 10", "return typeof(generic) == int;", "ERROR: cannot take typeid from generic type");
	TEST_FOR_FAIL("invalid generic type use 11", "return typeof(@T) == int;", "ERROR: cannot take typeid from generic type");
	TEST_FOR_FAIL("invalid generic type use 12", "class Foo<T>{} return typeof(Foo) == int;", "ERROR: cannot take typeid from generic type");
	TEST_FOR_FAIL("invalid generic type use 13", "generic foo(); return foo();", "ERROR: return type can't be generic");

	TEST_FOR_FAIL("non-value argument", "int f(typeid x){ return 1; } assert(typeof(f).argument);", "ERROR: expected '.first'/'.last'/'[N]'/'.size' after 'argument'");
	TEST_FOR_FAIL("unresolved type", "typeof(int ref(int, int).argument) a;", "ERROR: expected '.first'/'.last'/'[N]'/'.size' after 'argument'");
	
	TEST_FOR_FAIL("duplicate enum member name", "enum Bar{ A, A, C, D }", "ERROR: name 'A' is already taken");

	TEST_FOR_FAIL("invalid cast", "int ref(bool ref) x = int f(bool ref x){ return *x; }; return x(2);", "ERROR: cannot convert 'int' to 'bool ref'");

	TEST_FOR_FAIL("scope switch restore", "auto bar(generic x){ retprn x; } class Foo<T>{} int Foo:foo(typeof(bar(1)) e){ return 1; } Foo<int ref> test2; return test2.foo(1);", "ERROR: can't find function 'Foo::foo' with following arguments:");

	TEST_FOR_FAIL("invalid array element size 1", "int[2][100000] a;", "ERROR: array element size cannot exceed 65535 bytes");
	TEST_FOR_FAIL("invalid array element size 2", "int[][100000] a;", "ERROR: array element size cannot exceed 65535 bytes");
	TEST_FOR_FAIL("invalid array element size 3", "int[100000] b; typeof(b)[2] a2;", "ERROR: array element size cannot exceed 65535 bytes");
	TEST_FOR_FAIL("invalid array element size 4", "int[100000] b; typeof(b)[] a2;", "ERROR: array element size cannot exceed 65535 bytes");
	TEST_FOR_FAIL("invalid array element size 5", "int[100000] b; auto x = { b, b };", "ERROR: array element size cannot exceed 65535 bytes");
	TEST_FOR_FAIL("invalid array element size 6", "auto x = new (int[100000])[2];", "ERROR: array element size cannot exceed 65535 bytes");

	TEST_FOR_FAIL("generic function instantiation creates a conflict", "auto foo(generic a, int f = 1){ return -a; } foo(1, 2); auto foo(generic a = 1, int f = 2){ return a; } return foo();", "ERROR: function 'foo' is being defined with the same set of arguments");

	TEST_FOR_FAIL("direct member function call 1", "class Foo{ int x; } (void Foo:reset(){ x = 0; })();", "ERROR: member function can't be called without a class instance");
	TEST_FOR_FAIL("direct member function call 2", "class Foo{ int x; } (void Foo:reset(generic a){ x = 0; })(4);", "ERROR: member function can't be called without a class instance");
	TEST_FOR_FAIL("direct member function call 3", "class Foo{ int x; } auto foo(void ref() f){ f(); } foo(void Foo:reset(){ x = 0; });", "ERROR: member function can't be called without a class instance");

	TEST_FOR_FAIL("early recursive call", "auto foo(){ return foo(); }", "ERROR: function type is unresolved at this point");

	TEST_FOR_FAIL("scope switch in the same scope", "namespace Runner{ int foo(generic a){ return x + a; } int x = foo(2); return x; }", "ERROR: unknown identifier 'x'");

	TEST_FOR_FAIL("for each variables scope", "auto arr = { { 1, 2, 3, 4 }, { 1, 2, 3, 4 } }; for(auto a in arr, auto b in a){ return 0; }", "ERROR: unknown identifier 'a'");

	TEST_FOR_FAIL("function argument size limit", "class Large{ int x, y, z, w; int[16] pad; } class Huge{ Large[512] b; } auto test8(Huge a, b){ return b.b[110].x; }", "ERROR: function argument size cannot exceed 65536");

	TEST_FOR_FAIL("restore after explicit generic error type", "auto op2<@T>(@T a, b, c, d){ return a - b - c - d; } return op2 with<T>(1000, 2, 3, 4);", "ERROR: 'T' is not a known type name");

	TEST_FOR_FAIL("fuzzing test crash", "fo<@T, @U(){}", "ERROR: '>' expected after generic type alias list");
	TEST_FOR_FAIL("fuzzing test crash", "oid foo<@>(){}", "ERROR: explicit generic type alias is expected after '@'");
	TEST_FOR_FAIL("fuzzing test crash", "t ref(int, int)> a; re;", "ERROR: 't' is not a known type name");
	TEST_FOR_FAIL("fuzzing test crash", "aab[4] m;++++++i++;d;i.", "ERROR: member name expected after '.'");
	TEST_FOR_FAIL("fuzzing test crash", "!!!!!!!!!!!!!!!!n f();", "ERROR: ';' not found after expression");
	TEST_FOR_FAIL("fuzzing test crash", "class Test{ t a{ get{ return 0; } set } return 0;", "ERROR: function body expected after 'set'");
	TEST_FOR_FAIL("fuzzing test crash", "namespace Test{ vourn Test.a.bab(3) + Test.b.bar(;", "ERROR: ')' not found after function variable list");
	TEST_FOR_FAIL("fuzzing test crash", "int a = 4;@if(typeof(a) != inggggggggggggggt){f;}ora;", "ERROR: unknown identifier 'inggggggggggggggt'");
	TEST_FOR_FAIL("fuzzing test crash", "class Foo<T>{} auto foo(generic a, generic ref(Foo<typeof(()>) m){} return foo(1, <x>{ -4; });", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("fuzzing test crash", "import std.list; import old.list; return 1;", "ERROR: type 'list_node' in module 'old/list.nc' is already defined in module 'std/list.nc'");
	TEST_FOR_FAIL("fuzzing test crash", "coroutine auto __runner(){nt i = 1; return i.str() == \"0\" &else& int(i.str()) == i;}return __runner();", "ERROR: expression not found after binary operation");
	TEST_FOR_FAIL("fuzzing test crash", "auto bar(auto ref x){ if(o tydefotypeoo<int> ", "ERROR: closing ')' not found after 'if' condition");
	TEST_FOR_FAIL("fuzzing test crash", "auto average(generic ref(int) f){ return f(5); }return int(average(<i>{ i += 8; i * aaaaaaaaaaa5; }));", "ERROR: unknown identifier 'aaaaaaaaaaa5'");
	TEST_FOR_FAIL("fuzzing test crash", "void int:rgba(int r, g, b, a){r(){ return (*this >> 16) & 0xff; }int ift.g() bm(b.y", "ERROR: ';' not found after expression");
	TEST_FOR_FAIL("fuzzing test crash", "auto __runner(){class Foo<T>{ T t; }Foo<Foo<int>> b; b.t.t Foo<Foo<ijt>> b; b.t.t = 0;>> z)turn z.t.t; }returFoo<tin>> z){ return z.tuern __runner();", "ERROR: ';' not found after expression");
	TEST_FOR_FAIL("fuzzing test crash", "class vec2 extendable{ float x, y; int foo(int i, j){ return i / j; } } class vec2 : vec2{ float z; int foo(int j, i){ return i / j; } }", "ERROR: 'vec2' is being redefined");
	TEST_FOR_FAIL("fuzzing test crash", "coroutine int gen3from(int x){kint ref() m;for(int i = 0; i < 3; i++){int help(){return x + i;}help();m = help;}return m();i = gen3from2(2);t2 = t2 * 10 + i;}return (t1 == 567756) + (t2 == 442344);", "ERROR: 'kint' is not a known type name");
	TEST_FOR_FAIL("fuzzing test crash", "import std.algorithm;coroutine auto __runner(){int[] arr = { 2, 6 };sort(arr, <int a, int b>{ a > b; });map(arr, <int ref x>{ *x = *x * 2; })==arr = filter(arr, <ynt x>{ x < 8; });int sum = 0;for(i in arr) sum += i;return sum;}return __runner();", "ERROR: 'ynt' is not a known type name");
	TEST_FOR_FAIL("fuzzing test crash", "Auto int:toString(){}", "ERROR: 'Auto' is not a known type name");
	TEST_FOR_FAIL("fuzzing test crash", "int a = 4;@if(typeof(a) !=)tn i{f;}ora;", "ERROR: expression not found after binary operation");
	TEST_FOR_FAIL("fuzzing test crash", "class Foo<T>{ T x; }Foo a = Foo<int>((;a.x = 6;int foo(Foo a){return a.x;}foo(a);", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("fuzzing test crash", "int auto.c(){ return 0; }", "ERROR: cannot add accessor to type 'auto'");
	TEST_FOR_FAIL("fuzzing test crash", "int auto:c(){ return 0; }", "ERROR: cannot add member function to type 'auto'");
	TEST_FOR_FAIL("fuzzing test crash", "coroutine auto foo(){ int bar(); int bar(int x); yield bar; }", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("fuzzing test crash", "coroutine auto foo(){ auto a = 2; class T{ int b = a; } }", "ERROR: member function 'T::T$' cannot access external variable 'a'");
	TEST_FOR_FAIL("fuzzing test crash", "auto(@e x=){}", "ERROR: default argument value not found after '='");
	TEST_FOR_FAIL("fuzzing test crash", "coroutine auto foo(){ for(a in foo){} }", "ERROR: function 'foo' type is unresolved at this point");
	TEST_FOR_FAIL("fuzzing test crash", "switch(auto(@n x){}){}", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("fuzzing test crash", "long x = auto r(){};", "ERROR: cannot convert 'void ref()' to 'long'");
	TEST_FOR_FAIL("fuzzing test crash", "(.target", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("fuzzing test crash", "(.first", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("fuzzing test crash", "(.last", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("fuzzing test crash", "(.return", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("fuzzing test crash", "(.size", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("fuzzing test crash", "(.arraySize", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("fuzzing test crash", "auto bar<@T,@U>(){ return 1; } return bar with<int,int>() + bar with<>();", "ERROR: type name is expected after 'with'");
	TEST_FOR_FAIL("fuzzing test crash", "class Foo<T}d Foo : o(typeof(Foo<auto ref>o(", "ERROR: '>' expected after generic type alias list");
	TEST_FOR_FAIL("fuzzing test crash", "class Foo<T}typeid Foo:foo(typeof(T(= }Foo<void>test3; test3.foo(4", "ERROR: '>' expected after generic type alias list");
	TEST_FOR_FAIL("fuzzing test crash", "auto foo(generic x){}coroutine auto f(){auto x = foo;}", "ERROR: cannot instantiate generic function, because target type is not known");
	TEST_FOR_FAIL("fuzzing test crash", "@if{", "ERROR: '(' not found after 'if'");
	TEST_FOR_FAIL("fuzzing test crash", "@\"", "ERROR: unclosed string constant");
	TEST_FOR_FAIL("fuzzing test crash", "class const int,", "ERROR: class name expected");
	TEST_FOR_FAIL("fuzzing test crash", "@if new", "ERROR: '(' not found after 'if'");
	TEST_FOR_FAIL("fuzzing test crash", " generic  double = typeof  in  typeid[] ", "ERROR: typeof must be followed by '('");
	TEST_FOR_FAIL("fuzzing test crash", "void foo(); (void ref a(){})() += foo();", "ERROR: function must return a value of type 'void ref'");
	TEST_FOR_FAIL("fuzzing test crash", "class Foo{ const auto a = { for(; 1;) yield 1; }; }", "ERROR: member function can't be called without a class instance");
	TEST_FOR_FAIL("fuzzing test crash", "new short[] >>= void();", "ERROR: second operand type is 'void'");
	TEST_FOR_FAIL("fuzzing test crash", "void foo(){} typeof(foo())[] x;", "ERROR: cannot define an array of 'void'");
	TEST_FOR_FAIL("fuzzing test crash", " generic ref  b = b += b; ", "ERROR: variable 'b' is being used while its type is unknown");
	TEST_FOR_FAIL("fuzzing test crash", "while  bool  bool((bool(+= bool (", "ERROR: '(' not found after 'while'");
	TEST_FOR_FAIL("fuzzing test crash", "yield{ for(switch  generic[]", "ERROR: ';' not found after initializer in 'for'");
	TEST_FOR_FAIL("fuzzing test crash", "@?{{?@=@=", "ERROR: name expected after '@'");
	TEST_FOR_FAIL("fuzzing test crash", "coroutine auto({auto a=2class T i b=a", "ERROR: ')' not found after function variable list");
	TEST_FOR_FAIL("fuzzing test crash", "auto f<@T>(){} f with<@U>();", "ERROR: cannot take typeid from generic type");
	TEST_FOR_FAIL("fuzzing test crash", "auto o(@T ref, i}o(&, <", "ERROR: variable name not found after type in function variable list");
	TEST_FOR_FAIL("fuzzing test crash", "int foo(@T ref(int) f){ return f(5); } foo(<x>{ auto(@t t){} });", "ERROR: ambiguity, the expression is a generic function");
	TEST_FOR_FAIL("fuzzing test crash", "auto o(generic ref, r}o(k, <", "ERROR: variable name not found after type in function variable list");
	TEST_FOR_FAIL("fuzzing test crash", "an[n[n[n[n[n[n[a[n[nn[n[a[n[n[n<", "ERROR: expression not found after binary operation");
	TEST_FOR_FAIL("fuzzing test crash", "int(new int{ class a{ int R{get} } };", "ERROR: function body expected after 'get'");
	TEST_FOR_FAIL("fuzzing test crash", "auto foo(generic i, j){}foo(i:2);", "ERROR: can't find function 'foo' with following arguments:");
	TEST_FOR_FAIL("fuzzing test crash", "class j{ @if(j o(){class j{}}){}}", "ERROR: 'j' is being redefined");
	TEST_FOR_FAIL("fuzzing test crash", "double fo(do typeof(fo).argument[.", "ERROR: ')' not found after function variable list");
	TEST_FOR_FAIL("fuzzing test crash", "class X{} int X:foo(){ return 1; } auto ref y; int ref() z = y.foo;", "ERROR: can't convert dynamic function set to 'int ref()'");
	TEST_FOR_FAIL("fuzzing test crash", "auto ref x; *x = assert;", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("fuzzing test crash", "int(new{ class auto ref x = Foo; void Foo(", "ERROR: type name expected after 'new'");

	TEST_FOR_FAIL("fuzzing test crash (eval)", " class @ if {", "ERROR: class name expected");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "int[typeof(x)(1)] arr;", "ERROR: unknown identifier 'x'");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "int[float(float)] arr;", "ERROR: can't find function 'float::float' with following arguments:");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "@ if(@=", "ERROR: closing ')' not found after 'if' condition");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "@if(  double({ typeof(", "ERROR: expression not found after typeof(");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "sizeof  typeof  sizeof[1 ? sizeof", "ERROR: sizeof must be followed by '('");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "sizeof  typeof  sizeof[(auto (float  with", "ERROR: sizeof must be followed by '('");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "sizeof  typeof  sizeof[!float  with", "ERROR: sizeof must be followed by '('");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "void  argument  argument .", "ERROR: ';' not found after variable definition");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "typeof @ for(b  in  1  1", "ERROR: typeof must be followed by '('");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "auto []r r[2", "ERROR: ';' not found after variable definition");
	TEST_FOR_FAIL("fuzzing test crash (eval)", "int x coroutine void bar1(({x yield}int[2]arr for(i in arr)bar1(", "ERROR: ';' not found after variable definition");
}

const char	*testModuleImportsSelf1 = "import n; return 1;";
struct Test_testModuleImportsSelf1 : TestQueue
{
	static const char* FileReadHandler(const char* name, unsigned* size)
	{
		(void)name;
		*size = (unsigned)strlen(testModuleImportsSelf1) + 1;
		return testModuleImportsSelf1;
	}
	static void FileFreeHandler(const char *data)
	{
		(void)data;
	}
	virtual void Run()
	{
		nullcSetFileReadHandler(FileReadHandler, FileFreeHandler);
		if(Tests::messageVerbose)
			printf("Module imports itself 1 \r\n");

		testsCount[TEST_TYPE_FAILURE]++;
		nullres good = nullcCompile(testModuleImportsSelf1);
		if(!good)
		{
			char buf[4096];
			strncpy(buf, nullcGetLastError(), 4095); buf[4095] = 0;

			const char *expected = "ERROR: found cyclic dependency on module 'n.nc'\n  at line 1: 'import n; return 1;'\n              ^\n [in module 'n.nc']\n  at line 1: 'import n; return 1;'\n              ^\n";

			if(strcmp(expected, buf) != 0)
			{
				printf("Failed %s but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", "Module imports itself 1", buf, expected);
			}else{
				testsPassed[TEST_TYPE_FAILURE]++;
			}
		}else{
			printf("Test \"%s\" failed to fail.\r\n", "Module imports itself 1");
		}
		nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
	}
};
Test_testModuleImportsSelf1 testModuleImportSelf1;

const char	*testModuleImportsSelf2a = "import b; return 1;";
const char	*testModuleImportsSelf2b = "import a; return 1;";
struct Test_testModuleImportsSelf2 : TestQueue
{
	static const char* FileReadHandler(const char* name, unsigned* size)
	{
		if(name[0] == 'a')
		{
			*size = (unsigned)strlen(testModuleImportsSelf2a) + 1;
			return testModuleImportsSelf2a;
		}else{
			*size = (unsigned)strlen(testModuleImportsSelf2b) + 1;
			return testModuleImportsSelf2b;
		}
	}
	static void FileFreeHandler(const char *data)
	{
		(void)data;
	}
	virtual void Run()
	{
		nullcSetFileReadHandler(FileReadHandler, FileFreeHandler);
		if(Tests::messageVerbose)
			printf("Module imports itself 2 \r\n");

		testsCount[TEST_TYPE_FAILURE]++;
		nullres good = nullcCompile(testModuleImportsSelf2a);
		if(!good)
		{
			char buf[4096];
			strncpy(buf, nullcGetLastError(), 4095); buf[4095] = 0;

			const char *expected = "ERROR: found cyclic dependency on module 'b.nc'\n  at line 1: 'import b; return 1;'\n              ^\n [in module 'a.nc']\n [in module 'b.nc']\n";

			if(strcmp(expected, buf) != 0)
			{
				printf("Failed %s but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", "Module imports itself 1", buf, expected);
			}else{
				testsPassed[TEST_TYPE_FAILURE]++;
			}
		}else{
			printf("Test \"%s\" failed to fail.\r\n", "Module imports itself 1");
		}
		nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
	}
};
Test_testModuleImportsSelf2 testModuleImportSelf2;
