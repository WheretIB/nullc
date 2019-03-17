#include "runtime.h"

struct Zomg{ double a; double b; };

int Call_int_ref_float_float_float_Zomg_Zomg_Zomg_(float x, float y, float z, Zomg a, Zomg b, Zomg c, void* __context)
{
	return x == 1.0 && y == 2.0 && z == 3.0 && a.a == 4.0 && a.b == 5.0 && b.a == 6.0 && b.b == 7.0 && c.a == 8.0 && c.b == 9.0;
}
