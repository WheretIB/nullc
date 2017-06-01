#include "ExpressionTree.h"

#include "BinaryCache.h"
#include "Bytecode.h"
#include "ExpressionEval.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

namespace
{
	void Stop(ExpressionContext &ctx, const char *pos, const char *msg, va_list args)
	{
		ctx.errorPos = pos;

		if(ctx.errorBuf && ctx.errorBufSize)
		{
			vsnprintf(ctx.errorBuf, ctx.errorBufSize, msg, args);
			ctx.errorBuf[ctx.errorBufSize - 1] = '\0';
		}

		longjmp(ctx.errorHandler, 1);
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

		unsigned digit;
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
		unsigned digit;
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

	SynBinaryOpType GetBinaryOpType(SynModifyAssignType type)
	{
		switch(type)
		{
		case SYN_MODIFY_ASSIGN_ADD:
			return SYN_BINARY_OP_ADD;
		case SYN_MODIFY_ASSIGN_SUB:
			return SYN_BINARY_OP_SUB;
		case SYN_MODIFY_ASSIGN_MUL:
			return SYN_BINARY_OP_MUL;
		case SYN_MODIFY_ASSIGN_DIV:
			return SYN_BINARY_OP_DIV;
		case SYN_MODIFY_ASSIGN_POW:
			return SYN_BINARY_OP_POW;
		case SYN_MODIFY_ASSIGN_MOD:
			return SYN_BINARY_OP_MOD;
		case SYN_MODIFY_ASSIGN_SHL:
			return SYN_BINARY_OP_SHL;
		case SYN_MODIFY_ASSIGN_SHR:
			return SYN_BINARY_OP_SHR;
		case SYN_MODIFY_ASSIGN_BIT_AND:
			return SYN_BINARY_OP_BIT_AND;
		case SYN_MODIFY_ASSIGN_BIT_OR:
			return SYN_BINARY_OP_BIT_OR;
		case SYN_MODIFY_ASSIGN_BIT_XOR:
			return SYN_BINARY_OP_BIT_XOR;
		}

		return SYN_BINARY_OP_UNKNOWN;
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

		FastVector<FunctionData*> &functions = ctx.scope->functions;

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

	bool IsDerivedFrom(TypeClass *type, TypeClass *target)
	{
		while(type)
		{
			if(target == type)
				return true;

			type = type->baseClass;
		}

		return false;
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
	typeNullPtr = NULL;

	typeAuto = NULL;
	typeAutoRef = NULL;
	typeAutoArray = NULL;

	typeMap.init();
	functionMap.init();
	variableMap.init();

	scope = NULL;

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
	unsigned depth = scope ? scope->depth + 1 : 0;

	scope = new ScopeData(depth, scope);
}

void ExpressionContext::PushScope(NamespaceData *nameSpace)
{
	unsigned depth = scope ? scope->depth + 1 : 0;

	scope = new ScopeData(depth, scope, nameSpace);
}

void ExpressionContext::PushScope(FunctionData *function)
{
	unsigned depth = scope ? scope->depth + 1 : 0;

	scope = new ScopeData(depth, scope, function);
}

void ExpressionContext::PushScope(TypeBase *type)
{
	unsigned depth = scope ? scope->depth + 1 : 0;

	scope = new ScopeData(depth, scope, type);
}

void ExpressionContext::PopScope(SynBase *location)
{
	// When namespace scope ends, all the contents remain accessible through an outer namespace/global scope
	if(!location && scope->ownerNamespace)
	{
		ScopeData *adopter = scope->scope;

		while(!adopter->ownerNamespace && adopter->scope)
			adopter = adopter->scope;

		adopter->variables.push_back(scope->variables.data, scope->variables.size());
		adopter->functions.push_back(scope->functions.data, scope->functions.size());
		adopter->types.push_back(scope->types.data, scope->types.size());
		adopter->aliases.push_back(scope->aliases.data, scope->aliases.size());

		scope->variables.clear();
		scope->functions.clear();
		scope->types.clear();
		scope->aliases.clear();

		scope = scope->scope;
		return;
	}

	// Remove scope members from lookup maps
	for(int i = int(scope->variables.size()) - 1; i >= 0; i--)
	{
		VariableData *variable = scope->variables[i];

		if(variableMap.find(variable->nameHash, variable))
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

		if(functionMap.find(function->nameHash, function))
			functionMap.remove(function->nameHash, function);
	}

	for(int i = int(scope->types.size()) - 1; i >= 0; i--)
	{
		TypeBase *type = scope->types[i];

		if(typeMap.find(type->nameHash, type))
			typeMap.remove(type->nameHash, type);
	}

	for(int i = int(scope->aliases.size()) - 1; i >= 0; i--)
	{
		AliasData *alias = scope->aliases[i];

		if(typeMap.find(alias->nameHash, alias->type))
			typeMap.remove(alias->nameHash, alias->type);
	}

	scope = scope->scope;
}

void ExpressionContext::RestoreScopesAtPoint(ScopeData *target, SynBase *location)
{
	// Restore parent first, up to the current scope
	if(target->scope != scope)
		RestoreScopesAtPoint(target->scope, location);

	for(unsigned i = 0; i < target->variables.size(); i++)
	{
		VariableData *variable = target->variables[i];

		if(!location || variable->source->pos <= location->pos)
			variableMap.insert(variable->nameHash, variable);
	}

	for(unsigned i = 0; i < target->functions.size(); i++)
	{
		FunctionData *function = target->functions[i];

		// Class functions are kept visible, no need to add again
		if(function->scope->ownerType)
			continue;

		if(!location || function->source->pos <= location->pos)
			functionMap.insert(function->nameHash, function);
	}

	for(unsigned i = 0; i < target->types.size(); i++)
	{
		TypeBase *type = target->types[i];

		if(TypeClass *exact = getType<TypeClass>(type))
		{
			if(!location || exact->source->pos <= location->pos)
				typeMap.insert(type->nameHash, type);
		}
		else if(TypeGenericClassProto *exact = getType<TypeGenericClassProto>(type))
		{
			if(!location || exact->definition->pos <= location->pos)
				typeMap.insert(type->nameHash, type);
		}
		else
		{
			typeMap.insert(type->nameHash, type);
		}
	}

	for(unsigned i = 0; i < target->aliases.size(); i++)
	{
		AliasData *alias = target->aliases[i];

		if(!location || alias->source->pos <= location->pos)
			typeMap.insert(alias->nameHash, alias->type);
	}

	scope = target;
}

void ExpressionContext::SwitchToScopeAtPoint(SynBase *currLocation, ScopeData *target, SynBase *targetLocation)
{
	// Reach the same depth
	while(scope->depth > target->depth)
		PopScope();

	// Reach the same parent
	ScopeData *curr = target;

	while(curr->depth > scope->depth)
		curr = curr->scope;

	while(scope->scope != curr->scope)
	{
		PopScope();

		curr = curr->scope;
	}

	// When the common parent is reached, remove it without ejecting namespace variables into the outer scope
	PopScope(currLocation);

	// Now restore each namespace data up to the source location
	RestoreScopesAtPoint(target, targetLocation);
}

NamespaceData* ExpressionContext::GetCurrentNamespace()
{
	// Simply walk up the scopes and find the current one
	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if(NamespaceData *ns = curr->ownerNamespace)
			return ns;
	}

	return NULL;
}

FunctionData* ExpressionContext::GetCurrentFunction()
{
	// Walk up, but if we reach a type owner, stop - we're not in a context of a function
	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if(curr->ownerType)
			return NULL;

		if(FunctionData *function = curr->ownerFunction)
			return function;
	}

	return NULL;
}

TypeBase* ExpressionContext::GetCurrentType()
{
	// Walk up, but if we reach a function, return it's owner
	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if(curr->ownerFunction)
			return curr->ownerFunction->scope->ownerType;

		if(TypeBase *type = curr->ownerType)
			return type;
	}

	return NULL;
}

unsigned ExpressionContext::GetGenericClassInstantiationDepth()
{
	unsigned depth = 0;

	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
		if(TypeClass *type = getType<TypeClass>(curr->ownerType))
		{
			if(!type->generics.empty())
				depth++;
		}
	}

	return depth;
}

void ExpressionContext::AddType(TypeBase *type)
{
	scope->types.push_back(type);

	if(!isType<TypeGenericClassProto>(type))
		assert(!type->isGeneric);

	types.push_back(type);
	typeMap.insert(type->nameHash, type);
}

void ExpressionContext::AddFunction(FunctionData *function)
{
	scope->functions.push_back(function);

	functions.push_back(function);
	functionMap.insert(function->nameHash, function);
}

void ExpressionContext::AddVariable(VariableData *variable)
{
	scope->variables.push_back(variable);

	variables.push_back(variable);
	variableMap.insert(variable->nameHash, variable);
}

void ExpressionContext::AddAlias(AliasData *alias)
{
	scope->aliases.push_back(alias);

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

TypeArray* ExpressionContext::GetArrayType(const char *pos, TypeBase* type, long long length)
{
	if(length <= 0)
		Stop(pos, "ERROR: array size can't be negative or zero");

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

	result->members.push_back(new VariableHandle(new VariableData(NULL, scope, 4, typeInt, InplaceStr("size"), NULLC_PTR_SIZE, uniqueVariableId++)));

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

ExprBase* CreateBinaryOp(ExpressionContext &ctx, SynBase *source, SynBinaryOpType op, ExprBase *lhs, ExprBase *rhs);

ExprBase* CreateVariableAccess(ExpressionContext &ctx, SynBase *source, IntrusiveList<SynIdentifier> path, InplaceStr name);

FunctionValue SelectBestFunction(ExpressionContext &ctx, const char *pos, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, bool allowFailure);
FunctionValue CreateGenericFunctionInstance(ExpressionContext &ctx, FunctionValue proto, SmallArray<ArgumentData, 32> &arguments);
void GetNodeFunctions(ExpressionContext &ctx, SynBase *source, ExprBase *function, SmallArray<FunctionValue, 32> &functions);
ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, bool allowFailure);
ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, ExprBase *arg1, bool allowFailure);
ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, ExprBase *arg1, ExprBase *arg2, bool allowFailure);
ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, SmallArray<ArgumentData, 32> &arguments, bool allowFailure);
ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<ArgumentData, 32> &arguments, bool allowFailure);
ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SynCallArgument *argumentHead, bool allowFailure);
ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, bool allowFailure);

