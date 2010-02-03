#include "stdafx.h"

void*	NULLC::defaultAlloc(int size)
{
	return ::new(std::nothrow) char[size];
}
void	NULLC::defaultDealloc(void* ptr)
{
	return ::delete (char*)ptr;
}

void*	(*NULLC::alloc)(int) = NULLC::defaultAlloc;
void	(*NULLC::dealloc)(void*) = NULLC::defaultDealloc;

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
