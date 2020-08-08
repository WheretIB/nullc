#include "TestParseFail.h"

#include "TestBase.h"
#include "../NULLC/StrAlgo.h"

void RunParseFailTests()
{
	//TEST_FOR_FAIL("parsing", "");

	TEST_FOR_FAIL("lexer", "return \"", "ERROR: unclosed string constant");
	TEST_FOR_FAIL("lexer", "return '", "ERROR: unclosed character constant");

	TEST_FOR_FAIL("parsing", "return 0x;", "ERROR: '0x' must be followed by number");
	TEST_FOR_FAIL("parsing", "return 0101bb;", "ERROR: unknown number suffix 'bb'");
	TEST_FOR_FAIL("parsing", "return 2lll;", "ERROR: unknown number suffix 'lll'");
	TEST_FOR_FAIL("parsing", "return 1.3ffff;", "ERROR: unknown number suffix 'ffff'");
	TEST_FOR_FAIL("parsing", "int[12 a;", "ERROR: ']' not found after expression");
	TEST_FOR_FAIL("parsing", "typeof 12 a;", "ERROR: typeof must be followed by '('");
	TEST_FOR_FAIL("parsing", "typeof(12 a;", "ERROR: ')' not found after expression in typeof");
	TEST_FOR_FAIL("parsing", "typeof() a;", "ERROR: expression not found after typeof(");
	TEST_FOR_FAIL("parsing", "auto two(generic a, typeof() ab){ return a + b; } return two(1, 2);", "ERROR: expression not found after typeof(");
	TEST_FOR_FAIL("parsing", "class{}", "ERROR: class name expected");
	TEST_FOR_FAIL("parsing", "class Test int a;", "ERROR: '{' not found after class name");
	TEST_FOR_FAIL("parsing", "class Test{ int; }", "ERROR: class member name expected after type");
	TEST_FOR_FAIL("parsing", "class Test{ int a, ; }", "ERROR: member name expected after ','");
	TEST_FOR_FAIL("parsing", "class Test{ int a, b }", "ERROR: ';' not found after class member list");
	TEST_FOR_FAIL("parsing", "class Test{ int a; return 5;", "ERROR: '}' not found after class definition");
	TEST_FOR_FAIL("parsing", "auto(int a, ){}", "ERROR: argument name not found after ',' in function argument list");
	TEST_FOR_FAIL("parsing", "void f(int a, b){} return a(1, );", "ERROR: expression not found after ',' in function argument list");
	TEST_FOR_FAIL("parsing", "void f(int a, b){} return a(1, 2;", "ERROR: ')' not found after function argument list");
	TEST_FOR_FAIL("parsing", "int operator[(int a, b){ return a+b; }", "ERROR: ']' not found after '[' in operator definition");
	TEST_FOR_FAIL("parsing", "auto(int a, b{}", "ERROR: ')' not found after function variable list");
	TEST_FOR_FAIL("parsing", "auto(int a, b) return 0;", "ERROR: '{' not found after function header");
	TEST_FOR_FAIL("parsing", "auto(int a, b){ return a+b; return 1;", "ERROR: '}' not found after function body");
	TEST_FOR_FAIL("parsing", "int a[4];", "ERROR: array size must be specified after type name");
	TEST_FOR_FAIL("parsing", "int a=;", "ERROR: expression not found after '='");
	TEST_FOR_FAIL("parsing", "int a, ;", "ERROR: next variable definition excepted after ','");
	TEST_FOR_FAIL("parsing", "align int a=2;", "ERROR: '(' expected after align");
	TEST_FOR_FAIL("parsing", "align() int a = 2;", "ERROR: alignment value not found after align(");
	TEST_FOR_FAIL("parsing", "align(2 int a;", "ERROR: ')' expected after alignment value");
	TEST_FOR_FAIL("parsing", "if", "ERROR: '(' not found after 'if'");
	TEST_FOR_FAIL("parsing", "if(", "ERROR: condition not found in 'if' statement");
	TEST_FOR_FAIL("parsing", "if(1", "ERROR: closing ')' not found after 'if' condition");
	TEST_FOR_FAIL("parsing", "if(1)", "ERROR: expression not found after 'if'");
	TEST_FOR_FAIL("parsing", "if(1) 1; else ", "ERROR: expression not found after 'else'");
	TEST_FOR_FAIL("parsing", "for", "ERROR: '(' not found after 'for'");
	TEST_FOR_FAIL("parsing", "for({})", "ERROR: ';' not found after initializer in 'for'");
	TEST_FOR_FAIL("parsing", "for(;1<2)", "ERROR: ';' not found after condition in 'for'");
	TEST_FOR_FAIL("parsing", "for({)", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "for(;;)", "ERROR: condition not found in 'for' statement");
	TEST_FOR_FAIL("parsing", "for(;1<2;{)", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "for(;1<2;{})", "ERROR: body not found after 'for' header");
	TEST_FOR_FAIL("parsing", "for(;1<2;)", "ERROR: body not found after 'for' header");
	TEST_FOR_FAIL("parsing", "for(;1<2", "ERROR: ';' not found after condition in 'for'");
	TEST_FOR_FAIL("parsing", "for(;1<2;{}", "ERROR: ')' not found after 'for' statement");
	TEST_FOR_FAIL("parsing", "for(;1<2;{}){", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "for(i in ){}", "ERROR: expression expected after 'in'");
	TEST_FOR_FAIL("parsing", "for(0 in ){}", "ERROR: expression not found after binary operation");
	TEST_FOR_FAIL("parsing", "import std.range; for(i in range(1, 10), b in ){}", "ERROR: expression expected after 'in'");
	TEST_FOR_FAIL("parsing", "import std.range; for(i in range(1, 10), int b){}", "ERROR: 'in' expected after variable name");
	TEST_FOR_FAIL("parsing", "while", "ERROR: '(' not found after 'while'");
	TEST_FOR_FAIL("parsing", "while(", "ERROR: expression expected after 'while('");
	TEST_FOR_FAIL("parsing", "while(1", "ERROR: closing ')' not found after expression in 'while' statement");
	TEST_FOR_FAIL("parsing", "while(1)", "ERROR: body not found after 'while' header");
	TEST_FOR_FAIL("parsing", "do", "ERROR: expression expected after 'do'");
	TEST_FOR_FAIL("parsing", "do 1;", "ERROR: 'while' expected after 'do' statement");
	TEST_FOR_FAIL("parsing", "do 1; while", "ERROR: '(' not found after 'while'");
	TEST_FOR_FAIL("parsing", "do 1; while(", "ERROR: expression expected after 'while('");
	TEST_FOR_FAIL("parsing", "do 1; while(1", "ERROR: closing ')' not found after expression in 'while' statement");
	TEST_FOR_FAIL("parsing", "do 1; while(0)", "ERROR: while(...) should be followed by ';'");
	TEST_FOR_FAIL("parsing", "switch", "ERROR: '(' not found after 'switch'");
	TEST_FOR_FAIL("parsing", "switch(", "ERROR: expression not found after 'switch('");
	TEST_FOR_FAIL("parsing", "switch(2", "ERROR: closing ')' not found after expression in 'switch' statement");
	TEST_FOR_FAIL("parsing", "switch(2)", "ERROR: '{' not found after 'switch(...)'");
	TEST_FOR_FAIL("parsing", "switch(2){ case", "ERROR: expression expected after 'case'");
	TEST_FOR_FAIL("parsing", "switch(2){ case 2", "ERROR: ':' expected");
	TEST_FOR_FAIL("parsing", "switch(2){ case 2:", "ERROR: '}' not found after 'switch' statement");
	TEST_FOR_FAIL("parsing", "switch(2){ default:", "ERROR: '}' not found after 'switch' statement");
	TEST_FOR_FAIL("parsing", "return", "ERROR: return statement must be followed by an expression or ';'");
	TEST_FOR_FAIL("parsing", "yield", "ERROR: yield statement must be followed by an expression or ';'");
	TEST_FOR_FAIL("parsing", "break", "ERROR: break statement must be followed by ';' or a constant");
	TEST_FOR_FAIL("parsing", "for(;1;) continue; continue", "ERROR: continue statement must be followed by ';' or a constant");
	TEST_FOR_FAIL("parsing", "(", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("parsing", "(1", "ERROR: closing ')' not found after '('");
	TEST_FOR_FAIL("parsing", "*", "ERROR: expression not found after '*'");
	TEST_FOR_FAIL("parsing", "int i; i.", "ERROR: member name expected after '.'");
	TEST_FOR_FAIL("parsing", "int[4] i; i[2", "ERROR: ']' not found after expression");
	TEST_FOR_FAIL("parsing", "&", "ERROR: variable not found after '&'");
	TEST_FOR_FAIL("parsing", "!", "ERROR: expression not found after '!'");
	TEST_FOR_FAIL("parsing", "~", "ERROR: expression not found after '~'");
	TEST_FOR_FAIL("parsing", "--", "ERROR: variable not found after '--'");
	TEST_FOR_FAIL("parsing", "++", "ERROR: variable not found after '++'");
	TEST_FOR_FAIL("parsing", "+", "ERROR: expression not found after '+'");
	TEST_FOR_FAIL("parsing", "-", "ERROR: expression not found after '-'");
	TEST_FOR_FAIL("parsing", "'aa'", "ERROR: only one character can be inside single quotes");
	TEST_FOR_FAIL("parsing", "sizeof", "ERROR: sizeof must be followed by '('");
	TEST_FOR_FAIL("parsing", "sizeof(", "ERROR: expression or type not found after sizeof(");
	TEST_FOR_FAIL("parsing", "sizeof(int", "ERROR: ')' not found after expression in sizeof");
	TEST_FOR_FAIL("parsing", "new", "ERROR: type name expected after 'new'");
	TEST_FOR_FAIL("parsing", "return *new +(4);", "ERROR: type name expected after 'new'");
	TEST_FOR_FAIL("parsing", "new int[", "ERROR: matching ']' not found");
	TEST_FOR_FAIL("parsing", "new int[12", "ERROR: ']' not found after expression");
	TEST_FOR_FAIL("parsing", "auto a = {", "ERROR: value not found after '{'");
	TEST_FOR_FAIL("parsing", "auto a = {1,", "ERROR: value not found after ','");
	TEST_FOR_FAIL("parsing", "auto a = {1,2", "ERROR: '}' not found after inline array");
	TEST_FOR_FAIL("parsing", "return 1+;", "ERROR: expression not found after binary operation");
	TEST_FOR_FAIL("parsing", "1?", "ERROR: expression not found after '?'");
	TEST_FOR_FAIL("parsing", "1?2", "ERROR: ':' not found after expression in ternary operator");
	TEST_FOR_FAIL("parsing", "1?2:", "ERROR: expression not found after ':'");
	TEST_FOR_FAIL("parsing", "int i; i+=;", "ERROR: expression not found after '+=' operator");
	TEST_FOR_FAIL("parsing", "int i; i**=;", "ERROR: expression not found after '**=' operator");
	TEST_FOR_FAIL("parsing", "int i", "ERROR: ';' not found after variable definition");
	TEST_FOR_FAIL("parsing", "int i; i = 5", "ERROR: ';' not found after expression");
	TEST_FOR_FAIL("parsing", "{", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "auto ref() a;", "ERROR: return type of a function type cannot be auto");
	TEST_FOR_FAIL("parsing", "int ref(int ref(int, auto), double) b;", "ERROR: function argument cannot be an auto type");
	TEST_FOR_FAIL("parsing", "typedef", "ERROR: typename expected after typedef");
	TEST_FOR_FAIL("parsing", "typedef double", "ERROR: alias name expected after typename in typedef expression");
	TEST_FOR_FAIL("parsing", "typedef double somename", "ERROR: ';' not found after typedef");
	TEST_FOR_FAIL("parsing", "typedef double int;", "ERROR: there is already a type or an alias with the same name");
	TEST_FOR_FAIL("parsing", "typedef double somename; typedef int somename;", "ERROR: there is already a type or an alias with the same name");

	TEST_FOR_FAIL("parsing", "class Test{ int a{ } return 0;", "ERROR: 'get' is expected after '{'");
	TEST_FOR_FAIL("parsing", "class Test{ int a{ get } return 0;", "ERROR: function body expected after 'get'");
	TEST_FOR_FAIL("parsing", "class Test{ int a{ get{ return 0; } set } return 0;", "ERROR: function body expected after 'set'");
	TEST_FOR_FAIL("parsing", "class Test{ int a{ get{ return 0; } set( } return 0;", "ERROR: r-value name not found after '('");
	TEST_FOR_FAIL("parsing", "class Test{ int a{ get{ return 0; } set(value } return 0;", "ERROR: ')' not found after r-value");
	TEST_FOR_FAIL("parsing", "class Test{ int a{ get{ return 0; } set(value){ } ", "ERROR: '}' is expected after property");

	TEST_FOR_FAIL("parsing", "int[$] a; return 0;", "ERROR: matching ']' not found");
	TEST_FOR_FAIL("parsing", "int ref(auto, int) a; return 0;", "ERROR: function argument cannot be an auto type");
	TEST_FOR_FAIL("parsing", "int ref(float, int a; return 0;", "ERROR: ')' not found after function argument list");
	TEST_FOR_FAIL("parsing", "int func(int){ }", "ERROR: variable name not found after type in function variable list");
	TEST_FOR_FAIL("parsing", "int func(int a = ){ }", "ERROR: default argument value not found after '='");
	TEST_FOR_FAIL("parsing", "int func(float b, int a = ){ }", "ERROR: default argument value not found after '='");

	TEST_FOR_FAIL("parsing", "void func(){} auto duck(){ return func; } duck()(1,); ", "ERROR: expression not found after ',' in function argument list");
	TEST_FOR_FAIL("parsing", "void func(){} auto duck(){ return func; } duck()(1,2; ", "ERROR: ')' not found after function argument list");
	TEST_FOR_FAIL("parsing", "int b; b = ", "ERROR: expression not found after '='");
	TEST_FOR_FAIL("parsing", "noalign int a, b", "ERROR: ';' not found after variable definition");

	TEST_FOR_FAIL("parsing", "auto x = @4; return x;", "ERROR: name expected after '@'");

	TEST_FOR_FAIL("parsing", "coroutine + foo(){}", "ERROR: function return type not found after 'coroutine'");
	TEST_FOR_FAIL("parsing", "coroutine int foo){}", "ERROR: '(' expected after function name");
	TEST_FOR_FAIL("parsing", "int operator({}", "ERROR: ')' not found after '(' in operator definition");

	TEST_FOR_FAIL("parsing", "import {}", "ERROR: name expected after import");
	TEST_FOR_FAIL("parsing", "import std.", "ERROR: name expected after '.'");
	TEST_FOR_FAIL("parsing", "import std.range", "ERROR: ';' not found after import expression");
	TEST_FOR_FAIL("parsing", "%", "ERROR: unexpected symbol");
	TEST_FOR_FAIL("parsing", "", "ERROR: module contains no code");

	{
		char code[8192];
		char name[NULLC_MAX_VARIABLE_NAME_LENGTH + 1];
		for(unsigned int i = 0; i < NULLC_MAX_VARIABLE_NAME_LENGTH + 1; i++)
			name[i] = 'a';
		name[NULLC_MAX_VARIABLE_NAME_LENGTH] = 0;
		NULLC::SafeSprintf(code, 8192, "int %s = 12; return %s;", name, name);
		char error[128];
		NULLC::SafeSprintf(error, 128, "ERROR: variable name length is limited to %d symbols", NULLC_MAX_VARIABLE_NAME_LENGTH);
		TEST_FOR_FAIL("Variable name is too long.", code, error);
	}

	TEST_FOR_FAIL("parsing", "bar(<!i>{ i; });", "ERROR: function argument name not found after '<'");
	TEST_FOR_FAIL("parsing", "bar(<i, >{ i; });", "ERROR: function argument name not found after ','");
	TEST_FOR_FAIL("parsing", "void bar(void ref(int, int) x); bar(<i, j{ i; });", "ERROR: '>' expected after short inline function argument list");
	TEST_FOR_FAIL("parsing", "void bar(void ref(int, int) x); bar(<i, j>{ i; );", "ERROR: '}' not found after function body");

	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo(1,2,3)).argument;", "ERROR: 'argument' can only be applied to a function type, but we have 'double'");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument;", "ERROR: expected '.first'/'.last'/'[N]'/'.size' after 'argument'");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument.;", "ERROR: member name expected after '.'");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument[;", "ERROR: ']' not found after expression");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument[1;", "ERROR: ']' not found after expression");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument[10];", "ERROR: function arguemnt set '(int,int,int)' has only 3 argument(s)");
	TEST_FOR_FAIL("parsing", "double foo(){ return 0; } typeof(foo).argument.first;", "ERROR: function argument set is empty");
	TEST_FOR_FAIL("parsing", "double foo(){ return 0; } typeof(foo).argument.last;", "ERROR: function argument set is empty");
	TEST_FOR_FAIL("parsing", "return double == typeof(1).return;", "ERROR: 'return' can only be applied to a function type, but we have 'int'");
	TEST_FOR_FAIL("parsing", "return double == typeof(1).target;", "ERROR: 'target' can only be applied to a pointer or array type, but we have 'int'");

	TEST_FOR_FAIL("parsing", "coroutine int +foo(){}", "ERROR: function name not found after return type");

	TEST_FOR_FAIL("parsing", "int foo(generic a);", "ERROR: generic function cannot be forward-declared");
	TEST_FOR_FAIL("parsing", "int foo(generic a) return 1; }", "ERROR: '{' not found after function header");
	TEST_FOR_FAIL("parsing", "int foo(generic a){ return ##a; }", "ERROR: return statement must be followed by an expression or ';'");

	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).arguments;", "ERROR: member variable or function 'arguments' is not defined in class 'typeid'");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument.firsta;", "ERROR: expected '.first'/'.last'/'[N]'/'.size' after 'argument'");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument.lastu;", "ERROR: expected '.first'/'.last'/'[N]'/'.size' after 'argument'");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).returnee;", "ERROR: member variable or function 'returnee' is not defined in class 'typeid'");
	TEST_FOR_FAIL("parsing", "int u; return int == typeof(&u).targetme;", "ERROR: member variable or function 'targetme' is not defined in class 'typeid'");

	TEST_FOR_FAIL("parsing", "@if(0){ int #; else float a;", "ERROR: unknown lexeme");
	TEST_FOR_FAIL("parsing", "@if(0){ int a; }else{", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "@if(0){ int a; }else{ float b;", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "@if(1){", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "@if(1){ int a; ", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "@if(1){ int a; }else{ float #;", "ERROR: unknown lexeme");
	TEST_FOR_FAIL("parsing", "int x = 3; @if((auto(){ return x; })()){ int a; }", "ERROR: couldn't evaluate condition at compilation time");

	TEST_FOR_FAIL("parsing", "typeof(1).isReference.target x;", "ERROR: typeof expression result is not a type");
	TEST_FOR_FAIL("parsing", "typeof(1).isReference ref x;", "ERROR: typeof expression result is not a type");
	TEST_FOR_FAIL("parsing", "typeof(1).isReference[4] x;", "ERROR: typeof expression result is not a type");

	TEST_FOR_FAIL("parsing", "typeof(1).arraySize x;", "ERROR: 'arraySize' can only be applied to an array type, but we have 'int'");

	TEST_FOR_FAIL("parsing", "class Foo<>{}", "ERROR: generic type alias required after '<'");
	TEST_FOR_FAIL("parsing", "class Foo<T, >{}", "ERROR: generic type alias required after ','");
	TEST_FOR_FAIL("parsing", "class Foo<T, U{}", "ERROR: '>' expected after generic type alias list");
	TEST_FOR_FAIL("parsing", "class Foo<T, U>}", "ERROR: '{' not found after class name");
	TEST_FOR_FAIL("parsing", "class Foo<T>{", "ERROR: '}' not found after class definition");

	TEST_FOR_FAIL("parsing", "class Foo<T>{ T x; } Foo a;", "ERROR: initializer is required to resolve generic type 'Foo'");
	TEST_FOR_FAIL("parsing", "int<float> a;", "ERROR: type 'int' can't have generic arguments");
	TEST_FOR_FAIL("parsing", "class Foo<T, U>{} Foo<> a;", "ERROR: typename required after '<'");
	TEST_FOR_FAIL("parsing", "class Foo<T, U>{} Foo<int, > b;", "ERROR: typename required after ','");
	TEST_FOR_FAIL("parsing", "class Foo<T, U>{} Foo<int, int b;", "ERROR: '>' expected after generic type alias list");

	TEST_FOR_FAIL("parsing", "class Foo{ int a; flaot b; }", "ERROR: 'flaot' is not a known type name");

	TEST_FOR_FAIL("parsing", "class Foo<int>{ int a; } Foo<double> x;", "ERROR: there is already a type or an alias with the same name");

	TEST_FOR_FAIL("parsing", "align(4) c", "ERROR: variable or class definition is expected after alignment specifier");

	TEST_FOR_FAIL("parsing", "@if(1) }", "ERROR: expression not found after 'if'");
	TEST_FOR_FAIL("parsing", "@if(0) return 1; else }", "ERROR: expression not found after 'else'");

	TEST_FOR_FAIL("parsing", "class Foo{ int[5] arr; } return Foo.arr2.arraySize;", "ERROR: member variable or function 'arr2' is not defined in class 'typeid'");

	TEST_FOR_FAIL("parsing", "class Foo{ const; }", "ERROR: type name expected after const");
	TEST_FOR_FAIL("parsing", "class Foo{ const int ref a = nullptr; }", "ERROR: only basic numeric types can be used as constants");
	TEST_FOR_FAIL("parsing", "class Foo{ const void a; }", "ERROR: '=' not found after constant name");
	TEST_FOR_FAIL("parsing", "class Foo{ const int; }", "ERROR: constant name expected after type");
	TEST_FOR_FAIL("parsing", "class Foo{ const int a; }", "ERROR: '=' not found after constant name");
	TEST_FOR_FAIL("parsing", "class Foo{ const int a = ; }", "ERROR: expression not found after '='");
	TEST_FOR_FAIL("parsing", "class Foo{ const int a = 4, ; }", "ERROR: constant name expected after ','");
	TEST_FOR_FAIL("parsing", "class Foo{ const int a = 4, b }", "ERROR: ';' not found after constants");
	TEST_FOR_FAIL("parsing", "class Foo{ const float a = 4, b; }", "ERROR: only integer constant list gets automatically incremented by 1");

	TEST_FOR_FAIL("parsing", "char a = '';", "ERROR: empty character constant");

	TEST_FOR_FAIL("parsing", "int i; typeof(5).isArray x;", "ERROR: typeof expression result is not a type");

	TEST_FOR_FAIL("parsing", "int foo(int i = 4, j){ return i + j; } return foo(j: 5, 2);", "ERROR: function argument name expected after ','");
	TEST_FOR_FAIL("parsing", "int foo(int i = 4, j){ return i + j; } return foo(j: 5, i: );", "ERROR: expression not found after ':' in function argument list");
	TEST_FOR_FAIL("parsing", "int foo(int i = 4, j){ return i + j; } return foo(j: );", "ERROR: expression not found after ':' in function argument list");

	TEST_FOR_FAIL("parsing", "int[4] arr; (typeof(arr).arraySize ref)(1);", "ERROR: typeof expression result is not a type");
	TEST_FOR_FAIL("parsing", "namespace", "ERROR: namespace name required");
	TEST_FOR_FAIL("parsing", "namespace Bar", "ERROR: '{' not found after namespace name");
	TEST_FOR_FAIL("parsing", "namespace Bar{", "ERROR: '}' not found after namespace body");
	TEST_FOR_FAIL("parsing", "class Foo{ const int b = 4; int a; } Foo.b x;", "ERROR: ';' not found after expression");
	TEST_FOR_FAIL("parsing", "enum", "ERROR: enum name expected");
	TEST_FOR_FAIL("parsing", "enum X", "ERROR: '{' not found after enum name");
	TEST_FOR_FAIL("parsing", "enum X{", "ERROR: enumeration name expected after '{'");
	TEST_FOR_FAIL("parsing", "enum X{ A", "ERROR: '}' not found after enum definition");
	TEST_FOR_FAIL("parsing", "enum X{ A, ", "ERROR: enumeration name expected after ','");
	TEST_FOR_FAIL("parsing", "enum X{ A =", "ERROR: expression not found after '='");
	TEST_FOR_FAIL("parsing", "class X{ int x; } new X(){", "ERROR: '}' not found after custom constructor body");
	TEST_FOR_FAIL("parsing", "new int(", "ERROR: ')' not found after function argument list");
	TEST_FOR_FAIL("parsing", "int a = typeof(1)(", "ERROR: ')' not found after function argument list");

	TEST_FOR_FAIL("parsing", "int i = 0; do{ return i;", "ERROR: closing '}' not found");

	TEST_FOR_FAIL("parsing", "int foo(explicit explicit int){ return 4; }", "ERROR: 'explicit' is not a known type name");
	TEST_FOR_FAIL("parsing", "int foo(explicit){ return 4; }", "ERROR: type name not found after 'explicit' specifier");
	TEST_FOR_FAIL("parsing", "int foo(explicit a){ return 4; }", "ERROR: variable name not found after type in function variable list");
	TEST_FOR_FAIL("parsing", "int foo(explicit int a, explicit b){ return 4; }", "ERROR: variable name not found after type in function variable list");

	TEST_FOR_FAIL("parsing", "int y; auto x = coroutine y;", "ERROR: function name not found after return type");
	TEST_FOR_FAIL("parsing", "int y; auto x = coroutine int y;", "ERROR: '(' expected after function name");

	TEST_FOR_FAIL("parsing", "namespace Test{} return Test.+(5);", "ERROR: namespace member is expected after '.'");
	TEST_FOR_FAIL("parsing", "int x = ( ((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((1)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))) );", "ERROR: reached nested '(' limit of 256");
	TEST_FOR_FAIL("parsing", "auto x = { {{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{1}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}} };", "ERROR: reached nested array limit of 256");
	TEST_FOR_FAIL("parsing", "{ {{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{int x;}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}} }", "ERROR: reached nested '{' limit of 256");

	TEST_FOR_FAIL("parsing", "int r = 0; switch(4){ default: r = 2; break; default: r = 5; break; } return r;", "ERROR: default switch case is already defined");
	TEST_FOR_FAIL("parsing", "int r = 0; switch(4){ default: r = 2; break; case 4: r = 3; } return r;", "ERROR: default switch case can't be followed by more cases");
}
