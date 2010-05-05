#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4996)	// function is deprecated
#pragma warning(disable: 4127)	// conditional expression is constant
#pragma warning(disable: 4611)	// interaction between '_setjmp' and C++ object destruction is non-portable
#endif

#ifndef _MSC_VER
#define __forceinline inline // TODO: NULLC_FORCEINLINE?
#endif

#include "nullcdef.h"

#include <new>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#ifndef _MSC_VER
	#include <stdint.h>
#endif

#include <string.h>
#include <setjmp.h>

#include <math.h>

#include <assert.h>

#ifdef NDEBUG
#undef assert
#define assert(expr)	((void)sizeof(!(expr)))
#endif

namespace NULLC
{
	void*	defaultAlloc(int size);
	void	defaultDealloc(void* ptr);

	extern void*	(*alloc)(int);
	extern void		(*dealloc)(void*);

	void*	alignedAlloc(int size);
	void	alignedDealloc(void* ptr);

	template<typename T>
	static T*		construct()
	{
		return new(alloc(sizeof(T))) T();
	}
	template<typename T>
	static T*		construct(int count)
	{
		void *tmp = (void*)alloc(4 + count * sizeof(T));
		new(tmp) T[count];
		return (T*)tmp;
	}

	template<typename T>
	static void		destruct(T* ptr)
	{
		if(!ptr)
			return;
		ptr->~T();
		dealloc((void*)ptr);
	}
	template<typename T>
	static void		destruct(T* ptr, size_t count)
	{
		if(!ptr)
			return;
		for(size_t i = 0; i < count; i++)
			ptr[i].~T();
		dealloc(ptr);
	}

	const void* defaultFileLoad(const char* name, unsigned int* size, int* nullcShouldFreePtr);

	extern const void* (*fileLoad)(const char*, unsigned int*, int*);
}

#include "Array.h"
#include "Pool.h"
#include "StrAlgo.h"
