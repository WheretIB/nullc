#include "ExpressionTree.h"

#include "BinaryCache.h"
#include "Bytecode.h"
#include "ExpressionEval.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

namespace
{
	jmp_buf errorHandler;

	void Stop(ExpressionContext &ctx, const char *pos, const char *msg, va_list args)
	{
		ctx.errorPos = pos;

		if(ctx.errorBuf && ctx.errorBufSize)
		{
			vsnprintf(ctx.errorBuf, ctx.errorBufSize, msg, args);
			ctx.errorBuf[ctx.errorBufSize - 1] = '\0';
		}

		longjmp(errorHandler, 1);
	}

	void Stop(ExpressionContext &ctx, const char *pos, const char *msg, ...)
	{
		va_list args;
		va_start(args, msg);

		Stop(ctx, pos, msg, args);

		va_end(args);
	}

	unsigned char ParseEscapeSequence(ExpressionContext &ctx, const char* str)
	{
		assert(str[0] == '\\');

		switch(str[1])
		{
		case 'n':
			return '\n';
		case 'r':
			return '\r';
		case 't':
			return '\t';
		case '0':
			return '\0';
		case '\'':
			return '\'';
		case '\"':
			return '\"';
		case '\\':
			return '\\';
		}

		Stop(ctx, str, "ERROR: unknown escape sequence");

		return 0;
	}

	int ParseInteger(ExpressionContext &ctx, const char* str)
	{
		(void)ctx;

		unsigned int digit;
		int a = 0;

		while((digit = *str - '0') < 10)
		{
			a = a * 10 + digit;
			str++;
		}

		return a;
	}

	long long ParseLong(ExpressionContext &ctx, const char* s, const char* e, int base)
	{
		unsigned long long res = 0;

		for(const char *p = s; p < e; p++)
		{
			int digit = ((*p >= '0' && *p <= '9') ? *p - '0' : (*p & ~0x20) - 'A' + 10);

			if(digit < 0 || digit >= base)
				Stop(ctx, p, "ERROR: digit %d is not allowed in base %d", digit, base);

			res = res * base + digit;
		}

		return res;
	}

	double ParseDouble(ExpressionContext &ctx, const char *str)
	{
		unsigned int digit;
		double integer = 0.0;

		while((digit = *str - '0') < 10)
		{
			integer = integer * 10.0 + digit;
			str++;
		}

		double fractional = 0.0;
	
		if(*str == '.')
		{
			double power = 0.1f;
			str++;

			while((digit = *str - '0') < 10)
			{
				fractional = fractional + power * digit;
				power /= 10.0;
				str++;
			}
		}

		if(*str == 'e')
		{
			str++;

			if(*str == '-')
				return (integer + fractional) * pow(10.0, (double)-ParseInteger(ctx, str + 1));
			else
				return (integer + fractional) * pow(10.0, (double)ParseInteger(ctx, str));
		}

		return integer + fractional;
	}

	bool IsBinaryOp(SynUnaryOpType type)
	{
		return type == SYN_UNARY_OP_BIT_NOT;
	}

	bool IsLogicalOp(SynUnaryOpType type)
	{
		return type == SYN_UNARY_OP_LOGICAL_NOT;
	}

	bool IsBinaryOp(SynBinaryOpType type)
	{
		switch(type)
		{
		case SYN_BINARY_OP_SHL:
		case SYN_BINARY_OP_SHR:
		case SYN_BINARY_OP_BIT_AND:
		case SYN_BINARY_OP_BIT_OR:
		case SYN_BINARY_OP_BIT_XOR:
			return true;
		}

		return false;
	}

	bool IsComparisonOp(SynBinaryOpType type)
	{
		switch(type)
		{
		case SYN_BINARY_OP_LESS:
		case SYN_BINARY_OP_LESS_EQUAL:
		case SYN_BINARY_OP_GREATER:
		case SYN_BINARY_OP_GREATER_EQUAL:
		case SYN_BINARY_OP_EQUAL:
		case SYN_BINARY_OP_NOT_EQUAL:
			return true;
		}

		return false;
	}

	bool IsLogicalOp(SynBinaryOpType type)
	{
		switch(type)
		{
		case SYN_BINARY_OP_LOGICAL_AND:
		case SYN_BINARY_OP_LOGICAL_OR:
		case SYN_BINARY_OP_LOGICAL_XOR:
			return true;
		}

		return false;
	}

	ScopeData* NamedOrGlobalScopeFrom(ScopeData *scope)
	{
		if(!scope || scope->ownerNamespace || scope->scope == NULL)
			return scope;

		return NamedOrGlobalScopeFrom(scope->scope);
	}

	TypeBase* FindNextTypeFromScope(ScopeData *scope)
	{
		if(!scope)
			return NULL;

		if(scope->ownerType)
			return scope->ownerType;

		return FindNextTypeFromScope(scope->scope);
	}

	unsigned GetAlignmentOffset(long long offset, unsigned alignment)
	{
		// If alignment is set and address is not aligned
		if(alignment != 0 && offset % alignment != 0)
			return alignment - (offset % alignment);

		return 0;
	}

	unsigned AllocateVariableInScope(ScopeData *scope, unsigned alignment, TypeBase *type)
	{
		assert((alignment & (alignment - 1)) == 0 && alignment <= 16);

		long long size = type->size;

		assert(scope);

		while(scope->scope)
		{
			if(scope->ownerFunction)
			{
				scope->ownerFunction->stackSize += GetAlignmentOffset(scope->ownerFunction->stackSize, alignment);

				unsigned result = unsigned(scope->ownerFunction->stackSize);

				scope->ownerFunction->stackSize += size;

				return result;
			}

			if(scope->ownerType)
			{
				scope->ownerType->size += GetAlignmentOffset(scope->ownerType->size, alignment);

				unsigned result = unsigned(scope->ownerType->size);

				scope->ownerType->size += size;

				return result;
			}

			scope = scope->scope;
		}

		scope->globalSize += GetAlignmentOffset(scope->globalSize, alignment);

		unsigned result = unsigned(scope->globalSize);

		scope->globalSize += size;

		return result;
	}

	VariableData* AllocateClassMember(SynBase *source, ScopeData *scope, TypeBase *type, InplaceStr name, unsigned uniqueId)
	{
		unsigned offset = AllocateVariableInScope(scope, type->alignment, type);

		assert(!type->isGeneric);

		return new VariableData(source, scope, type->alignment, type, name, offset, uniqueId);
	}

	void FinalizeAlignment(TypeClass *type)
	{
		unsigned maximumAlignment = 0;

		// Additional padding may apply to preserve the alignment of members
		for(VariableHandle *curr = type->members.head; curr; curr = curr->next)
			maximumAlignment = maximumAlignment > curr->variable->alignment ? maximumAlignment : curr->variable->alignment;

		// If explicit alignment is not specified, then class must be aligned to the maximum alignment of the members
		if(type->alignment == 0)
			type->alignment = maximumAlignment;

		// In NULLC, all classes have sizes multiple of 4, so add additional padding if necessary
		maximumAlignment = type->alignment < 4 ? 4 : type->alignment;

		if(type->size % maximumAlignment != 0)
		{
			type->padding = maximumAlignment - (type->size % maximumAlignment);
			type->size += maximumAlignment - (type->size % maximumAlignment);
		}
	}

	void ImplementPrototype(ExpressionContext &ctx, FunctionData *function)
	{
		if(function->isPrototype)
			return;

		FastVector<FunctionData*> &functions = ctx.scopes.back()->functions;

		for(unsigned i = 0; i < functions.size(); i++)
		{
			FunctionData *curr = functions[i];

			// Skip current function
			if(curr == function)
				continue;

			// TODO: generic function list

			if(curr->isPrototype && curr->type == function->type && curr->name == function->name)
			{
				curr->implementation = function;

				ctx.HideFunction(curr);
				break;
			}
		}
	}

	FunctionData* CheckUniqueness(ExpressionContext &ctx, FunctionData* function)
	{
		HashMap<FunctionData*>::Node *curr = ctx.functionMap.first(function->nameHash);

		while(curr)
		{
			// Skip current function
			if(curr->value == function)
			{
				curr = ctx.functionMap.next(curr);
				continue;
			}

			// TODO: generic function list

			if(curr->value->type == function->type)
				return curr->value;

			curr = ctx.functionMap.next(curr);
		}

		return NULL;
	}

	
}

ExpressionContext::ExpressionContext()
{
	errorPos = NULL;
	errorBuf = NULL;
	errorBufSize = 0;

	typeVoid = NULL;

	typeBool = NULL;

	typeChar = NULL;
	typeShort = NULL;
	typeInt = NULL;
	typeLong = NULL;

	typeFloat = NULL;
	typeDouble = NULL;

	typeTypeID = NULL;
	typeFunctionID = NULL;

	typeAuto = NULL;
	typeAutoRef = NULL;
	typeAutoArray = NULL;

	typeMap.init();
	functionMap.init();
	variableMap.init();

	globalScope = NULL;

	genericTypeMap.init();

	uniqueNamespaceId = 0;
	uniqueVariableId = 0;
	uniqueFunctionId = 0;
	uniqueAliasId = 0;

	unnamedFuncCount = 0;
	unnamedVariableCount = 0;
}

void ExpressionContext::Stop(const char *pos, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	::Stop(*this, pos, msg, args);

	va_end(args);
}

void ExpressionContext::PushScope()
{
	unsigned depth = scopes.size();
	ScopeData *scope = scopes.empty() ? NULL : scopes.back();

	scopes.push_back(new ScopeData(depth, scope));
}

void ExpressionContext::PushScope(NamespaceData *nameSpace)
{
	unsigned depth = scopes.size();
	ScopeData *scope = scopes.empty() ? NULL : scopes.back();

	scopes.push_back(new ScopeData(depth, scope, nameSpace));
}

void ExpressionContext::PushScope(FunctionData *function)
{
	unsigned depth = scopes.size();
	ScopeData *scope = scopes.empty() ? NULL : scopes.back();

	scopes.push_back(new ScopeData(depth, scope, function));
}

void ExpressionContext::PushScope(TypeBase *type)
{
	unsigned depth = scopes.size();
	ScopeData *scope = scopes.empty() ? NULL : scopes.back();

	scopes.push_back(new ScopeData(depth, scope, type));
}

void ExpressionContext::PopScope()
{
	// Remove scope members from lookup maps
	ScopeData *scope = scopes.back();

	// When namespace scope ends, all the contents remain accessible
	if(scope->ownerNamespace)
	{
		scopes.pop_back();
		return;
	}

	for(int i = int(scope->variables.size()) - 1; i >= 0; i--)
	{
		VariableData *variable = scope->variables[i];

		variableMap.remove(variable->nameHash, variable);
	}

	for(int i = int(scope->functions.size()) - 1; i >= 0; i--)
	{
		FunctionData *function = scope->functions[i];

		// Keep class functions visible
		if(function->scope->ownerType)
			continue;

		if(scope->scope && function->isPrototype && !function->implementation)
			Stop(function->source->pos, "ERROR: local function '%.*s' went out of scope unimplemented", FMT_ISTR(function->name));

		functionMap.remove(function->nameHash, function);
	}

	for(int i = int(scope->types.size()) - 1; i >= 0; i--)
	{
		TypeBase *type = scope->types[i];

		typeMap.remove(type->nameHash, type);
	}

	for(int i = int(scope->aliases.size()) - 1; i >= 0; i--)
	{
		AliasData *alias = scope->aliases[i];

		typeMap.remove(alias->nameHash, alias->type);
	}

	scopes.pop_back();
}

NamespaceData* ExpressionContext::GetCurrentNamespace()
{
	// Simply walk up the scopes and find the current one
	for(int i = int(scopes.size()) - 1; i >= 0; i--)
	{
		ScopeData *scope = scopes[i];

		if(NamespaceData *ns = scope->ownerNamespace)
			return ns;
	}

	return NULL;
}

FunctionData* ExpressionContext::GetCurrentFunction()
{
	// Walk up, but if we reach a type owner, stop - we're not in a context of a function
	for(int i = int(scopes.size()) - 1; i >= 0; i--)
	{
		ScopeData *scope = scopes[i];

		if(scope->ownerType)
			return NULL;

		if(FunctionData *function = scope->ownerFunction)
			return function;
	}

	return NULL;
}

TypeBase* ExpressionContext::GetCurrentType()
{
	// Walk up, but if we reach a function, return it's owner
	for(int i = int(scopes.size()) - 1; i >= 0; i--)
	{
		ScopeData *scope = scopes[i];

		if(scope->ownerFunction)
			return scope->ownerFunction->scope->ownerType;

		if(TypeBase *type = scope->ownerType)
			return type;
	}

	return NULL;
}

unsigned ExpressionContext::GetGenericClassInstantiationDepth()
{
	unsigned depth = 0;

	for(unsigned i = 0; i < scopes.size(); i++)
	{
		ScopeData *scope = scopes[i];

		if(TypeClass *type = getType<TypeClass>(scope->ownerType))
		{
			if(!type->genericTypes.empty())
				depth++;
		}
	}

	return depth;
}

void ExpressionContext::AddType(TypeBase *type)
{
	scopes.back()->types.push_back(type);

	if(!isType<TypeGenericClassProto>(type))
		assert(!type->isGeneric);

	types.push_back(type);
	typeMap.insert(type->nameHash, type);
}