FunctionValue GetFunctionForType(ExpressionContext &ctx, SynBase *source, ExprBase *value, TypeFunction *type)
{
	// Collect a set of available functions
	SmallArray<FunctionValue, 32> functions;

	GetNodeFunctions(ctx, source, value, functions);

	if(!functions.empty())
	{
		// Analyze arguments
		SmallArray<ArgumentData, 32> arguments;

		for(TypeHandle *curr = type->arguments.head; curr; curr = curr->next)
			arguments.push_back(ArgumentData(source, false, InplaceStr(), curr->type, NULL));

		FunctionValue bestOverload = SelectBestFunction(ctx, source->pos, functions, arguments, true);

		if(bestOverload)
		{
			FunctionData *function = bestOverload.function;

			TypeFunction *overloadType = getType<TypeFunction>(function->type);

			if(overloadType->isGeneric || (function->scope->ownerType && function->scope->ownerType->isGeneric))
				bestOverload = CreateGenericFunctionInstance(ctx, bestOverload, arguments);

			if(bestOverload)
			{
				if(type->returnType == ctx.typeAuto)
					type = ctx.GetFunctionType(bestOverload.function->type->returnType, type->arguments);

				if(bestOverload.function->type == type)
					return bestOverload;
			}
		}
	}

	return FunctionValue();
}

ExprBase* CreateCast(ExpressionContext &ctx, SynBase *source, ExprBase *value, TypeBase *type, bool isFunctionArgument)
{
	// When function is used as value, hide its visibility immediately after use
	if(ExprFunctionDefinition *definition = getType<ExprFunctionDefinition>(value))
	{
		if(definition->wasHidden)
		{
			ctx.HideFunction(definition->function);

			definition->wasHidden = true;
		}
	}

	if(value->type == type)
		return value;

	if(ctx.IsNumericType(value->type) && ctx.IsNumericType(type))
		return new ExprTypeCast(source, type, value, EXPR_CAST_NUMERICAL);

	if(type == ctx.typeBool)
	{
		if(isType<TypeRef>(value->type))
			return new ExprTypeCast(source, type, value, EXPR_CAST_PTR_TO_BOOL);

		if(isType<TypeUnsizedArray>(value->type))
			return new ExprTypeCast(source, type, value, EXPR_CAST_UNSIZED_TO_BOOL);

		if(isType<TypeFunction>(value->type))
			return new ExprTypeCast(source, type, value, EXPR_CAST_FUNCTION_TO_BOOL);
	}

	if(value->type == ctx.typeNullPtr)
	{
		// nullptr to type ref conversion
		if(isType<TypeRef>(type))
			return new ExprTypeCast(source, type, value, EXPR_CAST_NULL_TO_PTR);

		// nullptr to auto ref conversion
		if(type == ctx.typeAutoRef)
			return new ExprTypeCast(source, type, value, EXPR_CAST_NULL_TO_AUTO_PTR);

		// nullptr to type[] conversion
		if(isType<TypeUnsizedArray>(type))
			return new ExprTypeCast(source, type, value, EXPR_CAST_NULL_TO_UNSIZED);

		// nullptr to auto[] conversion
		if(type == ctx.typeAutoArray)
			return new ExprTypeCast(source, type, value, EXPR_CAST_NULL_TO_AUTO_ARRAY);

		// nullptr to function type conversion
		if(isType<TypeFunction>(type))
			return new ExprTypeCast(source, type, value, EXPR_CAST_NULL_TO_FUNCTION);
	}

	if(TypeUnsizedArray *target = getType<TypeUnsizedArray>(type))
	{
		// type[N] to type[] conversion
		if(TypeArray *valueType = getType<TypeArray>(value->type))
		{
			if(target->subType == valueType->subType)
			{
				if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
				{
					ExprBase *address = new ExprGetAddress(source, ctx.GetReferenceType(value->type), node->variable);

					return new ExprTypeCast(source, type, address, EXPR_CAST_ARRAY_PTR_TO_UNSIZED);
				}
				else if(ExprDereference *node = getType<ExprDereference>(value))
				{
					return new ExprTypeCast(source, type, node->value, EXPR_CAST_ARRAY_PTR_TO_UNSIZED);
				}

				return new ExprTypeCast(source, type, value, EXPR_CAST_ARRAY_TO_UNSIZED);
			}
		}
	}

	if(TypeRef *target = getType<TypeRef>(type))
	{
		if(TypeRef *valueType = getType<TypeRef>(value->type))
		{
			// type[N] ref to type[] ref conversion
			if(isType<TypeUnsizedArray>(target->subType) && isType<TypeArray>(valueType->subType))
			{
				TypeUnsizedArray *targetSub = getType<TypeUnsizedArray>(target->subType);
				TypeArray *sourceSub = getType<TypeArray>(valueType->subType);

				if(targetSub->subType == sourceSub->subType)
				{
					return new ExprTypeCast(source, type, value, EXPR_CAST_ARRAY_PTR_TO_UNSIZED_PTR);
				}
			}
		}
		else if(value->type == ctx.typeAutoRef)
		{
			return new ExprTypeCast(source, type, value, EXPR_CAST_AUTO_PTR_TO_PTR);
		}
		else if(isFunctionArgument)
		{
			// type to type ref conversion
			if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
			{
				ExprBase *address = new ExprGetAddress(source, ctx.GetReferenceType(value->type), node->variable);

				return address;
			}
			else if(ExprDereference *node = getType<ExprDereference>(value))
			{
				return node->value;
			}

			return new ExprTypeCast(source, type, value, EXPR_CAST_ANY_TO_PTR);
		}
	}

	if(type == ctx.typeAutoRef)
	{
		// type ref to auto ref conversion
		if(TypeRef *valueType = getType<TypeRef>(value->type))
			return new ExprTypeCast(source, type, value, EXPR_CAST_PTR_TO_AUTO_PTR);

		if(isFunctionArgument)
		{
			// type to auto ref conversion
			if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
			{
				ExprBase *address = new ExprGetAddress(source, ctx.GetReferenceType(value->type), node->variable);

				return new ExprTypeCast(source, type, address, EXPR_CAST_PTR_TO_AUTO_PTR);
			}
			else if(ExprDereference *node = getType<ExprDereference>(value))
			{
				return new ExprTypeCast(source, type, node->value, EXPR_CAST_PTR_TO_AUTO_PTR);
			}

			return new ExprTypeCast(source, type, CreateCast(ctx, source, value, ctx.GetReferenceType(value->type), true), EXPR_CAST_PTR_TO_AUTO_PTR);
		}
		else
		{
			// type to auto ref conversion (boxing)
			return CreateFunctionCall(ctx, source, InplaceStr("duplicate"), value, false);
		}
	}

	if(type == ctx.typeAutoArray)
	{
		// type[] to auto[] conversion
		if(TypeUnsizedArray *valueType = getType<TypeUnsizedArray>(value->type))
			return new ExprTypeCast(source, type, value, EXPR_CAST_UNSIZED_TO_AUTO_ARRAY);
		
		if(TypeArray *valueType = getType<TypeArray>(value->type))
		{
			ExprBase *unsized = CreateCast(ctx, source, value, ctx.GetUnsizedArrayType(valueType->subType), false);

			return CreateCast(ctx, source, unsized, type, false);
		}
	}

	if(TypeFunction *target = getType<TypeFunction>(type))
	{
		if(FunctionValue function = GetFunctionForType(ctx, source, value, target))
			return new ExprFunctionAccess(source, type, function.function, function.context);
	}

	if(value->type == ctx.typeAutoRef)
	{
		// auto ref to type (unboxing)
		if(!isType<TypeRef>(type))
		{
			ExprBase *ptr = CreateCast(ctx, source, value, ctx.GetReferenceType(type), false);

			return new ExprDereference(source, type, ptr);
		}
	}

	Stop(ctx, source->pos, "ERROR: can't convert '%.*s' to '%.*s'", FMT_ISTR(value->type->name), FMT_ISTR(type->name));

	return NULL;
}

ExprBase* CreateConditionCast(ExpressionContext &ctx, SynBase *source, ExprBase *value)
{
	if(!ctx.IsNumericType(value->type) && !isType<TypeRef>(value->type))
	{
		// TODO: function overload

		if(isType<TypeFunction>(value->type) || isType<TypeUnsizedArray>(value->type) || value->type == ctx.typeAutoRef)
		{
			ExprBase *nullPtr = new ExprNullptrLiteral(value->source, ctx.typeNullPtr);

			return CreateBinaryOp(ctx, source, SYN_BINARY_OP_NOT_EQUAL, value, nullPtr);
		}
		else
		{
			Stop(ctx, source->pos, "ERROR: condition type cannot be '%.*s' and function for conversion to bool is undefined", FMT_ISTR(value->type->name));
		}
	}

	return value;
}

ExprAssignment* CreateAssignment(ExpressionContext &ctx, SynBase *source, ExprBase *lhs, ExprBase *rhs)
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
		Stop(ctx, source->pos, "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(lhs->type->name));

	if(rhs->type == ctx.typeVoid)
		Stop(ctx, source->pos, "ERROR: cannot convert from void to %.*s", FMT_ISTR(rhs->type->name));

	if(lhs->type == ctx.typeVoid)
		Stop(ctx, source->pos, "ERROR: cannot convert from %.*s to void", FMT_ISTR(lhs->type->name));

	rhs = CreateCast(ctx, source, rhs, lhs->type, false);

	return new ExprAssignment(source, lhs->type, wrapped, rhs);
}

