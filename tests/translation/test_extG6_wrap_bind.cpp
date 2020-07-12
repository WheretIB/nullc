#include "runtime.h"

#pragma pack(push, 4)
struct Zomg{ int a; int *b; };
#pragma pack(pop)

int Call_int_ref_Zomg_(Zomg x, void* __context)
{
	return x.a == -1 && *x.b == -2;
}
