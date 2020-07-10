class RefList<T>
{
	void push_back(T ref node)
	{
		assert(node != nullptr);
		assert(node.next == nullptr);
		assert(!node.listed);

		node.listed = true;

		if(!head)
		{
			head = tail = node;
		}
		else
		{
			tail.next = node;
			tail = node;
		}
	}

	bool empty()
	{
		return head == nullptr;
	}

	int size()
	{
		int count = 0;

		T ref curr = head;

		while(curr)
		{
			count++;

			curr = T ref(curr.next);
		}

		return count;
	}

	T ref head;
	T ref tail;
}

auto operator[](RefList<@T> ref lhs, int index)
{
	T ref curr = lhs.head;

	while(index && curr)
	{
		curr = curr.next;
		index--;
	}

	assert(index == 0);

	return curr;
}

auto RefList:start()
{
	return coroutine auto()
	{
		T ref curr = head;
		while(curr)
		{
			yield curr;
			curr = T ref(curr.next);
		}
		
		return nullptr;
	};
}
