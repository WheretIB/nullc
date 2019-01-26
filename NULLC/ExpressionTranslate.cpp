#include "ExpressionTranslate.h"

#include <stdarg.h>

#include "ExpressionTree.h"

// TODO: common
ScopeData* GlobalScopeFrom(ScopeData *scope);

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

void Print(ExpressionTranslateContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	vfprintf(ctx.file, format, args);

	va_end(args);
}

void PrintIndent(ExpressionTranslateContext &ctx)
{
	for(unsigned i = 0; i < ctx.depth; i++)
		fprintf(ctx.file, ctx.indent);
}

void PrintLine(ExpressionTranslateContext &ctx)
{
	fprintf(ctx.file, "\n");
}

void PrintIndentedLine(ExpressionTranslateContext &ctx, const char *format, ...)
{
	PrintIndent(ctx);

	va_list args;
	va_start(args, format);

	vfprintf(ctx.file, format, args);

	va_end(args);

	PrintLine(ctx);
}

void PrintEscapedName(ExpressionTranslateContext &ctx, InplaceStr name)
{
	for(unsigned i = 0; i < name.length(); i++)
	{
		char ch = name.begin[i];

		if(ch == ' ' || ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == ',' || ch == ':' || ch == '$' || ch == '<' || ch == '>')
			fprintf(ctx.file, "_");
		else
			fprintf(ctx.file, "%c", ch);
	}
}

void TranslateTypeName(ExpressionTranslateContext &ctx, TypeBase *type)
{
	if(TypeVoid *typeVoid = getType<TypeVoid>(type))
	{
		Print(ctx, "void");
	}
	else if(TypeBool *typeBool = getType<TypeBool>(type))
	{
		Print(ctx, "bool");
	}
	else if(TypeChar *typeChar = getType<TypeChar>(type))
	{
		Print(ctx, "char");
	}
	else if(TypeShort *typeShort = getType<TypeShort>(type))
	{
		Print(ctx, "short");
	}
	else if(TypeInt *typeInt = getType<TypeInt>(type))
	{
		Print(ctx, "int");
	}
	else if(TypeLong *typeLong = getType<TypeLong>(type))
	{
		Print(ctx, "long long");
	}
	else if(TypeFloat *typeFloat = getType<TypeFloat>(type))
	{
		Print(ctx, "float");
	}
	else if(TypeDouble *typeDouble = getType<TypeDouble>(type))
	{
		Print(ctx, "double");
	}
	else if(TypeTypeID *typeTypeid = getType<TypeTypeID>(type))
	{
		Print(ctx, "unsigned");
	}
	else if(TypeFunctionID *typeFunctionID = getType<TypeFunctionID>(type))
	{
		Print(ctx, "__function");
	}
	else if(TypeGeneric *typeGeneric = getType<TypeGeneric>(type))
	{
		assert(!"generic type TypeGeneric is not translated");
	}
	else if(TypeGenericAlias *typeGenericAlias = getType<TypeGenericAlias>(type))
	{
		assert(!"generic type TypeGenericAlias is not translated");
	}
	else if(TypeAuto *typeAuto = getType<TypeAuto>(type))
	{
		assert(!"virtual type TypeAuto is not translated");
	}
	else if(TypeAutoRef *typeAutoRef = getType<TypeAutoRef>(type))
	{
		Print(ctx, "NULLCRef");
	}
	else if(TypeAutoArray *typeAutoArray = getType<TypeAutoArray>(type))
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
		Print(ctx, "NULLCArray<");
		TranslateTypeName(ctx, typeUnsizedArray->subType);
		Print(ctx, ">");
	}
	else if(TypeFunction *typeFunction = getType<TypeFunction>(type))
	{
		Print(ctx, "NULLCFuncPtr<__typeProxy_");
		PrintEscapedName(ctx, typeFunction->name);
		Print(ctx, ">");
	}
	else if(TypeGenericClassProto *typeGenericClassProto = getType<TypeGenericClassProto>(type))
	{
		assert(!"generic type TypeGenericClassProto is not translated");
	}
	else if(TypeGenericClass *typeGenericClass = getType<TypeGenericClass>(type))
	{
		assert(!"generic type TypeGenericClass is not translated");
	}
	else if(TypeClass *typeClass = getType<TypeClass>(type))
	{
		PrintEscapedName(ctx, typeClass->name);
	}
	else if(TypeEnum *typeEnum = getType<TypeEnum>(type))
	{
		PrintEscapedName(ctx, typeEnum->name);
	}
	else if(TypeFunctionSet *typeFunctionSet = getType<TypeFunctionSet>(type))
	{
		assert(!"virtual type TypeFunctionSet is not translated");
	}
	else if(TypeArgumentSet *typeArgumentSet = getType<TypeArgumentSet>(type))
	{
		assert(!"virtual type TypeArgumentSet is not translated");
	}
	else if(TypeMemberSet *typeMemberSet = getType<TypeMemberSet>(type))
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
	if(variable->name == InplaceStr("this"))
	{
		Print(ctx, "__context");
	}
	else
	{
		if(*variable->name.begin == '$')
		{
			InplaceStr name = InplaceStr(variable->name.begin + 1, variable->name.end);

			Print(ctx, "__%.*s", FMT_ISTR(name));
		}
		else
		{
			Print(ctx, "%.*s", FMT_ISTR(variable->name));
		}

		if(variable->scope != ctx.ctx.globalScope && !variable->scope->ownerType && !variable->scope->ownerFunction && !variable->scope->ownerNamespace)
			Print(ctx, "_%d", variable->uniqueId);
	}
}