ExprBase* CreateBinaryOp(ExpressionContext &ctx, SynBase *source, SynBinaryOpType op, ExprBase *lhs, ExprBase *rhs)
{
	bool skipOverload = false;

	// Built-in comparisons
	if(op == SYN_BINARY_OP_EQUAL || op == SYN_BINARY_OP_NOT_EQUAL)
	{
		if(lhs->type != rhs->type)
		{
			if(lhs->type == ctx.typeNullPtr)
				lhs = CreateCast(ctx, source, lhs, rhs->type, false);

			if(rhs->type == ctx.typeNullPtr)
				rhs = CreateCast(ctx, source, rhs, lhs->type, false);
		}

		if(lhs->type == ctx.typeAutoRef && lhs->type == rhs->type)
		{
			return CreateFunctionCall(ctx, source, InplaceStr(op == SYN_BINARY_OP_EQUAL ? "__rcomp" : "__rncomp"), lhs, rhs, false);
		}

		if(isType<TypeFunction>(lhs->type) && lhs->type == rhs->type)
		{
			IntrusiveList<TypeHandle> types;
			types.push_back(new TypeHandle(ctx.typeInt));
			TypeBase *type = ctx.GetFunctionType(ctx.typeVoid, types);;

			lhs = new ExprTypeCast(lhs->source, type, lhs, EXPR_CAST_REINTERPRET);
			rhs = new ExprTypeCast(rhs->source, type, rhs, EXPR_CAST_REINTERPRET);

			return CreateFunctionCall(ctx, source, InplaceStr(op == SYN_BINARY_OP_EQUAL ? "__pcomp" : "__pncomp"), lhs, rhs, false);
		}

		if(isType<TypeUnsizedArray>(lhs->type) && lhs->type == rhs->type)
		{
			if(ExprBase *result = CreateFunctionCall(ctx, source, InplaceStr(GetOpName(op)), lhs, rhs, true))
				return result;

			return CreateFunctionCall(ctx, source, InplaceStr(op == SYN_BINARY_OP_EQUAL ? "__acomp" : "__ancomp"), lhs, rhs, false);
		}

		if(lhs->type == ctx.typeTypeID && rhs->type == ctx.typeTypeID)
			skipOverload = true;
	}

	// Promotion to bool for some types
	if(op == SYN_BINARY_OP_LOGICAL_AND || op == SYN_BINARY_OP_LOGICAL_OR || op == SYN_BINARY_OP_LOGICAL_XOR)
	{
		lhs = CreateConditionCast(ctx, lhs->source, lhs);
		rhs = CreateConditionCast(ctx, rhs->source, rhs);
	}

	if(!skipOverload)
	{
		if(ExprBase *result = CreateFunctionCall(ctx, source, InplaceStr(GetOpName(op)), lhs, rhs, true))
			return result;
	}

	// TODO: 'in' is a function call
	// TODO: && and || could have an operator overload where second argument is wrapped in a function for short-circuit evaluation

	bool ok = false;
	
	ok |= ctx.IsNumericType(lhs->type) && ctx.IsNumericType(rhs->type);
	ok |= lhs->type == ctx.typeTypeID && rhs->type == ctx.typeTypeID && (op == SYN_BINARY_OP_EQUAL || op == SYN_BINARY_OP_NOT_EQUAL);
	ok |= isType<TypeRef>(lhs->type) && lhs->type == rhs->type && (op == SYN_BINARY_OP_EQUAL || op == SYN_BINARY_OP_NOT_EQUAL);

	if(!ok)
		Stop(ctx, source->pos, "ERROR: binary operations between complex types are not supported yet");

	if(lhs->type == ctx.typeVoid)
		Stop(ctx, source->pos, "ERROR: first operand type is 'void'");

	if(rhs->type == ctx.typeVoid)
		Stop(ctx, source->pos, "ERROR: second operand type is 'void'");

	bool binaryOp = IsBinaryOp(op);
	bool comparisonOp = IsComparisonOp(op);
	bool logicalOp = IsLogicalOp(op);

	if(ctx.IsFloatingPointType(lhs->type) || ctx.IsFloatingPointType(rhs->type))
	{
		if(logicalOp || binaryOp)
			Stop(ctx, source->pos, "ERROR: operation %s is not supported on '%.*s' and '%.*s'", GetOpName(op), FMT_ISTR(lhs->type->name), FMT_ISTR(rhs->type->name));
	}

	if(logicalOp)
	{
		// Logical operations require both operands to be 'bool'
		lhs = CreateCast(ctx, source, lhs, ctx.typeBool, false);
		rhs = CreateCast(ctx, source, rhs, ctx.typeBool, false);
	}
	else if(ctx.IsNumericType(lhs->type) && ctx.IsNumericType(rhs->type))
	{
		// Numeric operations promote both operands to a common type
		TypeBase *commonType = ctx.GetBinaryOpResultType(lhs->type, rhs->type);

		lhs = CreateCast(ctx, source, lhs, commonType, false);
		rhs = CreateCast(ctx, source, rhs, commonType, false);
	}

	if(lhs->type != rhs->type)
		Stop(ctx, source->pos, "ERROR: operation %s is not supported on '%.*s' and '%.*s'", GetOpName(op), FMT_ISTR(lhs->type->name), FMT_ISTR(rhs->type->name));

	TypeBase *resultType = NULL;

	if(comparisonOp || logicalOp)
		resultType = ctx.typeBool;
	else
		resultType = lhs->type;

	return new ExprBinaryOp(source, resultType, op, lhs, rhs);
}

ExprBase* AnalyzeNumber(ExpressionContext &ctx, SynNumber *syntax);
ExprBase* AnalyzeExpression(ExpressionContext &ctx, SynBase *syntax);
ExprBase* AnalyzeStatement(ExpressionContext &ctx, SynBase *syntax);
ExprBlock* AnalyzeBlock(ExpressionContext &ctx, SynBlock *syntax, bool createScope);
ExprAliasDefinition* AnalyzeTypedef(ExpressionContext &ctx, SynTypedef *syntax);
ExprBase* AnalyzeClassDefinition(ExpressionContext &ctx, SynClassDefinition *syntax, TypeGenericClassProto *proto, IntrusiveList<TypeHandle> generics);
void AnalyzeClassElements(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassElements *syntax);
ExprBase* AnalyzeFunctionDefinition(ExpressionContext &ctx, SynFunctionDefinition *syntax, TypeFunction *instance, TypeBase *instanceParent, IntrusiveList<MatchData> aliases);

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

	if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(Evaluate(ctx, CreateCast(ctx, size, sizeValue, ctx.typeLong, false))))
		return ctx.GetArrayType(size->pos, type, number->value);

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
		
		if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(Evaluate(ctx, CreateCast(ctx, node, size, ctx.typeLong, false))))
			return ctx.GetArrayType(syntax->pos, type, number->value);

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

		for(ScopeData *nsScope = NamedOrGlobalScopeFrom(ctx.scope); nsScope; nsScope = NamedOrGlobalScopeFrom(nsScope->scope))
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

	if(ExprIntegerLiteral *alignValue = getType<ExprIntegerLiteral>(Evaluate(ctx, CreateCast(ctx, syntax, align, ctx.typeLong, false))))
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

	for(unsigned i = 0; i < value.length(); i++)
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

	return new ExprArray(syntax, ctx.GetArrayType(syntax->pos, subType, values.size()), values);
}

ExprBase* CreateFunctionAccess(ExpressionContext &ctx, SynBase *source, HashMap<FunctionData*>::Node *function, ExprBase *context)
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

		return new ExprFunctionOverloadSet(source, type, functions, context);
	}

	return new ExprFunctionAccess(source, function->value->type, function->value, context);
}

ExprBase* CreateVariableAccess(ExpressionContext &ctx, SynBase *source, IntrusiveList<SynIdentifier> path, InplaceStr name)
{
	VariableData **variable = NULL;

	for(ScopeData *nsScope = NamedOrGlobalScopeFrom(ctx.scope); nsScope; nsScope = NamedOrGlobalScopeFrom(nsScope->scope))
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
			ExprBase *thisAccess = CreateVariableAccess(ctx, source, IntrusiveList<SynIdentifier>(), InplaceStr("this"));

			if(!thisAccess)
				Stop(ctx, source->pos, "ERROR: 'this' variable is not available");

			// Member access only shifts an address, so we are left with a reference to get value from
			ExprMemberAccess *shift = new ExprMemberAccess(source, ctx.GetReferenceType(data->type), thisAccess, data);

			return new ExprDereference(source, data->type, shift);
		}

		// TODO: external variable lookup

		return new ExprVariableAccess(source, data->type, data);
	}

	HashMap<FunctionData*>::Node *function = NULL;
	
	if(path.empty())
	{
		if(TypeBase* type = FindNextTypeFromScope(ctx.scope))
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
	}

	if(!function)
	{
		for(ScopeData *nsScope = NamedOrGlobalScopeFrom(ctx.scope); nsScope; nsScope = NamedOrGlobalScopeFrom(nsScope->scope))
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

			if(TypeBase **type = ctx.typeMap.find(hash))
			{
				hash = StringHashContinue((*type)->nameHash, "::");

				hash = StringHashContinue(hash, name.begin, name.end);

				function = ctx.functionMap.first(hash);

				if(function)
					break;
			}
		}
	}

	if(function)
	{
		// TODO: function context

		return CreateFunctionAccess(ctx, source, function, NULL);
	}

	return NULL;
}

ExprBase* AnalyzeVariableAccess(ExpressionContext &ctx, SynIdentifier *syntax)
{
	ExprBase *value = CreateVariableAccess(ctx, syntax, IntrusiveList<SynIdentifier>(), syntax->name);

	if(!value)
		Stop(ctx, syntax->pos, "ERROR: unknown variable");

	return value;
}

ExprBase* AnalyzeVariableAccess(ExpressionContext &ctx, SynTypeSimple *syntax)
{
	ExprBase *value = CreateVariableAccess(ctx, syntax, syntax->path, syntax->name);

	if(!value)
		Stop(ctx, syntax->pos, "ERROR: unknown variable");

	return value;
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

	if(!ctx.IsNumericType(value->type))
		Stop(ctx, syntax->pos, "ERROR: %s is not supported on '%.*s'", (syntax->isIncrement ? "increment" : "decrement"), FMT_ISTR(value->type->name));

	return new ExprPostModify(syntax, value->type, wrapped, syntax->isIncrement);
}

