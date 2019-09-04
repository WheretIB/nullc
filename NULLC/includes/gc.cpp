#include "gc.h"
#include "../nullc.h"
#include "../nullbind.h"

#include "../StdLib.h"

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.gc", NULLC::funcPtr, name, index)) return false;
bool	nullcInitGCModule()
{
	REGISTER_FUNC(CollectMemory, "NamespaceGC::CollectMemory", 0);
	REGISTER_FUNC(UsedMemory, "NamespaceGC::UsedMemory", 0);
	REGISTER_FUNC(MarkTime, "NamespaceGC::MarkTime", 0);
	REGISTER_FUNC(CollectTime, "NamespaceGC::CollectTime", 0);

	return true;
}
