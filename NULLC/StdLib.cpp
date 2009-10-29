#include "StdLib.h"

#include "nullc.h"
#include <math.h>

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
