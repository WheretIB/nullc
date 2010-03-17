#include "time.h"
#include "../nullc.h"
#include <time.h>

namespace NULLCTime
{
	int clock()
	{
		return ::clock();
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("std.time", (void(*)())NULLCTime::funcPtr, name, index)) return false;
bool	nullcInitTimeModule()
{
	REGISTER_FUNC(clock, "clock", 0);

	return true;
}
