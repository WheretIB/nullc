#include "runtime.h"
// Array classes
typedef struct
{
	unsigned int classID;
	int pos;
} member_iterator ;
typedef struct
{
	unsigned int type;
	NULLCArray<char > name;
} member_info ;
typedef struct
{
	unsigned int funcID;
	int pos;
} argument_iterator ;
typedef struct
{
	unsigned int elemType;
	int flags;
	int elemSize;
	NULLCAutoArray data;
	int currSize;
} vector ;
typedef struct
{
	vector * arr;
	int pos;
} vector_iterator ;
extern int isFunction(unsigned int type, void* unused);
extern int isClass(unsigned int type, void* unused);
extern int isSimple(unsigned int type, void* unused);
extern int isArray(unsigned int type, void* unused);
extern int isPointer(unsigned int type, void* unused);
extern int isFunction(NULLCRef type, void* unused);
extern int isClass(NULLCRef type, void* unused);
extern int isSimple(NULLCRef type, void* unused);
extern int isArray(NULLCRef type, void* unused);
extern int isPointer(NULLCRef type, void* unused);
extern int typeid__size_(unsigned int * __context);
extern NULLCArray<char > typeid__name_(unsigned int * __context);
extern int typeid__memberCount(unsigned int * __context);
extern unsigned int typeid__memberType(int member, unsigned int * __context);
extern NULLCArray<char > typeid__memberName(int member, unsigned int * __context);
extern unsigned int typeid__subType(unsigned int * __context);
extern int typeid__arraySize(unsigned int * __context);
extern unsigned int typeid__returnType(unsigned int * __context);
extern int typeid__argumentCount(unsigned int * __context);
extern unsigned int typeid__argumentType(int argument, unsigned int * __context);
extern member_iterator typeid__members(unsigned int * __context);
extern member_iterator * member_iterator__start(member_iterator * __context);
extern int member_iterator__hasnext(member_iterator * __context);
extern member_info member_iterator__next(member_iterator * __context);
extern argument_iterator typeid__arguments(unsigned int * __context);
extern argument_iterator * argument_iterator__start(argument_iterator * __context);
extern int argument_iterator__hasnext(argument_iterator * __context);
extern unsigned int argument_iterator__next(argument_iterator * __context);

void cConstructVector(vector * vec, unsigned int type, int reserved, void* unused)
{
	vec->elemType = type;
	vec->flags = isPointer(type, NULL);
	vec->currSize = 0;
	vec->elemSize = typeid__size_(&type);
	vec->data.typeID = type;
	if(reserved)
	{
		vec->data.ptr = (char*)__newS(vec->elemSize * reserved, NULL);
		vec->data.len = reserved;
	}else{
		vec->data.ptr = NULL;
		vec->data.len = 0;
	}
}
NULLCRef vector_iterator__next(vector_iterator * iter)
{
	NULLCRef ret;
	ret.typeID = (iter->arr->flags ? typeid__subType(&iter->arr->elemType) : iter->arr->elemType);
	ret.ptr =  iter->arr->flags ? ((char**)iter->arr->data.ptr)[iter->pos] : iter->arr->data.ptr + iter->arr->elemSize * iter->pos;
	iter->pos++;
	return ret;
}
int vector_iterator__hasnext(vector_iterator * iter)
{
	return iter->pos < iter->arr->currSize;
}
void vector__push_back(NULLCRef val, vector * vec)
{
	// Check that we received type that is equal to array element type
	if(val.typeID != (vec->flags ? typeid__subType(&vec->elemType) : vec->elemType))
	{
		nullcThrowError("vector::push_back received value (%s) that is different from vector type (%s)", typeid__name_(&val.typeID).ptr, typeid__name_(&vec->elemType).ptr);
		return;
	}
	// If not enough space
	if(vec->currSize == vec->data.len)
	{
		// Allocate new
		unsigned int newSize = 32 > vec->data.len ? 32 : (vec->data.len << 1) + vec->data.len;
		char *newData = (char*)__newS(vec->elemSize * newSize, NULL);
		memcpy(newData, vec->data.ptr, vec->elemSize * vec->data.len);
		vec->data.len = newSize;
		vec->data.ptr = newData;
	}
	memcpy(vec->data.ptr + vec->elemSize * vec->currSize, vec->flags ? (char*)&val.ptr : val.ptr, vec->elemSize);
	vec->currSize++;
}
void vector__pop_back(vector * vec)
{
	if(!vec->currSize)
	{
		nullcThrowError("vector::pop_back called on an empty vector");
		return;
	}
	vec->currSize--;
}
NULLCRef vector__front(vector * vec)
{
	NULLCRef ret = { 0, 0 };
	if(!vec->currSize)
	{
		nullcThrowError("vector::front called on an empty vector");
		return ret;
	}
	ret.typeID = (vec->flags ? typeid__subType(&vec->elemType) : vec->elemType);
	ret.ptr = vec->flags ? *(char**)vec->data.ptr : vec->data.ptr;
	return ret;
}
NULLCRef vector__back(vector * vec)
{
	NULLCRef ret = { 0, 0 };
	if(!vec->currSize)
	{
		nullcThrowError("vector::back called on an empty vector");
		return ret;
	}
	ret.typeID = (vec->flags ? typeid__subType(&vec->elemType) : vec->elemType);
	ret.ptr = vec->flags ? ((char**)vec->data.ptr)[vec->currSize - 1] : (vec->data.ptr + vec->elemSize * (vec->currSize - 1));
	return ret;
}
NULLCRef __operatorIndex(vector * vec, int index, void* unused)
{
	NULLCRef ret = { 0, 0 };
	if(index >= vec->currSize)
	{
		nullcThrowError("operator[] array index out of bounds");
		return ret;
	}
	ret.typeID = (vec->flags ? typeid__subType(&vec->elemType) : vec->elemType);
	ret.ptr = vec->flags ? ((char**)vec->data.ptr)[index] : (vec->data.ptr + vec->elemSize * index);
	return ret;
}
void vector__reserve(int size, vector * vec)
{
	// If not enough space
	if(size > vec->data.len)
	{
		// Allocate new
		char *newData = (char*)__newS(vec->elemSize * size, NULL);
		memcpy(newData, vec->data.ptr, vec->elemSize * vec->data.len);
		vec->data.len = size;
		vec->data.ptr = newData;
	}
}
void vector__resize(int size, vector * vec)
{
	vector__reserve(size, vec);
	vec->currSize = size;
}
void vector__clear(vector * vec)
{
	vec->currSize = 0;
}
void vector__destroy(vector * vec)
{
	vec->currSize = 0;
	vec->data.len = 0;
	vec->data.ptr = NULL;
}
int vector__size(vector * vec)
{
	return vec->currSize;
}
int vector__capacity(vector * vec)
{
	return vec->data.len;
}
