// std.vector
import std.typeinfo;

class vector
{
	typeid	elemType;
	int		flags;
	int		elemSize;
	char[]	data;
	int		currSize;
}

void cConstructVector(vector ref v, typeid type, int reserved);

vector vector(typeid type, int reserved)
{
	vector ret;
	cConstructVector(ret, type, reserved);
	return ret;
}

vector vector(typeid type)
{
	vector ret;
	cConstructVector(ret, type, 0);
	return ret;
}

class vector_iterator
{
	vector ref arr;
	int pos;
}
auto vector:start()
{
	vector_iterator iter;
	iter.arr = this;
	iter.pos = 0;
	return iter;
}
auto ref vector_iterator:next();
int vector_iterator:hasnext();

void vector:push_back(auto ref val);
void vector:pop_back();
auto ref vector:front();
auto ref vector:back();
auto ref operator[](vector ref v, int index);
void vector:reserve(int size);
void vector:resize(int size);
void vector:clear();
void vector:destroy();
int vector:size();
int vector:capacity();
