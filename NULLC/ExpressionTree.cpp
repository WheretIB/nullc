#include "ExpressionTree.h"

#include "ParseClass.h"

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

	bool IsNumericType(ExpressionContext &ctx, TypeBase* a)
	{
		if(a == ctx.typeBool)
			return true;

		if(a == ctx.typeChar)
			return true;

		if(a == ctx.typeShort)
			return true;

		if(a == ctx.typeInt)
			return true;

		if(a == ctx.typeLong)
			return true;

		if(a == ctx.typeFloat)
			return true;

		if(a == ctx.typeDouble)
			return true;

		return false;
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

	typeGeneric = NULL;

	typeAuto = NULL;
	typeAutoRef = NULL;
	typeAutoArray = NULL;

	typeMap.init();
	functionMap.init();
	variableMap.init();
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

	delete scopes.back();
	scopes.pop_back();
}

void ExpressionContext::AddType(TypeBase *type)
{
	scopes.back()->types.push_back(type);

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

TypeRef* ExpressionContext::GetReferenceType(TypeBase* type)
{
	if(type->refType)
		return type->refType;

	// Create new type
	unsigned nameLength = type->name.length() + strlen(" ref");
	char *name = new char[nameLength + 1];
	sprintf(name, "%.*s ref", unsigned(type->name.end - type->name.begin), type->name.begin);

	TypeRef* result = new TypeRef(InplaceStr(name), type);

	// Save it for future use
	type->refType = result;

	types.push_back(result);

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
	unsigned nameLength = type->name.length() + strlen("[]") + 21;
	char *name = new char[nameLength + 1];
	sprintf(name, "%.*s[%lld]", unsigned(type->name.end - type->name.begin), type->name.begin, length);

	TypeArray* result = new TypeArray(InplaceStr(name), type, length);

	// Save it for future use
	type->arrayTypes.push_back(result);

	types.push_back(result);

	return result;
}

TypeUnsizedArray* ExpressionContext::GetUnsizedArrayType(TypeBase* type)
{
	if(type->unsizedArrayType)
		return type->unsizedArrayType;

	// Create new type
	unsigned nameLength = type->name.length() + strlen("[]");
	char *name = new char[nameLength + 1];
	sprintf(name, "%.*s[]", unsigned(type->name.end - type->name.begin), type->name.begin);

	TypeUnsizedArray* result = new TypeUnsizedArray(InplaceStr(name), type);

	result->members.push_back(new VariableHandle(new VariableData(scopes.back(), 4, typeInt, InplaceStr("size"))));

	// Save it for future use
	type->unsizedArrayType = result;

	types.push_back(result);

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
	unsigned nameLength = returnType->name.length() + strlen(" ref()");

	for(TypeHandle *arg = arguments.head; arg; arg = arg->next)
		nameLength += arg->type->name.length() + 1;

	char *name = new char[nameLength + 1];

	char *pos = name;

	sprintf(pos, "%.*s", unsigned(returnType->name.end - returnType->name.begin), returnType->name.begin);
	pos += returnType->name.length();

	strcpy(pos, " ref(");
	pos += 5;

	for(TypeHandle *arg = arguments.head; arg; arg = arg->next)
	{
		sprintf(pos, "%.*s", unsigned(arg->type->name.end - arg->type->name.begin), arg->type->name.begin);
		pos += arg->type->name.length();

		if(arg->next)
			*pos++ = ',';
	}

	*pos++ = ')';
	*pos++ = 0;

	TypeFunction* result = new TypeFunction(InplaceStr(name), returnType, arguments);

	types.push_back(result);

	return result;
}

ExprBase* AnalyzeNumber(ExpressionContext &ctx, SynNumber *syntax);
ExprBase* AnalyzeExpression(ExpressionContext &ctx, SynBase *syntax);
ExprBase* AnalyzeStatement(ExpressionContext &ctx, SynBase *syntax);
ExprBlock* AnalyzeBlock(ExpressionContext &ctx, SynBlock *syntax, bool createScope);
void AnalyzeClassElements(ExpressionContext &ctx, ExprClassDefinition *classDefinition, SynClassElements *syntax);

TypeBase* AnalyzeType(ExpressionContext &ctx, SynBase *syntax, bool onlyType = true)
{
	if(SynTypeAuto *node = getType<SynTypeAuto>(syntax))
	{
		return ctx.typeAuto;
	}

	if(SynTypeGeneric *node = getType<SynTypeGeneric>(syntax))
	{
		return ctx.typeGeneric;
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
		TypeBase *type = AnalyzeType(ctx, node->value);

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

ExprBase* AnalyzeVariableAccess(ExpressionContext &ctx, SynTypeSimple *syntax)
{
	if(syntax->path.empty())
	{
		// TODO: current namespace

		if(VariableData **variable = ctx.variableMap.find(GetStringHash(syntax->name.begin, syntax->name.end)))
		{
			// TODO: check external variable access

			return new ExprVariableAccess((*variable)->type, *variable);
		}

		if(FunctionData **function = ctx.functionMap.find(GetStringHash(syntax->name.begin, syntax->name.end)))
		{
			return new ExprFunctionAccess((*function)->type, *function);
		}
	}

	// TODO: namespace path

	Stop(ctx, syntax->pos, "ERROR: unknown variable");

	return NULL;
}

ExprUnaryOp* AnalyzeUnaryOp(ExpressionContext &ctx, SynUnaryOp *syntax)
{
	ExprBase *value = AnalyzeExpression(ctx, syntax->value);

	TypeBase *resultType = NULL;

	if(syntax->type == SYN_UNARY_OP_LOGICAL_NOT)
		resultType = ctx.typeBool;
	else
		resultType = value->type;

	return new ExprUnaryOp(resultType, syntax->type, value);
}

ExprBinaryOp* AnalyzeBinaryOp(ExpressionContext &ctx, SynBinaryOp *syntax)
{
	ExprBase *lhs = AnalyzeExpression(ctx, syntax->lhs);
	ExprBase *rhs = AnalyzeExpression(ctx, syntax->rhs);

	TypeBase *resultType = NULL;

	if(IsNumericType(ctx, lhs->type) && IsNumericType(ctx, rhs->type))
		resultType = GetBinaryOpResultType(ctx, lhs->type, rhs->type);
	else
		Stop(ctx, syntax->pos, "ERROR: Unknown binary operation");

	if(syntax->type == SYN_BINARY_OP_LOGICAL_AND || syntax->type == SYN_BINARY_OP_LOGICAL_OR || syntax->type == SYN_BINARY_OP_LOGICAL_XOR)
		resultType = ctx.typeBool;

	// TODO: implicit casts

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

	Stop(ctx, syntax->pos, "ERROR: cannot dereference type '%.*s' that is not a pointer", unsigned(value->type->name.end - value->type->name.begin), value->type->name.begin);

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

	Stop(ctx, syntax->pos, "ERROR: member variable or function '%.*s' is not defined in class '%*.s'", unsigned(syntax->member.end - syntax->member.begin), syntax->member.begin, unsigned(value->type->name.end - value->type->name.begin), value->type->name.begin);

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

	Stop(ctx, syntax->pos, "ERROR: type '%.*s' is not an array", unsigned(value->type->name.end - value->type->name.begin), value->type->name.begin);

	return NULL;
}

ExprReturn* AnalyzeReturn(ExpressionContext &ctx, SynReturn *syntax)
{
	// TODO: lots of things

	if(syntax->value)
	{
		ExprBase *result = AnalyzeExpression(ctx, syntax->value);

		return new ExprReturn(result->type, result);
	}

	return new ExprReturn(ctx.typeVoid, NULL);
}

ExprVariableDefinition* AnalyzeVariableDefinition(ExpressionContext &ctx, SynVariableDefinition *syntax, unsigned alignment, TypeBase *type)
{
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

ExprFunctionDefinition* AnalyzeFunctionDefinition(ExpressionContext &ctx, SynFunctionDefinition *syntax)
{
	if(syntax->coroutine)
		Stop(ctx, syntax->pos, "ERROR: coroutines are not implemented");

	if(syntax->parentType)
		Stop(ctx, syntax->pos, "ERROR: external class member functions are not implemented");

	if(!syntax->aliases.empty())
		Stop(ctx, syntax->pos, "ERROR: functions with explicit generic arguments are not implemented");

	TypeBase *returnType = AnalyzeType(ctx, syntax->returnType);

	IntrusiveList<TypeHandle> argTypes;

	for(SynFunctionArgument *argument = syntax->arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next))
	{
		TypeBase *type = AnalyzeType(ctx, argument->type);

		argTypes.push_back(new TypeHandle(type));
	}

	TypeFunction *functionType = ctx.GetFunctionType(returnType, argTypes);

	FunctionData *function = new FunctionData(ctx.scopes.back(), functionType, syntax->name);

	ctx.AddFunction(function);

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

	return new ExprFunctionDefinition(ctx.typeVoid, syntax->prototype, function, arguments, expressions);
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

	for(SynFunctionDefinition *function = syntax->functions.head; function; function = getType<SynFunctionDefinition>(function->next))
		classDefinition->functions.push_back(AnalyzeFunctionDefinition(ctx, function));

	if(syntax->accessors.head)
		Stop(ctx, syntax->pos, "ERROR: class accessors not implemented");
	//for(SynAccessor *accessor = syntax->accessors.head; accessor; accessor = getType<SynAccessor>(accessor->next))
	//	classType->accessors.push_back(AnalyzeAccessorDefinition(ctx, accessor));

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

	for(SynClassStaticIf *staticIf = syntax->staticIfs.head; staticIf; staticIf = getType<SynClassStaticIf>(staticIf->next))
		AnalyzeClassStaticIf(ctx, classDefinition, staticIf);
}

ExprBase* AnalyzeClassDefinition(ExpressionContext &ctx, SynClassDefinition *syntax)
{
	unsigned alignment = syntax->align ? AnalyzeAlignment(ctx, syntax->align) : 0;

	if(!syntax->aliases.empty())
	{
		unsigned nameLength = syntax->name.length() + strlen("<>");

		for(SynIdentifier *alias = syntax->aliases.head; alias; alias = getType<SynIdentifier>(alias->next))
			nameLength += strlen("generic") + 1;

		char *name = new char[nameLength + 1];

		char *pos = name;

		sprintf(pos, "%.*s", unsigned(syntax->name.end - syntax->name.begin), syntax->name.begin);
		pos += syntax->name.length();

		*pos++ = '<';

		for(SynIdentifier *alias = syntax->aliases.head; alias; alias = getType<SynIdentifier>(alias->next))
		{
			strcpy(pos, "generic");
			pos += strlen("generic");

			if(alias->next)
				*pos++ = ',';
		}

		*pos++ = '>';
		*pos++ = 0;

		TypeGenericClassProto *genericProtoType = new TypeGenericClassProto(InplaceStr(name), syntax->extendable, syntax);

		ctx.AddType(genericProtoType);

		genericProtoType->alignment = alignment;

		return new ExprGenericClassPrototype(ctx.typeVoid, genericProtoType);
	}

	TypeBase *baseClass = syntax->baseClass ? AnalyzeType(ctx, syntax->baseClass) : NULL;

	TypeClass *classType = new TypeClass(ctx.scopes.back(), syntax->name, syntax->extendable, baseClass);

	ctx.AddType(classType);

	ExprClassDefinition *classDefinition = new ExprClassDefinition(ctx.typeVoid, classType);

	ctx.PushScope(classType);

	AnalyzeClassElements(ctx, classDefinition, syntax->elements);

	ctx.PopScope();

	return classDefinition;
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
		return AnalyzeClassDefinition(ctx, node);
	}

	if(SynIfElse *node = getType<SynIfElse>(syntax))
	{
		return AnalyzeIfElse(ctx, node);
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

	ctx.AddType(ctx.typeGeneric = new TypeGeneric(InplaceStr("generic")));

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
