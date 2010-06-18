#include "runtime.h"
#include <memory>
#include <math.h>
#include <stdarg.h>
#undef assert
#ifdef _MSC_VER
	#include <assert.h>
	#define __assert(_Expression) (void)( (!!(_Expression)) || (_wassert(_CRT_WIDE(#_Expression), _CRT_WIDE(__FILE__), __LINE__), 0) )
#else
#define __assert (void)0;
#endif

template<typename T>
class FastVector
{
public:
	FastVector()
	{
		data = (T*)malloc(sizeof(T) * 128);
		memset(data, 0, sizeof(T));
		max = 128;
		count = 0;
	}
	explicit FastVector(unsigned int reserved)
	{
		data = (T*)malloc(sizeof(T) * reserved);
		memset(data, 0, reserved * sizeof(T));
		max = reserved;
		count = 0;
	}
	~FastVector()
	{
		free(data);
	}

	T*		push_back()
	{
		count++;
		if(count == max)
			grow(count);
		return &data[count - 1];
	};
	void		push_back(const T& val)
	{
		data[count++] = val;
		if(count == max)
			grow(count);
	};
	void		push_back(const T* valPtr, unsigned int elem)
	{
		if(count + elem >= max)
			grow(count + elem);
		for(unsigned int i = 0; i < elem; i++)
			data[count++] = valPtr[i];
	};
	T&		back()
	{
		return data[count-1];
	}
	unsigned int		size()
	{
		return count;
	}
	void		pop_back()
	{
		count--;
	}
	void		clear()
	{
		count = 0;
	}
	T&		operator[](unsigned int index)
	{
		return data[index];
	}
	void		resize(unsigned int newSize)
	{
		if(newSize >= max)
			grow(newSize);
		count = newSize;
	}
	void		shrink(unsigned int newSize)
	{
		count = newSize;
	}
	void		reserve(unsigned int resSize)
	{
		if(resSize >= max)
			grow(resSize);
	}

	void	grow(unsigned int newSize)
	{
		if(max + (max >> 1) > newSize)
			newSize = max + (max >> 1);
		else
			newSize += 32;
		T* newData;
		newData = (T*)malloc(sizeof(T) * newSize);
		memset(newData, 0, newSize * sizeof(T));
		memcpy(newData, data, max * sizeof(T));
		free(data);
		data = newData;
		max = newSize;
	}
	T	*data;
	T	one;
	unsigned int	max, count;
private:
	// Disable assignment and copy constructor
	void operator =(FastVector &r);
	FastVector(FastVector &r);
};

int			__nullcPow(int a, int b)
{
	return (int)pow((double)a, (double)b);
}

double		__nullcPow(double a, double b)
{
	return pow(a, b);
}

long long	__nullcPow(long long num, long long pow)
{
	if(pow < 0)
		return (num == 1 ? 1 : 0);
	if(pow == 0)
		return 1;
	if(pow == 1)
		return num;
	if(pow > 64)
		return num;
	long long res = 1;
	int power = (int)pow;
	while(power)
	{
		if(power & 0x01)
		{
			res *= num;
			power--;
		}
		num *= num;
		power >>= 1;
	}
	return res;
}

double		__nullcMod(double a, double b)
{
	return fmod(a, b);
}
int			__nullcPowSet(char *a, int b)
{
	return *a = (char)__nullcPow((int)*a, b);
}
int			__nullcPowSet(short *a, int b)
{
	return *a = (short)__nullcPow((int)*a, b);
}
int			__nullcPowSet(int *a, int b)
{
	return *a = __nullcPow(*a, b);
}
double		__nullcPowSet(float *a, double b)
{
	return *a = (float)__nullcPow((double)*a, b);
}
double		__nullcPowSet(double *a, double b)
{
	return *a = __nullcPow(*a, b);
}
long long	__nullcPowSet(long long *a, long long b)
{
	return *a = __nullcPow(*a, b);
}
void	__nullcSetArray(short arr[], short val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}
void	__nullcSetArray(int arr[], int val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}
void	__nullcSetArray(float arr[], float val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}
void	__nullcSetArray(double arr[], double val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}
void	__nullcSetArray(long long arr[], long long val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}

void __nullcCloseUpvalue(__nullcUpvalue *&head, void *ptr)
{
	__nullcUpvalue *curr = head;
	while(curr && (char*)curr->ptr <= ptr)
	{
		__nullcUpvalue *next = curr->next;
		unsigned int size = curr->size;
		head = curr->next;
		memcpy(&curr->next, curr->ptr, size);
		curr->ptr = (unsigned int*)&curr->next;
		curr = next;
	}
}
NULLCFuncPtr	__nullcMakeFunction(void* ptr, void* context)
{
	NULLCFuncPtr ret;
	ret.ptr = ptr;
	ret.context = context;
	return ret;
}

