// sgl.list

class list_node<T>
{
	list_node<T> ref prev, next;
	T elem;
	list<T> ref parent;
}

class list<T>
{
	list_node<T> ref first, last;
}

auto list_node.value()
{
	return elem;
}
auto list_node.value(T val)
{
	return elem = val;
}

void list::list()
{
	first = last = nullptr;
}

void list::push_back(T elem)
{
	if(!first)
	{
		first = last = new list_node<T>;
		last.prev = last.next = nullptr;
	}else{
		last.next = new list_node<T>;
		last.next.prev = last;
		last.next.next = nullptr;
		last = last.next;
	}
	last.elem = elem;
	last.parent = this;
}
void list::push_front(T elem)
{
	if(!first)
	{
		first = last = new list_node<T>;
		first.prev = first.next = nullptr;
	}else{
		first.prev = new list_node<T>;
		first.prev.next = first;
		first.prev.prev = nullptr;
		first = first.prev;
	}
	first.elem = elem;
	first.parent = this;
}
void list::pop_back()
{
	if(!last)
		assert(0, "list::pop_back list is empty");
	last = last.prev;
}
void list::pop_front()
{
	if(!first)
		assert(0, "list::pop_back list is empty");
	first = first.next;
}
void list::insert(list_node<T> ref it, T elem)
{
	if(it.parent != this)
		assert(0, "list::insert iterator is from a different list");
	auto next = it.next;
	it.next = new list_node<T>;
	it.next.elem = elem;
	it.next.prev = it;
	it.next.next = next;
	it.next.parent = this;
	if(next)
		next.prev = it.next;
}
void list::erase(list_node<T> ref it)
{
	if(it.parent != this)
		assert(0, "list::insert iterator is from a different list");
	auto prev = it.prev, next = it.next;
	if(prev)
		prev.next = next;
	if(next)
		next.prev = prev;
	if(it == first)
		first = first.next;
	if(it == last)
		last = last.prev;
}
void list::clear()
{
	first = last = nullptr;
}
auto list::back()
{
	assert(first != nullptr, "list::back called on empty list");
	return &last.elem;
}
auto list::front()
{
	assert(first != nullptr, "list::front called on empty list");
	return &first.elem;
}
auto list::begin()
{
	return first;
}
auto list::end()
{
	return last;
}
int list::empty()
{
	return first == nullptr;
}
auto list::find(T elem)
{
	auto curr = first;

	while(curr)
	{
		if(curr.elem == elem)
			return curr;
		curr = curr.next;
	}

	return nullptr;
}

// iteration
class list_iterator<T>
{
	list_node<T> ref curr;
}
auto list_iterator::list_iterator(list_node<T> ref start)
{
	curr = start;
}
auto list_iterator::start()
{
	return *this;
}
auto list_iterator::next()
{
	auto ret = &curr.elem;
	curr = curr.next;
	return ret;
}
auto list_iterator::hasnext()
{
	return curr ? 1 : 0;
}
auto list::start()
{
	return list_iterator<T>(this.first);
}

// aggregate function and other features on sequences

auto list::sum(generic ref(T) f)
{
	typeof(f).return sum;
	for(i in list_iterator<T>(first))
		sum += f(i);
	return sum;
}

auto list::average(generic ref(T) f)
{
	typeof(f).return sum;
	int count = 0;
	for(i in list_iterator<T>(first))
	{
		sum += f(i);
		count++;
	}
	return sum / count;
}

auto list::min_element()
{
	auto min = first.elem;
	for(i in list_iterator<T>(first.next))
		min = i < min ? i : min;
	return min;
}

auto list::max_element()
{
	auto max = first.elem;
	for(i in list_iterator<T>(first.next))
		max = i > max ? i : max;
	return max;
}

auto list::min_element(generic ref(T) f)
{
	typeof(f).return min = f(first.elem), tmp;
	for(i in list_iterator<T>(first.next))
	{
		tmp = f(i);
		min = tmp < min ? tmp : min;
	}
	return min;
}

auto list::max_element(generic ref(T) f)
{
	typeof(f).return max = f(first.elem), tmp;
	for(i in list_iterator<T>(first.next))
	{
		tmp = f(i);
		max = tmp > max ? tmp : max;
	}
	return max;
}

auto list::count_if(generic ref(T) f)
{
	int c = 0;
	for(i in list_iterator<T>(first))
		if(f(i))
			c++;
	return c;
}

auto list::all(generic ref(T) f)
{
	int c = first ? 1 : 0;
	for(i in list_iterator<T>(first))
	{
		c = c && f(i);
		if(!c)
			break; // exit immediately if one of elements doesn't pass the test
	}
	return c;
}

auto list::any(generic ref(T) f)
{
	int c = 0;
	for(i in list_iterator<T>(first))
	{
		c = c || f(i);
		if(c)
			break; // exit immediately if one of elements passed the test
	}
	return c;
}

bool operator in(generic x, list<generic> ref arr)
{
	for(i in arr)
		if(x == i)
			return true;
	return false;
}

// operator = for list node element?
// list with one element?
