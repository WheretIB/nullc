#include "map.h"
#include "../nullc.h"

namespace NULLCMap
{

}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("std.map", (void(*)())NULLCMap::funcPtr, name, index)) return false;
bool	nullcInitMapModule()
{

	return true;
}

void	nullcDeinitMapModule()
{

}
