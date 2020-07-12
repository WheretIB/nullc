#include "runtime.h"

NULLCArray< int > Call_int___ref_int___(NULLCArray< int > a, void* __context)
{
	((int*)a.ptr)[1] = 1;
	return a;
}
