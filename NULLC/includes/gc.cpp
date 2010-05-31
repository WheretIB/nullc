#include "gc.h"
#include "../nullc.h"

#include "../StdLib.h"

namespace NULLCGC
{
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("std.gc", (void(*)())NULLCGC::funcPtr, name, index)) return false;
bool	nullcInitGCModule()
{
	if(!nullcBindModuleFunction("std.gc", (void(*)())NULLC::CollectMemory, "NamespaceGC::CollectMemory", 0))
		return false;
	if(!nullcBindModuleFunction("std.gc", (void(*)())NULLC::UsedMemory, "NamespaceGC::UsedMemory", 0))
		return false;
	if(!nullcBindModuleFunction("std.gc", (void(*)())NULLC::MarkTime, "NamespaceGC::MarkTime", 0))
		return false;
	if(!nullcBindModuleFunction("std.gc", (void(*)())NULLC::CollectTime, "NamespaceGC::CollectTime", 0))
		return false;
	return true;
}
