#include "StdLib.h"

#include "nullc.h"
#include <string.h>

#include "stdafx.h"
#include "Pool.h"

namespace NULLC
{
	const unsigned int poolBlockSize = 16 * 1024;
	const unsigned int minGlobalBlockSize = 8 * 1024;

	ChunkedStackPool<poolBlockSize>	globalPool;
	FastVector<void*>				globalObjects;
}

void* NULLC::AllocObject(int size)
{
	if((unsigned int)size >= minGlobalBlockSize)
	{
		globalObjects.push_back(new char[size]);
		memset(globalObjects.back(), 0, size);
		return globalObjects.back();
	}
	void *data = globalPool.Allocate(size);
	memset(data, 0, size);
	return data;
}

NullCArray NULLC::AllocArray(int size, int count)
{
	NullCArray ret;
	ret.ptr = (char*)AllocObject(count * size);
	ret.len = count;
	return ret;
}

void NULLC::ClearMemory()
{
	globalPool.Clear();
	for(unsigned int i = 0; i < globalObjects.size(); i++)
		delete (char*)globalObjects[i];
	globalObjects.clear();
}

void NULLC::ResetMemory()
{
	ClearMemory();
	globalPool.~ChunkedStackPool();
	globalObjects.reset();
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
