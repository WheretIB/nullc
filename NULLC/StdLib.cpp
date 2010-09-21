#include "StdLib.h"

#include "nullc.h"
#include <string.h>
#include <time.h>

#include "stdafx.h"
#include "Pool.h"

#include "Executor_Common.h"
#include "includes/typeinfo.h"

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
				MyLargeBlock* newPage = new(NULLC::alloc(sizeof(MyLargeBlock))) MyLargeBlock;
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
		assert(number < 128);
		MyLargeBlock *curr = activePages;
		while(curr)
		{
			for(unsigned int i = 0; i < (curr == activePages ? lastNum : countInBlock); i++)
			{
				if(curr->page[i].marker < 128)
					curr->page[i].marker = number;
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
				if(curr->page[i].marker == number)
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

	Linker	*linker = NULL;

	double	markTime = 0.0;
	double	collectTime = 0.0;
}

void NULLC::SetLinker(Linker *linker)
{
	NULLC::linker = linker;
}

void* NULLC::AllocObject(int size)
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
	*(int*)data = 0;
	return (char*)data + 4;
}

unsigned int NULLC::UsedMemory()
{
	return usedMemory;
}

NULLCArray NULLC::AllocArray(int size, int count)
{
	NULLCArray ret;
	ret.ptr = (char*)AllocObject(count * size);
	ret.len = count;
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
//	printf("%d used memory (%d collectable cap, %d max cap)\r\n", usedMemory, collectableMinimum, globalMemoryLimit);

	double time = (double(clock()) / CLOCKS_PER_SEC);

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

//	printf("%d used memory\r\n", usedMemory);

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

void NULLC::ClearMemory()
{
	usedMemory = 0;

	pool8.~ObjectBlockPool();
	pool16.~ObjectBlockPool();
	pool32.~ObjectBlockPool();
	pool64.~ObjectBlockPool();
	pool128.~ObjectBlockPool();
	pool256.~ObjectBlockPool();
	pool512.~ObjectBlockPool();

	for(unsigned int i = 0; i < globalObjects.size(); i++)
		NULLC::dealloc(globalObjects[i]);
	globalObjects.clear();
}

void NULLC::ResetMemory()
{
	ClearMemory();
	globalObjects.reset();
	ResetGC();
}

void NULLC::SetGlobalLimit(unsigned int limit)
{
	globalMemoryLimit = limit;
	collectableMinimum = limit < 1024 * 1024 ? limit : 1024 * 1024;
}

void NULLC::Assert(int val)
{
	if(!val)
		nullcThrowError("Assertion failed");
}

void NULLC::Assert2(int val, NULLCArray message)
{
	if(!val)
		nullcThrowError(message.ptr);
}

NULLCRef NULLC::CopyObject(NULLCRef ptr)
{
	NULLCRef ret;
	ret.typeID = ptr.typeID;
	unsigned int objSize = linker->exTypes[ret.typeID].size;
	ret.ptr = (char*)AllocObject(objSize);
	memcpy(ret.ptr, ptr.ptr, objSize);
	return ret;
}

void NULLC::CopyArray(NULLCAutoArray* dst, NULLCAutoArray src)
{
	dst->typeID = src.typeID;
	dst->len = src.len;
	dst->ptr = (char*)NULLC::AllocObject(src.len * linker->exTypes[src.typeID].size);
	memcpy(dst->ptr, src.ptr, src.len * linker->exTypes[src.typeID].size);
}

NULLCRef NULLC::ReplaceObject(NULLCRef l, NULLCRef r)
{
	if(l.typeID != r.typeID)
	{
		nullcThrowError("ERROR: cannot convert from %s ref to %s ref", &linker->exSymbols[linker->exTypes[r.typeID].offsetToName], &linker->exSymbols[linker->exTypes[l.typeID].offsetToName]);
		return l;
	}
	memcpy(l.ptr, r.ptr, linker->exTypes[r.typeID].size);
	return l;
}

void NULLC::SwapObjects(NULLCRef l, NULLCRef r)
{
	if(l.typeID != r.typeID)
	{
		nullcThrowError("ERROR: types don't match (%s ref, %s ref)", &linker->exSymbols[linker->exTypes[r.typeID].offsetToName], &linker->exSymbols[linker->exTypes[l.typeID].offsetToName]);
		return;
	}
	unsigned size = linker->exTypes[l.typeID].size;

	char tmpStack[512];
	// $$ should use some extendable static storage for big objects
	char *tmp = size < 512 ? tmpStack : (char*)NULLC::AllocObject(size);
	memcpy(tmp, l.ptr, size);
	memcpy(l.ptr, r.ptr, size);
	memcpy(r.ptr, tmp, size);
}

int NULLC::CompareObjects(NULLCRef l, NULLCRef r)
{
	if(l.typeID != r.typeID)
	{
		nullcThrowError("ERROR: types don't match (%s ref, %s ref)", &linker->exSymbols[linker->exTypes[r.typeID].offsetToName], &linker->exSymbols[linker->exTypes[l.typeID].offsetToName]);
		return 0;
	}
	return 0 == memcmp(l.ptr, r.ptr, linker->exTypes[l.typeID].size);
}

int NULLC::StrEqual(NULLCArray a, NULLCArray b)
{
	if(a.len != b.len)
		return 0;
	for(unsigned int i = 0; i < a.len; i++)
		if(a.ptr[i] != b.ptr[i])
			return 0;
	return 1;
}

int NULLC::StrNEqual(NULLCArray a, NULLCArray b)
{
	return !StrEqual(a, b);
}

NULLCArray NULLC::StrConcatenate(NULLCArray a, NULLCArray b)
{
	NULLCArray ret;

	// If first part is zero-terminated, override zero in the new string
	int shift = a.len && (a.ptr[a.len-1] == 0);
	ret.len = a.len + b.len - shift;
	ret.ptr = (char*)AllocObject(ret.len);
	if(!ret.ptr)
		return ret;

	memcpy(ret.ptr, a.ptr, a.len);
	memcpy(ret.ptr + a.len - shift, b.ptr, b.len);

	return ret;
}

NULLCArray NULLC::StrConcatenateAndSet(NULLCArray *a, NULLCArray b)
{
	return *a = StrConcatenate(*a, b);
}

int NULLC::Int(int a)
{
	return a;
}

long long NULLC::Long(long long a)
{
	return a;

}
float NULLC::Float(float a)
{
	return a;
}

double NULLC::Double(double a)
{
	return a;
}

NULLCArray NULLC::IntToStr(int* r)
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
	NULLCArray arr = AllocArray(1, (int)(curr - buf) + 1);
	char *str = arr.ptr;
	do 
	{
		--curr;
		*str++ = *curr;
	}while(curr != buf);
	return arr;
}

