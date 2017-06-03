#include "TypeTree.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

InplaceStr GetReferenceTypeName(TypeBase* type)
{
	unsigned nameLength = type->name.length() + strlen(" ref");
	char *name = new char[nameLength + 1];
	sprintf(name, "%.*s ref", FMT_ISTR(type->name));

	return InplaceStr(name);
}

InplaceStr GetArrayTypeName(TypeBase* type, long long length)
{
	unsigned nameLength = type->name.length() + strlen("[]") + 21;
	char *name = new char[nameLength + 1];
	sprintf(name, "%.*s[%lld]", FMT_ISTR(type->name), length);

	return InplaceStr(name);
}

InplaceStr GetUnsizedArrayTypeName(TypeBase* type)
{
	unsigned nameLength = type->name.length() + strlen("[]");
	char *name = new char[nameLength + 1];
	sprintf(name, "%.*s[]", FMT_ISTR(type->name));

	return InplaceStr(name);
}

InplaceStr GetFunctionTypeName(TypeBase* returnType, IntrusiveList<TypeHandle> arguments)
{
	unsigned nameLength = returnType->name.length() + strlen(" ref()");

	for(TypeHandle *arg = arguments.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + 1;

	char *name = new char[nameLength + 1];

	char *pos = name;

	sprintf(pos, "%.*s", FMT_ISTR(returnType->name));
	pos += returnType->name.length();

	strcpy(pos, " ref(");
	pos += 5;

	for(TypeHandle *arg = arguments.head; arg; arg = arg->next)
	{
		sprintf(pos, "%.*s", FMT_ISTR(arg->type->name));
		pos += arg->type->name.length();

		if(arg->next)
			*pos++ = ',';
	}

	*pos++ = ')';
	*pos++ = 0;

	return InplaceStr(name);
}

InplaceStr GetGenericClassName(TypeBase* proto, IntrusiveList<TypeHandle> generics)
{
	unsigned nameLength = proto->name.length() + strlen("<>");

	for(TypeHandle *arg = generics.head; arg; arg = arg->next)
	{
		if(arg->type->isGeneric)
			nameLength += strlen("generic") + 1;
		else
			nameLength += arg->type->name.length() + 1;
	}

	char *name = new char[nameLength + 1];

	char *pos = name;

	sprintf(pos, "%.*s", FMT_ISTR(proto->name));
	pos += proto->name.length();

	strcpy(pos, "<");
	pos += 1;

	for(TypeHandle *arg = generics.head; arg; arg = arg->next)
	{
		if(arg->type->isGeneric)
		{
			strcpy(pos, "generic");
			pos += strlen("generic");
		}
		else
		{
			sprintf(pos, "%.*s", FMT_ISTR(arg->type->name));
			pos += arg->type->name.length();
		}

		if(arg->next)
			*pos++ = ',';
	}

	*pos++ = '>';
	*pos++ = 0;

	return InplaceStr(name);
}

InplaceStr GetFunctionSetTypeName(IntrusiveList<TypeHandle> types)
{
	unsigned nameLength = 0;

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + strlen(" or ");

	char *name = new char[nameLength + 1];

	char *pos = name;

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
	{
		sprintf(pos, "%.*s", FMT_ISTR(arg->type->name));
		pos += arg->type->name.length();

		if(arg->next)
		{
			sprintf(pos, " or ");
			pos += strlen(" or ");
		}
	}

	*pos++ = 0;

	return InplaceStr(name);
}

InplaceStr GetArgumentSetTypeName(IntrusiveList<TypeHandle> types)
{
	unsigned nameLength = 2;

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + 1;

	char *name = new char[nameLength + 1];

	char *pos = name;

	*pos++ = '(';

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
	{
		sprintf(pos, "%.*s", FMT_ISTR(arg->type->name));
		pos += arg->type->name.length();

		if(arg->next)
			*pos++ = ',';
	}

	*pos++ = ')';
	*pos++ = 0;

	return InplaceStr(name);
}

InplaceStr GetTypeNameInScope(ScopeData *scope, InplaceStr str)
{
	bool foundNamespace = false;

	unsigned nameLength = str.length();

	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if((curr->ownerType || curr->ownerFunction) && !foundNamespace)
			break;

		if(curr->ownerNamespace)
		{
			nameLength += curr->ownerNamespace->name.length() + 1;

			foundNamespace = true;
		}
	}

	if(!foundNamespace)
		return str;

	char *name = new char[nameLength + 1];

	// Format a string back-to-front
	char *pos = name + nameLength + 1;

	pos -= 1;
	*pos = 0;

	pos -= str.length();
	memcpy(pos, str.begin, str.length());

	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if(curr->ownerNamespace)
		{
			InplaceStr nsName = curr->ownerNamespace->name;

			pos -= 1;
			*pos = '.';

			pos -= nsName.length();
			memcpy(pos, nsName.begin, nsName.length());
		}
	}

	assert(pos == name);

	return InplaceStr(name);
}

InplaceStr GetVariableNameInScope(ScopeData *scope, InplaceStr str)
{
	return GetTypeNameInScope(scope, str);
}

InplaceStr GetFunctionNameInScope(ScopeData *scope, TypeBase *parentType, InplaceStr str, bool isOperator, bool isAccessor)
{
	if(isOperator)
		return str;

	if(parentType)
	{
		char *name = new char[parentType->name.length() + 2 + str.length() + (isAccessor ? 1 : 0) + 1];

		sprintf(name, "%.*s::%.*s%s", FMT_ISTR(parentType->name), FMT_ISTR(str), isAccessor ? "$" : "");

		return InplaceStr(name);
	}

	assert(!isAccessor);

	bool foundNamespace = false;

	unsigned nameLength = str.length();

	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		// Function scope, just use the name
		if(curr->ownerFunction && !foundNamespace)
			return str;

		if(curr->ownerType)
			assert(foundNamespace);

		if(curr->ownerNamespace)
		{
			nameLength += curr->ownerNamespace->name.length() + 1;

			foundNamespace = true;
		}
	}

	char *name = new char[nameLength + 1];

	if(!foundNamespace)
		return str;

	// Format a string back-to-front
	char *pos = name + nameLength + 1;

	pos -= 1;
	*pos = 0;

	pos -= str.length();
	memcpy(pos, str.begin, str.length());

	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if(curr->ownerNamespace)
		{
			InplaceStr nsName = curr->ownerNamespace->name;

			pos -= 1;
			*pos = '.';

			pos -= nsName.length();
			memcpy(pos, nsName.begin, nsName.length());
		}
	}

	assert(pos == name);

	return InplaceStr(name);
}

unsigned GetAlignmentOffset(long long offset, unsigned alignment)
{
	// If alignment is set and address is not aligned
	if(alignment != 0 && offset % alignment != 0)
		return alignment - (offset % alignment);

	return 0;
}
