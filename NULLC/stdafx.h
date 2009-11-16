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

#include <string.h>
#include <setjmp.h>

#include <math.h>

#include <assert.h>

#ifdef NDEBUG
#undef assert
#define assert(expr)	((void)sizeof(!(expr)))
#endif

#include "Array.h"
#include "Pool.h"
#include "StrAlgo.h"

namespace NULLC
{
	void*	defaultAlloc(size_t size);
	void	defaultDealloc(void* ptr);

	extern void*	(*alloc)(size_t);
	extern void		(*dealloc)(void*);

	template<typename T>
	static T*		construct()
	{
		return new(alloc(sizeof(T))) T();
	}
	template<typename T>
	static T*		construct(size_t count)
	{
		return new(alloc(count * sizeof(T))) T[count];
	}

	template<typename T>
	static void		destruct(T* ptr)
	{
		ptr->~T();
		dealloc((void*)ptr);
	}
	template<typename T>
	static void		destruct(T* ptr, size_t count)
	{
		for(size_t i = 0; i < count; i++)
			ptr[i].~T();
		dealloc(ptr);
	}
}

