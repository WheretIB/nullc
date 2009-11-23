#pragma once

#pragma warning(disable: 4275)
#define _HAS_EXCEPTIONS 0
#include <vector>

template<typename T>
class ObjectPool
{
public:
	ObjectPool()
	{
		freeList.reserve(16);
	}
	~ObjectPool()
	{
		for(vector<void*>::iterator s = freeList.begin(), e = freeList.end(); s != e; s++)
			delete *s;
	}

	T*		Create()
	{
		if(!freeList.empty())
		{
			T* ptr = (T*)freeList.back();
			freeList.pop_back();
			return new (ptr) T();
		}
		return new T();
	}
	void	Destroy(T* ptr)
	{
		ptr->~T();
		freeList.push_back(ptr);
	}
private:
	std::vector<void*> freeList;
};

template<typename T>
union SmallBlock
{
	char	data[sizeof(T)];
	SmallBlock*	next;
};

template<typename T, int countInBlock>
struct LargeBlock
{
	typedef typename SmallBlock<T> Block;
	Block	page[countInBlock];
	LargeBlock*	next;
};

template<typename T, int countInBlock>
class ObjectBlockPool
{
	typedef typename SmallBlock<T> MySmallBlock;
	typedef typename LargeBlock<T, countInBlock> MyLargeBlock;
public:
	ObjectBlockPool()
	{
		freeBlocks = NULL;
		activePages = NULL;
		lastNum = countInBlock;
		allocated = 0;
	}
	~ObjectBlockPool()
	{
		if(!activePages)
			return;
		do
		{
			MyLargeBlock* following = activePages->next;
			delete activePages;
			activePages = following;
		}while(activePages != NULL);
	}

	T*		Create()
	{
		allocated++;
		MySmallBlock*	result;
		if(freeBlocks)
		{
			result = freeBlocks;
			freeBlocks = freeBlocks->next;
			
		}else{
			if(lastNum == countInBlock)
			{
				MyLargeBlock* newPage = new MyLargeBlock;
				newPage->next = activePages;
				activePages = newPage;
				lastNum = 0;
			}
			result = &activePages->page[lastNum++];
		}
		return ::new(static_cast<void*>(result)) T();
	}

	void	Destroy(T* ptr)
	{
		if(!ptr)
			return;
		allocated--;
		MySmallBlock* freedBlock = static_cast<MySmallBlock*>(static_cast<void*>(ptr));
		ptr->~T();	//Destroy object

		freedBlock->next = freeBlocks;
		freeBlocks = freedBlock;
	}
	int		GetAllocatedCount()
	{
		return allocated;
	}
private:
	MySmallBlock*	freeBlocks;
	MyLargeBlock*	activePages;
	unsigned int	lastNum;
	int		allocated;
};