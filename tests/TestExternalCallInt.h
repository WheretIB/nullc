
LOAD_MODULE_BIND_(test_ext1, "test.ext1", "char char_(char a); short short_(short a); int int_(int a); long long_(long a); float float_(float a); double double_(double a);")
{
	BIND_FUNCTION("test.ext1", TestInt, "char_", 0);
	BIND_FUNCTION("test.ext1", TestInt, "short_", 0);
	BIND_FUNCTION("test.ext1", TestInt, "int_", 0);
	BIND_FUNCTION("test.ext1", TestLong, "long_", 0);
	BIND_FUNCTION("test.ext1", TestFloat, "float_", 0);
	BIND_FUNCTION("test.ext1", TestDouble, "double_", 0);
}

const char	*TEST_CODE(testExternalCallBasic1) = "import test.ext1" MODULE_SUFFIX ";\r\n	auto Char = char_;\r\n		return Char(24);";
TEST_RESULT_("External function call. char type.", testExternalCallBasic1, "24");
const char	*TEST_CODE(testExternalCallBasic2) = "import test.ext1" MODULE_SUFFIX ";\r\n	auto Short = short_;\r\n	return Short(57);";
TEST_RESULT_("External function call. short type.", testExternalCallBasic2, "57");
const char	*TEST_CODE(testExternalCallBasic3) = "import test.ext1" MODULE_SUFFIX ";\r\n		auto Int = int_;\r\n		return Int(2458);";
TEST_RESULT_("External function call. int type.", testExternalCallBasic3, "2458");
const char	*TEST_CODE(testExternalCallBasic4) = "import test.ext1" MODULE_SUFFIX ";\r\n	auto Long = long_;\r\n		return Long(14841324198l);";
TEST_RESULT_("External function call. long type.", testExternalCallBasic4, "14841324198L");
const char	*TEST_CODE(testExternalCallBasic5) = "import test.ext1" MODULE_SUFFIX ";\r\n	auto Float = float_;\r\n	return int(Float(3.0));";
TEST_RESULT_("External function call. float type.", testExternalCallBasic5, "3");
const char	*TEST_CODE(testExternalCallBasic6) = "import test.ext1" MODULE_SUFFIX ";\r\n	auto Double = double_;\r\n	return int(Double(2.0));";
TEST_RESULT_("External function call. double type.", testExternalCallBasic6, "2");

LOAD_MODULE_BIND_(test_ext2, "test.ext2", "int Call(char a, short b, int c, long d, char e, short f, int g, long h, char i, short j, int k, long l);")
{
	BIND_FUNCTION("test.ext2", TestExt2, "Call", 0);
}
const char	*TEST_CODE(testExternalCall2) =
"import test.ext2" MODULE_SUFFIX ";\r\n\
return Call(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);";
TEST_RESULT_("External function call. Integer types, arguments through stack.", testExternalCall2, "1");

LOAD_MODULE_BIND_(test_ext3, "test.ext3", "int Call(char a, short b, int c, long d, char e, short f, int g, long h, char i, short j, int k, long l);")
{
	BIND_FUNCTION("test.ext3", TestExt3, "Call", 0);
}
const char	*TEST_CODE(testExternalCall3) =
"import test.ext3" MODULE_SUFFIX ";\r\n\
return Call(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12);";
TEST_RESULT_("External function call. Integer types, arguments through stack. Sign-extend.", testExternalCall3, "1");

LOAD_MODULE_BIND_(test_ext4, "test.ext4", "int Call(char a, short b, int c, long d, float e, double f, char g, short h, int i, long j, float k, double l);")
{
	BIND_FUNCTION("test.ext4", TestExt4, "Call", 0);
}
const char	*TEST_CODE(testExternalCall4) =
"import test.ext4" MODULE_SUFFIX ";\r\n\
return Call(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12);";
TEST_RESULT_("External function call. Basic types, arguments through stack. sx", testExternalCall4, "1");

