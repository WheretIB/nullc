#include "ExpressionTree.h"

#include "ParseClass.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

namespace
{
	jmp_buf errorHandler;

	void Stop(ExpressionContext &ctx, const char *pos, const char *msg, va_list args)
	{
		ctx.errorPos = pos;

		char errorText[4096];

		vsnprintf(errorText, 4096, msg, args);
		errorText[4096 - 1] = '\0';

		ctx.errorMsg = InplaceStr(errorText);

		longjmp(errorHandler, 1);
	}

	void Stop(ExpressionContext &ctx, const char *pos, const char *msg, ...)
	{
		va_list args;
		va_start(args, msg);

		Stop(ctx, pos, msg, args);
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

	bool IsIntegerType(ExpressionContext &ctx, TypeBase* type)
	{
		if(type == ctx.typeBool)
			return true;

		if(type == ctx.typeChar)
			return true;

		if(type == ctx.typeShort)
			return true;

		if(type == ctx.typeInt)
			return true;

		if(type == ctx.typeLong)
			return true;

		return false;
	}

	bool IsFloatingPointType(ExpressionContext &ctx, TypeBase* type)
	{
		if(type == ctx.typeFloat)
			return true;

		if(type == ctx.typeDouble)
			return true;

		return false;
	}

	bool IsNumericType(ExpressionContext &ctx, TypeBase* type)
	{
		return IsIntegerType(ctx, type) || IsFloatingPointType(ctx, type);
	}

	TypeBase* GetBinaryOpResultType(ExpressionContext &ctx, TypeBase* a, TypeBase* b)
	{
		if(a == ctx.typeDouble || b == ctx.typeDouble)
			return ctx.typeDouble;

		if(a == ctx.typeFloat || b == ctx.typeFloat)
			return ctx.typeFloat;

		if(a == ctx.typeLong || b == ctx.typeLong)
			return ctx.typeLong;

		if(a == ctx.typeInt || b == ctx.typeInt)
			return ctx.typeInt;

		if(a == ctx.typeShort || b == ctx.typeShort)
			return ctx.typeShort;

		if(a == ctx.typeChar || b == ctx.typeChar)
			return ctx.typeChar;

		if(a == ctx.typeBool || b == ctx.typeBool)
			return ctx.typeBool;

		return NULL;
	}

	bool IsBinaryOp(SynUnaryOpType type)
	{
		return type == SYN_UNARY_OP_BIT_NOT;
	}

	bool IsLogicalOp(SynUnaryOpType type)
	{
		return type == SYN_UNARY_OP_LOGICAL_NOT;
	}

	const char* GetOpName(SynUnaryOpType type)
	{
		switch(type)
		{
		case SYN_UNARY_OP_PLUS:
			return "+";
		case SYN_UNARY_OP_NEGATE:
			return "-";
		case SYN_UNARY_OP_BIT_NOT:
			return "~";
		case SYN_UNARY_OP_LOGICAL_NOT:
			return "!";
		}

		assert(!"unknown operation type");
		return "";
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

	const char* GetOpName(SynBinaryOpType type)
	{
		switch(type)
		{
		case SYN_BINARY_OP_ADD:
			return "+";
		case SYN_BINARY_OP_SUB:
			return "-";
		case SYN_BINARY_OP_MUL:
			return "*";
		case SYN_BINARY_OP_DIV:
			return "/";
		case SYN_BINARY_OP_MOD:
			return "%";
		case SYN_BINARY_OP_POW:
			return "**";
		case SYN_BINARY_OP_SHL:
			return "<<";
		case SYN_BINARY_OP_SHR:
			return ">>";
		case SYN_BINARY_OP_LESS:
			return "<";
		case SYN_BINARY_OP_LESS_EQUAL:
			return "<=";
		case SYN_BINARY_OP_GREATER:
			return ">";
		case SYN_BINARY_OP_GREATER_EQUAL:
			return ">=";
		case SYN_BINARY_OP_EQUAL:
			return "==";
		case SYN_BINARY_OP_NOT_EQUAL:
			return "!=";
		case SYN_BINARY_OP_BIT_AND:
			return "&";
		case SYN_BINARY_OP_BIT_OR:
			return "|";
		case SYN_BINARY_OP_BIT_XOR:
			return "^";
		case SYN_BINARY_OP_LOGICAL_AND:
			return "&&";
		case SYN_BINARY_OP_LOGICAL_OR:
			return "||";
		case SYN_BINARY_OP_LOGICAL_XOR:
			return "^^";
		case SYN_BINARY_OP_IN:
			return "in";
		}

		assert(!"unknown operation type");
		return "";
	}
}

ExpressionContext::ExpressionContext()
{
	errorPos = NULL;

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

	genericTypeMap.init();
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

	delete scopes.back();
	scopes.pop_back();
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

	result->members.push_back(new VariableHandle(new VariableData(scopes.back(), 4, typeInt, InplaceStr("size"))));

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

	if(IsNumericType(ctx, value->type) && IsNumericType(ctx, value->type))
		return new ExprTypeCast(type, value);

	if(type == ctx.typeBool)
	{
		if(isType<TypeRef>(value->type))
			return new ExprTypeCast(type, value);

		if(isType<TypeUnsizedArray>(value->type))
			return new ExprTypeCast(type, value);

		if(isType<TypeFunction>(value->type))
			return new ExprTypeCast(type, value);
	}

	Stop(ctx, pos, "ERROR: can't convert '%.*s' to '%.*s'", FMT_ISTR(value->type->name), FMT_ISTR(type->name));

	return NULL;
}

ExprBase* AnalyzeNumber(ExpressionContext &ctx, SynNumber *syntax);
ExprBase* AnalyzeExpression(ExpressionContext &ctx, SynBase *syntax);
ExprBase* AnalyzeStatement(ExpressionContext &ctx, SynBase *syntax);
ExprBlock* AnalyzeBlock(ExpressionContext &ctx, SynBlock *syntax, bool createScope);
ExprBase* AnalyzeClassDefinition(ExpressionContext &ctx, SynClassDefinition *syntax, TypeGenericClassProto *proto, IntrusiveList<TypeHandle> generics);
void AnalyzeClassElements(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassElements *syntax);

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

		if(isType<TypeAuto>(type))
		{
			if(node->size)
				Stop(ctx, syntax->pos, "ERROR: no size allowed");

			return ctx.typeAutoArray;
		}

		if(!node->size)
			return ctx.GetUnsizedArrayType(type);

		ExprBase *size = AnalyzeExpression(ctx, node->size);
		
		// TODO: replace with compile-time evaluation
		if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(size))
			return ctx.GetArrayType(type, number->value);

