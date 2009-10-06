#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4996)	// function is deprecated
#pragma warning(disable: 4127)	// conditional expression is constant
#pragma warning(disable: 4611)	// interaction between '_setjmp' and C++ object destruction is non-portable
#endif

#ifndef _MSC_VER
#define __forceinline inline // TODO: NULLC_FORCEINLINE?
#define _snprintf snprintf
#endif

#include "nullcdef.h"

#include <new>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include <math.h>

#include <assert.h>

#ifdef NDEBUG
#undef assert
#define assert(expr)	((void)sizeof(!(expr)))
#endif

char*		DuplicateString(const char *str);

unsigned int GetStringHash(const char *str);
unsigned int GetStringHash(const char *str, const char *end);
unsigned int StringHashContinue(unsigned int hash, const char *str);
unsigned int StringHashContinue(unsigned int hash, const char *str, const char *end);

char* PrintInteger(char* str, int number);

template<typename T, bool zeroNewMemory = false, bool skipConstructor = false>
class FastVector
{
public:
	explicit FastVector(unsigned int reserved = 256)
	{
		if(!skipConstructor)
			data = new T[reserved];
		else
			data = (T*)new char[sizeof(T) * reserved];
		if(zeroNewMemory)
			memset(data, 0, reserved * sizeof(T));
		max = reserved;
		m_size = 0;
	}
	~FastVector()
	{
		if(!skipConstructor)
			delete[] data;
		else
			delete[] (char*)(data);
	}

	__forceinline T*		push_back(){ m_size++; if(m_size==max) grow(m_size); return &data[m_size - 1]; };
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
	__forceinline void		resize(unsigned int newsize){ if(newsize >= max) grow(newsize); m_size = newsize; }
	__forceinline void		shrink(unsigned int newSize){ m_size = newSize; }
	__forceinline void		reserve(unsigned int ressize){ if(ressize >= max) grow(ressize); }

	__inline void	grow(unsigned int newSize)
	{
		if(max+(max>>1) > newSize)
			newSize = max+(max>>1);
		else
			newSize += 32;
		T* ndata;
		if(!skipConstructor)
			ndata = new T[newSize];
		else
			ndata = (T*)new char[sizeof(T) * newSize];
		if(zeroNewMemory)
			memset(ndata, 0, newSize * sizeof(T));
		memcpy(ndata, data, max * sizeof(T));
		if(!skipConstructor)
			delete[] data;
		else
			delete[] (char*)(data);
		data=ndata;
		max=newSize;
	}
	T	*data;
	unsigned int	max, m_size;
};

template<int chunkSize>
class ChunkedStackPool
{
public:
	ChunkedStackPool()
	{
		curr = first = new StackChunk;
		first->next = NULL;
		size = 0;
	}
	~ChunkedStackPool()
	{
		while(first)
		{
			StackChunk *next = first->next;
			delete first;
			first = next;
		}
	}

	void	Clear()
	{
		curr = first;
		size = 0;
	}
	void	ClearTo(unsigned int bytes)
	{
		curr = first;
		while(bytes > chunkSize)
		{
			assert(curr->next != NULL);
			curr = curr->next;
			bytes -= chunkSize;
		}
		size = bytes;
	}
	void*	Allocate(unsigned int bytes)
	{
		assert(bytes < chunkSize);
		if(size + bytes < chunkSize)
		{
			size += bytes;
			return curr->data + size - bytes;
		}
		if(curr->next)
		{
			curr = curr->next;
			size = bytes;
			return curr->data;
		}
		curr->next = new StackChunk;
		curr = curr->next;
		curr->next = NULL;
		size = bytes;
		return curr->data;
	}
	unsigned int GetSize()
	{
		unsigned int wholeSize = 0;
		StackChunk *temp = first;
		while(temp != curr)
		{
			wholeSize += chunkSize;
			temp = temp->next;
		}
		return wholeSize + size;
	}
private:
	struct StackChunk
	{
		StackChunk *next;
		char		data[chunkSize];
	};
	StackChunk	*first, *curr;
	unsigned int size;
};

// A string that doesn't terminate with a \0 character
class InplaceStr
{
public:
	InplaceStr(){ begin = NULL; end = NULL; }
	// It is possible to construct it from \0-terminated string
	explicit InplaceStr(const char *strBegin){ begin = strBegin; end = begin + strlen(begin); }
	// And from non-terminating strings
	InplaceStr(const char *strBegin, unsigned int length){ begin = strBegin; end = begin + length; }
	InplaceStr(const char *strBegin, const char *strEnd){ begin = strBegin; end = strEnd; }

	const char *begin, *end;
};
