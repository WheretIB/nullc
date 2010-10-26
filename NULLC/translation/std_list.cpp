#include "runtime.h"
// Typeid redirect table
static unsigned __nullcTR[153];
// Function pointer table
static __nullcFunctionArray* __nullcFM;
// Function pointer redirect table
static unsigned __nullcFR[125];
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
struct __typeProxy_int_ref_typeid_{};
struct __typeProxy_typeid_ref_int_{};
struct __typeProxy_typeid_ref__{};
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
struct __typeProxy_member_iterator_ref__{};
struct __typeProxy_member_iterator_ref_ref__{};
struct __typeProxy_member_info_ref__{};
struct __typeProxy_auto_ref_ref_auto_ref_int_{};
struct __typeProxy_auto_ref_ref_auto_ref_char___{};
struct member_iterator_obj 
{
	unsigned int classID;
	NULLCRef obj;
	int pos;
};
struct member_info_obj 
{
	unsigned int type;
	NULLCArray<char > name;
	NULLCRef value;
};
struct __typeProxy_member_iterator_obj_ref_auto_ref_{};
struct __typeProxy_member_iterator_obj_ref_ref__{};
struct __typeProxy_member_info_obj_ref__{};
struct argument_iterator 
{
	unsigned int funcID;
	int pos;
};
struct __typeProxy_argument_iterator_ref__{};
struct __typeProxy_argument_iterator_ref_ref__{};
struct list_node 
{
	list_node * prev;
	list_node * next;
	NULLCRef elem;
	NULLCRef parent;
};
struct list 
{
	unsigned int elemType;
	int anyType;
	list_node * first;
	list_node * last;
};
struct __typeProxy_auto_ref_ref__{};
struct char_32_ 
{
	char ptr[32];
	char_32_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
	char_32_ (){}
	char_32_ (const char* data){ memcpy(ptr, data, 32); }
};
struct char_35_ 
{
	char ptr[36];
	char_35_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
	char_35_ (){}
	char_35_ (const char* data){ memcpy(ptr, data, 35); }
};
struct char_2_ 
{
	char ptr[4];
	char_2_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
	char_2_ (){}
	char_2_ (const char* data){ memcpy(ptr, data, 2); }
};
struct __typeProxy_void_ref_typeid_{};
struct __typeProxy_list_ref_typeid_{};
struct char_33_ 
{
	char ptr[36];
	char_33_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
	char_33_ (){}
	char_33_ (const char* data){ memcpy(ptr, data, 33); }
};
struct __typeProxy_void_ref_list_node_ref_auto_ref_{};
struct char_29_ 
{
	char ptr[32];
	char_29_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
	char_29_ (){}
	char_29_ (const char* data){ memcpy(ptr, data, 29); }
};
struct char_47_ 
{
	char ptr[48];
	char_47_ & set(unsigned index, char const & val){ ptr[index] = val; return *this; }
	char_47_ (){}
	char_47_ (const char* data){ memcpy(ptr, data, 47); }
};
struct __typeProxy_void_ref_list_node_ref_{};
struct __typeProxy_list_node_ref_ref__{};
struct list_iterator 
{
	list_node * curr;
};
struct __typeProxy_list_iterator_ref_list_node_ref_{};
struct __typeProxy_list_iterator_ref__{};
#pragma pack(pop)
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
extern NULLCRef typeGetMember(NULLCRef obj, int member, void* unused);
extern NULLCRef typeGetMember(NULLCRef obj, NULLCArray<char > name, void* unused);
extern member_iterator_obj typeid__members_member_iterator_obj_ref_auto_ref_(NULLCRef obj, unsigned int * __context);
extern member_iterator_obj * member_iterator_obj__start_member_iterator_obj_ref_ref__(member_iterator_obj * __context);
extern int member_iterator_obj__hasnext_int_ref__(member_iterator_obj * __context);
extern member_info_obj member_iterator_obj__next_member_info_obj_ref__(member_iterator_obj * __context);
extern argument_iterator typeid__arguments_argument_iterator_ref__(unsigned int * __context);
extern argument_iterator * argument_iterator__start_argument_iterator_ref_ref__(argument_iterator * __context);
extern int argument_iterator__hasnext_int_ref__(argument_iterator * __context);
extern unsigned int argument_iterator__next_typeid_ref__(argument_iterator * __context);
NULLCRef list_node__value__auto_ref_ref__(list_node * __context);
NULLCRef list_node__value__auto_ref_ref_auto_ref_(NULLCRef val, list_node * __context);
void list__list_void_ref_typeid_(unsigned int type, list * __context);
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
unsigned int __list_type_107(void* unused);
unsigned int __list_type_108(void* unused);
NULLCRef  list_node__value__auto_ref_ref__(list_node * __context)
{
	return *(&(*(&__context))->elem);
}
NULLCRef  list_node__value__auto_ref_ref_auto_ref_(NULLCRef val_0, list_node * __context)
{
	list * listParent_20;
	unsigned int __temp1_28;
	char_32_ __temp2_32;
	char_35_ __temp3_64;
	char_2_ __temp4_100;
	*(&listParent_20) = (list * )__nullcGetAutoRef(*(&(*(&__context))->parent), __nullcTR[126]);
	if((!(*(&(*(&listParent_20))->anyType))) && (__operatorNEqual(typeid__(*(&val_0), (void*)0), isPointer(*(&(*(&listParent_20))->elemType), (void*)0) ? typeid__subType_typeid_ref__((unsigned int * )(&(*(&listParent_20))->elemType)) : *(&(*(&listParent_20))->elemType), (void*)0)))
	{
		assert(0, __operatorAdd(__operatorAdd(__operatorAdd(__operatorAdd((memcpy((&__temp2_32)->ptr, "list_node.value argument type (\x0", 32), __makeNullcArray<char >(&__temp2_32, 32)), (*(&__temp1_28) = typeid__(*(&val_0), (void*)0), typeid__name__char___ref__((unsigned int * )(&__temp1_28))), (void*)0), (memcpy((&__temp3_64)->ptr, ") differs from list element type (\x0\x0", 36), __makeNullcArray<char >(&__temp3_64, 35)), (void*)0), typeid__name__char___ref__((unsigned int * )(&(*(&listParent_20))->elemType)), (void*)0), (memcpy((&__temp4_100)->ptr, ")\x0\x0\x0", 4), __makeNullcArray<char >(&__temp4_100, 2)), (void*)0), (void*)0);
	}
	return *(&(*(&__context))->elem) = duplicate(*(&val_0), (void*)0);
}
void  list__list_void_ref_typeid_(unsigned int type_0, list * __context)
{
	*(&(*(&__context))->anyType) = __operatorEqual(*(&type_0), 7, (void*)0);
	*(&(*(&__context))->elemType) = *(&type_0);
	*(&(*(&__context))->first) = *(&(*(&__context))->last) = (list_node * )(0);
}
list  list__(unsigned int type_0, void* unused)
{
	list ret_12;
	list__list_void_ref_typeid_(*(&type_0), (list * )(&ret_12));
	return *(&ret_12);
}
void  list__push_back_void_ref_auto_ref_(NULLCRef elem_0, list * __context)
{
	unsigned int __temp5_20;
	char_32_ __temp6_24;
	char_35_ __temp7_56;
	char_2_ __temp8_92;
	if((!(*(&(*(&__context))->anyType))) && (__operatorNEqual(typeid__(*(&elem_0), (void*)0), isPointer(*(&(*(&__context))->elemType), (void*)0) ? typeid__subType_typeid_ref__((unsigned int * )(&(*(&__context))->elemType)) : *(&(*(&__context))->elemType), (void*)0)))
	{
		assert(0, __operatorAdd(__operatorAdd(__operatorAdd(__operatorAdd((memcpy((&__temp6_24)->ptr, "list::push_back argument type (\x0", 32), __makeNullcArray<char >(&__temp6_24, 32)), (*(&__temp5_20) = typeid__(*(&elem_0), (void*)0), typeid__name__char___ref__((unsigned int * )(&__temp5_20))), (void*)0), (memcpy((&__temp7_56)->ptr, ") differs from list element type (\x0\x0", 36), __makeNullcArray<char >(&__temp7_56, 35)), (void*)0), typeid__name__char___ref__((unsigned int * )(&(*(&__context))->elemType)), (void*)0), (memcpy((&__temp8_92)->ptr, ")\x0\x0\x0", 4), __makeNullcArray<char >(&__temp8_92, 2)), (void*)0), (void*)0);
	}
	if(!(*(&(*(&__context))->first)))
	{
		*(&(*(&__context))->first) = *(&(*(&__context))->last) = (list_node * )__newS(40, __nullcTR[124]);
		*(&(*(&(*(&__context))->first))->prev) = *(&(*(&(*(&__context))->first))->next) = (list_node * )(0);
		*(&(*(&(*(&__context))->first))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_0) : duplicate(*(&elem_0), (void*)0);
	}else{
		*(&(*(&(*(&__context))->last))->next) = (list_node * )__newS(40, __nullcTR[124]);
		*(&(*(&(*(&(*(&__context))->last))->next))->prev) = *(&(*(&__context))->last);
		*(&(*(&(*(&(*(&__context))->last))->next))->next) = (list_node * )(0);
		*(&(*(&__context))->last) = *(&(*(&(*(&__context))->last))->next);
		*(&(*(&(*(&__context))->last))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_0) : duplicate(*(&elem_0), (void*)0);
	}
	*(&(*(&(*(&__context))->last))->parent) = __nullcMakeAutoRef((void*)*(&__context), __nullcTR[126]);
}
void  list__push_front_void_ref_auto_ref_(NULLCRef elem_0, list * __context)
{
	unsigned int __temp9_20;
	char_33_ __temp10_24;
	char_35_ __temp11_60;
	char_2_ __temp12_96;
	if((!(*(&(*(&__context))->anyType))) && (__operatorNEqual(typeid__(*(&elem_0), (void*)0), *(&(*(&__context))->elemType), (void*)0)))
	{
		assert(0, __operatorAdd(__operatorAdd(__operatorAdd(__operatorAdd((memcpy((&__temp10_24)->ptr, "list::push_front argument type (\x0\x0\x0\x0", 36), __makeNullcArray<char >(&__temp10_24, 33)), (*(&__temp9_20) = typeid__(*(&elem_0), (void*)0), typeid__name__char___ref__((unsigned int * )(&__temp9_20))), (void*)0), (memcpy((&__temp11_60)->ptr, ") differs from list element type (\x0\x0", 36), __makeNullcArray<char >(&__temp11_60, 35)), (void*)0), typeid__name__char___ref__((unsigned int * )(&(*(&__context))->elemType)), (void*)0), (memcpy((&__temp12_96)->ptr, ")\x0\x0\x0", 4), __makeNullcArray<char >(&__temp12_96, 2)), (void*)0), (void*)0);
	}
	if(!(*(&(*(&__context))->first)))
	{
		*(&(*(&__context))->first) = *(&(*(&__context))->last) = (list_node * )__newS(40, __nullcTR[124]);
		*(&(*(&(*(&__context))->first))->prev) = *(&(*(&(*(&__context))->first))->next) = (list_node * )(0);
		*(&(*(&(*(&__context))->first))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_0) : duplicate(*(&elem_0), (void*)0);
	}else{
		*(&(*(&(*(&__context))->first))->prev) = (list_node * )__newS(40, __nullcTR[124]);
		*(&(*(&(*(&(*(&__context))->first))->prev))->next) = *(&(*(&__context))->first);
		*(&(*(&(*(&(*(&__context))->first))->prev))->prev) = (list_node * )(0);
		*(&(*(&__context))->first) = *(&(*(&(*(&__context))->first))->prev);
		*(&(*(&(*(&__context))->first))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_0) : duplicate(*(&elem_0), (void*)0);
	}
	*(&(*(&(*(&__context))->last))->parent) = __nullcMakeAutoRef((void*)*(&__context), __nullcTR[126]);
}
void  list__insert_void_ref_list_node_ref_auto_ref_(list_node * it_0, NULLCRef elem_8, list * __context)
{
	unsigned int __temp13_28;
	char_29_ __temp14_32;
	char_35_ __temp15_64;
	char_2_ __temp16_100;
	char_47_ __temp17_104;
	list_node * next_152;
	if((!(*(&(*(&__context))->anyType))) && (__operatorNEqual(typeid__(*(&elem_8), (void*)0), *(&(*(&__context))->elemType), (void*)0)))
	{
		assert(0, __operatorAdd(__operatorAdd(__operatorAdd(__operatorAdd((memcpy((&__temp14_32)->ptr, "list::insert argument type (\x0\x0\x0\x0", 32), __makeNullcArray<char >(&__temp14_32, 29)), (*(&__temp13_28) = typeid__(*(&elem_8), (void*)0), typeid__name__char___ref__((unsigned int * )(&__temp13_28))), (void*)0), (memcpy((&__temp15_64)->ptr, ") differs from list element type (\x0\x0", 36), __makeNullcArray<char >(&__temp15_64, 35)), (void*)0), typeid__name__char___ref__((unsigned int * )(&(*(&__context))->elemType)), (void*)0), (memcpy((&__temp16_100)->ptr, ")\x0\x0\x0", 4), __makeNullcArray<char >(&__temp16_100, 2)), (void*)0), (void*)0);
	}
	if(((list * )__nullcGetAutoRef(*(&(*(&it_0))->parent), __nullcTR[126])) != (*(&__context)))
	{
		assert(0, (memcpy((&__temp17_104)->ptr, "list::insert iterator is from a different list\x0\x0", 48), __makeNullcArray<char >(&__temp17_104, 47)), (void*)0);
	}
	*(&next_152) = *(&(*(&it_0))->next);
	*(&(*(&it_0))->next) = (list_node * )__newS(40, __nullcTR[124]);
	*(&(*(&(*(&it_0))->next))->elem) = isPointer(*(&(*(&__context))->elemType), (void*)0) ? *(&elem_8) : duplicate(*(&elem_8), (void*)0);
	*(&(*(&(*(&it_0))->next))->prev) = *(&it_0);
	*(&(*(&(*(&it_0))->next))->next) = *(&next_152);
	*(&(*(&(*(&it_0))->next))->parent) = __nullcMakeAutoRef((void*)*(&__context), __nullcTR[126]);
	if(*(&next_152))
	{
		*(&(*(&next_152))->prev) = *(&(*(&it_0))->next);
	}
}
void  list__erase_void_ref_list_node_ref_(list_node * it_0, list * __context)
{
	char_47_ __temp18_16;
	list_node * prev_64;
	list_node * next_72;
	if(((list * )__nullcGetAutoRef(*(&(*(&it_0))->parent), __nullcTR[126])) != (*(&__context)))
	{
		assert(0, (memcpy((&__temp18_16)->ptr, "list::insert iterator is from a different list\x0\x0", 48), __makeNullcArray<char >(&__temp18_16, 47)), (void*)0);
	}
	*(&prev_64) = *(&(*(&it_0))->prev);
	*(&next_72) = *(&(*(&it_0))->next);
	if(*(&prev_64))
	{
		*(&(*(&prev_64))->next) = *(&next_72);
	}
	if(*(&next_72))
	{
		*(&(*(&next_72))->prev) = *(&prev_64);
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
	char_32_ __temp19_8;
	assert((*(&(*(&__context))->first)) != ((list_node * )(0)), (memcpy((&__temp19_8)->ptr, "list::back called on empty list\x0", 32), __makeNullcArray<char >(&__temp19_8, 32)), (void*)0);
	return *(&(*(&(*(&__context))->last))->elem);
}
NULLCRef  list__front_auto_ref_ref__(list * __context)
{
	char_33_ __temp20_8;
	assert((*(&(*(&__context))->first)) != ((list_node * )(0)), (memcpy((&__temp20_8)->ptr, "list::front called on empty list\x0\x0\x0\x0", 36), __makeNullcArray<char >(&__temp20_8, 33)), (void*)0);
	return *(&(*(&(*(&__context))->first))->elem);
}
list_node *  list__begin_list_node_ref_ref__(list * __context)
{
	return __nullcCheckedRet(__nullcTR[124], (*(&(*(&__context))->first)), (void*)&__context, 0);
}
list_node *  list__end_list_node_ref_ref__(list * __context)
{
	return __nullcCheckedRet(__nullcTR[124], (*(&(*(&__context))->last)), (void*)&__context, 0);
}
int  list__empty_int_ref__(list * __context)
{
	return (*(&(*(&__context))->first)) == ((list_node * )(0));
}
list_iterator  list_iterator__(list_node * start_0, void* unused)
{
	list_iterator ret_16;
	*(&(&ret_16)->curr) = *(&start_0);
	return *(&ret_16);
}
list_iterator  list__start_list_iterator_ref__(list * __context)
{
	return list_iterator__(*(&(*(&__context))->first), (void*)0);
}
NULLCRef  list_iterator__next_auto_ref_ref__(list_iterator * __context)
{
	NULLCRef ret_8;
	*(&ret_8) = *(&(*(&(*(&__context))->curr))->elem);
	*(&(*(&__context))->curr) = *(&(*(&(*(&__context))->curr))->next);
	return *(&ret_8);
}
int  list_iterator__hasnext_int_ref__(list_iterator * __context)
{
	return *(&(*(&__context))->curr) ? 1 : 0;
}
unsigned int  __list_type_107(void* unused)
{
	return 7;
}
unsigned int  __list_type_108(void* unused)
{
	return 7;
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
	__nullcTR[96] = __nullcRegisterType(1872746701u, "int ref(typeid)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[97] = __nullcRegisterType(4231238349u, "typeid ref(int)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[98] = __nullcRegisterType(2501899970u, "typeid ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[99] = __nullcRegisterType(1748991366u, "member_iterator", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[100] = __nullcRegisterType(3319164712u, "member_info", 16, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[101] = __nullcRegisterType(1329745667u, "member_iterator ref", 8, __nullcTR[99], 1, NULLC_POINTER);
	__nullcTR[102] = __nullcRegisterType(1559597102u, "typeid ref ref", 8, __nullcTR[92], 1, NULLC_POINTER);
	__nullcTR[103] = __nullcRegisterType(689053972u, "member_iterator ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[104] = __nullcRegisterType(3055261440u, "member_iterator ref ref", 8, __nullcTR[101], 1, NULLC_POINTER);
	__nullcTR[105] = __nullcRegisterType(2875022417u, "member_iterator ref ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[106] = __nullcRegisterType(4051874520u, "auto ref ref", 8, __nullcTR[7], 1, NULLC_POINTER);
	__nullcTR[107] = __nullcRegisterType(2623357349u, "member_info ref", 8, __nullcTR[100], 1, NULLC_POINTER);
	__nullcTR[108] = __nullcRegisterType(682902582u, "member_info ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[109] = __nullcRegisterType(3545359766u, "auto ref ref(auto ref,int)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[110] = __nullcRegisterType(4270549729u, "auto ref ref(auto ref,char[])", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[111] = __nullcRegisterType(1332006336u, "member_iterator_obj", 20, __nullcTR[0], 3, NULLC_CLASS);
	__nullcTR[112] = __nullcRegisterType(2625618018u, "member_info_obj", 28, __nullcTR[0], 3, NULLC_CLASS);
	__nullcTR[113] = __nullcRegisterType(3970509885u, "member_iterator_obj ref", 8, __nullcTR[111], 1, NULLC_POINTER);
	__nullcTR[114] = __nullcRegisterType(4024342884u, "member_iterator_obj ref(auto ref)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[115] = __nullcRegisterType(969048378u, "member_iterator_obj ref ref", 8, __nullcTR[113], 1, NULLC_POINTER);
	__nullcTR[116] = __nullcRegisterType(3026697483u, "member_iterator_obj ref ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[117] = __nullcRegisterType(1566613471u, "member_info_obj ref", 8, __nullcTR[112], 1, NULLC_POINTER);
	__nullcTR[118] = __nullcRegisterType(940054768u, "member_info_obj ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[119] = __nullcRegisterType(2249706641u, "argument_iterator", 8, __nullcTR[0], 2, NULLC_CLASS);
	__nullcTR[120] = __nullcRegisterType(795945870u, "argument_iterator ref", 8, __nullcTR[119], 1, NULLC_POINTER);
	__nullcTR[121] = __nullcRegisterType(3496627295u, "argument_iterator ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[122] = __nullcRegisterType(2485895435u, "argument_iterator ref ref", 8, __nullcTR[120], 1, NULLC_POINTER);
	__nullcTR[123] = __nullcRegisterType(1310733596u, "argument_iterator ref ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[124] = __nullcRegisterType(2811333126u, "list_node", 40, __nullcTR[0], 4, NULLC_CLASS);
	__nullcTR[125] = __nullcRegisterType(3385236355u, "list_node ref", 8, __nullcTR[124], 1, NULLC_POINTER);
	__nullcTR[126] = __nullcRegisterType(2090473057u, "list", 24, __nullcTR[0], 4, NULLC_CLASS);
	__nullcTR[127] = __nullcRegisterType(1559940649u, "auto ref ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[128] = __nullcRegisterType(988947328u, "list_node ref ref", 8, __nullcTR[125], 1, NULLC_POINTER);
	__nullcTR[129] = __nullcRegisterType(3466845534u, "list ref", 8, __nullcTR[126], 1, NULLC_POINTER);
	__nullcTR[130] = __nullcRegisterType(235068123u, "list ref ref", 8, __nullcTR[129], 1, NULLC_POINTER);
	__nullcTR[131] = __nullcRegisterType(156721184u, "char[32]", 32, __nullcTR[6], 32, NULLC_ARRAY);
	__nullcTR[132] = __nullcRegisterType(2824728221u, "char[32] ref", 8, __nullcTR[131], 1, NULLC_POINTER);
	__nullcTR[133] = __nullcRegisterType(156721283u, "char[35]", 36, __nullcTR[6], 35, NULLC_ARRAY);
	__nullcTR[134] = __nullcRegisterType(2942134400u, "char[35] ref", 8, __nullcTR[133], 1, NULLC_POINTER);
	__nullcTR[135] = __nullcRegisterType(3258512237u, "char[2]", 4, __nullcTR[6], 2, NULLC_ARRAY);
	__nullcTR[136] = __nullcRegisterType(1396858986u, "char[2] ref", 8, __nullcTR[135], 1, NULLC_POINTER);
	__nullcTR[137] = __nullcRegisterType(3930092916u, "void ref(typeid)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[138] = __nullcRegisterType(3807592574u, "list ref(typeid)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[139] = __nullcRegisterType(156721217u, "char[33]", 36, __nullcTR[6], 33, NULLC_ARRAY);
	__nullcTR[140] = __nullcRegisterType(2863863614u, "char[33] ref", 8, __nullcTR[139], 1, NULLC_POINTER);
	__nullcTR[141] = __nullcRegisterType(2874089669u, "void ref(list_node ref,auto ref)", 12, __nullcTR[0], 2, NULLC_FUNCTION);
	__nullcTR[142] = __nullcRegisterType(156720326u, "char[29]", 32, __nullcTR[6], 29, NULLC_ARRAY);
	__nullcTR[143] = __nullcRegisterType(1807208003u, "char[29] ref", 8, __nullcTR[142], 1, NULLC_POINTER);
	__nullcTR[144] = __nullcRegisterType(156722438u, "char[47]", 48, __nullcTR[6], 47, NULLC_ARRAY);
	__nullcTR[145] = __nullcRegisterType(16905859u, "char[47] ref", 8, __nullcTR[144], 1, NULLC_POINTER);
	__nullcTR[146] = __nullcRegisterType(2550440515u, "void ref(list_node ref)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[147] = __nullcRegisterType(3221817553u, "list_node ref ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[148] = __nullcRegisterType(2965682314u, "list_iterator", 8, __nullcTR[0], 1, NULLC_CLASS);
	__nullcTR[149] = __nullcRegisterType(2117430279u, "list_iterator ref", 8, __nullcTR[148], 1, NULLC_POINTER);
	__nullcTR[150] = __nullcRegisterType(3496029590u, "list_iterator ref(list_node ref)", 12, __nullcTR[0], 1, NULLC_FUNCTION);
	__nullcTR[151] = __nullcRegisterType(3779104536u, "list_iterator ref()", 12, __nullcTR[0], 0, NULLC_FUNCTION);
	__nullcTR[152] = __nullcRegisterType(865965572u, "list_iterator ref ref", 8, __nullcTR[149], 1, NULLC_POINTER);
	__nullcRegisterMembers(__nullcTR[7], 2, __nullcTR[8], 0, __nullcTR[9], 4);
	__nullcRegisterMembers(__nullcTR[8], 0);
	__nullcRegisterMembers(__nullcTR[10], 3, __nullcTR[8], 0, __nullcTR[9], 4, __nullcTR[4], 12);
	__nullcRegisterMembers(__nullcTR[77], 1, __nullcTR[13], 0);
	__nullcRegisterMembers(__nullcTR[99], 2, __nullcTR[8], 0, __nullcTR[4], 4);
	__nullcRegisterMembers(__nullcTR[100], 2, __nullcTR[8], 0, __nullcTR[13], 4);
	__nullcRegisterMembers(__nullcTR[111], 3, __nullcTR[8], 0, __nullcTR[7], 4, __nullcTR[4], 16);
	__nullcRegisterMembers(__nullcTR[112], 3, __nullcTR[8], 0, __nullcTR[13], 4, __nullcTR[7], 16);
	__nullcRegisterMembers(__nullcTR[119], 2, __nullcTR[8], 0, __nullcTR[4], 4);
	__nullcRegisterMembers(__nullcTR[124], 4, __nullcTR[125], 0, __nullcTR[125], 8, __nullcTR[7], 16, __nullcTR[7], 28);
	__nullcRegisterMembers(__nullcTR[126], 4, __nullcTR[8], 0, __nullcTR[4], 4, __nullcTR[125], 8, __nullcTR[125], 16);
	__nullcRegisterMembers(__nullcTR[148], 1, __nullcTR[125], 0);
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
	__nullcFR[79] = 0;
	__nullcFR[80] = 0;
	__nullcFR[81] = 0;
	__nullcFR[82] = __nullcRegisterFunction("typeid__name__char___ref__", (void*)typeid__name__char___ref__, 92u, 2);
	__nullcFR[83] = __nullcRegisterFunction("typeid__memberCount_int_ref__", (void*)typeid__memberCount_int_ref__, 92u, 2);
	__nullcFR[84] = __nullcRegisterFunction("typeid__memberType_typeid_ref_int_", (void*)typeid__memberType_typeid_ref_int_, 92u, 2);
	__nullcFR[85] = __nullcRegisterFunction("typeid__memberName_char___ref_int_", (void*)typeid__memberName_char___ref_int_, 92u, 2);
	__nullcFR[86] = __nullcRegisterFunction("typeid__subType_typeid_ref__", (void*)typeid__subType_typeid_ref__, 92u, 2);
	__nullcFR[87] = __nullcRegisterFunction("typeid__arraySize_int_ref__", (void*)typeid__arraySize_int_ref__, 92u, 2);
	__nullcFR[88] = __nullcRegisterFunction("typeid__returnType_typeid_ref__", (void*)typeid__returnType_typeid_ref__, 92u, 2);
	__nullcFR[89] = __nullcRegisterFunction("typeid__argumentCount_int_ref__", (void*)typeid__argumentCount_int_ref__, 92u, 2);
	__nullcFR[90] = __nullcRegisterFunction("typeid__argumentType_typeid_ref_int_", (void*)typeid__argumentType_typeid_ref_int_, 92u, 2);
	__nullcFR[91] = __nullcRegisterFunction("typeid__members_member_iterator_ref__", (void*)typeid__members_member_iterator_ref__, 92u, 2);
	__nullcFR[92] = __nullcRegisterFunction("member_iterator__start_member_iterator_ref_ref__", (void*)member_iterator__start_member_iterator_ref_ref__, 101u, 2);
	__nullcFR[93] = __nullcRegisterFunction("member_iterator__hasnext_int_ref__", (void*)member_iterator__hasnext_int_ref__, 101u, 2);
	__nullcFR[94] = __nullcRegisterFunction("member_iterator__next_member_info_ref__", (void*)member_iterator__next_member_info_ref__, 101u, 2);
	__nullcFR[95] = 0;
	__nullcFR[96] = 0;
	__nullcFR[97] = __nullcRegisterFunction("typeid__members_member_iterator_obj_ref_auto_ref_", (void*)typeid__members_member_iterator_obj_ref_auto_ref_, 92u, 2);
	__nullcFR[98] = __nullcRegisterFunction("member_iterator_obj__start_member_iterator_obj_ref_ref__", (void*)member_iterator_obj__start_member_iterator_obj_ref_ref__, 113u, 2);
	__nullcFR[99] = __nullcRegisterFunction("member_iterator_obj__hasnext_int_ref__", (void*)member_iterator_obj__hasnext_int_ref__, 113u, 2);
	__nullcFR[100] = __nullcRegisterFunction("member_iterator_obj__next_member_info_obj_ref__", (void*)member_iterator_obj__next_member_info_obj_ref__, 113u, 2);
	__nullcFR[101] = __nullcRegisterFunction("typeid__arguments_argument_iterator_ref__", (void*)typeid__arguments_argument_iterator_ref__, 92u, 2);
	__nullcFR[102] = __nullcRegisterFunction("argument_iterator__start_argument_iterator_ref_ref__", (void*)argument_iterator__start_argument_iterator_ref_ref__, 120u, 2);
	__nullcFR[103] = __nullcRegisterFunction("argument_iterator__hasnext_int_ref__", (void*)argument_iterator__hasnext_int_ref__, 120u, 2);
	__nullcFR[104] = __nullcRegisterFunction("argument_iterator__next_typeid_ref__", (void*)argument_iterator__next_typeid_ref__, 120u, 2);
	__nullcFR[105] = __nullcRegisterFunction("list_node__value__auto_ref_ref__", (void*)list_node__value__auto_ref_ref__, 125u, 2);
	__nullcFR[106] = __nullcRegisterFunction("list_node__value__auto_ref_ref_auto_ref_", (void*)list_node__value__auto_ref_ref_auto_ref_, 125u, 2);
	__nullcFR[107] = __nullcRegisterFunction("list__list_void_ref_typeid_", (void*)list__list_void_ref_typeid_, 129u, 2);
	__nullcFR[108] = __nullcRegisterFunction("list__", (void*)list__, 31u, 0);
	__nullcFR[109] = __nullcRegisterFunction("list__push_back_void_ref_auto_ref_", (void*)list__push_back_void_ref_auto_ref_, 129u, 2);
	__nullcFR[110] = __nullcRegisterFunction("list__push_front_void_ref_auto_ref_", (void*)list__push_front_void_ref_auto_ref_, 129u, 2);
	__nullcFR[111] = __nullcRegisterFunction("list__insert_void_ref_list_node_ref_auto_ref_", (void*)list__insert_void_ref_list_node_ref_auto_ref_, 129u, 2);
	__nullcFR[112] = __nullcRegisterFunction("list__erase_void_ref_list_node_ref_", (void*)list__erase_void_ref_list_node_ref_, 129u, 2);
	__nullcFR[113] = __nullcRegisterFunction("list__clear_void_ref__", (void*)list__clear_void_ref__, 129u, 2);
	__nullcFR[114] = __nullcRegisterFunction("list__back_auto_ref_ref__", (void*)list__back_auto_ref_ref__, 129u, 2);
	__nullcFR[115] = __nullcRegisterFunction("list__front_auto_ref_ref__", (void*)list__front_auto_ref_ref__, 129u, 2);
	__nullcFR[116] = __nullcRegisterFunction("list__begin_list_node_ref_ref__", (void*)list__begin_list_node_ref_ref__, 129u, 2);
	__nullcFR[117] = __nullcRegisterFunction("list__end_list_node_ref_ref__", (void*)list__end_list_node_ref_ref__, 129u, 2);
	__nullcFR[118] = __nullcRegisterFunction("list__empty_int_ref__", (void*)list__empty_int_ref__, 129u, 2);
	__nullcFR[119] = __nullcRegisterFunction("list_iterator__", (void*)list_iterator__, 31u, 0);
	__nullcFR[120] = __nullcRegisterFunction("list__start_list_iterator_ref__", (void*)list__start_list_iterator_ref__, 129u, 2);
	__nullcFR[121] = __nullcRegisterFunction("list_iterator__next_auto_ref_ref__", (void*)list_iterator__next_auto_ref_ref__, 149u, 2);
	__nullcFR[122] = __nullcRegisterFunction("list_iterator__hasnext_int_ref__", (void*)list_iterator__hasnext_int_ref__, 149u, 2);
	__nullcFR[123] = __nullcRegisterFunction("__list_type_107", (void*)__list_type_107, 31u, 0);
	__nullcFR[124] = __nullcRegisterFunction("__list_type_108", (void*)__list_type_108, 31u, 0);
}
