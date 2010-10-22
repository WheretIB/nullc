#include "TestParseFail.h"

#include "TestBase.h"
#include "../NULLC/StrAlgo.h"

void RunParseFailTests()
{
	//TEST_FOR_FAIL("parsing", "");

	TEST_FOR_FAIL("lexer", "return \"", "ERROR: return statement must be followed by ';'");
	TEST_FOR_FAIL("lexer", "return '", "ERROR: return statement must be followed by ';'");

	TEST_FOR_FAIL("parsing", "return 0x;", "ERROR: '0x' must be followed by number");
	TEST_FOR_FAIL("parsing", "int[12 a;", "ERROR: matching ']' not found");
	TEST_FOR_FAIL("parsing", "typeof 12 a;", "ERROR: typeof must be followed by '('");
	TEST_FOR_FAIL("parsing", "typeof(12 a;", "ERROR: ')' not found after expression in typeof");
	TEST_FOR_FAIL("parsing", "typeof() a;", "ERROR: expression not found after typeof(");
	TEST_FOR_FAIL("parsing", "class{}", "ERROR: class name expected");
	TEST_FOR_FAIL("parsing", "class Test int a;", "ERROR: '{' not found after class name");
	TEST_FOR_FAIL("parsing", "class Test{ int; }", "ERROR: class member name expected after type");
	TEST_FOR_FAIL("parsing", "class Test{ int a, ; }", "ERROR: member name expected after ','");
	TEST_FOR_FAIL("parsing", "class Test{ int a, b }", "ERROR: ';' not found after class member list");
	TEST_FOR_FAIL("parsing", "class Test{ int a; return 5;", "ERROR: '}' not found after class definition");
	TEST_FOR_FAIL("parsing", "auto(int a, ){}", "ERROR: variable name not found after type in function variable list");
	TEST_FOR_FAIL("parsing", "void f(int a, b){} return a(1, );", "ERROR: expression not found after ',' in function parameter list");
	TEST_FOR_FAIL("parsing", "void f(int a, b){} return a(1, 2;", "ERROR: ')' not found after function parameter list");
	TEST_FOR_FAIL("parsing", "int operator[(int a, b){ return a+b; }", "ERROR: ']' not found after '[' in operator definition");
	TEST_FOR_FAIL("parsing", "auto(int a, b{}", "ERROR: ')' not found after function variable list");
	TEST_FOR_FAIL("parsing", "auto(int a, b) return 0;", "ERROR: '{' not found after function header");
	TEST_FOR_FAIL("parsing", "auto(int a, b){ return a+b; return 1;", "ERROR: '}' not found after function body");
	TEST_FOR_FAIL("parsing", "int a[4];", "ERROR: array size must be specified after typename");
	TEST_FOR_FAIL("parsing", "int a=;", "ERROR: expression not found after '='");
	TEST_FOR_FAIL("parsing", "int = 3;", "ERROR: unexpected symbol '=' after type name. Variable name is expected at this point");
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
	TEST_FOR_FAIL("parsing", "for({)", "ERROR: '}' not found after '{'");
	TEST_FOR_FAIL("parsing", "for(;;)", "ERROR: condition not found in 'for' statement");
	TEST_FOR_FAIL("parsing", "for(;1<2;{)", "ERROR: '}' not found after '{'");
	TEST_FOR_FAIL("parsing", "for(;1<2;{})", "ERROR: body not found after 'for' header");
	TEST_FOR_FAIL("parsing", "for(;1<2;)", "ERROR: body not found after 'for' header");
	TEST_FOR_FAIL("parsing", "for(;1<2", "ERROR: ';' not found after condition in 'for'");
	TEST_FOR_FAIL("parsing", "for(;1<2;{}", "ERROR: ')' not found after 'for' statement");
	TEST_FOR_FAIL("parsing", "for(;1<2;{}){", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "for(i in ){}", "ERROR: expression expected after 'in'");
	TEST_FOR_FAIL("parsing", "for(0 in ){}", "ERROR: variable name expected before 'in'");
	TEST_FOR_FAIL("parsing", "import std.range; for(i in range(1, 10), int in i){}", "ERROR: variable name expected before 'in'");
	TEST_FOR_FAIL("parsing", "import std.range; for(i in range(1, 10), b in ){}", "ERROR: expression expected after 'in'");
	TEST_FOR_FAIL("parsing", "import std.range; for(i in range(1, 10), int b){}", "ERROR: 'in' expected after variable name");
	TEST_FOR_FAIL("parsing", "while", "ERROR: '(' not found after 'while'");
	TEST_FOR_FAIL("parsing", "while(", "ERROR: expression expected after 'while('");
	TEST_FOR_FAIL("parsing", "while(1", "ERROR: closing ')' not found after expression in 'while' statement");
	TEST_FOR_FAIL("parsing", "while(1)", "ERROR: expression or ';' expected after 'while(...)'");
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
	TEST_FOR_FAIL("parsing", "switch(2){ case", "ERROR: expression expected after 'case' of 'default'");
	TEST_FOR_FAIL("parsing", "switch(2){ case 2", "ERROR: ':' expected");
	TEST_FOR_FAIL("parsing", "switch(2){ case 2:", "ERROR: '}' not found after 'switch' statement");
	TEST_FOR_FAIL("parsing", "switch(2){ default:", "ERROR: '}' not found after 'switch' statement");
	TEST_FOR_FAIL("parsing", "return", "ERROR: return statement must be followed by ';'");
	TEST_FOR_FAIL("parsing", "break", "ERROR: break statement must be followed by ';'");
	TEST_FOR_FAIL("parsing", "for(;1;) continue; continue", "ERROR: continue statement must be followed by ';'");
	TEST_FOR_FAIL("parsing", "(", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("parsing", "(1", "ERROR: closing ')' not found after '('");
	TEST_FOR_FAIL("parsing", "*", "ERROR: variable name not found after '*'");
	TEST_FOR_FAIL("parsing", "int i; i.", "ERROR: member variable expected after '.'");
	TEST_FOR_FAIL("parsing", "int[4] i; i[", "ERROR: expression not found after '['");
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
	TEST_FOR_FAIL("parsing", "return *new integer(4);", "ERROR: type name expected after 'new'");
	TEST_FOR_FAIL("parsing", "new int[", "ERROR: expression not found after '['");
	TEST_FOR_FAIL("parsing", "new int[12", "ERROR: ']' not found after expression");
	TEST_FOR_FAIL("parsing", "auto a = {", "ERROR: value not found after '{'");
	TEST_FOR_FAIL("parsing", "auto a = {1,", "ERROR: value not found after ','");
	TEST_FOR_FAIL("parsing", "auto a = {1,2", "ERROR: '}' not found after inline array");
	TEST_FOR_FAIL("parsing", "return 1+;", "ERROR: terminal expression not found after binary operation");
	TEST_FOR_FAIL("parsing", "1?", "ERROR: expression not found after '?'");
	TEST_FOR_FAIL("parsing", "1?2", "ERROR: ':' not found after expression in ternary operator");
	TEST_FOR_FAIL("parsing", "1?2:", "ERROR: expression not found after ':'");
	TEST_FOR_FAIL("parsing", "int i; i+=;", "ERROR: expression not found after assignment operator");
	TEST_FOR_FAIL("parsing", "int i; i**=;", "ERROR: expression not found after '**='");
	TEST_FOR_FAIL("parsing", "int i", "ERROR: ';' not found after variable definition");
	TEST_FOR_FAIL("parsing", "int i; i = 5", "ERROR: ';' not found after expression");
	TEST_FOR_FAIL("parsing", "{", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "auto ref() a;", "ERROR: return type of a function type cannot be auto");
	TEST_FOR_FAIL("parsing", "int ref(int ref(int, auto), double) b;", "ERROR: parameter type of a function type cannot be auto");
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

	TEST_FOR_FAIL("parsing", "int[$] a; return 0;", "ERROR: unexpected expression after '['");
	TEST_FOR_FAIL("parsing", "int ref(auto, int) a; return 0;", "ERROR: parameter type of a function type cannot be auto");
	TEST_FOR_FAIL("parsing", "int ref(float, int a; return 0;", "ERROR: unexpected symbol '(' after type name. Variable name is expected at this point");
	TEST_FOR_FAIL("parsing", "int func(int){ }", "ERROR: variable name not found after type in function variable list");
	TEST_FOR_FAIL("parsing", "int func(int a = ){ }", "ERROR: default parameter value not found after '='");
	TEST_FOR_FAIL("parsing", "int func(float b, int a = ){ }", "ERROR: default parameter value not found after '='");

	TEST_FOR_FAIL("parsing", "void func(){} auto duck(){ return func; } duck()(1,); ", "ERROR: expression not found after ',' in function parameter list");
	TEST_FOR_FAIL("parsing", "void func(){} auto duck(){ return func; } duck()(1,2; ", "ERROR: ')' not found after function parameter list");
	TEST_FOR_FAIL("parsing", "int b; b = ", "ERROR: expression not found after '='");
	TEST_FOR_FAIL("parsing", "noalign int a, b", "ERROR: ';' not found after variable definition");

	TEST_FOR_FAIL("parsing", "auto x = @4; return x;", "ERROR: string expected after '@'");

	TEST_FOR_FAIL("parsing", "coroutine foo foo(){}", "ERROR: function return type not found after 'coroutine'");
	TEST_FOR_FAIL("parsing", "coroutine int foo){}", "ERROR: '(' expected after function name");
	TEST_FOR_FAIL("parsing", "int operator({}", "ERROR: ')' not found after '(' in operator definition");

	TEST_FOR_FAIL("parsing", "import {}", "ERROR: string expected after import");
	TEST_FOR_FAIL("parsing", "import std.", "ERROR: string expected after '.'");
	TEST_FOR_FAIL("parsing", "import std.range", "ERROR: ';' not found after import expression");
	TEST_FOR_FAIL("parsing", "%", "ERROR: unexpected symbol");
	TEST_FOR_FAIL("parsing", "", "ERROR: module contains no code");

	{
		char code[8192];
		char name[NULLC_MAX_VARIABLE_NAME_LENGTH + 1];
		for(unsigned int i = 0; i < NULLC_MAX_VARIABLE_NAME_LENGTH + 1; i++)
			name[i] = 'a';
		name[NULLC_MAX_VARIABLE_NAME_LENGTH] = 0;
		SafeSprintf(code, 8192, "int %s = 12; return %s;", name, name);
		char error[128];
		SafeSprintf(error, 128, "ERROR: variable name length is limited to %d symbols", NULLC_MAX_VARIABLE_NAME_LENGTH);
		TEST_FOR_FAIL("Variable name is too long.", code, error);
	}

	TEST_FOR_FAIL("parsing", "bar(<!i>{ i; });", "ERROR: function argument name not found after '<'");
	TEST_FOR_FAIL("parsing", "bar(<i, >{ i; });", "ERROR: function argument name not found after ','");
	TEST_FOR_FAIL("parsing", "void bar(void ref(int, int) x); bar(<i, j{ i; });", "ERROR: '>' expected after short inline function argument list");
	TEST_FOR_FAIL("parsing", "void bar(void ref(int, int) x); bar(<i, j> i; });", "ERROR: '{' not found after function header");
	TEST_FOR_FAIL("parsing", "void bar(void ref(int, int) x); bar(<i, j>{ i; );", "ERROR: '}' not found after function body");

	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).name;", "ERROR: expected 'argument'/'return'/'target' at this point");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo(1,2,3)).argument;", "ERROR: 'argument' can only be applied to a function type, but we have 'double'");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument;", "ERROR: expected '.first'/'.last'/'[N]' at this point");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument.;", "ERROR: expected 'first'/'last' at this point");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument[;", "ERROR: argument number expected after '['");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument[1;", "ERROR: expected ']'");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument[10;", "ERROR: this function type 'double ref(int,int,int)' has only 3 argument(s)");
	TEST_FOR_FAIL("parsing", "double foo(){ return 0; } typeof(foo).argument.first;", "ERROR: this function type 'double ref()' doesn't have arguments");
	TEST_FOR_FAIL("parsing", "double foo(){ return 0; } typeof(foo).argument.last;", "ERROR: this function type 'double ref()' doesn't have arguments");
	TEST_FOR_FAIL("parsing", "return double == typeof(1).return;", "ERROR: 'return' can only be applied to a function type, but we have 'int'");
	TEST_FOR_FAIL("parsing", "return double == typeof(1).target;", "ERROR: 'target' can only be applied to a pointer or array type, but we have 'int'");

	TEST_FOR_FAIL("parsing", "auto foo(generic a = 5){ return -a; } return foo();", "ERROR: default argument values are unsupported in generic functions");
	TEST_FOR_FAIL("parsing", "auto foo(generic a, b = 4){ return -a + b; } return foo(5);", "ERROR: default argument values are unsupported in generic functions");

	TEST_FOR_FAIL("parsing", "coroutine int +foo(){}", "ERROR: function name not found after return type");

	TEST_FOR_FAIL("parsing", "int foo(generic a);", "ERROR: generic function cannot be forward-declared");
	TEST_FOR_FAIL("parsing", "int foo(generic a) return 1; }", "ERROR: '{' not found after function header");
	TEST_FOR_FAIL("parsing", "int foo(generic a){ return ##a; }", "ERROR: unknown lexeme in function body");

	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).arguments;", "ERROR: expected 'argument'/'return'/'target' at this point");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument.firsta;", "ERROR: expected 'first'/'last' at this point");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).argument.lastu;", "ERROR: expected 'first'/'last' at this point");
	TEST_FOR_FAIL("parsing", "double foo(int i, j, k){ return i * j + k; } typeof(foo).returnee;", "ERROR: expected 'argument'/'return'/'target' at this point");
	TEST_FOR_FAIL("parsing", "int u; return int == typeof(&u).targetme;", "ERROR: expected 'argument'/'return'/'target' at this point");

	TEST_FOR_FAIL("parsing", "@if(1) int a; else float a;", "ERROR: '{' not found after 'if' in static if");
	TEST_FOR_FAIL("parsing", "@if(0){ int #; else float a;", "ERROR: unknown lexeme in 'if' body");
	TEST_FOR_FAIL("parsing", "@if(0){ int a; }else float b;", "ERROR: '{' not found after 'else' in static if");
	TEST_FOR_FAIL("parsing", "@if(0){ int a; }else{", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "@if(0){ int a; }else{ float b;", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "@if(1){", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "@if(1){ int a; ", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "@if(1){ int a; }else float b;", "ERROR: '{' not found after 'else' in static if");
	TEST_FOR_FAIL("parsing", "@if(1){ int a; }else{ float #;", "ERROR: unknown lexeme in 'else' body");
	TEST_FOR_FAIL("parsing", "@if((auto(){ return 1; })()){ int a; }", "ERROR: couldn't evaluate condition at compilation time");
}
