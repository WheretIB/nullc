#include "runtime.h"

#include <stdint.h>

void* GetPtr_void_ref_ref_int_(int i, void* __context)
{
	return (void*)(intptr_t)(0x80000000 | i);
}

int Call_int_ref_void_ref_int_long_void_ref_(void* a, int b, long long c, void* d, void* __context)
{
	return ((intptr_t)a) == (intptr_t)(0x80000000u | 1) && b == -2 && c == -3 && ((intptr_t)d) == (intptr_t)(0x80000000u | 4);
}
