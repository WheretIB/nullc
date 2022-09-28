#include "stdafx.h"

void*	NULLC::defaultAlloc(int size)
{
	return ::new/*(std::nothrow)*/ char[size];
}
void	NULLC::defaultDealloc(void* ptr)
{
	::delete[] (char*)ptr;
}

void*	(*NULLC::alloc)(int) = NULLC::defaultAlloc;
void	(*NULLC::dealloc)(void*) = NULLC::defaultDealloc;

void* NULLC::alignedAlloc(int size)
{
	void *unaligned = alloc((size + 16 - 1) + sizeof(void*));
	if(!unaligned)
		return NULL;
	void *ptr = (void*)(((intptr_t)unaligned + sizeof(void*) + 16 - 1) & ~(16 - 1));
	memcpy((void**)ptr - 1, &unaligned, sizeof(unaligned));
	return ptr;
}

void* NULLC::alignedAlloc(int size, int extraSize)
{
	void *unaligned = alloc((size + 16 - 1) + sizeof(void*) + extraSize);
	if(!unaligned)
		return NULL;
	void *ptr = (void*)((((intptr_t)unaligned + sizeof(void*) + extraSize + 16 - 1) & ~(16 - 1)) - extraSize);
	memcpy((void**)ptr - 1, &unaligned, sizeof(unaligned));
	return ptr;
}

void NULLC::alignedDealloc(void* ptr)
{
	void* unaligned = NULL;
	memcpy(&unaligned, (void**)ptr - 1, sizeof(unaligned));
	dealloc(unaligned);
}

const char* NULLC::defaultFileLoad(const char* name, unsigned* size)
{
	assert(name);
	assert(size);

	FILE *file = fopen(name, "rb");
	if(file)
	{
		fseek(file, 0, SEEK_END);
		*size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char *fileContent = (char*)NULLC::alloc(*size + 1);
		unsigned read = (unsigned)fread(fileContent, 1, *size, file);

		if(read != *size)
		{
			NULLC::dealloc(fileContent);
			*size = 0;

			fclose(file);
			return NULL;
		}

		fileContent[*size] = 0;
		fclose(file);
		return fileContent;
	}
	*size = 0;
	return NULL;
}

void NULLC::defaultFileFree(const char* data)
{
	if(data)
		NULLC::dealloc((char*)data);
}

const char* (*NULLC::fileLoad)(const char*, unsigned*) = NULLC::defaultFileLoad;
void (*NULLC::fileFree)(const char*) = NULLC::defaultFileFree;

#ifndef WITH_STDCPP_LIB
/*
This code is to have an executable without libstd++ library dependency
g++ -g -Wall -fno-rtti -fno-exceptions  *.cpp -o YourParser
 */

// MSVC uses __cdecl calling convention for new/delete :-O
#ifdef _MSC_VER
#  define NEWDECL_CALL __cdecl
#else
#  define NEWDECL_CALL
#endif

extern "C" void __cxa_pure_virtual ()
{
    puts("__cxa_pure_virtual called\n");
    abort ();
}

void * NEWDECL_CALL operator new (size_t size)
{
    void *p = malloc (size);
    if(!p)
    {
        puts("not enough memory\n");
        abort ();
    }
    return p;
}

void * NEWDECL_CALL operator new [] (size_t size)
{
    return ::operator new(size);
}

void NEWDECL_CALL operator delete (void *p)
{
    if (p) free (p);
}

void NEWDECL_CALL operator delete [] (void *p)
{
    if (p) free (p);
}

void NEWDECL_CALL operator delete [] (void *p, size_t)
{
    if (p) free (p);
}

void NEWDECL_CALL operator delete (void *p, size_t)
{
    if (p) free (p);
}
#endif //WITH_STDCPP_LIB