ExprBase* AnalyzeUnaryOp(ExpressionContext &ctx, SynUnaryOp *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	if(ExprBase *result = CreateFunctionCall(ctx, syntax, InplaceStr(GetOpName(syntax->type)), value, true))
		return result;

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

ExprBase* AnalyzeBinaryOp(ExpressionContext &ctx, SynBinaryOp *syntax)
{
	ExprBase *lhs = AnalyzeExpression(ctx, syntax->lhs);
	ExprBase *rhs = AnalyzeExpression(ctx, syntax->rhs);

	return CreateBinaryOp(ctx, syntax, syntax->type, lhs, rhs);
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

	condition = CreateConditionCast(ctx, condition->source, condition);

	ExprBase *trueBlock = AnalyzeStatement(ctx, syntax->trueBlock);
	ExprBase *falseBlock = AnalyzeStatement(ctx, syntax->falseBlock);

	TypeBase *resultType = NULL;

	if(trueBlock->type == falseBlock->type)
	{
		resultType = trueBlock->type;
	}
	else if(ctx.IsNumericType(trueBlock->type) && ctx.IsNumericType(falseBlock->type))
	{
		resultType = ctx.GetBinaryOpResultType(trueBlock->type, falseBlock->type);

		trueBlock = CreateCast(ctx, syntax->trueBlock, trueBlock, resultType, false);
		falseBlock = CreateCast(ctx, syntax->falseBlock, falseBlock, resultType, false);
	}
	else
	{
		Stop(ctx, syntax->pos, "ERROR: Unknown common type");
	}

	return new ExprConditional(syntax, resultType, condition, trueBlock, falseBlock);
}

ExprBase* AnalyzeAssignment(ExpressionContext &ctx, SynAssignment *syntax)
{
	ExprBase *lhs = AnalyzeExpression(ctx, syntax->lhs);
	ExprBase *rhs = AnalyzeExpression(ctx, syntax->rhs);

	if(ExprBase *result = CreateFunctionCall(ctx, syntax, InplaceStr("="), lhs, rhs, true))
		return result;

	return CreateAssignment(ctx, syntax, lhs, rhs);
}

ExprBase* AnalyzeModifyAssignment(ExpressionContext &ctx, SynModifyAssignment *syntax)
{
	ExprBase *lhs = AnalyzeExpression(ctx, syntax->lhs);
	ExprBase *rhs = AnalyzeExpression(ctx, syntax->rhs);

	if(ExprBase *result = CreateFunctionCall(ctx, syntax, InplaceStr(GetOpName(syntax->type)), lhs, rhs, true))
		return result;

	return CreateAssignment(ctx, syntax, lhs, CreateBinaryOp(ctx, syntax, GetBinaryOpType(syntax->type), lhs, rhs));
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
			// Search for a member variable
			for(VariableHandle *el = node->members.head; el; el = el->next)
			{
				if(el->variable->name == syntax->member)
				{
					// Member access only shifts an address, so we are left with a reference to get value from
					ExprMemberAccess *shift = new ExprMemberAccess(syntax, ctx.GetReferenceType(el->variable->type), wrapped, el->variable);

					return new ExprDereference(syntax, el->variable->type, shift);
				}
			}

			// Look for a member function
			unsigned hash = StringHashContinue(node->nameHash, "::");

			hash = StringHashContinue(hash, syntax->member.begin, syntax->member.end);

			if(HashMap<FunctionData*>::Node *function = ctx.functionMap.first(hash))
				return CreateFunctionAccess(ctx, syntax, function, wrapped);

			// Look for a member function in a generic class base and instantiate them
			if(TypeClass *classType = getType<TypeClass>(node))
			{
				if(TypeGenericClassProto *protoType = classType->proto)
				{
					unsigned hash = StringHashContinue(protoType->nameHash, "::");

					hash = StringHashContinue(hash, syntax->member.begin, syntax->member.end);

					if(HashMap<FunctionData*>::Node *function = ctx.functionMap.first(hash))
						return CreateFunctionAccess(ctx, syntax, function, wrapped);
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
	bool findOverload = syntax->arguments.empty() || syntax->arguments.size() > 1;

	for(SynCallArgument *curr = syntax->arguments.head; curr; curr = getType<SynCallArgument>(curr->next))
	{
		if(!curr->name.empty())
			findOverload = true;
	}

	if(ExprBase *overloads = CreateVariableAccess(ctx, syntax, IntrusiveList<SynIdentifier>(), InplaceStr("[]")))
	{
		SynCallArgument *arg0 = new SynCallArgument(syntax->value->pos, InplaceStr(), syntax->value);

		// Link in value as the first argument
		arg0->next = syntax->arguments.head;

		if(ExprBase *result = CreateFunctionCall(ctx, syntax, overloads, arg0, !findOverload))
			return result;

		arg0->next = NULL;
	}

	if(findOverload)
		Stop(ctx, syntax->pos, "ERROR: overloaded '[]' operator is not available");

	SynCallArgument *argument = syntax->arguments.head;

	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	ExprBase *index = AnalyzeExpression(ctx, argument->value);

	index = CreateCast(ctx, syntax, index, ctx.typeInt, false);

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

bool HasNamedCallArguments(SmallArray<ArgumentData, 32> &arguments)
{
	for(unsigned i = 0; i < arguments.size(); i++)
	{
		if(!arguments[i].name.empty())
			return true;
	}

	return false;
}

bool HasMatchingArgumentNames(SmallArray<ArgumentData, 32> &functionArguments, SmallArray<ArgumentData, 32> &arguments)
{
	for(unsigned i = 0; i < arguments.size(); i++)
	{
		InplaceStr argumentName = arguments[i].name;

		if(argumentName.empty())
			continue;

		bool found = false;

		for(unsigned k = 0; k < functionArguments.size(); k++)
		{
			if(functionArguments[k].name == argumentName)
			{
				found = true;
				break;
			}
		}

		if(!found)
			return false;
	}

	return true;
}

bool PrepareArgumentsForFunctionCall(ExpressionContext &ctx, SmallArray<ArgumentData, 32> &functionArguments, SmallArray<ArgumentData, 32> &arguments, SmallArray<ArgumentData, 32> &result, bool prepareValues)
{
	result.clear();

	if(HasNamedCallArguments(arguments))
	{
		if(!HasMatchingArgumentNames(functionArguments, arguments))
			return false;

		// Add first unnamed arguments
		for(unsigned i = 0; i < arguments.size(); i++)
		{
			ArgumentData &argument = arguments[i];

			if(argument.name.empty())
				result.push_back(argument);
			else
				break;
		}

		unsigned unnamedCount = result.size();

		// Reserve slots for all remaining arguments
		for(unsigned i = unnamedCount; i < functionArguments.size(); i++)
			result.push_back(ArgumentData());

		// Put named arguments in appropriate slots
		for(unsigned i = unnamedCount; i < arguments.size(); i++)
		{
			ArgumentData &argument = arguments[i];

			unsigned targetPos = 0;

			for(unsigned k = 0; k < functionArguments.size(); k++)
			{
				if(functionArguments[k].name == argument.name)
				{
					if(result[targetPos].type != NULL)
						Stop(ctx, argument.value->source->pos, "ERROR: argument '%.*s' is already set", FMT_ISTR(argument.name));

					result[targetPos] = argument;
					break;
				}

				targetPos++;
			}
		}

		// Fill in any unset arguments with default values
		for(unsigned i = 0; i < functionArguments.size(); i++)
		{
			ArgumentData &argument = functionArguments[i];

			if(result[i].type == NULL && argument.value)
				result[i] = argument;
		}

		// All arguments must be set
		for(unsigned i = unnamedCount; i < arguments.size(); i++)
		{
			if(result[i].type == NULL)
				return false;
		}
	}
	else
	{
		// Add arguments
		result.push_back(arguments.data, arguments.size());

		// Add any arguments with default values
		for(unsigned i = result.size(); i < functionArguments.size(); i++)
		{
			ArgumentData &argument = functionArguments[i];

			if(argument.value)
				result.push_back(argument);
		}

		// Create variadic pack if neccessary
		TypeBase *varArgType = ctx.GetUnsizedArrayType(ctx.typeAutoRef);

		if(!functionArguments.empty() && functionArguments.back().type == varArgType && !functionArguments.back().isExplicit)
		{
			if(result.size() >= functionArguments.size() - 1 && !(result.size() == functionArguments.size() && result.back().type == varArgType))
			{
				ExprBase *value = NULL;

				if(prepareValues)
				{
					SynBase *source = result[0].value->source;

					IntrusiveList<ExprBase> values;

					for(unsigned i = functionArguments.size() - 1; i < result.size(); i++)
						values.push_back(CreateCast(ctx, result[i].value->source, result[i].value, ctx.typeAutoRef, true));

					if(values.empty())
						value = new ExprNullptrLiteral(source, ctx.typeNullPtr);
					else
						value = new ExprArray(source, ctx.GetArrayType(source->pos, ctx.typeAutoRef, values.size()), values);

					value = CreateCast(ctx, source, value, varArgType, true);
				}

				result.shrink(functionArguments.size() - 1);
				result.push_back(ArgumentData(NULL, false, functionArguments.back().name, varArgType, value));
			}
		}
	}

	if(result.size() != functionArguments.size())
		return false;

	// Convert all arguments to target type if this is a real call
	if(prepareValues)
	{
		for(unsigned i = 0; i < result.size(); i++)
		{
			ArgumentData &argument = result[i];

			assert(argument.value);

			TypeBase *target = functionArguments[i].type;

			argument.value = CreateCast(ctx, argument.value->source, argument.value, target, true);
		}
	}

	return true;
}

unsigned GetFunctionRating(ExpressionContext &ctx, FunctionData *function, TypeFunction *instance, SmallArray<ArgumentData, 32> &arguments)
{
	if(function->arguments.size() != arguments.size())
		return ~0u;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.

	unsigned rating = 0;

	unsigned i = 0;

	for(TypeHandle *argType = instance->arguments.head; argType; argType = argType->next, i++)
	{
		ArgumentData &expectedArgument = function->arguments[i];
		TypeBase *expectedType = argType->type;

		ArgumentData &actualArgument = arguments[i];
		TypeBase *actualType = actualArgument.type;

		if(expectedType != actualType)
		{
			if(actualType == ctx.typeNullPtr)
			{
				// nullptr is convertable to T ref, T[] and function pointers
				if(isType<TypeRef>(expectedType) || isType<TypeUnsizedArray>(expectedType) || isType<TypeFunction>(expectedType))
					continue;

				// nullptr is also convertable to auto ref and auto[], but it has the same rating as type ref -> auto ref and array -> auto[] defined below
				if(expectedType == ctx.typeAutoRef || expectedType == ctx.typeAutoArray)
				{
					rating += 5;
					continue;
				}
			}

			// Generic function argument
			if(expectedType->isGeneric)
				continue;

			if(expectedArgument.isExplicit)
			{
				if(TypeFunction *target = getType<TypeFunction>(expectedType))
				{
					if(actualArgument.value)
					{
						if(FunctionValue function = GetFunctionForType(ctx, actualArgument.value->source, actualArgument.value, target))
							continue;
					}
				}

				return ~0u;
			}

			// array -> class (unsized array)
			if(isType<TypeUnsizedArray>(expectedType) && isType<TypeArray>(actualType))
			{
				TypeUnsizedArray *lArray = getType<TypeUnsizedArray>(expectedType);
				TypeArray *rArray = getType<TypeArray>(actualType);

				if(lArray->subType == rArray->subType)
				{
					rating += 2;
					continue;
				}
			}

			// array -> auto[]
			if(expectedType == ctx.typeAutoArray && (isType<TypeArray>(actualType) || isType<TypeUnsizedArray>(actualType)))
			{
				rating += 5;
				continue;
			}

			// array[N] ref -> array[] -> array[] ref
			if(isType<TypeRef>(expectedType) && isType<TypeRef>(actualType))
			{
				TypeRef *lRef = getType<TypeRef>(expectedType);
				TypeRef *rRef = getType<TypeRef>(actualType);

				if(isType<TypeUnsizedArray>(lRef->subType) && isType<TypeArray>(rRef->subType))
				{
					TypeUnsizedArray *lArray = getType<TypeUnsizedArray>(lRef->subType);
					TypeArray *rArray = getType<TypeArray>(rRef->subType);

					if(lArray->subType == rArray->subType)
					{
						rating += 10;
						continue;
					}
				}
			}

			// derived ref -> base ref
			// base ref -> derived ref
			if(isType<TypeRef>(expectedType) && isType<TypeRef>(actualType))
			{
				TypeRef *lRef = getType<TypeRef>(expectedType);
				TypeRef *rRef = getType<TypeRef>(actualType);

				if(isType<TypeClass>(lRef->subType) && isType<TypeClass>(rRef->subType))
				{
					TypeClass *lClass = getType<TypeClass>(lRef->subType);
					TypeClass *rClass = getType<TypeClass>(rRef->subType);

					if(IsDerivedFrom(rClass, lClass))
					{
						rating += 5;
						continue;
					}

					if(IsDerivedFrom(lClass, rClass))
					{
						rating += 10;
						continue;
					}
				}
			}

			if(isType<TypeClass>(expectedType) && isType<TypeClass>(actualType))
			{
				TypeClass *lClass = getType<TypeClass>(expectedType);
				TypeClass *rClass = getType<TypeClass>(actualType);

				if(IsDerivedFrom(rClass, lClass))
				{
					rating += 5;
					continue;
				}
			}

			if(isType<TypeFunction>(expectedType))
			{
				TypeFunction *lFunction = getType<TypeFunction>(expectedType);

				if(actualArgument.value)
				{
					if(FunctionValue function = GetFunctionForType(ctx, actualArgument.value->source, actualArgument.value, lFunction))
						continue;
				}
				
				return ~0u;
			}

			// type -> type ref
			if(isType<TypeRef>(expectedType))
			{
				TypeRef *lRef = getType<TypeRef>(expectedType);

				if(lRef->subType == actualType)
				{
					rating += 5;
					continue;
				}
			}

			// type ref -> auto ref
			if(expectedType == ctx.typeAutoRef && isType<TypeRef>(actualType))
			{
				rating += 5;
				continue;
			}

			// type -> type ref -> auto ref
			if(expectedType == ctx.typeAutoRef)
			{
				rating += 10;
				continue;
			}

			// numeric -> numeric
			if(ctx.IsNumericType(expectedType) && ctx.IsNumericType(actualType))
			{
				rating += 1;
				continue;
			}

			return ~0u;
		}
	}

	return rating;
}

TypeBase* MatchGenericType(ExpressionContext &ctx, TypeBase *matchType, TypeBase *argType, IntrusiveList<MatchData> &aliases, bool strict)
{
	if(!matchType->isGeneric)
	{
		if(matchType == argType)
			return argType;

		return NULL;
	}

	// 'generic' match with 'type' results with 'type'
	if(TypeGeneric *lhs = getType<TypeGeneric>(matchType))
	{
		if(lhs->name == InplaceStr("generic"))
			return argType;

		for(MatchData *curr = aliases.head; curr; curr = curr->next)
		{
			if(curr->name == lhs->name)
				return curr->type;
		}

		aliases.push_back(new MatchData(lhs->name, argType));

		return argType;
	}

	if(TypeRef *lhs = getType<TypeRef>(matchType))
	{
		// 'generic ref' match with 'type ref' results with 'type ref'
		if(TypeRef *rhs = getType<TypeRef>(argType))
			return argType;

		if(strict)
			return NULL;

		// 'generic ref' match with 'type' results with 'type ref'
		return ctx.GetReferenceType(argType);
	}

	if(TypeArray *lhs = getType<TypeArray>(matchType))
	{
		// Only match with arrays of the same size
		if(TypeArray *rhs = getType<TypeArray>(argType))
		{
			if(lhs->size == rhs->size)
				return argType;
		}

		return NULL;
	}

	if(TypeUnsizedArray *lhs = getType<TypeUnsizedArray>(matchType))
	{
		// 'generic[]' match with 'type[]' results with 'type[]'
		if(TypeUnsizedArray *rhs = getType<TypeUnsizedArray>(argType))
			return argType;

		if(strict)
			return NULL;

		// 'generic[]' match with 'type[N]' results with 'type[]'
		if(TypeArray *rhs = getType<TypeArray>(argType))
			return ctx.GetUnsizedArrayType(rhs->subType);

		return NULL;
	}

	if(TypeFunction *lhs = getType<TypeFunction>(matchType))
	{
		// Only match with other function type
		if(TypeFunction *rhs = getType<TypeFunction>(argType))
		{
			TypeBase *returnType = MatchGenericType(ctx, lhs->returnType, rhs->returnType, aliases, true);

			if(!returnType)
				return NULL;

			IntrusiveList<TypeHandle> arguments;

			TypeHandle *lhsArg = lhs->arguments.head;
			TypeHandle *rhsArg = rhs->arguments.head;

			while(lhsArg && rhsArg)
			{
				TypeBase *argMatched = MatchGenericType(ctx, lhsArg->type, rhsArg->type, aliases, true);

				if(!argMatched)
					return NULL;

				arguments.push_back(new TypeHandle(argMatched));

				lhsArg = lhsArg->next;
				rhsArg = rhsArg->next;
			}

			// Different number of arguments
			if(lhsArg || rhsArg)
				return NULL;

			return ctx.GetFunctionType(returnType, arguments);
		}

		return NULL;
	}

	if(TypeGenericClass *lhs = getType<TypeGenericClass>(matchType))
	{
		// Match with a generic class instance
		if(TypeClass *rhs = getType<TypeClass>(argType))
		{
			if(lhs->proto != rhs->proto)
				return NULL;

			TypeHandle *lhsArg = lhs->generics.head;
			MatchData *rhsArg = rhs->generics.head;

			while(lhsArg && rhsArg)
			{
				TypeBase *argMatched = MatchGenericType(ctx, lhsArg->type, rhsArg->type, aliases, true);

				if(!argMatched)
					return NULL;

				lhsArg = lhsArg->next;
				rhsArg = rhsArg->next;
			}

			return argType;
		}

		return NULL;
	}

	Stop(ctx, NULL, "ERROR: unknown generic type match");

	return NULL;
}

TypeFunction* GetGenericFunctionInstanceType(ExpressionContext &ctx, FunctionData *function, SmallArray<ArgumentData, 32> &arguments, IntrusiveList<MatchData> &aliases)
{
	assert(function->arguments.size() == arguments.size());

	IntrusiveList<TypeHandle> types;

	for(unsigned i = 0; i < function->arguments.size(); i++)
	{
		TypeBase *type = MatchGenericType(ctx, function->arguments[i].type, arguments[i].type, aliases, false);

		if(!type)
			return NULL;

		types.push_back(new TypeHandle(type));
	}

	return ctx.GetFunctionType(function->type->returnType, types);
}

void StopOnFunctionSelectError(ExpressionContext &ctx, const char *pos, char* errPos, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, SmallArray<unsigned, 32> &ratings, unsigned bestRating, bool showInstanceInfo)
{
	assert(!functions.empty());

	errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "  %.*s(", FMT_ISTR(functions[0].function->name));

	for(unsigned i = 0; i < arguments.size(); i++)
		errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "%s%.*s", i != 0 ? ", " : "", FMT_ISTR(arguments[i].type->name));

	errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), ")\n");

	errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), bestRating == ~0u ? " the only available are:\n" : " candidates are:\n");

	for(unsigned i = 0; i < functions.size(); i++)
	{
		FunctionData *function = functions[i].function;

		if(ratings[i] != bestRating)
			continue;

		errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "  %.*s %.*s(", FMT_ISTR(function->type->returnType->name), FMT_ISTR(function->name));

		for(unsigned k = 0; k < function->arguments.size(); k++)
		{
			ArgumentData &argument = function->arguments[k];

			errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "%s%s%.*s", k != 0 ? ", " : "", argument.isExplicit ? "explicit " : "", FMT_ISTR(argument.type->name));
		}

		if(function->type->isGeneric && showInstanceInfo)
		{
			IntrusiveList<MatchData> aliases;
			SmallArray<ArgumentData, 32> result;

			// Handle named argument order, default argument values and variadic functions
			if(!PrepareArgumentsForFunctionCall(ctx, function->arguments, arguments, result, false))
			{
				errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), ") (wasn't instanced here");
			}
			else if(TypeFunction *instance = GetGenericFunctionInstanceType(ctx, function, arguments, aliases))
			{
				GetFunctionRating(ctx, function, instance, result);

				errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), ") instanced to\r\n    %.*s(", FMT_ISTR(function->name));

				TypeHandle *curr = instance->arguments.head;

				for(unsigned k = 0; k < function->arguments.size(); k++)
				{
					ArgumentData &argument = function->arguments[k];

					errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "%s%.*s", k != 0 ? ", " : "", argument.isExplicit ? "explicit " : "", FMT_ISTR(curr->type->name));

					curr = curr->next;
				}
			}
			else
			{
				errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), ") (wasn't instanced here");
			}
		}

		errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), ")\n");
	}

	ctx.errorPos = pos;

	longjmp(ctx.errorHandler, 1);
}