		Stop(ctx, syntax->pos, "ERROR: can't get array size");
	}

	if(SynArrayIndex *node = getType<SynArrayIndex>(syntax))
	{
		TypeBase *type = AnalyzeType(ctx, node->value, onlyType);

		if(!onlyType && !type)
			return NULL;

		if(isType<TypeAuto>(type))
		{
			if(!node->arguments.empty())
				Stop(ctx, syntax->pos, "ERROR: no size allowed");

			return ctx.typeAutoArray;
		}

		if(node->arguments.empty())
			return ctx.GetUnsizedArrayType(type);

		if(node->arguments.size() > 1)
			Stop(ctx, syntax->pos, "ERROR: ',' is not expected in array type size");

		ExprBase *size = AnalyzeExpression(ctx, node->arguments.head);
		
		// TODO: replace with compile-time evaluation
		if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(size))
			return ctx.GetArrayType(type, number->value);

		Stop(ctx, syntax->pos, "ERROR: can't get array size");
	}

	if(SynTypeFunction *node = getType<SynTypeFunction>(syntax))
	{
		TypeBase *returnType = AnalyzeType(ctx, node->returnType);

		IntrusiveList<TypeHandle> arguments;

		for(SynBase *el = node->arguments.head; el; el = el->next)
			arguments.push_back(new TypeHandle(AnalyzeType(ctx, el)));

		return ctx.GetFunctionType(returnType, arguments);
	}

	if(SynTypeof *node = getType<SynTypeof>(syntax))
	{
		ExprBase *value = AnalyzeExpression(ctx, node->value);

		return value->type;
	}

	if(SynTypeSimple *node = getType<SynTypeSimple>(syntax))
	{
		if(node->path.empty())
		{
			// TODO: current namespace

			if(TypeBase **type = ctx.typeMap.find(GetStringHash(node->name.begin, node->name.end)))
				return *type;
		}

		// TODO: namespace path

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

	if(ExprIntegerLiteral *alignValue = getType<ExprIntegerLiteral>(align))
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
			return new ExprIntegerLiteral(ctx.typeInt, num);

		return new ExprIntegerLiteral(ctx.typeLong, num);
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
				return new ExprIntegerLiteral(ctx.typeInt, num);

			return new ExprIntegerLiteral(ctx.typeLong, num);
		}
		else if(syntax->suffix == InplaceStr("l"))
		{
			long long num = ParseLong(ctx, value.begin, value.end, 10);

			return new ExprIntegerLiteral(ctx.typeLong, num);
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
				return new ExprIntegerLiteral(ctx.typeInt, num);

			return new ExprIntegerLiteral(ctx.typeLong, num);
		}

		long long num = ParseLong(ctx, value.begin, value.end, 10);

		if(int(num) == num)
			return new ExprIntegerLiteral(ctx.typeInt, num);

		Stop(ctx, value.begin, "ERROR: overflow in decimal constant");
	}
		
	if(syntax->suffix == InplaceStr("f"))
	{
		double num = ParseDouble(ctx, value.begin);

		return new ExprRationalLiteral(ctx.typeFloat, float(num));
	}
	else if(!syntax->suffix.empty())
	{
		Stop(ctx, syntax->suffix.begin, "ERROR: unknown number suffix '%.*s'", syntax->suffix.length(), syntax->suffix.begin);
	}

	double num = ParseDouble(ctx, value.begin);

	return new ExprRationalLiteral(ctx.typeDouble, num);
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

	return new ExprArray(ctx.GetArrayType(subType, values.size()), values);
}

