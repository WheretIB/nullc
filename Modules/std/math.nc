// Constants
class MathConstants
{
	double pi;
	double e;
}
MathConstants math;
math.pi = 3.14159265358979323846;
math.e = 2.71828182845904523536;

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
double	atan2(double y, x);

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
	
	void float2(float x, y)
	{
		this.x = x;
		this.y = y;
	}
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
float2 ref operator +=(float2 ref a, float2 b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}
float2 ref operator -=(float2 ref a, float2 b)
{
	a.x -= b.x;
	a.y -= b.y;
	return a;
}
float2 ref operator *=(float2 ref a, float b)
{
	a.x *= b;
	a.y *= b;
	return a;
}
float2 ref operator /=(float2 ref a, float b)
{
	a.x /= b;
	a.y /= b;
	return a;
}

align(4) class float3
{
	float x, y, z;
	
	void float3(float x, y, z)
	{
		this.x = x;
		this.y = y;
		this.z = z;
	}
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
float3 ref operator +=(float3 ref a, float3 b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	return a;
}
float3 ref operator -=(float3 ref a, float3 b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	return a;
}
float3 ref operator *=(float3 ref a, float b)
{
	a.x *= b;
	a.y *= b;
	a.z *= b;
	return a;
}
float3 ref operator /=(float3 ref a, float b)
{
	a.x /= b;
	a.y /= b;
	a.z /= b;
	return a;
}

align(4) class float4
{
	float x, y, z, w;
	
	void float4(float x, y, z, w)
	{
		this.x = x;
		this.y = y;
		this.z = z;
		this.w = w;
	}
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
float4 ref operator +=(float4 ref a, float4 b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	a.w += b.w;
	return a;
}
float4 ref operator -=(float4 ref a, float4 b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	a.w -= b.w;
	return a;
}
float4 ref operator *=(float4 ref a, float b)
{
	a.x *= b;
	a.y *= b;
	a.z *= b;
	a.w *= b;
	return a;
}
float4 ref operator /=(float4 ref a, float b)
{
	a.x /= b;
	a.y /= b;
	a.z /= b;
	a.w /= b;
	return a;
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
float4 ref operator=(float4 ref a, double[4] xyzw)
{
	*a = float4(xyzw[0], xyzw[1], xyzw[2], xyzw[3]);
	return a;
}

align(4) class float4x4
{
	float4 row1, row2, row3, row4;
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

float dot(float2 ref a, float2 ref b);
float dot(float3 ref a, float3 ref b);
float dot(float4 ref a, float4 ref b);

float3 reflect(float3 normal, float3 dir)
{
	return dir - 2.0 * dot(normal, dir) * normal;
}
