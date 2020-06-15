#include "ExpressionTranslate.h"

#include <stdarg.h>

#include "Bytecode.h"
#include "BinaryCache.h"
#include "Compiler.h"
#include "ExpressionTree.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

NULLC_PRINT_FORMAT_CHECK(2, 3) void Print(ExpressionTranslateContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);
}

void PrintIndent(ExpressionTranslateContext &ctx)
{
	for(unsigned i = 0; i < ctx.depth; i++)
		ctx.output.Print(ctx.indent);
}

void PrintLine(ExpressionTranslateContext &ctx)
{
	ctx.output.Print("\n");
}

NULLC_PRINT_FORMAT_CHECK(2, 3) void PrintIndentedLine(ExpressionTranslateContext &ctx, const char *format, ...)
{
	PrintIndent(ctx);

	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);

	PrintLine(ctx);
}

void PrintEscapedName(ExpressionTranslateContext &ctx, InplaceStr name)
{
	for(unsigned i = 0; i < name.length(); i++)
	{
		char ch = name.begin[i];

		if(ch == ' ' || ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == ',' || ch == ':' || ch == '$' || ch == '<' || ch == '>' || ch == '.')
			ctx.output.Print("_");
		else
			ctx.output.Print(ch);
	}
}

bool UseNonStaticTemplate(ExpressionTranslateContext &ctx, FunctionData *function)
{
	if(function->scope != ctx.ctx.globalScope && !function->scope->ownerNamespace && !function->scope->ownerType)
		return false;
	else if(*function->name->name.begin == '$')
		return false;
	else if(function->isHidden)
		return false;
	else if(ctx.ctx.IsGenericInstance(function))
		return true;

	return false;
}

void TranslateFunctionName(ExpressionTranslateContext &ctx, FunctionData *function);

void PrintEscapedTypeName(ExpressionTranslateContext &ctx, TypeBase *type)
{
	if(TypeClass *typeClass = getType<TypeClass>(type))
	{
		if(typeClass->scope->ownerFunction)
		{
			TranslateFunctionName(ctx, typeClass->scope->ownerFunction);
			Print(ctx, "_");
		}
		else if(typeClass->scope != ctx.ctx.globalScope && !typeClass->scope->ownerType && !typeClass->scope->ownerFunction && !typeClass->scope->ownerNamespace)
		{
			Print(ctx, "scope_%d_", typeClass->scope->uniqueId);
		}

		PrintEscapedName(ctx, typeClass->name);
	}
	else
	{
		PrintEscapedName(ctx, type->name);
	}
}

void TranslateTypeName(ExpressionTranslateContext &ctx, TypeBase *type)
{
	if(isType<TypeVoid>(type))
	{
		Print(ctx, "void");
	}
	else if(isType<TypeBool>(type))
	{
		Print(ctx, "bool");
	}
	else if(isType<TypeChar>(type))
	{
		Print(ctx, "char");
	}
	else if(isType<TypeShort>(type))
	{
		Print(ctx, "short");
	}
	else if(isType<TypeInt>(type))
	{
		Print(ctx, "int");
	}
	else if(isType<TypeLong>(type))
	{
		Print(ctx, "long long");
	}
	else if(isType<TypeFloat>(type))
	{
		Print(ctx, "float");
	}
	else if(isType<TypeDouble>(type))
	{
		Print(ctx, "double");
	}
	else if(isType<TypeTypeID>(type))
	{
		Print(ctx, "unsigned");
	}
	else if(isType<TypeFunctionID>(type))
	{
		Print(ctx, "__function");
	}
	else if(isType<TypeNullptr>(type))
	{
		Print(ctx, "_nullptr");
	}
	else if(isType<TypeGeneric>(type))
	{
		assert(!"generic type TypeGeneric is not translated");
	}
	else if(isType<TypeGenericAlias>(type))
	{
		assert(!"generic type TypeGenericAlias is not translated");
	}
	else if(isType<TypeAuto>(type))
	{
		assert(!"virtual type TypeAuto is not translated");
	}
	else if(isType<TypeAutoRef>(type))
	{
		Print(ctx, "NULLCRef");
	}
	else if(isType<TypeAutoArray>(type))
	{
		Print(ctx, "NULLCAutoArray");
	}
	else if(TypeRef *typeRef = getType<TypeRef>(type))
	{
		TranslateTypeName(ctx, typeRef->subType);
		Print(ctx, "*");
	}
	else if(TypeArray *typeArray = getType<TypeArray>(type))
	{
		PrintEscapedName(ctx, typeArray->name);
	}
	else if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(type))
	{
		Print(ctx, "NULLCArray< ");
		TranslateTypeName(ctx, typeUnsizedArray->subType);
		Print(ctx, " >");
	}
	else if(TypeFunction *typeFunction = getType<TypeFunction>(type))
	{
		Print(ctx, "NULLCFuncPtr<__typeProxy_");
		PrintEscapedName(ctx, typeFunction->name);
		Print(ctx, ">");
	}
	else if(isType<TypeGenericClassProto>(type))
	{
		assert(!"generic type TypeGenericClassProto is not translated");
	}
	else if(isType<TypeGenericClass>(type))
	{
		assert(!"generic type TypeGenericClass is not translated");
	}
	else if(TypeClass *typeClass = getType<TypeClass>(type))
	{
		PrintEscapedTypeName(ctx, typeClass);
	}
	else if(TypeEnum *typeEnum = getType<TypeEnum>(type))
	{
		PrintEscapedName(ctx, typeEnum->name);
	}
	else if(isType<TypeFunctionSet>(type))
	{
		assert(!"virtual type TypeFunctionSet is not translated");
	}
	else if(isType<TypeArgumentSet>(type))
	{
		assert(!"virtual type TypeArgumentSet is not translated");
	}
	else if(isType<TypeMemberSet>(type))
	{
		assert(!"virtual type TypeMemberSet is not translated");
	}
	else
	{
		assert(!"unknown type");
	}
}

void TranslateVariableName(ExpressionTranslateContext &ctx, VariableData *variable)
{
	if(variable->name->name == InplaceStr("this"))
	{
		Print(ctx, "__context");
	}
	else
	{
		if(*variable->name->name.begin == '$')
		{
			Print(ctx, "__");

			PrintEscapedName(ctx, InplaceStr(variable->name->name.begin + 1, variable->name->name.end));
		}
		else
		{
			PrintEscapedName(ctx, variable->name->name);
		}

		if(variable->scope != ctx.ctx.globalScope && !variable->scope->ownerType && !variable->scope->ownerFunction && !variable->scope->ownerNamespace)
			Print(ctx, "_%d", variable->uniqueId);
	}
}

void TranslateTypeDefinition(ExpressionTranslateContext &ctx, TypeBase *type)
{
	if(type->hasTranslation)
		return;

	type->hasTranslation = true;

	if(type->isGeneric)
		return;

	if(TypeFunction *typeFunction = getType<TypeFunction>(type))
	{
		Print(ctx, "struct __typeProxy_");
		PrintEscapedName(ctx, typeFunction->name);
		Print(ctx, "{};");
		PrintLine(ctx);
	}
	else if(TypeArray *typeArray = getType<TypeArray>(type))
	{
		TranslateTypeDefinition(ctx, typeArray->subType);

		Print(ctx, "struct ");
		PrintEscapedName(ctx, typeArray->name);
		PrintLine(ctx);

		PrintIndentedLine(ctx, "{");
		ctx.depth++;

		PrintIndent(ctx);
		TranslateTypeName(ctx, typeArray->subType);
		Print(ctx, " ptr[%lld];", typeArray->length + ((4 - (typeArray->length * typeArray->subType->size % 4)) & 3)); // Round total byte size to 4
		PrintLine(ctx);

		PrintIndent(ctx);
		PrintEscapedName(ctx, typeArray->name);
		Print(ctx, "& set(unsigned index, ");
		TranslateTypeName(ctx, typeArray->subType);
		Print(ctx, " const& val){ ptr[index] = val; return *this; }");
		PrintLine(ctx);

		PrintIndent(ctx);
		TranslateTypeName(ctx, typeArray->subType);
		Print(ctx, "* index(unsigned i){ if(unsigned(i) < %u) return &ptr[i]; nullcThrowError(\"ERROR: array index out of bounds\"); return 0; }", (unsigned)typeArray->length);
		PrintLine(ctx);

		ctx.depth--;
		PrintIndentedLine(ctx, "};");
	}
	else if(TypeClass *typeClass = getType<TypeClass>(type))
	{
		for(MemberHandle *curr = typeClass->members.head; curr; curr = curr->next)
			TranslateTypeDefinition(ctx, curr->variable->type);

		Print(ctx, "struct ");
		PrintEscapedTypeName(ctx, typeClass);
		PrintLine(ctx);

		PrintIndentedLine(ctx, "{");
		ctx.depth++;

		unsigned offset = 0;
		unsigned index = 0;

		for(MemberHandle *curr = typeClass->members.head; curr; curr = curr->next)
		{
			if(curr->variable->offset > offset)
				PrintIndentedLine(ctx, "char pad_%d[%d];", index, int(curr->variable->offset - offset));

			PrintIndent(ctx);

			TranslateTypeName(ctx, curr->variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, curr->variable);
			Print(ctx, ";");
			PrintLine(ctx);

			offset = unsigned(curr->variable->offset + curr->variable->type->size);
			index++;
		}

		if(typeClass->padding != 0)
			PrintIndentedLine(ctx, "char pad_%d[%d];", index, typeClass->padding);

		ctx.depth--;
		PrintIndentedLine(ctx, "};");
	}
	else if(TypeEnum *typeEnum = getType<TypeEnum>(type))
	{
		Print(ctx, "struct ");
		PrintEscapedName(ctx, typeEnum->name);
		PrintLine(ctx);

		PrintIndentedLine(ctx, "{");
		ctx.depth++;

		PrintIndent(ctx);
		PrintEscapedName(ctx, typeEnum->name);
		Print(ctx, "(): value(0){}");
		PrintLine(ctx);

		PrintIndent(ctx);
		PrintEscapedName(ctx, typeEnum->name);
		Print(ctx, "(int v): value(v){}");
		PrintLine(ctx);

		PrintIndentedLine(ctx, "int value;");

		ctx.depth--;
		PrintIndentedLine(ctx, "};");
	}
}