ExprBase* AnalyzeVariableAccess(ExpressionContext &ctx, InplaceStr name)
{
	if(VariableData **variable = ctx.variableMap.find(GetStringHash(name.begin, name.end)))
	{
		VariableData *data = *variable;

		// TODO: check external variable access

		if(data->type == ctx.typeAuto)
			Stop(ctx, name.begin, "ERROR: variable '%.*s' is being used while its type is unknown", FMT_ISTR(name));

		return new ExprVariableAccess(data->type, data);
	}

	if(FunctionData **function = ctx.functionMap.find(GetStringHash(name.begin, name.end)))
	{
		return new ExprFunctionAccess((*function)->type, *function);
	}

	// TODO: 'this' pointer

	Stop(ctx, name.begin, "ERROR: unknown variable");

	return NULL;
}

ExprBase* AnalyzeVariableAccess(ExpressionContext &ctx, SynIdentifier *syntax)
{
	return AnalyzeVariableAccess(ctx, syntax->name);
}

ExprBase* AnalyzeVariableAccess(ExpressionContext &ctx, SynTypeSimple *syntax)
{
	if(syntax->path.empty())
	{
		// TODO: current namespace

		return AnalyzeVariableAccess(ctx, syntax->name);
	}

	// TODO: namespace path

	Stop(ctx, syntax->pos, "ERROR: unknown namespaced variable");

	return NULL;
}

ExprPreModify* AnalyzePreModify(ExpressionContext &ctx, SynPreModify *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	return new ExprPreModify(value->type, value, syntax->isIncrement);
}

ExprPostModify* AnalyzePostModify(ExpressionContext &ctx, SynPostModify *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	return new ExprPostModify(value->type, value, syntax->isIncrement);
}

