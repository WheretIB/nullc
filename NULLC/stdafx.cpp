#include "stdafx.h"

void*	NULLC::defaultAlloc(int size)
{
	return ::new char[size];
}
void	NULLC::defaultDealloc(void* ptr)
{
	return ::delete (char*)ptr;
}

void*	(*NULLC::alloc)(int) = NULLC::defaultAlloc;
void	(*NULLC::dealloc)(void*) = NULLC::defaultDealloc;
