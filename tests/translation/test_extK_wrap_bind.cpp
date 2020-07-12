#include "runtime.h"

int Call_int_ref_int_ref_int_ref_(int* a, int* b, void* __context)
{
	return *a == 1 && *b == 2;
}
