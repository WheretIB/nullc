#include "StdLib.h"

#include "nullc.h"
#include "nullc_debug.h"
#include <string.h>
#include <time.h>

#include "stdafx.h"
#include "Pool.h"
#include "Tree.h"

#include "Executor_Common.h"
#include "includes/typeinfo.h"

// memory structure				   |base->
// small object storage:			marker, data...
// big object storage:		size,	marker, data...
// small array storage:				marker, count, data...
// big array storage:		size,	marker, count, data...

namespace NULLC
{
	static Linker	*linker = NULL;
	FastVector<NULLCRef>	finalizeList;

	static uintptr_t OBJECT_VISIBLE		= 1 << 0;
	static uintptr_t OBJECT_FREED		= 1 << 1;
	static uintptr_t OBJECT_FINALIZABLE	= 1 << 2;
	static uintptr_t OBJECT_FINALIZED	= 1 << 3;
	static uintptr_t OBJECT_ARRAY		= 1 << 4;
	static uintptr_t OBJECT_MASK		= OBJECT_VISIBLE | OBJECT_FREED;

	void FinalizeObject(markerType& marker, char* base)
	{
		if(marker & NULLC::OBJECT_ARRAY)
		{
			ExternTypeInfo &typeInfo = NULLC::linker->exTypes[(unsigned)marker >> 8];

			unsigned arrayPadding = typeInfo.defaultAlign > 4 ? typeInfo.defaultAlign : 4;

			unsigned count = *(unsigned*)(base + sizeof(markerType) + arrayPadding - 4);
			NULLCRef r = { (unsigned)marker >> 8, base + sizeof(markerType) + arrayPadding }; // skip over marker and array size

			for(unsigned i = 0; i < count; i++)
			{
				NULLC::finalizeList.push_back(r);
				r.ptr += typeInfo.size;
			}
		}else{
			NULLCRef r = { (unsigned)marker >> 8, base + sizeof(markerType) }; // skip over marker
			NULLC::finalizeList.push_back(r);
		}
		marker |= NULLC::OBJECT_FINALIZED;
	}
}

template<int elemSize>
union SmallBlock
{
	char			data[elemSize];
	markerType		marker;
	SmallBlock		*next;
};

#pragma pack(push, 1)
template<int elemSize, int countInBlock>
struct LargeBlock
{
	typedef SmallBlock<elemSize> Block;

	// Padding is used to break the 16 byte alignment of pages in a way that after a marker offset is added to the block, the object pointer will be correctly aligned
	char padding[16 - sizeof(markerType)];
	Block		page[countInBlock];

