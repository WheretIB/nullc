// std.range

class range_iterator
{
	int pos;
	int max;
	int step;
}
auto range_iterator::start()
{
	return *this;
}
int range_iterator::next()
{
	pos += step;
	return pos - step;
}
int range_iterator::hasnext()
{
	return pos < max;
}
auto range(int min, max, step = 1)
{
	range_iterator r;
	r.pos = min;
	r.max = max + 1;
	r.step = step;
	return r;
}

