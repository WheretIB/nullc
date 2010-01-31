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
	
	float2 float2(float x, y)
	{
		float2 res;
		res.x = x;
		res.y = y;
		return res;
	}

	float2 xx{ get{ return float2(x, x); } };
	float2 xy{ get{ return float2(x, y); } set{ x = r.x; y = r.y; } };
	float2 yx{ get{ return float2(y, x); } set{ y = r.x; x = r.y; } };
	float2 yy{ get{ return float2(y, y); } };
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
	
	float3 float3(float x, y, z)
	{
		float3 res;
		res.x = x;
		res.y = y;
		res.z = z;
		return res;
	}

	float2 xx{ get{ return float2(x, x); } };
	float2 xy{ get{ return float2(x, y); } set{ x = r.x; y = r.y; } };
	float2 xz{ get{ return float2(x, z); } set{ x = r.x; z = r.y; } };
	float2 yx{ get{ return float2(y, x); } set{ y = r.x; x = r.y; } };
	float2 yy{ get{ return float2(y, y); } };
	float2 yz{ get{ return float2(y, z); } set{ y = r.x; z = r.y; } };
	float2 zx{ get{ return float2(z, x); } set{ z = r.x; x = r.y; } };
	float2 zy{ get{ return float2(z, y); } set{ z = r.x; y = r.y; } };
	float2 zz{ get{ return float2(z, z); } };
	
	float3 xxx{ get{ return float3(x, x, x); } };
	float3 xxy{ get{ return float3(x, x, y); } };
	float3 xxz{ get{ return float3(x, x, z); } };
	float3 xyx{ get{ return float3(x, y, x); } };
	float3 xyy{ get{ return float3(x, y, y); } };
	float3 xyz{ get{ return float3(x, y, z); } set{ x = r.x; y = r.y; z = r.z; } };
	float3 xzx{ get{ return float3(x, z, x); } };
	float3 xzy{ get{ return float3(x, z, y); } set{ x = r.x; z = r.y; y = r.z; } };
	float3 xzz{ get{ return float3(x, z, z); } };
	float3 yxx{ get{ return float3(y, x, x); } };
	float3 yxy{ get{ return float3(y, x, y); } };
	float3 yxz{ get{ return float3(y, x, z); } set{ y = r.x; x = r.y; z = r.z; } };
	float3 yyx{ get{ return float3(y, y, x); } };
	float3 yyy{ get{ return float3(y, y, y); } };
	float3 yyz{ get{ return float3(y, y, z); } };
	float3 yzx{ get{ return float3(y, z, x); } set{ y = r.x; z = r.y; x = r.z; } };
	float3 yzy{ get{ return float3(y, z, y); } };
	float3 yzz{ get{ return float3(y, z, z); } };
	float3 zxx{ get{ return float3(z, x, x); } };
	float3 zxy{ get{ return float3(z, x, y); } set{ z = r.x; x = r.y; y = r.z; } };
	float3 zxz{ get{ return float3(z, x, z); } };
	float3 zyx{ get{ return float3(z, y, x); } set{ z = r.x; y = r.y; x = r.z; } };
	float3 zyy{ get{ return float3(z, y, y); } };
	float3 zyz{ get{ return float3(z, y, z); } };
	float3 zzx{ get{ return float3(z, z, x); } };
	float3 zzy{ get{ return float3(z, z, y); } };
	float3 zzz{ get{ return float3(z, z, z); } };
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
	
	float4 float4(float x, y, z, w)
	{
		float4 res;
		res.x = x;
		res.y = y;
		res.z = z;
		res.w = w;
		return res;
	}

	float2 xx{ get{ return float2(x, x); } };
	float2 xy{ get{ return float2(x, y); } set{ x = r.x; y = r.y; } };
	float2 xz{ get{ return float2(x, z); } set{ x = r.x; z = r.y; } };
	float2 xw{ get{ return float2(x, w); } set{ x = r.x; w = r.y; } };
	float2 yx{ get{ return float2(y, x); } set{ y = r.x; x = r.y; } };
	float2 yy{ get{ return float2(y, y); } };
	float2 yz{ get{ return float2(y, z); } set{ y = r.x; z = r.y; } };
	float2 yw{ get{ return float2(y, w); } set{ y = r.x; w = r.y; } };
	float2 zx{ get{ return float2(z, x); } set{ z = r.x; x = r.y; } };
	float2 zy{ get{ return float2(z, y); } set{ z = r.x; y = r.y; } };
	float2 zz{ get{ return float2(z, z); } };
	float2 zw{ get{ return float2(z, w); } set{ z = r.x; w = r.y; } };
	float2 wx{ get{ return float2(w, x); } set{ w = r.x; x = r.y; } };
	float2 wy{ get{ return float2(w, y); } set{ w = r.x; y = r.y; } };
	float2 wz{ get{ return float2(w, z); } set{ w = r.x; z = r.y; } };
	float2 ww{ get{ return float2(w, w); } };
	
	float3 xxx{ get{ return float3(x, x, x); } };
	float3 xxy{ get{ return float3(x, x, y); } };
	float3 xxz{ get{ return float3(x, x, z); } };
	float3 xxw{ get{ return float3(x, x, w); } };
	float3 xyx{ get{ return float3(x, y, x); } };
	float3 xyy{ get{ return float3(x, y, y); } };
	float3 xyz{ get{ return float3(x, y, z); } set{ x = r.x; y = r.y; z = r.z; } };
	float3 xyw{ get{ return float3(x, y, w); } set{ x = r.x; y = r.y; w = r.z; } };
	float3 xzx{ get{ return float3(x, z, x); } };
	float3 xzy{ get{ return float3(x, z, y); } set{ x = r.x; z = r.y; y = r.z; } };
	float3 xzz{ get{ return float3(x, z, z); } };
	float3 xzw{ get{ return float3(x, z, w); } set{ x = r.x; z = r.y; w = r.z; } };
	float3 xwx{ get{ return float3(x, w, x); } };
	float3 xwy{ get{ return float3(x, w, y); } set{ x = r.x; w = r.y; y = r.z; } };
	float3 xwz{ get{ return float3(x, w, z); } set{ x = r.x; w = r.y; z = r.z; } };
	float3 xww{ get{ return float3(x, w, w); } };
	float3 yxx{ get{ return float3(y, x, x); } };
	float3 yxy{ get{ return float3(y, x, y); } };
	float3 yxz{ get{ return float3(y, x, z); } set{ y = r.x; x = r.y; z = r.z; } };
	float3 yxw{ get{ return float3(y, x, w); } set{ y = r.x; x = r.y; w = r.z; } };
	float3 yyx{ get{ return float3(y, y, x); } };
	float3 yyy{ get{ return float3(y, y, y); } };
	float3 yyz{ get{ return float3(y, y, z); } };
	float3 yyw{ get{ return float3(y, y, w); } };
	float3 yzx{ get{ return float3(y, z, x); } set{ y = r.x; z = r.y; x = r.z; } };
	float3 yzy{ get{ return float3(y, z, y); } };
	float3 yzz{ get{ return float3(y, z, z); } };
	float3 yzw{ get{ return float3(y, z, w); } set{ y = r.x; z = r.y; w = r.z; } };
	float3 ywx{ get{ return float3(y, w, x); } set{ y = r.x; w = r.y; x = r.z; } };
	float3 ywy{ get{ return float3(y, w, y); } };
	float3 ywz{ get{ return float3(y, w, z); } set{ y = r.x; w = r.y; z = r.z; } };
	float3 yww{ get{ return float3(y, w, w); } };
	float3 zxx{ get{ return float3(z, x, x); } };
	float3 zxy{ get{ return float3(z, x, y); } set{ z = r.x; x = r.y; y = r.z; } };
	float3 zxz{ get{ return float3(z, x, z); } };
	float3 zxw{ get{ return float3(z, x, w); } set{ z = r.x; x = r.y; w = r.z; } };
	float3 zyx{ get{ return float3(z, y, x); } set{ z = r.x; y = r.y; x = r.z; } };
	float3 zyy{ get{ return float3(z, y, y); } };
	float3 zyz{ get{ return float3(z, y, z); } };
	float3 zyw{ get{ return float3(z, y, w); } set{ z = r.x; y = r.y; w = r.z; } };
	float3 zzx{ get{ return float3(z, z, x); } };
	float3 zzy{ get{ return float3(z, z, y); } };
	float3 zzz{ get{ return float3(z, z, z); } };
	float3 zzw{ get{ return float3(z, z, w); } };
	float3 zwx{ get{ return float3(z, w, x); } set{ z = r.x; w = r.y; x = r.z; } };
	float3 zwy{ get{ return float3(z, w, y); } set{ z = r.x; w = r.y; y = r.z; } };
	float3 zwz{ get{ return float3(z, w, z); } };
	float3 zww{ get{ return float3(z, w, w); } };
	float3 wxx{ get{ return float3(w, x, x); } };
	float3 wxy{ get{ return float3(w, x, y); } set{ w = r.x; x = r.y; y = r.z; } };
	float3 wxz{ get{ return float3(w, x, z); } set{ w = r.x; x = r.y; z = r.z; } };
	float3 wxw{ get{ return float3(w, x, w); } };
	float3 wyx{ get{ return float3(w, y, x); } set{ w = r.x; y = r.y; x = r.z; } };
	float3 wyy{ get{ return float3(w, y, y); } };
	float3 wyz{ get{ return float3(w, y, z); } set{ w = r.x; y = r.y; z = r.z; } };
	float3 wyw{ get{ return float3(w, y, w); } };
	float3 wzx{ get{ return float3(w, z, x); } set{ w = r.x; z = r.y; x = r.z; } };
	float3 wzy{ get{ return float3(w, z, y); } set{ w = r.x; z = r.y; y = r.z; } };
	float3 wzz{ get{ return float3(w, z, z); } };
	float3 wzw{ get{ return float3(w, z, w); } };
	float3 wwx{ get{ return float3(w, w, x); } };
	float3 wwy{ get{ return float3(w, w, y); } };
	float3 wwz{ get{ return float3(w, w, z); } };
	float3 www{ get{ return float3(w, w, w); } };
	
	float4 xxxx{ get{ return float4(x, x, x, x); } };
	float4 xxxy{ get{ return float4(x, x, x, y); } };
	float4 xxxz{ get{ return float4(x, x, x, z); } };
	float4 xxxw{ get{ return float4(x, x, x, w); } };
	float4 xxyx{ get{ return float4(x, x, y, x); } };
	float4 xxyy{ get{ return float4(x, x, y, y); } };
	float4 xxyz{ get{ return float4(x, x, y, z); } };
	float4 xxyw{ get{ return float4(x, x, y, w); } };
	float4 xxzx{ get{ return float4(x, x, z, x); } };
	float4 xxzy{ get{ return float4(x, x, z, y); } };
	float4 xxzz{ get{ return float4(x, x, z, z); } };
	float4 xxzw{ get{ return float4(x, x, z, w); } };
	float4 xxwx{ get{ return float4(x, x, w, x); } };
	float4 xxwy{ get{ return float4(x, x, w, y); } };
	float4 xxwz{ get{ return float4(x, x, w, z); } };
	float4 xxww{ get{ return float4(x, x, w, w); } };
	float4 xyxx{ get{ return float4(x, y, x, x); } };
	float4 xyxy{ get{ return float4(x, y, x, y); } };
	float4 xyxz{ get{ return float4(x, y, x, z); } };
	float4 xyxw{ get{ return float4(x, y, x, w); } };
	float4 xyyx{ get{ return float4(x, y, y, x); } };
	float4 xyyy{ get{ return float4(x, y, y, y); } };
	float4 xyyz{ get{ return float4(x, y, y, z); } };
	float4 xyyw{ get{ return float4(x, y, y, w); } };
	float4 xyzx{ get{ return float4(x, y, z, x); } };
	float4 xyzy{ get{ return float4(x, y, z, y); } };
	float4 xyzz{ get{ return float4(x, y, z, z); } };
	float4 xyzw{ get{ return float4(x, y, z, w); } set{ x = r.x; y = r.y; z = r.z; w = r.w; } };
	float4 xywx{ get{ return float4(x, y, w, x); } };
	float4 xywy{ get{ return float4(x, y, w, y); } };
	float4 xywz{ get{ return float4(x, y, w, z); } set{ x = r.x; y = r.y; w = r.z; z = r.w; } };
	float4 xyww{ get{ return float4(x, y, w, w); } };
	float4 xzxx{ get{ return float4(x, z, x, x); } };
	float4 xzxy{ get{ return float4(x, z, x, y); } };
	float4 xzxz{ get{ return float4(x, z, x, z); } };
	float4 xzxw{ get{ return float4(x, z, x, w); } };
	float4 xzyx{ get{ return float4(x, z, y, x); } };
	float4 xzyy{ get{ return float4(x, z, y, y); } };
	float4 xzyz{ get{ return float4(x, z, y, z); } };
	float4 xzyw{ get{ return float4(x, z, y, w); } set{ x = r.x; z = r.y; y = r.z; w = r.w; } };
	float4 xzzx{ get{ return float4(x, z, z, x); } };
	float4 xzzy{ get{ return float4(x, z, z, y); } };
	float4 xzzz{ get{ return float4(x, z, z, z); } };
	float4 xzzw{ get{ return float4(x, z, z, w); } };
	float4 xzwx{ get{ return float4(x, z, w, x); } };
	float4 xzwy{ get{ return float4(x, z, w, y); } set{ x = r.x; z = r.y; w = r.z; y = r.w; } };
	float4 xzwz{ get{ return float4(x, z, w, z); } };
	float4 xzww{ get{ return float4(x, z, w, w); } };
	float4 xwxx{ get{ return float4(x, w, x, x); } };
	float4 xwxy{ get{ return float4(x, w, x, y); } };
	float4 xwxz{ get{ return float4(x, w, x, z); } };
	float4 xwxw{ get{ return float4(x, w, x, w); } };
	float4 xwyx{ get{ return float4(x, w, y, x); } };
	float4 xwyy{ get{ return float4(x, w, y, y); } };
	float4 xwyz{ get{ return float4(x, w, y, z); } set{ x = r.x; w = r.y; y = r.z; z = r.w; } };
	float4 xwyw{ get{ return float4(x, w, y, w); } };
	float4 xwzx{ get{ return float4(x, w, z, x); } };
	float4 xwzy{ get{ return float4(x, w, z, y); } set{ x = r.x; w = r.y; z = r.z; y = r.w; } };
	float4 xwzz{ get{ return float4(x, w, z, z); } };
	float4 xwzw{ get{ return float4(x, w, z, w); } };
	float4 xwwx{ get{ return float4(x, w, w, x); } };
	float4 xwwy{ get{ return float4(x, w, w, y); } };
	float4 xwwz{ get{ return float4(x, w, w, z); } };
	float4 xwww{ get{ return float4(x, w, w, w); } };
	float4 yxxx{ get{ return float4(y, x, x, x); } };
	float4 yxxy{ get{ return float4(y, x, x, y); } };
	float4 yxxz{ get{ return float4(y, x, x, z); } };
	float4 yxxw{ get{ return float4(y, x, x, w); } };
	float4 yxyx{ get{ return float4(y, x, y, x); } };
	float4 yxyy{ get{ return float4(y, x, y, y); } };
	float4 yxyz{ get{ return float4(y, x, y, z); } };
	float4 yxyw{ get{ return float4(y, x, y, w); } };
	float4 yxzx{ get{ return float4(y, x, z, x); } };
	float4 yxzy{ get{ return float4(y, x, z, y); } };
	float4 yxzz{ get{ return float4(y, x, z, z); } };
	float4 yxzw{ get{ return float4(y, x, z, w); } set{ y = r.x; x = r.y; z = r.z; w = r.w; } };
	float4 yxwx{ get{ return float4(y, x, w, x); } };
	float4 yxwy{ get{ return float4(y, x, w, y); } };
	float4 yxwz{ get{ return float4(y, x, w, z); } set{ y = r.x; x = r.y; w = r.z; z = r.w; } };
	float4 yxww{ get{ return float4(y, x, w, w); } };
	float4 yyxx{ get{ return float4(y, y, x, x); } };
	float4 yyxy{ get{ return float4(y, y, x, y); } };
	float4 yyxz{ get{ return float4(y, y, x, z); } };
	float4 yyxw{ get{ return float4(y, y, x, w); } };
	float4 yyyx{ get{ return float4(y, y, y, x); } };
	float4 yyyy{ get{ return float4(y, y, y, y); } };
	float4 yyyz{ get{ return float4(y, y, y, z); } };
	float4 yyyw{ get{ return float4(y, y, y, w); } };
	float4 yyzx{ get{ return float4(y, y, z, x); } };
	float4 yyzy{ get{ return float4(y, y, z, y); } };
	float4 yyzz{ get{ return float4(y, y, z, z); } };
	float4 yyzw{ get{ return float4(y, y, z, w); } };
	float4 yywx{ get{ return float4(y, y, w, x); } };
	float4 yywy{ get{ return float4(y, y, w, y); } };
	float4 yywz{ get{ return float4(y, y, w, z); } };
	float4 yyww{ get{ return float4(y, y, w, w); } };
	float4 yzxx{ get{ return float4(y, z, x, x); } };
	float4 yzxy{ get{ return float4(y, z, x, y); } };
	float4 yzxz{ get{ return float4(y, z, x, z); } };
	float4 yzxw{ get{ return float4(y, z, x, w); } set{ y = r.x; z = r.y; x = r.z; w = r.w; } };
	float4 yzyx{ get{ return float4(y, z, y, x); } };
	float4 yzyy{ get{ return float4(y, z, y, y); } };
	float4 yzyz{ get{ return float4(y, z, y, z); } };
	float4 yzyw{ get{ return float4(y, z, y, w); } };
	float4 yzzx{ get{ return float4(y, z, z, x); } };
	float4 yzzy{ get{ return float4(y, z, z, y); } };
	float4 yzzz{ get{ return float4(y, z, z, z); } };
	float4 yzzw{ get{ return float4(y, z, z, w); } };
	float4 yzwx{ get{ return float4(y, z, w, x); } set{ y = r.x; z = r.y; w = r.z; x = r.w; } };
	float4 yzwy{ get{ return float4(y, z, w, y); } };
	float4 yzwz{ get{ return float4(y, z, w, z); } };
	float4 yzww{ get{ return float4(y, z, w, w); } };
	float4 ywxx{ get{ return float4(y, w, x, x); } };
	float4 ywxy{ get{ return float4(y, w, x, y); } };
	float4 ywxz{ get{ return float4(y, w, x, z); } set{ y = r.x; w = r.y; x = r.z; z = r.w; } };
	float4 ywxw{ get{ return float4(y, w, x, w); } };
	float4 ywyx{ get{ return float4(y, w, y, x); } };
	float4 ywyy{ get{ return float4(y, w, y, y); } };
	float4 ywyz{ get{ return float4(y, w, y, z); } };
	float4 ywyw{ get{ return float4(y, w, y, w); } };
	float4 ywzx{ get{ return float4(y, w, z, x); } set{ y = r.x; w = r.y; z = r.z; x = r.w; } };
	float4 ywzy{ get{ return float4(y, w, z, y); } };
	float4 ywzz{ get{ return float4(y, w, z, z); } };
	float4 ywzw{ get{ return float4(y, w, z, w); } };
	float4 ywwx{ get{ return float4(y, w, w, x); } };
	float4 ywwy{ get{ return float4(y, w, w, y); } };
	float4 ywwz{ get{ return float4(y, w, w, z); } };
	float4 ywww{ get{ return float4(y, w, w, w); } };
	float4 zxxx{ get{ return float4(z, x, x, x); } };
	float4 zxxy{ get{ return float4(z, x, x, y); } };
	float4 zxxz{ get{ return float4(z, x, x, z); } };
	float4 zxxw{ get{ return float4(z, x, x, w); } };
	float4 zxyx{ get{ return float4(z, x, y, x); } };
	float4 zxyy{ get{ return float4(z, x, y, y); } };
	float4 zxyz{ get{ return float4(z, x, y, z); } };
	float4 zxyw{ get{ return float4(z, x, y, w); } set{ z = r.x; x = r.y; y = r.z; w = r.w; } };
	float4 zxzx{ get{ return float4(z, x, z, x); } };
	float4 zxzy{ get{ return float4(z, x, z, y); } };
	float4 zxzz{ get{ return float4(z, x, z, z); } };
	float4 zxzw{ get{ return float4(z, x, z, w); } };
	float4 zxwx{ get{ return float4(z, x, w, x); } };
	float4 zxwy{ get{ return float4(z, x, w, y); } set{ z = r.x; x = r.y; w = r.z; y = r.w; } };
	float4 zxwz{ get{ return float4(z, x, w, z); } };
	float4 zxww{ get{ return float4(z, x, w, w); } };
	float4 zyxx{ get{ return float4(z, y, x, x); } };
	float4 zyxy{ get{ return float4(z, y, x, y); } };
	float4 zyxz{ get{ return float4(z, y, x, z); } };
	float4 zyxw{ get{ return float4(z, y, x, w); } set{ z = r.x; y = r.y; x = r.z; w = r.w; } };
	float4 zyyx{ get{ return float4(z, y, y, x); } };
	float4 zyyy{ get{ return float4(z, y, y, y); } };
	float4 zyyz{ get{ return float4(z, y, y, z); } };
	float4 zyyw{ get{ return float4(z, y, y, w); } };
	float4 zyzx{ get{ return float4(z, y, z, x); } };
	float4 zyzy{ get{ return float4(z, y, z, y); } };
	float4 zyzz{ get{ return float4(z, y, z, z); } };
	float4 zyzw{ get{ return float4(z, y, z, w); } };
	float4 zywx{ get{ return float4(z, y, w, x); } set{ z = r.x; y = r.y; w = r.z; x = r.w; } };
	float4 zywy{ get{ return float4(z, y, w, y); } };
	float4 zywz{ get{ return float4(z, y, w, z); } };
	float4 zyww{ get{ return float4(z, y, w, w); } };
	float4 zzxx{ get{ return float4(z, z, x, x); } };
	float4 zzxy{ get{ return float4(z, z, x, y); } };
	float4 zzxz{ get{ return float4(z, z, x, z); } };
	float4 zzxw{ get{ return float4(z, z, x, w); } };
	float4 zzyx{ get{ return float4(z, z, y, x); } };
	float4 zzyy{ get{ return float4(z, z, y, y); } };
	float4 zzyz{ get{ return float4(z, z, y, z); } };
	float4 zzyw{ get{ return float4(z, z, y, w); } };
	float4 zzzx{ get{ return float4(z, z, z, x); } };
	float4 zzzy{ get{ return float4(z, z, z, y); } };
	float4 zzzz{ get{ return float4(z, z, z, z); } };
	float4 zzzw{ get{ return float4(z, z, z, w); } };
	float4 zzwx{ get{ return float4(z, z, w, x); } };
	float4 zzwy{ get{ return float4(z, z, w, y); } };
	float4 zzwz{ get{ return float4(z, z, w, z); } };
	float4 zzww{ get{ return float4(z, z, w, w); } };
	float4 zwxx{ get{ return float4(z, w, x, x); } };
	float4 zwxy{ get{ return float4(z, w, x, y); } set{ z = r.x; w = r.y; x = r.z; y = r.w; } };
	float4 zwxz{ get{ return float4(z, w, x, z); } };
	float4 zwxw{ get{ return float4(z, w, x, w); } };
	float4 zwyx{ get{ return float4(z, w, y, x); } set{ z = r.x; w = r.y; y = r.z; x = r.w; } };
	float4 zwyy{ get{ return float4(z, w, y, y); } };
	float4 zwyz{ get{ return float4(z, w, y, z); } };
	float4 zwyw{ get{ return float4(z, w, y, w); } };
	float4 zwzx{ get{ return float4(z, w, z, x); } };
	float4 zwzy{ get{ return float4(z, w, z, y); } };
	float4 zwzz{ get{ return float4(z, w, z, z); } };
	float4 zwzw{ get{ return float4(z, w, z, w); } };
	float4 zwwx{ get{ return float4(z, w, w, x); } };
	float4 zwwy{ get{ return float4(z, w, w, y); } };
	float4 zwwz{ get{ return float4(z, w, w, z); } };
	float4 zwww{ get{ return float4(z, w, w, w); } };
	float4 wxxx{ get{ return float4(w, x, x, x); } };
	float4 wxxy{ get{ return float4(w, x, x, y); } };
	float4 wxxz{ get{ return float4(w, x, x, z); } };
	float4 wxxw{ get{ return float4(w, x, x, w); } };
	float4 wxyx{ get{ return float4(w, x, y, x); } };
	float4 wxyy{ get{ return float4(w, x, y, y); } };
	float4 wxyz{ get{ return float4(w, x, y, z); } set{ w = r.x; x = r.y; y = r.z; z = r.w; } };
	float4 wxyw{ get{ return float4(w, x, y, w); } };
	float4 wxzx{ get{ return float4(w, x, z, x); } };
	float4 wxzy{ get{ return float4(w, x, z, y); } set{ w = r.x; x = r.y; z = r.z; y = r.w; } };
	float4 wxzz{ get{ return float4(w, x, z, z); } };
	float4 wxzw{ get{ return float4(w, x, z, w); } };
	float4 wxwx{ get{ return float4(w, x, w, x); } };
	float4 wxwy{ get{ return float4(w, x, w, y); } };
	float4 wxwz{ get{ return float4(w, x, w, z); } };
	float4 wxww{ get{ return float4(w, x, w, w); } };
	float4 wyxx{ get{ return float4(w, y, x, x); } };
	float4 wyxy{ get{ return float4(w, y, x, y); } };
	float4 wyxz{ get{ return float4(w, y, x, z); } set{ w = r.x; y = r.y; x = r.z; z = r.w; } };
	float4 wyxw{ get{ return float4(w, y, x, w); } };
	float4 wyyx{ get{ return float4(w, y, y, x); } };
	float4 wyyy{ get{ return float4(w, y, y, y); } };
	float4 wyyz{ get{ return float4(w, y, y, z); } };
	float4 wyyw{ get{ return float4(w, y, y, w); } };
	float4 wyzx{ get{ return float4(w, y, z, x); } set{ w = r.x; y = r.y; z = r.z; x = r.w; } };
	float4 wyzy{ get{ return float4(w, y, z, y); } };
	float4 wyzz{ get{ return float4(w, y, z, z); } };
	float4 wyzw{ get{ return float4(w, y, z, w); } };
	float4 wywx{ get{ return float4(w, y, w, x); } };
	float4 wywy{ get{ return float4(w, y, w, y); } };
	float4 wywz{ get{ return float4(w, y, w, z); } };
	float4 wyww{ get{ return float4(w, y, w, w); } };
	float4 wzxx{ get{ return float4(w, z, x, x); } };
	float4 wzxy{ get{ return float4(w, z, x, y); } set{ w = r.x; z = r.y; x = r.z; y = r.w; } };
	float4 wzxz{ get{ return float4(w, z, x, z); } };
	float4 wzxw{ get{ return float4(w, z, x, w); } };
	float4 wzyx{ get{ return float4(w, z, y, x); } set{ w = r.x; z = r.y; y = r.z; x = r.w; } };
	float4 wzyy{ get{ return float4(w, z, y, y); } };
	float4 wzyz{ get{ return float4(w, z, y, z); } };
	float4 wzyw{ get{ return float4(w, z, y, w); } };
	float4 wzzx{ get{ return float4(w, z, z, x); } };
	float4 wzzy{ get{ return float4(w, z, z, y); } };
	float4 wzzz{ get{ return float4(w, z, z, z); } };
	float4 wzzw{ get{ return float4(w, z, z, w); } };
	float4 wzwx{ get{ return float4(w, z, w, x); } };
	float4 wzwy{ get{ return float4(w, z, w, y); } };
	float4 wzwz{ get{ return float4(w, z, w, z); } };
	float4 wzww{ get{ return float4(w, z, w, w); } };
	float4 wwxx{ get{ return float4(w, w, x, x); } };
	float4 wwxy{ get{ return float4(w, w, x, y); } };
	float4 wwxz{ get{ return float4(w, w, x, z); } };
	float4 wwxw{ get{ return float4(w, w, x, w); } };
	float4 wwyx{ get{ return float4(w, w, y, x); } };
	float4 wwyy{ get{ return float4(w, w, y, y); } };
	float4 wwyz{ get{ return float4(w, w, y, z); } };
	float4 wwyw{ get{ return float4(w, w, y, w); } };
	float4 wwzx{ get{ return float4(w, w, z, x); } };
	float4 wwzy{ get{ return float4(w, w, z, y); } };
	float4 wwzz{ get{ return float4(w, w, z, z); } };
	float4 wwzw{ get{ return float4(w, w, z, w); } };
	float4 wwwx{ get{ return float4(w, w, w, x); } };
	float4 wwwy{ get{ return float4(w, w, w, y); } };
	float4 wwwz{ get{ return float4(w, w, w, z); } };
	float4 wwww{ get{ return float4(w, w, w, w); } };
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

float dot(float2 ref a, float2 ref b);
float dot(float3 ref a, float3 ref b);
float dot(float4 ref a, float4 ref b);
