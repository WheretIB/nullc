#include "vector.h"
#include "../nullc.h"
#include "typeinfo.h"

namespace NULLCVector
{
	struct vector
	{
		unsigned int	elemType;
		unsigned int	elemSize;
		NullCArray		data;
		unsigned int	size;
	};

	struct vector_iterator
	{
		vector*			arr;
		unsigned int	pos;
	};

	void ConstructVector(vector* vec, unsigned int type, int reserved)
	{
		vec->elemType = type;
		vec->size = 0;
		vec->elemSize = nullcGetTypeSize(type);
		if(reserved)
		{
			vec->data.ptr = (char*)nullcAllocate(vec->elemSize * reserved);
			vec->data.len = reserved;
		}else{
			vec->data.ptr = NULL;
			vec->data.len = 0;
		}
	}

	void VectorPushBack(NULLCRef val, vector* vec)
	{
		// Check that we received type that is equal to array element type
		if(val.typeID != vec->elemType)
		{
			nullcThrowError("vector::push_back received value that is different from vector type");
			return;
		}
		// If not enough space
		if(vec->size == vec->data.len)
		{
			// Allocate new
			unsigned int newSize = 32 > vec->data.len ? 32 : (vec->data.len << 1) + vec->data.len;
			char *newData = (char*)nullcAllocate(vec->elemSize * newSize);
			memcpy(newData, vec->data.ptr, vec->elemSize * vec->data.len);
			vec->data.len = newSize;
			vec->data.ptr = newData;
		}
		memcpy(vec->data.ptr + vec->elemSize * vec->size, val.ptr, vec->elemSize);
		vec->size++;
	}

	void VectorPopBack(vector* vec)
	{
		if(!vec->size)
		{
			nullcThrowError("vector::pop_back called on an empty vector");
			return;
		}
		vec->size--;
	}

	NULLCRef VectorBack(vector* vec)
	{
		NULLCRef ret = { 0, 0 };
		if(!vec->size)
		{
			nullcThrowError("vector::back called on an empty vector");
			return ret;
		}
		ret.typeID = vec->elemType;
		ret.ptr = vec->data.ptr + vec->elemSize * (vec->size - 1);
		return ret;
	}

	NULLCRef VectorIndex(vector* vec, unsigned int index)
	{
		NULLCRef ret = { 0, 0 };
		if(index >= vec->size)
		{
			nullcThrowError("operator[] array index out of bounds");
			return ret;
		}
		ret.typeID = vec->elemType;
		ret.ptr = vec->data.ptr + vec->elemSize * index;
		return ret;
	}

	void VectorReserve(unsigned int size, vector* vec)
	{
		// If not enough space
		if(size > vec->data.len)
		{
			// Allocate new
			char *newData = (char*)nullcAllocate(vec->elemSize * size);
			memcpy(newData, vec->data.ptr, vec->elemSize * vec->data.len);
			vec->data.len = size;
			vec->data.ptr = newData;
		}
	}

	void VectorResize(unsigned int size, vector* vec)
	{
		VectorReserve(size, vec);
		vec->size = size;
	}

	void VectorClear(vector* vec)
	{
		vec->size = 0;
	}

	void VectorDestroy(vector* vec)
	{
		vec->size = 0;
		vec->data.len = 0;
		vec->data.ptr = NULL;
	}

	int VectorSize(vector* vec)
	{
		return vec->size;
	}

	int VectorCapacity(vector* vec)
	{
		return vec->data.len;
	}

	NULLCRef VectorNext(vector_iterator* iter)
	{
		NULLCRef ret;
		ret.typeID = iter->arr->elemType;
		if(iter->pos >= iter->arr->size)
			ret.ptr = NULL;
		else
			ret.ptr = iter->arr->data.ptr + iter->arr->elemSize * iter->pos;
		iter->pos++;

		return ret;
	}
	int VectorHasNext(vector_iterator* iter)
	{
		return iter->pos <= iter->arr->size;
	}

}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("std.vector", (void(*)())NULLCVector::funcPtr, name, index)) return false;
bool	nullcInitVectorModule()
{
	REGISTER_FUNC(ConstructVector, "cConstructVector", 0);

	REGISTER_FUNC(VectorPushBack, "vector::push_back", 0);
	REGISTER_FUNC(VectorPopBack, "vector::pop_back", 0);
	REGISTER_FUNC(VectorBack, "vector::back", 0);
	REGISTER_FUNC(VectorIndex, "[]", 0);
	REGISTER_FUNC(VectorReserve, "vector::reserve", 0);
	REGISTER_FUNC(VectorResize, "vector::resize", 0);
	REGISTER_FUNC(VectorClear, "vector::clear", 0);
	REGISTER_FUNC(VectorDestroy, "vector::destroy", 0);
	REGISTER_FUNC(VectorSize, "vector::size", 0);
	REGISTER_FUNC(VectorCapacity, "vector::capacity", 0);

	REGISTER_FUNC(VectorNext, "vector_iterator::next", 0);
	REGISTER_FUNC(VectorHasNext, "vector_iterator::hasnext", 0);

	return true;
}

void	nullcDeinitVectorModule()
{

}
