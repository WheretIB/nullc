#include "math.h"
#include "../../NULLC/nullc.h"

#include <math.h>

namespace NULLCMath
{
	double Cos(double deg)
	{
		return cos(deg);
	}

	double Sin(double deg)
	{
		return sin(deg);
	}

	double Tan(double deg)
	{
		return tan(deg);
	}

	double Ctg(double deg)
	{
		return 1.0 / tan(deg);
	}


	double Cosh(double deg)
	{
		return cosh(deg);
	}

	double Sinh(double deg)
	{
		return sinh(deg);
	}

	double Tanh(double deg)
	{
		return tanh(deg);
	}

	double Coth(double deg)
	{
		return 1.0 / tanh(deg);
	}


	double Acos(double deg)
	{
		return acos(deg);
	}

	double Asin(double deg)
	{
		return asin(deg);
	}

	double Atan(double deg)
	{
		return atan(deg);
	}

	double Ceil(double num)
	{
		return ceil(num);
	}

	double Floor(double num)
	{
		return floor(num);
	}

	double Exp(double num)
	{
		return exp(num);
	}

	double Log(double num)
	{
		return log(num);
	}

	double Sqrt(double num)
	{
		return sqrt(num);
	}

	double clamp(double val, double min, double max)
	{
		if(val < min)
			return min;
		if(val > max)
			return max;
		return val;
	}
	double saturate(double val)
	{
		if(val < 0.0)
			return 0.0;
		if(val > 1.0)
			return 1.0;
		return val;
	}
	double abs(double val)
	{
		if(val < 0.0)
			return -val;
		return val;
	}

	struct float2
	{
		float x, y;
	};
	struct float3
	{
		float x, y, z;
	};
	struct float4
	{
		float x, y, z, w;
	};
	float* operatorIndex2(float2* a, unsigned int index)
	{
		if(index >= 2)
			nullcThrowError("Array index out of bounds");
		return (float*)&a->x + index;
	}
	float* operatorIndex3(float3* a, unsigned int index)
	{
		if(index >= 3)
			nullcThrowError("Array index out of bounds");
		return (float*)&a->x + index;
	}
	float* operatorIndex4(float4* a, unsigned int index)
	{
		if(index >= 4)
			nullcThrowError("Array index out of bounds");
		return (float*)&a->x + index;
	}

	float length2(float4* v)
	{
		return sqrt(v->x * v->x + v->y * v->y);
	}
	float normalize2(float4* v)
	{
		float len = length2(v), invLen = 1.0f / len;
		v->x *= invLen;
		v->y *= invLen;
		return len;
	}

	float length3(float4* v)
	{
		return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
	}
	float normalize3(float4* v)
	{
		float len = length3(v), invLen = 1.0f / len;
		v->x *= invLen;
		v->y *= invLen;
		v->z *= invLen;
		return len;
	}

	float dot2(float2 *a, float2 *b)
	{
		return a->x*b->x + a->y*b->y;
	}
	float dot3(float3 *a, float3 *b)
	{
		return a->x*b->x + a->y*b->y + a->z*b->z;
	}
	float dot4(float4 *a, float4 *b)
	{
		return a->x*b->x + a->y*b->y + a->z*b->z + a->w*b->w;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("std.math", (void(*)())NULLCMath::funcPtr, name, index)) return false;
bool	nullcInitMathModule()
{
	REGISTER_FUNC(Cos, "cos", 0);
	REGISTER_FUNC(Sin, "sin", 0);
	REGISTER_FUNC(Tan, "tan", 0);
	REGISTER_FUNC(Ctg, "ctg", 0);

	REGISTER_FUNC(Cosh, "cosh", 0);
	REGISTER_FUNC(Sinh, "sinh", 0);
	REGISTER_FUNC(Tanh, "tanh", 0);
	REGISTER_FUNC(Coth, "coth", 0);

	REGISTER_FUNC(Acos, "acos", 0);
	REGISTER_FUNC(Asin, "asin", 0);
	REGISTER_FUNC(Atan, "atan", 0);

	REGISTER_FUNC(Ceil, "ceil", 0);
	REGISTER_FUNC(Floor, "floor", 0);
	REGISTER_FUNC(Exp, "exp", 0);
	REGISTER_FUNC(Log, "log", 0);

	REGISTER_FUNC(Sqrt, "sqrt", 0);

	REGISTER_FUNC(clamp, "clamp", 0);
	REGISTER_FUNC(saturate, "saturate", 0);
	REGISTER_FUNC(abs, "abs", 0);

	REGISTER_FUNC(operatorIndex2, "[]", 0);
	REGISTER_FUNC(operatorIndex3, "[]", 1);
	REGISTER_FUNC(operatorIndex4, "[]", 2);

	REGISTER_FUNC(length2, "float2::length", 0);
	REGISTER_FUNC(normalize2, "float2::normalize", 0);

	REGISTER_FUNC(length3, "float3::length", 0);
	REGISTER_FUNC(normalize3, "float3::normalize", 0);

	REGISTER_FUNC(length3, "float4::length", 0);
	REGISTER_FUNC(normalize3, "float4::normalize", 0);

	REGISTER_FUNC(dot2, "dot", 0);
	REGISTER_FUNC(dot3, "dot", 1);
	REGISTER_FUNC(dot4, "dot", 2);
	return true;
}
