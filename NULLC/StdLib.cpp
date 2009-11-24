#include "StdLib.h"

#include "nullc.h"
#include <math.h>
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
	if(size >= minGlobalBlockSize)
	{
		globalObjects.push_back(new char[size]);
		return globalObjects.back();
	}
	return globalPool.Allocate(size);
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

double NULLC::Cos(double deg)
{
	return cos(deg);
}

double NULLC::Sin(double deg)
{
	return sin(deg);
}

double NULLC::Tan(double deg)
{
	return tan(deg);
}

double NULLC::Ctg(double deg)
{
	return 1.0 / tan(deg);
}


double NULLC::Cosh(double deg)
{
	return cosh(deg);
}

double NULLC::Sinh(double deg)
{
	return sinh(deg);
}

double NULLC::Tanh(double deg)
{
	return tanh(deg);
}

double NULLC::Coth(double deg)
{
	return 1.0 / tanh(deg);
}


double NULLC::Acos(double deg)
{
	return acos(deg);
}

double NULLC::Asin(double deg)
{
	return asin(deg);
}

double NULLC::Atan(double deg)
{
	return atan(deg);
}

double NULLC::Ceil(double num)
{
	return ceil(num);
}

double NULLC::Floor(double num)
{
	return floor(num);
}

double NULLC::Exp(double num)
{
	return exp(num);
}

double NULLC::Log(double num)
{
	return log(num);
}

double NULLC::Sqrt(double num)
{
	return sqrt(num);
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
