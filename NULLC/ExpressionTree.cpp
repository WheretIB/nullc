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
	TypeRef* typeRef = new TypeRef(type);

	// Save it for future use
	type->refType = typeRef;

	types.push_back(typeRef);

	return typeRef;
}

ExprBase* AnalyzeExpression(ExpressionContext &ctx, SynBase *syntax);
ExprBase* AnalyzeStatement(ExpressionContext &ctx, SynBase *syntax);

TypeBase* AnalyzeType(ExpressionContext &ctx, SynBase *syntax)
{
	if(SynTypeAuto *node = getType<SynTypeAuto>(syntax))
	{
		return ctx.typeAuto;
	}

	if(SynTypeGeneric *node = getType<SynTypeGeneric>(syntax))
	{
		return ctx.typeGeneric;
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
		//return NULL;
	}

	if(SynTypeReference *node = getType<SynTypeReference>(syntax))
	{
		TypeBase *type = AnalyzeType(ctx, node->type);

		if(isType<TypeAuto>(type))
			return ctx.typeAutoRef;

		return ctx.GetReferenceType(type);
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
		
	double num = ParseDouble(ctx, value.begin);

	return new ExprRationalLiteral(ctx.typeDouble, num);
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

	if(!returnType)
		Stop(ctx, syntax->pos, "ERROR: unknown type");

	IntrusiveList<TypeFunction::Argument> argTypes;

	for(SynFunctionArgument *argument = syntax->arguments.head; argument; argument = getType<SynFunctionArgument>(argument->next))
	{
		TypeBase *type = AnalyzeType(ctx, argument->type);

		if(!type)
			Stop(ctx, syntax->pos, "ERROR: unknown type");

		argTypes.push_back(new TypeFunction::Argument(type));
	}

	TypeFunction *functionType = new TypeFunction(returnType, argTypes);

	FunctionData *function = new FunctionData(functionType, syntax->name);

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

	if(SynNumber *node = getType<SynNumber>(syntax))
	{
		return AnalyzeNumber(ctx, node);
	}

	if(SynUnaryOp *node = getType<SynUnaryOp>(syntax))
	{
		return NULL;
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

	ctx.AddType(ctx.typeTypeID = new TypeFloat());
	ctx.AddType(ctx.typeFunctionID = new TypeDouble());

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
