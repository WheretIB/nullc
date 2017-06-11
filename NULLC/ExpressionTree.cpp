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

	ScopeData* NamedScopeFrom(ScopeData *scope)
	{
		if(!scope || scope->ownerNamespace)
			return scope;

		return NamedScopeFrom(scope->scope);
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

	VariableData* AllocateClassMember(ExpressionContext &ctx, SynBase *source, TypeBase *type, InplaceStr name, unsigned uniqueId)
	{
		unsigned offset = AllocateVariableInScope(ctx.scope, type->alignment, type);

		assert(!type->isGeneric);

		VariableData *variable = new VariableData(source, ctx.scope, type->alignment, type, name, offset, uniqueId);

		ctx.AddVariable(variable);

		return variable;
	}

	VariableData* AllocateTemporary(ExpressionContext &ctx, SynBase *source, TypeBase *type)
	{
		char *name = new char[16];
		sprintf(name, "$temp%d", ctx.unnamedVariableCount++);

		unsigned offset = AllocateVariableInScope(ctx.scope, type->alignment, type);
		VariableData *variable = new VariableData(source, ctx.scope, type->alignment, type, InplaceStr(name), offset, ctx.uniqueVariableId++);

		ctx.AddVariable(variable);

		return variable;
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
	baseModuleFunctionCount = 0;

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
	uniqueScopeId = 0;

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

	ScopeData *next = new ScopeData(depth, scope, uniqueScopeId++);

	if(scope)
		scope->scopes.push_back(next);

	scope = next;
}

void ExpressionContext::PushScope(NamespaceData *nameSpace)
{
	unsigned depth = scope ? scope->depth + 1 : 0;

	ScopeData *next = new ScopeData(depth, scope, uniqueScopeId++, nameSpace);

	if(scope)
		scope->scopes.push_back(next);

	scope = next;
}

void ExpressionContext::PushScope(FunctionData *function)
{
	unsigned depth = scope ? scope->depth + 1 : 0;

	ScopeData *next = new ScopeData(depth, scope, uniqueScopeId++, function);

	if(scope)
		scope->scopes.push_back(next);

	scope = next;
}

void ExpressionContext::PushScope(TypeBase *type)
{
	unsigned depth = scope ? scope->depth + 1 : 0;

	ScopeData *next = new ScopeData(depth, scope, uniqueScopeId++, type);

	if(scope)
		scope->scopes.push_back(next);

	scope = next;
}

void ExpressionContext::PushTemporaryScope()
{
	unsigned depth = scope ? scope->depth + 1 : 0;

	scope = new ScopeData(depth, scope, 0);
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

		if(!location || variable->imported || variable->source->pos <= location->pos)
			variableMap.insert(variable->nameHash, variable);
	}

	for(unsigned i = 0; i < target->functions.size(); i++)
	{
		FunctionData *function = target->functions[i];

		// Class functions are kept visible, no need to add again
		if(function->scope->ownerType)
			continue;

		if(!location || function->imported || function->source->pos <= location->pos)
			functionMap.insert(function->nameHash, function);
	}

	for(unsigned i = 0; i < target->types.size(); i++)
	{
		TypeBase *type = target->types[i];

		if(TypeClass *exact = getType<TypeClass>(type))
		{
			if(!location || exact->imported || exact->source->pos <= location->pos)
				typeMap.insert(type->nameHash, type);
		}
		else if(TypeGenericClassProto *exact = getType<TypeGenericClassProto>(type))
		{
			if(!location || exact->definition->imported || exact->definition->pos <= location->pos)
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

		if(!location || alias->imported || alias->source->pos <= location->pos)
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
	// Simply walk up the scopes and find the current one
	for(ScopeData *curr = scope; curr; curr = curr->scope)
	{
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

TypeFunction* ExpressionContext::GetFunctionType(TypeBase* returnType, SmallArray<ArgumentData, 32> &arguments)
{
	IntrusiveList<TypeHandle> types;

	for(unsigned i = 0; i < arguments.size(); i++)
		types.push_back(new TypeHandle(arguments[i].type));

	return GetFunctionType(returnType, types);
}

ExprBase* CreateTypeidMemberAccess(ExpressionContext &ctx, SynBase *source, TypeBase *type, InplaceStr member);

ExprBase* CreateBinaryOp(ExpressionContext &ctx, SynBase *source, SynBinaryOpType op, ExprBase *lhs, ExprBase *rhs);

ExprBase* CreateVariableAccess(ExpressionContext &ctx, SynBase *source, IntrusiveList<SynIdentifier> path, InplaceStr name);
ExprBase* CreateFunctionAccess(ExpressionContext &ctx, SynBase *source, HashMap<FunctionData*>::Node *function, ExprBase *context);

TypeBase* MatchGenericType(ExpressionContext &ctx, SynBase *source, TypeBase *matchType, TypeBase *argType, IntrusiveList<MatchData> &aliases, bool strict);
TypeBase* ResolveGenericTypeAliases(ExpressionContext &ctx, SynBase *source, TypeBase *type, IntrusiveList<MatchData> aliases);

FunctionValue SelectBestFunction(ExpressionContext &ctx, SynBase *source, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, SmallArray<unsigned, 32> &ratings);
FunctionValue CreateGenericFunctionInstance(ExpressionContext &ctx, SynBase *source, FunctionValue proto, SmallArray<ArgumentData, 32> &arguments);
void GetNodeFunctions(ExpressionContext &ctx, SynBase *source, ExprBase *function, SmallArray<FunctionValue, 32> &functions);
void StopOnFunctionSelectError(ExpressionContext &ctx, SynBase *source, char* errPos, SmallArray<FunctionValue, 32> &functions);
void StopOnFunctionSelectError(ExpressionContext &ctx, SynBase *source, char* errPos, InplaceStr functionName, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, SmallArray<unsigned, 32> &ratings, unsigned bestRating, bool showInstanceInfo);
ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, bool allowFailure);
ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, ExprBase *arg1, bool allowFailure);
ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, ExprBase *arg1, ExprBase *arg2, bool allowFailure);
ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, SmallArray<ArgumentData, 32> &arguments, bool allowFailure);
ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<ArgumentData, 32> &arguments, bool allowFailure);
ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SynCallArgument *argumentHead, bool allowFailure);
ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<FunctionValue, 32> &functions, SynCallArgument *argumentHead, bool allowFailure);
ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, bool allowFailure);

bool RestoreParentTypeScope(ExpressionContext &ctx, SynBase *source, TypeBase *parentType);
ExprBase* CreateFunctionDefinition(ExpressionContext &ctx, SynBase *source, bool prototype, bool coroutine, TypeBase *parentType, bool accessor, TypeBase *returnType, bool isOperator, InplaceStr(name), IntrusiveList<SynIdentifier> aliases, IntrusiveList<SynFunctionArgument> arguments, IntrusiveList<SynBase> expressions, TypeFunction *instance, IntrusiveList<MatchData> matches);

