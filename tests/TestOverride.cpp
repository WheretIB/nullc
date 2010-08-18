#include "TestBase.h"

const char	*testFunctionOverrideInternal =
"import std.dynamic;\r\n\
\r\n\
int funcA(int x){ return -x; }\r\n\
int funcB(int x){ return x * 2; }\r\n\
\r\n\
int a = funcA(5);\r\n\
int b = funcB(5);\r\n\
\r\n\
override(funcA, funcB);\r\n\
override(funcB, auto(int y){ return y * 3 - 2; });\r\n\
\r\n\
return funcA(5) * funcB(5);";
TEST("Function override between internal functions", testFunctionOverrideInternal, "130")
{
	CHECK_INT("a", 0, -5);
	CHECK_INT("b", 0, 10);
}

int RewriteA(int x)
{
	return -x;
}

int RewriteB(int x)
{
	return x * 3 - 2;
}

LOAD_MODULE_BIND(func_rewrite, "func.rewrite", "int funcA(int x); int funcNew(int x);")
{
	nullcBindModuleFunction("func.rewrite", (void(*)())RewriteA, "funcA", 0);
	nullcBindModuleFunction("func.rewrite", (void(*)())RewriteB, "funcNew", 0);
}

const char	*testFunctionOverrideInternalExternal =
"import std.dynamic;\r\n\
import func.rewrite;\r\n\
\r\n\
int funcB(int x){ return x * 2; }\r\n\
\r\n\
int a = funcA(5);\r\n\
int b = funcB(5);\r\n\
\r\n\
override(funcA, funcB);\r\n\
override(funcB, funcNew);\r\n\
\r\n\
return funcA(5) * funcB(5);";
TEST_VM("Function override between internal and external functions", testFunctionOverrideInternalExternal, "130")
{
	CHECK_INT("a", 0, -5);
	CHECK_INT("b", 0, 10);
}

const char	*testFunctionOverrideInternalPtr =
"import std.dynamic;\r\n\
\r\n\
int funcA(int x){ return -x; }\r\n\
int funcB(int x){ return x * 2; }\r\n\
auto a_ = funcA;\r\n\
auto b_ = funcB;\r\n\
\r\n\
int a = a_(5);\r\n\
int b = b_(5);\r\n\
\r\n\
override(funcA, funcB);\r\n\
override(funcB, auto(int y){ return y * 3 - 2; });\r\n\
\r\n\
return a_(5) * b_(5);";
TEST("Function override between internal functions (with function pointers)", testFunctionOverrideInternalPtr, "130")
{
	CHECK_INT("a", 0, -5);
	CHECK_INT("b", 0, 10);
}

const char	*testFunctionOverrideInternalExternalPtr =
"import std.dynamic;\r\n\
import func.rewrite;\r\n\
\r\n\
int funcB(int x){ return x * 2; }\r\n\
auto a_ = funcA;\r\n\
auto b_ = funcB;\r\n\
\r\n\
int a = a_(5);\r\n\
int b = b_(5);\r\n\
\r\n\
override(funcA, funcB);\r\n\
override(funcB, funcNew);\r\n\
\r\n\
return a_(5) * b_(5);";
TEST_VM("Function override between internal and external functions (with function pointers)", testFunctionOverrideInternalExternalPtr, "130")
{
	CHECK_INT("a", 0, -5);
	CHECK_INT("b", 0, 10);
}
