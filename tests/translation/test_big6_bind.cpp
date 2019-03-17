#include "runtime.h"

struct X{ double a; int b; };

X Call_X_ref__(void* __context)
{
	X x = { 4, 8 };
	return x;
}