LOAD_MODULE_BIND_(test_ext4d, "test.ext4d", "int Call(float a, double b, int c, long d, float e, double f, float g, double h, int i, long j, float k, double l);")
{
	BIND_FUNCTION("test.ext4d", TestExt4d, "Call", 0);
}
const char	*TEST_CODE(testExternalCall4d) =
"import test.ext4d" MODULE_SUFFIX ";\r\n\
return Call(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12);";
TEST_RESULT_("External function call. Basic types (more FP), arguments through stack. sx", testExternalCall4d, "1");

LOAD_MODULE_BIND_(test_ext5, "test.ext5", "int Call(char a, short b, int c, char d, short e, int f);")
{
	BIND_FUNCTION("test.ext5", TestExt5, "Call", 0);
}
const char	*TEST_CODE(testExternalCall5) =
"import test.ext5" MODULE_SUFFIX ";\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT_("External function call. Integer types (-long), arguments in registers. sx", testExternalCall5, "1");

LOAD_MODULE_BIND_(test_ext6, "test.ext6", "int Call(char a, short b, int c, long d, long e, int f);")
{
	BIND_FUNCTION("test.ext6", TestExt6, "Call", 0);
}
const char	*TEST_CODE(testExternalCall6) =
"import test.ext6" MODULE_SUFFIX ";\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT_("External function call. Integer types, arguments in registers. sx", testExternalCall6, "1");

LOAD_MODULE_BIND_(test_ext7, "test.ext7", "int Call(char a, short b, double c, double d, long e, int f);")
{
	BIND_FUNCTION("test.ext7", TestExt7, "Call", 0);
}
const char	*TEST_CODE(testExternalCall7) =
"import test.ext7" MODULE_SUFFIX ";\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT_("External function call. Integer types and double, arguments in registers. sx", testExternalCall7, "1");

LOAD_MODULE_BIND_(test_ext8, "test.ext8", "int Call(float a, float b, double c, double d, float e, float f);")
{
	BIND_FUNCTION("test.ext8", TestExt8, "Call", 0);
}
const char	*TEST_CODE(testExternalCall8) =
"import test.ext8" MODULE_SUFFIX ";\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT_("External function call. float and double, arguments in registers. sx", testExternalCall8, "1");

LOAD_MODULE_BIND_(test_ext8ex, "test.ext8ex", "int Call(float a, b, double c, d, float e, f, double g, h, float i, j);")
{
	BIND_FUNCTION("test.ext8ex", TestExt8_ex, "Call", 0);
}
const char	*TEST_CODE(testExternalCall8Ex) =
"import test.ext8ex" MODULE_SUFFIX ";\r\n\
return Call(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10);";
TEST_RESULT_("External function call. more float and double, registers. sx", testExternalCall8Ex, "1");

LOAD_MODULE_BIND_(test_ext9, "test.ext9", "int Call(char a, short b, int c, long d, float e, double f);")
{
	BIND_FUNCTION("test.ext9", TestExt9, "Call", 0);
}
const char	*TEST_CODE(testExternalCall9) =
"import test.ext9" MODULE_SUFFIX ";\r\n\
return Call(-1, -2, -3, -4, -5, -6);";
TEST_RESULT_("External function call. Basic types, arguments in registers. sx", testExternalCall9, "1");

LOAD_MODULE_BIND_(test_extA, "test.extA", "void ref GetPtr(int i); int Call(void ref a, int b, long c, void ref d);")
{
	BIND_FUNCTION("test.extA", TestGetPtr, "GetPtr", 0);
	BIND_FUNCTION("test.extA", TestExt10, "Call", 0);
}
const char	*TEST_CODE(testExternalCallA) =
"import test.extA" MODULE_SUFFIX ";\r\n\
return Call(GetPtr(1), -2, -3, GetPtr(4));";
TEST_RESULT_("External function call. Pointer w/o sign extend, arguments in registers. sx", testExternalCallA, "1");

