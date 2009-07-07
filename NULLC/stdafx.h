#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#pragma warning(disable: 4530)
#pragma warning(disable: 4127)
#endif

#ifndef _MSC_VER
#define __forceinline inline // TODO: NULLC_FORCEINLINE?
#endif

#include "nullcdef.h"

#include <stdlib.h>

#include "SharedPtr/shared_ptr.hpp"

#include <vector>

#include <list>
#include <string>

#include <math.h>
using namespace std;

#include <assert.h>
#ifdef NDEBUG
#undef assert
#define assert(expr)	((void)sizeof(!(expr)))
#endif

template<typename T, bool zeroNewMemory = false>
class FastVector
{
public:
	FastVector()
	{
		data = new T[256];
		if(zeroNewMemory)
			memset(data, 0, 256 * sizeof(T));
		max = 256;
		m_size = 0;
	}
	FastVector(unsigned int reserved)
	{
		data = new T[reserved];
		if(zeroNewMemory)
			memset(data, 0, reserved * sizeof(T));
		max = reserved;
		m_size = 0;
	}
	~FastVector(){ delete[] data; }

	__forceinline void		push_back(const T& val){ data[m_size++] = val; if(m_size==max) grow(m_size); };
	__forceinline void		push_back(const T* valptr, unsigned int count)
	{
		if(m_size+count>=max) grow(m_size+count);
		for(unsigned int i = 0; i < count; i++) data[m_size++] = valptr[i];
	};
	__forceinline T&		back(){ return data[m_size-1]; }
	__forceinline unsigned int		size(){ return m_size; }
	__forceinline void		pop_back(){ m_size--; }
	__forceinline void		clear(){ m_size = 0; }
	__forceinline T&		operator[](unsigned int index){ return data[index]; }
	__forceinline void		resize(unsigned int newsize){ m_size = newsize; if(m_size>=max) grow(m_size); }
	__forceinline void		shrink(unsigned int newSize){ m_size = newSize; }
	__forceinline void		reserve(unsigned int ressize){ if(ressize >= max) grow(ressize); }
private:
	__inline void	grow(unsigned int newSize)
	{
		if(max+(max>>1) > newSize)
			newSize = max+(max>>1);
		else
			newSize += 32;
		//assert(max+(max>>1) >= newSize);
		T* ndata = new T[newSize];
		if(zeroNewMemory)
			memset(ndata, 0, newSize*sizeof(T));
		memcpy(ndata, data, m_size*sizeof(T));
		delete[] data;
		data=ndata;
		max=newSize;
	}
	T	*data;
	unsigned int	max, m_size;
};