ExprUnaryOp* AnalyzeUnaryOp(ExpressionContext &ctx, SynUnaryOp *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	// TODO: check operator overload

	bool binaryOp = IsBinaryOp(syntax->type);
	bool logicalOp = IsLogicalOp(syntax->type);

	// Type check
	if(IsFloatingPointType(ctx, value->type))
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
	else if(!IsNumericType(ctx, value->type))
	{
		Stop(ctx, syntax->pos, "ERROR: unary operation '%s' is not supported on '%.*s'", GetOpName(syntax->type), FMT_ISTR(value->type->name));
	}

	TypeBase *resultType = NULL;

	if(logicalOp)
		resultType = ctx.typeBool;
	else
		resultType = value->type;

	return new ExprUnaryOp(resultType, syntax->type, value);
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

	if(lhs->type == ctx.typeVoid)
		Stop(ctx, syntax->pos, "ERROR: first operand type is 'void'");

	if(rhs->type == ctx.typeVoid)
		Stop(ctx, syntax->pos, "ERROR: second operand type is 'void'");

	bool binaryOp = IsBinaryOp(syntax->type);
	bool comparisonOp = IsComparisonOp(syntax->type);
	bool logicalOp = IsLogicalOp(syntax->type);

	if(IsFloatingPointType(ctx, lhs->type) || IsFloatingPointType(ctx, rhs->type))
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
	else if(IsNumericType(ctx, lhs->type) && IsNumericType(ctx, rhs->type))
	{
		// Numeric operations promote both operands to a common type
		TypeBase *commonType = GetBinaryOpResultType(ctx, lhs->type, rhs->type);

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

	return new ExprBinaryOp(resultType, syntax->type, lhs, rhs);
}

ExprGetAddress* AnalyzeGetAddress(ExpressionContext &ctx, SynGetAddress *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	return new ExprGetAddress(ctx.GetReferenceType(value->type), value);
}

ExprDereference* AnalyzeDereference(ExpressionContext &ctx, SynDereference *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	if(TypeRef *type = getType<TypeRef>(value->type))
	{
		return new ExprDereference(type->subType, value);
	}

	Stop(ctx, syntax->pos, "ERROR: cannot dereference type '%.*s' that is not a pointer", FMT_ISTR(value->type->name));

	return NULL;
}

ExprConditional* AnalyzeConditional(ExpressionContext &ctx, SynConditional *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	ExprBase *trueBlock = AnalyzeStatement(ctx, syntax->trueBlock);
	ExprBase *falseBlock = AnalyzeStatement(ctx, syntax->falseBlock);

	TypeBase *resultType = NULL;

	if(trueBlock->type == falseBlock->type)
		resultType = trueBlock->type;
	else if(IsNumericType(ctx, trueBlock->type) && IsNumericType(ctx, falseBlock->type))
		resultType = GetBinaryOpResultType(ctx, trueBlock->type, falseBlock->type);
	else
		Stop(ctx, syntax->pos, "ERROR: Unknown common type");

	return new ExprConditional(resultType, condition, trueBlock, falseBlock);
}

ExprAssignment* AnalyzeAssignment(ExpressionContext &ctx, SynAssignment *syntax)
{
	ExprBase *lhs = AnalyzeExpression(ctx, syntax->lhs);
	ExprBase *rhs = AnalyzeExpression(ctx, syntax->rhs);

	return new ExprAssignment(lhs->type, lhs, rhs);
}

ExprModifyAssignment* AnalyzeModifyAssignment(ExpressionContext &ctx, SynModifyAssignment *syntax)
{
	ExprBase *lhs = AnalyzeExpression(ctx, syntax->lhs);
	ExprBase *rhs = AnalyzeExpression(ctx, syntax->rhs);

	return new ExprModifyAssignment(lhs->type, syntax->type, lhs, rhs);
}

ExprBase* AnalyzeMemberAccess(ExpressionContext &ctx, SynMemberAccess *syntax)
{
	// It could be a type property
	if(TypeBase *type = AnalyzeType(ctx, syntax->value, false))
	{
		if(syntax->member == InplaceStr("isReference"))
		{
			return new ExprBoolLiteral(ctx.typeBool, isType<TypeRef>(type));
		}

		if(syntax->member == InplaceStr("isArray"))
		{
			return new ExprBoolLiteral(ctx.typeBool, isType<TypeArray>(type) || isType<TypeUnsizedArray>(type));
		}

		if(syntax->member == InplaceStr("isFunction"))
		{
			return new ExprBoolLiteral(ctx.typeBool, isType<TypeFunction>(type));
		}

		if(syntax->member == InplaceStr("arraySize"))
		{
			if(TypeArray *arrType = getType<TypeArray>(type))
				return new ExprIntegerLiteral(ctx.typeInt, arrType->length);
				
			if(TypeUnsizedArray *arrType = getType<TypeUnsizedArray>(type))
				return new ExprIntegerLiteral(ctx.typeInt, -1);

			Stop(ctx, syntax->pos, "ERROR: 'arraySize' can only be applied to an array type, but we have '%s'", type->name);
		}

		Stop(ctx, syntax->pos, "ERROR: unknown expression type");
		// isReference/isArray/isFunction/arraySize/hasMember(x)/class constant/class typedef
	}

	ExprBase* value = AnalyzeExpression(ctx, syntax->value);

	if(TypeArray *node = getType<TypeArray>(value->type))
	{
		if(syntax->member == InplaceStr("size"))
			return new ExprIntegerLiteral(ctx.typeInt, node->length);

		Stop(ctx, syntax->pos, "ERROR: array doesn't have member with this name");
	}

	if(TypeStruct *node = getType<TypeStruct>(value->type))
	{
		for(VariableHandle *el = node->members.head; el; el = el->next)
		{
			if(el->variable->name == syntax->member)
				return new ExprMemberAccess(el->variable->type, value, el->variable); 
		}
	}

	Stop(ctx, syntax->pos, "ERROR: member variable or function '%.*s' is not defined in class '%.*s'", FMT_ISTR(syntax->member), FMT_ISTR(value->type->name));

	return NULL;
}

ExprArrayIndex* AnalyzeArrayIndex(ExpressionContext &ctx, SynTypeArray *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->type);
	ExprBase *index = AnalyzeExpression(ctx, syntax->size);

	if(TypeArray *type = getType<TypeArray>(value->type))
		return new ExprArrayIndex(type->subType, value, index);

	if(TypeUnsizedArray *type = getType<TypeUnsizedArray>(value->type))
		return new ExprArrayIndex(type->subType, value, index);

	Stop(ctx, syntax->pos, "ERROR: type '%.*s' is not an array", FMT_ISTR(value->type->name));

	return NULL;
}

