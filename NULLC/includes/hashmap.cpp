#include "hashmap.h"
#include "../nullc.h"

namespace NULLCHashmap
{

}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("std.hashmap", (void(*)())NULLCHashmap::funcPtr, name, index)) return false;
bool	nullcInitHashmapModule()
{

	return true;
}
