#include "runtime.h"

#include <math.h>

struct float2
{
	float x;
	float y;
};
struct float3
{
	float x;
	float y;
	float z;
};
struct float4
{
	float x;
	float y;
	float z;
	float w;
};

double cos_double_ref_double_(double deg, void* __context)
{
	return cos(deg);
}

double sin_double_ref_double_(double deg, void* __context)
{
	return sin(deg);
}

double tan_double_ref_double_(double deg, void* __context)
{
	return tan(deg);
}

double ctg_double_ref_double_(double deg, void* __context)
{
	return 1.0 / tan(deg);
}

double cosh_double_ref_double_(double deg, void* __context)
{
	return cosh(deg);
}

double sinh_double_ref_double_(double deg, void* __context)
{
	return sinh(deg);
}

double tanh_double_ref_double_(double deg, void* __context)
{
	return tanh(deg);
}

double coth_double_ref_double_(double deg, void* __context)
{
	return 1.0 / tanh(deg);
}

double acos_double_ref_double_(double deg, void* __context)
{
	return acos(deg);
}

double asin_double_ref_double_(double deg, void* __context)
{
	return asin(deg);
}

double atan_double_ref_double_(double deg, void* __context)
{
	return atan(deg);
}

double atan2_double_ref_double_double_(double y, double x, void* __context)
{
	return atan2(y, x);
}

double ceil_double_ref_double_(double num, void* __context)
{
	return ceil(num);
}

double floor_double_ref_double_(double num, void* __context)
{
	return floor(num);
}

double exp_double_ref_double_(double num, void* __context)
{
	return exp(num);
}

double log_double_ref_double_(double num, void* __context)
{
	return log(num);
}

double sqrt_double_ref_double_(double num, void* __context)
{
	return sqrt(num);
}

double clamp_double_ref_double_double_double_(double val, double min, double max, void* __context)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

double saturate_double_ref_double_(double val, void* __context)
{
	if(val < 0.0)
		return 0.0;
	if(val > 1.0)
		return 1.0;
	return val;
}

double abs_double_ref_double_(double val, void* __context)
{
	if(val < 0.0)
		return -val;
	return val;
}

float * __operatorIndex_float_ref_ref_float2_ref_int_(float2 * a, int index, void* __context)
{
	if((unsigned)index >= 2)
		nullcThrowError("Array index out of bounds");
	return (float*)&a->x + index;
}

float * __operatorIndex_float_ref_ref_float3_ref_int_(float3 * a, int index, void* __context)
{
	if((unsigned)index >= 3)
		nullcThrowError("Array index out of bounds");
	return (float*)&a->x + index;
}

float * __operatorIndex_float_ref_ref_float4_ref_int_(float4 * a, int index, void* __context)
{
	if((unsigned)index >= 4)
		nullcThrowError("Array index out of bounds");
	return (float*)&a->x + index;
}

float float2__length_float_ref__(float2 * v)
{
	return sqrt(v->x * v->x + v->y * v->y);
}
float float2__normalize_float_ref__(float2 * v)
{
	float len = float2__length_float_ref__(v), invLen = 1.0f / len;
	v->x *= invLen;
	v->y *= invLen;
	return len;
}

float float3__length_float_ref__(float3 * v)
{
	return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}
float float3__normalize_float_ref__(float3 * v)
{
	float len = float3__length_float_ref__(v), invLen = 1.0f / len;
	v->x *= invLen;
	v->y *= invLen;
	v->z *= invLen;
	return len;
}

float float4__length_float_ref__(float4 * v)
{
	return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}
float float4__normalize_float_ref__(float4 * v)
{
	float len = float4__length_float_ref__(v), invLen = 1.0f / len;
	v->x *= invLen;
	v->y *= invLen;
	v->z *= invLen;
	return len;
}

float dot_float_ref_float2_ref_float2_ref_(float2 * a, float2 * b, void* __context)
{
	return a->x*b->x + a->y*b->y;
}
float dot_float_ref_float3_ref_float3_ref_(float3 * a, float3 * b, void* __context)
{
	return a->x*b->x + a->y*b->y + a->z*b->z;
}
float dot_float_ref_float4_ref_float4_ref_(float4 * a, float4 * b, void* __context)
{
	return a->x*b->x + a->y*b->y + a->z*b->z + a->w*b->w;
}

