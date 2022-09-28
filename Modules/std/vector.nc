// sgl.vector
import std.algorithm;

class vector<T>
{
	T[]		data;
	int		count;
}

void vector::vector(int reserved)
{
	assert(reserved >= 0);
	if(reserved)
		data = new T[reserved];
	count = 0;
}
void vector::vector()
{
	data = nullptr;
	count = 0;
}

void vector::push_back(T val)
{
	if(count == data.size)
		this.grow(count);

	data[count++] = val;
}

void vector::pop_back()
{
	assert(count);
	count--;
}

auto vector::back()
{
	assert(count);
	return &data[count - 1];
}

auto vector::front()
{
	assert(count);
	return &data[0];
}

auto operator[](vector<generic> ref v, int index)
{
	assert(index < v.count);
	return &v.data[index];
}

void vector::grow(int targetSize)
{
	int nReserved = data.size + (data.size >> 1) + 1;

	if (targetSize > nReserved)
		nReserved = targetSize;

	T[] nArr = new T[nReserved];
	array_copy(nArr, data);
	data = nArr;
}

auto vector::size()
{
	return count;
}

auto vector::capacity()
{
	return data.size;
}

void vector::reserve(int size)
{
	assert(size >= 0);

	if(size <= data.size)
		return;

	grow(size);
}

void vector::resize(int size)
{
	assert(size >= 0);

	T[] nArr = new T[size];
	for(i in data, j in nArr)
		j = i;
	data = nArr;
	count = size;
}

void vector::clear()
{
	count = 0;
}

void vector::destroy()
{
	count = 0;
	data = nullptr;
}

// iterator over vector contents
class vector_iterator<T>
{
	vector<T> ref arr;
	int pos;
}
auto vector::start()
{
	vector_iterator<T> iter;
	iter.arr = this;
	iter.pos = 0;
	return iter;
}
auto vector_iterator::next()
{
	return &arr.data[pos++];
}
int vector_iterator::hasnext()
{
	return arr != nullptr && pos < arr.count;
}

// vector splices
class vector_splice<T>
{
	vector<T> ref base;
	int start, end;
}
auto operator[](vector<@T> ref a, int start, end)
{
	vector_splice<T> s;
	s.base = a;
	assert(start >= 0 && end >= 0 && start < a.size() && end >= start);
	s.start = start;
	s.end = end;
	return s;
}
auto operator[](vector_splice<@T> ref a, int index)
{
	assert(index >= 0 && index <= (a.end - a.start));
	return a.base[index + a.start];
}

// aggregate function and other features on sequences

auto vector::sum(generic ref(T) f)
{
	typeof(f).return sum;
	for(int i = 0; i < count; i++)
		sum += f(data[i]);
	return sum;
}

auto vector::average(generic ref(T) f)
{
	return sum(f) / count;
}

auto vector::min_element()
{
	int min = 0;
	for(int i = 1; i < count; i++)
		min = data[i] < data[min] ? i : min;
	return data[min];
}

auto vector::max_element()
{
	int max = 0;
	for(int i = 1; i < count; i++)
		max = data[i] > data[max] ? i : max;
	return data[max];
}

auto vector::min_element(generic ref(T) f)
{
	typeof(f).return min = f(data[0]), tmp;
	for(int i = 1; i < count; i++)
	{
		tmp = f(data[i]);
		min = tmp < min ? tmp : min;
	}
	return min;
}

auto vector::max_element(generic ref(T) f)
{
	typeof(f).return max = f(data[0]), tmp;
	for(int i = 1; i < count; i++)
	{
		tmp = f(data[i]);
		max = tmp > max ? tmp : max;
	}
	return max;
}

auto vector::count_if(generic ref(T) f)
{
	int c = 0;
	for(int i = 0; i < count; i++)
		if(f(data[i]))
			c++;
	return c;
}

auto vector::all(generic ref(T) f)
{
	int c = count ? 1 : 0;
	for(int i = 0; i < count && c; i++) // exit immediately if one of elements doesn't pass the test
		c = c && f(data[i]);
	return c;
}

auto vector::any(generic ref(T) f)
{
	int c = 0;
	for(int i = 0; i < count && !c; i++) // exit immediately if one of elements passed the test
		c = c || f(data[i]);
	return c;
}

void vector_sort_impl(generic arr, generic pred)
{
	sort(arr.data, 0, arr.count, pred);
}

auto vector::sort(generic ref(T, T) pred)
{
	vector_sort_impl(this, pred);
}

bool operator in(generic x, vector<generic> ref arr)
{
	for(i in arr)
		if(x == i)
			return true;
	return false;
}

auto vector::push_back_mult(generic ref() f)
{
	for(i in f)
		push_back(i);
}

auto vector::push_back_mult(generic[] arr)
{
	for(i in arr)
		push_back(i);
}

auto vector::fill(generic ref() f)
{
	clear();
	for(i in f)
		push_back(i);
}

auto vector::fill(generic[] arr)
{
	clear();
	for(i in arr)
		push_back(i);
}

auto vector::foldl(generic ref(T, T) f)
{
	auto tmp = data[0];
	for(int i = 1; i < count; i++)
		tmp = f(tmp, data[i]);
	return tmp;
}

auto vector::foldr(generic ref(T, T) f)
{
	auto tmp = data[count - 1];
	for(int i = count - 2; i >= 0; i--)
		tmp = f(tmp, data[i]);
	return tmp;
}

auto operator=(vector<generic> ref v, generic[] arr)
{
	v.fill(arr);
}

auto vector::map(generic ref(T) f)
{
	vector<typeof(f).return> res;
	res.reserve(count);
	for(int i = 0; i < count; i++)
		res.push_back(f(data[i]));
	return res;
}

auto vector::filter(generic ref(T) f)
{
	vector<T> res;
	res.reserve(count);
	for(int i = 0; i < count; i++)
	{
		if(f(data[i]))
			res.push_back(data[i]);
	}
	return res;
}
