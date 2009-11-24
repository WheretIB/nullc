#pragma once
#include "nullcdef.h"

namespace NULLC
{
	void	Assert(int val);
	double	Cos(double deg);
	double	Sin(double deg);
	double	Tan(double deg);
	double	Ctg(double deg);

	double	Cosh(double deg);
	double	Sinh(double deg);
	double	Tanh(double deg);
	double	Coth(double deg);

	double	Acos(double deg);
	double	Asin(double deg);
	double	Atan(double deg);

	double	Ceil(double num);
	double	Floor(double num);
	double	Exp(double num);
	double	Log(double num);

	double	Sqrt(double num);

	int		StrEqual(NullCArray a, NullCArray b);
	int		StrNEqual(NullCArray a, NullCArray b);
	NullCArray	StrConcatenate(NullCArray a, NullCArray b);

	// Basic type constructors
	int			Int(int a);
	long long	Long(long long a);
	float		Float(float a);
	double		Double(double a);
	
}
