import std.math;

float2 float2.xx(){ return float2(x, x); }
float2 float2.xy(){ return float2(x, y); } float2 float2.xy(float2 r){ x = r.x; y = r.y; return float2(x, y); }
float2 float2.yx(){ return float2(y, x); } float2 float2.yx(float2 r){ y = r.x; x = r.y; return float2(x, y); }
float2 float2.yy(){ return float2(y, y); }

float2 float3.xx(){ return float2(x, x); }
float2 float3.xy(){ return float2(x, y); } float2 float3.xy(float2 r){ x = r.x; y = r.y; return float2(x, y); }
float2 float3.xz(){ return float2(x, z); } float2 float3.xz(float2 r){ x = r.x; z = r.y; return float2(x, y); }
float2 float3.yx(){ return float2(y, x); } float2 float3.yx(float2 r){ y = r.x; x = r.y; return float2(x, y); }
float2 float3.yy(){ return float2(y, y); }
float2 float3.yz(){ return float2(y, z); } float2 float3.yz(float2 r){ y = r.x; z = r.y; return float2(x, y); }
float2 float3.zx(){ return float2(z, x); } float2 float3.zx(float2 r){ z = r.x; x = r.y; return float2(x, y); }
float2 float3.zy(){ return float2(z, y); } float2 float3.zy(float2 r){ z = r.x; y = r.y; return float2(x, y); }
float2 float3.zz(){ return float2(z, z); }
	
float3 float3.xxx(){ return float3(x, x, x); }
float3 float3.xxy(){ return float3(x, x, y); }
float3 float3.xxz(){ return float3(x, x, z); }
float3 float3.xyx(){ return float3(x, y, x); }
float3 float3.xyy(){ return float3(x, y, y); }
float3 float3.xyz(){ return float3(x, y, z); } float3 float3.xyz(float3 r){ x = r.x; y = r.y; z = r.z; return float3(x, y, z); }
float3 float3.xzx(){ return float3(x, z, x); }
float3 float3.xzy(){ return float3(x, z, y); } float3 float3.xzy(float3 r){ x = r.x; z = r.y; y = r.z; return float3(x, y, z); }
float3 float3.xzz(){ return float3(x, z, z); }
float3 float3.yxx(){ return float3(y, x, x); }
float3 float3.yxy(){ return float3(y, x, y); }
float3 float3.yxz(){ return float3(y, x, z); } float3 float3.yxz(float3 r){ y = r.x; x = r.y; z = r.z; return float3(x, y, z); }
float3 float3.yyx(){ return float3(y, y, x); }
float3 float3.yyy(){ return float3(y, y, y); }
float3 float3.yyz(){ return float3(y, y, z); }
float3 float3.yzx(){ return float3(y, z, x); } float3 float3.yzx(float3 r){ y = r.x; z = r.y; x = r.z; return float3(x, y, z); }
float3 float3.yzy(){ return float3(y, z, y); }
float3 float3.yzz(){ return float3(y, z, z); }
float3 float3.zxx(){ return float3(z, x, x); }
float3 float3.zxy(){ return float3(z, x, y); } float3 float3.zxy(float3 r){ z = r.x; x = r.y; y = r.z; return float3(x, y, z); }
float3 float3.zxz(){ return float3(z, x, z); }
float3 float3.zyx(){ return float3(z, y, x); } float3 float3.zyx(float3 r){ z = r.x; y = r.y; x = r.z; return float3(x, y, z); }
float3 float3.zyy(){ return float3(z, y, y); }
float3 float3.zyz(){ return float3(z, y, z); }
float3 float3.zzx(){ return float3(z, z, x); }
float3 float3.zzy(){ return float3(z, z, y); }
float3 float3.zzz(){ return float3(z, z, z); }

