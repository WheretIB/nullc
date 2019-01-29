#include "runtime.h"

NULLCRef Call_auto_ref_ref_auto_ref_(NULLCRef x, void* __context)
{
	*(int*)x.ptr = x.typeID == 4 && *(int*)x.ptr == 3;
	return x;
}
