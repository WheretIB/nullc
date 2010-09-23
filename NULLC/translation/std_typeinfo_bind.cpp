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
int isFunction(unsigned int type, void* unused)
{
	return __nullcGetTypeInfo(type)->category == NULLC_FUNCTION;
}
int isClass(unsigned int type, void* unused)
{
	return __nullcGetTypeInfo(type)->category == NULLC_CLASS;
}
int isSimple(unsigned int type, void* unused)
{
	// void double float long int short char
	unsigned hash = __nullcGetTypeInfo(type)->hash;
	return (hash == 2090838615u) || (hash == 4181547808u) || (hash == 259121563u) || (hash == 2090479413u) || (hash == 193495088u) || (hash == 274395349u) || (hash == 2090147939u);
}
int isArray(unsigned int type, void* unused)
{
	return __nullcGetTypeInfo(type)->category == NULLC_ARRAY;
}
int isPointer(unsigned int type, void* unused)
{
	return __nullcGetTypeInfo(type)->category == NULLC_POINTER;
}
int isFunction(NULLCRef type, void* unused)
{
	return __nullcGetTypeInfo(type.typeID)->category == NULLC_FUNCTION;
}
int isClass(NULLCRef type, void* unused)
{
	return __nullcGetTypeInfo(type.typeID)->category == NULLC_CLASS;
}
int isSimple(NULLCRef type, void* unused)
{
	// void double float long int short char
	unsigned hash = __nullcGetTypeInfo(type.typeID)->hash;
	return (hash == 2090838615u) || (hash == 4181547808u) || (hash == 259121563u) || (hash == 2090479413u) || (hash == 193495088u) || (hash == 274395349u) || (hash == 2090147939u);
}
int isArray(NULLCRef type, void* unused)
{
	return __nullcGetTypeInfo(type.typeID)->category == NULLC_ARRAY;
}
int isPointer(NULLCRef type, void* unused)
{
	return __nullcGetTypeInfo(type.typeID)->category == NULLC_POINTER;
}
NULLCArray<char> typeid__name__char___ref__(unsigned int * __context)
{
	NULLCArray<char> ret;
	ret.ptr = (char*)__nullcGetTypeInfo(*__context)->name;
	ret.size = strlen(__nullcGetTypeInfo(*__context)->name) + 1;
	return ret;
}
int typeid__memberCount_int_ref__(unsigned int * __context)
{
	NULLCTypeInfo *exType = __nullcGetTypeInfo(*__context);
	if(exType->category != NULLC_CLASS)
	{
		nullcThrowError("typeid::memberCount: type (%s) is not a class", exType->name);
		return 0;
	}
	return exType->memberCount;
}
unsigned int typeid__memberType_typeid_ref_int_(int member, unsigned int * __context)
{
	// $$$
	return 0;
}
NULLCArray<char> typeid__memberName_char___ref_int_(int member, unsigned int * __context)
{
	NULLCArray<char> ret;
	return ret;
}
unsigned int typeid__subType_typeid_ref__(unsigned int * __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(*__context);
	if(type->category != NULLC_ARRAY && type->category != NULLC_POINTER)
	{
		nullcThrowError("typeid::subType received type (%s) that neither pointer nor array", type->name);
		return ~0u;
	}
	return type->subTypeID;
}
int typeid__arraySize_int_ref__(unsigned int * __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(*__context);
	if(type->category != NULLC_ARRAY)
	{
		nullcThrowError("typeid::arraySize received type (%s) that is not an array", type->name);
		return -1;
	}
	return type->memberCount;
}
unsigned int typeid__returnType_typeid_ref__(unsigned int * __context)
{
	// $$$
	return 0;
}
int typeid__argumentCount_int_ref__(unsigned int * __context)
{
	// $$$
	return 0;
}
unsigned int typeid__argumentType_typeid_ref_int_(int argument, unsigned int * __context)
{
	// $$$
	return 0;
}
