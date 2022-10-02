#pragma once
#ifndef NULLC_ARRAY_H
#define NULLC_ARRAY_H

#include "Allocator.h"

#ifndef _MSC_VER
#undef __forceinline
#define __forceinline inline // TODO: NULLC_FORCEINLINE?
#endif

template<typename T, bool zeroNewMemory = false, bool skipConstructor = false>
class FastVector
{
public:
	FastVector()
	{
		data = 0;
		max = 0;
		count = 0;
	}

	explicit FastVector(unsigned reserved)
	{
		if(!skipConstructor)
			data = NULLC::construct<T>(reserved);
		else
			data = (T*)NULLC::alignedAlloc(sizeof(T) * reserved);

		assert(data);

		if(zeroNewMemory)
			memset(data, 0, reserved * sizeof(T));

		max = reserved;
		count = 0;
	}

	~FastVector()
	{
		if(data)
		{
			if(!skipConstructor)
				NULLC::destruct(data, max);
			else
				NULLC::alignedDealloc(data);
		}
	}

	void reset()
	{
		if(data)
		{
			if(!skipConstructor)
				NULLC::destruct(data, max);
			else
				NULLC::alignedDealloc(data);
		}

		data = 0;
		max = 0;
		count = 0;
	}

	T* push_back()
	{
		if(count == max)
			grow(count);

		count++;

		return &data[count - 1];
	}

	__forceinline void push_back(const T& val)
	{
		if(count == max)
		{
			grow_and_add(count, val);
			return;
		}

		data[count++] = val;
	}

	__forceinline void push_back(const T* valPtr, unsigned elem)
	{
		if(count + elem >= max)
		{
			grow_and_add(count + elem, valPtr, elem);
			return;
		}

		for(unsigned i = 0; i < elem; i++)
			data[count++] = valPtr[i];
	}

	__forceinline T& back()
	{
		assert(count > 0);

		return data[count - 1];
	}

	__forceinline unsigned size() const
	{
		return count;
	}

	__forceinline bool empty() const
	{
		return count == 0;
	}

	__forceinline void pop_back()
	{
		assert(count > 0);

		count--;
	}

	__forceinline void clear()
	{
		count = 0;
	}

	__forceinline T& operator[](unsigned index)
	{
		assert(index < count);

		return data[index];
	}

	__forceinline void resize(unsigned newSize)
	{
		if(newSize >= max)
			grow(newSize);

		count = newSize;
	}

	__forceinline void shrink(unsigned newSize)
	{
		assert(newSize <= count);

		count = newSize;
	}

	__forceinline void reserve(unsigned resSize)
	{
		if(resSize >= max)
			grow(resSize);
	}

	void grow_no_destroy(unsigned newSize)
	{
		if(max + (max >> 1) > newSize)
			newSize = max + (max >> 1);
		else
			newSize += 32;

		T* newData;

		if(!skipConstructor)
			newData = NULLC::construct<T>(newSize);
		else
			newData = (T*)NULLC::alignedAlloc(sizeof(T) * newSize);

		assert(newData);

		if(zeroNewMemory)
			NULLC_no_warning_memset(newData, 0, newSize * sizeof(T));

		if(data)
			NULLC_no_warning_memcpy(newData, data, max * sizeof(T));

		data = newData;
		max = newSize;
	}

	void grow(unsigned newSize)
	{
		T* oldData = data;
		unsigned oldMax = max;

		grow_no_destroy(newSize);

		if(oldData)
		{
			if(!skipConstructor)
				NULLC::destruct(oldData, oldMax);
			else
				NULLC::alignedDealloc(oldData);
		}
	}

	void grow_and_add(unsigned newSize, const T& val)
	{
		T* oldData = data;
		unsigned oldMax = max;

		grow_no_destroy(newSize);

		data[count++] = val;

		if(oldData)
		{
			if(!skipConstructor)
				NULLC::destruct(oldData, oldMax);
			else
				NULLC::alignedDealloc(oldData);
		}
	}

	void grow_and_add(unsigned newSize, const T* valPtr, unsigned elem)
	{
		T* oldData = data;
		unsigned oldMax = max;

		grow_no_destroy(newSize);

		for(unsigned i = 0; i < elem; i++)
			data[count++] = valPtr[i];

		if(oldData)
		{
			if(!skipConstructor)
				NULLC::destruct(oldData, oldMax);
			else
				NULLC::alignedDealloc(oldData);
		}
	}

	T *data;
	unsigned max, count;

private:
	// Disable assignment and copy constructor
	void operator =(FastVector &r);
	FastVector(FastVector &r);
};

template<typename T, unsigned N>
class FixedArray
{
public:
	__forceinline T& front()
	{
		return data[0];
	}

	__forceinline T& back()
	{
		return data[N - 1];
	}

	__forceinline T* begin()
	{
		return &data[0];
	}
	
	__forceinline T* end()
	{
		return &data[N];
	}
	
	__forceinline bool empty() const
	{
		return false;
	}

	__forceinline unsigned size() const
	{
		return N;
	}

