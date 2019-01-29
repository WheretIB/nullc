#include "runtime.h"

class TestAlignment5StructX{ double x; int y; };
class TestAlignment5StructY{ TestAlignment5StructX x; int y; };

int TestAlignment5StructYSizeof_int_ref__(void* unused)
{
	return sizeof(TestAlignment5StructY);
}