	LargeBlock	*next;
};
#pragma pack(pop)

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
			NULLC::alignedDealloc(activePages);
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
			freeBlocks = (MySmallBlock*)((intptr_t)freeBlocks->next & ~NULLC::OBJECT_MASK);
		}else{
			if(lastNum == countInBlock)
			{
				MyLargeBlock* newPage = new(NULLC::alignedAlloc(sizeof(MyLargeBlock))) MyLargeBlock;
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
		freedBlock->next = (MySmallBlock*)((intptr_t)freeBlocks | NULLC::OBJECT_FREED);
		freeBlocks = freedBlock;
	}
	bool IsBasePointer(void* ptr)
	{
		MyLargeBlock *curr = activePages;
		while(curr)
		{
			if((char*)ptr >= (char*)curr->page && (char*)ptr <= (char*)curr->page + sizeof(MyLargeBlock))
			{
				if(((unsigned int)(intptr_t)((char*)ptr - (char*)curr->page) & (elemSize - 1)) == sizeof(markerType))
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
		return (char*)best->page + (fromBase & ~(elemSize - 1)) + sizeof(markerType);
	}
	void Mark(unsigned int number)
	{
		assert(number <= 1);
		MyLargeBlock *curr = activePages;
		while(curr)
		{
			for(unsigned int i = 0; i < (curr == activePages ? lastNum : countInBlock); i++)
			{
				curr->page[i].marker = (curr->page[i].marker & ~NULLC::OBJECT_VISIBLE) | number;
			}
			curr = curr->next;
		}
	}
	unsigned int FreeMarked()
	{
		unsigned int freed = 0;
		MyLargeBlock *curr = activePages;
		while(curr)
		{
			for(unsigned int i = 0; i < (curr == activePages ? lastNum : countInBlock); i++)
			{
				if(!(curr->page[i].marker & (NULLC::OBJECT_VISIBLE | NULLC::OBJECT_FREED)))
				{
					if((curr->page[i].marker & NULLC::OBJECT_FINALIZABLE) && !(curr->page[i].marker & NULLC::OBJECT_FINALIZED))
					{
						NULLC::FinalizeObject(curr->page[i].marker, curr->page[i].data);
					}else{
						Free(&curr->page[i]);
						freed++;
					}
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

	struct Range
	{
		Range(): start(NULL), end(NULL)
		{
		}
		Range(void* start, void* end): start(start), end(end)
		{
		}

		// Ranges are equal if they intersect
		bool operator==(const Range& rhs) const
		{
			return !(start > rhs.end || end < rhs.start);
		}

		bool operator<(const Range& rhs) const
		{
			return end < rhs.start;
		}

		bool operator>(const Range& rhs) const
		{
			return start > rhs.end;
		}

		void *start, *end;
	};

	typedef Tree<Range>::iterator BigBlockIterator;
	Tree<Range>	bigBlocks;

	unsigned currentMark = 0;
	unsigned unusedBlocks = 0;

	FastVector<Range> toErase;

	void MarkBlock(Range& curr);
	void CollectBlock(Range& curr);
	void FinalizeBlock(Range& curr);
	void ClearBlock(Range& curr);

	double	markTime = 0.0;
	double	collectTime = 0.0;
}

void NULLC::SetLinker(Linker *linker)
{
	NULLC::linker = linker;
}

void* NULLC::AllocObject(int size, unsigned type)
{
	if(size < 0)
	{
		nullcThrowError("ERROR: requested memory size is less than zero");
		return NULL;
	}
	void *data = NULL;
	size += sizeof(markerType);

	if((unsigned int)(usedMemory + size) > globalMemoryLimit)
	{
		CollectMemory();
		if((unsigned int)(usedMemory + size) > globalMemoryLimit)
		{
			nullcThrowError("ERROR: reached global memory maximum");
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
				void *ptr = NULLC::alloc(size + 4);
				if(ptr == NULL)
				{
					nullcThrowError("Allocation failed.");
					return NULL;
				}

				Range range(ptr, (char*)ptr + size + 4);
				bigBlocks.insert(range);

				realSize = *(int*)ptr = size;
				data = (char*)ptr + 4;
			}
		}
	}
	usedMemory += realSize;

	if(data == NULL)
	{
		nullcThrowError("ERROR: allocation failed");
		return NULL;
	}
	int finalize = 0;
	if(type && (linker->exTypes[type].typeFlags & ExternTypeInfo::TYPE_HAS_FINALIZER))
		finalize = (int)OBJECT_FINALIZABLE;

	memset(data, 0, size);
	*(markerType*)data = finalize | (type << 8);
	return (char*)data + sizeof(markerType);
}

unsigned int NULLC::UsedMemory()
{
	return usedMemory;
}

NULLCArray NULLC::AllocArray(unsigned size, unsigned count, unsigned type)
{
	NULLCArray ret;

	ret.len = 0;
	ret.ptr = NULL;

	if((unsigned long long)size * count > globalMemoryLimit)
	{
		nullcThrowError("ERROR: can't allocate array with %u elements of size %u", count, size);
		return ret;
	}

	ExternTypeInfo &typeInfo = NULLC::linker->exTypes[type];

	unsigned arrayPadding = typeInfo.defaultAlign > 4 ? typeInfo.defaultAlign : 4;

	unsigned bytes = count * size;
	
	if(bytes == 0)
		bytes += 4;

	char *ptr = (char*)AllocObject(bytes + arrayPadding, type);

	if(!ptr)
		return ret;

	ret.len = count;
	ret.ptr = arrayPadding + ptr;

	((unsigned*)ret.ptr)[-1] = count;

	markerType *marker = (markerType*)(ptr - sizeof(markerType));
	*marker |= OBJECT_ARRAY;

	return ret;
}

void NULLC::MarkBlock(Range& curr)
{
	markerType *marker = (markerType*)((char*)curr.start + 4);
	*marker = (*marker & ~NULLC::OBJECT_VISIBLE) | currentMark;
}

void NULLC::MarkMemory(unsigned int number)
{
	assert(number <= 1);

	currentMark = number;

	bigBlocks.for_each(MarkBlock);

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
	if(BigBlockIterator it = bigBlocks.find(Range(ptr, ptr)))
	{
		void *block = it->key.start;

		if((char*)ptr - 4 - sizeof(markerType) == block)
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
	if(BigBlockIterator it = bigBlocks.find(Range(ptr, ptr)))
	{
		void *block = it->key.start;

		if(ptr >= block && ptr <= (char*)block + *(unsigned int*)block)
			return (char*)block + 4 + sizeof(markerType);
	}

	return NULL;
}

void NULLC::CollectBlock(Range& curr)
{
	void *block = curr.start;

	markerType &marker = *(markerType*)((char*)block + 4);
	if(!(marker & NULLC::OBJECT_VISIBLE))
	{
		if((marker & NULLC::OBJECT_FINALIZABLE) && !(marker & NULLC::OBJECT_FINALIZED))
		{
			NULLC::FinalizeObject(marker, (char*)block + 4);
		}else{
			usedMemory -= *(unsigned int*)block;
			NULLC::dealloc(block);

			toErase.push_back(curr);

			unusedBlocks++;
		}
	}
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
	unusedBlocks = 0;

	toErase.clear();

	bigBlocks.for_each(CollectBlock);

	for(unsigned i = 0; i < toErase.size(); i++)
		bigBlocks.erase(toErase[i]);

	toErase.clear();

	//printf("%d unused globally allocated blocks destroyed\r\n", unusedBlocks);

	//printf("%d used memory\r\n", usedMemory);

	// Objects allocated from pools are freed
	unusedBlocks = pool8.FreeMarked();
	usedMemory -= unusedBlocks * 8;
//	printf("%d unused pool blocks freed (8 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool16.FreeMarked();
	usedMemory -= unusedBlocks * 16;
//	printf("%d unused pool blocks freed (16 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool32.FreeMarked();
	usedMemory -= unusedBlocks * 32;
//	printf("%d unused pool blocks freed (32 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool64.FreeMarked();
	usedMemory -= unusedBlocks * 64;
//	printf("%d unused pool blocks freed (64 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool128.FreeMarked();
	usedMemory -= unusedBlocks * 128;
//	printf("%d unused pool blocks freed (128 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool256.FreeMarked();
	usedMemory -= unusedBlocks * 256;
//	printf("%d unused pool blocks freed (256 bytes)\r\n", unusedBlocks);
	unusedBlocks = pool512.FreeMarked();
	usedMemory -= unusedBlocks * 512;
//	printf("%d unused pool blocks freed (512 bytes)\r\n", unusedBlocks);

//	printf("%d used memory\r\n", usedMemory);

	collectTime += (double(clock()) / CLOCKS_PER_SEC) - time;

	if(usedMemory + (usedMemory >> 1) >= collectableMinimum)
		collectableMinimum <<= 1;

	nullcRunFunction("__finalizeObjects");
	finalizeList.clear();
}

double NULLC::MarkTime()
{
	return markTime;
}

double NULLC::CollectTime()
{
	return collectTime;
}

void NULLC::FinalizeBlock(Range& curr)
{
	void *block = curr.start;

	markerType &marker = *(markerType*)((char*)block + 4);
	if(!(marker & NULLC::OBJECT_VISIBLE) && (marker & NULLC::OBJECT_FINALIZABLE) && !(marker & NULLC::OBJECT_FINALIZED))
		NULLC::FinalizeObject(marker, (char*)block + 4);
}

void NULLC::FinalizeMemory()
{
	MarkMemory(0);
	pool8.FreeMarked();
	pool16.FreeMarked();
	pool32.FreeMarked();
	pool64.FreeMarked();
	pool128.FreeMarked();
	pool256.FreeMarked();
	pool512.FreeMarked();

	bigBlocks.for_each(FinalizeBlock);

	nullcRunFunction("__finalizeObjects");
	finalizeList.clear();
}

void NULLC::ClearBlock(Range& curr)
{
	NULLC::dealloc(curr.start);
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

	bigBlocks.for_each(ClearBlock);
	bigBlocks.clear();

	toErase.clear();

	finalizeList.clear();
}

void NULLC::ResetMemory()
{
	ClearMemory();

	bigBlocks.reset();
	toErase.reset();

	finalizeList.reset();
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
	if(!dst)
	{
		nullcThrowError("ERROR: null pointer access");
		return;
	}
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

void NULLC::AssignObject(NULLCRef l, NULLCRef r)
{
	if(linker->exTypes[l.typeID].subType != r.typeID)
	{
		nullcThrowError("ERROR: can't assign value of type %s to a pointer of type %s", &linker->exSymbols[linker->exTypes[r.typeID].offsetToName], &linker->exSymbols[linker->exTypes[l.typeID].offsetToName]);
		return;
	}
	memcpy(l.ptr, &r.ptr, linker->exTypes[l.typeID].size);
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

int NULLC::Char(char a)
{
	return a;
}

int NULLC::Short(short a)
{
	return a;
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

int NULLC::UnsignedValueChar(unsigned char a)
{
	return a;
}

int NULLC::UnsignedValueShort(unsigned short a)
{
	return a;
}

long long NULLC::UnsignedValueInt(unsigned int a)
{
	return a;
}

int NULLC::StrToShort(NULLCArray str)
{
	return str.ptr ? short(atoi(str.ptr)) : 0;
}

NULLCArray NULLC::ShortToStr(short* r)
{
	NULLCArray arr = { 0 };
	if(!r)
	{
		nullcThrowError("ERROR: null pointer access");
		return arr;
	}

	short number = *r;
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
	arr = AllocArray(1, (int)(curr - buf) + 1);
	char *str = arr.ptr;
	do 
	{
		--curr;
		*str++ = *curr;
	}while(curr != buf);
	return arr;
}

int NULLC::StrToInt(NULLCArray str)
{
	return str.ptr ? atoi(str.ptr) : 0;
}

NULLCArray NULLC::IntToStr(int* r)
{
	NULLCArray arr = { 0 };
	if(!r)
	{
		nullcThrowError("ERROR: null pointer access");
		return arr;
	}

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
	arr = AllocArray(1, (int)(curr - buf) + 1);
	char *str = arr.ptr;
	do 
	{
		--curr;
		*str++ = *curr;
	}while(curr != buf);
	return arr;
}

long long NULLC::StrToLong(NULLCArray str)
{
	if(!str.ptr)
		return 0;

	const char *p = str.ptr;

	bool negative = *p == '-';
	if(negative)
		p++;

	unsigned long long res = 0;
	while(unsigned(*p - '0') < 10)
		res = res * 10 + unsigned(*p++ - '0');

	return res * (negative ? -1 : 1);
}

NULLCArray NULLC::LongToStr(long long* r)
{
	NULLCArray arr = { 0 };
	if(!r)
	{
		nullcThrowError("ERROR: null pointer access");
		return arr;
	}

	long long number = *r;
	bool sign = 0;
	char buf[32];
	char *curr = buf;
	if(number < 0)
		sign = 1;

	*curr++ = (char)(abs(number % 10) + '0');
	while(number /= 10)
		*curr++ = (char)(abs(number % 10) + '0');
	if(sign)
		*curr++ = '-';
	arr = AllocArray(1, (int)(curr - buf) + 1);
	char *str = arr.ptr;
	do 
	{
		--curr;
		*str++ = *curr;
	}while(curr != buf);
	return arr;
}

float NULLC::StrToFloat(NULLCArray str)
{
	return str.ptr ? float(atof(str.ptr)) : 0;
}

NULLCArray NULLC::FloatToStr(int precision, bool exponent, float* r)
{
	NULLCArray arr = { 0 };
	if(!r)
	{
		nullcThrowError("ERROR: null pointer access");
		return arr;
	}

	char buf[256];
	SafeSprintf(buf, 256, exponent ? "%.*e" : "%.*f", precision, *r);
	arr = AllocArray(1, (int)strlen(buf) + 1);
	memcpy(arr.ptr, buf, arr.len);
	return arr;
}

double NULLC::StrToDouble(NULLCArray str)
{
	return str.ptr ? atof(str.ptr) : 0;
}

NULLCArray NULLC::DoubleToStr(int precision, bool exponent, double* r)
{
	NULLCArray arr = { 0 };
	if(!r)
	{
		nullcThrowError("ERROR: null pointer access");
		return arr;
	}

	char buf[256];
	SafeSprintf(buf, 256, exponent ? "%.*e" : "%.*f", precision, *r);
	arr = AllocArray(1, (int)strlen(buf) + 1);
	memcpy(arr.ptr, buf, arr.len);
	return arr;
}

NULLCFuncPtr NULLC::FunctionRedirect(NULLCRef r, NULLCArray* arr)
{
	NULLCFuncPtr ret = { 0, 0 };

	if(!arr)
	{
		nullcThrowError("ERROR: null pointer access");
		return ret;
	}

	unsigned int *funcs = (unsigned int*)arr->ptr;
	if(r.typeID >= arr->len)
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

NULLCFuncPtr NULLC::FunctionRedirectPtr(NULLCRef r, NULLCArray* arr)
{
	NULLCFuncPtr ret = { 0, 0 };

	if(!arr)
	{
		nullcThrowError("ERROR: null pointer access");
		return ret;
	}

	unsigned int *funcs = (unsigned int*)arr->ptr;

	if(r.typeID >= arr->len)
	{
		nullcThrowError("ERROR: type index is out of bounds of redirection table");
		return ret;
	}

	ret.context = funcs[r.typeID] ? r.ptr : 0;
	ret.id = funcs[r.typeID];

	return ret;
}

NULLC::TypeIDHelper NULLC::Typeid(NULLCRef r)
{
	TypeIDHelper help;

	if(linker->exTypes[r.typeID].typeFlags & ExternTypeInfo::TYPE_IS_EXTENDABLE)
		help.id = *(int*)r.ptr;
	else
		help.id = r.typeID;

	return help;
}

int NULLC::TypeSize(int* a)
{
	if(!a)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

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

int NULLC::RefLCompare(NULLCRef a, NULLCRef b)
{
	return a.ptr < b.ptr;
}

int NULLC::RefLECompare(NULLCRef a, NULLCRef b)
{
	return a.ptr <= b.ptr;
}

int NULLC::RefGCompare(NULLCRef a, NULLCRef b)
{
	return a.ptr > b.ptr;
}

int NULLC::RefGECompare(NULLCRef a, NULLCRef b)
{
	return a.ptr >= b.ptr;
}

int NULLC::RefHash(NULLCRef a)
{
	long long value = (long long)(intptr_t)(a.ptr);
	return (int)((value >> 32) ^ value);
}

int NULLC::FuncCompare(NULLCFuncPtr a, NULLCFuncPtr b)
{
	return a.context == b.context && a.id == b.id;
}
int NULLC::FuncNCompare(NULLCFuncPtr a, NULLCFuncPtr b)
{
	return a.context != b.context || a.id != b.id;
}

int NULLC::ArrayCompare(NULLCAutoArray a, NULLCAutoArray b)
{
	return a.len == b.len && a.ptr == b.ptr;
}
int NULLC::ArrayNCompare(NULLCAutoArray a, NULLCAutoArray b)
{
	return a.len != b.len || a.ptr != b.ptr;
}

int NULLC::TypeCount()
{
	return nullcGetTypeCount();
}

NULLCAutoArray* NULLC::AutoArrayAssign(NULLCAutoArray* left, NULLCRef right)
{
	if(!left)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}
	if(right.typeID == NULLC_TYPE_AUTO_ARRAY)
	{
		*left = *(NULLCAutoArray*)right.ptr;
		return left;
	}
	if(!nullcIsArray(right.typeID))
	{
		nullcThrowError("ERROR: cannot convert from '%s' to 'auto[]'", nullcGetTypeName(right.typeID));
		return 0;
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

	if(!right)
	{
		nullcThrowError("ERROR: null pointer access");
		return ret;
	}
	if(left.typeID == NULLC_TYPE_AUTO_ARRAY)
	{
		*(NULLCAutoArray*)left.ptr = *right;
		return ret;
	}
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
	if(!left || !right)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

	left->len = right->len;
	left->ptr = right->ptr;
	left->typeID = right->typeID;
	return left;
}

NULLCRef NULLC::AutoArrayIndex(NULLCAutoArray* left, unsigned int index)
{
	NULLCRef ret = { 0, 0 };

	if(!left)
	{
		nullcThrowError("ERROR: null pointer access");
		return ret;
	}
	if(index >= left->len)
	{
		nullcThrowError("ERROR: array index out of bounds");
		return ret;
	}
	ret.typeID = left->typeID;
	ret.ptr = (char*)left->ptr + index * nullcGetTypeSize(ret.typeID);
	return ret;
}

void NULLC::AutoArray(NULLCAutoArray* arr, int type, unsigned count)
{
	if(!arr)
	{
		nullcThrowError("ERROR: null pointer access");
		return;
	}
	if((unsigned long long)count * linker->exTypes[type].size > globalMemoryLimit)
	{
		nullcThrowError("ERROR: can't allocate array with %u elements of size %u", count, linker->exTypes[type].size);
		return;
	}

	arr->typeID = type;
	arr->len = count;
	arr->ptr = (char*)AllocObject(count * linker->exTypes[type].size);
}

void NULLC::AutoArraySet(NULLCRef x, unsigned pos, NULLCAutoArray* arr)
{
	if(!arr)
	{
		nullcThrowError("ERROR: null pointer access");
		return;
	}
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
	if(!arr)
	{
		nullcThrowError("ERROR: null pointer access");
		return;
	}
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

NULLCArray NULLC::GetFinalizationList()
{
	NULLCArray arr;
	arr.ptr = (char*)finalizeList.data;
	arr.len = finalizeList.size();
	return arr;
}

void NULLC::ArrayCopy(NULLCAutoArray dst, NULLCAutoArray src)
{
	if(dst.ptr == src.ptr)
		return;

	if(dst.typeID != src.typeID)
	{
		nullcThrowError("ERROR: destination element type '%s' doesn't match source element type '%s'", nullcGetTypeName(dst.typeID), nullcGetTypeName(src.typeID));
		return;
	}
	if(dst.len < src.len)
	{
		nullcThrowError("ERROR: destination array size '%d' is smaller than source array size '%s'", dst.len, src.len);
		return;
	}

	memcpy(dst.ptr, src.ptr, nullcGetTypeSize(dst.typeID) * src.len);
}

void* NULLC::AssertDerivedFromBase(unsigned* derived, unsigned base)
{
	if(!derived)
		return derived;

	unsigned typeId = *derived;
	for(;;)
	{
		if(base == typeId)
			return derived;
		if(linker->exTypes[typeId].baseType)
		{
			typeId = linker->exTypes[typeId].baseType;
		}else{
			break;
		}
	}
	nullcThrowError("ERROR: cannot convert from '%s' to '%s'", nullcGetTypeName(*derived), nullcGetTypeName(base));
	return derived;
}
