#include "TypeTree.h"

#include "ExpressionTree.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

InplaceStr GetOperatorName(InplaceStr name)
{
	if(name == InplaceStr("+"))
		return InplaceStr("__operatorAdd");

	if(name == InplaceStr("-"))
		return InplaceStr("__operatorSub");

	if(name == InplaceStr("*"))
		return InplaceStr("__operatorMul");

	if(name == InplaceStr("/"))
		return InplaceStr("__operatorDiv");

	if(name == InplaceStr("%"))
		return InplaceStr("__operatorMod");

	if(name == InplaceStr("**"))
		return InplaceStr("__operatorPow");

	if(name == InplaceStr("<"))
		return InplaceStr("__operatorLess");

	if(name == InplaceStr(">"))
		return InplaceStr("__operatorGreater");

	if(name == InplaceStr("<="))
		return InplaceStr("__operatorLEqual");

	if(name == InplaceStr(">="))
		return InplaceStr("__operatorGEqual");

	if(name == InplaceStr("=="))
		return InplaceStr("__operatorEqual");

	if(name == InplaceStr("!="))
		return InplaceStr("__operatorNEqual");

	if(name == InplaceStr("<<"))
		return InplaceStr("__operatorShiftLeft");

	if(name == InplaceStr(">>"))
		return InplaceStr("__operatorShiftRight");

	if(name == InplaceStr("&"))
		return InplaceStr("__operatorBitAnd");

	if(name == InplaceStr("|"))
		return InplaceStr("__operatorBitOr");

	if(name == InplaceStr("^"))
		return InplaceStr("__operatorBitXor");

	if(name == InplaceStr("&&"))
		return InplaceStr("__operatorLogAnd");

	if(name == InplaceStr("||"))
		return InplaceStr("__operatorLogOr");

	if(name == InplaceStr("^^"))
		return InplaceStr("__operatorLogXor");

	if(name == InplaceStr("="))
		return InplaceStr("__operatorSet");

	if(name == InplaceStr("+="))
		return InplaceStr("__operatorAddSet");

	if(name == InplaceStr("-="))
		return InplaceStr("__operatorSubSet");

	if(name == InplaceStr("*="))
		return InplaceStr("__operatorMulSet");

	if(name == InplaceStr("/="))
		return InplaceStr("__operatorDivSet");

	if(name == InplaceStr("**="))
		return InplaceStr("__operatorPowSet");

	if(name == InplaceStr("%="))
		return InplaceStr("__operatorModSet");

	if(name == InplaceStr("<<="))
		return InplaceStr("__operatorShlSet");

	if(name == InplaceStr(">>="))
		return InplaceStr("__operatorShrSet");

	if(name == InplaceStr("&="))
		return InplaceStr("__operatorAndSet");

	if(name == InplaceStr("|="))
		return InplaceStr("__operatorOrSet");

	if(name == InplaceStr("^="))
		return InplaceStr("__operatorXorSet");

	if(name == InplaceStr("[]"))
		return InplaceStr("__operatorIndex");

	if(name == InplaceStr("!"))
		return InplaceStr("__operatorLogNot");

	if(name == InplaceStr("~"))
		return InplaceStr("__operatorBitNot");

	if(name == InplaceStr("()"))
		return InplaceStr("__operatorFuncCall");

	return InplaceStr();
}

char* NameWriteUnsigned(char *pos, unsigned value)
{
	char reverse[16];
	char *curr = reverse;

	*curr++ = (char)((value % 10) + '0');

	while(value /= 10)
		*curr++ = (char)((value % 10) + '0');

	do
	{
		--curr;
		*pos++ = *curr;
	}
	while(curr != reverse);

	return pos;
}

InplaceStr GetReferenceTypeName(ExpressionContext &ctx, TypeBase* type)
{
	unsigned typeNameLength = unsigned(type->name.end - type->name.begin);

	unsigned nameLength = typeNameLength + 4 + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);

	if(typeNameLength)
		memcpy(name, type->name.begin, typeNameLength);

	memcpy(name + typeNameLength, " ref", 5);

	assert(strlen(name) + 1 == nameLength);
	return InplaceStr(name);
}

InplaceStr GetArrayTypeName(ExpressionContext &ctx, TypeBase* type, long long length)
{
	unsigned nameLength = type->name.length() + unsigned(strlen("[]")) + 21 + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);
	NULLC::SafeSprintf(name, nameLength, "%.*s[%lld]", FMT_ISTR(type->name), length);

	return InplaceStr(name);
}

