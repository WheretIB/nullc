#pragma once

#include <assert.h>

template<typename T>
class IntrusiveList
{
public:
	IntrusiveList(): head(0), tail(0)
	{
	}

	void push_back(T *node)
	{
		assert(node);

		if(!head)
		{
			head = tail = node;
		}
		else
		{
			tail->next = node;
			tail = node;
		}
	}

	bool empty() const
	{
		return head == 0;
	}

	unsigned size() const
	{
		unsigned count = 0;

		T *curr = head;

		while(curr)
		{
			count++;

			curr = static_cast<T*>(curr->next);
		}

		return count;
	}

	T *head;
	T *tail;
};
