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
			nullcThrowError("typeid::memberCount: type (%s) is not a class", &linker->exSymbols[exType.offsetToName]);
			return 0;
		}
		return exType.memberCount;
	}
	int MemberType(int member, int* type)
	{
		ExternTypeInfo &exType = linker->exTypes[*type];
		if(exType.subCat != ExternTypeInfo::CAT_CLASS)
		{
			nullcThrowError("typeid::memberType: type (%s) is not a class", &linker->exSymbols[exType.offsetToName]);
			return 0;
		}
		if((unsigned int)member > exType.memberCount)
		{
			nullcThrowError("typeid::memberType: member number illegal, type (%s) has only %d members", &linker->exSymbols[exType.offsetToName], exType.memberCount);
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
			nullcThrowError("typeid::memberName: type (%s) is not a class", &linker->exSymbols[exType.offsetToName]);
			return NullCArray();
		}
		if((unsigned int)member > exType.memberCount)
		{
			nullcThrowError("typeid::memberName: member number illegal, type (%s) has only %d members", &linker->exSymbols[exType.offsetToName], exType.memberCount);
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

	int TypeSubType(int &typeID)
	{
		ExternTypeInfo &type = linker->exTypes[typeID];
		if(type.subCat != ExternTypeInfo::CAT_ARRAY && type.subCat != ExternTypeInfo::CAT_POINTER)
		{
			nullcThrowError("typeid::subType received type (%s) that neither pointer nor array", &linker->exSymbols[type.offsetToName]);
			return -1;
		}
		return type.subType;
	}

	int TypeArraySize(int &typeID)
	{
		ExternTypeInfo &type = linker->exTypes[typeID];
		if(type.subCat != ExternTypeInfo::CAT_ARRAY)
		{
			nullcThrowError("typeid::arraySize received type (%s) that is not an array", &linker->exSymbols[type.offsetToName]);
			return -1;
		}
		return type.arrSize;
	}

	int TypeReturnType(int &typeID)
	{
		ExternTypeInfo &type = linker->exTypes[typeID];
		if(type.subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("typeid::returnType received type (%s) that is not a function", &linker->exSymbols[type.offsetToName]);
			return -1;
		}
		unsigned int *memberList = &linker->exTypeExtra[0];
		return memberList[type.memberOffset];
	}

	int TypeArgumentCount(int &typeID)
	{
		ExternTypeInfo &type = linker->exTypes[typeID];
		if(type.subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("typeid::argumentCount received type (%s) that is not a function", &linker->exSymbols[type.offsetToName]);
			return -1;
		}
		return type.memberCount;
	}

	int TypeArgumentType(int argument, int &typeID)
	{
		ExternTypeInfo &type = linker->exTypes[typeID];
		if(type.subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("typeid::argumentType received type (%s) that is not a function", &linker->exSymbols[type.offsetToName]);
			return -1;
		}
		if((unsigned int)argument > type.memberCount)
		{
			nullcThrowError("typeid::argumentType: argument number illegal, function (%s) has only %d argument(s)", &linker->exSymbols[type.offsetToName], type.memberCount);
			return -1;
		}
		unsigned int *memberList = &linker->exTypeExtra[0];
		return memberList[type.memberOffset + argument + 1];
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("std.typeinfo", (void(*)())NULLCTypeInfo::funcPtr, name, index)) return false;
bool	nullcInitTypeinfoModule(Linker* linker)
{
	NULLCTypeInfo::linker = linker;

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

	REGISTER_FUNC(TypeSubType, "typeid::subType", 0);
	REGISTER_FUNC(TypeArraySize, "typeid::arraySize", 0);
	REGISTER_FUNC(TypeReturnType, "typeid::returnType", 0);
	REGISTER_FUNC(TypeArgumentCount, "typeid::argumentCount", 0);
	REGISTER_FUNC(TypeArgumentType, "typeid::argumentType", 0);

	return true;
}

unsigned int	nullcGetTypeSize(unsigned int typeID)
{
	return NULLCTypeInfo::linker->exTypes[typeID].size;
}

const char*		nullcGetTypeName(unsigned int typeID)
{
	return &NULLCTypeInfo::linker->exSymbols[NULLCTypeInfo::linker->exTypes[typeID].offsetToName];
}

unsigned int	nullcGetFunctionType(unsigned int funcID)
{
	return NULLCTypeInfo::linker->exFunctions[funcID].funcType;
}

const char*		nullcGetFunctionName(unsigned int funcID)
{
	return &NULLCTypeInfo::linker->exSymbols[NULLCTypeInfo::linker->exFunctions[funcID].offsetToName];
}

unsigned int	nullcGetTypeCount()
{
	return NULLCTypeInfo::linker->exTypes.size();
}