void TranslateFunctionName(ExpressionTranslateContext &ctx, FunctionData *function)
{
	InplaceStr name = function->name->name;

	if(function->implementation)
		function = function->implementation;

	if(*name.begin == '$')
	{
		Print(ctx, "__");

		name = InplaceStr(name.begin + 1, name.end);
	}

	InplaceStr operatorName = GetOperatorName(name);

	if(!operatorName.empty())
		name = operatorName;

	if(function->scope->ownerType)
	{
		if(name.length() > function->scope->ownerType->name.length() + 2)
		{
			InplaceStr namePart = InplaceStr(name.begin + function->scope->ownerType->name.length() + 2, name.end);
			InplaceStr operatorName = GetOperatorName(namePart);

			if(!operatorName.empty())
			{
				PrintEscapedTypeName(ctx, function->scope->ownerType);
				Print(ctx, "__");
				PrintEscapedName(ctx, operatorName);
			}
			else
			{
				PrintEscapedTypeName(ctx, function->scope->ownerType);
				Print(ctx, "__");
				PrintEscapedName(ctx, namePart);
			}
		}
		else
		{
			PrintEscapedName(ctx, name);
		}

		for(MatchData *alias = function->generics.head; alias; alias = alias->next)
		{
			Print(ctx, "_");
			PrintEscapedTypeName(ctx, alias->type);
			Print(ctx, "_");
		}

		Print(ctx, "_");
		PrintEscapedName(ctx, function->type->name);
	}
	else if((function->scope == ctx.ctx.globalScope || function->scope->ownerNamespace) && !function->isHidden)
	{
		PrintEscapedName(ctx, name);

		for(MatchData *alias = function->generics.head; alias; alias = alias->next)
		{
			Print(ctx, "_");
			PrintEscapedTypeName(ctx, alias->type);
			Print(ctx, "_");
		}

		Print(ctx, "_");
		PrintEscapedName(ctx, function->type->name);
	}
	else
	{
		PrintEscapedName(ctx, name);

		for(MatchData *alias = function->generics.head; alias; alias = alias->next)
		{
			Print(ctx, "_");
			PrintEscapedTypeName(ctx, alias->type);
			Print(ctx, "_");
		}

		Print(ctx, "_%d", function->functionIndex);
	}

	if(!name.empty() && *(name.end - 1) == '$')
		Print(ctx, "_");
}

void TranslateVoid(ExpressionTranslateContext &ctx, ExprVoid *expression)
{
	(void)expression;

	Print(ctx, "/*void*/");
}

void TranslateBoolLiteral(ExpressionTranslateContext &ctx, ExprBoolLiteral *expression)
{
	if(expression->value)
		Print(ctx, "true");
	else
		Print(ctx, "false");
}

void TranslateCharacterLiteral(ExpressionTranslateContext &ctx, ExprCharacterLiteral *expression)
{
	Print(ctx, "char(%u)", expression->value);
}

void TranslateStringLiteral(ExpressionTranslateContext &ctx, ExprStringLiteral *expression)
{
	TranslateTypeName(ctx, expression->type);
	Print(ctx, "()");

	for(unsigned i = 0; i < expression->length; i++)
		Print(ctx, ".set(%d, %d)", i, expression->value[i]);
}

void TranslateIntegerLiteral(ExpressionTranslateContext &ctx, ExprIntegerLiteral *expression)
{
	if(expression->type == ctx.ctx.typeShort)
	{
		Print(ctx, "((short)%d)", short(expression->value));
	}
	else if(expression->type == ctx.ctx.typeInt)
	{
		Print(ctx, "%d", int(expression->value));
	}
	else if(expression->type == ctx.ctx.typeLong)
	{
		Print(ctx, "%lldll", expression->value);
	}
	else if(isType<TypeEnum>(expression->type))
	{
		TranslateTypeName(ctx, expression->type);
		Print(ctx, "(%d)", int(expression->value));
	}
	else
	{
		assert(!"unknown type");
	}
}

void TranslateRationalLiteral(ExpressionTranslateContext &ctx, ExprRationalLiteral *expression)
{
	if(expression->type == ctx.ctx.typeFloat)
		Print(ctx, "((float)%e)", expression->value);
	else if(expression->type == ctx.ctx.typeDouble)
		Print(ctx, "%e", expression->value);
	else
		assert(!"unknown type");
}

void TranslateTypeLiteral(ExpressionTranslateContext &ctx, ExprTypeLiteral *expression)
{
	Print(ctx, "__nullcTR[%d]", expression->value->typeIndex);
}

void TranslateNullptrLiteral(ExpressionTranslateContext &ctx, ExprNullptrLiteral *expression)
{
	Print(ctx, "(");
	TranslateTypeName(ctx, expression->type);
	Print(ctx, ")0");
}

void TranslateFunctionIndexLiteral(ExpressionTranslateContext &ctx, ExprFunctionIndexLiteral *expression)
{
	Print(ctx, "__nullcFR[%d]", expression->function->functionIndex);
}

void TranslatePassthrough(ExpressionTranslateContext &ctx, ExprPassthrough *expression)
{
	Translate(ctx, expression->value);
}

void TranslateArray(ExpressionTranslateContext &ctx, ExprArray *expression)
{
	TranslateTypeName(ctx, expression->type);
	Print(ctx, "()");

	unsigned index = 0;

	for(ExprBase *curr = expression->values.head; curr; curr = curr->next)
	{
		Print(ctx, ".set(%d, ", index);
		Translate(ctx, curr);
		Print(ctx, ")");

		index++;
	}
}

void TranslatePreModify(ExpressionTranslateContext &ctx, ExprPreModify *expression)
{
	Print(ctx, expression->isIncrement ? "++(*(" : "--(*(");
	Translate(ctx, expression->value);
	Print(ctx, "))");
}

void TranslatePostModify(ExpressionTranslateContext &ctx, ExprPostModify *expression)
{
	Print(ctx, "(*(");
	Translate(ctx, expression->value);
	Print(ctx, expression->isIncrement ? "))++" : "))--");
}

void TranslateCast(ExpressionTranslateContext &ctx, ExprTypeCast *expression)
{
	switch(expression->category)
	{
	case EXPR_CAST_NUMERICAL:
		Print(ctx, "(");
		TranslateTypeName(ctx, expression->type);
		Print(ctx, ")(");
		Translate(ctx, expression->value);
		Print(ctx, ")");
		break;
	case EXPR_CAST_PTR_TO_BOOL:
		Print(ctx, "(!!(");
		Translate(ctx, expression->value);
		Print(ctx, "))");
		break;
	case EXPR_CAST_UNSIZED_TO_BOOL:
		Print(ctx, "((");
		Translate(ctx, expression->value);
		Print(ctx, ").ptr != 0)");
		break;
	case EXPR_CAST_FUNCTION_TO_BOOL:
		Print(ctx, "((");
		Translate(ctx, expression->value);
		Print(ctx, ").id != 0)");
		break;
	case EXPR_CAST_NULL_TO_PTR:
		if(TypeRef *typeRef = getType<TypeRef>(expression->type))
		{
			Print(ctx, "(");
			TranslateTypeName(ctx, typeRef->subType);
			Print(ctx, "*)0");
		}
		break;
	case EXPR_CAST_NULL_TO_AUTO_PTR:
		Print(ctx, "__nullcMakeAutoRef(0, 0)");
		break;
	case EXPR_CAST_NULL_TO_UNSIZED:
		if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(expression->type))
		{
			Print(ctx, "NULLCArray< ");
			TranslateTypeName(ctx, typeUnsizedArray->subType);
			Print(ctx, " >()");
		}
		break;
	case EXPR_CAST_NULL_TO_AUTO_ARRAY:
		Print(ctx, "__makeAutoArray(0, NULLCArray<void>())");
		break;
	case EXPR_CAST_NULL_TO_FUNCTION:
		TranslateTypeName(ctx, expression->type);
		Print(ctx, "()");
		break;
	case EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
		if(TypeRef *typeRef = getType<TypeRef>(expression->value->type))
		{
			TypeArray *typeArray = getType<TypeArray>(typeRef->subType);

			assert(typeArray);
			assert(unsigned(typeArray->length) == typeArray->length);

			Print(ctx, "__makeNullcArray< ");
			TranslateTypeName(ctx, typeArray->subType);
			Print(ctx, " >(");
			Translate(ctx, expression->value);
			Print(ctx, ", %d)", (unsigned)typeArray->length);
		}
		break;
	case EXPR_CAST_PTR_TO_AUTO_PTR:
		if(TypeRef *typeRef = getType<TypeRef>(expression->value->type))
		{
			TypeClass *classType = getType<TypeClass>(typeRef->subType);

			if(classType && (classType->extendable || classType->baseClass))
			{
				Print(ctx, "__nullcMakeExtendableAutoRef(");
				Translate(ctx, expression->value);
				Print(ctx, ")");
			}
			else
			{
				Print(ctx, "__nullcMakeAutoRef(");
				Translate(ctx, expression->value);
				Print(ctx, ", __nullcTR[%d])", typeRef->subType->typeIndex);
			}
		}
		break;
	case EXPR_CAST_AUTO_PTR_TO_PTR:
		if(TypeRef *typeRef = getType<TypeRef>(expression->type))
		{
			Print(ctx, "(");
			TranslateTypeName(ctx, typeRef->subType);
			Print(ctx, "*)__nullcGetAutoRef(");
			Translate(ctx, expression->value);
			Print(ctx, ", __nullcTR[%d])", typeRef->subType->typeIndex);
		}
		break;
	case EXPR_CAST_UNSIZED_TO_AUTO_ARRAY:
		if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(expression->value->type))
		{
			Print(ctx, "__makeAutoArray(__nullcTR[%d], ", typeUnsizedArray->subType->typeIndex);
			Translate(ctx, expression->value);
			Print(ctx, ")");
		}
		break;
	case EXPR_CAST_REINTERPRET:
		if(isType<TypeUnsizedArray>(expression->type) && isType<TypeUnsizedArray>(expression->value->type))
		{
			Translate(ctx, expression->value);
		}
		else if(expression->type == ctx.ctx.typeInt && expression->value->type == ctx.ctx.typeTypeID)
		{
			Print(ctx, "(int)(");
			Translate(ctx, expression->value);
			Print(ctx, ")");
		}
		else if(isType<TypeEnum>(expression->type) && expression->value->type == ctx.ctx.typeInt)
		{
			TranslateTypeName(ctx, expression->type);
			Print(ctx, "(");
			Translate(ctx, expression->value);
			Print(ctx, ")");
		}
		else if(expression->type == ctx.ctx.typeInt && isType<TypeEnum>(expression->value->type))
		{
			Translate(ctx, expression->value);
			Print(ctx, ".value");
		}
		else if(isType<TypeRef>(expression->type) && isType<TypeRef>(expression->value->type))
		{
			Print(ctx, "(");
			TranslateTypeName(ctx, expression->type);
			Print(ctx, ")(");
			Translate(ctx, expression->value);
			Print(ctx, ")");
		}
		else if(isType<TypeFunction>(expression->type) && isType<TypeFunction>(expression->value->type))
		{
			TranslateTypeName(ctx, expression->type);
			Print(ctx, "(");
			Translate(ctx, expression->value);
			Print(ctx, ")");
		}
		else
		{
			assert(!"unknown cast");
		}
		break;
	default:
		assert(!"unknown cast");
	}
}

