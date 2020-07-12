#include "runtime.h"

struct Char{ char a; };
int Call_int_ref_Char_(Char x, void* __context)
{
	return x.a == -1;
}
struct Short{ short a; };
int Call_int_ref_Short_(Short x, void* __context)
{
	return x.a == -2;
}
struct Int{ int a; };
int Call_int_ref_Int_(Int x, void* __context)
{
	return x.a == -3;
}
struct Long{ long long a; };
int Call_int_ref_Long_(Long x, void* __context)
{
	return x.a == -4;
}
struct Float{ float a; };
int Call_int_ref_Float_(Float x, void* __context)
{
	return x.a == -5.0f;
}
struct Double{ double a; };
int Call_int_ref_Double_(Double x, void* __context)
{
	return x.a == -6.0;
}