NULLCArray NULLC::DoubleToStr(int precision, double* r)
{
	char buf[256];
	SafeSprintf(buf, 256, "%.*f", precision, *r);
	NULLCArray arr = AllocArray(1, (int)strlen(buf) + 1);
	memcpy(arr.ptr, buf, arr.len);
	return arr;
}

NULLCFuncPtr NULLC::FunctionRedirect(NULLCRef r, NULLCArray* arr)
{
	unsigned int *funcs = (unsigned int*)arr->ptr;
	NULLCFuncPtr ret = { 0, 0 };
	if(r.typeID > arr->len)
	{
		nullcThrowError("ERROR: type index is out of bounds of redirection table");
		return ret;
	}
	// If there is no implementation for a method
	if(!funcs[r.typeID])
	{
		// Find implemented function ID as a type reference
		unsigned int found = 0;
		for(; found < arr->len; found++)
		{
			if(funcs[found])
				break;
		}
		if(found == arr->len)
			nullcThrowError("ERROR: type '%s' doesn't implement method", nullcGetTypeName(r.typeID));
		else
			nullcThrowError("ERROR: type '%s' doesn't implement method '%s%s' of type '%s'", nullcGetTypeName(r.typeID), nullcGetTypeName(r.typeID), strchr(nullcGetFunctionName(funcs[found]), ':'), nullcGetTypeName(nullcGetFunctionType(funcs[found])));
		return ret;
	}
	ret.context = r.ptr;
	ret.id = funcs[r.typeID];
	return ret;
}

NULLC::TypeIDHelper NULLC::Typeid(NULLCRef r)
{
	TypeIDHelper help;
	help.id = r.typeID;
	return help;
}

int NULLC::TypeSize(int* a)
{
	return linker->exTypes[*a].size;
}

int NULLC::TypesEqual(int a, int b)
{
	return a == b;
}
int NULLC::TypesNEqual(int a, int b)
{
	return a != b;
}

