#pragma once

#include "Allocator.h"

// Only for zero-initialized POD types
template<typename Key, typename Value, typename Hasher, unsigned N>
class SmallDenseMap
{
public:
	struct Node
	{
		Key key;
		Value value;
	};

	SmallDenseMap(Allocator *allocator = 0): allocator(allocator)
	{
		assert(N >= 2 && "size is too small");
		assert((N & (N - 1)) == 0 && "only power of 2 sizes are supported");

		bucketCount = N;

		count = 0;

		data = storage;

		memset(data, 0, sizeof(Node) * bucketCount);
	}

	~SmallDenseMap()
	{
		if(data != storage)
		{
			if(allocator)
				allocator->dealloc(data);
			else
				NULLC::dealloc(data);
		}
	}

	void set_allocator(Allocator *newAllocator)
	{
		assert(count == 0);
		assert(data == storage);

		this->allocator = newAllocator;
	}

	void clear()
	{
		count = 0;

		memset(data, 0, sizeof(Node) * bucketCount);
	}

	void insert(const Key& key, const Value& value)
	{
		// Default value is used as an empty marker
		assert(key != Key());

		// Keep occupancy at <80%
		if(count + (count >> 2) >= bucketCount)
			rehash();

		unsigned bucketMask = bucketCount - 1;
		unsigned bucket = Hasher()(key) & bucketMask;

		for(unsigned i = 0; i < bucketCount; i++)
		{
			Node &item = data[bucket];

			if(item.key == Key())
			{
				item.key = key;
				item.value = value;

				count++;
				return;
			}

			if(item.key == key)
			{
				item.value = value;
				return;
			}

			// Quadratic probing
			bucket = (bucket + i + 1) & bucketMask;
		}

		assert(!"couldn't insert node");
	}

	Value* find(const Key& key) const
	{
		unsigned bucketMask = bucketCount - 1;
		unsigned bucket = Hasher()(key) & bucketMask;

		for(unsigned i = 0; i < bucketCount; i++)
		{
			Node &item = data[bucket];

			if(item.key == Key())
				return NULL;

			if(item.key == key)
				return &item.value;

			// Quadratic probing
			bucket = (bucket + i + 1) & bucketMask;
		}

		return NULL;
	}

	void rehash()
	{
		unsigned oldBucketCount = bucketCount;
		Node *oldData = data;

		bucketCount *= 2;

		if(allocator)
			data = (Node*)allocator->alloc(sizeof(Node) * bucketCount);
		else
			data = (Node*)NULLC::alloc(sizeof(Node) * bucketCount);

		memset(data, 0, sizeof(Node) * bucketCount);

		count = 0;

		for(unsigned i = 0; i < oldBucketCount; i++)
		{
			Node &item = oldData[i];

			if(item.key == Key())
				continue;

			insert(item.key, item.value);
		}

		if(oldData != storage)
		{
			if(allocator)
				allocator->dealloc(oldData);
			else
				NULLC::dealloc(oldData);
		}
	}

	unsigned size() const
	{
		return count;
	}

private:
	unsigned bucketCount;
	unsigned count;

	Node *data;
	Node storage[N];

	Allocator *allocator;

private:
	SmallDenseMap(const SmallDenseMap&);
	SmallDenseMap& operator=(const SmallDenseMap&);
};

template<typename Key, typename Hasher, unsigned N>
class SmallDenseSet
{
public:
	SmallDenseSet(Allocator *allocator = 0): allocator(allocator)
	{
		assert(N >= 2 && "size is too small");
		assert((N & (N - 1)) == 0 && "only power of 2 sizes are supported");

		bucketCount = N;

		count = 0;

		data = storage;

		memset(data, 0, sizeof(Key) * bucketCount);
	}

	~SmallDenseSet()
	{
		if(data != storage)
		{
			if(allocator)
				allocator->dealloc(data);
			else
				NULLC::dealloc(data);
		}
	}

	void set_allocator(Allocator *newAllocator)
	{
		assert(count == 0);
		assert(data == storage);

		this->allocator = newAllocator;
	}

	void clear()
	{
		count = 0;

		memset(data, 0, sizeof(Key) * bucketCount);
	}

	void insert(const Key& key)
	{
		// Default value is used as an empty marker
		assert(key != Key());

		// Keep occupancy at <80%
		if(count + (count >> 2) >= bucketCount)
			rehash();

		unsigned bucketMask = bucketCount - 1;
		unsigned bucket = Hasher()(key) & bucketMask;

		for(unsigned i = 0; i < bucketCount; i++)
		{
			Key &item = data[bucket];

			if(item == Key())
			{
				item = key;

				count++;
				return;
			}

			if(item == key)
				return;

			// Quadratic probing
			bucket = (bucket + i + 1) & bucketMask;
		}

		assert(!"couldn't insert node");
	}

	bool contains(const Key& key)
	{
		unsigned bucketMask = bucketCount - 1;
		unsigned bucket = Hasher()(key) & bucketMask;

		for(unsigned i = 0; i < bucketCount; i++)
		{
			Key &item = data[bucket];

			if(item == Key())
				return false;

			if(item == key)
				return true;

			// Quadratic probing
			bucket = (bucket + i + 1) & bucketMask;
		}

		return false;
	}

	void rehash()
	{
		unsigned oldBucketCount = bucketCount;
		Key *oldData = data;

		bucketCount *= 2;

		if(allocator)
			data = (Key*)allocator->alloc(sizeof(Key) * bucketCount);
		else
			data = (Key*)NULLC::alloc(sizeof(Key) * bucketCount);

		memset(data, 0, sizeof(Key) * bucketCount);

		count = 0;

		for(unsigned i = 0; i < oldBucketCount; i++)
		{
			Key &item = oldData[i];

			if(item == Key())
				continue;

			insert(item);
		}

		if(oldData != storage)
		{
			if(allocator)
				allocator->dealloc(oldData);
			else
				NULLC::dealloc(oldData);
		}
	}

private:
	unsigned bucketCount;
	unsigned count;

