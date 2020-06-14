#include "runtime.h"

struct X{ float a, b, c; };

X Call_X_ref__(void* __context)
{
	X x = { 1, 2, 3 };
	return x;
}