FunctionValue SelectBestFunction(ExpressionContext &ctx, const char *pos, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, bool allowFailure)
{
	SmallArray<unsigned, 32> ratings;

	ratings.resize(functions.size());

	unsigned bestRating = ~0u;
	FunctionValue bestFunction;

	unsigned bestGenericRating = ~0u;
	FunctionValue bestGenericFunction;
	
	for(unsigned i = 0; i < functions.size(); i++)
	{
		FunctionValue value = functions[i];

		FunctionData *function = value.function;

		SmallArray<ArgumentData, 32> result;

		// Handle named argument order, default argument values and variadic functions
		if(!PrepareArgumentsForFunctionCall(ctx, function->arguments, arguments, result, false))
		{
			ratings[i] = ~0u;
			continue;
		}

		ratings[i] = GetFunctionRating(ctx, function, function->type, result);

		if(ratings[i] == ~0u)
			continue;

		if(function->type->isGeneric || (function->scope->ownerType && function->scope->ownerType->isGeneric))
		{
			IntrusiveList<MatchData> aliases;
			TypeFunction *instance = GetGenericFunctionInstanceType(ctx, function, result, aliases);

			if(!instance)
			{
				ratings[i] = ~0u;
				continue;
			}
			
			ratings[i] = GetFunctionRating(ctx, function, instance, result);

			if(ratings[i] < bestGenericRating)
			{
				bestGenericRating = ratings[i];
				bestGenericFunction = value;
			}
		}
		else
		{
			if(ratings[i] < bestRating)
			{
				bestRating = ratings[i];
				bestFunction = value;
			}
		}
	}

	// Use generic function only if it is better that selected
	if(bestGenericRating < bestRating)
	{
		bestRating = bestGenericRating;
		bestFunction = bestGenericFunction;
	}
	else
	{
		// Hide all generic functions from selection
		for(unsigned i = 0; i < functions.size(); i++)
		{
			FunctionData *function = functions[i].function;

			if(function->type->isGeneric || (function->scope->ownerType && function->scope->ownerType->isGeneric))
				ratings[i] = ~0u;
		}
	}

	// Didn't find an appropriate function
	if(bestFunction == NULL)
	{
		assert(bestRating == ~0u);

		if(allowFailure)
			return FunctionValue();

		char *errPos = ctx.errorBuf;
		errPos += SafeSprintf(errPos, ctx.errorBufSize, "ERROR: can't find function with following parameters:\n");
		StopOnFunctionSelectError(ctx, pos, errPos, functions, arguments, ratings, bestRating, true);
	}

	// Check if multiple functions share the same rating
	for(unsigned i = 0; i < functions.size(); i++)
	{
		if(functions[i] != bestFunction && ratings[i] == bestRating)
		{
			char *errPos = ctx.errorBuf;
			errPos += SafeSprintf(errPos, ctx.errorBufSize, "ERROR: ambiguity, there is more than one overloaded function available for the call:\n");
			StopOnFunctionSelectError(ctx, pos, errPos, functions, arguments, ratings, bestRating, true);
		}
	}

	return bestFunction;
}

