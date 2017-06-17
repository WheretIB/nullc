#include "ExpressionGraph.h"

#include <stdarg.h>

#include "ExpressionTree.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

void Print(ExpressionGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	vfprintf(ctx.file, format, args);

	va_end(args);
}

void PrintIndent(ExpressionGraphContext &ctx)
{
	for(unsigned i = 0; i < ctx.depth; i++)
		fprintf(ctx.file, "  ");
}

void PrintIndented(ExpressionGraphContext &ctx, InplaceStr name, TypeBase *type, const char *format, ...)
{
	PrintIndent(ctx);

	if(!name.empty())
		fprintf(ctx.file, "%.*s: ", FMT_ISTR(name));

	if(type)
		fprintf(ctx.file, "%.*s ", FMT_ISTR(type->name));

	va_list args;
	va_start(args, format);

	vfprintf(ctx.file, format, args);

	va_end(args);

	fprintf(ctx.file, "\n");
}

void PrintEnterBlock(ExpressionGraphContext &ctx, InplaceStr name, TypeBase *type, const char *format, ...)
{
	PrintIndent(ctx);

	if(!name.empty())
		fprintf(ctx.file, "%.*s: ", FMT_ISTR(name));

	if(type)
		fprintf(ctx.file, "%.*s ", FMT_ISTR(type->name));

	va_list args;
	va_start(args, format);

	vfprintf(ctx.file, format, args);

	va_end(args);

	fprintf(ctx.file, "{\n");

	ctx.depth++;
}

void PrintLeaveBlock(ExpressionGraphContext &ctx)
{
	ctx.depth--;

	PrintIndent(ctx);

	fprintf(ctx.file, "}\n");
}

bool ContainsData(ScopeData *scope, bool imported)
{
	for(unsigned i = 0; i < scope->types.size(); i++)
	{
		TypeBase *data = scope->types[i];

		if(imported == (getType<TypeClass>(data) && getType<TypeClass>(data)->imported))
			return true;
	}

	for(unsigned i = 0; i < scope->functions.size(); i++)
	{
		if(imported == scope->functions[i]->imported)
			return true;
	}

	if(!scope->ownerFunction || scope->ownerFunction->imported == imported)
	{
		for(unsigned i = 0; i < scope->variables.size(); i++)
		{
			if(imported == scope->variables[i]->imported)
				return true;
		}
	}

	for(unsigned i = 0; i < scope->aliases.size(); i++)
	{
		if(imported == scope->aliases[i]->imported)
			return true;
	}

	for(unsigned i = 0; i < scope->scopes.size(); i++)
	{
		if(ContainsData(scope->scopes[i], imported))
			return true;
	}

	return false;
}