ExprArrayIndex* AnalyzeArrayIndex(ExpressionContext &ctx, SynArrayIndex *syntax)
{
	if(syntax->arguments.size() > 1)
		Stop(ctx, syntax->pos, "ERROR: multiple array indexes are not supported");

	if(syntax->arguments.empty())
		Stop(ctx, syntax->pos, "ERROR: array index is missing");

	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	ExprBase *index = AnalyzeExpression(ctx, syntax->arguments.head);

	if(TypeArray *type = getType<TypeArray>(value->type))
		return new ExprArrayIndex(type->subType, value, index);

	if(TypeUnsizedArray *type = getType<TypeUnsizedArray>(value->type))
		return new ExprArrayIndex(type->subType, value, index);

	Stop(ctx, syntax->pos, "ERROR: type '%.*s' is not an array", FMT_ISTR(value->type->name));

	return NULL;
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

		arguments.push_back(AnalyzeExpression(ctx, el->value));
	}

	if(TypeFunction *type = getType<TypeFunction>(function->type))
	{
		return new ExprFunctionCall(type->returnType, function, arguments);
	}

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

		arguments.push_back(new ExprIntegerLiteral(ctx.typeInt, type->size));
		arguments.push_back(count);
		arguments.push_back(new ExprTypeLiteral(ctx.typeTypeID, type));

		call = new ExprFunctionCall(ctx.GetReferenceType(type), new ExprFunctionAccess((*function)->type, *function), arguments);
	}
	else
	{
		FunctionData **function = ctx.functionMap.find(GetStringHash("__newS"));

		if(!function || (*function)->name != InplaceStr("__newS"))
			Stop(ctx, syntax->pos, "ERROR: internal error, __newS not found");

		IntrusiveList<ExprBase> arguments;

		arguments.push_back(new ExprIntegerLiteral(ctx.typeInt, type->size));
		arguments.push_back(new ExprTypeLiteral(ctx.typeTypeID, type));

		call = new ExprFunctionCall(ctx.GetReferenceType(type), new ExprFunctionAccess((*function)->type, *function), arguments);
	}

	return call;
}

