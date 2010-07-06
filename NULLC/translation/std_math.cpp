#include "runtime.h"
// Typeid redirect table
static unsigned __nullcTR[141];
// Function pointer redirect table
static void* __nullcFR[584];
// Array classes
struct MathConstants 
{
	double pi;
	double e;
};
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
struct double_4_ 
{
	double ptr[4];
	double_4_ & set(unsigned index, double const & val){ ptr[index] = val; return *this; }
};
struct float4x4 
{
	float4 row1;
	float4 row2;
	float4 row3;
	float4 row4;
};
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
float2 float2__float2_float2_ref_float_float_(float x, float y, float2 * __context);
float2 float2__xx__float2_ref__(float2 * __context);
float2 float2__xy__float2_ref__(float2 * __context);
void float2__xy__void_ref_float2_(float2 r, float2 * __context);
float2 float2__yx__float2_ref__(float2 * __context);
void float2__yx__void_ref_float2_(float2 r, float2 * __context);
float2 float2__yy__float2_ref__(float2 * __context);
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
float3 float3__float3_float3_ref_float_float_float_(float x, float y, float z, float3 * __context);
float2 float3__xx__float2_ref__(float3 * __context);
float2 float3__xy__float2_ref__(float3 * __context);
void float3__xy__void_ref_float2_(float2 r, float3 * __context);
float2 float3__xz__float2_ref__(float3 * __context);
void float3__xz__void_ref_float2_(float2 r, float3 * __context);
float2 float3__yx__float2_ref__(float3 * __context);
void float3__yx__void_ref_float2_(float2 r, float3 * __context);
float2 float3__yy__float2_ref__(float3 * __context);
float2 float3__yz__float2_ref__(float3 * __context);
void float3__yz__void_ref_float2_(float2 r, float3 * __context);
float2 float3__zx__float2_ref__(float3 * __context);
void float3__zx__void_ref_float2_(float2 r, float3 * __context);
float2 float3__zy__float2_ref__(float3 * __context);
void float3__zy__void_ref_float2_(float2 r, float3 * __context);
float2 float3__zz__float2_ref__(float3 * __context);
float3 float3__xxx__float3_ref__(float3 * __context);
float3 float3__xxy__float3_ref__(float3 * __context);
float3 float3__xxz__float3_ref__(float3 * __context);
float3 float3__xyx__float3_ref__(float3 * __context);
float3 float3__xyy__float3_ref__(float3 * __context);
float3 float3__xyz__float3_ref__(float3 * __context);
void float3__xyz__void_ref_float3_(float3 r, float3 * __context);
float3 float3__xzx__float3_ref__(float3 * __context);
float3 float3__xzy__float3_ref__(float3 * __context);
void float3__xzy__void_ref_float3_(float3 r, float3 * __context);
float3 float3__xzz__float3_ref__(float3 * __context);
float3 float3__yxx__float3_ref__(float3 * __context);
float3 float3__yxy__float3_ref__(float3 * __context);
float3 float3__yxz__float3_ref__(float3 * __context);
void float3__yxz__void_ref_float3_(float3 r, float3 * __context);
float3 float3__yyx__float3_ref__(float3 * __context);
float3 float3__yyy__float3_ref__(float3 * __context);
float3 float3__yyz__float3_ref__(float3 * __context);
float3 float3__yzx__float3_ref__(float3 * __context);
void float3__yzx__void_ref_float3_(float3 r, float3 * __context);
float3 float3__yzy__float3_ref__(float3 * __context);
float3 float3__yzz__float3_ref__(float3 * __context);
float3 float3__zxx__float3_ref__(float3 * __context);
float3 float3__zxy__float3_ref__(float3 * __context);
void float3__zxy__void_ref_float3_(float3 r, float3 * __context);
float3 float3__zxz__float3_ref__(float3 * __context);
float3 float3__zyx__float3_ref__(float3 * __context);
void float3__zyx__void_ref_float3_(float3 r, float3 * __context);
float3 float3__zyy__float3_ref__(float3 * __context);
float3 float3__zyz__float3_ref__(float3 * __context);
float3 float3__zzx__float3_ref__(float3 * __context);
float3 float3__zzy__float3_ref__(float3 * __context);
float3 float3__zzz__float3_ref__(float3 * __context);
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
float4 float4__float4_float4_ref_float_float_float_float_(float x, float y, float z, float w, float4 * __context);
float2 float4__xx__float2_ref__(float4 * __context);
float2 float4__xy__float2_ref__(float4 * __context);
void float4__xy__void_ref_float2_(float2 r, float4 * __context);
float2 float4__xz__float2_ref__(float4 * __context);
void float4__xz__void_ref_float2_(float2 r, float4 * __context);
float2 float4__xw__float2_ref__(float4 * __context);
void float4__xw__void_ref_float2_(float2 r, float4 * __context);
float2 float4__yx__float2_ref__(float4 * __context);
void float4__yx__void_ref_float2_(float2 r, float4 * __context);
float2 float4__yy__float2_ref__(float4 * __context);
float2 float4__yz__float2_ref__(float4 * __context);
void float4__yz__void_ref_float2_(float2 r, float4 * __context);
float2 float4__yw__float2_ref__(float4 * __context);
void float4__yw__void_ref_float2_(float2 r, float4 * __context);
float2 float4__zx__float2_ref__(float4 * __context);
void float4__zx__void_ref_float2_(float2 r, float4 * __context);
float2 float4__zy__float2_ref__(float4 * __context);
void float4__zy__void_ref_float2_(float2 r, float4 * __context);
float2 float4__zz__float2_ref__(float4 * __context);
float2 float4__zw__float2_ref__(float4 * __context);
void float4__zw__void_ref_float2_(float2 r, float4 * __context);
float2 float4__wx__float2_ref__(float4 * __context);
void float4__wx__void_ref_float2_(float2 r, float4 * __context);
float2 float4__wy__float2_ref__(float4 * __context);
void float4__wy__void_ref_float2_(float2 r, float4 * __context);
float2 float4__wz__float2_ref__(float4 * __context);
void float4__wz__void_ref_float2_(float2 r, float4 * __context);
float2 float4__ww__float2_ref__(float4 * __context);
float3 float4__xxx__float3_ref__(float4 * __context);
float3 float4__xxy__float3_ref__(float4 * __context);
float3 float4__xxz__float3_ref__(float4 * __context);
float3 float4__xxw__float3_ref__(float4 * __context);
float3 float4__xyx__float3_ref__(float4 * __context);
float3 float4__xyy__float3_ref__(float4 * __context);
float3 float4__xyz__float3_ref__(float4 * __context);
void float4__xyz__void_ref_float3_(float3 r, float4 * __context);
float3 float4__xyw__float3_ref__(float4 * __context);
void float4__xyw__void_ref_float3_(float3 r, float4 * __context);
float3 float4__xzx__float3_ref__(float4 * __context);
float3 float4__xzy__float3_ref__(float4 * __context);
void float4__xzy__void_ref_float3_(float3 r, float4 * __context);
float3 float4__xzz__float3_ref__(float4 * __context);
float3 float4__xzw__float3_ref__(float4 * __context);
void float4__xzw__void_ref_float3_(float3 r, float4 * __context);
float3 float4__xwx__float3_ref__(float4 * __context);
float3 float4__xwy__float3_ref__(float4 * __context);
void float4__xwy__void_ref_float3_(float3 r, float4 * __context);
float3 float4__xwz__float3_ref__(float4 * __context);
void float4__xwz__void_ref_float3_(float3 r, float4 * __context);
float3 float4__xww__float3_ref__(float4 * __context);
float3 float4__yxx__float3_ref__(float4 * __context);
float3 float4__yxy__float3_ref__(float4 * __context);
float3 float4__yxz__float3_ref__(float4 * __context);
void float4__yxz__void_ref_float3_(float3 r, float4 * __context);
float3 float4__yxw__float3_ref__(float4 * __context);
void float4__yxw__void_ref_float3_(float3 r, float4 * __context);
float3 float4__yyx__float3_ref__(float4 * __context);
float3 float4__yyy__float3_ref__(float4 * __context);
float3 float4__yyz__float3_ref__(float4 * __context);
float3 float4__yyw__float3_ref__(float4 * __context);
float3 float4__yzx__float3_ref__(float4 * __context);
void float4__yzx__void_ref_float3_(float3 r, float4 * __context);
float3 float4__yzy__float3_ref__(float4 * __context);
float3 float4__yzz__float3_ref__(float4 * __context);
float3 float4__yzw__float3_ref__(float4 * __context);
void float4__yzw__void_ref_float3_(float3 r, float4 * __context);
float3 float4__ywx__float3_ref__(float4 * __context);
void float4__ywx__void_ref_float3_(float3 r, float4 * __context);
float3 float4__ywy__float3_ref__(float4 * __context);
float3 float4__ywz__float3_ref__(float4 * __context);
void float4__ywz__void_ref_float3_(float3 r, float4 * __context);
float3 float4__yww__float3_ref__(float4 * __context);
float3 float4__zxx__float3_ref__(float4 * __context);
float3 float4__zxy__float3_ref__(float4 * __context);
void float4__zxy__void_ref_float3_(float3 r, float4 * __context);
float3 float4__zxz__float3_ref__(float4 * __context);
float3 float4__zxw__float3_ref__(float4 * __context);
void float4__zxw__void_ref_float3_(float3 r, float4 * __context);
float3 float4__zyx__float3_ref__(float4 * __context);
void float4__zyx__void_ref_float3_(float3 r, float4 * __context);
float3 float4__zyy__float3_ref__(float4 * __context);
float3 float4__zyz__float3_ref__(float4 * __context);
float3 float4__zyw__float3_ref__(float4 * __context);
void float4__zyw__void_ref_float3_(float3 r, float4 * __context);
float3 float4__zzx__float3_ref__(float4 * __context);
float3 float4__zzy__float3_ref__(float4 * __context);
float3 float4__zzz__float3_ref__(float4 * __context);
float3 float4__zzw__float3_ref__(float4 * __context);
float3 float4__zwx__float3_ref__(float4 * __context);
void float4__zwx__void_ref_float3_(float3 r, float4 * __context);
float3 float4__zwy__float3_ref__(float4 * __context);
void float4__zwy__void_ref_float3_(float3 r, float4 * __context);
float3 float4__zwz__float3_ref__(float4 * __context);
float3 float4__zww__float3_ref__(float4 * __context);
float3 float4__wxx__float3_ref__(float4 * __context);
float3 float4__wxy__float3_ref__(float4 * __context);
void float4__wxy__void_ref_float3_(float3 r, float4 * __context);
float3 float4__wxz__float3_ref__(float4 * __context);
void float4__wxz__void_ref_float3_(float3 r, float4 * __context);
float3 float4__wxw__float3_ref__(float4 * __context);
float3 float4__wyx__float3_ref__(float4 * __context);
void float4__wyx__void_ref_float3_(float3 r, float4 * __context);
float3 float4__wyy__float3_ref__(float4 * __context);
float3 float4__wyz__float3_ref__(float4 * __context);
void float4__wyz__void_ref_float3_(float3 r, float4 * __context);
float3 float4__wyw__float3_ref__(float4 * __context);
float3 float4__wzx__float3_ref__(float4 * __context);
void float4__wzx__void_ref_float3_(float3 r, float4 * __context);
float3 float4__wzy__float3_ref__(float4 * __context);
void float4__wzy__void_ref_float3_(float3 r, float4 * __context);
float3 float4__wzz__float3_ref__(float4 * __context);
float3 float4__wzw__float3_ref__(float4 * __context);
float3 float4__wwx__float3_ref__(float4 * __context);
float3 float4__wwy__float3_ref__(float4 * __context);
float3 float4__wwz__float3_ref__(float4 * __context);
float3 float4__www__float3_ref__(float4 * __context);
float4 float4__xxxx__float4_ref__(float4 * __context);
float4 float4__xxxy__float4_ref__(float4 * __context);
float4 float4__xxxz__float4_ref__(float4 * __context);
float4 float4__xxxw__float4_ref__(float4 * __context);
float4 float4__xxyx__float4_ref__(float4 * __context);
float4 float4__xxyy__float4_ref__(float4 * __context);
float4 float4__xxyz__float4_ref__(float4 * __context);
float4 float4__xxyw__float4_ref__(float4 * __context);
float4 float4__xxzx__float4_ref__(float4 * __context);
float4 float4__xxzy__float4_ref__(float4 * __context);
float4 float4__xxzz__float4_ref__(float4 * __context);
float4 float4__xxzw__float4_ref__(float4 * __context);
float4 float4__xxwx__float4_ref__(float4 * __context);
float4 float4__xxwy__float4_ref__(float4 * __context);
float4 float4__xxwz__float4_ref__(float4 * __context);
float4 float4__xxww__float4_ref__(float4 * __context);
float4 float4__xyxx__float4_ref__(float4 * __context);
float4 float4__xyxy__float4_ref__(float4 * __context);
float4 float4__xyxz__float4_ref__(float4 * __context);
float4 float4__xyxw__float4_ref__(float4 * __context);
float4 float4__xyyx__float4_ref__(float4 * __context);
float4 float4__xyyy__float4_ref__(float4 * __context);
float4 float4__xyyz__float4_ref__(float4 * __context);
float4 float4__xyyw__float4_ref__(float4 * __context);
float4 float4__xyzx__float4_ref__(float4 * __context);
float4 float4__xyzy__float4_ref__(float4 * __context);
float4 float4__xyzz__float4_ref__(float4 * __context);
float4 float4__xyzw__float4_ref__(float4 * __context);
void float4__xyzw__void_ref_float4_(float4 r, float4 * __context);
float4 float4__xywx__float4_ref__(float4 * __context);
float4 float4__xywy__float4_ref__(float4 * __context);
float4 float4__xywz__float4_ref__(float4 * __context);
void float4__xywz__void_ref_float4_(float4 r, float4 * __context);
float4 float4__xyww__float4_ref__(float4 * __context);
float4 float4__xzxx__float4_ref__(float4 * __context);
float4 float4__xzxy__float4_ref__(float4 * __context);
float4 float4__xzxz__float4_ref__(float4 * __context);
float4 float4__xzxw__float4_ref__(float4 * __context);
float4 float4__xzyx__float4_ref__(float4 * __context);
float4 float4__xzyy__float4_ref__(float4 * __context);
float4 float4__xzyz__float4_ref__(float4 * __context);
float4 float4__xzyw__float4_ref__(float4 * __context);
void float4__xzyw__void_ref_float4_(float4 r, float4 * __context);
float4 float4__xzzx__float4_ref__(float4 * __context);
float4 float4__xzzy__float4_ref__(float4 * __context);
float4 float4__xzzz__float4_ref__(float4 * __context);
float4 float4__xzzw__float4_ref__(float4 * __context);
float4 float4__xzwx__float4_ref__(float4 * __context);
float4 float4__xzwy__float4_ref__(float4 * __context);
void float4__xzwy__void_ref_float4_(float4 r, float4 * __context);
float4 float4__xzwz__float4_ref__(float4 * __context);
float4 float4__xzww__float4_ref__(float4 * __context);
float4 float4__xwxx__float4_ref__(float4 * __context);
float4 float4__xwxy__float4_ref__(float4 * __context);
float4 float4__xwxz__float4_ref__(float4 * __context);
float4 float4__xwxw__float4_ref__(float4 * __context);
float4 float4__xwyx__float4_ref__(float4 * __context);
float4 float4__xwyy__float4_ref__(float4 * __context);
float4 float4__xwyz__float4_ref__(float4 * __context);
void float4__xwyz__void_ref_float4_(float4 r, float4 * __context);
float4 float4__xwyw__float4_ref__(float4 * __context);
float4 float4__xwzx__float4_ref__(float4 * __context);
float4 float4__xwzy__float4_ref__(float4 * __context);
void float4__xwzy__void_ref_float4_(float4 r, float4 * __context);
float4 float4__xwzz__float4_ref__(float4 * __context);
float4 float4__xwzw__float4_ref__(float4 * __context);
float4 float4__xwwx__float4_ref__(float4 * __context);
float4 float4__xwwy__float4_ref__(float4 * __context);
float4 float4__xwwz__float4_ref__(float4 * __context);
float4 float4__xwww__float4_ref__(float4 * __context);
float4 float4__yxxx__float4_ref__(float4 * __context);
float4 float4__yxxy__float4_ref__(float4 * __context);
float4 float4__yxxz__float4_ref__(float4 * __context);
float4 float4__yxxw__float4_ref__(float4 * __context);
float4 float4__yxyx__float4_ref__(float4 * __context);
float4 float4__yxyy__float4_ref__(float4 * __context);
float4 float4__yxyz__float4_ref__(float4 * __context);
float4 float4__yxyw__float4_ref__(float4 * __context);
float4 float4__yxzx__float4_ref__(float4 * __context);
float4 float4__yxzy__float4_ref__(float4 * __context);
float4 float4__yxzz__float4_ref__(float4 * __context);
float4 float4__yxzw__float4_ref__(float4 * __context);
void float4__yxzw__void_ref_float4_(float4 r, float4 * __context);
float4 float4__yxwx__float4_ref__(float4 * __context);
float4 float4__yxwy__float4_ref__(float4 * __context);
float4 float4__yxwz__float4_ref__(float4 * __context);
void float4__yxwz__void_ref_float4_(float4 r, float4 * __context);
float4 float4__yxww__float4_ref__(float4 * __context);
float4 float4__yyxx__float4_ref__(float4 * __context);
float4 float4__yyxy__float4_ref__(float4 * __context);
float4 float4__yyxz__float4_ref__(float4 * __context);
float4 float4__yyxw__float4_ref__(float4 * __context);
float4 float4__yyyx__float4_ref__(float4 * __context);
float4 float4__yyyy__float4_ref__(float4 * __context);
float4 float4__yyyz__float4_ref__(float4 * __context);
float4 float4__yyyw__float4_ref__(float4 * __context);
float4 float4__yyzx__float4_ref__(float4 * __context);
float4 float4__yyzy__float4_ref__(float4 * __context);
float4 float4__yyzz__float4_ref__(float4 * __context);
float4 float4__yyzw__float4_ref__(float4 * __context);
float4 float4__yywx__float4_ref__(float4 * __context);
float4 float4__yywy__float4_ref__(float4 * __context);
float4 float4__yywz__float4_ref__(float4 * __context);
float4 float4__yyww__float4_ref__(float4 * __context);
float4 float4__yzxx__float4_ref__(float4 * __context);
float4 float4__yzxy__float4_ref__(float4 * __context);
float4 float4__yzxz__float4_ref__(float4 * __context);
float4 float4__yzxw__float4_ref__(float4 * __context);
void float4__yzxw__void_ref_float4_(float4 r, float4 * __context);
float4 float4__yzyx__float4_ref__(float4 * __context);
float4 float4__yzyy__float4_ref__(float4 * __context);
float4 float4__yzyz__float4_ref__(float4 * __context);
float4 float4__yzyw__float4_ref__(float4 * __context);
float4 float4__yzzx__float4_ref__(float4 * __context);
float4 float4__yzzy__float4_ref__(float4 * __context);
float4 float4__yzzz__float4_ref__(float4 * __context);
float4 float4__yzzw__float4_ref__(float4 * __context);
float4 float4__yzwx__float4_ref__(float4 * __context);
void float4__yzwx__void_ref_float4_(float4 r, float4 * __context);
float4 float4__yzwy__float4_ref__(float4 * __context);
float4 float4__yzwz__float4_ref__(float4 * __context);
float4 float4__yzww__float4_ref__(float4 * __context);
float4 float4__ywxx__float4_ref__(float4 * __context);
float4 float4__ywxy__float4_ref__(float4 * __context);
float4 float4__ywxz__float4_ref__(float4 * __context);
void float4__ywxz__void_ref_float4_(float4 r, float4 * __context);
float4 float4__ywxw__float4_ref__(float4 * __context);
float4 float4__ywyx__float4_ref__(float4 * __context);
float4 float4__ywyy__float4_ref__(float4 * __context);
float4 float4__ywyz__float4_ref__(float4 * __context);
float4 float4__ywyw__float4_ref__(float4 * __context);
float4 float4__ywzx__float4_ref__(float4 * __context);
void float4__ywzx__void_ref_float4_(float4 r, float4 * __context);
float4 float4__ywzy__float4_ref__(float4 * __context);
float4 float4__ywzz__float4_ref__(float4 * __context);
float4 float4__ywzw__float4_ref__(float4 * __context);
float4 float4__ywwx__float4_ref__(float4 * __context);
float4 float4__ywwy__float4_ref__(float4 * __context);
float4 float4__ywwz__float4_ref__(float4 * __context);
float4 float4__ywww__float4_ref__(float4 * __context);
float4 float4__zxxx__float4_ref__(float4 * __context);
float4 float4__zxxy__float4_ref__(float4 * __context);
float4 float4__zxxz__float4_ref__(float4 * __context);
float4 float4__zxxw__float4_ref__(float4 * __context);
float4 float4__zxyx__float4_ref__(float4 * __context);
float4 float4__zxyy__float4_ref__(float4 * __context);
float4 float4__zxyz__float4_ref__(float4 * __context);
float4 float4__zxyw__float4_ref__(float4 * __context);
void float4__zxyw__void_ref_float4_(float4 r, float4 * __context);
float4 float4__zxzx__float4_ref__(float4 * __context);
float4 float4__zxzy__float4_ref__(float4 * __context);
float4 float4__zxzz__float4_ref__(float4 * __context);
float4 float4__zxzw__float4_ref__(float4 * __context);
float4 float4__zxwx__float4_ref__(float4 * __context);
float4 float4__zxwy__float4_ref__(float4 * __context);
void float4__zxwy__void_ref_float4_(float4 r, float4 * __context);
float4 float4__zxwz__float4_ref__(float4 * __context);
float4 float4__zxww__float4_ref__(float4 * __context);
float4 float4__zyxx__float4_ref__(float4 * __context);
float4 float4__zyxy__float4_ref__(float4 * __context);
float4 float4__zyxz__float4_ref__(float4 * __context);
float4 float4__zyxw__float4_ref__(float4 * __context);
void float4__zyxw__void_ref_float4_(float4 r, float4 * __context);
float4 float4__zyyx__float4_ref__(float4 * __context);
float4 float4__zyyy__float4_ref__(float4 * __context);
float4 float4__zyyz__float4_ref__(float4 * __context);
float4 float4__zyyw__float4_ref__(float4 * __context);
float4 float4__zyzx__float4_ref__(float4 * __context);
float4 float4__zyzy__float4_ref__(float4 * __context);
float4 float4__zyzz__float4_ref__(float4 * __context);
float4 float4__zyzw__float4_ref__(float4 * __context);
float4 float4__zywx__float4_ref__(float4 * __context);
void float4__zywx__void_ref_float4_(float4 r, float4 * __context);
float4 float4__zywy__float4_ref__(float4 * __context);
float4 float4__zywz__float4_ref__(float4 * __context);
float4 float4__zyww__float4_ref__(float4 * __context);
float4 float4__zzxx__float4_ref__(float4 * __context);
float4 float4__zzxy__float4_ref__(float4 * __context);
float4 float4__zzxz__float4_ref__(float4 * __context);
float4 float4__zzxw__float4_ref__(float4 * __context);
float4 float4__zzyx__float4_ref__(float4 * __context);
float4 float4__zzyy__float4_ref__(float4 * __context);
float4 float4__zzyz__float4_ref__(float4 * __context);
float4 float4__zzyw__float4_ref__(float4 * __context);
float4 float4__zzzx__float4_ref__(float4 * __context);
float4 float4__zzzy__float4_ref__(float4 * __context);
float4 float4__zzzz__float4_ref__(float4 * __context);
float4 float4__zzzw__float4_ref__(float4 * __context);
float4 float4__zzwx__float4_ref__(float4 * __context);
float4 float4__zzwy__float4_ref__(float4 * __context);
float4 float4__zzwz__float4_ref__(float4 * __context);
float4 float4__zzww__float4_ref__(float4 * __context);
float4 float4__zwxx__float4_ref__(float4 * __context);
float4 float4__zwxy__float4_ref__(float4 * __context);
void float4__zwxy__void_ref_float4_(float4 r, float4 * __context);
float4 float4__zwxz__float4_ref__(float4 * __context);
float4 float4__zwxw__float4_ref__(float4 * __context);
float4 float4__zwyx__float4_ref__(float4 * __context);
void float4__zwyx__void_ref_float4_(float4 r, float4 * __context);
float4 float4__zwyy__float4_ref__(float4 * __context);
float4 float4__zwyz__float4_ref__(float4 * __context);
float4 float4__zwyw__float4_ref__(float4 * __context);
float4 float4__zwzx__float4_ref__(float4 * __context);
float4 float4__zwzy__float4_ref__(float4 * __context);
float4 float4__zwzz__float4_ref__(float4 * __context);
float4 float4__zwzw__float4_ref__(float4 * __context);
float4 float4__zwwx__float4_ref__(float4 * __context);
float4 float4__zwwy__float4_ref__(float4 * __context);
float4 float4__zwwz__float4_ref__(float4 * __context);
float4 float4__zwww__float4_ref__(float4 * __context);
float4 float4__wxxx__float4_ref__(float4 * __context);
float4 float4__wxxy__float4_ref__(float4 * __context);
float4 float4__wxxz__float4_ref__(float4 * __context);
float4 float4__wxxw__float4_ref__(float4 * __context);
float4 float4__wxyx__float4_ref__(float4 * __context);
float4 float4__wxyy__float4_ref__(float4 * __context);
float4 float4__wxyz__float4_ref__(float4 * __context);
void float4__wxyz__void_ref_float4_(float4 r, float4 * __context);
float4 float4__wxyw__float4_ref__(float4 * __context);
float4 float4__wxzx__float4_ref__(float4 * __context);
float4 float4__wxzy__float4_ref__(float4 * __context);
void float4__wxzy__void_ref_float4_(float4 r, float4 * __context);
float4 float4__wxzz__float4_ref__(float4 * __context);
float4 float4__wxzw__float4_ref__(float4 * __context);
float4 float4__wxwx__float4_ref__(float4 * __context);
float4 float4__wxwy__float4_ref__(float4 * __context);
float4 float4__wxwz__float4_ref__(float4 * __context);
float4 float4__wxww__float4_ref__(float4 * __context);
float4 float4__wyxx__float4_ref__(float4 * __context);
float4 float4__wyxy__float4_ref__(float4 * __context);
float4 float4__wyxz__float4_ref__(float4 * __context);
void float4__wyxz__void_ref_float4_(float4 r, float4 * __context);
float4 float4__wyxw__float4_ref__(float4 * __context);
float4 float4__wyyx__float4_ref__(float4 * __context);
float4 float4__wyyy__float4_ref__(float4 * __context);
float4 float4__wyyz__float4_ref__(float4 * __context);
float4 float4__wyyw__float4_ref__(float4 * __context);
float4 float4__wyzx__float4_ref__(float4 * __context);
void float4__wyzx__void_ref_float4_(float4 r, float4 * __context);
float4 float4__wyzy__float4_ref__(float4 * __context);
float4 float4__wyzz__float4_ref__(float4 * __context);
float4 float4__wyzw__float4_ref__(float4 * __context);
float4 float4__wywx__float4_ref__(float4 * __context);
float4 float4__wywy__float4_ref__(float4 * __context);
float4 float4__wywz__float4_ref__(float4 * __context);
float4 float4__wyww__float4_ref__(float4 * __context);
float4 float4__wzxx__float4_ref__(float4 * __context);
float4 float4__wzxy__float4_ref__(float4 * __context);
void float4__wzxy__void_ref_float4_(float4 r, float4 * __context);
float4 float4__wzxz__float4_ref__(float4 * __context);
float4 float4__wzxw__float4_ref__(float4 * __context);
float4 float4__wzyx__float4_ref__(float4 * __context);
void float4__wzyx__void_ref_float4_(float4 r, float4 * __context);
float4 float4__wzyy__float4_ref__(float4 * __context);
float4 float4__wzyz__float4_ref__(float4 * __context);
float4 float4__wzyw__float4_ref__(float4 * __context);
float4 float4__wzzx__float4_ref__(float4 * __context);
float4 float4__wzzy__float4_ref__(float4 * __context);
float4 float4__wzzz__float4_ref__(float4 * __context);
float4 float4__wzzw__float4_ref__(float4 * __context);
float4 float4__wzwx__float4_ref__(float4 * __context);
float4 float4__wzwy__float4_ref__(float4 * __context);
float4 float4__wzwz__float4_ref__(float4 * __context);
float4 float4__wzww__float4_ref__(float4 * __context);
float4 float4__wwxx__float4_ref__(float4 * __context);
float4 float4__wwxy__float4_ref__(float4 * __context);
float4 float4__wwxz__float4_ref__(float4 * __context);
float4 float4__wwxw__float4_ref__(float4 * __context);
float4 float4__wwyx__float4_ref__(float4 * __context);
float4 float4__wwyy__float4_ref__(float4 * __context);
float4 float4__wwyz__float4_ref__(float4 * __context);
float4 float4__wwyw__float4_ref__(float4 * __context);
float4 float4__wwzx__float4_ref__(float4 * __context);
float4 float4__wwzy__float4_ref__(float4 * __context);
float4 float4__wwzz__float4_ref__(float4 * __context);
float4 float4__wwzw__float4_ref__(float4 * __context);
float4 float4__wwwx__float4_ref__(float4 * __context);
float4 float4__wwwy__float4_ref__(float4 * __context);
float4 float4__wwwz__float4_ref__(float4 * __context);
float4 float4__wwww__float4_ref__(float4 * __context);
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
float float2__length_float_ref__(float2 * __context);
float float2__normalize_float_ref__(float2 * __context);
float float3__length_float_ref__(float3 * __context);
float float3__normalize_float_ref__(float3 * __context);
float float4__length_float_ref__(float4 * __context);
float float4__normalize_float_ref__(float4 * __context);
float dot(float2 * a, float2 * b, void* unused);
float dot(float3 * a, float3 * b, void* unused);
float dot(float4 * a, float4 * b, void* unused);
MathConstants math;
float2  float2__float2_float2_ref_float_float_(float x_0, float y_4, float2 * __context)
{
	float2 res_12;
	*(&(&res_12)->x) = *(&x_0);
	*(&(&res_12)->y) = *(&y_4);
	return *(&res_12);
}
float2  float2__xx__float2_ref__(float2 * __context)
{
	return float2__float2_float2_ref_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float2  float2__xy__float2_ref__(float2 * __context)
{
	return float2__float2_float2_ref_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
void  float2__xy__void_ref_float2_(float2 r_0, float2 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float2__yx__float2_ref__(float2 * __context)
{
	return float2__float2_float2_ref_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
void  float2__yx__void_ref_float2_(float2 r_0, float2 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float2__yy__float2_ref__(float2 * __context)
{
	return float2__float2_float2_ref_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float2  float2__(float x_0, float y_4, void* unused)
{
	float2 ret_12;
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
float3  float3__float3_float3_ref_float_float_float_(float x_0, float y_4, float z_8, float3 * __context)
{
	float3 res_16;
	*(&(&res_16)->x) = *(&x_0);
	*(&(&res_16)->y) = *(&y_4);
	*(&(&res_16)->z) = *(&z_8);
	return *(&res_16);
}
float2  float3__xx__float2_ref__(float3 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float2  float3__xy__float2_ref__(float3 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
void  float3__xy__void_ref_float2_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float3__xz__float2_ref__(float3 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
void  float3__xz__void_ref_float2_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float3__yx__float2_ref__(float3 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
void  float3__yx__void_ref_float2_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float3__yy__float2_ref__(float3 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float2  float3__yz__float2_ref__(float3 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
void  float3__yz__void_ref_float2_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float3__zx__float2_ref__(float3 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
void  float3__zx__void_ref_float2_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float3__zy__float2_ref__(float3 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
void  float3__zy__void_ref_float2_(float2 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float3__zz__float2_ref__(float3 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float3__xxx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float3  float3__xxy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float3  float3__xxz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float3  float3__xyx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float3  float3__xyy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float3  float3__xyz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
void  float3__xyz__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float3__xzx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float3  float3__xzy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
void  float3__xzy__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float3__xzz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float3  float3__yxx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float3  float3__yxy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float3  float3__yxz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
void  float3__yxz__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float3__yyx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float3  float3__yyy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float3  float3__yyz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float3  float3__yzx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
void  float3__yzx__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float3__yzy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float3  float3__yzz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float3  float3__zxx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float3  float3__zxy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
void  float3__zxy__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float3__zxz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float3  float3__zyx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
void  float3__zyx__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float3__zyy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float3  float3__zyz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float3  float3__zzx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float3  float3__zzy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float3  float3__zzz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float3  float3__(float x_0, float y_4, float z_8, void* unused)
{
	float3 ret_16;
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
float4  float4__float4_float4_ref_float_float_float_float_(float x_0, float y_4, float z_8, float w_12, float4 * __context)
{
	float4 res_20;
	*(&(&res_20)->x) = *(&x_0);
	*(&(&res_20)->y) = *(&y_4);
	*(&(&res_20)->z) = *(&z_8);
	*(&(&res_20)->w) = *(&w_12);
	return *(&res_20);
}
float2  float4__xx__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float2  float4__xy__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
void  float4__xy__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float4__xz__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
void  float4__xz__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float4__xw__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
void  float4__xw__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
}
float2  float4__yx__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
void  float4__yx__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float4__yy__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float2  float4__yz__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
void  float4__yz__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float4__yw__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
void  float4__yw__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
}
float2  float4__zx__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
void  float4__zx__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float4__zy__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
void  float4__zy__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float4__zz__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float2  float4__zw__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
void  float4__zw__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
}
float2  float4__wx__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
void  float4__wx__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float4__wy__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
void  float4__wy__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float4__wz__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
void  float4__wz__void_ref_float2_(float2 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
}
float2  float4__ww__float2_ref__(float4 * __context)
{
	return float2__(*(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float3  float4__xxx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float3  float4__xxy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
float3  float4__xxz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
float3  float4__xxw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
float3  float4__xyx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
float3  float4__xyy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float3  float4__xyz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
void  float4__xyz__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__xyw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
void  float4__xyw__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__xzx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
float3  float4__xzy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
void  float4__xzy__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__xzz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float4__xzw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
void  float4__xzw__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__xwx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
float3  float4__xwy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
void  float4__xwy__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__xwz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
void  float4__xwz__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__xww__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float3  float4__yxx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float3  float4__yxy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
float3  float4__yxz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
void  float4__yxz__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__yxw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
void  float4__yxw__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__yyx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
float3  float4__yyy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float3  float4__yyz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
float3  float4__yyw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
float3  float4__yzx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
void  float4__yzx__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__yzy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
float3  float4__yzz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float4__yzw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
void  float4__yzw__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__ywx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
void  float4__ywx__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__ywy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
float3  float4__ywz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
void  float4__ywz__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__yww__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float3  float4__zxx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float3  float4__zxy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
void  float4__zxy__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__zxz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
float3  float4__zxw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
void  float4__zxw__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__zyx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
void  float4__zyx__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__zyy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float3  float4__zyz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
float3  float4__zyw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
void  float4__zyw__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
}
float3  float4__zzx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
float3  float4__zzy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
float3  float4__zzz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float4__zzw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
float3  float4__zwx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
void  float4__zwx__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__zwy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
void  float4__zwy__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__zwz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
float3  float4__zww__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float3  float4__wxx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), (void*)0);
}
float3  float4__wxy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), (void*)0);
}
void  float4__wxy__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__wxz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), (void*)0);
}
void  float4__wxz__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__wxw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), (void*)0);
}
float3  float4__wyx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), (void*)0);
}
void  float4__wyx__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__wyy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), (void*)0);
}
float3  float4__wyz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), (void*)0);
}
void  float4__wyz__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float4__wyw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), (void*)0);
}
float3  float4__wzx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), (void*)0);
}
void  float4__wzx__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float4__wzy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), (void*)0);
}
void  float4__wzy__void_ref_float3_(float3 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float4__wzz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), (void*)0);
}
float3  float4__wzw__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), (void*)0);
}
float3  float4__wwx__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), (void*)0);
}
float3  float4__wwy__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), (void*)0);
}
float3  float4__wwz__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), (void*)0);
}
float3  float4__www__float3_ref__(float4 * __context)
{
	return float3__(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), (void*)0);
}
float4  float4__xxxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xxxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xxxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xxxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xxyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xxyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xxyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xxyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xxzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xxzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xxzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xxzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xxwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xxwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xxwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xxww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xyxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xyxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xyxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xyxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xyyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xyyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xyyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xyyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xyzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xyzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xyzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xyzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
void  float4__xyzw__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__xywx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xywy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xywz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
void  float4__xywz__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__xyww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xzxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xzxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xzxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xzxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xzyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xzyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xzyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xzyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
void  float4__xzyw__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__xzzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xzzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xzzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xzzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xzwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xzwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
void  float4__xzwy__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__xzwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xzww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xwxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xwxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xwxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xwxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xwyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xwyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xwyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
void  float4__xwyz__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__xwyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xwzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xwzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
void  float4__xwzy__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__xwzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xwzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__xwwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__xwwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__xwwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__xwww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yxxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yxxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yxxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yxxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yxyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yxyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yxyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yxyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yxzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yxzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yxzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yxzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
void  float4__yxzw__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__yxwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yxwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yxwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
void  float4__yxwz__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__yxww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yyxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yyxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yyxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yyxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yyyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yyyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yyyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yyyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yyzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yyzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yyzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yyzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yywx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yywy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yywz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yyww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yzxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yzxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yzxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yzxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
void  float4__yzxw__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__yzyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yzyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yzyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yzyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yzzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__yzzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yzzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yzzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__yzwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
void  float4__yzwx__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__yzwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__yzwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__yzww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__ywxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__ywxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__ywxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
void  float4__ywxz__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__ywxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__ywyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__ywyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__ywyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__ywyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__ywzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
void  float4__ywzx__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__ywzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__ywzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__ywzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__ywwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__ywwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__ywwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__ywww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zxxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zxxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zxxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zxxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zxyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zxyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zxyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zxyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
void  float4__zxyw__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__zxzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zxzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zxzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zxzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zxwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zxwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
void  float4__zxwy__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__zxwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zxww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zyxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zyxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zyxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zyxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
void  float4__zyxw__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->w) = *(&(&r_0)->w);
}
float4  float4__zyyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zyyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zyyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zyyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zyzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zyzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zyzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zyzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zywx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
void  float4__zywx__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->w) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__zywy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zywz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zyww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zzxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zzxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zzxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zzxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zzyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zzyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zzyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zzyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zzzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zzzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zzzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zzzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zzwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zzwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zzwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zzww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zwxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zwxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
void  float4__zwxy__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__zwxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zwxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zwyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
void  float4__zwyx__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->w) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__zwyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zwyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zwyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zwzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zwzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zwzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zwzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__zwwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__zwwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__zwwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__zwww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wxxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wxxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wxxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wxxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wxyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wxyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wxyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
void  float4__wxyz__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__wxyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wxzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wxzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
void  float4__wxzy__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__wxzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wxzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wxwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wxwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wxwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wxww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wyxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wyxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wyxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
void  float4__wyxz__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->z) = *(&(&r_0)->w);
}
float4  float4__wyxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wyyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wyyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wyyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wyyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wyzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
void  float4__wyzx__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__wyzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wyzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wyzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wywx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wywy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wywz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wyww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wzxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wzxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
void  float4__wzxy__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
	*(&(*(&__context))->y) = *(&(&r_0)->w);
}
float4  float4__wzxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wzxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wzyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
void  float4__wzyx__void_ref_float4_(float4 r_0, float4 * __context)
{
	*(&(*(&__context))->w) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
	*(&(*(&__context))->x) = *(&(&r_0)->w);
}
float4  float4__wzyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wzyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wzyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wzzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wzzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wzzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wzzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wzwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wzwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wzwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wzww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wwxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wwxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wwxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wwxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wwyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wwyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wwyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wwyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wwzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wwzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wwzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wwzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&__context));
}
float4  float4__wwwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&__context));
}
float4  float4__wwwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&__context));
}
float4  float4__wwwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&__context));
}
float4  float4__wwww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&__context));
}
float4  float4__(float x_0, float y_4, float z_8, float w_12, void* unused)
{
	float4 ret_20;
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
int __init_std_math_nc()
{
	static int moduleInitialized = 0;
	if(moduleInitialized++)
		return 0;
	__nullcTR[0] = __nullcRegisterType(2090838615u, "void", 0, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[1] = __nullcRegisterType(4181547808u, "double", 8, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[2] = __nullcRegisterType(259121563u, "float", 4, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[3] = __nullcRegisterType(2090479413u, "long", 8, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[4] = __nullcRegisterType(193495088u, "int", 4, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[5] = __nullcRegisterType(274395349u, "short", 2, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[6] = __nullcRegisterType(2090147939u, "char", 1, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[7] = __nullcRegisterType(1166360283u, "auto ref", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[8] = __nullcRegisterType(524429492u, "typeid", 4, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[9] = __nullcRegisterType(3198057556u, "void ref", 4, __nullcTR[0], 1, NULLC_POINTER);
	__nullcTR[10] = __nullcRegisterType(4071234806u, "auto[]", 12, __nullcTR[0], 3, NULLC_CLASS);
	__nullcTR[11] = __nullcRegisterType(3150998963u, "auto ref[]", 8, __nullcTR[7], -1, NULLC_ARRAY);
	__nullcTR[12] = __nullcRegisterType(2550963152u, "void ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[13] = __nullcRegisterType(4133409083u, "char[]", 8, __nullcTR[6], -1, NULLC_ARRAY);
	__nullcTR[14] = __nullcRegisterType(3878423506u, "void ref(int,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[15] = __nullcRegisterType(1362586038u, "int ref(char[],char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[16] = __nullcRegisterType(3953727713u, "char[] ref(char[],char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[17] = __nullcRegisterType(3214832952u, "char[] ref", 4, __nullcTR[13], 1, NULLC_POINTER);
	__nullcTR[18] = __nullcRegisterType(90259294u, "char[] ref(char[] ref,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[19] = __nullcRegisterType(3410585167u, "char ref(char)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[20] = __nullcRegisterType(1890834067u, "short ref(short)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[21] = __nullcRegisterType(2745832905u, "int ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[22] = __nullcRegisterType(3458960563u, "long ref(long)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[23] = __nullcRegisterType(4223928607u, "float ref(float)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[24] = __nullcRegisterType(4226161577u, "double ref(double)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[25] = __nullcRegisterType(2570056003u, "void ref(char)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[26] = __nullcRegisterType(154671200u, "char ref", 4, __nullcTR[6], 1, NULLC_POINTER);
	__nullcTR[27] = __nullcRegisterType(2657142493u, "char ref ref", 4, __nullcTR[26], 1, NULLC_POINTER);
	__nullcTR[28] = __nullcRegisterType(3834141397u, "void ref(short)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[29] = __nullcRegisterType(3010777554u, "short ref", 4, __nullcTR[5], 1, NULLC_POINTER);
	__nullcTR[30] = __nullcRegisterType(576776527u, "short ref ref", 4, __nullcTR[29], 1, NULLC_POINTER);
	__nullcTR[31] = __nullcRegisterType(2671810221u, "int ref", 4, __nullcTR[4], 1, NULLC_POINTER);
	__nullcTR[32] = __nullcRegisterType(3857294250u, "int ref ref", 4, __nullcTR[31], 1, NULLC_POINTER);
	__nullcTR[33] = __nullcRegisterType(2580994645u, "void ref(long)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[34] = __nullcRegisterType(2414624818u, "long ref", 4, __nullcTR[3], 1, NULLC_POINTER);
	__nullcTR[35] = __nullcRegisterType(799573935u, "long ref ref", 4, __nullcTR[34], 1, NULLC_POINTER);
	__nullcTR[36] = __nullcRegisterType(3330106459u, "void ref(float)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[37] = __nullcRegisterType(1384297880u, "float ref", 4, __nullcTR[2], 1, NULLC_POINTER);
	__nullcTR[38] = __nullcRegisterType(2577874965u, "float ref ref", 4, __nullcTR[37], 1, NULLC_POINTER);
	__nullcTR[39] = __nullcRegisterType(60945760u, "void ref(double)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[40] = __nullcRegisterType(3234425245u, "double ref", 4, __nullcTR[1], 1, NULLC_POINTER);
	__nullcTR[41] = __nullcRegisterType(1954705050u, "double ref ref", 4, __nullcTR[40], 1, NULLC_POINTER);
	__nullcTR[42] = __nullcRegisterType(554739849u, "char[] ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[43] = __nullcRegisterType(2528639597u, "void ref ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[44] = __nullcRegisterType(262756424u, "int[]", 8, __nullcTR[4], -1, NULLC_ARRAY);
	__nullcTR[45] = __nullcRegisterType(1780011448u, "int[] ref(int,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[46] = __nullcRegisterType(3724107199u, "auto ref ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[47] = __nullcRegisterType(3761170085u, "void ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[48] = __nullcRegisterType(4190091973u, "int[] ref", 4, __nullcTR[44], 1, NULLC_POINTER);
	__nullcTR[49] = __nullcRegisterType(844911189u, "void ref() ref(auto ref,int[] ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[50] = __nullcRegisterType(2816069557u, "char[] ref ref", 4, __nullcTR[17], 1, NULLC_POINTER);
	__nullcTR[51] = __nullcRegisterType(2985493640u, "char[] ref ref(char[] ref,int[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[52] = __nullcRegisterType(2463794733u, "short[]", 8, __nullcTR[5], -1, NULLC_ARRAY);
	__nullcTR[53] = __nullcRegisterType(3958330154u, "short[] ref", 4, __nullcTR[52], 1, NULLC_POINTER);
	__nullcTR[54] = __nullcRegisterType(745297575u, "short[] ref ref", 4, __nullcTR[53], 1, NULLC_POINTER);
	__nullcTR[55] = __nullcRegisterType(1935747820u, "short[] ref ref(short[] ref,int[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[56] = __nullcRegisterType(3010510963u, "float[]", 8, __nullcTR[2], -1, NULLC_ARRAY);
	__nullcTR[57] = __nullcRegisterType(2248491120u, "float[] ref", 4, __nullcTR[56], 1, NULLC_POINTER);
	__nullcTR[58] = __nullcRegisterType(1040232248u, "double[]", 8, __nullcTR[1], -1, NULLC_ARRAY);
	__nullcTR[59] = __nullcRegisterType(2393077485u, "float[] ref ref", 4, __nullcTR[57], 1, NULLC_POINTER);
	__nullcTR[60] = __nullcRegisterType(2697529781u, "double[] ref", 4, __nullcTR[58], 1, NULLC_POINTER);
	__nullcTR[61] = __nullcRegisterType(2467461000u, "float[] ref ref(float[] ref,double[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[62] = __nullcRegisterType(2066091864u, "typeid ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[63] = __nullcRegisterType(1268871368u, "int ref(typeid,typeid)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[64] = __nullcRegisterType(2688408224u, "int ref(void ref(int),void ref(int))", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[65] = __nullcRegisterType(1908472638u, "int ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[66] = __nullcRegisterType(1137649267u, "auto[] ref", 4, __nullcTR[10], 1, NULLC_POINTER);
	__nullcTR[67] = __nullcRegisterType(451037873u, "auto[] ref ref(auto[] ref,auto ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[68] = __nullcRegisterType(3824954777u, "auto ref ref(auto ref,auto[] ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[69] = __nullcRegisterType(3832966281u, "auto[] ref ref(auto[] ref,auto[] ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[70] = __nullcRegisterType(477490926u, "auto ref ref(auto[] ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[71] = __nullcRegisterType(3720955042u, "const_string", 8, __nullcTR[0], 1, NULLC_CLASS);
	__nullcTR[72] = __nullcRegisterType(1951548447u, "const_string ref", 4, __nullcTR[71], 1, NULLC_POINTER);
	__nullcTR[73] = __nullcRegisterType(504936988u, "const_string ref ref", 4, __nullcTR[72], 1, NULLC_POINTER);
	__nullcTR[74] = __nullcRegisterType(2448680601u, "const_string ref ref ref", 4, __nullcTR[73], 1, NULLC_POINTER);
	__nullcTR[75] = __nullcRegisterType(3099572969u, "const_string ref ref(const_string ref,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[76] = __nullcRegisterType(2016055718u, "const_string ref(char[])", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[77] = __nullcRegisterType(195623906u, "char ref(const_string ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[78] = __nullcRegisterType(3956827940u, "int ref(const_string,const_string)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[79] = __nullcRegisterType(3539544093u, "int ref(const_string,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[80] = __nullcRegisterType(730969981u, "int ref(char[],const_string)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[81] = __nullcRegisterType(727035222u, "const_string ref(const_string,const_string)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[82] = __nullcRegisterType(1003630799u, "const_string ref(const_string,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[83] = __nullcRegisterType(2490023983u, "const_string ref(char[],const_string)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[84] = __nullcRegisterType(6493228u, "MathConstants", 16, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[85] = __nullcRegisterType(3875326121u, "MathConstants ref", 4, __nullcTR[84], 1, NULLC_POINTER);
	__nullcTR[86] = __nullcRegisterType(1895912087u, "double ref(double,double,double)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[87] = __nullcRegisterType(4256044333u, "float2", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[88] = __nullcRegisterType(2568615123u, "float2 ref(float,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[89] = __nullcRegisterType(2750571050u, "float2 ref", 4, __nullcTR[87], 1, NULLC_POINTER);
	__nullcTR[90] = __nullcRegisterType(1779669499u, "float2 ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[91] = __nullcRegisterType(1029629351u, "float2 ref ref", 4, __nullcTR[89], 1, NULLC_POINTER);
	__nullcTR[92] = __nullcRegisterType(4163559332u, "float2 ref ref ref", 4, __nullcTR[91], 1, NULLC_POINTER);
	__nullcTR[93] = __nullcRegisterType(2519331085u, "void ref(float2)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[94] = __nullcRegisterType(2810325943u, "float2 ref(float2,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[95] = __nullcRegisterType(3338924485u, "float2 ref(float2,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[96] = __nullcRegisterType(3159920773u, "float2 ref(float,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[97] = __nullcRegisterType(583224465u, "float2 ref ref(float2 ref,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[98] = __nullcRegisterType(668426079u, "float2 ref ref(float2 ref,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[99] = __nullcRegisterType(4256044334u, "float3", 12, __nullcTR[0], 3, NULLC_CLASS);
	__nullcTR[100] = __nullcRegisterType(3886564502u, "float3 ref(float,float,float)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[101] = __nullcRegisterType(2751756971u, "float3 ref", 4, __nullcTR[99], 1, NULLC_POINTER);
	__nullcTR[102] = __nullcRegisterType(2983941800u, "float3 ref ref", 4, __nullcTR[101], 1, NULLC_POINTER);
	__nullcTR[103] = __nullcRegisterType(1200220453u, "float3 ref ref ref", 4, __nullcTR[102], 1, NULLC_POINTER);
	__nullcTR[104] = __nullcRegisterType(3071137468u, "float3 ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[105] = __nullcRegisterType(2519331118u, "void ref(float3)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[106] = __nullcRegisterType(3894790970u, "float3 ref(float3,float3)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[107] = __nullcRegisterType(1679830247u, "float3 ref(float3,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[108] = __nullcRegisterType(1832056551u, "float3 ref(float,float3)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[109] = __nullcRegisterType(4106114452u, "float3 ref ref(float3 ref,float3)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[110] = __nullcRegisterType(1816384513u, "float3 ref ref(float3 ref,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[111] = __nullcRegisterType(4256044335u, "float4", 16, __nullcTR[0], 4, NULLC_CLASS);
	__nullcTR[112] = __nullcRegisterType(247594841u, "float4 ref(float,float,float,float)", 8, __nullcTR[0], 4, NULLC_FUNCTION);
	__nullcTR[113] = __nullcRegisterType(2752942892u, "float4 ref", 4, __nullcTR[111], 1, NULLC_POINTER);
	__nullcTR[114] = __nullcRegisterType(643286953u, "float4 ref ref", 4, __nullcTR[113], 1, NULLC_POINTER);
	__nullcTR[115] = __nullcRegisterType(2531848870u, "float4 ref ref ref", 4, __nullcTR[114], 1, NULLC_POINTER);
	__nullcTR[116] = __nullcRegisterType(67638141u, "float4 ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[117] = __nullcRegisterType(2519331151u, "void ref(float4)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[118] = __nullcRegisterType(684288701u, "float4 ref(float4,float4)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[119] = __nullcRegisterType(20736009u, "float4 ref(float4,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[120] = __nullcRegisterType(504192329u, "float4 ref(float,float4)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[121] = __nullcRegisterType(3334037143u, "float4 ref ref(float4 ref,float4)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[122] = __nullcRegisterType(2964342947u, "float4 ref ref(float4 ref,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[123] = __nullcRegisterType(2011060230u, "float3 ref(float2,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[124] = __nullcRegisterType(1832056518u, "float3 ref(float,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[125] = __nullcRegisterType(351965992u, "float4 ref(float3,float)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[126] = __nullcRegisterType(1070631033u, "float4 ref(float2,float2)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[127] = __nullcRegisterType(3253984809u, "float4 ref(float2,float,float)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[128] = __nullcRegisterType(2294885289u, "float4 ref(float,float,float2)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[129] = __nullcRegisterType(504192296u, "float4 ref(float,float3)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[130] = __nullcRegisterType(4262891852u, "double[4]", 32, __nullcTR[1], 4, NULLC_ARRAY);
	__nullcTR[131] = __nullcRegisterType(3368457716u, "float4 ref ref(float4 ref,double[4])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[132] = __nullcRegisterType(1583994313u, "double[4] ref", 4, __nullcTR[130], 1, NULLC_POINTER);
	__nullcTR[133] = __nullcRegisterType(562572443u, "float4x4", 64, __nullcTR[0], 4, NULLC_CLASS);
	__nullcTR[134] = __nullcRegisterType(3309272130u, "float ref ref(float2 ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[135] = __nullcRegisterType(3377073507u, "float ref ref(float3 ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[136] = __nullcRegisterType(3444874884u, "float ref ref(float4 ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[137] = __nullcRegisterType(4261839081u, "float ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[138] = __nullcRegisterType(2081580927u, "float ref(float2 ref,float2 ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[139] = __nullcRegisterType(289501729u, "float ref(float3 ref,float3 ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[140] = __nullcRegisterType(2792389827u, "float ref(float4 ref,float4 ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcFR[0] = (void*)0;
	__nullcFR[1] = (void*)0;
	__nullcFR[2] = (void*)0;
	__nullcFR[3] = (void*)0;
	__nullcFR[4] = (void*)0;
	__nullcFR[5] = (void*)0;
	__nullcFR[6] = (void*)char__;
	__nullcFR[7] = (void*)short__;
	__nullcFR[8] = (void*)int__;
	__nullcFR[9] = (void*)long__;
	__nullcFR[10] = (void*)float__;
	__nullcFR[11] = (void*)double__;
	__nullcFR[12] = (void*)char__char_void_ref_char_;
	__nullcFR[13] = (void*)short__short_void_ref_short_;
	__nullcFR[14] = (void*)int__int_void_ref_int_;
	__nullcFR[15] = (void*)long__long_void_ref_long_;
	__nullcFR[16] = (void*)float__float_void_ref_float_;
	__nullcFR[17] = (void*)double__double_void_ref_double_;
	__nullcFR[18] = (void*)int__str_char___ref__;
	__nullcFR[19] = (void*)__newS;
	__nullcFR[20] = (void*)__newA;
	__nullcFR[21] = (void*)duplicate;
	__nullcFR[22] = (void*)__redirect;
	__nullcFR[23] = (void*)0;
	__nullcFR[24] = (void*)0;
	__nullcFR[25] = (void*)0;
	__nullcFR[26] = (void*)typeid__;
	__nullcFR[27] = (void*)0;
	__nullcFR[28] = (void*)0;
	__nullcFR[29] = (void*)__pcomp;
	__nullcFR[30] = (void*)__pncomp;
	__nullcFR[31] = (void*)__typeCount;
	__nullcFR[32] = (void*)0;
	__nullcFR[33] = (void*)0;
	__nullcFR[34] = (void*)0;
	__nullcFR[35] = (void*)0;
	__nullcFR[36] = (void*)const_string__size__int_ref__;
	__nullcFR[37] = (void*)0;
	__nullcFR[38] = (void*)const_string__;
	__nullcFR[39] = (void*)0;
	__nullcFR[40] = (void*)0;
	__nullcFR[41] = (void*)0;
	__nullcFR[42] = (void*)0;
	__nullcFR[43] = (void*)0;
	__nullcFR[44] = (void*)0;
	__nullcFR[45] = (void*)0;
	__nullcFR[46] = (void*)0;
	__nullcFR[47] = (void*)0;
	__nullcFR[48] = (void*)0;
	__nullcFR[49] = (void*)__char_a_12;
	__nullcFR[50] = (void*)__short_a_13;
	__nullcFR[51] = (void*)__int_a_14;
	__nullcFR[52] = (void*)__long_a_15;
	__nullcFR[53] = (void*)__float_a_16;
	__nullcFR[54] = (void*)__double_a_17;
	__nullcFR[55] = (void*)cos;
	__nullcFR[56] = (void*)sin;
	__nullcFR[57] = (void*)tan;
	__nullcFR[58] = (void*)ctg;
	__nullcFR[59] = (void*)cosh;
	__nullcFR[60] = (void*)sinh;
	__nullcFR[61] = (void*)tanh;
	__nullcFR[62] = (void*)coth;
	__nullcFR[63] = (void*)acos;
	__nullcFR[64] = (void*)asin;
	__nullcFR[65] = (void*)atan;
	__nullcFR[66] = (void*)ceil;
	__nullcFR[67] = (void*)floor;
	__nullcFR[68] = (void*)exp;
	__nullcFR[69] = (void*)log;
	__nullcFR[70] = (void*)sqrt;
	__nullcFR[71] = (void*)clamp;
	__nullcFR[72] = (void*)saturate;
	__nullcFR[73] = (void*)abs;
	__nullcFR[74] = (void*)float2__float2_float2_ref_float_float_;
	__nullcFR[75] = (void*)float2__xx__float2_ref__;
	__nullcFR[76] = (void*)float2__xy__float2_ref__;
	__nullcFR[77] = (void*)float2__xy__void_ref_float2_;
	__nullcFR[78] = (void*)float2__yx__float2_ref__;
	__nullcFR[79] = (void*)float2__yx__void_ref_float2_;
	__nullcFR[80] = (void*)float2__yy__float2_ref__;
	__nullcFR[81] = (void*)float2__;
	__nullcFR[82] = (void*)0;
	__nullcFR[83] = (void*)0;
	__nullcFR[84] = (void*)0;
	__nullcFR[85] = (void*)0;
	__nullcFR[86] = (void*)0;
	__nullcFR[87] = (void*)0;
	__nullcFR[88] = (void*)0;
	__nullcFR[89] = (void*)0;
	__nullcFR[90] = (void*)0;
	__nullcFR[91] = (void*)0;
	__nullcFR[92] = (void*)0;
	__nullcFR[93] = (void*)float3__float3_float3_ref_float_float_float_;
	__nullcFR[94] = (void*)float3__xx__float2_ref__;
	__nullcFR[95] = (void*)float3__xy__float2_ref__;
	__nullcFR[96] = (void*)float3__xy__void_ref_float2_;
	__nullcFR[97] = (void*)float3__xz__float2_ref__;
	__nullcFR[98] = (void*)float3__xz__void_ref_float2_;
	__nullcFR[99] = (void*)float3__yx__float2_ref__;
	__nullcFR[100] = (void*)float3__yx__void_ref_float2_;
	__nullcFR[101] = (void*)float3__yy__float2_ref__;
	__nullcFR[102] = (void*)float3__yz__float2_ref__;
	__nullcFR[103] = (void*)float3__yz__void_ref_float2_;
	__nullcFR[104] = (void*)float3__zx__float2_ref__;
	__nullcFR[105] = (void*)float3__zx__void_ref_float2_;
	__nullcFR[106] = (void*)float3__zy__float2_ref__;
	__nullcFR[107] = (void*)float3__zy__void_ref_float2_;
	__nullcFR[108] = (void*)float3__zz__float2_ref__;
	__nullcFR[109] = (void*)float3__xxx__float3_ref__;
	__nullcFR[110] = (void*)float3__xxy__float3_ref__;
	__nullcFR[111] = (void*)float3__xxz__float3_ref__;
	__nullcFR[112] = (void*)float3__xyx__float3_ref__;
	__nullcFR[113] = (void*)float3__xyy__float3_ref__;
	__nullcFR[114] = (void*)float3__xyz__float3_ref__;
	__nullcFR[115] = (void*)float3__xyz__void_ref_float3_;
	__nullcFR[116] = (void*)float3__xzx__float3_ref__;
	__nullcFR[117] = (void*)float3__xzy__float3_ref__;
	__nullcFR[118] = (void*)float3__xzy__void_ref_float3_;
	__nullcFR[119] = (void*)float3__xzz__float3_ref__;
	__nullcFR[120] = (void*)float3__yxx__float3_ref__;
	__nullcFR[121] = (void*)float3__yxy__float3_ref__;
	__nullcFR[122] = (void*)float3__yxz__float3_ref__;
	__nullcFR[123] = (void*)float3__yxz__void_ref_float3_;
	__nullcFR[124] = (void*)float3__yyx__float3_ref__;
	__nullcFR[125] = (void*)float3__yyy__float3_ref__;
	__nullcFR[126] = (void*)float3__yyz__float3_ref__;
	__nullcFR[127] = (void*)float3__yzx__float3_ref__;
	__nullcFR[128] = (void*)float3__yzx__void_ref_float3_;
	__nullcFR[129] = (void*)float3__yzy__float3_ref__;
	__nullcFR[130] = (void*)float3__yzz__float3_ref__;
	__nullcFR[131] = (void*)float3__zxx__float3_ref__;
	__nullcFR[132] = (void*)float3__zxy__float3_ref__;
	__nullcFR[133] = (void*)float3__zxy__void_ref_float3_;
	__nullcFR[134] = (void*)float3__zxz__float3_ref__;
	__nullcFR[135] = (void*)float3__zyx__float3_ref__;
	__nullcFR[136] = (void*)float3__zyx__void_ref_float3_;
	__nullcFR[137] = (void*)float3__zyy__float3_ref__;
	__nullcFR[138] = (void*)float3__zyz__float3_ref__;
	__nullcFR[139] = (void*)float3__zzx__float3_ref__;
	__nullcFR[140] = (void*)float3__zzy__float3_ref__;
	__nullcFR[141] = (void*)float3__zzz__float3_ref__;
	__nullcFR[142] = (void*)0;
	__nullcFR[143] = (void*)0;
	__nullcFR[144] = (void*)0;
	__nullcFR[145] = (void*)0;
	__nullcFR[146] = (void*)0;
	__nullcFR[147] = (void*)0;
	__nullcFR[148] = (void*)0;
	__nullcFR[149] = (void*)0;
	__nullcFR[150] = (void*)0;
	__nullcFR[151] = (void*)0;
	__nullcFR[152] = (void*)0;
	__nullcFR[153] = (void*)0;
	__nullcFR[154] = (void*)float4__float4_float4_ref_float_float_float_float_;
	__nullcFR[155] = (void*)float4__xx__float2_ref__;
	__nullcFR[156] = (void*)float4__xy__float2_ref__;
	__nullcFR[157] = (void*)float4__xy__void_ref_float2_;
	__nullcFR[158] = (void*)float4__xz__float2_ref__;
	__nullcFR[159] = (void*)float4__xz__void_ref_float2_;
	__nullcFR[160] = (void*)float4__xw__float2_ref__;
	__nullcFR[161] = (void*)float4__xw__void_ref_float2_;
	__nullcFR[162] = (void*)float4__yx__float2_ref__;
	__nullcFR[163] = (void*)float4__yx__void_ref_float2_;
	__nullcFR[164] = (void*)float4__yy__float2_ref__;
	__nullcFR[165] = (void*)float4__yz__float2_ref__;
	__nullcFR[166] = (void*)float4__yz__void_ref_float2_;
	__nullcFR[167] = (void*)float4__yw__float2_ref__;
	__nullcFR[168] = (void*)float4__yw__void_ref_float2_;
	__nullcFR[169] = (void*)float4__zx__float2_ref__;
	__nullcFR[170] = (void*)float4__zx__void_ref_float2_;
	__nullcFR[171] = (void*)float4__zy__float2_ref__;
	__nullcFR[172] = (void*)float4__zy__void_ref_float2_;
	__nullcFR[173] = (void*)float4__zz__float2_ref__;
	__nullcFR[174] = (void*)float4__zw__float2_ref__;
	__nullcFR[175] = (void*)float4__zw__void_ref_float2_;
	__nullcFR[176] = (void*)float4__wx__float2_ref__;
	__nullcFR[177] = (void*)float4__wx__void_ref_float2_;
	__nullcFR[178] = (void*)float4__wy__float2_ref__;
	__nullcFR[179] = (void*)float4__wy__void_ref_float2_;
	__nullcFR[180] = (void*)float4__wz__float2_ref__;
	__nullcFR[181] = (void*)float4__wz__void_ref_float2_;
	__nullcFR[182] = (void*)float4__ww__float2_ref__;
	__nullcFR[183] = (void*)float4__xxx__float3_ref__;
	__nullcFR[184] = (void*)float4__xxy__float3_ref__;
	__nullcFR[185] = (void*)float4__xxz__float3_ref__;
	__nullcFR[186] = (void*)float4__xxw__float3_ref__;
	__nullcFR[187] = (void*)float4__xyx__float3_ref__;
	__nullcFR[188] = (void*)float4__xyy__float3_ref__;
	__nullcFR[189] = (void*)float4__xyz__float3_ref__;
	__nullcFR[190] = (void*)float4__xyz__void_ref_float3_;
	__nullcFR[191] = (void*)float4__xyw__float3_ref__;
	__nullcFR[192] = (void*)float4__xyw__void_ref_float3_;
	__nullcFR[193] = (void*)float4__xzx__float3_ref__;
	__nullcFR[194] = (void*)float4__xzy__float3_ref__;
	__nullcFR[195] = (void*)float4__xzy__void_ref_float3_;
	__nullcFR[196] = (void*)float4__xzz__float3_ref__;
	__nullcFR[197] = (void*)float4__xzw__float3_ref__;
	__nullcFR[198] = (void*)float4__xzw__void_ref_float3_;
	__nullcFR[199] = (void*)float4__xwx__float3_ref__;
	__nullcFR[200] = (void*)float4__xwy__float3_ref__;
	__nullcFR[201] = (void*)float4__xwy__void_ref_float3_;
	__nullcFR[202] = (void*)float4__xwz__float3_ref__;
	__nullcFR[203] = (void*)float4__xwz__void_ref_float3_;
	__nullcFR[204] = (void*)float4__xww__float3_ref__;
	__nullcFR[205] = (void*)float4__yxx__float3_ref__;
	__nullcFR[206] = (void*)float4__yxy__float3_ref__;
	__nullcFR[207] = (void*)float4__yxz__float3_ref__;
	__nullcFR[208] = (void*)float4__yxz__void_ref_float3_;
	__nullcFR[209] = (void*)float4__yxw__float3_ref__;
	__nullcFR[210] = (void*)float4__yxw__void_ref_float3_;
	__nullcFR[211] = (void*)float4__yyx__float3_ref__;
	__nullcFR[212] = (void*)float4__yyy__float3_ref__;
	__nullcFR[213] = (void*)float4__yyz__float3_ref__;
	__nullcFR[214] = (void*)float4__yyw__float3_ref__;
	__nullcFR[215] = (void*)float4__yzx__float3_ref__;
	__nullcFR[216] = (void*)float4__yzx__void_ref_float3_;
	__nullcFR[217] = (void*)float4__yzy__float3_ref__;
	__nullcFR[218] = (void*)float4__yzz__float3_ref__;
	__nullcFR[219] = (void*)float4__yzw__float3_ref__;
	__nullcFR[220] = (void*)float4__yzw__void_ref_float3_;
	__nullcFR[221] = (void*)float4__ywx__float3_ref__;
	__nullcFR[222] = (void*)float4__ywx__void_ref_float3_;
	__nullcFR[223] = (void*)float4__ywy__float3_ref__;
	__nullcFR[224] = (void*)float4__ywz__float3_ref__;
	__nullcFR[225] = (void*)float4__ywz__void_ref_float3_;
	__nullcFR[226] = (void*)float4__yww__float3_ref__;
	__nullcFR[227] = (void*)float4__zxx__float3_ref__;
	__nullcFR[228] = (void*)float4__zxy__float3_ref__;
	__nullcFR[229] = (void*)float4__zxy__void_ref_float3_;
	__nullcFR[230] = (void*)float4__zxz__float3_ref__;
	__nullcFR[231] = (void*)float4__zxw__float3_ref__;
	__nullcFR[232] = (void*)float4__zxw__void_ref_float3_;
	__nullcFR[233] = (void*)float4__zyx__float3_ref__;
	__nullcFR[234] = (void*)float4__zyx__void_ref_float3_;
	__nullcFR[235] = (void*)float4__zyy__float3_ref__;
	__nullcFR[236] = (void*)float4__zyz__float3_ref__;
	__nullcFR[237] = (void*)float4__zyw__float3_ref__;
	__nullcFR[238] = (void*)float4__zyw__void_ref_float3_;
	__nullcFR[239] = (void*)float4__zzx__float3_ref__;
	__nullcFR[240] = (void*)float4__zzy__float3_ref__;
	__nullcFR[241] = (void*)float4__zzz__float3_ref__;
	__nullcFR[242] = (void*)float4__zzw__float3_ref__;
	__nullcFR[243] = (void*)float4__zwx__float3_ref__;
	__nullcFR[244] = (void*)float4__zwx__void_ref_float3_;
	__nullcFR[245] = (void*)float4__zwy__float3_ref__;
	__nullcFR[246] = (void*)float4__zwy__void_ref_float3_;
	__nullcFR[247] = (void*)float4__zwz__float3_ref__;
	__nullcFR[248] = (void*)float4__zww__float3_ref__;
	__nullcFR[249] = (void*)float4__wxx__float3_ref__;
	__nullcFR[250] = (void*)float4__wxy__float3_ref__;
	__nullcFR[251] = (void*)float4__wxy__void_ref_float3_;
	__nullcFR[252] = (void*)float4__wxz__float3_ref__;
	__nullcFR[253] = (void*)float4__wxz__void_ref_float3_;
	__nullcFR[254] = (void*)float4__wxw__float3_ref__;
	__nullcFR[255] = (void*)float4__wyx__float3_ref__;
	__nullcFR[256] = (void*)float4__wyx__void_ref_float3_;
	__nullcFR[257] = (void*)float4__wyy__float3_ref__;
	__nullcFR[258] = (void*)float4__wyz__float3_ref__;
	__nullcFR[259] = (void*)float4__wyz__void_ref_float3_;
	__nullcFR[260] = (void*)float4__wyw__float3_ref__;
	__nullcFR[261] = (void*)float4__wzx__float3_ref__;
	__nullcFR[262] = (void*)float4__wzx__void_ref_float3_;
	__nullcFR[263] = (void*)float4__wzy__float3_ref__;
	__nullcFR[264] = (void*)float4__wzy__void_ref_float3_;
	__nullcFR[265] = (void*)float4__wzz__float3_ref__;
	__nullcFR[266] = (void*)float4__wzw__float3_ref__;
	__nullcFR[267] = (void*)float4__wwx__float3_ref__;
	__nullcFR[268] = (void*)float4__wwy__float3_ref__;
	__nullcFR[269] = (void*)float4__wwz__float3_ref__;
	__nullcFR[270] = (void*)float4__www__float3_ref__;
	__nullcFR[271] = (void*)float4__xxxx__float4_ref__;
	__nullcFR[272] = (void*)float4__xxxy__float4_ref__;
	__nullcFR[273] = (void*)float4__xxxz__float4_ref__;
	__nullcFR[274] = (void*)float4__xxxw__float4_ref__;
	__nullcFR[275] = (void*)float4__xxyx__float4_ref__;
	__nullcFR[276] = (void*)float4__xxyy__float4_ref__;
	__nullcFR[277] = (void*)float4__xxyz__float4_ref__;
	__nullcFR[278] = (void*)float4__xxyw__float4_ref__;
	__nullcFR[279] = (void*)float4__xxzx__float4_ref__;
	__nullcFR[280] = (void*)float4__xxzy__float4_ref__;
	__nullcFR[281] = (void*)float4__xxzz__float4_ref__;
	__nullcFR[282] = (void*)float4__xxzw__float4_ref__;
	__nullcFR[283] = (void*)float4__xxwx__float4_ref__;
	__nullcFR[284] = (void*)float4__xxwy__float4_ref__;
	__nullcFR[285] = (void*)float4__xxwz__float4_ref__;
	__nullcFR[286] = (void*)float4__xxww__float4_ref__;
	__nullcFR[287] = (void*)float4__xyxx__float4_ref__;
	__nullcFR[288] = (void*)float4__xyxy__float4_ref__;
	__nullcFR[289] = (void*)float4__xyxz__float4_ref__;
	__nullcFR[290] = (void*)float4__xyxw__float4_ref__;
	__nullcFR[291] = (void*)float4__xyyx__float4_ref__;
	__nullcFR[292] = (void*)float4__xyyy__float4_ref__;
	__nullcFR[293] = (void*)float4__xyyz__float4_ref__;
	__nullcFR[294] = (void*)float4__xyyw__float4_ref__;
	__nullcFR[295] = (void*)float4__xyzx__float4_ref__;
	__nullcFR[296] = (void*)float4__xyzy__float4_ref__;
	__nullcFR[297] = (void*)float4__xyzz__float4_ref__;
	__nullcFR[298] = (void*)float4__xyzw__float4_ref__;
	__nullcFR[299] = (void*)float4__xyzw__void_ref_float4_;
	__nullcFR[300] = (void*)float4__xywx__float4_ref__;
	__nullcFR[301] = (void*)float4__xywy__float4_ref__;
	__nullcFR[302] = (void*)float4__xywz__float4_ref__;
	__nullcFR[303] = (void*)float4__xywz__void_ref_float4_;
	__nullcFR[304] = (void*)float4__xyww__float4_ref__;
	__nullcFR[305] = (void*)float4__xzxx__float4_ref__;
	__nullcFR[306] = (void*)float4__xzxy__float4_ref__;
	__nullcFR[307] = (void*)float4__xzxz__float4_ref__;
	__nullcFR[308] = (void*)float4__xzxw__float4_ref__;
	__nullcFR[309] = (void*)float4__xzyx__float4_ref__;
	__nullcFR[310] = (void*)float4__xzyy__float4_ref__;
	__nullcFR[311] = (void*)float4__xzyz__float4_ref__;
	__nullcFR[312] = (void*)float4__xzyw__float4_ref__;
	__nullcFR[313] = (void*)float4__xzyw__void_ref_float4_;
	__nullcFR[314] = (void*)float4__xzzx__float4_ref__;
	__nullcFR[315] = (void*)float4__xzzy__float4_ref__;
	__nullcFR[316] = (void*)float4__xzzz__float4_ref__;
	__nullcFR[317] = (void*)float4__xzzw__float4_ref__;
	__nullcFR[318] = (void*)float4__xzwx__float4_ref__;
	__nullcFR[319] = (void*)float4__xzwy__float4_ref__;
	__nullcFR[320] = (void*)float4__xzwy__void_ref_float4_;
	__nullcFR[321] = (void*)float4__xzwz__float4_ref__;
	__nullcFR[322] = (void*)float4__xzww__float4_ref__;
	__nullcFR[323] = (void*)float4__xwxx__float4_ref__;
	__nullcFR[324] = (void*)float4__xwxy__float4_ref__;
	__nullcFR[325] = (void*)float4__xwxz__float4_ref__;
	__nullcFR[326] = (void*)float4__xwxw__float4_ref__;
	__nullcFR[327] = (void*)float4__xwyx__float4_ref__;
	__nullcFR[328] = (void*)float4__xwyy__float4_ref__;
	__nullcFR[329] = (void*)float4__xwyz__float4_ref__;
	__nullcFR[330] = (void*)float4__xwyz__void_ref_float4_;
	__nullcFR[331] = (void*)float4__xwyw__float4_ref__;
	__nullcFR[332] = (void*)float4__xwzx__float4_ref__;
	__nullcFR[333] = (void*)float4__xwzy__float4_ref__;
	__nullcFR[334] = (void*)float4__xwzy__void_ref_float4_;
	__nullcFR[335] = (void*)float4__xwzz__float4_ref__;
	__nullcFR[336] = (void*)float4__xwzw__float4_ref__;
	__nullcFR[337] = (void*)float4__xwwx__float4_ref__;
	__nullcFR[338] = (void*)float4__xwwy__float4_ref__;
	__nullcFR[339] = (void*)float4__xwwz__float4_ref__;
	__nullcFR[340] = (void*)float4__xwww__float4_ref__;
	__nullcFR[341] = (void*)float4__yxxx__float4_ref__;
	__nullcFR[342] = (void*)float4__yxxy__float4_ref__;
	__nullcFR[343] = (void*)float4__yxxz__float4_ref__;
	__nullcFR[344] = (void*)float4__yxxw__float4_ref__;
	__nullcFR[345] = (void*)float4__yxyx__float4_ref__;
	__nullcFR[346] = (void*)float4__yxyy__float4_ref__;
	__nullcFR[347] = (void*)float4__yxyz__float4_ref__;
	__nullcFR[348] = (void*)float4__yxyw__float4_ref__;
	__nullcFR[349] = (void*)float4__yxzx__float4_ref__;
	__nullcFR[350] = (void*)float4__yxzy__float4_ref__;
	__nullcFR[351] = (void*)float4__yxzz__float4_ref__;
	__nullcFR[352] = (void*)float4__yxzw__float4_ref__;
	__nullcFR[353] = (void*)float4__yxzw__void_ref_float4_;
	__nullcFR[354] = (void*)float4__yxwx__float4_ref__;
	__nullcFR[355] = (void*)float4__yxwy__float4_ref__;
	__nullcFR[356] = (void*)float4__yxwz__float4_ref__;
	__nullcFR[357] = (void*)float4__yxwz__void_ref_float4_;
	__nullcFR[358] = (void*)float4__yxww__float4_ref__;
	__nullcFR[359] = (void*)float4__yyxx__float4_ref__;
	__nullcFR[360] = (void*)float4__yyxy__float4_ref__;
	__nullcFR[361] = (void*)float4__yyxz__float4_ref__;
	__nullcFR[362] = (void*)float4__yyxw__float4_ref__;
	__nullcFR[363] = (void*)float4__yyyx__float4_ref__;
	__nullcFR[364] = (void*)float4__yyyy__float4_ref__;
	__nullcFR[365] = (void*)float4__yyyz__float4_ref__;
	__nullcFR[366] = (void*)float4__yyyw__float4_ref__;
	__nullcFR[367] = (void*)float4__yyzx__float4_ref__;
	__nullcFR[368] = (void*)float4__yyzy__float4_ref__;
	__nullcFR[369] = (void*)float4__yyzz__float4_ref__;
	__nullcFR[370] = (void*)float4__yyzw__float4_ref__;
	__nullcFR[371] = (void*)float4__yywx__float4_ref__;
	__nullcFR[372] = (void*)float4__yywy__float4_ref__;
	__nullcFR[373] = (void*)float4__yywz__float4_ref__;
	__nullcFR[374] = (void*)float4__yyww__float4_ref__;
	__nullcFR[375] = (void*)float4__yzxx__float4_ref__;
	__nullcFR[376] = (void*)float4__yzxy__float4_ref__;
	__nullcFR[377] = (void*)float4__yzxz__float4_ref__;
	__nullcFR[378] = (void*)float4__yzxw__float4_ref__;
	__nullcFR[379] = (void*)float4__yzxw__void_ref_float4_;
	__nullcFR[380] = (void*)float4__yzyx__float4_ref__;
	__nullcFR[381] = (void*)float4__yzyy__float4_ref__;
	__nullcFR[382] = (void*)float4__yzyz__float4_ref__;
	__nullcFR[383] = (void*)float4__yzyw__float4_ref__;
	__nullcFR[384] = (void*)float4__yzzx__float4_ref__;
	__nullcFR[385] = (void*)float4__yzzy__float4_ref__;
	__nullcFR[386] = (void*)float4__yzzz__float4_ref__;
	__nullcFR[387] = (void*)float4__yzzw__float4_ref__;
	__nullcFR[388] = (void*)float4__yzwx__float4_ref__;
	__nullcFR[389] = (void*)float4__yzwx__void_ref_float4_;
	__nullcFR[390] = (void*)float4__yzwy__float4_ref__;
	__nullcFR[391] = (void*)float4__yzwz__float4_ref__;
	__nullcFR[392] = (void*)float4__yzww__float4_ref__;
	__nullcFR[393] = (void*)float4__ywxx__float4_ref__;
	__nullcFR[394] = (void*)float4__ywxy__float4_ref__;
	__nullcFR[395] = (void*)float4__ywxz__float4_ref__;
	__nullcFR[396] = (void*)float4__ywxz__void_ref_float4_;
	__nullcFR[397] = (void*)float4__ywxw__float4_ref__;
	__nullcFR[398] = (void*)float4__ywyx__float4_ref__;
	__nullcFR[399] = (void*)float4__ywyy__float4_ref__;
	__nullcFR[400] = (void*)float4__ywyz__float4_ref__;
	__nullcFR[401] = (void*)float4__ywyw__float4_ref__;
	__nullcFR[402] = (void*)float4__ywzx__float4_ref__;
	__nullcFR[403] = (void*)float4__ywzx__void_ref_float4_;
	__nullcFR[404] = (void*)float4__ywzy__float4_ref__;
	__nullcFR[405] = (void*)float4__ywzz__float4_ref__;
	__nullcFR[406] = (void*)float4__ywzw__float4_ref__;
	__nullcFR[407] = (void*)float4__ywwx__float4_ref__;
	__nullcFR[408] = (void*)float4__ywwy__float4_ref__;
	__nullcFR[409] = (void*)float4__ywwz__float4_ref__;
	__nullcFR[410] = (void*)float4__ywww__float4_ref__;
	__nullcFR[411] = (void*)float4__zxxx__float4_ref__;
	__nullcFR[412] = (void*)float4__zxxy__float4_ref__;
	__nullcFR[413] = (void*)float4__zxxz__float4_ref__;
	__nullcFR[414] = (void*)float4__zxxw__float4_ref__;
	__nullcFR[415] = (void*)float4__zxyx__float4_ref__;
	__nullcFR[416] = (void*)float4__zxyy__float4_ref__;
	__nullcFR[417] = (void*)float4__zxyz__float4_ref__;
	__nullcFR[418] = (void*)float4__zxyw__float4_ref__;
	__nullcFR[419] = (void*)float4__zxyw__void_ref_float4_;
	__nullcFR[420] = (void*)float4__zxzx__float4_ref__;
	__nullcFR[421] = (void*)float4__zxzy__float4_ref__;
	__nullcFR[422] = (void*)float4__zxzz__float4_ref__;
	__nullcFR[423] = (void*)float4__zxzw__float4_ref__;
	__nullcFR[424] = (void*)float4__zxwx__float4_ref__;
	__nullcFR[425] = (void*)float4__zxwy__float4_ref__;
	__nullcFR[426] = (void*)float4__zxwy__void_ref_float4_;
	__nullcFR[427] = (void*)float4__zxwz__float4_ref__;
	__nullcFR[428] = (void*)float4__zxww__float4_ref__;
	__nullcFR[429] = (void*)float4__zyxx__float4_ref__;
	__nullcFR[430] = (void*)float4__zyxy__float4_ref__;
	__nullcFR[431] = (void*)float4__zyxz__float4_ref__;
	__nullcFR[432] = (void*)float4__zyxw__float4_ref__;
	__nullcFR[433] = (void*)float4__zyxw__void_ref_float4_;
	__nullcFR[434] = (void*)float4__zyyx__float4_ref__;
	__nullcFR[435] = (void*)float4__zyyy__float4_ref__;
	__nullcFR[436] = (void*)float4__zyyz__float4_ref__;
	__nullcFR[437] = (void*)float4__zyyw__float4_ref__;
	__nullcFR[438] = (void*)float4__zyzx__float4_ref__;
	__nullcFR[439] = (void*)float4__zyzy__float4_ref__;
	__nullcFR[440] = (void*)float4__zyzz__float4_ref__;
	__nullcFR[441] = (void*)float4__zyzw__float4_ref__;
	__nullcFR[442] = (void*)float4__zywx__float4_ref__;
	__nullcFR[443] = (void*)float4__zywx__void_ref_float4_;
	__nullcFR[444] = (void*)float4__zywy__float4_ref__;
	__nullcFR[445] = (void*)float4__zywz__float4_ref__;
	__nullcFR[446] = (void*)float4__zyww__float4_ref__;
	__nullcFR[447] = (void*)float4__zzxx__float4_ref__;
	__nullcFR[448] = (void*)float4__zzxy__float4_ref__;
	__nullcFR[449] = (void*)float4__zzxz__float4_ref__;
	__nullcFR[450] = (void*)float4__zzxw__float4_ref__;
	__nullcFR[451] = (void*)float4__zzyx__float4_ref__;
	__nullcFR[452] = (void*)float4__zzyy__float4_ref__;
	__nullcFR[453] = (void*)float4__zzyz__float4_ref__;
	__nullcFR[454] = (void*)float4__zzyw__float4_ref__;
	__nullcFR[455] = (void*)float4__zzzx__float4_ref__;
	__nullcFR[456] = (void*)float4__zzzy__float4_ref__;
	__nullcFR[457] = (void*)float4__zzzz__float4_ref__;
	__nullcFR[458] = (void*)float4__zzzw__float4_ref__;
	__nullcFR[459] = (void*)float4__zzwx__float4_ref__;
	__nullcFR[460] = (void*)float4__zzwy__float4_ref__;
	__nullcFR[461] = (void*)float4__zzwz__float4_ref__;
	__nullcFR[462] = (void*)float4__zzww__float4_ref__;
	__nullcFR[463] = (void*)float4__zwxx__float4_ref__;
	__nullcFR[464] = (void*)float4__zwxy__float4_ref__;
	__nullcFR[465] = (void*)float4__zwxy__void_ref_float4_;
	__nullcFR[466] = (void*)float4__zwxz__float4_ref__;
	__nullcFR[467] = (void*)float4__zwxw__float4_ref__;
	__nullcFR[468] = (void*)float4__zwyx__float4_ref__;
	__nullcFR[469] = (void*)float4__zwyx__void_ref_float4_;
	__nullcFR[470] = (void*)float4__zwyy__float4_ref__;
	__nullcFR[471] = (void*)float4__zwyz__float4_ref__;
	__nullcFR[472] = (void*)float4__zwyw__float4_ref__;
	__nullcFR[473] = (void*)float4__zwzx__float4_ref__;
	__nullcFR[474] = (void*)float4__zwzy__float4_ref__;
	__nullcFR[475] = (void*)float4__zwzz__float4_ref__;
	__nullcFR[476] = (void*)float4__zwzw__float4_ref__;
	__nullcFR[477] = (void*)float4__zwwx__float4_ref__;
	__nullcFR[478] = (void*)float4__zwwy__float4_ref__;
	__nullcFR[479] = (void*)float4__zwwz__float4_ref__;
	__nullcFR[480] = (void*)float4__zwww__float4_ref__;
	__nullcFR[481] = (void*)float4__wxxx__float4_ref__;
	__nullcFR[482] = (void*)float4__wxxy__float4_ref__;
	__nullcFR[483] = (void*)float4__wxxz__float4_ref__;
	__nullcFR[484] = (void*)float4__wxxw__float4_ref__;
	__nullcFR[485] = (void*)float4__wxyx__float4_ref__;
	__nullcFR[486] = (void*)float4__wxyy__float4_ref__;
	__nullcFR[487] = (void*)float4__wxyz__float4_ref__;
	__nullcFR[488] = (void*)float4__wxyz__void_ref_float4_;
	__nullcFR[489] = (void*)float4__wxyw__float4_ref__;
	__nullcFR[490] = (void*)float4__wxzx__float4_ref__;
	__nullcFR[491] = (void*)float4__wxzy__float4_ref__;
	__nullcFR[492] = (void*)float4__wxzy__void_ref_float4_;
	__nullcFR[493] = (void*)float4__wxzz__float4_ref__;
	__nullcFR[494] = (void*)float4__wxzw__float4_ref__;
	__nullcFR[495] = (void*)float4__wxwx__float4_ref__;
	__nullcFR[496] = (void*)float4__wxwy__float4_ref__;
	__nullcFR[497] = (void*)float4__wxwz__float4_ref__;
	__nullcFR[498] = (void*)float4__wxww__float4_ref__;
	__nullcFR[499] = (void*)float4__wyxx__float4_ref__;
	__nullcFR[500] = (void*)float4__wyxy__float4_ref__;
	__nullcFR[501] = (void*)float4__wyxz__float4_ref__;
	__nullcFR[502] = (void*)float4__wyxz__void_ref_float4_;
	__nullcFR[503] = (void*)float4__wyxw__float4_ref__;
	__nullcFR[504] = (void*)float4__wyyx__float4_ref__;
	__nullcFR[505] = (void*)float4__wyyy__float4_ref__;
	__nullcFR[506] = (void*)float4__wyyz__float4_ref__;
	__nullcFR[507] = (void*)float4__wyyw__float4_ref__;
	__nullcFR[508] = (void*)float4__wyzx__float4_ref__;
	__nullcFR[509] = (void*)float4__wyzx__void_ref_float4_;
	__nullcFR[510] = (void*)float4__wyzy__float4_ref__;
	__nullcFR[511] = (void*)float4__wyzz__float4_ref__;
	__nullcFR[512] = (void*)float4__wyzw__float4_ref__;
	__nullcFR[513] = (void*)float4__wywx__float4_ref__;
	__nullcFR[514] = (void*)float4__wywy__float4_ref__;
	__nullcFR[515] = (void*)float4__wywz__float4_ref__;
	__nullcFR[516] = (void*)float4__wyww__float4_ref__;
	__nullcFR[517] = (void*)float4__wzxx__float4_ref__;
	__nullcFR[518] = (void*)float4__wzxy__float4_ref__;
	__nullcFR[519] = (void*)float4__wzxy__void_ref_float4_;
	__nullcFR[520] = (void*)float4__wzxz__float4_ref__;
	__nullcFR[521] = (void*)float4__wzxw__float4_ref__;
	__nullcFR[522] = (void*)float4__wzyx__float4_ref__;
	__nullcFR[523] = (void*)float4__wzyx__void_ref_float4_;
	__nullcFR[524] = (void*)float4__wzyy__float4_ref__;
	__nullcFR[525] = (void*)float4__wzyz__float4_ref__;
	__nullcFR[526] = (void*)float4__wzyw__float4_ref__;
	__nullcFR[527] = (void*)float4__wzzx__float4_ref__;
	__nullcFR[528] = (void*)float4__wzzy__float4_ref__;
	__nullcFR[529] = (void*)float4__wzzz__float4_ref__;
	__nullcFR[530] = (void*)float4__wzzw__float4_ref__;
	__nullcFR[531] = (void*)float4__wzwx__float4_ref__;
	__nullcFR[532] = (void*)float4__wzwy__float4_ref__;
	__nullcFR[533] = (void*)float4__wzwz__float4_ref__;
	__nullcFR[534] = (void*)float4__wzww__float4_ref__;
	__nullcFR[535] = (void*)float4__wwxx__float4_ref__;
	__nullcFR[536] = (void*)float4__wwxy__float4_ref__;
	__nullcFR[537] = (void*)float4__wwxz__float4_ref__;
	__nullcFR[538] = (void*)float4__wwxw__float4_ref__;
	__nullcFR[539] = (void*)float4__wwyx__float4_ref__;
	__nullcFR[540] = (void*)float4__wwyy__float4_ref__;
	__nullcFR[541] = (void*)float4__wwyz__float4_ref__;
	__nullcFR[542] = (void*)float4__wwyw__float4_ref__;
	__nullcFR[543] = (void*)float4__wwzx__float4_ref__;
	__nullcFR[544] = (void*)float4__wwzy__float4_ref__;
	__nullcFR[545] = (void*)float4__wwzz__float4_ref__;
	__nullcFR[546] = (void*)float4__wwzw__float4_ref__;
	__nullcFR[547] = (void*)float4__wwwx__float4_ref__;
	__nullcFR[548] = (void*)float4__wwwy__float4_ref__;
	__nullcFR[549] = (void*)float4__wwwz__float4_ref__;
	__nullcFR[550] = (void*)float4__wwww__float4_ref__;
	__nullcFR[551] = (void*)0;
	__nullcFR[552] = (void*)0;
	__nullcFR[553] = (void*)0;
	__nullcFR[554] = (void*)0;
	__nullcFR[555] = (void*)0;
	__nullcFR[556] = (void*)0;
	__nullcFR[557] = (void*)0;
	__nullcFR[558] = (void*)0;
	__nullcFR[559] = (void*)0;
	__nullcFR[560] = (void*)0;
	__nullcFR[561] = (void*)0;
	__nullcFR[562] = (void*)0;
	__nullcFR[563] = (void*)0;
	__nullcFR[564] = (void*)0;
	__nullcFR[565] = (void*)0;
	__nullcFR[566] = (void*)0;
	__nullcFR[567] = (void*)0;
	__nullcFR[568] = (void*)0;
	__nullcFR[569] = (void*)0;
	__nullcFR[570] = (void*)0;
	__nullcFR[571] = (void*)reflect;
	__nullcFR[572] = (void*)0;
	__nullcFR[573] = (void*)0;
	__nullcFR[574] = (void*)0;
	__nullcFR[575] = (void*)float2__length_float_ref__;
	__nullcFR[576] = (void*)float2__normalize_float_ref__;
	__nullcFR[577] = (void*)float3__length_float_ref__;
	__nullcFR[578] = (void*)float3__normalize_float_ref__;
	__nullcFR[579] = (void*)float4__length_float_ref__;
	__nullcFR[580] = (void*)float4__normalize_float_ref__;
	__nullcFR[581] = (void*)0;
	__nullcFR[582] = (void*)0;
	__nullcFR[583] = (void*)0;
	*(&(&math)->pi) = 3.141593;
	*(&(&math)->e) = 2.718282;
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
}
