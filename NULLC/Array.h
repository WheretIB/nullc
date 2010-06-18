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
		data = &one;
		if(zeroNewMemory)
			memset(data, 0, sizeof(T));
		max = 1;
		count = 0;
	}
	explicit FastVector(unsigned int reserved)
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
		if(data != &one)
		{
			if(!skipConstructor)
				NULLC::destruct(data, max);
			else
				NULLC::alignedDealloc(data);
		}
	}
	void	reset()
	{
		if(data != &one)
		{
			if(!skipConstructor)
				NULLC::destruct(data, max);
			else
				NULLC::alignedDealloc(data);
		}
		data = &one;
		if(zeroNewMemory)
			memset(data, 0, sizeof(T));
		max = 1;
		count = 0;
	}

	T*		push_back()
	{
		count++;
		if(count == max)
			grow(count);
		return &data[count - 1];
	};
	__forceinline void		push_back(const T& val)
	{
		assert(data);
		data[count++] = val;
		if(count == max)
			grow(count);
	};
	__forceinline void		push_back(const T* valPtr, unsigned int elem)
	{
		if(count + elem >= max)
			grow(count + elem);
		for(unsigned int i = 0; i < elem; i++)
			data[count++] = valPtr[i];
	};
	__forceinline T&		back()
	{
		assert(count > 0);
		return data[count-1];
	}
	__forceinline unsigned int		size()
	{
		return count;
	}
	__forceinline void		pop_back()
	{
		assert(count > 0);
		count--;
	}
	__forceinline void		clear()
	{
		count = 0;
	}
	__forceinline T&		operator[](unsigned int index)
	{
		assert(index < count);
		return data[index];
	}
	__forceinline void		resize(unsigned int newSize)
	{
		if(newSize >= max)
			grow(newSize);
		count = newSize;
	}
	__forceinline void		shrink(unsigned int newSize)
	{
		assert(newSize <= count);
		count = newSize;
	}
	__forceinline void		reserve(unsigned int resSize)
	{
		if(resSize >= max)
			grow(resSize);
	}

	__inline void	grow(unsigned int newSize)
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
		memcpy(newData, data, max * sizeof(T));
		if(data != &one)
		{
			if(!skipConstructor)
				NULLC::destruct(data, max);
			else
				NULLC::alignedDealloc(data);
		}
		data = newData;
		max = newSize;
	}
	T	*data;
	T	one;
	unsigned int	max, count;
private:
	// Disable assignment and copy constructor
	void operator =(FastVector &r);
	FastVector(FastVector &r);
};

#endif
