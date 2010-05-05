#include "random.h"
#include "../nullc.h"
#include <stdlib.h>

namespace NULLCRandom
{
	void srand(int s)
	{
		::srand(s);
	}
	int rand()
	{
		return ::rand();
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("std.random", (void(*)())NULLCRandom::funcPtr, name, index)) return false;
bool	nullcInitRandomModule()
{
	REGISTER_FUNC(srand, "srand", 0);
	REGISTER_FUNC(rand, "rand", 0);

	return true;
}
