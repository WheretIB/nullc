#include "runtime.h"

struct Zomg{ float a; double b; };

int Call_int_ref_Zomg_(Zomg x, void* __context)
{
	return x.a == -1.0f && x.b == -2.0;
}