FunctionValue CreateGenericFunctionInstance(ExpressionContext &ctx, FunctionValue proto, SmallArray<ArgumentData, 32> &arguments)
{
	FunctionData *function = proto.function;

	SmallArray<ArgumentData, 32> result;

	if(!PrepareArgumentsForFunctionCall(ctx, function->arguments, arguments, result, false))
		assert(!"unexpected");

	IntrusiveList<MatchData> aliases;

	TypeFunction *instance = GetGenericFunctionInstanceType(ctx, function, result, aliases);

	assert(instance);

	// Switch to original function scope
	ScopeData *scope = ctx.scope;

	ctx.SwitchToScopeAtPoint(NULL, function->scope, function->source);

	SynFunctionDefinition *syntax = getType<SynFunctionDefinition>(function->source);

	if(!syntax)
		Stop(ctx, NULL, "ERROR: imported generic function call is not supported");

	TypeBase *instanceParent = NULL;

	if(syntax->parentType)
	{
		assert(function->scope->ownerType);
		assert(isType<TypeRef>(proto.context->type));

		if(TypeRef *type = getType<TypeRef>(proto.context->type))
			instanceParent = type->subType;
	}

	ExprBase *expr = AnalyzeFunctionDefinition(ctx, syntax, instance, instanceParent, aliases);

	ExprFunctionDefinition *definition = getType<ExprFunctionDefinition>(expr);

	assert(definition);

	function->instances.push_back(definition);

	// Restore old scope
	ctx.SwitchToScopeAtPoint(function->source, scope, NULL);

	return FunctionValue(definition->function, proto.context);
}

void GetNodeFunctions(ExpressionContext &ctx, SynBase *source, ExprBase *function, SmallArray<FunctionValue, 32> &functions)
{
	if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(function))
	{
		functions.push_back(FunctionValue(node->function, node->context));
	}
	else if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(function))
	{
		// TODO: function context

		functions.push_back(FunctionValue(node->function, NULL));
	}
	else if(ExprGenericFunctionPrototype *node = getType<ExprGenericFunctionPrototype>(function))
	{
		// TODO: function context

		functions.push_back(FunctionValue(node->function, NULL));
	}
	else if(ExprFunctionOverloadSet *node = getType<ExprFunctionOverloadSet>(function))
	{
		for(FunctionHandle *arg = node->functions.head; arg; arg = arg->next)
			functions.push_back(FunctionValue(arg->function, node->context));
	}
	else if(!isType<TypeFunction>(function->type))
	{
		Stop(ctx, source->pos, "ERROR: unknown call");
	}
}

ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, bool allowFailure)
{
	SmallArray<ArgumentData, 32> arguments;

	arguments.push_back(ArgumentData(arg0->source, false, InplaceStr(), arg0->type, arg0));

	return CreateFunctionCall(ctx, source, name, arguments, allowFailure);
}

ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, ExprBase *arg1, bool allowFailure)
{
	SmallArray<ArgumentData, 32> arguments;

	arguments.push_back(ArgumentData(arg0->source, false, InplaceStr(), arg0->type, arg0));
	arguments.push_back(ArgumentData(arg1->source, false, InplaceStr(), arg1->type, arg1));

	return CreateFunctionCall(ctx, source, name, arguments, allowFailure);
}

ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, ExprBase *arg1, ExprBase *arg2, bool allowFailure)
{
	SmallArray<ArgumentData, 32> arguments;

	arguments.push_back(ArgumentData(arg0->source, false, InplaceStr(), arg0->type, arg0));
	arguments.push_back(ArgumentData(arg1->source, false, InplaceStr(), arg1->type, arg1));
	arguments.push_back(ArgumentData(arg2->source, false, InplaceStr(), arg2->type, arg2));

	return CreateFunctionCall(ctx, source, name, arguments, allowFailure);
}

ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, SmallArray<ArgumentData, 32> &arguments, bool allowFailure)
{
	if(ExprBase *overloads = CreateVariableAccess(ctx, source, IntrusiveList<SynIdentifier>(), name))
	{
		if(ExprFunctionCall *result = CreateFunctionCall(ctx, source, overloads, arguments, allowFailure))
			return result;
	}

	if(!allowFailure)
		Stop(ctx, source->pos, "ERROR: unknown identifier '%.*s'", FMT_ISTR(name));

	return NULL;
}

ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<ArgumentData, 32> &arguments, bool allowFailure)
{
	// Collect a set of available functions
	SmallArray<FunctionValue, 32> functions;

	GetNodeFunctions(ctx, source, value, functions);

	return CreateFunctionCall(ctx, source, value, functions, arguments, allowFailure);
}

ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SynCallArgument *argumentHead, bool allowFailure)
{
	// Collect a set of available functions
	SmallArray<FunctionValue, 32> functions;

	GetNodeFunctions(ctx, source, value, functions);

	// Analyze arguments
	SmallArray<ArgumentData, 32> arguments;
	
	for(SynCallArgument *el = argumentHead; el; el = getType<SynCallArgument>(el->next))
	{
		if(functions.empty() && !el->name.empty())
			Stop(ctx, source->pos, "ERROR: function argument names are unknown at this point");

		ExprBase *argument = AnalyzeExpression(ctx, el->value);

		arguments.push_back(ArgumentData(el, false, el->name, argument->type, argument));
	}

	return CreateFunctionCall(ctx, source, value, functions, arguments, allowFailure);
}

ExprFunctionCall* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, bool allowFailure)
{
	TypeFunction *type = getType<TypeFunction>(value->type);

	IntrusiveList<ExprBase> actualArguments;

	if(!functions.empty())
	{
		FunctionValue bestOverload = SelectBestFunction(ctx, source->pos, functions, arguments, allowFailure);

		if(allowFailure && !bestOverload)
			return NULL;

		FunctionData *function = bestOverload.function;

		type = getType<TypeFunction>(function->type);

		if(type->isGeneric || (function->scope->ownerType && function->scope->ownerType->isGeneric))
		{
			bestOverload = CreateGenericFunctionInstance(ctx, bestOverload, arguments);

			function = bestOverload.function;

			type = getType<TypeFunction>(function->type);
		}

		value = new ExprFunctionAccess(source, function->type, function, bestOverload.context);

		SmallArray<ArgumentData, 32> result;

		PrepareArgumentsForFunctionCall(ctx, function->arguments, arguments, result, true);

		for(unsigned i = 0; i < result.size(); i++)
			actualArguments.push_back(result[i].value);
	}
	else
	{
		SmallArray<ArgumentData, 32> functionArguments;

		for(TypeHandle *argType = type->arguments.head; argType; argType = argType->next)
			functionArguments.push_back(ArgumentData(NULL, false, InplaceStr(), argType->type, NULL));

		SmallArray<ArgumentData, 32> result;

		if(!PrepareArgumentsForFunctionCall(ctx, functionArguments, arguments, result, true))
		{
			if(allowFailure)
				return NULL;

			char *errPos = ctx.errorBuf;

			if(arguments.size() != functionArguments.size())
				errPos += SafeSprintf(errPos, ctx.errorBufSize, "ERROR: function expects %d argument(s), while %d are supplied\r\n", functionArguments.size(), arguments.size());
			else
				errPos += SafeSprintf(errPos, ctx.errorBufSize, "ERROR: there is no conversion from specified arguments and the ones that function accepts\r\n");

			errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "\tExpected: (");

			for(unsigned i = 0; i < functionArguments.size(); i++)
				errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "%s%.*s", i != 0 ? ", " : "", FMT_ISTR(functionArguments[i].type->name));

			errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), ")\r\n");
			
			errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "\tProvided: (");

			for(unsigned i = 0; i < arguments.size(); i++)
				errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "%s%.*s", i != 0 ? ", " : "", FMT_ISTR(arguments[i].type->name));

			errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), ")");

			ctx.errorPos = source->pos;

			longjmp(ctx.errorHandler, 1);
		}

		for(unsigned i = 0; i < result.size(); i++)
			actualArguments.push_back(result[i].value);
	}

	assert(type);

	if(type->isGeneric)
		Stop(ctx, source->pos, "ERROR: generic function call is not supported");

	if(type->returnType == ctx.typeAuto)
		Stop(ctx, source->pos, "ERROR: function can't return auto");

	assert(actualArguments.size() == type->arguments.size());

	{
		ExprBase *actual = actualArguments.head;
		TypeHandle *expected = type->arguments.head;

		for(; actual && expected; actual = actual->next, expected = expected->next)
			assert(actual->type == expected->type);
	}

	return new ExprFunctionCall(source, type->returnType, value, actualArguments);
}

ExprFunctionCall* AnalyzeFunctionCall(ExpressionContext &ctx, SynFunctionCall *syntax)
{
	ExprBase *function = AnalyzeExpression(ctx, syntax->value);

	if(ExprTypeLiteral *type = getType<ExprTypeLiteral>(function))
	{
		if(ExprBase *constructor = CreateVariableAccess(ctx, syntax->value, IntrusiveList<SynIdentifier>(), type->value->name))
			function = constructor;
		else
			Stop(ctx, syntax->pos, "ERROR: implicit type constructors are not supported");
	}

	if(!syntax->aliases.empty())
		Stop(ctx, syntax->pos, "ERROR: function call with explicit generic arguments is not supported");

	return CreateFunctionCall(ctx, syntax, function, syntax->arguments.head, false);
}