float2 float4.xx(){ return float2(x, x); }
float2 float4.xy(){ return float2(x, y); } float2 float4.xy(float2 r){ x = r.x; y = r.y; return float2(x, y); }
float2 float4.xz(){ return float2(x, z); } float2 float4.xz(float2 r){ x = r.x; z = r.y; return float2(x, y); }
float2 float4.xw(){ return float2(x, w); } float2 float4.xw(float2 r){ x = r.x; w = r.y; return float2(x, y); }
float2 float4.yx(){ return float2(y, x); } float2 float4.yx(float2 r){ y = r.x; x = r.y; return float2(x, y); }
float2 float4.yy(){ return float2(y, y); }
float2 float4.yz(){ return float2(y, z); } float2 float4.yz(float2 r){ y = r.x; z = r.y; return float2(x, y); }
float2 float4.yw(){ return float2(y, w); } float2 float4.yw(float2 r){ y = r.x; w = r.y; return float2(x, y); }
float2 float4.zx(){ return float2(z, x); } float2 float4.zx(float2 r){ z = r.x; x = r.y; return float2(x, y); }
float2 float4.zy(){ return float2(z, y); } float2 float4.zy(float2 r){ z = r.x; y = r.y; return float2(x, y); }
float2 float4.zz(){ return float2(z, z); }
float2 float4.zw(){ return float2(z, w); } float2 float4.zw(float2 r){ z = r.x; w = r.y; return float2(x, y); }
float2 float4.wx(){ return float2(w, x); } float2 float4.wx(float2 r){ w = r.x; x = r.y; return float2(x, y); }
float2 float4.wy(){ return float2(w, y); } float2 float4.wy(float2 r){ w = r.x; y = r.y; return float2(x, y); }
float2 float4.wz(){ return float2(w, z); } float2 float4.wz(float2 r){ w = r.x; z = r.y; return float2(x, y); }
float2 float4.ww(){ return float2(w, w); }
	
