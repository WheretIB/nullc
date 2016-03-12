#pragma once
#ifndef NULLC_ARRAY_H
#define NULLC_ARRAY_H

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
			grow(count);

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

	__forceinline unsigned size()
	{
		return count;
	}

	__forceinline bool empty()
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

	__inline void grow(unsigned newSize)
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
			memset(newData, 0, newSize * sizeof(T));

		if(data)
		{
			memcpy(newData, data, max * sizeof(T));

			if(!skipConstructor)
				NULLC::destruct(data, max);
			else
				NULLC::alignedDealloc(data);
		}

		data = newData;
		max = newSize;
	}

	T *data;
	unsigned max, count;

private:
	// Disable assignment and copy constructor
	void operator =(FastVector &r);
	FastVector(FastVector &r);
};

#endif
