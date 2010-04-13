#include "gc.h"
#include "../nullc.h"

#include "../StdLib.h"

namespace NULLCGC
{
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("std.gc", (void(*)())NULLCGC::funcPtr, name, index)) return false;
bool	nullcInitGCModule()
{
	if(!nullcAddModuleFunction("std.gc", (void(*)())NULLC::CollectMemory, "NamespaceGC::CollectMemory", 0))
		return false;

	return true;
}
