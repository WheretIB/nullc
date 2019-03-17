#include "runtime.h"

#include <stdlib.h>

void srand_void_ref_int_(int i, void* __context)
{
	srand(i);
}

int rand_int_ref__(void* __context)
{
	return rand();
}
