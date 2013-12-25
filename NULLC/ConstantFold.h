#pragma once
#include "InstructionSet.h"

template<typename T>
static void	Swap(T& a, T& b)
{
	T temp = a;
	a = b;
	b = temp;
}

// Functions to apply binary operations in compile-time
template<typename T>
inline T optDoOperation(CmdID cmd, T a, T b)
{
	if(cmd == cmdAdd)
		return a + b;
	if(cmd == cmdSub)
		return a - b;
	if(cmd == cmdMul)
		return a * b;
	if(cmd == cmdLess)
		return a < b;
	if(cmd == cmdGreater)
		return a > b;
	if(cmd == cmdGEqual)
		return a >= b;
	if(cmd == cmdLEqual)
		return a <= b;
	if(cmd == cmdEqual)
		return a == b;
	if(cmd == cmdNEqual)
		return a != b;
	return optDoSpecial(cmd, a, b);
}
template<typename T>
inline T optDoSpecial(CmdID cmd, T a, T b)
{
	(void)cmd; (void)b; (void)a;	// C4100
	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: optDoSpecial call with unknown type");
	return 0;
}
template<>
inline int optDoSpecial<>(CmdID cmd, int a, int b)
{
	if(cmd == cmdDiv)
	{
		if(b == 0)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: division by zero during constant folding");
		return a / b;
	}
	if(cmd == cmdPow)
	{
		if(b < 0)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: negative power on integer number in exponentiation during constant folding");

		int result = 1;
		int power = (int)b;
		while(power)
		{
			if(power & 1)
			{
				result *= a;
				power--;
			}
			a *= a;
			power >>= 1;
		}
		return result;
	}
	if(cmd == cmdShl)
		return a << b;
	if(cmd == cmdShr)
		return a >> b;
	if(cmd == cmdMod)
	{
		if(b == 0)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: modulus division by zero during constant folding");
		return a % b;
	}
	if(cmd == cmdBitAnd)
		return a & b;
	if(cmd == cmdBitXor)
		return a ^ b;
	if(cmd == cmdBitOr)
		return a | b;
	if(cmd == cmdLogAnd)
		return a && b;
	if(cmd == cmdLogXor)
		return !!a ^ !!b;
	if(cmd == cmdLogOr)
		return a || b;
	assert(!"optDoSpecial<int> with unknown command");
	return 0;
}
template<>
inline long long optDoSpecial<>(CmdID cmd, long long a, long long b)
{
	if(cmd == cmdDiv)
	{
		if(b == 0)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: division by zero during constant folding");
		return a / b;
	}
	if(cmd == cmdPow)
	{
		if(b < 0)
			return (a == 1 ? 1 : 0);
		if(b == 0)
			return 1;
		if(b == 1)
			return a;
		if(b > 64)
			return a;
		long long res = 1;
		int power = (int)b;
		while(power)
		{
			if(power & 0x01)
			{
				res *= a;
				power--;
			}
			a *= a;
			power >>= 1;
		}
		return res;
	}
	if(cmd == cmdShl)
		return a << b;
	if(cmd == cmdShr)
		return a >> b;
	if(cmd == cmdMod)
	{
		if(b == 0)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: modulus division by zero during constant folding");
		return a % b;
	}
	if(cmd == cmdBitAnd)
		return a & b;
	if(cmd == cmdBitXor)
		return a ^ b;
	if(cmd == cmdBitOr)
		return a | b;
	if(cmd == cmdLogAnd)
		return a && b;
	if(cmd == cmdLogXor)
		return !!a ^ !!b;
	if(cmd == cmdLogOr)
		return a || b;
	assert(!"optDoSpecial<long long> with unknown command");
	return 0;
}
template<>
inline double optDoSpecial<>(CmdID cmd, double a, double b)
{
	if(cmd == cmdDiv)
		return a / b;
	if(cmd == cmdMod)
		return fmod(a,b);
	if(cmd == cmdPow)
		return pow(a, b);
	if(cmd == cmdShl)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: << is illegal for floating-point numbers");
	if(cmd == cmdShr)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: >> is illegal for floating-point numbers");
	if(cmd == cmdBitAnd)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: & is illegal for floating-point numbers");
	if(cmd == cmdBitOr)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: | is illegal for floating-point numbers");
	if(cmd == cmdBitXor)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: ^ is illegal for floating-point numbers");
	if(cmd == cmdLogAnd)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: && is illegal for floating-point numbers");
	if(cmd == cmdLogXor)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: ^^ is illegal for floating-point numbers");
	if(cmd == cmdLogOr)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: || is illegal for floating-point numbers");
	assert(!"optDoSpecial<double> with unknown command");
	return 0.0;
}