void ExpressionContext::AddFunction(FunctionData *function)
{
	scopes.back()->functions.push_back(function);

	functions.push_back(function);
	functionMap.insert(function->nameHash, function);
}

void ExpressionContext::AddVariable(VariableData *variable)
{
	scopes.back()->variables.push_back(variable);

	variables.push_back(variable);
	variableMap.insert(variable->nameHash, variable);
}

void ExpressionContext::AddAlias(AliasData *alias)
{
	scopes.back()->aliases.push_back(alias);

	typeMap.insert(alias->nameHash, alias->type);
}

void ExpressionContext::HideFunction(FunctionData *function)
{
	functionMap.remove(function->nameHash, function);

	FastVector<FunctionData*> &functions = function->scope->functions;

	for(unsigned i = 0; i < functions.size(); i++)
	{
		if(functions[i] == function)
		{
			functions[i] = functions.back();
			functions.pop_back();
		}
	}
}

bool ExpressionContext::IsIntegerType(TypeBase* type)
{
	if(type == typeBool)
		return true;

	if(type == typeChar)
		return true;

	if(type == typeShort)
		return true;

	if(type == typeInt)
		return true;

	if(type == typeLong)
		return true;

	return false;
}

bool ExpressionContext::IsFloatingPointType(TypeBase* type)
{
	if(type == typeFloat)
		return true;

	if(type == typeDouble)
		return true;

	return false;
}

bool ExpressionContext::IsNumericType(TypeBase* type)
{
	return IsIntegerType(type) || IsFloatingPointType(type);
}

TypeBase* ExpressionContext::GetBinaryOpResultType(TypeBase* a, TypeBase* b)
{
	if(a == typeDouble || b == typeDouble)
		return typeDouble;

	if(a == typeFloat || b == typeFloat)
		return typeFloat;

	if(a == typeLong || b == typeLong)
		return typeLong;

	if(a == typeInt || b == typeInt)
		return typeInt;

	if(a == typeShort || b == typeShort)
		return typeShort;

	if(a == typeChar || b == typeChar)
		return typeChar;

	if(a == typeBool || b == typeBool)
		return typeBool;

	return NULL;
}

TypeRef* ExpressionContext::GetReferenceType(TypeBase* type)
{
	if(type->refType)
		return type->refType;

	// Create new type
	TypeRef* result = new TypeRef(GetReferenceTypeName(type), type);

	if(!type->isGeneric)
	{
		// Save it for future use
		type->refType = result;

		types.push_back(result);
	}

	return result;
}

TypeArray* ExpressionContext::GetArrayType(TypeBase* type, long long length)
{
	for(unsigned i = 0; i < type->arrayTypes.size(); i++)
	{
		if(type->arrayTypes[i]->length == length)
			return type->arrayTypes[i];
	}

	// Create new type
	TypeArray* result = new TypeArray(GetArrayTypeName(type, length), type, length);

	result->alignment = type->alignment;

	unsigned maximumAlignment = result->alignment < 4 ? 4 : result->alignment;

	if(result->size % maximumAlignment != 0)
	{
		result->padding = maximumAlignment - (result->size % maximumAlignment);
		result->size += result->padding;
	}

	if(!type->isGeneric)
	{
		// Save it for future use
		type->arrayTypes.push_back(result);

		types.push_back(result);
	}

	return result;
}

TypeUnsizedArray* ExpressionContext::GetUnsizedArrayType(TypeBase* type)
{
	if(type->unsizedArrayType)
		return type->unsizedArrayType;

	// Create new type
	TypeUnsizedArray* result = new TypeUnsizedArray(GetUnsizedArrayTypeName(type), type);

	result->members.push_back(new VariableHandle(new VariableData(NULL, scopes.back(), 4, typeInt, InplaceStr("size"), NULLC_PTR_SIZE, uniqueVariableId++)));

	result->size = NULLC_PTR_SIZE + 4;

	if(!type->isGeneric)
	{
		// Save it for future use
		type->unsizedArrayType = result;

		types.push_back(result);
	}

	return result;
}

TypeFunction* ExpressionContext::GetFunctionType(TypeBase* returnType, IntrusiveList<TypeHandle> arguments)
{
	for(unsigned i = 0; i < types.size(); i++)
	{
		if(TypeFunction *type = getType<TypeFunction>(types[i]))
		{
			if(type->returnType != returnType)
				continue;

			TypeHandle *leftArg = type->arguments.head;
			TypeHandle *rightArg = arguments.head;

			while(leftArg && rightArg && leftArg->type == rightArg->type)
			{
				leftArg = leftArg->next;
				rightArg = rightArg->next;
			}

			if(leftArg != rightArg)
				continue;

			return type;
		}
	}

	// Create new type
	TypeFunction* result = new TypeFunction(GetFunctionTypeName(returnType, arguments), returnType, arguments);

	if(!result->isGeneric)
	{
		types.push_back(result);
	}

	return result;
}

ExprBase* CreateCast(ExpressionContext &ctx, const char *pos, ExprBase *value, TypeBase *type)
{
	if(value->type == type)
		return value;

	if(ctx.IsNumericType(value->type) && ctx.IsNumericType(type))
		return new ExprTypeCast(value->source, type, value, EXPR_CAST_NUMERICAL);

	if(type == ctx.typeBool)
	{
		if(isType<TypeRef>(value->type))
			return new ExprTypeCast(value->source, type, value, EXPR_CAST_PTR_TO_BOOL);

		if(isType<TypeUnsizedArray>(value->type))
			return new ExprTypeCast(value->source, type, value, EXPR_CAST_UNSIZED_TO_BOOL);

		if(isType<TypeFunction>(value->type))
			return new ExprTypeCast(value->source, type, value, EXPR_CAST_FUNCTION_TO_BOOL);
	}

	if(TypeUnsizedArray *target = getType<TypeUnsizedArray>(type))
	{
		// type[N] to type[] conversion
		if(TypeArray *source = getType<TypeArray>(value->type))
		{
			if(target->subType == source->subType)
			{
				if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
				{
					ExprBase *address = new ExprGetAddress(value->source, ctx.GetReferenceType(value->type), node->variable);

					return new ExprTypeCast(value->source, type, address, EXPR_CAST_ARRAY_PTR_TO_UNSIZED);
				}
				else if(ExprDereference *node = getType<ExprDereference>(value))
				{
					return new ExprTypeCast(value->source, type, node->value, EXPR_CAST_ARRAY_PTR_TO_UNSIZED);
				}

				return new ExprTypeCast(value->source, type, value, EXPR_CAST_ARRAY_TO_UNSIZED);
			}
		}
	}

	if(TypeRef *target = getType<TypeRef>(type))
	{
		if(TypeRef *source = getType<TypeRef>(value->type))
		{
			// type[N] ref to type[] ref conversion
			if(isType<TypeUnsizedArray>(target->subType) && isType<TypeArray>(source->subType))
			{
				TypeUnsizedArray *targetSub = getType<TypeUnsizedArray>(target->subType);
				TypeArray *sourceSub = getType<TypeArray>(source->subType);

				if(targetSub->subType == sourceSub->subType)
				{
					return new ExprTypeCast(value->source, type, value, EXPR_CAST_ARRAY_PTR_TO_UNSIZED_PTR);
				}
			}
		}
	}

	Stop(ctx, pos, "ERROR: can't convert '%.*s' to '%.*s'", FMT_ISTR(value->type->name), FMT_ISTR(type->name));

	return NULL;
}

ExprBase* CreateConditionCast(ExpressionContext &ctx, ExprBase *value)
{
	if(!ctx.IsNumericType(value->type) && !isType<TypeRef>(value->type))
	{
		// TODO: function overload

		if(isType<TypeFunction>(value->type) || isType<TypeUnsizedArray>(value->type) || value->type == ctx.typeAutoRef)
		{
			ExprBase *nullPtr = new ExprNullptrLiteral(value->source, ctx.GetReferenceType(ctx.typeVoid));

			return new ExprBinaryOp(value->source, ctx.typeBool, SYN_BINARY_OP_NOT_EQUAL, value, nullPtr);
		}
		else
		{
			Stop(ctx, value->source->pos, "ERROR: condition type cannot be '%.*s' and function for conversion to bool is undefined", FMT_ISTR(value->type->name));
		}
	}

	return value;
}

ExprAssignment* CreateAssignment(ExpressionContext &ctx, const char *pos, ExprBase *lhs, ExprBase *rhs)
{
	ExprBase* wrapped = lhs;

	if(ExprVariableAccess *node = getType<ExprVariableAccess>(lhs))
	{
		wrapped = new ExprGetAddress(lhs->source, ctx.GetReferenceType(lhs->type), node->variable);
	}
	else if(ExprDereference *node = getType<ExprDereference>(lhs))
	{
		wrapped = node->value;
	}

	if(!isType<TypeRef>(wrapped->type))
		Stop(ctx, pos, "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(lhs->type->name));

	if(rhs->type == ctx.typeVoid)
		Stop(ctx, pos, "ERROR: cannot convert from void to %.*s", FMT_ISTR(rhs->type->name));

	if(lhs->type == ctx.typeVoid)
		Stop(ctx, pos, "ERROR: cannot convert from %.*s to void", FMT_ISTR(lhs->type->name));

	if(lhs->type != rhs->type)
		rhs = CreateCast(ctx, pos, rhs, lhs->type);

	return new ExprAssignment(lhs->source, lhs->type, wrapped, rhs);
}

ExprBase* AnalyzeNumber(ExpressionContext &ctx, SynNumber *syntax);
ExprBase* AnalyzeExpression(ExpressionContext &ctx, SynBase *syntax);
ExprBase* AnalyzeStatement(ExpressionContext &ctx, SynBase *syntax);
ExprBlock* AnalyzeBlock(ExpressionContext &ctx, SynBlock *syntax, bool createScope);
ExprBase* AnalyzeClassDefinition(ExpressionContext &ctx, SynClassDefinition *syntax, TypeGenericClassProto *proto, IntrusiveList<TypeHandle> generics);
void AnalyzeClassElements(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassElements *syntax);

// Apply in reverse order
TypeBase* ApplyArraySizesToType(ExpressionContext &ctx, TypeBase *type, SynBase *sizes)
{
	SynBase *size = sizes;

	if(isType<SynNothing>(size))
		size = NULL;

	if(sizes->next)
		type = ApplyArraySizesToType(ctx, type, sizes->next);

	if(isType<TypeAuto>(type))
	{
		if(size)
			Stop(ctx, size->pos, "ERROR: cannot specify array size for auto");

		return ctx.typeAutoArray;
	}

	if(!size)
		return ctx.GetUnsizedArrayType(type);

	ExprBase *sizeValue = AnalyzeExpression(ctx, size);

	if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(Evaluate(ctx, CreateCast(ctx, size->pos, sizeValue, ctx.typeLong))))
		return ctx.GetArrayType(type, number->value);

	Stop(ctx, size->pos, "ERROR: can't get array size");

	return NULL;
}

