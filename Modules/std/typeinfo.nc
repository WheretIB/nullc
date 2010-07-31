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

int typeid.size();
char[] typeid.name();

// For class types
int		typeid:memberCount();
typeid	typeid:memberType(int member);
char[]	typeid:memberName(int member);

// For array and pointer types
typeid	typeid:subType();

// for array types
int		typeid:arraySize();

// for function type
typeid	typeid:returnType();
int		typeid:argumentCount();
typeid	typeid:argumentType(int argument);

// iteration over members
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
auto typeid:members()
{
	member_iterator ret;
	ret.classID = *this;
	ret.pos = 0;
	return ret;
}
auto member_iterator:start()
{
	return this;
}
auto member_iterator:hasnext()
{
	return pos < classID.memberCount();
}
auto member_iterator:next()
{
	member_info ret;
	ret.type = classID.memberType(pos);
	ret.name = classID.memberName(pos);
	pos++;
	return ret;
}

// iteration over arguments
class argument_iterator
{
	typeid funcID;
	int pos;
}
auto typeid:arguments()
{
	argument_iterator ret;
	ret.funcID = *this;
	ret.pos = 0;
	return ret;
}
auto argument_iterator:start()
{
	return this;
}
auto argument_iterator:hasnext()
{
	return pos < funcID.argumentCount();
}
auto argument_iterator:next()
{
	return funcID.argumentType(pos++);
}

auto ref	typeGetMember(auto ref obj, int member);
auto ref	typeGetMember(auto ref obj, char[] name);