ExprBase* AnalyzeNew(ExpressionContext &ctx, SynNew *syntax)
{
	if(!syntax->constructor.empty())
		Stop(ctx, syntax->pos, "ERROR: custom constructors are not supported");

	if(!syntax->arguments.empty())
		Stop(ctx, syntax->pos, "ERROR: constructor call is not supported");

	TypeBase *type = AnalyzeType(ctx, syntax->type);

	ExprBase *size = new ExprIntegerLiteral(syntax, ctx.typeInt, type->size);
	ExprBase *typeId = new ExprTypeCast(syntax, ctx.typeInt, new ExprTypeLiteral(syntax, ctx.typeTypeID, type), EXPR_CAST_REINTERPRET);

	if(syntax->count)
	{
		ExprBase *count = AnalyzeExpression(ctx, syntax->count);

		return new ExprTypeCast(syntax, ctx.GetUnsizedArrayType(type), CreateFunctionCall(ctx, syntax, InplaceStr("__newA"), size, count, typeId, false), EXPR_CAST_REINTERPRET);
	}

	return new ExprTypeCast(syntax, ctx.GetReferenceType(type), CreateFunctionCall(ctx, syntax, InplaceStr("__newS"), size, typeId, false), EXPR_CAST_REINTERPRET);
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
			if(result->type->isGeneric)
				Stop(ctx, syntax->pos, "ERROR: generic return type is not supported");

			returnType = result->type;

			function->type = ctx.GetFunctionType(returnType, function->type->arguments);
		}
		else
		{
			result = CreateCast(ctx, syntax, result, function->type->returnType, false);
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
			result = CreateCast(ctx, syntax, result, function->type->returnType, false);
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

	InplaceStr fullName = GetVariableNameInScope(ctx.scope, syntax->name);

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

		if(TypeFunction *target = getType<TypeFunction>(initializer->type))
		{
			if(FunctionValue bestOverload = GetFunctionForType(ctx, initializer->source, initializer, target))
				initializer = new ExprFunctionAccess(initializer->source, bestOverload.function->type, bestOverload.function, bestOverload.context);
		}

		type = initializer->type;
	}

	if(alignment == 0 && type->alignment != 0)
		alignment = type->alignment;

	assert(!type->isGeneric);
	assert(type != ctx.typeAuto);

	unsigned offset = AllocateVariableInScope(ctx.scope, alignment, type);
	VariableData *variable = new VariableData(syntax, ctx.scope, alignment, type, fullName, offset, ctx.uniqueVariableId++);

	ctx.AddVariable(variable);

	if(initializer)
	{
		TypeArray *arrType = getType<TypeArray>(variable->type);

		// Single-level array might be set with a single element at the point of definition
		if(arrType && !isType<TypeArray>(initializer->type))
		{
			initializer = CreateCast(ctx, syntax->initializer, initializer, arrType->subType, false);

			initializer = new ExprArraySetup(syntax->initializer, ctx.typeVoid, variable, initializer);
		}
		else
		{
			initializer = CreateAssignment(ctx, syntax->initializer, new ExprVariableAccess(syntax->initializer, variable->type, variable), initializer);
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

ExprBase* AnalyzeFunctionDefinition(ExpressionContext &ctx, SynFunctionDefinition *syntax, TypeFunction *instance, TypeBase *instanceParent, IntrusiveList<MatchData> aliases)
{
	TypeBase *parentType = syntax->parentType ? AnalyzeType(ctx, syntax->parentType) : NULL;

	if(instanceParent)
		parentType = instanceParent;

	bool addedParentScope = false;

	if(parentType && ctx.scope->ownerType != parentType)
	{
		addedParentScope = true;

		ctx.PushScope(parentType);

		// TODO: introduce class contents
		if(TypeClass *classType = getType<TypeClass>(parentType))
		{
			for(MatchData *el = classType->generics.head; el; el = el->next)
				ctx.AddAlias(new AliasData(syntax, ctx.scope, el->type, el->name, ctx.uniqueAliasId++));

			for(MatchData *el = classType->aliases.head; el; el = el->next)
				ctx.AddAlias(new AliasData(syntax, ctx.scope, el->type, el->name, ctx.uniqueAliasId++));

			for(VariableHandle *el = classType->members.head; el; el = el->next)
				ctx.AddVariable(el->variable);
		}
		else if(TypeGenericClassProto *genericProto = getType<TypeGenericClassProto>(parentType))
		{
			SynClassDefinition *definition = genericProto->definition;

			for(SynIdentifier *curr = definition->aliases.head; curr; curr = getType<SynIdentifier>(curr->next))
				ctx.AddAlias(new AliasData(syntax, ctx.scope, new TypeGeneric(InplaceStr("generic")), curr->name, ctx.uniqueAliasId++));
		}
	}

	if(!syntax->aliases.empty())
		Stop(ctx, syntax->pos, "ERROR: functions with explicit generic arguments are not implemented");

	TypeBase *returnType = AnalyzeType(ctx, syntax->returnType);

	IntrusiveList<TypeHandle> argTypes;
	SmallArray<ArgumentData, 32> argData;

	TypeHandle *instanceArg = instance ? instance->arguments.head : NULL;

	for(SynFunctionArgument *argument = syntax->arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next))
	{
		TypeBase *type = NULL;

		if(instance)
		{
			type = instanceArg->type;

			instanceArg = instanceArg->next;
		}
		else
		{
			// Create temporary scope with known arguments for reference in type expression
			ctx.PushScope();

			{
				SynFunctionArgument *prevArg = syntax->arguments.head;
				TypeHandle *prevType = argTypes.head;

				while(prevArg && prevArg != argument)
				{
					ctx.AddVariable(new VariableData(prevArg, ctx.scope, 0, prevType->type, prevArg->name, 0, ctx.uniqueVariableId++));

					prevArg = getType<SynFunctionArgument>(prevArg->next);
					prevType = prevType->next;
				}
			}
		
			type = AnalyzeType(ctx, argument->type);

			// Remove temporary scope
			ctx.PopScope();
		}

		ExprBase *initializer = argument->initializer ? AnalyzeExpression(ctx, argument->initializer) : NULL;

		argTypes.push_back(new TypeHandle(type));
		argData.push_back(ArgumentData(argument, argument->isExplicit, argument->name, type, initializer));
	}

	TypeFunction *functionType = ctx.GetFunctionType(returnType, argTypes);

	if(instance)
		assert(functionType == instance);

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
		functionName = GetFunctionNameInScope(ctx.scope, syntax->name, syntax->accessor);
	}

	// TODO: function type should be stored in type list

	FunctionData *function = new FunctionData(syntax, ctx.scope, syntax->coroutine, functionType, functionName, ctx.uniqueFunctionId++);

	function->aliases = aliases;

	// Fill in argument data
	for(unsigned i = 0; i < argData.size(); i++)
		function->arguments.push_back(argData[i]);

	// If the type is known, implement the prototype immediately
	if(functionType->returnType != ctx.typeAuto)
		ImplementPrototype(ctx, function);

	ctx.AddFunction(function);

	if(function->type->isGeneric || (parentType && isType<TypeGenericClassProto>(parentType)))
	{
		assert(!instance);

		if(syntax->prototype)
			Stop(ctx, syntax->pos, "ERROR: generic function cannot be forward-declared");

		if(addedParentScope)
			ctx.PopScope();

		return new ExprGenericFunctionPrototype(syntax, ctx.typeVoid, function);
	}

	ctx.PushScope(function);

	for(MatchData *curr = function->aliases.head; curr; curr = curr->next)
		ctx.AddAlias(new AliasData(syntax, ctx.scope, curr->type, curr->name, ctx.uniqueAliasId++));

	if(TypeBase *parent = function->scope->ownerType)
	{
		TypeBase *type = ctx.GetReferenceType(parent);

		assert(!type->isGeneric);

		unsigned offset = AllocateVariableInScope(ctx.scope, 0, type);
		VariableData *variable = new VariableData(syntax, ctx.scope, 0, type, InplaceStr("this"), offset, ctx.uniqueVariableId++);

		ctx.AddVariable(variable);
	}

	IntrusiveList<ExprVariableDefinition> arguments;

	for(unsigned i = 0; i < argData.size(); i++)
	{
		ArgumentData &argument = argData[i];

		assert(!argument.type->isGeneric);

		unsigned offset = AllocateVariableInScope(ctx.scope, 0, argument.type);
		VariableData *variable = new VariableData(argument.source, ctx.scope, 0, argument.type, argument.name, offset, ctx.uniqueVariableId++);

		ctx.AddVariable(variable);

		arguments.push_back(new ExprVariableDefinition(argument.source, ctx.typeVoid, variable, NULL));
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
			Stop(ctx, syntax->pos, "ERROR: function must return a value of type '%.*s'", FMT_ISTR(returnType->name));
	}

	ctx.PopScope();

	if(addedParentScope)
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

	condition = CreateConditionCast(ctx, condition->source, condition);

	if(ExprBoolLiteral *number = getType<ExprBoolLiteral>(Evaluate(ctx, CreateCast(ctx, syntax, condition, ctx.typeBool, false))))
	{
		if(number->value)
			AnalyzeClassElements(ctx, classDefinition, syntax->trueBlock);
		else if(syntax->falseBlock)
			AnalyzeClassElements(ctx, classDefinition, syntax->falseBlock);
	}
	else
	{
		Stop(ctx, syntax->pos, "ERROR: can't get condition value");
	}
}

