#include "runtime.h"

// Array classes
#pragma pack(push, 4)
struct member_iterator
{
	unsigned int classID;
	int pos;
};
struct member_info
{
	unsigned int type;
	NULLCArray<char> name;
};
struct argument_iterator
{
	unsigned int funcID;
	int pos;
};
#pragma pack(pop)

int isFunction_int_ref_typeid_(unsigned type, void* __context)
{
	return __nullcGetTypeInfo(type)->category == NULLC_FUNCTION;
}

int isClass_int_ref_typeid_(unsigned type, void* __context)
{
	return __nullcGetTypeInfo(type)->category == NULLC_CLASS;
}

int isSimple_int_ref_typeid_(unsigned type, void* __context)
{
	return type <= NULLC_BASETYPE_DOUBLE;
}

int isArray_int_ref_typeid_(unsigned type, void* __context)
{
	return __nullcGetTypeInfo(type)->category == NULLC_ARRAY;
}

int isPointer_int_ref_typeid_(unsigned type, void* __context)
{
	return __nullcGetTypeInfo(type)->category == NULLC_POINTER;
}

int isFunction_int_ref_auto_ref_(NULLCRef type, void* __context)
{
	return __nullcGetTypeInfo(type.typeID)->category == NULLC_FUNCTION;
}

int isClass_int_ref_auto_ref_(NULLCRef type, void* __context)
{
	return __nullcGetTypeInfo(type.typeID)->category == NULLC_CLASS;
}

int isSimple_int_ref_auto_ref_(NULLCRef type, void* __context)
{
	return type.typeID <= NULLC_BASETYPE_DOUBLE;
}

int isArray_int_ref_auto_ref_(NULLCRef type, void* __context)
{
	return __nullcGetTypeInfo(type.typeID)->category == NULLC_ARRAY;
}

int isPointer_int_ref_auto_ref_(NULLCRef type, void* __context)
{
	return __nullcGetTypeInfo(type.typeID)->category == NULLC_POINTER;
}

NULLCArray<char> typeid__name__char___ref___(unsigned* __context)
{
	const char *name = __nullcGetTypeInfo(*__context)->name;

	NULLCArray<char> ret = __newA_int___ref_int_int_int_(1, (int)strlen(name) + 1, NULLC_BASETYPE_CHAR, 0);
	memcpy(ret.ptr, name, ret.size);

	return ret;
}

int typeid__memberCount_int_ref__(unsigned* __context)
{
	NULLCTypeInfo *exType = __nullcGetTypeInfo(*__context);

	if(exType->category != NULLC_CLASS)
	{
		nullcThrowError("typeid::memberCount: type (%s) is not a class", exType->name);
		return 0;
	}

	return exType->memberCount;
}

unsigned typeid__memberType_typeid_ref_int_(int member, unsigned* __context)
{
	NULLCTypeInfo *exType = __nullcGetTypeInfo(*__context);

	if(exType->category != NULLC_CLASS)
	{
		nullcThrowError("typeid::memberType: type (%s) is not a class", exType->name);
		return 0;
	}

	if((unsigned int)member >= exType->memberCount)
	{
		nullcThrowError("typeid::memberType: member number illegal, type (%s) has only %d members", exType->name, exType->memberCount);
		return 0;
	}

	NULLCMemberInfo *members = __nullcGetTypeMembers(*__context);

	return members[member].typeID;
}

NULLCArray<char> typeid__memberName_char___ref_int_(int member, unsigned* __context)
{
	NULLCTypeInfo *exType = __nullcGetTypeInfo(*__context);

	if(exType->category != NULLC_CLASS)
	{
		nullcThrowError("typeid::memberName: type (%s) is not a class", exType->name);
		return NULLCArray<char>();
	}

	if((unsigned int)member >= exType->memberCount)
	{
		nullcThrowError("typeid::memberName: member number illegal, type (%s) has only %d members", exType->name, exType->memberCount);
		return NULLCArray<char>();
	}

	NULLCMemberInfo *members = __nullcGetTypeMembers(*__context);

	const char *name = members[member].name;

	NULLCArray<char> ret = __newA_int___ref_int_int_int_(1, (int)strlen(name) + 1, NULLC_BASETYPE_CHAR, 0);
	memcpy(ret.ptr, name, ret.size);

	return ret;
}

