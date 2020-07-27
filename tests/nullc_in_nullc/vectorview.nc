class VectorView<T>
{
	void VectorView()
	{
	}

	void VectorView(T[] data, int capacity)
	{
        this.data = data;
        this.capacity = capacity;
	}

	T ref push_back()
	{
		assert(count < capacity);

		return &data[count++];
	}

	void push_back(T elem)
	{
		assert(count < capacity);

		data[count++] = elem;
	}

	void push_back(T[] source, int offset, int count)
	{
		for(int i = 0; i < count; i++)
			push_back(source[offset + i]);
	}

	int count;
	int capacity;

	T[] data;
}
