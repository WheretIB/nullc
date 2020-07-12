#include "runtime.h"

NULLCArray<int> Call_int___ref_int___int___typeid_(NULLCArray<int> a, NULLCArray<int> b, unsigned u, void* __context)
{
	bool res = a.size == 2 && ((int*)a.ptr)[0] == 1 && ((int*)a.ptr)[1] == 2 && b.size == 2 && ((int*)b.ptr)[0] == 3 && ((int*)b.ptr)[1] == 4 && u == 4;
	((int*)a.ptr)[1] = res;
	return a;
}
