#include <memory.h>
#include <string.h>
#ifndef NULL
	#define NULL 0
#endif

#pragma pack(push, 4)
// Wrapper over NULLC array, for use in external functions
template<typename T>
struct NULLCArray
{
	char *ptr;
	int size;

	NULLCArray()
	{
		ptr = NULL;
		size = 0;
	}

	template<typename Y>
	NULLCArray(const NULLCArray<Y> r)
	{
		ptr = r.ptr;
		size = r.size;
	}
};

// Wrapper over NULLC auto ref class for use in external functions
struct NULLCRef
{
	union
	{
		unsigned int	typeID;
		unsigned int	type;
	};
	char			*ptr;
};

// Wrapper over NULLC function pointer for use in external functions
template<typename T = void>
struct NULLCFuncPtr
{
	void	*context;
	unsigned int id;

	NULLCFuncPtr()
	{
		context = NULL;
		id = 0;
	}
	template<typename Y>
	NULLCFuncPtr(const NULLCFuncPtr<Y> r)
	{
		context = r.context;
		id = r.id;
	}
};

// Wrapper over NULLC auto[] class for use in external functions
struct NULLCAutoArray
{
	union
	{
		unsigned int	typeID;
		unsigned int	type;
	};
	char			*ptr;
	union
	{
		int	len;
		int	size;
	};
};
#pragma pack(pop)

struct NULLCTypeInfo
{
	unsigned int	hash;
	const char		*name;
	unsigned int	size;
	union
	{
		unsigned int	subTypeID;
		unsigned int	baseClassID;
		unsigned int	returnTypeID;
	};
	int				memberCount;
	unsigned int	category;
	unsigned int	alignment;
	unsigned int	flags;
	unsigned int	members;
};

struct NULLCMemberInfo
{
	unsigned int typeID;
	unsigned int offset;
	const char *name;
};

struct NULLCFuncInfo
{
	unsigned	hash;
	const char	*name;
	unsigned	extraType;
	unsigned	funcType;
};

#define NULLC_BASETYPE_VOID 0
#define NULLC_BASETYPE_BOOL 1
#define NULLC_BASETYPE_CHAR 2
#define NULLC_BASETYPE_SHORT 3
#define NULLC_BASETYPE_INT 4
#define NULLC_BASETYPE_LONG 5
#define NULLC_BASETYPE_FLOAT 6
#define NULLC_BASETYPE_DOUBLE 7

#define NULLC_NONE 0
#define NULLC_CLASS 1
#define NULLC_ARRAY 2
#define NULLC_POINTER 3
#define NULLC_FUNCTION 4

#define NULLC_TYPE_FLAG_HAS_FINALIZER 1 << 0
#define NULLC_TYPE_FLAG_IS_EXTENDABLE 1 << 1

#if defined(__GNUC__)

#define NULLC_ALIGN_GCC(x) __attribute__((aligned(x)))
#define NULLC_ALIGN_MSVC(x)

#elif defined(_MSC_VER)

#define NULLC_ALIGN_GCC(x)
#define NULLC_ALIGN_MSVC(x) __declspec(align(x))

#else

#define NULLC_ALIGN_GCC(x)
#define NULLC_ALIGN_MSVC(x)

#endif

namespace FunctionCategory
{
	enum placeholder
	{
		NORMAL,
		LOCAL,
		THISCALL,
		COROUTINE
	};
};

template<typename T>
inline NULLCArray<T> __makeNullcArray(void* ptr, unsigned int size)
{
	NULLCArray<T> ret;
	ret.ptr = (char*)ptr;
	ret.size = size;
	return ret;
}

template<typename T>
inline NULLCArray<T> __makeNullcArray(unsigned int zero, void* unused)
{
	NULLCArray<T> ret;
	ret.ptr = 0;
	ret.size = 0;
	return ret;
}

