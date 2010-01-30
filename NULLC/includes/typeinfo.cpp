#include "typeinfo.h"
#include "../nullc.h"

namespace NULLCTypeInfo
{
	Linker *linker = NULL;

	int MemberCount(int* type)
	{
		ExternTypeInfo &exType = linker->exTypes[*type];
		if(exType.subCat != ExternTypeInfo::CAT_CLASS)
		{
			nullcThrowError("Type is not a class");
			return 0;
		}
		return exType.memberCount;
	}
	int MemberType(int member, int* type)
	{
		ExternTypeInfo &exType = linker->exTypes[*type];
		if(exType.subCat != ExternTypeInfo::CAT_CLASS)
		{
			nullcThrowError("Type is not a class");
			return 0;
		}
		if((unsigned int)member > exType.memberCount)
		{
			nullcThrowError("Member number illegal");
			return 0;
		}
		unsigned int *memberList = &linker->exTypeExtra[0];
		return memberList[exType.memberOffset + member];
	}
	NullCArray MemberName(int member, int* type)
	{
		NullCArray ret;

		ExternTypeInfo &exType = linker->exTypes[*type];
		if(exType.subCat != ExternTypeInfo::CAT_CLASS)
		{
			nullcThrowError("Type is not a class");
			return NullCArray();
		}
		if((unsigned int)member > exType.memberCount)
		{
			nullcThrowError("Member number illegal");
			return NullCArray();
		}
		char *symbols = &linker->exSymbols[0];
		unsigned int strLength = (unsigned int)strlen(symbols + exType.offsetToName) + 1;
		const char *memberName = symbols + exType.offsetToName + strLength;
		for(int n = 0; n < member; n++)
		{
			strLength = (unsigned int)strlen(memberName) + 1;
			memberName += strLength;
		}

		ret.ptr = (char*)memberName;
		ret.len = (unsigned int)strlen(memberName) + 1;
		return ret;
	}

	int Typeid(NULLCRef r)
	{
		return r.typeID;
	}

	int IsFunction(int id)
	{
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_FUNCTION;
	}

	int IsClass(int id)
	{
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_CLASS;
	}

	int IsSimple(int id)
	{
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_NONE;
	}

	int IsArray(int id)
	{
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_ARRAY;
	}

	int IsPointer(int id)
	{
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_POINTER;
	}

	int IsFunctionRef(NULLCRef r)
	{
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_FUNCTION;
	}

	int IsClassRef(NULLCRef r)
	{
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_CLASS;
	}

	int IsSimpleRef(NULLCRef r)
	{
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_NONE;
	}

	int IsArrayRef(NULLCRef r)
	{
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_ARRAY;
	}

	int IsPointerRef(NULLCRef r)
	{
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_POINTER;
	}

	unsigned int TypeSize(int* type)
	{
		return linker->exTypes[*type].size;
	}

	NullCArray TypeName(int* type)
	{
		NullCArray ret;
		FastVector<ExternTypeInfo> &exTypes = linker->exTypes;
		char *symbols = &linker->exSymbols[0];

		ret.ptr = exTypes[*type].offsetToName + symbols;
		ret.len = (unsigned int)strlen(ret.ptr) + 1;
		return ret;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("std.typeinfo", (void(*)())NULLCTypeInfo::funcPtr, name, index)) return false;
bool	nullcInitTypeinfoModule(Linker* linker)
{
	NULLCTypeInfo::linker = linker;

	REGISTER_FUNC(Typeid, "typeid", 0);

	REGISTER_FUNC(IsFunction, "isFunction", 0);
	REGISTER_FUNC(IsFunctionRef, "isFunction", 1);
	REGISTER_FUNC(IsClass, "isClass", 0);
	REGISTER_FUNC(IsClassRef, "isClass", 1);
	REGISTER_FUNC(IsSimple, "isSimple", 0);
	REGISTER_FUNC(IsSimpleRef, "isSimple", 1);
	REGISTER_FUNC(IsArray, "isArray", 0);
	REGISTER_FUNC(IsArrayRef, "isArray", 1);
	REGISTER_FUNC(IsPointer, "isPointer", 0);
	REGISTER_FUNC(IsPointerRef, "isPointer", 1);

	REGISTER_FUNC(MemberCount, "typeid::memberCount", 0);
	REGISTER_FUNC(MemberType, "typeid::memberType", 0);
	REGISTER_FUNC(MemberName, "typeid::memberName", 0);

	REGISTER_FUNC(TypeSize, "typeid::size$", 0);
	REGISTER_FUNC(TypeName, "typeid::name$", 0);

	return true;
}

void	nullcDeinitTypeinfoModule()
{

}

unsigned int	nullcGetTypeSize(unsigned int typeID)
{
	return NULLCTypeInfo::linker->exTypes[typeID].size;
}
