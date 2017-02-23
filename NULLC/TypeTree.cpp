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
