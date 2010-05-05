//#include "std_math.cpp"
#include "runtime.h"
#include <math.h>

typedef struct
{
	float x;
	float y;
} float2;
typedef struct
{
	float x;
	float y;
	float z;
} float3;
typedef struct
{
	float x;
	float y;
	float z;
	float w;
} float4;

double cos(double deg, void* unused)
{
	return cos(deg);
}
double sin(double deg, void* unused)
{
	return sin(deg);
}
double tan(double deg, void* unused)
{
	return tan(deg);
}
double ctg(double deg, void* unused)
{
	return 1.0 / tan(deg);
}

double cosh(double deg, void* unused)
{
	return cosh(deg);
}
double sinh(double deg, void* unused)
{
	return sinh(deg);
}
double tanh(double deg, void* unused)
{
	return tanh(deg);
}
double coth(double deg, void* unused)
{
	return 1.0 / tanh(deg);
}
double acos(double deg, void* unused)
{
	return acos(deg);
}
double asin(double deg, void* unused)
{
	return asin(deg);
}
double atan(double deg, void* unused)
{
	return atan(deg);
}
double ceil(double num, void* unused)
{
	return ceil(num);
}
double floor(double num, void* unused)
{
	return floor(num);
}
double exp(double num, void* unused)
{
	return exp(num);
}
double log(double num, void* unused)
{
	return log(num);
}
double sqrt(double num, void* unused)
{
	return sqrt(num);
}

double clamp(double val, double min, double max, void* unused)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

double saturate(double val, void* unused)
{
	if(val < 0.0)
		return 0.0;
	if(val > 1.0)
		return 1.0;
	return val;
}

double abs(double val, void* unused)
{
	if(val < 0.0)
		return -val;
	return val;
}

float * __operatorIndex(float2 * a, int index, void* unused)
{
	if((unsigned)index >= 2)
		nullcThrowError("Array index out of bounds");
	return (float*)&a->x + index;
}

float * __operatorIndex(float3 * a, int index, void* unused)
{
	if((unsigned)index >= 3)
		nullcThrowError("Array index out of bounds");
	return (float*)&a->x + index;
}

float * __operatorIndex(float4 * a, int index, void* unused)
{
	if((unsigned)index >= 4)
		nullcThrowError("Array index out of bounds");
	return (float*)&a->x + index;
}

float float2__length(float2 * v)
{
	return sqrt(v->x * v->x + v->y * v->y);
}
float float2__normalize(float2 * v)
{
	float len = float2__length(v), invLen = 1.0f / len;
	v->x *= invLen;
	v->y *= invLen;
	return len;
}

float float3__length(float3 * v)
{
	return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}
float float3__normalize(float3 * v)
{
	float len = float3__length(v), invLen = 1.0f / len;
	v->x *= invLen;
	v->y *= invLen;
	v->z *= invLen;
	return len;
}

float float4__length(float3 * v)
{
	return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}
float float4__normalize(float3 * v)
{
	float len = float4__length(v), invLen = 1.0f / len;
	v->x *= invLen;
	v->y *= invLen;
	v->z *= invLen;
	return len;
}

float dot(float2 * a, float2 * b, void* unused)
{
	return a->x*b->x + a->y*b->y;
}
float dot(float3 * a, float3 * b, void* unused)
{
	return a->x*b->x + a->y*b->y + a->z*b->z;
}
float dot(float4 * a, float4 * b, void* unused)
{
	return a->x*b->x + a->y*b->y + a->z*b->z + a->w*b->w;
}

