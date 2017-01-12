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

	template<typename T>
	unsigned GetTypeAlignment()
	{
		struct Helper
		{
			char x;
			T y;
		};

		return sizeof(Helper) - sizeof(T);
	}

	// Function returns reference type for the type
	TypeInfo* GetReferenceType(ExpressionContext &ctx, TypeInfo* type)
	{
		// If type already has reference type, return it
		if(type->refType)
			return type->refType;

		// Create new type
		TypeInfo* newInfo = new TypeInfo(ctx.typeInfo.size(), NULL, type->refLevel + 1, 0, 1, type, TypeInfo::NULLC_PTR_TYPE);
		newInfo->alignBytes = 4;
		newInfo->size = NULLC_PTR_SIZE;

		// Save it for future use
		type->refType = newInfo;

		newInfo->dependsOnGeneric = type->dependsOnGeneric;

		ctx.typeInfo.push_back(newInfo);
		return newInfo;
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
	typeChar = NULL;
	typeShort = NULL;
	typeInt = NULL;
	typeFloat = NULL;
	typeLong = NULL;
	typeDouble = NULL;
	typeObject = NULL;
	typeTypeid = NULL;
	typeAutoArray = NULL;
	typeFunction = NULL;
	typeGeneric = NULL;
	typeBool = NULL;
}

TypeInfo* AddBasicType(ExpressionContext &ctx, const char *name, TypeInfo::TypeCategory category, unsigned size, unsigned align)
{
	TypeInfo *type = new TypeInfo(ctx.typeInfo.size(), name, 0, 0, 1, NULL, category);

	type->alignBytes = align;
	type->size = size;

	ctx.typeInfo.push_back(type);

	return type;
}

ExprBase* AnalyzeExpression(ExpressionContext &ctx, SynBase *syntax)
{
	if(SynBool *node = getType<SynBool>(syntax))
	{
		return new ExprNumber(ctx.typeBool, int(node->value));
	}

	if(SynNumber *node = getType<SynNumber>(syntax))
	{
		InplaceStr &value = node->value;

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
				return new ExprNumber(ctx.typeInt, int(num));

			return new ExprNumber(ctx.typeLong, num);
		}

		bool isFP = false;

		for(unsigned int i = 0; i < value.length(); i++)
		{
			if(value.begin[i] == '.' || value.begin[i] == 'e')
				isFP = true;
		}

		if(!isFP)
		{
			if(node->suffix == InplaceStr("b"))
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
					return new ExprNumber(ctx.typeInt, int(num));

				return new ExprNumber(ctx.typeLong, num);
			}
			else if(node->suffix == InplaceStr("l"))
			{
				long long num = ParseLong(ctx, value.begin, value.end, 10);

				return new ExprNumber(ctx.typeLong, num);
			}
			else if(!node->suffix.empty())
			{
				Stop(ctx, node->suffix.begin, "ERROR: unknown number suffix '%.*s'", node->suffix.length(), node->suffix.begin);
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
					return new ExprNumber(ctx.typeInt, int(num));

				return new ExprNumber(ctx.typeLong, num);
			}

			long long num = ParseLong(ctx, value.begin, value.end, 10);

			if(int(num) == num)
				return new ExprNumber(ctx.typeInt, int(num));

			Stop(ctx, value.begin, "ERROR: overflow in decimal constant");
		}
		
		if(node->suffix == InplaceStr("f"))
		{
			double num = ParseDouble(ctx, value.begin);

			return new ExprNumber(ctx.typeFloat, float(num));
		}
		
		double num = ParseDouble(ctx, value.begin);

		return new ExprNumber(ctx.typeDouble, num);
	}

	if(SynReturn *node = getType<SynReturn>(syntax))
	{
		ExprBase *result = AnalyzeExpression(ctx, node->value);

		// TODO: lot of things

		return new ExprReturn(result->type, result);
	}

	Stop(ctx, syntax->pos, "ERROR: unknown expression type");

	return NULL;
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
			expressions.push_back(AnalyzeExpression(ctx, expr));

		return new ExprModule(ctx.typeVoid, expressions);
	}

	return NULL;
}

ExprBase* Analyze(ExpressionContext &ctx, SynBase *syntax)
{
	// Add basic typess
	ctx.typeVoid = AddBasicType(ctx, "void", TypeInfo::TYPE_VOID, 0, TypeInfo::ALIGNMENT_UNSPECIFIED);

	ctx.typeDouble = AddBasicType(ctx, "double", TypeInfo::TYPE_DOUBLE, 8, GetTypeAlignment<double>());
	ctx.typeFloat = AddBasicType(ctx, "float", TypeInfo::TYPE_FLOAT, 4, GetTypeAlignment<float>());

	ctx.typeLong = AddBasicType(ctx, "long", TypeInfo::TYPE_LONG, 8, GetTypeAlignment<long long>());
	ctx.typeInt = AddBasicType(ctx, "int", TypeInfo::TYPE_INT, 4, GetTypeAlignment<int>());
	ctx.typeShort = AddBasicType(ctx, "short", TypeInfo::TYPE_SHORT, 2, GetTypeAlignment<short>());
	ctx.typeChar = AddBasicType(ctx, "char", TypeInfo::TYPE_CHAR, 1, TypeInfo::ALIGNMENT_UNSPECIFIED);

	ctx.typeObject = AddBasicType(ctx, "auto ref", TypeInfo::TYPE_COMPLEX, 0, 4);
	ctx.typeTypeid = AddBasicType(ctx, "typeid", TypeInfo::TYPE_COMPLEX, 4, GetTypeAlignment<int>());

	// Object type depends on typeid which is defined later
	ctx.typeObject->AddMemberVariable("type", ctx.typeTypeid);
	ctx.typeObject->AddMemberVariable("ptr", GetReferenceType(ctx, ctx.typeVoid));
	ctx.typeObject->size = 4 + NULLC_PTR_SIZE;

	ctx.typeAutoArray = AddBasicType(ctx, "auto[]", TypeInfo::TYPE_COMPLEX, 0, 4);

	ctx.typeAutoArray->AddMemberVariable("type", ctx.typeTypeid);
	ctx.typeAutoArray->AddMemberVariable("ptr", GetReferenceType(ctx, ctx.typeVoid));
	ctx.typeAutoArray->AddMemberVariable("size", ctx.typeInt);
	ctx.typeAutoArray->size = 8 + NULLC_PTR_SIZE;

	ctx.typeFunction = AddBasicType(ctx, "__function", TypeInfo::TYPE_COMPLEX, 4, TypeInfo::ALIGNMENT_UNSPECIFIED);

	ctx.typeGeneric = AddBasicType(ctx, "generic", TypeInfo::TYPE_VOID, 0, TypeInfo::ALIGNMENT_UNSPECIFIED);
	ctx.typeGeneric->dependsOnGeneric = true;

	ctx.typeBool = AddBasicType(ctx, "bool", TypeInfo::TYPE_CHAR, 1, TypeInfo::ALIGNMENT_UNSPECIFIED);

	// Analyze module
	if(!setjmp(errorHandler))
		return AnalyzeModule(ctx, syntax);

	return NULL;
}
