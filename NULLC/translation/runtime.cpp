#include "runtime.h"
#include <memory>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#undef assert
#define __assert(_Expression) if(!(_Expression)){ printf("assertion failed"); abort(); };

int	SafeSprintf(char* dst, size_t size, const char* src, ...)
{
	va_list args;
	va_start(args, src);

	int result = vsnprintf(dst, size, src, args);
	dst[size-1] = '\0';
	return (result == -1 || (size_t)result >= size) ? (int)size : result;
}

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
	void	reset()
	{
		free(data);
		data = (T*)malloc(sizeof(T) * 128);
		memset(data, 0, sizeof(T));
		max = 128;
		count = 0;
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
	unsigned int	max, count;
private:
	// Disable assignment and copy constructor
	void operator =(FastVector &r);
	FastVector(FastVector &r);
};

FastVector<NULLCTypeInfo>	__nullcTypeList;
FastVector<unsigned int>	__nullcTypePart;

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
	__nullcTypeList.back().members = 0;
	return __nullcTypeList.size() - 1;
}
void __nullcRegisterMembers(unsigned id, unsigned count, ...)
{
	if(__nullcTypeList[id].members || !count)
		return;
	va_list args;
	va_start(args, count);
	__nullcTypeList[id].members = __nullcTypePart.size();
	for(unsigned i = 0; i < count * 2; i++)
	{
		__nullcTypePart.push_back(va_arg(args, int));
		__nullcTypePart.push_back(va_arg(args, int));
	}
	va_end(args);
}

NULLCTypeInfo* __nullcGetTypeInfo(unsigned id)
{
	return &__nullcTypeList[id];
}
bool nullcIsArray(unsigned int typeID)
{
	return __nullcGetTypeInfo(typeID)->category == NULLC_ARRAY;
}

const char* nullcGetTypeName(unsigned int typeID)
{
	return __nullcGetTypeInfo(typeID)->name;
}

unsigned int nullcGetArraySize(unsigned int typeID)
{
	return __nullcGetTypeInfo(typeID)->memberCount;
}
unsigned int nullcGetSubType(unsigned int typeID)
{
	return __nullcGetTypeInfo(typeID)->subTypeID;
}
unsigned int nullcGetTypeSize(unsigned int typeID)
{
	return __nullcGetTypeInfo(typeID)->size;
}

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

namespace GC
{
	// Range of memory that is not checked. Used to exclude pointers to stack from marking and GC
	char	*unmanageableBase = NULL;
	char	*unmanageableTop = NULL;
}