float3 float4.xxx(){ return float3(x, x, x); }
float3 float4.xxy(){ return float3(x, x, y); }
float3 float4.xxz(){ return float3(x, x, z); }
float3 float4.xxw(){ return float3(x, x, w); }
float3 float4.xyx(){ return float3(x, y, x); }
float3 float4.xyy(){ return float3(x, y, y); }
float3 float4.xyz(){ return float3(x, y, z); } float3 float4.xyz(float3 r){ x = r.x; y = r.y; z = r.z; return float3(x, y, z); }
float3 float4.xyw(){ return float3(x, y, w); } float3 float4.xyw(float3 r){ x = r.x; y = r.y; w = r.z; return float3(x, y, z); }
float3 float4.xzx(){ return float3(x, z, x); }
float3 float4.xzy(){ return float3(x, z, y); } float3 float4.xzy(float3 r){ x = r.x; z = r.y; y = r.z; return float3(x, y, z); }
float3 float4.xzz(){ return float3(x, z, z); }
float3 float4.xzw(){ return float3(x, z, w); } float3 float4.xzw(float3 r){ x = r.x; z = r.y; w = r.z; return float3(x, y, z); }
float3 float4.xwx(){ return float3(x, w, x); }
float3 float4.xwy(){ return float3(x, w, y); } float3 float4.xwy(float3 r){ x = r.x; w = r.y; y = r.z; return float3(x, y, z); }
float3 float4.xwz(){ return float3(x, w, z); } float3 float4.xwz(float3 r){ x = r.x; w = r.y; z = r.z; return float3(x, y, z); }
float3 float4.xww(){ return float3(x, w, w); }
float3 float4.yxx(){ return float3(y, x, x); }
float3 float4.yxy(){ return float3(y, x, y); }
float3 float4.yxz(){ return float3(y, x, z); } float3 float4.yxz(float3 r){ y = r.x; x = r.y; z = r.z; return float3(x, y, z); }
float3 float4.yxw(){ return float3(y, x, w); } float3 float4.yxw(float3 r){ y = r.x; x = r.y; w = r.z; return float3(x, y, z); }
float3 float4.yyx(){ return float3(y, y, x); }
float3 float4.yyy(){ return float3(y, y, y); }
float3 float4.yyz(){ return float3(y, y, z); }
float3 float4.yyw(){ return float3(y, y, w); }
float3 float4.yzx(){ return float3(y, z, x); } float3 float4.yzx(float3 r){ y = r.x; z = r.y; x = r.z; return float3(x, y, z); }
float3 float4.yzy(){ return float3(y, z, y); }
float3 float4.yzz(){ return float3(y, z, z); }
float3 float4.yzw(){ return float3(y, z, w); } float3 float4.yzw(float3 r){ y = r.x; z = r.y; w = r.z; return float3(x, y, z); }
float3 float4.ywx(){ return float3(y, w, x); } float3 float4.ywx(float3 r){ y = r.x; w = r.y; x = r.z; return float3(x, y, z); }
float3 float4.ywy(){ return float3(y, w, y); }
float3 float4.ywz(){ return float3(y, w, z); } float3 float4.ywz(float3 r){ y = r.x; w = r.y; z = r.z; return float3(x, y, z); }
float3 float4.yww(){ return float3(y, w, w); }
float3 float4.zxx(){ return float3(z, x, x); }
float3 float4.zxy(){ return float3(z, x, y); } float3 float4.zxy(float3 r){ z = r.x; x = r.y; y = r.z; return float3(x, y, z); }
float3 float4.zxz(){ return float3(z, x, z); }
float3 float4.zxw(){ return float3(z, x, w); } float3 float4.zxw(float3 r){ z = r.x; x = r.y; w = r.z; return float3(x, y, z); }
float3 float4.zyx(){ return float3(z, y, x); } float3 float4.zyx(float3 r){ z = r.x; y = r.y; x = r.z; return float3(x, y, z); }
float3 float4.zyy(){ return float3(z, y, y); }
float3 float4.zyz(){ return float3(z, y, z); }
float3 float4.zyw(){ return float3(z, y, w); } float3 float4.zyw(float3 r){ z = r.x; y = r.y; w = r.z; return float3(x, y, z); }
float3 float4.zzx(){ return float3(z, z, x); }
float3 float4.zzy(){ return float3(z, z, y); }
float3 float4.zzz(){ return float3(z, z, z); }
float3 float4.zzw(){ return float3(z, z, w); }
float3 float4.zwx(){ return float3(z, w, x); } float3 float4.zwx(float3 r){ z = r.x; w = r.y; x = r.z; return float3(x, y, z); }
float3 float4.zwy(){ return float3(z, w, y); } float3 float4.zwy(float3 r){ z = r.x; w = r.y; y = r.z; return float3(x, y, z); }
float3 float4.zwz(){ return float3(z, w, z); }
float3 float4.zww(){ return float3(z, w, w); }
float3 float4.wxx(){ return float3(w, x, x); }
float3 float4.wxy(){ return float3(w, x, y); } float3 float4.wxy(float3 r){ w = r.x; x = r.y; y = r.z; return float3(x, y, z); }
float3 float4.wxz(){ return float3(w, x, z); } float3 float4.wxz(float3 r){ w = r.x; x = r.y; z = r.z; return float3(x, y, z); }
float3 float4.wxw(){ return float3(w, x, w); }
float3 float4.wyx(){ return float3(w, y, x); } float3 float4.wyx(float3 r){ w = r.x; y = r.y; x = r.z; return float3(x, y, z); }
float3 float4.wyy(){ return float3(w, y, y); }
float3 float4.wyz(){ return float3(w, y, z); } float3 float4.wyz(float3 r){ w = r.x; y = r.y; z = r.z; return float3(x, y, z); }
float3 float4.wyw(){ return float3(w, y, w); }
float3 float4.wzx(){ return float3(w, z, x); } float3 float4.wzx(float3 r){ w = r.x; z = r.y; x = r.z; return float3(x, y, z); }
float3 float4.wzy(){ return float3(w, z, y); } float3 float4.wzy(float3 r){ w = r.x; z = r.y; y = r.z; return float3(x, y, z); }
float3 float4.wzz(){ return float3(w, z, z); }
float3 float4.wzw(){ return float3(w, z, w); }
float3 float4.wwx(){ return float3(w, w, x); }
float3 float4.wwy(){ return float3(w, w, y); }
float3 float4.wwz(){ return float3(w, w, z); }
float3 float4.www(){ return float3(w, w, w); }
	