ExprReturn* AnalyzeReturn(ExpressionContext &ctx, SynReturn *syntax)
{
	// TODO: implicit cast
	// TODO: return type deduction

	if(syntax->value)
	{
		ExprBase *result = AnalyzeExpression(ctx, syntax->value);

		return new ExprReturn(result->type, result);
	}

	return new ExprReturn(ctx.typeVoid, NULL);
}

ExprVariableDefinition* AnalyzeVariableDefinition(ExpressionContext &ctx, SynVariableDefinition *syntax, unsigned alignment, TypeBase *type)
{
	// TODO: apply current namespace
	VariableData *variable = new VariableData(ctx.scopes.back(), alignment, type, syntax->name);

	ctx.AddVariable(variable);

	ExprBase *initializer = syntax->initializer ? AnalyzeExpression(ctx, syntax->initializer) : NULL;

	return new ExprVariableDefinition(ctx.typeVoid, variable, initializer);
}

ExprVariableDefinitions* AnalyzeVariableDefinitions(ExpressionContext &ctx, SynVariableDefinitions *syntax)
{
	unsigned alignment = syntax->align ? AnalyzeAlignment(ctx, syntax->align) : 0;

	TypeBase *type = AnalyzeType(ctx, syntax->type);

	IntrusiveList<ExprVariableDefinition> definitions;

	for(SynVariableDefinition *el = syntax->definitions.head; el; el = getType<SynVariableDefinition>(el->next))
		definitions.push_back(AnalyzeVariableDefinition(ctx, el, alignment, type));

	return new ExprVariableDefinitions(ctx.typeVoid, definitions);
}

ExprBase* AnalyzeFunctionDefinition(ExpressionContext &ctx, SynFunctionDefinition *syntax)
{
	if(syntax->coroutine)
		Stop(ctx, syntax->pos, "ERROR: coroutines are not implemented");

	if(syntax->parentType)
		Stop(ctx, syntax->pos, "ERROR: external class member functions are not implemented");

	if(!syntax->aliases.empty())
		Stop(ctx, syntax->pos, "ERROR: functions with explicit generic arguments are not implemented");

	TypeBase *returnType = AnalyzeType(ctx, syntax->returnType);

	IntrusiveList<TypeHandle> argTypes;

	// TODO: each argument should be able to reference previous one in an unevaluated context
	for(SynFunctionArgument *argument = syntax->arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next))
	{
		TypeBase *type = AnalyzeType(ctx, argument->type);

		argTypes.push_back(new TypeHandle(type));
	}

	TypeFunction *functionType = ctx.GetFunctionType(returnType, argTypes);

	// TODO: apply current namespace
	// TODO: generate lambda name
	// TODO: function type should be stored in type list
	FunctionData *function = new FunctionData(ctx.scopes.back(), functionType, syntax->name, syntax);

	ctx.AddFunction(function);

	if(functionType->isGeneric)
	{
		if(syntax->prototype)
			Stop(ctx, syntax->pos, "ERROR: generic function cannot be forward-declared");

		return new ExprGenericFunctionPrototype(ctx.typeVoid, function);
	}

	IntrusiveList<ExprVariableDefinition> arguments;

	ctx.PushScope(function);

	for(SynFunctionArgument *argument = syntax->arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next))
	{
		if(argument->isExplicit)
			Stop(ctx, syntax->pos, "ERROR: explicit type arguments are not supported");

		TypeBase *type = AnalyzeType(ctx, argument->type);

		VariableData *variable = new VariableData(ctx.scopes.back(), 0, type, argument->name);

		ctx.AddVariable(variable);

		ExprBase *initializer = argument->initializer ? AnalyzeExpression(ctx, argument->initializer) : NULL;

		arguments.push_back(new ExprVariableDefinition(ctx.typeVoid, variable, initializer));
	}

	IntrusiveList<ExprBase> expressions;

	for(SynBase *expression = syntax->expressions.head; expression; expression = expression->next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	ctx.PopScope();

	return new ExprFunctionDefinition(functionType, syntax->prototype, function, arguments, expressions);
}

