#include "runtime.h"

struct Zomg{ int a; char b; short c; };

int Call_int_ref_Zomg_(Zomg x, void* __context)
{
	return x.a == -1 && x.b == -2 && x.c == -3;
}
