#include "runtime.h"

struct X{ float a; };

X Call_X_ref__(void* __context)
{
	X x = { 4 };
	return x;
}