void TranslateUnaryOp(ExpressionTranslateContext &ctx, ExprUnaryOp *expression)
{
	switch(expression->op)
	{
	case SYN_UNARY_OP_PLUS:
		Print(ctx, "+(");
		break;
	case SYN_UNARY_OP_NEGATE:
		Print(ctx, "-(");
		break;
	case SYN_UNARY_OP_BIT_NOT:
		Print(ctx, "~(");
		break;
	case SYN_UNARY_OP_LOGICAL_NOT:
		Print(ctx, "!(");
		break;
	default:
		assert(!"unknown type");
	}

	Translate(ctx, expression->value);
	Print(ctx, ")");
}

void TranslateBinaryOp(ExpressionTranslateContext &ctx, ExprBinaryOp *expression)
{
	if(isType<TypeEnum>(expression->type))
	{
		TranslateTypeName(ctx, expression->type);
		Print(ctx, "(");
	}

	if(expression->op == SYN_BINARY_OP_POW)
	{
		Print(ctx, "__nullcPow((");
		Translate(ctx, expression->lhs);
		Print(ctx, ")");

		if(isType<TypeEnum>(expression->lhs->type))
			Print(ctx, ".value");

		Print(ctx, ", (");
		Translate(ctx, expression->rhs);
		Print(ctx, ")");

		if(isType<TypeEnum>(expression->rhs->type))
			Print(ctx, ".value");

		Print(ctx, ")");
	}
	else if(expression->op == SYN_BINARY_OP_MOD && (expression->lhs->type == ctx.ctx.typeFloat || expression->lhs->type == ctx.ctx.typeDouble))
	{
		Print(ctx, "__nullcMod((");
		Translate(ctx, expression->lhs);
		Print(ctx, "), (");
		Translate(ctx, expression->rhs);
		Print(ctx, "))");
	}
	else if(expression->op == SYN_BINARY_OP_LOGICAL_XOR)
	{
		Print(ctx, "!!(");
		Translate(ctx, expression->lhs);
		Print(ctx, ")");

		if(isType<TypeEnum>(expression->lhs->type))
			Print(ctx, ".value");

		Print(ctx, " != !!(");
		Translate(ctx, expression->rhs);
		Print(ctx, ")");

		if(isType<TypeEnum>(expression->rhs->type))
			Print(ctx, ".value");
	}
	else
	{
		Print(ctx, "(");
		Translate(ctx, expression->lhs);
		Print(ctx, ")");

		if(isType<TypeEnum>(expression->lhs->type))
			Print(ctx, ".value");

		switch(expression->op)
		{
		case SYN_BINARY_OP_ADD:
			Print(ctx, " + ");
			break;
		case SYN_BINARY_OP_SUB:
			Print(ctx, " - ");
			break;
		case SYN_BINARY_OP_MUL:
			Print(ctx, " * ");
			break;
		case SYN_BINARY_OP_DIV:
			Print(ctx, " / ");
			break;
		case SYN_BINARY_OP_MOD:
			Print(ctx, " %% ");
			break;
		case SYN_BINARY_OP_SHL:
			Print(ctx, " << ");
			break;
		case SYN_BINARY_OP_SHR:
			Print(ctx, " >> ");
			break;
		case SYN_BINARY_OP_LESS:
			Print(ctx, " < ");
			break;
		case SYN_BINARY_OP_LESS_EQUAL:
			Print(ctx, " <= ");
			break;
		case SYN_BINARY_OP_GREATER:
			Print(ctx, " > ");
			break;
		case SYN_BINARY_OP_GREATER_EQUAL:
			Print(ctx, " >= ");
			break;
		case SYN_BINARY_OP_EQUAL:
			Print(ctx, " == ");
			break;
		case SYN_BINARY_OP_NOT_EQUAL:
			Print(ctx, " != ");
			break;
		case SYN_BINARY_OP_BIT_AND:
			Print(ctx, " & ");
			break;
		case SYN_BINARY_OP_BIT_OR:
			Print(ctx, " | ");
			break;
		case SYN_BINARY_OP_BIT_XOR:
			Print(ctx, " ^ ");
			break;
		case SYN_BINARY_OP_LOGICAL_AND:
			Print(ctx, " && ");
			break;
		case SYN_BINARY_OP_LOGICAL_OR:
			Print(ctx, " || ");
			break;
		default:
			assert(!"unknown type");
		}

		Print(ctx, "(");
		Translate(ctx, expression->rhs);
		Print(ctx, ")");

		if(isType<TypeEnum>(expression->rhs->type))
			Print(ctx, ".value");
	}

	if(isType<TypeEnum>(expression->type))
	{
		Print(ctx, ")");
	}
}

void TranslateGetAddress(ExpressionTranslateContext &ctx, ExprGetAddress *expression)
{
	Print(ctx, "&");

	if(expression->variable->variable->scope == ctx.ctx.globalScope)
		Print(ctx, "::");

	TranslateVariableName(ctx, expression->variable->variable);
}

void TranslateDereference(ExpressionTranslateContext &ctx, ExprDereference *expression)
{
	Print(ctx, "*(");
	Translate(ctx, expression->value);
	Print(ctx, ")");
}

void TranslateUnboxing(ExpressionTranslateContext &ctx, ExprUnboxing *expression)
{
	Translate(ctx, expression->value);
}

void TranslateConditional(ExpressionTranslateContext &ctx, ExprConditional *expression)
{
	Print(ctx, "(");
	Translate(ctx, expression->condition);
	Print(ctx, ") ? (");
	Translate(ctx, expression->trueBlock);
	Print(ctx, ") : (");
	Translate(ctx, expression->falseBlock);
	Print(ctx, ")");
}

void TranslateAssignment(ExpressionTranslateContext &ctx, ExprAssignment *expression)
{
	Print(ctx, "*(");
	Translate(ctx, expression->lhs);
	Print(ctx, ") = (");
	Translate(ctx, expression->rhs);
	Print(ctx, ")");
}

void TranslateMemberAccess(ExpressionTranslateContext &ctx, ExprMemberAccess *expression)
{
	Print(ctx, "&(");
	Translate(ctx, expression->value);
	Print(ctx, ")->");
	TranslateVariableName(ctx, expression->member->variable);
}

void TranslateArrayIndex(ExpressionTranslateContext &ctx, ExprArrayIndex *expression)
{
	if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(expression->value->type))
	{
		Print(ctx, "__nullcIndexUnsizedArray(");
		Translate(ctx, expression->value);
		Print(ctx, ", ");
		Translate(ctx, expression->index);
		Print(ctx, ", %lld)", typeUnsizedArray->subType->size);
	}
	else
	{
		Print(ctx, "(");
		Translate(ctx, expression->value);
		Print(ctx, ")->index(");
		Translate(ctx, expression->index);
		Print(ctx, ")");
	}
}

void TranslateReturn(ExpressionTranslateContext &ctx, ExprReturn *expression)
{
	if(expression->coroutineStateUpdate)
	{
		Translate(ctx, expression->coroutineStateUpdate);
		Print(ctx, ";");
		PrintLine(ctx);
		PrintIndent(ctx);
	}

	ExprSequence *closures = getType<ExprSequence>(expression->closures);

	if(closures && closures->expressions.empty())
		closures = NULL;

	if(!ctx.currentFunction)
	{
		assert(!closures);

		if(expression->value->type == ctx.ctx.typeBool || expression->value->type == ctx.ctx.typeChar || expression->value->type == ctx.ctx.typeShort || expression->value->type == ctx.ctx.typeInt)
			Print(ctx, "__nullcOutputResultInt((int)(");
		else if(expression->value->type == ctx.ctx.typeLong)
			Print(ctx, "__nullcOutputResultLong((long long)(");
		else if(expression->value->type == ctx.ctx.typeFloat || expression->value->type == ctx.ctx.typeDouble)
			Print(ctx, "__nullcOutputResultDouble((double)(");
		else if(isType<TypeEnum>(expression->value->type))
			Print(ctx, "__nullcOutputResultInt((int)(");
		else
			assert(!"unknown global return type");

		Translate(ctx, expression->value);

		if(isType<TypeEnum>(expression->value->type))
			Print(ctx, ".value");

		Print(ctx, "));");
		PrintLine(ctx);
		PrintIndent(ctx);

		Print(ctx, "return 0;");
	}
	else
	{
		if(expression->value->type == ctx.ctx.typeVoid)
		{
			if(closures)
			{
				Translate(ctx, expression->closures);
				Print(ctx, ";");
				PrintLine(ctx);
				PrintIndent(ctx);
			}

			Translate(ctx, expression->value);
			Print(ctx, ";");
			PrintLine(ctx);
			PrintIndent(ctx);

			Print(ctx, "return;");
		}
		else if(closures)
		{
			Print(ctx, "__nullcReturnValue_%d = ", ctx.nextReturnValueId);
			Translate(ctx, expression->value);
			Print(ctx, ";");
			PrintLine(ctx);
			PrintIndent(ctx);

			Translate(ctx, expression->closures);
			Print(ctx, ";");
			PrintLine(ctx);
			PrintIndent(ctx);

			Print(ctx, "return __nullcReturnValue_%d;", ctx.nextReturnValueId);
		}
		else
		{
			Print(ctx, "return ");
			Translate(ctx, expression->value);
			Print(ctx, ";");
		}
	}
}

void TranslateYield(ExpressionTranslateContext &ctx, ExprYield *expression)
{
	if(expression->coroutineStateUpdate)
	{
		Translate(ctx, expression->coroutineStateUpdate);
		Print(ctx, ";");
		PrintLine(ctx);
		PrintIndent(ctx);
	}

	ExprSequence *closures = getType<ExprSequence>(expression->closures);

	if(closures && closures->expressions.empty())
		closures = NULL;

	if(expression->value->type == ctx.ctx.typeVoid)
	{
		if(closures)
		{
			Translate(ctx, expression->closures);
			Print(ctx, ";");
			PrintLine(ctx);
			PrintIndent(ctx);
		}

		Translate(ctx, expression->value);
		Print(ctx, ";");
		PrintLine(ctx);
		PrintIndent(ctx);

		Print(ctx, "return;");
	}
	else if(closures)
	{
		Print(ctx, "__nullcReturnValue_%d = ", ctx.nextReturnValueId);
		Translate(ctx, expression->value);
		Print(ctx, ";");
		PrintLine(ctx);
		PrintIndent(ctx);

		Translate(ctx, expression->closures);
		Print(ctx, ";");
		PrintLine(ctx);
		PrintIndent(ctx);

		Print(ctx, "return __nullcReturnValue_%d;", ctx.nextReturnValueId);
	}
	else
	{
		Print(ctx, "return ");
		Translate(ctx, expression->value);
		Print(ctx, ";");
	}

	PrintLine(ctx);

	Print(ctx, "yield_%d:", ctx.currentFunction->nextTranslateRestoreBlock++);
}