int			__nullcPow(int a, int b);
double		__nullcPow(double a, double b);
long long	__nullcPow(long long a, long long b);
double		__nullcMod(double a, double b);
int			__nullcPowSet(char *a, int b);
int			__nullcPowSet(short *a, int b);
int			__nullcPowSet(int *a, int b);
double		__nullcPowSet(float *a, double b);
double		__nullcPowSet(double *a, double b);
long long	__nullcPowSet(long long *a, long long b);
void	__nullcSetArray(short arr[], short val, unsigned int count);
void	__nullcSetArray(int arr[], int val, unsigned int count);
void	__nullcSetArray(float arr[], float val, unsigned int count);
void	__nullcSetArray(double arr[], double val, unsigned int count);
void	__nullcSetArray(long long arr[], long long val, unsigned int count);

template<typename T>
void __nullcSetupArray(T* ptr, unsigned size, T value)
{
	for(unsigned i = 0; i < size; i++)
		ptr[i] = value;
}

struct __nullcUpvalue
{
	void *ptr;
	__nullcUpvalue *next;
	unsigned int size;
};
void __nullcCloseUpvalue(__nullcUpvalue *&head, void *ptr);
NULLCFuncPtr<>	__nullcMakeFunction(unsigned int id, void* context);
NULLCRef		__nullcMakeAutoRef(void* ptr, unsigned int typeID);
NULLCRef		__nullcMakeExtendableAutoRef(void* ptr);
void*			__nullcGetAutoRef(const NULLCRef &ref, unsigned int typeID);
NULLCAutoArray	__makeAutoArray(unsigned type, NULLCArray<void> arr);

typedef int __function;
typedef void* _nullptr;

bool operator ==(const NULLCRef& a, const NULLCRef& b);
bool operator !=(const NULLCRef& a, const NULLCRef& b);
bool operator !(const NULLCRef& a);

int  __operatorEqual(unsigned int a, unsigned int b, void* unused);
int  __operatorNEqual(unsigned int a, unsigned int b, void* unused);

// Base module types
struct __FinalizeProxy;
struct __typeProxy_void_ref__;
struct __typeProxy_void_ref_int_;
struct const_string;