FunctionValue GetFunctionForType(ExpressionContext &ctx, SynBase *source, ExprBase *value, TypeFunction *type)
{
	// Collect a set of available functions
	SmallArray<FunctionValue, 32> functions;

	GetNodeFunctions(ctx, source, value, functions);

	if(!functions.empty())
	{
		FunctionValue bestMatch;
		TypeFunction *bestMatchTarget = NULL;

		FunctionValue bestGenericMatch;
		TypeFunction *bestGenericMatchTarget = NULL;

		for(unsigned i = 0; i < functions.size(); i++)
		{
			TypeFunction *functionType = functions[i].function->type;

			if(type->arguments.size() != functionType->arguments.size())
				continue;

			if(type->isGeneric)
			{
				IntrusiveList<MatchData> aliases;

				TypeBase *returnType = MatchGenericType(ctx, source, type->returnType, functionType->returnType, aliases, true);
				IntrusiveList<TypeHandle> arguments;

				for(TypeHandle *lhs = type->arguments.head, *rhs = functionType->arguments.head; lhs && rhs; lhs = lhs->next, rhs = rhs->next)
				{
					TypeBase *match = MatchGenericType(ctx, source, lhs->type, rhs->type, aliases, true);

					if(match && !match->isGeneric)
						arguments.push_back(new TypeHandle(match));
				}

				if(returnType && arguments.size() == type->arguments.size())
				{
					if(bestGenericMatch)
						return FunctionValue();

					bestGenericMatch = functions[i];
					bestGenericMatchTarget = ctx.GetFunctionType(returnType, arguments);
				}
			}
			else if(functionType->isGeneric)
			{
				unsigned matches = 0;

				IntrusiveList<MatchData> aliases;

				for(TypeHandle *lhs = functionType->arguments.head, *rhs = type->arguments.head; lhs && rhs; lhs = lhs->next, rhs = rhs->next)
				{
					TypeBase *match = MatchGenericType(ctx, source, lhs->type, rhs->type, aliases, true);

					if(match && !match->isGeneric)
						matches++;
				}

				if(matches == type->arguments.size())
				{
					if(bestGenericMatch)
						return FunctionValue();

					bestGenericMatch = functions[i];
					bestGenericMatchTarget = type;
				}
			}
			else if(functionType == type)
			{
				if(bestMatch)
					return FunctionValue();

				bestMatch = functions[i];
				bestMatchTarget = type;
			}
		}

		FunctionValue bestOverload = bestMatch ? bestMatch : bestGenericMatch;
		TypeFunction *bestTarget = bestMatch ? bestMatchTarget : bestGenericMatchTarget;

		if(bestOverload)
		{
			SmallArray<ArgumentData, 32> arguments;

			for(TypeHandle *curr = bestTarget->arguments.head; curr; curr = curr->next)
				arguments.push_back(ArgumentData(source, false, InplaceStr(), curr->type, NULL));

			FunctionData *function = bestOverload.function;

			TypeFunction *overloadType = getType<TypeFunction>(function->type);

			if(overloadType->isGeneric || (function->scope->ownerType && function->scope->ownerType->isGeneric))
				bestOverload = CreateGenericFunctionInstance(ctx, source, bestOverload, arguments);

			if(bestOverload)
			{
				if(bestTarget->returnType == ctx.typeAuto)
					bestTarget = ctx.GetFunctionType(bestOverload.function->type->returnType, bestTarget->arguments);

				if(bestOverload.function->type == bestTarget)
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
	if(!ctx.IsNumericType(value->type))
	{
		// TODO: function overload

		if(isType<TypeRef>(value->type))
			return CreateCast(ctx, source, value, ctx.typeBool, false);

		if(isType<TypeUnsizedArray>(value->type))
			return CreateCast(ctx, source, value, ctx.typeBool, false);

		if(isType<TypeFunction>(value->type))
			return CreateCast(ctx, source, value, ctx.typeBool, false);

		if(value->type == ctx.typeAutoRef)
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

ExprBase* CreateAssignment(ExpressionContext &ctx, SynBase *source, ExprBase *lhs, ExprBase *rhs)
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
	else if(ExprFunctionCall *node = getType<ExprFunctionCall>(lhs))
	{
		// Try to transform 'get' accessor to 'set'
		if(ExprFunctionAccess *access = getType<ExprFunctionAccess>(node->function))
		{
			if(access->function->accessor)
			{
				if(HashMap<FunctionData*>::Node *function = ctx.functionMap.first(access->function->name.hash()))
				{
					ExprBase *overloads = CreateFunctionAccess(ctx, source, function, access->context);

					SmallArray<ArgumentData, 32> arguments;

					arguments.push_back(ArgumentData(rhs->source, false, InplaceStr(), rhs->type, rhs));

					if(ExprBase *call = CreateFunctionCall(ctx, source, overloads, arguments, true))
						return call;
				}
			}
		}

		if(TypeRef *refType = getType<TypeRef>(lhs->type))
			lhs = new ExprDereference(source, refType->subType, lhs);
	}

	if(!isType<TypeRef>(wrapped->type))
		Stop(ctx, source->pos, "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(lhs->type->name));

	if(rhs->type == ctx.typeVoid)
		Stop(ctx, source->pos, "ERROR: cannot convert from void to %.*s", FMT_ISTR(lhs->type->name));

	if(lhs->type == ctx.typeVoid)
		Stop(ctx, source->pos, "ERROR: cannot convert from %.*s to void", FMT_ISTR(rhs->type->name));

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
			TypeBase *type = ctx.GetFunctionType(ctx.typeVoid, types);

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
	ok |= isType<TypeEnum>(lhs->type) && lhs->type == rhs->type;

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
ExprBase* AnalyzeFunctionDefinition(ExpressionContext &ctx, SynFunctionDefinition *syntax, TypeFunction *instance, TypeBase *instanceParent, IntrusiveList<MatchData> matches);
ExprBase* AnalyzeShortFunctionDefinition(ExpressionContext &ctx, SynShortFunctionDefinition *syntax, TypeFunction *argumentType);
ExprBase* AnalyzeShortFunctionDefinition(ExpressionContext &ctx, SynShortFunctionDefinition *syntax, TypeBase *type, SmallArray<ArgumentData, 32> &arguments);

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

	ExpressionEvalContext evalCtx(ctx);

	if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(Evaluate(evalCtx, CreateCast(ctx, size, sizeValue, ctx.typeLong, false))))
	{
		if(number->value <= 0)
			Stop(ctx, size->pos, "ERROR: array size can't be negative or zero");

		return ctx.GetArrayType(type, number->value);
	}

	Stop(ctx, size->pos, "ERROR: can't get array size");

	return NULL;
}

TypeBase* CreateGenericTypeInstance(ExpressionContext &ctx, SynBase *source, TypeGenericClassProto *proto, IntrusiveList<TypeHandle> &types)
{
	InplaceStr className = GetGenericClassName(proto, types);

	// Check if type already exists
	if(TypeClass **prev = ctx.genericTypeMap.find(className.hash()))
		return *prev;

	// Switch to original type scope
	ScopeData *scope = ctx.scope;

	ctx.SwitchToScopeAtPoint(NULL, proto->scope, proto->source);

	ExprBase *result = AnalyzeClassDefinition(ctx, proto->definition, proto, types);

	// Restore old scope
	ctx.SwitchToScopeAtPoint(proto->source, scope, NULL);

	if(ExprClassDefinition *definition = getType<ExprClassDefinition>(result))
	{
		proto->instances.push_back(result);

		return definition->classType;
	}

	Stop(ctx, source->pos, "ERROR: type '%s' couldn't be instantiated", proto->name);

	return NULL;
}

TypeBase* AnalyzeType(ExpressionContext &ctx, SynBase *syntax, bool onlyType = true, bool *failed = NULL)
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
		TypeBase *type = AnalyzeType(ctx, node->type, true, failed);

		if(isType<TypeAuto>(type))
			return ctx.typeAutoRef;

		return ctx.GetReferenceType(type);
	}

	if(SynTypeArray *node = getType<SynTypeArray>(syntax))
	{
		TypeBase *type = AnalyzeType(ctx, node->type, onlyType, failed);

		if(!onlyType && !type)
			return NULL;

		return ApplyArraySizesToType(ctx, type, node->sizes.head);
	}

	if(SynArrayIndex *node = getType<SynArrayIndex>(syntax))
	{
		TypeBase *type = AnalyzeType(ctx, node->value, onlyType, failed);

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

		SynCallArgument *argument = node->arguments.head;

		if(!argument->name.empty())
			Stop(ctx, syntax->pos, "ERROR: named argument not expected in array type size");

		ExprBase *size = AnalyzeExpression(ctx, argument->value);
		
		ExpressionEvalContext evalCtx(ctx);

		if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(Evaluate(evalCtx, CreateCast(ctx, node, size, ctx.typeLong, false))))
		{
			if(TypeArgumentSet *lhs = getType<TypeArgumentSet>(type))
			{
				if(number->value < 0)
					Stop(ctx, syntax->pos, "ERROR: argument index can't be negative");

				if(number->value >= lhs->types.size())
					Stop(ctx, syntax->pos, "ERROR: this function type '%.*s' has only %d argument(s)", FMT_ISTR(type->name), lhs->types.size());

				return lhs->types[unsigned(number->value)]->type;
			}

			if(number->value <= 0)
				Stop(ctx, syntax->pos, "ERROR: array size can't be negative or zero");

			return ctx.GetArrayType(type, number->value);
		}

		Stop(ctx, syntax->pos, "ERROR: index must be a constant expression");
	}

	if(SynTypeFunction *node = getType<SynTypeFunction>(syntax))
	{
		TypeBase *returnType = AnalyzeType(ctx, node->returnType, true, failed);

		IntrusiveList<TypeHandle> arguments;

		for(SynBase *el = node->arguments.head; el; el = el->next)
		{
			TypeBase *argType = AnalyzeType(ctx, el, true, failed);

			if(argType == ctx.typeAuto)
				Stop(ctx, syntax->pos, "ERROR: function parameter cannot be an auto type");

			if(argType == ctx.typeVoid)
				Stop(ctx, syntax->pos, "ERROR: function parameter cannot be a void type");

			arguments.push_back(new TypeHandle(argType));
		}

		return ctx.GetFunctionType(returnType, arguments);
	}

	if(SynTypeof *node = getType<SynTypeof>(syntax))
	{
		jmp_buf prevErrorHandler;
		memcpy(&prevErrorHandler, &ctx.errorHandler, sizeof(jmp_buf));

		if(!setjmp(ctx.errorHandler))
		{
			TypeBase *type = AnalyzeType(ctx, node->value, false);

			if(!type)
			{
				ExprBase *value = AnalyzeExpression(ctx, node->value);

				if(value->type == ctx.typeAuto)
					Stop(ctx, syntax->pos, "ERROR: cannot take typeid from auto type");

				type = value->type;
			}

			memcpy(&ctx.errorHandler, &prevErrorHandler, sizeof(jmp_buf));

			if(type)
				return type;
		}
		else
		{
			memcpy(&ctx.errorHandler, &prevErrorHandler, sizeof(jmp_buf));

			if(failed)
			{
				*failed = true;
				return new TypeGeneric(InplaceStr("generic"));
			}

			longjmp(ctx.errorHandler, 1);
		}
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

		Stop(ctx, syntax->pos, "ERROR: unknown simple type");
	}

	if(SynMemberAccess *node = getType<SynMemberAccess>(syntax))
	{
		TypeBase *value = AnalyzeType(ctx, node->value, onlyType, failed);

		if(!onlyType && !value)
			return NULL;

		if(isType<TypeGeneric>(value))
			return new TypeGeneric(InplaceStr("generic"));

		ExprBase *result = CreateTypeidMemberAccess(ctx, syntax, value, node->member);

		if(ExprTypeLiteral *typeLiteral = getType<ExprTypeLiteral>(result))
			return typeLiteral->value;

		// [n]

		if(!onlyType)
			return NULL;

		// isReference/isArray/isFunction/arraySize/hasMember(x)/class member/class typedef

		Stop(ctx, syntax->pos, "ERROR: unknown member access type");

		return NULL;
	}

	if(SynTypeGenericInstance *node = getType<SynTypeGenericInstance>(syntax))
	{
		TypeBase *baseType = AnalyzeType(ctx, node->baseType, true, failed);

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
				TypeBase *type = AnalyzeType(ctx, el, true, failed);

				isGeneric |= type->isGeneric;

				types.push_back(new TypeHandle(type));
			}

			InplaceStr className = GetGenericClassName(proto, types);

			if(isGeneric)
				return new TypeGenericClass(className, proto, types);
			
			return CreateGenericTypeInstance(ctx, syntax, proto, types);
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

	ExpressionEvalContext evalCtx(ctx);

	if(ExprIntegerLiteral *alignValue = getType<ExprIntegerLiteral>(Evaluate(evalCtx, CreateCast(ctx, syntax, align, ctx.typeLong, false))))
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

	return new ExprArray(syntax, ctx.GetArrayType(subType, values.size()), values);
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

		ExprBase *access = new ExprVariableAccess(source, data->type, data);

		if(data->isReference)
		{
			assert(isType<TypeRef>(data->type));

			TypeRef *type = getType<TypeRef>(data->type);

			access = new ExprDereference(source, type->subType, access);
		}

		return access;
	}

	if(path.empty())
	{
		// Try a class constant or an alias
		if(TypeStruct *structType = getType<TypeStruct>(ctx.GetCurrentType()))
		{
			for(ConstantData *curr = structType->constants.head; curr; curr = curr->next)
			{
				if(curr->name == name)
					return curr->value;
			}
		}
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

				if(function)
				{
					ExprBase *thisAccess = CreateVariableAccess(ctx, source, IntrusiveList<SynIdentifier>(), InplaceStr("this"));

					if(!thisAccess)
						Stop(ctx, source->pos, "ERROR: 'this' variable is not available");

					ExprBase *access = CreateFunctionAccess(ctx, source, function, thisAccess);

					return CreateFunctionCall(ctx, source, access, NULL, false);
				}
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
		}
	}

	if(function)
	{
		// TODO: function context

		return CreateFunctionAccess(ctx, source, function, new ExprNullptrLiteral(source, ctx.GetReferenceType(ctx.typeVoid)));
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
		wrapped = new ExprGetAddress(syntax, ctx.GetReferenceType(value->type), node->variable);
	else if(ExprDereference *node = getType<ExprDereference>(value))
		wrapped = node->value;

	if(!isType<TypeRef>(wrapped->type))
		Stop(ctx, syntax->pos, "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(value->type->name));

	return new ExprPreModify(syntax, value->type, wrapped, syntax->isIncrement);
}

ExprPostModify* AnalyzePostModify(ExpressionContext &ctx, SynPostModify *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	ExprBase* wrapped = value;

	if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
		wrapped = new ExprGetAddress(syntax, ctx.GetReferenceType(value->type), node->variable);
	else if(ExprDereference *node = getType<ExprDereference>(value))
		wrapped = node->value;

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

	// Handle null pointer promotion
	if(trueBlock->type != falseBlock->type)
	{
		if(trueBlock->type == ctx.typeNullPtr)
			trueBlock = CreateCast(ctx, syntax->trueBlock, trueBlock, falseBlock->type, false);

		if(falseBlock->type == ctx.typeNullPtr)
			falseBlock = CreateCast(ctx, syntax->falseBlock, falseBlock, trueBlock->type, false);
	}

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

ExprBase* CreateTypeidMemberAccess(ExpressionContext &ctx, SynBase *source, TypeBase *type, InplaceStr member)
{
	if(member == InplaceStr("isReference"))
	{
		return new ExprBoolLiteral(source, ctx.typeBool, isType<TypeRef>(type));
	}

	if(member == InplaceStr("isArray"))
	{
		return new ExprBoolLiteral(source, ctx.typeBool, isType<TypeArray>(type) || isType<TypeUnsizedArray>(type));
	}

	if(member == InplaceStr("isFunction"))
	{
		return new ExprBoolLiteral(source, ctx.typeBool, isType<TypeFunction>(type));
	}

	if(member == InplaceStr("arraySize"))
	{
		if(TypeArray *arrType = getType<TypeArray>(type))
			return new ExprIntegerLiteral(source, ctx.typeInt, arrType->length);

		if(TypeUnsizedArray *arrType = getType<TypeUnsizedArray>(type))
			return new ExprIntegerLiteral(source, ctx.typeInt, -1);

		Stop(ctx, source->pos, "ERROR: 'arraySize' can only be applied to an array type, but we have '%.*s'", FMT_ISTR(type->name));
	}

	if(member == InplaceStr("size"))
	{
		if(TypeArgumentSet *argumentsType = getType<TypeArgumentSet>(type))
			return new ExprIntegerLiteral(source, ctx.typeInt, argumentsType->types.size());

		Stop(ctx, source->pos, "ERROR: 'size' can only be applied to an function type, but we have '%.*s'", FMT_ISTR(type->name));
	}

	if(member == InplaceStr("argument"))
	{
		if(TypeFunction *functionType = getType<TypeFunction>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, new TypeArgumentSet(GetArgumentSetTypeName(functionType->arguments), functionType->arguments));

		Stop(ctx, source->pos, "ERROR: 'argument' can only be applied to a function type, but we have '%.*s'", FMT_ISTR(type->name));
	}

	if(member == InplaceStr("return"))
	{
		if(TypeFunction *functionType = getType<TypeFunction>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, functionType->returnType);

		Stop(ctx, source->pos, "ERROR: 'return' can only be applied to a function type, but we have '%.*s'", FMT_ISTR(type->name));
	}

	if(member == InplaceStr("target"))
	{
		if(TypeRef *refType = getType<TypeRef>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, refType->subType);

		if(TypeArray *arrType = getType<TypeArray>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, arrType->subType);

		if(TypeUnsizedArray *arrType = getType<TypeUnsizedArray>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, arrType->subType);

		Stop(ctx, source->pos, "ERROR: 'target' can only be applied to a pointer or array type, but we have '%.*s'", FMT_ISTR(type->name));
	}

	if(member == InplaceStr("first"))
	{
		if(TypeArgumentSet *argumentsType = getType<TypeArgumentSet>(type))
		{
			if(argumentsType->types.empty())
				Stop(ctx, source->pos, "ERROR: this function type '%.*s' doesn't have arguments", FMT_ISTR(type->name));

			return new ExprTypeLiteral(source, ctx.typeTypeID, argumentsType->types.head->type);
		}

		Stop(ctx, source->pos, "ERROR: 'first' can only be applied to a function type, but we have '%.*s'", FMT_ISTR(type->name));
	}

	if(member == InplaceStr("last"))
	{
		if(TypeArgumentSet *argumentsType = getType<TypeArgumentSet>(type))
		{
			if(argumentsType->types.empty())
				Stop(ctx, source->pos, "ERROR: this function type '%.*s' doesn't have arguments", FMT_ISTR(type->name));

			return new ExprTypeLiteral(source, ctx.typeTypeID, argumentsType->types.tail->type);
		}

		Stop(ctx, source->pos, "ERROR: 'last' can only be applied to a function type, but we have '%.*s'", FMT_ISTR(type->name));
	}

	if(TypeClass *classType = getType<TypeClass>(type))
	{
		for(MatchData *curr = classType->aliases.head; curr; curr = curr->next)
		{
			if(curr->name == member)
				return new ExprTypeLiteral(source, ctx.typeTypeID, curr->type);
		}

		for(MatchData *curr = classType->generics.head; curr; curr = curr->next)
		{
			if(curr->name == member)
				return new ExprTypeLiteral(source, ctx.typeTypeID, curr->type);
		}
	}

	if(TypeStruct *structType = getType<TypeStruct>(type))
	{
		for(VariableHandle *curr = structType->members.head; curr; curr = curr->next)
		{
			if(curr->variable->name == member)
				return new ExprTypeLiteral(source, ctx.typeTypeID, curr->variable->type);
		}

		for(ConstantData *curr = structType->constants.head; curr; curr = curr->next)
		{
			if(curr->name == member)
				return curr->value;
		}
	}

	// hasMember(x)/[n]

	return NULL;
}

ExprBase* CreateMemberAccess(ExpressionContext &ctx, SynBase *source, ExprBase *value, InplaceStr name)
{
	ExprBase* wrapped = value;

	if(TypeRef *refType = getType<TypeRef>(value->type))
	{
		value = new ExprDereference(source, refType->subType, value);

		if(TypeRef *refType = getType<TypeRef>(value->type))
		{
			wrapped = value;

			value = new ExprDereference(source, refType->subType, value);
		}
	}
	else if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
	{
		wrapped = new ExprGetAddress(source, ctx.GetReferenceType(value->type), node->variable);
	}
	else if(ExprDereference *node = getType<ExprDereference>(value))
	{
		wrapped = node->value;
	}
	else if(!isType<TypeRef>(wrapped->type))
	{
		VariableData *storage = AllocateTemporary(ctx, source, wrapped->type);

		CreateAssignment(ctx, source, new ExprVariableAccess(source, wrapped->type, storage), value);

		wrapped = new ExprGetAddress(source, ctx.GetReferenceType(wrapped->type), storage);
	}

	if(TypeArray *node = getType<TypeArray>(value->type))
	{
		if(name == InplaceStr("size"))
			return new ExprIntegerLiteral(source, ctx.typeInt, node->length);

		Stop(ctx, source->pos, "ERROR: array doesn't have member with this name");
	}

	if(isType<TypeRef>(wrapped->type))
	{
		if(ExprTypeLiteral *node = getType<ExprTypeLiteral>(value))
		{
			if(ExprBase *result = CreateTypeidMemberAccess(ctx, source, node->value, name))
				return result;
		}

		if(TypeStruct *node = getType<TypeStruct>(value->type))
		{
			// Search for a member variable
			for(VariableHandle *el = node->members.head; el; el = el->next)
			{
				if(el->variable->name == name)
				{
					// Member access only shifts an address, so we are left with a reference to get value from
					ExprMemberAccess *shift = new ExprMemberAccess(source, ctx.GetReferenceType(el->variable->type), wrapped, el->variable);

					return new ExprDereference(source, el->variable->type, shift);
				}
			}
		}

		// Look for a member function
		unsigned hash = StringHashContinue(value->type->nameHash, "::");

		hash = StringHashContinue(hash, name.begin, name.end);

		if(HashMap<FunctionData*>::Node *function = ctx.functionMap.first(hash))
			return CreateFunctionAccess(ctx, source, function, wrapped);

		// Look for an accessor
		hash = StringHashContinue(hash, "$");

		if(HashMap<FunctionData*>::Node *function = ctx.functionMap.first(hash))
		{
			ExprBase *access = CreateFunctionAccess(ctx, source, function, wrapped);

			return CreateFunctionCall(ctx, source, access, NULL, false);
		}

		// Look for a member function in a generic class base and instantiate them
		if(TypeClass *classType = getType<TypeClass>(value->type))
		{
			if(TypeGenericClassProto *protoType = classType->proto)
			{
				unsigned hash = StringHashContinue(protoType->nameHash, "::");

				hash = StringHashContinue(hash, name.begin, name.end);

				if(HashMap<FunctionData*>::Node *function = ctx.functionMap.first(hash))
					return CreateFunctionAccess(ctx, source, function, wrapped);

				// Look for an accessor
				hash = StringHashContinue(hash, "$");

				if(HashMap<FunctionData*>::Node *function = ctx.functionMap.first(hash))
				{
					ExprBase *access = CreateFunctionAccess(ctx, source, function, wrapped);

					return CreateFunctionCall(ctx, source, access, NULL, false);
				}
			}
		}

		Stop(ctx, source->pos, "ERROR: member variable or function '%.*s' is not defined in class '%.*s'", FMT_ISTR(name), FMT_ISTR(value->type->name));
	}

	Stop(ctx, source->pos, "ERROR: can't access member '%.*s' of type '%.*s'", FMT_ISTR(name), FMT_ISTR(value->type->name));

	return NULL;
}

ExprBase* AnalyzeMemberAccess(ExpressionContext &ctx, SynMemberAccess *syntax)
{
	// It could be a type property
	if(TypeBase *type = AnalyzeType(ctx, syntax->value, false))
	{
		if(ExprBase *result = CreateTypeidMemberAccess(ctx, syntax, type, syntax->member))
			return result;

		Stop(ctx, syntax->pos, "ERROR: unknown member expression type");
	}

	ExprBase* value = AnalyzeExpression(ctx, syntax->value);

	return CreateMemberAccess(ctx, syntax, value, syntax->member);
}

ExprBase* CreateArrayIndex(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<ArgumentData, 32> &arguments)
{
	ExprBase* wrapped = value;

	if(TypeRef *refType = getType<TypeRef>(value->type))
	{
		value = new ExprDereference(source, refType->subType, value);

		if(isType<TypeUnsizedArray>(value->type))
			wrapped = value;
	}
	else if(isType<TypeUnsizedArray>(value->type))
	{
		wrapped = value; // Do not modify
	}
	else if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
	{
		wrapped = new ExprGetAddress(source, ctx.GetReferenceType(value->type), node->variable);
	}
	else if(ExprDereference *node = getType<ExprDereference>(value))
	{
		wrapped = node->value;
	}
	else if(!isType<TypeRef>(wrapped->type))
	{
		VariableData *storage = AllocateTemporary(ctx, source, wrapped->type);

		CreateAssignment(ctx, source, new ExprVariableAccess(source, wrapped->type, storage), value);

		wrapped = new ExprGetAddress(source, ctx.GetReferenceType(wrapped->type), storage);
	}

	if(isType<TypeRef>(wrapped->type) || isType<TypeUnsizedArray>(value->type))
	{
		bool findOverload = arguments.empty() || arguments.size() > 1;

		for(unsigned i = 0; i < arguments.size(); i++)
		{
			if(!arguments[i].name.empty())
				findOverload = true;
		}

		if(ExprBase *overloads = CreateVariableAccess(ctx, source, IntrusiveList<SynIdentifier>(), InplaceStr("[]")))
		{
			SmallArray<ArgumentData, 32> callArguments;
			callArguments.push_back(ArgumentData(wrapped->source, false, InplaceStr(), wrapped->type, wrapped));

			for(unsigned i = 0; i < arguments.size(); i++)
				callArguments.push_back(arguments[i]);

			if(ExprBase *result = CreateFunctionCall(ctx, source, overloads, callArguments, !findOverload))
			{
				if(TypeRef *refType = getType<TypeRef>(result->type))
					return new ExprDereference(source, refType->subType, result);

				return result;
			}
		}

		if(findOverload)
			Stop(ctx, source->pos, "ERROR: overloaded '[]' operator is not available");

		ExprBase *index = CreateCast(ctx, source, arguments[0].value, ctx.typeInt, false);

		ExpressionEvalContext evalCtx(ctx);

		ExprIntegerLiteral *indexValue = getType<ExprIntegerLiteral>(Evaluate(evalCtx, index));

		if(indexValue && indexValue->value < 0)
			Stop(ctx, source->pos, "ERROR: array index cannot be negative");

		if(TypeArray *type = getType<TypeArray>(value->type))
		{
			if(indexValue && indexValue->value >= type->length)
				Stop(ctx, source->pos, "ERROR: array index bounds");

			// Array index only shifts an address, so we are left with a reference to get value from
			ExprArrayIndex *shift = new ExprArrayIndex(source, ctx.GetReferenceType(type->subType), wrapped, index);

			return new ExprDereference(source, type->subType, shift);
		}

		if(TypeUnsizedArray *type = getType<TypeUnsizedArray>(value->type))
		{
			// Array index only shifts an address, so we are left with a reference to get value from
			ExprArrayIndex *shift = new ExprArrayIndex(source, ctx.GetReferenceType(type->subType), wrapped, index);

			return new ExprDereference(source, type->subType, shift);
		}
	}

	Stop(ctx, source->pos, "ERROR: type '%.*s' is not an array", FMT_ISTR(value->type->name));

	return NULL;
}

ExprBase* AnalyzeArrayIndex(ExpressionContext &ctx, SynArrayIndex *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	SmallArray<ArgumentData, 32> arguments;

	for(SynCallArgument *curr = syntax->arguments.head; curr; curr = getType<SynCallArgument>(curr->next))
	{
		ExprBase *index = AnalyzeExpression(ctx, curr->value);

		arguments.push_back(ArgumentData(index->source, false, curr->name, index->type, index));
	}

	return CreateArrayIndex(ctx, syntax, value, arguments);
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

InplaceStr GetTemporaryFunctionName(ExpressionContext &ctx)
{
	char *name = new char[16];
	sprintf(name, "$func%d", ctx.unnamedFuncCount++);

	return InplaceStr(name);
}

InplaceStr GetFunctionName(ExpressionContext &ctx, ScopeData *scope, TypeBase *parentType, InplaceStr name, bool isOperator, bool isAccessor)
{
	if(name.empty())
		return GetTemporaryFunctionName(ctx);

	return GetFunctionNameInScope(scope, parentType, name, isOperator, isAccessor);
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

			if(result[i].type == NULL)
			{
				if(ExprBase *value = argument.value)
					result.push_back(ArgumentData(argument.source, false, InplaceStr(), value->type, value));
			}
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

			if(ExprBase *value = argument.value)
				result.push_back(ArgumentData(argument.source, false, InplaceStr(), value->type, value));
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
						value = new ExprArray(source, ctx.GetArrayType(ctx.typeAutoRef, values.size()), values);

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
					if(actualArgument.value && (isType<TypeFunction>(actualArgument.type) || isType<TypeFunctionSet>(actualArgument.type)))
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

				if(actualArgument.value && (isType<TypeFunction>(actualArgument.type) || isType<TypeFunctionSet>(actualArgument.type)))
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

TypeBase* MatchGenericType(ExpressionContext &ctx, SynBase *source, TypeBase *matchType, TypeBase *argType, IntrusiveList<MatchData> &aliases, bool strict)
{
	if(!matchType->isGeneric)
	{
		if(argType->isGeneric)
		{
			IntrusiveList<MatchData> subAliases;

			if(TypeBase *improved = MatchGenericType(ctx, source, argType, matchType, subAliases, true))
				argType = improved;
		}

		if(matchType == argType)
			return argType;

		if(strict)
			return NULL;

		return matchType;
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
		{
			if(TypeBase *match = MatchGenericType(ctx, source, lhs->subType, rhs->subType, aliases, true))
				return ctx.GetReferenceType(match);

			return NULL;
		}

		if(strict)
			return NULL;

		// 'generic ref' match with 'type' results with 'type ref'
		if(TypeBase *match = MatchGenericType(ctx, source, lhs->subType, argType, aliases, true))
			return ctx.GetReferenceType(match);

		return NULL;
	}

	if(TypeArray *lhs = getType<TypeArray>(matchType))
	{
		// Only match with arrays of the same size
		if(TypeArray *rhs = getType<TypeArray>(argType))
		{
			if(lhs->size == rhs->size)
			{
				if(TypeBase *match = MatchGenericType(ctx, source, lhs->subType, rhs->subType, aliases, true))
					return ctx.GetArrayType(match, lhs->size);

				return NULL;
			}
		}

		return NULL;
	}

	if(TypeUnsizedArray *lhs = getType<TypeUnsizedArray>(matchType))
	{
		// 'generic[]' match with 'type[]' results with 'type[]'
		if(TypeUnsizedArray *rhs = getType<TypeUnsizedArray>(argType))
		{
			if(TypeBase *match = MatchGenericType(ctx, source, lhs->subType, rhs->subType, aliases, true))
				return ctx.GetUnsizedArrayType(match);

			return NULL;
		}

		if(strict)
			return NULL;

		// 'generic[]' match with 'type[N]' results with 'type[]'
		if(TypeArray *rhs = getType<TypeArray>(argType))
		{
			if(TypeBase *match = MatchGenericType(ctx, source, lhs->subType, rhs->subType, aliases, true))
				return ctx.GetUnsizedArrayType(match);
		}

		return NULL;
	}

	if(TypeFunction *lhs = getType<TypeFunction>(matchType))
	{
		// Only match with other function type
		if(TypeFunction *rhs = getType<TypeFunction>(argType))
		{
			TypeBase *returnType = MatchGenericType(ctx, source, lhs->returnType, rhs->returnType, aliases, true);

			if(!returnType)
				return NULL;

			IntrusiveList<TypeHandle> arguments;

			TypeHandle *lhsArg = lhs->arguments.head;
			TypeHandle *rhsArg = rhs->arguments.head;

			while(lhsArg && rhsArg)
			{
				TypeBase *argMatched = MatchGenericType(ctx, source, lhsArg->type, rhsArg->type, aliases, true);

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
				TypeBase *argMatched = MatchGenericType(ctx, source, lhsArg->type, rhsArg->type, aliases, true);

				if(!argMatched)
					return NULL;

				lhsArg = lhsArg->next;
				rhsArg = rhsArg->next;
			}

			return argType;
		}

		return NULL;
	}

	Stop(ctx, source->pos, "ERROR: unknown generic type match");

	return NULL;
}

TypeBase* ResolveGenericTypeAliases(ExpressionContext &ctx, SynBase *source, TypeBase *type, IntrusiveList<MatchData> aliases)
{
	if(!type->isGeneric || aliases.empty())
		return type;

	// Replace with alias type if there is a match, otherwise leave as generic
	if(TypeGeneric *lhs = getType<TypeGeneric>(type))
	{
		if(lhs->name == InplaceStr("generic"))
			return type;

		for(MatchData *curr = aliases.head; curr; curr = curr->next)
		{
			if(curr->name == lhs->name)
				return curr->type;
		}

		return type;
	}

	if(TypeRef *lhs = getType<TypeRef>(type))
		return ctx.GetReferenceType(ResolveGenericTypeAliases(ctx, source, lhs->subType, aliases));

	if(TypeArray *lhs = getType<TypeArray>(type))
		return ctx.GetArrayType(ResolveGenericTypeAliases(ctx, source, lhs->subType, aliases), lhs->size);

	if(TypeUnsizedArray *lhs = getType<TypeUnsizedArray>(type))
		return ctx.GetUnsizedArrayType(ResolveGenericTypeAliases(ctx, source, lhs->subType, aliases));

	if(TypeFunction *lhs = getType<TypeFunction>(type))
	{
		TypeBase *returnType = ResolveGenericTypeAliases(ctx, source, lhs->returnType, aliases);

		IntrusiveList<TypeHandle> arguments;

		for(TypeHandle *curr = lhs->arguments.head; curr; curr = curr->next)
			arguments.push_back(new TypeHandle(ResolveGenericTypeAliases(ctx, source, curr->type, aliases)));

		return ctx.GetFunctionType(returnType, arguments);
	}

	if(TypeGenericClass *lhs = getType<TypeGenericClass>(type))
	{
		bool isGeneric = false;
		IntrusiveList<TypeHandle> types;

		for(TypeHandle *curr = lhs->generics.head; curr; curr = curr->next)
		{
			TypeBase *type = ResolveGenericTypeAliases(ctx, source, curr->type, aliases);

			isGeneric |= type->isGeneric;

			types.push_back(new TypeHandle(type));
		}

		if(isGeneric)
			return new TypeGenericClass(GetGenericClassName(lhs->proto, types), lhs->proto, types);
			
		return CreateGenericTypeInstance(ctx, source, lhs->proto, types);
	}

	Stop(ctx, source->pos, "ERROR: unknown generic type resolve");

	return NULL;
}

TypeBase* MatchArgumentType(ExpressionContext &ctx, SynBase *source, TypeBase *expectedType, TypeBase *actualType, ExprBase *actualValue, IntrusiveList<MatchData> &aliases)
{
	if(actualType->isGeneric)
	{
		if(TypeFunction *target = getType<TypeFunction>(expectedType))
		{
			if(FunctionValue bestOverload = GetFunctionForType(ctx, source, actualValue, target))
				actualType = bestOverload.function->type;
		}

		if(actualType->isGeneric)
			return NULL;
	}

	return MatchGenericType(ctx, source, expectedType, actualType, aliases, false);
}

TypeFunction* GetGenericFunctionInstanceType(ExpressionContext &ctx, SynBase *source, FunctionData *function, SmallArray<ArgumentData, 32> &arguments, IntrusiveList<MatchData> &aliases)
{
	assert(function->arguments.size() == arguments.size());

	IntrusiveList<TypeHandle> types;

	if(SynFunctionDefinition *syntax = function->definition)
	{
		TypeBase *parentType = syntax->parentType ? AnalyzeType(ctx, syntax->parentType) : NULL;

		bool addedParentScope = RestoreParentTypeScope(ctx, source, parentType);

		// Create temporary scope with known arguments for reference in type expression
		ctx.PushTemporaryScope();

		unsigned pos = 0;

		for(SynFunctionArgument *argument = syntax->arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next), pos++)
		{
			bool failed = false;
			TypeBase *expectedType = AnalyzeType(ctx, argument->type, true, &failed);

			if(failed)
				break;

			ArgumentData &actualArgument = arguments[pos];

			TypeBase *type = MatchArgumentType(ctx, argument, expectedType, actualArgument.type, actualArgument.value, aliases);

			if(!type)
				break;

			ctx.AddVariable(new VariableData(argument, ctx.scope, 0, type, argument->name, 0, ctx.uniqueVariableId++));

			types.push_back(new TypeHandle(type));
		}

		ctx.PopScope();

		if(addedParentScope)
			ctx.PopScope();
	}
	else
	{
		if(function->imported)
			Stop(ctx, source->pos, "ERROR: imported generic function call is not supported");

		for(unsigned i = 0; i < function->arguments.size(); i++)
		{
			ArgumentData &funtionArgument = function->arguments[i];

			ArgumentData &actualArgument = arguments[i];

			TypeBase *type = MatchArgumentType(ctx, funtionArgument.source, funtionArgument.type, actualArgument.type, actualArgument.value, aliases);

			if(!type)
				return NULL;

			types.push_back(new TypeHandle(type));
		}
	}

	if(types.size() != arguments.size())
		return NULL;

	return ctx.GetFunctionType(function->type->returnType, types);
}

void StopOnFunctionSelectError(ExpressionContext &ctx, SynBase *source, char* errPos, SmallArray<FunctionValue, 32> &functions)
{
	SmallArray<ArgumentData, 32> arguments;
	SmallArray<unsigned, 32> ratings;

	StopOnFunctionSelectError(ctx, source, errPos, InplaceStr(), functions, arguments, ratings, 0, false);
}

void StopOnFunctionSelectError(ExpressionContext &ctx, SynBase *source, char* errPos, InplaceStr functionName, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, SmallArray<unsigned, 32> &ratings, unsigned bestRating, bool showInstanceInfo)
{
	if(!functionName.empty())
	{
		errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "  %.*s(", FMT_ISTR(functionName));

		for(unsigned i = 0; i < arguments.size(); i++)
			errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "%s%.*s", i != 0 ? ", " : "", FMT_ISTR(arguments[i].type->name));

		errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), ")\n");
	}

	errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), bestRating == ~0u ? " the only available are:\n" : " candidates are:\n");

	for(unsigned i = 0; i < functions.size(); i++)
	{
		FunctionData *function = functions[i].function;

		if(!ratings.empty() && ratings[i] != bestRating)
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
			else if(TypeFunction *instance = GetGenericFunctionInstanceType(ctx, source, function, arguments, aliases))
			{
				GetFunctionRating(ctx, function, instance, result);

				errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), ") instanced to\r\n    %.*s(", FMT_ISTR(function->name));

				TypeHandle *curr = instance->arguments.head;

				for(unsigned k = 0; k < function->arguments.size(); k++)
				{
					ArgumentData &argument = function->arguments[k];

					errPos += SafeSprintf(errPos, ctx.errorBufSize - int(errPos - ctx.errorBuf), "%s%s%.*s", k != 0 ? ", " : "", argument.isExplicit ? "explicit " : "", FMT_ISTR(curr->type->name));

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

	ctx.errorPos = source->pos;

	longjmp(ctx.errorHandler, 1);
}

