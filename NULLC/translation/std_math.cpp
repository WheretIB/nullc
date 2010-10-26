#include "runtime.h"
// Typeid redirect table
static unsigned __nullcTR[150];
// Function pointer table
static __nullcFunctionArray* __nullcFM;
// Function pointer redirect table
static unsigned __nullcFR[601];
// Function pointers, arrays, classes
#pragma pack(push, 4)
struct __typeProxy_void_ref_int_{};
struct __typeProxy_void_ref_int_char___{};
struct __typeProxy_int_ref_char___char___{};
struct __typeProxy_char___ref_char___char___{};
struct __typeProxy_char___ref_char___ref_char___{};
struct __typeProxy_char_ref_char_{};
struct __typeProxy_short_ref_short_{};
struct __typeProxy_int_ref_int_{};
struct __typeProxy_long_ref_long_{};
struct __typeProxy_float_ref_float_{};
struct __typeProxy_double_ref_double_{};
struct __typeProxy_void_ref_char_{};
struct __typeProxy_void_ref_short_{};
struct __typeProxy_void_ref_long_{};
struct __typeProxy_void_ref_float_{};
struct __typeProxy_void_ref_double_{};
struct __typeProxy_char___ref__{};
struct __typeProxy_char___ref_int_{};
struct __typeProxy_void_ref_ref_int_{};
struct __typeProxy_int___ref_int_int_{};
struct __typeProxy_auto_ref_ref_auto_ref_{};
struct __typeProxy_void_ref_auto___ref_auto___{};
struct __typeProxy_auto___ref_auto___{};
struct __typeProxy_auto_ref_ref_auto_ref_auto_ref_{};
struct __typeProxy_void_ref_auto_ref_auto_ref_{};
struct __typeProxy_int_ref_auto_ref_auto_ref_{};
struct __typeProxy_void_ref__{};
struct __typeProxy_void_ref___ref_auto_ref_int___ref_{};
struct __typeProxy_char___ref_ref_char___ref_int___{};
struct __typeProxy_short___ref_ref_short___ref_int___{};
struct __typeProxy_float___ref_ref_float___ref_double___{};
struct __typeProxy_typeid_ref_auto_ref_{};
struct __typeProxy_int_ref__{};
struct __typeProxy_int_ref_typeid_typeid_{};
struct __typeProxy_int_ref_void_ref_int__void_ref_int__{};
struct __typeProxy_auto___ref_ref_auto___ref_auto_ref_{};
struct __typeProxy_auto_ref_ref_auto_ref_auto___ref_{};
struct __typeProxy_auto___ref_ref_auto___ref_auto___ref_{};
struct __typeProxy_auto_ref_ref_auto___ref_int_{};
struct __typeProxy_const_string_ref_ref_const_string_ref_char___{};
struct __typeProxy_const_string_ref_char___{};
struct __typeProxy_char_ref_const_string_ref_int_{};
struct __typeProxy_int_ref_const_string_const_string_{};
struct __typeProxy_int_ref_const_string_char___{};
struct __typeProxy_int_ref_char___const_string_{};
struct __typeProxy_const_string_ref_const_string_const_string_{};
struct __typeProxy_const_string_ref_const_string_char___{};
struct __typeProxy_const_string_ref_char___const_string_{};
struct __typeProxy_int_ref_auto_ref_{};
struct __typeProxy_void_ref_auto___ref_typeid_int_{};
struct __typeProxy_auto___ref_typeid_int_{};
struct __typeProxy_void_ref_auto_ref_int_{};
struct __typeProxy_void_ref_auto___ref_int_{};
struct __typeProxy_void_ref_auto_ref_{};
struct MathConstants 
{
	double pi;
	double e;
};
struct __typeProxy_double_ref_double_double_double_{};
struct __typeProxy_float2_ref_float_float_{};
struct __typeProxy_float2_ref__{};
struct __typeProxy_void_ref_float2_{};
struct float2 
{
	float x;
	float y;
};
struct __typeProxy_float2_ref_float2_float2_{};
struct __typeProxy_float2_ref_float2_float_{};
struct __typeProxy_float2_ref_float_float2_{};
struct __typeProxy_float2_ref_ref_float2_ref_float2_{};
struct __typeProxy_float2_ref_ref_float2_ref_float_{};
struct __typeProxy_float3_ref_float_float_float_{};
struct __typeProxy_float3_ref__{};
struct __typeProxy_void_ref_float3_{};
struct float3 
{
	float x;
	float y;
	float z;
};
struct __typeProxy_float3_ref_float3_float3_{};
struct __typeProxy_float3_ref_float3_float_{};
struct __typeProxy_float3_ref_float_float3_{};
struct __typeProxy_float3_ref_ref_float3_ref_float3_{};
struct __typeProxy_float3_ref_ref_float3_ref_float_{};
struct __typeProxy_float4_ref_float_float_float_float_{};
struct __typeProxy_float4_ref__{};
struct __typeProxy_void_ref_float4_{};
struct float4 
{
	float x;
	float y;
	float z;
	float w;
};
struct __typeProxy_float4_ref_float4_float4_{};
struct __typeProxy_float4_ref_float4_float_{};
struct __typeProxy_float4_ref_float_float4_{};
struct __typeProxy_float4_ref_ref_float4_ref_float4_{};
struct __typeProxy_float4_ref_ref_float4_ref_float_{};
struct __typeProxy_float3_ref_float2_float_{};
struct __typeProxy_float3_ref_float_float2_{};
struct __typeProxy_float4_ref_float3_float_{};
struct __typeProxy_float4_ref_float2_float2_{};
struct __typeProxy_float4_ref_float2_float_float_{};
struct __typeProxy_float4_ref_float_float_float2_{};
struct __typeProxy_float4_ref_float_float3_{};
struct double_4_ 
{
	double ptr[4];
	double_4_ & set(unsigned index, double const & val){ ptr[index] = val; return *this; }
};
struct __typeProxy_float4_ref_ref_float4_ref_double_4__{};
struct float4x4 
{
	float4 row1;
	float4 row2;
	float4 row3;
	float4 row4;
};
struct __typeProxy_float_ref_ref_float2_ref_int_{};
struct __typeProxy_float_ref_ref_float3_ref_int_{};
struct __typeProxy_float_ref_ref_float4_ref_int_{};
struct __typeProxy_float_ref__{};
struct __typeProxy_float_ref_float2_ref_float2_ref_{};
struct __typeProxy_float_ref_float3_ref_float3_ref_{};
struct __typeProxy_float_ref_float4_ref_float4_ref_{};
#pragma pack(pop)
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
float3 reflect(float3 normal, float3 dir, void* unused);
MathConstants math;
float2  float2__float2_float2_ref_float_float_(float x_0, float y_4, float2 * __context)
{
	float2 res_16;
	*(&(&res_16)->x) = *(&x_0);
	*(&(&res_16)->y) = *(&y_4);
	return *(&res_16);
}
float2  float2__xx__float2_ref__(float2 * __context)
{
	return float2__float2_float2_ref_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), (float2 * )(*(&__context)));
}
float2  float2__xy__float2_ref__(float2 * __context)
{
	return float2__float2_float2_ref_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), (float2 * )(*(&__context)));
}
void  float2__xy__void_ref_float2_(float2 r_0, float2 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
}
float2  float2__yx__float2_ref__(float2 * __context)
{
	return float2__float2_float2_ref_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), (float2 * )(*(&__context)));
}
void  float2__yx__void_ref_float2_(float2 r_0, float2 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
}
float2  float2__yy__float2_ref__(float2 * __context)
{
	return float2__float2_float2_ref_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), (float2 * )(*(&__context)));
}
float2  float2__(float x_0, float y_4, void* unused)
{
	float2 ret_16;
	*(&(&ret_16)->x) = *(&x_0);
	*(&(&ret_16)->y) = *(&y_4);
	return *(&ret_16);
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
float2 *  __operatorAddSet(float2 * a_0, float2 b_8, void* unused)
{
	*(&(*(&a_0))->x) += *(&(&b_8)->x);
	*(&(*(&a_0))->y) += *(&(&b_8)->y);
	return __nullcCheckedRet(__nullcTR[99], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float2 *  __operatorSubSet(float2 * a_0, float2 b_8, void* unused)
{
	*(&(*(&a_0))->x) -= *(&(&b_8)->x);
	*(&(*(&a_0))->y) -= *(&(&b_8)->y);
	return __nullcCheckedRet(__nullcTR[99], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float2 *  __operatorMulSet(float2 * a_0, float b_8, void* unused)
{
	*(&(*(&a_0))->x) *= *(&b_8);
	*(&(*(&a_0))->y) *= *(&b_8);
	return __nullcCheckedRet(__nullcTR[99], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float2 *  __operatorDivSet(float2 * a_0, float b_8, void* unused)
{
	*(&(*(&a_0))->x) /= *(&b_8);
	*(&(*(&a_0))->y) /= *(&b_8);
	return __nullcCheckedRet(__nullcTR[99], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float3  float3__float3_float3_ref_float_float_float_(float x_0, float y_4, float z_8, float3 * __context)
{
	float3 res_20;
	*(&(&res_20)->x) = *(&x_0);
	*(&(&res_20)->y) = *(&y_4);
	*(&(&res_20)->z) = *(&z_8);
	return *(&res_20);
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
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), (float3 * )(*(&__context)));
}
float3  float3__xxy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), (float3 * )(*(&__context)));
}
float3  float3__xxz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), (float3 * )(*(&__context)));
}
float3  float3__xyx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), (float3 * )(*(&__context)));
}
float3  float3__xyy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), (float3 * )(*(&__context)));
}
float3  float3__xyz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), (float3 * )(*(&__context)));
}
void  float3__xyz__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float3__xzx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), (float3 * )(*(&__context)));
}
float3  float3__xzy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), (float3 * )(*(&__context)));
}
void  float3__xzy__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->x) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float3__xzz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), (float3 * )(*(&__context)));
}
float3  float3__yxx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), (float3 * )(*(&__context)));
}
float3  float3__yxy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), (float3 * )(*(&__context)));
}
float3  float3__yxz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), (float3 * )(*(&__context)));
}
void  float3__yxz__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->z) = *(&(&r_0)->z);
}
float3  float3__yyx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), (float3 * )(*(&__context)));
}
float3  float3__yyy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), (float3 * )(*(&__context)));
}
float3  float3__yyz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), (float3 * )(*(&__context)));
}
float3  float3__yzx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), (float3 * )(*(&__context)));
}
void  float3__yzx__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->y) = *(&(&r_0)->x);
	*(&(*(&__context))->z) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float3__yzy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), (float3 * )(*(&__context)));
}
float3  float3__yzz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), (float3 * )(*(&__context)));
}
float3  float3__zxx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), (float3 * )(*(&__context)));
}
float3  float3__zxy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), (float3 * )(*(&__context)));
}
void  float3__zxy__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->x) = *(&(&r_0)->y);
	*(&(*(&__context))->y) = *(&(&r_0)->z);
}
float3  float3__zxz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), (float3 * )(*(&__context)));
}
float3  float3__zyx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), (float3 * )(*(&__context)));
}
void  float3__zyx__void_ref_float3_(float3 r_0, float3 * __context)
{
	*(&(*(&__context))->z) = *(&(&r_0)->x);
	*(&(*(&__context))->y) = *(&(&r_0)->y);
	*(&(*(&__context))->x) = *(&(&r_0)->z);
}
float3  float3__zyy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), (float3 * )(*(&__context)));
}
float3  float3__zyz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), (float3 * )(*(&__context)));
}
float3  float3__zzx__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), (float3 * )(*(&__context)));
}
float3  float3__zzy__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), (float3 * )(*(&__context)));
}
float3  float3__zzz__float3_ref__(float3 * __context)
{
	return float3__float3_float3_ref_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), (float3 * )(*(&__context)));
}
float3  float3__(float x_0, float y_4, float z_8, void* unused)
{
	float3 ret_20;
	*(&(&ret_20)->x) = *(&x_0);
	*(&(&ret_20)->y) = *(&y_4);
	*(&(&ret_20)->z) = *(&z_8);
	return *(&ret_20);
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
float3 *  __operatorAddSet(float3 * a_0, float3 b_8, void* unused)
{
	*(&(*(&a_0))->x) += *(&(&b_8)->x);
	*(&(*(&a_0))->y) += *(&(&b_8)->y);
	*(&(*(&a_0))->z) += *(&(&b_8)->z);
	return __nullcCheckedRet(__nullcTR[110], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float3 *  __operatorSubSet(float3 * a_0, float3 b_8, void* unused)
{
	*(&(*(&a_0))->x) -= *(&(&b_8)->x);
	*(&(*(&a_0))->y) -= *(&(&b_8)->y);
	*(&(*(&a_0))->z) -= *(&(&b_8)->z);
	return __nullcCheckedRet(__nullcTR[110], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float3 *  __operatorMulSet(float3 * a_0, float b_8, void* unused)
{
	*(&(*(&a_0))->x) *= *(&b_8);
	*(&(*(&a_0))->y) *= *(&b_8);
	*(&(*(&a_0))->z) *= *(&b_8);
	return __nullcCheckedRet(__nullcTR[110], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float3 *  __operatorDivSet(float3 * a_0, float b_8, void* unused)
{
	*(&(*(&a_0))->x) /= *(&b_8);
	*(&(*(&a_0))->y) /= *(&b_8);
	*(&(*(&a_0))->z) /= *(&b_8);
	return __nullcCheckedRet(__nullcTR[110], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float4  float4__float4_float4_ref_float_float_float_float_(float x_0, float y_4, float z_8, float w_12, float4 * __context)
{
	float4 res_24;
	*(&(&res_24)->x) = *(&x_0);
	*(&(&res_24)->y) = *(&y_4);
	*(&(&res_24)->z) = *(&z_8);
	*(&(&res_24)->w) = *(&w_12);
	return *(&res_24);
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xxxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xxxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xxxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xxyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xxyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xxyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xxyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xxzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xxzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xxzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xxzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xxwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xxwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xxwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xxww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xyxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xyxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xyxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xyxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xyyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xyyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xyyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xyyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xyzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xyzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xyzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xyzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xywy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xywz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xzxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xzxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xzxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xzxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xzyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xzyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xzyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xzyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xzzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xzzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xzzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xzwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xzwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xzww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xwxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xwxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xwxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xwxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xwyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xwyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xwyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xwzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xwzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xwzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__xwwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__xwwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__xwwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__xwww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yxxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yxxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yxxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yxxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yxyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yxyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yxyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yxyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yxzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yxzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yxzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yxzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yxwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yxwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yyxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yyxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yyxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yyxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yyyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yyyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yyyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yyyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yyzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yyzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yyzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yyzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yywx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yywy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yywz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yyww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yzxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yzxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yzxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yzxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yzyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yzyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yzyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yzzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__yzzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yzzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yzzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__yzwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__yzwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__yzww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__ywxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__ywxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__ywxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__ywyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__ywyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__ywyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__ywyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__ywzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__ywzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__ywzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__ywwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__ywwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__ywwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__ywww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zxxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zxxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zxxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zxxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zxyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zxyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zxyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zxyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zxzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zxzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zxzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zxwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zxwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zxww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zyxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zyxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zyxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zyxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zyyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zyyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zyyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zyzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zyzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zyzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zyzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zywx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zywz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zyww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zzxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zzxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zzxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zzxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zzyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zzyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zzyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zzyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zzzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zzzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zzzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zzzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zzwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zzwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zzwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zzww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zwxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zwxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zwxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zwyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zwyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zwyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zwzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zwzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zwzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zwzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__zwwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__zwwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__zwwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__zwww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wxxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wxxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wxxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wxxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wxyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wxyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wxyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wxzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wxzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wxzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wxwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wxwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wxwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wxww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wyxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wyxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wyxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wyyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wyyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wyyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wyyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wyzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wyzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wyzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wywx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wywy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wywz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wyww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wzxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wzxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wzxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wzyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
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
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wzyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wzyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wzzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wzzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wzzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wzzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wzwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wzwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wzwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wzww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wwxx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wwxy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wwxz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wwxw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wwyx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wwyy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wwyz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wwyw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wwzx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wwzy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wwzz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wwzw__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__wwwx__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->x), (float4 * )(*(&__context)));
}
float4  float4__wwwy__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->y), (float4 * )(*(&__context)));
}
float4  float4__wwwz__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->z), (float4 * )(*(&__context)));
}
float4  float4__wwww__float4_ref__(float4 * __context)
{
	return float4__float4_float4_ref_float_float_float_float_(*(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), *(&(*(&__context))->w), (float4 * )(*(&__context)));
}
float4  float4__(float x_0, float y_4, float z_8, float w_12, void* unused)
{
	float4 ret_24;
	*(&(&ret_24)->x) = *(&x_0);
	*(&(&ret_24)->y) = *(&y_4);
	*(&(&ret_24)->z) = *(&z_8);
	*(&(&ret_24)->w) = *(&w_12);
	return *(&ret_24);
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
float4 *  __operatorAddSet(float4 * a_0, float4 b_8, void* unused)
{
	*(&(*(&a_0))->x) += *(&(&b_8)->x);
	*(&(*(&a_0))->y) += *(&(&b_8)->y);
	*(&(*(&a_0))->z) += *(&(&b_8)->z);
	*(&(*(&a_0))->w) += *(&(&b_8)->w);
	return __nullcCheckedRet(__nullcTR[121], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float4 *  __operatorSubSet(float4 * a_0, float4 b_8, void* unused)
{
	*(&(*(&a_0))->x) -= *(&(&b_8)->x);
	*(&(*(&a_0))->y) -= *(&(&b_8)->y);
	*(&(*(&a_0))->z) -= *(&(&b_8)->z);
	*(&(*(&a_0))->w) -= *(&(&b_8)->w);
	return __nullcCheckedRet(__nullcTR[121], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float4 *  __operatorMulSet(float4 * a_0, float b_8, void* unused)
{
	*(&(*(&a_0))->x) *= *(&b_8);
	*(&(*(&a_0))->y) *= *(&b_8);
	*(&(*(&a_0))->z) *= *(&b_8);
	*(&(*(&a_0))->w) *= *(&b_8);
	return __nullcCheckedRet(__nullcTR[121], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
}
float4 *  __operatorDivSet(float4 * a_0, float b_8, void* unused)
{
	*(&(*(&a_0))->x) /= *(&b_8);
	*(&(*(&a_0))->y) /= *(&b_8);
	*(&(*(&a_0))->z) /= *(&b_8);
	*(&(*(&a_0))->w) /= *(&b_8);
	return __nullcCheckedRet(__nullcTR[121], (*(&a_0)), (void*)&a_0, (void*)&b_8, (void*)&unused, 0);
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
float4 *  __operatorSet(float4 * a_0, double_4_ xyzw_8, void* unused)
{
	*(*(&a_0)) = float4__((float )(*(&(&xyzw_8)->ptr[__nullcIndex(0, 4u)])), (float )(*(&(&xyzw_8)->ptr[__nullcIndex(1, 4u)])), (float )(*(&(&xyzw_8)->ptr[__nullcIndex(2, 4u)])), (float )(*(&(&xyzw_8)->ptr[__nullcIndex(3, 4u)])), (void*)0);
	return __nullcCheckedRet(__nullcTR[121], (*(&a_0)), (void*)&a_0, (void*)&xyzw_8, (void*)&unused, 0);
}
float3  reflect(float3 normal_0, float3 dir_12, void* unused)
{
	return __operatorSub(*(&dir_12), __operatorMul((float )((2.000000) * (double )(dot(&normal_0, &dir_12, (void*)0))), *(&normal_0), (void*)0), (void*)0);
}
int __init_std_math_nc()
{
	static int moduleInitialized = 0;
	if(moduleInitialized++)
		return 0;
	__nullcFM = __nullcGetFunctionTable();
	int __local = 0;
	__nullcRegisterBase((void*)&__local);
	__nullcTR[0] = __nullcRegisterType(2090838615u, "void", 0, __nullcTR[0], 0, 0);
	__nullcTR[1] = __nullcRegisterType(4181547808u, "double", 8, __nullcTR[0], 0, 0);
	__nullcTR[2] = __nullcRegisterType(259121563u, "float", 4, __nullcTR[0], 0, 0);
	__nullcTR[3] = __nullcRegisterType(2090479413u, "long", 8, __nullcTR[0], 0, 0);
	__nullcTR[4] = __nullcRegisterType(193495088u, "int", 4, __nullcTR[0], 0, 0);
	__nullcTR[5] = __nullcRegisterType(274395349u, "short", 2, __nullcTR[0], 0, 0);
	__nullcTR[6] = __nullcRegisterType(2090147939u, "char", 1, __nullcTR[0], 0, 0);
	__nullcTR[7] = __nullcRegisterType(1166360283u, "auto ref", 12, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[8] = __nullcRegisterType(524429492u, "typeid", 4, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[9] = __nullcRegisterType(3198057556u, "void ref", 8, __nullcTR[0], 1, NULLC_POINTER);
	__nullcTR[10] = __nullcRegisterType(4071234806u, "auto[]", 16, __nullcTR[0], 3, NULLC_CLASS);
	__nullcTR[11] = __nullcRegisterType(3150998963u, "auto ref[]", 12, __nullcTR[7], -1, NULLC_ARRAY);
	__nullcTR[12] = __nullcRegisterType(2550963152u, "void ref(int)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[13] = __nullcRegisterType(4133409083u, "char[]", 12, __nullcTR[6], -1, NULLC_ARRAY);
	__nullcTR[14] = __nullcRegisterType(3878423506u, "void ref(int,char[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[15] = __nullcRegisterType(1362586038u, "int ref(char[],char[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[16] = __nullcRegisterType(3953727713u, "char[] ref(char[],char[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[17] = __nullcRegisterType(3214832952u, "char[] ref", 8, __nullcTR[13], 1, NULLC_POINTER);
	__nullcTR[18] = __nullcRegisterType(90259294u, "char[] ref(char[] ref,char[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[19] = __nullcRegisterType(3410585167u, "char ref(char)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[20] = __nullcRegisterType(1890834067u, "short ref(short)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[21] = __nullcRegisterType(2745832905u, "int ref(int)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[22] = __nullcRegisterType(3458960563u, "long ref(long)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[23] = __nullcRegisterType(4223928607u, "float ref(float)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[24] = __nullcRegisterType(4226161577u, "double ref(double)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[25] = __nullcRegisterType(2570056003u, "void ref(char)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[26] = __nullcRegisterType(154671200u, "char ref", 8, __nullcTR[6], 1, NULLC_POINTER);
	__nullcTR[27] = __nullcRegisterType(2657142493u, "char ref ref", 8, __nullcTR[26], 1, NULLC_POINTER);
	__nullcTR[28] = __nullcRegisterType(3834141397u, "void ref(short)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[29] = __nullcRegisterType(3010777554u, "short ref", 8, __nullcTR[5], 1, NULLC_POINTER);
	__nullcTR[30] = __nullcRegisterType(576776527u, "short ref ref", 8, __nullcTR[29], 1, NULLC_POINTER);
	__nullcTR[31] = __nullcRegisterType(2671810221u, "int ref", 8, __nullcTR[4], 1, NULLC_POINTER);
	__nullcTR[32] = __nullcRegisterType(3857294250u, "int ref ref", 8, __nullcTR[31], 1, NULLC_POINTER);
	__nullcTR[33] = __nullcRegisterType(2580994645u, "void ref(long)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[34] = __nullcRegisterType(2414624818u, "long ref", 8, __nullcTR[3], 1, NULLC_POINTER);
	__nullcTR[35] = __nullcRegisterType(799573935u, "long ref ref", 8, __nullcTR[34], 1, NULLC_POINTER);
	__nullcTR[36] = __nullcRegisterType(3330106459u, "void ref(float)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[37] = __nullcRegisterType(1384297880u, "float ref", 8, __nullcTR[2], 1, NULLC_POINTER);
	__nullcTR[38] = __nullcRegisterType(2577874965u, "float ref ref", 8, __nullcTR[37], 1, NULLC_POINTER);
	__nullcTR[39] = __nullcRegisterType(60945760u, "void ref(double)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[40] = __nullcRegisterType(3234425245u, "double ref", 8, __nullcTR[1], 1, NULLC_POINTER);
	__nullcTR[41] = __nullcRegisterType(1954705050u, "double ref ref", 8, __nullcTR[40], 1, NULLC_POINTER);
	__nullcTR[42] = __nullcRegisterType(554739849u, "char[] ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[43] = __nullcRegisterType(2745156404u, "char[] ref(int)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[44] = __nullcRegisterType(2528639597u, "void ref ref(int)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[45] = __nullcRegisterType(262756424u, "int[]", 12, __nullcTR[4], -1, NULLC_ARRAY);
	__nullcTR[46] = __nullcRegisterType(1780011448u, "int[] ref(int,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[47] = __nullcRegisterType(3724107199u, "auto ref ref(auto ref)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[48] = __nullcRegisterType(1137649267u, "auto[] ref", 8, __nullcTR[10], 1, NULLC_POINTER);
	__nullcTR[49] = __nullcRegisterType(665024592u, "void ref(auto[] ref,auto[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[50] = __nullcRegisterType(2649967381u, "auto[] ref(auto[])", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[51] = __nullcRegisterType(2430896065u, "auto ref ref(auto ref,auto ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[52] = __nullcRegisterType(3364104125u, "void ref(auto ref,auto ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[53] = __nullcRegisterType(4131070326u, "int ref(auto ref,auto ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[54] = __nullcRegisterType(3761170085u, "void ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[55] = __nullcRegisterType(4190091973u, "int[] ref", 8, __nullcTR[45], 1, NULLC_POINTER);
	__nullcTR[56] = __nullcRegisterType(844911189u, "void ref() ref(auto ref,int[] ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[57] = __nullcRegisterType(2816069557u, "char[] ref ref", 8, __nullcTR[17], 1, NULLC_POINTER);
	__nullcTR[58] = __nullcRegisterType(2985493640u, "char[] ref ref(char[] ref,int[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[59] = __nullcRegisterType(2463794733u, "short[]", 12, __nullcTR[5], -1, NULLC_ARRAY);
	__nullcTR[60] = __nullcRegisterType(3958330154u, "short[] ref", 8, __nullcTR[59], 1, NULLC_POINTER);
	__nullcTR[61] = __nullcRegisterType(745297575u, "short[] ref ref", 8, __nullcTR[60], 1, NULLC_POINTER);
	__nullcTR[62] = __nullcRegisterType(1935747820u, "short[] ref ref(short[] ref,int[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[63] = __nullcRegisterType(3010510963u, "float[]", 12, __nullcTR[2], -1, NULLC_ARRAY);
	__nullcTR[64] = __nullcRegisterType(2248491120u, "float[] ref", 8, __nullcTR[63], 1, NULLC_POINTER);
	__nullcTR[65] = __nullcRegisterType(1040232248u, "double[]", 12, __nullcTR[1], -1, NULLC_ARRAY);
	__nullcTR[66] = __nullcRegisterType(2393077485u, "float[] ref ref", 8, __nullcTR[64], 1, NULLC_POINTER);
	__nullcTR[67] = __nullcRegisterType(2697529781u, "double[] ref", 8, __nullcTR[65], 1, NULLC_POINTER);
	__nullcTR[68] = __nullcRegisterType(2467461000u, "float[] ref ref(float[] ref,double[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[69] = __nullcRegisterType(2066091864u, "typeid ref(auto ref)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[70] = __nullcRegisterType(1908472638u, "int ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[71] = __nullcRegisterType(1268871368u, "int ref(typeid,typeid)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[72] = __nullcRegisterType(2688408224u, "int ref(void ref(int),void ref(int))", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[73] = __nullcRegisterType(451037873u, "auto[] ref ref(auto[] ref,auto ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[74] = __nullcRegisterType(3824954777u, "auto ref ref(auto ref,auto[] ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[75] = __nullcRegisterType(3832966281u, "auto[] ref ref(auto[] ref,auto[] ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[76] = __nullcRegisterType(477490926u, "auto ref ref(auto[] ref,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[77] = __nullcRegisterType(3720955042u, "const_string", 12, __nullcTR[0], 1, NULLC_CLASS);
	__nullcTR[78] = __nullcRegisterType(1951548447u, "const_string ref", 8, __nullcTR[77], 1, NULLC_POINTER);
	__nullcTR[79] = __nullcRegisterType(504936988u, "const_string ref ref", 8, __nullcTR[78], 1, NULLC_POINTER);
	__nullcTR[80] = __nullcRegisterType(3099572969u, "const_string ref ref(const_string ref,char[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[81] = __nullcRegisterType(2016055718u, "const_string ref(char[])", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[82] = __nullcRegisterType(195623906u, "char ref(const_string ref,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[83] = __nullcRegisterType(3956827940u, "int ref(const_string,const_string)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[84] = __nullcRegisterType(3539544093u, "int ref(const_string,char[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[85] = __nullcRegisterType(730969981u, "int ref(char[],const_string)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[86] = __nullcRegisterType(727035222u, "const_string ref(const_string,const_string)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[87] = __nullcRegisterType(1003630799u, "const_string ref(const_string,char[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[88] = __nullcRegisterType(2490023983u, "const_string ref(char[],const_string)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[89] = __nullcRegisterType(3335638996u, "int ref(auto ref)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[90] = __nullcRegisterType(4064539429u, "void ref(auto[] ref,typeid,int)", 12, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[91] = __nullcRegisterType(3726221418u, "auto[] ref(typeid,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[92] = __nullcRegisterType(3504529713u, "typeid ref", 8, __nullcTR[8], 1, NULLC_POINTER);
	__nullcTR[93] = __nullcRegisterType(527603730u, "void ref(auto ref,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[94] = __nullcRegisterType(4086116458u, "void ref(auto[] ref,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[95] = __nullcRegisterType(1812738619u, "void ref(auto ref)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[96] = __nullcRegisterType(6493228u, "MathConstants", 16, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[97] = __nullcRegisterType(3875326121u, "MathConstants ref", 8, __nullcTR[96], 1, NULLC_POINTER);
	__nullcTR[98] = __nullcRegisterType(1895912087u, "double ref(double,double,double)", 12, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[99] = __nullcRegisterType(4256044333u, "float2", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[100] = __nullcRegisterType(2568615123u, "float2 ref(float,float)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[101] = __nullcRegisterType(2750571050u, "float2 ref", 8, __nullcTR[99], 1, NULLC_POINTER);
	__nullcTR[102] = __nullcRegisterType(1779669499u, "float2 ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[103] = __nullcRegisterType(1029629351u, "float2 ref ref", 8, __nullcTR[101], 1, NULLC_POINTER);
	__nullcTR[104] = __nullcRegisterType(2519331085u, "void ref(float2)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[105] = __nullcRegisterType(2810325943u, "float2 ref(float2,float2)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[106] = __nullcRegisterType(3338924485u, "float2 ref(float2,float)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[107] = __nullcRegisterType(3159920773u, "float2 ref(float,float2)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[108] = __nullcRegisterType(583224465u, "float2 ref ref(float2 ref,float2)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[109] = __nullcRegisterType(668426079u, "float2 ref ref(float2 ref,float)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[110] = __nullcRegisterType(4256044334u, "float3", 12, __nullcTR[0], 3, NULLC_CLASS);
	__nullcTR[111] = __nullcRegisterType(3886564502u, "float3 ref(float,float,float)", 12, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[112] = __nullcRegisterType(2751756971u, "float3 ref", 8, __nullcTR[110], 1, NULLC_POINTER);
	__nullcTR[113] = __nullcRegisterType(2983941800u, "float3 ref ref", 8, __nullcTR[112], 1, NULLC_POINTER);
	__nullcTR[114] = __nullcRegisterType(3071137468u, "float3 ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[115] = __nullcRegisterType(2519331118u, "void ref(float3)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[116] = __nullcRegisterType(3894790970u, "float3 ref(float3,float3)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[117] = __nullcRegisterType(1679830247u, "float3 ref(float3,float)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[118] = __nullcRegisterType(1832056551u, "float3 ref(float,float3)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[119] = __nullcRegisterType(4106114452u, "float3 ref ref(float3 ref,float3)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[120] = __nullcRegisterType(1816384513u, "float3 ref ref(float3 ref,float)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[121] = __nullcRegisterType(4256044335u, "float4", 16, __nullcTR[0], 4, NULLC_CLASS);
	__nullcTR[122] = __nullcRegisterType(247594841u, "float4 ref(float,float,float,float)", 12, __nullcTR[0], 4, NULLC_FUNCTION);
	__nullcTR[123] = __nullcRegisterType(2752942892u, "float4 ref", 8, __nullcTR[121], 1, NULLC_POINTER);
	__nullcTR[124] = __nullcRegisterType(643286953u, "float4 ref ref", 8, __nullcTR[123], 1, NULLC_POINTER);
	__nullcTR[125] = __nullcRegisterType(67638141u, "float4 ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[126] = __nullcRegisterType(2519331151u, "void ref(float4)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[127] = __nullcRegisterType(684288701u, "float4 ref(float4,float4)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[128] = __nullcRegisterType(20736009u, "float4 ref(float4,float)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[129] = __nullcRegisterType(504192329u, "float4 ref(float,float4)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[130] = __nullcRegisterType(3334037143u, "float4 ref ref(float4 ref,float4)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[131] = __nullcRegisterType(2964342947u, "float4 ref ref(float4 ref,float)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[132] = __nullcRegisterType(2011060230u, "float3 ref(float2,float)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[133] = __nullcRegisterType(1832056518u, "float3 ref(float,float2)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[134] = __nullcRegisterType(351965992u, "float4 ref(float3,float)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[135] = __nullcRegisterType(1070631033u, "float4 ref(float2,float2)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[136] = __nullcRegisterType(3253984809u, "float4 ref(float2,float,float)", 12, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[137] = __nullcRegisterType(2294885289u, "float4 ref(float,float,float2)", 12, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[138] = __nullcRegisterType(504192296u, "float4 ref(float,float3)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[139] = __nullcRegisterType(4262891852u, "double[4]", 32, __nullcTR[1], 4, NULLC_ARRAY);
	__nullcTR[140] = __nullcRegisterType(3368457716u, "float4 ref ref(float4 ref,double[4])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[141] = __nullcRegisterType(1583994313u, "double[4] ref", 8, __nullcTR[139], 1, NULLC_POINTER);
	__nullcTR[142] = __nullcRegisterType(562572443u, "float4x4", 64, __nullcTR[0], 4, NULLC_CLASS);
	__nullcTR[143] = __nullcRegisterType(3309272130u, "float ref ref(float2 ref,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[144] = __nullcRegisterType(3377073507u, "float ref ref(float3 ref,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[145] = __nullcRegisterType(3444874884u, "float ref ref(float4 ref,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[146] = __nullcRegisterType(4261839081u, "float ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[147] = __nullcRegisterType(2081580927u, "float ref(float2 ref,float2 ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[148] = __nullcRegisterType(289501729u, "float ref(float3 ref,float3 ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[149] = __nullcRegisterType(2792389827u, "float ref(float4 ref,float4 ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcRegisterMembers(__nullcTR[7], 2, __nullcTR[8], 0, __nullcTR[9], 4);
	__nullcRegisterMembers(__nullcTR[8], 0);
	__nullcRegisterMembers(__nullcTR[10], 3, __nullcTR[8], 0, __nullcTR[9], 4, __nullcTR[4], 12);
	__nullcRegisterMembers(__nullcTR[77], 1, __nullcTR[13], 0);
	__nullcRegisterMembers(__nullcTR[96], 2, __nullcTR[1], 0, __nullcTR[1], 8);
	__nullcRegisterMembers(__nullcTR[99], 2, __nullcTR[2], 0, __nullcTR[2], 4);
	__nullcRegisterMembers(__nullcTR[110], 3, __nullcTR[2], 0, __nullcTR[2], 4, __nullcTR[2], 8);
	__nullcRegisterMembers(__nullcTR[121], 4, __nullcTR[2], 0, __nullcTR[2], 4, __nullcTR[2], 8, __nullcTR[2], 12);
	__nullcRegisterMembers(__nullcTR[142], 4, __nullcTR[121], 0, __nullcTR[121], 16, __nullcTR[121], 32, __nullcTR[121], 48);
	__nullcRegisterGlobal((void*)&math, __nullcTR[96]);
	__nullcFR[0] = 0;
	__nullcFR[1] = 0;
	__nullcFR[2] = 0;
	__nullcFR[3] = 0;
	__nullcFR[4] = 0;
	__nullcFR[5] = 0;
	__nullcFR[6] = __nullcRegisterFunction("char__", (void*)char__, 4294967295u, 0);
	__nullcFR[7] = __nullcRegisterFunction("short__", (void*)short__, 4294967295u, 0);
	__nullcFR[8] = __nullcRegisterFunction("int__", (void*)int__, 4294967295u, 0);
	__nullcFR[9] = __nullcRegisterFunction("long__", (void*)long__, 4294967295u, 0);
	__nullcFR[10] = __nullcRegisterFunction("float__", (void*)float__, 4294967295u, 0);
	__nullcFR[11] = __nullcRegisterFunction("double__", (void*)double__, 4294967295u, 0);
	__nullcFR[12] = __nullcRegisterFunction("char__char_void_ref_char_", (void*)char__char_void_ref_char_, 26u, 2);
	__nullcFR[13] = __nullcRegisterFunction("short__short_void_ref_short_", (void*)short__short_void_ref_short_, 29u, 2);
	__nullcFR[14] = __nullcRegisterFunction("int__int_void_ref_int_", (void*)int__int_void_ref_int_, 31u, 2);
	__nullcFR[15] = __nullcRegisterFunction("long__long_void_ref_long_", (void*)long__long_void_ref_long_, 34u, 2);
	__nullcFR[16] = __nullcRegisterFunction("float__float_void_ref_float_", (void*)float__float_void_ref_float_, 37u, 2);
	__nullcFR[17] = __nullcRegisterFunction("double__double_void_ref_double_", (void*)double__double_void_ref_double_, 40u, 2);
	__nullcFR[18] = __nullcRegisterFunction("int__str_char___ref__", (void*)int__str_char___ref__, 31u, 2);
	__nullcFR[19] = __nullcRegisterFunction("double__str_char___ref_int_", (void*)double__str_char___ref_int_, 40u, 2);
	__nullcFR[20] = __nullcRegisterFunction("__newS", (void*)__newS, 4294967295u, 0);
	__nullcFR[21] = __nullcRegisterFunction("__newA", (void*)__newA, 4294967295u, 0);
	__nullcFR[22] = 0;
	__nullcFR[23] = __nullcRegisterFunction("__duplicate_array", (void*)__duplicate_array, 4294967295u, 0);
	__nullcFR[24] = 0;
	__nullcFR[25] = __nullcRegisterFunction("replace", (void*)replace, 4294967295u, 0);
	__nullcFR[26] = __nullcRegisterFunction("swap", (void*)swap, 4294967295u, 0);
	__nullcFR[27] = __nullcRegisterFunction("equal", (void*)equal, 4294967295u, 0);
	__nullcFR[28] = __nullcRegisterFunction("__redirect", (void*)__redirect, 4294967295u, 0);
	__nullcFR[29] = 0;
	__nullcFR[30] = 0;
	__nullcFR[31] = 0;
	__nullcFR[32] = __nullcRegisterFunction("typeid__", (void*)typeid__, 4294967295u, 0);
	__nullcFR[33] = __nullcRegisterFunction("typeid__size__int_ref__", (void*)typeid__size__int_ref__, 92u, 2);
	__nullcFR[34] = 0;
	__nullcFR[35] = 0;
	__nullcFR[36] = __nullcRegisterFunction("__rcomp", (void*)__rcomp, 4294967295u, 0);
	__nullcFR[37] = __nullcRegisterFunction("__rncomp", (void*)__rncomp, 4294967295u, 0);
	__nullcFR[38] = __nullcRegisterFunction("__pcomp", (void*)__pcomp, 4294967295u, 0);
	__nullcFR[39] = __nullcRegisterFunction("__pncomp", (void*)__pncomp, 4294967295u, 0);
	__nullcFR[40] = __nullcRegisterFunction("__typeCount", (void*)__typeCount, 4294967295u, 0);
	__nullcFR[41] = 0;
	__nullcFR[42] = 0;
	__nullcFR[43] = 0;
	__nullcFR[44] = 0;
	__nullcFR[45] = __nullcRegisterFunction("const_string__size__int_ref__", (void*)const_string__size__int_ref__, 78u, 2);
	__nullcFR[46] = 0;
	__nullcFR[47] = __nullcRegisterFunction("const_string__", (void*)const_string__, 4294967295u, 0);
	__nullcFR[48] = 0;
	__nullcFR[49] = 0;
	__nullcFR[50] = 0;
	__nullcFR[51] = 0;
	__nullcFR[52] = 0;
	__nullcFR[53] = 0;
	__nullcFR[54] = 0;
	__nullcFR[55] = 0;
	__nullcFR[56] = 0;
	__nullcFR[57] = 0;
	__nullcFR[58] = __nullcRegisterFunction("isStackPointer", (void*)isStackPointer, 4294967295u, 0);
	__nullcFR[59] = __nullcRegisterFunction("auto_array_impl", (void*)auto_array_impl, 4294967295u, 0);
	__nullcFR[60] = __nullcRegisterFunction("auto_array", (void*)auto_array, 4294967295u, 0);
	__nullcFR[61] = __nullcRegisterFunction("auto____set_void_ref_auto_ref_int_", (void*)auto____set_void_ref_auto_ref_int_, 48u, 2);
	__nullcFR[62] = __nullcRegisterFunction("__force_size", (void*)__force_size, 4294967295u, 0);
	__nullcFR[63] = __nullcRegisterFunction("isCoroutineReset", (void*)isCoroutineReset, 4294967295u, 0);
	__nullcFR[64] = __nullcRegisterFunction("__assertCoroutine", (void*)__assertCoroutine, 4294967295u, 0);
	__nullcFR[65] = __nullcRegisterFunction("__char_a_12", (void*)__char_a_12, 4294967295u, 0);
	__nullcFR[66] = __nullcRegisterFunction("__short_a_13", (void*)__short_a_13, 4294967295u, 0);
	__nullcFR[67] = __nullcRegisterFunction("__int_a_14", (void*)__int_a_14, 4294967295u, 0);
	__nullcFR[68] = __nullcRegisterFunction("__long_a_15", (void*)__long_a_15, 4294967295u, 0);
	__nullcFR[69] = __nullcRegisterFunction("__float_a_16", (void*)__float_a_16, 4294967295u, 0);
	__nullcFR[70] = __nullcRegisterFunction("__double_a_17", (void*)__double_a_17, 4294967295u, 0);
	__nullcFR[71] = __nullcRegisterFunction("__str_precision_19", (void*)__str_precision_19, 4294967295u, 0);
	__nullcFR[72] = __nullcRegisterFunction("cos", (void*)cos, 4294967295u, 0);
	__nullcFR[73] = __nullcRegisterFunction("sin", (void*)sin, 4294967295u, 0);
	__nullcFR[74] = __nullcRegisterFunction("tan", (void*)tan, 4294967295u, 0);
	__nullcFR[75] = __nullcRegisterFunction("ctg", (void*)ctg, 4294967295u, 0);
	__nullcFR[76] = __nullcRegisterFunction("cosh", (void*)cosh, 4294967295u, 0);
	__nullcFR[77] = __nullcRegisterFunction("sinh", (void*)sinh, 4294967295u, 0);
	__nullcFR[78] = __nullcRegisterFunction("tanh", (void*)tanh, 4294967295u, 0);
	__nullcFR[79] = __nullcRegisterFunction("coth", (void*)coth, 4294967295u, 0);
	__nullcFR[80] = __nullcRegisterFunction("acos", (void*)acos, 4294967295u, 0);
	__nullcFR[81] = __nullcRegisterFunction("asin", (void*)asin, 4294967295u, 0);
	__nullcFR[82] = __nullcRegisterFunction("atan", (void*)atan, 4294967295u, 0);
	__nullcFR[83] = __nullcRegisterFunction("ceil", (void*)ceil, 4294967295u, 0);
	__nullcFR[84] = __nullcRegisterFunction("floor", (void*)floor, 4294967295u, 0);
	__nullcFR[85] = __nullcRegisterFunction("exp", (void*)exp, 4294967295u, 0);
	__nullcFR[86] = __nullcRegisterFunction("log", (void*)log, 4294967295u, 0);
	__nullcFR[87] = __nullcRegisterFunction("sqrt", (void*)sqrt, 4294967295u, 0);
	__nullcFR[88] = __nullcRegisterFunction("clamp", (void*)clamp, 4294967295u, 0);
	__nullcFR[89] = __nullcRegisterFunction("saturate", (void*)saturate, 4294967295u, 0);
	__nullcFR[90] = __nullcRegisterFunction("abs", (void*)abs, 4294967295u, 0);
	__nullcFR[91] = __nullcRegisterFunction("float2__float2_float2_ref_float_float_", (void*)float2__float2_float2_ref_float_float_, 101u, 2);
	__nullcFR[92] = __nullcRegisterFunction("float2__xx__float2_ref__", (void*)float2__xx__float2_ref__, 101u, 2);
	__nullcFR[93] = __nullcRegisterFunction("float2__xy__float2_ref__", (void*)float2__xy__float2_ref__, 101u, 2);
	__nullcFR[94] = __nullcRegisterFunction("float2__xy__void_ref_float2_", (void*)float2__xy__void_ref_float2_, 101u, 2);
	__nullcFR[95] = __nullcRegisterFunction("float2__yx__float2_ref__", (void*)float2__yx__float2_ref__, 101u, 2);
	__nullcFR[96] = __nullcRegisterFunction("float2__yx__void_ref_float2_", (void*)float2__yx__void_ref_float2_, 101u, 2);
	__nullcFR[97] = __nullcRegisterFunction("float2__yy__float2_ref__", (void*)float2__yy__float2_ref__, 101u, 2);
	__nullcFR[98] = __nullcRegisterFunction("float2__", (void*)float2__, 31u, 0);
	__nullcFR[99] = 0;
	__nullcFR[100] = 0;
	__nullcFR[101] = 0;
	__nullcFR[102] = 0;
	__nullcFR[103] = 0;
	__nullcFR[104] = 0;
	__nullcFR[105] = 0;
	__nullcFR[106] = 0;
	__nullcFR[107] = 0;
	__nullcFR[108] = 0;
	__nullcFR[109] = 0;
	__nullcFR[110] = __nullcRegisterFunction("float3__float3_float3_ref_float_float_float_", (void*)float3__float3_float3_ref_float_float_float_, 112u, 2);
	__nullcFR[111] = __nullcRegisterFunction("float3__xx__float2_ref__", (void*)float3__xx__float2_ref__, 112u, 2);
	__nullcFR[112] = __nullcRegisterFunction("float3__xy__float2_ref__", (void*)float3__xy__float2_ref__, 112u, 2);
	__nullcFR[113] = __nullcRegisterFunction("float3__xy__void_ref_float2_", (void*)float3__xy__void_ref_float2_, 112u, 2);
	__nullcFR[114] = __nullcRegisterFunction("float3__xz__float2_ref__", (void*)float3__xz__float2_ref__, 112u, 2);
	__nullcFR[115] = __nullcRegisterFunction("float3__xz__void_ref_float2_", (void*)float3__xz__void_ref_float2_, 112u, 2);
	__nullcFR[116] = __nullcRegisterFunction("float3__yx__float2_ref__", (void*)float3__yx__float2_ref__, 112u, 2);
	__nullcFR[117] = __nullcRegisterFunction("float3__yx__void_ref_float2_", (void*)float3__yx__void_ref_float2_, 112u, 2);
	__nullcFR[118] = __nullcRegisterFunction("float3__yy__float2_ref__", (void*)float3__yy__float2_ref__, 112u, 2);
	__nullcFR[119] = __nullcRegisterFunction("float3__yz__float2_ref__", (void*)float3__yz__float2_ref__, 112u, 2);
	__nullcFR[120] = __nullcRegisterFunction("float3__yz__void_ref_float2_", (void*)float3__yz__void_ref_float2_, 112u, 2);
	__nullcFR[121] = __nullcRegisterFunction("float3__zx__float2_ref__", (void*)float3__zx__float2_ref__, 112u, 2);
	__nullcFR[122] = __nullcRegisterFunction("float3__zx__void_ref_float2_", (void*)float3__zx__void_ref_float2_, 112u, 2);
	__nullcFR[123] = __nullcRegisterFunction("float3__zy__float2_ref__", (void*)float3__zy__float2_ref__, 112u, 2);
	__nullcFR[124] = __nullcRegisterFunction("float3__zy__void_ref_float2_", (void*)float3__zy__void_ref_float2_, 112u, 2);
	__nullcFR[125] = __nullcRegisterFunction("float3__zz__float2_ref__", (void*)float3__zz__float2_ref__, 112u, 2);
	__nullcFR[126] = __nullcRegisterFunction("float3__xxx__float3_ref__", (void*)float3__xxx__float3_ref__, 112u, 2);
	__nullcFR[127] = __nullcRegisterFunction("float3__xxy__float3_ref__", (void*)float3__xxy__float3_ref__, 112u, 2);
	__nullcFR[128] = __nullcRegisterFunction("float3__xxz__float3_ref__", (void*)float3__xxz__float3_ref__, 112u, 2);
	__nullcFR[129] = __nullcRegisterFunction("float3__xyx__float3_ref__", (void*)float3__xyx__float3_ref__, 112u, 2);
	__nullcFR[130] = __nullcRegisterFunction("float3__xyy__float3_ref__", (void*)float3__xyy__float3_ref__, 112u, 2);
	__nullcFR[131] = __nullcRegisterFunction("float3__xyz__float3_ref__", (void*)float3__xyz__float3_ref__, 112u, 2);
	__nullcFR[132] = __nullcRegisterFunction("float3__xyz__void_ref_float3_", (void*)float3__xyz__void_ref_float3_, 112u, 2);
	__nullcFR[133] = __nullcRegisterFunction("float3__xzx__float3_ref__", (void*)float3__xzx__float3_ref__, 112u, 2);
	__nullcFR[134] = __nullcRegisterFunction("float3__xzy__float3_ref__", (void*)float3__xzy__float3_ref__, 112u, 2);
	__nullcFR[135] = __nullcRegisterFunction("float3__xzy__void_ref_float3_", (void*)float3__xzy__void_ref_float3_, 112u, 2);
	__nullcFR[136] = __nullcRegisterFunction("float3__xzz__float3_ref__", (void*)float3__xzz__float3_ref__, 112u, 2);
	__nullcFR[137] = __nullcRegisterFunction("float3__yxx__float3_ref__", (void*)float3__yxx__float3_ref__, 112u, 2);
	__nullcFR[138] = __nullcRegisterFunction("float3__yxy__float3_ref__", (void*)float3__yxy__float3_ref__, 112u, 2);
	__nullcFR[139] = __nullcRegisterFunction("float3__yxz__float3_ref__", (void*)float3__yxz__float3_ref__, 112u, 2);
	__nullcFR[140] = __nullcRegisterFunction("float3__yxz__void_ref_float3_", (void*)float3__yxz__void_ref_float3_, 112u, 2);
	__nullcFR[141] = __nullcRegisterFunction("float3__yyx__float3_ref__", (void*)float3__yyx__float3_ref__, 112u, 2);
	__nullcFR[142] = __nullcRegisterFunction("float3__yyy__float3_ref__", (void*)float3__yyy__float3_ref__, 112u, 2);
	__nullcFR[143] = __nullcRegisterFunction("float3__yyz__float3_ref__", (void*)float3__yyz__float3_ref__, 112u, 2);
	__nullcFR[144] = __nullcRegisterFunction("float3__yzx__float3_ref__", (void*)float3__yzx__float3_ref__, 112u, 2);
	__nullcFR[145] = __nullcRegisterFunction("float3__yzx__void_ref_float3_", (void*)float3__yzx__void_ref_float3_, 112u, 2);
	__nullcFR[146] = __nullcRegisterFunction("float3__yzy__float3_ref__", (void*)float3__yzy__float3_ref__, 112u, 2);
	__nullcFR[147] = __nullcRegisterFunction("float3__yzz__float3_ref__", (void*)float3__yzz__float3_ref__, 112u, 2);
	__nullcFR[148] = __nullcRegisterFunction("float3__zxx__float3_ref__", (void*)float3__zxx__float3_ref__, 112u, 2);
	__nullcFR[149] = __nullcRegisterFunction("float3__zxy__float3_ref__", (void*)float3__zxy__float3_ref__, 112u, 2);
	__nullcFR[150] = __nullcRegisterFunction("float3__zxy__void_ref_float3_", (void*)float3__zxy__void_ref_float3_, 112u, 2);
	__nullcFR[151] = __nullcRegisterFunction("float3__zxz__float3_ref__", (void*)float3__zxz__float3_ref__, 112u, 2);
	__nullcFR[152] = __nullcRegisterFunction("float3__zyx__float3_ref__", (void*)float3__zyx__float3_ref__, 112u, 2);
	__nullcFR[153] = __nullcRegisterFunction("float3__zyx__void_ref_float3_", (void*)float3__zyx__void_ref_float3_, 112u, 2);
	__nullcFR[154] = __nullcRegisterFunction("float3__zyy__float3_ref__", (void*)float3__zyy__float3_ref__, 112u, 2);
	__nullcFR[155] = __nullcRegisterFunction("float3__zyz__float3_ref__", (void*)float3__zyz__float3_ref__, 112u, 2);
	__nullcFR[156] = __nullcRegisterFunction("float3__zzx__float3_ref__", (void*)float3__zzx__float3_ref__, 112u, 2);
	__nullcFR[157] = __nullcRegisterFunction("float3__zzy__float3_ref__", (void*)float3__zzy__float3_ref__, 112u, 2);
	__nullcFR[158] = __nullcRegisterFunction("float3__zzz__float3_ref__", (void*)float3__zzz__float3_ref__, 112u, 2);
	__nullcFR[159] = 0;
	__nullcFR[160] = 0;
	__nullcFR[161] = 0;
	__nullcFR[162] = 0;
	__nullcFR[163] = 0;
	__nullcFR[164] = 0;
	__nullcFR[165] = 0;
	__nullcFR[166] = 0;
	__nullcFR[167] = 0;
	__nullcFR[168] = 0;
	__nullcFR[169] = 0;
	__nullcFR[170] = 0;
	__nullcFR[171] = __nullcRegisterFunction("float4__float4_float4_ref_float_float_float_float_", (void*)float4__float4_float4_ref_float_float_float_float_, 123u, 2);
	__nullcFR[172] = __nullcRegisterFunction("float4__xx__float2_ref__", (void*)float4__xx__float2_ref__, 123u, 2);
	__nullcFR[173] = __nullcRegisterFunction("float4__xy__float2_ref__", (void*)float4__xy__float2_ref__, 123u, 2);
	__nullcFR[174] = __nullcRegisterFunction("float4__xy__void_ref_float2_", (void*)float4__xy__void_ref_float2_, 123u, 2);
	__nullcFR[175] = __nullcRegisterFunction("float4__xz__float2_ref__", (void*)float4__xz__float2_ref__, 123u, 2);
	__nullcFR[176] = __nullcRegisterFunction("float4__xz__void_ref_float2_", (void*)float4__xz__void_ref_float2_, 123u, 2);
	__nullcFR[177] = __nullcRegisterFunction("float4__xw__float2_ref__", (void*)float4__xw__float2_ref__, 123u, 2);
	__nullcFR[178] = __nullcRegisterFunction("float4__xw__void_ref_float2_", (void*)float4__xw__void_ref_float2_, 123u, 2);
	__nullcFR[179] = __nullcRegisterFunction("float4__yx__float2_ref__", (void*)float4__yx__float2_ref__, 123u, 2);
	__nullcFR[180] = __nullcRegisterFunction("float4__yx__void_ref_float2_", (void*)float4__yx__void_ref_float2_, 123u, 2);
	__nullcFR[181] = __nullcRegisterFunction("float4__yy__float2_ref__", (void*)float4__yy__float2_ref__, 123u, 2);
	__nullcFR[182] = __nullcRegisterFunction("float4__yz__float2_ref__", (void*)float4__yz__float2_ref__, 123u, 2);
	__nullcFR[183] = __nullcRegisterFunction("float4__yz__void_ref_float2_", (void*)float4__yz__void_ref_float2_, 123u, 2);
	__nullcFR[184] = __nullcRegisterFunction("float4__yw__float2_ref__", (void*)float4__yw__float2_ref__, 123u, 2);
	__nullcFR[185] = __nullcRegisterFunction("float4__yw__void_ref_float2_", (void*)float4__yw__void_ref_float2_, 123u, 2);
	__nullcFR[186] = __nullcRegisterFunction("float4__zx__float2_ref__", (void*)float4__zx__float2_ref__, 123u, 2);
	__nullcFR[187] = __nullcRegisterFunction("float4__zx__void_ref_float2_", (void*)float4__zx__void_ref_float2_, 123u, 2);
	__nullcFR[188] = __nullcRegisterFunction("float4__zy__float2_ref__", (void*)float4__zy__float2_ref__, 123u, 2);
	__nullcFR[189] = __nullcRegisterFunction("float4__zy__void_ref_float2_", (void*)float4__zy__void_ref_float2_, 123u, 2);
	__nullcFR[190] = __nullcRegisterFunction("float4__zz__float2_ref__", (void*)float4__zz__float2_ref__, 123u, 2);
	__nullcFR[191] = __nullcRegisterFunction("float4__zw__float2_ref__", (void*)float4__zw__float2_ref__, 123u, 2);
	__nullcFR[192] = __nullcRegisterFunction("float4__zw__void_ref_float2_", (void*)float4__zw__void_ref_float2_, 123u, 2);
	__nullcFR[193] = __nullcRegisterFunction("float4__wx__float2_ref__", (void*)float4__wx__float2_ref__, 123u, 2);
	__nullcFR[194] = __nullcRegisterFunction("float4__wx__void_ref_float2_", (void*)float4__wx__void_ref_float2_, 123u, 2);
	__nullcFR[195] = __nullcRegisterFunction("float4__wy__float2_ref__", (void*)float4__wy__float2_ref__, 123u, 2);
	__nullcFR[196] = __nullcRegisterFunction("float4__wy__void_ref_float2_", (void*)float4__wy__void_ref_float2_, 123u, 2);
	__nullcFR[197] = __nullcRegisterFunction("float4__wz__float2_ref__", (void*)float4__wz__float2_ref__, 123u, 2);
	__nullcFR[198] = __nullcRegisterFunction("float4__wz__void_ref_float2_", (void*)float4__wz__void_ref_float2_, 123u, 2);
	__nullcFR[199] = __nullcRegisterFunction("float4__ww__float2_ref__", (void*)float4__ww__float2_ref__, 123u, 2);
	__nullcFR[200] = __nullcRegisterFunction("float4__xxx__float3_ref__", (void*)float4__xxx__float3_ref__, 123u, 2);
	__nullcFR[201] = __nullcRegisterFunction("float4__xxy__float3_ref__", (void*)float4__xxy__float3_ref__, 123u, 2);
	__nullcFR[202] = __nullcRegisterFunction("float4__xxz__float3_ref__", (void*)float4__xxz__float3_ref__, 123u, 2);
	__nullcFR[203] = __nullcRegisterFunction("float4__xxw__float3_ref__", (void*)float4__xxw__float3_ref__, 123u, 2);
	__nullcFR[204] = __nullcRegisterFunction("float4__xyx__float3_ref__", (void*)float4__xyx__float3_ref__, 123u, 2);
	__nullcFR[205] = __nullcRegisterFunction("float4__xyy__float3_ref__", (void*)float4__xyy__float3_ref__, 123u, 2);
	__nullcFR[206] = __nullcRegisterFunction("float4__xyz__float3_ref__", (void*)float4__xyz__float3_ref__, 123u, 2);
	__nullcFR[207] = __nullcRegisterFunction("float4__xyz__void_ref_float3_", (void*)float4__xyz__void_ref_float3_, 123u, 2);
	__nullcFR[208] = __nullcRegisterFunction("float4__xyw__float3_ref__", (void*)float4__xyw__float3_ref__, 123u, 2);
	__nullcFR[209] = __nullcRegisterFunction("float4__xyw__void_ref_float3_", (void*)float4__xyw__void_ref_float3_, 123u, 2);
	__nullcFR[210] = __nullcRegisterFunction("float4__xzx__float3_ref__", (void*)float4__xzx__float3_ref__, 123u, 2);
	__nullcFR[211] = __nullcRegisterFunction("float4__xzy__float3_ref__", (void*)float4__xzy__float3_ref__, 123u, 2);
	__nullcFR[212] = __nullcRegisterFunction("float4__xzy__void_ref_float3_", (void*)float4__xzy__void_ref_float3_, 123u, 2);
	__nullcFR[213] = __nullcRegisterFunction("float4__xzz__float3_ref__", (void*)float4__xzz__float3_ref__, 123u, 2);
	__nullcFR[214] = __nullcRegisterFunction("float4__xzw__float3_ref__", (void*)float4__xzw__float3_ref__, 123u, 2);
	__nullcFR[215] = __nullcRegisterFunction("float4__xzw__void_ref_float3_", (void*)float4__xzw__void_ref_float3_, 123u, 2);
	__nullcFR[216] = __nullcRegisterFunction("float4__xwx__float3_ref__", (void*)float4__xwx__float3_ref__, 123u, 2);
	__nullcFR[217] = __nullcRegisterFunction("float4__xwy__float3_ref__", (void*)float4__xwy__float3_ref__, 123u, 2);
	__nullcFR[218] = __nullcRegisterFunction("float4__xwy__void_ref_float3_", (void*)float4__xwy__void_ref_float3_, 123u, 2);
	__nullcFR[219] = __nullcRegisterFunction("float4__xwz__float3_ref__", (void*)float4__xwz__float3_ref__, 123u, 2);
	__nullcFR[220] = __nullcRegisterFunction("float4__xwz__void_ref_float3_", (void*)float4__xwz__void_ref_float3_, 123u, 2);
	__nullcFR[221] = __nullcRegisterFunction("float4__xww__float3_ref__", (void*)float4__xww__float3_ref__, 123u, 2);
	__nullcFR[222] = __nullcRegisterFunction("float4__yxx__float3_ref__", (void*)float4__yxx__float3_ref__, 123u, 2);
	__nullcFR[223] = __nullcRegisterFunction("float4__yxy__float3_ref__", (void*)float4__yxy__float3_ref__, 123u, 2);
	__nullcFR[224] = __nullcRegisterFunction("float4__yxz__float3_ref__", (void*)float4__yxz__float3_ref__, 123u, 2);
	__nullcFR[225] = __nullcRegisterFunction("float4__yxz__void_ref_float3_", (void*)float4__yxz__void_ref_float3_, 123u, 2);
	__nullcFR[226] = __nullcRegisterFunction("float4__yxw__float3_ref__", (void*)float4__yxw__float3_ref__, 123u, 2);
	__nullcFR[227] = __nullcRegisterFunction("float4__yxw__void_ref_float3_", (void*)float4__yxw__void_ref_float3_, 123u, 2);
	__nullcFR[228] = __nullcRegisterFunction("float4__yyx__float3_ref__", (void*)float4__yyx__float3_ref__, 123u, 2);
	__nullcFR[229] = __nullcRegisterFunction("float4__yyy__float3_ref__", (void*)float4__yyy__float3_ref__, 123u, 2);
	__nullcFR[230] = __nullcRegisterFunction("float4__yyz__float3_ref__", (void*)float4__yyz__float3_ref__, 123u, 2);
	__nullcFR[231] = __nullcRegisterFunction("float4__yyw__float3_ref__", (void*)float4__yyw__float3_ref__, 123u, 2);
	__nullcFR[232] = __nullcRegisterFunction("float4__yzx__float3_ref__", (void*)float4__yzx__float3_ref__, 123u, 2);
	__nullcFR[233] = __nullcRegisterFunction("float4__yzx__void_ref_float3_", (void*)float4__yzx__void_ref_float3_, 123u, 2);
	__nullcFR[234] = __nullcRegisterFunction("float4__yzy__float3_ref__", (void*)float4__yzy__float3_ref__, 123u, 2);
	__nullcFR[235] = __nullcRegisterFunction("float4__yzz__float3_ref__", (void*)float4__yzz__float3_ref__, 123u, 2);
	__nullcFR[236] = __nullcRegisterFunction("float4__yzw__float3_ref__", (void*)float4__yzw__float3_ref__, 123u, 2);
	__nullcFR[237] = __nullcRegisterFunction("float4__yzw__void_ref_float3_", (void*)float4__yzw__void_ref_float3_, 123u, 2);
	__nullcFR[238] = __nullcRegisterFunction("float4__ywx__float3_ref__", (void*)float4__ywx__float3_ref__, 123u, 2);
	__nullcFR[239] = __nullcRegisterFunction("float4__ywx__void_ref_float3_", (void*)float4__ywx__void_ref_float3_, 123u, 2);
	__nullcFR[240] = __nullcRegisterFunction("float4__ywy__float3_ref__", (void*)float4__ywy__float3_ref__, 123u, 2);
	__nullcFR[241] = __nullcRegisterFunction("float4__ywz__float3_ref__", (void*)float4__ywz__float3_ref__, 123u, 2);
	__nullcFR[242] = __nullcRegisterFunction("float4__ywz__void_ref_float3_", (void*)float4__ywz__void_ref_float3_, 123u, 2);
	__nullcFR[243] = __nullcRegisterFunction("float4__yww__float3_ref__", (void*)float4__yww__float3_ref__, 123u, 2);
	__nullcFR[244] = __nullcRegisterFunction("float4__zxx__float3_ref__", (void*)float4__zxx__float3_ref__, 123u, 2);
	__nullcFR[245] = __nullcRegisterFunction("float4__zxy__float3_ref__", (void*)float4__zxy__float3_ref__, 123u, 2);
	__nullcFR[246] = __nullcRegisterFunction("float4__zxy__void_ref_float3_", (void*)float4__zxy__void_ref_float3_, 123u, 2);
	__nullcFR[247] = __nullcRegisterFunction("float4__zxz__float3_ref__", (void*)float4__zxz__float3_ref__, 123u, 2);
	__nullcFR[248] = __nullcRegisterFunction("float4__zxw__float3_ref__", (void*)float4__zxw__float3_ref__, 123u, 2);
	__nullcFR[249] = __nullcRegisterFunction("float4__zxw__void_ref_float3_", (void*)float4__zxw__void_ref_float3_, 123u, 2);
	__nullcFR[250] = __nullcRegisterFunction("float4__zyx__float3_ref__", (void*)float4__zyx__float3_ref__, 123u, 2);
	__nullcFR[251] = __nullcRegisterFunction("float4__zyx__void_ref_float3_", (void*)float4__zyx__void_ref_float3_, 123u, 2);
	__nullcFR[252] = __nullcRegisterFunction("float4__zyy__float3_ref__", (void*)float4__zyy__float3_ref__, 123u, 2);
	__nullcFR[253] = __nullcRegisterFunction("float4__zyz__float3_ref__", (void*)float4__zyz__float3_ref__, 123u, 2);
	__nullcFR[254] = __nullcRegisterFunction("float4__zyw__float3_ref__", (void*)float4__zyw__float3_ref__, 123u, 2);
	__nullcFR[255] = __nullcRegisterFunction("float4__zyw__void_ref_float3_", (void*)float4__zyw__void_ref_float3_, 123u, 2);
	__nullcFR[256] = __nullcRegisterFunction("float4__zzx__float3_ref__", (void*)float4__zzx__float3_ref__, 123u, 2);
	__nullcFR[257] = __nullcRegisterFunction("float4__zzy__float3_ref__", (void*)float4__zzy__float3_ref__, 123u, 2);
	__nullcFR[258] = __nullcRegisterFunction("float4__zzz__float3_ref__", (void*)float4__zzz__float3_ref__, 123u, 2);
	__nullcFR[259] = __nullcRegisterFunction("float4__zzw__float3_ref__", (void*)float4__zzw__float3_ref__, 123u, 2);
	__nullcFR[260] = __nullcRegisterFunction("float4__zwx__float3_ref__", (void*)float4__zwx__float3_ref__, 123u, 2);
	__nullcFR[261] = __nullcRegisterFunction("float4__zwx__void_ref_float3_", (void*)float4__zwx__void_ref_float3_, 123u, 2);
	__nullcFR[262] = __nullcRegisterFunction("float4__zwy__float3_ref__", (void*)float4__zwy__float3_ref__, 123u, 2);
	__nullcFR[263] = __nullcRegisterFunction("float4__zwy__void_ref_float3_", (void*)float4__zwy__void_ref_float3_, 123u, 2);
	__nullcFR[264] = __nullcRegisterFunction("float4__zwz__float3_ref__", (void*)float4__zwz__float3_ref__, 123u, 2);
	__nullcFR[265] = __nullcRegisterFunction("float4__zww__float3_ref__", (void*)float4__zww__float3_ref__, 123u, 2);
	__nullcFR[266] = __nullcRegisterFunction("float4__wxx__float3_ref__", (void*)float4__wxx__float3_ref__, 123u, 2);
	__nullcFR[267] = __nullcRegisterFunction("float4__wxy__float3_ref__", (void*)float4__wxy__float3_ref__, 123u, 2);
	__nullcFR[268] = __nullcRegisterFunction("float4__wxy__void_ref_float3_", (void*)float4__wxy__void_ref_float3_, 123u, 2);
	__nullcFR[269] = __nullcRegisterFunction("float4__wxz__float3_ref__", (void*)float4__wxz__float3_ref__, 123u, 2);
	__nullcFR[270] = __nullcRegisterFunction("float4__wxz__void_ref_float3_", (void*)float4__wxz__void_ref_float3_, 123u, 2);
	__nullcFR[271] = __nullcRegisterFunction("float4__wxw__float3_ref__", (void*)float4__wxw__float3_ref__, 123u, 2);
	__nullcFR[272] = __nullcRegisterFunction("float4__wyx__float3_ref__", (void*)float4__wyx__float3_ref__, 123u, 2);
	__nullcFR[273] = __nullcRegisterFunction("float4__wyx__void_ref_float3_", (void*)float4__wyx__void_ref_float3_, 123u, 2);
	__nullcFR[274] = __nullcRegisterFunction("float4__wyy__float3_ref__", (void*)float4__wyy__float3_ref__, 123u, 2);
	__nullcFR[275] = __nullcRegisterFunction("float4__wyz__float3_ref__", (void*)float4__wyz__float3_ref__, 123u, 2);
	__nullcFR[276] = __nullcRegisterFunction("float4__wyz__void_ref_float3_", (void*)float4__wyz__void_ref_float3_, 123u, 2);
	__nullcFR[277] = __nullcRegisterFunction("float4__wyw__float3_ref__", (void*)float4__wyw__float3_ref__, 123u, 2);
	__nullcFR[278] = __nullcRegisterFunction("float4__wzx__float3_ref__", (void*)float4__wzx__float3_ref__, 123u, 2);
	__nullcFR[279] = __nullcRegisterFunction("float4__wzx__void_ref_float3_", (void*)float4__wzx__void_ref_float3_, 123u, 2);
	__nullcFR[280] = __nullcRegisterFunction("float4__wzy__float3_ref__", (void*)float4__wzy__float3_ref__, 123u, 2);
	__nullcFR[281] = __nullcRegisterFunction("float4__wzy__void_ref_float3_", (void*)float4__wzy__void_ref_float3_, 123u, 2);
	__nullcFR[282] = __nullcRegisterFunction("float4__wzz__float3_ref__", (void*)float4__wzz__float3_ref__, 123u, 2);
	__nullcFR[283] = __nullcRegisterFunction("float4__wzw__float3_ref__", (void*)float4__wzw__float3_ref__, 123u, 2);
	__nullcFR[284] = __nullcRegisterFunction("float4__wwx__float3_ref__", (void*)float4__wwx__float3_ref__, 123u, 2);
	__nullcFR[285] = __nullcRegisterFunction("float4__wwy__float3_ref__", (void*)float4__wwy__float3_ref__, 123u, 2);
	__nullcFR[286] = __nullcRegisterFunction("float4__wwz__float3_ref__", (void*)float4__wwz__float3_ref__, 123u, 2);
	__nullcFR[287] = __nullcRegisterFunction("float4__www__float3_ref__", (void*)float4__www__float3_ref__, 123u, 2);
	__nullcFR[288] = __nullcRegisterFunction("float4__xxxx__float4_ref__", (void*)float4__xxxx__float4_ref__, 123u, 2);
	__nullcFR[289] = __nullcRegisterFunction("float4__xxxy__float4_ref__", (void*)float4__xxxy__float4_ref__, 123u, 2);
	__nullcFR[290] = __nullcRegisterFunction("float4__xxxz__float4_ref__", (void*)float4__xxxz__float4_ref__, 123u, 2);
	__nullcFR[291] = __nullcRegisterFunction("float4__xxxw__float4_ref__", (void*)float4__xxxw__float4_ref__, 123u, 2);
	__nullcFR[292] = __nullcRegisterFunction("float4__xxyx__float4_ref__", (void*)float4__xxyx__float4_ref__, 123u, 2);
	__nullcFR[293] = __nullcRegisterFunction("float4__xxyy__float4_ref__", (void*)float4__xxyy__float4_ref__, 123u, 2);
	__nullcFR[294] = __nullcRegisterFunction("float4__xxyz__float4_ref__", (void*)float4__xxyz__float4_ref__, 123u, 2);
	__nullcFR[295] = __nullcRegisterFunction("float4__xxyw__float4_ref__", (void*)float4__xxyw__float4_ref__, 123u, 2);
	__nullcFR[296] = __nullcRegisterFunction("float4__xxzx__float4_ref__", (void*)float4__xxzx__float4_ref__, 123u, 2);
	__nullcFR[297] = __nullcRegisterFunction("float4__xxzy__float4_ref__", (void*)float4__xxzy__float4_ref__, 123u, 2);
	__nullcFR[298] = __nullcRegisterFunction("float4__xxzz__float4_ref__", (void*)float4__xxzz__float4_ref__, 123u, 2);
	__nullcFR[299] = __nullcRegisterFunction("float4__xxzw__float4_ref__", (void*)float4__xxzw__float4_ref__, 123u, 2);
	__nullcFR[300] = __nullcRegisterFunction("float4__xxwx__float4_ref__", (void*)float4__xxwx__float4_ref__, 123u, 2);
	__nullcFR[301] = __nullcRegisterFunction("float4__xxwy__float4_ref__", (void*)float4__xxwy__float4_ref__, 123u, 2);
	__nullcFR[302] = __nullcRegisterFunction("float4__xxwz__float4_ref__", (void*)float4__xxwz__float4_ref__, 123u, 2);
	__nullcFR[303] = __nullcRegisterFunction("float4__xxww__float4_ref__", (void*)float4__xxww__float4_ref__, 123u, 2);
	__nullcFR[304] = __nullcRegisterFunction("float4__xyxx__float4_ref__", (void*)float4__xyxx__float4_ref__, 123u, 2);
	__nullcFR[305] = __nullcRegisterFunction("float4__xyxy__float4_ref__", (void*)float4__xyxy__float4_ref__, 123u, 2);
	__nullcFR[306] = __nullcRegisterFunction("float4__xyxz__float4_ref__", (void*)float4__xyxz__float4_ref__, 123u, 2);
	__nullcFR[307] = __nullcRegisterFunction("float4__xyxw__float4_ref__", (void*)float4__xyxw__float4_ref__, 123u, 2);
	__nullcFR[308] = __nullcRegisterFunction("float4__xyyx__float4_ref__", (void*)float4__xyyx__float4_ref__, 123u, 2);
	__nullcFR[309] = __nullcRegisterFunction("float4__xyyy__float4_ref__", (void*)float4__xyyy__float4_ref__, 123u, 2);
	__nullcFR[310] = __nullcRegisterFunction("float4__xyyz__float4_ref__", (void*)float4__xyyz__float4_ref__, 123u, 2);
	__nullcFR[311] = __nullcRegisterFunction("float4__xyyw__float4_ref__", (void*)float4__xyyw__float4_ref__, 123u, 2);
	__nullcFR[312] = __nullcRegisterFunction("float4__xyzx__float4_ref__", (void*)float4__xyzx__float4_ref__, 123u, 2);
	__nullcFR[313] = __nullcRegisterFunction("float4__xyzy__float4_ref__", (void*)float4__xyzy__float4_ref__, 123u, 2);
	__nullcFR[314] = __nullcRegisterFunction("float4__xyzz__float4_ref__", (void*)float4__xyzz__float4_ref__, 123u, 2);
	__nullcFR[315] = __nullcRegisterFunction("float4__xyzw__float4_ref__", (void*)float4__xyzw__float4_ref__, 123u, 2);
	__nullcFR[316] = __nullcRegisterFunction("float4__xyzw__void_ref_float4_", (void*)float4__xyzw__void_ref_float4_, 123u, 2);
	__nullcFR[317] = __nullcRegisterFunction("float4__xywx__float4_ref__", (void*)float4__xywx__float4_ref__, 123u, 2);
	__nullcFR[318] = __nullcRegisterFunction("float4__xywy__float4_ref__", (void*)float4__xywy__float4_ref__, 123u, 2);
	__nullcFR[319] = __nullcRegisterFunction("float4__xywz__float4_ref__", (void*)float4__xywz__float4_ref__, 123u, 2);
	__nullcFR[320] = __nullcRegisterFunction("float4__xywz__void_ref_float4_", (void*)float4__xywz__void_ref_float4_, 123u, 2);
	__nullcFR[321] = __nullcRegisterFunction("float4__xyww__float4_ref__", (void*)float4__xyww__float4_ref__, 123u, 2);
	__nullcFR[322] = __nullcRegisterFunction("float4__xzxx__float4_ref__", (void*)float4__xzxx__float4_ref__, 123u, 2);
	__nullcFR[323] = __nullcRegisterFunction("float4__xzxy__float4_ref__", (void*)float4__xzxy__float4_ref__, 123u, 2);
	__nullcFR[324] = __nullcRegisterFunction("float4__xzxz__float4_ref__", (void*)float4__xzxz__float4_ref__, 123u, 2);
	__nullcFR[325] = __nullcRegisterFunction("float4__xzxw__float4_ref__", (void*)float4__xzxw__float4_ref__, 123u, 2);
	__nullcFR[326] = __nullcRegisterFunction("float4__xzyx__float4_ref__", (void*)float4__xzyx__float4_ref__, 123u, 2);
	__nullcFR[327] = __nullcRegisterFunction("float4__xzyy__float4_ref__", (void*)float4__xzyy__float4_ref__, 123u, 2);
	__nullcFR[328] = __nullcRegisterFunction("float4__xzyz__float4_ref__", (void*)float4__xzyz__float4_ref__, 123u, 2);
	__nullcFR[329] = __nullcRegisterFunction("float4__xzyw__float4_ref__", (void*)float4__xzyw__float4_ref__, 123u, 2);
	__nullcFR[330] = __nullcRegisterFunction("float4__xzyw__void_ref_float4_", (void*)float4__xzyw__void_ref_float4_, 123u, 2);
	__nullcFR[331] = __nullcRegisterFunction("float4__xzzx__float4_ref__", (void*)float4__xzzx__float4_ref__, 123u, 2);
	__nullcFR[332] = __nullcRegisterFunction("float4__xzzy__float4_ref__", (void*)float4__xzzy__float4_ref__, 123u, 2);
	__nullcFR[333] = __nullcRegisterFunction("float4__xzzz__float4_ref__", (void*)float4__xzzz__float4_ref__, 123u, 2);
	__nullcFR[334] = __nullcRegisterFunction("float4__xzzw__float4_ref__", (void*)float4__xzzw__float4_ref__, 123u, 2);
	__nullcFR[335] = __nullcRegisterFunction("float4__xzwx__float4_ref__", (void*)float4__xzwx__float4_ref__, 123u, 2);
	__nullcFR[336] = __nullcRegisterFunction("float4__xzwy__float4_ref__", (void*)float4__xzwy__float4_ref__, 123u, 2);
	__nullcFR[337] = __nullcRegisterFunction("float4__xzwy__void_ref_float4_", (void*)float4__xzwy__void_ref_float4_, 123u, 2);
	__nullcFR[338] = __nullcRegisterFunction("float4__xzwz__float4_ref__", (void*)float4__xzwz__float4_ref__, 123u, 2);
	__nullcFR[339] = __nullcRegisterFunction("float4__xzww__float4_ref__", (void*)float4__xzww__float4_ref__, 123u, 2);
	__nullcFR[340] = __nullcRegisterFunction("float4__xwxx__float4_ref__", (void*)float4__xwxx__float4_ref__, 123u, 2);
	__nullcFR[341] = __nullcRegisterFunction("float4__xwxy__float4_ref__", (void*)float4__xwxy__float4_ref__, 123u, 2);
	__nullcFR[342] = __nullcRegisterFunction("float4__xwxz__float4_ref__", (void*)float4__xwxz__float4_ref__, 123u, 2);
	__nullcFR[343] = __nullcRegisterFunction("float4__xwxw__float4_ref__", (void*)float4__xwxw__float4_ref__, 123u, 2);
	__nullcFR[344] = __nullcRegisterFunction("float4__xwyx__float4_ref__", (void*)float4__xwyx__float4_ref__, 123u, 2);
	__nullcFR[345] = __nullcRegisterFunction("float4__xwyy__float4_ref__", (void*)float4__xwyy__float4_ref__, 123u, 2);
	__nullcFR[346] = __nullcRegisterFunction("float4__xwyz__float4_ref__", (void*)float4__xwyz__float4_ref__, 123u, 2);
	__nullcFR[347] = __nullcRegisterFunction("float4__xwyz__void_ref_float4_", (void*)float4__xwyz__void_ref_float4_, 123u, 2);
	__nullcFR[348] = __nullcRegisterFunction("float4__xwyw__float4_ref__", (void*)float4__xwyw__float4_ref__, 123u, 2);
	__nullcFR[349] = __nullcRegisterFunction("float4__xwzx__float4_ref__", (void*)float4__xwzx__float4_ref__, 123u, 2);
	__nullcFR[350] = __nullcRegisterFunction("float4__xwzy__float4_ref__", (void*)float4__xwzy__float4_ref__, 123u, 2);
	__nullcFR[351] = __nullcRegisterFunction("float4__xwzy__void_ref_float4_", (void*)float4__xwzy__void_ref_float4_, 123u, 2);
	__nullcFR[352] = __nullcRegisterFunction("float4__xwzz__float4_ref__", (void*)float4__xwzz__float4_ref__, 123u, 2);
	__nullcFR[353] = __nullcRegisterFunction("float4__xwzw__float4_ref__", (void*)float4__xwzw__float4_ref__, 123u, 2);
	__nullcFR[354] = __nullcRegisterFunction("float4__xwwx__float4_ref__", (void*)float4__xwwx__float4_ref__, 123u, 2);
	__nullcFR[355] = __nullcRegisterFunction("float4__xwwy__float4_ref__", (void*)float4__xwwy__float4_ref__, 123u, 2);
	__nullcFR[356] = __nullcRegisterFunction("float4__xwwz__float4_ref__", (void*)float4__xwwz__float4_ref__, 123u, 2);
	__nullcFR[357] = __nullcRegisterFunction("float4__xwww__float4_ref__", (void*)float4__xwww__float4_ref__, 123u, 2);
	__nullcFR[358] = __nullcRegisterFunction("float4__yxxx__float4_ref__", (void*)float4__yxxx__float4_ref__, 123u, 2);
	__nullcFR[359] = __nullcRegisterFunction("float4__yxxy__float4_ref__", (void*)float4__yxxy__float4_ref__, 123u, 2);
	__nullcFR[360] = __nullcRegisterFunction("float4__yxxz__float4_ref__", (void*)float4__yxxz__float4_ref__, 123u, 2);
	__nullcFR[361] = __nullcRegisterFunction("float4__yxxw__float4_ref__", (void*)float4__yxxw__float4_ref__, 123u, 2);
	__nullcFR[362] = __nullcRegisterFunction("float4__yxyx__float4_ref__", (void*)float4__yxyx__float4_ref__, 123u, 2);
	__nullcFR[363] = __nullcRegisterFunction("float4__yxyy__float4_ref__", (void*)float4__yxyy__float4_ref__, 123u, 2);
	__nullcFR[364] = __nullcRegisterFunction("float4__yxyz__float4_ref__", (void*)float4__yxyz__float4_ref__, 123u, 2);
	__nullcFR[365] = __nullcRegisterFunction("float4__yxyw__float4_ref__", (void*)float4__yxyw__float4_ref__, 123u, 2);
	__nullcFR[366] = __nullcRegisterFunction("float4__yxzx__float4_ref__", (void*)float4__yxzx__float4_ref__, 123u, 2);
	__nullcFR[367] = __nullcRegisterFunction("float4__yxzy__float4_ref__", (void*)float4__yxzy__float4_ref__, 123u, 2);
	__nullcFR[368] = __nullcRegisterFunction("float4__yxzz__float4_ref__", (void*)float4__yxzz__float4_ref__, 123u, 2);
	__nullcFR[369] = __nullcRegisterFunction("float4__yxzw__float4_ref__", (void*)float4__yxzw__float4_ref__, 123u, 2);
	__nullcFR[370] = __nullcRegisterFunction("float4__yxzw__void_ref_float4_", (void*)float4__yxzw__void_ref_float4_, 123u, 2);
	__nullcFR[371] = __nullcRegisterFunction("float4__yxwx__float4_ref__", (void*)float4__yxwx__float4_ref__, 123u, 2);
	__nullcFR[372] = __nullcRegisterFunction("float4__yxwy__float4_ref__", (void*)float4__yxwy__float4_ref__, 123u, 2);
	__nullcFR[373] = __nullcRegisterFunction("float4__yxwz__float4_ref__", (void*)float4__yxwz__float4_ref__, 123u, 2);
	__nullcFR[374] = __nullcRegisterFunction("float4__yxwz__void_ref_float4_", (void*)float4__yxwz__void_ref_float4_, 123u, 2);
	__nullcFR[375] = __nullcRegisterFunction("float4__yxww__float4_ref__", (void*)float4__yxww__float4_ref__, 123u, 2);
	__nullcFR[376] = __nullcRegisterFunction("float4__yyxx__float4_ref__", (void*)float4__yyxx__float4_ref__, 123u, 2);
	__nullcFR[377] = __nullcRegisterFunction("float4__yyxy__float4_ref__", (void*)float4__yyxy__float4_ref__, 123u, 2);
	__nullcFR[378] = __nullcRegisterFunction("float4__yyxz__float4_ref__", (void*)float4__yyxz__float4_ref__, 123u, 2);
	__nullcFR[379] = __nullcRegisterFunction("float4__yyxw__float4_ref__", (void*)float4__yyxw__float4_ref__, 123u, 2);
	__nullcFR[380] = __nullcRegisterFunction("float4__yyyx__float4_ref__", (void*)float4__yyyx__float4_ref__, 123u, 2);
	__nullcFR[381] = __nullcRegisterFunction("float4__yyyy__float4_ref__", (void*)float4__yyyy__float4_ref__, 123u, 2);
	__nullcFR[382] = __nullcRegisterFunction("float4__yyyz__float4_ref__", (void*)float4__yyyz__float4_ref__, 123u, 2);
	__nullcFR[383] = __nullcRegisterFunction("float4__yyyw__float4_ref__", (void*)float4__yyyw__float4_ref__, 123u, 2);
	__nullcFR[384] = __nullcRegisterFunction("float4__yyzx__float4_ref__", (void*)float4__yyzx__float4_ref__, 123u, 2);
	__nullcFR[385] = __nullcRegisterFunction("float4__yyzy__float4_ref__", (void*)float4__yyzy__float4_ref__, 123u, 2);
	__nullcFR[386] = __nullcRegisterFunction("float4__yyzz__float4_ref__", (void*)float4__yyzz__float4_ref__, 123u, 2);
	__nullcFR[387] = __nullcRegisterFunction("float4__yyzw__float4_ref__", (void*)float4__yyzw__float4_ref__, 123u, 2);
	__nullcFR[388] = __nullcRegisterFunction("float4__yywx__float4_ref__", (void*)float4__yywx__float4_ref__, 123u, 2);
	__nullcFR[389] = __nullcRegisterFunction("float4__yywy__float4_ref__", (void*)float4__yywy__float4_ref__, 123u, 2);
	__nullcFR[390] = __nullcRegisterFunction("float4__yywz__float4_ref__", (void*)float4__yywz__float4_ref__, 123u, 2);
	__nullcFR[391] = __nullcRegisterFunction("float4__yyww__float4_ref__", (void*)float4__yyww__float4_ref__, 123u, 2);
	__nullcFR[392] = __nullcRegisterFunction("float4__yzxx__float4_ref__", (void*)float4__yzxx__float4_ref__, 123u, 2);
	__nullcFR[393] = __nullcRegisterFunction("float4__yzxy__float4_ref__", (void*)float4__yzxy__float4_ref__, 123u, 2);
	__nullcFR[394] = __nullcRegisterFunction("float4__yzxz__float4_ref__", (void*)float4__yzxz__float4_ref__, 123u, 2);
	__nullcFR[395] = __nullcRegisterFunction("float4__yzxw__float4_ref__", (void*)float4__yzxw__float4_ref__, 123u, 2);
	__nullcFR[396] = __nullcRegisterFunction("float4__yzxw__void_ref_float4_", (void*)float4__yzxw__void_ref_float4_, 123u, 2);
	__nullcFR[397] = __nullcRegisterFunction("float4__yzyx__float4_ref__", (void*)float4__yzyx__float4_ref__, 123u, 2);
	__nullcFR[398] = __nullcRegisterFunction("float4__yzyy__float4_ref__", (void*)float4__yzyy__float4_ref__, 123u, 2);
	__nullcFR[399] = __nullcRegisterFunction("float4__yzyz__float4_ref__", (void*)float4__yzyz__float4_ref__, 123u, 2);
	__nullcFR[400] = __nullcRegisterFunction("float4__yzyw__float4_ref__", (void*)float4__yzyw__float4_ref__, 123u, 2);
	__nullcFR[401] = __nullcRegisterFunction("float4__yzzx__float4_ref__", (void*)float4__yzzx__float4_ref__, 123u, 2);
	__nullcFR[402] = __nullcRegisterFunction("float4__yzzy__float4_ref__", (void*)float4__yzzy__float4_ref__, 123u, 2);
	__nullcFR[403] = __nullcRegisterFunction("float4__yzzz__float4_ref__", (void*)float4__yzzz__float4_ref__, 123u, 2);
	__nullcFR[404] = __nullcRegisterFunction("float4__yzzw__float4_ref__", (void*)float4__yzzw__float4_ref__, 123u, 2);
	__nullcFR[405] = __nullcRegisterFunction("float4__yzwx__float4_ref__", (void*)float4__yzwx__float4_ref__, 123u, 2);
	__nullcFR[406] = __nullcRegisterFunction("float4__yzwx__void_ref_float4_", (void*)float4__yzwx__void_ref_float4_, 123u, 2);
	__nullcFR[407] = __nullcRegisterFunction("float4__yzwy__float4_ref__", (void*)float4__yzwy__float4_ref__, 123u, 2);
	__nullcFR[408] = __nullcRegisterFunction("float4__yzwz__float4_ref__", (void*)float4__yzwz__float4_ref__, 123u, 2);
	__nullcFR[409] = __nullcRegisterFunction("float4__yzww__float4_ref__", (void*)float4__yzww__float4_ref__, 123u, 2);
	__nullcFR[410] = __nullcRegisterFunction("float4__ywxx__float4_ref__", (void*)float4__ywxx__float4_ref__, 123u, 2);
	__nullcFR[411] = __nullcRegisterFunction("float4__ywxy__float4_ref__", (void*)float4__ywxy__float4_ref__, 123u, 2);
	__nullcFR[412] = __nullcRegisterFunction("float4__ywxz__float4_ref__", (void*)float4__ywxz__float4_ref__, 123u, 2);
	__nullcFR[413] = __nullcRegisterFunction("float4__ywxz__void_ref_float4_", (void*)float4__ywxz__void_ref_float4_, 123u, 2);
	__nullcFR[414] = __nullcRegisterFunction("float4__ywxw__float4_ref__", (void*)float4__ywxw__float4_ref__, 123u, 2);
	__nullcFR[415] = __nullcRegisterFunction("float4__ywyx__float4_ref__", (void*)float4__ywyx__float4_ref__, 123u, 2);
	__nullcFR[416] = __nullcRegisterFunction("float4__ywyy__float4_ref__", (void*)float4__ywyy__float4_ref__, 123u, 2);
	__nullcFR[417] = __nullcRegisterFunction("float4__ywyz__float4_ref__", (void*)float4__ywyz__float4_ref__, 123u, 2);
	__nullcFR[418] = __nullcRegisterFunction("float4__ywyw__float4_ref__", (void*)float4__ywyw__float4_ref__, 123u, 2);
	__nullcFR[419] = __nullcRegisterFunction("float4__ywzx__float4_ref__", (void*)float4__ywzx__float4_ref__, 123u, 2);
	__nullcFR[420] = __nullcRegisterFunction("float4__ywzx__void_ref_float4_", (void*)float4__ywzx__void_ref_float4_, 123u, 2);
	__nullcFR[421] = __nullcRegisterFunction("float4__ywzy__float4_ref__", (void*)float4__ywzy__float4_ref__, 123u, 2);
	__nullcFR[422] = __nullcRegisterFunction("float4__ywzz__float4_ref__", (void*)float4__ywzz__float4_ref__, 123u, 2);
	__nullcFR[423] = __nullcRegisterFunction("float4__ywzw__float4_ref__", (void*)float4__ywzw__float4_ref__, 123u, 2);
	__nullcFR[424] = __nullcRegisterFunction("float4__ywwx__float4_ref__", (void*)float4__ywwx__float4_ref__, 123u, 2);
	__nullcFR[425] = __nullcRegisterFunction("float4__ywwy__float4_ref__", (void*)float4__ywwy__float4_ref__, 123u, 2);
	__nullcFR[426] = __nullcRegisterFunction("float4__ywwz__float4_ref__", (void*)float4__ywwz__float4_ref__, 123u, 2);
	__nullcFR[427] = __nullcRegisterFunction("float4__ywww__float4_ref__", (void*)float4__ywww__float4_ref__, 123u, 2);
	__nullcFR[428] = __nullcRegisterFunction("float4__zxxx__float4_ref__", (void*)float4__zxxx__float4_ref__, 123u, 2);
	__nullcFR[429] = __nullcRegisterFunction("float4__zxxy__float4_ref__", (void*)float4__zxxy__float4_ref__, 123u, 2);
	__nullcFR[430] = __nullcRegisterFunction("float4__zxxz__float4_ref__", (void*)float4__zxxz__float4_ref__, 123u, 2);
	__nullcFR[431] = __nullcRegisterFunction("float4__zxxw__float4_ref__", (void*)float4__zxxw__float4_ref__, 123u, 2);
	__nullcFR[432] = __nullcRegisterFunction("float4__zxyx__float4_ref__", (void*)float4__zxyx__float4_ref__, 123u, 2);
	__nullcFR[433] = __nullcRegisterFunction("float4__zxyy__float4_ref__", (void*)float4__zxyy__float4_ref__, 123u, 2);
	__nullcFR[434] = __nullcRegisterFunction("float4__zxyz__float4_ref__", (void*)float4__zxyz__float4_ref__, 123u, 2);
	__nullcFR[435] = __nullcRegisterFunction("float4__zxyw__float4_ref__", (void*)float4__zxyw__float4_ref__, 123u, 2);
	__nullcFR[436] = __nullcRegisterFunction("float4__zxyw__void_ref_float4_", (void*)float4__zxyw__void_ref_float4_, 123u, 2);
	__nullcFR[437] = __nullcRegisterFunction("float4__zxzx__float4_ref__", (void*)float4__zxzx__float4_ref__, 123u, 2);
	__nullcFR[438] = __nullcRegisterFunction("float4__zxzy__float4_ref__", (void*)float4__zxzy__float4_ref__, 123u, 2);
	__nullcFR[439] = __nullcRegisterFunction("float4__zxzz__float4_ref__", (void*)float4__zxzz__float4_ref__, 123u, 2);
	__nullcFR[440] = __nullcRegisterFunction("float4__zxzw__float4_ref__", (void*)float4__zxzw__float4_ref__, 123u, 2);
	__nullcFR[441] = __nullcRegisterFunction("float4__zxwx__float4_ref__", (void*)float4__zxwx__float4_ref__, 123u, 2);
	__nullcFR[442] = __nullcRegisterFunction("float4__zxwy__float4_ref__", (void*)float4__zxwy__float4_ref__, 123u, 2);
	__nullcFR[443] = __nullcRegisterFunction("float4__zxwy__void_ref_float4_", (void*)float4__zxwy__void_ref_float4_, 123u, 2);
	__nullcFR[444] = __nullcRegisterFunction("float4__zxwz__float4_ref__", (void*)float4__zxwz__float4_ref__, 123u, 2);
	__nullcFR[445] = __nullcRegisterFunction("float4__zxww__float4_ref__", (void*)float4__zxww__float4_ref__, 123u, 2);
	__nullcFR[446] = __nullcRegisterFunction("float4__zyxx__float4_ref__", (void*)float4__zyxx__float4_ref__, 123u, 2);
	__nullcFR[447] = __nullcRegisterFunction("float4__zyxy__float4_ref__", (void*)float4__zyxy__float4_ref__, 123u, 2);
	__nullcFR[448] = __nullcRegisterFunction("float4__zyxz__float4_ref__", (void*)float4__zyxz__float4_ref__, 123u, 2);
	__nullcFR[449] = __nullcRegisterFunction("float4__zyxw__float4_ref__", (void*)float4__zyxw__float4_ref__, 123u, 2);
	__nullcFR[450] = __nullcRegisterFunction("float4__zyxw__void_ref_float4_", (void*)float4__zyxw__void_ref_float4_, 123u, 2);
	__nullcFR[451] = __nullcRegisterFunction("float4__zyyx__float4_ref__", (void*)float4__zyyx__float4_ref__, 123u, 2);
	__nullcFR[452] = __nullcRegisterFunction("float4__zyyy__float4_ref__", (void*)float4__zyyy__float4_ref__, 123u, 2);
	__nullcFR[453] = __nullcRegisterFunction("float4__zyyz__float4_ref__", (void*)float4__zyyz__float4_ref__, 123u, 2);
	__nullcFR[454] = __nullcRegisterFunction("float4__zyyw__float4_ref__", (void*)float4__zyyw__float4_ref__, 123u, 2);
	__nullcFR[455] = __nullcRegisterFunction("float4__zyzx__float4_ref__", (void*)float4__zyzx__float4_ref__, 123u, 2);
	__nullcFR[456] = __nullcRegisterFunction("float4__zyzy__float4_ref__", (void*)float4__zyzy__float4_ref__, 123u, 2);
	__nullcFR[457] = __nullcRegisterFunction("float4__zyzz__float4_ref__", (void*)float4__zyzz__float4_ref__, 123u, 2);
	__nullcFR[458] = __nullcRegisterFunction("float4__zyzw__float4_ref__", (void*)float4__zyzw__float4_ref__, 123u, 2);
	__nullcFR[459] = __nullcRegisterFunction("float4__zywx__float4_ref__", (void*)float4__zywx__float4_ref__, 123u, 2);
	__nullcFR[460] = __nullcRegisterFunction("float4__zywx__void_ref_float4_", (void*)float4__zywx__void_ref_float4_, 123u, 2);
	__nullcFR[461] = __nullcRegisterFunction("float4__zywy__float4_ref__", (void*)float4__zywy__float4_ref__, 123u, 2);
	__nullcFR[462] = __nullcRegisterFunction("float4__zywz__float4_ref__", (void*)float4__zywz__float4_ref__, 123u, 2);
	__nullcFR[463] = __nullcRegisterFunction("float4__zyww__float4_ref__", (void*)float4__zyww__float4_ref__, 123u, 2);
	__nullcFR[464] = __nullcRegisterFunction("float4__zzxx__float4_ref__", (void*)float4__zzxx__float4_ref__, 123u, 2);
	__nullcFR[465] = __nullcRegisterFunction("float4__zzxy__float4_ref__", (void*)float4__zzxy__float4_ref__, 123u, 2);
	__nullcFR[466] = __nullcRegisterFunction("float4__zzxz__float4_ref__", (void*)float4__zzxz__float4_ref__, 123u, 2);
	__nullcFR[467] = __nullcRegisterFunction("float4__zzxw__float4_ref__", (void*)float4__zzxw__float4_ref__, 123u, 2);
	__nullcFR[468] = __nullcRegisterFunction("float4__zzyx__float4_ref__", (void*)float4__zzyx__float4_ref__, 123u, 2);
	__nullcFR[469] = __nullcRegisterFunction("float4__zzyy__float4_ref__", (void*)float4__zzyy__float4_ref__, 123u, 2);
	__nullcFR[470] = __nullcRegisterFunction("float4__zzyz__float4_ref__", (void*)float4__zzyz__float4_ref__, 123u, 2);
	__nullcFR[471] = __nullcRegisterFunction("float4__zzyw__float4_ref__", (void*)float4__zzyw__float4_ref__, 123u, 2);
	__nullcFR[472] = __nullcRegisterFunction("float4__zzzx__float4_ref__", (void*)float4__zzzx__float4_ref__, 123u, 2);
	__nullcFR[473] = __nullcRegisterFunction("float4__zzzy__float4_ref__", (void*)float4__zzzy__float4_ref__, 123u, 2);
	__nullcFR[474] = __nullcRegisterFunction("float4__zzzz__float4_ref__", (void*)float4__zzzz__float4_ref__, 123u, 2);
	__nullcFR[475] = __nullcRegisterFunction("float4__zzzw__float4_ref__", (void*)float4__zzzw__float4_ref__, 123u, 2);
	__nullcFR[476] = __nullcRegisterFunction("float4__zzwx__float4_ref__", (void*)float4__zzwx__float4_ref__, 123u, 2);
	__nullcFR[477] = __nullcRegisterFunction("float4__zzwy__float4_ref__", (void*)float4__zzwy__float4_ref__, 123u, 2);
	__nullcFR[478] = __nullcRegisterFunction("float4__zzwz__float4_ref__", (void*)float4__zzwz__float4_ref__, 123u, 2);
	__nullcFR[479] = __nullcRegisterFunction("float4__zzww__float4_ref__", (void*)float4__zzww__float4_ref__, 123u, 2);
	__nullcFR[480] = __nullcRegisterFunction("float4__zwxx__float4_ref__", (void*)float4__zwxx__float4_ref__, 123u, 2);
	__nullcFR[481] = __nullcRegisterFunction("float4__zwxy__float4_ref__", (void*)float4__zwxy__float4_ref__, 123u, 2);
	__nullcFR[482] = __nullcRegisterFunction("float4__zwxy__void_ref_float4_", (void*)float4__zwxy__void_ref_float4_, 123u, 2);
	__nullcFR[483] = __nullcRegisterFunction("float4__zwxz__float4_ref__", (void*)float4__zwxz__float4_ref__, 123u, 2);
	__nullcFR[484] = __nullcRegisterFunction("float4__zwxw__float4_ref__", (void*)float4__zwxw__float4_ref__, 123u, 2);
	__nullcFR[485] = __nullcRegisterFunction("float4__zwyx__float4_ref__", (void*)float4__zwyx__float4_ref__, 123u, 2);
	__nullcFR[486] = __nullcRegisterFunction("float4__zwyx__void_ref_float4_", (void*)float4__zwyx__void_ref_float4_, 123u, 2);
	__nullcFR[487] = __nullcRegisterFunction("float4__zwyy__float4_ref__", (void*)float4__zwyy__float4_ref__, 123u, 2);
	__nullcFR[488] = __nullcRegisterFunction("float4__zwyz__float4_ref__", (void*)float4__zwyz__float4_ref__, 123u, 2);
	__nullcFR[489] = __nullcRegisterFunction("float4__zwyw__float4_ref__", (void*)float4__zwyw__float4_ref__, 123u, 2);
	__nullcFR[490] = __nullcRegisterFunction("float4__zwzx__float4_ref__", (void*)float4__zwzx__float4_ref__, 123u, 2);
	__nullcFR[491] = __nullcRegisterFunction("float4__zwzy__float4_ref__", (void*)float4__zwzy__float4_ref__, 123u, 2);
	__nullcFR[492] = __nullcRegisterFunction("float4__zwzz__float4_ref__", (void*)float4__zwzz__float4_ref__, 123u, 2);
	__nullcFR[493] = __nullcRegisterFunction("float4__zwzw__float4_ref__", (void*)float4__zwzw__float4_ref__, 123u, 2);
	__nullcFR[494] = __nullcRegisterFunction("float4__zwwx__float4_ref__", (void*)float4__zwwx__float4_ref__, 123u, 2);
	__nullcFR[495] = __nullcRegisterFunction("float4__zwwy__float4_ref__", (void*)float4__zwwy__float4_ref__, 123u, 2);
	__nullcFR[496] = __nullcRegisterFunction("float4__zwwz__float4_ref__", (void*)float4__zwwz__float4_ref__, 123u, 2);
	__nullcFR[497] = __nullcRegisterFunction("float4__zwww__float4_ref__", (void*)float4__zwww__float4_ref__, 123u, 2);
	__nullcFR[498] = __nullcRegisterFunction("float4__wxxx__float4_ref__", (void*)float4__wxxx__float4_ref__, 123u, 2);
	__nullcFR[499] = __nullcRegisterFunction("float4__wxxy__float4_ref__", (void*)float4__wxxy__float4_ref__, 123u, 2);
	__nullcFR[500] = __nullcRegisterFunction("float4__wxxz__float4_ref__", (void*)float4__wxxz__float4_ref__, 123u, 2);
	__nullcFR[501] = __nullcRegisterFunction("float4__wxxw__float4_ref__", (void*)float4__wxxw__float4_ref__, 123u, 2);
	__nullcFR[502] = __nullcRegisterFunction("float4__wxyx__float4_ref__", (void*)float4__wxyx__float4_ref__, 123u, 2);
	__nullcFR[503] = __nullcRegisterFunction("float4__wxyy__float4_ref__", (void*)float4__wxyy__float4_ref__, 123u, 2);
	__nullcFR[504] = __nullcRegisterFunction("float4__wxyz__float4_ref__", (void*)float4__wxyz__float4_ref__, 123u, 2);
	__nullcFR[505] = __nullcRegisterFunction("float4__wxyz__void_ref_float4_", (void*)float4__wxyz__void_ref_float4_, 123u, 2);
	__nullcFR[506] = __nullcRegisterFunction("float4__wxyw__float4_ref__", (void*)float4__wxyw__float4_ref__, 123u, 2);
	__nullcFR[507] = __nullcRegisterFunction("float4__wxzx__float4_ref__", (void*)float4__wxzx__float4_ref__, 123u, 2);
	__nullcFR[508] = __nullcRegisterFunction("float4__wxzy__float4_ref__", (void*)float4__wxzy__float4_ref__, 123u, 2);
	__nullcFR[509] = __nullcRegisterFunction("float4__wxzy__void_ref_float4_", (void*)float4__wxzy__void_ref_float4_, 123u, 2);
	__nullcFR[510] = __nullcRegisterFunction("float4__wxzz__float4_ref__", (void*)float4__wxzz__float4_ref__, 123u, 2);
	__nullcFR[511] = __nullcRegisterFunction("float4__wxzw__float4_ref__", (void*)float4__wxzw__float4_ref__, 123u, 2);
	__nullcFR[512] = __nullcRegisterFunction("float4__wxwx__float4_ref__", (void*)float4__wxwx__float4_ref__, 123u, 2);
	__nullcFR[513] = __nullcRegisterFunction("float4__wxwy__float4_ref__", (void*)float4__wxwy__float4_ref__, 123u, 2);
	__nullcFR[514] = __nullcRegisterFunction("float4__wxwz__float4_ref__", (void*)float4__wxwz__float4_ref__, 123u, 2);
	__nullcFR[515] = __nullcRegisterFunction("float4__wxww__float4_ref__", (void*)float4__wxww__float4_ref__, 123u, 2);
	__nullcFR[516] = __nullcRegisterFunction("float4__wyxx__float4_ref__", (void*)float4__wyxx__float4_ref__, 123u, 2);
	__nullcFR[517] = __nullcRegisterFunction("float4__wyxy__float4_ref__", (void*)float4__wyxy__float4_ref__, 123u, 2);
	__nullcFR[518] = __nullcRegisterFunction("float4__wyxz__float4_ref__", (void*)float4__wyxz__float4_ref__, 123u, 2);
	__nullcFR[519] = __nullcRegisterFunction("float4__wyxz__void_ref_float4_", (void*)float4__wyxz__void_ref_float4_, 123u, 2);
	__nullcFR[520] = __nullcRegisterFunction("float4__wyxw__float4_ref__", (void*)float4__wyxw__float4_ref__, 123u, 2);
	__nullcFR[521] = __nullcRegisterFunction("float4__wyyx__float4_ref__", (void*)float4__wyyx__float4_ref__, 123u, 2);
	__nullcFR[522] = __nullcRegisterFunction("float4__wyyy__float4_ref__", (void*)float4__wyyy__float4_ref__, 123u, 2);
	__nullcFR[523] = __nullcRegisterFunction("float4__wyyz__float4_ref__", (void*)float4__wyyz__float4_ref__, 123u, 2);
	__nullcFR[524] = __nullcRegisterFunction("float4__wyyw__float4_ref__", (void*)float4__wyyw__float4_ref__, 123u, 2);
	__nullcFR[525] = __nullcRegisterFunction("float4__wyzx__float4_ref__", (void*)float4__wyzx__float4_ref__, 123u, 2);
	__nullcFR[526] = __nullcRegisterFunction("float4__wyzx__void_ref_float4_", (void*)float4__wyzx__void_ref_float4_, 123u, 2);
	__nullcFR[527] = __nullcRegisterFunction("float4__wyzy__float4_ref__", (void*)float4__wyzy__float4_ref__, 123u, 2);
	__nullcFR[528] = __nullcRegisterFunction("float4__wyzz__float4_ref__", (void*)float4__wyzz__float4_ref__, 123u, 2);
	__nullcFR[529] = __nullcRegisterFunction("float4__wyzw__float4_ref__", (void*)float4__wyzw__float4_ref__, 123u, 2);
	__nullcFR[530] = __nullcRegisterFunction("float4__wywx__float4_ref__", (void*)float4__wywx__float4_ref__, 123u, 2);
	__nullcFR[531] = __nullcRegisterFunction("float4__wywy__float4_ref__", (void*)float4__wywy__float4_ref__, 123u, 2);
	__nullcFR[532] = __nullcRegisterFunction("float4__wywz__float4_ref__", (void*)float4__wywz__float4_ref__, 123u, 2);
	__nullcFR[533] = __nullcRegisterFunction("float4__wyww__float4_ref__", (void*)float4__wyww__float4_ref__, 123u, 2);
	__nullcFR[534] = __nullcRegisterFunction("float4__wzxx__float4_ref__", (void*)float4__wzxx__float4_ref__, 123u, 2);
	__nullcFR[535] = __nullcRegisterFunction("float4__wzxy__float4_ref__", (void*)float4__wzxy__float4_ref__, 123u, 2);
	__nullcFR[536] = __nullcRegisterFunction("float4__wzxy__void_ref_float4_", (void*)float4__wzxy__void_ref_float4_, 123u, 2);
	__nullcFR[537] = __nullcRegisterFunction("float4__wzxz__float4_ref__", (void*)float4__wzxz__float4_ref__, 123u, 2);
	__nullcFR[538] = __nullcRegisterFunction("float4__wzxw__float4_ref__", (void*)float4__wzxw__float4_ref__, 123u, 2);
	__nullcFR[539] = __nullcRegisterFunction("float4__wzyx__float4_ref__", (void*)float4__wzyx__float4_ref__, 123u, 2);
	__nullcFR[540] = __nullcRegisterFunction("float4__wzyx__void_ref_float4_", (void*)float4__wzyx__void_ref_float4_, 123u, 2);
	__nullcFR[541] = __nullcRegisterFunction("float4__wzyy__float4_ref__", (void*)float4__wzyy__float4_ref__, 123u, 2);
	__nullcFR[542] = __nullcRegisterFunction("float4__wzyz__float4_ref__", (void*)float4__wzyz__float4_ref__, 123u, 2);
	__nullcFR[543] = __nullcRegisterFunction("float4__wzyw__float4_ref__", (void*)float4__wzyw__float4_ref__, 123u, 2);
	__nullcFR[544] = __nullcRegisterFunction("float4__wzzx__float4_ref__", (void*)float4__wzzx__float4_ref__, 123u, 2);
	__nullcFR[545] = __nullcRegisterFunction("float4__wzzy__float4_ref__", (void*)float4__wzzy__float4_ref__, 123u, 2);
	__nullcFR[546] = __nullcRegisterFunction("float4__wzzz__float4_ref__", (void*)float4__wzzz__float4_ref__, 123u, 2);
	__nullcFR[547] = __nullcRegisterFunction("float4__wzzw__float4_ref__", (void*)float4__wzzw__float4_ref__, 123u, 2);
	__nullcFR[548] = __nullcRegisterFunction("float4__wzwx__float4_ref__", (void*)float4__wzwx__float4_ref__, 123u, 2);
	__nullcFR[549] = __nullcRegisterFunction("float4__wzwy__float4_ref__", (void*)float4__wzwy__float4_ref__, 123u, 2);
	__nullcFR[550] = __nullcRegisterFunction("float4__wzwz__float4_ref__", (void*)float4__wzwz__float4_ref__, 123u, 2);
	__nullcFR[551] = __nullcRegisterFunction("float4__wzww__float4_ref__", (void*)float4__wzww__float4_ref__, 123u, 2);
	__nullcFR[552] = __nullcRegisterFunction("float4__wwxx__float4_ref__", (void*)float4__wwxx__float4_ref__, 123u, 2);
	__nullcFR[553] = __nullcRegisterFunction("float4__wwxy__float4_ref__", (void*)float4__wwxy__float4_ref__, 123u, 2);
	__nullcFR[554] = __nullcRegisterFunction("float4__wwxz__float4_ref__", (void*)float4__wwxz__float4_ref__, 123u, 2);
	__nullcFR[555] = __nullcRegisterFunction("float4__wwxw__float4_ref__", (void*)float4__wwxw__float4_ref__, 123u, 2);
	__nullcFR[556] = __nullcRegisterFunction("float4__wwyx__float4_ref__", (void*)float4__wwyx__float4_ref__, 123u, 2);
	__nullcFR[557] = __nullcRegisterFunction("float4__wwyy__float4_ref__", (void*)float4__wwyy__float4_ref__, 123u, 2);
	__nullcFR[558] = __nullcRegisterFunction("float4__wwyz__float4_ref__", (void*)float4__wwyz__float4_ref__, 123u, 2);
	__nullcFR[559] = __nullcRegisterFunction("float4__wwyw__float4_ref__", (void*)float4__wwyw__float4_ref__, 123u, 2);
	__nullcFR[560] = __nullcRegisterFunction("float4__wwzx__float4_ref__", (void*)float4__wwzx__float4_ref__, 123u, 2);
	__nullcFR[561] = __nullcRegisterFunction("float4__wwzy__float4_ref__", (void*)float4__wwzy__float4_ref__, 123u, 2);
	__nullcFR[562] = __nullcRegisterFunction("float4__wwzz__float4_ref__", (void*)float4__wwzz__float4_ref__, 123u, 2);
	__nullcFR[563] = __nullcRegisterFunction("float4__wwzw__float4_ref__", (void*)float4__wwzw__float4_ref__, 123u, 2);
	__nullcFR[564] = __nullcRegisterFunction("float4__wwwx__float4_ref__", (void*)float4__wwwx__float4_ref__, 123u, 2);
	__nullcFR[565] = __nullcRegisterFunction("float4__wwwy__float4_ref__", (void*)float4__wwwy__float4_ref__, 123u, 2);
	__nullcFR[566] = __nullcRegisterFunction("float4__wwwz__float4_ref__", (void*)float4__wwwz__float4_ref__, 123u, 2);
	__nullcFR[567] = __nullcRegisterFunction("float4__wwww__float4_ref__", (void*)float4__wwww__float4_ref__, 123u, 2);
	__nullcFR[568] = 0;
	__nullcFR[569] = 0;
	__nullcFR[570] = 0;
	__nullcFR[571] = 0;
	__nullcFR[572] = 0;
	__nullcFR[573] = 0;
	__nullcFR[574] = 0;
	__nullcFR[575] = 0;
	__nullcFR[576] = 0;
	__nullcFR[577] = 0;
	__nullcFR[578] = 0;
	__nullcFR[579] = 0;
	__nullcFR[580] = 0;
	__nullcFR[581] = 0;
	__nullcFR[582] = 0;
	__nullcFR[583] = 0;
	__nullcFR[584] = 0;
	__nullcFR[585] = 0;
	__nullcFR[586] = 0;
	__nullcFR[587] = 0;
	__nullcFR[588] = 0;
	__nullcFR[589] = 0;
	__nullcFR[590] = 0;
	__nullcFR[591] = __nullcRegisterFunction("float2__length_float_ref__", (void*)float2__length_float_ref__, 4294967295u, 2);
	__nullcFR[592] = __nullcRegisterFunction("float2__normalize_float_ref__", (void*)float2__normalize_float_ref__, 4294967295u, 2);
	__nullcFR[593] = __nullcRegisterFunction("float3__length_float_ref__", (void*)float3__length_float_ref__, 4294967295u, 2);
	__nullcFR[594] = __nullcRegisterFunction("float3__normalize_float_ref__", (void*)float3__normalize_float_ref__, 4294967295u, 2);
	__nullcFR[595] = __nullcRegisterFunction("float4__length_float_ref__", (void*)float4__length_float_ref__, 4294967295u, 2);
	__nullcFR[596] = __nullcRegisterFunction("float4__normalize_float_ref__", (void*)float4__normalize_float_ref__, 4294967295u, 2);
	__nullcFR[597] = 0;
	__nullcFR[598] = 0;
	__nullcFR[599] = 0;
	__nullcFR[600] = __nullcRegisterFunction("reflect", (void*)reflect, 31u, 0);
	*(&(&math)->pi) = 3.141593;
	*(&(&math)->e) = 2.718282;
	/* node translation unknown */
	/* node translation unknown */
	/* node translation unknown */
}