unsigned typeid__subType_typeid_ref__(unsigned* __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(*__context);
	if(type->category != NULLC_ARRAY && type->category != NULLC_POINTER)
	{
		nullcThrowError("typeid::subType received type (%s) that neither pointer nor array", type->name);
		return 0;
	}
	return type->subTypeID;
}

int typeid__arraySize_int_ref__(unsigned* __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(*__context);
	if(type->category != NULLC_ARRAY)
	{
		nullcThrowError("typeid::arraySize received type (%s) that is not an array", type->name);
		return -1;
	}
	return type->memberCount;
}

unsigned typeid__returnType_typeid_ref__(unsigned* __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(*__context);

	if(type->category != NULLC_FUNCTION)
	{
		nullcThrowError("typeid::returnType received type (%s) that is not a function", type->name);
		return 0;
	}

	return type->returnTypeID;
}

int typeid__argumentCount_int_ref__(unsigned* __context)
{
	NULLCTypeInfo *exType = __nullcGetTypeInfo(*__context);

	if(exType->category != NULLC_FUNCTION)
	{
		nullcThrowError("typeid::argumentCount received type (%s) that is not a function", exType->name);
		return 0;
	}

	return exType->memberCount;
}

unsigned typeid__argumentType_typeid_ref_int_(int argument, unsigned* __context)
{
	NULLCTypeInfo *exType = __nullcGetTypeInfo(*__context);

	if(exType->category != NULLC_FUNCTION)
	{
		nullcThrowError("typeid::argumentType received type (%s) that is not a function", exType->name);
		return 0;
	}

	if((unsigned int)argument >= exType->memberCount)
	{
		nullcThrowError("typeid::argumentType: argument number illegal, function (%s) has only %d argument(s)", exType->name, exType->memberCount);
		return 0;
	}

	NULLCMemberInfo *members = __nullcGetTypeMembers(*__context);

	return members[argument].typeID;
}

NULLCRef typeGetMember_auto_ref_ref_auto_ref_int_(NULLCRef obj, int member, void* __context)
{
	NULLCTypeInfo *exType = __nullcGetTypeInfo(obj.typeID);

	if((exType->category != NULLC_CLASS) || ((unsigned)member >= exType->memberCount))
		return NULLCRef();

	NULLCMemberInfo *members = __nullcGetTypeMembers(obj.typeID);

	NULLCRef ret;

	ret.typeID = members[member].typeID;
	ret.ptr = obj.ptr + members[member].offset;

	return ret;
}

NULLCRef typeGetMember_auto_ref_ref_auto_ref_char___(NULLCRef obj, NULLCArray<char> name, void* __context)
{
	NULLCTypeInfo *exType = __nullcGetTypeInfo(obj.typeID);

	if(exType->category != NULLC_CLASS)
		return NULLCRef();

	NULLCMemberInfo *members = __nullcGetTypeMembers(obj.typeID);

	for(unsigned i = 0; i < exType->memberCount; i++)
	{
		if(strcmp(members[i].name, name.ptr) == 0)
			return typeGetMember_auto_ref_ref_auto_ref_int_(obj, i, 0);
	}

	return NULLCRef();
}

NULLCRef pointerGetTarget_auto_ref_ref_auto_ref_(NULLCRef obj, void* __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(obj.type);

	if(type->category != NULLC_POINTER)
	{
		nullcThrowError("pointerGetTarget: '%s' is not a pointer", type->name);

		NULLCRef r = { 0, 0 };
		return r;
	}

	NULLCRef r;
	r.ptr = *(char**)obj.ptr;
	r.typeID = type->subTypeID;
	return r;
}

unsigned getType_typeid_ref_char___(NULLCArray<char> name, void* __context)
{
	if(!name.ptr)
		return 0;

	unsigned hash = __nullcGetStringHash(name.ptr);

	for(unsigned i = 0, e = __nullcGetTypeCount(); i < e; i++)
	{
		NULLCTypeInfo *type = __nullcGetTypeInfo(i);

		if(type->hash == hash)
			return i;
	}

	return 0;
}

