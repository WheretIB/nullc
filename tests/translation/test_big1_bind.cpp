#include "runtime.h"

struct X{ int a, b, c, d; };

X Call_X_ref__(void* __context)
{
	X x = { 1, 2, 3, 4 };
	return x;
}
