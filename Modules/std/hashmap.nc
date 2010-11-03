// sgl.hashmap

class hashmap_node<Key, Value>
{
	int			hash;
	Key			key;
	Value		value;
	hashmap_node<Key, Value> ref next;
}
class hashmap<Key, Value>
{
	typedef hashmap_node<Key, Value> Node;
	const int bucketCount = 1024;
	const int bucketMask = bucketCount - 1;

	Node ref[]	entries;
	int ref(Key)	compute_hash;
}

void hashmap:hashmap()
{
	entries = new hashmap_node<Key, Value> ref[bucketCount];
	this.compute_hash = hash_value;
}
void hashmap:hashmap(int ref(Key) compute_hash)
{
	entries = new hashmap_node<Key, Value> ref[bucketCount];
	this.compute_hash = compute_hash;
}

void hashmap:clear()
{
	for(i in entries)
		i = nullptr;
}
auto operator[](hashmap<@K, @V> ref m, typeof(m).target.Key key)
{
	@if(typeof(key) != K){ *"operand type is not equal to hashmap key type"; }
	
	//auto hash = m.compute_hash(a);
	auto x = m.find(key);
	if(x) // if a key-value exists
	{
		return x;
	}else{ // otherwise, add 
		int hash = m.compute_hash(key);
		int bucket = hash & typeof(m).target.bucketMask;
		auto n = new typeof(m).target.Node;
		@if(typeof(key).isArray)
		{
			auto[] tmp = key;
			n.key = duplicate(tmp);
		}else{
			n.key = duplicate(key);
		}
		n.hash = hash;
		n.next = m.entries[bucket];
		m.entries[bucket] = n;
		return &n.value;
	}
}
void hashmap:remove(Key key)
{
	int hash = compute_hash(key);
	int bucket = hash & bucketMask;
	Node ref curr = entries[bucket], prev = nullptr;
	while(curr)
	{
		if(curr.hash == hash && curr.key == key)
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

auto hashmap:find(Key key)
{
	int hash = compute_hash(key);
	int bucket = hash & bucketMask;
	Node ref curr = entries[bucket];
	while(curr)
	{
		if(curr.hash == hash && curr.key == key)
			return &curr.value;
		curr = curr.next;
	}
	return nullptr;
}

// provide a hash computation function for character arrays. User can define his own functions for other types 
auto hash_value(char[] arr)
{
	int hash = 5381;
	for(i in arr)
		hash = ((hash << 5) + hash) + i;
	return hash;
}