FunctionValue SelectBestFunction(ExpressionContext &ctx, SynBase *source, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, SmallArray<unsigned, 32> &ratings)
{
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
			TypeFunction *instance = GetGenericFunctionInstanceType(ctx, source, function, result, aliases);

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

	return bestFunction;
}

FunctionValue CreateGenericFunctionInstance(ExpressionContext &ctx, SynBase *source, FunctionValue proto, SmallArray<ArgumentData, 32> &arguments)
{
	FunctionData *function = proto.function;

	SmallArray<ArgumentData, 32> result;

	if(!PrepareArgumentsForFunctionCall(ctx, function->arguments, arguments, result, false))
		assert(!"unexpected");

	IntrusiveList<MatchData> aliases;

	TypeFunction *instance = GetGenericFunctionInstanceType(ctx, source, function, result, aliases);

	assert(instance);
	assert(!instance->isGeneric);

	TypeBase *parentType = NULL;

	if(SynFunctionDefinition *syntax = function->definition)
	{
		if(syntax->parentType)
		{
			assert(function->scope->ownerType);
			assert(isType<TypeRef>(proto.context->type));

			if(TypeRef *type = getType<TypeRef>(proto.context->type))
				parentType = type->subType;
		}
	}

	// Search for an existing functions
	for(unsigned i = 0; i < function->instances.size(); i++)
	{
		FunctionData *data = function->instances[i];

		TypeFunction *testType = instance;

		if(testType->returnType == ctx.typeAuto)
			testType = ctx.GetFunctionType(data->type->returnType, instance->arguments);

		if(data->type == testType)
			return FunctionValue(function->instances[i], proto.context);
	}

	// Switch to original function scope
	ScopeData *scope = ctx.scope;

	ctx.SwitchToScopeAtPoint(NULL, function->scope, function->source);

	ExprBase *expr = NULL;
	
	if(SynFunctionDefinition *syntax = function->definition)
		expr = AnalyzeFunctionDefinition(ctx, syntax, instance, parentType, aliases);
	else if(SynShortFunctionDefinition *node = getType<SynShortFunctionDefinition>(function->declaration->source))
		expr = AnalyzeShortFunctionDefinition(ctx, node, instance);
	else
		Stop(ctx, source->pos, "ERROR: imported generic function call is not supported");

	// Restore old scope
	ctx.SwitchToScopeAtPoint(function->source, scope, NULL);

	ExprFunctionDefinition *definition = getType<ExprFunctionDefinition>(expr);

	assert(definition);

	function->instances.push_back(definition->function);

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

		functions.push_back(FunctionValue(node->function, new ExprNullptrLiteral(source, ctx.GetReferenceType(ctx.typeVoid))));
	}
	else if(ExprGenericFunctionPrototype *node = getType<ExprGenericFunctionPrototype>(function))
	{
		// TODO: function context

		functions.push_back(FunctionValue(node->function, new ExprNullptrLiteral(source, ctx.GetReferenceType(ctx.typeVoid))));
	}
	else if(ExprFunctionOverloadSet *node = getType<ExprFunctionOverloadSet>(function))
	{
		for(FunctionHandle *arg = node->functions.head; arg; arg = arg->next)
			functions.push_back(FunctionValue(arg->function, node->context));
	}
}

ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, bool allowFailure)
{
	SmallArray<ArgumentData, 32> arguments;

	arguments.push_back(ArgumentData(arg0->source, false, InplaceStr(), arg0->type, arg0));

	return CreateFunctionCall(ctx, source, name, arguments, allowFailure);
}

ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, ExprBase *arg1, bool allowFailure)
{
	SmallArray<ArgumentData, 32> arguments;

	arguments.push_back(ArgumentData(arg0->source, false, InplaceStr(), arg0->type, arg0));
	arguments.push_back(ArgumentData(arg1->source, false, InplaceStr(), arg1->type, arg1));

	return CreateFunctionCall(ctx, source, name, arguments, allowFailure);
}

ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, ExprBase *arg0, ExprBase *arg1, ExprBase *arg2, bool allowFailure)
{
	SmallArray<ArgumentData, 32> arguments;

	arguments.push_back(ArgumentData(arg0->source, false, InplaceStr(), arg0->type, arg0));
	arguments.push_back(ArgumentData(arg1->source, false, InplaceStr(), arg1->type, arg1));
	arguments.push_back(ArgumentData(arg2->source, false, InplaceStr(), arg2->type, arg2));

	return CreateFunctionCall(ctx, source, name, arguments, allowFailure);
}

ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, InplaceStr name, SmallArray<ArgumentData, 32> &arguments, bool allowFailure)
{
	if(ExprBase *overloads = CreateVariableAccess(ctx, source, IntrusiveList<SynIdentifier>(), name))
	{
		if(ExprBase *result = CreateFunctionCall(ctx, source, overloads, arguments, allowFailure))
			return result;
	}

	if(!allowFailure)
		Stop(ctx, source->pos, "ERROR: unknown identifier '%.*s'", FMT_ISTR(name));

	return NULL;
}

ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<ArgumentData, 32> &arguments, bool allowFailure)
{
	// Collect a set of available functions
	SmallArray<FunctionValue, 32> functions;

	GetNodeFunctions(ctx, source, value, functions);

	return CreateFunctionCall(ctx, source, value, functions, arguments, allowFailure);
}

ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SynCallArgument *argumentHead, bool allowFailure)
{
	// Collect a set of available functions
	SmallArray<FunctionValue, 32> functions;

	GetNodeFunctions(ctx, source, value, functions);

	return CreateFunctionCall(ctx, source, value, functions, argumentHead, allowFailure);
}

ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<FunctionValue, 32> &functions, SynCallArgument *argumentHead, bool allowFailure)
{
	// Analyze arguments
	SmallArray<ArgumentData, 32> arguments;
	
	for(SynCallArgument *el = argumentHead; el; el = getType<SynCallArgument>(el->next))
	{
		if(functions.empty() && !el->name.empty())
			Stop(ctx, source->pos, "ERROR: function argument names are unknown at this point");

		ExprBase *argument = NULL;

		if(SynShortFunctionDefinition *node = getType<SynShortFunctionDefinition>(el->value))
		{
			SmallArray<ExprBase*, 32> options;

			if(functions.empty())
			{
				if(ExprBase *option = AnalyzeShortFunctionDefinition(ctx, node, value->type, arguments))
					options.push_back(option);
			}
			else
			{
				for(unsigned i = 0; i < functions.size(); i++)
				{
					if(ExprBase *option = AnalyzeShortFunctionDefinition(ctx, node, functions[i].function->type, arguments))
					{
						bool found = false;

						for(unsigned k = 0; k < options.size(); k++)
						{
							if(options[k]->type == option->type)
								found = true;
						}

						if(!found)
							options.push_back(option);
					}
				}
			}

			if(options.empty())
				Stop(ctx, source->pos, "ERROR: cannot find function which accepts a function with %d argument(s) as an argument #%d", node->arguments.size(), arguments.size() + 1);

			if(options.size() == 1)
			{
				argument = options[0];
			}
			else
			{
				IntrusiveList<TypeHandle> types;
				IntrusiveList<FunctionHandle> overloads;

				for(unsigned i = 0; i < options.size(); i++)
				{
					ExprBase *option = options[i];

					assert(isType<ExprFunctionDefinition>(option) || isType<ExprGenericFunctionPrototype>(option));

					types.push_back(new TypeHandle(option->type));

					if(ExprFunctionDefinition *function = getType<ExprFunctionDefinition>(option))
						overloads.push_back(new FunctionHandle(function->function));
					else if(ExprGenericFunctionPrototype *function = getType<ExprGenericFunctionPrototype>(option))
						overloads.push_back(new FunctionHandle(function->function));
				}

				TypeFunctionSet *type = new TypeFunctionSet(GetFunctionSetTypeName(types), types);

				argument = new ExprFunctionOverloadSet(source, type, overloads, new ExprNullptrLiteral(source, ctx.GetReferenceType(ctx.typeVoid)));
			}
		}
		else
		{
			argument = AnalyzeExpression(ctx, el->value);
		}

		arguments.push_back(ArgumentData(el, false, el->name, argument->type, argument));
	}

	return CreateFunctionCall(ctx, source, value, functions, arguments, allowFailure);
}