	Key *data;
	Key storage[N];

	Allocator *allocator;

private:
	SmallDenseSet(const SmallDenseSet&);
	SmallDenseSet& operator=(const SmallDenseSet&);
};

struct SmallDenseMapUnsignedHasher
{
	unsigned operator()(unsigned value) const
	{
		return value * 2654435769u;
	}
};

template<typename Value>
class DirectChainedMap
{
public:
	struct Node
	{
		Node()
		{
			hash = 0;
			value = Value();
			next = NULL;
		}

		unsigned hash;
		Value value;
		Node *next;
	};

	struct NodeIterator
	{
		NodeIterator()
		{
			start = NULL;
			node = NULL;
		}

		NodeIterator(Node *start, Node *node)
		{
			this->start = start;
			this->node = node;
		}

		Node *start;
		Node *node;

		// Safe bool cast
		typedef void (NodeIterator::*bool_type)() const;
		void safe_bool() const{}

		operator bool_type() const
		{
			return node ? &NodeIterator::safe_bool : 0;
		}
	};

	DirectChainedMap(Allocator *allocator) : allocator(allocator)
	{
		assert(allocator);

		bucketCount = 4;
		count = 0;

		data = (Node*)allocator->alloc(sizeof(Node) * bucketCount);
		memset(data, 0, sizeof(Node) * bucketCount);
	}

	~DirectChainedMap()
	{
		allocator->dealloc(data);
	}

	void clear()
	{
		memset(data, 0, sizeof(Node) * bucketCount);

		count = 0;
	}

	void insert(unsigned hash, Value value)
	{
		// Keep occupancy at <80%
		if(count + (count >> 2) >= bucketCount)
			rehash();

		count++;

		unsigned bucketMask = bucketCount - 1;
		unsigned bucket = hash & bucketMask;

		Node *target = &data[bucket];

		if(target->next == NULL)
		{
			target->hash = hash;
			target->value = value;
			target->next = target; // Cycle lets us know that this is the last node
		}
		else
		{
			Node *n = (Node*)allocator->alloc(sizeof(Node));

			n->hash = target->hash;
			n->value = target->value;
			n->next = target->next;

			target->hash = hash;
			target->value = value;
			target->next = n;
		}
	}

	void remove(unsigned hash, Value value)
	{
		unsigned bucketMask = bucketCount - 1;
		unsigned bucket = hash & bucketMask;

		Node *start = &data[bucket];
		Node *curr = start;
		Node *prev = NULL;

		if(!curr->next)
		{
			assert(!"element is not in the map");
			return;
		}

		while(curr)
		{
			if(curr->hash == hash && curr->value == value)
				break;

			prev = curr;
			curr = curr->next;

			if(curr == start)
				return;
		}

		count--;

		assert(curr && "element is not in the map");

		if(prev)
		{
			prev->next = curr->next;

			allocator->dealloc(curr);
		}
		else if(start->next == start)
		{
			start->next = NULL;
		}
		else
		{
			allocator->dealloc(start->next);

			*start = *start->next;
		}
	}

	Value* find(unsigned hash)
	{
		unsigned bucketMask = bucketCount - 1;
		unsigned bucket = hash & bucketMask;

		Node *start = &data[bucket];
		Node *curr = start;

		if(!curr->next)
			return NULL;

		while(curr)
		{
			if(curr->hash == hash)
				return &curr->value;

			curr = curr->next;

			if(curr == start)
				return NULL;
		}

		return NULL;
	}

	NodeIterator first(unsigned hash)
	{
		unsigned bucketMask = bucketCount - 1;
		unsigned bucket = hash & bucketMask;

		Node *start = &data[bucket];
		Node *curr = start;

		if(!curr->next)
			return NodeIterator();

		while(curr)
		{
			if(curr->hash == hash)
				return NodeIterator(start, curr);

			curr = curr->next;

			if(curr == start)
				return NodeIterator();
		}

		return NodeIterator();
	}

	NodeIterator next(NodeIterator it)
	{
		Node *start = it.start;
		Node *node = it.node;

		unsigned hash = node->hash;

		node = node->next;

		if(node == start)
			return NodeIterator();

		while(node)
		{
			if(node->hash == hash)
				return NodeIterator(start, node);

			node = node->next;

			if(node == start)
				return NodeIterator();
		}

		return NodeIterator();
	}

	void rehash_insert_reverse(Node *start, Node *node)
	{
		Node *next = node->next;

		if(next != start)
			rehash_insert_reverse(start, next);

		insert(node->hash, node->value);
	}

	void rehash()
	{
		unsigned oldBucketCount = bucketCount;
		Node *oldData = data;

		bucketCount *= 2;
		count = 0;

		data = (Node*)allocator->alloc(sizeof(Node) * bucketCount);
		memset(data, 0, sizeof(Node) * bucketCount);

		for(unsigned i = 0; i < oldBucketCount; i++)
		{
			Node *start = &oldData[i];

			if(start->next == NULL)
				continue;

			rehash_insert_reverse(start, start);
		}

		allocator->dealloc(oldData);
	}

private:
	unsigned bucketCount;
	unsigned count;

	Node *data;

	Allocator *allocator;

private:
	DirectChainedMap(const DirectChainedMap&);
	DirectChainedMap& operator=(const DirectChainedMap&);
};
