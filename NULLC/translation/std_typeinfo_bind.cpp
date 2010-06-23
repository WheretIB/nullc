#include "runtime.h"
// Array classes
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
int isFunction(unsigned int type, void* unused)
{
	// $$$
	return 0;
}
int isClass(unsigned int type, void* unused)
{
	// $$$
	return 0;
}
int isSimple(unsigned int type, void* unused)
{
	// $$$
	return 0;
}
int isArray(unsigned int type, void* unused)
{
	// $$$
	return 0;
}
int isPointer(unsigned int type, void* unused)
{
	// $$$
	return 0;
}
int isFunction(NULLCRef type, void* unused)
{
	// $$$
	return 0;
}
int isClass(NULLCRef type, void* unused)
{
	// $$$
	return 0;
}
int isSimple(NULLCRef type, void* unused)
{
	// $$$
	return 0;
}
int isArray(NULLCRef type, void* unused)
{
	// $$$
	return 0;
}
int isPointer(NULLCRef type, void* unused)
{
	// $$$
	return 0;
}
int typeid__size__int_ref__(unsigned int * __context)
{
	return __nullcGetTypeInfo(*__context)->size;
}
NULLCArray<char> typeid__name__char___ref__(unsigned int * __context)
{
	NULLCArray<char> ret;
	return ret;
}
int typeid__memberCount_int_ref__(unsigned int * __context)
{
	// $$$
	return 0;
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
	// $$$
	return 0;
}
int typeid__arraySize_int_ref__(unsigned int * __context)
{
	// $$$
	return 0;
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
