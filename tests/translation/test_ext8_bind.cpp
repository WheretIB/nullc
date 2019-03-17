#include "runtime.h"

int Call_int_ref_float_float_double_double_float_float_(float a, float b, double c, double d, float e, float f, void* __context)
{
	return a == -1.0f && b == -2.0f && c == -3.0 && d == -4.0 && e == -5.0f && f == -6.0f;
}
