#include "typeinfo.h"

#include "../nullc.h"
#include "../nullbind.h"
#include "../Linker.h"

namespace NULLCTypeInfo
{
	Linker *linker = NULL;

	struct TypeID{ unsigned typeID;  };
	TypeID getTypeID(unsigned id){ TypeID ret; ret.typeID = id; return ret; }

	int MemberCount(int* type)
	{
		assert(linker);
		ExternTypeInfo &exType = linker->exTypes[*type];
		if(exType.subCat != ExternTypeInfo::CAT_CLASS)
		{
			nullcThrowError("typeid::memberCount: type (%s) is not a class", &linker->exSymbols[exType.offsetToName]);
			return 0;
		}
		return exType.memberCount;
	}
	TypeID MemberType(int member, int* type)
	{
		assert(linker);
		ExternTypeInfo &exType = linker->exTypes[*type];
		if(exType.subCat != ExternTypeInfo::CAT_CLASS)
		{
			nullcThrowError("typeid::memberType: type (%s) is not a class", &linker->exSymbols[exType.offsetToName]);
			return getTypeID(0);
		}
		if((unsigned int)member >= exType.memberCount)
		{
			nullcThrowError("typeid::memberType: member number illegal, type (%s) has only %d members", &linker->exSymbols[exType.offsetToName], exType.memberCount);
			return getTypeID(0);
		}
		ExternMemberInfo *memberList = &linker->exTypeExtra[0];
		return getTypeID(memberList[exType.memberOffset + member].type);
	}
	NULLCArray MemberName(int member, int* type)
	{
		assert(linker);
		NULLCArray ret;

		ExternTypeInfo &exType = linker->exTypes[*type];
		if(exType.subCat != ExternTypeInfo::CAT_CLASS)
		{
			nullcThrowError("typeid::memberName: type (%s) is not a class", &linker->exSymbols[exType.offsetToName]);
			return NULLCArray();
		}
		if((unsigned int)member >= exType.memberCount)
		{
			nullcThrowError("typeid::memberName: member number illegal, type (%s) has only %d members", &linker->exSymbols[exType.offsetToName], exType.memberCount);
			return NULLCArray();
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
	NULLCRef MemberByIndex(NULLCRef obj, int member)
	{
		assert(linker);
		NULLCRef ret;
		ret.ptr = NULL;
		ret.typeID = 0;

		ExternTypeInfo &exType = linker->exTypes[obj.typeID];
		if((exType.subCat != ExternTypeInfo::CAT_CLASS) || ((unsigned)member >= exType.memberCount))
			return ret;

		ExternMemberInfo *memberList = &linker->exTypeExtra[exType.memberOffset];
		ret.typeID = memberList[member].type;
		ret.ptr = obj.ptr + memberList[member].offset;
		return ret;
	}
	NULLCRef MemberByName(NULLCRef obj, NULLCArray member)
	{
		assert(linker);
		ExternTypeInfo &exType = linker->exTypes[obj.typeID];
		if(exType.subCat != ExternTypeInfo::CAT_CLASS)
			return NULLCRef();
		const char *str = &linker->exSymbols[exType.offsetToName];
		const char *memberName = str + strlen(str) + 1;
		for(unsigned i = 0; i < exType.memberCount; i++)
		{
			if(strcmp(memberName, member.ptr) == 0)
				return MemberByIndex(obj, i);
			memberName += strlen(memberName) + 1;
		}
		return NULLCRef();
	}

	NULLCRef GetPointerTarget(NULLCRef obj)
	{
		assert(linker);

		ExternTypeInfo &type = linker->exTypes[obj.typeID];

		if(type.subCat != ExternTypeInfo::CAT_POINTER)
		{
			nullcThrowError("pointerGetTarget: '%s' is not a pointer", &linker->exSymbols[type.offsetToName]);

			NULLCRef r = { 0, 0 };
			return r;
		}

		NULLCRef r;
		r.ptr = *(char**)obj.ptr;
		r.typeID = type.subType;
		return r;
	}

	int IsFunction(int id)
	{
		assert(linker);
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_FUNCTION;
	}

	int IsClass(int id)
	{
		assert(linker);
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_CLASS;
	}

	int IsSimple(int id)
	{
		assert(linker);
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_NONE;
	}

	int IsArray(int id)
	{
		assert(linker);
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_ARRAY;
	}

	int IsPointer(int id)
	{
		assert(linker);
		return linker->exTypes[id].subCat == ExternTypeInfo::CAT_POINTER;
	}

	int IsFunctionRef(NULLCRef r)
	{
		assert(linker);
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_FUNCTION;
	}

	int IsClassRef(NULLCRef r)
	{
		assert(linker);
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_CLASS;
	}

	int IsSimpleRef(NULLCRef r)
	{
		assert(linker);
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_NONE;
	}

	int IsArrayRef(NULLCRef r)
	{
		assert(linker);
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_ARRAY;
	}

	int IsPointerRef(NULLCRef r)
	{
		assert(linker);
		return linker->exTypes[r.typeID].subCat == ExternTypeInfo::CAT_POINTER;
	}

	NULLCArray TypeName(int* type)
	{
		assert(linker);
		NULLCArray ret;
		FastVector<ExternTypeInfo> &exTypes = linker->exTypes;
		char *symbols = &linker->exSymbols[0];

		ret.ptr = exTypes[*type].offsetToName + symbols;
		ret.len = (unsigned int)strlen(ret.ptr) + 1;
		return ret;
	}

	TypeID TypeSubType(int* typeID)
	{
		assert(linker);
		ExternTypeInfo &type = linker->exTypes[*typeID];
		if(type.subCat != ExternTypeInfo::CAT_ARRAY && type.subCat != ExternTypeInfo::CAT_POINTER)
		{
			nullcThrowError("typeid::subType received type (%s) that neither pointer nor array", &linker->exSymbols[type.offsetToName]);
			return getTypeID(0);
		}
		return getTypeID(type.subType);
	}

	int TypeArraySize(int* typeID)
	{
		assert(linker);
		ExternTypeInfo &type = linker->exTypes[*typeID];
		if(type.subCat != ExternTypeInfo::CAT_ARRAY)
		{
			nullcThrowError("typeid::arraySize received type (%s) that is not an array", &linker->exSymbols[type.offsetToName]);
			return -1;
		}
		return type.arrSize;
	}

	TypeID TypeReturnType(int* typeID)
	{
		assert(linker);
		ExternTypeInfo &type = linker->exTypes[*typeID];
		if(type.subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("typeid::returnType received type (%s) that is not a function", &linker->exSymbols[type.offsetToName]);
			return getTypeID(0);
		}
		ExternMemberInfo *memberList = &linker->exTypeExtra[0];
		return getTypeID(memberList[type.memberOffset].type);
	}

	int TypeArgumentCount(int* typeID)
	{
		assert(linker);
		ExternTypeInfo &type = linker->exTypes[*typeID];
		if(type.subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("typeid::argumentCount received type (%s) that is not a function", &linker->exSymbols[type.offsetToName]);
			return -1;
		}
		return type.memberCount;
	}

	TypeID TypeArgumentType(int argument, int* typeID)
	{
		assert(linker);
		ExternTypeInfo &type = linker->exTypes[*typeID];
		if(type.subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("typeid::argumentType received type (%s) that is not a function", &linker->exSymbols[type.offsetToName]);
			return getTypeID(0);
		}
		if((unsigned int)argument >= type.memberCount)
		{
			nullcThrowError("typeid::argumentType: argument number illegal, function (%s) has only %d argument(s)", &linker->exSymbols[type.offsetToName], type.memberCount);
			return getTypeID(0);
		}
		ExternMemberInfo *memberList = &linker->exTypeExtra[0];
		return getTypeID(memberList[type.memberOffset + argument + 1].type);
	}

	TypeID GetType(NULLCArray name)
	{
		if(!name.ptr)
			return getTypeID(0);

		unsigned hash = NULLC::GetStringHash(name.ptr);

		for(unsigned i = 0; i < linker->exTypes.size(); i++)
		{
			if(linker->exTypes[i].nameHash == hash)
				return getTypeID(i);
		}
		return getTypeID(0);
	}

	NULLCRef GetInstanceByName(NULLCArray name)
	{
		NULLCRef r = { 0, 0 };

		TypeID typeID = GetType(name);
		if(!typeID.typeID)
			return r;

		r.ptr = (char*)nullcAllocateTyped(typeID.typeID);
		r.typeID = typeID.typeID;
		return r;
	}

	NULLCRef GetInstanceByType(TypeID id)
	{
		NULLCRef r = { 0, 0 };

		if(!id.typeID)
			return r;

		r.ptr = (char*)nullcAllocateTyped(id.typeID);
		r.typeID = id.typeID;
		return r;
	}
	NULLCAutoArray GetArrayByName(NULLCArray name, unsigned size)
	{
		NULLCAutoArray r = { 0, 0, 0 };

		TypeID id = GetType(name);
		if(!id.typeID)
			return r;

		NULLCArray arr = nullcAllocateArrayTyped(id.typeID, size);
		r.len = size;
		r.typeID = id.typeID;
		r.ptr = arr.ptr;
		return r;
	}
	NULLCAutoArray GetArrayByType(TypeID id, unsigned size)
	{
		NULLCAutoArray r = { 0, 0, 0 };

		if(!id.typeID)
			return r;

		NULLCArray arr = nullcAllocateArrayTyped(id.typeID, size);
		r.len = size;
		r.typeID = id.typeID;
		r.ptr = arr.ptr;
		return r;
	}

	TypeID GetFunctionContextType(NULLCRef obj)
	{
		assert(linker);

		ExternTypeInfo &type = linker->exTypes[obj.typeID];

		if(type.subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("functionGetContextType: received type '%s' that is not a function", &linker->exSymbols[type.offsetToName]);
			return getTypeID(0);
		}

		ExternFuncInfo &function = linker->exFunctions[*(int*)(obj.ptr + NULLC_PTR_SIZE)];
		return getTypeID(function.contextType != ~0u ? function.contextType : 0);
	}

	NULLCRef GetFunctionContext(NULLCRef obj)
	{
		assert(linker);
		NULLCRef ret;

		ExternTypeInfo &type = linker->exTypes[obj.typeID];

		if(type.subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("functionGetContext: received type '%s' that is not a function", &linker->exSymbols[type.offsetToName]);
			ret.ptr = NULL;
			ret.typeID = 0;
			return ret;
		}

		NULLCFuncPtr *ptr = (NULLCFuncPtr*)obj.ptr;

		ExternFuncInfo &function = linker->exFunctions[ptr->id];
		ret.ptr = (char*)ptr->context;
		ret.typeID = 0;

		if(function.contextType != ~0u)
		{
			ExternTypeInfo &contextType = linker->exTypes[function.contextType];
			ret.typeID = contextType.subType;
		}

		return ret;
	}

	void SetFunctionContext(NULLCRef obj, NULLCRef context)
	{
		assert(linker);

		ExternTypeInfo &objectType = linker->exTypes[obj.typeID];

		if(objectType.subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("functionSetContext: received type '%s' that is not a function", &linker->exSymbols[objectType.offsetToName]);
			return;
		}

		NULLCFuncPtr *ptr = (NULLCFuncPtr*)obj.ptr;

		ExternFuncInfo &function = linker->exFunctions[ptr->id];

		if(function.contextType == ~0u)
		{
			nullcThrowError("functionSetContext: function '%s' doesn't have a context", &linker->exSymbols[function.offsetToName]);
			return;
		}

		ExternTypeInfo &contextRefType = linker->exTypes[function.contextType];

		if(contextRefType.subType != context.typeID)
		{
			nullcThrowError("functionSetContext: cannot set context of type '%s' to the function '%s' expecting context of type '%s'", &linker->exSymbols[linker->exTypes[context.typeID].offsetToName], &linker->exSymbols[function.offsetToName], &linker->exSymbols[linker->exTypes[function.contextType].offsetToName]);
			return;
		}

		ptr->context = context.ptr;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.typeinfo", NULLCTypeInfo::funcPtr, name, index)) return false;
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

	REGISTER_FUNC(TypeName, "typeid::name$", 0);

	REGISTER_FUNC(TypeSubType, "typeid::subType", 0);
	REGISTER_FUNC(TypeArraySize, "typeid::arraySize", 0);
	REGISTER_FUNC(TypeReturnType, "typeid::returnType", 0);
	REGISTER_FUNC(TypeArgumentCount, "typeid::argumentCount", 0);
	REGISTER_FUNC(TypeArgumentType, "typeid::argumentType", 0);

	REGISTER_FUNC(MemberByIndex, "typeGetMember", 0);
	REGISTER_FUNC(MemberByName, "typeGetMember", 1);

	REGISTER_FUNC(GetPointerTarget, "pointerGetTarget", 0);

	REGISTER_FUNC(GetType, "getType", 0);
	REGISTER_FUNC(GetInstanceByName, "createInstanceByName", 0);
	REGISTER_FUNC(GetInstanceByType, "createInstanceByType", 0);
	REGISTER_FUNC(GetArrayByName, "createArrayByName", 0);
	REGISTER_FUNC(GetArrayByType, "createArrayByType", 0);

	REGISTER_FUNC(GetFunctionContextType, "functionGetContextType", 0);
	REGISTER_FUNC(GetFunctionContext, "functionGetContext", 0);
	REGISTER_FUNC(SetFunctionContext, "functionSetContext", 0);

	return true;
}

void	nullcInitTypeinfoModuleLinkerOnly(Linker* linker)
{
	NULLCTypeInfo::linker = linker;
}

void	nullcDeinitTypeinfoModule()
{
	NULLCTypeInfo::linker = NULL;
}

unsigned int	nullcGetTypeSize(unsigned int typeID)
{
	assert(NULLCTypeInfo::linker);
	return NULLCTypeInfo::linker->exTypes[typeID].size;
}

const char*		nullcGetTypeName(unsigned int typeID)
{
	assert(NULLCTypeInfo::linker);
	return &NULLCTypeInfo::linker->exSymbols[NULLCTypeInfo::linker->exTypes[typeID].offsetToName];
}

unsigned int	nullcGetFunctionType(unsigned int funcID)
{
	assert(NULLCTypeInfo::linker);
	return NULLCTypeInfo::linker->exFunctions[funcID].funcType;
}

const char*		nullcGetFunctionName(unsigned int funcID)
{
	assert(NULLCTypeInfo::linker);
	return &NULLCTypeInfo::linker->exSymbols[NULLCTypeInfo::linker->exFunctions[funcID].offsetToName];
}

unsigned int	nullcGetArraySize(unsigned int typeID)
{
	int ID = typeID;
	return NULLCTypeInfo::TypeArraySize(&ID);
}

unsigned int	nullcGetTypeCount()
{
	assert(NULLCTypeInfo::linker);
	return NULLCTypeInfo::linker->exTypes.size();
}

int			nullcIsFunction(unsigned int id)
{
	return NULLCTypeInfo::IsFunction(id);
}

int				nullcIsClass(unsigned int id)
{
	return NULLCTypeInfo::IsClass(id);
}

int				nullcIsSimple(unsigned int id)
{
	return NULLCTypeInfo::IsSimple(id);
}

int				nullcIsArray(unsigned int id)
{
	return NULLCTypeInfo::IsArray(id);
}

int				nullcIsPointer(unsigned int id)
{
	return NULLCTypeInfo::IsPointer(id);
}

unsigned int	nullcGetSubType(int id)
{
	return NULLCTypeInfo::TypeSubType(&id).typeID;
}
