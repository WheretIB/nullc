#include "runtime.h"
// Typeid redirect table
static unsigned __nullcTR[103];
// Array classes
typedef struct
{
	double pi;
	double e;
} MathConstants;
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
struct double_4_ 
{
	double ptr[4];
	double_4_ & set(unsigned index, double const & val){ ptr[index] = val; return *this; }
};
typedef struct
{
	float4 row1;
	float4 row2;
	float4 row3;
	float4 row4;
} float4x4;
double cos(double deg, void* unused);
double sin(double deg, void* unused);
double tan(double deg, void* unused);
double ctg(double deg, void* unused);
double cosh(double deg, void* unused);
double sinh(double deg, void* unused);
double tanh(double deg, void* unused);
double coth(double deg, void* unused);
double acos(double deg, void* unused);
double asin(double deg, void* unused);
double atan(double deg, void* unused);
double ceil(double num, void* unused);
double floor(double num, void* unused);
double exp(double num, void* unused);
double log(double num, void* unused);
double sqrt(double num, void* unused);
double clamp(double val, double min, double max, void* unused);
double saturate(double val, void* unused);
double abs(double val, void* unused);
float2 float2__float2(float x, float y, float2 * __context);
float2 float2__xx_(float2 * __context);
float2 float2__xy_(float2 * __context);
void float2__xy_(float2 r, float2 * __context);
float2 float2__yx_(float2 * __context);
void float2__yx_(float2 r, float2 * __context);
float2 float2__yy_(float2 * __context);
float2 float2__(float x, float y, void* unused);
float2 __operatorAdd(float2 a, float2 b, void* unused);
float2 __operatorSub(float2 a, float2 b, void* unused);
float2 __operatorMul(float2 a, float2 b, void* unused);
float2 __operatorDiv(float2 a, float2 b, void* unused);
float2 __operatorMul(float2 a, float b, void* unused);
float2 __operatorMul(float a, float2 b, void* unused);
float2 __operatorDiv(float2 a, float b, void* unused);
float2 * __operatorAddSet(float2 * a, float2 b, void* unused);
float2 * __operatorSubSet(float2 * a, float2 b, void* unused);
float2 * __operatorMulSet(float2 * a, float b, void* unused);
float2 * __operatorDivSet(float2 * a, float b, void* unused);
float3 float3__float3(float x, float y, float z, float3 * __context);
float2 float3__xx_(float3 * __context);
float2 float3__xy_(float3 * __context);
void float3__xy_(float2 r, float3 * __context);
float2 float3__xz_(float3 * __context);
void float3__xz_(float2 r, float3 * __context);
float2 float3__yx_(float3 * __context);
void float3__yx_(float2 r, float3 * __context);
float2 float3__yy_(float3 * __context);
float2 float3__yz_(float3 * __context);
void float3__yz_(float2 r, float3 * __context);
float2 float3__zx_(float3 * __context);
void float3__zx_(float2 r, float3 * __context);
float2 float3__zy_(float3 * __context);
void float3__zy_(float2 r, float3 * __context);
float2 float3__zz_(float3 * __context);
float3 float3__xxx_(float3 * __context);
float3 float3__xxy_(float3 * __context);
float3 float3__xxz_(float3 * __context);
float3 float3__xyx_(float3 * __context);
float3 float3__xyy_(float3 * __context);
float3 float3__xyz_(float3 * __context);
void float3__xyz_(float3 r, float3 * __context);
float3 float3__xzx_(float3 * __context);
float3 float3__xzy_(float3 * __context);
void float3__xzy_(float3 r, float3 * __context);
float3 float3__xzz_(float3 * __context);
float3 float3__yxx_(float3 * __context);
float3 float3__yxy_(float3 * __context);
float3 float3__yxz_(float3 * __context);
void float3__yxz_(float3 r, float3 * __context);
float3 float3__yyx_(float3 * __context);
float3 float3__yyy_(float3 * __context);
float3 float3__yyz_(float3 * __context);
float3 float3__yzx_(float3 * __context);
void float3__yzx_(float3 r, float3 * __context);
float3 float3__yzy_(float3 * __context);
float3 float3__yzz_(float3 * __context);
float3 float3__zxx_(float3 * __context);
float3 float3__zxy_(float3 * __context);
void float3__zxy_(float3 r, float3 * __context);
float3 float3__zxz_(float3 * __context);
float3 float3__zyx_(float3 * __context);
void float3__zyx_(float3 r, float3 * __context);
float3 float3__zyy_(float3 * __context);
float3 float3__zyz_(float3 * __context);
float3 float3__zzx_(float3 * __context);
float3 float3__zzy_(float3 * __context);
float3 float3__zzz_(float3 * __context);
float3 float3__(float x, float y, float z, void* unused);
float3 __operatorAdd(float3 a, float3 b, void* unused);
float3 __operatorSub(float3 a, float3 b, void* unused);
float3 __operatorMul(float3 a, float3 b, void* unused);
float3 __operatorDiv(float3 a, float3 b, void* unused);
float3 __operatorMul(float3 a, float b, void* unused);
float3 __operatorMul(float a, float3 b, void* unused);
float3 __operatorDiv(float3 a, float b, void* unused);
float3 * __operatorAddSet(float3 * a, float3 b, void* unused);
float3 * __operatorSubSet(float3 * a, float3 b, void* unused);
float3 * __operatorMulSet(float3 * a, float b, void* unused);
float3 * __operatorDivSet(float3 * a, float b, void* unused);
float4 float4__float4(float x, float y, float z, float w, float4 * __context);
float2 float4__xx_(float4 * __context);
float2 float4__xy_(float4 * __context);
void float4__xy_(float2 r, float4 * __context);
float2 float4__xz_(float4 * __context);
void float4__xz_(float2 r, float4 * __context);
float2 float4__xw_(float4 * __context);
void float4__xw_(float2 r, float4 * __context);
float2 float4__yx_(float4 * __context);
void float4__yx_(float2 r, float4 * __context);
float2 float4__yy_(float4 * __context);
float2 float4__yz_(float4 * __context);
void float4__yz_(float2 r, float4 * __context);
float2 float4__yw_(float4 * __context);
void float4__yw_(float2 r, float4 * __context);
float2 float4__zx_(float4 * __context);
void float4__zx_(float2 r, float4 * __context);
float2 float4__zy_(float4 * __context);
void float4__zy_(float2 r, float4 * __context);
float2 float4__zz_(float4 * __context);
float2 float4__zw_(float4 * __context);
void float4__zw_(float2 r, float4 * __context);
float2 float4__wx_(float4 * __context);
void float4__wx_(float2 r, float4 * __context);
float2 float4__wy_(float4 * __context);
void float4__wy_(float2 r, float4 * __context);
float2 float4__wz_(float4 * __context);
void float4__wz_(float2 r, float4 * __context);
float2 float4__ww_(float4 * __context);
float3 float4__xxx_(float4 * __context);
float3 float4__xxy_(float4 * __context);
float3 float4__xxz_(float4 * __context);
float3 float4__xxw_(float4 * __context);
float3 float4__xyx_(float4 * __context);
float3 float4__xyy_(float4 * __context);
float3 float4__xyz_(float4 * __context);
void float4__xyz_(float3 r, float4 * __context);
float3 float4__xyw_(float4 * __context);
void float4__xyw_(float3 r, float4 * __context);
float3 float4__xzx_(float4 * __context);
float3 float4__xzy_(float4 * __context);
void float4__xzy_(float3 r, float4 * __context);
float3 float4__xzz_(float4 * __context);
float3 float4__xzw_(float4 * __context);
void float4__xzw_(float3 r, float4 * __context);
float3 float4__xwx_(float4 * __context);
float3 float4__xwy_(float4 * __context);
void float4__xwy_(float3 r, float4 * __context);
float3 float4__xwz_(float4 * __context);
void float4__xwz_(float3 r, float4 * __context);
float3 float4__xww_(float4 * __context);
float3 float4__yxx_(float4 * __context);
float3 float4__yxy_(float4 * __context);
float3 float4__yxz_(float4 * __context);
void float4__yxz_(float3 r, float4 * __context);
float3 float4__yxw_(float4 * __context);
void float4__yxw_(float3 r, float4 * __context);
float3 float4__yyx_(float4 * __context);
float3 float4__yyy_(float4 * __context);
float3 float4__yyz_(float4 * __context);
float3 float4__yyw_(float4 * __context);
float3 float4__yzx_(float4 * __context);
void float4__yzx_(float3 r, float4 * __context);
float3 float4__yzy_(float4 * __context);
float3 float4__yzz_(float4 * __context);
float3 float4__yzw_(float4 * __context);
void float4__yzw_(float3 r, float4 * __context);
float3 float4__ywx_(float4 * __context);
void float4__ywx_(float3 r, float4 * __context);
float3 float4__ywy_(float4 * __context);
float3 float4__ywz_(float4 * __context);
void float4__ywz_(float3 r, float4 * __context);
float3 float4__yww_(float4 * __context);
float3 float4__zxx_(float4 * __context);
float3 float4__zxy_(float4 * __context);
void float4__zxy_(float3 r, float4 * __context);
float3 float4__zxz_(float4 * __context);
float3 float4__zxw_(float4 * __context);
void float4__zxw_(float3 r, float4 * __context);
float3 float4__zyx_(float4 * __context);
void float4__zyx_(float3 r, float4 * __context);
float3 float4__zyy_(float4 * __context);
float3 float4__zyz_(float4 * __context);
float3 float4__zyw_(float4 * __context);
void float4__zyw_(float3 r, float4 * __context);
float3 float4__zzx_(float4 * __context);
float3 float4__zzy_(float4 * __context);
float3 float4__zzz_(float4 * __context);
float3 float4__zzw_(float4 * __context);
float3 float4__zwx_(float4 * __context);
void float4__zwx_(float3 r, float4 * __context);
float3 float4__zwy_(float4 * __context);
void float4__zwy_(float3 r, float4 * __context);
float3 float4__zwz_(float4 * __context);
float3 float4__zww_(float4 * __context);
float3 float4__wxx_(float4 * __context);
float3 float4__wxy_(float4 * __context);
void float4__wxy_(float3 r, float4 * __context);
float3 float4__wxz_(float4 * __context);
void float4__wxz_(float3 r, float4 * __context);
float3 float4__wxw_(float4 * __context);
float3 float4__wyx_(float4 * __context);
void float4__wyx_(float3 r, float4 * __context);
float3 float4__wyy_(float4 * __context);
float3 float4__wyz_(float4 * __context);
void float4__wyz_(float3 r, float4 * __context);
float3 float4__wyw_(float4 * __context);
float3 float4__wzx_(float4 * __context);
void float4__wzx_(float3 r, float4 * __context);
float3 float4__wzy_(float4 * __context);
void float4__wzy_(float3 r, float4 * __context);
float3 float4__wzz_(float4 * __context);
float3 float4__wzw_(float4 * __context);
float3 float4__wwx_(float4 * __context);
float3 float4__wwy_(float4 * __context);
float3 float4__wwz_(float4 * __context);
float3 float4__www_(float4 * __context);
float4 float4__xxxx_(float4 * __context);
float4 float4__xxxy_(float4 * __context);
float4 float4__xxxz_(float4 * __context);
float4 float4__xxxw_(float4 * __context);
float4 float4__xxyx_(float4 * __context);
float4 float4__xxyy_(float4 * __context);
float4 float4__xxyz_(float4 * __context);
float4 float4__xxyw_(float4 * __context);
float4 float4__xxzx_(float4 * __context);
float4 float4__xxzy_(float4 * __context);
float4 float4__xxzz_(float4 * __context);
float4 float4__xxzw_(float4 * __context);
float4 float4__xxwx_(float4 * __context);
float4 float4__xxwy_(float4 * __context);
float4 float4__xxwz_(float4 * __context);
float4 float4__xxww_(float4 * __context);
float4 float4__xyxx_(float4 * __context);
float4 float4__xyxy_(float4 * __context);
float4 float4__xyxz_(float4 * __context);
float4 float4__xyxw_(float4 * __context);
float4 float4__xyyx_(float4 * __context);
float4 float4__xyyy_(float4 * __context);
float4 float4__xyyz_(float4 * __context);
float4 float4__xyyw_(float4 * __context);
float4 float4__xyzx_(float4 * __context);
float4 float4__xyzy_(float4 * __context);
float4 float4__xyzz_(float4 * __context);
float4 float4__xyzw_(float4 * __context);
void float4__xyzw_(float4 r, float4 * __context);
float4 float4__xywx_(float4 * __context);
float4 float4__xywy_(float4 * __context);
float4 float4__xywz_(float4 * __context);
void float4__xywz_(float4 r, float4 * __context);
float4 float4__xyww_(float4 * __context);
float4 float4__xzxx_(float4 * __context);
float4 float4__xzxy_(float4 * __context);
float4 float4__xzxz_(float4 * __context);
float4 float4__xzxw_(float4 * __context);
float4 float4__xzyx_(float4 * __context);
float4 float4__xzyy_(float4 * __context);
float4 float4__xzyz_(float4 * __context);
float4 float4__xzyw_(float4 * __context);
void float4__xzyw_(float4 r, float4 * __context);
float4 float4__xzzx_(float4 * __context);
float4 float4__xzzy_(float4 * __context);
float4 float4__xzzz_(float4 * __context);
float4 float4__xzzw_(float4 * __context);
float4 float4__xzwx_(float4 * __context);
float4 float4__xzwy_(float4 * __context);
void float4__xzwy_(float4 r, float4 * __context);
float4 float4__xzwz_(float4 * __context);
float4 float4__xzww_(float4 * __context);
float4 float4__xwxx_(float4 * __context);
float4 float4__xwxy_(float4 * __context);
float4 float4__xwxz_(float4 * __context);
float4 float4__xwxw_(float4 * __context);
float4 float4__xwyx_(float4 * __context);
float4 float4__xwyy_(float4 * __context);
float4 float4__xwyz_(float4 * __context);
void float4__xwyz_(float4 r, float4 * __context);
float4 float4__xwyw_(float4 * __context);
float4 float4__xwzx_(float4 * __context);
float4 float4__xwzy_(float4 * __context);
void float4__xwzy_(float4 r, float4 * __context);
float4 float4__xwzz_(float4 * __context);
float4 float4__xwzw_(float4 * __context);
float4 float4__xwwx_(float4 * __context);
float4 float4__xwwy_(float4 * __context);
float4 float4__xwwz_(float4 * __context);
float4 float4__xwww_(float4 * __context);
float4 float4__yxxx_(float4 * __context);
float4 float4__yxxy_(float4 * __context);
float4 float4__yxxz_(float4 * __context);
float4 float4__yxxw_(float4 * __context);
float4 float4__yxyx_(float4 * __context);
float4 float4__yxyy_(float4 * __context);
float4 float4__yxyz_(float4 * __context);
float4 float4__yxyw_(float4 * __context);
float4 float4__yxzx_(float4 * __context);
float4 float4__yxzy_(float4 * __context);
float4 float4__yxzz_(float4 * __context);
float4 float4__yxzw_(float4 * __context);
void float4__yxzw_(float4 r, float4 * __context);
float4 float4__yxwx_(float4 * __context);
float4 float4__yxwy_(float4 * __context);
float4 float4__yxwz_(float4 * __context);
void float4__yxwz_(float4 r, float4 * __context);
float4 float4__yxww_(float4 * __context);
float4 float4__yyxx_(float4 * __context);
float4 float4__yyxy_(float4 * __context);
float4 float4__yyxz_(float4 * __context);
float4 float4__yyxw_(float4 * __context);
float4 float4__yyyx_(float4 * __context);
float4 float4__yyyy_(float4 * __context);
float4 float4__yyyz_(float4 * __context);
float4 float4__yyyw_(float4 * __context);
float4 float4__yyzx_(float4 * __context);
float4 float4__yyzy_(float4 * __context);
float4 float4__yyzz_(float4 * __context);
float4 float4__yyzw_(float4 * __context);
float4 float4__yywx_(float4 * __context);
float4 float4__yywy_(float4 * __context);
float4 float4__yywz_(float4 * __context);
float4 float4__yyww_(float4 * __context);
float4 float4__yzxx_(float4 * __context);
float4 float4__yzxy_(float4 * __context);
float4 float4__yzxz_(float4 * __context);
float4 float4__yzxw_(float4 * __context);
void float4__yzxw_(float4 r, float4 * __context);
float4 float4__yzyx_(float4 * __context);
float4 float4__yzyy_(float4 * __context);
float4 float4__yzyz_(float4 * __context);
float4 float4__yzyw_(float4 * __context);
float4 float4__yzzx_(float4 * __context);
float4 float4__yzzy_(float4 * __context);
float4 float4__yzzz_(float4 * __context);
float4 float4__yzzw_(float4 * __context);
float4 float4__yzwx_(float4 * __context);
void float4__yzwx_(float4 r, float4 * __context);
float4 float4__yzwy_(float4 * __context);
float4 float4__yzwz_(float4 * __context);
float4 float4__yzww_(float4 * __context);
float4 float4__ywxx_(float4 * __context);
float4 float4__ywxy_(float4 * __context);
float4 float4__ywxz_(float4 * __context);
void float4__ywxz_(float4 r, float4 * __context);
float4 float4__ywxw_(float4 * __context);
float4 float4__ywyx_(float4 * __context);
float4 float4__ywyy_(float4 * __context);
float4 float4__ywyz_(float4 * __context);
float4 float4__ywyw_(float4 * __context);
float4 float4__ywzx_(float4 * __context);
void float4__ywzx_(float4 r, float4 * __context);
float4 float4__ywzy_(float4 * __context);
float4 float4__ywzz_(float4 * __context);
float4 float4__ywzw_(float4 * __context);
float4 float4__ywwx_(float4 * __context);
float4 float4__ywwy_(float4 * __context);
float4 float4__ywwz_(float4 * __context);
float4 float4__ywww_(float4 * __context);
float4 float4__zxxx_(float4 * __context);
float4 float4__zxxy_(float4 * __context);
float4 float4__zxxz_(float4 * __context);
float4 float4__zxxw_(float4 * __context);
float4 float4__zxyx_(float4 * __context);
float4 float4__zxyy_(float4 * __context);
float4 float4__zxyz_(float4 * __context);
float4 float4__zxyw_(float4 * __context);
void float4__zxyw_(float4 r, float4 * __context);
float4 float4__zxzx_(float4 * __context);
float4 float4__zxzy_(float4 * __context);
float4 float4__zxzz_(float4 * __context);
float4 float4__zxzw_(float4 * __context);
float4 float4__zxwx_(float4 * __context);
float4 float4__zxwy_(float4 * __context);
void float4__zxwy_(float4 r, float4 * __context);
float4 float4__zxwz_(float4 * __context);
float4 float4__zxww_(float4 * __context);
float4 float4__zyxx_(float4 * __context);
float4 float4__zyxy_(float4 * __context);
float4 float4__zyxz_(float4 * __context);
float4 float4__zyxw_(float4 * __context);
void float4__zyxw_(float4 r, float4 * __context);
float4 float4__zyyx_(float4 * __context);
float4 float4__zyyy_(float4 * __context);
float4 float4__zyyz_(float4 * __context);
float4 float4__zyyw_(float4 * __context);
float4 float4__zyzx_(float4 * __context);
float4 float4__zyzy_(float4 * __context);
float4 float4__zyzz_(float4 * __context);
float4 float4__zyzw_(float4 * __context);
float4 float4__zywx_(float4 * __context);
void float4__zywx_(float4 r, float4 * __context);
float4 float4__zywy_(float4 * __context);
float4 float4__zywz_(float4 * __context);
float4 float4__zyww_(float4 * __context);
float4 float4__zzxx_(float4 * __context);
float4 float4__zzxy_(float4 * __context);
float4 float4__zzxz_(float4 * __context);
float4 float4__zzxw_(float4 * __context);
float4 float4__zzyx_(float4 * __context);
float4 float4__zzyy_(float4 * __context);
float4 float4__zzyz_(float4 * __context);
float4 float4__zzyw_(float4 * __context);
float4 float4__zzzx_(float4 * __context);
float4 float4__zzzy_(float4 * __context);
float4 float4__zzzz_(float4 * __context);
float4 float4__zzzw_(float4 * __context);
float4 float4__zzwx_(float4 * __context);
float4 float4__zzwy_(float4 * __context);
float4 float4__zzwz_(float4 * __context);
float4 float4__zzww_(float4 * __context);
float4 float4__zwxx_(float4 * __context);
float4 float4__zwxy_(float4 * __context);
void float4__zwxy_(float4 r, float4 * __context);
float4 float4__zwxz_(float4 * __context);
float4 float4__zwxw_(float4 * __context);
float4 float4__zwyx_(float4 * __context);
void float4__zwyx_(float4 r, float4 * __context);
float4 float4__zwyy_(float4 * __context);
float4 float4__zwyz_(float4 * __context);
float4 float4__zwyw_(float4 * __context);
float4 float4__zwzx_(float4 * __context);
float4 float4__zwzy_(float4 * __context);
float4 float4__zwzz_(float4 * __context);
float4 float4__zwzw_(float4 * __context);
float4 float4__zwwx_(float4 * __context);
float4 float4__zwwy_(float4 * __context);
float4 float4__zwwz_(float4 * __context);
float4 float4__zwww_(float4 * __context);
float4 float4__wxxx_(float4 * __context);
float4 float4__wxxy_(float4 * __context);
float4 float4__wxxz_(float4 * __context);
float4 float4__wxxw_(float4 * __context);
float4 float4__wxyx_(float4 * __context);
float4 float4__wxyy_(float4 * __context);
float4 float4__wxyz_(float4 * __context);
void float4__wxyz_(float4 r, float4 * __context);
float4 float4__wxyw_(float4 * __context);
float4 float4__wxzx_(float4 * __context);
float4 float4__wxzy_(float4 * __context);
void float4__wxzy_(float4 r, float4 * __context);
float4 float4__wxzz_(float4 * __context);
float4 float4__wxzw_(float4 * __context);
float4 float4__wxwx_(float4 * __context);
float4 float4__wxwy_(float4 * __context);
float4 float4__wxwz_(float4 * __context);
float4 float4__wxww_(float4 * __context);
float4 float4__wyxx_(float4 * __context);
float4 float4__wyxy_(float4 * __context);
float4 float4__wyxz_(float4 * __context);
void float4__wyxz_(float4 r, float4 * __context);
float4 float4__wyxw_(float4 * __context);
float4 float4__wyyx_(float4 * __context);
float4 float4__wyyy_(float4 * __context);
float4 float4__wyyz_(float4 * __context);
float4 float4__wyyw_(float4 * __context);
float4 float4__wyzx_(float4 * __context);
void float4__wyzx_(float4 r, float4 * __context);
float4 float4__wyzy_(float4 * __context);
float4 float4__wyzz_(float4 * __context);
float4 float4__wyzw_(float4 * __context);
float4 float4__wywx_(float4 * __context);
float4 float4__wywy_(float4 * __context);
float4 float4__wywz_(float4 * __context);
float4 float4__wyww_(float4 * __context);
float4 float4__wzxx_(float4 * __context);
float4 float4__wzxy_(float4 * __context);
void float4__wzxy_(float4 r, float4 * __context);
float4 float4__wzxz_(float4 * __context);
float4 float4__wzxw_(float4 * __context);
float4 float4__wzyx_(float4 * __context);
void float4__wzyx_(float4 r, float4 * __context);
float4 float4__wzyy_(float4 * __context);
float4 float4__wzyz_(float4 * __context);
float4 float4__wzyw_(float4 * __context);
float4 float4__wzzx_(float4 * __context);
float4 float4__wzzy_(float4 * __context);
float4 float4__wzzz_(float4 * __context);
float4 float4__wzzw_(float4 * __context);
float4 float4__wzwx_(float4 * __context);
float4 float4__wzwy_(float4 * __context);
float4 float4__wzwz_(float4 * __context);
float4 float4__wzww_(float4 * __context);
float4 float4__wwxx_(float4 * __context);
float4 float4__wwxy_(float4 * __context);
float4 float4__wwxz_(float4 * __context);
float4 float4__wwxw_(float4 * __context);
float4 float4__wwyx_(float4 * __context);
float4 float4__wwyy_(float4 * __context);
float4 float4__wwyz_(float4 * __context);
float4 float4__wwyw_(float4 * __context);
float4 float4__wwzx_(float4 * __context);
float4 float4__wwzy_(float4 * __context);
float4 float4__wwzz_(float4 * __context);
float4 float4__wwzw_(float4 * __context);
float4 float4__wwwx_(float4 * __context);
float4 float4__wwwy_(float4 * __context);
float4 float4__wwwz_(float4 * __context);
float4 float4__wwww_(float4 * __context);
float4 float4__(float x, float y, float z, float w, void* unused);
float4 __operatorAdd(float4 a, float4 b, void* unused);
float4 __operatorSub(float4 a, float4 b, void* unused);
float4 __operatorMul(float4 a, float4 b, void* unused);
float4 __operatorDiv(float4 a, float4 b, void* unused);
float4 __operatorMul(float4 a, float b, void* unused);
float4 __operatorMul(float a, float4 b, void* unused);
float4 __operatorDiv(float4 a, float b, void* unused);
float4 * __operatorAddSet(float4 * a, float4 b, void* unused);
float4 * __operatorSubSet(float4 * a, float4 b, void* unused);
float4 * __operatorMulSet(float4 * a, float b, void* unused);
float4 * __operatorDivSet(float4 * a, float b, void* unused);
float3 float3__(float2 xy, float z, void* unused);
float3 float3__(float x, float2 yz, void* unused);
float4 float4__(float3 xyz, float w, void* unused);
float4 float4__(float2 xy, float2 zw, void* unused);
float4 float4__(float2 xy, float z, float w, void* unused);
float4 float4__(float x, float y, float2 zw, void* unused);
float4 float4__(float x, float3 yzw, void* unused);
float4 * __operatorSet(float4 * a, double_4_ xyzw, void* unused);
float3 reflect(float3 normal, float3 dir, void* unused);
float * __operatorIndex(float2 * a, int index, void* unused);
float * __operatorIndex(float3 * a, int index, void* unused);
float * __operatorIndex(float4 * a, int index, void* unused);
float float2__length(float2 * __context);
float float2__normalize(float2 * __context);
float float3__length(float3 * __context);
float float3__normalize(float3 * __context);
float float4__length(float4 * __context);
float float4__normalize(float4 * __context);
float dot(float2 * a, float2 * b, void* unused);
float dot(float3 * a, float3 * b, void* unused);
float dot(float4 * a, float4 * b, void* unused);
MathConstants math;
float2  float2__float2(float x_0, float y_4, float2 * __context)
{
	float2 res_12;
	/* node translation unknown */
	*(&(&res_12)->x) = *(&x_0);
	*(&(&res_12)->y) = *(&y_4);
	return *(&res_12);
}
float2  float2__xx_(float2 * __context)
{
	return float2__float2(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float2  float2__xy_(float2 * __context)
{
	return float2__float2(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
void  float2__xy_(float2 r_0, float2 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float2__yx_(float2 * __context)
{
	return float2__float2(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
void  float2__yx_(float2 r_0, float2 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float2__yy_(float2 * __context)
{
	return float2__float2(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float2  float2__(float x_0, float y_4, void* unused)
{
	float2 ret_12;
	/* node translation unknown */
	*(&(&ret_12)->x) = *(&x_0);
	*(&(&ret_12)->y) = *(&y_4);
	return *(&ret_12);
}
float2  __operatorAdd(float2 a_0, float2 b_8, void* unused)
{
	return float2__((*(&(&a_0)->x)) + (*(&(&b_8)->x)), (*(&(&a_0)->y)) + (*(&(&b_8)->y)), (void*)0);
}
float2  __operatorSub(float2 a_0, float2 b_8, void* unused)
{
	return float2__((*(&(&a_0)->x)) - (*(&(&b_8)->x)), (*(&(&a_0)->y)) - (*(&(&b_8)->y)), (void*)0);
}
float2  __operatorMul(float2 a_0, float2 b_8, void* unused)
{
	return float2__((*(&(&a_0)->x)) * (*(&(&b_8)->x)), (*(&(&a_0)->y)) * (*(&(&b_8)->y)), (void*)0);
}
float2  __operatorDiv(float2 a_0, float2 b_8, void* unused)
{
	return float2__((*(&(&a_0)->x)) / (*(&(&b_8)->x)), (*(&(&a_0)->y)) / (*(&(&b_8)->y)), (void*)0);
}
float2  __operatorMul(float2 a_0, float b_8, void* unused)
{
	return float2__((*(&(&a_0)->x)) * (*(&b_8)), (*(&(&a_0)->y)) * (*(&b_8)), (void*)0);
}
float2  __operatorMul(float a_0, float2 b_4, void* unused)
{
	return float2__((*(&a_0)) * (*(&(&b_4)->x)), (*(&a_0)) * (*(&(&b_4)->y)), (void*)0);
}
float2  __operatorDiv(float2 a_0, float b_8, void* unused)
{
	return float2__((*(&(&a_0)->x)) / (*(&b_8)), (*(&(&a_0)->y)) / (*(&b_8)), (void*)0);
}
float2 *  __operatorAddSet(float2 * a_0, float2 b_4, void* unused)
{
	*(&(*(&a_0))->x) += *(&(&b_4)->x);
	*(&(*(&a_0))->y) += *(&(&b_4)->y);
	return *(&a_0);
}
float2 *  __operatorSubSet(float2 * a_0, float2 b_4, void* unused)
{
	*(&(*(&a_0))->x) -= *(&(&b_4)->x);
	*(&(*(&a_0))->y) -= *(&(&b_4)->y);
	return *(&a_0);
}
float2 *  __operatorMulSet(float2 * a_0, float b_4, void* unused)
{
	*(&(*(&a_0))->x) *= *(&b_4);
	*(&(*(&a_0))->y) *= *(&b_4);
	return *(&a_0);
}
float2 *  __operatorDivSet(float2 * a_0, float b_4, void* unused)
{
	*(&(*(&a_0))->x) /= *(&b_4);
	*(&(*(&a_0))->y) /= *(&b_4);
	return *(&a_0);
}
float3  float3__float3(float x_0, float y_4, float z_8, float3 * __context)
{
	float3 res_16;
	/* node translation unknown */
	*(&(&res_16)->x) = *(&x_0);
	*(&(&res_16)->y) = *(&y_4);
	*(&(&res_16)->z) = *(&z_8);
	return *(&res_16);
}
float2  float3__xx_(float3 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float2  float3__xy_(float3 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
void  float3__xy_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float3__xz_(float3 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
void  float3__xz_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float3__yx_(float3 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
void  float3__yx_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float3__yy_(float3 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float2  float3__yz_(float3 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
void  float3__yz_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float3__zx_(float3 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
void  float3__zx_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float3__zy_(float3 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
void  float3__zy_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float3__zz_(float3 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float3__xxx_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float3  float3__xxy_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float3  float3__xxz_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float3  float3__xyx_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float3  float3__xyy_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float3  float3__xyz_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
void  float3__xyz_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float3__xzx_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float3  float3__xzy_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
void  float3__xzy_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float3__xzz_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float3  float3__yxx_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float3  float3__yxy_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float3  float3__yxz_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
void  float3__yxz_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float3__yyx_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float3  float3__yyy_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float3  float3__yyz_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float3  float3__yzx_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
void  float3__yzx_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float3__yzy_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float3  float3__yzz_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float3  float3__zxx_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float3  float3__zxy_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
void  float3__zxy_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float3__zxz_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float3  float3__zyx_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
void  float3__zyx_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float3__zyy_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float3  float3__zyz_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float3  float3__zzx_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float3  float3__zzy_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float3  float3__zzz_(float3 * __context)
{
	return float3__float3(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float3  float3__(float x_0, float y_4, float z_8, void* unused)
{
	float3 ret_16;
	/* node translation unknown */
	*(&(&ret_16)->x) = *(&x_0);
	*(&(&ret_16)->y) = *(&y_4);
	*(&(&ret_16)->z) = *(&z_8);
	return *(&ret_16);
}
float3  __operatorAdd(float3 a_0, float3 b_12, void* unused)
{
	return float3__((*(&(&a_0)->x)) + (*(&(&b_12)->x)), (*(&(&a_0)->y)) + (*(&(&b_12)->y)), (*(&(&a_0)->z)) + (*(&(&b_12)->z)), (void*)0);
}
float3  __operatorSub(float3 a_0, float3 b_12, void* unused)
{
	return float3__((*(&(&a_0)->x)) - (*(&(&b_12)->x)), (*(&(&a_0)->y)) - (*(&(&b_12)->y)), (*(&(&a_0)->z)) - (*(&(&b_12)->z)), (void*)0);
}
float3  __operatorMul(float3 a_0, float3 b_12, void* unused)
{
	return float3__((*(&(&a_0)->x)) * (*(&(&b_12)->x)), (*(&(&a_0)->y)) * (*(&(&b_12)->y)), (*(&(&a_0)->z)) * (*(&(&b_12)->z)), (void*)0);
}
float3  __operatorDiv(float3 a_0, float3 b_12, void* unused)
{
	return float3__((*(&(&a_0)->x)) / (*(&(&b_12)->x)), (*(&(&a_0)->y)) / (*(&(&b_12)->y)), (*(&(&a_0)->z)) / (*(&(&b_12)->z)), (void*)0);
}
float3  __operatorMul(float3 a_0, float b_12, void* unused)
{
	return float3__((*(&(&a_0)->x)) * (*(&b_12)), (*(&(&a_0)->y)) * (*(&b_12)), (*(&(&a_0)->z)) * (*(&b_12)), (void*)0);
}
float3  __operatorMul(float a_0, float3 b_4, void* unused)
{
	return float3__((*(&a_0)) * (*(&(&b_4)->x)), (*(&a_0)) * (*(&(&b_4)->y)), (*(&a_0)) * (*(&(&b_4)->z)), (void*)0);
}
float3  __operatorDiv(float3 a_0, float b_12, void* unused)
{
	return float3__((*(&(&a_0)->x)) / (*(&b_12)), (*(&(&a_0)->y)) / (*(&b_12)), (*(&(&a_0)->z)) / (*(&b_12)), (void*)0);
}
float3 *  __operatorAddSet(float3 * a_0, float3 b_4, void* unused)
{
	*(&(*(&a_0))->x) += *(&(&b_4)->x);
	*(&(*(&a_0))->y) += *(&(&b_4)->y);
	*(&(*(&a_0))->z) += *(&(&b_4)->z);
	return *(&a_0);
}
float3 *  __operatorSubSet(float3 * a_0, float3 b_4, void* unused)
{
	*(&(*(&a_0))->x) -= *(&(&b_4)->x);
	*(&(*(&a_0))->y) -= *(&(&b_4)->y);
	*(&(*(&a_0))->z) -= *(&(&b_4)->z);
	return *(&a_0);
}
float3 *  __operatorMulSet(float3 * a_0, float b_4, void* unused)
{
	*(&(*(&a_0))->x) *= *(&b_4);
	*(&(*(&a_0))->y) *= *(&b_4);
	*(&(*(&a_0))->z) *= *(&b_4);
	return *(&a_0);
}
float3 *  __operatorDivSet(float3 * a_0, float b_4, void* unused)
{
	*(&(*(&a_0))->x) /= *(&b_4);
	*(&(*(&a_0))->y) /= *(&b_4);
	*(&(*(&a_0))->z) /= *(&b_4);
	return *(&a_0);
}
float4  float4__float4(float x_0, float y_4, float z_8, float w_12, float4 * __context)
{
	float4 res_20;
	/* node translation unknown */
	*(&(&res_20)->x) = *(&x_0);
	*(&(&res_20)->y) = *(&y_4);
	*(&(&res_20)->z) = *(&z_8);
	*(&(&res_20)->w) = *(&w_12);
	return *(&res_20);
}
float2  float4__xx_(float4 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float2  float4__xy_(float4 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
void  float4__xy_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float4__xz_(float4 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
void  float4__xz_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float4__xw_(float4 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
void  float4__xw_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
}
float2  float4__yx_(float4 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
void  float4__yx_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float4__yy_(float4 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float2  float4__yz_(float4 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
void  float4__yz_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float4__yw_(float4 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
void  float4__yw_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
}
float2  float4__zx_(float4 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
void  float4__zx_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float4__zy_(float4 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
void  float4__zy_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float4__zz_(float4 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float2  float4__zw_(float4 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
void  float4__zw_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
}
float2  float4__wx_(float4 * __context)
{
	return float2__(*(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
void  float4__wx_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float4__wy_(float4 * __context)
{
	return float2__(*(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
void  float4__wy_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float4__wz_(float4 * __context)
{
	return float2__(*(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
void  float4__wz_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float4__ww_(float4 * __context)
{
	return float2__(*(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float3  float4__xxx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float3  float4__xxy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
float3  float4__xxz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
float3  float4__xxw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
float3  float4__xyx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
float3  float4__xyy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float3  float4__xyz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
void  float4__xyz_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__xyw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
void  float4__xyw_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__xzx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
float3  float4__xzy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
void  float4__xzy_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__xzz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float4__xzw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
void  float4__xzw_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__xwx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
float3  float4__xwy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
void  float4__xwy_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__xwz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
void  float4__xwz_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__xww_(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float3  float4__yxx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float3  float4__yxy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
float3  float4__yxz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
void  float4__yxz_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__yxw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
void  float4__yxw_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__yyx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
float3  float4__yyy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float3  float4__yyz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
float3  float4__yyw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
float3  float4__yzx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
void  float4__yzx_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__yzy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
float3  float4__yzz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float4__yzw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
void  float4__yzw_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__ywx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
void  float4__ywx_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__ywy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
float3  float4__ywz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
void  float4__ywz_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__yww_(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float3  float4__zxx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float3  float4__zxy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
void  float4__zxy_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__zxz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
float3  float4__zxw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
void  float4__zxw_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__zyx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
void  float4__zyx_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__zyy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float3  float4__zyz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
float3  float4__zyw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
void  float4__zyw_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__zzx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
float3  float4__zzy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
float3  float4__zzz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float4__zzw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
float3  float4__zwx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
void  float4__zwx_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__zwy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
void  float4__zwy_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__zwz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
float3  float4__zww_(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float3  float4__wxx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float3  float4__wxy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
void  float4__wxy_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__wxz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
void  float4__wxz_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__wxw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
float3  float4__wyx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
void  float4__wyx_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__wyy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float3  float4__wyz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
void  float4__wyz_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__wyw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
float3  float4__wzx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
void  float4__wzx_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__wzy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
void  float4__wzy_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__wzz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float4__wzw_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
float3  float4__wwx_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
float3  float4__wwy_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
float3  float4__wwz_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
float3  float4__www_(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float4  float4__xxxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xxxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xxxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xxxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xxyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xxyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xxyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xxyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xxzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xxzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xxzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xxzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xxwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xxwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xxwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xxww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xyxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xyxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xyxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xyxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xyyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xyyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xyyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xyyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xyzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xyzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xyzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xyzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
void  float4__xyzw_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__xywx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xywy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xywz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
void  float4__xywz_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__xyww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xzxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xzxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xzxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xzxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xzyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xzyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xzyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xzyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
void  float4__xzyw_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__xzzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xzzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xzzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xzzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xzwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xzwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
void  float4__xzwy_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__xzwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xzww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xwxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xwxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xwxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xwxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xwyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xwyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xwyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
void  float4__xwyz_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__xwyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xwzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xwzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
void  float4__xwzy_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__xwzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xwzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xwwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xwwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xwwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xwww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yxxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yxxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yxxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yxxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yxyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yxyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yxyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yxyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yxzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yxzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yxzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yxzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
void  float4__yxzw_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__yxwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yxwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yxwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
void  float4__yxwz_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__yxww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yyxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yyxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yyxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yyxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yyyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yyyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yyyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yyyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yyzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yyzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yyzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yyzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yywx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yywy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yywz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yyww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yzxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yzxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yzxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yzxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
void  float4__yzxw_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__yzyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yzyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yzyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yzyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yzzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yzzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yzzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yzzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yzwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
void  float4__yzwx_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__yzwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yzwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yzww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__ywxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__ywxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__ywxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
void  float4__ywxz_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__ywxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__ywyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__ywyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__ywyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__ywyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__ywzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
void  float4__ywzx_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__ywzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__ywzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__ywzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__ywwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__ywwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__ywwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__ywww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zxxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zxxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zxxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zxxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zxyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zxyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zxyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zxyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
void  float4__zxyw_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__zxzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zxzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zxzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zxzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zxwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zxwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
void  float4__zxwy_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__zxwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zxww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zyxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zyxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zyxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zyxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
void  float4__zyxw_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__zyyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zyyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zyyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zyyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zyzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zyzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zyzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zyzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zywx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
void  float4__zywx_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__zywy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zywz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zyww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zzxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zzxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zzxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zzxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zzyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zzyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zzyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zzyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zzzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zzzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zzzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zzzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zzwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zzwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zzwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zzww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zwxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zwxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
void  float4__zwxy_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__zwxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zwxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zwyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
void  float4__zwyx_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__zwyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zwyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zwyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zwzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zwzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zwzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zwzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zwwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zwwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zwwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zwww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wxxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wxxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wxxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wxxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wxyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wxyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wxyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
void  float4__wxyz_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__wxyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wxzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wxzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
void  float4__wxzy_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__wxzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wxzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wxwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wxwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wxwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wxww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wyxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wyxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wyxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
void  float4__wyxz_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__wyxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wyyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wyyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wyyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wyyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wyzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
void  float4__wyzx_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__wyzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wyzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wyzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wywx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wywy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wywz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wyww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wzxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wzxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
void  float4__wzxy_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__wzxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wzxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wzyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
void  float4__wzyx_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__wzyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wzyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wzyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wzzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wzzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wzzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wzzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wzwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wzwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wzwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wzww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wwxx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wwxy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wwxz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wwxw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wwyx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wwyy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wwyz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wwyw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wwzx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wwzy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wwzz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wwzw_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wwwx_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wwwy_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wwwz_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wwww_(float4 * __context)
{
	return float4__float4(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__(float x_0, float y_4, float z_8, float w_12, void* unused)
{
	float4 ret_20;
	/* node translation unknown */
	*(&(&ret_20)->x) = *(&x_0);
	*(&(&ret_20)->y) = *(&y_4);
	*(&(&ret_20)->z) = *(&z_8);
	*(&(&ret_20)->w) = *(&w_12);
	return *(&ret_20);
}
float4  __operatorAdd(float4 a_0, float4 b_16, void* unused)
{
	return float4__((*(&(&a_0)->x)) + (*(&(&b_16)->x)), (*(&(&a_0)->y)) + (*(&(&b_16)->y)), (*(&(&a_0)->z)) + (*(&(&b_16)->z)), (*(&(&a_0)->w)) + (*(&(&b_16)->w)), (void*)0);
}
float4  __operatorSub(float4 a_0, float4 b_16, void* unused)
{
	return float4__((*(&(&a_0)->x)) - (*(&(&b_16)->x)), (*(&(&a_0)->y)) - (*(&(&b_16)->y)), (*(&(&a_0)->z)) - (*(&(&b_16)->z)), (*(&(&a_0)->w)) - (*(&(&b_16)->w)), (void*)0);
}
float4  __operatorMul(float4 a_0, float4 b_16, void* unused)
{
	return float4__((*(&(&a_0)->x)) * (*(&(&b_16)->x)), (*(&(&a_0)->y)) * (*(&(&b_16)->y)), (*(&(&a_0)->z)) * (*(&(&b_16)->z)), (*(&(&a_0)->w)) * (*(&(&b_16)->w)), (void*)0);
}
float4  __operatorDiv(float4 a_0, float4 b_16, void* unused)
{
	return float4__((*(&(&a_0)->x)) / (*(&(&b_16)->x)), (*(&(&a_0)->y)) / (*(&(&b_16)->y)), (*(&(&a_0)->z)) / (*(&(&b_16)->z)), (*(&(&a_0)->w)) / (*(&(&b_16)->w)), (void*)0);
}
float4  __operatorMul(float4 a_0, float b_16, void* unused)
{
	return float4__((*(&(&a_0)->x)) * (*(&b_16)), (*(&(&a_0)->y)) * (*(&b_16)), (*(&(&a_0)->z)) * (*(&b_16)), (*(&(&a_0)->w)) * (*(&b_16)), (void*)0);
}
float4  __operatorMul(float a_0, float4 b_4, void* unused)
{
	return float4__((*(&a_0)) * (*(&(&b_4)->x)), (*(&a_0)) * (*(&(&b_4)->y)), (*(&a_0)) * (*(&(&b_4)->z)), (*(&a_0)) * (*(&(&b_4)->w)), (void*)0);
}
float4  __operatorDiv(float4 a_0, float b_16, void* unused)
{
	return float4__((*(&(&a_0)->x)) / (*(&b_16)), (*(&(&a_0)->y)) / (*(&b_16)), (*(&(&a_0)->z)) / (*(&b_16)), (*(&(&a_0)->w)) / (*(&b_16)), (void*)0);
}
float4 *  __operatorAddSet(float4 * a_0, float4 b_4, void* unused)
{
	*(&(*(&a_0))->x) += *(&(&b_4)->x);
	*(&(*(&a_0))->y) += *(&(&b_4)->y);
	*(&(*(&a_0))->z) += *(&(&b_4)->z);
	*(&(*(&a_0))->w) += *(&(&b_4)->w);
	return *(&a_0);
}
float4 *  __operatorSubSet(float4 * a_0, float4 b_4, void* unused)
{
	*(&(*(&a_0))->x) -= *(&(&b_4)->x);
	*(&(*(&a_0))->y) -= *(&(&b_4)->y);
	*(&(*(&a_0))->z) -= *(&(&b_4)->z);
	*(&(*(&a_0))->w) -= *(&(&b_4)->w);
	return *(&a_0);
}
float4 *  __operatorMulSet(float4 * a_0, float b_4, void* unused)
{
	*(&(*(&a_0))->x) *= *(&b_4);
	*(&(*(&a_0))->y) *= *(&b_4);
	*(&(*(&a_0))->z) *= *(&b_4);
	*(&(*(&a_0))->w) *= *(&b_4);
	return *(&a_0);
}
float4 *  __operatorDivSet(float4 * a_0, float b_4, void* unused)
{
	*(&(*(&a_0))->x) /= *(&b_4);
	*(&(*(&a_0))->y) /= *(&b_4);
	*(&(*(&a_0))->z) /= *(&b_4);
	*(&(*(&a_0))->w) /= *(&b_4);
	return *(&a_0);
}
float3  float3__(float2 xy_0, float z_8, void* unused)
{
	return float3__(*(&(&xy_0)->x), *(&(&xy_0)->y), *(&z_8), (void*)0);
}
float3  float3__(float x_0, float2 yz_4, void* unused)
{
	return float3__(*(&x_0), *(&(&yz_4)->x), *(&(&yz_4)->y), (void*)0);
}
float4  float4__(float3 xyz_0, float w_12, void* unused)
{
	return float4__(*(&(&xyz_0)->x), *(&(&xyz_0)->y), *(&(&xyz_0)->z), *(&w_12), (void*)0);
}
float4  float4__(float2 xy_0, float2 zw_8, void* unused)
{
	return float4__(*(&(&xy_0)->x), *(&(&xy_0)->y), *(&(&zw_8)->x), *(&(&zw_8)->y), (void*)0);
}
float4  float4__(float2 xy_0, float z_8, float w_12, void* unused)
{
	return float4__(*(&(&xy_0)->x), *(&(&xy_0)->y), *(&z_8), *(&w_12), (void*)0);
}
float4  float4__(float x_0, float y_4, float2 zw_8, void* unused)
{
	return float4__(*(&x_0), *(&y_4), *(&(&zw_8)->x), *(&(&zw_8)->y), (void*)0);
}
float4  float4__(float x_0, float3 yzw_4, void* unused)
{
	return float4__(*(&x_0), *(&(&yzw_4)->x), *(&(&yzw_4)->y), *(&(&yzw_4)->z), (void*)0);
}
float4 *  __operatorSet(float4 * a_0, double_4_ xyzw_4, void* unused)
{
	*(*(&a_0)) = float4__((float )(*(&(&xyzw_4)->ptr[__nullcIndex(0, 4u)])), (float )(*(&(&xyzw_4)->ptr[__nullcIndex(1, 4u)])), (float )(*(&(&xyzw_4)->ptr[__nullcIndex(2, 4u)])), (float )(*(&(&xyzw_4)->ptr[__nullcIndex(3, 4u)])), (void*)0);
	return *(&a_0);
}
float3  reflect(float3 normal_0, float3 dir_12, void* unused)
{
	/* node translation unknown */
}
int initStdMath()
{
	__nullcTR[0] = __nullcRegisterType(2090838615u, "void", 0, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[1] = __nullcRegisterType(4181547808u, "double", 8, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[2] = __nullcRegisterType(259121563u, "float", 4, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[3] = __nullcRegisterType(2090479413u, "long", 8, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[4] = __nullcRegisterType(193495088u, "int", 4, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[5] = __nullcRegisterType(274395349u, "short", 2, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[6] = __nullcRegisterType(2090147939u, "char", 1, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[7] = __nullcRegisterType(1166360283u, "auto ref", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[8] = __nullcRegisterType(524429492u, "typeid", 4, __nullcTR[0], 1, NULLC_CLASS);
	__nullcTR[9] = __nullcRegisterType(3198057556u, "void ref", 4, __nullcTR[0], 1, NULLC_POINTER);
	__nullcTR[10] = __nullcRegisterType(3150998963u, "auto ref[]", 8, __nullcTR[7], -1, NULLC_ARRAY);
	__nullcTR[11] = __nullcRegisterType(2550963152u, "void ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[12] = __nullcRegisterType(4133409083u, "char[]", 8, __nullcTR[6], -1, NULLC_ARRAY);
	__nullcTR[13] = __nullcRegisterType(3878423506u, "void ref(int,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[14] = __nullcRegisterType(1362586038u, "int ref(char[],char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[15] = __nullcRegisterType(3953727713u, "char[] ref(char[],char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[16] = __nullcRegisterType(3214832952u, "char[] ref", 4, __nullcTR[12], 1, NULLC_POINTER);
	__nullcTR[17] = __nullcRegisterType(90259294u, "char[] ref(char[] ref,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[18] = __nullcRegisterType(3410585167u, "char ref(char)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[19] = __nullcRegisterType(1890834067u, "short ref(short)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[20] = __nullcRegisterType(2745832905u, "int ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[21] = __nullcRegisterType(3458960563u, "long ref(long)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[22] = __nullcRegisterType(4223928607u, "float ref(float)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[23] = __nullcRegisterType(4226161577u, "double ref(double)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[24] = __nullcRegisterType(554739849u, "char[] ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[25] = __nullcRegisterType(2528639597u, "void ref ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[26] = __nullcRegisterType(262756424u, "int[]", 8, __nullcTR[4], -1, NULLC_ARRAY);
	__nullcTR[27] = __nullcRegisterType(1780011448u, "int[] ref(int,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[28] = __nullcRegisterType(3724107199u, "auto ref ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[29] = __nullcRegisterType(3761170085u, "void ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[30] = __nullcRegisterType(4190091973u, "int[] ref", 4, __nullcTR[26], 1, NULLC_POINTER);
	__nullcTR[31] = __nullcRegisterType(844911189u, "void ref() ref(auto ref,int[] ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[32] = __nullcRegisterType(2671810221u, "int ref", 4, __nullcTR[4], 1, NULLC_POINTER);
	__nullcTR[33] = __nullcRegisterType(2816069557u, "char[] ref ref", 4, __nullcTR[16], 1, NULLC_POINTER);
	__nullcTR[34] = __nullcRegisterType(154671200u, "char ref", 4, __nullcTR[6], 1, NULLC_POINTER);
	__nullcTR[35] = __nullcRegisterType(2985493640u, "char[] ref ref(char[] ref,int[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[36] = __nullcRegisterType(2463794733u, "short[]", 8, __nullcTR[5], -1, NULLC_ARRAY);
	__nullcTR[37] = __nullcRegisterType(3958330154u, "short[] ref", 4, __nullcTR[36], 1, NULLC_POINTER);
	__nullcTR[38] = __nullcRegisterType(745297575u, "short[] ref ref", 4, __nullcTR[37], 1, NULLC_POINTER);
	__nullcTR[39] = __nullcRegisterType(3010777554u, "short ref", 4, __nullcTR[5], 1, NULLC_POINTER);
	__nullcTR[40] = __nullcRegisterType(1935747820u, "short[] ref ref(short[] ref,int[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[41] = __nullcRegisterType(3010510963u, "float[]", 8, __nullcTR[2], -1, NULLC_ARRAY);
	__nullcTR[42] = __nullcRegisterType(2248491120u, "float[] ref", 4, __nullcTR[41], 1, NULLC_POINTER);
	__nullcTR[43] = __nullcRegisterType(1040232248u, "double[]", 8, __nullcTR[1], -1, NULLC_ARRAY);
	__nullcTR[44] = __nullcRegisterType(2393077485u, "float[] ref ref", 4, __nullcTR[42], 1, NULLC_POINTER);
	__nullcTR[45] = __nullcRegisterType(2697529781u, "double[] ref", 4, __nullcTR[43], 1, NULLC_POINTER);
	__nullcTR[46] = __nullcRegisterType(1384297880u, "float ref", 4, __nullcTR[2], 1, NULLC_POINTER);
	__nullcTR[47] = __nullcRegisterType(3234425245u, "double ref", 4, __nullcTR[1], 1, NULLC_POINTER);
	__nullcTR[48] = __nullcRegisterType(2467461000u, "float[] ref ref(float[] ref,double[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[49] = __nullcRegisterType(6493228u, "MathConstants", 16, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[50] = __nullcRegisterType(3875326121u, "MathConstants ref", 4, __nullcTR[49], 1, NULLC_POINTER);
	__nullcTR[51] = __nullcRegisterType(1895912087u, "double ref(double,double,double)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[52] = __nullcRegisterType(4256044333u, "float2", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[53] = __nullcRegisterType(2568615123u, "float2 ref(float,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[54] = __nullcRegisterType(2750571050u, "float2 ref", 4, __nullcTR[52], 1, NULLC_POINTER);
	__nullcTR[55] = __nullcRegisterType(1779669499u, "float2 ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[56] = __nullcRegisterType(1029629351u, "float2 ref ref", 4, __nullcTR[54], 1, NULLC_POINTER);
	__nullcTR[57] = __nullcRegisterType(2519331085u, "void ref(float2)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[58] = __nullcRegisterType(2810325943u, "float2 ref(float2,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[59] = __nullcRegisterType(3338924485u, "float2 ref(float2,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[60] = __nullcRegisterType(3159920773u, "float2 ref(float,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[61] = __nullcRegisterType(583224465u, "float2 ref ref(float2 ref,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[62] = __nullcRegisterType(668426079u, "float2 ref ref(float2 ref,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[63] = __nullcRegisterType(4256044334u, "float3", 12, __nullcTR[0], 3, NULLC_CLASS);
	__nullcTR[64] = __nullcRegisterType(3886564502u, "float3 ref(float,float,float)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[65] = __nullcRegisterType(2751756971u, "float3 ref", 4, __nullcTR[63], 1, NULLC_POINTER);
	__nullcTR[66] = __nullcRegisterType(2983941800u, "float3 ref ref", 4, __nullcTR[65], 1, NULLC_POINTER);
	__nullcTR[67] = __nullcRegisterType(3071137468u, "float3 ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[68] = __nullcRegisterType(2519331118u, "void ref(float3)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[69] = __nullcRegisterType(3894790970u, "float3 ref(float3,float3)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[70] = __nullcRegisterType(1679830247u, "float3 ref(float3,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[71] = __nullcRegisterType(1832056551u, "float3 ref(float,float3)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[72] = __nullcRegisterType(4106114452u, "float3 ref ref(float3 ref,float3)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[73] = __nullcRegisterType(1816384513u, "float3 ref ref(float3 ref,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[74] = __nullcRegisterType(4256044335u, "float4", 16, __nullcTR[0], 4, NULLC_CLASS);
	__nullcTR[75] = __nullcRegisterType(247594841u, "float4 ref(float,float,float,float)", 8, __nullcTR[0], 4, NULLC_FUNCTION);
	__nullcTR[76] = __nullcRegisterType(2752942892u, "float4 ref", 4, __nullcTR[74], 1, NULLC_POINTER);
	__nullcTR[77] = __nullcRegisterType(643286953u, "float4 ref ref", 4, __nullcTR[76], 1, NULLC_POINTER);
	__nullcTR[78] = __nullcRegisterType(67638141u, "float4 ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[79] = __nullcRegisterType(2519331151u, "void ref(float4)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[80] = __nullcRegisterType(684288701u, "float4 ref(float4,float4)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[81] = __nullcRegisterType(20736009u, "float4 ref(float4,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[82] = __nullcRegisterType(504192329u, "float4 ref(float,float4)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[83] = __nullcRegisterType(3334037143u, "float4 ref ref(float4 ref,float4)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[84] = __nullcRegisterType(2964342947u, "float4 ref ref(float4 ref,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[85] = __nullcRegisterType(2011060230u, "float3 ref(float2,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[86] = __nullcRegisterType(1832056518u, "float3 ref(float,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[87] = __nullcRegisterType(351965992u, "float4 ref(float3,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[88] = __nullcRegisterType(1070631033u, "float4 ref(float2,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[89] = __nullcRegisterType(3253984809u, "float4 ref(float2,float,float)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[90] = __nullcRegisterType(2294885289u, "float4 ref(float,float,float2)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[91] = __nullcRegisterType(504192296u, "float4 ref(float,float3)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[92] = __nullcRegisterType(4262891852u, "double[4]", 32, __nullcTR[1], 4, NULLC_ARRAY);
	__nullcTR[93] = __nullcRegisterType(3368457716u, "float4 ref ref(float4 ref,double[4])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[94] = __nullcRegisterType(1583994313u, "double[4] ref", 4, __nullcTR[92], 1, NULLC_POINTER);
	__nullcTR[95] = __nullcRegisterType(562572443u, "float4x4", 64, __nullcTR[0], 4, NULLC_CLASS);
	__nullcTR[96] = __nullcRegisterType(3309272130u, "float ref ref(float2 ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[97] = __nullcRegisterType(3377073507u, "float ref ref(float3 ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[98] = __nullcRegisterType(3444874884u, "float ref ref(float4 ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[99] = __nullcRegisterType(4261839081u, "float ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[100] = __nullcRegisterType(2081580927u, "float ref(float2 ref,float2 ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[101] = __nullcRegisterType(289501729u, "float ref(float3 ref,float3 ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[102] = __nullcRegisterType(2792389827u, "float ref(float4 ref,float4 ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	/* node translation unknown */
	/* node translation unknown */
	*(&(&math)->pi) = 3.141593;
	*(&(&math)->e) = 2.718282;
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
}