void AnalyzeClassStaticIf(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassStaticIf *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	// TODO: replace with compile-time evaluation
	if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(condition))
	{
		if(number->value != 0)
			AnalyzeClassElements(ctx, classDefinition, syntax->trueBlock);
		else if(syntax->falseBlock)
			AnalyzeClassElements(ctx, classDefinition, syntax->falseBlock);
	}

	Stop(ctx, syntax->pos, "ERROR: can't get condition value");
}

void AnalyzeClassElements(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassElements *syntax)
{
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
	if(!proto && !syntax->aliases.empty())
	{
		TypeGenericClassProto *genericProtoType = new TypeGenericClassProto(syntax->name, syntax);

		ctx.AddType(genericProtoType);

		return new ExprVoid(ctx.typeVoid);
	}

	assert(generics.size() == syntax->aliases.size());

	// TODO: apply current namespace
	InplaceStr className = generics.empty() ? syntax->name : GetGenericClassName(proto, generics);

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

	ExprClassDefinition *classDefinition = new ExprClassDefinition(ctx.typeVoid, classType);

	ctx.PushScope(classType);

	IntrusiveList<AliasHandle> aliases;

	{
		TypeHandle *currType = classType->genericTypes.head;
		SynIdentifier *currName = classType->genericNames.head;

		while(currType && currName)
		{
			ctx.AddAlias(new AliasData(ctx.scopes.back(), currType->type, currName->name));

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
		NamespaceData *ns = new NamespaceData(ctx.scopes.back(), name->name);

		ctx.PushScope(ns);
	}

	IntrusiveList<ExprBase> expressions;

	for(SynBase *expression = syntax->expressions.head; expression; expression = expression->next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	for(SynIdentifier *name = syntax->path.head; name; name = getType<SynIdentifier>(name->next))
		ctx.PopScope();

	return new ExprBlock(ctx.typeVoid, expressions);
}

ExprVoid* AnalyzeTypedef(ExpressionContext &ctx, SynTypedef *syntax)
{
	TypeBase *type = AnalyzeType(ctx, syntax->type);

	AliasData *alias = new AliasData(ctx.scopes.back(), type, syntax->alias);

	ctx.AddAlias(alias);

	return new ExprVoid(ctx.typeVoid);
}

ExprBase* AnalyzeIfElse(ExpressionContext &ctx, SynIfElse *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

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

			return new ExprVoid(ctx.typeVoid);
		}

		Stop(ctx, syntax->pos, "ERROR: can't get condition value");
	}

	ExprBase *trueBlock = AnalyzeStatement(ctx, syntax->trueBlock);
	ExprBase *falseBlock = syntax->falseBlock ? AnalyzeStatement(ctx, syntax->falseBlock) : NULL;

	return new ExprIfElse(ctx.typeVoid, condition, trueBlock, falseBlock);
}

ExprFor* AnalyzeFor(ExpressionContext &ctx, SynFor *syntax)
{
	ExprBase *initializer = syntax->initializer ? AnalyzeStatement(ctx, syntax->initializer) : new ExprVoid(ctx.typeVoid);
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);
	ExprBase *increment = syntax->increment ? AnalyzeStatement(ctx, syntax->increment) : new ExprVoid(ctx.typeVoid);
	ExprBase *body = syntax->body ? AnalyzeStatement(ctx, syntax->body) : new ExprVoid(ctx.typeVoid);

	return new ExprFor(ctx.typeVoid, initializer, condition, increment, body);
}

ExprWhile* AnalyzeWhile(ExpressionContext &ctx, SynWhile *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);
	ExprBase *body = syntax->body ? AnalyzeStatement(ctx, syntax->body) : new ExprVoid(ctx.typeVoid);

	return new ExprWhile(ctx.typeVoid, condition, body);
}

ExprDoWhile* AnalyzeDoWhile(ExpressionContext &ctx, SynDoWhile *syntax)
{
	ExprBase *body = syntax->body ? AnalyzeStatement(ctx, syntax->body) : new ExprVoid(ctx.typeVoid);
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	return new ExprDoWhile(ctx.typeVoid, body, condition);
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

	return new ExprBlock(ctx.typeVoid, expressions);
}

