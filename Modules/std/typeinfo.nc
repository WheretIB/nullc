// std.typeinfo

int isFunction(typeid type);
int isClass(typeid type);
int isSimple(typeid type);
int isArray(typeid type);
int isPointer(typeid type);

int isFunction(auto ref type);
int isClass(auto ref type);
int isSimple(auto ref type);
int isArray(auto ref type);
int isPointer(auto ref type);

char[] typeid.name();

// For class types
int		typeid::memberCount();
typeid	typeid::memberType(int member);
char[]	typeid::memberName(int member);

// For array and pointer types
typeid	typeid::subType();

// for array types
int		typeid::arraySize();

// for function type
typeid	typeid::returnType();
int		typeid::argumentCount();
typeid	typeid::argumentType(int argument);

// iteration over members of a type
class member_iterator
{
	typeid classID;
	int pos;
}
class member_info
{
	typeid		type;
	char[]		name;
}
auto typeid::members()
{
	member_iterator ret;
	ret.classID = *this;
	ret.pos = 0;
	return ret;
}
auto member_iterator::start()
{
	return this;
}
auto member_iterator::hasnext()
{
	return pos < classID.memberCount();
}
auto member_iterator::next()
{
	member_info ret;
	ret.type = classID.memberType(pos);
	ret.name = classID.memberName(pos);
	pos++;
	return ret;
}

// Get pointer to the class member either by index or name
auto ref	typeGetMember(auto ref obj, int member);
auto ref	typeGetMember(auto ref obj, char[] name);

// Get pointer to the target of a pointer
auto ref	pointerGetTarget(auto ref obj);

// iteration over members of an object
class member_iterator_obj
{
	typeid classID;
	auto ref	obj;
	int pos;
}
class member_info_obj
{
	typeid		type;
	char[]		name;
	auto ref	value;
}
auto typeid::members(auto ref obj)
{
	member_iterator_obj ret;
	ret.classID = *this;
	ret.obj = obj;
	ret.pos = 0;
	return ret;
}
auto member_iterator_obj::start()
{
	return this;
}
auto member_iterator_obj::hasnext()
{
	return pos < classID.memberCount();
}
auto member_iterator_obj::next()
{
	member_info_obj ret;
	ret.type = classID.memberType(pos);
	ret.name = classID.memberName(pos);
	ret.value = typeGetMember(obj, pos);
	pos++;
	return ret;
}

// iteration over arguments
class argument_iterator
{
	typeid funcID;
	int pos;
}
auto typeid::arguments()
{
	argument_iterator ret;
	ret.funcID = *this;
	ret.pos = 0;
	return ret;
}
auto argument_iterator::start()
{
	return this;
}
auto argument_iterator::hasnext()
{
	return pos < funcID.argumentCount();
}
auto argument_iterator::next()
{
	return funcID.argumentType(pos++);
}

typeid getType(char[] name);

auto ref createInstanceByName(char[] name);
auto ref createInstanceByType(typeid type);

auto[] createArrayByName(char[] name, int size);
auto[] createArrayByType(typeid type, int size);

typeid		functionGetContextType(auto ref function);
auto ref	functionGetContext(auto ref function);
void		functionSetContext(auto ref function, auto ref context);