InplaceStr GetUnsizedArrayTypeName(ExpressionContext &ctx, TypeBase* type)
{
	unsigned nameLength = type->name.length() + unsigned(strlen("[]")) + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	memcpy(pos, type->name.begin, type->name.length());
	pos += type->name.length();

	memcpy(pos, "[]", 2);
	pos += 2;

	*pos++ = 0;

	assert(strlen(name) + 1 == nameLength);
	return InplaceStr(name);
}

InplaceStr GetFunctionTypeName(ExpressionContext &ctx, TypeBase* returnType, IntrusiveList<TypeHandle> arguments)
{
	unsigned nameLength = returnType->name.length() + unsigned(strlen(" ref()")) + 1;

	for(TypeHandle *arg = arguments.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + (arg->next ? 1 : 0);

	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	if(returnType->name.begin)
		memcpy(pos, returnType->name.begin, returnType->name.length());
	pos += returnType->name.length();

	memcpy(pos, " ref(", 5);
	pos += 5;

	for(TypeHandle *arg = arguments.head; arg; arg = arg->next)
	{
		if(arg->type->name.begin)
			memcpy(pos, arg->type->name.begin, arg->type->name.length());
		pos += arg->type->name.length();

		if(arg->next)
			*pos++ = ',';
	}

	*pos++ = ')';
	*pos++ = 0;

	assert(strlen(name) + 1 == nameLength);
	return InplaceStr(name);
}

InplaceStr GetGenericClassTypeName(ExpressionContext &ctx, TypeBase* proto, IntrusiveList<TypeHandle> generics)
{
	unsigned nameLength = proto->name.length() + unsigned(strlen("<>")) + 1;

	for(TypeHandle *arg = generics.head; arg; arg = arg->next)
	{
		if(arg->type->isGeneric)
			nameLength += unsigned(strlen("generic"));
		else
			nameLength += arg->type->name.length();

		if(arg->next)
			nameLength += 1;
	}

	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	memcpy(pos, proto->name.begin, proto->name.length());
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
			memcpy(pos, arg->type->name.begin, arg->type->name.length());
			pos += arg->type->name.length();
		}

		if(arg->next)
			*pos++ = ',';
	}

	*pos++ = '>';
	*pos++ = 0;

	assert(strlen(name) + 1 == nameLength);
	return InplaceStr(name);
}

InplaceStr GetFunctionSetTypeName(ExpressionContext &ctx, IntrusiveList<TypeHandle> types)
{
	assert(!types.empty());

	unsigned nameLength = 0;

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + (arg->next ? unsigned(strlen(" or ")) : 0);

	nameLength += 1;

	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
	{
		memcpy(pos, arg->type->name.begin, arg->type->name.length());
		pos += arg->type->name.length();

		if(arg->next)
		{
			strcpy(pos, " or ");
			pos += strlen(" or ");
		}
	}

	*pos++ = 0;

	assert(strlen(name) + 1 == nameLength);
	return InplaceStr(name);
}

InplaceStr GetArgumentSetTypeName(ExpressionContext &ctx, IntrusiveList<TypeHandle> types)
{
	unsigned nameLength = 2;

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + (arg->next ? 1 : 0);

	nameLength += 1;

	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	*pos++ = '(';

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
	{
		memcpy(pos, arg->type->name.begin, arg->type->name.length());
		pos += arg->type->name.length();

		if(arg->next)
			*pos++ = ',';
	}

	*pos++ = ')';
	*pos++ = 0;

	assert(strlen(name) + 1 == nameLength);
	return InplaceStr(name);
}

InplaceStr GetMemberSetTypeName(ExpressionContext &ctx, TypeBase* type)
{
	unsigned nameLength = type->name.length() + unsigned(strlen(" members")) + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	memcpy(pos, type->name.begin, type->name.length());
	pos += type->name.length();

	strcpy(pos, " members");
	pos += strlen(" members");

	*pos++ = 0;

	assert(strlen(name) + 1 == nameLength);
	return InplaceStr(name);
}

InplaceStr GetGenericAliasTypeName(ExpressionContext &ctx, InplaceStr baseName)
{
	unsigned nameLength = baseName.length() + 1 + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	*pos++ = '@';

	memcpy(pos, baseName.begin, baseName.length());
	pos += baseName.length();

	*pos++ = 0;

	assert(strlen(name) + 1 == nameLength);
	return InplaceStr(name);
}

