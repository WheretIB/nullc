#pragma once

namespace detail
{
	template <size_t size> union freeblock
	{
    	char bytes[size];
		freeblock* next;
	};

	template <size_t size> struct quick_allocator
	{
		typedef freeblock<size> block;

		enum { items_per_page = 32768 / size }; // 32k pages

		static block* free;
		static block* page;
		static unsigned int last;

		static void* alloc()
		{
			if (block* x = free)
			{
				free = x->next;
            	return x;
			}
			else
			{
				if (last == items_per_page)
				{
					// "Listen to me carefully: there is no memory leak"
					// -- Scott Meyers, Eff C++ 2nd Ed Item 10
					page = static_cast<block*>(malloc(sizeof(block) * items_per_page));
					last = 0;
				}

            	return &page[last++];
        	}
    	}

		static void dealloc(void* p)
		{
        	if (p != 0) // 18.4.1.1/13
			{
				block* b = static_cast<block*>(p);
				b->next = free;
            	free = b;
        	}
    	}
	};

	template <size_t size> freeblock<size>* quick_allocator<size>::free = 0;
	template <size_t size> freeblock<size>* quick_allocator<size>::page = 0;
	template <size_t size> unsigned int quick_allocator<size>::last = quick_allocator<size>::items_per_page;
}