ExprBase* CreateFunctionCall(ExpressionContext &ctx, SynBase *source, ExprBase *value, SmallArray<FunctionValue, 32> &functions, SmallArray<ArgumentData, 32> &arguments, bool allowFailure)
{
	TypeFunction *type = getType<TypeFunction>(value->type);

	IntrusiveList<ExprBase> actualArguments;

	if(!functions.empty())
	{
		SmallArray<unsigned, 32> ratings;

		FunctionValue bestOverload = SelectBestFunction(ctx, source, functions, arguments, ratings);

		// Didn't find an appropriate function
		if(!bestOverload)
		{
			if(allowFailure)
				return NULL;

			// auto ref -> type cast
			if(isType<ExprTypeLiteral>(value) && arguments.size() == 1 && arguments[0].type == ctx.typeAutoRef && arguments[0].name.empty())
				return CreateCast(ctx, source, arguments[0].value, ((ExprTypeLiteral*)value)->value, true);

			char *errPos = ctx.errorBuf;
			errPos += SafeSprintf(errPos, ctx.errorBufSize, "ERROR: can't find function with following parameters:\n");
			StopOnFunctionSelectError(ctx, source, errPos, functions[0].function->name, functions, arguments, ratings, ~0u, true);
		}

		unsigned bestRating = ~0u;

		for(unsigned i = 0; i < functions.size(); i++)
		{
			if(functions[i].function == bestOverload.function)
				bestRating = ratings[i];
		}

		// Check if multiple functions share the same rating
		for(unsigned i = 0; i < functions.size(); i++)
		{
			if(functions[i].function != bestOverload.function && ratings[i] == bestRating)
			{
				char *errPos = ctx.errorBuf;
				errPos += SafeSprintf(errPos, ctx.errorBufSize, "ERROR: ambiguity, there is more than one overloaded function available for the call:\n");
				StopOnFunctionSelectError(ctx, source, errPos, functions[0].function->name, functions, arguments, ratings, bestRating, true);
			}
		}

		FunctionData *function = bestOverload.function;

		type = getType<TypeFunction>(function->type);

		if(type->isGeneric || (function->scope->ownerType && function->scope->ownerType->isGeneric))
		{
			bestOverload = CreateGenericFunctionInstance(ctx, source, bestOverload, arguments);

			function = bestOverload.function;

			type = getType<TypeFunction>(function->type);
		}

		value = new ExprFunctionAccess(source, function->type, function, bestOverload.context);

		SmallArray<ArgumentData, 32> result;

		PrepareArgumentsForFunctionCall(ctx, function->arguments, arguments, result, true);

		for(unsigned i = 0; i < result.size(); i++)
			actualArguments.push_back(result[i].value);
	}
	else if(type)
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
	else if(isType<ExprTypeLiteral>(value) && arguments.size() == 1 && arguments[0].type == ctx.typeAutoRef && arguments[0].name.empty())
	{
		// auto ref -> type cast
		return CreateCast(ctx, source, arguments[0].value, ((ExprTypeLiteral*)value)->value, true);
	}
	else
	{
		Stop(ctx, source->pos, "ERROR: unknown call");
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

		assert(actual == NULL);
		assert(expected == NULL);
	}

	return new ExprFunctionCall(source, type->returnType, value, actualArguments);
}

