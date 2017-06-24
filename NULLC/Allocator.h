#pragma once
#ifndef NULLC_ALLOCATOR_H
#define NULLC_ALLOCATOR_H

#include "stdafx.h"
#include "Pool.h"

struct Allocator
{
	virtual void* alloc(int size)
	{
		return NULLC::alloc(size);
	}

	virtual void dealloc(void* ptr)
	{
		NULLC::dealloc(ptr);
	}

	template<typename T>
	T* construct()
	{
		return new(alloc(sizeof(T))) T();
	}
	template<typename T>
	T* construct(int count)
	{
		void *tmp = (void*)alloc(4 + count * sizeof(T));
		new(tmp) T[count];
		return (T*)tmp;
	}

	template<typename T>
	void destruct(T* ptr)
	{
		if(!ptr)
			return;
		ptr->~T();
		dealloc((void*)ptr);
	}
	template<typename T>
	void destruct(T* ptr, size_t count)
	{
		if(!ptr)
			return;
		for(size_t i = 0; i < count; i++)
			ptr[i].~T();
		dealloc(ptr);
	}
};

template<typename T>
struct GrowingAllocatorRef: Allocator
{
	GrowingAllocatorRef(T &pool): pool(pool)
	{
	}

	virtual void* alloc(int size)
	{
		return pool.Allocate(size);
	}

	virtual void dealloc(void* ptr)
	{
		(void)ptr;
	}

	T &pool;

private:
	// Disable assignment and copy constructor
	void operator =(GrowingAllocatorRef &r);
	GrowingAllocatorRef(GrowingAllocatorRef &r);
};

#endif
