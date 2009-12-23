//double pi = 3.141592683;

align(4) class float2
{
	float x, y;
}
float2 float2(float x, y)
{
	float2 ret;
	ret.x = x;
	ret.y = y;
	return ret;
}
float2 operator+(float2 a, float2 b)
{
	return float2(a.x + b.x, a.y + b.y);
}
float2 operator-(float2 a, float2 b)
{
	return float2(a.x - b.x, a.y - b.y);
}
float2 operator*(float2 a, float2 b)
{
	return float2(a.x * b.x, a.y * b.y);
}
float2 operator/(float2 a, float2 b)
{
	return float2(a.x / b.x, a.y / b.y);
}
float2 operator*(float2 a, float b)
{
	return float2(a.x * b, a.y * b);
}
float2 operator*(float a, float2 b)
{
	return float2(a * b.x, a * b.y);
}
float2 operator/(float2 a, float b)
{
	return float2(a.x / b, a.y / b);
}
float dot(float2 a, float2 b)
{
	return a.x * b.x + a.y * b.y;
}
float ref operator[](float2 ref a, int index)
{
	if(index == 0)
		return &a.x;
	else if(index == 1)
		return &a.y;
	assert(0);
}

align(4) class float3
{
	float x, y, z;
}
float3 float3(float x, y, z)
{
	float3 ret;
	ret.x = x;
	ret.y = y;
	ret.z = z;
	return ret;
}
float3 operator+(float3 a, float3 b)
{
	return float3(a.x + b.x, a.y + b.y, a.z + b.z);
}
float3 operator-(float3 a, float3 b)
{
	return float3(a.x - b.x, a.y - b.y, a.z - b.z);
}
float3 operator*(float3 a, float3 b)
{
	return float3(a.x * b.x, a.y * b.y, a.z * b.z);
}
float3 operator/(float3 a, float3 b)
{
	return float3(a.x / b.x, a.y / b.y, a.z / b.z);
}
float3 operator*(float3 a, float b)
{
	return float3(a.x * b, a.y * b, a.z * b);
}
float3 operator*(float a, float3 b)
{
	return float3(a * b.x, a * b.y, a * b.z);
}
float3 operator/(float3 a, float b)
{
	return float3(a.x / b, a.y / b, a.z / b);
}
float dot(float3 a, float3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
float ref operator[](float3 ref a, int index)
{
	if(index == 0)
		return &a.x;
	else if(index == 1)
		return &a.y;
	else if(index == 2)
		return &a.z;
	assert(0);
}

align(4) class float4
{
	float x, y, z, w;
}
float4 float4(float x, y, z, w)
{
	float4 ret;
	ret.x = x;
	ret.y = y;
	ret.z = z;
	ret.w = w;
	return ret;
}
float4 operator+(float4 a, float4 b)
{
	return float4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}
float4 operator-(float4 a, float4 b)
{
	return float4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}
float4 operator*(float4 a, float4 b)
{
	return float4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}
float4 operator/(float4 a, float4 b)
{
	return float4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}
float4 operator*(float4 a, float b)
{
	return float4(a.x * b, a.y * b, a.z * b, a.w * b);
}
float4 operator*(float a, float4 b)
{
	return float4(a * b.x, a * b.y, a * b.z, a * b.w);
}
float4 operator/(float4 a, float b)
{
	return float4(a.x / b, a.y / b, a.z / b, a.w / b);
}
float dot(float4 a, float4 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
float ref operator[](float4 ref a, int index)
{
	if(index == 0)
		return &a.x;
	else if(index == 1)
		return &a.y;
	else if(index == 2)
		return &a.z;
	else if(index == 3)
		return &a.w;
	assert(0);
}

float3 float3(float2 xy, float z)
{
	return float3(xy.x, xy.y, z);
}
float3 float3(float x, float2 yz)
{
	return float3(x, yz.x, yz.y);
}

float4 float4(float3 xyz, float w)
{
	return float4(xyz.x, xyz.y, xyz.z, w);
}
float4 float4(float2 xy, float2 zw)
{
	return float4(xy.x, xy.y, zw.x, zw.y);
}
float4 float4(float2 xy, float z, w)
{
	return float4(xy.x, xy.y, z, w);
}
float4 float4(float x, y, float2 zw)
{
	return float4(x, y, zw.x, zw.y);
}
float4 float4(float x, float3 yzw)
{
	return float4(x, yzw.x, yzw.y, yzw.z);
}

align(4) class float4x4
{
	float4 row1, row2, row3, row4;
}
