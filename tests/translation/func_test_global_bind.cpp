#include "runtime.h"

struct __typeProxy_int_ref_int_;
struct __typeProxy_int_ref_int_int_;

int RecallerPtr_int_ref_int_ref_int__(NULLCFuncPtr<__typeProxy_int_ref_int_> fPtr, void* __context)
{
	__nullcFunctionArray* __nullcFM =__nullcGetFunctionTable();
	return ((int(*)(int, void*))(*__nullcFM)[fPtr.id])(14, fPtr.context);
}

void bubble_void_ref_int___int_ref_int_int__(NULLCArray<int> arr, NULLCFuncPtr<__typeProxy_int_ref_int_int_> comp, void* __context)
{
	__nullcFunctionArray* __nullcFM =__nullcGetFunctionTable();

	int *elem = (int*)arr.ptr;

	for(unsigned int k = 0; k < arr.size; k++)
	{
		for(unsigned int l = arr.size-1; l > k; l--)
		{
			if(((int(*)(int, int, void*))(*__nullcFM)[comp.id])(elem[l], elem[l-1], comp.context))
			{
				int tmp = elem[l];
				elem[l] = elem[l-1];
				elem[l-1] = tmp;
			}
		}
	}
}
