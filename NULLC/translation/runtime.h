#include <memory.h>

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
typedef struct
{
	unsigned int	typeID;
	char			*ptr;
} NULLCRef;

// Wrapper over NULLC function pointer for use in external functions
typedef struct
{
	void	*context;
	void	*ptr;
} NULLCFuncPtr;

typedef struct
{
	unsigned int	hash;
	const char		*name;
	unsigned int	size;
	unsigned int	subTypeID;
	int				memberCount;
	unsigned int	category;
} NULLCTypeInfo;

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
NULLCFuncPtr	__nullcMakeFunction(void* ptr, void* context);
NULLCRef		__nullcMakeAutoRef(void* ptr, unsigned int typeID);
void*			__nullcGetAutoRef(const NULLCRef &ref, unsigned int typeID);

bool operator ==(const NULLCFuncPtr& a, const NULLCFuncPtr& b);
bool operator ==(const NULLCRef& a, const NULLCRef& b);
bool operator !(const NULLCRef& a);

#undef assert
void  assert(int val, void* unused);
void  assert(int val, NULLCArray<char> message, void* unused);
int  __operatorEqual(NULLCArray<char> a, NULLCArray<char> b, void* unused);
int  __operatorNEqual(NULLCArray<char> a, NULLCArray<char> b, void* unused);
NULLCArray<char>  __operatorAdd(NULLCArray<char> a, NULLCArray<char> b, void* unused);
NULLCArray<char>  __operatorAddSet(NULLCArray<char> * a, NULLCArray<char> b, void* unused);
char  char__(char a, void* unused);
short  short__(short a, void* unused);
int  int__(int a, void* unused);
long  long__(long a, void* unused);
float  float__(float a, void* unused);
double  double__(double a, void* unused);
NULLCArray<char>  int__str(int* __context);
int  __newS(int size, void* unused);
NULLCArray<void>  __newA(int size, int count, void* unused);
NULLCRef  duplicate(NULLCRef obj, void* unused);

inline unsigned int	__nullcIndex(unsigned int index, unsigned int size)
{
	assert(index < size, 0);
	return index;
}

void nullcThrowError(const char* error, ...);
unsigned __nullcRegisterType(unsigned hash, const char *name, unsigned size, unsigned subTypeID, int memberCount, unsigned category);
NULLCTypeInfo* __nullcGetTypeInfo(unsigned id);