NULLCRef createInstanceByName_auto_ref_ref_char___(NULLCArray<char> name, void* __context)
{
	NULLCRef r = { 0, 0 };

	unsigned typeID = getType_typeid_ref_char___(name, 0);

	if(!typeID)
		return r;

	r.ptr = (char*)NULLC::AllocObject(__nullcGetTypeInfo(typeID)->size, typeID);
	r.typeID = typeID;

	return r;
}

NULLCRef createInstanceByType_auto_ref_ref_typeid_(unsigned type, void* __context)
{
	NULLCRef r = { 0, 0 };

	if(!type)
		return r;

	r.ptr = (char*)NULLC::AllocObject(__nullcGetTypeInfo(type)->size, type);
	r.typeID = type;

	return r;
}

NULLCAutoArray createArrayByName_auto___ref_char___int_(NULLCArray<char> name, int size, void* __context)
{
	NULLCAutoArray r = { 0, 0, 0 };

	unsigned typeID = getType_typeid_ref_char___(name, 0);

	if(!typeID)
		return r;

	NULLCArray<char> arr = NULLC::AllocArray(__nullcGetTypeInfo(typeID)->size, size, typeID);

	r.len = size;
	r.typeID = typeID;
	r.ptr = arr.ptr;

	return r;
}

NULLCAutoArray createArrayByType_auto___ref_typeid_int_(unsigned type, int size, void* __context)
{
	NULLCAutoArray r = { 0, 0, 0 };

	if(!type)
		return r;

	NULLCArray<char> arr = NULLC::AllocArray(__nullcGetTypeInfo(type)->size, size, type);

	r.len = size;
	r.typeID = type;
	r.ptr = arr.ptr;

	return r;
}

unsigned functionGetContextType_typeid_ref_auto_ref_(NULLCRef function, void* __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(function.typeID);

	if(type->category != NULLC_FUNCTION)
	{
		nullcThrowError("functionGetContextType: received type '%s' that is not a function", type->name);
		return 0;
	}

	if(!function.ptr)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

	NULLCFuncPtr<> &fPtr = *(NULLCFuncPtr<>*)function.ptr;

	NULLCFuncInfo *func = __nullcGetFunctionInfo(fPtr.id);

	return func->extraType;
}

NULLCRef functionGetContext_auto_ref_ref_auto_ref_(NULLCRef function, void* __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(function.typeID);

	if(type->category != NULLC_FUNCTION)
	{
		nullcThrowError("functionGetContext: received type '%s' that is not a function", type->name);
		return NULLCRef();
	}

	if(!function.ptr)
	{
		nullcThrowError("ERROR: null pointer access");
		return NULLCRef();
	}

	NULLCFuncPtr<> &fPtr = *(NULLCFuncPtr<>*)function.ptr;

	NULLCFuncInfo *func = __nullcGetFunctionInfo(fPtr.id);

	NULLCTypeInfo *contextType = __nullcGetTypeInfo(func->extraType);

	NULLCRef ret;

	ret.ptr = (char*)fPtr.context;
	ret.typeID = contextType->subTypeID;

	return ret;
}

void functionSetContext_void_ref_auto_ref_auto_ref_(NULLCRef function, NULLCRef context, void* __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(function.typeID);

	if(type->category != NULLC_FUNCTION)
	{
		nullcThrowError("functionSetContext: received type '%s' that is not a function", type->name);
		return;
	}

	if(!function.ptr)
	{
		nullcThrowError("ERROR: null pointer access");
		return;
	}

	NULLCFuncPtr<> &fPtr = *(NULLCFuncPtr<>*)function.ptr;

	NULLCFuncInfo *func = __nullcGetFunctionInfo(fPtr.id);

	NULLCTypeInfo *contextType = __nullcGetTypeInfo(func->extraType);

	if(contextType->subTypeID != context.typeID)
	{
		nullcThrowError("functionSetContext: cannot set context of type '%s' to the function '%s' expecting context of type '%s'", __nullcGetTypeInfo(context.typeID)->name, func->name, __nullcGetTypeInfo(func->extraType));
		return;
	}

	fPtr.context = context.ptr;
}