// Base module functions
void assert_void_ref_int_(int val, void* __context);
void assert_void_ref_int_char___(int val, NULLCArray<char> message, void* __context);
int __operatorEqual_int_ref_char___char___(NULLCArray<char> a, NULLCArray<char> b, void* __context);
int __operatorNEqual_int_ref_char___char___(NULLCArray<char> a, NULLCArray<char> b, void* __context);
NULLCArray<char> __operatorAdd_char___ref_char___char___(NULLCArray<char> a, NULLCArray<char> b, void* __context);
NULLCArray<char> __operatorAddSet_char___ref_char___ref_char___(NULLCArray<char>* a, NULLCArray<char> b, void* __context);
bool bool_bool_ref_bool_(bool a, void* __context);
char char_char_ref_char_(char a, void* __context);
short short_short_ref_short_(short a, void* __context);
int int_int_ref_int_(int a, void* __context);
long long long_long_ref_long_(long long a, void* __context);
float float_float_ref_float_(float a, void* __context);
double double_double_ref_double_(double a, void* __context);
void bool__bool_void_ref_bool_(bool a, bool* __context);
void char__char_void_ref_char_(char a, char* __context);
void short__short_void_ref_short_(short a, short* __context);
void int__int_void_ref_int_(int a, int* __context);
void long__long_void_ref_long_(long long a, long long* __context);
void float__float_void_ref_float_(float a, float* __context);
void double__double_void_ref_double_(double a, double* __context);
int as_unsigned_int_ref_char_(char a, void* __context);
int as_unsigned_int_ref_short_(short a, void* __context);
long long as_unsigned_long_ref_int_(int a, void* __context);
short short_short_ref_char___(NULLCArray<char> str, void* __context);
void short__short_void_ref_char___(NULLCArray<char> str, short* __context);
NULLCArray<char> short__str_char___ref__(short* __context);
int int_int_ref_char___(NULLCArray<char> str, void* __context);
void int__int_void_ref_char___(NULLCArray<char> str, int* __context);
NULLCArray<char> int__str_char___ref__(int* __context);
long long long_long_ref_char___(NULLCArray<char> str, void* __context);
void long__long_void_ref_char___(NULLCArray<char> str, long long* __context);
NULLCArray<char> long__str_char___ref__(long long* __context);
float float_float_ref_char___(NULLCArray<char> str, void* __context);
void float__float_void_ref_char___(NULLCArray<char> str, float* __context);
NULLCArray<char> float__str_char___ref_int_bool_(int precision, bool showExponent, float* __context);
double double_double_ref_char___(NULLCArray<char> str, void* __context);
void double__double_void_ref_char___(NULLCArray<char> str, double* __context);
NULLCArray<char> double__str_char___ref_int_bool_(int precision, bool showExponent, double* __context);
void* __newS_void_ref_ref_int_int_(int size, int type, void* __context);
NULLCArray<int> __newA_int___ref_int_int_int_(int size, int count, int type, void* __context);
NULLCRef duplicate_auto_ref_ref_auto_ref_(NULLCRef obj, void* __context);
void __duplicate_array_void_ref_auto___ref_auto___(NULLCAutoArray* dst, NULLCAutoArray src, void* __context);
NULLCAutoArray duplicate_auto___ref_auto___(NULLCAutoArray arr, void* __context);
NULLCRef replace_auto_ref_ref_auto_ref_auto_ref_(NULLCRef l, NULLCRef r, void* __context);
void swap_void_ref_auto_ref_auto_ref_(NULLCRef l, NULLCRef r, void* __context);
int equal_int_ref_auto_ref_auto_ref_(NULLCRef l, NULLCRef r, void* __context);
void assign_void_ref_auto_ref_auto_ref_(NULLCRef l, NULLCRef r, void* __context);
void array_copy_void_ref_auto___auto___(NULLCAutoArray l, NULLCAutoArray r, void* __context);
NULLCFuncPtr<__typeProxy_void_ref__> __redirect_void_ref___ref_auto_ref___function___ref_(NULLCRef r, NULLCArray<__function>* f, void* __context);
NULLCFuncPtr<__typeProxy_void_ref__> __redirect_ptr_void_ref___ref_auto_ref___function___ref_(NULLCRef r, NULLCArray<__function>* f, void* __context);
NULLCArray<char>* __operatorSet_char___ref_ref_char___ref_int___(NULLCArray<char>* dst, NULLCArray<int> src, void* __context);
NULLCArray<short>* __operatorSet_short___ref_ref_short___ref_int___(NULLCArray<short>* dst, NULLCArray<int> src, void* __context);
NULLCArray<float>* __operatorSet_float___ref_ref_float___ref_double___(NULLCArray<float>* dst, NULLCArray<double> src, void* __context);
unsigned typeid_typeid_ref_auto_ref_(NULLCRef type, void* __context);
int typeid__size__int_ref___(unsigned* __context);
int __operatorEqual_int_ref_typeid_typeid_(unsigned a, unsigned b, void* __context);
int __operatorNEqual_int_ref_typeid_typeid_(unsigned a, unsigned b, void* __context);
int __rcomp_int_ref_auto_ref_auto_ref_(NULLCRef a, NULLCRef b, void* __context);
int __rncomp_int_ref_auto_ref_auto_ref_(NULLCRef a, NULLCRef b, void* __context);
bool __operatorLess_bool_ref_auto_ref_auto_ref_(NULLCRef a, NULLCRef b, void* __context);
bool __operatorLEqual_bool_ref_auto_ref_auto_ref_(NULLCRef a, NULLCRef b, void* __context);
bool __operatorGreater_bool_ref_auto_ref_auto_ref_(NULLCRef a, NULLCRef b, void* __context);
bool __operatorGEqual_bool_ref_auto_ref_auto_ref_(NULLCRef a, NULLCRef b, void* __context);
int hash_value_int_ref_auto_ref_(NULLCRef x, void* __context);
int __pcomp_int_ref_void_ref_int__void_ref_int__(NULLCFuncPtr<__typeProxy_void_ref_int_> a, NULLCFuncPtr<__typeProxy_void_ref_int_> b, void* __context);
int __pncomp_int_ref_void_ref_int__void_ref_int__(NULLCFuncPtr<__typeProxy_void_ref_int_> a, NULLCFuncPtr<__typeProxy_void_ref_int_> b, void* __context);
int __acomp_int_ref_auto___auto___(NULLCAutoArray a, NULLCAutoArray b, void* __context);
int __ancomp_int_ref_auto___auto___(NULLCAutoArray a, NULLCAutoArray b, void* __context);
int __typeCount_int_ref__(void* __context);
NULLCAutoArray* __operatorSet_auto___ref_ref_auto___ref_auto_ref_(NULLCAutoArray* l, NULLCRef r, void* __context);
NULLCRef __aaassignrev_auto_ref_ref_auto_ref_auto___ref_(NULLCRef l, NULLCAutoArray* r, void* __context);
NULLCRef __operatorIndex_auto_ref_ref_auto___ref_int_(NULLCAutoArray* l, int index, void* __context);
int const_string__size__int_ref___(const_string* __context);
const_string* __operatorSet_const_string_ref_ref_const_string_ref_char___(const_string* l, NULLCArray<char> arr, void* __context);
const_string const_string_const_string_ref_char___(NULLCArray<char> arr, void* __context);
char __operatorIndex_char_ref_const_string_ref_int_(const_string* l, int index, void* __context);
int __operatorEqual_int_ref_const_string_const_string_(const_string a, const_string b, void* __context);
int __operatorEqual_int_ref_const_string_char___(const_string a, NULLCArray<char> b, void* __context);
int __operatorEqual_int_ref_char___const_string_(NULLCArray<char> a, const_string b, void* __context);
int __operatorNEqual_int_ref_const_string_const_string_(const_string a, const_string b, void* __context);
int __operatorNEqual_int_ref_const_string_char___(const_string a, NULLCArray<char> b, void* __context);
int __operatorNEqual_int_ref_char___const_string_(NULLCArray<char> a, const_string b, void* __context);
const_string __operatorAdd_const_string_ref_const_string_const_string_(const_string a, const_string b, void* __context);
const_string __operatorAdd_const_string_ref_const_string_char___(const_string a, NULLCArray<char> b, void* __context);
const_string __operatorAdd_const_string_ref_char___const_string_(NULLCArray<char> a, const_string b, void* __context);
int isStackPointer_int_ref_auto_ref_(NULLCRef x, void* __context);
void auto_array_impl_void_ref_auto___ref_typeid_int_(NULLCAutoArray* arr, unsigned type, int count, void* __context);
NULLCAutoArray auto_array_auto___ref_typeid_int_(unsigned type, int count, void* __context);
void auto____set_void_ref_auto_ref_int_(NULLCRef x, int pos, NULLCAutoArray* __context);
void __force_size_void_ref_auto___ref_int_(NULLCAutoArray* s, int size, void* __context);
int isCoroutineReset_int_ref_auto_ref_(NULLCRef f, void* __context);
void __assertCoroutine_void_ref_auto_ref_(NULLCRef f, void* __context);
NULLCArray<NULLCRef> __getFinalizeList_auto_ref___ref__(void* __context);
void __FinalizeProxy__finalize_void_ref__(__FinalizeProxy* __context);
void __finalizeObjects_void_ref__(void* __context);
void* assert_derived_from_base_void_ref_ref_void_ref_typeid_(void* derived, unsigned base, void* __context);
void __closeUpvalue_void_ref_void_ref_ref_void_ref_int_int_(void** l, void* v, int offset, int size, void* __context);
int float__str_610894668_precision__int_ref___(void* __context);
bool float__str_610894668_showExponent__bool_ref___(void* __context);
int double__str_610894668_precision__int_ref___(void* __context);
bool double__str_610894668_showExponent__bool_ref___(void* __context);