void TranslateVariableDefinition(ExpressionTranslateContext &ctx, ExprVariableDefinition *expression)
{
	VariableData *variable = expression->variable->variable;

	Print(ctx, "/* Definition of variable '%.*s' */", FMT_ISTR(variable->name->name));

	TypeBase *type = variable->type;

	if(expression->initializer)
	{
		// Zero-initialize classes beforehand (if they are stored in a real local)
		bool zeroInitialize = !(variable->isVmAlloca || variable->lookupOnly) && isType<TypeClass>(type);

		if(zeroInitialize)
		{
			assert(!(variable->isVmAlloca || variable->lookupOnly));

			Print(ctx, "(memset(&");
			TranslateVariableName(ctx, variable);
			Print(ctx, ", 0, sizeof(");
			TranslateVariableName(ctx, variable);
			Print(ctx, ")), ");
		}

		if(ExprBlock *blockInitializer = getType<ExprBlock>(expression->initializer))
		{
			// Translate block initializer as a sequence
			Print(ctx, "(");
			PrintLine(ctx);

			ctx.depth++;

			for(ExprBase *curr = blockInitializer->expressions.head; curr; curr = curr->next)
			{
				PrintIndent(ctx);

				Translate(ctx, curr);

				if(curr->next)
					Print(ctx, ",");

				PrintLine(ctx);
			}

			ctx.depth--;

			PrintIndent(ctx);
			Print(ctx, ")");
		}
		else
		{
			Translate(ctx, expression->initializer);
		}

		if(zeroInitialize)
			Print(ctx, ")");
	}
	else if(variable->isVmAlloca || variable->lookupOnly || variable->scope->ownerNamespace)
	{
		Print(ctx, "0");
	}
	else
	{
		if(isType<TypeVoid>(type))
		{
			Print(ctx, "0");
		}
		else if(isType<TypeBool>(type))
		{
			TranslateVariableName(ctx, variable);
			Print(ctx, " = false");
		}
		else if(isType<TypeChar>(type) || isType<TypeShort>(type) || isType<TypeInt>(type) || isType<TypeLong>(type) || isType<TypeTypeID>(type) || isType<TypeFunctionID>(type) || isType<TypeNullptr>(type) || isType<TypeRef>(type))
		{
			TranslateVariableName(ctx, variable);
			Print(ctx, " = 0");
		}
		else if(isType<TypeFloat>(type))
		{
			TranslateVariableName(ctx, variable);
			Print(ctx, " = 0.0f");
		}
		else if(isType<TypeDouble>(type))
		{
			TranslateVariableName(ctx, variable);
			Print(ctx, " = 0.0");
		}
		else if(isType<TypeAutoRef>(type) || isType<TypeAutoArray>(type) || isType<TypeArray>(type) || isType<TypeUnsizedArray>(type) || isType<TypeFunction>(type) || isType<TypeClass>(type) || isType<TypeEnum>(type))
		{
			Print(ctx, "memset(&");
			TranslateVariableName(ctx, variable);
			Print(ctx, ", 0, sizeof(");
			TranslateVariableName(ctx, variable);
			Print(ctx, "))");
		}
		else
		{
			assert(!"unknown type");
		}
	}
}

void TranslateArraySetup(ExpressionTranslateContext &ctx, ExprArraySetup *expression)
{
	TypeRef *refType = getType<TypeRef>(expression->lhs->type);

	assert(refType);

	TypeArray *arrayType = getType<TypeArray>(refType->subType);

	assert(arrayType);

	Print(ctx, "__nullcSetupArray((");
	Translate(ctx, expression->lhs);
	Print(ctx, ")->ptr, %u, ", (unsigned)arrayType->length);
	Translate(ctx, expression->initializer);
	Print(ctx, ")");
}

void TranslateVariableDefinitions(ExpressionTranslateContext &ctx, ExprVariableDefinitions *expression)
{
	for(ExprBase *value = expression->definitions.head; value; value = value->next)
	{
		Translate(ctx, value);

		if(value->next)
			Print(ctx, ", ");
	}
}

void TranslateVariableAccess(ExpressionTranslateContext &ctx, ExprVariableAccess *expression)
{
	if(expression->variable->type == ctx.ctx.typeVoid)
	{
		Print(ctx, "void()");
		return;
	}

	if(expression->variable->scope == ctx.ctx.globalScope)
		Print(ctx, "::");

	TranslateVariableName(ctx, expression->variable);
}

void TranslateFunctionContextAccess(ExpressionTranslateContext &ctx, ExprFunctionContextAccess *expression)
{
	TypeRef *refType = getType<TypeRef>(expression->function->contextType);

	assert(refType);

	TypeClass *classType = getType<TypeClass>(refType->subType);

	assert(classType);

	if(classType->size == 0)
	{
		Print(ctx, "(");
		TranslateTypeName(ctx, expression->type);
		Print(ctx, ")0");
	}
	else
	{
		TranslateVariableName(ctx, expression->contextVariable);
	}
}

void TranslateFunctionDefinition(ExpressionTranslateContext &ctx, ExprFunctionDefinition *expression)
{
	if(!ctx.skipFunctionDefinitions)
	{
		// Skip nested definitions
		ctx.skipFunctionDefinitions = true;

		FunctionData *function = expression->function;

		bool isStatic = false;
		bool isGeneric = false;

		if(function->scope != ctx.ctx.globalScope && !function->scope->ownerNamespace && !function->scope->ownerType)
			isStatic = true;
		else if(*function->name->name.begin == '$')
			isStatic = true;
		else if(function->isHidden)
			isStatic = true;
		else if(ctx.ctx.IsGenericInstance(function))
			isGeneric = true;

		if(isStatic)
			Print(ctx, "static ");
		else if(isGeneric)
			Print(ctx, "template<int I> ");

		TranslateTypeName(ctx, function->type->returnType);
		Print(ctx, " ");
		TranslateFunctionName(ctx, function);
		Print(ctx, "(");

		for(ExprVariableDefinition *curr = expression->arguments.head; curr; curr = getType<ExprVariableDefinition>(curr->next))
		{
			TranslateTypeName(ctx, curr->variable->variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, curr->variable->variable);
			Print(ctx, ", ");
		}

		TranslateTypeName(ctx, expression->contextArgument->variable->variable->type);
		Print(ctx, " ");
		TranslateVariableName(ctx, expression->contextArgument->variable->variable);

		Print(ctx, ")");
		PrintLine(ctx);
		PrintIndentedLine(ctx, "{");
		ctx.depth++;

		for(unsigned k = 0; k < function->functionScope->allVariables.size(); k++)
		{
			VariableData *variable = function->functionScope->allVariables[k];

			// Don't need variables allocated by intermediate vm compilation
			if(variable->isVmAlloca)
				continue;

			if(variable->lookupOnly)
				continue;

			bool isArgument = false;

			for(ExprVariableDefinition *curr = expression->arguments.head; curr; curr = getType<ExprVariableDefinition>(curr->next))
			{
				if(variable == curr->variable->variable)
				{
					isArgument = true;
					break;
				}
			}

			if(isArgument || variable == expression->contextArgument->variable->variable)
				continue;

			if(variable->type == ctx.ctx.typeVoid)
				continue;

			PrintIndent(ctx);
			TranslateTypeName(ctx, variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, variable);
			Print(ctx, ";");
			PrintLine(ctx);
		}

		if(function->type->returnType != ctx.ctx.typeVoid)
		{
			PrintIndent(ctx);
			TranslateTypeName(ctx, function->type->returnType);
			Print(ctx, " __nullcReturnValue_%d;", ctx.nextReturnValueId);
			PrintLine(ctx);
		}

		if(expression->coroutineStateRead)
		{
			PrintIndent(ctx);
			Print(ctx, "int __currJmpOffset = ");
			Translate(ctx, expression->coroutineStateRead);
			Print(ctx, ";");
			PrintLine(ctx);

			for(unsigned i = 0; i < function->yieldCount; i++)
			{
				PrintIndentedLine(ctx, "if(__currJmpOffset == %d)", i + 1);

				ctx.depth++;

				PrintIndentedLine(ctx, "goto yield_%d;", i + 1);

				ctx.depth--;
			}
		}

		for(ExprBase *value = expression->expressions.head; value; value = value->next)
		{
			PrintIndent(ctx);

			Translate(ctx, value);

			Print(ctx, ";");

			PrintLine(ctx);
		}


		ctx.depth--;

		PrintIndentedLine(ctx, "}");

		ctx.nextReturnValueId++;

		if(isGeneric)
		{
			Print(ctx, "template ");

			TranslateTypeName(ctx, function->type->returnType);
			Print(ctx, " ");
			TranslateFunctionName(ctx, function);
			Print(ctx, "<0>(");

			for(ExprVariableDefinition *curr = expression->arguments.head; curr; curr = getType<ExprVariableDefinition>(curr->next))
			{
				TranslateTypeName(ctx, curr->variable->variable->type);
				Print(ctx, " ");
				TranslateVariableName(ctx, curr->variable->variable);
				Print(ctx, ", ");
			}

			TranslateTypeName(ctx, expression->contextArgument->variable->variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, expression->contextArgument->variable->variable);

			Print(ctx, ");");
			PrintLine(ctx);
		}

		ctx.skipFunctionDefinitions = false;
	}
	else
	{
		Print(ctx, "(");
		TranslateTypeName(ctx, expression->type);
		Print(ctx, ")");
		Print(ctx, "__nullcMakeFunction(__nullcFR[%d], 0)", expression->function->functionIndex);
	}
}

void TranslateGenericFunctionPrototype(ExpressionTranslateContext &ctx, ExprGenericFunctionPrototype *expression)
{
	Print(ctx, "/* Definition of generic function prototype '%.*s' */", FMT_ISTR(expression->function->name->name));

	if(!expression->contextVariables.head)
	{
		Print(ctx, "0");
		return;
	}

	Print(ctx, "(");
	PrintLine(ctx);

	ctx.depth++;

	for(ExprBase *curr = expression->contextVariables.head; curr; curr = curr->next)
	{
		PrintIndent(ctx);

		Translate(ctx, curr);

		if(curr->next)
			Print(ctx, ", ");

		PrintLine(ctx);
	}

	ctx.depth--;

	PrintIndent(ctx);
	Print(ctx, ")");
}

