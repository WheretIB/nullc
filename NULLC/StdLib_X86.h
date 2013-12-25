#pragma once

#ifdef NULLC_BUILD_X86_JIT
// Implementations of some functions, called from user binary code

int intPow(int power, int number)
{
	if(power < 0)
		return number == 1 ? 1 : (number == -1 ? (power & 1 ? -1 : 1) : 0);

	int result = 1;
	while(power)
	{
		if(power & 1)
		{
			result *= number;
			power--;
		}
		number *= number;
		power >>= 1;
	}
	return result;
}

double doublePow(double a, double b)
{
	return pow(b, a);
}

double doubleMod(double a, double b)
{
	return fmod(b, a);
}

long long longMul(long long a, long long b)
{
	return b * a;
}

long long longDiv(long long a, long long b)
{
	return b / a;
}

long long longMod(long long a, long long b)
{
	return b % a;
}

long long longShl(long long a, long long b)
{
	return b << a;
}

long long longShr(long long a, long long b)
{
	return b >> a;
}

long long longPow(long long power, long long number)
{
	if(power < 0)
		return number == 1 ? 1 : (number == -1 ? (power & 1 ? -1 : 1) : 0);

	long long result = 1;
	while(power)
	{
		if(power & 1)
		{
			result *= number;
			power--;
		}
		number *= number;
		power >>= 1;
	}
	return result;
}

#endif
