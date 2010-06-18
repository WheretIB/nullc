#include "runtime.h"
// Typeid redirect table
static unsigned __nullcTR[77];
// Array classes
typedef struct
{
	unsigned int classID;
	int pos;
} member_iterator;
typedef struct
{
	unsigned int type;
	NULLCArray<char > name;
} member_info;
typedef struct
{
	unsigned int funcID;
	int pos;
} argument_iterator;
int isFunction(unsigned int type, void* unused);
int isClass(unsigned int type, void* unused);
int isSimple(unsigned int type, void* unused);
int isArray(unsigned int type, void* unused);
int isPointer(unsigned int type, void* unused);
int isFunction(NULLCRef type, void* unused);
int isClass(NULLCRef type, void* unused);
int isSimple(NULLCRef type, void* unused);
int isArray(NULLCRef type, void* unused);
int isPointer(NULLCRef type, void* unused);
int typeid__size_(unsigned int * __context);
NULLCArray<char > typeid__name_(unsigned int * __context);
int typeid__memberCount(unsigned int * __context);
unsigned int typeid__memberType(int member, unsigned int * __context);
NULLCArray<char > typeid__memberName(int member, unsigned int * __context);
unsigned int typeid__subType(unsigned int * __context);
int typeid__arraySize(unsigned int * __context);
unsigned int typeid__returnType(unsigned int * __context);
int typeid__argumentCount(unsigned int * __context);
unsigned int typeid__argumentType(int argument, unsigned int * __context);
member_iterator typeid__members(unsigned int * __context);
member_iterator * member_iterator__start(member_iterator * __context);
int member_iterator__hasnext(member_iterator * __context);
member_info member_iterator__next(member_iterator * __context);
argument_iterator typeid__arguments(unsigned int * __context);
argument_iterator * argument_iterator__start(argument_iterator * __context);
int argument_iterator__hasnext(argument_iterator * __context);
unsigned int argument_iterator__next(argument_iterator * __context);
member_iterator  typeid__members(unsigned int * __context)
{
	member_iterator ret_4;
	*(&(&ret_4)->classID) = *(*(&__context));
	*(&(&ret_4)->pos) = 0;
	return *(&ret_4);
}
member_iterator *  member_iterator__start(member_iterator * __context)
{
	return *(&__context);
}
int  member_iterator__hasnext(member_iterator * __context)
{
	return (*(&(*(&__context))->pos)) < (typeid__memberCount(&(*(&__context))->classID));
}
member_info  member_iterator__next(member_iterator * __context)
{
	member_info ret_4;
	*(&(&ret_4)->type) = typeid__memberType(*(&(*(&__context))->pos), &(*(&__context))->classID);
	*(&(&ret_4)->name) = typeid__memberName((*(&(*(&__context))->pos))++, &(*(&__context))->classID);
	return *(&ret_4);
}
argument_iterator  typeid__arguments(unsigned int * __context)
{
	argument_iterator ret_4;
	*(&(&ret_4)->funcID) = *(*(&__context));
	*(&(&ret_4)->pos) = 0;
	return *(&ret_4);
}
argument_iterator *  argument_iterator__start(argument_iterator * __context)
{
	return *(&__context);
}
int  argument_iterator__hasnext(argument_iterator * __context)
{
	return (*(&(*(&__context))->pos)) < (typeid__argumentCount(&(*(&__context))->funcID));
}
unsigned int  argument_iterator__next(argument_iterator * __context)
{
	return typeid__argumentType((*(&(*(&__context))->pos))++, &(*(&__context))->funcID);
}
int initStdTypeInfo()
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
	__nullcTR[25] = __nullcRegisterType(554739849u, "char[] ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[26] = __nullcRegisterType(2528639597u, "void ref ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[27] = __nullcRegisterType(262756424u, "int[]", 8, __nullcTR[4], -1, NULLC_ARRAY);
	__nullcTR[28] = __nullcRegisterType(1780011448u, "int[] ref(int,int)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[29] = __nullcRegisterType(3724107199u, "auto ref ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[30] = __nullcRegisterType(3761170085u, "void ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[31] = __nullcRegisterType(4190091973u, "int[] ref", 4, __nullcTR[27], 1, NULLC_POINTER);
	__nullcTR[32] = __nullcRegisterType(844911189u, "void ref() ref(auto ref,int[] ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[33] = __nullcRegisterType(2671810221u, "int ref", 4, __nullcTR[4], 1, NULLC_POINTER);
	__nullcTR[34] = __nullcRegisterType(2816069557u, "char[] ref ref", 4, __nullcTR[17], 1, NULLC_POINTER);
	__nullcTR[35] = __nullcRegisterType(154671200u, "char ref", 4, __nullcTR[6], 1, NULLC_POINTER);
	__nullcTR[36] = __nullcRegisterType(2985493640u, "char[] ref ref(char[] ref,int[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[37] = __nullcRegisterType(2463794733u, "short[]", 8, __nullcTR[5], -1, NULLC_ARRAY);
	__nullcTR[38] = __nullcRegisterType(3958330154u, "short[] ref", 4, __nullcTR[37], 1, NULLC_POINTER);
	__nullcTR[39] = __nullcRegisterType(745297575u, "short[] ref ref", 4, __nullcTR[38], 1, NULLC_POINTER);
	__nullcTR[40] = __nullcRegisterType(3010777554u, "short ref", 4, __nullcTR[5], 1, NULLC_POINTER);
	__nullcTR[41] = __nullcRegisterType(1935747820u, "short[] ref ref(short[] ref,int[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[42] = __nullcRegisterType(3010510963u, "float[]", 8, __nullcTR[2], -1, NULLC_ARRAY);
	__nullcTR[43] = __nullcRegisterType(2248491120u, "float[] ref", 4, __nullcTR[42], 1, NULLC_POINTER);
	__nullcTR[44] = __nullcRegisterType(1040232248u, "double[]", 8, __nullcTR[1], -1, NULLC_ARRAY);
	__nullcTR[45] = __nullcRegisterType(2393077485u, "float[] ref ref", 4, __nullcTR[43], 1, NULLC_POINTER);
	__nullcTR[46] = __nullcRegisterType(2697529781u, "double[] ref", 4, __nullcTR[44], 1, NULLC_POINTER);
	__nullcTR[47] = __nullcRegisterType(1384297880u, "float ref", 4, __nullcTR[2], 1, NULLC_POINTER);
	__nullcTR[48] = __nullcRegisterType(3234425245u, "double ref", 4, __nullcTR[1], 1, NULLC_POINTER);
	__nullcTR[49] = __nullcRegisterType(2467461000u, "float[] ref ref(float[] ref,double[])", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[50] = __nullcRegisterType(2066091864u, "typeid ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[51] = __nullcRegisterType(1268871368u, "int ref(typeid,typeid)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[52] = __nullcRegisterType(2688408224u, "int ref(void ref(int),void ref(int))", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[53] = __nullcRegisterType(1908472638u, "int ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[54] = __nullcRegisterType(1872746701u, "int ref(typeid)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[55] = __nullcRegisterType(3335638996u, "int ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[56] = __nullcRegisterType(4231238349u, "typeid ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[57] = __nullcRegisterType(2745156404u, "char[] ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[58] = __nullcRegisterType(2501899970u, "typeid ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[59] = __nullcRegisterType(1748991366u, "member_iterator", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[60] = __nullcRegisterType(3319164712u, "member_info", 12, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[61] = __nullcRegisterType(3504529713u, "typeid ref", 4, __nullcTR[8], 1, NULLC_POINTER);
	__nullcTR[62] = __nullcRegisterType(1329745667u, "member_iterator ref", 4, __nullcTR[59], 1, NULLC_POINTER);
	__nullcTR[63] = __nullcRegisterType(1559597102u, "typeid ref ref", 4, __nullcTR[61], 1, NULLC_POINTER);
	__nullcTR[64] = __nullcRegisterType(689053972u, "member_iterator ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[65] = __nullcRegisterType(3055261440u, "member_iterator ref ref", 4, __nullcTR[62], 1, NULLC_POINTER);
	__nullcTR[66] = __nullcRegisterType(2875022417u, "member_iterator ref ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[67] = __nullcRegisterType(4163016061u, "member_iterator ref ref ref", 4, __nullcTR[65], 1, NULLC_POINTER);
	__nullcTR[68] = __nullcRegisterType(4051874520u, "auto ref ref", 4, __nullcTR[7], 1, NULLC_POINTER);
	__nullcTR[69] = __nullcRegisterType(2623357349u, "member_info ref", 4, __nullcTR[60], 1, NULLC_POINTER);
	__nullcTR[70] = __nullcRegisterType(682902582u, "member_info ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[71] = __nullcRegisterType(2249706641u, "argument_iterator", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[72] = __nullcRegisterType(795945870u, "argument_iterator ref", 4, __nullcTR[71], 1, NULLC_POINTER);
	__nullcTR[73] = __nullcRegisterType(3496627295u, "argument_iterator ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[74] = __nullcRegisterType(2485895435u, "argument_iterator ref ref", 4, __nullcTR[72], 1, NULLC_POINTER);
	__nullcTR[75] = __nullcRegisterType(1310733596u, "argument_iterator ref ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[76] = __nullcRegisterType(1459539208u, "argument_iterator ref ref ref", 4, __nullcTR[74], 1, NULLC_POINTER);
}
