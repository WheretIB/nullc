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
