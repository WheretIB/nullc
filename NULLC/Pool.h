#pragma once
#ifndef NULLC_POOL_H
#define NULLC_POOL_H

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

#endif
