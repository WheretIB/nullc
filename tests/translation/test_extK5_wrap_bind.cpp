#include "runtime.h"

int Call_int_ref_int___int___int___int_(NULLCArray<int> x, NULLCArray<int> y, NULLCArray<int> z, int w, void* __context)
{
	return *(int*)x.ptr == 10 && *(int*)y.ptr == 20 && *(int*)z.ptr == 30 &&
		w == 4 && x.size == 1 && y.size == 2 && z.size == 3;
}