void TranslateFunctionAccess(ExpressionTranslateContext &ctx, ExprFunctionAccess *expression)
{
	Print(ctx, "(");
	TranslateTypeName(ctx, expression->type);
	Print(ctx, ")");
	Print(ctx, "__nullcMakeFunction(__nullcFR[%d], ", expression->function->functionIndex);
	Translate(ctx, expression->context);
	Print(ctx, ")");
}

void TranslateFunctionCall(ExpressionTranslateContext &ctx, ExprFunctionCall *expression)
{
	if(ExprFunctionAccess *functionAccess = getType<ExprFunctionAccess>(expression->function))
	{
		TranslateFunctionName(ctx, functionAccess->function);

		if(UseNonStaticTemplate(ctx, functionAccess->function))
			Print(ctx, "<0>");

		Print(ctx, "(");

		for(ExprBase *value = expression->arguments.head; value; value = value->next)
		{
			Translate(ctx, value);

			Print(ctx, ", ");
		}

		Translate(ctx, functionAccess->context);

		Print(ctx, ")");
	}
	else if(TypeFunction *typeFunction = getType<TypeFunction>(expression->function->type))
	{
		// TODO: side effects may be performed multiple times since the node is translated two times
		Print(ctx, "((");
		TranslateTypeName(ctx, typeFunction->returnType);
		Print(ctx, "(*)(");

		for(TypeHandle *curr = typeFunction->arguments.head; curr; curr = curr->next)
		{
			TranslateTypeName(ctx, curr->type);

			Print(ctx, ", ");
		}

		Print(ctx, "void*))(*__nullcFM)[(");
		Translate(ctx, expression->function);
		Print(ctx, ").id])(");

		for(ExprBase *value = expression->arguments.head; value; value = value->next)
		{
			Translate(ctx, value);

			Print(ctx, ", ");
		}

		Print(ctx, "(");
		Translate(ctx, expression->function);
		Print(ctx, ").context)");
	}
	else
	{
		assert(!"unknwon type");
	}
}

void TranslateAliasDefinition(ExpressionTranslateContext &ctx, ExprAliasDefinition *expression)
{
	Print(ctx, "/* Definition of class typedef '%.*s' */", FMT_ISTR(expression->alias->name->name));

	Print(ctx, "0");
}

void TranslateClassPrototype(ExpressionTranslateContext &ctx, ExprClassPrototype *expression)
{
	Print(ctx, "/* Definition of class prototype '%.*s' */", FMT_ISTR(expression->classType->name));

	Print(ctx, "0");
}

void TranslateGenericClassPrototype(ExpressionTranslateContext &ctx, ExprGenericClassPrototype *expression)
{
	Print(ctx, "/* Definition of generic class prototype '%.*s' */", FMT_ISTR(expression->genericProtoType->name));

	Print(ctx, "0");
}

void TranslateClassDefinition(ExpressionTranslateContext &ctx, ExprClassDefinition *expression)
{
	Print(ctx, "/* Definition of class '%.*s' */", FMT_ISTR(expression->classType->name));

	Print(ctx, "0");
}

void TranslateEnumDefinition(ExpressionTranslateContext &ctx, ExprEnumDefinition *expression)
{
	Print(ctx, "/* Definition of enum '%.*s' */", FMT_ISTR(expression->enumType->name));

	Print(ctx, "0");
}

void TranslateIfElse(ExpressionTranslateContext &ctx, ExprIfElse *expression)
{
	Print(ctx, "if(");
	Translate(ctx, expression->condition);
	Print(ctx, ")");
	PrintLine(ctx);

	PrintIndentedLine(ctx, "{");
	ctx.depth++;
	PrintIndent(ctx);

	Translate(ctx, expression->trueBlock);
	
	Print(ctx, ";");

	PrintLine(ctx);
	ctx.depth--;
	PrintIndentedLine(ctx, "}");

	if(expression->falseBlock)
	{
		PrintIndentedLine(ctx, "else");

		PrintIndentedLine(ctx, "{");
		ctx.depth++;
		PrintIndent(ctx);

		Translate(ctx, expression->falseBlock);

		Print(ctx, ";");

		PrintLine(ctx);
		ctx.depth--;
		PrintIndentedLine(ctx, "}");
	}
}

void TranslateFor(ExpressionTranslateContext &ctx, ExprFor *expression)
{
	unsigned loopBreakId = ctx.nextLoopBreakId++;
	unsigned loopContinueId = ctx.nextLoopContinueId++;
	ctx.loopBreakIdStack.push_back(loopBreakId);
	ctx.loopContinueIdStack.push_back(loopContinueId);

	Translate(ctx, expression->initializer);
	Print(ctx, ";");
	PrintLine(ctx);

	PrintIndent(ctx);
	Print(ctx, "while(");
	Translate(ctx, expression->condition);
	Print(ctx, ")");
	PrintLine(ctx);

	PrintIndentedLine(ctx, "{");
	ctx.depth++;
	PrintIndent(ctx);

	Translate(ctx, expression->body);

	Print(ctx, ";");

	PrintLine(ctx);

	Print(ctx, "continue_%d:;", loopContinueId);
	PrintLine(ctx);

	PrintIndentedLine(ctx, "// Increment");

	PrintIndent(ctx);
	Translate(ctx, expression->increment);

	Print(ctx, ";");

	PrintLine(ctx);
	ctx.depth--;
	PrintIndentedLine(ctx, "}");

	Print(ctx, "break_%d:", loopBreakId);
	PrintLine(ctx);
	PrintIndent(ctx);

	ctx.loopBreakIdStack.pop_back();
	ctx.loopContinueIdStack.pop_back();
}

void TranslateWhile(ExpressionTranslateContext &ctx, ExprWhile *expression)
{
	unsigned loopBreakId = ctx.nextLoopBreakId++;
	unsigned loopContinueId = ctx.nextLoopContinueId++;
	ctx.loopBreakIdStack.push_back(loopBreakId);
	ctx.loopContinueIdStack.push_back(loopContinueId);

	Print(ctx, "while(");
	Translate(ctx, expression->condition);
	Print(ctx, ")");
	PrintLine(ctx);

	PrintIndentedLine(ctx, "{");
	ctx.depth++;
	PrintIndent(ctx);

	Translate(ctx, expression->body);

	Print(ctx, ";");

	Print(ctx, "continue_%d:;", loopContinueId);
	PrintLine(ctx);

	PrintLine(ctx);
	ctx.depth--;
	PrintIndentedLine(ctx, "}");

	Print(ctx, "break_%d:", loopBreakId);
	PrintLine(ctx);
	PrintIndent(ctx);

	ctx.loopBreakIdStack.pop_back();
	ctx.loopContinueIdStack.pop_back();
}

void TranslateDoWhile(ExpressionTranslateContext &ctx, ExprDoWhile *expression)
{
	unsigned loopBreakId = ctx.nextLoopBreakId++;
	unsigned loopContinueId = ctx.nextLoopContinueId++;
	ctx.loopBreakIdStack.push_back(loopBreakId);
	ctx.loopContinueIdStack.push_back(loopContinueId);

	Print(ctx, "do");
	PrintLine(ctx);

	PrintIndentedLine(ctx, "{");
	ctx.depth++;
	PrintIndent(ctx);

	Translate(ctx, expression->body);

	Print(ctx, ";");
	PrintLine(ctx);

	Print(ctx, "continue_%d:;", loopContinueId);
	PrintLine(ctx);

	ctx.depth--;
	PrintIndentedLine(ctx, "}");

	PrintIndent(ctx);
	Print(ctx, "while(");
	Translate(ctx, expression->condition);
	Print(ctx, ");");
	PrintLine(ctx);

	Print(ctx, "break_%d:", loopBreakId);
	PrintLine(ctx);
	PrintIndent(ctx);

	ctx.loopBreakIdStack.pop_back();
	ctx.loopContinueIdStack.pop_back();
}

void TranslateSwitch(ExpressionTranslateContext &ctx, ExprSwitch *expression)
{
	unsigned loopBreakId = ctx.nextLoopBreakId++;
	ctx.loopBreakIdStack.push_back(loopBreakId);

	Print(ctx, "{");
	PrintLine(ctx);
	ctx.depth++;

	PrintIndent(ctx);
	Translate(ctx, expression->condition);
	Print(ctx, ";");
	PrintLine(ctx);

	unsigned i;

	i = 0;
	for(ExprBase *curr = expression->cases.head; curr; curr = curr->next, i++)
	{
		PrintIndent(ctx);
		Print(ctx, "if(");
		Translate(ctx, curr);
		Print(ctx, ")");
		PrintLine(ctx);

		ctx.depth++;
		PrintIndentedLine(ctx, "goto switch_%d_case_%d;", loopBreakId, i);
		ctx.depth--;
	}

	PrintIndentedLine(ctx, "goto switch_%d_default;", loopBreakId);

	i = 0;
	for(ExprBase *curr = expression->blocks.head; curr; curr = curr->next, i++)
	{
		Print(ctx, "switch_%d_case_%d:", loopBreakId, i);
		PrintLine(ctx);

		PrintIndent(ctx);
		Translate(ctx, curr);

		if(curr->next)
			PrintLine(ctx);
	}

	Print(ctx, "switch_%d_default:", loopBreakId);
	PrintLine(ctx);

	if(expression->defaultBlock)
	{
		PrintIndent(ctx);
		Translate(ctx, expression->defaultBlock);
	}
	else
	{
		PrintIndentedLine(ctx, ";");
	}

	PrintLine(ctx);

	ctx.depth--;
	PrintIndentedLine(ctx, "}");

	Print(ctx, "break_%d:", loopBreakId);
	PrintLine(ctx);
	PrintIndent(ctx);

	ctx.loopBreakIdStack.pop_back();
}

void TranslateBreak(ExpressionTranslateContext &ctx, ExprBreak *expression)
{
	if(ExprSequence *closures = getType<ExprSequence>(expression->closures))
	{
		if(!closures->expressions.empty())
		{
			Translate(ctx, expression->closures);
			Print(ctx, ";");
			PrintLine(ctx);
			PrintIndent(ctx);
		}
	}

	Print(ctx, "goto break_%d;", ctx.loopBreakIdStack[ctx.loopBreakIdStack.size() - expression->depth]);
}

