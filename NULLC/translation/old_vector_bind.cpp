#include "runtime.h"
// Array classes
#pragma pack(push, 4)
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
#pragma pack(pop)

int vector_isPointer_int_ref_typeid_(unsigned type, void* __context)
{
	return __nullcGetTypeInfo(type)->category == NULLC_POINTER;
}

unsigned vector_typeid__subType_typeid_ref__(unsigned* __context)
{
	NULLCTypeInfo *type = __nullcGetTypeInfo(*__context);
	if(type->category != NULLC_ARRAY && type->category != NULLC_POINTER)
	{
		nullcThrowError("typeid::subType received type (%s) that neither pointer nor array", type->name);
		return ~0u;
	}
	return type->subTypeID;
}

NULLCArray<char> vector_typeid__name__char___ref___(unsigned* __context)
{
	NULLCArray<char> ret;
	ret.ptr = (char*)__nullcGetTypeInfo(*__context)->name;
	ret.size = strlen(__nullcGetTypeInfo(*__context)->name) + 1;
	return ret;
}

void cConstructVector_void_ref_vector_ref_typeid_int_(vector * vec, unsigned int type, int reserved, void* unused)
{
	vec->elemType = type;
	vec->flags = vector_isPointer_int_ref_typeid_(type, NULL);
	vec->currSize = 0;
	vec->elemSize = typeid__size__int_ref___(&type);
	vec->data.typeID = type;
	if(reserved)
	{
		vec->data.ptr = (char*)__newS_void_ref_ref_int_int_(vec->elemSize * reserved, 0, 0);
		vec->data.len = reserved;
	}else{
		vec->data.ptr = NULL;
		vec->data.len = 0;
	}
}
NULLCRef vector_iterator__next_auto_ref_ref__(vector_iterator * iter)
{
	NULLCRef ret;
	ret.typeID = (iter->arr->flags ? vector_typeid__subType_typeid_ref__(&iter->arr->elemType) : iter->arr->elemType);
	ret.ptr =  iter->arr->flags ? ((char**)iter->arr->data.ptr)[iter->pos] : iter->arr->data.ptr + iter->arr->elemSize * iter->pos;
	iter->pos++;
	return ret;
}
int vector_iterator__hasnext_int_ref__(vector_iterator * iter)
{
	return iter->arr && iter->pos < iter->arr->currSize;
}
void vector__push_back_void_ref_auto_ref_(NULLCRef val, vector * vec)
{
	// Check that we received type that is equal to array element type
	if(val.typeID != (vec->flags ? vector_typeid__subType_typeid_ref__(&vec->elemType) : vec->elemType))
	{
		nullcThrowError("vector::push_back received value (%s) that is different from vector type (%s)", vector_typeid__name__char___ref___(&val.typeID).ptr, vector_typeid__name__char___ref___(&vec->elemType).ptr);
		return;
	}
	// If not enough space
	if(vec->currSize == vec->data.len)
	{
		// Allocate new
		unsigned int newSize = 32 > vec->data.len ? 32 : (vec->data.len << 1) + vec->data.len;
		char *newData = (char*)__newS_void_ref_ref_int_int_(vec->elemSize * newSize, 0, 0);
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
	ret.typeID = (vec->flags ? vector_typeid__subType_typeid_ref__(&vec->elemType) : vec->elemType);
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
	ret.typeID = (vec->flags ? vector_typeid__subType_typeid_ref__(&vec->elemType) : vec->elemType);
	ret.ptr = vec->flags ? ((char**)vec->data.ptr)[vec->currSize - 1] : (vec->data.ptr + vec->elemSize * (vec->currSize - 1));
	return ret;
}
NULLCRef __operatorIndex_auto_ref_ref_vector_ref_int_(vector * vec, int index, void* unused)
{
	NULLCRef ret = { 0, 0 };
	if(index >= vec->currSize)
	{
		nullcThrowError("operator[] array index out of bounds");
		return ret;
	}
	ret.typeID = (vec->flags ? vector_typeid__subType_typeid_ref__(&vec->elemType) : vec->elemType);
	ret.ptr = vec->flags ? ((char**)vec->data.ptr)[index] : (vec->data.ptr + vec->elemSize * index);
	return ret;
}
void vector__reserve_void_ref_int_(int size, vector * vec)
{
	// If not enough space
	if(size > vec->data.len)
	{
		// Allocate new
		char *newData = (char*)__newS_void_ref_ref_int_int_(vec->elemSize * size, 0, 0);
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
