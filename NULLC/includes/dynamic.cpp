#include "dynamic.h"

#include "../nullc.h"
#include "../nullbind.h"
#include "../nullc_debug.h"

#include "../StrAlgo.h"

namespace NULLCDynamic
{
	Linker *linker = NULL;

	void OverrideFunction(NULLCRef dest, NULLCRef src)
	{
		assert(linker);
		if(linker->exTypes[dest.typeID].subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("Destination variable is not a function");
			return;
		}
		if(linker->exTypes[src.typeID].subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("Source variable is not a function");
			return;
		}
		if(dest.typeID != src.typeID)
		{
			nullcThrowError("Cannot convert from '%s' to '%s'", &linker->exSymbols[linker->exTypes[src.typeID].offsetToName], &linker->exSymbols[linker->exTypes[dest.typeID].offsetToName]);
			return;
		}

		nullcRedirectFunction(((NULLCFuncPtr*)dest.ptr)->id, ((NULLCFuncPtr*)src.ptr)->id);
	}

	void Override(NULLCRef dest, NULLCArray code)
	{
		assert(linker);
		static unsigned int overrideID = 0;

		if(linker->exTypes[dest.typeID].subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("Destination variable is not a function");
			return;
		}

		char tmp[2048];
		char *it = tmp;
		ExternMemberInfo *memberList = &linker->exTypeExtra[linker->exTypes[dest.typeID].memberOffset];
		ExternTypeInfo &returnType = linker->exTypes[memberList[0].type];
		it += NULLC::SafeSprintf(it, 2048 - int(it - tmp), "import __last;\r\n%s __override%d(", &linker->exSymbols[0] + returnType.offsetToName, overrideID);

		for(unsigned int i = 0, memberCount = linker->exTypes[dest.typeID].memberCount; i != memberCount; i++)
			it += NULLC::SafeSprintf(it, 2048 - int(it - tmp), "%s arg%d%s", &linker->exSymbols[0] + linker->exTypes[memberList[i + 1].type].offsetToName, i, i == memberCount - 1 ? "" : ", ");
		it += NULLC::SafeSprintf(it, 2048 - int(it - tmp), "){ %s }", code.ptr);
		overrideID++;

		if(!nullcCompile(tmp))
		{
			nullcThrowError("%s", nullcGetLastError());
			return;
		}
		char *bytecode = NULL;
		nullcGetBytecodeNoCache(&bytecode);
		if(!nullcLinkCode(bytecode))
		{
			delete[] bytecode;
			nullcThrowError("%s", nullcGetLastError());
			return;
		}
		delete[] bytecode;

		nullcRedirectFunction(((NULLCFuncPtr*)dest.ptr)->id, linker->exFunctions.size() - 1);
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.dynamic", NULLCDynamic::funcPtr, name, index)) return false;
bool	nullcInitDynamicModule(Linker* linker)
{
	NULLCDynamic::linker = linker;

	REGISTER_FUNC(OverrideFunction, "override", 0);
	REGISTER_FUNC(Override, "override", 1);

	return true;
}

void	nullcDeinitDynamicModule()
{
	NULLCDynamic::linker = NULL;
}