float4 float4.xxxx(){ return float4(x, x, x, x); }
float4 float4.xxxy(){ return float4(x, x, x, y); }
float4 float4.xxxz(){ return float4(x, x, x, z); }
float4 float4.xxxw(){ return float4(x, x, x, w); }
float4 float4.xxyx(){ return float4(x, x, y, x); }
float4 float4.xxyy(){ return float4(x, x, y, y); }
float4 float4.xxyz(){ return float4(x, x, y, z); }
float4 float4.xxyw(){ return float4(x, x, y, w); }
float4 float4.xxzx(){ return float4(x, x, z, x); }
float4 float4.xxzy(){ return float4(x, x, z, y); }
float4 float4.xxzz(){ return float4(x, x, z, z); }
float4 float4.xxzw(){ return float4(x, x, z, w); }
float4 float4.xxwx(){ return float4(x, x, w, x); }
float4 float4.xxwy(){ return float4(x, x, w, y); }
float4 float4.xxwz(){ return float4(x, x, w, z); }
float4 float4.xxww(){ return float4(x, x, w, w); }
float4 float4.xyxx(){ return float4(x, y, x, x); }
float4 float4.xyxy(){ return float4(x, y, x, y); }
float4 float4.xyxz(){ return float4(x, y, x, z); }
float4 float4.xyxw(){ return float4(x, y, x, w); }
float4 float4.xyyx(){ return float4(x, y, y, x); }
float4 float4.xyyy(){ return float4(x, y, y, y); }
float4 float4.xyyz(){ return float4(x, y, y, z); }
float4 float4.xyyw(){ return float4(x, y, y, w); }
float4 float4.xyzx(){ return float4(x, y, z, x); }
float4 float4.xyzy(){ return float4(x, y, z, y); }
float4 float4.xyzz(){ return float4(x, y, z, z); }
float4 float4.xyzw(){ return float4(x, y, z, w); } float4 float4.xyzw(float4 r){ x = r.x; y = r.y; z = r.z; w = r.w; return float4(x, y, z, w); }
float4 float4.xywx(){ return float4(x, y, w, x); }
float4 float4.xywy(){ return float4(x, y, w, y); }
float4 float4.xywz(){ return float4(x, y, w, z); } float4 float4.xywz(float4 r){ x = r.x; y = r.y; w = r.z; z = r.w; return float4(x, y, z, w); }
float4 float4.xyww(){ return float4(x, y, w, w); }
float4 float4.xzxx(){ return float4(x, z, x, x); }
float4 float4.xzxy(){ return float4(x, z, x, y); }
float4 float4.xzxz(){ return float4(x, z, x, z); }
float4 float4.xzxw(){ return float4(x, z, x, w); }
float4 float4.xzyx(){ return float4(x, z, y, x); }
float4 float4.xzyy(){ return float4(x, z, y, y); }
float4 float4.xzyz(){ return float4(x, z, y, z); }
float4 float4.xzyw(){ return float4(x, z, y, w); } float4 float4.xzyw(float4 r){ x = r.x; z = r.y; y = r.z; w = r.w; return float4(x, y, z, w); }
float4 float4.xzzx(){ return float4(x, z, z, x); }
float4 float4.xzzy(){ return float4(x, z, z, y); }
float4 float4.xzzz(){ return float4(x, z, z, z); }
float4 float4.xzzw(){ return float4(x, z, z, w); }
float4 float4.xzwx(){ return float4(x, z, w, x); }
float4 float4.xzwy(){ return float4(x, z, w, y); } float4 float4.xzwy(float4 r){ x = r.x; z = r.y; w = r.z; y = r.w; return float4(x, y, z, w); }
float4 float4.xzwz(){ return float4(x, z, w, z); }
float4 float4.xzww(){ return float4(x, z, w, w); }
float4 float4.xwxx(){ return float4(x, w, x, x); }
float4 float4.xwxy(){ return float4(x, w, x, y); }
float4 float4.xwxz(){ return float4(x, w, x, z); }
float4 float4.xwxw(){ return float4(x, w, x, w); }
float4 float4.xwyx(){ return float4(x, w, y, x); }
float4 float4.xwyy(){ return float4(x, w, y, y); }
float4 float4.xwyz(){ return float4(x, w, y, z); } float4 float4.xwyz(float4 r){ x = r.x; w = r.y; y = r.z; z = r.w; return float4(x, y, z, w); }
float4 float4.xwyw(){ return float4(x, w, y, w); }
float4 float4.xwzx(){ return float4(x, w, z, x); }
float4 float4.xwzy(){ return float4(x, w, z, y); } float4 float4.xwzy(float4 r){ x = r.x; w = r.y; z = r.z; y = r.w; return float4(x, y, z, w); }
float4 float4.xwzz(){ return float4(x, w, z, z); }
float4 float4.xwzw(){ return float4(x, w, z, w); }
float4 float4.xwwx(){ return float4(x, w, w, x); }
float4 float4.xwwy(){ return float4(x, w, w, y); }
float4 float4.xwwz(){ return float4(x, w, w, z); }
float4 float4.xwww(){ return float4(x, w, w, w); }
float4 float4.yxxx(){ return float4(y, x, x, x); }
float4 float4.yxxy(){ return float4(y, x, x, y); }
float4 float4.yxxz(){ return float4(y, x, x, z); }
float4 float4.yxxw(){ return float4(y, x, x, w); }
float4 float4.yxyx(){ return float4(y, x, y, x); }
float4 float4.yxyy(){ return float4(y, x, y, y); }
float4 float4.yxyz(){ return float4(y, x, y, z); }
float4 float4.yxyw(){ return float4(y, x, y, w); }
float4 float4.yxzx(){ return float4(y, x, z, x); }
float4 float4.yxzy(){ return float4(y, x, z, y); }
float4 float4.yxzz(){ return float4(y, x, z, z); }
float4 float4.yxzw(){ return float4(y, x, z, w); } float4 float4.yxzw(float4 r){ y = r.x; x = r.y; z = r.z; w = r.w; return float4(x, y, z, w); }
float4 float4.yxwx(){ return float4(y, x, w, x); }
float4 float4.yxwy(){ return float4(y, x, w, y); }
float4 float4.yxwz(){ return float4(y, x, w, z); } float4 float4.yxwz(float4 r){ y = r.x; x = r.y; w = r.z; z = r.w; return float4(x, y, z, w); }
float4 float4.yxww(){ return float4(y, x, w, w); }
float4 float4.yyxx(){ return float4(y, y, x, x); }
float4 float4.yyxy(){ return float4(y, y, x, y); }
float4 float4.yyxz(){ return float4(y, y, x, z); }
float4 float4.yyxw(){ return float4(y, y, x, w); }
float4 float4.yyyx(){ return float4(y, y, y, x); }
float4 float4.yyyy(){ return float4(y, y, y, y); }
float4 float4.yyyz(){ return float4(y, y, y, z); }
float4 float4.yyyw(){ return float4(y, y, y, w); }
float4 float4.yyzx(){ return float4(y, y, z, x); }
float4 float4.yyzy(){ return float4(y, y, z, y); }
float4 float4.yyzz(){ return float4(y, y, z, z); }
float4 float4.yyzw(){ return float4(y, y, z, w); }
float4 float4.yywx(){ return float4(y, y, w, x); }
float4 float4.yywy(){ return float4(y, y, w, y); }
float4 float4.yywz(){ return float4(y, y, w, z); }
float4 float4.yyww(){ return float4(y, y, w, w); }
float4 float4.yzxx(){ return float4(y, z, x, x); }
float4 float4.yzxy(){ return float4(y, z, x, y); }
float4 float4.yzxz(){ return float4(y, z, x, z); }
float4 float4.yzxw(){ return float4(y, z, x, w); } float4 float4.yzxw(float4 r){ y = r.x; z = r.y; x = r.z; w = r.w; return float4(x, y, z, w); }
float4 float4.yzyx(){ return float4(y, z, y, x); }
float4 float4.yzyy(){ return float4(y, z, y, y); }
float4 float4.yzyz(){ return float4(y, z, y, z); }
float4 float4.yzyw(){ return float4(y, z, y, w); }
float4 float4.yzzx(){ return float4(y, z, z, x); }
float4 float4.yzzy(){ return float4(y, z, z, y); }
float4 float4.yzzz(){ return float4(y, z, z, z); }
float4 float4.yzzw(){ return float4(y, z, z, w); }
float4 float4.yzwx(){ return float4(y, z, w, x); } float4 float4.yzwx(float4 r){ y = r.x; z = r.y; w = r.z; x = r.w; return float4(x, y, z, w); }
float4 float4.yzwy(){ return float4(y, z, w, y); }
float4 float4.yzwz(){ return float4(y, z, w, z); }
float4 float4.yzww(){ return float4(y, z, w, w); }
float4 float4.ywxx(){ return float4(y, w, x, x); }
float4 float4.ywxy(){ return float4(y, w, x, y); }
float4 float4.ywxz(){ return float4(y, w, x, z); } float4 float4.ywxz(float4 r){ y = r.x; w = r.y; x = r.z; z = r.w; return float4(x, y, z, w); }
float4 float4.ywxw(){ return float4(y, w, x, w); }
float4 float4.ywyx(){ return float4(y, w, y, x); }
float4 float4.ywyy(){ return float4(y, w, y, y); }
float4 float4.ywyz(){ return float4(y, w, y, z); }
float4 float4.ywyw(){ return float4(y, w, y, w); }
float4 float4.ywzx(){ return float4(y, w, z, x); } float4 float4.ywzx(float4 r){ y = r.x; w = r.y; z = r.z; x = r.w; return float4(x, y, z, w); }
float4 float4.ywzy(){ return float4(y, w, z, y); }
float4 float4.ywzz(){ return float4(y, w, z, z); }
float4 float4.ywzw(){ return float4(y, w, z, w); }
float4 float4.ywwx(){ return float4(y, w, w, x); }
float4 float4.ywwy(){ return float4(y, w, w, y); }
float4 float4.ywwz(){ return float4(y, w, w, z); }
float4 float4.ywww(){ return float4(y, w, w, w); }
float4 float4.zxxx(){ return float4(z, x, x, x); }
float4 float4.zxxy(){ return float4(z, x, x, y); }
float4 float4.zxxz(){ return float4(z, x, x, z); }
float4 float4.zxxw(){ return float4(z, x, x, w); }
float4 float4.zxyx(){ return float4(z, x, y, x); }
float4 float4.zxyy(){ return float4(z, x, y, y); }
float4 float4.zxyz(){ return float4(z, x, y, z); }
float4 float4.zxyw(){ return float4(z, x, y, w); } float4 float4.zxyw(float4 r){ z = r.x; x = r.y; y = r.z; w = r.w; return float4(x, y, z, w); }
float4 float4.zxzx(){ return float4(z, x, z, x); }
float4 float4.zxzy(){ return float4(z, x, z, y); }
float4 float4.zxzz(){ return float4(z, x, z, z); }
float4 float4.zxzw(){ return float4(z, x, z, w); }
float4 float4.zxwx(){ return float4(z, x, w, x); }
float4 float4.zxwy(){ return float4(z, x, w, y); } float4 float4.zxwy(float4 r){ z = r.x; x = r.y; w = r.z; y = r.w; return float4(x, y, z, w); }
float4 float4.zxwz(){ return float4(z, x, w, z); }
float4 float4.zxww(){ return float4(z, x, w, w); }
float4 float4.zyxx(){ return float4(z, y, x, x); }
float4 float4.zyxy(){ return float4(z, y, x, y); }
float4 float4.zyxz(){ return float4(z, y, x, z); }
float4 float4.zyxw(){ return float4(z, y, x, w); } float4 float4.zyxw(float4 r){ z = r.x; y = r.y; x = r.z; w = r.w; return float4(x, y, z, w); }
float4 float4.zyyx(){ return float4(z, y, y, x); }
float4 float4.zyyy(){ return float4(z, y, y, y); }
float4 float4.zyyz(){ return float4(z, y, y, z); }
float4 float4.zyyw(){ return float4(z, y, y, w); }
float4 float4.zyzx(){ return float4(z, y, z, x); }
float4 float4.zyzy(){ return float4(z, y, z, y); }
float4 float4.zyzz(){ return float4(z, y, z, z); }
float4 float4.zyzw(){ return float4(z, y, z, w); }
float4 float4.zywx(){ return float4(z, y, w, x); } float4 float4.zywx(float4 r){ z = r.x; y = r.y; w = r.z; x = r.w; return float4(x, y, z, w); }
float4 float4.zywy(){ return float4(z, y, w, y); }
float4 float4.zywz(){ return float4(z, y, w, z); }
float4 float4.zyww(){ return float4(z, y, w, w); }
float4 float4.zzxx(){ return float4(z, z, x, x); }
float4 float4.zzxy(){ return float4(z, z, x, y); }
float4 float4.zzxz(){ return float4(z, z, x, z); }
float4 float4.zzxw(){ return float4(z, z, x, w); }
float4 float4.zzyx(){ return float4(z, z, y, x); }
float4 float4.zzyy(){ return float4(z, z, y, y); }
float4 float4.zzyz(){ return float4(z, z, y, z); }
float4 float4.zzyw(){ return float4(z, z, y, w); }
float4 float4.zzzx(){ return float4(z, z, z, x); }
float4 float4.zzzy(){ return float4(z, z, z, y); }
float4 float4.zzzz(){ return float4(z, z, z, z); }
float4 float4.zzzw(){ return float4(z, z, z, w); }
float4 float4.zzwx(){ return float4(z, z, w, x); }
float4 float4.zzwy(){ return float4(z, z, w, y); }
float4 float4.zzwz(){ return float4(z, z, w, z); }
float4 float4.zzww(){ return float4(z, z, w, w); }
float4 float4.zwxx(){ return float4(z, w, x, x); }
float4 float4.zwxy(){ return float4(z, w, x, y); } float4 float4.zwxy(float4 r){ z = r.x; w = r.y; x = r.z; y = r.w; return float4(x, y, z, w); }
float4 float4.zwxz(){ return float4(z, w, x, z); }
float4 float4.zwxw(){ return float4(z, w, x, w); }
float4 float4.zwyx(){ return float4(z, w, y, x); } float4 float4.zwyx(float4 r){ z = r.x; w = r.y; y = r.z; x = r.w; return float4(x, y, z, w); }
float4 float4.zwyy(){ return float4(z, w, y, y); }
float4 float4.zwyz(){ return float4(z, w, y, z); }
float4 float4.zwyw(){ return float4(z, w, y, w); }
float4 float4.zwzx(){ return float4(z, w, z, x); }
float4 float4.zwzy(){ return float4(z, w, z, y); }
float4 float4.zwzz(){ return float4(z, w, z, z); }
float4 float4.zwzw(){ return float4(z, w, z, w); }
float4 float4.zwwx(){ return float4(z, w, w, x); }
float4 float4.zwwy(){ return float4(z, w, w, y); }
float4 float4.zwwz(){ return float4(z, w, w, z); }
float4 float4.zwww(){ return float4(z, w, w, w); }
float4 float4.wxxx(){ return float4(w, x, x, x); }
float4 float4.wxxy(){ return float4(w, x, x, y); }
float4 float4.wxxz(){ return float4(w, x, x, z); }
float4 float4.wxxw(){ return float4(w, x, x, w); }
float4 float4.wxyx(){ return float4(w, x, y, x); }
float4 float4.wxyy(){ return float4(w, x, y, y); }
float4 float4.wxyz(){ return float4(w, x, y, z); } float4 float4.wxyz(float4 r){ w = r.x; x = r.y; y = r.z; z = r.w; return float4(x, y, z, w); }
float4 float4.wxyw(){ return float4(w, x, y, w); }
float4 float4.wxzx(){ return float4(w, x, z, x); }
float4 float4.wxzy(){ return float4(w, x, z, y); } float4 float4.wxzy(float4 r){ w = r.x; x = r.y; z = r.z; y = r.w; return float4(x, y, z, w); }
float4 float4.wxzz(){ return float4(w, x, z, z); }
float4 float4.wxzw(){ return float4(w, x, z, w); }
float4 float4.wxwx(){ return float4(w, x, w, x); }
float4 float4.wxwy(){ return float4(w, x, w, y); }
float4 float4.wxwz(){ return float4(w, x, w, z); }
float4 float4.wxww(){ return float4(w, x, w, w); }
float4 float4.wyxx(){ return float4(w, y, x, x); }
float4 float4.wyxy(){ return float4(w, y, x, y); }
float4 float4.wyxz(){ return float4(w, y, x, z); } float4 float4.wyxz(float4 r){ w = r.x; y = r.y; x = r.z; z = r.w; return float4(x, y, z, w); }
float4 float4.wyxw(){ return float4(w, y, x, w); }
float4 float4.wyyx(){ return float4(w, y, y, x); }
float4 float4.wyyy(){ return float4(w, y, y, y); }
float4 float4.wyyz(){ return float4(w, y, y, z); }
float4 float4.wyyw(){ return float4(w, y, y, w); }
float4 float4.wyzx(){ return float4(w, y, z, x); } float4 float4.wyzx(float4 r){ w = r.x; y = r.y; z = r.z; x = r.w; return float4(x, y, z, w); }
float4 float4.wyzy(){ return float4(w, y, z, y); }
float4 float4.wyzz(){ return float4(w, y, z, z); }
float4 float4.wyzw(){ return float4(w, y, z, w); }
float4 float4.wywx(){ return float4(w, y, w, x); }
float4 float4.wywy(){ return float4(w, y, w, y); }
float4 float4.wywz(){ return float4(w, y, w, z); }
float4 float4.wyww(){ return float4(w, y, w, w); }
float4 float4.wzxx(){ return float4(w, z, x, x); }
float4 float4.wzxy(){ return float4(w, z, x, y); } float4 float4.wzxy(float4 r){ w = r.x; z = r.y; x = r.z; y = r.w; return float4(x, y, z, w); }
float4 float4.wzxz(){ return float4(w, z, x, z); }
float4 float4.wzxw(){ return float4(w, z, x, w); }
float4 float4.wzyx(){ return float4(w, z, y, x); } float4 float4.wzyx(float4 r){ w = r.x; z = r.y; y = r.z; x = r.w; return float4(x, y, z, w); }
float4 float4.wzyy(){ return float4(w, z, y, y); }
float4 float4.wzyz(){ return float4(w, z, y, z); }
float4 float4.wzyw(){ return float4(w, z, y, w); }
float4 float4.wzzx(){ return float4(w, z, z, x); }
float4 float4.wzzy(){ return float4(w, z, z, y); }
float4 float4.wzzz(){ return float4(w, z, z, z); }
float4 float4.wzzw(){ return float4(w, z, z, w); }
float4 float4.wzwx(){ return float4(w, z, w, x); }
float4 float4.wzwy(){ return float4(w, z, w, y); }
float4 float4.wzwz(){ return float4(w, z, w, z); }
float4 float4.wzww(){ return float4(w, z, w, w); }
float4 float4.wwxx(){ return float4(w, w, x, x); }
float4 float4.wwxy(){ return float4(w, w, x, y); }
float4 float4.wwxz(){ return float4(w, w, x, z); }
float4 float4.wwxw(){ return float4(w, w, x, w); }
float4 float4.wwyx(){ return float4(w, w, y, x); }
float4 float4.wwyy(){ return float4(w, w, y, y); }
float4 float4.wwyz(){ return float4(w, w, y, z); }
float4 float4.wwyw(){ return float4(w, w, y, w); }
float4 float4.wwzx(){ return float4(w, w, z, x); }
float4 float4.wwzy(){ return float4(w, w, z, y); }
float4 float4.wwzz(){ return float4(w, w, z, z); }
float4 float4.wwzw(){ return float4(w, w, z, w); }
float4 float4.wwwx(){ return float4(w, w, w, x); }
float4 float4.wwwy(){ return float4(w, w, w, y); }
float4 float4.wwwz(){ return float4(w, w, w, z); }
float4 float4.wwww(){ return float4(w, w, w, w); }
