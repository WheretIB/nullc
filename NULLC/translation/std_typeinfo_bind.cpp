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
	// TDOO: bool, type index lookup
	// void double float long int short char
	unsigned hash = __nullcGetTypeInfo(type)->hash;
	return (hash == 2090838615u) || (hash == 4181547808u) || (hash == 259121563u) || (hash == 2090479413u) || (hash == 193495088u) || (hash == 274395349u) || (hash == 2090147939u);
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
	// TDOO: bool, type index lookup
	// void double float long int short char
	unsigned hash = __nullcGetTypeInfo(type.typeID)->hash;
	return (hash == 2090838615u) || (hash == 4181547808u) || (hash == 259121563u) || (hash == 2090479413u) || (hash == 193495088u) || (hash == 274395349u) || (hash == 2090147939u);
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
	NULLCArray<char> ret;
	ret.ptr = (char*)__nullcGetTypeInfo(*__context)->name;
	ret.size = strlen(__nullcGetTypeInfo(*__context)->name) + 1;
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
	// $$$
	return 0;
}

NULLCArray<char> typeid__memberName_char___ref_int_(int member, unsigned* __context)
{
	NULLCArray<char> ret;
	return ret;
}

unsigned typeid__subType_typeid_ref__(unsigned* __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(*__context);
	if(type->category != NULLC_ARRAY && type->category != NULLC_POINTER)
	{
		nullcThrowError("typeid::subType received type (%s) that neither pointer nor array", type->name);
		return ~0u;
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
	// $$$
	return 0;
}

int typeid__argumentCount_int_ref__(unsigned* __context)
{
	// $$$
	return 0;
}

unsigned typeid__argumentType_typeid_ref_int_(int argument, unsigned* __context)
{
	// $$$
	return 0;
}

NULLCRef typeGetMember_auto_ref_ref_auto_ref_int_(NULLCRef obj, int member, void* __context)
{
	// $$$
	NULLCRef r = { 0, 0 };
	return r;
}

NULLCRef typeGetMember_auto_ref_ref_auto_ref_char___(NULLCRef obj, NULLCArray<char> name, void* __context)
{
	// $$$
	NULLCRef r = { 0, 0 };
	return r;
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
	// $$$
	return 0;
}

NULLCRef createInstanceByName_auto_ref_ref_char___(NULLCArray<char> name, void* __context)
{
	// $$$
	NULLCRef r = { 0, 0 };
	return r;
}

NULLCRef createInstanceByType_auto_ref_ref_typeid_(unsigned type, void* __context)
{
	// $$$
	NULLCRef r = { 0, 0 };
	return r;
}

NULLCAutoArray createArrayByName_auto___ref_char___int_(NULLCArray<char> name, int size, void* __context)
{
	// $$$
	NULLCAutoArray arr = { 0, 0, 0 };
	return arr;
}

NULLCAutoArray createArrayByType_auto___ref_typeid_int_(unsigned type, int size, void* __context)
{
	// $$$
	NULLCAutoArray arr = { 0, 0, 0 };
	return arr;
}

unsigned functionGetContextType_typeid_ref_auto_ref_(NULLCRef function, void* __context)
{
	// $$$
	return 0;
}

NULLCRef functionGetContext_auto_ref_ref_auto_ref_(NULLCRef function, void* __context)
{
	// $$$
	NULLCRef r = { 0, 0 };
	return r;
}

void functionSetContext_void_ref_auto_ref_auto_ref_(NULLCRef function, NULLCRef context, void* __context)
{
	// $$$
}
