#include "StdLib.h"

#include "nullc.h"
#include <string.h>

#include "stdafx.h"
#include "Pool.h"

#include "Executor_Common.h"

namespace NULLC
{
	const unsigned int poolBlockSize = 16 * 1024;
	const unsigned int minGlobalBlockSize = 8 * 1024;

	ChunkedStackPool<poolBlockSize>	globalPool;
	FastVector<void*>				globalObjects;
	FastVector<void*>				objectsFromPool;

	unsigned int defaultMark = 42;

	unsigned int usedMemory = 0;

	unsigned int collectableMinimum = 32 * 1024;
	unsigned int globalMemoryLimit = 32 * 1024 * 1024;
}

void* NULLC::AllocObject(int size)
{
#ifdef ENABLE_GC
	usedMemory += size;
	if((unsigned int)size > collectableMinimum)
	{
		CollectMemory();
	}else if((unsigned int)size > globalMemoryLimit){
		nullcThrowError("Reached global memory maximum");
		return NULL;
	}
#endif
	if((unsigned int)size >= minGlobalBlockSize)
	{
		globalObjects.push_back(new char[size + 4]);
		memset(globalObjects.back(), 0, size + 4);
		*(int*)globalObjects.back() = defaultMark;
		return (char*)globalObjects.back() + 4;
	}
	char *data = (char*)globalPool.Allocate(size + 4);
	objectsFromPool.push_back(data);
	memset(data, 0, size + 4);
	*(int*)data = defaultMark;
	return data + 4;
}

NullCArray NULLC::AllocArray(int size, int count)
{
	NullCArray ret;
	ret.ptr = (char*)AllocObject(count * size);
	ret.len = count;
	return ret;
}

void NULLC::MarkMemory(unsigned int number)
{
	for(unsigned int i = 0; i < globalObjects.size(); i++)
		*(int*)globalObjects[i] = number;
	for(unsigned int i = 0; i < objectsFromPool.size(); i++)
		*(int*)objectsFromPool[i] = number;
}

void NULLC::SweepMemory(unsigned int number)
{
	unsigned int unusedBlocks = 0;
	for(unsigned int i = 0; i < globalObjects.size(); i++)
		if(*(unsigned int*)globalObjects[i] == number)
			unusedBlocks++;
	printf("%d unused globally allocated blocks\r\n", unusedBlocks);
	unusedBlocks = 0;
	for(unsigned int i = 0; i < objectsFromPool.size(); i++)
		if(*(unsigned int*)objectsFromPool[i] == number)
			unusedBlocks++;
	printf("%d unused pool allocated blocks\r\n", unusedBlocks);
}

#ifdef ENABLE_GC
void NULLC::CollectMemory()
{
	// All memory blocks are marked with 0
	MarkMemory(0);
	// Used memory blocks are marked with 1
	MarkUsedBlocks(1);

	// Globally allocated objects marked with 0 are deleted
	unsigned int unusedBlocks = 0;
	for(unsigned int i = 0; i < globalObjects.size(); i++)
	{
		if(*(unsigned int*)globalObjects[i] == 0)
		{
			delete (char*)globalObjects[i];
			globalObjects[i] = globalObjects.back();
			globalObjects.pop_back();
			unusedBlocks++;
		}
	}
	printf("%d unused globally allocated blocks destroyed (%d remains)\r\n", unusedBlocks, globalObjects.size());

	// Memory allocated from pool is defragmented
}
#endif

void NULLC::ClearMemory()
{
	NULLC::usedMemory = 0;

	globalPool.Clear();
	for(unsigned int i = 0; i < globalObjects.size(); i++)
		delete (char*)globalObjects[i];
	globalObjects.clear();
	objectsFromPool.clear();
}

void NULLC::ResetMemory()
{
	ClearMemory();
	globalPool.~ChunkedStackPool();
	globalObjects.reset();
	objectsFromPool.reset();
}

void NULLC::Assert(int val)
{
	if(!val)
		nullcThrowError("Assertion failed");
}

int NULLC::StrEqual(NullCArray a, NullCArray b)
{
	if(a.len != b.len)
		return 0;
	for(unsigned int i = 0; i < a.len; i++)
		if(a.ptr[i] != b.ptr[i])
			return 0;
	return 1;
}

int NULLC::StrNEqual(NullCArray a, NullCArray b)
{
	return !StrEqual(a, b);
}

NullCArray NULLC::StrConcatenate(NullCArray a, NullCArray b)
{
	NullCArray ret;

	ret.len = a.len + b.len - 1;
	ret.ptr = (char*)AllocObject(ret.len);

	memcpy(ret.ptr, a.ptr, a.len);
	memcpy(ret.ptr + a.len - 1, b.ptr, b.len);

	return ret;
}

NullCArray NULLC::StrConcatenateAndSet(NullCArray *a, NullCArray b)
{
	return *a = StrConcatenate(*a, b);
}

int NULLC::Int(int a)
{
	return a;
}

long long NULLC::Long(long long a)
{
	return a;

}
float NULLC::Float(float a)
{
	return a;
}

double NULLC::Double(double a)
{
	return a;
}

NullCArray NULLC::IntToStr(int* r)
{
	int number = *r;
	char buf[16];
	char *curr = buf;
	*curr++ = (char)(number % 10 + '0');
	while(number /= 10)
		*curr++ = (char)(number % 10 + '0');
	NullCArray arr = AllocArray(1, (int)(curr - buf) + 1);
	char *str = arr.ptr;
	do 
	{
		--curr;
		*str++ = *curr;
	}while(curr != buf);
	return arr;
}
