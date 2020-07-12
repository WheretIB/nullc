#include "runtime.h"

struct X{ float a; float b; int c; };

X Call_X_ref__(void* __context)
{
	X x = { 4, 8, 16 };
	return x;
}