TypeBase* AnalyzeType(ExpressionContext &ctx, SynBase *syntax, bool onlyType = true)
{
	if(SynTypeAuto *node = getType<SynTypeAuto>(syntax))
	{
		return ctx.typeAuto;
	}

	if(SynTypeGeneric *node = getType<SynTypeGeneric>(syntax))
	{
		return new TypeGeneric(InplaceStr("generic"));
	}

	if(SynTypeAlias *node = getType<SynTypeAlias>(syntax))
	{
		TypeGeneric *type = new TypeGeneric(node->name);

		return type;
	}

	if(SynTypeReference *node = getType<SynTypeReference>(syntax))
	{
		TypeBase *type = AnalyzeType(ctx, node->type);

		if(isType<TypeAuto>(type))
			return ctx.typeAutoRef;

		return ctx.GetReferenceType(type);
	}

	if(SynTypeArray *node = getType<SynTypeArray>(syntax))
	{
		TypeBase *type = AnalyzeType(ctx, node->type, onlyType);

		if(!onlyType && !type)
			return NULL;

		return ApplyArraySizesToType(ctx, type, node->sizes.head);
	}

	if(SynArrayIndex *node = getType<SynArrayIndex>(syntax))
	{
		TypeBase *type = AnalyzeType(ctx, node->value, onlyType);

		if(!onlyType && !type)
			return NULL;

		if(isType<TypeAuto>(type))
		{
			if(!node->arguments.empty())
				Stop(ctx, syntax->pos, "ERROR: cannot specify array size for auto");

			return ctx.typeAutoArray;
		}

		if(node->arguments.empty())
			return ctx.GetUnsizedArrayType(type);

		if(node->arguments.size() > 1)
			Stop(ctx, syntax->pos, "ERROR: ',' is not expected in array type size");

		ExprBase *size = AnalyzeExpression(ctx, node->arguments.head);
		
		if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(Evaluate(ctx, CreateCast(ctx, node->pos, size, ctx.typeLong))))
			return ctx.GetArrayType(type, number->value);

		Stop(ctx, syntax->pos, "ERROR: can't get array size");
	}

	if(SynTypeFunction *node = getType<SynTypeFunction>(syntax))
	{
		TypeBase *returnType = AnalyzeType(ctx, node->returnType);

		IntrusiveList<TypeHandle> arguments;

		for(SynBase *el = node->arguments.head; el; el = el->next)
		{
			TypeBase *argType = AnalyzeType(ctx, el);

			if(argType == ctx.typeAuto)
				Stop(ctx, syntax->pos, "ERROR: function parameter cannot be an auto type");

			arguments.push_back(new TypeHandle(argType));
		}

		return ctx.GetFunctionType(returnType, arguments);
	}

	if(SynTypeof *node = getType<SynTypeof>(syntax))
	{
		ExprBase *value = AnalyzeExpression(ctx, node->value);

		if(value->type == ctx.typeAuto)
			Stop(ctx, syntax->pos, "ERROR: cannot take typeid from auto type");

		return value->type;
	}

	if(SynTypeSimple *node = getType<SynTypeSimple>(syntax))
	{
		TypeBase **type = NULL;

		for(ScopeData *nsScope = NamedOrGlobalScopeFrom(ctx.scopes.back()); nsScope; nsScope = NamedOrGlobalScopeFrom(nsScope->scope))
		{
			unsigned hash = nsScope->ownerNamespace ? StringHashContinue(nsScope->ownerNamespace->fullNameHash, ".") : GetStringHash("");

			for(SynIdentifier *part = node->path.head; part; part = getType<SynIdentifier>(part->next))
			{
				hash = StringHashContinue(hash, part->name.begin, part->name.end);
				hash = StringHashContinue(hash, ".");
			}

			hash = StringHashContinue(hash, node->name.begin, node->name.end);

			type = ctx.typeMap.find(hash);

			if(type)
				return *type;
		}

		// Might be a variable
		if(!onlyType)
			return NULL;

		Stop(ctx, syntax->pos, "ERROR: unknown type");
	}

	if(SynMemberAccess *node = getType<SynMemberAccess>(syntax))
	{
		TypeBase *value = AnalyzeType(ctx, node->value, onlyType);

		if(!onlyType && !value)
			return NULL;

		if(node->member == InplaceStr("argument"))
		{
			if(TypeFunction *type = getType<TypeFunction>(value))
			{
				// first/last/[n]/size
				Stop(ctx, syntax->pos, "ERROR: not implemented");
			}

			Stop(ctx, syntax->pos, "ERROR: 'argument' can only be applied to a function type, but we have '%s'", value->name);
		}
		else if(node->member == InplaceStr("return"))
		{
			if(TypeFunction *type = getType<TypeFunction>(value))
				return type->returnType;

			Stop(ctx, syntax->pos, "ERROR: 'return' can only be applied to a function type, but we have '%s'", value->name);
		}
		else if(node->member == InplaceStr("target"))
		{
			if(TypeRef *type = getType<TypeRef>(value))
				return type->subType;
			else if(TypeArray *type = getType<TypeArray>(value))
				return type->subType;
			else if(TypeUnsizedArray *type = getType<TypeUnsizedArray>(value))
				return type->subType;

			Stop(ctx, syntax->pos, "ERROR: 'target' can only be applied to a pointer or array type, but we have '%s'", value->name);
		}

		if(!onlyType)
			return NULL;

		// isReference/isArray/isFunction/arraySize/hasMember(x)/class member/class typedef

		Stop(ctx, syntax->pos, "ERROR: unknown type");

		return NULL;
	}

	if(SynTypeGenericInstance *node = getType<SynTypeGenericInstance>(syntax))
	{
		TypeBase *baseType = AnalyzeType(ctx, node->baseType);

		// TODO: overloads with a different number of generic arguments

		if(TypeGenericClassProto *proto = getType<TypeGenericClassProto>(baseType))
		{
			IntrusiveList<SynIdentifier> aliases = proto->definition->aliases;

			if(node->types.size() < aliases.size())
				Stop(ctx, syntax->pos, "ERROR: there where only '%d' argument(s) to a generic type that expects '%d'", node->types.size(), aliases.size());

			if(node->types.size() > aliases.size())
				Stop(ctx, syntax->pos, "ERROR: type has only '%d' generic argument(s) while '%d' specified", aliases.size(), node->types.size());

			bool isGeneric = false;
			IntrusiveList<TypeHandle> types;

			for(SynBase *el = node->types.head; el; el = el->next)
			{
				TypeBase *type = AnalyzeType(ctx, el);

				isGeneric |= type->isGeneric;

				types.push_back(new TypeHandle(type));
			}

			// TODO: apply current namespace
			InplaceStr className = GetGenericClassName(proto, types);

			if(isGeneric)
				return new TypeGenericClass(className, proto, types);
			
			// Check if type already exists
			if(TypeClass **prev = ctx.genericTypeMap.find(className.hash()))
				return *prev;

			ExprBase *result = AnalyzeClassDefinition(ctx, proto->definition, proto, types);

			if(ExprClassDefinition *definition = getType<ExprClassDefinition>(result))
			{
				proto->instances.push_back(result);

				return definition->classType;
			}

			Stop(ctx, syntax->pos, "ERROR: type '%s' couldn't be instantiated", baseType->name);
		}

		Stop(ctx, syntax->pos, "ERROR: type '%s' can't have generic arguments", baseType->name);
	}

	if(!onlyType)
		return NULL;

	Stop(ctx, syntax->pos, "ERROR: unknown type");

	return NULL;
}

unsigned AnalyzeAlignment(ExpressionContext &ctx, SynAlign *syntax)
{
	// noalign
	if(!syntax->value)
		return 1;

	ExprBase *align = AnalyzeNumber(ctx, syntax->value);

	if(ExprIntegerLiteral *alignValue = getType<ExprIntegerLiteral>(Evaluate(ctx, CreateCast(ctx, syntax->pos, align, ctx.typeLong))))
	{
		if(alignValue->value > 16)
			Stop(ctx, syntax->pos, "ERROR: alignment must be less than 16 bytes");

		if(alignValue->value & (alignValue->value - 1))
			Stop(ctx, syntax->pos, "ERROR: alignment must be power of two");

		return unsigned(alignValue->value);
	}

	Stop(ctx, syntax->pos, "ERROR: alignment must be a constant expression");

	return NULL;
}

ExprBase* AnalyzeNumber(ExpressionContext &ctx, SynNumber *syntax)
{
	InplaceStr &value = syntax->value;

	// Hexadecimal
	if(value.length() > 1 && value.begin[1] == 'x')
	{
		if(value.length() == 2)
			Stop(ctx, value.begin + 2, "ERROR: '0x' must be followed by number");

		// Skip 0x
		unsigned pos = 2;

		// Skip leading zeros
		while(value.begin[pos] == '0')
			pos++;

		if(int(value.length() - pos) > 16)
			Stop(ctx, value.begin, "ERROR: overflow in hexadecimal constant");

		long long num = ParseLong(ctx, value.begin + pos, value.end, 16);

		// If number overflows integer number, create long number
		if(int(num) == num)
			return new ExprIntegerLiteral(syntax, ctx.typeInt, num);

		return new ExprIntegerLiteral(syntax, ctx.typeLong, num);
	}

	bool isFP = false;

	for(unsigned int i = 0; i < value.length(); i++)
	{
		if(value.begin[i] == '.' || value.begin[i] == 'e')
			isFP = true;
	}

	if(!isFP)
	{
		if(syntax->suffix == InplaceStr("b"))
		{
			unsigned pos = 0;

			// Skip leading zeros
			while(value.begin[pos] == '0')
				pos++;

			if(int(value.length() - pos) > 64)
				Stop(ctx, value.begin, "ERROR: overflow in binary constant");

			long long num = ParseLong(ctx, value.begin + pos, value.end, 2);

			// If number overflows integer number, create long number
			if(int(num) == num)
				return new ExprIntegerLiteral(syntax, ctx.typeInt, num);

			return new ExprIntegerLiteral(syntax, ctx.typeLong, num);
		}
		else if(syntax->suffix == InplaceStr("l"))
		{
			long long num = ParseLong(ctx, value.begin, value.end, 10);

			return new ExprIntegerLiteral(syntax, ctx.typeLong, num);
		}
		else if(!syntax->suffix.empty())
		{
			Stop(ctx, syntax->suffix.begin, "ERROR: unknown number suffix '%.*s'", syntax->suffix.length(), syntax->suffix.begin);
		}

		if(value.length() > 1 && value.begin[0] == '0' && isDigit(value.begin[1]))
		{
			unsigned pos = 0;

			// Skip leading zeros
			while(value.begin[pos] == '0')
				pos++;

			if(int(value.length() - pos) > 22 || (int(value.length() - pos) > 21 && value.begin[pos] != '1'))
				Stop(ctx, value.begin, "ERROR: overflow in octal constant");

			long long num = ParseLong(ctx, value.begin, value.end, 8);

			// If number overflows integer number, create long number
			if(int(num) == num)
				return new ExprIntegerLiteral(syntax, ctx.typeInt, num);

			return new ExprIntegerLiteral(syntax, ctx.typeLong, num);
		}

		long long num = ParseLong(ctx, value.begin, value.end, 10);

		if(int(num) == num)
			return new ExprIntegerLiteral(syntax, ctx.typeInt, num);

		Stop(ctx, value.begin, "ERROR: overflow in decimal constant");
	}

	if(syntax->suffix == InplaceStr("f"))
	{
		double num = ParseDouble(ctx, value.begin);

		return new ExprRationalLiteral(syntax, ctx.typeFloat, float(num));
	}
	else if(!syntax->suffix.empty())
	{
		Stop(ctx, syntax->suffix.begin, "ERROR: unknown number suffix '%.*s'", syntax->suffix.length(), syntax->suffix.begin);
	}

	double num = ParseDouble(ctx, value.begin);

	return new ExprRationalLiteral(syntax, ctx.typeDouble, num);
}

ExprArray* AnalyzeArray(ExpressionContext &ctx, SynArray *syntax)
{
	assert(syntax->values.head);

	IntrusiveList<ExprBase> values;

	TypeBase *subType = NULL;

	for(SynBase *el = syntax->values.head; el; el = el->next)
	{
		ExprBase *value = AnalyzeExpression(ctx, el);

		if(subType == NULL)
			subType = value->type;
		else if(subType != value->type)
			Stop(ctx, el->pos, "ERROR: array element type mismatch");

		values.push_back(value);
	}

	return new ExprArray(syntax, ctx.GetArrayType(subType, values.size()), values);
}

ExprBase* AnalyzeVariableAccess(ExpressionContext &ctx, SynBase *syntax, IntrusiveList<SynIdentifier> path, InplaceStr name)
{
	VariableData **variable = NULL;

	for(ScopeData *nsScope = NamedOrGlobalScopeFrom(ctx.scopes.back()); nsScope; nsScope = NamedOrGlobalScopeFrom(nsScope->scope))
	{
		unsigned hash = nsScope->ownerNamespace ? StringHashContinue(nsScope->ownerNamespace->fullNameHash, ".") : GetStringHash("");

		for(SynIdentifier *part = path.head; part; part = getType<SynIdentifier>(part->next))
		{
			hash = StringHashContinue(hash, part->name.begin, part->name.end);
			hash = StringHashContinue(hash, ".");
		}

		hash = StringHashContinue(hash, name.begin, name.end);

		variable = ctx.variableMap.find(hash);

		if(variable)
			break;
	}

	if(variable)
	{
		VariableData *data = *variable;

		// TODO: check external variable access

		if(data->type == ctx.typeAuto)
			Stop(ctx, name.begin, "ERROR: variable '%.*s' is being used while its type is unknown", FMT_ISTR(name));

		// Is this is a class member access
		if(data->scope->ownerType)
		{
			ExprBase *thisAccess = AnalyzeVariableAccess(ctx, syntax, IntrusiveList<SynIdentifier>(), InplaceStr("this"));

			// Member access only shifts an address, so we are left with a reference to get value from
			ExprMemberAccess *shift = new ExprMemberAccess(syntax, ctx.GetReferenceType(data->type), thisAccess, data);

			return new ExprDereference(syntax, data->type, shift);
		}

		// TODO: external variable lookup

		return new ExprVariableAccess(syntax, data->type, data);
	}

	HashMap<FunctionData*>::Node *function = NULL;

	if(TypeBase* type = FindNextTypeFromScope(ctx.scopes.back()))
	{
		unsigned hash = StringHashContinue(type->nameHash, "::");

		hash = StringHashContinue(hash, name.begin, name.end);

		function = ctx.functionMap.first(hash);

		// Try accessor
		if(!function)
		{
			hash = StringHashContinue(hash, "$");

			function = ctx.functionMap.first(hash);
		}
	}

	if(!function)
	{
		for(ScopeData *nsScope = NamedOrGlobalScopeFrom(ctx.scopes.back()); nsScope; nsScope = NamedOrGlobalScopeFrom(nsScope->scope))
		{
			unsigned hash = nsScope->ownerNamespace ? StringHashContinue(nsScope->ownerNamespace->fullNameHash, ".") : GetStringHash("");

			for(SynIdentifier *part = path.head; part; part = getType<SynIdentifier>(part->next))
			{
				hash = StringHashContinue(hash, part->name.begin, part->name.end);
				hash = StringHashContinue(hash, ".");
			}

			hash = StringHashContinue(hash, name.begin, name.end);

			function = ctx.functionMap.first(hash);

			if(function)
				break;
		}
	}

	if(function)
	{
		if(HashMap<FunctionData*>::Node *curr = ctx.functionMap.next(function))
		{
			IntrusiveList<TypeHandle> types;
			IntrusiveList<FunctionHandle> functions;

			types.push_back(new TypeHandle(function->value->type));
			functions.push_back(new FunctionHandle(function->value));
			
			while(curr)
			{
				types.push_back(new TypeHandle(curr->value->type));
				functions.push_back(new FunctionHandle(curr->value));

				curr = ctx.functionMap.next(curr);
			}

			TypeFunctionSet *type = new TypeFunctionSet(GetFunctionSetTypeName(types), types);

			return new ExprFunctionOverloadSet(syntax, type, functions);
		}

		return new ExprFunctionAccess(syntax, function->value->type, function->value);
	}

	Stop(ctx, name.begin, "ERROR: unknown variable");

	return NULL;
}

