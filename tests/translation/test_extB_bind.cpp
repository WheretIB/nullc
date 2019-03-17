#include "runtime.h"

struct Foo{};

int Call_int_ref_char_Foo_int_Foo_float_double_(char a, Foo b, int c, Foo d, float e, double f, void* __context)
{
	(void)b;
	(void)d;
	return a == -1 && c == -3 && e == -5.0f && f == -6.0;
}
