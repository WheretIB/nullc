#include "runtime.h"
// Typeid redirect table
static unsigned __nullcTR[138];
// Function pointer table
static __nullcFunctionArray* __nullcFM;
// Function pointer redirect table
static unsigned __nullcFR[100];
// Array classes
struct member_iterator 
{
	unsigned int classID;
	int pos;
};
struct member_info 
{
	unsigned int type;
	NULLCArray<char > name;
};
struct argument_iterator 
{
	unsigned int funcID;
	int pos;
};
struct list_node 
{
	list_node * prev;
	list_node * next;
	NULLCRef elem;
};
struct list 
{
	unsigned int elemType;
	int anyType;
	list_node * first;
	list_node * last;
};
struct char_32_ 
{
	char ptr[32];
	char_32_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
};
struct char_35_ 
{
	char ptr[36];
	char_35_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
};
struct char_2_ 
{
	char ptr[4];
	char_2_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
};
struct char_33_ 
{
	char ptr[36];
	char_33_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
};
struct char_29_ 
{
	char ptr[32];
	char_29_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
};
struct list_iterator 
{
	list_node * curr;
};
extern int isFunction(unsigned int type, void* unused);
extern int isClass(unsigned int type, void* unused);
extern int isSimple(unsigned int type, void* unused);
extern int isArray(unsigned int type, void* unused);
extern int isPointer(unsigned int type, void* unused);
extern int isFunction(NULLCRef type, void* unused);
extern int isClass(NULLCRef type, void* unused);
extern int isSimple(NULLCRef type, void* unused);
extern int isArray(NULLCRef type, void* unused);
extern int isPointer(NULLCRef type, void* unused);
extern int typeid__size__int_ref__(unsigned int * __context);
extern NULLCArray<char > typeid__name__char___ref__(unsigned int * __context);
extern int typeid__memberCount_int_ref__(unsigned int * __context);
extern unsigned int typeid__memberType_typeid_ref_int_(int member, unsigned int * __context);
extern NULLCArray<char > typeid__memberName_char___ref_int_(int member, unsigned int * __context);
extern unsigned int typeid__subType_typeid_ref__(unsigned int * __context);
extern int typeid__arraySize_int_ref__(unsigned int * __context);
extern unsigned int typeid__returnType_typeid_ref__(unsigned int * __context);
extern int typeid__argumentCount_int_ref__(unsigned int * __context);
extern unsigned int typeid__argumentType_typeid_ref_int_(int argument, unsigned int * __context);
extern member_iterator typeid__members_member_iterator_ref__(unsigned int * __context);
extern member_iterator * member_iterator__start_member_iterator_ref_ref__(member_iterator * __context);
extern int member_iterator__hasnext_int_ref__(member_iterator * __context);
extern member_info member_iterator__next_member_info_ref__(member_iterator * __context);
extern argument_iterator typeid__arguments_argument_iterator_ref__(unsigned int * __context);
extern argument_iterator * argument_iterator__start_argument_iterator_ref_ref__(argument_iterator * __context);
extern int argument_iterator__hasnext_int_ref__(argument_iterator * __context);
extern unsigned int argument_iterator__next_typeid_ref__(argument_iterator * __context);
NULLCRef list_node__value__auto_ref_ref__(list_node * __context);
list list__(void* unused);
list list__(unsigned int type, void* unused);
void list__push_back_void_ref_auto_ref_(NULLCRef elem, list * __context);
void list__push_front_void_ref_auto_ref_(NULLCRef elem, list * __context);
void list__insert_void_ref_list_node_ref_auto_ref_(list_node * it, NULLCRef elem, list * __context);
void list__erase_void_ref_list_node_ref_(list_node * it, list * __context);
void list__clear_void_ref__(list * __context);
NULLCRef list__back_auto_ref_ref__(list * __context);
NULLCRef list__front_auto_ref_ref__(list * __context);
list_node * list__begin_list_node_ref_ref__(list * __context);
list_node * list__end_list_node_ref_ref__(list * __context);
int list__empty_int_ref__(list * __context);
list_iterator list_iterator__(list_node * start, void* unused);
list_iterator list__start_list_iterator_ref__(list * __context);
NULLCRef list_iterator__next_auto_ref_ref__(list_iterator * __context);
int list_iterator__hasnext_int_ref__(list_iterator * __context);
NULLCRef  list_node__value__auto_ref_ref__(list_node * __context)
{
	return *(&(*(&__context))->elem);
}
list  list__(void* unused)
{
	list ret_4;
	*(&(&ret_4)->anyType) = 1;
	*(&(&ret_4)->elemType) = 7;
	*(&(&ret_4)->first) = *(&(&ret_4)->last) = (list_node * )(0);
	return *(&ret_4);
}
list  list__(unsigned int type_0, void* unused)
{
	list ret_8;
	*(&(&ret_8)->anyType) = __operatorEqual(*(&type_0), 7, (void*)0);
	*(&(&ret_8)->elemType) = *(&type_0);
	*(&(&ret_8)->first) = *(&(&ret_8)->last) = (list_node * )(0);
	return *(&ret_8);
}
void  list__push_back_void_ref_auto_ref_(NULLCRef elem_0, list * __context)
{
	unsigned int __temp1_12;
	char_32_ __temp2_16;
	char_35_ __temp3_48;
	char_2_ __temp4_84;
	if((!*(&(*(&__context))->anyType)) && (__operatorNEqual(typeid__(*(&elem_0), (void*)0), isPointer(*(&(*(&__context))->elemType), (void*)0) ? typeid__subType_typeid_ref__(&(*(&__context))->elemType) : *(&(*(&__context))->elemType), (void*)0)))
	{
		assert(0, __operatorAdd(__operatorAdd(__operatorAdd(__operatorAdd((memcpy((&__temp2_16)->ptr, "list::push_back argument type (\x0", 32), __makeNullcArray<char >(&__temp2_16, 32)), (*(&__temp1_12) = typeid__(*(&elem_0), (void*)0), typeid__name__char___ref__(&__temp1_12)), (void*)0), (memcpy((&__temp3_48)->ptr, ") differs from list element type (\x0\x0", 36), __makeNullcArray<char >(&__temp3_48, 35)), (void*)0), typeid__name__char___ref__(&(*(&__context))->elemType), (void*)0), (memcpy((&__temp4_84)->ptr, ")\x0\x0\x0", 4), __makeNullcArray<char >(&__temp4_84, 2)), (void*)0), (void*)0);
	}
	if(!*(&(*(&__context))->first))
	{
		*(&(*(&__context))->first) = *(&(*(&__context))->last) = (list_node * )__newS(16, __nullcTR[107]);
		*(&(*(&(*(&__context))->first))->prev) = *(&(*(&(*(&__context))->first))->next) = (list_node * )(0);
		*(&(*(&(*(&__context))->first))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_0) : duplicate(*(&elem_0), (void*)0);
	}else{
		*(&(*(&(*(&__context))->last))->next) = (list_node * )__newS(16, __nullcTR[107]);
		*(&(*(&(*(&(*(&__context))->last))->next))->prev) = *(&(*(&__context))->last);
		*(&(*(&(*(&(*(&__context))->last))->next))->next) = (list_node * )(0);
		*(&(*(&__context))->last) = *(&(*(&(*(&__context))->last))->next);
		*(&(*(&(*(&__context))->last))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_0) : duplicate(*(&elem_0), (void*)0);
	}
}
void  list__push_front_void_ref_auto_ref_(NULLCRef elem_0, list * __context)
{
	unsigned int __temp5_12;
	char_33_ __temp6_16;
	char_35_ __temp7_52;
	char_2_ __temp8_88;
	if((!*(&(*(&__context))->anyType)) && (__operatorNEqual(typeid__(*(&elem_0), (void*)0), *(&(*(&__context))->elemType), (void*)0)))
	{
		assert(0, __operatorAdd(__operatorAdd(__operatorAdd(__operatorAdd((memcpy((&__temp6_16)->ptr, "list::push_front argument type (\x0\x0\x0\x0", 36), __makeNullcArray<char >(&__temp6_16, 33)), (*(&__temp5_12) = typeid__(*(&elem_0), (void*)0), typeid__name__char___ref__(&__temp5_12)), (void*)0), (memcpy((&__temp7_52)->ptr, ") differs from list element type (\x0\x0", 36), __makeNullcArray<char >(&__temp7_52, 35)), (void*)0), typeid__name__char___ref__(&(*(&__context))->elemType), (void*)0), (memcpy((&__temp8_88)->ptr, ")\x0\x0\x0", 4), __makeNullcArray<char >(&__temp8_88, 2)), (void*)0), (void*)0);
	}
	if(!*(&(*(&__context))->first))
	{
		*(&(*(&__context))->first) = *(&(*(&__context))->last) = (list_node * )__newS(16, __nullcTR[107]);
		*(&(*(&(*(&__context))->first))->prev) = *(&(*(&(*(&__context))->first))->next) = (list_node * )(0);
		*(&(*(&(*(&__context))->first))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_0) : duplicate(*(&elem_0), (void*)0);
	}else{
		*(&(*(&(*(&__context))->first))->prev) = (list_node * )__newS(16, __nullcTR[107]);
		*(&(*(&(*(&(*(&__context))->first))->prev))->next) = *(&(*(&__context))->first);
		*(&(*(&(*(&(*(&__context))->first))->prev))->prev) = (list_node * )(0);
		*(&(*(&__context))->first) = *(&(*(&(*(&__context))->first))->prev);
		*(&(*(&(*(&__context))->first))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_0) : duplicate(*(&elem_0), (void*)0);
	}
}
void  list__insert_void_ref_list_node_ref_auto_ref_(list_node * it_0, NULLCRef elem_4, list * __context)
{
	unsigned int __temp9_16;
	char_29_ __temp10_20;
	char_35_ __temp11_52;
	char_2_ __temp12_88;
	list_node * next_92;
	if((!*(&(*(&__context))->anyType)) && (__operatorNEqual(typeid__(*(&elem_4), (void*)0), *(&(*(&__context))->elemType), (void*)0)))
	{
		assert(0, __operatorAdd(__operatorAdd(__operatorAdd(__operatorAdd((memcpy((&__temp10_20)->ptr, "list::insert argument type (\x0\x0\x0\x0", 32), __makeNullcArray<char >(&__temp10_20, 29)), (*(&__temp9_16) = typeid__(*(&elem_4), (void*)0), typeid__name__char___ref__(&__temp9_16)), (void*)0), (memcpy((&__temp11_52)->ptr, ") differs from list element type (\x0\x0", 36), __makeNullcArray<char >(&__temp11_52, 35)), (void*)0), typeid__name__char___ref__(&(*(&__context))->elemType), (void*)0), (memcpy((&__temp12_88)->ptr, ")\x0\x0\x0", 4), __makeNullcArray<char >(&__temp12_88, 2)), (void*)0), (void*)0);
	}
	*(&next_92) = *(&(*(&it_0))->next);
	*(&(*(&it_0))->next) = (list_node * )__newS(16, __nullcTR[107]);
	*(&(*(&(*(&it_0))->next))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_4) : duplicate(*(&elem_4), (void*)0);
	*(&(*(&(*(&it_0))->next))->prev) = *(&it_0);
	*(&(*(&(*(&it_0))->next))->next) = *(&next_92);
	if(*(&next_92))
	{
		*(&(*(&next_92))->prev) = *(&(*(&it_0))->next);
	}
}
void  list__erase_void_ref_list_node_ref_(list_node * it_0, list * __context)
{
	list_node * prev_8;
	list_node * next_12;
	*(&prev_8) = *(&(*(&it_0))->prev);
	*(&next_12) = *(&(*(&it_0))->next);
	if(*(&prev_8))
	{
		*(&(*(&prev_8))->next) = *(&next_12);
	}
	if(*(&next_12))
	{
		*(&(*(&next_12))->prev) = *(&prev_8);
	}
	if((*(&it_0)) == (*(&(*(&__context))->first)))
	{
		*(&(*(&__context))->first) = *(&(*(&(*(&__context))->first))->next);
	}
	if((*(&it_0)) == (*(&(*(&__context))->last)))
	{
		*(&(*(&__context))->last) = *(&(*(&(*(&__context))->last))->prev);
	}
}
void  list__clear_void_ref__(list * __context)
{
	*(&(*(&__context))->first) = *(&(*(&__context))->last) = (list_node * )(0);
}
NULLCRef  list__back_auto_ref_ref__(list * __context)
{
	char_32_ __temp13_4;
	assert((*(&(*(&__context))->first)) != ((list_node * )(void * )(0)), (memcpy((&__temp13_4)->ptr, "list::back called on empty list\x0", 32), __makeNullcArray<char >(&__temp13_4, 32)), (void*)0);
	return *(&(*(&(*(&__context))->last))->elem);
}
NULLCRef  list__front_auto_ref_ref__(list * __context)
{
	char_33_ __temp14_4;
	assert((*(&(*(&__context))->first)) != ((list_node * )(void * )(0)), (memcpy((&__temp14_4)->ptr, "list::front called on empty list\x0\x0\x0\x0", 36), __makeNullcArray<char >(&__temp14_4, 33)), (void*)0);
	return *(&(*(&(*(&__context))->first))->elem);
}
list_node *  list__begin_list_node_ref_ref__(list * __context)
{
	return *(&(*(&__context))->first);
}
list_node *  list__end_list_node_ref_ref__(list * __context)
{
	return *(&(*(&__context))->last);
}
int  list__empty_int_ref__(list * __context)
{
	return (*(&(*(&__context))->first)) == ((list_node * )(void * )(0));
}
list_iterator  list_iterator__(list_node * start_0, void* unused)
{
	list_iterator ret_8;
	*(&(&ret_8)->curr) = *(&start_0);
	return *(&ret_8);
}
list_iterator  list__start_list_iterator_ref__(list * __context)
{
	return list_iterator__(*(&(*(&__context))->first), (void*)0);
}
NULLCRef  list_iterator__next_auto_ref_ref__(list_iterator * __context)
{
	NULLCRef ret_4;
	*(&ret_4) = *(&(*(&(*(&__context))->curr))->elem);
	*(&(*(&__context))->curr) = *(&(*(&(*(&__context))->curr))->next);
	return *(&ret_4);
}
int  list_iterator__hasnext_int_ref__(list_iterator * __context)
{
	return *(&(*(&__context))->curr) ? 1 : 0;
}
extern int __init_std_typeinfo_nc();
int __init_std_list_nc()
{
	static int moduleInitialized = 0;
	if(moduleInitialized++)
		return 0;
	__nullcFM = __nullcGetFunctionTable();
	int __local = 0;
	__nullcRegisterBase((void*)&__local);
	__init_std_typeinfo_nc();
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
	__nullcTR[84] = __nullcRegisterType(1872746701u, "int ref(typeid)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[85] = __nullcRegisterType(3335638996u, "int ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[86] = __nullcRegisterType(4231238349u, "typeid ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[87] = __nullcRegisterType(2745156404u, "char[] ref(int)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[88] = __nullcRegisterType(2501899970u, "typeid ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[89] = __nullcRegisterType(1748991366u, "member_iterator", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[90] = __nullcRegisterType(3319164712u, "member_info", 12, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[91] = __nullcRegisterType(3504529713u, "typeid ref", 4, __nullcTR[8], 1, NULLC_POINTER);
	__nullcTR[92] = __nullcRegisterType(1329745667u, "member_iterator ref", 4, __nullcTR[89], 1, NULLC_POINTER);
	__nullcTR[93] = __nullcRegisterType(1559597102u, "typeid ref ref", 4, __nullcTR[91], 1, NULLC_POINTER);
	__nullcTR[94] = __nullcRegisterType(689053972u, "member_iterator ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[95] = __nullcRegisterType(3055261440u, "member_iterator ref ref", 4, __nullcTR[92], 1, NULLC_POINTER);
	__nullcTR[96] = __nullcRegisterType(2875022417u, "member_iterator ref ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[97] = __nullcRegisterType(4163016061u, "member_iterator ref ref ref", 4, __nullcTR[95], 1, NULLC_POINTER);
	__nullcTR[98] = __nullcRegisterType(4051874520u, "auto ref ref", 4, __nullcTR[7], 1, NULLC_POINTER);
	__nullcTR[99] = __nullcRegisterType(2623357349u, "member_info ref", 4, __nullcTR[90], 1, NULLC_POINTER);
	__nullcTR[100] = __nullcRegisterType(682902582u, "member_info ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[101] = __nullcRegisterType(2249706641u, "argument_iterator", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[102] = __nullcRegisterType(795945870u, "argument_iterator ref", 4, __nullcTR[101], 1, NULLC_POINTER);
	__nullcTR[103] = __nullcRegisterType(3496627295u, "argument_iterator ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[104] = __nullcRegisterType(2485895435u, "argument_iterator ref ref", 4, __nullcTR[102], 1, NULLC_POINTER);
	__nullcTR[105] = __nullcRegisterType(1310733596u, "argument_iterator ref ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[106] = __nullcRegisterType(1459539208u, "argument_iterator ref ref ref", 4, __nullcTR[104], 1, NULLC_POINTER);
	__nullcTR[107] = __nullcRegisterType(2811333126u, "list_node", 16, __nullcTR[0], 3, NULLC_CLASS);
	__nullcTR[108] = __nullcRegisterType(3385236355u, "list_node ref", 4, __nullcTR[107], 1, NULLC_POINTER);
	__nullcTR[109] = __nullcRegisterType(1559940649u, "auto ref ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[110] = __nullcRegisterType(988947328u, "list_node ref ref", 4, __nullcTR[108], 1, NULLC_POINTER);
	__nullcTR[111] = __nullcRegisterType(3865797117u, "list_node ref ref ref", 4, __nullcTR[110], 1, NULLC_POINTER);
	__nullcTR[112] = __nullcRegisterType(2090473057u, "list", 16, __nullcTR[0], 4, NULLC_CLASS);
	__nullcTR[113] = __nullcRegisterType(118534703u, "list ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[114] = __nullcRegisterType(3466845534u, "list ref", 4, __nullcTR[112], 1, NULLC_POINTER);
	__nullcTR[115] = __nullcRegisterType(3807592574u, "list ref(typeid)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[116] = __nullcRegisterType(1812738619u, "void ref(auto ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[117] = __nullcRegisterType(235068123u, "list ref ref", 4, __nullcTR[114], 1, NULLC_POINTER);
	__nullcTR[118] = __nullcRegisterType(3077459672u, "list ref ref ref", 4, __nullcTR[117], 1, NULLC_POINTER);
	__nullcTR[119] = __nullcRegisterType(156721184u, "char[32]", 32, __nullcTR[6], 32, NULLC_ARRAY);
	__nullcTR[120] = __nullcRegisterType(2824728221u, "char[32] ref", 4, __nullcTR[119], 1, NULLC_POINTER);
	__nullcTR[121] = __nullcRegisterType(156721283u, "char[35]", 36, __nullcTR[6], 35, NULLC_ARRAY);
	__nullcTR[122] = __nullcRegisterType(2942134400u, "char[35] ref", 4, __nullcTR[121], 1, NULLC_POINTER);
	__nullcTR[123] = __nullcRegisterType(3258512237u, "char[2]", 4, __nullcTR[6], 2, NULLC_ARRAY);
	__nullcTR[124] = __nullcRegisterType(1396858986u, "char[2] ref", 4, __nullcTR[123], 1, NULLC_POINTER);
	__nullcTR[125] = __nullcRegisterType(156721217u, "char[33]", 36, __nullcTR[6], 33, NULLC_ARRAY);
	__nullcTR[126] = __nullcRegisterType(2863863614u, "char[33] ref", 4, __nullcTR[125], 1, NULLC_POINTER);
	__nullcTR[127] = __nullcRegisterType(2874089669u, "void ref(list_node ref,auto ref)", 8, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[128] = __nullcRegisterType(156720326u, "char[29]", 32, __nullcTR[6], 29, NULLC_ARRAY);
	__nullcTR[129] = __nullcRegisterType(1807208003u, "char[29] ref", 4, __nullcTR[128], 1, NULLC_POINTER);
	__nullcTR[130] = __nullcRegisterType(2550440515u, "void ref(list_node ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[131] = __nullcRegisterType(3221817553u, "list_node ref ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[132] = __nullcRegisterType(2965682314u, "list_iterator", 4, __nullcTR[0], 1, NULLC_CLASS);
	__nullcTR[133] = __nullcRegisterType(2117430279u, "list_iterator ref", 4, __nullcTR[132], 1, NULLC_POINTER);
	__nullcTR[134] = __nullcRegisterType(3496029590u, "list_iterator ref(list_node ref)", 8, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[135] = __nullcRegisterType(3779104536u, "list_iterator ref()", 8, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[136] = __nullcRegisterType(865965572u, "list_iterator ref ref", 4, __nullcTR[133], 1, NULLC_POINTER);
	__nullcTR[137] = __nullcRegisterType(1423210113u, "list_iterator ref ref ref", 4, __nullcTR[136], 1, NULLC_POINTER);
	__nullcRegisterMembers(__nullcTR[7], 2, __nullcTR[8], 0, __nullcTR[9], 4);
	__nullcRegisterMembers(__nullcTR[8], 0);
	__nullcRegisterMembers(__nullcTR[10], 3, __nullcTR[8], 0, __nullcTR[9], 4, __nullcTR[4], 8);
	__nullcRegisterMembers(__nullcTR[71], 1, __nullcTR[13], 0);
	__nullcRegisterMembers(__nullcTR[89], 2, __nullcTR[8], 0, __nullcTR[4], 4);
	__nullcRegisterMembers(__nullcTR[90], 2, __nullcTR[8], 0, __nullcTR[13], 4);
	__nullcRegisterMembers(__nullcTR[101], 2, __nullcTR[8], 0, __nullcTR[4], 4);
	__nullcRegisterMembers(__nullcTR[107], 3, __nullcTR[108], 0, __nullcTR[108], 4, __nullcTR[7], 8);
	__nullcRegisterMembers(__nullcTR[112], 4, __nullcTR[8], 0, __nullcTR[4], 4, __nullcTR[108], 8, __nullcTR[108], 12);
	__nullcRegisterMembers(__nullcTR[132], 1, __nullcTR[108], 0);
	__nullcFR[0] = 0;
	__nullcFR[1] = 0;
	__nullcFR[2] = 0;
	__nullcFR[3] = 0;
	__nullcFR[4] = 0;
	__nullcFR[5] = __nullcRegisterFunction("__operatorAddSet", (void*)__operatorAddSet, 4294967295u);
	__nullcFR[6] = __nullcRegisterFunction("char__", (void*)char__, 4294967295u);
	__nullcFR[7] = __nullcRegisterFunction("short__", (void*)short__, 4294967295u);
	__nullcFR[8] = __nullcRegisterFunction("int__", (void*)int__, 4294967295u);
	__nullcFR[9] = __nullcRegisterFunction("long__", (void*)long__, 4294967295u);
	__nullcFR[10] = __nullcRegisterFunction("float__", (void*)float__, 4294967295u);
	__nullcFR[11] = __nullcRegisterFunction("double__", (void*)double__, 4294967295u);
	__nullcFR[12] = __nullcRegisterFunction("char__char_void_ref_char_", (void*)char__char_void_ref_char_, 4294967295u);
	__nullcFR[13] = __nullcRegisterFunction("short__short_void_ref_short_", (void*)short__short_void_ref_short_, 4294967295u);
	__nullcFR[14] = __nullcRegisterFunction("int__int_void_ref_int_", (void*)int__int_void_ref_int_, 4294967295u);
	__nullcFR[15] = __nullcRegisterFunction("long__long_void_ref_long_", (void*)long__long_void_ref_long_, 4294967295u);
	__nullcFR[16] = __nullcRegisterFunction("float__float_void_ref_float_", (void*)float__float_void_ref_float_, 4294967295u);
	__nullcFR[17] = __nullcRegisterFunction("double__double_void_ref_double_", (void*)double__double_void_ref_double_, 4294967295u);
	__nullcFR[18] = __nullcRegisterFunction("int__str_char___ref__", (void*)int__str_char___ref__, 4294967295u);
	__nullcFR[19] = __nullcRegisterFunction("__newS", (void*)__newS, 4294967295u);
	__nullcFR[20] = __nullcRegisterFunction("__newA", (void*)__newA, 4294967295u);
	__nullcFR[21] = __nullcRegisterFunction("duplicate", (void*)duplicate, 4294967295u);
	__nullcFR[22] = __nullcRegisterFunction("__redirect", (void*)__redirect, 4294967295u);
	__nullcFR[23] = 0;
	__nullcFR[24] = 0;
	__nullcFR[25] = 0;
	__nullcFR[26] = __nullcRegisterFunction("typeid__", (void*)typeid__, 4294967295u);
	__nullcFR[27] = 0;
	__nullcFR[28] = 0;
	__nullcFR[29] = __nullcRegisterFunction("__pcomp", (void*)__pcomp, 4294967295u);
	__nullcFR[30] = __nullcRegisterFunction("__pncomp", (void*)__pncomp, 4294967295u);
	__nullcFR[31] = __nullcRegisterFunction("__typeCount", (void*)__typeCount, 4294967295u);
	__nullcFR[32] = 0;
	__nullcFR[33] = 0;
	__nullcFR[34] = 0;
	__nullcFR[35] = 0;
	__nullcFR[36] = __nullcRegisterFunction("const_string__size__int_ref__", (void*)const_string__size__int_ref__, 4294967295u);
	__nullcFR[37] = 0;
	__nullcFR[38] = __nullcRegisterFunction("const_string__", (void*)const_string__, 4294967295u);
	__nullcFR[39] = 0;
	__nullcFR[40] = 0;
	__nullcFR[41] = 0;
	__nullcFR[42] = 0;
	__nullcFR[43] = 0;
	__nullcFR[44] = 0;
	__nullcFR[45] = 0;
	__nullcFR[46] = 0;
	__nullcFR[47] = 0;
	__nullcFR[48] = 0;
	__nullcFR[49] = __nullcRegisterFunction("__char_a_12", (void*)__char_a_12, 4294967295u);
	__nullcFR[50] = __nullcRegisterFunction("__short_a_13", (void*)__short_a_13, 4294967295u);
	__nullcFR[51] = __nullcRegisterFunction("__int_a_14", (void*)__int_a_14, 4294967295u);
	__nullcFR[52] = __nullcRegisterFunction("__long_a_15", (void*)__long_a_15, 4294967295u);
	__nullcFR[53] = __nullcRegisterFunction("__float_a_16", (void*)__float_a_16, 4294967295u);
	__nullcFR[54] = __nullcRegisterFunction("__double_a_17", (void*)__double_a_17, 4294967295u);
	__nullcFR[55] = 0;
	__nullcFR[56] = 0;
	__nullcFR[57] = 0;
	__nullcFR[58] = 0;
	__nullcFR[59] = 0;
	__nullcFR[60] = 0;
	__nullcFR[61] = 0;
	__nullcFR[62] = 0;
	__nullcFR[63] = 0;
	__nullcFR[64] = 0;
	__nullcFR[65] = __nullcRegisterFunction("typeid__size__int_ref__", (void*)typeid__size__int_ref__, 4294967295u);
	__nullcFR[66] = __nullcRegisterFunction("typeid__name__char___ref__", (void*)typeid__name__char___ref__, 4294967295u);
	__nullcFR[67] = __nullcRegisterFunction("typeid__memberCount_int_ref__", (void*)typeid__memberCount_int_ref__, 4294967295u);
	__nullcFR[68] = __nullcRegisterFunction("typeid__memberType_typeid_ref_int_", (void*)typeid__memberType_typeid_ref_int_, 4294967295u);
	__nullcFR[69] = __nullcRegisterFunction("typeid__memberName_char___ref_int_", (void*)typeid__memberName_char___ref_int_, 4294967295u);
	__nullcFR[70] = __nullcRegisterFunction("typeid__subType_typeid_ref__", (void*)typeid__subType_typeid_ref__, 4294967295u);
	__nullcFR[71] = __nullcRegisterFunction("typeid__arraySize_int_ref__", (void*)typeid__arraySize_int_ref__, 4294967295u);
	__nullcFR[72] = __nullcRegisterFunction("typeid__returnType_typeid_ref__", (void*)typeid__returnType_typeid_ref__, 4294967295u);
	__nullcFR[73] = __nullcRegisterFunction("typeid__argumentCount_int_ref__", (void*)typeid__argumentCount_int_ref__, 4294967295u);
	__nullcFR[74] = __nullcRegisterFunction("typeid__argumentType_typeid_ref_int_", (void*)typeid__argumentType_typeid_ref_int_, 4294967295u);
	__nullcFR[75] = __nullcRegisterFunction("typeid__members_member_iterator_ref__", (void*)typeid__members_member_iterator_ref__, 4294967295u);
	__nullcFR[76] = __nullcRegisterFunction("member_iterator__start_member_iterator_ref_ref__", (void*)member_iterator__start_member_iterator_ref_ref__, 4294967295u);
	__nullcFR[77] = __nullcRegisterFunction("member_iterator__hasnext_int_ref__", (void*)member_iterator__hasnext_int_ref__, 4294967295u);
	__nullcFR[78] = __nullcRegisterFunction("member_iterator__next_member_info_ref__", (void*)member_iterator__next_member_info_ref__, 4294967295u);
	__nullcFR[79] = __nullcRegisterFunction("typeid__arguments_argument_iterator_ref__", (void*)typeid__arguments_argument_iterator_ref__, 4294967295u);
	__nullcFR[80] = __nullcRegisterFunction("argument_iterator__start_argument_iterator_ref_ref__", (void*)argument_iterator__start_argument_iterator_ref_ref__, 4294967295u);
	__nullcFR[81] = __nullcRegisterFunction("argument_iterator__hasnext_int_ref__", (void*)argument_iterator__hasnext_int_ref__, 4294967295u);
	__nullcFR[82] = __nullcRegisterFunction("argument_iterator__next_typeid_ref__", (void*)argument_iterator__next_typeid_ref__, 4294967295u);
	__nullcFR[83] = __nullcRegisterFunction("list_node__value__auto_ref_ref__", (void*)list_node__value__auto_ref_ref__, 108u);
	__nullcFR[84] = 0;
	__nullcFR[85] = 0;
	__nullcFR[86] = __nullcRegisterFunction("list__push_back_void_ref_auto_ref_", (void*)list__push_back_void_ref_auto_ref_, 114u);
	__nullcFR[87] = __nullcRegisterFunction("list__push_front_void_ref_auto_ref_", (void*)list__push_front_void_ref_auto_ref_, 114u);
	__nullcFR[88] = __nullcRegisterFunction("list__insert_void_ref_list_node_ref_auto_ref_", (void*)list__insert_void_ref_list_node_ref_auto_ref_, 114u);
	__nullcFR[89] = __nullcRegisterFunction("list__erase_void_ref_list_node_ref_", (void*)list__erase_void_ref_list_node_ref_, 114u);
	__nullcFR[90] = __nullcRegisterFunction("list__clear_void_ref__", (void*)list__clear_void_ref__, 114u);
	__nullcFR[91] = __nullcRegisterFunction("list__back_auto_ref_ref__", (void*)list__back_auto_ref_ref__, 114u);
	__nullcFR[92] = __nullcRegisterFunction("list__front_auto_ref_ref__", (void*)list__front_auto_ref_ref__, 114u);
	__nullcFR[93] = __nullcRegisterFunction("list__begin_list_node_ref_ref__", (void*)list__begin_list_node_ref_ref__, 114u);
	__nullcFR[94] = __nullcRegisterFunction("list__end_list_node_ref_ref__", (void*)list__end_list_node_ref_ref__, 114u);
	__nullcFR[95] = __nullcRegisterFunction("list__empty_int_ref__", (void*)list__empty_int_ref__, 114u);
	__nullcFR[96] = __nullcRegisterFunction("list_iterator__", (void*)list_iterator__, 31u);
	__nullcFR[97] = __nullcRegisterFunction("list__start_list_iterator_ref__", (void*)list__start_list_iterator_ref__, 114u);
	__nullcFR[98] = __nullcRegisterFunction("list_iterator__next_auto_ref_ref__", (void*)list_iterator__next_auto_ref_ref__, 133u);
	__nullcFR[99] = __nullcRegisterFunction("list_iterator__hasnext_int_ref__", (void*)list_iterator__hasnext_int_ref__, 133u);
}