ExprBase* AnalyzeVariableAccess(ExpressionContext &ctx, SynIdentifier *syntax)
{
	return AnalyzeVariableAccess(ctx, syntax, IntrusiveList<SynIdentifier>(), syntax->name);
}

ExprBase* AnalyzeVariableAccess(ExpressionContext &ctx, SynTypeSimple *syntax)
{
	return AnalyzeVariableAccess(ctx, syntax, syntax->path, syntax->name);
}

ExprPreModify* AnalyzePreModify(ExpressionContext &ctx, SynPreModify *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	ExprBase* wrapped = value;

	if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
	{
		wrapped = new ExprGetAddress(syntax, ctx.GetReferenceType(value->type), node->variable);
	}
	else if(ExprDereference *node = getType<ExprDereference>(value))
	{
		wrapped = node->value;
	}

	if(!isType<TypeRef>(wrapped->type))
		Stop(ctx, syntax->pos, "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(value->type->name));

	return new ExprPreModify(syntax, value->type, wrapped, syntax->isIncrement);
}

ExprPostModify* AnalyzePostModify(ExpressionContext &ctx, SynPostModify *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	ExprBase* wrapped = value;

	if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
	{
		wrapped = new ExprGetAddress(syntax, ctx.GetReferenceType(value->type), node->variable);
	}
	else if(ExprDereference *node = getType<ExprDereference>(value))
	{
		wrapped = node->value;
	}

	if(!isType<TypeRef>(wrapped->type))
		Stop(ctx, syntax->pos, "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(value->type->name));

	return new ExprPostModify(syntax, value->type, wrapped, syntax->isIncrement);
}

ExprUnaryOp* AnalyzeUnaryOp(ExpressionContext &ctx, SynUnaryOp *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	// TODO: check operator overload

	bool binaryOp = IsBinaryOp(syntax->type);
	bool logicalOp = IsLogicalOp(syntax->type);

	// Type check
	if(ctx.IsFloatingPointType(value->type))
	{
		if(binaryOp || logicalOp)
			Stop(ctx, syntax->pos, "ERROR: unary operation '%s' is not supported on '%.*s'", GetOpName(syntax->type), FMT_ISTR(value->type->name));
	}
	else if(value->type == ctx.typeBool || value->type == ctx.typeAutoRef)
	{
		if(!logicalOp)
			Stop(ctx, syntax->pos, "ERROR: unary operation '%s' is not supported on '%.*s'", GetOpName(syntax->type), FMT_ISTR(value->type->name));
	}
	else if(isType<TypeRef>(value->type))
	{
		if(!logicalOp)
			Stop(ctx, syntax->pos, "ERROR: unary operation '%s' is not supported on '%.*s'", GetOpName(syntax->type), FMT_ISTR(value->type->name));
	}
	else if(!ctx.IsNumericType(value->type))
	{
		Stop(ctx, syntax->pos, "ERROR: unary operation '%s' is not supported on '%.*s'", GetOpName(syntax->type), FMT_ISTR(value->type->name));
	}

	TypeBase *resultType = NULL;

	if(logicalOp)
		resultType = ctx.typeBool;
	else
		resultType = value->type;

	return new ExprUnaryOp(syntax, resultType, syntax->type, value);
}

ExprBinaryOp* AnalyzeBinaryOp(ExpressionContext &ctx, SynBinaryOp *syntax)
{
	ExprBase *lhs = AnalyzeExpression(ctx, syntax->lhs);
	ExprBase *rhs = AnalyzeExpression(ctx, syntax->rhs);

	// TODO: 'in' is a function call
	// TODO: == and != for typeAutoRef is a function call
	// TODO: == and != for function types is a function call
	// TODO: == and != for unsized arrays is an overload call or a function call
	// TODO: && and || could have an operator overload where second argument is wrapped in a function for short-circuit evaluation
	// TODO: check operator overload

	if(!ctx.IsNumericType(lhs->type) || !ctx.IsNumericType(rhs->type))
		Stop(ctx, syntax->pos, "ERROR: binary operations between complex types are not supported yet");

	if(lhs->type == ctx.typeVoid)
		Stop(ctx, syntax->pos, "ERROR: first operand type is 'void'");

	if(rhs->type == ctx.typeVoid)
		Stop(ctx, syntax->pos, "ERROR: second operand type is 'void'");

	bool binaryOp = IsBinaryOp(syntax->type);
	bool comparisonOp = IsComparisonOp(syntax->type);
	bool logicalOp = IsLogicalOp(syntax->type);

	if(ctx.IsFloatingPointType(lhs->type) || ctx.IsFloatingPointType(rhs->type))
	{
		if(logicalOp || binaryOp)
			Stop(ctx, syntax->pos, "ERROR: operation %s is not supported on '%.*s' and '%.*s'", GetOpName(syntax->type), FMT_ISTR(lhs->type->name), FMT_ISTR(rhs->type->name));
	}

	if(logicalOp)
	{
		// Logical operations require both operands to be 'bool'
		lhs = CreateCast(ctx, syntax->lhs->pos, lhs, ctx.typeBool);
		rhs = CreateCast(ctx, syntax->rhs->pos, rhs, ctx.typeBool);
	}
	else if(ctx.IsNumericType(lhs->type) && ctx.IsNumericType(rhs->type))
	{
		// Numeric operations promote both operands to a common type
		TypeBase *commonType = ctx.GetBinaryOpResultType(lhs->type, rhs->type);

		lhs = CreateCast(ctx, syntax->lhs->pos, lhs, commonType);
		rhs = CreateCast(ctx, syntax->rhs->pos, rhs, commonType);
	}

	if(lhs->type != rhs->type)
		Stop(ctx, syntax->pos, "ERROR: operation %s is not supported on '%.*s' and '%.*s'", GetOpName(syntax->type), FMT_ISTR(lhs->type->name), FMT_ISTR(rhs->type->name));

	TypeBase *resultType = NULL;

	if(comparisonOp || logicalOp)
		resultType = ctx.typeBool;
	else
		resultType = lhs->type;

	return new ExprBinaryOp(syntax, resultType, syntax->type, lhs, rhs);
}

ExprBase* AnalyzeGetAddress(ExpressionContext &ctx, SynGetAddress *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
	{
		return new ExprGetAddress(syntax, ctx.GetReferenceType(value->type), node->variable);
	}
	else if(ExprDereference *node = getType<ExprDereference>(value))
	{
		return node->value;
	}

	Stop(ctx, syntax->pos, "ERROR: cannot get address of the expression");

	return NULL;
}

ExprDereference* AnalyzeDereference(ExpressionContext &ctx, SynDereference *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	if(TypeRef *type = getType<TypeRef>(value->type))
	{
		return new ExprDereference(syntax, type->subType, value);
	}

	Stop(ctx, syntax->pos, "ERROR: cannot dereference type '%.*s' that is not a pointer", FMT_ISTR(value->type->name));

	return NULL;
}

ExprConditional* AnalyzeConditional(ExpressionContext &ctx, SynConditional *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	condition = CreateConditionCast(ctx, condition);

	ExprBase *trueBlock = AnalyzeStatement(ctx, syntax->trueBlock);
	ExprBase *falseBlock = AnalyzeStatement(ctx, syntax->falseBlock);

	TypeBase *resultType = NULL;

	if(trueBlock->type == falseBlock->type)
		resultType = trueBlock->type;
	else if(ctx.IsNumericType(trueBlock->type) && ctx.IsNumericType(falseBlock->type))
		resultType = ctx.GetBinaryOpResultType(trueBlock->type, falseBlock->type);
	else
		Stop(ctx, syntax->pos, "ERROR: Unknown common type");

	return new ExprConditional(syntax, resultType, condition, trueBlock, falseBlock);
}

ExprAssignment* AnalyzeAssignment(ExpressionContext &ctx, SynAssignment *syntax)
{
	ExprBase *lhs = AnalyzeExpression(ctx, syntax->lhs);
	ExprBase *rhs = AnalyzeExpression(ctx, syntax->rhs);

	return CreateAssignment(ctx, syntax->pos, lhs, rhs);
}

ExprModifyAssignment* AnalyzeModifyAssignment(ExpressionContext &ctx, SynModifyAssignment *syntax)
{
	ExprBase *lhs = AnalyzeExpression(ctx, syntax->lhs);
	ExprBase *rhs = AnalyzeExpression(ctx, syntax->rhs);

	ExprBase* wrapped = lhs;

	if(ExprVariableAccess *node = getType<ExprVariableAccess>(lhs))
	{
		wrapped = new ExprGetAddress(syntax, ctx.GetReferenceType(lhs->type), node->variable);
	}
	else if(ExprDereference *node = getType<ExprDereference>(lhs))
	{
		wrapped = node->value;
	}

	if(!isType<TypeRef>(wrapped->type))
		Stop(ctx, syntax->pos, "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(lhs->type->name));

	if(rhs->type == ctx.typeVoid)
		Stop(ctx, syntax->pos, "ERROR: cannot convert from void to %.*s", FMT_ISTR(rhs->type->name));

	if(lhs->type == ctx.typeVoid)
		Stop(ctx, syntax->pos, "ERROR: cannot convert from %.*s to void", FMT_ISTR(lhs->type->name));

	return new ExprModifyAssignment(syntax, lhs->type, syntax->type, wrapped, rhs);
}

ExprBase* AnalyzeMemberAccess(ExpressionContext &ctx, SynMemberAccess *syntax)
{
	// It could be a type property
	if(TypeBase *type = AnalyzeType(ctx, syntax->value, false))
	{
		if(syntax->member == InplaceStr("isReference"))
		{
			return new ExprBoolLiteral(syntax, ctx.typeBool, isType<TypeRef>(type));
		}

		if(syntax->member == InplaceStr("isArray"))
		{
			return new ExprBoolLiteral(syntax, ctx.typeBool, isType<TypeArray>(type) || isType<TypeUnsizedArray>(type));
		}

		if(syntax->member == InplaceStr("isFunction"))
		{
			return new ExprBoolLiteral(syntax, ctx.typeBool, isType<TypeFunction>(type));
		}

		if(syntax->member == InplaceStr("arraySize"))
		{
			if(TypeArray *arrType = getType<TypeArray>(type))
				return new ExprIntegerLiteral(syntax, ctx.typeInt, arrType->length);
				
			if(TypeUnsizedArray *arrType = getType<TypeUnsizedArray>(type))
				return new ExprIntegerLiteral(syntax, ctx.typeInt, -1);

			Stop(ctx, syntax->pos, "ERROR: 'arraySize' can only be applied to an array type, but we have '%s'", type->name);
		}

		Stop(ctx, syntax->pos, "ERROR: unknown expression type");
		// isReference/isArray/isFunction/arraySize/hasMember(x)/class constant/class typedef
	}

	ExprBase* value = AnalyzeExpression(ctx, syntax->value);

	if(TypeArray *node = getType<TypeArray>(value->type))
	{
		if(syntax->member == InplaceStr("size"))
			return new ExprIntegerLiteral(syntax, ctx.typeInt, node->length);

		Stop(ctx, syntax->pos, "ERROR: array doesn't have member with this name");
	}

	ExprBase* wrapped = value;

	if(TypeRef *refType = getType<TypeRef>(value->type))
	{
		value = new ExprDereference(syntax, refType->subType, value);
	}
	else
	{
		if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
		{
			wrapped = new ExprGetAddress(syntax, ctx.GetReferenceType(value->type), node->variable);
		}
		else if(ExprDereference *node = getType<ExprDereference>(value))
		{
			wrapped = node->value;
		}
	}

	if(isType<TypeRef>(wrapped->type))
	{
		if(TypeStruct *node = getType<TypeStruct>(value->type))
		{
			for(VariableHandle *el = node->members.head; el; el = el->next)
			{
				if(el->variable->name == syntax->member)
				{
					// Member access only shifts an address, so we are left with a reference to get value from
					ExprMemberAccess *shift = new ExprMemberAccess(syntax, ctx.GetReferenceType(el->variable->type), wrapped, el->variable);

					return new ExprDereference(syntax, el->variable->type, shift);
				}
			}

			Stop(ctx, syntax->pos, "ERROR: member variable or function '%.*s' is not defined in class '%.*s'", FMT_ISTR(syntax->member), FMT_ISTR(value->type->name));
		}
	}

	Stop(ctx, syntax->pos, "ERROR: can't access member '%.*s' of type '%.*s'", FMT_ISTR(syntax->member), FMT_ISTR(value->type->name));

	return NULL;
}

ExprBase* AnalyzeArrayIndex(ExpressionContext &ctx, SynArrayIndex *syntax)
{
	if(syntax->arguments.size() > 1)
		Stop(ctx, syntax->pos, "ERROR: multiple array indexes are not supported");

	if(syntax->arguments.empty())
		Stop(ctx, syntax->pos, "ERROR: array index is missing");

	SynCallArgument *argument = syntax->arguments.head;

	if(!argument->name.empty())
		Stop(ctx, syntax->pos, "ERROR: named array indexes are not supported");

	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	ExprBase *index = AnalyzeExpression(ctx, argument->value);

	index = CreateCast(ctx, syntax->pos, index, ctx.typeInt);

	ExprIntegerLiteral *indexValue = getType<ExprIntegerLiteral>(Evaluate(ctx, index));

	if(indexValue && indexValue->value < 0)
		Stop(ctx, syntax->pos, "ERROR: array index cannot be negative");

	ExprBase* wrapped = value;

	if(TypeRef *refType = getType<TypeRef>(value->type))
	{
		value = new ExprDereference(syntax, refType->subType, value);

		if(isType<TypeUnsizedArray>(value->type))
			wrapped = value;
	}
	else if(isType<TypeUnsizedArray>(value->type))
	{
		wrapped = value; // Do not modify
	}
	else
	{
		if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
		{
			wrapped = new ExprGetAddress(syntax, ctx.GetReferenceType(value->type), node->variable);
		}
		else if(ExprDereference *node = getType<ExprDereference>(value))
		{
			wrapped = node->value;
		}
	}

	if(isType<TypeRef>(wrapped->type) || isType<TypeUnsizedArray>(value->type))
	{
		if(TypeArray *type = getType<TypeArray>(value->type))
		{
			if(indexValue && indexValue->value >= type->length)
				Stop(ctx, syntax->pos, "ERROR: array index bounds");

			// Array index only shifts an address, so we are left with a reference to get value from
			ExprArrayIndex *shift = new ExprArrayIndex(syntax, ctx.GetReferenceType(type->subType), wrapped, index);

			return new ExprDereference(syntax, type->subType, shift);
		}

		if(TypeUnsizedArray *type = getType<TypeUnsizedArray>(value->type))
		{
			// Array index only shifts an address, so we are left with a reference to get value from
			ExprArrayIndex *shift = new ExprArrayIndex(syntax, ctx.GetReferenceType(type->subType), wrapped, index);

			return new ExprDereference(syntax, type->subType, shift);
		}
	}

	Stop(ctx, syntax->pos, "ERROR: type '%.*s' is not an array", FMT_ISTR(value->type->name));

	return NULL;
}

ExprBase* AnalyzeArrayIndex(ExpressionContext &ctx, SynTypeArray *syntax)
{
	assert(syntax->sizes.head);

	SynArrayIndex *value = NULL;

	// Convert to a chain of SynArrayIndex
	for(SynBase *el = syntax->sizes.head; el; el = el->next)
	{
		IntrusiveList<SynCallArgument> arguments;

		if(!isType<SynNothing>(el))
			arguments.push_back(new SynCallArgument(el->pos, InplaceStr(), el));

		value = new SynArrayIndex(el->pos, value ? value : syntax->type, arguments);
	}

	return AnalyzeArrayIndex(ctx, value);
}

ExprFunctionCall* AnalyzeFunctionCall(ExpressionContext &ctx, SynFunctionCall *syntax)
{
	ExprBase *function = AnalyzeExpression(ctx, syntax->value);

	if(!syntax->aliases.empty())
		Stop(ctx, syntax->pos, "ERROR: function call with explicit generic arguments is not supported");

	IntrusiveList<ExprBase> arguments;

	for(SynCallArgument *el = syntax->arguments.head; el; el = getType<SynCallArgument>(el->next))
	{
		if(!el->name.empty())
			Stop(ctx, syntax->pos, "ERROR: named function arguments are not supported");

		ExprBase *argument = AnalyzeExpression(ctx, el->value);

		if(isType<ExprGenericFunctionPrototype>(argument))
			Stop(ctx, syntax->pos, "ERROR: generic function arguments are not supported");

		if(isType<ExprFunctionOverloadSet>(argument))
			Stop(ctx, syntax->pos, "ERROR: function overload arguments are not supported");

		if(TypeFunction *type = getType<TypeFunction>(argument->type))
		{
			if(type->isGeneric)
				Stop(ctx, syntax->pos, "ERROR: generic function pointer arguments are not supported");
		}

		arguments.push_back(argument);
	}

	if(TypeFunction *type = getType<TypeFunction>(function->type))
	{
		if(type->isGeneric)
			Stop(ctx, syntax->pos, "ERROR: generic function call is not supported");

		if(type->returnType == ctx.typeAuto)
			Stop(ctx, syntax->pos, "ERROR: function can't return auto");

		return new ExprFunctionCall(syntax, type->returnType, function, arguments);
	}

	if(isType<ExprFunctionOverloadSet>(function))
		Stop(ctx, syntax->pos, "ERROR: function overloads are not supported");

	Stop(ctx, syntax->pos, "ERROR: unknown call");

	return NULL;
}

ExprFunctionCall* AnalyzeNew(ExpressionContext &ctx, SynNew *syntax)
{
	if(!syntax->constructor.empty())
		Stop(ctx, syntax->pos, "ERROR: custom constructors are not supported");

	if(!syntax->arguments.empty())
		Stop(ctx, syntax->pos, "ERROR: constructor call is not supported");

	TypeBase *type = AnalyzeType(ctx, syntax->type);

	ExprFunctionCall *call = NULL;

	if(syntax->count)
	{
		// TODO: implicit cast for manual function call
		ExprBase *count = AnalyzeExpression(ctx, syntax->count);

		FunctionData **function = ctx.functionMap.find(GetStringHash("__newA"));

		if(!function || (*function)->name != InplaceStr("__newA"))
			Stop(ctx, syntax->pos, "ERROR: internal error, __newA not found");

		IntrusiveList<ExprBase> arguments;

		arguments.push_back(new ExprIntegerLiteral(syntax, ctx.typeInt, type->size));
		arguments.push_back(count);
		arguments.push_back(new ExprTypeLiteral(syntax, ctx.typeTypeID, type));

		call = new ExprFunctionCall(syntax, ctx.GetUnsizedArrayType(type), new ExprFunctionAccess(syntax, (*function)->type, *function), arguments);
	}
	else
	{
		FunctionData **function = ctx.functionMap.find(GetStringHash("__newS"));

		if(!function || (*function)->name != InplaceStr("__newS"))
			Stop(ctx, syntax->pos, "ERROR: internal error, __newS not found");

		IntrusiveList<ExprBase> arguments;

		arguments.push_back(new ExprIntegerLiteral(syntax, ctx.typeInt, type->size));
		arguments.push_back(new ExprTypeLiteral(syntax, ctx.typeTypeID, type));

		call = new ExprFunctionCall(syntax, ctx.GetReferenceType(type), new ExprFunctionAccess(syntax, (*function)->type, *function), arguments);
	}

	return call;
}

ExprReturn* AnalyzeReturn(ExpressionContext &ctx, SynReturn *syntax)
{
	// TODO: implicit cast
	// TODO: return type deduction

	ExprBase *result = syntax->value ? AnalyzeExpression(ctx, syntax->value) : new ExprVoid(syntax, ctx.typeVoid);

	if(FunctionData *function = ctx.GetCurrentFunction())
	{
		TypeBase *returnType = function->type->returnType;

		// If return type is auto, set it to type that is being returned
		if(returnType == ctx.typeAuto)
		{
			returnType = result->type;

			function->type = ctx.GetFunctionType(returnType, function->type->arguments);
		}
		else
		{
			result = CreateCast(ctx, syntax->pos, result, function->type->returnType);
		}

		if(returnType == ctx.typeVoid && result->type != ctx.typeVoid)
			Stop(ctx, syntax->pos, "ERROR: 'void' function returning a value");
		if(returnType != ctx.typeVoid && result->type == ctx.typeVoid)
			Stop(ctx, syntax->pos, "ERROR: function must return a value of type '%s'", FMT_ISTR(returnType->name));

		function->hasExplicitReturn = true;

		// TODO: checked return value

		return new ExprReturn(syntax, ctx.typeVoid, result);
	}

	if(!ctx.IsNumericType(result->type))
		Stop(ctx, syntax->pos, "ERROR: global return cannot accept '%.*s'", FMT_ISTR(result->type->name));

	return new ExprReturn(syntax, ctx.typeVoid, result);
}

ExprYield* AnalyzeYield(ExpressionContext &ctx, SynYield *syntax)
{
	// TODO: implicit cast
	// TODO: return type deduction

	ExprBase *result = syntax->value ? AnalyzeExpression(ctx, syntax->value) : new ExprVoid(syntax, ctx.typeVoid);

	if(FunctionData *function = ctx.GetCurrentFunction())
	{
		if(!function->coroutine)
			Stop(ctx, syntax->pos, "ERROR: yield can only be used inside a coroutine");

		TypeBase *returnType = function->type->returnType;

		// If return type is auto, set it to type that is being returned
		if(returnType == ctx.typeAuto)
		{
			returnType = result->type;

			function->type = ctx.GetFunctionType(returnType, function->type->arguments);
		}
		else
		{
			result = CreateCast(ctx, syntax->pos, result, function->type->returnType);
		}

		if(returnType == ctx.typeVoid && result->type != ctx.typeVoid)
			Stop(ctx, syntax->pos, "ERROR: 'void' function returning a value");
		if(returnType != ctx.typeVoid && result->type == ctx.typeVoid)
			Stop(ctx, syntax->pos, "ERROR: function must return a value of type '%s'", FMT_ISTR(returnType->name));

		function->hasExplicitReturn = true;

		// TODO: checked return value

		return new ExprYield(syntax, ctx.typeVoid, result);
	}

	Stop(ctx, syntax->pos, "ERROR: global yield is not allowed");

	return NULL;
}

ExprVariableDefinition* AnalyzeVariableDefinition(ExpressionContext &ctx, SynVariableDefinition *syntax, unsigned alignment, TypeBase *type)
{
	if(syntax->name == InplaceStr("this"))
		Stop(ctx, syntax->pos, "ERROR: 'this' is a reserved keyword");

	InplaceStr fullName = GetVariableNameInScope(ctx.scopes.back(), syntax->name);

	if(ctx.typeMap.find(fullName.hash()))
		Stop(ctx, syntax->pos, "ERROR: name '%.*s' is already taken for a class", FMT_ISTR(syntax->name));

	// TODO: check for variables with the same name in current scope
	// TODO: check for functions with the same name

	ExprBase *initializer = syntax->initializer ? AnalyzeExpression(ctx, syntax->initializer) : NULL;

	if(type == ctx.typeAuto)
	{
		if(!initializer)
			Stop(ctx, syntax->pos, "ERROR: auto variable must be initialized in place of definition");

		if(initializer->type == ctx.typeVoid)
			Stop(ctx, syntax->pos, "ERROR: r-value type is 'void'");

		type = initializer->type;
	}

	if(alignment == 0 && type->alignment != 0)
		alignment = type->alignment;

	assert(!type->isGeneric);
	assert(type != ctx.typeAuto);

	unsigned offset = AllocateVariableInScope(ctx.scopes.back(), alignment, type);
	VariableData *variable = new VariableData(syntax, ctx.scopes.back(), alignment, type, fullName, offset, ctx.uniqueVariableId++);

	ctx.AddVariable(variable);

	if(initializer)
	{
		TypeArray *arrType = getType<TypeArray>(variable->type);

		// Single-level array might be set with a single element at the point of definition
		if(arrType && !isType<TypeArray>(initializer->type))
		{
			initializer = CreateCast(ctx, syntax->initializer->pos, initializer, arrType->subType);

			initializer = new ExprArraySetup(syntax->initializer, ctx.typeVoid, variable, initializer);
		}
		else
		{
			initializer = CreateAssignment(ctx, syntax->initializer->pos, new ExprVariableAccess(syntax->initializer, variable->type, variable), initializer);
		}
	}

	return new ExprVariableDefinition(syntax, ctx.typeVoid, variable, initializer);
}

ExprVariableDefinitions* AnalyzeVariableDefinitions(ExpressionContext &ctx, SynVariableDefinitions *syntax)
{
	unsigned alignment = syntax->align ? AnalyzeAlignment(ctx, syntax->align) : 0;

	TypeBase *type = AnalyzeType(ctx, syntax->type);

	IntrusiveList<ExprVariableDefinition> definitions;

	for(SynVariableDefinition *el = syntax->definitions.head; el; el = getType<SynVariableDefinition>(el->next))
		definitions.push_back(AnalyzeVariableDefinition(ctx, el, alignment, type));

	return new ExprVariableDefinitions(syntax, ctx.typeVoid, definitions);
}

ExprBase* AnalyzeFunctionDefinition(ExpressionContext &ctx, SynFunctionDefinition *syntax)
{
	TypeBase *parentType = syntax->parentType ? AnalyzeType(ctx, syntax->parentType) : NULL;

	if(parentType)
	{
		ctx.PushScope(parentType);

		// TODO: introduce class contents
		if(TypeClass *classType = getType<TypeClass>(parentType))
		{
			for(VariableHandle *el = classType->members.head; el; el = el->next)
				ctx.AddVariable(el->variable);
		}
	}

	if(!syntax->aliases.empty())
		Stop(ctx, syntax->pos, "ERROR: functions with explicit generic arguments are not implemented");

	TypeBase *returnType = AnalyzeType(ctx, syntax->returnType);

	IntrusiveList<TypeHandle> argTypes;

	for(SynFunctionArgument *argument = syntax->arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next))
	{
		// Create temporary scope with known arguments for reference in type expression
		ctx.PushScope();

		{
			SynFunctionArgument *prevArg = syntax->arguments.head;
			TypeHandle *prevType = argTypes.head;

			while(prevArg && prevArg != argument)
			{
				ctx.AddVariable(new VariableData(prevArg, ctx.scopes.back(), 0, prevType->type, prevArg->name, 0, ctx.uniqueVariableId++));

				prevArg = getType<SynFunctionArgument>(prevArg->next);
				prevType = prevType->next;
			}
		}
		
		TypeBase *type = AnalyzeType(ctx, argument->type);

		// Remove temporary scope
		ctx.PopScope();

		argTypes.push_back(new TypeHandle(type));
	}

	TypeFunction *functionType = ctx.GetFunctionType(returnType, argTypes);

	InplaceStr functionName;

	if(syntax->name.empty())
	{
		char *name = new char[16];
		sprintf(name, "$func%d", ctx.unnamedFuncCount++);

		functionName = InplaceStr(name);
	}
	else if(syntax->isOperator)
	{
		functionName = syntax->name;
	}
	else
	{
		functionName = GetFunctionNameInScope(ctx.scopes.back(), syntax->name);
	}

	// TODO: function type should be stored in type list

	FunctionData *function = new FunctionData(syntax, ctx.scopes.back(), syntax->coroutine, functionType, functionName, ctx.uniqueFunctionId++);

	// If the type is known, implement the prototype immediately
	if(functionType->returnType != ctx.typeAuto)
		ImplementPrototype(ctx, function);

	ctx.AddFunction(function);

	if(function->type->isGeneric || (parentType && isType<TypeGenericClassProto>(parentType)))
	{
		if(syntax->prototype)
			Stop(ctx, syntax->pos, "ERROR: generic function cannot be forward-declared");

		if(parentType)
			ctx.PopScope();

		return new ExprGenericFunctionPrototype(syntax, ctx.typeVoid, function);
	}

	ctx.PushScope(function);

	if(TypeBase *parent = function->scope->ownerType)
	{
		TypeBase *type = ctx.GetReferenceType(parent);

		assert(!type->isGeneric);

		unsigned offset = AllocateVariableInScope(ctx.scopes.back(), 0, type);
		VariableData *variable = new VariableData(syntax, ctx.scopes.back(), 0, type, InplaceStr("this"), offset, ctx.uniqueVariableId++);

		ctx.AddVariable(variable);
	}

	IntrusiveList<ExprVariableDefinition> arguments;

	for(SynFunctionArgument *argument = syntax->arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next))
	{
		if(argument->isExplicit)
			Stop(ctx, syntax->pos, "ERROR: explicit type arguments are not supported");

		TypeBase *type = AnalyzeType(ctx, argument->type);

		assert(!type->isGeneric);

		unsigned offset = AllocateVariableInScope(ctx.scopes.back(), 0, type);
		VariableData *variable = new VariableData(argument, ctx.scopes.back(), 0, type, argument->name, offset, ctx.uniqueVariableId++);

		ctx.AddVariable(variable);

		ExprBase *initializer = argument->initializer ? AnalyzeExpression(ctx, argument->initializer) : NULL;

		arguments.push_back(new ExprVariableDefinition(argument, ctx.typeVoid, variable, initializer));
	}

	IntrusiveList<ExprBase> expressions;

	if(syntax->prototype)
	{
		if(function->type->returnType == ctx.typeAuto)
			Stop(ctx, syntax->pos, "ERROR: function prototype with unresolved return type");

		function->isPrototype = true;
	}
	else
	{
		for(SynBase *expression = syntax->expressions.head; expression; expression = expression->next)
			expressions.push_back(AnalyzeStatement(ctx, expression));

		// If the function type is still auto it means that it hasn't returned anything
		if(function->type->returnType == ctx.typeAuto)
			function->type = ctx.GetFunctionType(ctx.typeVoid, function->type->arguments);

		if(function->type->returnType != ctx.typeVoid && !function->hasExplicitReturn)
			Stop(ctx, syntax->pos, "ERROR: function must return a value of type '%s'", FMT_ISTR(returnType->name));
	}

	ctx.PopScope();

	if(parentType)
		ctx.PopScope();

	// If the type was deduced, implement prototype now that it's known
	ImplementPrototype(ctx, function);

	FunctionData *conflict = CheckUniqueness(ctx, function);

	if(conflict)
		Stop(ctx, syntax->pos, "ERROR: function '%.*s' is being defined with the same set of parameters", FMT_ISTR(function->name));

	return new ExprFunctionDefinition(syntax, function->type, function, arguments, expressions);
}

void AnalyzeClassStaticIf(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassStaticIf *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	condition = CreateConditionCast(ctx, condition);

	if(ExprBoolLiteral *number = getType<ExprBoolLiteral>(Evaluate(ctx, CreateCast(ctx, syntax->pos, condition, ctx.typeBool))))
	{
		if(number->value)
			AnalyzeClassElements(ctx, classDefinition, syntax->trueBlock);
		else if(syntax->falseBlock)
			AnalyzeClassElements(ctx, classDefinition, syntax->falseBlock);
	}

	Stop(ctx, syntax->pos, "ERROR: can't get condition value");
}

void AnalyzeClassElements(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassElements *syntax)
{
	// TODO: can't access sizeof and type members until finalization

	if(syntax->typedefs.head)
		Stop(ctx, syntax->pos, "ERROR: class typedefs not implemented");
	//for(SynTypedef *typeDef = syntax->typedefs.head; typeDef; typeDef = getType<SynTypedef>(typeDef->next))
	//	classType->typedefs.push_back(AnalyzeTypedef(ctx, typeDef));

	for(SynVariableDefinitions *member = syntax->members.head; member; member = getType<SynVariableDefinitions>(member->next))
	{
		ExprVariableDefinitions *node = AnalyzeVariableDefinitions(ctx, member);

		for(ExprVariableDefinition *definition = node->definitions.head; definition; definition = getType<ExprVariableDefinition>(definition->next))
		{
			if(definition->initializer)
				Stop(ctx, syntax->pos, "ERROR: member can't have an initializer");

			classDefinition->classType->members.push_back(new VariableHandle(definition->variable));
		}
	}

	FinalizeAlignment(classDefinition->classType);

	if(syntax->constantSets.head)
		Stop(ctx, syntax->pos, "ERROR: class constants not implemented");
	//for(SynConstantSet *constantSet = syntax->constantSets.head; constantSet; constantSet = getType<SynConstantSet>(constantSet->next))
	//	classType->constants.push_back(AnalyzeConstantSet(ctx, constantSet));

	for(SynFunctionDefinition *function = syntax->functions.head; function; function = getType<SynFunctionDefinition>(function->next))
		classDefinition->functions.push_back(AnalyzeFunctionDefinition(ctx, function));

	if(syntax->accessors.head)
		Stop(ctx, syntax->pos, "ERROR: class accessors not implemented");
	//for(SynAccessor *accessor = syntax->accessors.head; accessor; accessor = getType<SynAccessor>(accessor->next))
	//	classType->accessors.push_back(AnalyzeAccessorDefinition(ctx, accessor));

	// TODO: The way SynClassElements is made, it could allow member re-ordering! class should contain in-order members and static if's
	// TODO: We should be able to analyze all static if typedefs before members and constants and analyze them before functions
	for(SynClassStaticIf *staticIf = syntax->staticIfs.head; staticIf; staticIf = getType<SynClassStaticIf>(staticIf->next))
		AnalyzeClassStaticIf(ctx, classDefinition, staticIf);
}

ExprBase* AnalyzeClassDefinition(ExpressionContext &ctx, SynClassDefinition *syntax, TypeGenericClassProto *proto, IntrusiveList<TypeHandle> generics)
{
	InplaceStr typeName = GetTypeNameInScope(ctx.scopes.back(), syntax->name);

	if(!proto && !syntax->aliases.empty())
	{
		TypeGenericClassProto *genericProtoType = new TypeGenericClassProto(typeName, syntax);

		ctx.AddType(genericProtoType);

		return new ExprGenericClassPrototype(syntax, ctx.typeVoid, genericProtoType);
	}

	assert(generics.size() == syntax->aliases.size());

	InplaceStr className = generics.empty() ? typeName : GetGenericClassName(proto, generics);

	if(!generics.empty())
	{
		// Check if type already exists
		assert(ctx.genericTypeMap.find(className.hash()) == NULL);

		if(ctx.GetGenericClassInstantiationDepth() > NULLC_MAX_GENERIC_INSTANCE_DEPTH)
			Stop(ctx, syntax->pos, "ERROR: reached maximum generic type instance depth (%d)", NULLC_MAX_GENERIC_INSTANCE_DEPTH);
	}

	unsigned alignment = syntax->align ? AnalyzeAlignment(ctx, syntax->align) : 0;

	TypeBase *baseClass = syntax->baseClass ? AnalyzeType(ctx, syntax->baseClass) : NULL;

	TypeClass *classType = new TypeClass(ctx.scopes.back(), className, syntax->aliases, generics, syntax->extendable, baseClass);

	classType->alignment = alignment;

	ctx.AddType(classType);

	if(!generics.empty())
		ctx.genericTypeMap.insert(className.hash(), classType);

	ExprClassDefinition *classDefinition = new ExprClassDefinition(syntax, ctx.typeVoid, classType);

	ctx.PushScope(classType);

	IntrusiveList<AliasHandle> aliases;

	{
		TypeHandle *currType = classType->genericTypes.head;
		SynIdentifier *currName = classType->genericNames.head;

		while(currType && currName)
		{
			ctx.AddAlias(new AliasData(currName, ctx.scopes.back(), currType->type, currName->name, ctx.uniqueAliasId++));

			currType = currType->next;
			currName = getType<SynIdentifier>(currName->next);
		}
	}

	// TODO: Base class members should be introduced into the scope

	AnalyzeClassElements(ctx, classDefinition, syntax->elements);

	ctx.PopScope();

	return classDefinition;
}

ExprBlock* AnalyzeNamespaceDefinition(ExpressionContext &ctx, SynNamespaceDefinition *syntax)
{
	for(SynIdentifier *name = syntax->path.head; name; name = getType<SynIdentifier>(name->next))
	{
		NamespaceData *ns = new NamespaceData(syntax, ctx.scopes.back(), ctx.GetCurrentNamespace(), name->name, ctx.uniqueNamespaceId++);

		ctx.namespaces.push_back(ns);

		ctx.PushScope(ns);
	}

	IntrusiveList<ExprBase> expressions;

	for(SynBase *expression = syntax->expressions.head; expression; expression = expression->next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	for(SynIdentifier *name = syntax->path.head; name; name = getType<SynIdentifier>(name->next))
		ctx.PopScope();

	return new ExprBlock(syntax, ctx.typeVoid, expressions);
}

ExprAliasDefinition* AnalyzeTypedef(ExpressionContext &ctx, SynTypedef *syntax)
{
	TypeBase *type = AnalyzeType(ctx, syntax->type);

	AliasData *alias = new AliasData(syntax, ctx.scopes.back(), type, syntax->alias, ctx.uniqueAliasId++);

	ctx.AddAlias(alias);

	return new ExprAliasDefinition(syntax, ctx.typeVoid, alias);
}

ExprBase* AnalyzeIfElse(ExpressionContext &ctx, SynIfElse *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	condition = CreateConditionCast(ctx, condition);

	if(syntax->staticIf)
	{
		// TODO: replace with compile-time evaluation
		if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(condition))
		{
			if(number->value != 0)
			{
				if(SynBlock *node = getType<SynBlock>(syntax->trueBlock))
					return AnalyzeBlock(ctx, node, false);
				else
					return AnalyzeStatement(ctx, syntax->trueBlock);
			}
			else if(syntax->falseBlock)
			{
				if(SynBlock *node = getType<SynBlock>(syntax->falseBlock))
					return AnalyzeBlock(ctx, node, false);
				else
					return AnalyzeStatement(ctx, syntax->falseBlock);
			}

			return new ExprVoid(syntax, ctx.typeVoid);
		}

		Stop(ctx, syntax->pos, "ERROR: can't get condition value");
	}

	ExprBase *trueBlock = AnalyzeStatement(ctx, syntax->trueBlock);
	ExprBase *falseBlock = syntax->falseBlock ? AnalyzeStatement(ctx, syntax->falseBlock) : NULL;

	return new ExprIfElse(syntax, ctx.typeVoid, condition, trueBlock, falseBlock);
}

ExprFor* AnalyzeFor(ExpressionContext &ctx, SynFor *syntax)
{
	ExprBase *initializer = syntax->initializer ? AnalyzeStatement(ctx, syntax->initializer) : new ExprVoid(syntax, ctx.typeVoid);
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);
	ExprBase *increment = syntax->increment ? AnalyzeStatement(ctx, syntax->increment) : new ExprVoid(syntax, ctx.typeVoid);
	ExprBase *body = syntax->body ? AnalyzeStatement(ctx, syntax->body) : new ExprVoid(syntax, ctx.typeVoid);

	condition = CreateConditionCast(ctx, condition);

	return new ExprFor(syntax, ctx.typeVoid, initializer, condition, increment, body);
}

