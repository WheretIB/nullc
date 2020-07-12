#include "runtime.h"

#pragma pack(push, 4)
struct Zomg{ int a; int *b; };
#pragma pack(pop)

Zomg Call_Zomg_ref_Zomg_(Zomg x, void* __context)
{
	x.a = x.a == -1 && *x.b == -2;
	return x;
}
