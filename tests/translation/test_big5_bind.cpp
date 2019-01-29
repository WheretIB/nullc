#include "runtime.h"

struct X{ int a; double b; };

X Call_X_ref__(void* __context)
{
	X x = { 1, 2 };
	return x;
}
