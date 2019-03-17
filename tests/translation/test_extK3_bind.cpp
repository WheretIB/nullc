#include "runtime.h"

NULLCRef Call_auto_ref_ref_auto_ref_int_ref_(NULLCRef x, int* b, void* __context)
{
	*b = (x.typeID == 4 && *(int*)x.ptr == 3 && *b == 2) ? 20 : 0;
	x.typeID = 4;
	*(int*)x.ptr = 30;
	return x;
}
