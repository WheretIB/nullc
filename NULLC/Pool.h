#pragma once
#ifndef NULLC_POOL_H
#define NULLC_POOL_H

template<int chunkSize>
class ChunkedStackPool
{
public:
	ChunkedStackPool()
	{
		curr = first = &one;
		first->next = NULL;
		size = 0;
	}
	~ChunkedStackPool()
	{
		first = first->next;
		while(first)
		{
			StackChunk *next = first->next;
			NULLC::dealloc(first);
			first = next;
		}
		curr = first = &one;
		first->next = NULL;
		size = 0;
	}

	void	Clear()
	{
		curr = first;
		size = 0;
	}
	void	ClearTo(unsigned int bytes)
	{
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
		assert(bytes < chunkSize);
		if(size + bytes < chunkSize)
		{
			size += bytes;
			return curr->data + size - bytes;
		}
		if(curr->next)
		{
			curr = curr->next;
			size = bytes;
			return curr->data;
		}
		curr->next = new(NULLC::alloc(sizeof(StackChunk))) StackChunk;
		curr = curr->next;
		curr->next = NULL;
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
	StackChunk	one;
	unsigned int size;
};

#endif
