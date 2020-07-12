#include "runtime.h"

int Call_int_ref_int___int_int___int___(NULLCArray<int> x, int w, NULLCArray<int> y, NULLCArray<int> z, void* __context)
{
	return *(int*)x.ptr == 10 && *(int*)y.ptr == 20 && *(int*)z.ptr == 30 &&
		w == 4 && x.size == 1 && y.size == 2 && z.size == 3;
}
