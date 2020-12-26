// std.algorithm

void copy_backwards(generic arr, int begin, end, target)
{
	while(begin != end)
		arr[--target] = arr[--end];
}

void insertion_sort(generic arr, int begin, end, generic pred)
{
	assert(begin != end);

	for(int it = begin + 1; it != end; ++it)
	{
		typeof(arr).target val = arr[it];

		if(pred(val, arr[begin]))
		{
			// move to front
			copy_backwards(arr, begin, it, it + 1);
			arr[begin] = val;
		}else{
			int hole = it;

			// move hole backwards
			while(pred(val, arr[hole - 1]))
			{
				arr[hole] = arr[hole - 1];
				hole--;
			}

			// fill hole with element
			arr[hole] = val;
		}
	}
}
void sort_swap(generic ref a, generic ref b)
{
	typeof(a).target tmp = *a;
	*a = *b;
	*b = tmp;
}

void partition(generic arr, int begin, middle, end, generic pred, int ref out_eqbeg, out_eqend)
{
	int eqbeg = middle, eqend = middle + 1;

	// expand equal range
	while(eqbeg != begin && equal(&arr[eqbeg - 1], &arr[eqbeg])) --eqbeg;
	while(eqend != end && equal(&arr[eqend], &arr[eqbeg])) ++eqend;

	// process outer elements
	int ltend = eqbeg, gtbeg = eqend;

	for(;1;)
	{
		// find the element from the right side that belongs to the left one
		for(; gtbeg != end; ++gtbeg)
		{
			if(!pred(arr[eqbeg], arr[gtbeg]))
			{
				if(equal(&arr[gtbeg], &arr[eqbeg]))
					sort_swap(&arr[gtbeg], &arr[eqend++]);
				else
					break;
			}
		}

		// find the element from the left side that belongs to the right one
		for(; ltend != begin; --ltend)
		{
			if(!pred(arr[ltend - 1], arr[eqbeg]))
			{
				if(equal(&arr[eqbeg], &arr[ltend - 1]))
					sort_swap(&arr[ltend - 1], &arr[--eqbeg]);
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
				sort_swap(&arr[ltend], &arr[eqbeg]);
			sort_swap(&arr[eqbeg], &arr[--eqend]);
		}else if(ltend == begin){
			if(eqend != gtbeg)
				sort_swap(&arr[eqbeg], &arr[eqend]);
			++eqend;
			sort_swap(&arr[gtbeg++], &arr[eqbeg++]);
		}else
			sort_swap(&arr[gtbeg++], &arr[--ltend]);
	}
}

void median3(generic arr, int first, middle, last, generic pred)
{
	if(pred(arr[middle], arr[first]))
		sort_swap(&arr[middle], &arr[first]);
	if(pred(arr[last], arr[middle]))
		sort_swap(&arr[last], &arr[middle]);
	if(pred(arr[middle], arr[first]))
		sort_swap(&arr[middle], &arr[first]);
}

void median(generic arr, int first, middle, last, generic pred)
{
	if(last - first <= 40)
	{
		// median of three for small chunks
		median3(arr, first, middle, last, pred);
	}else{
		// median of nine
		int step = (last - first + 1) / 8;
		
		median3(arr, first, first + step, first + 2 * step, pred);
		median3(arr, middle - step, middle, middle + step, pred);
		median3(arr, last - 2 * step, last - step, last, pred);
		median3(arr, first + step, middle, last - step, pred);
	}
}

void sort(generic arr, int begin, end, generic pred)
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

void sort(generic arr, int ref(typeof(arr).target ref, typeof(arr).target ref) pred)
{
	sort(arr, 0, arr.size, pred);
}

auto map(generic arr, generic ref(typeof(arr).target ref) f)
{
	auto res = new typeof(f).return[arr.size];
	for(int i = 0; i < arr.size; i++)
		res[i] = f(arr[i]);
	return res;
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

auto foldl(generic array, typeof(array).target ref(typeof(array).target, typeof(array).target) f)
{
	auto tmp = array[0];
	for(int i = 1; i < array.size; i++)
		tmp = f(tmp, array[i]);
	return tmp;
}

auto foldr(generic array, typeof(array).target ref(typeof(array).target, typeof(array).target) f)
{
	auto tmp = array[array.size - 1];
	for(int i = array.size - 2; i >= 0; i--)
		tmp = f(tmp, array[i]);
	return tmp;
}

auto bind_first(generic f, v)
{
	@if(typeof(f).argument.size == 1)
		return auto(){ return f(v); };
	else if(typeof(f).argument.size == 2)
		return auto(typeof(f).argument[1] v1){ return f(v, v1); };
	else if(typeof(f).argument.size == 2)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2){ return f(v, v1, v2); };
	else if(typeof(f).argument.size == 3)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3){ return f(v, v1, v2, v3); };
	else if(typeof(f).argument.size == 4)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4){ return f(v, v1, v2, v3, v4); };
	else if(typeof(f).argument.size == 5)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4, typeof(f).argument[5] v5){ return f(v, v1, v2, v3, v4, v5); };
	else if(typeof(f).argument.size == 6)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4, typeof(f).argument[5] v5, typeof(f).argument[6] v6){ return f(v, v1, v2, v3, v4, v5, v6); };
	else if(typeof(f).argument.size == 7)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4, typeof(f).argument[5] v5, typeof(f).argument[6] v6, typeof(f).argument[7] v7){ return f(v, v1, v2, v3, v4, v5, v6, v7); };
	else if(typeof(f).argument.size == 8)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4, typeof(f).argument[5] v5, typeof(f).argument[6] v6, typeof(f).argument[7] v7, typeof(f).argument[8] v8){ return f(v, v1, v2, v3, v4, v5, v6, v7, v8); };
	else
		_unsupported_argument_count_;
}

auto bind_last(generic f, v)
{
	@if(typeof(f).argument.size == 1)
		return auto(){ return f(v); };
	else if(typeof(f).argument.size == 2)
		return auto(typeof(f).argument[1] v1){ return f(v1, v); };
	else if(typeof(f).argument.size == 2)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2){ return f(v1, v2, v); };
	else if(typeof(f).argument.size == 3)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3){ return f(v1, v2, v3, v); };
	else if(typeof(f).argument.size == 4)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4){ return f(v1, v2, v3, v4, v); };
	else if(typeof(f).argument.size == 5)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4, typeof(f).argument[5] v5){ return f(v1, v2, v3, v4, v5, v); };
	else if(typeof(f).argument.size == 6)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4, typeof(f).argument[5] v5, typeof(f).argument[6] v6){ return f(v1, v2, v3, v4, v5, v6, v); };
	else if(typeof(f).argument.size == 7)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4, typeof(f).argument[5] v5, typeof(f).argument[6] v6, typeof(f).argument[7] v7){ return f(v1, v2, v3, v4, v5, v6, v7, v); };
	else if(typeof(f).argument.size == 8)
		return auto(typeof(f).argument[1] v1, typeof(f).argument[2] v2, typeof(f).argument[3] v3, typeof(f).argument[4] v4, typeof(f).argument[5] v5, typeof(f).argument[6] v6, typeof(f).argument[7] v7, typeof(f).argument[8] v8){ return f(v1, v2, v3, v4, v5, v6, v7, v8, v); };
	else
		_unsupported_argument_count_;
}
