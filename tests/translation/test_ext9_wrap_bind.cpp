#include "runtime.h"

int Call_int_ref_char_short_int_long_float_double_(char a, short b, int c, long long d, float e, double f, void* __context)
{
	return a == -1 && b == -2 && c == -3 && d == -4 && e == -5.0f && f == -6.0;
}