NULLCRef		__nullcMakeAutoRef(void* ptr, unsigned int typeID)
{
	NULLCRef ret;
	ret.ptr = (char*)ptr;
	ret.typeID = typeID;
	return ret;
}
void*			__nullcGetAutoRef(const NULLCRef &ref, unsigned int typeID)
{
	__assert(ref.typeID == typeID);
	return (void*)ref.ptr;
}

bool operator ==(const NULLCRef& a, const NULLCRef& b)
{
	return a.ptr == b.ptr && a.typeID == b.typeID;
}
bool operator !=(const NULLCRef& a, const NULLCRef& b)
{
	return a.ptr != b.ptr || a.typeID != b.typeID;
}
bool operator !(const NULLCRef& a)
{
	return !a.ptr;
}

int  __operatorEqual(unsigned int a, unsigned int b, void* unused)
{
	return a == b;
}
int  __operatorNEqual(unsigned int a, unsigned int b, void* unused)
{
	return a != b;
}

void  assert(int val, void* unused)
{
	__assert(val);
}

void  assert(int val, NULLCArray<char> message, void* unused)
{
	__assert(val);
}

int  __operatorEqual(NULLCArray<char> a, NULLCArray<char> b, void* unused)
{
	if(a.size != b.size)
		return 0;
	if(memcmp(a.ptr, b.ptr, a.size) == 0)
		return 1;
	return 0;
}

int  __operatorNEqual(NULLCArray<char> a, NULLCArray<char> b, void* unused)
{
	return !__operatorEqual(a, b, 0);
}

NULLCArray<char>  __operatorAdd(NULLCArray<char> a, NULLCArray<char> b, void* unused)
{
	NULLCArray<char> ret;

	ret.size = a.size + b.size - 1;
	ret.ptr = (char*)(intptr_t)__newS(ret.size, 0);
	if(!ret.ptr)
		return ret;

	memcpy(ret.ptr, a.ptr, a.size);
	memcpy(ret.ptr + a.size - 1, b.ptr, b.size);

	return ret;
}

NULLCArray<char>  __operatorAddSet(NULLCArray<char> * a, NULLCArray<char> b, void* unused)
{
	return *a = __operatorAdd(*a, b, 0);
}

char  char__(char a, void* unused)
{
	return a;
}

short  short__(short a, void* unused)
{
	return a;
}

int  int__(int a, void* unused)
{
	return a;
}

long  long__(long a, void* unused)
{
	return a;
}

float  float__(float a, void* unused)
{
	return a;
}

double  double__(double a, void* unused)
{
	return a;
}

NULLCArray<char>  int__str(int* __context)
{
	NULLCArray<char> ret;
	return ret;
}

int  __newS(int size, void* unused)
{
	void *ptr = malloc(size);
	memset(ptr, 0, size);
	return (int)(intptr_t)ptr;
}

NULLCArray<void>  __newA(int size, int count, void* unused)
{
	NULLCArray<void> ret;
	ret.size = count;
	ret.ptr = (char*)malloc(size * count);
	memset(ret.ptr, 0, size * count);
	return ret;
}

NULLCRef  duplicate(NULLCRef obj, void* unused)
{
	NULLCRef ret;
	return ret;
}

void nullcThrowError(const char* error, ...)
{
	va_list args;
	va_start(args, error);
	vprintf(error, args);
	va_end(args);
}

FastVector<NULLCTypeInfo>	__nullcTypeList;
unsigned __nullcRegisterType(unsigned hash, const char *name, unsigned size, unsigned subTypeID, int memberCount, unsigned category)
{
	for(unsigned int i = 0; i < __nullcTypeList.size(); i++)
	{
		if(__nullcTypeList[i].hash == hash)
			return i;
	}
	__nullcTypeList.push_back(NULLCTypeInfo());
	__nullcTypeList.back().hash = hash;
	__nullcTypeList.back().name = name;
	__nullcTypeList.back().size = size;
	__nullcTypeList.back().subTypeID = subTypeID;
	__nullcTypeList.back().memberCount = memberCount;
	__nullcTypeList.back().category = category;
	return __nullcTypeList.size() - 1;
}

NULLCTypeInfo* __nullcGetTypeInfo(unsigned id)
{
	return &__nullcTypeList[id];
}

unsigned int typeid__(NULLCRef type, void* unused)
{
	return type.typeID;
}

int __pcomp(NULLCFuncPtr a, NULLCFuncPtr b, void* unused)
{
	return a.context == b.context && a.ptr == b.ptr;
}
int __pncomp(NULLCFuncPtr a, NULLCFuncPtr b, void* unused)
{
	return a.context != b.context || a.ptr != b.ptr;
}
