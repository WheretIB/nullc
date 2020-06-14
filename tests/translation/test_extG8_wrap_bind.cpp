#include "runtime.h"

struct Zomg{ float a; float b; int c; };

int Call_int_ref_Zomg_(Zomg x, void* __context)
{
	return x.a == -1.0f && x.b == -2.0f && x.c == -3;
}