inline unsigned int	__nullcIndex(unsigned int index, unsigned int size)
{
	assert_void_ref_int_(index < size, 0);
	return index;
}

void nullcThrowError(const char* error, ...);
unsigned __nullcRegisterType(unsigned hash, const char *name, unsigned size, unsigned subTypeID, int memberCount, unsigned category, unsigned alignment, unsigned flags);
void __nullcRegisterMembers(unsigned id, unsigned count, ...);
unsigned __nullcGetTypeCount();
NULLCTypeInfo* __nullcGetTypeInfo(unsigned id);
NULLCMemberInfo* __nullcGetTypeMembers(unsigned id);

unsigned int typeid__(NULLCRef type, void* unused);

unsigned int __nullcGetStringHash(const char *str);

typedef void* __nullcFunction;
typedef __nullcFunction* __nullcFunctionArray;
__nullcFunctionArray* __nullcGetFunctionTable();
unsigned __nullcRegisterFunction(const char* name, void* fPtr, unsigned extraType, unsigned funcType, bool unique);
NULLCFuncInfo* __nullcGetFunctionInfo(unsigned id);

void __nullcRegisterGlobal(void* ptr, unsigned typeID);
void __nullcRegisterBase(void* ptr);

namespace NULLC
{
	void*		AllocObject(int size, unsigned typeID);
	NULLCArray<int>	AllocArray(int size, int count, unsigned typeID);
	NULLCRef	CopyObject(NULLCRef ptr);

