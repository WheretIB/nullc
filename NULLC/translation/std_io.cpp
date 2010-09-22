#include "runtime.h"
// Typeid redirect table
static unsigned __nullcTR[130];
// Function pointer table
static __nullcFunctionArray* __nullcFM;
// Function pointer redirect table
static unsigned __nullcFR[97];
// Array classes
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
struct __typeProxy_void_ref_char___{};
struct __typeProxy_void_ref_int_int_{};
struct __typeProxy_void_ref_long_int_{};
struct __typeProxy_int_ref_char___{};
struct __typeProxy_void_ref_int_ref_{};
struct __typeProxy_void_ref_int_ref_int_ref_{};
struct StdOut 
{
};
struct StdEndline 
{
};
struct StdNonTerminatedTag 
{
};
struct StdBase 
{
	int base;
};
struct __typeProxy_char___ref_StdNonTerminatedTag_{};
struct int_2_ 
{
	int ptr[2];
	int_2_ & set(unsigned index, int const & val){ ptr[index] = val; return *this; }
};
struct __StdIO___func1_84_cls 
{
	void *x_ext;
	int_2_ x_dat;
};
struct __typeProxy_char___ref_StdNonTerminatedTag__ref_char___{};
struct __typeProxy_StdBase_ref_int_{};
struct StdIO 
{
	StdOut out;
	StdEndline endl;
	StdNonTerminatedTag non_terminated_tag;
	StdBase bin;
	StdBase oct;
	StdBase dec;
	StdBase hex;
	StdBase currBase;
};
struct __typeProxy_StdOut_ref_StdOut_StdBase_{};
struct __typeProxy_StdOut_ref_StdOut_char___ref_StdNonTerminatedTag__{};
struct __typeProxy_StdOut_ref_StdOut_char___{};
struct __typeProxy_StdOut_ref_StdOut_const_string_{};
struct __typeProxy_StdOut_ref_StdOut_StdEndline_{};
struct char_3_ 
{
	char ptr[4];
	char_3_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
	char_3_ (){}
	char_3_ (const char* data){ memcpy(ptr, data, 3); }
};
struct __typeProxy_StdOut_ref_StdOut_char_{};
struct __typeProxy_StdOut_ref_StdOut_int_{};
struct __typeProxy_StdOut_ref_StdOut_double_{};
struct __typeProxy_StdOut_ref_StdOut_long_{};
__nullcUpvalue *__upvalue_83_x_0 = 0;
__nullcUpvalue *__upvalue_83___context = 0;
void Print(NULLCArray<char > text, void* unused);
void Print(int num, int base, void* unused);
void Print(double num, void* unused);
void Print(long long num, int base, void* unused);
void Print(char ch, void* unused);
int Input(NULLCArray<char > buf, void* unused);
void Input(int * num, void* unused);
void Write(NULLCArray<char > buf, void* unused);
void SetConsoleCursorPos(int x, int y, void* unused);
void GetKeyboardState(NULLCArray<char > state, void* unused);
void GetMouseState(int * x, int * y, void* unused);
NULLCFuncPtr<__typeProxy_char___ref_StdNonTerminatedTag_> StdIO__non_terminated_char___ref_StdNonTerminatedTag__ref_char___(NULLCArray<char > x, StdIO * __context);
NULLCArray<char > StdIO___func1_84(StdNonTerminatedTag y, void* __StdIO___func1_84_ext);
StdBase StdIO__base_StdBase_ref_int_(int base, StdIO * __context);
StdOut __operatorShiftLeft(StdOut out, StdBase base, void* unused);
StdOut __operatorShiftLeft(StdOut out, NULLCFuncPtr<__typeProxy_char___ref_StdNonTerminatedTag_> wrapper, void* unused);
StdOut __operatorShiftLeft(StdOut out, NULLCArray<char > str, void* unused);
StdOut __operatorShiftLeft(StdOut out, const_string str, void* unused);
StdOut __operatorShiftLeft(StdOut out, StdEndline str, void* unused);
StdOut __operatorShiftLeft(StdOut out, char ch, void* unused);
StdOut __operatorShiftLeft(StdOut out, int num, void* unused);
StdOut __operatorShiftLeft(StdOut out, double num, void* unused);
StdOut __operatorShiftLeft(StdOut out, long long num, void* unused);
int __Print_base_73(void* unused);
int __Print_base_75(void* unused);
StdIO io;
NULLCArray<char >  StdIO___func1_84(StdNonTerminatedTag y_0, void* __StdIO___func1_84_ext_4)
{
	return (*((NULLCArray<char > * )((__nullcUpvalue*)((char*)__StdIO___func1_84_ext_4 + 0))->ptr));
}
NULLCFuncPtr<__typeProxy_char___ref_StdNonTerminatedTag_>  StdIO__non_terminated_char___ref_StdNonTerminatedTag__ref_char___(NULLCArray<char > x_0, StdIO * __context)
{
	__StdIO___func1_84_cls * __StdIO___func1_84_ext_12;
	NULLCFuncPtr<__typeProxy_char___ref_StdNonTerminatedTag_> __nullcRetVar0 = (	(__StdIO___func1_84_ext_12 = (__StdIO___func1_84_cls * )__newS(12, __nullcTR[109])),
	(((int**)__StdIO___func1_84_ext_12)[0] = (int*)&x_0),
	(((int**)__StdIO___func1_84_ext_12)[1] = (int*)__upvalue_83_x_0),
	(((int*)__StdIO___func1_84_ext_12)[2] = 8),
	(__upvalue_83_x_0 = (__nullcUpvalue*)((int*)__StdIO___func1_84_ext_12 + 0)),
	(NULLCFuncPtr<__typeProxy_char___ref_StdNonTerminatedTag_> )__nullcMakeFunction(__nullcFR[84], __StdIO___func1_84_ext_12));
	__nullcCloseUpvalue(__upvalue_83_x_0, &x_0);
	return __nullcRetVar0;
	__nullcCloseUpvalue(__upvalue_83_x_0, &x_0);
}
StdBase  StdIO__base_StdBase_ref_int_(int base_0, StdIO * __context)
{
	StdBase n_8;
	assert(((*(&base_0)) > (1)) && ((*(&base_0)) <= (16)), (void*)0);
	*(&(&n_8)->base) = *(&base_0);
	return *(&n_8);
}
StdOut  __operatorShiftLeft(StdOut out_0, StdBase base_4, void* unused)
{
	*(&(&io)->currBase) = *(&base_4);
	return *(&out_0);
}
StdOut  __operatorShiftLeft(StdOut out_0, NULLCFuncPtr<__typeProxy_char___ref_StdNonTerminatedTag_> wrapper_4, void* unused)
{
	Write(((NULLCArray<char > (*)(StdNonTerminatedTag , void*))(*__nullcFM)[(*(&wrapper_4)).id])(*(&(&io)->non_terminated_tag), (*(&wrapper_4)).context), (void*)0);
	return *(&out_0);
}
StdOut  __operatorShiftLeft(StdOut out_0, NULLCArray<char > str_4, void* unused)
{
	Print(*(&str_4), (void*)0);
	return *(&out_0);
}
StdOut  __operatorShiftLeft(StdOut out_0, const_string str_4, void* unused)
{
	Print(*(&(&str_4)->arr), (void*)0);
	return *(&out_0);
}
StdOut  __operatorShiftLeft(StdOut out_0, StdEndline str_4, void* unused)
{
	char_3_ __temp1_12;
	Print((memcpy((&__temp1_12)->ptr, "\xd\xa\x0\x0", 4), __makeNullcArray<char >(&__temp1_12, 3)), (void*)0);
	return *(&out_0);
}
StdOut  __operatorShiftLeft(StdOut out_0, char ch_4, void* unused)
{
	Print(*(&ch_4), (void*)0);
	return *(&out_0);
}
StdOut  __operatorShiftLeft(StdOut out_0, int num_4, void* unused)
{
	Print(*(&num_4), *(&(&(&io)->currBase)->base), (void*)0);
	return *(&out_0);
}
StdOut  __operatorShiftLeft(StdOut out_0, double num_4, void* unused)
{
	Print(*(&num_4), (void*)0);
	return *(&out_0);
}
StdOut  __operatorShiftLeft(StdOut out_0, long long num_4, void* unused)
{
	Print(*(&num_4), *(&(&(&io)->currBase)->base), (void*)0);
	return *(&out_0);
}
int  __Print_base_73(void* unused)
{
	return 10;
}
int  __Print_base_75(void* unused)
{
	return 10;
}
int __init_std_io_nc()
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
	__nullcTR[43] = __nullcRegisterType(2745156404u, "char[] ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[44] = __nullcRegisterType(2528639597u, "void ref ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[45] = __nullcRegisterType(262756424u, "int[]", 8, __nullcTR[4], -1, NULLC_ARRAY);
	__nullcTR[46] = __nullcRegisterType(1780011448u, "int[] ref(int,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[47] = __nullcRegisterType(3724107199u, "auto ref ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[48] = __nullcRegisterType(1137649267u, "auto[] ref", 4, __nullcTR[10], 1, NULLC_POINTER);
	__nullcTR[49] = __nullcRegisterType(665024592u, "void ref(auto[] ref,auto[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[50] = __nullcRegisterType(2649967381u, "auto[] ref(auto[])", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[51] = __nullcRegisterType(2430896065u, "auto ref ref(auto ref,auto ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[52] = __nullcRegisterType(3364104125u, "void ref(auto ref,auto ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[53] = __nullcRegisterType(4131070326u, "int ref(auto ref,auto ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[54] = __nullcRegisterType(3761170085u, "void ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[55] = __nullcRegisterType(4190091973u, "int[] ref", 4, __nullcTR[45], 1, NULLC_POINTER);
	__nullcTR[56] = __nullcRegisterType(844911189u, "void ref() ref(auto ref,int[] ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[57] = __nullcRegisterType(2816069557u, "char[] ref ref", 4, __nullcTR[17], 1, NULLC_POINTER);
	__nullcTR[58] = __nullcRegisterType(2985493640u, "char[] ref ref(char[] ref,int[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[59] = __nullcRegisterType(2463794733u, "short[]", 8, __nullcTR[5], -1, NULLC_ARRAY);
	__nullcTR[60] = __nullcRegisterType(3958330154u, "short[] ref", 4, __nullcTR[59], 1, NULLC_POINTER);
	__nullcTR[61] = __nullcRegisterType(745297575u, "short[] ref ref", 4, __nullcTR[60], 1, NULLC_POINTER);
	__nullcTR[62] = __nullcRegisterType(1935747820u, "short[] ref ref(short[] ref,int[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[63] = __nullcRegisterType(3010510963u, "float[]", 8, __nullcTR[2], -1, NULLC_ARRAY);
	__nullcTR[64] = __nullcRegisterType(2248491120u, "float[] ref", 4, __nullcTR[63], 1, NULLC_POINTER);
	__nullcTR[65] = __nullcRegisterType(1040232248u, "double[]", 8, __nullcTR[1], -1, NULLC_ARRAY);
	__nullcTR[66] = __nullcRegisterType(2393077485u, "float[] ref ref", 4, __nullcTR[64], 1, NULLC_POINTER);
	__nullcTR[67] = __nullcRegisterType(2697529781u, "double[] ref", 4, __nullcTR[65], 1, NULLC_POINTER);
	__nullcTR[68] = __nullcRegisterType(2467461000u, "float[] ref ref(float[] ref,double[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[69] = __nullcRegisterType(2066091864u, "typeid ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[70] = __nullcRegisterType(1908472638u, "int ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[71] = __nullcRegisterType(1268871368u, "int ref(typeid,typeid)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[72] = __nullcRegisterType(2688408224u, "int ref(void ref(int),void ref(int))", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[73] = __nullcRegisterType(451037873u, "auto[] ref ref(auto[] ref,auto ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[74] = __nullcRegisterType(3824954777u, "auto ref ref(auto ref,auto[] ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[75] = __nullcRegisterType(3832966281u, "auto[] ref ref(auto[] ref,auto[] ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[76] = __nullcRegisterType(477490926u, "auto ref ref(auto[] ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[77] = __nullcRegisterType(3720955042u, "const_string", 8, __nullcTR[0], 1, NULLC_CLASS);
	__nullcTR[78] = __nullcRegisterType(1951548447u, "const_string ref", 4, __nullcTR[77], 1, NULLC_POINTER);
	__nullcTR[79] = __nullcRegisterType(504936988u, "const_string ref ref", 4, __nullcTR[78], 1, NULLC_POINTER);
	__nullcTR[80] = __nullcRegisterType(3099572969u, "const_string ref ref(const_string ref,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[81] = __nullcRegisterType(2016055718u, "const_string ref(char[])", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[82] = __nullcRegisterType(195623906u, "char ref(const_string ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[83] = __nullcRegisterType(3956827940u, "int ref(const_string,const_string)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[84] = __nullcRegisterType(3539544093u, "int ref(const_string,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[85] = __nullcRegisterType(730969981u, "int ref(char[],const_string)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[86] = __nullcRegisterType(727035222u, "const_string ref(const_string,const_string)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[87] = __nullcRegisterType(1003630799u, "const_string ref(const_string,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[88] = __nullcRegisterType(2490023983u, "const_string ref(char[],const_string)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[89] = __nullcRegisterType(3335638996u, "int ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[90] = __nullcRegisterType(4064539429u, "void ref(auto[] ref,typeid,int)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcTR[91] = __nullcRegisterType(3726221418u, "auto[] ref(typeid,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[92] = __nullcRegisterType(3504529713u, "typeid ref", 4, __nullcTR[8], 1, NULLC_POINTER);
	__nullcTR[93] = __nullcRegisterType(527603730u, "void ref(auto ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[94] = __nullcRegisterType(4086116458u, "void ref(auto[] ref,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[95] = __nullcRegisterType(1812738619u, "void ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[96] = __nullcRegisterType(2767335131u, "void ref(char[])", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[97] = __nullcRegisterType(3255288871u, "void ref(int,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[98] = __nullcRegisterType(69713196u, "void ref(long,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[99] = __nullcRegisterType(709988916u, "int ref(char[])", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[100] = __nullcRegisterType(3241370989u, "void ref(int ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[101] = __nullcRegisterType(1591343073u, "void ref(int ref,int ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[102] = __nullcRegisterType(3521544424u, "StdOut", 0, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[103] = __nullcRegisterType(1586345743u, "StdEndline", 0, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[104] = __nullcRegisterType(3327808132u, "StdNonTerminatedTag", 0, __nullcTR[0], 0, NULLC_CLASS);
	__nullcTR[105] = __nullcRegisterType(246360107u, "StdBase", 4, __nullcTR[0], 1, NULLC_CLASS);
	__nullcTR[106] = __nullcRegisterType(236863752u, "StdIO", 20, __nullcTR[0], 8, NULLC_CLASS);
	__nullcTR[107] = __nullcRegisterType(2247820165u, "StdIO ref", 4, __nullcTR[106], 1, NULLC_POINTER);
	__nullcTR[108] = __nullcRegisterType(2550674952u, "char[] ref(StdNonTerminatedTag)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[109] = __nullcRegisterType(180207047u, "__StdIO::$func1_84_cls", 12, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[110] = __nullcRegisterType(81026074u, "int[2]", 8, __nullcTR[4], 2, NULLC_ARRAY);
	__nullcTR[111] = __nullcRegisterType(2339948484u, "__StdIO::$func1_84_cls ref", 4, __nullcTR[109], 1, NULLC_POINTER);
	__nullcTR[112] = __nullcRegisterType(2792523841u, "__StdIO::$func1_84_cls ref ref", 4, __nullcTR[111], 1, NULLC_POINTER);
	__nullcTR[113] = __nullcRegisterType(2537081868u, "char[] ref(StdNonTerminatedTag) ref(char[])", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[114] = __nullcRegisterType(2770388008u, "StdBase ref", 4, __nullcTR[105], 1, NULLC_POINTER);
	__nullcTR[115] = __nullcRegisterType(1821960228u, "StdBase ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[116] = __nullcRegisterType(231959531u, "StdOut ref(StdOut,StdBase)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[117] = __nullcRegisterType(1906324325u, "StdOut ref", 4, __nullcTR[102], 1, NULLC_POINTER);
	__nullcTR[118] = __nullcRegisterType(211946088u, "StdOut ref(StdOut,char[] ref(StdNonTerminatedTag))", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[119] = __nullcRegisterType(949711617u, "StdNonTerminatedTag ref", 4, __nullcTR[104], 1, NULLC_POINTER);
	__nullcTR[120] = __nullcRegisterType(769095813u, "char[] ref(StdNonTerminatedTag) ref", 4, __nullcTR[108], 1, NULLC_POINTER);
	__nullcTR[121] = __nullcRegisterType(1457376283u, "StdOut ref(StdOut,char[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[122] = __nullcRegisterType(2142474658u, "StdOut ref(StdOut,const_string)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[123] = __nullcRegisterType(1732329455u, "StdOut ref(StdOut,StdEndline)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[124] = __nullcRegisterType(3258512270u, "char[3]", 4, __nullcTR[6], 3, NULLC_ARRAY);
	__nullcTR[125] = __nullcRegisterType(1435994379u, "char[3] ref", 4, __nullcTR[124], 1, NULLC_POINTER);
	__nullcTR[126] = __nullcRegisterType(3306372739u, "StdOut ref(StdOut,char)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[127] = __nullcRegisterType(2182824208u, "StdOut ref(StdOut,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[128] = __nullcRegisterType(3045954208u, "StdOut ref(StdOut,double)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[129] = __nullcRegisterType(3317311381u, "StdOut ref(StdOut,long)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcRegisterMembers(__nullcTR[7], 2, __nullcTR[8], 0, __nullcTR[9], 4);
	__nullcRegisterMembers(__nullcTR[8], 0);
	__nullcRegisterMembers(__nullcTR[10], 3, __nullcTR[8], 0, __nullcTR[9], 4, __nullcTR[4], 8);
	__nullcRegisterMembers(__nullcTR[77], 1, __nullcTR[13], 0);
	__nullcRegisterMembers(__nullcTR[102], 0);
	__nullcRegisterMembers(__nullcTR[103], 0);
	__nullcRegisterMembers(__nullcTR[104], 0);
	__nullcRegisterMembers(__nullcTR[105], 1, __nullcTR[4], 0);
	__nullcRegisterMembers(__nullcTR[106], 8, __nullcTR[102], 0, __nullcTR[103], 0, __nullcTR[104], 0, __nullcTR[105], 0, __nullcTR[105], 4, __nullcTR[105], 8, __nullcTR[105], 12, __nullcTR[105], 16);
	__nullcRegisterMembers(__nullcTR[109], 2, __nullcTR[17], 0, __nullcTR[110], 4);
	__nullcRegisterGlobal((void*)&io, __nullcTR[106]);
	__nullcFR[0] = 0;
	__nullcFR[1] = 0;
	__nullcFR[2] = 0;
	__nullcFR[3] = 0;
	__nullcFR[4] = 0;
	__nullcFR[5] = __nullcRegisterFunction("__operatorAddSet", (void*)__operatorAddSet, 4294967295u, 0);
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
	__nullcFR[72] = 0;
	__nullcFR[73] = 0;
	__nullcFR[74] = 0;
	__nullcFR[75] = 0;
	__nullcFR[76] = 0;
	__nullcFR[77] = 0;
	__nullcFR[78] = 0;
	__nullcFR[79] = __nullcRegisterFunction("Write", (void*)Write, 4294967295u, 0);
	__nullcFR[80] = __nullcRegisterFunction("SetConsoleCursorPos", (void*)SetConsoleCursorPos, 4294967295u, 0);
	__nullcFR[81] = __nullcRegisterFunction("GetKeyboardState", (void*)GetKeyboardState, 4294967295u, 0);
	__nullcFR[82] = __nullcRegisterFunction("GetMouseState", (void*)GetMouseState, 4294967295u, 0);
	__nullcFR[83] = __nullcRegisterFunction("StdIO__non_terminated_char___ref_StdNonTerminatedTag__ref_char___", (void*)StdIO__non_terminated_char___ref_StdNonTerminatedTag__ref_char___, 107u, 2);
	__nullcFR[84] = __nullcRegisterFunction("StdIO___func1_84", (void*)StdIO___func1_84, 111u, 1);
	__nullcFR[85] = __nullcRegisterFunction("StdIO__base_StdBase_ref_int_", (void*)StdIO__base_StdBase_ref_int_, 107u, 2);
	__nullcFR[86] = 0;
	__nullcFR[87] = 0;
	__nullcFR[88] = 0;
	__nullcFR[89] = 0;
	__nullcFR[90] = 0;
	__nullcFR[91] = 0;
	__nullcFR[92] = 0;
	__nullcFR[93] = 0;
	__nullcFR[94] = 0;
	__nullcFR[95] = __nullcRegisterFunction("__Print_base_73", (void*)__Print_base_73, 31u, 0);
	__nullcFR[96] = __nullcRegisterFunction("__Print_base_75", (void*)__Print_base_75, 31u, 0);
	/* node translation unknown */
	*(&(&(&io)->bin)->base) = 2;
	*(&(&(&io)->oct)->base) = 8;
	*(&(&(&io)->dec)->base) = 10;
	*(&(&(&io)->hex)->base) = 16;
	*(&(&io)->currBase) = *(&(&io)->dec);
}
