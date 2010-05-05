#include "stdafx.h"

void*	NULLC::defaultAlloc(int size)
{
	return ::new(std::nothrow) char[size];
}
void	NULLC::defaultDealloc(void* ptr)
{
	::delete[] (char*)ptr;
}

void*	(*NULLC::alloc)(int) = NULLC::defaultAlloc;
void	(*NULLC::dealloc)(void*) = NULLC::defaultDealloc;

void*	NULLC::alignedAlloc(int size)
{
	void *unaligned = alloc((size + 16 - 1) + sizeof(void*));
	if(!unaligned)
		return NULL;
	void *ptr = (void*)(((intptr_t)unaligned + sizeof(void*) + 16 - 1) & ~(16 - 1));
	*((void**)ptr - 1) = unaligned;
	return ptr;
}
void	NULLC::alignedDealloc(void* ptr)
{
	dealloc(*((void **)ptr - 1));
}

const void*	NULLC::defaultFileLoad(const char* name, unsigned int* size, int* nullcShouldFreePtr)
{
	assert(name);
	assert(size);
	assert(nullcShouldFreePtr);

	FILE *file = fopen(name, "rb");
	if(file)
	{
		fseek(file, 0, SEEK_END);
		*size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char *fileContent = (char*)NULLC::alloc(*size + 1);
		fread(fileContent, 1, *size, file);
		fileContent[*size] = 0;
		fclose(file);
		*nullcShouldFreePtr = 1;
		return fileContent;
	}
	*size = 0;
	*nullcShouldFreePtr = false;
	return NULL;
}

const void*	(*NULLC::fileLoad)(const char*, unsigned int*, int*) = NULLC::defaultFileLoad;