ExprBase* AnalyzeFunctionCall(ExpressionContext &ctx, SynFunctionCall *syntax)
{
	ExprBase *function = AnalyzeExpression(ctx, syntax->value);

	if(ExprTypeLiteral *type = getType<ExprTypeLiteral>(function))
	{
		if(ExprBase *regular = CreateVariableAccess(ctx, syntax->value, IntrusiveList<SynIdentifier>(), type->value->name))
		{
			// Collect a set of available functions
			SmallArray<FunctionValue, 32> functions;

			GetNodeFunctions(ctx, syntax, regular, functions);

			return CreateFunctionCall(ctx, syntax, function, functions, syntax->arguments.head, false);
		}
		else
		{
			TypeClass *classType = getType<TypeClass>(type->value);

			VariableData *variable = AllocateTemporary(ctx, syntax, type->value);

			ExprBase *pointer = new ExprGetAddress(syntax, ctx.GetReferenceType(type->value), variable);

			ExprBase *definition = new ExprVariableDefinition(syntax, ctx.typeVoid, variable, NULL);

			unsigned hash = StringHashContinue(type->value->nameHash, "::");

			if(classType)
			{
				InplaceStr functionName = classType->name;

				if(TypeGenericClassProto *proto = classType->proto)
					functionName = proto->name;

				// TODO: add type scopes and lookup owner namespace
				for(const char *pos = functionName.end; pos > functionName.begin; pos--)
				{
					if(*pos == '.')
					{
						functionName = InplaceStr(pos + 1, functionName.end);
						break;
					}
				}

				hash = StringHashContinue(hash, functionName.begin, functionName.end);
			}
			else
			{
				hash = StringHashContinue(hash, type->value->name.begin, type->value->name.end);
			}

			ExprBase *constructor = NULL;

			if(HashMap<FunctionData*>::Node *node = ctx.functionMap.first(hash))
			{
				constructor = CreateFunctionAccess(ctx, syntax, node, pointer);
			}
			else if(classType)
			{
				if(TypeGenericClassProto *proto = classType->proto)
				{
					// Look for a member function in a generic class base and instantiate them
					unsigned hash = StringHashContinue(proto->nameHash, "::");

					hash = StringHashContinue(hash, proto->name.begin, proto->name.end);

					if(HashMap<FunctionData*>::Node *node = ctx.functionMap.first(hash))
						constructor = CreateFunctionAccess(ctx, syntax, node, pointer);
				}
			}

			if(!constructor && syntax->arguments.empty())
				return new ExprVariableAccess(syntax, variable->type, variable);

			if(constructor)
			{
				ExprBase *call = CreateFunctionCall(ctx, syntax, constructor, syntax->arguments.head, false);

				IntrusiveList<ExprBase> expressions;

				expressions.push_back(definition);
				expressions.push_back(call);
				expressions.push_back(new ExprVariableAccess(syntax, variable->type, variable));

				return new ExprSequence(syntax, type->value, expressions);
			}
		}
	}

	if(!syntax->aliases.empty())
		Stop(ctx, syntax->pos, "ERROR: function call with explicit generic arguments is not supported");

	return CreateFunctionCall(ctx, syntax, function, syntax->arguments.head, false);
}

ExprBase* AnalyzeNew(ExpressionContext &ctx, SynNew *syntax)
{
	TypeBase *type = AnalyzeType(ctx, syntax->type);

	ExprBase *size = new ExprIntegerLiteral(syntax, ctx.typeInt, type->size);
	ExprBase *typeId = new ExprTypeCast(syntax, ctx.typeInt, new ExprTypeLiteral(syntax, ctx.typeTypeID, type), EXPR_CAST_REINTERPRET);

	if(syntax->count)
	{
		assert(syntax->arguments.empty());
		assert(syntax->constructor.empty());

		ExprBase *count = AnalyzeExpression(ctx, syntax->count);

		return new ExprTypeCast(syntax, ctx.GetUnsizedArrayType(type), CreateFunctionCall(ctx, syntax, InplaceStr("__newA"), size, count, typeId, false), EXPR_CAST_REINTERPRET);
	}

	ExprBase *alloc = new ExprTypeCast(syntax, ctx.GetReferenceType(type), CreateFunctionCall(ctx, syntax, InplaceStr("__newS"), size, typeId, false), EXPR_CAST_REINTERPRET);

	// Call constructor
	TypeRef *allocType = getType<TypeRef>(alloc->type);

	TypeBase *parentType = allocType->subType;

	unsigned hash = StringHashContinue(parentType->name.hash(), "::");

	if(TypeClass *classType = getType<TypeClass>(parentType))
	{
		InplaceStr functionName = parentType->name;

		if(TypeGenericClassProto *proto = classType->proto)
			functionName = proto->name;

		// TODO: add type scopes and lookup owner namespace
		for(const char *pos = functionName.end; pos > functionName.begin; pos--)
		{
			if(*pos == '.')
			{
				functionName = InplaceStr(pos + 1, functionName.end);
				break;
			}
		}

		hash = StringHashContinue(hash, functionName.begin, functionName.end);
	}
	else
	{
		hash = StringHashContinue(hash, parentType->name.begin, parentType->name.end);
	}

	if(HashMap<FunctionData*>::Node *function = ctx.functionMap.first(hash))
	{
		VariableData *variable = AllocateTemporary(ctx, syntax, alloc->type);

		ExprBase *pointer = new ExprVariableAccess(syntax, variable->type, variable);

		ExprBase *definition = new ExprVariableDefinition(syntax, ctx.typeVoid, variable, CreateAssignment(ctx, syntax, pointer, alloc));

		ExprBase *overloads = CreateFunctionAccess(ctx, syntax, function, pointer);

		ExprBase *call = CreateFunctionCall(ctx, syntax, overloads, syntax->arguments.head, false);

		IntrusiveList<ExprBase> expressions;

		expressions.push_back(definition);
		expressions.push_back(call);
		expressions.push_back(pointer);

		alloc = new ExprSequence(syntax, allocType, expressions);
	}
	else if(syntax->arguments.size() == 1 && syntax->arguments.head->name.empty())
	{
		VariableData *variable = AllocateTemporary(ctx, syntax, alloc->type);

		ExprBase *pointer = new ExprVariableAccess(syntax, variable->type, variable);

		ExprBase *definition = new ExprVariableDefinition(syntax, ctx.typeVoid, variable, CreateAssignment(ctx, syntax, pointer, alloc));

		ExprBase *copy = CreateAssignment(ctx, syntax, new ExprDereference(syntax, parentType, pointer), AnalyzeExpression(ctx, syntax->arguments.head->value));

		IntrusiveList<ExprBase> expressions;

		expressions.push_back(definition);
		expressions.push_back(copy);
		expressions.push_back(pointer);

		alloc = new ExprSequence(syntax, allocType, expressions);
	}
	else if(!syntax->arguments.empty())
	{
		Stop(ctx, syntax->pos, "ERROR: function '%.*s::%.*s' that accepts %d arguments is undefined", FMT_ISTR(parentType->name), FMT_ISTR(parentType->name), syntax->arguments.size());
	}

	// Handle custom constructor
	if(!syntax->constructor.empty())
	{
		VariableData *variable = AllocateTemporary(ctx, syntax, alloc->type);

		ExprBase *pointer = new ExprVariableAccess(syntax, variable->type, variable);

		ExprBase *definition = new ExprVariableDefinition(syntax, ctx.typeVoid, variable, CreateAssignment(ctx, syntax, pointer, alloc));

		// Create a member function with the constructor body
		InplaceStr name = GetTemporaryFunctionName(ctx);

		ExprBase *function = CreateFunctionDefinition(ctx, syntax, false, false, parentType, false, ctx.typeVoid, false, name, IntrusiveList<SynIdentifier>(), IntrusiveList<SynFunctionArgument>(), syntax->constructor, NULL, IntrusiveList<MatchData>());

		ExprFunctionDefinition *functionDefinition = getType<ExprFunctionDefinition>(function);

		// Call this member function
		SmallArray<FunctionValue, 32> functions;
		functions.push_back(FunctionValue(functionDefinition->function, new ExprVariableAccess(syntax, alloc->type, variable)));

		SmallArray<ArgumentData, 32> arguments;

		ExprBase *call = CreateFunctionCall(ctx, syntax, function, functions, arguments, false);

		IntrusiveList<ExprBase> expressions;

		expressions.push_back(definition);
		expressions.push_back(call);
		expressions.push_back(pointer);

		alloc = new ExprSequence(syntax, allocType, expressions);
	}

	return alloc;
}

ExprReturn* AnalyzeReturn(ExpressionContext &ctx, SynReturn *syntax)
{
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

	if(!ctx.IsNumericType(result->type) && !isType<TypeEnum>(result->type))
		Stop(ctx, syntax->pos, "ERROR: global return cannot accept '%.*s'", FMT_ISTR(result->type->name));

	return new ExprReturn(syntax, ctx.typeVoid, result);
}