void PrintGraph(ExpressionGraphContext &ctx, ScopeData *scope, bool printImported)
{
	PrintIndent(ctx);

	if(FunctionData *owner = scope->ownerFunction)
		Print(ctx, "ScopeData({%.*s: f%04x}: s%04x){\n", FMT_ISTR(owner->name), owner->uniqueId, scope->uniqueId);
	else if(TypeBase *owner = scope->ownerType)
		Print(ctx, "ScopeData({%.*s}: s%04x){\n", FMT_ISTR(owner->name), scope->uniqueId);
	else if(NamespaceData *owner = scope->ownerNamespace)
		Print(ctx, "ScopeData({%.*s: n%04x}: s%04x){\n", FMT_ISTR(owner->name), owner->uniqueId, scope->uniqueId);
	else
		Print(ctx, "ScopeData({}: s%04x){\n", scope->uniqueId);

	ctx.depth++;

	if(!scope->types.empty())
	{
		PrintEnterBlock(ctx, InplaceStr(), 0, "types");

		for(unsigned i = 0; i < scope->types.size(); i++)
		{
			TypeBase *data = scope->types[i];

			bool imported = getType<TypeClass>(data) && getType<TypeClass>(data)->imported;

			if(printImported != imported)
				continue;

			PrintIndented(ctx, InplaceStr(), data, ";");
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope->functions.empty())
	{
		PrintEnterBlock(ctx, InplaceStr(), 0, "functions");

		for(unsigned i = 0; i < scope->functions.size(); i++)
		{
			FunctionData *data = scope->functions[i];

			if(printImported != data->imported)
				continue;

			PrintIndented(ctx, InplaceStr(), data->type, "%.*s: f%04x", FMT_ISTR(data->name), data->uniqueId);
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope->variables.empty())
	{
		PrintEnterBlock(ctx, InplaceStr(), 0, "variables");

		for(unsigned i = 0; i < scope->variables.size(); i++)
		{
			VariableData *data = scope->variables[i];

			if(printImported != data->imported)
				continue;

			PrintIndented(ctx, InplaceStr(), data->type, "%.*s: v%04x", FMT_ISTR(data->name), data->uniqueId);
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope->aliases.empty())
	{
		PrintEnterBlock(ctx, InplaceStr(), 0, "aliases");

		for(unsigned i = 0; i < scope->aliases.size(); i++)
		{
			AliasData *data = scope->aliases[i];

			if(printImported != data->imported)
				continue;

			PrintIndented(ctx, InplaceStr("typedef"), data->type, "%.*s: a%04x", FMT_ISTR(data->name), data->uniqueId);
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope->scopes.empty())
	{
		for(unsigned i = 0; i < scope->scopes.size(); i++)
		{
			ScopeData *data = scope->scopes[i];

			if(data->ownerFunction && data->ownerFunction->imported)
				continue;

			if(ContainsData(data, printImported))
				PrintGraph(ctx, data, printImported);
		}
	}

	ctx.depth--;

	PrintIndent(ctx);
	Print(ctx, "}\n");
}

void PrintGraph(ExpressionGraphContext &ctx, ExprBase *expression, InplaceStr name)
{
	if(ExprVoid *node = getType<ExprVoid>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprVoid()");
	}
	else if(ExprBoolLiteral *node = getType<ExprBoolLiteral>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprBoolLiteral(%s)", node->value ? "true" : "false");
	}
	else if(ExprCharacterLiteral *node = getType<ExprCharacterLiteral>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprCharacterLiteral(\'%c\')", node->value);
	}
	else if(ExprStringLiteral *node = getType<ExprStringLiteral>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprStringLiteral(\'%.*s\')", node->length, node->value);
	}
	else if(ExprIntegerLiteral *node = getType<ExprIntegerLiteral>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprIntegerLiteral(%lld)", node->value);
	}
	else if(ExprRationalLiteral *node = getType<ExprRationalLiteral>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprRationalLiteral(%f)", node->value);
	}
	else if(ExprTypeLiteral *node = getType<ExprTypeLiteral>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprTypeLiteral(%.*s)", FMT_ISTR(node->value->name));
	}
	else if(ExprNullptrLiteral *node = getType<ExprNullptrLiteral>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprNullptrLiteral()");
	}
	else if(ExprPassthrough *node = getType<ExprPassthrough>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprPassthrough()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprArray *node = getType<ExprArray>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprArray()");

		for(ExprBase *value = node->values.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprPreModify *node = getType<ExprPreModify>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprPreModify(%s)", node->isIncrement ? "++" : "--");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprPostModify *node = getType<ExprPostModify>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprPostModify(%s)", node->isIncrement ? "++" : "--");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprTypeCast *node = getType<ExprTypeCast>(expression))
	{
		const char *category = "";

		switch(node->category)
		{
		case EXPR_CAST_NUMERICAL:
			category = "EXPR_CAST_NUMERICAL";
			break;
		case EXPR_CAST_PTR_TO_BOOL:
			category = "EXPR_CAST_PTR_TO_BOOL";
			break;
		case EXPR_CAST_UNSIZED_TO_BOOL:
			category = "EXPR_CAST_UNSIZED_TO_BOOL";
			break;
		case EXPR_CAST_FUNCTION_TO_BOOL:
			category = "EXPR_CAST_FUNCTION_TO_BOOL";
			break;
		case EXPR_CAST_NULL_TO_PTR:
			category = "EXPR_CAST_NULL_TO_PTR";
			break;
		case EXPR_CAST_NULL_TO_AUTO_PTR:
			category = "EXPR_CAST_NULL_TO_AUTO_PTR";
			break;
		case EXPR_CAST_NULL_TO_UNSIZED:
			category = "EXPR_CAST_NULL_TO_UNSIZED";
			break;
		case EXPR_CAST_NULL_TO_AUTO_ARRAY:
			category = "EXPR_CAST_NULL_TO_AUTO_ARRAY";
			break;
		case EXPR_CAST_NULL_TO_FUNCTION:
			category = "EXPR_CAST_NULL_TO_FUNCTION";
			break;
		case EXPR_CAST_ARRAY_TO_UNSIZED:
			category = "EXPR_CAST_ARRAY_TO_UNSIZED";
			break;
		case EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
			category = "EXPR_CAST_ARRAY_PTR_TO_UNSIZED";
			break;
		case EXPR_CAST_ARRAY_PTR_TO_UNSIZED_PTR:
			category = "EXPR_CAST_ARRAY_PTR_TO_UNSIZED_PTR";
			break;
		case EXPR_CAST_PTR_TO_AUTO_PTR:
			category = "EXPR_CAST_PTR_TO_AUTO_PTR";
			break;
		case EXPR_CAST_ANY_TO_PTR:
			category = "EXPR_CAST_ANY_TO_PTR";
			break;
		case EXPR_CAST_AUTO_PTR_TO_PTR:
			category = "EXPR_CAST_AUTO_PTR_TO_PTR";
			break;
		case EXPR_CAST_UNSIZED_TO_AUTO_ARRAY:
			category = "EXPR_CAST_UNSIZED_TO_AUTO_ARRAY";
			break;
		case EXPR_CAST_REINTERPRET:
			category = "EXPR_CAST_REINTERPRET";
			break;
		}

		PrintEnterBlock(ctx, name, node->type, "ExprTypeCast(%s)", category);

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprUnaryOp *node = getType<ExprUnaryOp>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprUnaryOp(%s)", GetOpName(node->op));

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprBinaryOp *node = getType<ExprBinaryOp>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprBinaryOp(%s)", GetOpName(node->op));

		PrintGraph(ctx, node->lhs, "lhs");
		PrintGraph(ctx, node->rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(ExprGetAddress *node = getType<ExprGetAddress>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprGetAddress(%.*s: v%04x)", FMT_ISTR(node->variable->name), node->variable->uniqueId);
	}
	else if(ExprDereference *node = getType<ExprDereference>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprDereference()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprConditional *node = getType<ExprConditional>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprConditional()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->trueBlock, "trueBlock");
		PrintGraph(ctx, node->falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(ExprAssignment *node = getType<ExprAssignment>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprAssignment()");

		PrintGraph(ctx, node->lhs, "lhs");
		PrintGraph(ctx, node->rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(ExprMemberAccess *node = getType<ExprMemberAccess>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprMemberAccess(%.*s: v%04x)", FMT_ISTR(node->member->name), node->member->uniqueId);

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprArrayIndex *node = getType<ExprArrayIndex>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprArrayIndex()");

		PrintGraph(ctx, node->value, "value");
		PrintGraph(ctx, node->index, "index");

		PrintLeaveBlock(ctx);
	}
	else if(ExprReturn *node = getType<ExprReturn>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprReturn()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprYield *node = getType<ExprYield>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprYield()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableDefinition *node = getType<ExprVariableDefinition>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprVariableDefinition(%.*s %.*s: v%04x)", FMT_ISTR(node->variable->type->name), FMT_ISTR(node->variable->name), node->variable->uniqueId);

		PrintGraph(ctx, node->initializer, "initializer");

		PrintLeaveBlock(ctx);
	}
	else if(ExprArraySetup *node = getType<ExprArraySetup>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprArraySetup(%.*s %.*s: v%04x)", FMT_ISTR(node->variable->type->name), FMT_ISTR(node->variable->name), node->variable->uniqueId);

		PrintGraph(ctx, node->initializer, "initializer");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableDefinitions *node = getType<ExprVariableDefinitions>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprVariableDefinitions()");

		for(ExprBase *value = node->definitions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableAccess *node = getType<ExprVariableAccess>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprVariableAccess(%.*s: v%04x)", FMT_ISTR(node->variable->name), node->variable->uniqueId);
	}
	else if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(expression))
	{
		if(ctx.skipFunctionDefinitions)
		{
			PrintIndented(ctx, name, node->type, "ExprFunctionDefinition(%s%.*s: f%04x);", node->function->isPrototype ? "prototype, " : "", FMT_ISTR(node->function->name), node->function->uniqueId);
			return;
		}

		if(node->function->isPrototype)
		{
			PrintEnterBlock(ctx, name, node->type, "ExprFunctionDefinition(%s%.*s: f%04x)", node->function->isPrototype ? "prototype, " : "", FMT_ISTR(node->function->name), node->function->uniqueId);
			
			if(FunctionData *implementation = node->function->implementation)
				PrintIndented(ctx, InplaceStr("implementation"), implementation->type, "%.*s: f%04x", FMT_ISTR(implementation->name), implementation->uniqueId);

			PrintLeaveBlock(ctx);

			return;
		}

		ctx.skipFunctionDefinitions = true;

		PrintEnterBlock(ctx, name, node->type, "ExprFunctionDefinition(%s%.*s: f%04x)", node->function->isPrototype ? "prototype, " : "", FMT_ISTR(node->function->name), node->function->uniqueId);

		PrintGraph(ctx, node->contextArgument, "contextArgument");

		PrintEnterBlock(ctx, InplaceStr("arguments"), 0, "");

		for(ExprBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("expressions"), 0, "");

		for(ExprBase *expr = node->expressions.head; expr; expr = expr->next)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);

		ctx.skipFunctionDefinitions = false;
	}
	else if(ExprGenericFunctionPrototype *node = getType<ExprGenericFunctionPrototype>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprGenericFunctionPrototype(%.*s: f%04x)", FMT_ISTR(node->function->name), node->function->uniqueId);
	}
	else if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprFunctionAccess(%.*s: f%04x)", FMT_ISTR(node->function->name), node->function->uniqueId);

		PrintGraph(ctx, node->context, "context");

		PrintLeaveBlock(ctx);
	}
	else if(ExprFunctionOverloadSet *node = getType<ExprFunctionOverloadSet>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprFunctionOverloadSet()");

		PrintEnterBlock(ctx, InplaceStr("functions"), 0, "");

		for(FunctionHandle *arg = node->functions.head; arg; arg = arg->next)
			PrintIndented(ctx, name, arg->function->type, "%.*s: f%04x", FMT_ISTR(arg->function->name), arg->function->uniqueId);

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->context, "context");

		PrintLeaveBlock(ctx);
	}
	else if(ExprFunctionCall *node = getType<ExprFunctionCall>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprFunctionCall()");

		PrintGraph(ctx, node->function, "function");

		PrintEnterBlock(ctx, InplaceStr("arguments"), 0, "");

		for(ExprBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprAliasDefinition *node = getType<ExprAliasDefinition>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprAliasDefinition(%.*s %.*s: a%04x)", FMT_ISTR(node->alias->type->name), FMT_ISTR(node->alias->name), node->alias->uniqueId);
	}
	else if(ExprGenericClassPrototype *node = getType<ExprGenericClassPrototype>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprGenericClassPrototype(%.*s)", FMT_ISTR(node->genericProtoType->name));

		PrintEnterBlock(ctx, InplaceStr("instances"), 0, "");

		for(ExprBase *value = node->genericProtoType->instances.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprClassDefinition *node = getType<ExprClassDefinition>(expression))
	{
		if(node->classType->baseClass)
			PrintEnterBlock(ctx, name, node->type, "ExprClassDefinition(%.*s: %.*s)", FMT_ISTR(node->classType->name), FMT_ISTR(node->classType->baseClass->name));
		else
			PrintEnterBlock(ctx, name, node->type, "ExprClassDefinition(%.*s%s)", FMT_ISTR(node->classType->name), node->classType->extendable ? " extendable" : "");

		PrintEnterBlock(ctx, InplaceStr("variables"), 0, "");

		for(VariableHandle *value = node->classType->members.head; value; value = value->next)
			PrintIndented(ctx, InplaceStr(), value->variable->type, "%.*s: v%04x", FMT_ISTR(value->variable->name), value->variable->uniqueId);

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("functions"), 0, "");

		for(ExprBase *value = node->functions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("constants"), 0, "");

		for(ConstantData *value = node->classType->constants.head; value; value = value->next)
			PrintGraph(ctx, value->value, value->name);

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprEnumDefinition *node = getType<ExprEnumDefinition>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprEnumDefinition(%.*s)", FMT_ISTR(node->enumType->name));

		PrintEnterBlock(ctx, InplaceStr("constants"), 0, "");

		for(ConstantData *value = node->enumType->constants.head; value; value = value->next)
			PrintGraph(ctx, value->value, value->name);

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->toInt, "toInt");
		PrintGraph(ctx, node->toEnum, "toEnum");

		PrintLeaveBlock(ctx);
	}
	else if(ExprIfElse *node = getType<ExprIfElse>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprIfElse()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->trueBlock, "trueBlock");
		PrintGraph(ctx, node->falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(ExprFor *node = getType<ExprFor>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprFor()");

		PrintGraph(ctx, node->initializer, "initializer");
		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->increment, "increment");
		PrintGraph(ctx, node->body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(ExprWhile *node = getType<ExprWhile>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprWhile()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(ExprDoWhile *node = getType<ExprDoWhile>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprDoWhile()");

		PrintGraph(ctx, node->body, "body");
		PrintGraph(ctx, node->condition, "condition");

		PrintLeaveBlock(ctx);
	}
	else if(ExprSwitch *node = getType<ExprSwitch>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprSwitch()");

		PrintGraph(ctx, node->condition, "condition");

		PrintEnterBlock(ctx, InplaceStr("cases"), 0, "");

		for(ExprBase *value = node->cases.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("blocks"), 0, "");

		for(ExprBase *value = node->blocks.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->defaultBlock, "defaultBlock");

		PrintLeaveBlock(ctx);
	}
	else if(ExprBreak *node = getType<ExprBreak>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprBreak(%d)", node->depth);
	}
	else if(ExprContinue *node = getType<ExprContinue>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprContinue(%d)", node->depth);
	}
	else if(ExprBlock *node = getType<ExprBlock>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprBlock()");

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprSequence *node = getType<ExprSequence>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprSequence()");

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprModule *node = getType<ExprModule>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprModule()");

		if(!ctx.skipImported)
			PrintGraph(ctx, node->moduleScope, true);

		PrintGraph(ctx, node->moduleScope, false);

		PrintEnterBlock(ctx, InplaceStr("definitions"), 0, "");

		for(unsigned i = 0; i < node->definitions.size(); i++)
			PrintGraph(ctx, node->definitions[i], "");

		PrintLeaveBlock(ctx);

		ctx.skipFunctionDefinitions = true;

		PrintEnterBlock(ctx, InplaceStr("expressions"), 0, "");

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(!expression)
	{
		PrintIndented(ctx, name, 0, "null");
	}
	else
	{
		assert(!"unknown type");
	}
}

void PrintGraph(ExpressionGraphContext &ctx, ExprBase *expression, const char *name)
{
	PrintGraph(ctx, expression, InplaceStr(name));
}
