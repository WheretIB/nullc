#include "runtime.h"

int Call_int_ref_auto_ref_(NULLCRef x, void* __context)
{
	return x.typeID == 4 && *(int*)x.ptr == 3;
}