ExprWhile* AnalyzeWhile(ExpressionContext &ctx, SynWhile *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);
	ExprBase *body = syntax->body ? AnalyzeStatement(ctx, syntax->body) : new ExprVoid(syntax, ctx.typeVoid);

	condition = CreateConditionCast(ctx, condition);

	return new ExprWhile(syntax, ctx.typeVoid, condition, body);
}

ExprDoWhile* AnalyzeDoWhile(ExpressionContext &ctx, SynDoWhile *syntax)
{
	ctx.PushScope();

	IntrusiveList<ExprBase> expressions;

	for(SynBase *expression = syntax->expressions.head; expression; expression = expression->next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	condition = CreateConditionCast(ctx, condition);

	return new ExprDoWhile(syntax, ctx.typeVoid, new ExprBlock(syntax, ctx.typeVoid, expressions), condition);
}

ExprBlock* AnalyzeBlock(ExpressionContext &ctx, SynBlock *syntax, bool createScope)
{
	if(createScope)
		ctx.PushScope();

	IntrusiveList<ExprBase> expressions;

	for(SynBase *expression = syntax->expressions.head; expression; expression = expression->next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	if(createScope)
		ctx.PopScope();

	return new ExprBlock(syntax, ctx.typeVoid, expressions);
}

ExprBase* AnalyzeExpression(ExpressionContext &ctx, SynBase *syntax)
{
	if(SynBool *node = getType<SynBool>(syntax))
	{
		return new ExprBoolLiteral(node, ctx.typeBool, node->value);
	}

	if(SynCharacter *node = getType<SynCharacter>(syntax))
	{
		unsigned char result = (unsigned char)node->value.begin[1];

		if(result == '\\')
			result = ParseEscapeSequence(ctx, node->value.begin + 1);

		return new ExprCharacterLiteral(node, ctx.typeChar, result);
	}

	if(SynString *node = getType<SynString>(syntax))
	{
		unsigned length = 0;

		if(node->rawLiteral)
		{
			length = node->value.length() - 2;
		}
		else
		{
			// Find the length of the string with collapsed escape-sequences
			for(const char *curr = node->value.begin + 1, *end = node->value.end - 1 ; curr < end; curr++, length++)
			{
				if(*curr == '\\')
					curr++;
			}
		}

		char *value = new char[length + 1];

		if(node->rawLiteral)
		{
			for(unsigned i = 0; i < length; i++)
				value[i] = node->value.begin[i + 1];

			value[length] = 0;
		}
		else
		{
			unsigned i = 0;

			// Find the length of the string with collapsed escape-sequences
			for(const char *curr = node->value.begin + 1, *end = node->value.end - 1 ; curr < end;)
			{
				if(*curr == '\\')
				{
					value[i++] = ParseEscapeSequence(ctx, curr);
					curr += 2;
				}
				else
				{
					value[i++] = *curr;
					curr += 1;
				}
			}

			value[length] = 0;
		}

		return new ExprStringLiteral(node, ctx.GetArrayType(ctx.typeChar, length + 1), value, length);
	}
	
	if(SynNullptr *node = getType<SynNullptr>(syntax))
	{
		return new ExprNullptrLiteral(node, ctx.GetReferenceType(ctx.typeVoid));
	}

	if(SynNumber *node = getType<SynNumber>(syntax))
	{
		return AnalyzeNumber(ctx, node);
	}

	if(SynArray *node = getType<SynArray>(syntax))
	{
		return AnalyzeArray(ctx, node);
	}

	if(SynPreModify *node = getType<SynPreModify>(syntax))
	{
		return AnalyzePreModify(ctx, node);
	}

	if(SynPostModify *node = getType<SynPostModify>(syntax))
	{
		return AnalyzePostModify(ctx, node);
	}

	if(SynUnaryOp *node = getType<SynUnaryOp>(syntax))
	{
		return AnalyzeUnaryOp(ctx, node);
	}

	if(SynBinaryOp *node = getType<SynBinaryOp>(syntax))
	{
		return AnalyzeBinaryOp(ctx, node);
	}
	
	if(SynGetAddress *node = getType<SynGetAddress>(syntax))
	{
		return AnalyzeGetAddress(ctx, node);
	}

	if(SynDereference *node = getType<SynDereference>(syntax))
	{
		return AnalyzeDereference(ctx, node);
	}

	if(SynTypeof *node = getType<SynTypeof>(syntax))
	{
		ExprBase *value = AnalyzeExpression(ctx, node->value);

		if(value->type == ctx.typeAuto)
			Stop(ctx, syntax->pos, "ERROR: cannot take typeid from auto type");

		return new ExprTypeLiteral(node, ctx.typeTypeID, value->type);
	}

	if(SynIdentifier *node = getType<SynIdentifier>(syntax))
	{
		return AnalyzeVariableAccess(ctx, node);
	}

	if(SynTypeSimple *node = getType<SynTypeSimple>(syntax))
	{
		// It could be a typeid
		if(TypeBase *type = AnalyzeType(ctx, node, false))
		{
			if(type == ctx.typeAuto)
				Stop(ctx, syntax->pos, "ERROR: cannot take typeid from auto type");

			return new ExprTypeLiteral(node, ctx.typeTypeID, type);
		}

		return AnalyzeVariableAccess(ctx, node);
	}

	if(SynSizeof *node = getType<SynSizeof>(syntax))
	{
		ExprBase *value = AnalyzeExpression(ctx, node->value);

		if(value->type == ctx.typeAuto)
			Stop(ctx, syntax->pos, "ERROR: sizeof(auto) is illegal");

		return new ExprIntegerLiteral(node, ctx.typeInt, value->type->size);
	}

	if(SynConditional *node = getType<SynConditional>(syntax))
	{
		return AnalyzeConditional(ctx, node);
	}

	if(SynAssignment *node = getType<SynAssignment>(syntax))
	{
		return AnalyzeAssignment(ctx, node);
	}

	if(SynModifyAssignment *node = getType<SynModifyAssignment>(syntax))
	{
		return AnalyzeModifyAssignment(ctx, node);
	}

	if(SynMemberAccess *node = getType<SynMemberAccess>(syntax))
	{
		// It could be a typeid
		if(TypeBase *type = AnalyzeType(ctx, syntax, false))
		{
			if(type == ctx.typeAuto)
				Stop(ctx, syntax->pos, "ERROR: cannot take typeid from auto type");

			return new ExprTypeLiteral(node, ctx.typeTypeID, type);
		}

		return AnalyzeMemberAccess(ctx, node);
	}

	if(SynTypeArray *node = getType<SynTypeArray>(syntax))
	{
		// It could be a typeid
		if(TypeBase *type = AnalyzeType(ctx, syntax, false))
		{
			if(type == ctx.typeAuto)
				Stop(ctx, syntax->pos, "ERROR: cannot take typeid from auto type");

			return new ExprTypeLiteral(node, ctx.typeTypeID, type);
		}

		return AnalyzeArrayIndex(ctx, node);
	}

	if(SynArrayIndex *node = getType<SynArrayIndex>(syntax))
	{
		return AnalyzeArrayIndex(ctx, node);
	}

	if(SynFunctionCall *node = getType<SynFunctionCall>(syntax))
	{
		return AnalyzeFunctionCall(ctx, node);
	}

	if(SynNew *node = getType<SynNew>(syntax))
	{
		return AnalyzeNew(ctx, node);
	}

	if(SynFunctionDefinition *node = getType<SynFunctionDefinition>(syntax))
	{
		return AnalyzeFunctionDefinition(ctx, node);
	}

	Stop(ctx, syntax->pos, "ERROR: unknown expression type");

	return NULL;
}

ExprBase* AnalyzeStatement(ExpressionContext &ctx, SynBase *syntax)
{
	if(SynReturn *node = getType<SynReturn>(syntax))
	{
		return AnalyzeReturn(ctx, node);
	}

	if(SynYield *node = getType<SynYield>(syntax))
	{
		return AnalyzeYield(ctx, node);
	}

	if(SynVariableDefinitions *node = getType<SynVariableDefinitions>(syntax))
	{
		return AnalyzeVariableDefinitions(ctx, node);
	}

	if(SynFunctionDefinition *node = getType<SynFunctionDefinition>(syntax))
	{
		return AnalyzeFunctionDefinition(ctx, node);
	}

	if(SynClassDefinition *node = getType<SynClassDefinition>(syntax))
	{
		IntrusiveList<TypeHandle> generics;

		return AnalyzeClassDefinition(ctx, node, NULL, generics);
	}

	if(SynNamespaceDefinition *node = getType<SynNamespaceDefinition>(syntax))
	{
		return AnalyzeNamespaceDefinition(ctx, node);
	}

	if(SynTypedef *node = getType<SynTypedef>(syntax))
	{
		return AnalyzeTypedef(ctx, node);
	}

	if(SynIfElse *node = getType<SynIfElse>(syntax))
	{
		return AnalyzeIfElse(ctx, node);
	}

	if(SynFor *node = getType<SynFor>(syntax))
	{
		return AnalyzeFor(ctx, node);
	}

	if(SynWhile *node = getType<SynWhile>(syntax))
	{
		return AnalyzeWhile(ctx, node);
	}

	if(SynDoWhile *node = getType<SynDoWhile>(syntax))
	{
		return AnalyzeDoWhile(ctx, node);
	}

	if(SynBlock *node = getType<SynBlock>(syntax))
	{
		return AnalyzeBlock(ctx, node, true);
	}

	return AnalyzeExpression(ctx, syntax);
}

struct ModuleContext
{
	ModuleContext(): bytecode(NULL), name(NULL)
	{
	}

	const char* bytecode;
	const char *name;

	FastVector<TypeBase*, true, true> types;
};

void ImportModuleNamespaces(ExpressionContext &ctx, SynBase *source, ModuleContext &module)
{
	ByteCode *bCode = (ByteCode*)module.bytecode;
	char *symbols = FindSymbols(bCode);

	// Import namespaces
	ExternNamespaceInfo *namespaceList = FindFirstNamespace(bCode);

	for(unsigned i = 0; i < bCode->namespaceCount; i++)
	{
		ExternNamespaceInfo &ns = namespaceList[i];

		NamespaceData *parent = NULL;

		if(ns.parentHash != ~0u)
		{
			for(unsigned k = 0; k < ctx.namespaces.size(); k++)
			{
				if(ctx.namespaces[k]->nameHash == ns.parentHash)
				{
					parent = ctx.namespaces[k];
					break;
				}
			}

			if(!parent)
				Stop(ctx, source->pos, "ERROR: namespace %s parent not found", symbols + ns.offsetToName);
		}

		if(parent)
		{
			Stop(ctx, source->pos, "ERROR: can't import nested namespace");
		}
		else
		{
			ctx.namespaces.push_back(new NamespaceData(source, ctx.scopes.back(), ctx.GetCurrentNamespace(), InplaceStr(symbols + ns.offsetToName), ctx.uniqueNamespaceId++));
		}
	}
}

void ImportModuleTypes(ExpressionContext &ctx, SynBase *source, ModuleContext &module)
{
	ByteCode *bCode = (ByteCode*)module.bytecode;
	char *symbols = FindSymbols(bCode);

	// Import types
	ExternTypeInfo *typeList = FindFirstType(bCode);
	ExternMemberInfo *memberList = (ExternMemberInfo*)(typeList + bCode->typeCount);
	ExternConstantInfo *constantList = FindFirstConstant(bCode);

	module.types.resize(bCode->typeCount);

	for(unsigned i = 0; i < bCode->typeCount; i++)
	{
		ExternTypeInfo &type = typeList[i];

		// Skip existing types
		if(TypeBase **prev = ctx.typeMap.find(type.nameHash))
		{
			module.types[i] = *prev;
			continue;
		}

		switch(type.subCat)
		{
		case ExternTypeInfo::CAT_NONE:
			if(strcmp(symbols + type.offsetToName, "generic") == 0)
			{
				// TODO: after generic type clean-up we should have this type as a real one
				module.types[i] = new TypeGeneric(InplaceStr("generic"));
			}
			else
			{
				Stop(ctx, source->pos, "ERROR: new type in module %s named %s unsupported", module.name, symbols + type.offsetToName);
			}
			break;
		case ExternTypeInfo::CAT_ARRAY:
			if(TypeBase *subType = module.types[type.subType])
			{
				if(type.arrSize == ~0u)
					module.types[i] = ctx.GetUnsizedArrayType(subType);
				else
					module.types[i] = ctx.GetArrayType(subType, type.arrSize);
			}
			else
			{
				Stop(ctx, source->pos, "ERROR: can't find sub type for '%s' in module %s", symbols + type.offsetToName, module.name);
			}
			break;
		case ExternTypeInfo::CAT_POINTER:
			if(TypeBase *subType = module.types[type.subType])
			{
				module.types[i] = ctx.GetReferenceType(subType);
			}
			else
			{
				Stop(ctx, source->pos, "ERROR: can't find sub type for '%s' in module %s", symbols + type.offsetToName, module.name);
			}
			break;
		case ExternTypeInfo::CAT_FUNCTION:
			if(TypeBase *returnType = module.types[memberList[type.memberOffset].type])
			{
				IntrusiveList<TypeHandle> arguments;

				for(unsigned n = 0; n < type.memberCount; n++)
				{
					TypeBase *argType = module.types[memberList[type.memberOffset + n + 1].type];

					if(!argType)
						Stop(ctx, source->pos, "ERROR: can't find argument %d type for '%s' in module %s", n + 1, symbols + type.offsetToName, module.name);

					arguments.push_back(new TypeHandle(argType));
				}

				module.types[i] = ctx.GetFunctionType(returnType, arguments);
			}
			else
			{
				Stop(ctx, source->pos, "ERROR: can't find return type for '%s' in module %s", symbols + type.offsetToName, module.name);
			}
			break;
		case ExternTypeInfo::CAT_CLASS:
			{
				if(type.namespaceHash != ~0u)
					Stop(ctx, source->pos, "ERROR: can't import namespace type");

				if(type.constantCount != 0)
					Stop(ctx, source->pos, "ERROR: can't import constants of type");

				if(type.definitionOffset != ~0u)
				{
					if(type.definitionOffset & 0x80000000)
						Stop(ctx, source->pos, "ERROR: can't import derived type");
					else
						Stop(ctx, source->pos, "ERROR: can't import generic base type");
				}

				InplaceStr className = InplaceStr(symbols + type.offsetToName);

				IntrusiveList<SynIdentifier> aliases;
				IntrusiveList<TypeHandle> generics;

				TypeClass *classType = new TypeClass(ctx.scopes.back(), className, aliases, generics, false, NULL);

				classType->alignment = type.defaultAlign;
				classType->size = type.size;

				ctx.AddType(classType);

				module.types[i] = classType;

				const char *memberNames = className.end + 1;

				ctx.PushScope(classType);

				for(unsigned n = 0; n < type.memberCount; n++)
				{
					InplaceStr memberName = InplaceStr(memberNames);
					memberNames = memberName.end + 1;

					TypeBase *memberType = module.types[memberList[type.memberOffset + n].type];

					if(!memberType)
						Stop(ctx, source->pos, "ERROR: can't find member %d type for '%s' in module %s", n + 1, symbols + type.offsetToName, module.name);

					VariableData *member = new VariableData(source, ctx.scopes.back(), 0, memberType, memberName, memberList[type.memberOffset + n].offset, ctx.uniqueVariableId++);

					classType->members.push_back(new VariableHandle(member));
				}

				ctx.PopScope();
			}
			break;
		default:
			Stop(ctx, source->pos, "ERROR: new type in module %s named %s unsupported", module.name, symbols + type.offsetToName);
		}
	}
}

void ImportModuleVariables(ExpressionContext &ctx, SynBase *source, ModuleContext &module)
{
	ByteCode *bCode = (ByteCode*)module.bytecode;
	char *symbols = FindSymbols(bCode);

	// Import variables
	ExternVarInfo *variableList = FindFirstVar(bCode);

	for(unsigned i = 0; i < bCode->variableExportCount; i++)
	{
		ExternVarInfo &variable = variableList[i];

		InplaceStr name = InplaceStr(symbols + variable.offsetToName);

		// Exclude temporary variables from import
		if(name == InplaceStr("$temp"))
			continue;

		TypeBase *type = module.types[variable.type];

		if(!type)
			Stop(ctx, source->pos, "ERROR: can't find variable '%s' type in module %s", symbols + variable.offsetToName, module.name);

		VariableData *data = new VariableData(source, ctx.scopes.back(), 0, type, name, variable.offset, ctx.uniqueVariableId++);

		ctx.AddVariable(data);
	}
}

void ImportModuleTypedefs(ExpressionContext &ctx, SynBase *source, ModuleContext &module)
{
	ByteCode *bCode = (ByteCode*)module.bytecode;
	char *symbols = FindSymbols(bCode);

	// Import type aliases
	ExternTypedefInfo *aliasList = FindFirstTypedef(bCode);

	for(unsigned i = 0; i < bCode->typedefCount; i++)
	{
		ExternTypedefInfo &alias = aliasList[i];

		InplaceStr aliasName = InplaceStr(symbols + alias.offsetToName);

		TypeBase *targetType = module.types[alias.targetType];

		if(!targetType)
			Stop(ctx, source->pos, "ERROR: can't find alias '%s' target type in module %s", symbols + alias.offsetToName, module.name);

		if(TypeBase **prev = ctx.typeMap.find(aliasName.hash()))
		{
			TypeBase *type = *prev;

			if(type->name == aliasName)
				Stop(ctx, source->pos, "ERROR: type '%.*s' alias '%s' is equal to previously imported class", FMT_ISTR(targetType->name), symbols + alias.offsetToName);

			if(type != targetType)
				Stop(ctx, source->pos, "ERROR: type '%.*s' alias '%s' is equal to previously imported alias", FMT_ISTR(targetType->name), symbols + alias.offsetToName);
		}
		else if(alias.parentType != ~0u)
		{
			Stop(ctx, source->pos, "ERROR: can't import class alias");
		}
		else
		{
			AliasData *alias = new AliasData(source, ctx.scopes.back(), targetType, aliasName, ctx.uniqueAliasId++);

			ctx.AddAlias(alias);
		}
	}
}

void ImportModuleFunctions(ExpressionContext &ctx, SynBase *source, ModuleContext &module)
{
	ByteCode *bCode = (ByteCode*)module.bytecode;
	char *symbols = FindSymbols(bCode);

	// Import functions
	ExternFuncInfo *functionList = FindFirstFunc(bCode);
	ExternLocalInfo *localList = FindFirstLocal(bCode);

	for(unsigned i = 0; i < bCode->functionCount - bCode->moduleFunctionCount; i++)
	{
		ExternFuncInfo &function = functionList[i];

		TypeBase *functionType = module.types[function.funcType];

		if(!functionType)
			Stop(ctx, source->pos, "ERROR: can't find function '%s' type in module %s", symbols + function.offsetToName, module.name);

		if(function.namespaceHash != ~0u)
			Stop(ctx, source->pos, "ERROR: can't import namespace function");

		TypeBase *parentType = NULL;

		if(function.parentType != ~0u)
		{
			parentType = module.types[function.parentType];

			if(!parentType)
				Stop(ctx, source->pos, "ERROR: can't find function '%s' parent type in module %s", symbols + function.offsetToName, module.name);
		}

		TypeBase *contextType = NULL;

		if(function.contextType != ~0u)
		{
			contextType = module.types[function.contextType];

			if(!contextType)
				Stop(ctx, source->pos, "ERROR: can't find function '%s' context type in module %s", symbols + function.offsetToName, module.name);
		}

		if(function.explicitTypeCount != 0)
			Stop(ctx, source->pos, "ERROR: can't import generic function with explicit arguments");

		bool coroutine = function.funcCat == ExternFuncInfo::COROUTINE;

		InplaceStr functionName = InplaceStr(symbols + function.offsetToName);

		FunctionData *data = new FunctionData(source, ctx.scopes.back(), coroutine, getType<TypeFunction>(functionType), functionName, ctx.uniqueFunctionId++);

		data->isExternal = true;

		ctx.AddFunction(data);

		if(parentType)
			ctx.PushScope(parentType);

		ctx.PushScope(data);

		if(parentType)
		{
			TypeBase *type = ctx.GetReferenceType(parentType);

			unsigned offset = AllocateVariableInScope(ctx.scopes.back(), 0, type);
			VariableData *variable = new VariableData(source, ctx.scopes.back(), 0, type, InplaceStr("this"), offset, ctx.uniqueVariableId++);

			ctx.AddVariable(variable);
		}

		for(unsigned n = 0; n < function.paramCount; n++)
		{
			ExternLocalInfo &argument = localList[function.offsetToFirstLocal + n];

			TypeBase *argType = module.types[argument.type];

			if(!argType)
				Stop(ctx, source->pos, "ERROR: can't find argument %d type for '%s' in module %s", n + 1, symbols + function.offsetToName, module.name);

			InplaceStr argName = InplaceStr(symbols + argument.offsetToName);

			unsigned offset = AllocateVariableInScope(ctx.scopes.back(), 0, argType);
			VariableData *variable = new VariableData(source, ctx.scopes.back(), 0, argType, argName, offset, ctx.uniqueVariableId++);

			ctx.AddVariable(variable);
		}

		if(function.funcType == 0)
		{
			TypeBase *returnType = ctx.typeAuto;

			if(function.genericReturnType != ~0u)
				returnType = module.types[function.genericReturnType];

			if(!returnType)
				Stop(ctx, source->pos, "ERROR: can't find generic function '%s' return type in module %s", symbols + function.offsetToName, module.name);

			IntrusiveList<TypeHandle> argTypes;

			for(unsigned n = 0; n < function.paramCount; n++)
			{
				ExternLocalInfo &argument = localList[function.offsetToFirstLocal + n];

				argTypes.push_back(new TypeHandle(module.types[argument.type]));
			}

			data->type = ctx.GetFunctionType(returnType, argTypes);
		}

		assert(data->type);

		ctx.PopScope();

		if(parentType)
			ctx.PopScope();
	}
}

void ImportModule(ExpressionContext &ctx, SynBase *source, const char* bytecode, const char* name)
{
	ModuleContext module;

	module.bytecode = bytecode;
	module.name = name;

	ImportModuleNamespaces(ctx, source, module);

	ImportModuleTypes(ctx, source, module);

	ImportModuleVariables(ctx, source, module);

	ImportModuleTypedefs(ctx, source, module);

	ImportModuleFunctions(ctx, source, module);
}

void AnalyzeModuleImport(ExpressionContext &ctx, SynModuleImport *syntax)
{
	const char *importPath = BinaryCache::GetImportPath();

	unsigned pathLength = (importPath ? strlen(importPath) : 0) + syntax->path.size() - 1 + strlen(".nc");

	for(SynIdentifier *part = syntax->path.head; part; part = getType<SynIdentifier>(part->next))
		pathLength += part->name.length();

	char *path = new char[pathLength + 1];
	char *pathNoImport = importPath ? path + strlen(importPath) : path;

	char *pos = path;

	if(importPath)
	{
		strcpy(pos, importPath);
		pos += strlen(importPath);
	}

	for(SynIdentifier *part = syntax->path.head; part; part = getType<SynIdentifier>(part->next))
	{
		sprintf(pos, "%.*s", FMT_ISTR(part->name));
		pos += part->name.length();

		if(part->next)
			*pos++ = '/';
	}

	strcpy(pos, ".nc");
	pos += strlen(".nc");

	*pos = 0;

	if(const char *bytecode = BinaryCache::GetBytecode(path))
		ImportModule(ctx, syntax, bytecode, pathNoImport);
	else if(const char *bytecode = BinaryCache::GetBytecode(pathNoImport))
		ImportModule(ctx, syntax, bytecode, pathNoImport);
	else
		Stop(ctx, syntax->pos, "ERROR: module import is not implemented");
}

ExprBase* AnalyzeModule(ExpressionContext &ctx, SynBase *syntax)
{
	if(const char *bytecode = BinaryCache::GetBytecode("$base$.nc"))
		ImportModule(ctx, syntax, bytecode, "$base$.nc");
	else
		Stop(ctx, syntax->pos, "ERROR: base module couldn't be imported");

	if(SynModule *node = getType<SynModule>(syntax))
	{
		for(SynModuleImport *import = node->imports.head; import; import = getType<SynModuleImport>(import->next))
			AnalyzeModuleImport(ctx, import);

		IntrusiveList<ExprBase> expressions;

		for(SynBase *expr = node->expressions.head; expr; expr = expr->next)
			expressions.push_back(AnalyzeStatement(ctx, expr));

		return new ExprModule(syntax, ctx.typeVoid, expressions);
	}

	return NULL;
}

ExprBase* Analyze(ExpressionContext &ctx, SynBase *syntax)
{
	ctx.PushScope();
	ctx.globalScope = ctx.scopes.back();

	ctx.AddType(ctx.typeVoid = new TypeVoid(InplaceStr("void")));

	ctx.AddType(ctx.typeBool = new TypeBool(InplaceStr("bool")));

	ctx.AddType(ctx.typeChar = new TypeChar(InplaceStr("char")));
	ctx.AddType(ctx.typeShort = new TypeShort(InplaceStr("short")));
	ctx.AddType(ctx.typeInt = new TypeInt(InplaceStr("int")));
	ctx.AddType(ctx.typeLong = new TypeLong(InplaceStr("long")));

	ctx.AddType(ctx.typeFloat = new TypeFloat(InplaceStr("float")));
	ctx.AddType(ctx.typeDouble = new TypeDouble(InplaceStr("double")));

	ctx.AddType(ctx.typeTypeID = new TypeTypeID(InplaceStr("typeid")));
	ctx.AddType(ctx.typeFunctionID = new TypeFunctionID(InplaceStr("__function")));

	ctx.AddType(ctx.typeAuto = new TypeAuto(InplaceStr("auto")));

	ctx.AddType(ctx.typeAutoRef = new TypeAutoRef(InplaceStr("auto ref")));
	ctx.PushScope(ctx.typeAutoRef);
	ctx.typeAutoRef->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scopes.back(), ctx.typeTypeID, InplaceStr("type"), ctx.uniqueVariableId++)));
	ctx.typeAutoRef->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scopes.back(), ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"), ctx.uniqueVariableId++)));
	ctx.PopScope();

	ctx.AddType(ctx.typeAutoArray = new TypeAutoArray(InplaceStr("auto[]")));
	ctx.PushScope(ctx.typeAutoArray);
	ctx.typeAutoArray->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scopes.back(), ctx.typeTypeID, InplaceStr("type"), ctx.uniqueVariableId++)));
	ctx.typeAutoArray->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scopes.back(), ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"), ctx.uniqueVariableId++)));
	ctx.typeAutoArray->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scopes.back(), ctx.typeInt, InplaceStr("size"), ctx.uniqueVariableId++)));
	ctx.PopScope();

	// Analyze module
	if(!setjmp(errorHandler))
	{
		ExprBase *module = AnalyzeModule(ctx, syntax);

		ctx.PopScope();

		return module;
	}

	return NULL;
}
