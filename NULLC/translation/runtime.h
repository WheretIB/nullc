#include <memory.h>
#include <string.h>
#ifndef NULL
	#define NULL 0
#endif

// Wrapper over NULLC array, for use in external functions
template<typename T>
struct NULLCArray
{
	char			*ptr;
	unsigned int	size;
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
typedef struct
{
	void	*context;
	unsigned int id;
} NULLCFuncPtr;

// Wrapper over NULLC auto[] class for use in external functions
typedef struct
{
	unsigned int	typeID;
	char			*ptr;
	unsigned int	len;
} NULLCAutoArray;

typedef struct
{
	unsigned int	hash;
	const char		*name;
	unsigned int	size;
	unsigned int	subTypeID;
	int				memberCount;
	unsigned int	category;
	unsigned int	members;

} NULLCTypeInfo;

#define NULLC_NONE 0
#define NULLC_CLASS 1
#define NULLC_ARRAY 2
#define NULLC_POINTER 3
#define NULLC_FUNCTION 4

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
#ifdef _MSC_VER
	if(zero != 0 || unused != 0)
		__asm int 3;
#endif
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

struct __nullcUpvalue
{
	void *ptr;
	__nullcUpvalue *next;
	unsigned int size;
};
void __nullcCloseUpvalue(__nullcUpvalue *&head, void *ptr);
NULLCFuncPtr	__nullcMakeFunction(unsigned int id, void* context);
NULLCRef		__nullcMakeAutoRef(void* ptr, unsigned int typeID);
void*			__nullcGetAutoRef(const NULLCRef &ref, unsigned int typeID);

bool operator ==(const NULLCRef& a, const NULLCRef& b);
bool operator !=(const NULLCRef& a, const NULLCRef& b);
bool operator !(const NULLCRef& a);

int  __operatorEqual(unsigned int a, unsigned int b, void* unused);
int  __operatorNEqual(unsigned int a, unsigned int b, void* unused);

#undef assert
void  assert(int val, void* unused);
void  assert(int val, const char* message, void* unused);
void  assert(int val, NULLCArray<char> message, void* unused);
int  __operatorEqual(NULLCArray<char> a, NULLCArray<char> b, void* unused);
int  __operatorNEqual(NULLCArray<char> a, NULLCArray<char> b, void* unused);
NULLCArray<char>  __operatorAdd(NULLCArray<char> a, NULLCArray<char> b, void* unused);
NULLCArray<char>  __operatorAddSet(NULLCArray<char> * a, NULLCArray<char> b, void* unused);
char  char__(char a, void* unused);
short  short__(short a, void* unused);
int  int__(int a, void* unused);
long long  long__(long long a, void* unused);
float  float__(float a, void* unused);
double  double__(double a, void* unused);
void char__char_void_ref_char_(char a, char *target);
void short__short_void_ref_short_(short a, short *target);
void int__int_void_ref_int_(int a, int *target);
void long__long_void_ref_long_(long long a, long long *target);
void float__float_void_ref_float_(float a, float *target);
void double__double_void_ref_double_(double a, double *target);
NULLCArray<char>  int__str_char___ref__(int* __context);

int  __newS(int size, unsigned typeID);
NULLCArray<void>  __newA(int size, int count, unsigned typeID);
NULLCRef  duplicate(NULLCRef obj, void* unused);

// const string implementation
struct const_string
{
	NULLCArray<char> arr;
};

int const_string__size__int_ref__(const_string* str);
const_string const_string__(NULLCArray<char> arr, void* unused);

char __char_a_12();
short __short_a_13();
int __int_a_14();
long long __long_a_15();
float __float_a_16();
double __double_a_17();

inline unsigned int	__nullcIndex(unsigned int index, unsigned int size)
{
	assert(index < size, 0);
	return index;
}

void nullcThrowError(const char* error, ...);
unsigned __nullcRegisterType(unsigned hash, const char *name, unsigned size, unsigned subTypeID, int memberCount, unsigned category);
void __nullcRegisterMembers(unsigned id, unsigned count, ...);
NULLCTypeInfo* __nullcGetTypeInfo(unsigned id);

unsigned int typeid__(NULLCRef type, void* unused);

int __pcomp(NULLCFuncPtr a, NULLCFuncPtr b, void* unused);
int __pncomp(NULLCFuncPtr a, NULLCFuncPtr b, void* unused);

int __typeCount(void* unused);

NULLCAutoArray* __operatorSet(NULLCAutoArray* l, NULLCRef r, void* unused);
NULLCRef __operatorSet(NULLCRef l, NULLCAutoArray* r, void* unused);
NULLCAutoArray* __operatorSet(NULLCAutoArray* l, NULLCAutoArray* r, void* unused);
NULLCRef __operatorIndex(NULLCAutoArray* l, unsigned int index, void* unused);

NULLCFuncPtr __redirect(NULLCRef r, NULLCArray<int>* f, void* unused);

// char inline array definition support
NULLCArray<char>* __operatorSet(NULLCArray<char>* dst, NULLCArray<int> src, void* unused);
// short inline array definition support
NULLCArray<short>* __operatorSet(NULLCArray<short>* dst, NULLCArray<int> src, void* unused);
// float inline array definition support
NULLCArray<float>* __operatorSet(NULLCArray<float>* dst, NULLCArray<double> src, void* unused);

typedef void* __nullcFunction;
typedef __nullcFunction* __nullcFunctionArray;
__nullcFunctionArray* __nullcGetFunctionTable();
unsigned __nullcRegisterFunction(const char* name, void* fPtr, unsigned extraType);

void __nullcRegisterGlobal(void* ptr, unsigned typeID);
void __nullcRegisterBase(void* ptr);

namespace NULLC
{
	void*		AllocObject(int size, unsigned typeID);
	NULLCArray<void>	AllocArray(int size, int count, unsigned typeID);
	NULLCRef	CopyObject(NULLCRef ptr);

	void		MarkMemory(unsigned int number);

	bool		IsBasePointer(void* ptr);
	void*		GetBasePointer(void* ptr);

	void		CollectMemory();
	unsigned int	UsedMemory();
	double		MarkTime();
	double		CollectTime();
}