InplaceStr GetFunctionContextTypeName(ExpressionContext &ctx, InplaceStr functionName, unsigned index)
{
	if((*functionName.begin <= '@' && *functionName.begin != '$') || *functionName.begin == '|' || *functionName.begin == '^' || *functionName.begin == '~')
	{
		InplaceStr operatorName = GetOperatorName(functionName);

		if(!operatorName.empty())
			functionName = operatorName;
	}

	unsigned nameLength = functionName.length() + 32 + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	*pos++ = '_';
	*pos++ = '_';

	memcpy(pos, functionName.begin, functionName.length());
	pos += functionName.length();

	*pos++ = '_';
	pos = NameWriteUnsigned(pos, index);

	strcpy(pos, "_cls");
	pos += strlen("_cls");

	*pos++ = 0;

	assert(strlen(name) < nameLength);
	return InplaceStr(name);
}

InplaceStr GetFunctionContextVariableName(ExpressionContext &ctx, FunctionData *function, unsigned index)
{
	InplaceStr functionName = function->name->name;

	if((*functionName.begin <= '@' && *functionName.begin != '$') || *functionName.begin == '|' || *functionName.begin == '^' || *functionName.begin == '~')
	{
		InplaceStr operatorName = GetOperatorName(functionName);

		if(!operatorName.empty())
			functionName = operatorName;
	}

	unsigned nameLength = functionName.length() + 32 + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	*pos++ = '$';

	memcpy(pos, functionName.begin, functionName.length());
	pos += functionName.length();

	*pos++ = '_';
	pos = NameWriteUnsigned(pos, function->type->name.hash());

	*pos++ = '_';
	pos = NameWriteUnsigned(pos, index);

	strcpy(pos, "_ext");
	pos += strlen("_ext");

	*pos++ = 0;

	assert(strlen(name) < nameLength);
	return InplaceStr(name);
}

InplaceStr GetFunctionTableName(ExpressionContext &ctx, FunctionData *function)
{
	assert(function->scope->ownerType);

	const char *pos = strstr(function->name->name.begin, "::");

	assert(pos);

	unsigned nameLength = function->name->name.length() + 32 + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);
	NULLC::SafeSprintf(name, nameLength, "$vtbl%010u%s", function->type->name.hash(), pos + 2);

	return InplaceStr(name);
}

InplaceStr GetFunctionContextMemberName(ExpressionContext &ctx, InplaceStr prefix, InplaceStr suffix, unsigned index)
{
	unsigned nameLength = prefix.length() + 1 + suffix.length() + (index != 0 ? 16 : 0) + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	memcpy(pos, prefix.begin, prefix.length());
	pos += prefix.length();

	*pos++ = '_';

	memcpy(pos, suffix.begin, suffix.length());
	pos += suffix.length();

	if(index != 0)
	{
		*pos++ = '_';
		pos = NameWriteUnsigned(pos, index);
	}

	*pos++ = 0;

	assert(strlen(name) < nameLength);
	return InplaceStr(name);
}

InplaceStr GetFunctionVariableUpvalueName(ExpressionContext &ctx, VariableData *variable)
{
	FunctionData *function = ctx.GetFunctionOwner(variable->scope);

	InplaceStr functionName = function ? function->name->name : InplaceStr("global");

	if((*functionName.begin <= '@' && *functionName.begin != '$') || *functionName.begin == '|' || *functionName.begin == '^' || *functionName.begin == '~')
	{
		InplaceStr operatorName = GetOperatorName(functionName);

		if(!operatorName.empty())
			functionName = operatorName;
	}

	unsigned nameLength = functionName.length() + variable->name->name.length() + 24 + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	strcpy(pos, "$upvalue_");
	pos += strlen("$upvalue_");

	memcpy(pos, functionName.begin, functionName.length());
	pos += functionName.length();

	*pos++ = '_';

	memcpy(pos, variable->name->name.begin, variable->name->name.length());
	pos += variable->name->name.length();

	*pos++ = '_';
	pos = NameWriteUnsigned(pos, variable->uniqueId);

	*pos++ = 0;

	assert(strlen(name) < nameLength);
	return InplaceStr(name);
}