LOAD_MODULE_BIND_(test_extB, "test.extB", "class Foo{} int Call(char a, Foo b, int c, Foo d, float e, double f);")
{
	BIND_FUNCTION("test.extB", TestExt11, "Call", 0);
}
const char	*TEST_CODE(testExternalCallB) =
"import test.extB" MODULE_SUFFIX ";\r\n\
Foo a, b;\r\n\
return Call(-1, a, -3, b, -5, -6);";
TEST_RESULT_("External function call. Class types sizeof() == 0, arguments in registers. sx", testExternalCallB, "1");

LOAD_MODULE_BIND_(test_extE, "test.extE", "int Call(int[] a);")
{
	BIND_FUNCTION("test.extE", TestExt14e, "Call", 0);
}
const char	*TEST_CODE(testExternalCalle) =
"import test.extE" MODULE_SUFFIX ";\r\n\
int[2] arr = { 1, 2 };\r\n\
return Call(arr);";
TEST_RESULT_("External function call. int[] argument in registers.", testExternalCalle, "3");

LOAD_MODULE_BIND_(test_extE2, "test.extE2", "int[] Call(int[] a);")
{
	BIND_FUNCTION("test.extE2", TestExt14, "Call", 0);
}
const char	*TEST_CODE(testExternalCallE) =
"import test.extE2" MODULE_SUFFIX ";\r\n\
int[2] arr = { 1, 0 };\r\n\
return Call(arr)[1];";
TEST_RESULT_("External function call. Complex return, arguments in registers.", testExternalCallE, "1");

LOAD_MODULE_BIND_(test_extC, "test.extC", "int Call(int[] a, int[] b, typeid u);")
{
	BIND_FUNCTION("test.extC", TestExt12, "Call", 0);
}
const char	*TEST_CODE(testExternalCallC) =
"import test.extC" MODULE_SUFFIX ";\r\n\
int[] arr = new int[2];\r\n\
arr[0] = 1; arr[1] = 2;\r\n\
return Call(arr, {3, 4}, int);";
TEST_RESULT_("External function call. Complex built-ins, arguments in registers.", testExternalCallC, "1");

LOAD_MODULE_BIND_(test_extC2, "test.extC2", "int Call(auto ref x);")
{
	BIND_FUNCTION("test.extC2", TestExtC2, "Call", 0);
}
const char *TEST_CODE(testExternalCallC2) = "import test.extC2" MODULE_SUFFIX "; return Call(3);";
TEST_RESULT_("External function call. auto ref argument.", testExternalCallC2, "1");

LOAD_MODULE_BIND_(test_extC3, "test.extC3", "auto ref Call(auto ref x);")
{
	BIND_FUNCTION("test.extC3", TestExtC3, "Call", 0);
}
const char *TEST_CODE(testExternalCallC3) = "import test.extC3" MODULE_SUFFIX "; return int(Call(3));";
TEST_RESULT_("External function call. auto ref return.", testExternalCallC3, "1");

LOAD_MODULE_BIND_(test_extD, "test.extD", "int[] Call(int[] a, int[] b, typeid u);")
{
	BIND_FUNCTION("test.extD", TestExt13, "Call", 0);
}
const char	*TEST_CODE(testExternalCallD) =
"import test.extD" MODULE_SUFFIX ";\r\n\
int[] arr = new int[2];\r\n\
arr[0] = 1; arr[1] = 2;\r\n\
return Call(arr, {3, 4}, int)[1];";
TEST_RESULT_("External function call. Complex built-in return, arguments in registers.", testExternalCallD, "1");

