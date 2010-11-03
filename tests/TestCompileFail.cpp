#include "TestCompileFail.h"

#include "TestBase.h"

void TEST_FOR_FAIL_FULL(const char* name, const char* str, const char* error)
{
	testsCount[TEST_FAILURE_INDEX]++;
	nullres good = nullcCompile(str);
	if(!good)
	{
		char buf[4096];
		strncpy(buf, nullcGetLastError(), 4095); buf[4095] = 0;
		if(strcmp(error, buf) != 0)
		{
			printf("Failed %s but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", name, buf, error);
		}else{
			testsPassed[TEST_FAILURE_INDEX]++;
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
	TEST_FOR_FAIL("Change of immutable value", "int i; return *i = 5;", "ERROR: cannot change immutable value of type int");
	TEST_FOR_FAIL("Hex overflow", "return 0xbeefbeefbeefbeefb;", "ERROR: overflow in hexadecimal constant");
	TEST_FOR_FAIL("Oct overflow", "return 03333333333333333333333;", "ERROR: overflow in octal constant");
	TEST_FOR_FAIL("Bin overflow", "return 10000000000000000000000000000000000000000000000000000000000000000b;", "ERROR: overflow in binary constant");
	TEST_FOR_FAIL("Logical not on double", "return !0.5;", "ERROR: logical NOT is not available on floating-point numbers");
	TEST_FOR_FAIL("Binary not on double", "return ~0.4;", "ERROR: binary NOT is not available on floating-point numbers");

	TEST_FOR_FAIL("No << on float", "return 1.0 << 2.0;", "ERROR: << is illegal for floating-point numbers");
	TEST_FOR_FAIL("No >> on float", "return 1.0 >> 2.0;", "ERROR: >> is illegal for floating-point numbers");
	TEST_FOR_FAIL("No | on float", "return 1.0 | 2.0;", "ERROR: | is illegal for floating-point numbers");
	TEST_FOR_FAIL("No & on float", "return 1.0 & 2.0;", "ERROR: & is illegal for floating-point numbers");
	TEST_FOR_FAIL("No ^ on float", "return 1.0 ^ 2.0;", "ERROR: ^ is illegal for floating-point numbers");
	TEST_FOR_FAIL("No && on float", "return 1.0 && 2.0;", "ERROR: && is illegal for floating-point numbers");
	TEST_FOR_FAIL("No || on float", "return 1.0 || 2.0;", "ERROR: || is illegal for floating-point numbers");
	TEST_FOR_FAIL("No ^^ on float", "return 1.0 ^^ 2.0;", "ERROR: ^^ is illegal for floating-point numbers");

	TEST_FOR_FAIL("Wrong return", "int a(){ return {1,2};} return 1;", "ERROR: function returns int[2] but supposed to return int");
	TEST_FOR_FAIL("Shouldn't return anything", "void a(){ return 1; } return 1;", "ERROR: 'void' function returning a value");
	TEST_FOR_FAIL("Should return something", "int a(){ return; } return 1;", "ERROR: function should return int");
	TEST_FOR_FAIL("Global return doesn't accept void", "void a(){} return a();", "ERROR: global return cannot accept void");
	TEST_FOR_FAIL("Global return doesn't accept complex types", "void a(){} return a;", "ERROR: global return cannot accept complex types");

	TEST_FOR_FAIL("Break followed by trash", "int a; break a; return 1;", "ERROR: break statement must be followed by ';' or a constant");
	TEST_FOR_FAIL("Break with depth 0", "break 0; return 1;", "ERROR: break level cannot be 0");
	TEST_FOR_FAIL("Break with depth too big", "while(1){ break 2; } return 1;", "ERROR: break level is greater that loop depth");

	TEST_FOR_FAIL("continue followed by trash", "int a; continue a; return 1;", "ERROR: continue statement must be followed by ';' or a constant");
	TEST_FOR_FAIL("continue with depth 0", "continue 0; return 1;", "ERROR: continue level cannot be 0");
	TEST_FOR_FAIL("continue with depth too big", "while(1){ continue 2; } return 1;", "ERROR: continue level is greater that loop depth");

	TEST_FOR_FAIL("Variable redefinition", "int a, a; return 1;", "ERROR: name 'a' is already taken for a variable in current scope");
	TEST_FOR_FAIL("Variable hides function", "void a(){} int a; return 1;", "ERROR: name 'a' is already taken for a function");

	TEST_FOR_FAIL("Uninit auto", "auto a; return 1;", "ERROR: auto variable must be initialized in place of definition");
	TEST_FOR_FAIL("Array of auto", "auto[4] a; return 1;", "ERROR: cannot specify array size for auto variable");
	TEST_FOR_FAIL("sizeof auto", "return sizeof(auto);", "ERROR: sizeof(auto) is illegal");

	TEST_FOR_FAIL("Unknown function", "return b;", "ERROR: variable or function 'b' is not defined");
	TEST_FOR_FAIL("Unclear decision", "void a(int b){} void a(float b){} return a;", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("Variable of unknown type used", "auto a = a + 1; return a;", "ERROR: variable 'a' is being used while its type is unknown");

	TEST_FOR_FAIL("Indexing not an array", "int a; return a[5];", "ERROR: indexing variable that is not an array (int)");
	TEST_FOR_FAIL("Array underflow", "int[4] a; a[-1] = 2; return 1;", "ERROR: array index cannot be negative");
	TEST_FOR_FAIL("Array overflow", "int[4] a; a[5] = 1; return 1;", "ERROR: array index out of bounds");

	TEST_FOR_FAIL("No matching function", "int f(int a, b){ return 0; } int f(int a, long b){ return 0; } return f(1)'", "ERROR: can't find function 'f' with following parameters:");
	TEST_FOR_FAIL("No clear decision", "int f(int a, b){ return 0; } int f(int a, long b){ return 0; } int f(){ return 0; } return f(1, 3.0)'", "ERROR: ambiguity, there is more than one overloaded function available for the call:");

	TEST_FOR_FAIL("Array without member", "int[4] a; return a.m;", "ERROR: array doesn't have member with this name");
	TEST_FOR_FAIL("No methods", "int[4] i; return i.ok();", "ERROR: function 'int[]::ok' is undefined");
	TEST_FOR_FAIL("void array", "void f(){} return { f(), f() };", "ERROR: array cannot be constructed from void type elements");
	TEST_FOR_FAIL("Name taken", "int a; void a(){} return 1;", "ERROR: name 'a' is already taken for a variable in current scope");
	TEST_FOR_FAIL("Auto parameter", "auto(auto a){} return 1;", "ERROR: function parameter cannot be an auto type");
	TEST_FOR_FAIL("Auto parameter 2 ", "int func(auto a, int i){ return 0; } return 0;", "ERROR: function parameter cannot be an auto type");
	TEST_FOR_FAIL("Function redefine", "int a(int b){ return 0; } int a(int c){ return 0; } return 1;", "ERROR: function 'a' is being defined with the same set of parameters");
	TEST_FOR_FAIL("Wrong overload", "int operator*(int a){} return 1;", "ERROR: binary operator definition or overload must accept exactly two arguments");
	TEST_FOR_FAIL("No member function", "int a; return a.ok();", "ERROR: function 'int::ok' is undefined");
	TEST_FOR_FAIL("Unclear decision - member function", "class test{ void a(int b){} void a(float b){} } test t; return t.a;", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("No function", "return k();", "ERROR: function 'k' is undefined");

	TEST_FOR_FAIL("void condition", "void f(){} if(f()){} return 1;", "ERROR: condition type cannot be 'void'");
	TEST_FOR_FAIL("void condition", "void f(){} if(f()){}else{} return 1;", "ERROR: condition type cannot be 'void'");
	TEST_FOR_FAIL("void condition", "void f(){} return f() ? 1 : 0;", "ERROR: condition type cannot be 'void'");
	TEST_FOR_FAIL("void condition", "void f(){} for(int i = 0; f(); i++){} return 1;", "ERROR: condition type cannot be 'void'");
	TEST_FOR_FAIL("void condition", "void f(){} while(f()){} return 1;", "ERROR: condition type cannot be 'void'");
	TEST_FOR_FAIL("void condition", "void f(){} do{}while(f()); return 1;", "ERROR: condition type cannot be 'void'");
	TEST_FOR_FAIL("void condition", "import std.math; float4 f(){ return float4(1,1,1,1); } if(f()){} return 1;", "ERROR: condition type cannot be 'float4'");
	TEST_FOR_FAIL("void condition", "import std.math; float4 f(){ return float4(1,1,1,1); } if(f()){}else{} return 1;", "ERROR: condition type cannot be 'float4'");
	TEST_FOR_FAIL("void condition", "import std.math; float4 f(){ return float4(1,1,1,1); } return f() ? 1 : 0;", "ERROR: condition type cannot be 'float4'");
	TEST_FOR_FAIL("void condition", "import std.math; float4 f(){ return float4(1,1,1,1); } for(int i = 0; f(); i++){} return 1;", "ERROR: condition type cannot be 'float4'");
	TEST_FOR_FAIL("void condition", "import std.math; float4 f(){ return float4(1,1,1,1); } while(f()){} return 1;", "ERROR: condition type cannot be 'float4'");
	TEST_FOR_FAIL("void condition", "import std.math; float4 f(){ return float4(1,1,1,1); } do{}while(f()); return 1;", "ERROR: condition type cannot be 'float4'");
	TEST_FOR_FAIL("void condition", "void f(){} switch(f()){ case 1: break; } return 1;", "ERROR: condition type cannot be 'void'");
	TEST_FOR_FAIL("void condition", "import std.math; float4 f(){ return float4(1,1,1,1); } switch(f()){ case 1: break; } return 1;", "ERROR: condition type cannot be 'float4'");
	TEST_FOR_FAIL("void case", "void f(){} switch(1){ case f(): break; } return 1;", "ERROR: case value type cannot be void");

	TEST_FOR_FAIL("class in class", "class test{ void f(){ class heh{ int h; } } } return 1;", "ERROR: different type is being defined");
	TEST_FOR_FAIL("class wrong alignment", "align(32) class test{int a;} return 1;", "ERROR: alignment must be less than 16 bytes");
	TEST_FOR_FAIL("class member auto", "class test{ auto i; } return 1;", "ERROR: auto cannot be used for class members");
	TEST_FOR_FAIL("class is too big", "class nobiggy{ int[128][128][4] a; } return 1;", "ERROR: class size cannot exceed 65535 bytes");

#ifdef NULLC_PURE_FUNCTIONS
	TEST_FOR_FAIL("array size not const", "import std.math; int[cos(12) * 16] a; return a[0];", "ERROR: array size must be a constant expression. During constant folding, 'cos' function couldn't be evaluated");
#else
	TEST_FOR_FAIL("array size not const", "import std.math; int[cos(12) * 16] a; return a[0];", "ERROR: array size must be a constant expression");
#endif
	TEST_FOR_FAIL("array size not positive", "int[-16] a; return a[0];", "ERROR: array size can't be negative or zero");

	TEST_FOR_FAIL("function parameter cannot be a void type", "int f(void a){ return 0; } return 1;", "ERROR: function parameter cannot be a void type");
	TEST_FOR_FAIL("function prototype with unresolved return type", "auto f(); return 1;", "ERROR: function prototype with unresolved return type");
	TEST_FOR_FAIL("Division by zero during constant folding", "return 5 / 0;", "ERROR: division by zero during constant folding");
	TEST_FOR_FAIL("Modulus division by zero during constant folding 1", "return 5 % 0;", "ERROR: modulus division by zero during constant folding");
	TEST_FOR_FAIL("Modulus division by zero during constant folding 2", "return 5l % 0l;", "ERROR: modulus division by zero during constant folding");

	TEST_FOR_FAIL("Variable as a function", "int a = 5; return a(4);", "ERROR: function '()' is undefined");

	TEST_FOR_FAIL("Function pointer call with wrong argument count", "int f(int a){ return -a; } auto foo = f; auto b = foo(); return foo(1, 2);", "ERROR: function expects 1 argument(s), while 0 are supplied");
	TEST_FOR_FAIL("Function pointer call with wrong argument types", "import std.math; int f(int a){ return -a; } auto foo = f; float4 v; return foo(v);", "ERROR: there is no conversion from specified arguments and the ones that function accepts");

	TEST_FOR_FAIL("Indirect function pointer call with wrong argument count", "int f(int a){ return -a; } typeof(f)[2] foo = { f, f }; auto b = foo[0](); return foo(1, 2);", "ERROR: function expects 1 argument(s), while 0 are supplied");
	TEST_FOR_FAIL("Indirect function pointer call with wrong argument types", "import std.math; int f(int a){ return -a; } typeof(f)[2] foo = { f, f }; float4 v; return foo[0](v);", "ERROR: there is no conversion from specified arguments and the ones that function accepts");

	TEST_FOR_FAIL("Array element type mistmatch", "import std.math;\r\nauto err = { 1, float2(2, 3), 4 };\r\nreturn 1;", "ERROR: element 1 doesn't match the type of element 0 (int)");
	TEST_FOR_FAIL("Ternary operator complex type mistmatch", "import std.math;\r\nint x = 1; auto err = x ? 1 : float2(2, 3);\r\nreturn 1;", "ERROR: ternary operator ?: result types are not equal (int : float2)");

	TEST_FOR_FAIL("Indexing value that is not an array 2", "return (1)[1];", "ERROR: indexing variable that is not an array (int)");
	TEST_FOR_FAIL("Illegal conversion from type[] ref to type[]", "int[] b = { 1, 2, 3 };int[] ref c = &b;int[] d = c;return 1;", "ERROR: cannot convert 'int[] ref' to 'int[]'");
	TEST_FOR_FAIL("Type redefinition", "class int{ int a, b; } return 1;", "ERROR: 'int' is being redefined");
	TEST_FOR_FAIL("Type redefinition 2", "typedef int uint; class uint{ int x; } return 1;", "ERROR: 'uint' is being redefined");

	TEST_FOR_FAIL("Illegal conversion 1", "import std.math; float3 a; a = 12.0; return 1;", "ERROR: cannot convert 'double' to 'float3'");
	TEST_FOR_FAIL("Illegal conversion 2", "import std.math; float3 a; float4 b; b = a; return 1;", "ERROR: cannot convert 'float3' to 'float4'");

	TEST_FOR_FAIL("For scope", "for(int i = 0; i < 1000; i++) i += 5; return i;", "ERROR: variable or function 'i' is not defined");

	TEST_FOR_FAIL("Class function return unclear 1", "class Test{int i;int foo(){ return i; }int foo(int k){ return i; }auto bar(){ return foo; }}return 1;", "ERROR: ambiguity, there is more than one overloaded function available:");
	TEST_FOR_FAIL("Class function return unclear 2", "int foo(){ return 2; }class Test{int i;int foo(){ return i; }auto bar(){ return foo; }}return 1;", "ERROR: ambiguity, there is more than one overloaded function available:");

	TEST_FOR_FAIL("Class externally defined method 1", "int dontexist:do(){ return 0; } return 1;", "ERROR: class name expected before ':' or '.'");
	TEST_FOR_FAIL("Class externally defined method 2", "int int:(){ return *this; } return 1;", "ERROR: function name expected after ':' or '.'");

	TEST_FOR_FAIL("Member variable or function is not found", "int a; a.b; return 1;", "ERROR: member variable or function 'b' is not defined in class 'int'");

	TEST_FOR_FAIL("Inplace array element type mismatch", "auto a = { 12, 15.0 };", "ERROR: element 1 doesn't match the type of element 0 (int)");
	TEST_FOR_FAIL("Ternary operator void return type", "void f(){} int a = 1; return a ? f() : 0.0;", "ERROR: one of ternary operator ?: result type is void (void : double)");
	TEST_FOR_FAIL("Ternary operator return type difference", "import std.math; int a = 1; return a ? 12 : float2(3, 4);", "ERROR: ternary operator ?: result types are not equal (int : float2)");

	TEST_FOR_FAIL("Variable type is unknow", "int test(int a, typeof(test) ptr){ return ptr(a, ptr); }", "ERROR: variable type is unknown");

	TEST_FOR_FAIL("Illegal pointer operation 1", "int ref a; a += a;", "ERROR: there is no build-in operator for types 'int ref' and 'int ref'");
	TEST_FOR_FAIL("Illegal pointer operation 2", "int ref a; a++;", "ERROR: increment is not supported on 'int ref'");
	TEST_FOR_FAIL("Illegal pointer operation 3", "int ref a; a = a * 5;", "ERROR: operation * is not supported on 'int ref' and 'int'");
	TEST_FOR_FAIL("Illegal class operation", "import std.math; float2 v; v = ~v;", "ERROR: unary operation '~' is not supported on 'float2'");

	TEST_FOR_FAIL("Default function parameter type mismatch", "import std.math;int f(int v = float3(3, 4, 5)){ return v; }return f();", "ERROR: cannot convert from 'float3' to 'int'");
	TEST_FOR_FAIL("Default function parameter type mismatch 2", "void func(){} int test(int a = func()){ return a; } return 0;", "ERROR: cannot convert from 'void' to 'int'");
	TEST_FOR_FAIL("Default function parameter of void type", "void func(){} int test(auto a = func()){ return a; } return 0;", "ERROR: function parameter cannot be a void type");

	TEST_FOR_FAIL("Undefined function call in function parameters", "int func(int a = func()){ return 0; } return 0;", "ERROR: function 'func' is undefined");
	TEST_FOR_FAIL("Property set function is missing", "int int.test(){ return *this; } int a; a.test = 5; return a.test;", "ERROR: cannot change immutable value of type int");
	TEST_FOR_FAIL("Illegal comparison", "return \"hello\" > 12;", "ERROR: operation > is not supported on 'char[6]' and 'int'");
	TEST_FOR_FAIL("Illegal array element", "auto a = { {15, 12 }, 14, {18, 48} };", "ERROR: element 1 doesn't match the type of element 0 (int[2])");
	TEST_FOR_FAIL("Wrong return type", "int ref a(){ float b=5; return &b; } return 9;", "ERROR: function returns float ref but supposed to return int ref");
	
	TEST_FOR_FAIL("Global variable size limit", "char[32*1024*1024] arr;", "ERROR: global variable size limit exceeded");
	TEST_FOR_FAIL("Unsized array initialization", "char[] arr = 1;", "ERROR: cannot convert 'int' to 'char[]'");
	
	TEST_FOR_FAIL("Invalid array index type A", "int[100] arr; void func(){} arr[func()] = 5;", "ERROR: cannot index array with type 'void'");
	TEST_FOR_FAIL("Invalid array index type B", "import std.math; float2 a; int[100] arr; arr[a] = 7;", "ERROR: cannot index array with type 'float2'");

	TEST_FOR_FAIL("None of the types implement method", "int i = 0; auto ref u = &i; return u.value();", "ERROR: function 'value' is undefined in any of existing classes");
	TEST_FOR_FAIL("None of the types implement correct method", "int i = 0; int int:value(){ return *this; } auto ref u = &i; return u.value(15);", "ERROR: none of the member ::value functions can handle the supplied parameter list without conversions");

	TEST_FOR_FAIL("Operator overload with no arguments", "int operator+(){ return 5; }", "ERROR: binary operator definition or overload must accept exactly two arguments");

	TEST_FOR_FAIL("new auto;", "auto a = new auto;", "ERROR: sizeof(auto) is illegal");
	TEST_FOR_FAIL("new void;", "auto a = new void;", "ERROR: cannot allocate space for void type");

	TEST_FOR_FAIL("Array underflow 2", "int[7][3] uu; uu[2][1] = 100; int[][3] kk = uu; return kk[2][-1000000];", "ERROR: array index cannot be negative");
	TEST_FOR_FAIL("Array overflow 2", "int[7][3] uu; uu[2][1] = 100; int[][3] kk = uu; return kk[2][1000000];", "ERROR: array index out of bounds");

	TEST_FOR_FAIL("Invalid conversion", "int a; int[] arr = new int[10]; arr = &a; return 0;", "ERROR: cannot convert 'int ref' to 'int[]'");

	TEST_FOR_FAIL("Usage of an undefined class", "class Foo{ Foo a; int i; }; Foo a; return 1;", "ERROR: Type 'Foo' is currently being defined. You can use 'Foo ref' or 'Foo[]' at this point");

	TEST_FOR_FAIL("Can't yield if not a coroutine", "int test(){ yield 4; } return test();", "ERROR: yield can only be used inside a coroutine");

	TEST_FOR_FAIL("Operation unsupported on reference 1", "int x; int ref y = &x; return *y++;", "ERROR: increment is not supported on 'int ref'");
	TEST_FOR_FAIL("Operation unsupported on reference 2", "int x; int ref y = &x; return *++y;", "ERROR: increment is not supported on 'int ref'");
	TEST_FOR_FAIL("Operation unsupported on reference 3", "int x; int ref y = &x; return *y--;", "ERROR: decrement is not supported on 'int ref'");
	TEST_FOR_FAIL("Operation unsupported on reference 4", "int x; int ref y = &x; return *--y;", "ERROR: decrement is not supported on 'int ref'");

	TEST_FOR_FAIL("Constructor returns a value", "auto int:int(int x, y){ *this = x; return this; } return *new int(4, 8);", "ERROR: constructor cannot be used after 'new' expression if return type is not void");

#ifdef NULLC_PURE_FUNCTIONS
const char	*testCompileTimeNoReturn =
"int foo(int m)\r\n\
{\r\n\
	if(m == 0)\r\n\
		return 1;\r\n\
}\r\n\
int[foo(3)] arr;";
	TEST_FOR_FAIL("Compile time function evaluation doesn't return.", testCompileTimeNoReturn, "ERROR: array size must be a constant expression. During constant folding, 'foo' function couldn't be evaluated");

#endif

	TEST_FOR_FAIL("Inline function with wrong type", "int foo(int ref(int) x){ return x(4); } foo(auto(){});", "ERROR: can't find function 'foo' with following parameters:");
	TEST_FOR_FAIL("Function argument already defined", "int foo(int x, x){ return x + x; } return foo(5, 4);", "ERROR: parameter with name 'x' is already defined");

	TEST_FOR_FAIL("Read-only member", "int[] arr; arr.size = 10; return arr.size;", "ERROR: cannot change immutable value of type int");
	TEST_FOR_FAIL("Read-only member", "auto ref x; x.type = int; return 1;", "ERROR: cannot change immutable value of type typeid");
	TEST_FOR_FAIL("Read-only member", "auto ref x, y; x.ptr = y.ptr; return 1;", "ERROR: cannot convert from void to void");
	TEST_FOR_FAIL("Read-only member", "auto[] x; x.type = int; return 1;", "ERROR: cannot change immutable value of type typeid");
	TEST_FOR_FAIL("Read-only member", "auto[] x; x.size = 10; return 1;", "ERROR: cannot change immutable value of type int");
	TEST_FOR_FAIL("Read-only member", "auto[] x, y; x.ptr = y.ptr; return 1;", "ERROR: cannot convert from void to void");
	TEST_FOR_FAIL("Read-only member", "int[] x = new int[2]; (&x.size)++; return x.size;", "ERROR: cannot take pointer to a read-only variable");
	TEST_FOR_FAIL("Read-only member", "int[] x = new int[2]; (&x.size)--; return x.size;", "ERROR: cannot take pointer to a read-only variable");
	TEST_FOR_FAIL("Read-only member", "auto[] x = new int[2]; (&x.size)++; return x.size;", "ERROR: cannot take pointer to a read-only variable");
	TEST_FOR_FAIL("Read-only member", "auto[] x = new int[2]; (&x.size)--; return x.size;", "ERROR: cannot take pointer to a read-only variable");
	TEST_FOR_FAIL("Read-only member", "int[] x = new int[2]; (auto(int ref x){ return x; })(&x.size) = 56; return x.size;", "ERROR: cannot take pointer to a read-only variable");

	nullcLoadModuleBySource("test.redefinitionPartA", "int foo(int x){ return -x; }");
	nullcLoadModuleBySource("test.redefinitionPartB", "int foo(int x){ return ~x; }");
	TEST_FOR_FAIL("Module function redefinition", "import test.redefinitionPartA; import test.redefinitionPartB; return foo();", "ERROR: function foo (type int ref(int)) is already defined. While importing test/redefinitionPartB.nc");

	TEST_FOR_FAIL("number constant overflow (hex)", "return 0x1deadbeefdeadbeef;", "ERROR: overflow in hexadecimal constant");
	TEST_FOR_FAIL("number constant overflow (oct)", "return 02777777777777777777777;", "ERROR: overflow in octal constant");
	TEST_FOR_FAIL("number constant overflow (bin)", "return 011111111111111111111111111111111111111111111111111111111111111111b;", "ERROR: overflow in binary constant");

	TEST_FOR_FAIL("variable with class name", "class Test{} int Test = 5; return Test;", "ERROR: name 'Test' is already taken for a class");

	TEST_FOR_FAIL("List comprehension of void type", "auto fail = { for(;0;){ yield; } };", "ERROR: cannot generate an array of 'void' element type");
	TEST_FOR_FAIL("List comprehension of unknown type", "auto fail = { for(;0;){} };", "ERROR: not a single element is generated, and an array element type is unknown");

	TEST_FOR_FAIL("Non-coroutine as an iterator 1", "int foo(){ return 1; } for(int i in foo) return 1;", "ERROR: function is not a coroutine");
	TEST_FOR_FAIL("Non-coroutine as an iterator 2", "auto omg(int z){ int foo(){ return z; } for(i in foo) return 1; } omg(1);", "ERROR: function is not a coroutine");

	TEST_FOR_FAIL("Dereferencing non-pointer", "int a = 5; return *a;", "ERROR: cannot dereference type 'int' that is not a pointer");

	TEST_FOR_FAIL("Short function outside argument list", "return <x>{ return 5; };", "ERROR: cannot infer type for inline function outside of the function call");
	
	TEST_FOR_FAIL("Short function outside argument list",
"void bar(void ref() x){ x(); }\r\n\
return bar(auto(){ int a = <x>{ return 5; }; });", "ERROR: cannot infer type for inline function outside of the function call");
	
	TEST_FOR_FAIL("Parent function not found", "return bar(<x>{ return -x; }, 5);", "ERROR: cannot find function or variable 'bar' which accepts a function with 1 argument(s) as an argument #0");
	
	TEST_FOR_FAIL("Multiple choices exist",
"int bar(int ref(double) f, double y){ return f(y); }\r\n\
int bar(int ref(int) f, int y){ return f(y); }\r\n\
return bar(<x>{ return -x; }, 5);", "ERROR: there are multiple function 'bar' overloads expecting different function types as an argument #0");
	
	TEST_FOR_FAIL("Argument is not a function",
"int bar(int x, int y){ return x + y; }\r\n\
return bar(<x>{ return -x; }, 5);", "ERROR: cannot find function or variable 'bar' which accepts a function with 1 argument(s) as an argument #0");
	
	TEST_FOR_FAIL("Wrong argument count",
"int bar(int ref(double) f, double y){ return f(y); }\r\n\
return bar(<>{ return -x; }, 5);", "ERROR: cannot find function or variable 'bar' which accepts a function with 0 argument(s) as an argument #0");

	TEST_FOR_FAIL("No expression list in short inline function", "int caller(int ref(int) f){ return f(5); } return caller(<x>{});", "ERROR: function must return a value of type 'int'");
	TEST_FOR_FAIL("One expression in short function if not a pop node", "int caller(int ref(int) f){ return f(5); } return caller(<x>{ if(x){} });", "ERROR: function must return a value of type 'int'");
	TEST_FOR_FAIL("Multiple expressions in short function without a pop node", "int caller(int ref(int) f){ return f(5); } return caller(<x>{ if(x){} if(x){} });", "ERROR: function must return a value of type 'int'");

	TEST_FOR_FAIL("Using an argument of a function in definition in expression", "auto foo(int a, int b = a){ return a + b; } return foo(5, 7);", "ERROR: variable or function 'a' is not defined");

	TEST_FOR_FAIL("Short inline function at generic argument position", "auto foo(generic a, b, generic f){ return f(a, b); } return foo(5, 4, <x,y>{ x * y; });", "ERROR: cannot find function or variable 'foo' which accepts a function with 2 argument(s) as an argument #2");

	TEST_FOR_FAIL("Inline generic function pointer", "int foo(int ref(int) f){ return f(5); } int x(generic a){ return -a; } return foo(x);", "ERROR: can't take pointer to a generic function");
	TEST_FOR_FAIL("Generic function pointer 2", "auto test(generic a, generic f){ return f(a); } auto foo(generic a){ return -a; } return test(5, foo);", "ERROR: can't take pointer to a generic function");
	TEST_FOR_FAIL("typeof from a combination of generic arguments", "auto sum(generic a, b, typeof(a*b) c){ return a + b; } return sum(3, 4.5, double);", "ERROR: can't find function 'sum' with following parameters:");

	TEST_FOR_FAIL("coroutine cannot be a member function", "class Foo{} coroutine int Foo:bar(){ yield 1; return 0; } return 1;", "ERROR: coroutine cannot be a member function");

	TEST_FOR_FAIL_GENERIC("error in generic function body", "auto sum(generic a, b, c){ return a + b + ; } return sum(3, 4.5, 5l);", "ERROR: while instantiating generic function sum(generic, generic, generic)", "ERROR: terminal expression not found after binary operation");
	TEST_FOR_FAIL_GENERIC("genric function accessing variable that is defined later", "int y = 4; auto foo(generic a){ return i + y; } int i = 2; return foo(4);", "ERROR: while instantiating generic function foo(generic)", "ERROR: variable or function 'i' is not defined");

	TEST_FOR_FAIL("multiple function prototypes", "int foo(); int foo(); int foo(){ return 1; } return 1;", "ERROR: function is already defined");
	TEST_FOR_FAIL("function prototype after definition", "int foo(){ return 1; } int foo(); return 1;", "ERROR: function is already defined");

	TEST_FOR_FAIL("unimplemented local function", "int foo(){ int bar(); return bar(); } return foo();", "ERROR: local function 'bar' went out of scope unimplemented");

	TEST_FOR_FAIL("wrong implementation scope", "int foo(int x){ int bar(); int y = bar(); int help(){ int bar(){ return -x; } return bar(); } help(); return y; } return foo(5);", "ERROR: function implementation is found in scope different from function prototype");

	TEST_FOR_FAIL("block depth is too large", "int foo()\
	{\
	{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{\
			int a = 4; int test(){ return a; }\
			}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\
		return 1;\
	}\
	return foo();", "ERROR: function block depth (256) is too large to handle");

	TEST_FOR_FAIL("typeid of auto", "typeof(auto);", "ERROR: cannot take typeid from auto type");
	TEST_FOR_FAIL("conversion from void to basic type", "int foo(int a){ return -a; } void bar(){} return foo(bar());", "ERROR: can't find function 'foo' with following parameters:");
	TEST_FOR_FAIL_GENERIC("buffer overrun prevention", "int foo(generic a){ return -; /* %s %s %s %s %s %s %s %s %s %s %s %s %s %s */ } return foo(1);", "ERROR: while instantiating generic function foo(generic)", "ERROR: expression not found after '-'");
	TEST_FOR_FAIL("Infinite instantiation recursion", "auto foo(generic a){ typeof(a) ref x; return foo(x); } return foo(1); }", "ERROR: while instantiating generic function foo(generic)");
	TEST_FOR_FAIL("Infinite instantiation recursion 2", "auto foo(generic a){ typeof(a) ref(typeof(a) ref, typeof(a) ref, typeof(a) ref) x; return foo(x); } return foo(1); }", "ERROR: while instantiating generic function foo(generic)");
	TEST_FOR_FAIL("auto resolved to void", "void foo(){} auto x = foo();", "ERROR: r-value type is 'void'");
	TEST_FOR_FAIL("unclear decision at return", "int foo(int x){ return -x; } int foo(float x){ return x * 2.0f; } auto bar(){ return foo; }", "ERROR: ambiguity, there is more than one overloaded function available:");

	TEST_FOR_FAIL_FULL("unclear target function",
"int foo(int x){ return -x; }\r\n\
int foo(float x){ return x * 2; }\r\n\
int bar(int ref(int) f){ return f(5); }\r\n\
int bar(int ref(float) f){ return f(10); }\r\n\
return bar(foo);",
"line 5 - ERROR: ambiguity, there is more than one overloaded function available for the call:\r\n\
  bar(int ref(float) or int ref(int))\r\n\
 candidates are:\r\n\
  int bar(int ref(float))\r\n\
  int bar(int ref(int))\r\n\
\r\n\
  at \"return bar(foo);\"\r\n\
                     ^\r\n");

	TEST_FOR_FAIL_FULL("no target function",
"int foo(double x){ return -x; }\r\n\
int foo(float x){ return x * 2; }\r\n\
int bar(int ref(int) f){ return f(5); }\r\n\
return bar(foo);",
"line 4 - ERROR: can't find function 'bar' with following parameters:\r\n\
  bar(int ref(float) or int ref(double))\r\n\
 the only available are:\r\n\
  int bar(int ref(int))\r\n\
\r\n\
  at \"return bar(foo);\"\r\n\
                     ^\r\n");

	TEST_FOR_FAIL("generic function instance type unknown", "auto y = auto(generic y){ return -y; };", "ERROR: cannot instance generic function, because target type is not known");

	TEST_FOR_FAIL_FULL("cannot instance function in argument list", "int foo(int f){ return f; }\r\nreturn foo(auto(generic y){ return -y; });",
"line 2 - ERROR: can't find function 'foo' with following parameters:\r\n\
  foo(`function`)\r\n\
 the only available are:\r\n\
  int foo(int)\r\n\
\r\n\
  at \"return foo(auto(generic y){ return -y; });\"\r\n\
                                               ^\r\n");

	TEST_FOR_FAIL("cannot instance function because target type is not a function", "int x = auto(generic y){ return -y; }; return x;", "ERROR: cannot instance generic function to a type 'int'");
	TEST_FOR_FAIL("cannot select function overload", "int foo(int x){ return -x; } int foo(float x){ return x * 2; } int x = foo;", "ERROR: cannot select function overload for a type 'int'");
	TEST_FOR_FAIL("Instanced inline function is wrong", "class X{} X x; int foo(int ref(int, X) f){ return f(5, x); } return foo(auto(generic y, double z){ return -y; });", "ERROR: cannot convert from 'int ref(int,double)' to 'int ref(int,X)'");
	TEST_FOR_FAIL("Typeof in generic function shouldn't skip all errors", "auto foo(generic x, typeof(foo(5)) y){ return x + y; }", "ERROR: function 'foo' is undefined");

	TEST_FOR_FAIL("no finalizable objects on stack", "class Foo{int a;} void Foo:finalize(){} Foo x;", "ERROR: cannot create 'Foo' that implements 'finalize' on stack");
	TEST_FOR_FAIL("no finalizable objects on stack 2", "class Foo{int a;} void Foo:finalize(){} auto x = *(new Foo);", "ERROR: cannot create 'Foo' that implements 'finalize' on stack");
	TEST_FOR_FAIL("no finalizable objects on stack 3", "class Foo{int a;} void Foo:finalize(){} Foo[10] arr;", "ERROR: class 'Foo' implements 'finalize' so only an unsized array type can be created");
	TEST_FOR_FAIL("no finalizable objects on stack 4", "class Foo{int a;} void Foo:finalize(){} class Bar{ Foo z; }", "ERROR: class 'Foo' implements 'finalize' so only a reference or an unsized array of 'Foo' can be put in a class");

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

	TEST_FOR_FAIL("generic type too many arguments", "class Foo<T>{ T a; } Foo<int, float, int>", "ERROR: type has only '1' generic argument(s) while '3' specified");
	TEST_FOR_FAIL("generic type wrong argument count", "class Foo<T, U>{ T a; } Foo<int>", "ERROR: there where only '1' argument(s) to a generic type that expects '2'");
	TEST_FOR_FAIL("generic instance type invisible after instance", "class Foo<T>{ T x, y, z; } Foo<int> x; T a; return a;", "ERROR: variable or function 'T' is not defined");

	TEST_FOR_FAIL("generic type used type in definition 1", "class Foo<T>{ T x; Bar<float> y; } class Bar<T>{ Foo<T ref> x; } Foo<Bar<int> > a; return 0;", "ERROR: while instantiating generic type Bar<int>:");
	TEST_FOR_FAIL("generic type used type in definition 2", "class Foo<T>{ T x; Bar<T ref> y; } class Bar<T>{ Foo<T ref> x; } Foo<Bar<int> > a; return 0;", "ERROR: while instantiating generic type Bar<int>:");

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
return x.foo();", "ERROR: while instantiating generic type Foo<int>:", "ERROR: variable or function 'c' is not defined");

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
return bar();", "ERROR: while instantiating generic type Foo<int>:", "ERROR: variable or function 'c' is not defined");

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
return x.foo();", "ERROR: while instantiating generic function Foo::foo()", "ERROR: variable or function 'c' is not defined");

	TEST_FOR_FAIL("generic type name is too long", 
"class Pair<T, U>{ T i; U j; }\r\n\
Pair<int ref(int ref(int, int), int ref(int, int)), int ref(int ref(int, int), int ref(int, int))> x;\r\n\
Pair<typeof(x), typeof(x)> x1;\r\n\
Pair<typeof(x1), typeof(x1)> x2;\r\n\
Pair<typeof(x2), typeof(x2)> x3;\r\n\
Pair<typeof(x3), typeof(x3)> x4;\r\n\
Pair<typeof(x4), typeof(x4)> x5;\r\n\
return sizeof(x5);", "ERROR: generated generic type name exceeds maximum type length '2048'");

	TEST_FOR_FAIL("generic type member function doesn't exist", "class Foo<T>{ T a; void foo(){} } Foo<int> x; x.food();", "ERROR: function 'Foo::food' is undefined");

	TEST_FOR_FAIL_FULL("complex function specialization fail",
"class Foo<T, U>{ T x; U y; }\r\n\
auto foo(Foo<int, generic> a){ return a.x * a.y; }\r\n\
Foo<int, int> b;\r\n\
Foo<float, float> c;\r\n\
b.x = 6; b.y = 1; c.x = 2; c.y = 1.5;\r\n\
return int(foo(b) + foo(c));",
"line 6 - ERROR: can't find function 'foo' with following parameters:\r\n\
  foo(Foo<float, float>)\r\n\
 the only available are:\r\n\
  int foo(Foo<int, int>)\r\n\
  auto foo(Foo) instanced to\r\n\
    foo(Foo<int, float>)\r\n\
\r\n\
  at \"return int(foo(b) + foo(c));\"\r\n\
                                ^\r\n\
");

	TEST_FOR_FAIL("generic function specialization fail", "class Bar{ typedef int ref iref; } class Foo<T>{ T x; } auto foo(Foo<generic> a){ return a.x; } Bar z; return foo(z);", "ERROR: can't find function 'foo' with following parameters:");
	TEST_FOR_FAIL("generic function specialization fail 2", "class Foo<T>{ T x; } auto foo(Foo<generic> a){ return a.x; } return foo(5);", "ERROR: can't find function 'foo' with following parameters:");
	TEST_FOR_FAIL("generic function specialization fail 3", "class Bar<T>{ T ref ref y; } class Foo<T>{ T x; } auto foo(Foo<generic> a){ return a.x; } Bar<float> z; return foo(z);", "ERROR: can't find function 'foo' with following parameters:");
	TEST_FOR_FAIL("generic function specialization fail 4", "class Foo<T>{ T x; }auto foo(Foo<generic, int> a){ return a.x; }Foo<float> z;return foo(z);", "ERROR: generic type accepts only 1 argument(s)");
	TEST_FOR_FAIL("generic function specialization fail 5", "class Foo<T, U>{ T x; } auto foo(Foo<generic> a){ return a.x; } Foo<int, int> z; return foo(z);", "ERROR: generic type expects 1 more argument(s)");

	TEST_FOR_FAIL("generic function specialization alias double", "class Foo<T, U>{ T x; } auto foo(Foo<@T, @T> x){ T y = x.x; return y + x.x; } Foo<int, float> a; return foo(a);", "ERROR: function 'foo' argument list has multiple 'T' aliases");

	TEST_FOR_FAIL(">> after a non-nester generic type name", "class Foo<T>{ T t; } int c = 1; int a = Foo<int>> c;", "ERROR: operation > is not supported on 'typeid' and 'int'");

	TEST_FOR_FAIL("generic instance type invisible after instance 2", "class Foo<T>{ T x; } Foo<int> x; Foo<int> y; T a;", "ERROR: variable or function 'T' is not defined");

	TEST_FOR_FAIL("generic function specialization fail 6", "class Foo<T>{ T x; } Foo<int> a; a.x = 5; auto foo(Foo<generic> m){ return -m.x; } return foo(&a);", "ERROR: can't find function 'foo' with following parameters:");

	TEST_FOR_FAIL("external function definition syntax inside a type 1", "class Foo{ void Foo:foo(){} }", "ERROR: cannot continue type 'Foo' definition inside 'Foo' type. Possible cause: external member function definition syntax inside a class");
	TEST_FOR_FAIL("external function definition syntax inside a type 2", "class Bar{} class Foo{ void Bar:foo(){} }", "ERROR: cannot continue type 'Bar' definition inside 'Foo' type. Possible cause: external member function definition syntax inside a class");

	TEST_FOR_FAIL_FULL("function pointer selection fail", "void foo(int a){} void foo(double a){}\r\nauto a = foo;\r\nreturn 1;",
"line 2 - ERROR: ambiguity, there is more than one overloaded function available:\r\n\
  foo(void ref(double))\r\n\
 candidates are:\r\n\
  void foo(double)\r\n\
  void foo(int)\r\n\
\r\n\
  at \"auto a = foo;\"\r\n\
               ^\r\n\
");

TEST_FOR_FAIL_FULL("function pointer selection fail 2",
"class Foo<T>{ T a; auto foo(){ return -a; } }\r\n\
int Foo<double>:foo(){ return -a; }\r\n\
Foo<int> x; x.a = 4; Foo<double> s; s.a = 40;\r\n\
auto y = x.foo; auto z = s.foo;\r\n\
return int(y() + z());",
"line 4 - ERROR: ambiguity, there is more than one overloaded function available:\r\n\
  Foo<double>::foo()\r\n\
 candidates are:\r\n\
  int Foo<double>::foo()\r\n\
  double Foo<double>::foo()\r\n\
\r\n\
  at \"auto y = x.foo; auto z = s.foo;\"\r\n\
                               ^\r\n\
");

	TEST_FOR_FAIL("pointer to a generic function", "class Foo{ auto foo(generic x){ return -x; } } Foo z; auto y = z.foo; return y(5);", "ERROR: can't take pointer to a generic function");
	TEST_FOR_FAIL("pointer to a generic function 2", "class Foo{ auto foo(int x){ return -x; } auto foo(generic x){ return -x; } auto bar(){ return foo; } } Foo z; auto y = z.bar(); return y(5);", "ERROR: can't take pointer to a generic function");

	TEST_FOR_FAIL("short inline function fail in variable 1", "int foo(int x){ return 2 * x; } int bar(int a, int ref(double) y){ return y(5.0); } auto x = bar; return x(foo, <x>{ -x; });", "ERROR: cannot find function or variable 'x' which accepts a function with 1 argument(s) as an argument #1");
	TEST_FOR_FAIL("short inline function fail in variable 2", "int bar(int ref(double) y){ return y(5.0); } auto x = bar; int foo(int x){ return x(<x>{ -x; }); }", "ERROR: cannot find function or variable 'x' which accepts a function with 1 argument(s) as an argument #0");
	TEST_FOR_FAIL("short inline function fail in variable 3", "int bar(int ref(double) y){ return y(5.0); } auto x = bar; int foo(){ return x(1, <x>{ -x; }); }", "ERROR: cannot find function or variable 'x' which accepts a function with 1 argument(s) as an argument #1");

	TEST_FOR_FAIL("generic type member function doesn't exist", "class Foo<T>{ T x; } Foo<int> y; auto z = y.bar; return 1;", "ERROR: member variable or function 'bar' is not defined in class 'Foo<int>'");

	TEST_FOR_FAIL("generic function default argument value 2", "auto foo(int y = 1, generic x){}", "ERROR: default argument values are unsupported in generic functions");

	TEST_FOR_FAIL("typedef dies after a generic function instance in an incorrect scope", "auto foo(generic x){ typedef typeof(x) T; T a = x * 2; return a; } int bar(){ int x = foo(5); T y; return x; } return bar();", "ERROR: variable or function 'T' is not defined");

	TEST_FOR_FAIL("alias redefinition in generic type definition", "class Foo<T, T>{ T x; } Foo<int, double> m;", "ERROR: there is already a type or an alias with the same name");

	TEST_FOR_FAIL("generic instance type invisible after instance 3", "class Foo<T>{} void foo(Foo<@T> x){ T y; } Foo<int> a; foo(a); T x; return sizeof(x);", "ERROR: variable or function 'T' is not defined");

	TEST_FOR_FAIL("generic instance type invisible after instance 3", "class Foo<T>{ T x; } void foo(Foo<@T>[] x){ x.x = 5; } Foo<int> a; foo(a); return a.x;", "ERROR: can't find function 'foo' with following parameters:");

	TEST_FOR_FAIL("unable to select function overload", "class Foo{ int y; int boo(generic x){ return x * y; } int boo(int x){ return x * y; } } Foo x; x.y = 6; int ref(double) m = x.boo; return m(2.5);", "ERROR: unable to select function overload for a type 'int ref(double)'");

	TEST_FOR_FAIL("sizeof type in definition", "class Foo{ int t(){ return sizeof(Foo); } int x; } Foo m; return m.t();", "ERROR: cannot take size of a type in definition");

	TEST_FOR_FAIL("wrong array index argument count 1", "int[16] arr; return arr[]", "ERROR: can't find function '[]' with following parameters:");
	TEST_FOR_FAIL("wrong array index argument count 2", "int[16] arr; return arr[2, 3]", "ERROR: can't find function '[]' with following parameters:");

	TEST_FOR_FAIL("foreach restores cycle depth", "int[4] arr; for(i in arr){} break;", "ERROR: break level is greater that loop depth");

	TEST_FOR_FAIL("short inline function in a place where function accepts generic function type specialization and one of arguments is generic", "auto average(generic ref(int, generic, int) f){ return f(8, 2.5, 3); } return average(<x, y, z>{ x * y + z; });", "ERROR: function allows any type for this argument so it must be specified explicitly");

	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(generic ref(int, float) f){ return f(8, 3); } auto f1(int x, int z){ return x * z; } return average(f1);", "ERROR: can't find function 'average' with following parameters:");
	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(generic ref(int, float) f){ return f(8, 3); } auto f1(int x){ return x; } return average(f1);", "ERROR: can't find function 'average' with following parameters:");
	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(generic ref(int) f){ return f(8, 3); } auto f1(int x, int z){ return x + z; } return average(f1);", "ERROR: can't find function 'average' with following parameters:");

	TEST_FOR_FAIL("cannot find suitable function", "class Foo<T>{} auto Foo:average(generic ref(int, T) f){ return f(5, 4); } Foo<int> m; return m.average(<x, y>{ 5+x+y; }, 4);", "ERROR: can't find function 'average' with following parameters:");

	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(generic ref(generic, int) f){ return f(4.0, 2); } auto f2(float x, y){ return x * 2.5; } return average(f2);", "ERROR: can't find function 'average' with following parameters:");
	TEST_FOR_FAIL("wrong function type passed for generic function specialization", "auto average(int ref(generic, int) f){ return f(2, 4); } auto f2(float x, y){ return x * 2.5; } return average(f2);", "ERROR: can't find function 'average' with following parameters:");

	TEST_FOR_FAIL("generic type constructor call without generic arguments", "class Foo<T>{ T curr; } auto Foo:Foo(int start){ curr = start; } Foo<int> a = Foo(5);", "ERROR: generic type arguments in <> are not found after constructor name");
	TEST_FOR_FAIL_GENERIC("generic type constructor call without generic arguments 2", "class list<T>{} class list_iterator<T>{} auto list_iterator:list_iterator(){} auto list:sum(){for(i in list_iterator()){}} list<int> arr; arr.sum();", "ERROR: while instantiating generic function list::sum()", "ERROR: generic type arguments in <> are not found after constructor name");
	
	TEST_FOR_FAIL_GENERIC("generic type member function local function is local", "class Foo<T>{ T x; } auto Foo:foo(){ auto bar(){ return x; } return this.bar(); } Foo<int> m; return m.foo();", "ERROR: while instantiating generic function Foo::foo()", "ERROR: function 'Foo::bar' is undefined");

	TEST_FOR_FAIL_GENERIC("type aliases are taken from instance", "class Foo<T>{} auto Foo:foo(T key){ bar(10); } auto Foo:bar(T key){} Foo<char[]> map; map.foo(\"foo\"); return 1;", "ERROR: while instantiating generic function Foo::foo(generic)", "ERROR: can't find function 'bar' with following parameters:");

	TEST_FOR_FAIL("wrong function return type", "int foo(int ref(generic) f){ return f(4); } auto f2(float x){ return x * 2.5; } return foo(f2);", "ERROR: can't find function 'foo' with following parameters:");
}
