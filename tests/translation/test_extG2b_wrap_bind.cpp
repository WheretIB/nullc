#include "runtime.h"

struct Zomg
{
	char x;
	char pad_1[3];
	int y;
};

Zomg Call_Zomg_ref_Zomg_(Zomg x, void* __context)
{
	x.x = x.x == -1 && x.y == -2;
	return x;
}
