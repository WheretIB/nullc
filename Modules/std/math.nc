//double pi = 3.141592683;

// scalar functions
double	cos(double deg);
double	sin(double deg);
double	tan(double deg);
double	ctg(double deg);

double	cosh(double deg);
double	sinh(double deg);
double	tanh(double deg);
double	coth(double deg);

double	acos(double deg);
double	asin(double deg);
double	atan(double deg);

double	ceil(double num);
double	floor(double num);
double	exp(double num);
double	log(double num);

double	sqrt(double num);
	
double clamp(double val, min, max);
double saturate(double val);
double abs(double val);

// vector classes
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

float3 reflect(float3 normal, float3 dir)
{
	//return 1;
}

float ref operator[](float2 ref a, int index);
float ref operator[](float3 ref a, int index);
float ref operator[](float4 ref a, int index);

float float2:length();
float float2:normalize();

float float3:length();
float float3:normalize();

float float4:length();
float float4:normalize();

float dot(float2 a, float2 b);
float dot(float3 a, float3 b);
float dot(float4 a, float4 b);