void TranslateContinue(ExpressionTranslateContext &ctx, ExprContinue *expression)
{
	if(ExprSequence *closures = getType<ExprSequence>(expression->closures))
	{
		if(!closures->expressions.empty())
		{
			Translate(ctx, expression->closures);
			Print(ctx, ";");
			PrintLine(ctx);
			PrintIndent(ctx);
		}
	}

	Print(ctx, "goto continue_%d;", ctx.loopContinueIdStack[ctx.loopContinueIdStack.size() - expression->depth]);
}

void TranslateBlock(ExpressionTranslateContext &ctx, ExprBlock *expression)
{
	Print(ctx, "{");
	PrintLine(ctx);

	ctx.depth++;

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		PrintIndent(ctx);

		Translate(ctx, value);

		Print(ctx, ";");

		PrintLine(ctx);
	}

	if(expression->closures)
	{
		PrintIndentedLine(ctx, "// Closures");

		PrintIndent(ctx);

		Translate(ctx, expression->closures);
		Print(ctx, ";");

		PrintLine(ctx);
	}

	ctx.depth--;

	PrintIndent(ctx);
	Print(ctx, "}");
}

void TranslateSequence(ExpressionTranslateContext &ctx, ExprSequence *expression)
{
	if(expression->expressions.empty())
	{
		Print(ctx, "/*empty sequence*/");
		return;
	}

	Print(ctx, "(");
	PrintLine(ctx);

	ctx.depth++;

	for(unsigned i = 0; i < expression->expressions.size(); i++)
	{
		PrintIndent(ctx);

		Translate(ctx, expression->expressions[i]);

		if(i + 1 < expression->expressions.size())
			Print(ctx, ", ");

		PrintLine(ctx);
	}

	ctx.depth--;

	PrintIndent(ctx);
	Print(ctx, ")");
}

const char* GetModuleOutputPath(Allocator *allocator, InplaceStr moduleName)
{
	unsigned length = unsigned(strlen("import_") + moduleName.length() + strlen(".cpp") + 1);
	char *targetName = (char*)allocator->alloc(length);

	char *pos = targetName;

	strcpy(pos, "import_");
	pos += strlen(pos);

	memcpy(pos, moduleName.begin, moduleName.length());
	pos += moduleName.length();

	*pos = 0;

	for(unsigned i = 0; i < strlen(targetName); i++)
	{
		if(targetName[i] == '/' || targetName[i] == '.')
			targetName[i] = '_';
	}

	strcpy(pos, ".cpp");

	return targetName;
}

const char* GetModuleMainName(Allocator *allocator, InplaceStr moduleName)
{
	unsigned length = unsigned(moduleName.length() + + strlen("__init_") + 1);
	char *targetName = (char*)allocator->alloc(length);

	NULLC::SafeSprintf(targetName, length, "__init_%.*s", FMT_ISTR(moduleName));

	for(unsigned i = 0; i < length; i++)
	{
		if(targetName[i] == '/' || targetName[i] == '.')
			targetName[i] = '_';
	}

	return targetName;
}

bool TranslateModuleImports(ExpressionTranslateContext &ctx, SmallArray<const char*, 32> &dependencies)
{
	// Translate all imports (expept base)
	for(unsigned i = 1; i < ctx.ctx.imports.size(); i++)
	{
		ModuleData *data = ctx.ctx.imports[i];

		const char *targetName = GetModuleOutputPath(ctx.allocator, data->name);

		bool found = false;

		for(unsigned k = 0; k < dependencies.size(); k++)
		{
			if(strcmp(dependencies[k], targetName) == 0)
			{
				found = true;
				break;
			}
		}

		if(found)
			continue;

		dependencies.push_back(targetName);

		assert(*data->name.end == 0);
		assert(strstr(data->name.begin, ".nc") != NULL);

		unsigned fileSize = 0;
		char *fileContent = NULL;
		bool bytecodeFile = false;

		const unsigned pathLength = 1024;
		char path[pathLength];

		unsigned modulePathPos = 0;
		while(const char *modulePath = BinaryCache::EnumImportPath(modulePathPos++))
		{
			NULLC::SafeSprintf(path, pathLength, "%s%.*s", modulePath, FMT_ISTR(data->name));

			fileContent = (char*)NULLC::fileLoad(path, &fileSize);

			if(fileContent)
				break;
		}

		if(!fileContent)
		{
			assert(*data->name.end == 0);

			const char *bytecode = BinaryCache::FindBytecode(data->name.begin, false);

			if(!bytecode)
			{
				NULLC::SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: module '%.*s' input file '%s' could not be opened", FMT_ISTR(data->name), path);
				return false;
			}

			fileContent = FindSource((ByteCode*)bytecode);
			bytecodeFile = true;
		}

		CompilerContext compilerCtx(ctx.allocator, 0, ArrayView<InplaceStr>());

		compilerCtx.errorBuf = ctx.errorBuf;
		compilerCtx.errorBufSize = ctx.errorBufSize;

		compilerCtx.outputCtx.openStream = ctx.output.openStream;
		compilerCtx.outputCtx.writeStream = ctx.output.writeStream;
		compilerCtx.outputCtx.closeStream = ctx.output.closeStream;

		compilerCtx.outputCtx.outputBuf = ctx.output.outputBuf;
		compilerCtx.outputCtx.outputBufSize = ctx.output.outputBufSize;

		compilerCtx.outputCtx.tempBuf = ctx.output.tempBuf;
		compilerCtx.outputCtx.tempBufSize = ctx.output.tempBufSize;

		ExprModule* nestedModule = AnalyzeModuleFromSource(compilerCtx, fileContent);

		if(!nestedModule)
		{
			if(ctx.errorPos)
				ctx.errorPos = compilerCtx.errorPos;

			if(ctx.errorBuf && ctx.errorBufSize)
			{
				unsigned currLen = (unsigned)strlen(ctx.errorBuf);
				NULLC::SafeSprintf(ctx.errorBuf + currLen, ctx.errorBufSize - currLen, " [in module '%.*s']", FMT_ISTR(data->name));
			}

			if(!bytecodeFile)
				NULLC::fileFree(fileContent);

			return false;
		}

		ExpressionTranslateContext nested(compilerCtx.exprCtx, compilerCtx.outputCtx, compilerCtx.allocator);

		assert(!compilerCtx.outputCtx.stream);

		compilerCtx.outputCtx.stream = compilerCtx.outputCtx.openStream(targetName);

		if(!compilerCtx.outputCtx.stream)
		{
			NULLC::SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: module '%.*s' output file '%s' could not be opened", FMT_ISTR(data->name), path);

			if(!bytecodeFile)
				NULLC::fileFree(fileContent);

			return false;
		}

		nested.mainName = GetModuleMainName(ctx.allocator, data->name);

		nested.indent = ctx.indent;

		nested.errorBuf = ctx.errorBuf;
		nested.errorBufSize = ctx.errorBufSize;

		if(!TranslateModule(nested, nestedModule, dependencies))
		{
			unsigned currLen = (unsigned)strlen(ctx.errorBuf);
			NULLC::SafeSprintf(ctx.errorBuf + currLen, ctx.errorBufSize - currLen, " [in module '%.*s']", FMT_ISTR(data->name));

			compilerCtx.outputCtx.closeStream(compilerCtx.outputCtx.stream);
			compilerCtx.outputCtx.stream = NULL;

			if(!bytecodeFile)
				NULLC::fileFree(fileContent);

			return false;
		}

		compilerCtx.outputCtx.closeStream(compilerCtx.outputCtx.stream);
		compilerCtx.outputCtx.stream = NULL;

		if(!bytecodeFile)
			NULLC::fileFree(fileContent);
	}

	// Translate all imports (expept base)
	for(unsigned i = 1; i < ctx.ctx.imports.size(); i++)
	{
		ModuleData* data = ctx.ctx.imports[i];

		PrintIndentedLine(ctx, "// Requires '%.*s'", FMT_ISTR(data->name));
	}

	return true;
}

void TranslateModuleTypePrototypes(ExpressionTranslateContext &ctx)
{
	PrintIndentedLine(ctx, "// Type prototypes");

	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
	{
		TypeBase *type = ctx.ctx.types[i];

		if(type->isGeneric)
			continue;

		if(TypeStruct *typeStruct = getType<TypeStruct>(type))
		{
			Print(ctx, "struct ");
			PrintEscapedTypeName(ctx, typeStruct);
			Print(ctx, ";");
			PrintLine(ctx);
		}
		else if(TypeArray *typeArray = getType<TypeArray>(type))
		{
			Print(ctx, "struct ");
			PrintEscapedName(ctx, typeArray->name);
			Print(ctx, ";");
			PrintLine(ctx);
		}
	}

	PrintLine(ctx);
}

void TranslateModuleTypeDefinitions(ExpressionTranslateContext &ctx)
{
	PrintIndentedLine(ctx, "// Type definitions");
	PrintIndentedLine(ctx, "#pragma pack(push, 4)");

	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
	{
		TypeBase *type = ctx.ctx.types[i];

		TranslateTypeDefinition(ctx, type);
	}
	PrintIndentedLine(ctx, "#pragma pack(pop)");

	PrintLine(ctx);
}

void TranslateModuleFunctionPrototypes(ExpressionTranslateContext &ctx)
{
	PrintIndentedLine(ctx, "// Function prototypes");

	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
	{
		FunctionData *function = ctx.ctx.functions[i];

		if(ctx.ctx.IsGenericFunction(function))
			continue;

		if(function->implementation)
			continue;

		bool isStatic = false;
		bool isGeneric = false;

		if(function->scope != ctx.ctx.globalScope && !function->scope->ownerNamespace && !function->scope->ownerType)
			isStatic = true;
		else if(*function->name->name.begin == '$')
			isStatic = true;
		else if(function->isHidden)
			isStatic = true;
		else if(ctx.ctx.IsGenericInstance(function))
			isGeneric = true;

		if(isStatic && function->importModule)
			continue;

		if(isStatic)
			Print(ctx, "static ");
		else if(isGeneric)
			Print(ctx, "template<int I> ");

		TranslateTypeName(ctx, function->type->returnType);
		Print(ctx, " ");
		TranslateFunctionName(ctx, function);
		Print(ctx, "(");

		if(function->importModule)
		{
			for(unsigned i = 0; i < function->arguments.size(); i++)
			{
				ArgumentData &argument = function->arguments[i];

				TranslateTypeName(ctx, argument.type);
				Print(ctx, " ");

				if(argument.name->name == InplaceStr("this"))
				{
					Print(ctx, "__context");
				}
				else if(*argument.name->name.begin == '$')
				{
					InplaceStr name = InplaceStr(argument.name->name.begin + 1, argument.name->name.end);

					Print(ctx, "__%.*s", FMT_ISTR(name));
				}
				else
				{
					Print(ctx, "%.*s", FMT_ISTR(argument.name->name));
				}

				Print(ctx, ", ");
			}

			TranslateTypeName(ctx, function->contextType);
			Print(ctx, " __context");
		}
		else
		{
			for(VariableHandle *curr = function->argumentVariables.head; curr; curr = curr->next)
			{
				TranslateTypeName(ctx, curr->variable->type);
				Print(ctx, " ");
				TranslateVariableName(ctx, curr->variable);
				Print(ctx, ", ");
			}

			TranslateTypeName(ctx, function->contextArgument->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, function->contextArgument);
		}

		Print(ctx, ");");
		PrintLine(ctx);
	}

	PrintLine(ctx);
}

