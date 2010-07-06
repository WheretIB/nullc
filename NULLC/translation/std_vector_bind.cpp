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
	NULLCArray<char > name;
};
struct argument_iterator
{
	unsigned int funcID;
	int pos;
};
struct vector
{
	unsigned int elemType;
	int flags;
	int elemSize;
	NULLCAutoArray data;
	int currSize;
};
struct vector_iterator
{
	vector * arr;
	int pos;
};
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
extern int typeid__size__int_ref__(unsigned int * __context);
extern NULLCArray<char > typeid__name__char___ref__(unsigned int * __context);
extern int typeid__memberCount_int_ref__(unsigned int * __context);
extern unsigned int typeid__memberType_typeid_ref_int_(int member, unsigned int * __context);
extern NULLCArray<char > typeid__memberName_char___ref_int_(int member, unsigned int * __context);
extern unsigned int typeid__subType_typeid_ref__(unsigned int * __context);
extern int typeid__arraySize_int_ref__(unsigned int * __context);
extern unsigned int typeid__returnType_typeid_ref__(unsigned int * __context);
extern int typeid__argumentCount_int_ref__(unsigned int * __context);
extern unsigned int typeid__argumentType_typeid_ref_int_(int argument, unsigned int * __context);
extern member_iterator typeid__members_member_iterator_ref__(unsigned int * __context);
extern member_iterator * member_iterator__start_member_iterator_ref_ref__(member_iterator * __context);
extern int member_iterator__hasnext_int_ref__(member_iterator * __context);
extern member_info member_iterator__next_member_info_ref__(member_iterator * __context);
extern argument_iterator typeid__arguments_argument_iterator_ref__(unsigned int * __context);
extern argument_iterator * argument_iterator__start_argument_iterator_ref_ref__(argument_iterator * __context);
extern int argument_iterator__hasnext_int_ref__(argument_iterator * __context);
extern unsigned int argument_iterator__next_typeid_ref__(argument_iterator * __context);

void cConstructVector(vector * vec, unsigned int type, int reserved, void* unused)
{
	vec->elemType = type;
	vec->flags = isPointer(type, NULL);
	vec->currSize = 0;
	vec->elemSize = typeid__size__int_ref__(&type);
	vec->data.typeID = type;
	if(reserved)
	{
		vec->data.ptr = (char*)__newS(vec->elemSize * reserved, 0);
		vec->data.len = reserved;
	}else{
		vec->data.ptr = NULL;
		vec->data.len = 0;
	}
}
NULLCRef vector_iterator__next_auto_ref_ref__(vector_iterator * iter)
{
	NULLCRef ret;
	ret.typeID = (iter->arr->flags ? typeid__subType_typeid_ref__(&iter->arr->elemType) : iter->arr->elemType);
	ret.ptr =  iter->arr->flags ? ((char**)iter->arr->data.ptr)[iter->pos] : iter->arr->data.ptr + iter->arr->elemSize * iter->pos;
	iter->pos++;
	return ret;
}
int vector_iterator__hasnext_int_ref__(vector_iterator * iter)
{
	return iter->pos < iter->arr->currSize;
}
void vector__push_back_void_ref_auto_ref_(NULLCRef val, vector * vec)
{
	// Check that we received type that is equal to array element type
	if(val.typeID != (vec->flags ? typeid__subType_typeid_ref__(&vec->elemType) : vec->elemType))
	{
		nullcThrowError("vector::push_back received value (%s) that is different from vector type (%s)", typeid__name__char___ref__(&val.typeID).ptr, typeid__name__char___ref__(&vec->elemType).ptr);
		return;
	}
	// If not enough space
	if(vec->currSize == vec->data.len)
	{
		// Allocate new
		unsigned int newSize = 32 > vec->data.len ? 32 : (vec->data.len << 1) + vec->data.len;
		char *newData = (char*)__newS(vec->elemSize * newSize, 0);
		memcpy(newData, vec->data.ptr, vec->elemSize * vec->data.len);
		vec->data.len = newSize;
		vec->data.ptr = newData;
	}
	memcpy(vec->data.ptr + vec->elemSize * vec->currSize, vec->flags ? (char*)&val.ptr : val.ptr, vec->elemSize);
	vec->currSize++;
}
void vector__pop_back_void_ref__(vector * vec)
{
	if(!vec->currSize)
	{
		nullcThrowError("vector::pop_back called on an empty vector");
		return;
	}
	vec->currSize--;
}
NULLCRef vector__front_auto_ref_ref__(vector * vec)
{
	NULLCRef ret = { 0, 0 };
	if(!vec->currSize)
	{
		nullcThrowError("vector::front called on an empty vector");
		return ret;
	}
	ret.typeID = (vec->flags ? typeid__subType_typeid_ref__(&vec->elemType) : vec->elemType);
	ret.ptr = vec->flags ? *(char**)vec->data.ptr : vec->data.ptr;
	return ret;
}
NULLCRef vector__back_auto_ref_ref__(vector * vec)
{
	NULLCRef ret = { 0, 0 };
	if(!vec->currSize)
	{
		nullcThrowError("vector::back called on an empty vector");
		return ret;
	}
	ret.typeID = (vec->flags ? typeid__subType_typeid_ref__(&vec->elemType) : vec->elemType);
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
	ret.typeID = (vec->flags ? typeid__subType_typeid_ref__(&vec->elemType) : vec->elemType);
	ret.ptr = vec->flags ? ((char**)vec->data.ptr)[index] : (vec->data.ptr + vec->elemSize * index);
	return ret;
}
void vector__reserve_void_ref_int_(int size, vector * vec)
{
	// If not enough space
	if(size > vec->data.len)
	{
		// Allocate new
		char *newData = (char*)__newS(vec->elemSize * size, 0);
		memcpy(newData, vec->data.ptr, vec->elemSize * vec->data.len);
		vec->data.len = size;
		vec->data.ptr = newData;
	}
}
void vector__resize_void_ref_int_(int size, vector * vec)
{
	vector__reserve_void_ref_int_(size, vec);
	vec->currSize = size;
}
void vector__clear_void_ref__(vector * vec)
{
	vec->currSize = 0;
}
void vector__destroy_void_ref__(vector * vec)
{
	vec->currSize = 0;
	vec->data.len = 0;
	vec->data.ptr = NULL;
}
int vector__size_int_ref__(vector * vec)
{
	return vec->currSize;
}
int vector__capacity_int_ref__(vector * vec)
{
	return vec->data.len;
}