ExprBase* AnalyzeExpression(ExpressionContext &ctx, SynBase *syntax)
{
	if(SynBool *node = getType<SynBool>(syntax))
	{
		return new ExprBoolLiteral(ctx.typeBool, node->value);
	}

	if(SynCharacter *node = getType<SynCharacter>(syntax))
	{
		unsigned char result = (unsigned char)node->value.begin[1];

		if(result == '\\')
			result = ParseEscapeSequence(ctx, node->value.begin + 1);

		return new ExprCharacterLiteral(ctx.typeChar, result);
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

		return new ExprStringLiteral(ctx.GetArrayType(ctx.typeChar, length + 1), value, length);
	}
	
	if(SynNullptr *node = getType<SynNullptr>(syntax))
	{
		return new ExprNullptrLiteral(ctx.GetReferenceType(ctx.typeVoid));
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

		return new ExprTypeLiteral(ctx.typeTypeID, value->type);
	}

	if(SynIdentifier *node = getType<SynIdentifier>(syntax))
	{
		return AnalyzeVariableAccess(ctx, node);
	}

	if(SynTypeSimple *node = getType<SynTypeSimple>(syntax))
	{
		// It could be a typeid
		if(TypeBase *type = AnalyzeType(ctx, node, false))
			return new ExprTypeLiteral(ctx.typeTypeID, type);

		return AnalyzeVariableAccess(ctx, node);
	}

	if(SynSizeof *node = getType<SynSizeof>(syntax))
	{
		ExprBase *value = AnalyzeExpression(ctx, node->value);

		return new ExprIntegerLiteral(ctx.typeInt, value->type->size);
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
			return new ExprTypeLiteral(ctx.typeTypeID, type);

		return AnalyzeMemberAccess(ctx, node);
	}

	if(SynTypeArray *node = getType<SynTypeArray>(syntax))
	{
		// It could be a typeid
		if(TypeBase *type = AnalyzeType(ctx, syntax, false))
			return new ExprTypeLiteral(ctx.typeTypeID, type);

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

void AnalyzeModuleImport(ExpressionContext &ctx, SynModuleImport *syntax)
{
	Stop(ctx, syntax->pos, "ERROR: module import is not implemented");
}

ExprBase* AnalyzeModule(ExpressionContext &ctx, SynBase *syntax)
{
	if(SynModule *node = getType<SynModule>(syntax))
	{
		for(SynModuleImport *import = node->imports.head; import; import = getType<SynModuleImport>(import->next))
			AnalyzeModuleImport(ctx, import);

		IntrusiveList<ExprBase> expressions;

		for(SynBase *expr = node->expressions.head; expr; expr = expr->next)
			expressions.push_back(AnalyzeStatement(ctx, expr));

		return new ExprModule(ctx.typeVoid, expressions);
	}

	return NULL;
}

ExprBase* Analyze(ExpressionContext &ctx, SynBase *syntax)
{
	ctx.PushScope();

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
	ctx.typeAutoRef->members.push_back(new VariableHandle(new VariableData(ctx.scopes.back(), 0, ctx.typeTypeID, InplaceStr("type"))));
	ctx.typeAutoRef->members.push_back(new VariableHandle(new VariableData(ctx.scopes.back(), 0, ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"))));

	ctx.AddType(ctx.typeAutoArray = new TypeAutoArray(InplaceStr("auto[]")));
	ctx.typeAutoArray->members.push_back(new VariableHandle(new VariableData(ctx.scopes.back(), 0, ctx.typeTypeID, InplaceStr("type"))));
	ctx.typeAutoArray->members.push_back(new VariableHandle(new VariableData(ctx.scopes.back(), 0, ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"))));
	ctx.typeAutoArray->members.push_back(new VariableHandle(new VariableData(ctx.scopes.back(), 0, ctx.typeInt, InplaceStr("size"))));

	// Analyze module
	if(!setjmp(errorHandler))
	{
		ExprBase *module = AnalyzeModule(ctx, syntax);

		ctx.PopScope();

		return module;
	}

	return NULL;
}
