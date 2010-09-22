#include "runtime.h"

int CheckAlignment(NULLCRef ptr, int alignment, void* unused)
{
	int asInt = (int)ptr.ptr;
	return asInt % alignment == 0;
}
