#include "runtime.h"

int CheckAlignment_int_ref_auto_ref_int_(NULLCRef ptr, int alignment, void* unused)
{
	int asInt = (int)ptr.ptr;
	return asInt % alignment == 0;
}
