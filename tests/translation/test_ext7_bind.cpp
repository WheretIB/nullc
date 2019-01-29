#include "runtime.h"

int Call_int_ref_char_short_double_double_long_int_(char a, short b, double c, double d, long long e, int f, void* __context)
{
	return a == -1 && b == -2 && c == -3.0 && d == -4.0 && e == -5 && f == -6;
}