int NULLC::RefCompare(NULLCRef a, NULLCRef b)
{
	return a.ptr == b.ptr;
}

int NULLC::RefNCompare(NULLCRef a, NULLCRef b)
{
	return a.ptr != b.ptr;
}

int NULLC::FuncCompare(NULLCFuncPtr a, NULLCFuncPtr b)
{
	return a.context == b.context && a.id == b.id;
}
int NULLC::FuncNCompare(NULLCFuncPtr a, NULLCFuncPtr b)
{
	return a.context != b.context || a.id != b.id;
}

int NULLC::TypeCount()
{
	return nullcGetTypeCount();
}

NULLCAutoArray* NULLC::AutoArrayAssign(NULLCAutoArray* left, NULLCRef right)
{
	if(!nullcIsArray(right.typeID))
	{
		nullcThrowError("ERROR: cannot convert from '%s' to 'auto[]'", nullcGetTypeName(right.typeID));
		return NULL;
	}
	left->len = nullcGetArraySize(right.typeID);
	if(left->len == ~0u)
	{
		NULLCArray *arr = (NULLCArray*)right.ptr;
		left->len = arr->len;
		left->ptr = arr->ptr;
	}else{
		left->ptr = right.ptr;
	}
	left->typeID = nullcGetSubType(right.typeID);
	return left;
}

NULLCRef NULLC::AutoArrayAssignRev(NULLCRef left, NULLCAutoArray *right)
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
		NULLCArray *arr = (NULLCArray*)left.ptr;
		arr->len = right->len;
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

NULLCAutoArray* NULLC::AutoArrayAssignSelf(NULLCAutoArray* left, NULLCAutoArray* right)
{
	left->len = right->len;
	left->ptr = right->ptr;
	left->typeID = right->typeID;
	return left;
}

NULLCRef NULLC::AutoArrayIndex(NULLCAutoArray* left, unsigned int index)
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

void NULLC::AutoArray(NULLCAutoArray* arr, int type, int count)
{
	arr->typeID = type;
	arr->len = count;
	arr->ptr = (char*)AllocObject(count * linker->exTypes[type].size);
}

void NULLC::AutoArraySet(NULLCRef x, unsigned pos, NULLCAutoArray* arr)
{
	if(x.typeID != arr->typeID)
	{
		nullcThrowError("ERROR: cannot convert from '%s' to an 'auto[]' element type '%s'", nullcGetTypeName(x.typeID), nullcGetTypeName(arr->typeID));
		return;
	}
	unsigned elemSize = linker->exTypes[arr->typeID].size;
	if(pos >= arr->len)
	{
		unsigned newSize = 1 + arr->len + (arr->len >> 1);
		if(pos >= newSize)
			newSize = pos;
		NULLCAutoArray n;
		AutoArray(&n, arr->typeID, newSize);
		if(!n.ptr)
			return;
		memcpy(n.ptr, arr->ptr, arr->len * elemSize);
		*arr = n;
	}
	memcpy(arr->ptr + elemSize * pos, x.ptr, elemSize);
}

void NULLC::ShrinkAutoArray(NULLCAutoArray* arr, unsigned size)
{
	if(size > (unsigned)arr->len)
	{
		nullcThrowError("ERROR: cannot extend array");
		return;
	}
	arr->len = size;
}

int NULLC::IsCoroutineReset(NULLCRef f)
{
	if(linker->exTypes[f.typeID].subCat != ExternTypeInfo::CAT_FUNCTION)
	{
		nullcThrowError("Argument is not a function");
		return 0;
	}
	NULLCFuncPtr *fPtr = (NULLCFuncPtr*)f.ptr;
	if(linker->exFunctions[fPtr->id].funcCat != ExternFuncInfo::COROUTINE)
	{
		nullcThrowError("Function is not a coroutine");
		return 0;
	}
	return !**(int**)fPtr->context;
}

void NULLC::AssertCoroutine(NULLCRef f)
{
	if(linker->exTypes[f.typeID].subCat != ExternTypeInfo::CAT_FUNCTION)
		nullcThrowError("Argument is not a function");
	NULLCFuncPtr *fPtr = (NULLCFuncPtr*)f.ptr;
	if(linker->exFunctions[fPtr->id].funcCat != ExternFuncInfo::COROUTINE)
		nullcThrowError("ERROR: function is not a coroutine");
}
