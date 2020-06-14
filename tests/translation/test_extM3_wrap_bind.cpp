#include "runtime.h"

#include <math.h>

struct Foo;

int Foo__Call_int_ref_double_double_double_double_(double x1, double x2, double x3, double x4, Foo* __context)
{
#define fpart(x) (x - floor(x))
	double xgap = fpart(x1 + 0.5);
	return int(xgap * 10.0) == 1 && __context && x2 == 2.0 && x3 == 3.0 && x4 == 4.0;
#undef fpart
}