void __nullcCloseUpvalue(__nullcUpvalue *&head, void *ptr)
{
	__nullcUpvalue *curr = head;
	
	GC::unmanageableBase = (char*)&curr;
	// close upvalue if it's target is equal to local variable, or it's address is out of stack
	while(curr && ((char*)curr->ptr == ptr || (char*)curr->ptr < GC::unmanageableBase || (char*)curr->ptr > GC::unmanageableTop))
	{
		__nullcUpvalue *next = curr->next;
		unsigned int size = curr->size;
		head = curr->next;
		memcpy(&curr->next, curr->ptr, size);
		curr->ptr = (unsigned int*)&curr->next;
		curr = next;
	}
}
NULLCFuncPtr	__nullcMakeFunction(unsigned int id, void* context)
{
	NULLCFuncPtr ret;
	ret.id = id;
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

void  assert(int val, const char* message, void* unused)
{
	if(!val)
		printf("%s\n", message);
	__assert(val);
}

void  assert(int val, NULLCArray<char> message, void* unused)
{
	if(!val)
		printf("%s\n", message.ptr);
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

long long  long__(long long a, void* unused)
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
void char__char_void_ref_char_(char a, char *target)
{
	*target = a;
}
void short__short_void_ref_short_(short a, short *target)
{
	*target = a;
}
void int__int_void_ref_int_(int a, int *target)
{
	*target = a;
}
void long__long_void_ref_long_(long long a, long long *target)
{
	*target = a;
}
void float__float_void_ref_float_(float a, float *target)
{
	*target = a;
}
void double__double_void_ref_double_(double a, double *target)
{
	*target = a;
}

int const_string__size__int_ref__(const_string* str)
{
	return str->arr.size;
}
const_string const_string__(NULLCArray<char> arr, void* unused)
{
	const_string str;
	str.arr = arr;
	return str;
}

char __char_a_12()
{
	return 0;
}
short __short_a_13()
{
	return 0;
}
int __int_a_14()
{
	return 0;
}
long long __long_a_15()
{
	return 0;
}
float __float_a_16()
{
	return 0;
}
double __double_a_17()
{
	return 0;
}

NULLCArray<char>  int__str_char___ref__(int* r)
{
	int number = *r;
	bool sign = 0;
	char buf[16];
	char *curr = buf;
	if(number < 0)
		sign = 1;

	*curr++ = (char)(abs(number % 10) + '0');
	while(number /= 10)
		*curr++ = (char)(abs(number % 10) + '0');
	if(sign)
		*curr++ = '-';
	NULLCArray<char> arr = __newA(1, (int)(curr - buf) + 1, 0);
	char *str = arr.ptr;
	do 
	{
		--curr;
		*str++ = *curr;
	}while(curr != buf);
	return arr;
}
NULLCArray<char>  double__str_char___ref_int_(int precision, int* r)
{
	char buf[256];
	SafeSprintf(buf, 256, "%.*f", precision, *r);
	NULLCArray<char> arr = __newA(1, (int)strlen(buf) + 1, 0);
	memcpy(arr.ptr, buf, arr.size);
	return arr;
}

NULLCRef  duplicate(NULLCRef obj, void* unused)
{
	NULLCRef ret;
	ret.typeID = obj.typeID;
	unsigned int objSize = nullcGetTypeSize(ret.typeID);
	ret.ptr = (char*)__newS(objSize, 0);
	memcpy(ret.ptr, obj.ptr, objSize);
	return ret;
}

NULLCRef replace(NULLCRef l, NULLCRef r, void* unused)
{
	if(l.typeID != r.typeID)
	{
		nullcThrowError("ERROR: cannot convert from %s ref to %s ref", __nullcGetTypeInfo(r.typeID)->name, __nullcGetTypeInfo(l.typeID)->name);
		return l;
	}
	memcpy(l.ptr, r.ptr, __nullcGetTypeInfo(r.typeID)->size);
	return l;
}

int __rcomp(NULLCRef a, NULLCRef b)
{
	return a.ptr == b.ptr;
}

int __rncomp(NULLCRef a, NULLCRef b)
{
	return a.ptr != b.ptr;
}

void nullcThrowError(const char* error, ...)
{
	va_list args;
	va_start(args, error);
	vprintf(error, args);
	va_end(args);
}

unsigned int typeid__(NULLCRef type, void* unused)
{
	return type.typeID;
}

int __pcomp(NULLCFuncPtr a, NULLCFuncPtr b, void* unused)
{
	return a.context == b.context && a.id == b.id;
}
int __pncomp(NULLCFuncPtr a, NULLCFuncPtr b, void* unused)
{
	return a.context != b.context || a.id != b.id;
}

int __typeCount(void* unused)
{
	return __nullcTypeList.size();
}

NULLCAutoArray* __operatorSet(NULLCAutoArray* left, NULLCRef right, void* unused)
{
	if(!nullcIsArray(right.typeID))
	{
		nullcThrowError("ERROR: cannot convert from '%s' to 'auto[]'", nullcGetTypeName(right.typeID));
		return NULL;
	}
	left->len = nullcGetArraySize(right.typeID);
	if(left->len == ~0u)
	{
		NULLCArray<char> *arr = (NULLCArray<char>*)right.ptr;
		left->len = arr->size;
		left->ptr = arr->ptr;
	}else{
		left->ptr = right.ptr;
	}
	left->typeID = nullcGetSubType(right.typeID);
	return left;
}
NULLCRef __operatorSet(NULLCRef left, NULLCAutoArray *right, void* unused)
{
	NULLCRef ret = { 0, 0 };
	if(!nullcIsArray(left.typeID))
	{
		nullcThrowError("ERROR: cannot convert from 'auto[]' to '%s'", nullcGetTypeName(left.typeID));
		return ret;
	}
	if(nullcGetSubType(left.typeID) != right->typeID)
	{
		nullcThrowError("ERROR: cannot convert from 'auto[]' (actual type '%s[%d]') to '%s'", nullcGetTypeName(right->typeID), right->len, nullcGetTypeName(left.typeID));
		return ret;
	}
	unsigned int leftLength = nullcGetArraySize(left.typeID);
	if(leftLength == ~0u)
	{
		NULLCArray<char> *arr = (NULLCArray<char>*)left.ptr;
		arr->size = right->len;
		arr->ptr = right->ptr;
	}else{
		if(leftLength != right->len)
		{
			nullcThrowError("ERROR: cannot convert from 'auto[]' (actual type '%s[%d]') to '%s'", nullcGetTypeName(right->typeID), right->len, nullcGetTypeName(left.typeID));
			return ret;
		}
		memcpy(left.ptr, right->ptr, leftLength * nullcGetTypeSize(right->typeID));
	}
	return left;
}
NULLCAutoArray* __operatorSet(NULLCAutoArray* left, NULLCAutoArray* right, void* unused)
{
	left->len = right->len;
	left->ptr = right->ptr;
	left->typeID = right->typeID;
	return left;
}
NULLCRef __operatorIndex(NULLCAutoArray* left, unsigned int index, void* unused)
{
	NULLCRef ret = { 0, 0 };
	if(index >= left->len)
	{
		nullcThrowError("ERROR: array index out of bounds");
		return ret;
	}
	ret.typeID = left->typeID;
	ret.ptr = (char*)left->ptr + index * nullcGetTypeSize(ret.typeID);
	return ret;
}

NULLCFuncPtr __redirect(NULLCRef r, NULLCArray<int>* arr, void* unused)
{
	unsigned int *funcs = (unsigned int*)arr->ptr;
	NULLCFuncPtr ret = { 0, 0 };
	if(r.typeID > arr->size)
	{
		nullcThrowError("ERROR: type index is out of bounds of redirection table");
		return ret;
	}
	// If there is no implementation for a method
	if(!funcs[r.typeID])
	{
		// Find implemented function ID as a type reference
		unsigned int found = 0;
		for(; found < arr->size; found++)
		{
			if(funcs[found])
				break;
		}
		//if(found == arr->size)
			nullcThrowError("ERROR: type '%s' doesn't implement method", nullcGetTypeName(r.typeID));
		//else
		//	nullcThrowError("ERROR: type '%s' doesn't implement method '%s%s' of type '%s'", nullcGetTypeName(r.typeID), nullcGetTypeName(r.typeID), strchr(nullcGetFunctionName(funcs[found]), ':'), nullcGetTypeName(nullcGetFunctionType(funcs[found])));
		return ret;
	}
	ret.context = r.ptr;
	ret.id = funcs[r.typeID];
	return ret;
}

NULLCArray<char>* __operatorSet(NULLCArray<char>* dst, NULLCArray<int> src, void* unused)
{
	if(dst->size < src.size)
		*dst = __newA(1, src.size, 0);
	for(int i = 0; i < src.size; i++)
		((char*)dst->ptr)[i] = ((int*)src.ptr)[i];
	return dst;
}
// short inline array definition support
NULLCArray<short>* __operatorSet(NULLCArray<short>* dst, NULLCArray<int> src, void* unused)
{
	if(dst->size < src.size)
		*dst = __newA(2, src.size, 0);
	for(int i = 0; i < src.size; i++)
		((short*)dst->ptr)[i] = ((int*)src.ptr)[i];
	return dst;
}
// float inline array definition support
NULLCArray<float>* __operatorSet(NULLCArray<float>* dst, NULLCArray<double> src, void* unused)
{
	if(dst->size < src.size)
		*dst = __newA(4, src.size, 0);
	for(int i = 0; i < src.size; i++)
		((float*)dst->ptr)[i] = ((double*)src.ptr)[i];
	return dst;
}

FastVector<__nullcFunction> funcTable;
struct NULLCFuncInfo
{
	unsigned	hash;
	unsigned	extraType;
	unsigned	funcType;
	NULLCFuncInfo(unsigned nHash, unsigned nType, unsigned nFuncType)
	{
		hash = nHash;
		extraType = nType;
		funcType = nFuncType;
	}
};
FastVector<NULLCFuncInfo> funcTableExt;

__nullcFunctionArray* __nullcGetFunctionTable()
{
	return &funcTable.data;
}

unsigned int GetStringHash(const char *str)
{
	unsigned int hash = 5381;
	int c;
	while((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c;
	return hash;
}

unsigned __nullcRegisterFunction(const char* name, void* fPtr, unsigned extraType, unsigned funcType)
{
	unsigned hash = GetStringHash(name);
	for(unsigned int i = 0; i < funcTable.size(); i++)
		if(funcTableExt[i].hash == hash)
			return i;
	funcTable.push_back(fPtr);
	funcTableExt.push_back(NULLCFuncInfo(hash, extraType, funcType));
	return funcTable.size() - 1;
}

// Memory allocation and GC

#define GC_DEBUG_PRINT(...)
//#define GC_DEBUG_PRINT printf

#define NULLC_PTR_SIZE sizeof(void*)
namespace GC
{
	unsigned int	objectName = GetStringHash("auto ref");
	unsigned int	autoArrayName = GetStringHash("auto[]");

	void CheckArray(char* ptr, const NULLCTypeInfo& type);
	void CheckClass(char* ptr, const NULLCTypeInfo& type);
	void CheckFunction(char* ptr);
	void CheckVariable(char* ptr, const NULLCTypeInfo& type);

	// Function that marks memory blocks belonging to GC
	void MarkPointer(char* ptr, const NULLCTypeInfo& type, bool takeSubtype)
	{
		// We have pointer to stack that has a pointer inside, so 'ptr' is really a pointer to pointer
		char **rPtr = (char**)ptr;
		// Check for unmanageable ranges. Range of 0x00000000-0x00010000 is unmanageable by default due to upvalues with offsets inside closures.
		if(*rPtr > (char*)0x00010000 && (*rPtr < unmanageableBase || *rPtr > unmanageableTop))
		{
			// Get type that pointer points to
			GC_DEBUG_PRINT("\tGlobal pointer %s %p (at %p)\r\n", type.name, *rPtr, ptr);

			// Get pointer to the start of memory block. Some pointers may point to the middle of memory blocks
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(*rPtr);
			// If there is no base, this pointer points to memory that is not GCs memory
			if(!basePtr)
				return;
			GC_DEBUG_PRINT("\tPointer base is %p\r\n", basePtr);
			// Marker is 4 bytes before the block
			unsigned int *marker = (unsigned int*)(basePtr)-1;
			GC_DEBUG_PRINT("\tMarker is %d\r\n", *marker & 0xff);

			// If block is unmarked
			if((*marker & 0xff) == 0)
			{
				// Mark block as used
				*marker |= 1;
				GC_DEBUG_PRINT("Type near memory %d, type %d (%d)\n", *marker >> 8, type.subTypeID, takeSubtype);
				// And if type is not simple, check memory to which pointer points to
				if(type.category != NULLC_NONE)
					CheckVariable(*rPtr, takeSubtype ? __nullcTypeList[/*type.subTypeID*/*marker >> 8] : type);
			}else if(takeSubtype && __nullcTypeList[type.subTypeID].category == NULLC_POINTER){
				MarkPointer(*rPtr, __nullcTypeList[type.subTypeID], true); 
			}
		}
	}

	// Function that checks arrays for pointers
	void CheckArray(char* ptr, const NULLCTypeInfo& type)
	{
		// Get array element type
		NULLCTypeInfo *subType = type.hash == autoArrayName ? NULL : &__nullcTypeList[type.subTypeID];
		// Real array size (changed for unsized arrays)
		unsigned int size = type.memberCount;
		// If array type is an unsized array, check pointer that points to actual array contents
		if(size == -1)
		{
			// Get real array size
			size = *(int*)(ptr + NULLC_PTR_SIZE);
			// Switch pointer to array data
			char **rPtr = (char**)ptr;
			ptr = *rPtr;
			// If uninitialized or points to stack memory, return
			if(!ptr || ptr <= (char*)0x00010000 || (ptr >= unmanageableBase && ptr <= unmanageableTop))
				return;
			GC_DEBUG_PRINT("\tGlobal pointer %p\r\n", ptr);
			// Get base pointer
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(ptr);
			// If there is no base pointer or memory already marked, exit
			if(!basePtr || (*((unsigned int*)(basePtr) - 1) & 0xff))
				return;
			// Mark memory as used
			*((unsigned int*)(basePtr) - 1) |= 1;
		}else if(type.hash == autoArrayName){
			NULLCAutoArray *data = (NULLCAutoArray*)ptr;
			// Get real variable type
			subType = &__nullcTypeList[data->typeID];
			// skip uninitialized array
			if(!data->ptr)
				return;
			// Mark target data
			MarkPointer((char*)&data->ptr, *subType, false);
			// Switch pointer to target
			ptr = data->ptr;
			// Get array size
			size = data->len;
		}
		// Otherwise, check every array element is it's either array, pointer of class
		switch(subType->category)
		{
		case NULLC_ARRAY:
			for(unsigned int i = 0; i < size; i++, ptr += subType->size)
				CheckArray(ptr, *subType);
			break;
		case NULLC_POINTER:
			for(unsigned int i = 0; i < size; i++, ptr += subType->size)
				MarkPointer(ptr, *subType, true);
			break;
		case NULLC_CLASS:
			for(unsigned int i = 0; i < size; i++, ptr += subType->size)
				CheckClass(ptr, *subType);
			break;
		case NULLC_FUNCTION:
			for(unsigned int i = 0; i < size; i++, ptr += subType->size)
				CheckFunction(ptr);
			break;
		}
	}

	// Function that checks classes for pointers
	void CheckClass(char* ptr, const NULLCTypeInfo& type)
	{
		const NULLCTypeInfo *realType = &type;
		if(type.hash == objectName)
		{
			// Get real variable type
			realType = &__nullcTypeList[*(int*)ptr];
			// Switch pointer to target
			char **rPtr = (char**)(ptr + 4);
			ptr = *rPtr;
			// If uninitialized or points to stack memory, return
			if(!ptr || ptr <= (char*)0x00010000 || (ptr >= unmanageableBase && ptr <= unmanageableTop))
				return;
			// Get base pointer
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(ptr);
			// If there is no base pointer or memory already marked, exit
			if(!basePtr || (*((unsigned int*)(basePtr) - 1) & 0xff))
				return;
			// Mark memory as used
			*((unsigned int*)(basePtr) - 1) |= 1;
			// Fixup target
			CheckVariable(*rPtr, *realType);
			// Exit
			return;
		}else if(type.hash == autoArrayName){
			CheckArray(ptr, type);
			// Exit
			return;
		}
		// Get class member type list
		unsigned int *memberList = &__nullcTypePart[realType->members];
		// Check pointer members
		for(unsigned int n = 0; n < realType->memberCount; n++)
		{
			// Get member type
			NULLCTypeInfo &subType = __nullcTypeList[memberList[n * 2]];
			unsigned int pos = memberList[n * 2 + 1];
			// Check member
			CheckVariable(ptr + pos, subType);
		}
	}

	// Function that checks function context for pointers
	void CheckFunction(char* ptr)
	{
		NULLCFuncPtr *fPtr = (NULLCFuncPtr*)ptr;
		// If there's no context, there's nothing to check
		if(!fPtr->context)
			return;
		const NULLCFuncInfo &func = funcTableExt[fPtr->id];
		// If context is "this" pointer
		if(func.extraType != ~0u)
			MarkPointer((char*)&fPtr->context, __nullcTypeList[func.extraType], true);
	}

	// Function that decides, how variable of type 'type' should be checked for pointers
	void CheckVariable(char* ptr, const NULLCTypeInfo& type)
	{
		switch(type.category)
		{
		case NULLC_ARRAY:
			CheckArray(ptr, type);
			break;
		case NULLC_POINTER:
			MarkPointer(ptr, type, true);
			break;
		case NULLC_CLASS:
			CheckClass(ptr, type);
			break;
		case NULLC_FUNCTION:
			CheckFunction(ptr);
			break;
		}
	}
}

struct GlobalRoot
{
	void	*ptr;
	unsigned typeID;
};
FastVector<GlobalRoot> rootSet;

// Main function for marking all pointers in a program
void MarkUsedBlocks()
{
	GC_DEBUG_PRINT("Unmanageable range: %p-%p\r\n", GC::unmanageableBase, GC::unmanageableTop);

	// Mark global variables
	for(unsigned int i = 0; i < rootSet.size(); i++)
	{
		GC_DEBUG_PRINT("Global %s (at %p)\r\n", __nullcTypeList[rootSet[i].typeID].name, rootSet[i].ptr);
		GC::CheckVariable((char*)rootSet[i].ptr, __nullcTypeList[rootSet[i].typeID]);
	}
	// Check that temporary stack range is correct
	assert(GC::unmanageableTop >= GC::unmanageableBase, "ERROR: GC - incorrect stack range", 0);
	char* tempStackBase = GC::unmanageableBase;
	// Check temporary stack for pointers
	while(tempStackBase < GC::unmanageableTop)
	{
		char *ptr = *(char**)(tempStackBase);
		// Check for unmanageable ranges. Range of 0x00000000-0x00010000 is unmanageable by default due to upvalues with offsets inside closures.
		if(ptr > (char*)0x00010000 && (ptr < GC::unmanageableBase || ptr > GC::unmanageableTop))
		{
			// Get pointer base
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(ptr);
			// If there is no base, this pointer points to memory that is not GCs memory
			if(basePtr)
			{
				unsigned int *marker = (unsigned int*)(basePtr)-1;
				// If block is unmarked, mark it as used
				if((*marker & 0xff) == 0)
				{
					*marker |= 1;
					GC_DEBUG_PRINT("Found %s type %d on stack at %p\n", __nullcTypeList[*marker >> 8].name, *marker >> 8, ptr);
					GC::CheckVariable(ptr, __nullcTypeList[*marker >> 8]);
				}
			}
		}
		tempStackBase += 4;
	}
}

void __nullcRegisterGlobal(void* ptr, unsigned typeID)
{
	GlobalRoot entry;
	entry.ptr = ptr;
	entry.typeID = typeID;
	rootSet.push_back(entry);
}
void __nullcRegisterBase(void* ptr)
{
	GC::unmanageableTop = GC::unmanageableTop ? ((char*)ptr > GC::unmanageableTop ? (char*)ptr : GC::unmanageableTop) : (char*)ptr;
}

namespace NULLC
{
	void*	defaultAlloc(int size);
	void	defaultDealloc(void* ptr);

	extern void*	(*alloc)(int);
	extern void		(*dealloc)(void*);
}


void*	NULLC::defaultAlloc(int size)
{
	return malloc(size);
}
void	NULLC::defaultDealloc(void* ptr)
{
	free(ptr);
}

void*	(*NULLC::alloc)(int) = NULLC::defaultAlloc;
void	(*NULLC::dealloc)(void*) = NULLC::defaultDealloc;

template<int elemSize>
union SmallBlock
{
	char			data[elemSize];
	unsigned int	marker;
	SmallBlock		*next;
};

template<int elemSize, int countInBlock>
struct LargeBlock
{
	typedef SmallBlock<elemSize> Block;
	Block		page[countInBlock];
	LargeBlock	*next;
};

template<int elemSize, int countInBlock>
class ObjectBlockPool
{
	typedef SmallBlock<elemSize> MySmallBlock;
	typedef LargeBlock<elemSize, countInBlock> MyLargeBlock;
public:
	ObjectBlockPool()
	{
		freeBlocks = &lastBlock;
		activePages = NULL;
		lastNum = countInBlock;
	}
	~ObjectBlockPool()
	{
		if(!activePages)
			return;
		do
		{
			MyLargeBlock* following = activePages->next;
			NULLC::dealloc(activePages);
			activePages = following;
		}while(activePages != NULL);
		freeBlocks = &lastBlock;
		activePages = NULL;
		lastNum = countInBlock;
		sortedPages.reset();
	}

	void* Alloc()
	{
		MySmallBlock*	result;
		if(freeBlocks && freeBlocks != &lastBlock)
		{
			result = freeBlocks;
			freeBlocks = freeBlocks->next;
		}else{
			if(lastNum == countInBlock)
			{
				MyLargeBlock* newPage = (MyLargeBlock*)NULLC::alloc(sizeof(MyLargeBlock));
				//memset(newPage, 0, sizeof(MyLargeBlock));
				newPage->next = activePages;
				activePages = newPage;
				lastNum = 0;
				sortedPages.push_back(newPage);
				int index = sortedPages.size() - 1;
				while(index > 0 && sortedPages[index] < sortedPages[index - 1])
				{
					MyLargeBlock *tmp = sortedPages[index];
					sortedPages[index] = sortedPages[index - 1];
					sortedPages[index - 1] = tmp;
					index--;
				}
			}
			result = &activePages->page[lastNum++];
		}
		return result;
	}

	void Free(void* ptr)
	{
		if(!ptr)
			return;
		MySmallBlock* freedBlock = static_cast<MySmallBlock*>(static_cast<void*>(ptr));
		freedBlock->next = freeBlocks;
		freeBlocks = freedBlock;
	}
	bool IsBasePointer(void* ptr)
	{
		MyLargeBlock *curr = activePages;
		while(curr)
		{
			if((char*)ptr >= (char*)curr->page && (char*)ptr <= (char*)curr->page + sizeof(MyLargeBlock))
			{
				if(((unsigned int)(intptr_t)((char*)ptr - (char*)curr->page) & (elemSize - 1)) == 4)
					return true;
			}
			curr = curr->next;
		}
		return false;
	}
	void* GetBasePointer(void* ptr)
	{
		if(!sortedPages.size() || ptr < sortedPages[0] || ptr > (char*)sortedPages.back() + sizeof(MyLargeBlock))
			return NULL;
		// Binary search
		unsigned int lowerBound = 0;
		unsigned int upperBound = sortedPages.size() - 1;
		unsigned int pointer = 0;
		while(upperBound - lowerBound > 1)
		{
			pointer = (lowerBound + upperBound) >> 1;
			if(ptr < sortedPages[pointer])
				upperBound = pointer;
			if(ptr > sortedPages[pointer])
				lowerBound = pointer;
		}
		if(ptr < sortedPages[pointer])
			pointer--;
		if(ptr > (char*)sortedPages[pointer]  + sizeof(MyLargeBlock))
			pointer++;
		MyLargeBlock *best = sortedPages[pointer];

		if(ptr < best || ptr > (char*)best + sizeof(MyLargeBlock))
			return NULL;
		unsigned int fromBase = (unsigned int)(intptr_t)((char*)ptr - (char*)best->page);
		return (char*)best->page + (fromBase & ~(elemSize - 1)) + 4;
	}
	void Mark(unsigned int number)
	{
		__assert(number < 128);
		MyLargeBlock *curr = activePages;
		while(curr)
		{
			for(unsigned int i = 0; i < (curr == activePages ? lastNum : countInBlock); i++)
			{
				if((curr->page[i].marker & 0xff) < 128)
					curr->page[i].marker = (curr->page[i].marker & ~0xff) | number;
			}
			curr = curr->next;
		}
	}
	unsigned int FreeMarked(unsigned int number)
	{
		unsigned int freed = 0;
		MyLargeBlock *curr = activePages;
		while(curr)
		{
			for(unsigned int i = 0; i < (curr == activePages ? lastNum : countInBlock); i++)
			{
				if((curr->page[i].marker & 0xff) == number)
				{
					Free(&curr->page[i]);
					freed++;
				}
			}
			curr = curr->next;
		}
		return freed;
	}

	MySmallBlock	lastBlock;

	MySmallBlock	*freeBlocks;
	MyLargeBlock	*activePages;
	unsigned int	lastNum;

	FastVector<MyLargeBlock*>	sortedPages;
};

namespace NULLC
{
	const unsigned int poolBlockSize = 64 * 1024;

	unsigned int usedMemory = 0;

	unsigned int collectableMinimum = 1024 * 1024;
	unsigned int globalMemoryLimit = 1024 * 1024 * 1024;

	ObjectBlockPool<8, poolBlockSize / 8>		pool8;
	ObjectBlockPool<16, poolBlockSize / 16>		pool16;
	ObjectBlockPool<32, poolBlockSize / 32>		pool32;
	ObjectBlockPool<64, poolBlockSize / 64>		pool64;
	ObjectBlockPool<128, poolBlockSize / 128>	pool128;
	ObjectBlockPool<256, poolBlockSize / 256>	pool256;
	ObjectBlockPool<512, poolBlockSize / 512>	pool512;

	FastVector<void*>				globalObjects;

	double	markTime = 0.0;
	double	collectTime = 0.0;
}

void* NULLC::AllocObject(int size, unsigned typeID)
{
	if(size < 0)
	{
		nullcThrowError("Requested memory size is less than zero.");
		return NULL;
	}
	void *data = NULL;
	size += 4;

	if((unsigned int)(usedMemory + size) > globalMemoryLimit)
	{
		CollectMemory();
		if((unsigned int)(usedMemory + size) > globalMemoryLimit)
		{
			nullcThrowError("Reached global memory maximum");
			return NULL;
		}
	}else if((unsigned int)(usedMemory + size) > collectableMinimum){
		CollectMemory();
	}
	unsigned int realSize = size;
	if(size <= 64)
	{
		if(size <= 16)
		{
			if(size <= 8)
			{
				data = pool8.Alloc();
				realSize = 8;
			}else{
				data = pool16.Alloc();
				realSize = 16;
			}
		}else{
			if(size <= 32)
			{
				data = pool32.Alloc();
				realSize = 32;
			}else{
				data = pool64.Alloc();
				realSize = 64;
			}
		}
	}else{
		if(size <= 256)
		{
			if(size <= 128)
			{
				data = pool128.Alloc();
				realSize = 128;
			}else{
				data = pool256.Alloc();
				realSize = 256;
			}
		}else{
			if(size <= 512)
			{
				data = pool512.Alloc();
				realSize = 512;
			}else{
				globalObjects.push_back(NULLC::alloc(size+4));
				if(globalObjects.back() == NULL)
				{
					nullcThrowError("Allocation failed.");
					return NULL;
				}
				realSize = *(int*)globalObjects.back() = size;
				data = (char*)globalObjects.back() + 4;
			}
		}
	}
	usedMemory += realSize;

	if(data == NULL)
	{
		nullcThrowError("Allocation failed.");
		return NULL;
	}

	memset(data, 0, size);
	*(int*)data = typeID << 8;
	return (char*)data + 4;
}

unsigned int NULLC::UsedMemory()
{
	return usedMemory;
}

NULLCArray<void> NULLC::AllocArray(int size, int count, unsigned typeID)
{
	NULLCArray<void> ret;
	ret.ptr = (char*)AllocObject(count * size, typeID);
	ret.size = count;
	return ret;
}

void NULLC::MarkMemory(unsigned int number)
{
	for(unsigned int i = 0; i < globalObjects.size(); i++)
		((unsigned int*)globalObjects[i])[1] = number;
	pool8.Mark(number);
	pool16.Mark(number);
	pool32.Mark(number);
	pool64.Mark(number);
	pool128.Mark(number);
	pool256.Mark(number);
	pool512.Mark(number);
}

bool NULLC::IsBasePointer(void* ptr)
{
	// Search in range of every pool
	if(pool8.IsBasePointer(ptr))
		return true;
	if(pool16.IsBasePointer(ptr))
		return true;
	if(pool32.IsBasePointer(ptr))
		return true;
	if(pool64.IsBasePointer(ptr))
		return true;
	if(pool128.IsBasePointer(ptr))
		return true;
	if(pool256.IsBasePointer(ptr))
		return true;
	if(pool512.IsBasePointer(ptr))
		return true;
	// Search in global pool
	for(unsigned int i = 0; i < globalObjects.size(); i++)
	{
		if((char*)ptr - 8 == globalObjects[i])
			return true;
	}
	return false;
}

void* NULLC::GetBasePointer(void* ptr)
{
	// Search in range of every pool
	if(void *base = pool8.GetBasePointer(ptr))
		return base;
	if(void *base = pool16.GetBasePointer(ptr))
		return base;
	if(void *base = pool32.GetBasePointer(ptr))
		return base;
	if(void *base = pool64.GetBasePointer(ptr))
		return base;
	if(void *base = pool128.GetBasePointer(ptr))
		return base;
	if(void *base = pool256.GetBasePointer(ptr))
		return base;
	if(void *base = pool512.GetBasePointer(ptr))
		return base;
	// Search in global pool
	for(unsigned int i = 0; i < globalObjects.size(); i++)
	{
		if(ptr >= globalObjects[i] && ptr <= (char*)globalObjects[i] + *(unsigned int*)globalObjects[i])
			return (char*)globalObjects[i] + 8;
	}
	return NULL;
}

void NULLC::CollectMemory()
{
	GC_DEBUG_PRINT("%d used memory (%d collectable cap, %d max cap)\r\n", usedMemory, collectableMinimum, globalMemoryLimit);

	double time = (double(clock()) / CLOCKS_PER_SEC);

	GC::unmanageableBase = (char*)&time;

	// All memory blocks are marked with 0
	MarkMemory(0);
	// Used memory blocks are marked with 1
	MarkUsedBlocks();

	markTime += (double(clock()) / CLOCKS_PER_SEC) - time;
	time = (double(clock()) / CLOCKS_PER_SEC);

	// Globally allocated objects marked with 0 are deleted
	unsigned int unusedBlocks = 0;
	for(unsigned int i = 0; i < globalObjects.size(); i++)
	{
		if(((unsigned int*)globalObjects[i])[1] == 0)
		{
			usedMemory -= *(unsigned int*)globalObjects[i];
			NULLC::dealloc(globalObjects[i]);
			globalObjects[i] = globalObjects.back();
			globalObjects.pop_back();
			unusedBlocks++;
		}
	}
//	printf("%d unused globally allocated blocks destroyed (%d remains)\r\n", unusedBlocks, globalObjects.size());

//	printf("%d used memory\r\n", usedMemory);

	// Objects allocated from pools are freed
	unusedBlocks = pool8.FreeMarked(0);
	usedMemory -= unusedBlocks * 8;
//	printf("%d unused pool blocks freed (8 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool16.FreeMarked(0);
	usedMemory -= unusedBlocks * 16;
//	printf("%d unused pool blocks freed (16 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool32.FreeMarked(0);
	usedMemory -= unusedBlocks * 32;
//	printf("%d unused pool blocks freed (32 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool64.FreeMarked(0);
	usedMemory -= unusedBlocks * 64;
//	printf("%d unused pool blocks freed (64 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool128.FreeMarked(0);
	usedMemory -= unusedBlocks * 128;
//	printf("%d unused pool blocks freed (128 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool256.FreeMarked(0);
	usedMemory -= unusedBlocks * 256;
//	printf("%d unused pool blocks freed (256 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool512.FreeMarked(0);
	usedMemory -= unusedBlocks * 512;
//	printf("%d unused pool blocks freed (512 bytes)\r\n", unusedBlocks);

	GC_DEBUG_PRINT("%d used memory\r\n", usedMemory);

	collectTime += (double(clock()) / CLOCKS_PER_SEC) - time;

	if(usedMemory + (usedMemory >> 1) >= collectableMinimum)
		collectableMinimum <<= 1;
}

double NULLC::MarkTime()
{
	return markTime;
}

double NULLC::CollectTime()
{
	return collectTime;
}

int  __newS(int size, unsigned typeID)
{
	return (int)NULLC::AllocObject(size, typeID);
}

NULLCArray<void>  __newA(int size, int count, unsigned typeID)
{
	return NULLC::AllocArray(size, count, typeID);
}

int isStackPointer(NULLCRef ptr, void* unused)
{
	GC::unmanageableBase = (char*)&ptr;
	return ptr.ptr >= GC::unmanageableBase && ptr.ptr <= GC::unmanageableTop;
}

int typeid__size__int_ref__(unsigned int * __context)
{
	return __nullcGetTypeInfo(*__context)->size;
}

NULLCAutoArray auto_array(unsigned int type, int count, void* unused)
{
	NULLCAutoArray res;
	res.typeID = type;
	res.len = count;
	res.ptr = (char*)__newS(typeid__size__int_ref__(&type) * (count), type);
	return res;
}
void auto____set_void_ref_auto_ref_int_(NULLCRef x, int pos, void* unused)
{
	NULLCAutoArray *arr = (NULLCAutoArray*)unused;
	if(x.typeID != arr->typeID)
	{
		nullcThrowError("ERROR: cannot convert from '%s' to an 'auto[]' element type '%s'", nullcGetTypeName(x.typeID), nullcGetTypeName(arr->typeID));
		return;
	}
	unsigned elemSize = __nullcGetTypeInfo(arr->typeID)->size;
	if(pos >= arr->len)
	{
		unsigned newSize = 1 + arr->len + (arr->len >> 1);
		if(pos >= newSize)
			newSize = pos;
		NULLCAutoArray n = auto_array(arr->typeID, newSize, NULL);
		memcpy(n.ptr, arr->ptr, arr->len * elemSize);
		*arr = n;
	}
	memcpy(arr->ptr + elemSize * pos, x.ptr, elemSize);
}
void __force_size(int* s, int size, void* unused)
{
	assert(size <= *s, "ERROR: cannot extend array", NULL);
	*s = size;
}

int isCoroutineReset(NULLCRef f, void* unused)
{
	if(__nullcGetTypeInfo(f.typeID)->category != NULLC_FUNCTION)
	{
		nullcThrowError("Argument is not a function");
		return 0;
	}
	NULLCFuncPtr *fPtr = (NULLCFuncPtr*)f.ptr;
	if(funcTableExt[fPtr->id].funcType != FunctionCategory::COROUTINE)
	{
		nullcThrowError("Function is not a coroutine");
		return 0;
	}
	return !**(int**)fPtr->context;
}
void __assertCoroutine(NULLCRef f, void* unused)
{
	if(__nullcGetTypeInfo(f.typeID)->category != NULLC_FUNCTION)
		nullcThrowError("Argument is not a function");
	NULLCFuncPtr *fPtr = (NULLCFuncPtr*)f.ptr;
	if(funcTableExt[fPtr->id].funcType != FunctionCategory::COROUTINE)
		nullcThrowError("ERROR: function is not a coroutine");
}
