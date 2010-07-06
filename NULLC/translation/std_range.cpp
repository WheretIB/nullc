#include "runtime.h"
// Typeid redirect table
static unsigned __nullcTR[90];
// Function pointer redirect table
static void* __nullcFR[60];
// Array classes
struct range_iterator 
{
	int pos;
	int max;
	int step;
};
range_iterator range_iterator__start_range_iterator_ref__(range_iterator * __context);
int range_iterator__next_int_ref__(range_iterator * __context);
int range_iterator__hasnext_int_ref__(range_iterator * __context);
range_iterator range(int min, int max, int step, void* unused);
int __range_step_58(void* unused);
range_iterator  range_iterator__start_range_iterator_ref__(range_iterator * __context)
{
	return *(*(&__context));
}
int  range_iterator__next_int_ref__(range_iterator * __context)
{
	*(&(*(&__context))->pos) += *(&(*(&__context))->step);
	return (*(&(*(&__context))->pos)) - (*(&(*(&__context))->step));
}
int  range_iterator__hasnext_int_ref__(range_iterator * __context)
{
	return (*(&(*(&__context))->pos)) <= (*(&(*(&__context))->max));
}
range_iterator  range(int min_0, int max_4, int step_8, void* unused)
{
	range_iterator r_16;
	*(&(&r_16)->pos) = *(&min_0);
	*(&(&r_16)->max) = (*(&max_4)) + (1);
	*(&(&r_16)->step) = *(&step_8);
	return *(&r_16);
}
int  __range_step_58(void* unused)
{
	return 1;
}
int __init_std_range_nc()
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
	__nullcTR[84] = __nullcRegisterType(3246223675u, "range_iterator", 12, __nullcTR[0], 3, NULLC_CLASS);
	__nullcTR[85] = __nullcRegisterType(957158712u, "range_iterator ref", 4, __nullcTR[84], 1, NULLC_POINTER);
	__nullcTR[86] = __nullcRegisterType(2006478773u, "range_iterator ref ref", 4, __nullcTR[85], 1, NULLC_POINTER);
	__nullcTR[87] = __nullcRegisterType(2963753097u, "range_iterator ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[88] = __nullcRegisterType(1763098802u, "range_iterator ref ref ref", 4, __nullcTR[86], 1, NULLC_POINTER);
	__nullcTR[89] = __nullcRegisterType(2954242914u, "range_iterator ref(int,int,int)", 8, __nullcTR[0], 3, NULLC_FUNCTION);
	__nullcFR[0] = (void*)0;
	__nullcFR[1] = (void*)0;
	__nullcFR[2] = (void*)0;
	__nullcFR[3] = (void*)0;
	__nullcFR[4] = (void*)0;
	__nullcFR[5] = (void*)__operatorAddSet;
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
	__nullcFR[55] = (void*)range_iterator__start_range_iterator_ref__;
	__nullcFR[56] = (void*)range_iterator__next_int_ref__;
	__nullcFR[57] = (void*)range_iterator__hasnext_int_ref__;
	__nullcFR[58] = (void*)range;
	__nullcFR[59] = (void*)__range_step_58;
}
