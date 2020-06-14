#include "runtime.h"

struct Zomg{ int a; short b; char c; };

Zomg Call_Zomg_ref_Zomg_(Zomg x, void* __context)
{
	x.a = x.a == -1 && x.b == -2 && x.c == -3;
	return x;
}
