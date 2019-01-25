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

InplaceStr GetReferenceTypeName(ExpressionContext &ctx, TypeBase* type)
{
	unsigned nameLength = type->name.length() + strlen(" ref");
	char *name = (char*)ctx.allocator->alloc(nameLength + 1);
	sprintf(name, "%.*s ref", FMT_ISTR(type->name));

	return InplaceStr(name);
}

InplaceStr GetArrayTypeName(ExpressionContext &ctx, TypeBase* type, long long length)
{
	unsigned nameLength = type->name.length() + strlen("[]") + 21;
	char *name = (char*)ctx.allocator->alloc(nameLength + 1);
	sprintf(name, "%.*s[%lld]", FMT_ISTR(type->name), length);

	return InplaceStr(name);
}

InplaceStr GetUnsizedArrayTypeName(ExpressionContext &ctx, TypeBase* type)
{
	unsigned nameLength = type->name.length() + strlen("[]");
	char *name = (char*)ctx.allocator->alloc(nameLength + 1);

	char *pos = name;

	memcpy(pos, type->name.begin, type->name.length());
	pos += type->name.length();

	memcpy(pos, "[]", 2);
	pos += 2;

	*pos++ = 0;

	return InplaceStr(name);
}

InplaceStr GetFunctionTypeName(ExpressionContext &ctx, TypeBase* returnType, IntrusiveList<TypeHandle> arguments)
{
	unsigned nameLength = returnType->name.length() + strlen(" ref()");

	for(TypeHandle *arg = arguments.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + 1;

	char *name = (char*)ctx.allocator->alloc(nameLength + 1);

	char *pos = name;

	memcpy(pos, returnType->name.begin, returnType->name.length());
	pos += returnType->name.length();

	memcpy(pos, " ref(", 5);
	pos += 5;

	for(TypeHandle *arg = arguments.head; arg; arg = arg->next)
	{
		memcpy(pos, arg->type->name.begin, arg->type->name.length());
		pos += arg->type->name.length();

		if(arg->next)
			*pos++ = ',';
	}

	*pos++ = ')';
	*pos++ = 0;

	return InplaceStr(name);
}

InplaceStr GetGenericClassTypeName(ExpressionContext &ctx, TypeBase* proto, IntrusiveList<TypeHandle> generics)
{
	unsigned nameLength = proto->name.length() + strlen("<>");

	for(TypeHandle *arg = generics.head; arg; arg = arg->next)
	{
		if(arg->type->isGeneric)
			nameLength += strlen("generic") + 1;
		else
			nameLength += arg->type->name.length() + 1;
	}

	char *name = (char*)ctx.allocator->alloc(nameLength + 1);

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

InplaceStr GetFunctionSetTypeName(ExpressionContext &ctx, IntrusiveList<TypeHandle> types)
{
	assert(!types.empty());

	unsigned nameLength = 0;

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + strlen(" or ");

	char *name = (char*)ctx.allocator->alloc(nameLength + 1);

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

InplaceStr GetArgumentSetTypeName(ExpressionContext &ctx, IntrusiveList<TypeHandle> types)
{
	unsigned nameLength = 2;

	for(TypeHandle *arg = types.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + 1;

	char *name = (char*)ctx.allocator->alloc(nameLength + 1);

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

InplaceStr GetMemberSetTypeName(ExpressionContext &ctx, TypeBase* type)
{
	unsigned nameLength = type->name.length() + strlen(" members");
	char *name = (char*)ctx.allocator->alloc(nameLength + 1);
	sprintf(name, "%.*s members", FMT_ISTR(type->name));

	return InplaceStr(name);
}


InplaceStr GetGenericAliasTypeName(ExpressionContext &ctx, InplaceStr baseName)
{
	unsigned nameLength = baseName.length() + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength + 1);
	sprintf(name, "@%.*s", FMT_ISTR(baseName));

	return InplaceStr(name);
}

InplaceStr GetFunctionContextTypeName(ExpressionContext &ctx, InplaceStr functionName, unsigned index)
{
	InplaceStr operatorName = GetOperatorName(functionName);

	if(!operatorName.empty())
		functionName = operatorName;

	unsigned nameLength = functionName.length() + 32;
	char *name = (char*)ctx.allocator->alloc(nameLength + 1);
	sprintf(name, "__%.*s_%d_cls", FMT_ISTR(functionName), index);

	return InplaceStr(name);
}

InplaceStr GetFunctionContextVariableName(ExpressionContext &ctx, FunctionData *function)
{
	InplaceStr functionName = function->name;
	InplaceStr operatorName = GetOperatorName(functionName);

	if(!operatorName.empty())
		functionName = operatorName;

	unsigned nameLength = functionName.length() + 32;
	char *name = (char*)ctx.allocator->alloc(nameLength + 1);
	sprintf(name, "$%.*s_%u_ext", FMT_ISTR(functionName), function->type->name.hash());

	return InplaceStr(name);
}

InplaceStr GetFunctionTableName(ExpressionContext &ctx, FunctionData *function)
{
	assert(function->scope->ownerType);

	const char *pos = strstr(function->name.begin, "::");

	assert(pos);

	unsigned nameLength = function->name.length() + 32;
	char *name = (char*)ctx.allocator->alloc(nameLength + 1);
	sprintf(name, "$vtbl%010u%s", function->type->name.hash(), pos + 2);

	return InplaceStr(name);
}

InplaceStr GetFunctionContextMemberName(ExpressionContext &ctx, InplaceStr prefix, InplaceStr suffix)
{
	unsigned nameLength = prefix.length() + 1 + suffix.length() + 1;
	char *name = (char*)ctx.allocator->alloc(nameLength);
	sprintf(name, "%.*s_%.*s", FMT_ISTR(prefix), FMT_ISTR(suffix));

	return InplaceStr(name);
}

InplaceStr GetFunctionVariableUpvalueName(ExpressionContext &ctx, VariableData *variable)
{
	FunctionData *function = ctx.GetFunctionOwner(variable->scope);

	assert(function);

	unsigned nameLength = function->name.length() + variable->name.length() + 24;
	char *name = (char*)ctx.allocator->alloc(nameLength);
	sprintf(name, "$upvalue_%.*s_%.*s_%04x", FMT_ISTR(function->name), FMT_ISTR(variable->name), variable->uniqueId);

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
			nameLength += curr->ownerNamespace->name.length() + 1;

			foundNamespace = true;
		}
	}

	if(!foundNamespace)
		return str;

	char *name = (char*)ctx.allocator->alloc(nameLength + 1);

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

InplaceStr GetVariableNameInScope(ExpressionContext &ctx, ScopeData *scope, InplaceStr str)
{
	return GetTypeNameInScope(ctx, scope, str);
}

InplaceStr GetFunctionNameInScope(ExpressionContext &ctx, ScopeData *scope, TypeBase *parentType, InplaceStr str, bool isOperator, bool isAccessor)
{
	if(parentType)
	{
		char *name = (char*)ctx.allocator->alloc(parentType->name.length() + 2 + str.length() + (isAccessor ? 1 : 0) + 1);

		sprintf(name, "%.*s::%.*s%s", FMT_ISTR(parentType->name), FMT_ISTR(str), isAccessor ? "$" : "");

		return InplaceStr(name);
	}

	if(isOperator)
		return str;

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

	char *name = (char*)ctx.allocator->alloc(nameLength + 1);

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
