#include "runtime.h"
// Array classes
typedef struct
{
	unsigned int classID;
	int pos;
} member_iterator;
typedef struct
{
	unsigned int type;
	NULLCArray<char> name;
} member_info;
typedef struct
{
	unsigned int funcID;
	int pos;
} argument_iterator;
unsigned int typeid__(NULLCRef type, void* unused)
{
	return type.typeID;
}
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
int typeid__size_(unsigned int * __context)
{
	return __nullcGetTypeInfo(*__context)->size;
}
NULLCArray<char> typeid__name_(unsigned int * __context)
{
	NULLCArray<char> ret();
	return ret;
}
int typeid__memberCount(unsigned int * __context)
{
	// $$$
	return 0;
}
unsigned int typeid__memberType(int member, unsigned int * __context)
{
	// $$$
	return 0;
}
NULLCArray<char> typeid__memberName(int member, unsigned int * __context)
{
	NULLCArray<char> ret();
	return ret;
}
unsigned int typeid__subType(unsigned int * __context)
{
	// $$$
	return 0;
}
int typeid__arraySize(unsigned int * __context)
{
	// $$$
	return 0;
}
unsigned int typeid__returnType(unsigned int * __context)
{
	// $$$
	return 0;
}
int typeid__argumentCount(unsigned int * __context)
{
	// $$$
	return 0;
}
unsigned int typeid__argumentType(int argument, unsigned int * __context)
{
	// $$$
	return 0;
}
int __operatorEqual(unsigned int a, unsigned int b, void* unused)
{
	return a == b;
}
int __operatorNEqual(unsigned int a, unsigned int b, void* unused)
{
	return a != b;
}