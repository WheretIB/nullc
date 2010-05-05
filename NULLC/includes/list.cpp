#include "list.h"
#include "../nullc.h"

namespace NULLCList
{

}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("std.list", (void(*)())NULLCList::funcPtr, name, index)) return false;
bool	nullcInitListModule()
{

	return true;
}
