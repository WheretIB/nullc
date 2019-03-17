#include "runtime.h"

struct X{ float a; int b; };

X Call_X_ref__(void* __context)
{
	X x = { 2, 16 };
	return x;
}
