#pragma once

#include "Pool.h"

template<typename Value>
class HashMap
{
	static const unsigned int	bucketCount = 1024;
	static const unsigned int	bucketMask = bucketCount - 1;

	ChunkedStackPool<4092>		nodePool;
public:
	struct Node
	{
		unsigned int	hash;
		Value			value;
		Node			*next;
	};

	HashMap()
	{
		entries = NULL;
	}
	void init()
	{
		if(!entries)
		{
			entries = NULLC::construct<Node*>(bucketCount);
			memset(entries, 0, sizeof(Node*) * bucketCount);
		}
	}
	~HashMap()
	{
		if(entries)
			NULLC::destruct(entries, bucketCount);
		entries = NULL;
	}
	void reset()
	{
		if(entries)
			NULLC::destruct(entries, bucketCount);
		entries = NULL;
		nodePool.~ChunkedStackPool();
	}

	void clear()
	{
		nodePool.Clear();
		memset(entries, 0, sizeof(Node*) * bucketCount);
	}

	void insert(unsigned int hash, Value value)
	{
		unsigned int bucket = hash & bucketMask;
		Node *n = (Node*)nodePool.Allocate(sizeof(Node));
		n->value = value;
		n->hash = hash;
		n->next = entries[bucket];
		entries[bucket] = n;
	}
	void remove(unsigned int hash, Value value)
	{
		unsigned int bucket = hash & bucketMask;
		Node *curr = entries[bucket], *prev = NULL;
		while(curr)
		{
			if(curr->hash == hash && curr->value == value)
				break;
			prev = curr;
			curr = curr->next;
		}
		assert(curr);
		if(prev)
			prev->next = curr->next;
		else
			entries[bucket] = curr->next;
	}

	Value* find(unsigned int hash)
	{
		Node *n = first(hash);
		return n ? &n->value : NULL;
	}
	Node* first(unsigned int hash)
	{
		unsigned int bucket = hash & bucketMask;
		Node *curr = entries[bucket];
		while(curr)
		{
			if(curr->hash == hash)
				return curr;
			curr = curr->next;
		}
		return NULL;
	}
	Node* next(Node* curr)
	{
		unsigned int hash = curr->hash;
		curr = curr->next;
		while(curr)
		{
			if(curr->hash == hash)
				return curr;
			curr = curr->next;
		}
		return NULL;
	}
private:
	Node	**entries;
};