ExprYield* AnalyzeYield(ExpressionContext &ctx, SynYield *syntax)
{
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

	if(VariableData **variable = ctx.variableMap.find(fullName.hash()))
	{
		if((*variable)->scope == ctx.scope)
			Stop(ctx, syntax->pos, "ERROR: name '%.*s' is already taken for a variable in current scope", FMT_ISTR(syntax->name));
	}

	if(FunctionData **functions = ctx.functionMap.find(fullName.hash()))
	{
		if((*functions)->scope == ctx.scope)
			Stop(ctx, syntax->pos, "ERROR: name '%.*s' is already taken for a function", FMT_ISTR(syntax->name));
	}

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

		if(isType<ExprFunctionOverloadSet>(initializer))
		{
			SmallArray<FunctionValue, 32> functions;

			GetNodeFunctions(ctx, initializer->source, initializer, functions);

			char *errPos = ctx.errorBuf;
			errPos += SafeSprintf(errPos, ctx.errorBufSize, "ERROR: ambiguity, there is more than one overloaded function available:\n");
			StopOnFunctionSelectError(ctx, syntax, errPos, functions);
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

ExprVariableDefinition* CreateMemberFunctionContext(ExpressionContext &ctx, SynBase *source, TypeBase *parent)
{
	TypeBase *type = ctx.GetReferenceType(parent);

	assert(!type->isGeneric);

	unsigned offset = AllocateVariableInScope(ctx.scope, 0, type);
	VariableData *variable = new VariableData(source, ctx.scope, 0, type, InplaceStr("this"), offset, ctx.uniqueVariableId++);

	ctx.AddVariable(variable);

	return new ExprVariableDefinition(source, ctx.typeVoid, variable, NULL);
}

bool RestoreParentTypeScope(ExpressionContext &ctx, SynBase *source, TypeBase *parentType)
{
	if(parentType && ctx.scope->ownerType != parentType)
	{
		ctx.PushScope(parentType);

		if(TypeClass *classType = getType<TypeClass>(parentType))
		{
			for(MatchData *el = classType->generics.head; el; el = el->next)
				ctx.AddAlias(new AliasData(source, ctx.scope, el->type, el->name, ctx.uniqueAliasId++));

			for(MatchData *el = classType->aliases.head; el; el = el->next)
				ctx.AddAlias(new AliasData(source, ctx.scope, el->type, el->name, ctx.uniqueAliasId++));

			for(VariableHandle *el = classType->members.head; el; el = el->next)
				ctx.AddVariable(el->variable);
		}
		else if(TypeGenericClassProto *genericProto = getType<TypeGenericClassProto>(parentType))
		{
			SynClassDefinition *definition = genericProto->definition;

			for(SynIdentifier *curr = definition->aliases.head; curr; curr = getType<SynIdentifier>(curr->next))
				ctx.AddAlias(new AliasData(source, ctx.scope, new TypeGeneric(InplaceStr("generic")), curr->name, ctx.uniqueAliasId++));
		}

		return true;
	}

	return false;
}

void CreateFunctionArgumentVariables(ExpressionContext &ctx, SmallArray<ArgumentData, 32> &arguments, IntrusiveList<ExprVariableDefinition> &variables)
{
	for(unsigned i = 0; i < arguments.size(); i++)
	{
		ArgumentData &argument = arguments[i];

		assert(!argument.type->isGeneric);

		unsigned offset = AllocateVariableInScope(ctx.scope, 0, argument.type);
		VariableData *variable = new VariableData(argument.source, ctx.scope, 0, argument.type, argument.name, offset, ctx.uniqueVariableId++);

		ctx.AddVariable(variable);

		variables.push_back(new ExprVariableDefinition(argument.source, ctx.typeVoid, variable, NULL));
	}
}

ExprBase* AnalyzeFunctionDefinition(ExpressionContext &ctx, SynFunctionDefinition *syntax, TypeFunction *instance, TypeBase *instanceParent, IntrusiveList<MatchData> matches)
{
	TypeBase *parentType = syntax->parentType ? AnalyzeType(ctx, syntax->parentType) : NULL;

	if(instanceParent)
		parentType = instanceParent;

	TypeBase *returnType = AnalyzeType(ctx, syntax->returnType);

	return CreateFunctionDefinition(ctx, syntax, syntax->prototype, syntax->coroutine, parentType, syntax->accessor, returnType, syntax->isOperator, syntax->name, syntax->aliases, syntax->arguments, syntax->expressions, instance, matches);
}

ExprBase* CreateFunctionDefinition(ExpressionContext &ctx, SynBase *source, bool prototype, bool coroutine, TypeBase *parentType, bool accessor, TypeBase *returnType, bool isOperator, InplaceStr(name), IntrusiveList<SynIdentifier> aliases, IntrusiveList<SynFunctionArgument> arguments, IntrusiveList<SynBase> expressions, TypeFunction *instance, IntrusiveList<MatchData> matches)
{
	bool addedParentScope = RestoreParentTypeScope(ctx, source, parentType);

	if(!aliases.empty())
		Stop(ctx, source->pos, "ERROR: functions with explicit generic arguments are not implemented");

	SmallArray<ArgumentData, 32> argData;

	TypeHandle *instanceArg = instance ? instance->arguments.head : NULL;

	bool hadGenericArgument = false;

	for(SynFunctionArgument *argument = arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next))
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
			ctx.PushTemporaryScope();

			unsigned pos = 0;

			for(SynFunctionArgument *prevArg = arguments.head; prevArg && prevArg != argument; prevArg = getType<SynFunctionArgument>(prevArg->next))
			{
				ArgumentData &data = argData[pos++];

				ctx.AddVariable(new VariableData(prevArg, ctx.scope, 0, data.type, data.name, 0, ctx.uniqueVariableId++));
			}

			bool failed = false;
			type = AnalyzeType(ctx, argument->type, true, hadGenericArgument ? &failed : NULL);

			if(type == ctx.typeAuto)
				Stop(ctx, argument->type->pos, "ERROR: function parameter cannot be an auto type");

			if(type == ctx.typeVoid)
				Stop(ctx, argument->type->pos, "ERROR: function parameter cannot be a void type");

			hadGenericArgument |= type->isGeneric;

			// Remove temporary scope
			ctx.PopScope();
		}

		ExprBase *initializer = argument->initializer ? AnalyzeExpression(ctx, argument->initializer) : NULL;

		argData.push_back(ArgumentData(argument, argument->isExplicit, argument->name, type, initializer));
	}

	TypeBase *contextType = parentType ? ctx.GetReferenceType(parentType) : ctx.GetReferenceType(ctx.typeVoid);

	TypeFunction *functionType = ctx.GetFunctionType(returnType, argData);

	if(instance)
		assert(functionType == instance);

	InplaceStr functionName = GetFunctionName(ctx, ctx.scope, parentType ? parentType : ctx.scope->ownerType, name, isOperator, accessor);

	if(VariableData **variable = ctx.variableMap.find(functionName.hash()))
	{
		if((*variable)->scope == ctx.scope)
			Stop(ctx, source->pos, "ERROR: name '%.*s' is already taken for a variable in current scope", FMT_ISTR(name));
	}

	FunctionData *function = new FunctionData(source, ctx.scope, coroutine, accessor, functionType, contextType, functionName, ctx.uniqueFunctionId++);

	function->aliases = matches;

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

		if(prototype)
			Stop(ctx, source->pos, "ERROR: generic function cannot be forward-declared");

		if(addedParentScope)
			ctx.PopScope();

		assert(isType<SynFunctionDefinition>(source));

		function->definition = getType<SynFunctionDefinition>(source);
		function->declaration = new ExprGenericFunctionPrototype(source, function->type, function);

		return function->declaration;
	}

	ctx.PushScope(function);

	function->functionScope = ctx.scope;

	for(MatchData *curr = function->aliases.head; curr; curr = curr->next)
		ctx.AddAlias(new AliasData(source, ctx.scope, curr->type, curr->name, ctx.uniqueAliasId++));

	ExprVariableDefinition *context = NULL;

	if(TypeBase *parent = function->scope->ownerType)
		context = CreateMemberFunctionContext(ctx, source, parent);

	IntrusiveList<ExprVariableDefinition> variables;

	CreateFunctionArgumentVariables(ctx, argData, variables);

	IntrusiveList<ExprBase> code;

	if(prototype)
	{
		if(function->type->returnType == ctx.typeAuto)
			Stop(ctx, source->pos, "ERROR: function prototype with unresolved return type");

		function->isPrototype = true;
	}
	else
	{
		for(SynBase *expression = expressions.head; expression; expression = expression->next)
			code.push_back(AnalyzeStatement(ctx, expression));

		// If the function type is still auto it means that it hasn't returned anything
		if(function->type->returnType == ctx.typeAuto)
			function->type = ctx.GetFunctionType(ctx.typeVoid, function->type->arguments);

		if(function->type->returnType != ctx.typeVoid && !function->hasExplicitReturn)
			Stop(ctx, source->pos, "ERROR: function must return a value of type '%.*s'", FMT_ISTR(returnType->name));
	}

	ctx.PopScope();

	if(addedParentScope)
		ctx.PopScope();

	// If the type was deduced, implement prototype now that it's known
	ImplementPrototype(ctx, function);

	FunctionData *conflict = CheckUniqueness(ctx, function);

	if(conflict)
	{
		if(instance)
		{
			ctx.HideFunction(function);

			return conflict->declaration;
		}

		Stop(ctx, source->pos, "ERROR: function '%.*s' is being defined with the same set of parameters", FMT_ISTR(function->name));
	}

	function->declaration = new ExprFunctionDefinition(source, function->type, function, context, variables, code);

	ctx.definitions.push_back(function->declaration);

	return function->declaration;
}

void DeduceShortFunctionReturnValue(ExpressionContext &ctx, SynBase *source, FunctionData *function, IntrusiveList<ExprBase> &expressions)
{
	if(function->hasExplicitReturn)
		return;

	TypeBase *expected = function->type->returnType;

	if(expected == ctx.typeVoid)
		return;

	TypeBase *actual = expressions.tail->type;

	if(actual == ctx.typeVoid)
		return;

	// If return type is auto, set it to type that is being returned
	if(function->type->returnType == ctx.typeAuto)
		function->type = ctx.GetFunctionType(actual, function->type->arguments);

	ExprBase *result = expected == ctx.typeAuto ? expressions.tail : CreateCast(ctx, source, expressions.tail, expected, false);
	result = new ExprReturn(source, ctx.typeVoid, result);

	if(expressions.head == expressions.tail)
	{
		expressions.head = expressions.tail = result;
	}
	else
	{
		ExprBase *curr = expressions.head;

		while(curr)
		{
			if(curr->next == expressions.tail)
				curr->next = result;

			curr = curr->next;
		}
	}

	function->hasExplicitReturn = true;
}