InplaceStr GetTypeNameInScope(ExpressionContext &ctx, ScopeData *scope, InplaceStr str)
{
	bool foundNamespace = false;

	unsigned nameLength = str.length();

	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if((curr->ownerType || curr->ownerFunction) && !foundNamespace)
			break;

		if(curr->ownerNamespace)
		{
			nameLength += curr->ownerNamespace->name.name.length() + 1;

			foundNamespace = true;
		}
	}

	nameLength += 1;

	if(!foundNamespace)
		return str;

	char *name = (char*)ctx.allocator->alloc(nameLength);

	// Format a string back-to-front
	char *pos = name + nameLength;

	pos -= 1;
	*pos = 0;

	if(unsigned strLength = str.length())
	{
		pos -= strLength;
		memcpy(pos, str.begin, strLength);
	}

	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if(curr->ownerNamespace)
		{
			InplaceStr nsName = curr->ownerNamespace->name.name;

			pos -= 1;
			*pos = '.';

			pos -= nsName.length();
			memcpy(pos, nsName.begin, nsName.length());
		}
	}

	assert(pos == name);

	return InplaceStr(name);
}

InplaceStr GetVariableNameInScope(ExpressionContext &ctx, ScopeData *scope, InplaceStr str)
{
	return GetTypeNameInScope(ctx, scope, str);
}

InplaceStr GetFunctionNameInScope(ExpressionContext &ctx, ScopeData *scope, TypeBase *parentType, InplaceStr str, bool isOperator, bool isAccessor)
{
	if(parentType)
	{
		unsigned nameLength = parentType->name.length() + 2 + str.length() + (isAccessor ? 1 : 0) + 1;
		char *name = (char*)ctx.allocator->alloc(nameLength);

		NULLC::SafeSprintf(name, nameLength, "%.*s::%.*s%s", FMT_ISTR(parentType->name), FMT_ISTR(str), isAccessor ? "$" : "");

		assert(strlen(name) + 1 == nameLength);
		return InplaceStr(name);
	}

	if(isOperator)
		return str;

	assert(!isAccessor);

	bool foundNamespace = false;

	unsigned nameLength = str.length();

	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		// Temporary scope is not evaluated
		if(curr->type == SCOPE_TEMPORARY)
			return str;

		// Function scope, just use the name
		if(curr->ownerFunction && !foundNamespace)
			return str;

		if(curr->ownerType)
			assert(foundNamespace);

		if(curr->ownerNamespace)
		{
			nameLength += curr->ownerNamespace->name.name.length() + 1;

			foundNamespace = true;
		}
	}

	nameLength += 1;

	if(!foundNamespace)
		return str;

	char *name = (char*)ctx.allocator->alloc(nameLength);

	// Format a string back-to-front
	char *pos = name + nameLength;

	pos -= 1;
	*pos = 0;

	pos -= str.length();
	memcpy(pos, str.begin, str.length());

	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if(curr->ownerNamespace)
		{
			InplaceStr nsName = curr->ownerNamespace->name.name;

			pos -= 1;
			*pos = '.';

			pos -= nsName.length();
			memcpy(pos, nsName.begin, nsName.length());
		}
	}

	assert(pos == name);

	return InplaceStr(name);
}

InplaceStr GetTemporaryName(ExpressionContext &ctx, unsigned index, const char *suffix)
{
	char buf[16];

	char *curr = buf;

	*curr++ = (char)((index % 10) + '0');

	while(index /= 10)
		*curr++ = (char)((index % 10) + '0');

	unsigned suffixLength = suffix ? (unsigned)strlen(suffix) : 0;

	unsigned nameLength = 16 + suffixLength;
	char *name = (char*)ctx.allocator->alloc(nameLength);

	char *pos = name;

	memcpy(pos, "$temp", 5);
	pos += 5;

	do
	{
		--curr;
		*pos++ = *curr;
	}
	while(curr != buf);

	if(suffix)
	{
		*pos++ = '_';

		memcpy(pos, suffix, suffixLength);
		pos += suffixLength;
	}

	*pos = 0;

	assert(strlen(name) < nameLength);
	return InplaceStr(name, pos);
}

unsigned GetAlignmentOffset(long long offset, unsigned alignment)
{
	// If alignment is set and address is not aligned
	if(alignment != 0 && offset % alignment != 0)
		return alignment - (offset % alignment);

	return 0;
}