LOAD_MODULE_BIND_(test_extF, "test.extF",
	"class Char{ char a; } int Call(Char a);\r\n\
class Short{ short a; } int Call(Short a);\r\n\
class Int{ int a; } int Call(Int a);\r\n\
class Long{ long a; } int Call(Long a);\r\n\
class Float{ float a; } int Call(Float a);\r\n\
class Double{ double a; } int Call(Double a);")
{
	BIND_FUNCTION("test.extF", TestExtF1, "Call", 0);
	BIND_FUNCTION("test.extF", TestExtF2, "Call", 1);
	BIND_FUNCTION("test.extF", TestExtF3, "Call", 2);
	BIND_FUNCTION("test.extF", TestExtF4, "Call", 3);
	BIND_FUNCTION("test.extF", TestExtF5, "Call", 4);
	BIND_FUNCTION("test.extF", TestExtF6, "Call", 5);
}
const char *TEST_CODE(testExternalCallClass1) = "import test.extF" MODULE_SUFFIX "; Char x; x.a = -1; return Call(x);";
TEST_RESULT_("External function call. char inside a class, argument in registers.", testExternalCallClass1, "1");
const char *TEST_CODE(testExternalCallClass2) = "import test.extF" MODULE_SUFFIX "; Short x; x.a = -2; return Call(x);";
TEST_RESULT_("External function call. short inside a class, argument in registers.", testExternalCallClass2, "1");
const char *TEST_CODE(testExternalCallClass3) = "import test.extF" MODULE_SUFFIX "; Int x; x.a = -3; return Call(x);";
TEST_RESULT_("External function call. int inside a class, argument in registers.", testExternalCallClass3, "1");
const char *TEST_CODE(testExternalCallClass4) = "import test.extF" MODULE_SUFFIX "; Long x; x.a = -4; return Call(x);";
TEST_RESULT_("External function call. long inside a class, argument in registers.", testExternalCallClass4, "1");
const char *TEST_CODE(testExternalCallClass5) = "import test.extF" MODULE_SUFFIX "; Float x; x.a = -5; return Call(x);";
TEST_RESULT_("External function call. float inside a class, argument in registers.", testExternalCallClass5, "1");
const char *TEST_CODE(testExternalCallClass6) = "import test.extF" MODULE_SUFFIX "; Double x; x.a = -6; return Call(x);";
TEST_RESULT_("External function call. double inside a class, argument in registers.", testExternalCallClass6, "1");

LOAD_MODULE_BIND_(test_extG, "test.extG", "class Zomg{ long x,y,z; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG", TestExtG, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG) =
"import test.extG" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
return Call(z);";
TEST_RESULT_("External function call. Complex argument (24 bytes) in registers.", testExternalCallG, "1");

LOAD_MODULE_BIND_(test_extG2, "test.extG2", "class Zomg{ char x; int y; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG2", TestExtG2, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG2) =
"import test.extG2" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2;\r\n\
return Call(z);";
TEST_RESULT_("External function call. { char; int; } in argument.", testExternalCallG2, "1");

LOAD_MODULE_BIND_(test_extG3, "test.extG3", "class Zomg{ int x; char y; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG3", TestExtG3, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG3) =
"import test.extG3" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2;\r\n\
return Call(z);";
TEST_RESULT_("External function call. { int; char; } in argument.", testExternalCallG3, "1");

LOAD_MODULE_BIND_(test_extG4, "test.extG4", "class Zomg{ int x; char y; short z; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG4", TestExtG4, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG4) =
"import test.extG4" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
return Call(z);";
TEST_RESULT_("External function call. { int; char; short; } in argument.", testExternalCallG4, "1");

LOAD_MODULE_BIND_(test_extG5, "test.extG5", "class Zomg{ int x; short y; char z; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG5", TestExtG5, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG5) =
"import test.extG5" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
return Call(z);";
TEST_RESULT_("External function call. { int; short; char; } in argument.", testExternalCallG5, "1");

LOAD_MODULE_BIND_(test_extG6, "test.extG6", "class Zomg{ int x; int ref y; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG6", TestExtG6, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG6) =
"import test.extG6" MODULE_SUFFIX ";\r\n\
int u = -2;\r\n\
Zomg z; z.x = -1; z.y = &u;\r\n\
return Call(z);";
TEST_RESULT_("External function call. { int; int ref; } in argument.", testExternalCallG6, "1");

