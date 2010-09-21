// std.algorithm

void copy_backwards(auto[] arr, int begin, end, target)
{
	while(begin != end)
		replace(arr[--target], arr[--end]);
}

void insertion_sort(auto[] arr, int begin, end, int ref(auto ref, auto ref) pred)
{
	assert(begin != end);

	for(int it = begin + 1; it != end; ++it)
	{
		auto ref val = duplicate(arr[it]);

		if(pred(val, arr[begin]))
		{
			// move to front
			copy_backwards(arr, begin, it, it + 1);
			replace(arr[begin], val);
		}else{
			int hole = it;

			// move hole backwards
			while(pred(val, arr[hole - 1]))
			{
				replace(arr[hole], arr[hole - 1]);
				hole--;
			}

			// fill hole with element
			replace(arr[hole], val);
		}
	}
}

void partition(auto[] arr, int begin, middle, end, int ref(auto ref, auto ref) pred, int ref out_eqbeg, out_eqend)
{
	int eqbeg = middle, eqend = middle + 1;

	// expand equal range
	while(eqbeg != begin && equal(arr[eqbeg - 1], arr[eqbeg])) --eqbeg;
	while(eqend != end && equal(arr[eqend], arr[eqbeg])) ++eqend;

	// process outer elements
	int ltend = eqbeg, gtbeg = eqend;

	for(;1;)
	{
		// find the element from the right side that belongs to the left one
		for(; gtbeg != end; ++gtbeg)
		{
			if(!pred(arr[eqbeg], arr[gtbeg]))
			{
				if(equal(arr[gtbeg], arr[eqbeg]))
					swap(arr[gtbeg], arr[eqend++]);
				else
					break;
			}
		}

		// find the element from the left side that belongs to the right one
		for(; ltend != begin; --ltend)
		{
			if(!pred(arr[ltend - 1], arr[eqbeg]))
			{
				if(equal(arr[eqbeg], arr[ltend - 1]))
					swap(arr[ltend - 1], arr[--eqbeg]);
				else
					break;
			}
		}

		// scanned all elements
		if(gtbeg == end && ltend == begin)
		{
			*out_eqbeg = eqbeg;
			*out_eqend = eqend;
			return;
		}

		// make room for elements by moving equal area
		if(gtbeg == end)
		{
			if(--ltend != --eqbeg)
				swap(arr[ltend], arr[eqbeg]);
			swap(arr[eqbeg], arr[--eqend]);
		}else if(ltend == begin){
			if(eqend != gtbeg)
				swap(arr[eqbeg], arr[eqend]);
			++eqend;
			swap(arr[gtbeg++], arr[eqbeg++]);
		}else
			swap(arr[gtbeg++], arr[--ltend]);
	}
}

void median3(auto[] arr, int first, middle, last, int ref(auto ref, auto ref) pred)
{
	if(pred(arr[middle], arr[first]))
		swap(arr[middle], arr[first]);
	if(pred(arr[last], arr[middle]))
		swap(arr[last], arr[middle]);
	if(pred(arr[middle], arr[first]))
		swap(arr[middle], arr[first]);
}

void median(auto[] arr, int first, middle, last, int ref(auto ref, auto ref) pred)
{
	if(last - first <= 40)
	{
		// median of three for small chunks
		median3(arr, first, middle, last, pred);
	}else	{
		// median of nine
		int step = (last - first + 1) / 8;
		
		median3(arr, first, first + step, first + 2 * step, pred);
		median3(arr, middle - step, middle, middle + step, pred);
		median3(arr, last - 2 * step, last - step, last, pred);
		median3(arr, first + step, middle, last - step, pred);
	}
}

void sort(auto[] arr, int begin, end, int ref(auto ref, auto ref) pred)
{
	// sort large chunks
	while(end - begin > 32)
	{
		// find median element
		int middle = begin + (end - begin) / 2;
		median(arr, begin, middle, end - 1, pred);

		// partition in three chunks (< = >)
		int eqbeg, eqend;
		partition(arr, begin, middle, end, pred, &eqbeg, &eqend);

		// loop on larger half
		if(eqbeg - begin > end - eqend)
		{
			sort(arr, eqend, end, pred);
			end = eqbeg;
		}else{
			sort(arr, begin, eqbeg, pred);
			begin = eqend;
		}
	}

	// insertion sort small chunk
	if(begin != end)
		insertion_sort(arr, begin, end, pred);
}

void sort(auto[] arr, int ref(auto ref, auto ref) pred)
{
	sort(arr, 0, arr.size, pred);
}

void map(auto[] arr, void ref(auto ref) f)
{
	for(int i = 0; i < arr.size; i++)
		f(arr[i]);
}

auto[] filter(auto[] arr, int ref(auto ref) f)
{
	auto[] n = auto_array(arr.type, arr.size);
	int el = 0;
	for(int i = 0; i < arr.size; i++)
		if(f(arr[i]))
			n.set(arr[i], el++);
	__force_size(n, el);
	return n;
}