	void		MarkMemory(unsigned int number);

	bool		IsBasePointer(void* ptr);
	void*		GetBasePointer(void* ptr);

	void		CollectMemory();
	unsigned int	UsedMemory();
	double		MarkTime();
	double		CollectTime();

	void		FinalizeMemory();
}

inline double __nullcZero()
{
	return 0.0;
}

int	__nullcOutputResultInt(int x);
int	__nullcOutputResultLong(long long x);
int	__nullcOutputResultDouble(double x);

template<typename T>
T* __nullcIndexUnsizedArray(const NULLCArray<T>& arr, int index, int elemSize)
{
	if(unsigned(index) < unsigned(arr.size))
		return (T*)(arr.ptr + index * elemSize);

	nullcThrowError("ERROR: array index out of bounds");
	return 0;
}

template<typename T> T __nullcCheckedRet(unsigned typeID, T data, void* top, ...)
{
	char arr[sizeof(T) ? -1 : 0];
}

#include <stdarg.h>

template<typename T> T* __nullcCheckedRet(unsigned typeID, T* data, void* top, ...)
{
	void *maxp = top;
	va_list argptr;
	va_start(argptr, top);
	while(void *next = va_arg(argptr, void*))
	{
		if(next > maxp)
			maxp = next;
	}
	if(data <= maxp && (char*)data >= ((char*)maxp - (1024 * 1024))) // assume 1Mb for stack -_-
	{
		unsigned int objSize = (unsigned)typeid__size__int_ref___(&typeID);
		char *copy = (char*)NULLC::AllocObject(objSize, typeID);
		memcpy(copy, data, objSize);
		return (T*)copy;
	}
	return data;
}
template<typename T> NULLCArray<T> __nullcCheckedRet(unsigned typeID, NULLCArray<T> data, void* top, ...)
{
	void *maxp = top;
	va_list argptr;
	va_start(argptr, top);
	while(void *next = va_arg(argptr, void*))
	{
		if(next > maxp)
			maxp = next;
	}
	if(data.ptr <= maxp && data.ptr >= ((char*)maxp - (1024 * 1024))) // assume 1Mb for stack -_-
	{
		unsigned int objSize = __nullcGetTypeInfo(__nullcGetTypeInfo(typeID)->subTypeID)->size;
		NULLCArray<T> copy = NULLC::AllocArray(objSize, data.size, __nullcGetTypeInfo(typeID)->subTypeID);
		memcpy(copy.ptr, data.ptr, objSize * data.size);
		return copy;
	}
	return data;
}

int __nullcInitBaseModule();
