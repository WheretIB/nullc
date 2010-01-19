#include "StdLib.h"

#include "nullc.h"
#include <string.h>

#include "stdafx.h"
#include "Pool.h"

#include "Executor_Common.h"

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
		MyLargeBlock *curr = activePages;
		while(curr)
		{
			if((char*)ptr >= (char*)curr->page && (char*)ptr <= (char*)curr->page + sizeof(MyLargeBlock))
			{
				unsigned int fromBase = (unsigned int)(intptr_t)((char*)ptr - (char*)curr->page);
				return (char*)curr->page + (fromBase & ~(elemSize - 1)) + 4;
			}
			curr = curr->next;
		}
		return NULL;
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
};

namespace NULLC
{
	const unsigned int poolBlockSize = 64 * 1024;

	unsigned int usedMemory = 0;

	unsigned int baseMinimum = 128 * 1024;
	unsigned int collectableMinimum = 128 * 1024;
	unsigned int globalMemoryLimit = 128 * 1024 * 1024;

	ObjectBlockPool<8, poolBlockSize / 8>		pool8;
	ObjectBlockPool<16, poolBlockSize / 16>		pool16;
	ObjectBlockPool<32, poolBlockSize / 32>		pool32;
	ObjectBlockPool<64, poolBlockSize / 64>		pool64;
	ObjectBlockPool<128, poolBlockSize / 128>	pool128;
	ObjectBlockPool<256, poolBlockSize / 256>	pool256;
	ObjectBlockPool<512, poolBlockSize / 512>	pool512;

	FastVector<void*>				globalObjects;
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

#ifdef ENABLE_GC
	if((unsigned int)(usedMemory + size) > globalMemoryLimit)
	{
		nullcThrowError("Reached global memory maximum");
		return NULL;
	}else if((unsigned int)(usedMemory + size) > collectableMinimum){
		CollectMemory();
	}
#endif
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

NullCArray NULLC::AllocArray(int size, int count)
{
	NullCArray ret;
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

#ifdef ENABLE_GC
void NULLC::CollectMemory()
{
	// All memory blocks are marked with 0
	MarkMemory(0);
	// Used memory blocks are marked with 1
	MarkUsedBlocks();

//	printf("%d used memory (%d collectable cap, %d max cap)\r\n", usedMemory, collectableMinimum, globalMemoryLimit);

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

	while(collectableMinimum < usedMemory)
		collectableMinimum <<= 1;
	if(usedMemory < baseMinimum)
		collectableMinimum = baseMinimum;
}
#endif

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
}

void NULLC::Assert(int val)
{
	if(!val)
		nullcThrowError("Assertion failed");
}

int NULLC::StrEqual(NullCArray a, NullCArray b)
{
	if(a.len != b.len)
		return 0;
	for(unsigned int i = 0; i < a.len; i++)
		if(a.ptr[i] != b.ptr[i])
			return 0;
	return 1;
}

int NULLC::StrNEqual(NullCArray a, NullCArray b)
{
	return !StrEqual(a, b);
}

NullCArray NULLC::StrConcatenate(NullCArray a, NullCArray b)
{
	NullCArray ret;

	ret.len = a.len + b.len - 1;
	ret.ptr = (char*)AllocObject(ret.len);

	memcpy(ret.ptr, a.ptr, a.len);
	memcpy(ret.ptr + a.len - 1, b.ptr, b.len);

	return ret;
}

NullCArray NULLC::StrConcatenateAndSet(NullCArray *a, NullCArray b)
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

NullCArray NULLC::IntToStr(int* r)
{
	int number = *r;
	char buf[16];
	char *curr = buf;
	*curr++ = (char)(number % 10 + '0');
	while(number /= 10)
		*curr++ = (char)(number % 10 + '0');
	NullCArray arr = AllocArray(1, (int)(curr - buf) + 1);
	char *str = arr.ptr;
	do 
	{
		--curr;
		*str++ = *curr;
	}while(curr != buf);
	return arr;
}