void TranslateModuleGlobalVariables(ExpressionTranslateContext &ctx)
{
	PrintIndentedLine(ctx, "// Global variables");

	for(unsigned int i = 0; i < ctx.ctx.variables.size(); i++)
	{
		VariableData *variable = ctx.ctx.variables[i];

		// Don't need variables allocated by intermediate vm compilation
		if(variable->isVmAlloca)
			continue;

		if(variable->type == ctx.ctx.typeVoid)
			continue;

		InplaceStr name = variable->name->name;

		if(variable->importModule)
		{
			Print(ctx, "extern ");
			TranslateTypeName(ctx, variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, variable);
			Print(ctx, ";");
			PrintLine(ctx);
		}
		else if(name.length() >= 5 && InplaceStr(name.begin, name.begin + 5) == InplaceStr("$temp"))
		{
			Print(ctx, "static ");
			TranslateTypeName(ctx, variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, variable);
			Print(ctx, ";");
			PrintLine(ctx);
		}
		else if(variable->scope == ctx.ctx.globalScope || variable->scope->ownerNamespace)
		{
			TranslateTypeName(ctx, variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, variable);
			Print(ctx, ";");
			PrintLine(ctx);
		}
		else if(ctx.ctx.GlobalScopeFrom(variable->scope))
		{
			Print(ctx, "static ");
			TranslateTypeName(ctx, variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, variable);
			Print(ctx, ";");
			PrintLine(ctx);
		}
	}

	PrintLine(ctx);
}

void TranslateModuleFunctionDefinitions(ExpressionTranslateContext &ctx, ExprModule *expression)
{
	PrintIndentedLine(ctx, "// Function definitions");

	for(unsigned i = 0; i < expression->definitions.size(); i++)
	{
		ExprFunctionDefinition *definition = getType<ExprFunctionDefinition>(expression->definitions[i]);

		if(definition->function->isPrototype)
			continue;

		ctx.currentFunction = definition->function;

		Translate(ctx, definition);

		PrintLine(ctx);
	}

	ctx.currentFunction = NULL;

	ctx.skipFunctionDefinitions = true;

	PrintLine(ctx);
}

void TranslateModuleTypeInformation(ExpressionTranslateContext &ctx)
{
	PrintIndentedLine(ctx, "// Register types");

	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
	{
		TypeBase *type = ctx.ctx.types[i];

		if(type->isGeneric)
		{
			PrintIndentedLine(ctx, "__nullcTR[%d] = 0; // generic type '%.*s'", i, FMT_ISTR(type->name));
			continue;
		}

		PrintIndent(ctx);
		Print(ctx, "__nullcTR[%d] = __nullcRegisterType(", i);
		Print(ctx, "%uu, ", type->nameHash);
		Print(ctx, "\"%.*s\", ", FMT_ISTR(type->name));
		Print(ctx, "%lld, ", type->size);

		if(TypeArray *typeArray = getType<TypeArray>(type))
		{
			Print(ctx, "__nullcTR[%d], ", typeArray->subType->typeIndex);
			Print(ctx, "%d, NULLC_ARRAY, %d, 0);", (unsigned)typeArray->length, typeArray->alignment);
		}
		else if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(type))
		{
			Print(ctx, "__nullcTR[%d], ", typeUnsizedArray->subType->typeIndex);
			Print(ctx, "-1, NULLC_ARRAY, %d, 0);", typeUnsizedArray->alignment);
		}
		else if(TypeRef *typeRef = getType<TypeRef>(type))
		{
			Print(ctx, "__nullcTR[%d], ", typeRef->subType->typeIndex);
			Print(ctx, "1, NULLC_POINTER, %d, 0);", typeRef->alignment);
		}
		else if(TypeFunction *typeFunction = getType<TypeFunction>(type))
		{
			Print(ctx, "__nullcTR[%d], ", typeFunction->returnType->typeIndex);
			Print(ctx, "0, NULLC_FUNCTION, %d, 0);", typeFunction->alignment);
		}
		else if(TypeClass *typeClass = getType<TypeClass>(type))
		{
			unsigned count = 0;

			for(MemberHandle *curr = typeClass->members.head; curr; curr = curr->next)
			{
				if(*curr->variable->name->name.begin == '$')
					continue;

				count++;
			}

			Print(ctx, "__nullcTR[%d], ", typeClass->baseClass ? typeClass->baseClass->typeIndex : 0);
			Print(ctx, "%d, NULLC_CLASS, %d, ", count, typeClass->alignment);

			if(typeClass->hasFinalizer && typeClass->extendable)
				Print(ctx, "NULLC_TYPE_FLAG_HAS_FINALIZER | NULLC_TYPE_FLAG_IS_EXTENDABLE");
			else if(typeClass->hasFinalizer)
				Print(ctx, "NULLC_TYPE_FLAG_HAS_FINALIZER");
			else if(typeClass->extendable)
				Print(ctx, "NULLC_TYPE_FLAG_IS_EXTENDABLE");
			else
				Print(ctx, "0");

			Print(ctx, ");");
		}
		else if(TypeStruct *typeStruct = getType<TypeStruct>(type))
		{
			unsigned count = 0;

			for(MemberHandle *curr = typeStruct->members.head; curr; curr = curr->next)
			{
				if(*curr->variable->name->name.begin == '$')
					continue;

				count++;
			}

			Print(ctx, "__nullcTR[0], ");
			Print(ctx, "%d, NULLC_CLASS, %d, 0);", count, typeStruct->alignment);
		}
		else
		{
			Print(ctx, "__nullcTR[0], ");
			Print(ctx, "0, NULLC_NONE, %d, 0);", type->alignment);
		}

		PrintLine(ctx);
	}

	PrintLine(ctx);

	PrintIndentedLine(ctx, "// Register type members");

	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
	{
		TypeBase *type = ctx.ctx.types[i];

		if(type->isGeneric)
			continue;

		if(TypeFunction *typeFunction = getType<TypeFunction>(type))
		{
			PrintIndent(ctx);
			Print(ctx, "__nullcRegisterMembers(__nullcTR[%d], %d", i, typeFunction->arguments.size());

			for(TypeHandle *curr = typeFunction->arguments.head; curr; curr = curr->next)
			{
				Print(ctx, ", __nullcTR[%d], 0", curr->type->typeIndex);
				Print(ctx, ", 0");
				Print(ctx, ", \"\"");
			}

			Print(ctx, "); // type '%.*s' arguments", FMT_ISTR(type->name));
			PrintLine(ctx);
		}
		else if(TypeStruct *typeStruct = getType<TypeStruct>(type))
		{
			unsigned count = 0;

			for(MemberHandle *curr = typeStruct->members.head; curr; curr = curr->next)
			{
				if(*curr->variable->name->name.begin == '$')
					continue;

				count++;
			}

			PrintIndent(ctx);
			Print(ctx, "__nullcRegisterMembers(__nullcTR[%d], %d", i, count);

			for(MemberHandle *curr = typeStruct->members.head; curr; curr = curr->next)
			{
				if(*curr->variable->name->name.begin == '$')
					continue;

				Print(ctx, ", __nullcTR[%d]", curr->variable->type->typeIndex);
				Print(ctx, ", %d", curr->variable->offset);
				Print(ctx, ", \"%.*s\"", FMT_ISTR(curr->variable->name->name));
			}

			Print(ctx, "); // type '%.*s' members", FMT_ISTR(type->name));
			PrintLine(ctx);
		}
	}

	PrintLine(ctx);
}

void TranslateModuleGlobalVariableInformation(ExpressionTranslateContext &ctx)
{
	PrintIndentedLine(ctx, "// Register globals");

	for(unsigned int i = 0; i < ctx.ctx.variables.size(); i++)
	{
		VariableData *variable = ctx.ctx.variables[i];

		// Don't need variables allocated by intermediate vm compilation
		if(variable->isVmAlloca)
			continue;

		if(variable->importModule)
			continue;

		if(variable->scope == ctx.ctx.globalScope || variable->scope->ownerNamespace || ctx.ctx.GlobalScopeFrom(variable->scope))
		{
			PrintIndent(ctx);
			Print(ctx, "__nullcRegisterGlobal((void*)&");
			TranslateVariableName(ctx, variable);
			Print(ctx, ", __nullcTR[%d]);", variable->type->typeIndex);
			PrintLine(ctx);
		}
	}

	PrintLine(ctx);
}

void TranslateModuleGlobalFunctionInformation(ExpressionTranslateContext &ctx)
{
	PrintIndentedLine(ctx, "// Register functions");

	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
	{
		FunctionData *function = ctx.ctx.functions[i];

		if(ctx.ctx.IsGenericFunction(function))
		{
			PrintIndentedLine(ctx, "__nullcFR[%d] = 0; // generic function '%.*s'", i, FMT_ISTR(function->name->name));
			continue;
		}

		bool isStatic = false;

		if(function->scope != ctx.ctx.globalScope && !function->scope->ownerNamespace && !function->scope->ownerType)
			isStatic = true;
		else if(*function->name->name.begin == '$')
			isStatic = true;
		else if(function->isHidden)
			isStatic = true;

		if(isStatic && function->importModule)
		{
			PrintIndentedLine(ctx, "__nullcFR[%d] = 0; // module '%.*s' internal function '%.*s'", i, FMT_ISTR(function->importModule->name), FMT_ISTR(function->name->name));
			continue;
		}

		PrintIndent(ctx);
		Print(ctx, "__nullcFR[%d] = __nullcRegisterFunction(\"", i);
		TranslateFunctionName(ctx, function);
		Print(ctx, "\", (void*)");
		TranslateFunctionName(ctx, function);

		if(UseNonStaticTemplate(ctx, function))
			Print(ctx, "<0>");

		if(function->contextType)
			Print(ctx, ", __nullcTR[%d]", function->contextType->typeIndex);
		else
			Print(ctx, ", -1");

		if(function->scope->ownerType)
			Print(ctx, ", FunctionCategory::THISCALL);");
		else if(function->coroutine)
			Print(ctx, ", FunctionCategory::COROUTINE);");
		else if(function->contextType != ctx.ctx.typeVoid->refType)
			Print(ctx, ", FunctionCategory::LOCAL);");
		else
			Print(ctx, ", FunctionCategory::NORMAL);");
		PrintLine(ctx);
	}

	PrintLine(ctx);
}

