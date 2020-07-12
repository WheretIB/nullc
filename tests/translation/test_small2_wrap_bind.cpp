#include "runtime.h"

struct X{ double a; };

X Call_X_ref__(void* __context)
{
	X x = { 6 };
	return x;
}
