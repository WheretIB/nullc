#include "vector.h"
#include "../nullc.h"
#include "../nullbind.h"
#include "typeinfo.h"

#include <string.h>
#include <memory.h>

namespace NULLCVector
{
#pragma pack(push, 4)
	struct vector
	{
		unsigned int	elemType;
		unsigned int	flags;
		unsigned int	elemSize;
		NULLCAutoArray	data;
		unsigned int	size;
	};

	struct vector_iterator
	{
		vector*			arr;
		unsigned int	pos;
	};
#pragma pack(pop)

	void ConstructVector(vector* vec, unsigned int type, int reserved)
	{
		vec->elemType = type;
		vec->flags = nullcIsPointer(type);
		vec->size = 0;
		vec->elemSize = nullcGetTypeSize(type);
		vec->data.typeID = type;
		if(reserved)
		{
			NULLCArray data = nullcAllocateArrayTyped(type, reserved);

			vec->data.ptr = data.ptr;
			vec->data.len = data.len;
		}else{
			vec->data.ptr = 0;
			vec->data.len = 0;
		}
	}

	void VectorPushBack(NULLCRef val, vector* vec)
	{
		// Check that we received type that is equal to array element type
		if(val.typeID != (vec->flags ? nullcGetSubType(vec->elemType) : vec->elemType))
		{
			nullcThrowError("vector::push_back received value (%s) that is different from vector type (%s)", nullcGetTypeName(val.typeID), nullcGetTypeName(vec->elemType));
			return;
		}
		// If not enough space
		if(vec->size == vec->data.len)
		{
			// Allocate new
			unsigned int newSize = 32 > vec->data.len ? 32 : (vec->data.len << 1) + vec->data.len;

			NULLCArray newData = nullcAllocateArrayTyped(vec->data.typeID, newSize);

			if(!newData.ptr)
				return;

			if(vec->data.len)
				memcpy(newData.ptr, vec->data.ptr, unsigned(vec->elemSize * vec->data.len));

			vec->data.ptr = newData.ptr;
			vec->data.len = newData.len;
		}
		memcpy(vec->data.ptr + vec->elemSize * vec->size, vec->flags ? (char*)&val.ptr : val.ptr, vec->elemSize);
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

	NULLCRef VectorFront(vector* vec)
	{
		NULLCRef ret = { 0, 0 };
		if(!vec->size)
		{
			nullcThrowError("vector::front called on an empty vector");
			return ret;
		}
		ret.typeID = (vec->flags ? nullcGetSubType(vec->elemType) : vec->elemType);
		ret.ptr = vec->flags ? *(char**)vec->data.ptr : vec->data.ptr;
		return ret;
	}

	NULLCRef VectorBack(vector* vec)
	{
		NULLCRef ret = { 0, 0 };
		if(!vec->size)
		{
			nullcThrowError("vector::back called on an empty vector");
			return ret;
		}
		ret.typeID = (vec->flags ? nullcGetSubType(vec->elemType) : vec->elemType);
		ret.ptr = vec->flags ? ((char**)vec->data.ptr)[vec->size - 1] : (vec->data.ptr + vec->elemSize * (vec->size - 1));
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
		ret.typeID = (vec->flags ? nullcGetSubType(vec->elemType) : vec->elemType);
		ret.ptr = vec->flags ? ((char**)vec->data.ptr)[index] : (vec->data.ptr + vec->elemSize * index);
		return ret;
	}

	void VectorReserve(unsigned int size, vector* vec)
	{
		// If not enough space
		if(size > vec->data.len)
		{
			// Allocate new
			NULLCArray newData = nullcAllocateArrayTyped(vec->data.typeID, size);

			if(!newData.ptr)
				return;

			memcpy(newData.ptr, vec->data.ptr, unsigned(vec->elemSize * vec->data.len));

			vec->data.ptr = newData.ptr;
			vec->data.len = newData.len;
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
		vec->data.ptr = 0;
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
		ret.typeID = (iter->arr->flags ? nullcGetSubType(iter->arr->elemType) : iter->arr->elemType);
		ret.ptr =  iter->arr->flags ? ((char**)iter->arr->data.ptr)[iter->pos] : iter->arr->data.ptr + iter->arr->elemSize * iter->pos;
		iter->pos++;
		return ret;
	}
	int VectorHasNext(vector_iterator* iter)
	{
		return iter->arr && iter->pos < iter->arr->size;
	}

}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("old.vector", NULLCVector::funcPtr, name, index)) return false;
bool	nullcInitVectorModule()
{
	REGISTER_FUNC(ConstructVector, "cConstructVector", 0);

	REGISTER_FUNC(VectorPushBack, "vector::push_back", 0);
	REGISTER_FUNC(VectorPopBack, "vector::pop_back", 0);
	REGISTER_FUNC(VectorFront, "vector::front", 0);
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