void TranslateFunctionName(ExpressionTranslateContext &ctx, FunctionData *function)
{
	InplaceStr name = function->name;

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
		PrintEscapedName(ctx, name);
		Print(ctx, "_");
		PrintEscapedName(ctx, function->type->name);
	}
	else if(function->scope == ctx.ctx.globalScope || function->scope->ownerNamespace)
	{
		PrintEscapedName(ctx, name);
		Print(ctx, "_");
		PrintEscapedName(ctx, function->type->name);
	}
	else
	{
		PrintEscapedName(ctx, name);
		Print(ctx, "_%d", function->functionIndex);
	}

	if(!name.empty() && *(name.end - 1) == '$')
		Print(ctx, "_");
}

void TranslateVoid(ExpressionTranslateContext &ctx, ExprVoid *expression)
{
	(void)expression;

	Print(ctx, "(void)");
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
		Print(ctx, "((short)%d)", short(expression->value));
	else if(expression->type == ctx.ctx.typeInt)
		Print(ctx, "%d", int(expression->value));
	else if(expression->type == ctx.ctx.typeLong)
		Print(ctx, "%lldll", expression->value);
	else if(isType<TypeEnum>(expression->type))
		Print(ctx, "%d", int(expression->value));
	else
		assert(!"unknown type");
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
	Print(ctx, expression->isIncrement ? "++(*(" : "--*(");
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
		Print(ctx, "/*TODO: %.*s ExprTypeCast(EXPR_CAST_UNSIZED_TO_BOOL)*/", FMT_ISTR(expression->type->name));
		break;
	case EXPR_CAST_FUNCTION_TO_BOOL:
		Print(ctx, "/*TODO: %.*s ExprTypeCast(EXPR_CAST_FUNCTION_TO_BOOL)*/", FMT_ISTR(expression->type->name));
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
			Print(ctx, "NULLCArray<");
			TranslateTypeName(ctx, typeUnsizedArray->subType);
			Print(ctx, ">()");
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

			Print(ctx, "__makeNullcArray<");
			TranslateTypeName(ctx, typeArray->subType);
			Print(ctx, ">(");
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
				Print(ctx, "/*TODO: %.*s ExprTypeCast(EXPR_CAST_PTR_TO_AUTO_PTR)*/", FMT_ISTR(expression->type->name));
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
	case EXPR_CAST_DERIVED_TO_BASE:
		Print(ctx, "/*TODO: %.*s ExprTypeCast(EXPR_CAST_DERIVED_TO_BASE)*/", FMT_ISTR(expression->type->name));
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
			Print(ctx, "/*TODO: %.*s ExprTypeCast(EXPR_CAST_REINTERPRET)*/", FMT_ISTR(expression->type->name));
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
		Print(ctx, "+");
		break;
	case SYN_UNARY_OP_NEGATE:
		Print(ctx, "-");
		break;
	case SYN_UNARY_OP_BIT_NOT:
		Print(ctx, "~");
		break;
	case SYN_UNARY_OP_LOGICAL_NOT:
		Print(ctx, "!");
		break;
	default:
		assert(!"unknown type");
	}

	Translate(ctx, expression->value);
}