	__forceinline T& operator[](unsigned index)
	{
		assert(index < N);

		return data[index];
	}

	void fill(const T& value)
	{
		for(unsigned i = 0; i < N; i++)
			data[i] = value;
	}

	T data[N];
};

template<typename T, unsigned N>
class SmallArray
{
public:
	SmallArray(Allocator *allocator = 0): allocator(allocator)
	{
		data = little;
		max = N;

		count = 0;
	}

	~SmallArray()
	{
		if(data != little)
		{
			if(allocator)
				allocator->destruct(data, max);
			else
				NULLC::destruct(data, max);
		}
	}

	void set_allocator(Allocator *newAllocator)
	{
		assert(data == little);

		this->allocator = newAllocator;
	}

	void reset()
	{
		if(data != little)
		{
			if(allocator)
				allocator->destruct(data, max);
			else
				NULLC::destruct(data, max);
		}

		data = little;
		max = N;

		count = 0;
	}

	T* push_back()
	{
		if(count == max)
			grow(count);

		assert(data);

		count++;

		return &data[count - 1];
	}

	__forceinline void push_back(const T& val)
	{
		if(count == max)
			grow(count);

		assert(data);

		data[count++] = val;
	}

	__forceinline void push_back(const T* valPtr, unsigned elem)
	{
		if(count + elem >= max)
			grow(count + elem);

		for(unsigned i = 0; i < elem; i++)
			data[count++] = valPtr[i];
	}

	__forceinline T& back()
	{
		assert(count > 0);

		return data[count - 1];
	}

	__forceinline bool empty() const
	{
		return count == 0;
	}

	__forceinline unsigned size() const
	{
		return count;
	}

	__forceinline void pop_back()
	{
		assert(count > 0);

		count--;
	}

	__forceinline void clear()
	{
		count = 0;
	}

	T* begin()
	{
		return data;
	}

	T* end()
	{
		return data + count;
	}

	__forceinline T& operator[](unsigned index)
	{
		assert(index < count);

		return data[index];
	}

	void resize(unsigned newSize)
	{
		if(newSize >= max)
			grow(newSize);

		count = newSize;
	}

	void shrink(unsigned newSize)
	{
		assert(newSize <= count);

		count = newSize;
	}

	void reserve(unsigned resSize)
	{
		if(resSize >= max)
			grow(resSize);
	}

	void grow(unsigned newSize)
	{
		if(max + (max >> 1) > newSize)
			newSize = max + (max >> 1);
		else
			newSize += 4;

		T* newData = 0;
		
		if(allocator)
			newData = allocator->construct<T>(newSize);
		else
			newData = NULLC::construct<T>(newSize);

		for(unsigned i = 0; i < count; i++)
			newData[i] = data[i];

		if(data != little)
		{
			if(allocator)
				allocator->destruct(data, max);
			else
				NULLC::destruct(data, max);
		}

		data = newData;

		max = newSize;
	}

	bool contains(const T& val) const
	{
		for(unsigned i = 0; i < count; i++)
		{
			if(data[i] == val)
				return true;
		}

		return false;
	}

	T *data;
	unsigned count, max;

private:
	// Disable assignment and copy constructor
	void operator =(SmallArray &r);
	SmallArray(SmallArray &r);

	T little[N];

	Allocator *allocator;
};

template<typename T>
class ArrayView
{
public:
	ArrayView()
	{
		data = 0;
		count = 0;
	}

	ArrayView(FastVector<T> &rhs)
	{
		data = rhs.data;
		count = rhs.size();
	}

	template<unsigned N>
	ArrayView(FixedArray<T, N> &rhs)
	{
		data = rhs.data;
		count = rhs.size();
	}

	template<unsigned N>
	ArrayView(SmallArray<T, N> &rhs)
	{
		data = rhs.data;
		count = rhs.size();
	}

	template<unsigned N>
	ArrayView(SmallArray<T, N> &rhs, unsigned size)
	{
		assert(size <= rhs.size());

		data = rhs.data;
		count = size;
	}

	template<unsigned N>
	ArrayView(T (&rhs)[N])
	{
		data = rhs;
		count = N;
	}

	T& back()
	{
		assert(count > 0);

		return data[count - 1];
	}

	bool empty() const
	{
		return count == 0;
	}

	unsigned size() const
	{
		return count;
	}

	T& operator[](unsigned index)
	{
		assert(index < count);

		return data[index];
	}

	T *data;
	unsigned count;
};

template<typename T>
struct VectorView
{
	VectorView(): count(0), capacity(0), data(NULL)
	{
	}

	VectorView(T* data, unsigned capacity): count(0), capacity(capacity), data(data)
	{
	}

	T& push_back()
	{
		assert(count < capacity);

		return data[count++];
	}

	void push_back(const T& elem)
	{
		assert(count < capacity);

		data[count++] = elem;
	}

	void push_back(const T* elems, unsigned amount)
	{
		assert(count + amount <= capacity);

		memcpy(data + count, elems, amount * sizeof(T));
		count += amount;
	}

	unsigned count;
	unsigned capacity;

	T *data;
};

#endif
