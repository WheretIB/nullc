#include "runtime.h"

struct Zomg{ int a; };

Zomg Call_Zomg_ref_auto_ref_(NULLCRef x, void* __context)
{
	Zomg ret;
	ret.a = x.typeID == 4 && *(int*)x.ptr == 3;
	return ret;
}
