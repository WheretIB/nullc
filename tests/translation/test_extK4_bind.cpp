#include "runtime.h"

int Call_int_ref_auto_ref_auto_ref_auto_ref_auto_ref_auto_ref_auto_ref_auto_ref_(NULLCRef x, NULLCRef y, NULLCRef z, NULLCRef w, NULLCRef a, NULLCRef b, NULLCRef c, void* __context)
{
	return *(int*)x.ptr == 1 && *(int*)y.ptr == 2 && *(int*)z.ptr == 3 &&
		*(int*)w.ptr == 4 && *(int*)a.ptr == 5 && *(int*)b.ptr == 6 && *(int*)c.ptr == 7;
}