void TranslateBinaryOp(ExpressionTranslateContext &ctx, ExprBinaryOp *expression)
{
	if(expression->op == SYN_BINARY_OP_POW)
	{
		Print(ctx, "__nullcPow(");
		Translate(ctx, expression->lhs);
		Print(ctx, ", ");
		Translate(ctx, expression->rhs);
		Print(ctx, ")");
	}
	else if(expression->op == SYN_BINARY_OP_LOGICAL_XOR)
	{
		Print(ctx, "!!(");
		Translate(ctx, expression->lhs);
		Print(ctx, ") != !!(");
		Translate(ctx, expression->rhs);
		Print(ctx, ")");
	}
	else
	{
		Print(ctx, "(");
		Translate(ctx, expression->lhs);
		Print(ctx, ")");

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
			Print(ctx, " % ");
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
	}
}

void TranslateGetAddress(ExpressionTranslateContext &ctx, ExprGetAddress *expression)
{
	Print(ctx, "&");
	TranslateVariableName(ctx, expression->variable);
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
	TranslateVariableName(ctx, expression->member);
}

void TranslateArrayIndex(ExpressionTranslateContext &ctx, ExprArrayIndex *expression)
{
	if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(expression->value->type))
	{
		Print(ctx, "((");
		TranslateTypeName(ctx, typeUnsizedArray->subType);
		Print(ctx, "*)&(");
		Translate(ctx, expression->value);
		Print(ctx, ").ptr[");
		Translate(ctx, expression->index);
		Print(ctx, "])");
	}
	else
	{
		Print(ctx, "(&(");
		Translate(ctx, expression->value);
		Print(ctx, ")->ptr[");
		Translate(ctx, expression->index);
		Print(ctx, "])");
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

	if(ExprSequence *closures = getType<ExprSequence>(expression->closures))
	{
		if(closures->expressions.head)
		{
			Translate(ctx, expression->closures);
			Print(ctx, ";");
			PrintLine(ctx);
			PrintIndent(ctx);
		}
	}

	if(!ctx.currentFunction)
	{
		if(expression->value->type == ctx.ctx.typeBool || expression->value->type == ctx.ctx.typeChar || expression->value->type == ctx.ctx.typeShort || expression->value->type == ctx.ctx.typeInt)
			Print(ctx, "__nullcOutputResultInt((int)");
		else if(expression->value->type == ctx.ctx.typeLong)
			Print(ctx, "__nullcOutputResultLong((long long)");
		else if(expression->value->type == ctx.ctx.typeFloat || expression->value->type == ctx.ctx.typeDouble)
			Print(ctx, "__nullcOutputResultDouble((double)");
		else if(isType<TypeEnum>(expression->value->type))
			Print(ctx, "__nullcOutputResultInt((int)");
		else
			assert(!"unknown global return type");

		Translate(ctx, expression->value);

		if(isType<TypeEnum>(expression->value->type))
			Print(ctx, ".value");

		Print(ctx, ");");
		PrintLine(ctx);
		PrintIndent(ctx);

		Print(ctx, "return 0;");
	}
	else
	{
		if(expression->value->type == ctx.ctx.typeVoid)
		{
			Print(ctx, "return;");
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

	if(ExprSequence *closures = getType<ExprSequence>(expression->closures))
	{
		if(closures->expressions.head)
		{
			Translate(ctx, expression->closures);
			Print(ctx, ";");
			PrintLine(ctx);
			PrintIndent(ctx);
		}
	}

	if(expression->value->type == ctx.ctx.typeVoid)
	{
		Print(ctx, "return;");
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
	Print(ctx, "/* Definition of variable '%.*s' */", FMT_ISTR(expression->variable->name));

	if(expression->initializer)
	{
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
	}
	else
	{
		Print(ctx, "0");
	}
}

void TranslateArraySetup(ExpressionTranslateContext &ctx, ExprArraySetup *expression)
{
	Print(ctx, "/*TODO: %.*s ExprArraySetup*/", FMT_ISTR(expression->type->name));
}

void TranslateVariableDefinitions(ExpressionTranslateContext &ctx, ExprVariableDefinitions *expression)
{
	for(ExprVariableDefinition *value = expression->definitions.head; value; value = getType<ExprVariableDefinition>(value->next))
	{
		Translate(ctx, value);

		if(value->next)
			Print(ctx, ", ");
	}
}

void TranslateVariableAccess(ExpressionTranslateContext &ctx, ExprVariableAccess *expression)
{
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
		TranslateVariableName(ctx, expression->function->contextVariable);
	}
}

void TranslateFunctionDefinition(ExpressionTranslateContext &ctx, ExprFunctionDefinition *expression)
{
	if(!ctx.skipFunctionDefinitions)
	{
		// Skip nested definitions
		ctx.skipFunctionDefinitions = true;

		TranslateTypeName(ctx, expression->function->type->returnType);
		Print(ctx, " ");
		TranslateFunctionName(ctx, expression->function);
		Print(ctx, "(");

		for(ExprVariableDefinition *curr = expression->arguments.head; curr; curr = getType<ExprVariableDefinition>(curr->next))
		{
			TranslateTypeName(ctx, curr->variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, curr->variable);
			Print(ctx, ", ");
		}

		TranslateTypeName(ctx, expression->contextArgument->variable->type);
		Print(ctx, " ");
		TranslateVariableName(ctx, expression->contextArgument->variable);

		Print(ctx, ")");
		PrintLine(ctx);
		PrintIndentedLine(ctx, "{");
		ctx.depth++;

		for(unsigned k = 0; k < expression->function->functionScope->allVariables.size(); k++)
		{
			VariableData *variable = expression->function->functionScope->allVariables[k];

			// Don't need variables allocated by intermediate vm compilation
			if(variable->isVmAlloca)
				continue;

			if(variable->lookupOnly)
				continue;

			bool isArgument = false;

			for(ExprVariableDefinition *curr = expression->arguments.head; curr; curr = getType<ExprVariableDefinition>(curr->next))
			{
				if(variable == curr->variable)
				{
					isArgument = true;
					break;
				}
			}

			if(isArgument || variable == expression->contextArgument->variable)
				continue;

			PrintIndent(ctx);
			TranslateTypeName(ctx, variable->type);
			Print(ctx, " ");
			TranslateVariableName(ctx, variable);
			Print(ctx, ";");
			PrintLine(ctx);
		}

		if(expression->coroutineStateRead)
		{
			PrintIndent(ctx);
			Print(ctx, "int __currJmpOffset = ");
			Translate(ctx, expression->coroutineStateRead);
			Print(ctx, ";");
			PrintLine(ctx);

			for(unsigned i = 0; i < expression->function->yieldCount; i++)
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
	Print(ctx, "/* Definition of generic function prototype '%.*s' */", FMT_ISTR(expression->function->name));

	for(ExprBase *value = expression->contextVariables.head; value; value = value->next)
	{
		Translate(ctx, value);
	}
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
	Print(ctx, "/* Definition of class typedef '%.*s' */", FMT_ISTR(expression->alias->name));
}

void TranslateClassPrototype(ExpressionTranslateContext &ctx, ExprClassPrototype *expression)
{
	Print(ctx, "/* Definition of class prototype '%.*s' */", FMT_ISTR(expression->classType->name));
}

void TranslateGenericClassPrototype(ExpressionTranslateContext &ctx, ExprGenericClassPrototype *expression)
{
	Print(ctx, "/* Definition of generic class prototype '%.*s' */", FMT_ISTR(expression->genericProtoType->name));
}

void TranslateClassDefinition(ExpressionTranslateContext &ctx, ExprClassDefinition *expression)
{
	Print(ctx, "/* Definition of class '%.*s' */", FMT_ISTR(expression->classType->name));
}

void TranslateEnumDefinition(ExpressionTranslateContext &ctx, ExprEnumDefinition *expression)
{
	Print(ctx, "/* Definition of enum '%.*s' */", FMT_ISTR(expression->enumType->name));

	Translate(ctx, expression->toInt);
	Translate(ctx, expression->toEnum);
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

	PrintIndentedLine(ctx, "// increment");

	PrintIndent(ctx);
	Translate(ctx, expression->increment);

	Print(ctx, ";");

	PrintLine(ctx);
	ctx.depth--;
	PrintIndentedLine(ctx, "}");
}

void TranslateWhile(ExpressionTranslateContext &ctx, ExprWhile *expression)
{
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
	ctx.depth--;
	PrintIndentedLine(ctx, "}");
}

void TranslateDoWhile(ExpressionTranslateContext &ctx, ExprDoWhile *expression)
{
	Print(ctx, "/*TODO: %.*s ExprDoWhile*/", FMT_ISTR(expression->type->name));
}

void TranslateSwitch(ExpressionTranslateContext &ctx, ExprSwitch *expression)
{
	Print(ctx, "/*TODO: %.*s ExprSwitch*/", FMT_ISTR(expression->type->name));
}

void TranslateBreak(ExpressionTranslateContext &ctx, ExprBreak *expression)
{
	Print(ctx, "/*TODO: %.*s ExprBreak*/", FMT_ISTR(expression->type->name));
}

void TranslateContinue(ExpressionTranslateContext &ctx, ExprContinue *expression)
{
	Print(ctx, "/*TODO: %.*s ExprContinue*/", FMT_ISTR(expression->type->name));
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
		PrintIndentedLine(ctx, "// closures");

		PrintIndent(ctx);

		Translate(ctx, expression->closures);

		PrintLine(ctx);
	}

	ctx.depth--;

	PrintIndent(ctx);
	Print(ctx, "}");
}

void TranslateSequence(ExpressionTranslateContext &ctx, ExprSequence *expression)
{
	if(!expression->expressions.head)
	{
		Print(ctx, "/*empty sequence*/");
		return;
	}

	Print(ctx, "(");

	for(ExprBase *curr = expression->expressions.head; curr; curr = curr->next)
	{
		Translate(ctx, curr);

		if(curr->next)
			Print(ctx, ", ");
	}

	Print(ctx, ")");
}

void TranslateModule(ExpressionTranslateContext &ctx, ExprModule *expression)
{
	// Generate type indexes
	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
	{
		TypeBase *type = ctx.ctx.types[i];

		type->typeIndex = i;
	}

	// Generate function indexes
	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
	{
		FunctionData *function = ctx.ctx.functions[i];

		function->functionIndex = i;

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
	PrintIndentedLine(ctx, "// Type definitions");
	PrintIndentedLine(ctx, "#pragma pack(push, 4)");

	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
	{
		TypeBase *type = ctx.ctx.types[i];

		if(type->isGeneric)
			continue;

		if(TypeFunction *typeFunction = getType<TypeFunction>(type))
		{
			Print(ctx, "struct __typeProxy_");
			PrintEscapedName(ctx, typeFunction->name);
			Print(ctx, "{};");
			PrintLine(ctx);
		}
		else if(TypeArray *typeArray = getType<TypeArray>(type))
		{
			Print(ctx, "struct ");
			PrintEscapedName(ctx, typeArray->name);
			PrintLine(ctx);

			PrintIndentedLine(ctx, "{");
			ctx.depth++;

			PrintIndent(ctx);
			TranslateTypeName(ctx, typeArray->subType);
			Print(ctx, " ptr[%d];", typeArray->length + ((4 - (typeArray->length * typeArray->subType->size % 4)) & 3)); // Round total byte size to 4
			PrintLine(ctx);

			PrintIndent(ctx);
			PrintEscapedName(ctx, typeArray->name);
			Print(ctx, "& set(unsigned index, ");
			TranslateTypeName(ctx, typeArray->subType);
			Print(ctx, " const& val){ ptr[index] = val; return *this; }");
			PrintLine(ctx);

			ctx.depth--;
			PrintIndentedLine(ctx, "};");
		}
		else if(TypeClass *typeClass = getType<TypeClass>(type))
		{
			Print(ctx, "struct ");
			PrintEscapedName(ctx, typeClass->name);
			PrintLine(ctx);

			PrintIndentedLine(ctx, "{");
			ctx.depth++;

			for(VariableHandle *curr = typeClass->members.head; curr; curr = curr->next)
			{
				PrintIndent(ctx);

				TranslateTypeName(ctx, curr->variable->type);
				Print(ctx, " ");
				TranslateVariableName(ctx, curr->variable);
				Print(ctx, ";");
				PrintLine(ctx);
			}

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
	PrintIndentedLine(ctx, "#pragma pack(pop)");

	PrintLine(ctx);
	PrintIndentedLine(ctx, "// Function prototypes");

	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
	{
		FunctionData *function = ctx.ctx.functions[i];

		if(ctx.ctx.IsGenericFunction(function))
			continue;

		if(function->scope != ctx.ctx.globalScope && !function->scope->ownerNamespace && !function->scope->ownerType)
			Print(ctx, "static ");

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

				if(argument.name == InplaceStr("this"))
				{
					Print(ctx, "__context");
				}
				else if(*argument.name.begin == '$')
				{
					InplaceStr name = InplaceStr(argument.name.begin + 1, argument.name.end);

					Print(ctx, "__%.*s", FMT_ISTR(name));
				}
				else
				{
					Print(ctx, "%.*s", FMT_ISTR(argument.name));
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
	PrintIndentedLine(ctx, "// Global variables");

	for(unsigned int i = 0; i < ctx.ctx.variables.size(); i++)
	{
		VariableData *variable = ctx.ctx.variables[i];

		// Don't need variables allocated by intermediate vm compilation
		if(variable->isVmAlloca)
			continue;

		if(variable->importModule)
		{
			Print(ctx, "extern ");
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
		else if(GlobalScopeFrom(variable->scope))
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
	PrintIndentedLine(ctx, "// Function definitions");

	for(unsigned i = 0; i < expression->definitions.size(); i++)
	{
		ExprFunctionDefinition *definition = getType<ExprFunctionDefinition>(expression->definitions[i]);

		ctx.currentFunction = definition->function;

		Translate(ctx, definition);

		PrintLine(ctx);
	}

	ctx.currentFunction = NULL;

	ctx.skipFunctionDefinitions = true;

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

	PrintIndentedLine(ctx, "");
	PrintIndentedLine(ctx, "// register types");

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
		Print(ctx, "%d, ", type->size);

		if(TypeArray *typeArray = getType<TypeArray>(type))
		{
			Print(ctx, "__nullcTR[%d], ", typeArray->subType->typeIndex);
			Print(ctx, "%d, NULLC_ARRAY);", (unsigned)typeArray->length);
		}
		else if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(type))
		{
			Print(ctx, "__nullcTR[%d], ", typeUnsizedArray->subType->typeIndex);
			Print(ctx, "-1, NULLC_ARRAY);");
		}
		else if(TypeRef *typeRef = getType<TypeRef>(type))
		{
			Print(ctx, "__nullcTR[%d], ", typeRef->subType->typeIndex);
			Print(ctx, "1, NULLC_POINTER);");
		}
		else if(TypeFunction *typeFunction = getType<TypeFunction>(type))
		{
			Print(ctx, "__nullcTR[0], ");
			Print(ctx, "0, NULLC_FUNCTION);");
		}
		else if(TypeStruct *typeStruct = getType<TypeStruct>(type))
		{
			Print(ctx, "__nullcTR[0], ");
			Print(ctx, "%d, NULLC_CLASS);", typeStruct->members.size());
		}
		else
		{
			Print(ctx, "__nullcTR[0], ");
			Print(ctx, "0, NULLC_NONE);");
		}

		PrintLine(ctx);
	}

	PrintIndentedLine(ctx, "");
	PrintIndentedLine(ctx, "// register type members");

	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
	{
		TypeBase *type = ctx.ctx.types[i];

		if(type->isGeneric)
			continue;

		if(TypeStruct *typeStruct = getType<TypeStruct>(type))
		{
			PrintIndent(ctx);
			Print(ctx, "__nullcRegisterMembers(__nullcTR[%d], %d", i, typeStruct->members.size());

			for(VariableHandle *curr = typeStruct->members.head; curr; curr = curr->next)
			{
				Print(ctx, ", __nullcTR[%d]", curr->variable->type->typeIndex);
				Print(ctx, ", %d", curr->variable->offset);
			}

			Print(ctx, "); // type '%.*s' members", FMT_ISTR(type->name));
			PrintLine(ctx);
		}
	}

	PrintIndentedLine(ctx, "");
	PrintIndentedLine(ctx, "// register globals");

	for(unsigned int i = 0; i < ctx.ctx.variables.size(); i++)
	{
		VariableData *variable = ctx.ctx.variables[i];

		// Don't need variables allocated by intermediate vm compilation
		if(variable->isVmAlloca)
			continue;

		if(variable->importModule)
			continue;

		if(variable->scope == ctx.ctx.globalScope || variable->scope->ownerNamespace || GlobalScopeFrom(variable->scope))
		{
			PrintIndent(ctx);
			Print(ctx, "__nullcRegisterGlobal((void*)&");
			TranslateVariableName(ctx, variable);
			Print(ctx, ", __nullcTR[%d]);", variable->type->typeIndex);
			PrintLine(ctx);
		}
	}

	PrintIndentedLine(ctx, "");
	PrintIndentedLine(ctx, "// register functions");

	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
	{
		FunctionData *function = ctx.ctx.functions[i];

		if(ctx.ctx.IsGenericFunction(function))
		{
			PrintIndentedLine(ctx, "__nullcFR[%d] = 0; // generic function '%.*s'", i, FMT_ISTR(function->name));
			continue;
		}

		PrintIndent(ctx);
		Print(ctx, "__nullcFR[%d] = __nullcRegisterFunction(\"", i);
		TranslateFunctionName(ctx, function);
		Print(ctx, "\", (void*)");
		TranslateFunctionName(ctx, function);

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

	PrintIndentedLine(ctx, "");
	PrintIndentedLine(ctx, "// setup");

	for(ExprBase *value = expression->setup.head; value; value = value->next)
	{
		PrintIndent(ctx);

		Translate(ctx, value);

		Print(ctx, ";");

		PrintLine(ctx);
	}

	PrintIndentedLine(ctx, "");
	PrintIndentedLine(ctx, "// expressions");

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		PrintIndent(ctx);

		Translate(ctx, value);

		Print(ctx, ";");

		PrintLine(ctx);
	}

	ctx.depth--;

	PrintIndentedLine(ctx, "}");
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
	else if(ExprFunctionOverloadSet *expr = getType<ExprFunctionOverloadSet>(expression))
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
	else if(ExprModule *expr = getType<ExprModule>(expression))
		TranslateModule(ctx, expr);
	else
		assert(!"unknown type");
}
