#pragma once

template<int chunkSize>
class ChunkedStackPool
{
public:
	ChunkedStackPool()
	{
		curr = first = NULL;
		size = chunkSize;
	}
	~ChunkedStackPool()
	{
		Reset();
	}

	void Reset()
	{
		while(first)
		{
			StackChunk *next = first->next;
			NULLC::dealloc(first);
			first = next;
		}
		curr = first = NULL;
		size = chunkSize;
	}

	void	Clear()
	{
		curr = first;
		size = first ? 0 : chunkSize;
	}

	void	ClearTo(unsigned int bytes)
	{
		if(!first)
			return;
		curr = first;
		while(bytes > chunkSize)
		{
			assert(curr->next != NULL);
			curr = curr->next;
			bytes -= chunkSize;
		}
		size = bytes;
	}

	void*	Allocate(unsigned int bytes)
	{
		bytes = (bytes + 7) & ~7; // 8-byte align

		assert(bytes <= chunkSize);
		if(size + bytes <= chunkSize)
		{
			size += bytes;
			return curr->data + size - bytes;
		}
		if(curr && curr->next)
		{
			curr = curr->next;
			size = bytes;
			return curr->data;
		}
		if(curr)
		{
			curr->next = new(NULLC::alloc(sizeof(StackChunk))) StackChunk;
			curr = curr->next;
			curr->next = NULL;
		}else{
			curr = first = new(NULLC::alloc(sizeof(StackChunk))) StackChunk;
			curr->next = NULL;
		}
		size = bytes;
		return curr->data;
	}

	unsigned int GetSize()
	{
		unsigned int wholeSize = 0;
		StackChunk *temp = first;
		while(temp != curr)
		{
			wholeSize += chunkSize;
			temp = temp->next;
		}
		return wholeSize + size;
	}
private:
	struct StackChunk
	{
		StackChunk *next;
		char		data[chunkSize];
	};
	StackChunk	*first, *curr;
	unsigned int size;
};

namespace detail
{
	template<typename T>
	union SmallBlock
	{
		char		data[sizeof(T)];
		SmallBlock	*next;
	};

	template<typename T, int countInBlock>
	struct LargeBlock
	{
		typedef SmallBlock<T> Block;
		Block		page[countInBlock];
		LargeBlock	*next;
	};
}

template<typename T, int countInBlock>
class TypedObjectPool
{
	typedef detail::SmallBlock<T> MySmallBlock;
	typedef typename detail::LargeBlock<T, countInBlock> MyLargeBlock;
public:
	TypedObjectPool()
	{
		freeBlocks = NULL;
		activePages = NULL;
		lastNum = countInBlock;
	}
	~TypedObjectPool()
	{
		Reset();
	}
	void Reset()
	{
		freeBlocks = NULL;
		lastNum = countInBlock;

		while(activePages)
		{
			MyLargeBlock *following = activePages->next;
			NULLC::dealloc(activePages);
			activePages = following;
		}
	}

	T* Allocate()
	{
		MySmallBlock *result;
		if(freeBlocks)
		{
			result = freeBlocks;
			freeBlocks = freeBlocks->next;
		}else{
			if(lastNum == countInBlock)
			{
				MyLargeBlock *newPage = new(NULLC::alloc(sizeof(MyLargeBlock))) MyLargeBlock;
				newPage->next = activePages;
				activePages = newPage;
				lastNum = 0;
			}
			result = &activePages->page[lastNum++];
		}
		return new(result) T;
	}

	void Deallocate(T* ptr)
	{
		if(!ptr)
			return;

		MySmallBlock *freedBlock = (MySmallBlock*)(void*)ptr;
		ptr->~T();	// Destroy object

		freedBlock->next = freeBlocks;
		freeBlocks = freedBlock;
	}
public:
	MySmallBlock	*freeBlocks;
	MyLargeBlock	*activePages;
	unsigned		lastNum;
};
