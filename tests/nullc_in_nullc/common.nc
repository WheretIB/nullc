import std.hashmap;
import std.vector;

int hash_value(int key)
{
	return key * 2654435761l;
}

auto hashmap::insert(Key key, Value value)
{
	int hash = compute_hash(key);
	int bucket = hash & bucketMask;
	auto n = new Node;
	@if(typeof(key).isArray)
	{
		auto[] tmp = key;
		n.key = duplicate(tmp);
	}else{
		@if(typeof(key) == auto ref)
		{
			n.key = key;
		}else{
			n.key = duplicate(key);
		}
	}
	n.hash = hash;
	n.value = value;
	n.next = entries[bucket];
	entries[bucket] = n;
	return &n.value;
}

bool vector::empty()
{
	return size() == 0;
}

auto hashmap::first(Key key)
{
	int hash = compute_hash(key);
	int bucket = hash & bucketMask;
	Node ref curr = entries[bucket];
	while(curr)
	{
		if(curr.hash == hash && curr.key == key)
			return curr;

		curr = curr.next;
	}
	return nullptr;
}

bool hashmap::contains(Key key)
{
	return find(key) != nullptr;
}

auto hashmap::next(hashmap_node<Key, Value> ref node)
{
	Node ref curr = node.next;

	while(curr)
	{
		if(curr.hash == node.hash && curr.key == node.key)
			return curr;

		curr = curr.next;
	}

	return nullptr;
}

auto hashmap::find(Key key, Value value)
{
	int hash = compute_hash(key);
	int bucket = hash & bucketMask;
	Node ref curr = entries[bucket];
	while(curr)
	{
		if(curr.hash == hash && curr.key == key && curr.value == value)
			return &curr.value;
		curr = curr.next;
	}
	return nullptr;
}

void hashmap::remove(Key key, Value value)
{
	int hash = compute_hash(key);
	int bucket = hash & bucketMask;
	Node ref curr = entries[bucket], prev = nullptr;
	while(curr)
	{
		if(curr.hash == hash && curr.key == key && curr.value == value)
			break;
		prev = curr;
		curr = curr.next;
	}
	assert(!!curr);
	if(prev)
		prev.next = curr.next;
	else
		entries[bucket] = curr.next;
}

void vector::push_back(T[] val, int size)
{
	for(int i = 0; i < size; i++)
		push_back(val[i]);
}

void vector::shrink(int size)
{
	assert(size <= count);

	count = size;
}