void AnalyzeClassElements(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassElements *syntax)
{
	// TODO: can't access sizeof and type members until finalization

	for(SynTypedef *typeDef = syntax->typedefs.head; typeDef; typeDef = getType<SynTypedef>(typeDef->next))
	{
		ExprAliasDefinition *alias = AnalyzeTypedef(ctx, typeDef);

		classDefinition->classType->aliases.push_back(new MatchData(alias->alias->name, alias->alias->type));
	}

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
		classDefinition->functions.push_back(AnalyzeFunctionDefinition(ctx, function, NULL, NULL, IntrusiveList<MatchData>()));

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
	InplaceStr typeName = GetTypeNameInScope(ctx.scope, syntax->name);

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

	TypeClass *baseClass = NULL;

	if(syntax->baseClass)
	{
		TypeBase *type = AnalyzeType(ctx, syntax->baseClass);

		baseClass = getType<TypeClass>(type);

		if(!baseClass || !baseClass->extendable)
			Stop(ctx, syntax->pos, "ERROR: type '%.*s' is not extendable", FMT_ISTR(type->name));
	}

	IntrusiveList<MatchData> actualGenerics;

	{
		TypeHandle *currType = generics.head;
		SynIdentifier *currName = syntax->aliases.head;

		while(currType && currName)
		{
			actualGenerics.push_back(new MatchData(currName->name, currType->type));

			currType = currType->next;
			currName = getType<SynIdentifier>(currName->next);
		}
	}
	
	TypeClass *classType = new TypeClass(syntax, className, proto, actualGenerics, syntax->extendable, baseClass);

	classType->alignment = alignment;

	ctx.AddType(classType);

	if(!generics.empty())
		ctx.genericTypeMap.insert(className.hash(), classType);

	ExprClassDefinition *classDefinition = new ExprClassDefinition(syntax, ctx.typeVoid, classType);

	ctx.PushScope(classType);

	for(MatchData *el = classType->generics.head; el; el = el->next)
		ctx.AddAlias(new AliasData(syntax, ctx.scope, el->type, el->name, ctx.uniqueAliasId++));

	// TODO: Base class members should be introduced into the scope

	AnalyzeClassElements(ctx, classDefinition, syntax->elements);

	ctx.PopScope();

	return classDefinition;
}

ExprBlock* AnalyzeNamespaceDefinition(ExpressionContext &ctx, SynNamespaceDefinition *syntax)
{
	if(ctx.scope != ctx.globalScope && ctx.scope->ownerNamespace == NULL)
		Stop(ctx, syntax->pos, "ERROR: a namespace definition must appear either at file scope or immediately within another namespace definition");

	for(SynIdentifier *name = syntax->path.head; name; name = getType<SynIdentifier>(name->next))
	{
		NamespaceData *ns = new NamespaceData(syntax, ctx.scope, ctx.GetCurrentNamespace(), name->name, ctx.uniqueNamespaceId++);

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

	AliasData *alias = new AliasData(syntax, ctx.scope, type, syntax->alias, ctx.uniqueAliasId++);

	ctx.AddAlias(alias);

	return new ExprAliasDefinition(syntax, ctx.typeVoid, alias);
}

ExprBase* AnalyzeIfElse(ExpressionContext &ctx, SynIfElse *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	condition = CreateConditionCast(ctx, condition->source, condition);

	if(syntax->staticIf)
	{
		if(ExprBoolLiteral *number = getType<ExprBoolLiteral>(Evaluate(ctx, CreateCast(ctx, syntax, condition, ctx.typeBool, false))))
		{
			if(number->value)
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

	condition = CreateConditionCast(ctx, condition->source, condition);

	return new ExprFor(syntax, ctx.typeVoid, initializer, condition, increment, body);
}

ExprWhile* AnalyzeWhile(ExpressionContext &ctx, SynWhile *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);
	ExprBase *body = syntax->body ? AnalyzeStatement(ctx, syntax->body) : new ExprVoid(syntax, ctx.typeVoid);

	condition = CreateConditionCast(ctx, condition->source, condition);

	return new ExprWhile(syntax, ctx.typeVoid, condition, body);
}

ExprDoWhile* AnalyzeDoWhile(ExpressionContext &ctx, SynDoWhile *syntax)
{
	ctx.PushScope();

	IntrusiveList<ExprBase> expressions;

	for(SynBase *expression = syntax->expressions.head; expression; expression = expression->next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	condition = CreateConditionCast(ctx, condition->source, condition);

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

		return new ExprStringLiteral(node, ctx.GetArrayType(syntax->pos, ctx.typeChar, length + 1), value, length);
	}
	
	if(SynNullptr *node = getType<SynNullptr>(syntax))
	{
		return new ExprNullptrLiteral(node, ctx.typeNullPtr);
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
		return AnalyzeFunctionDefinition(ctx, node, NULL, NULL, IntrusiveList<MatchData>());
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
		return AnalyzeFunctionDefinition(ctx, node, NULL, NULL, IntrusiveList<MatchData>());
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

	ByteCode* bytecode;
	const char *name;
	Lexer lexer;

	FastVector<TypeBase*, true, true> types;
};

void ImportModuleNamespaces(ExpressionContext &ctx, SynBase *source, ModuleContext &module)
{
	ByteCode *bCode = module.bytecode;
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
			ctx.namespaces.push_back(new NamespaceData(source, ctx.scope, ctx.GetCurrentNamespace(), InplaceStr(symbols + ns.offsetToName), ctx.uniqueNamespaceId++));
		}
	}
}

void ImportModuleTypes(ExpressionContext &ctx, SynBase *source, ModuleContext &module)
{
	ByteCode *bCode = module.bytecode;
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
					module.types[i] = ctx.GetArrayType(source->pos, subType, type.arrSize);
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
				InplaceStr className = InplaceStr(symbols + type.offsetToName);

				if(type.namespaceHash != ~0u)
					Stop(ctx, source->pos, "ERROR: can't import namespace type");

				if(type.constantCount != 0)
					Stop(ctx, source->pos, "ERROR: can't import constants of type");

				if(type.definitionOffset != ~0u)
				{
					if(type.definitionOffset & 0x80000000)
					{
						Stop(ctx, source->pos, "ERROR: can't import derived type");
					}
					else
					{
						Lexeme *start = type.definitionOffset + module.lexer.GetStreamStart() - 3;

						ParseContext pCtx;

						pCtx.currentLexeme = start;

						SynClassDefinition *definition = getType<SynClassDefinition>(ParseClassDefinition(pCtx));

						if(!definition)
							Stop(ctx, source->pos, "ERROR: failed to import generic class body");

						TypeGenericClassProto *genericProtoType = new TypeGenericClassProto(className, definition);

						ctx.AddType(genericProtoType);

						module.types[i] = genericProtoType;
					}

					continue;
				}

				IntrusiveList<MatchData> actualGenerics;

				TypeClass *classType = new TypeClass(source, className, NULL, actualGenerics, false, NULL);

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

					VariableData *member = new VariableData(source, ctx.scope, 0, memberType, memberName, memberList[type.memberOffset + n].offset, ctx.uniqueVariableId++);

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
	ByteCode *bCode = module.bytecode;
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

		VariableData *data = new VariableData(source, ctx.scope, 0, type, name, variable.offset, ctx.uniqueVariableId++);

		ctx.AddVariable(data);
	}
}

void ImportModuleTypedefs(ExpressionContext &ctx, SynBase *source, ModuleContext &module)
{
	ByteCode *bCode = module.bytecode;
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
			TypeBase *parentType = module.types[alias.parentType];

			if(!parentType)
				Stop(ctx, source->pos, "ERROR: can't find alias '%s' parent type", symbols + alias.offsetToName);

			if(TypeClass *type = getType<TypeClass>(parentType))
			{
				type->aliases.push_back(new MatchData(aliasName, targetType));
			}
			else if(!isType<TypeGenericClassProto>(parentType))
			{
				Stop(ctx, source->pos, "ERROR: can't import class alias");
			}
		}
		else
		{
			AliasData *alias = new AliasData(source, ctx.scope, targetType, aliasName, ctx.uniqueAliasId++);

			ctx.AddAlias(alias);
		}
	}
}

void ImportModuleFunctions(ExpressionContext &ctx, SynBase *source, ModuleContext &module)
{
	ByteCode *bCode = module.bytecode;
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

		FunctionData *data = new FunctionData(source, ctx.scope, coroutine, getType<TypeFunction>(functionType), functionName, ctx.uniqueFunctionId++);

		data->isExternal = true;

		ctx.AddFunction(data);

		if(parentType)
			ctx.PushScope(parentType);

		ctx.PushScope(data);

		if(parentType)
		{
			TypeBase *type = ctx.GetReferenceType(parentType);

			unsigned offset = AllocateVariableInScope(ctx.scope, 0, type);
			VariableData *variable = new VariableData(source, ctx.scope, 0, type, InplaceStr("this"), offset, ctx.uniqueVariableId++);

			ctx.AddVariable(variable);
		}

		for(unsigned n = 0; n < function.paramCount; n++)
		{
			ExternLocalInfo &argument = localList[function.offsetToFirstLocal + n];

			bool isExplicit = (argument.paramFlags & ExternLocalInfo::IS_EXPLICIT) != 0;

			TypeBase *argType = module.types[argument.type];

			if(!argType)
				Stop(ctx, source->pos, "ERROR: can't find argument %d type for '%s' in module %s", n + 1, symbols + function.offsetToName, module.name);

			InplaceStr argName = InplaceStr(symbols + argument.offsetToName);

			// TODO: default argument values
			data->arguments.push_back(ArgumentData(source, isExplicit, argName, argType, NULL));

			unsigned offset = AllocateVariableInScope(ctx.scope, 0, argType);
			VariableData *variable = new VariableData(source, ctx.scope, 0, argType, argName, offset, ctx.uniqueVariableId++);

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

	module.bytecode = (ByteCode*)bytecode;
	module.name = name;
	module.lexer.Lexify(FindSource(module.bytecode));

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
	assert(!ctx.globalScope);

	ctx.PushScope();
	ctx.globalScope = ctx.scope;

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
	ctx.AddType(ctx.typeNullPtr = new TypeFunctionID(InplaceStr("__nullptr")));

	ctx.AddType(ctx.typeAuto = new TypeAuto(InplaceStr("auto")));

	ctx.AddType(ctx.typeAutoRef = new TypeAutoRef(InplaceStr("auto ref")));
	ctx.PushScope(ctx.typeAutoRef);
	ctx.typeAutoRef->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scope, ctx.typeTypeID, InplaceStr("type"), ctx.uniqueVariableId++)));
	ctx.typeAutoRef->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scope, ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"), ctx.uniqueVariableId++)));
	ctx.PopScope();

	ctx.AddType(ctx.typeAutoArray = new TypeAutoArray(InplaceStr("auto[]")));
	ctx.PushScope(ctx.typeAutoArray);
	ctx.typeAutoArray->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scope, ctx.typeTypeID, InplaceStr("type"), ctx.uniqueVariableId++)));
	ctx.typeAutoArray->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scope, ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"), ctx.uniqueVariableId++)));
	ctx.typeAutoArray->members.push_back(new VariableHandle(AllocateClassMember(syntax, ctx.scope, ctx.typeInt, InplaceStr("size"), ctx.uniqueVariableId++)));
	ctx.PopScope();

	// Analyze module
	if(!setjmp(ctx.errorHandler))
	{
		ExprBase *module = AnalyzeModule(ctx, syntax);

		ctx.PopScope();

		return module;
	}

	return NULL;
}
