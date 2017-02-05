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
	scopes.push_back(new ScopeData());
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
	TypeRef* result = new TypeRef(type);

	// Save it for future use
	type->refType = result;

	types.push_back(result);

	return result;
}

TypeArray* ExpressionContext::GetArrayType(TypeBase* type, long long size)
{
	for(unsigned i = 0; i < type->arrayTypes.size(); i++)
	{
		if(type->arrayTypes[i]->size == size)
			return type->arrayTypes[i];
	}

	// Create new type
	TypeArray* result = new TypeArray(type, size);

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
	TypeUnsizedArray* result = new TypeUnsizedArray(type);

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
	TypeFunction* result = new TypeFunction(returnType, arguments);

	types.push_back(result);

	return result;
}

ExprBase* AnalyzeExpression(ExpressionContext &ctx, SynBase *syntax);
ExprBase* AnalyzeStatement(ExpressionContext &ctx, SynBase *syntax);

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
		TypeBase *type = AnalyzeType(ctx, node->type);

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

	Stop(ctx, syntax->pos, "ERROR: unknown type");

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
	VariableData *variable = new VariableData(alignment, type, syntax->name);

	ctx.AddVariable(variable);

	ExprBase *initializer = syntax->initializer ? AnalyzeExpression(ctx, syntax->initializer) : NULL;

	return new ExprVariableDefinition(ctx.typeVoid, variable, initializer);
}

ExprVariableDefinitions* AnalyzeVariableDefinitions(ExpressionContext &ctx, SynVariableDefinitions *syntax)
{
	unsigned alignment = 0;

	if(ExprBase *align = syntax->align ? AnalyzeNumber(ctx, syntax->align->value) : NULL)
	{
		if(ExprIntegerLiteral *alignValue = getType<ExprIntegerLiteral>(align))
			alignment = unsigned(alignValue->value);
		else
			Stop(ctx, syntax->align->value->pos, "ERROR: alignment must be an integer constant");
	}

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

	FunctionData *function = new FunctionData(functionType, syntax->name);

	ctx.AddFunction(function);

	IntrusiveList<ExprVariableDefinition> arguments;

	ctx.PushScope();

	for(SynFunctionArgument *argument = syntax->arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next))
	{
		if(argument->isExplicit)
			Stop(ctx, syntax->pos, "ERROR: explicit type arguments are not supported");

		TypeBase *type = AnalyzeType(ctx, argument->type);

		VariableData *variable = new VariableData(0, type, argument->name);

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

ExprBase* AnalyzeIfElse(ExpressionContext &ctx, SynIfElse *syntax)
{
	ExprBase *condition = AnalyzeExpression(ctx, syntax->condition);

	if(syntax->staticIf)
	{
		// TODO: replace with compile-time evaluation
		if(ExprIntegerLiteral *number = getType<ExprIntegerLiteral>(condition))
		{
			if(number->value != 0)
				return AnalyzeStatement(ctx, syntax->trueBlock);
			
			if(syntax->falseBlock)
				return AnalyzeStatement(ctx, syntax->falseBlock);

			return new ExprVoid(ctx.typeVoid);
		}

		Stop(ctx, syntax->pos, "ERROR: can't get condition value");
	}

	ExprBase *trueBlock = AnalyzeStatement(ctx, syntax->trueBlock);
	ExprBase *falseBlock = syntax->falseBlock ? AnalyzeStatement(ctx, syntax->falseBlock) : NULL;

	return new ExprIfElse(ctx.typeVoid, condition, trueBlock, falseBlock);
}

ExprBlock* AnalyzeBlock(ExpressionContext &ctx, SynBlock *syntax)
{
	ctx.PushScope();
	
	IntrusiveList<ExprBase> expressions;

	for(SynBase *expression = syntax->expressions.head; expression; expression = expression->next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

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

	if(SynTypeSimple *node = getType<SynTypeSimple>(syntax))
	{
		if(TypeBase *type = AnalyzeType(ctx, node, false))
			return new ExprTypeLiteral(ctx.typeTypeID, type);

		return AnalyzeVariableAccess(ctx, node);
	}

	if(SynConditional *node = getType<SynConditional>(syntax))
	{
		return AnalyzeConditional(ctx, node);
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

	if(SynIfElse *node = getType<SynIfElse>(syntax))
	{
		return AnalyzeIfElse(ctx, node);
	}

	if(SynBlock *node = getType<SynBlock>(syntax))
	{
		return AnalyzeBlock(ctx, node);
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

	ctx.AddType(ctx.typeVoid = new TypeVoid());

	ctx.AddType(ctx.typeBool = new TypeBool());

	ctx.AddType(ctx.typeChar = new TypeChar());
	ctx.AddType(ctx.typeShort = new TypeShort());
	ctx.AddType(ctx.typeInt = new TypeInt());
	ctx.AddType(ctx.typeLong = new TypeLong());

	ctx.AddType(ctx.typeFloat = new TypeFloat());
	ctx.AddType(ctx.typeDouble = new TypeDouble());

	ctx.AddType(ctx.typeTypeID = new TypeTypeID());
	ctx.AddType(ctx.typeFunctionID = new TypeFunctionID());

	ctx.AddType(ctx.typeGeneric = new TypeGeneric());

	ctx.AddType(ctx.typeAuto = new TypeAuto());

	ctx.AddType(ctx.typeAutoRef = new TypeAutoRef());
	ctx.typeAutoRef->members.push_back(new TypeStruct::Member("type", ctx.typeTypeID, 0));
	ctx.typeAutoRef->members.push_back(new TypeStruct::Member("ptr", ctx.GetReferenceType(ctx.typeVoid), 0));

	ctx.AddType(ctx.typeAutoArray = new TypeAutoArray());
	ctx.typeAutoArray->members.push_back(new TypeStruct::Member("type", ctx.typeTypeID, 0));
	ctx.typeAutoArray->members.push_back(new TypeStruct::Member("ptr", ctx.GetReferenceType(ctx.typeVoid), 0));
	ctx.typeAutoArray->members.push_back(new TypeStruct::Member("size", ctx.typeInt, 0));

	// Analyze module
	if(!setjmp(errorHandler))
	{
		ExprBase *module = AnalyzeModule(ctx, syntax);

		ctx.PopScope();

		return module;
	}

	return NULL;
}
