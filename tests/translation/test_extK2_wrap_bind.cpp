#include "runtime.h"

int Call_int_ref_auto_ref_int_ref_(NULLCRef x, int* b, void* __context)
{
	return x.typeID == 4 && *(int*)x.ptr == 3 && *b == 2;
}
