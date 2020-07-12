#include "runtime.h"

struct Zomg{ NULLCArray<int> a; };

int Call_int_ref_Zomg_(Zomg x, void* __context)
{
	return x.a.size == 2 && ((int*)x.a.ptr)[0] == 1 && ((int*)x.a.ptr)[1] == 2;
}
