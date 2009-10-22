#include "StdLib.h"

#include "nullc.h"
#include <math.h>

void nullcAssert(int val)
{
	if(!val)
		nullcThrowError("Assertion failed");
}

double nullcCos(double deg)
{
	return cos(deg);
}

double nullcSin(double deg)
{
	return sin(deg);
}

double nullcTan(double deg)
{
	return tan(deg);
}

double nullcCtg(double deg)
{
	return 1.0 / tan(deg);
}


double nullcCosh(double deg)
{
	return cosh(deg);
}

double nullcSinh(double deg)
{
	return sinh(deg);
}

double nullcTanh(double deg)
{
	return tanh(deg);
}

double nullcCoth(double deg)
{
	return 1.0 / tanh(deg);
}


double nullcAcos(double deg)
{
	return acos(deg);
}

double nullcAsin(double deg)
{
	return asin(deg);
}

double nullcAtan(double deg)
{
	return atan(deg);
}

double nullcCeil(double num)
{
	return ceil(num);
}

double nullcFloor(double num)
{
	return floor(num);
}

double nullcExp(double num)
{
	return exp(num);
}

double nullcLog(double num)
{
	return log(num);
}

double nullcSqrt(double num)
{
	return sqrt(num);
}

int strEqual(NullCArray a, NullCArray b)
{
	if(a.len != b.len)
		return 0;
	for(unsigned int i = 0; i < a.len; i++)
		if(a.ptr[i] != b.ptr[i])
			return 0;
	return 1;
}

int strNEqual(NullCArray a, NullCArray b)
{
	return !strEqual(a, b);
}