bool TranslateModule(ExpressionTranslateContext &ctx, ExprModule *expression, SmallArray<const char*, 32> &dependencies)
{
	if(!TranslateModuleImports(ctx, dependencies))
		return false;

	// Generate type indexes
	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
	{
		TypeBase *type = ctx.ctx.types[i];

		type->typeIndex = i;

		type->hasTranslation = false;
	}

	// Generate function indexes
	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
	{
		FunctionData *function = ctx.ctx.functions[i];

		function->nextTranslateRestoreBlock = 1;
	}

	PrintIndentedLine(ctx, "#include \"runtime.h\"");
	PrintIndentedLine(ctx, "// Typeid redirect table");
	PrintIndentedLine(ctx, "static unsigned __nullcTR[%d];", ctx.ctx.types.size());
	PrintIndentedLine(ctx, "// Function pointer table");
	PrintIndentedLine(ctx, "static __nullcFunctionArray* __nullcFM;");
	PrintIndentedLine(ctx, "// Function pointer redirect table");
	PrintIndentedLine(ctx, "static unsigned __nullcFR[%d];", ctx.ctx.functions.size());
	PrintLine(ctx);

	TranslateModuleTypePrototypes(ctx);

	TranslateModuleTypeDefinitions(ctx);
	
	TranslateModuleFunctionPrototypes(ctx);

	TranslateModuleGlobalVariables(ctx);
	
	TranslateModuleFunctionDefinitions(ctx, expression);

	PrintIndentedLine(ctx, "// Module initializers");

	for(unsigned i = 1; i < ctx.ctx.imports.size(); i++)
	{
		ModuleData *data = ctx.ctx.imports[i];

		PrintIndentedLine(ctx, "extern int %s();", GetModuleMainName(ctx.allocator, data->name));
	}

	PrintLine(ctx);
	PrintIndentedLine(ctx, "// Global code");

	PrintIndentedLine(ctx, "int %s()", ctx.mainName);
	PrintIndentedLine(ctx, "{");

	ctx.depth++;

	PrintIndentedLine(ctx, "static int moduleInitialized = 0;");
	PrintIndentedLine(ctx, "if(moduleInitialized++)");

	ctx.depth++;
	PrintIndentedLine(ctx, "return 0;");
	ctx.depth--;

	PrintIndentedLine(ctx, "__nullcFM = __nullcGetFunctionTable();");
	PrintIndentedLine(ctx, "int __local = 0;");
	PrintIndentedLine(ctx, "__nullcRegisterBase(&__local);");
	PrintIndentedLine(ctx, "__nullcInitBaseModule();");

	PrintLine(ctx);
	PrintIndentedLine(ctx, "// Initialize modules");

	for(unsigned i = 1; i < ctx.ctx.imports.size(); i++)
	{
		ModuleData *data = ctx.ctx.imports[i];

		PrintIndentedLine(ctx, "%s();", GetModuleMainName(ctx.allocator, data->name));
	}

	PrintLine(ctx);

	TranslateModuleTypeInformation(ctx);
	
	TranslateModuleGlobalVariableInformation(ctx);
	
	TranslateModuleGlobalFunctionInformation(ctx);
	
	PrintIndentedLine(ctx, "// Setup");

	for(ExprBase *value = expression->setup.head; value; value = value->next)
	{
		PrintIndent(ctx);

		Translate(ctx, value);

		Print(ctx, ";");

		PrintLine(ctx);
	}

	PrintLine(ctx);
	PrintIndentedLine(ctx, "// Expressions");

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		PrintIndent(ctx);

		Translate(ctx, value);

		Print(ctx, ";");

		PrintLine(ctx);
	}

	PrintIndentedLine(ctx, "return 0;");

	ctx.depth--;

	PrintIndentedLine(ctx, "}");

	ctx.output.Flush();

	return true;
}

void Translate(ExpressionTranslateContext &ctx, ExprBase *expression)
{
	if(ExprVoid *expr = getType<ExprVoid>(expression))
		TranslateVoid(ctx, expr);
	else if(ExprBoolLiteral *expr = getType<ExprBoolLiteral>(expression))
		TranslateBoolLiteral(ctx, expr);
	else if(ExprCharacterLiteral *expr = getType<ExprCharacterLiteral>(expression))
		TranslateCharacterLiteral(ctx, expr);
	else if(ExprStringLiteral *expr = getType<ExprStringLiteral>(expression))
		TranslateStringLiteral(ctx, expr);
	else if(ExprIntegerLiteral *expr = getType<ExprIntegerLiteral>(expression))
		TranslateIntegerLiteral(ctx, expr);
	else if(ExprRationalLiteral *expr = getType<ExprRationalLiteral>(expression))
		TranslateRationalLiteral(ctx, expr);
	else if(ExprTypeLiteral *expr = getType<ExprTypeLiteral>(expression))
		TranslateTypeLiteral(ctx, expr);
	else if(ExprNullptrLiteral *expr = getType<ExprNullptrLiteral>(expression))
		TranslateNullptrLiteral(ctx, expr);
	else if(ExprFunctionIndexLiteral *expr = getType<ExprFunctionIndexLiteral>(expression))
		TranslateFunctionIndexLiteral(ctx, expr);
	else if(ExprPassthrough *expr = getType<ExprPassthrough>(expression))
		TranslatePassthrough(ctx, expr);
	else if(ExprArray *expr = getType<ExprArray>(expression))
		TranslateArray(ctx, expr);
	else if(ExprPreModify *expr = getType<ExprPreModify>(expression))
		TranslatePreModify(ctx, expr);
	else if(ExprPostModify *expr = getType<ExprPostModify>(expression))
		TranslatePostModify(ctx, expr);
	else if(ExprTypeCast *expr = getType<ExprTypeCast>(expression))
		TranslateCast(ctx, expr);
	else if(ExprUnaryOp *expr = getType<ExprUnaryOp>(expression))
		TranslateUnaryOp(ctx, expr);
	else if(ExprBinaryOp *expr = getType<ExprBinaryOp>(expression))
		TranslateBinaryOp(ctx, expr);
	else if(ExprGetAddress *expr = getType<ExprGetAddress>(expression))
		TranslateGetAddress(ctx, expr);
	else if(ExprDereference *expr = getType<ExprDereference>(expression))
		TranslateDereference(ctx, expr);
	else if(ExprUnboxing *expr = getType<ExprUnboxing>(expression))
		TranslateUnboxing(ctx, expr);
	else if(ExprConditional *expr = getType<ExprConditional>(expression))
		TranslateConditional(ctx, expr);
	else if(ExprAssignment *expr = getType<ExprAssignment>(expression))
		TranslateAssignment(ctx, expr);
	else if(ExprMemberAccess *expr = getType<ExprMemberAccess>(expression))
		TranslateMemberAccess(ctx, expr);
	else if(ExprArrayIndex *expr = getType<ExprArrayIndex>(expression))
		TranslateArrayIndex(ctx, expr);
	else if(ExprReturn *expr = getType<ExprReturn>(expression))
		TranslateReturn(ctx, expr);
	else if(ExprYield *expr = getType<ExprYield>(expression))
		TranslateYield(ctx, expr);
	else if(ExprVariableDefinition *expr = getType<ExprVariableDefinition>(expression))
		TranslateVariableDefinition(ctx, expr);
	else if(ExprArraySetup *expr = getType<ExprArraySetup>(expression))
		TranslateArraySetup(ctx, expr);
	else if(ExprVariableDefinitions *expr = getType<ExprVariableDefinitions>(expression))
		TranslateVariableDefinitions(ctx, expr);
	else if(ExprVariableAccess *expr = getType<ExprVariableAccess>(expression))
		TranslateVariableAccess(ctx, expr);
	else if(ExprFunctionContextAccess *expr = getType<ExprFunctionContextAccess>(expression))
		TranslateFunctionContextAccess(ctx, expr);
	else if(ExprFunctionDefinition *expr = getType<ExprFunctionDefinition>(expression))
		TranslateFunctionDefinition(ctx, expr);
	else if(ExprGenericFunctionPrototype *expr = getType<ExprGenericFunctionPrototype>(expression))
		TranslateGenericFunctionPrototype(ctx, expr);
	else if(ExprFunctionAccess *expr = getType<ExprFunctionAccess>(expression))
		TranslateFunctionAccess(ctx, expr);
	else if(isType<ExprFunctionOverloadSet>(expression))
		assert(!"miscompiled tree");
	else if(isType<ExprShortFunctionOverloadSet>(expression))
		assert(!"miscompiled tree");
	else if(ExprFunctionCall *expr = getType<ExprFunctionCall>(expression))
		TranslateFunctionCall(ctx, expr);
	else if(ExprAliasDefinition *expr = getType<ExprAliasDefinition>(expression))
		TranslateAliasDefinition(ctx, expr);
	else if(ExprClassPrototype *expr = getType<ExprClassPrototype>(expression))
		TranslateClassPrototype(ctx, expr);
	else if(ExprGenericClassPrototype *expr = getType<ExprGenericClassPrototype>(expression))
		TranslateGenericClassPrototype(ctx, expr);
	else if(ExprClassDefinition *expr = getType<ExprClassDefinition>(expression))
		TranslateClassDefinition(ctx, expr);
	else if(ExprEnumDefinition *expr = getType<ExprEnumDefinition>(expression))
		TranslateEnumDefinition(ctx, expr);
	else if(ExprIfElse *expr = getType<ExprIfElse>(expression))
		TranslateIfElse(ctx, expr);
	else if(ExprFor *expr = getType<ExprFor>(expression))
		TranslateFor(ctx, expr);
	else if(ExprWhile *expr = getType<ExprWhile>(expression))
		TranslateWhile(ctx, expr);
	else if(ExprDoWhile *expr = getType<ExprDoWhile>(expression))
		TranslateDoWhile(ctx, expr);
	else if(ExprSwitch *expr = getType<ExprSwitch>(expression))
		TranslateSwitch(ctx, expr);
	else if(ExprBreak *expr = getType<ExprBreak>(expression))
		TranslateBreak(ctx, expr);
	else if(ExprContinue *expr = getType<ExprContinue>(expression))
		TranslateContinue(ctx, expr);
	else if(ExprBlock *expr = getType<ExprBlock>(expression))
		TranslateBlock(ctx, expr);
	else if(ExprSequence *expr = getType<ExprSequence>(expression))
		TranslateSequence(ctx, expr);
	else
		assert(!"unknown type");
}
