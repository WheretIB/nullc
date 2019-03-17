#include "runtime.h"

struct AlignedStruct
{
	char x;
	double y;
	char z;
};

int CheckAlignmentStruct_int_ref_auto_ref_(NULLCRef ptr, void* unused)
{
	AlignedStruct *obj = (AlignedStruct*)ptr.ptr;
	return obj->x == 0x34 && obj->y == 32.0 && obj->z == 0x45;
}
