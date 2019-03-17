#include "runtime.h"

int Call_int_ref_int___(NULLCArray< int > a, void* __context)
{
	return ((int*)a.ptr)[0] + ((int*)a.ptr)[1];
}
