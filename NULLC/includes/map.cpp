#include "map.h"
#include "../nullc.h"
#include "../nullbind.h"

namespace NULLCMap
{

}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.map", NULLCMap::funcPtr, name, index)) return false;
bool	nullcInitMapModule()
{

	return true;
}