LOAD_MODULE_BIND_(test_extG7, "test.extG7", "class Zomg{ float x; double y; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG7", TestExtG7, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG7) =
"import test.extG7" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2;\r\n\
return Call(z);";
TEST_RESULT_("External function call. { float; double; } in argument.", testExternalCallG7, "1");

LOAD_MODULE_BIND_(test_extG8, "test.extG8", "class Zomg{ float x; float y; int z; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG8", TestExtG8, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG8) =
"import test.extG8" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
return Call(z);";
TEST_RESULT_("External function call. { float; float; int; } in argument.", testExternalCallG8, "1");

LOAD_MODULE_BIND_(test_extG9, "test.extG9", "class Zomg{ long x; float y; float z; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG9", TestExtG9, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG9) =
"import test.extG9" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
return Call(z);";
TEST_RESULT_("External function call. { long; float; float; } in argument.", testExternalCallG9, "1");

LOAD_MODULE_BIND_(test_extG10, "test.extG10", "class ZomgSub{ int x, y, z; } class Zomg{ ZomgSub x; float y; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG10", TestExtG10, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG10) =
"import test.extG10" MODULE_SUFFIX ";\r\n\
Zomg z; z.x.x = -1; z.x.y = -2; z.x.z = -3; z.y = -4;\r\n\
return Call(z);";
TEST_RESULT_("External function call. { { int; int; int; }; float; } in argument.", testExternalCallG10, "1");

