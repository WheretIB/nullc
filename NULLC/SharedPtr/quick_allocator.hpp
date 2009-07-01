#pragma once

namespace detail
{
	template <size_t size> union block_t
	{
    	char bytes[size];
		block_t* next;
	};
	
	template <size_t size> struct page_t
	{
		enum { items_per_page = 32768 / size }; // 32k pages
		
		block_t<size> items[items_per_page];
		page_t* next;
	};

	template <size_t size> struct quick_allocator
	{
		typedef block_t<size> block;
		typedef page_t<size> page;

		static block* free_head;
		static page* page_head;
		static unsigned int last;
		
		struct cleanuper
		{
			~cleanuper()
			{
				page* next;
				
				for (page* p = page_head; p; p = next)
				{
					next = p->next;
					delete p;
				}
			}
		};

		static void* alloc()
		{
			if (block* b = free_head)
			{
				free_head = b->next;
            	return b->bytes;
			}
			else
			{
				if (last == page::items_per_page)
				{
					static cleanuper c;
					
					page* p = new page;
					
					p->next = page_head;
					page_head = p;
					
					last = 0;
				}

            	return page_head->items[last++].bytes;
        	}
    	}

		static void dealloc(void* p)
		{
        	if (p != 0) // 18.4.1.1/13
			{
				block* b = static_cast<block*>(p);
				
				b->next = free_head;
            	free_head = b;
        	}
    	}
	};

	template <size_t size> block_t<size>* quick_allocator<size>::free_head = 0;
	template <size_t size> page_t<size>* quick_allocator<size>::page_head = 0;
	template <size_t size> unsigned int quick_allocator<size>::last = quick_allocator<size>::page::items_per_page;
}
