import std.vector;

class ArrayView<T>
{
	void ArrayView(T[] rhs)
	{
		data = rhs;
		count = rhs.size;
	}

	void ArrayView(vector<T> rhs, int size)
	{
		assert(size <= rhs.size());

		data = rhs.data;
		count = size;
	}

	T ref back()
	{
		assert(count > 0);

		return &data[count - 1];
	}

	bool empty()
	{
		return count == 0;
	}

	int size()
	{
		return count;
	}

	T[] data;
	int count;
}

auto operator[](ArrayView<@T> arr, int index)
{
	assert(index < arr.count);

	return &arr.data[index];
}

auto operator=(ArrayView<@T> ref lhs, vector<@T> ref rhs)
{
	lhs.data = rhs.data;
	lhs.count = rhs.size();

	return lhs;
}

auto ArrayView::ArrayView(vector<@T> ref src)
{
	this.data = src.data;
	this.count = src.size();
}

auto ArrayView::ArrayView(vector<@T> ref src, int size)
{
	assert(size <= src.size());

	this.data = src.data;
	this.count = size;
}

auto ViewOf(vector<@T> ref src)
{
	return ArrayView<T>(src);
}