LOAD_MODULE_BIND_(test_extG11, "test.extG11", "class Zomg{ int[] x; } int Call(Zomg a);")
{
	BIND_FUNCTION("test.extG11", TestExtG11, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG11) =
"import test.extG11" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = { 1, 2 };\r\n\
return Call(z);";
TEST_RESULT_("External function call. { int[]; } in argument.", testExternalCallG11, "1");

LOAD_MODULE_BIND_(test_extG12, "test.extG12", "class Zomg{ double a; double b; } int Call(float x, y, z, Zomg a, b, c);")
{
	BIND_FUNCTION("test.extG12", TestExtG12, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG12) =
"import test.extG12" MODULE_SUFFIX ";\r\n\
Zomg a; a.a = 4; a.b = 5;\r\n\
Zomg b; b.a = 6; b.b = 7;\r\n\
Zomg c; c.a = 8; c.b = 9;\r\n\
return Call(1, 2, 3, a, b, c);";
TEST_RESULT_("External function call. Floating point register overflow mid argument.", testExternalCallG12, "1");

LOAD_MODULE_BIND_(test_extG2b, "test.extG2b", "class Zomg{ char x; int y; } Zomg Call(Zomg a);")
{
	BIND_FUNCTION("test.extG2b", TestExtG2b, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG2b) =
"import test.extG2b" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT_("External function call. { char; int; } returned.", testExternalCallG2b, "1");

LOAD_MODULE_BIND_(test_extG3b, "test.extG3b", "class Zomg{ int x; char y; } Zomg Call(Zomg a);")
{
	BIND_FUNCTION("test.extG3b", TestExtG3b, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG3b) =
"import test.extG3b" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT_("External function call. { int; char; } returned.", testExternalCallG3b, "1");

LOAD_MODULE_BIND_(test_extG4b, "test.extG4b", "class Zomg{ int x; char y; short z; } Zomg Call(Zomg a);")
{
	BIND_FUNCTION("test.extG4b", TestExtG4b, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG4b) =
"import test.extG4b" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT_("External function call. { int; char; short; } returned.", testExternalCallG4b, "1");

LOAD_MODULE_BIND_(test_extG5b, "test.extG5b", "class Zomg{ int x; short y; char z; } Zomg Call(Zomg a);")
{
	BIND_FUNCTION("test.extG5b", TestExtG5b, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG5b) =
"import test.extG5b" MODULE_SUFFIX ";\r\n\
Zomg z; z.x = -1; z.y = -2; z.z = -3;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT_("External function call. { int; short; char; } returned.", testExternalCallG5b, "1");

LOAD_MODULE_BIND_(test_extG6b, "test.extG6b", "class Zomg{ int x; int ref y; } Zomg Call(Zomg a);")
{
	BIND_FUNCTION("test.extG6b", TestExtG6b, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG6b) =
"import test.extG6b" MODULE_SUFFIX ";\r\n\
int u = -2;\r\n\
Zomg z; z.x = -1; z.y = &u;\r\n\
z = Call(z);\r\n\
return z.x == 1;";
TEST_RESULT_("External function call. { int; int ref; } returned.", testExternalCallG6b, "1");

LOAD_MODULE_BIND_(test_extG7b, "test.extG7b", "class Zomg{ int x; } Zomg Call(auto ref a);")
{
	BIND_FUNCTION("test.extG7b", TestExtG7b, "Call", 0);
}
const char	*TEST_CODE(testExternalCallG7b) =
"import test.extG7b" MODULE_SUFFIX ";\r\n\
Zomg z;\r\n\
z = Call(new int(3));\r\n\
return z.x == 1;";
TEST_RESULT_("External function call. auto ref. { int; } returned.", testExternalCallG7b, "1");

LOAD_MODULE_BIND_(test_extK, "test.extK", "int Call(int ref a, b);")
{
	BIND_FUNCTION("test.extK", TestExtK, "Call", 0);
}
const char	*TEST_CODE(testExternalCallK) = "import test.extK" MODULE_SUFFIX ";\r\n int a = 1, b = 2;\r\n	return Call(&a, &b);";
TEST_RESULT_("External function call. References.", testExternalCallK, "1");

LOAD_MODULE_BIND_(test_extK2, "test.extK2", "int Call(auto ref a, int ref b);")
{
	BIND_FUNCTION("test.extK2", TestExtK2, "Call", 0);
}
const char	*TEST_CODE(testExternalCallK2) = "import test.extK2" MODULE_SUFFIX ";\r\n int a = 3, b = 2;\r\n	return Call(a, &b);";
TEST_RESULT_("External function call. auto ref and int ref.", testExternalCallK2, "1");

LOAD_MODULE_BIND_(test_extK3, "test.extK3", "auto ref Call(auto ref a, int ref b);")
{
	BIND_FUNCTION("test.extK3", TestExtK3, "Call", 0);
}
const char	*TEST_CODE(testExternalCallK3) = "import test.extK3" MODULE_SUFFIX ";\r\n int a = 3, b = 2;\r\nauto ref u = Call(a, &b); return int(u) == 30 && b == 20;";
TEST_RESULT_("External function call. auto ref and int ref, auto ref return.", testExternalCallK3, "1");

LOAD_MODULE_BIND_(test_extK4, "test.extK4", "int Call(auto ref x, y, z, w, a, b, c);")
{
	BIND_FUNCTION("test.extK4", TestExtK4, "Call", 0);
}
const char	*TEST_CODE(testExternalCallK4) = "import test.extK4" MODULE_SUFFIX ";\r\n return Call(1, 2, 3, 4, 5, 6, 7);";
TEST_RESULT_("External function call. a lot of auto ref parameters.", testExternalCallK4, "1");

LOAD_MODULE_BIND_(test_extK5, "test.extK5", "int Call(int[] x, y, z, int w);")
{
	BIND_FUNCTION("test.extK5", TestExtK5, "Call", 0);
}
const char	*TEST_CODE(testExternalCallK5) = "import test.extK5" MODULE_SUFFIX ";\r\n return Call({10}, {20,2}, {30,1,2}, 4);";
TEST_RESULT_("External function call. int[] fills registers and int on stack", testExternalCallK5, "1");

LOAD_MODULE_BIND_(test_extK6, "test.extK6", "int Call(int[] x, int w, int[] y, z);")
{
	BIND_FUNCTION("test.extK6", TestExtK6, "Call", 0);
}
const char	*TEST_CODE(testExternalCallK6) = "import test.extK6" MODULE_SUFFIX ";\r\n return Call({10}, 4, {20,2}, {30,1,2});";
TEST_RESULT_("External function call. Class divided between reg/stack (amd64)", testExternalCallK6, "1");

LOAD_MODULE_BIND_(test_extL, "test.extL", "void Call(int ref a);")
{
	BIND_FUNCTION("test.extL", TestExtL, "Call", 0);
}
const char	*TEST_CODE(testExternalCallL) = "import test.extL" MODULE_SUFFIX ";\r\n int a = 2;\r\nCall(&a); return a;";
TEST_RESULT_("External function call. void return type.", testExternalCallL, "1");

LOAD_MODULE_BIND_(test_extM1, "test.extM1", "int Call(double a, b, c, d);")
{
	BIND_FUNCTION("test.extM1", TestExtM1, "Call", 0);
}
const char	*TEST_CODE(testExternalCallM1) = "import test.extM1" MODULE_SUFFIX ";\r\n double a = 1.6; return Call(a, 2.0, 3.0, 4.0);";
TEST_RESULT_("External function call. alignment test 1.", testExternalCallM1, "1");

LOAD_MODULE_BIND_(test_extM2, "test.extM2", "int Call(double a, b, c, d, int e);")
{
	BIND_FUNCTION("test.extM2", TestExtM2, "Call", 0);
}
const char	*TEST_CODE(testExternalCallM2) = "import test.extM2" MODULE_SUFFIX ";\r\n double a = 1.6; return Call(a, 2.0, 3.0, 4.0, 5);";
TEST_RESULT_("External function call. alignment test 2.", testExternalCallM2, "1");

LOAD_MODULE_BIND_(test_extM3, "test.extM3", "class Foo{ int x; } int Foo:Call(double a, b, c, d);")
{
	BIND_FUNCTION("test.extM3", TestExtM3, "Foo::Call", 0);
}
const char	*TEST_CODE(testExternalCallM3) = "import test.extM3" MODULE_SUFFIX ";\r\n Foo m; m.x = 5; double a = 1.6; return m.Call(a, 2.0, 3.0, 4.0);";
TEST_RESULT_("External function call. alignment test 3.", testExternalCallM3, "1");

LOAD_MODULE_BIND_(test_extM4, "test.extM4", "int Call(double a, b, c, d, int e, f);")
{
	BIND_FUNCTION("test.extM4", TestExtM4, "Call", 0);
}
const char	*TEST_CODE(testExternalCallM4) = "import test.extM4" MODULE_SUFFIX ";\r\n double a = 1.6; return Call(a, 2.0, 3.0, 4.0, 5, 6);";
TEST_RESULT_("External function call. alignment test 4.", testExternalCallM4, "1");

// big argument tests
// big arguments with int and float/double

#if !defined(NULLC_LLVM_SUPPORT) || (ALL_EXTERNAL_CALLS == 1)

LOAD_MODULE_BIND_(test_big1, "test.big1", "class X{ int a, b, c, d; } X Call();")
{
	BIND_FUNCTION("test.big1", TestReturnBig1, "Call", 0);
}
const char	*TEST_CODE(testBigReturnType1) = "import test.big1" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 1 && x.b == 2 && x.c == 3 && x.d == 4;";
TEST_RESULT_("External function call. Big return type 1.", testBigReturnType1, "1");

LOAD_MODULE_BIND_(test_big2, "test.big2", "class X{ float a, b, c; } X Call();")
{
	BIND_FUNCTION("test.big2", TestReturnBig2, "Call", 0);
}
const char	*TEST_CODE(testBigReturnType2) = "import test.big2" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 1.0f && x.b == 2.0f && x.c == 3.0f;";
TEST_RESULT_("External function call. Big return type 2.", testBigReturnType2, "1");

LOAD_MODULE_BIND_(test_big3, "test.big3", "class X{ float a; double b; } X Call();")
{
	BIND_FUNCTION("test.big3", TestReturnBig3, "Call", 0);
}
const char	*TEST_CODE(testBigReturnType3) = "import test.big3" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 1.0f && x.b == 2.0;";
TEST_RESULT_("External function call. Big return type 3.", testBigReturnType3, "1");

LOAD_MODULE_BIND_(test_big4, "test.big4", "class X{ int a; float b; } X Call();")
{
	BIND_FUNCTION("test.big4", TestReturnBig4, "Call", 0);
}
const char	*TEST_CODE(testBigReturnType4) = "import test.big4" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 1 && x.b == 2.0;";
TEST_RESULT_("External function call. Big return type 4.", testBigReturnType4, "1");

LOAD_MODULE_BIND_(test_big5, "test.big5", "class X{ int a; double b; } X Call();")
{
	BIND_FUNCTION("test.big5", TestReturnBig5, "Call", 0);
}
const char	*TEST_CODE(testBigReturnType5) = "import test.big5" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 1 && x.b == 2.0;";
TEST_RESULT_("External function call. Big return type 5.", testBigReturnType5, "1");

LOAD_MODULE_BIND_(test_big6, "test.big6", "class X{ double a; int b; } X Call();")
{
	BIND_FUNCTION("test.big6", TestReturnBig6, "Call", 0);
}
const char	*TEST_CODE(testBigReturnType6) = "import test.big6" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 4 && x.b == 8.0;";
TEST_RESULT_("External function call. Big return type 6.", testBigReturnType6, "1");

LOAD_MODULE_BIND_(test_big7, "test.big7", "class X{ float a; int b; } X Call();")
{
	BIND_FUNCTION("test.big7", TestReturnBig7, "Call", 0);

}
const char	*TEST_CODE(testBigReturnType7) = "import test.big7" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 2 && x.b == 16.0;";
TEST_RESULT_("External function call. Big return type 7.", testBigReturnType7, "1");

LOAD_MODULE_BIND_(test_big8, "test.big8", "class X{ float a; float b; int c; } X Call();")
{
	BIND_FUNCTION("test.big8", TestReturnBig8, "Call", 0);
}
const char	*TEST_CODE(testBigReturnType8) = "import test.big8" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 4 && x.b == 8.0 && x.c == 16.0;";
TEST_RESULT_("External function call. Big return type 8.", testBigReturnType8, "1");

LOAD_MODULE_BIND_(test_small1, "test.small1", "class X{ float a; } X Call();")
{
	BIND_FUNCTION("test.small1", TestReturnSmall1, "Call", 0);
}
const char	*TEST_CODE(testSmallReturnType1) = "import test.small1" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 4.0f;";
TEST_RESULT_("External function call. Small return type 1.", testSmallReturnType1, "1");

LOAD_MODULE_BIND_(test_small2, "test.small2", "class X{ double a; } X Call();")
{
	BIND_FUNCTION("test.small2", TestReturnSmall2, "Call", 0);
}
const char	*TEST_CODE(testSmallReturnType2) = "import test.small2" MODULE_SUFFIX ";\r\n X x; x = Call(); return x.a == 6.0;";
TEST_RESULT_("External function call. Small return type 2.", testSmallReturnType2, "1");

LOAD_MODULE_BIND_(test_small3, "test.small3", "class X{} X Call();")
{
	BIND_FUNCTION("test.small3", TestReturnSmall3, "Call", 0);
}
const char	*TEST_CODE(testSmallReturnType3) = "import test.small3" MODULE_SUFFIX ";\r\n X x; x = Call(); return 1;";
TEST_RESULT_("External function call. Small return type 3.", testSmallReturnType3, "1");

#endif
