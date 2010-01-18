#include "string.h"
#include "../../nullc/nullc.h"

namespace NULLCString
{

}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("std.string", (void(*)())NULLCString::funcPtr, name, index)) return false;
bool	nullcInitStringModule()
{
	return true;
}

void	nullcDeinitStringModule()
{

}
