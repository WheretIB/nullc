#include "stdafx.h"

void*	NULLC::defaultAlloc(size_t size)
{
	return ::new char[size];
}
void	NULLC::defaultDealloc(void* ptr)
{
	return ::delete ptr;
}

void*	(*NULLC::alloc)(size_t) = NULLC::defaultAlloc;
void	(*NULLC::dealloc)(void*) = NULLC::defaultDealloc;
