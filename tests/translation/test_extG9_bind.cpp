#include "runtime.h"

struct Zomg{ long long a; float b; float c; };

int Call_int_ref_Zomg_(Zomg x, void* __context)
{
	return x.a == -1 && x.b == -2.0f && x.c == -3.0f;
}