ExprBase* AnalyzeShortFunctionDefinition(ExpressionContext &ctx, SynShortFunctionDefinition *syntax, TypeFunction *argumentType)
{
	if(syntax->arguments.size() != argumentType->arguments.size())
		return NULL;

	TypeBase *returnType = argumentType->returnType;

	if(returnType->isGeneric)
		returnType = ctx.typeAuto;

	IntrusiveList<MatchData> argCasts;
	SmallArray<ArgumentData, 32> argData;

	TypeHandle *expected = argumentType->arguments.head;

	for(SynShortFunctionArgument *param = syntax->arguments.head; param; param = getType<SynShortFunctionArgument>(param->next))
	{
		TypeBase *type = NULL;

		if(param->type)
			type = AnalyzeType(ctx, param->type);

		if(type)
		{
			if(expected->type->isGeneric)
			{
				IntrusiveList<MatchData> aliases;

				if(TypeBase *match = MatchGenericType(ctx, syntax, expected->type, type, aliases, false))
					argData.push_back(ArgumentData(param, false, param->name, match, NULL));
				else
					return NULL;
			}
			else
			{
				char *name = new char[param->name.length() + 2];

				sprintf(name, "%.*s$", FMT_ISTR(param->name));

				argData.push_back(ArgumentData(param, false, InplaceStr(name), expected->type, NULL));

				argCasts.push_back(new MatchData(param->name, type));
			}
		}
		else
		{
			argData.push_back(ArgumentData(param, false, param->name, expected->type, NULL));
		}

		expected = expected->next;
	}

	InplaceStr functionName = GetFunctionName(ctx, ctx.scope, NULL, InplaceStr(), false, false);

	FunctionData *function = new FunctionData(syntax, ctx.scope, false, false, ctx.GetFunctionType(returnType, argData), ctx.GetReferenceType(ctx.typeVoid), functionName, ctx.uniqueFunctionId++);

	// Fill in argument data
	for(unsigned i = 0; i < argData.size(); i++)
		function->arguments.push_back(argData[i]);

	ctx.AddFunction(function);

	if(function->type->isGeneric)
	{
		function->declaration = new ExprGenericFunctionPrototype(syntax, function->type, function);

		return function->declaration;
	}

	ctx.PushScope(function);

	function->functionScope = ctx.scope;

	IntrusiveList<ExprVariableDefinition> arguments;

	CreateFunctionArgumentVariables(ctx, argData, arguments);

	IntrusiveList<ExprBase> expressions;

	// Create casts of arguments with a wrong type
	for(MatchData *el = argCasts.head; el; el = el->next)
	{
		unsigned offset = AllocateVariableInScope(ctx.scope, el->type->alignment, el->type);
		VariableData *variable = new VariableData(syntax, ctx.scope, el->type->alignment, el->type, el->name, offset, ctx.uniqueVariableId++);

		ctx.AddVariable(variable);

		ExprBase *pointer = new ExprVariableAccess(syntax, variable->type, variable);

		char *name = new char[el->name.length() + 2];

		sprintf(name, "%.*s$", FMT_ISTR(el->name));

		ExprBase *access = CreateVariableAccess(ctx, syntax, IntrusiveList<SynIdentifier>(), InplaceStr(name));

		access = CreateCast(ctx, syntax, access, el->type, true);

		expressions.push_back(new ExprVariableDefinition(syntax, ctx.typeVoid, variable, CreateAssignment(ctx, syntax, pointer, access)));
	}

	for(SynBase *expression = syntax->expressions.head; expression; expression = expression->next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	DeduceShortFunctionReturnValue(ctx, syntax, function, expressions);

	// If the function type is still auto it means that it hasn't returned anything
	if(function->type->returnType == ctx.typeAuto)
		function->type = ctx.GetFunctionType(ctx.typeVoid, function->type->arguments);

	if(function->type->returnType != ctx.typeVoid && !function->hasExplicitReturn)
		Stop(ctx, syntax->pos, "ERROR: function must return a value of type '%.*s'", FMT_ISTR(returnType->name));

	ctx.PopScope();

	// TODO: create captured context

	function->declaration = new ExprFunctionDefinition(syntax, function->type, function, NULL, arguments, expressions);

	ctx.definitions.push_back(function->declaration);

	return function->declaration;
}

ExprBase* AnalyzeShortFunctionDefinition(ExpressionContext &ctx, SynShortFunctionDefinition *syntax, TypeBase *type, SmallArray<ArgumentData, 32> &currArguments)
{
	TypeFunction *functionType = getType<TypeFunction>(type);

	// Only applies to function calls
	if(!functionType)
		return NULL;

	IntrusiveList<TypeHandle> &fuctionArgs = functionType->arguments;

	// Function doesn't accept any more arguments
	if(currArguments.size() + 1 > fuctionArgs.size())
		return NULL;

	// Get current argument type
	TypeBase *target = NULL;

	if(functionType->isGeneric)
	{
		// Collect aliases up to the current argument
		IntrusiveList<MatchData> aliases;

		for(unsigned i = 0; i < currArguments.size(); i++)
		{
			// Exit if the arguments before the short inline function fail to match
			if(!MatchGenericType(ctx, syntax, fuctionArgs[i]->type, currArguments[i].type, aliases, false))
				return NULL;
		}

		target = ResolveGenericTypeAliases(ctx, syntax, fuctionArgs[currArguments.size()]->type, aliases);
	}
	else
	{
		target = fuctionArgs[currArguments.size()]->type;
	}

	TypeFunction *argumentType = getType<TypeFunction>(target);

	if(!argumentType)
		return NULL;

	return AnalyzeShortFunctionDefinition(ctx, syntax, argumentType);
}

void AnalyzeClassStaticIf(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassStaticIf *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	condition = CreateConditionCast(ctx, condition->source, condition);

	ExpressionEvalContext evalCtx(ctx);

	if(ExprBoolLiteral *number = getType<ExprBoolLiteral>(Evaluate(evalCtx, CreateCast(ctx, syntax, condition, ctx.typeBool, false))))
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

void AnalyzeClassConstants(ExpressionContext &ctx, SynBase *source, TypeBase *type, IntrusiveList<SynConstant> constants, IntrusiveList<ConstantData> &target)
{
	for(SynConstant *constant = constants.head; constant; constant = getType<SynConstant>(constant->next))
	{
		ExprBase *value = NULL;
			
		if(constant->value)
		{
			value = AnalyzeExpression(ctx, constant->value);

			if(type == ctx.typeAuto)
				type = value->type;

			if(!ctx.IsNumericType(type))
				Stop(ctx, source->pos, "ERROR: only basic numeric types can be used as constants");

			ExpressionEvalContext evalCtx(ctx);

			value = Evaluate(evalCtx, CreateCast(ctx, constant, value, type, false));
		}
		else if(ctx.IsIntegerType(type) && constant != constants.head)
		{
			ExpressionEvalContext evalCtx(ctx);

			value = getType<ExprIntegerLiteral>(Evaluate(evalCtx, CreateCast(ctx, constant, CreateBinaryOp(ctx, constant, SYN_BINARY_OP_ADD, target.tail->value, new ExprIntegerLiteral(constant, type, 1)), type, false)));
		}
		else
		{
			if(constant == constants.head)
				Stop(ctx, source->pos, "ERROR: '=' not found after constant name");
			else
				Stop(ctx, source->pos, "ERROR: only integer constant list gets automatically incremented by 1");
		}

		if(!isType<ExprCharacterLiteral>(value) && !isType<ExprIntegerLiteral>(value) && !isType<ExprRationalLiteral>(value))
			Stop(ctx, source->pos, "ERROR: expression didn't evaluate to a constant number");

		target.push_back(new ConstantData(constant->name, value));
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

	for(SynConstantSet *constantSet = syntax->constantSets.head; constantSet; constantSet = getType<SynConstantSet>(constantSet->next))
	{
		TypeBase *type = AnalyzeType(ctx, constantSet->type);

		AnalyzeClassConstants(ctx, constantSet, type, constantSet->constants, classDefinition->classType->constants);
	}

	for(SynFunctionDefinition *function = syntax->functions.head; function; function = getType<SynFunctionDefinition>(function->next))
		classDefinition->functions.push_back(AnalyzeFunctionDefinition(ctx, function, NULL, NULL, IntrusiveList<MatchData>()));

	for(SynAccessor *accessor = syntax->accessors.head; accessor; accessor = getType<SynAccessor>(accessor->next))
	{
		SynBase *parentType = new SynTypeSimple(accessor->pos, IntrusiveList<SynIdentifier>(), classDefinition->classType->name);

		if(accessor->getBlock)
		{
			IntrusiveList<SynIdentifier> aliases;
			IntrusiveList<SynFunctionArgument> arguments;

			IntrusiveList<SynBase> expressions = accessor->getBlock->expressions;

			SynFunctionDefinition *function = new SynFunctionDefinition(accessor->pos, false, false, parentType, true, accessor->type, false, accessor->name, aliases, arguments, expressions);

			classDefinition->functions.push_back(AnalyzeFunctionDefinition(ctx, function, NULL, NULL, IntrusiveList<MatchData>()));
		}

		if(accessor->setBlock)
		{
			SynBase *returnType = new SynTypeAuto(accessor->pos);

			IntrusiveList<SynIdentifier> aliases;

			IntrusiveList<SynFunctionArgument> arguments;
			arguments.push_back(new SynFunctionArgument(accessor->pos, false, accessor->type, accessor->setName.empty() ? InplaceStr("r") : accessor->setName, NULL));

			IntrusiveList<SynBase> expressions = accessor->setBlock->expressions;

			SynFunctionDefinition *function = new SynFunctionDefinition(accessor->pos, false, false, parentType, true, returnType, false, accessor->name, aliases, arguments, expressions);

			classDefinition->functions.push_back(AnalyzeFunctionDefinition(ctx, function, NULL, NULL, IntrusiveList<MatchData>()));
		}
	}

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
		TypeGenericClassProto *genericProtoType = new TypeGenericClassProto(syntax, ctx.scope, typeName, syntax);

		ctx.AddType(genericProtoType);

		return new ExprGenericClassPrototype(syntax, ctx.typeVoid, genericProtoType);
	}

	assert(generics.size() == syntax->aliases.size());

	InplaceStr className = generics.empty() ? typeName : GetGenericClassName(proto, generics);

	if(ctx.typeMap.find(className.hash()))
		Stop(ctx, syntax->pos, "ERROR: '%.*s' is being redefined", FMT_ISTR(syntax->name));

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

		Stop(ctx, syntax->pos, "ERROR: inheritance is not supported");
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
	
	TypeClass *classType = new TypeClass(syntax, ctx.scope, className, proto, actualGenerics, syntax->extendable, baseClass);

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

	if(classType->size >= 64 * 1024)
		Stop(ctx, syntax->pos, "ERROR: class size cannot exceed 65535 bytes");

	return classDefinition;
}

void AnalyzeEnumConstants(ExpressionContext &ctx, SynBase *source, TypeBase *type, IntrusiveList<SynConstant> constants, IntrusiveList<ConstantData> &target)
{
	ExprIntegerLiteral *last = NULL;

	for(SynConstant *constant = constants.head; constant; constant = getType<SynConstant>(constant->next))
	{
		ExprIntegerLiteral *value = NULL;
			
		if(constant->value)
		{
			ExpressionEvalContext evalCtx(ctx);

			value = getType<ExprIntegerLiteral>(Evaluate(evalCtx, CreateCast(ctx, constant, AnalyzeExpression(ctx, constant->value), ctx.typeInt, false)));
		}
		else if(last)
		{
			ExpressionEvalContext evalCtx(ctx);

			value = getType<ExprIntegerLiteral>(Evaluate(evalCtx, CreateBinaryOp(ctx, constant, SYN_BINARY_OP_ADD, last, new ExprIntegerLiteral(constant, ctx.typeInt, 1))));
		}
		else
		{
			value = new ExprIntegerLiteral(source, ctx.typeInt, 1);
		}

		if(!value)
			Stop(ctx, source->pos, "ERROR: expression didn't evaluate to a constant number");

		last = value;

		target.push_back(new ConstantData(constant->name, new ExprIntegerLiteral(source, type, value->value)));
	}
}

ExprBase* AnalyzeEnumDefinition(ExpressionContext &ctx, SynEnumDefinition *syntax)
{
	InplaceStr typeName = GetTypeNameInScope(ctx.scope, syntax->name);

	TypeEnum *enumType = new TypeEnum(syntax, ctx.scope, typeName);

	AnalyzeEnumConstants(ctx, syntax, enumType, syntax->values, enumType->constants);

	enumType->alignment = ctx.typeInt->alignment;

	ctx.AddType(enumType);
	
	ScopeData *scope = ctx.scope;

	// Switch to global scope
	ctx.SwitchToScopeAtPoint(NULL, ctx.globalScope, NULL);

	// Create conversion operator int int(enum_type)
	ExprBase *castToInt = NULL;

	{
		SmallArray<ArgumentData, 32> arguments;
		arguments.push_back(ArgumentData(syntax, false, InplaceStr("x"), enumType, NULL));

		FunctionData *function = new FunctionData(syntax, ctx.scope, false, false, ctx.GetFunctionType(ctx.typeInt, arguments), ctx.GetReferenceType(ctx.typeVoid), InplaceStr("int"), ctx.uniqueFunctionId++);

		// Fill in argument data
		for(unsigned i = 0; i < arguments.size(); i++)
			function->arguments.push_back(arguments[i]);

		ctx.AddFunction(function);

		ctx.PushScope(function);

		function->functionScope = ctx.scope;

		IntrusiveList<ExprVariableDefinition> variables;
		CreateFunctionArgumentVariables(ctx, arguments, variables);

		IntrusiveList<ExprBase> expressions;
		expressions.push_back(new ExprReturn(syntax, ctx.typeVoid, new ExprTypeCast(syntax, ctx.typeInt, new ExprVariableAccess(syntax, enumType, variables.tail->variable), EXPR_CAST_REINTERPRET)));

		ctx.PopScope();

		castToInt = new ExprFunctionDefinition(syntax, function->type, function, NULL, variables, expressions);

		ctx.definitions.push_back(castToInt);
	}

	// Create conversion operator enum_type enum_type(int)
	ExprBase *castToEnum = NULL;

	{
		SmallArray<ArgumentData, 32> arguments;
		arguments.push_back(ArgumentData(syntax, false, InplaceStr("x"), ctx.typeInt, NULL));

		FunctionData *function = new FunctionData(syntax, ctx.scope, false, false, ctx.GetFunctionType(enumType, arguments), ctx.GetReferenceType(ctx.typeVoid), typeName, ctx.uniqueFunctionId++);

		// Fill in argument data
		for(unsigned i = 0; i < arguments.size(); i++)
			function->arguments.push_back(arguments[i]);

		ctx.AddFunction(function);

		ctx.PushScope(function);

		function->functionScope = ctx.scope;

		IntrusiveList<ExprVariableDefinition> variables;
		CreateFunctionArgumentVariables(ctx, arguments, variables);

		IntrusiveList<ExprBase> expressions;
		expressions.push_back(new ExprReturn(syntax, ctx.typeVoid, new ExprTypeCast(syntax, enumType, new ExprVariableAccess(syntax, ctx.typeInt, variables.tail->variable), EXPR_CAST_REINTERPRET)));

		ctx.PopScope();

		castToEnum = new ExprFunctionDefinition(syntax, function->type, function, NULL, variables, expressions);

		ctx.definitions.push_back(castToEnum);
	}

	// Restore old scope
	ctx.SwitchToScopeAtPoint(NULL, scope, NULL);

	return new ExprEnumDefinition(syntax, ctx.typeVoid, enumType, castToInt, castToEnum);
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
		ExpressionEvalContext evalCtx(ctx);

		if(ExprBoolLiteral *number = getType<ExprBoolLiteral>(Evaluate(evalCtx, CreateCast(ctx, syntax, condition, ctx.typeBool, false))))
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
	ctx.PushScope();

	ExprBase *initializer = syntax->initializer ? AnalyzeStatement(ctx, syntax->initializer) : new ExprVoid(syntax, ctx.typeVoid);
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);
	ExprBase *increment = syntax->increment ? AnalyzeStatement(ctx, syntax->increment) : new ExprVoid(syntax, ctx.typeVoid);
	ExprBase *body = syntax->body ? AnalyzeStatement(ctx, syntax->body) : new ExprVoid(syntax, ctx.typeVoid);

	condition = CreateConditionCast(ctx, condition->source, condition);

	ctx.PopScope();

	return new ExprFor(syntax, ctx.typeVoid, initializer, condition, increment, body);
}

ExprFor* AnalyzeForEach(ExpressionContext &ctx, SynForEach *syntax)
{
	ctx.PushScope();

	IntrusiveList<ExprBase> initializers;
	IntrusiveList<ExprBase> conditions;
	IntrusiveList<ExprBase> definitions;
	IntrusiveList<ExprBase> increments;

	for(SynForEachIterator *curr = syntax->iterators.head; curr; curr = getType<SynForEachIterator>(curr->next))
	{
		ExprBase *value = AnalyzeExpression(ctx, curr->value);

		TypeBase *type = NULL;

		if(curr->type)
			type = AnalyzeType(ctx, curr->type);

		// Special implementation of for each for built-in arrays
		if(isType<TypeArray>(value->type) || isType<TypeUnsizedArray>(value->type))
		{
			if(!type)
			{
				if(TypeArray *valueType = getType<TypeArray>(value->type))
					type = valueType->subType;
				else if(TypeUnsizedArray *valueType = getType<TypeUnsizedArray>(value->type))
					type = valueType->subType;
			}

			ExprBase* wrapped = value;

			if(ExprVariableAccess *node = getType<ExprVariableAccess>(value))
				wrapped = new ExprGetAddress(value->source, ctx.GetReferenceType(value->type), node->variable);
			else if(ExprDereference *node = getType<ExprDereference>(value))
				wrapped = node->value;

			if(!isType<TypeRef>(wrapped->type))
				Stop(ctx, value->source->pos, "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(value->type->name));

			// Create initializer
			VariableData *iterator = AllocateTemporary(ctx, curr, ctx.typeInt);

			ctx.AddVariable(iterator);

			ExprBase *iteratorValue = new ExprVariableAccess(curr, ctx.typeInt, iterator);
			ExprBase *iteratorAssignment = CreateAssignment(ctx, curr, iteratorValue, new ExprIntegerLiteral(curr, ctx.typeInt, 0));

			initializers.push_back(new ExprVariableDefinition(curr, ctx.typeVoid, iterator, iteratorAssignment));

			// Create condition
			conditions.push_back(CreateBinaryOp(ctx, curr, SYN_BINARY_OP_LESS, iteratorValue, CreateMemberAccess(ctx, curr, value, InplaceStr("size"))));

			// Create definition
			type = ctx.GetReferenceType(type);
			unsigned variableOffset = AllocateVariableInScope(ctx.scope, type->alignment, type);
			VariableData *variable = new VariableData(curr, ctx.scope, type->alignment, type, curr->name, variableOffset, ctx.uniqueVariableId++);

			variable->isReference = true;

			ctx.AddVariable(variable);

			ExprBase *variableValue = new ExprVariableAccess(curr, type, variable);

			SmallArray<ArgumentData, 32> arguments;
			arguments.push_back(ArgumentData(iteratorValue->source, false, InplaceStr(), iteratorValue->type, iteratorValue));

			ExprBase *arrayIndex = CreateArrayIndex(ctx, curr, value, arguments);

			assert(isType<ExprDereference>(arrayIndex));

			if(ExprDereference *node = getType<ExprDereference>(arrayIndex))
				arrayIndex = node->value;

			ExprBase *variableAssignment = CreateAssignment(ctx, curr, variableValue, arrayIndex);

			definitions.push_back(new ExprVariableDefinition(curr, ctx.typeVoid, variable, variableAssignment));

			// Create increment
			increments.push_back(new ExprPreModify(curr, ctx.typeInt, new ExprGetAddress(curr, ctx.GetReferenceType(ctx.typeInt), iterator), true));
		}
		else
		{
			Stop(ctx, value->source->pos, "ERROR: iterating over '%.*s' is not supported", FMT_ISTR(value->type->name));
		}
	}

	ExprBase *initializer = new ExprBlock(syntax, ctx.typeVoid, initializers);

	ExprBase *condition = NULL;

	for(ExprBase *curr = conditions.head; curr; curr = curr->next)
	{
		if(!condition)
			condition = curr;
		else
			condition = CreateBinaryOp(ctx, syntax, SYN_BINARY_OP_LOGICAL_AND, condition, curr);
	}

	ExprBase *increment = new ExprBlock(syntax, ctx.typeVoid, increments);

	if(syntax->body)
		definitions.push_back(AnalyzeStatement(ctx, syntax->body));

	ExprBase *body = new ExprBlock(syntax, ctx.typeVoid, definitions);

	ctx.PopScope();

	return new ExprFor(syntax, ctx.typeVoid, initializer, condition, increment, body);
}

ExprWhile* AnalyzeWhile(ExpressionContext &ctx, SynWhile *syntax)
{
	ctx.PushScope();

	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);
	ExprBase *body = syntax->body ? AnalyzeStatement(ctx, syntax->body) : new ExprVoid(syntax, ctx.typeVoid);

	condition = CreateConditionCast(ctx, condition->source, condition);

	ctx.PopScope();

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

	ctx.PopScope();

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
		if(TypeBase *type = AnalyzeType(ctx, node->value, false))
			return new ExprIntegerLiteral(node, ctx.typeInt, type->size);

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

	if(SynShortFunctionDefinition *node = getType<SynShortFunctionDefinition>(syntax))
	{
		Stop(ctx, syntax->pos, "ERROR: cannot infer type for inline function outside of the function call");
	}

	if(SynTypeReference *node = getType<SynTypeReference>(syntax))
		return new ExprTypeLiteral(node, ctx.typeTypeID, AnalyzeType(ctx, syntax));

	if(SynTypeFunction *node = getType<SynTypeFunction>(syntax))
		return new ExprTypeLiteral(node, ctx.typeTypeID, AnalyzeType(ctx, syntax));

	if(SynTypeGenericInstance *node = getType<SynTypeGenericInstance>(syntax))
		return new ExprTypeLiteral(node, ctx.typeTypeID, AnalyzeType(ctx, syntax));

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

	if(SynEnumDefinition *node = getType<SynEnumDefinition>(syntax))
	{
		return AnalyzeEnumDefinition(ctx, node);
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

	if(SynForEach *node = getType<SynForEach>(syntax))
	{
		return AnalyzeForEach(ctx, node);
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
	ExternTypedefInfo *aliasList = FindFirstTypedef(bCode);

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
				InplaceStr className = InplaceStr(symbols + type.offsetToName);

				TypeBase *importedType = NULL;

				if(type.namespaceHash != ~0u)
					Stop(ctx, source->pos, "ERROR: can't import namespace type");

				if(type.constantCount != 0)
					Stop(ctx, source->pos, "ERROR: can't import constants of type");

				if(type.definitionOffset != ~0u && type.definitionOffset & 0x80000000)
				{
					TypeBase *proto = module.types[type.definitionOffset & ~0x80000000];

					if(!proto)
						Stop(ctx, source->pos, "ERROR: can't find proto type for '%s' in module %s", symbols + type.offsetToName, module.name);

					TypeGenericClassProto *protoClass = getType<TypeGenericClassProto>(proto);

					if(!protoClass)
						Stop(ctx, source->pos, "ERROR: can't find correct proto type for '%s' in module %s", symbols + type.offsetToName, module.name);

					// Find all generics for this type
					bool isGeneric = false;
					IntrusiveList<TypeHandle> generics;
					IntrusiveList<MatchData> actualGenerics;

					for(unsigned k = 0; k < bCode->typedefCount; k++)
					{
						ExternTypedefInfo &alias = aliasList[k];

						InplaceStr aliasName = InplaceStr(symbols + alias.offsetToName);

						TypeBase *targetType = module.types[alias.targetType];

						if(!targetType)
							Stop(ctx, source->pos, "ERROR: can't find alias '%s' target type in module %s", symbols + alias.offsetToName, module.name);

						if(alias.parentType == i)
						{
							isGeneric |= targetType->isGeneric;
							generics.push_back(new TypeHandle(targetType));
							actualGenerics.push_back(new MatchData(aliasName, targetType));
						}
					}

					if(isGeneric)
					{
						importedType = new TypeGenericClass(className, protoClass, generics);
					}
					else
					{
						TypeClass *classType = new TypeClass(source, ctx.scope, className, protoClass, actualGenerics, false, NULL);

						classType->imported = true;

						importedType = classType;

						ctx.AddType(importedType);
					}
				}
				else if(type.definitionOffsetStart != ~0u)
				{
					Lexeme *start = type.definitionOffsetStart + module.lexer.GetStreamStart();

					ParseContext pCtx;

					pCtx.currentLexeme = start;

					SynClassDefinition *definition = getType<SynClassDefinition>(ParseClassDefinition(pCtx));

					if(!definition)
						Stop(ctx, source->pos, "ERROR: failed to import generic class body");

					definition->imported = true;

					importedType = new TypeGenericClassProto(source, ctx.scope, className, definition);

					ctx.AddType(importedType);
				}
				else
				{
					IntrusiveList<MatchData> actualGenerics;

					TypeClass *classType = new TypeClass(source, ctx.scope, className, NULL, actualGenerics, false, NULL);

					classType->imported = true;

					importedType = classType;

					ctx.AddType(importedType);
				}

				module.types[i] = importedType;

				importedType->alignment = type.defaultAlign;
				importedType->size = type.size;

				const char *memberNames = className.end + 1;

				if(TypeClass *classType = getType<TypeClass>(importedType))
				{
					ctx.PushScope(importedType);

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

		data->imported = true;

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
			else if(!isType<TypeGenericClass>(parentType) && !isType<TypeGenericClassProto>(parentType))
			{
				Stop(ctx, source->pos, "ERROR: can't import class alias");
			}
		}
		else
		{
			AliasData *alias = new AliasData(source, ctx.scope, targetType, aliasName, ctx.uniqueAliasId++);

			alias->imported = true;

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

		if(!contextType)
			contextType = ctx.GetReferenceType(ctx.typeVoid);

		if(function.explicitTypeCount != 0)
			Stop(ctx, source->pos, "ERROR: can't import generic function with explicit arguments");

		InplaceStr functionName = InplaceStr(symbols + function.offsetToName);

		bool coroutine = function.funcCat == ExternFuncInfo::COROUTINE;
		bool accessor = *(functionName.end - 1) == '$';

		if(parentType)
			ctx.PushScope(parentType);

		FunctionData *data = new FunctionData(source, ctx.scope, coroutine, accessor, getType<TypeFunction>(functionType), contextType, functionName, ctx.uniqueFunctionId++);

		data->imported = true;

		ctx.AddFunction(data);

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
			Lexeme *start = function.genericOffsetStart + module.lexer.GetStreamStart();

			ParseContext pCtx;

			pCtx.currentLexeme = start;

			SynFunctionDefinition *definition = ParseFunctionDefinition(pCtx);

			if(!definition)
				Stop(ctx, source->pos, "ERROR: failed to import generic functions body");

			data->definition = definition;

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

	ctx.baseModuleFunctionCount = ctx.functions.size();

	if(SynModule *node = getType<SynModule>(syntax))
	{
		for(SynModuleImport *import = node->imports.head; import; import = getType<SynModuleImport>(import->next))
			AnalyzeModuleImport(ctx, import);

		IntrusiveList<ExprBase> expressions;

		for(SynBase *expr = node->expressions.head; expr; expr = expr->next)
			expressions.push_back(AnalyzeStatement(ctx, expr));

		ExprModule *module = new ExprModule(syntax, ctx.typeVoid, ctx.globalScope, expressions);

		for(unsigned i = 0; i < ctx.definitions.size(); i++)
			module->definitions.push_back(ctx.definitions[i]);

		return module;
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
	ctx.typeAutoRef->members.push_back(new VariableHandle(AllocateClassMember(ctx, syntax, ctx.typeTypeID, InplaceStr("type"), ctx.uniqueVariableId++)));
	ctx.typeAutoRef->members.push_back(new VariableHandle(AllocateClassMember(ctx, syntax, ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"), ctx.uniqueVariableId++)));
	ctx.PopScope();

	ctx.AddType(ctx.typeAutoArray = new TypeAutoArray(InplaceStr("auto[]")));
	ctx.PushScope(ctx.typeAutoArray);
	ctx.typeAutoArray->members.push_back(new VariableHandle(AllocateClassMember(ctx, syntax, ctx.typeTypeID, InplaceStr("type"), ctx.uniqueVariableId++)));
	ctx.typeAutoArray->members.push_back(new VariableHandle(AllocateClassMember(ctx, syntax, ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"), ctx.uniqueVariableId++)));
	ctx.typeAutoArray->members.push_back(new VariableHandle(AllocateClassMember(ctx, syntax, ctx.typeInt, InplaceStr("size"), ctx.uniqueVariableId++)));
	ctx.PopScope();

	// Analyze module
	if(!setjmp(ctx.errorHandler))
	{
		ExprBase *module = AnalyzeModule(ctx, syntax);

		ctx.PopScope();

		assert(ctx.scope == NULL);

		return module;
	}

	return NULL;
}
