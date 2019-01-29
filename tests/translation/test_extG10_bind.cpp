#include "runtime.h"

struct TestExtG10FooSub{ int a; int b; int c; };
struct Zomg{ TestExtG10FooSub a; float b; };

int Call_int_ref_Zomg_(Zomg x, void* __context)
{
	return x.a.a == -1 && x.a.b == -2 && x.a.c == -3 && x.b == -4.0f;
}
