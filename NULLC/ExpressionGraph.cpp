#include "ExpressionGraph.h"

#include <stdarg.h>

#include "ExpressionTree.h"

void (*nullcDumpGraphExprBase)(ExprBase*) = DumpGraph;

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

NULLC_PRINT_FORMAT_CHECK(2, 3) void Print(ExpressionGraphContext &ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);
}

void PrintIndent(ExpressionGraphContext &ctx)
{
	for(unsigned i = 0; i < ctx.depth; i++)
		ctx.output.Print("  ");
}

NULLC_PRINT_FORMAT_CHECK(4, 5) void PrintIndented(ExpressionGraphContext &ctx, InplaceStr name, TypeBase *type, const char *format, ...)
{
	PrintIndent(ctx);

	if(!name.empty())
	{
		ctx.output.Print(name.begin, unsigned(name.end - name.begin));
		ctx.output.Print(": ");
	}

	if(type && !type->name.empty())
	{
		ctx.output.Print(type->name.begin, unsigned(type->name.end - type->name.begin));
		ctx.output.Print(" ");
	}

	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);

	ctx.output.Print("\n");
}

NULLC_PRINT_FORMAT_CHECK(4, 5) void PrintIndented(ExpressionGraphContext &ctx, InplaceStr name, ExprBase *node, const char *format, ...)
{
	PrintIndent(ctx);

	if(!name.empty())
	{
		ctx.output.Print(name.begin, unsigned(name.end - name.begin));
		ctx.output.Print(": ");
	}

	if(node->type && !node->type->name.empty())
	{
		ctx.output.Print(node->type->name.begin, unsigned(node->type->name.end - node->type->name.begin));
		ctx.output.Print(" ");
	}

	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);

	if(node->source && !node->source->isInternal)
	{
		Print(ctx, " // %s [%d:%d]-[%d:%d]", GetParseTreeNodeName(node->source), node->source->begin->line + 1, node->source->begin->column, node->source->end->line + 1, node->source->end->column + node->source->end->length);

		if(ModuleData *importModule = ctx.ctx.GetSourceOwner(node->source->begin))
			Print(ctx, " from '%.*s'", FMT_ISTR(importModule->name));
	}

	ctx.output.Print("\n");
}

void PrintEnterBlock(ExpressionGraphContext &ctx, const char *name)
{
	PrintIndent(ctx);

	ctx.output.Print(name);
	ctx.output.Print("{\n");

	ctx.depth++;
}

NULLC_PRINT_FORMAT_CHECK(4, 5) void PrintEnterBlock(ExpressionGraphContext &ctx, InplaceStr name, ExprBase *node, const char *format, ...)
{
	PrintIndent(ctx);

	if(!name.empty())
	{
		ctx.output.Print(name.begin, unsigned(name.end - name.begin));
		ctx.output.Print(": ");
	}

	if(node->type && !node->type->name.empty())
	{
		ctx.output.Print(node->type->name.begin, unsigned(node->type->name.end - node->type->name.begin));
		ctx.output.Print(" ");
	}

	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);

	ctx.output.Print('{');

	if(node->source && !node->source->isInternal)
	{
		Print(ctx, " // %s [%d:%d]-[%d:%d]", GetParseTreeNodeName(node->source), node->source->begin->line + 1, node->source->begin->column, node->source->end->line + 1, node->source->end->column + node->source->end->length);

		if(ModuleData *importModule = ctx.ctx.GetSourceOwner(node->source->begin))
			Print(ctx, " from '%.*s'", FMT_ISTR(importModule->name));
	}

	ctx.output.Print('\n');

	ctx.depth++;
}

void PrintEnterBlock(ExpressionGraphContext &ctx, InplaceStr name, TypeBase *type)
{
	PrintIndent(ctx);

	if(!name.empty())
	{
		ctx.output.Print(name.begin, unsigned(name.end - name.begin));
		ctx.output.Print(": ");
	}

	if(type && !type->name.empty())
	{
		ctx.output.Print(type->name.begin, unsigned(type->name.end - type->name.begin));
		ctx.output.Print(" ");
	}

	ctx.output.Print("{\n");

	ctx.depth++;
}

void PrintLeaveBlock(ExpressionGraphContext &ctx)
{
	ctx.depth--;

	PrintIndent(ctx);

	ctx.output.Print("}\n");
}

bool ContainsData(ScopeData *scope, bool checkImported)
{
	for(unsigned i = 0; i < scope->types.size(); i++)
	{
		TypeBase *data = scope->types[i];

		bool imported = getType<TypeClass>(data) && getType<TypeClass>(data)->importModule != NULL;

		if(checkImported == imported)
			return true;
	}

	for(unsigned i = 0; i < scope->functions.size(); i++)
	{
		bool imported = scope->functions[i]->importModule != NULL;

		if(checkImported == imported)
			return true;
	}

	bool ownerFunctionImported = scope->ownerFunction && scope->ownerFunction->importModule != NULL;

	if(!scope->ownerFunction || checkImported == ownerFunctionImported)
	{
		for(unsigned i = 0; i < scope->variables.size(); i++)
		{
			bool imported = scope->variables[i]->importModule != NULL;

			if(checkImported == imported)
				return true;
		}
	}

	for(unsigned i = 0; i < scope->aliases.size(); i++)
	{
		bool imported = scope->aliases[i]->importModule != NULL;

		if(checkImported == imported)
			return true;
	}

	for(unsigned i = 0; i < scope->scopes.size(); i++)
	{
		if(ContainsData(scope->scopes[i], checkImported))
			return true;
	}

	return false;
}

void PrintGraph(ExpressionGraphContext &ctx, ScopeData *scope, bool printImported)
{
	PrintIndent(ctx);

	const char *type = "";

	switch(scope->type)
	{
	case SCOPE_EXPLICIT:
		type = "SCOPE_EXPLICIT";
		break;
	case SCOPE_NAMESPACE:
		type = "SCOPE_NAMESPACE";
		break;
	case SCOPE_FUNCTION:
		type = "SCOPE_FUNCTION";
		break;
	case SCOPE_TYPE:
		type = "SCOPE_TYPE";
		break;
	case SCOPE_LOOP:
		type = "SCOPE_LOOP";
		break;
	case SCOPE_TEMPORARY:
		type = "SCOPE_TEMPORARY";
		break;
	default:
		assert(!"unknown type");
	}

	if(FunctionData *owner = scope->ownerFunction)
		Print(ctx, "ScopeData(%s, {%.*s: f%04x}: s%04x) @ 0x%x-0x%x {\n", type, FMT_ISTR(owner->name->name), owner->uniqueId, scope->uniqueId, unsigned(scope->startOffset), unsigned(scope->dataSize));
	else if(TypeBase *owner = scope->ownerType)
		Print(ctx, "ScopeData(%s, {%.*s}: s%04x){\n", type, FMT_ISTR(owner->name), scope->uniqueId);
	else if(NamespaceData *owner = scope->ownerNamespace)
		Print(ctx, "ScopeData(%s, {%.*s: n%04x}: s%04x){\n", type, FMT_ISTR(owner->name.name), owner->uniqueId, scope->uniqueId);
	else
		Print(ctx, "ScopeData(%s, {}: s%04x) @ 0x%x-0x%x {\n", type, scope->uniqueId, unsigned(scope->startOffset), unsigned(scope->dataSize));

	ctx.depth++;

	if(!scope->types.empty())
	{
		PrintEnterBlock(ctx, "types");

		for(unsigned i = 0; i < scope->types.size(); i++)
		{
			TypeBase *data = scope->types[i];

			bool imported = getType<TypeClass>(data) && getType<TypeClass>(data)->importModule != NULL;

			if(printImported != imported)
				continue;

			PrintIndent(ctx);

			if(data->isGeneric)
			{
				if(data->importModule)
					Print(ctx, "%.*s: generic from %.*s\n", FMT_ISTR(data->name), FMT_ISTR(data->importModule->name));
				else
					Print(ctx, "%.*s: generic\n", FMT_ISTR(data->name));
			}
			else
			{
				if(data->importModule)
					Print(ctx, "%.*s: size(%d) align(%d) padding(%d)%s from %.*s\n", FMT_ISTR(data->name), unsigned(data->size), data->alignment, data->padding, data->hasPointers ? " gc_check" : "", FMT_ISTR(data->importModule->name));
				else
					Print(ctx, "%.*s: size(%d) align(%d) padding(%d)%s\n", FMT_ISTR(data->name), unsigned(data->size), data->alignment, data->padding, data->hasPointers ? " gc_check" : "");
			}
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope->functions.empty())
	{
		PrintEnterBlock(ctx, "functions");

		for(unsigned i = 0; i < scope->functions.size(); i++)
		{
			FunctionData *data = scope->functions[i];

			bool imported = data->importModule != NULL;

			if(printImported != imported)
				continue;

			PrintIndented(ctx, InplaceStr(), data->type, "%.*s: f%04x", FMT_ISTR(data->name->name), data->uniqueId);
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope->variables.empty())
	{
		PrintEnterBlock(ctx, "variables");

		for(unsigned i = 0; i < scope->variables.size(); i++)
		{
			VariableData *data = scope->variables[i];

			bool imported = data->importModule != NULL;

			if(printImported != imported)
				continue;

			PrintIndented(ctx, InplaceStr(), data->type, "%.*s: v%04x @ 0x%x%s%s%s%s", FMT_ISTR(data->name->name), data->uniqueId, data->offset, data->isReadonly ? " readonly" : "", data->isReference ? " reference" : "", data->usedAsExternal ? " captured" : "", data->type->hasPointers ? " gc_check" : "");
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope->aliases.empty())
	{
		PrintEnterBlock(ctx, "aliases");

		for(unsigned i = 0; i < scope->aliases.size(); i++)
		{
			AliasData *data = scope->aliases[i];

			bool imported = data->importModule != NULL;

			if(printImported != imported)
				continue;

			PrintIndented(ctx, InplaceStr("typedef"), data->type, "%.*s: a%04x", FMT_ISTR(data->name->name), data->uniqueId);
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope->scopes.empty())
	{
		for(unsigned i = 0; i < scope->scopes.size(); i++)
		{
			ScopeData *data = scope->scopes[i];

			if(data->ownerFunction && data->ownerFunction->importModule != NULL)
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
	if(ctx.depth > 1024)
	{
		PrintIndented(ctx, name, expression, "{...}");
		return;
	}

	if(ExprError *node = getType<ExprError>(expression))
	{
		if(!node->values.empty())
		{
			PrintEnterBlock(ctx, name, node, "ExprError()");

			for(unsigned i = 0; i < node->values.size(); i++)
				PrintGraph(ctx, node->values[i], "");

			PrintLeaveBlock(ctx);
		}
		else
		{
			PrintIndented(ctx, name, node, "ExprError()");
		}
	}
	else if(ExprErrorTypeMemberAccess *node = getType<ExprErrorTypeMemberAccess>(expression))
	{
		PrintIndented(ctx, name, node, "ExprErrorTypeMemberAccess(%.*s)", FMT_ISTR(node->value->name));
	}
	else if(ExprVoid *node = getType<ExprVoid>(expression))
	{
		PrintIndented(ctx, name, node, "ExprVoid()");
	}
	else if(ExprBoolLiteral *node = getType<ExprBoolLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprBoolLiteral(%s)", node->value ? "true" : "false");
	}
	else if(ExprCharacterLiteral *node = getType<ExprCharacterLiteral>(expression))
	{
		if((int)node->value < (int)' ')
			PrintIndented(ctx, name, node, "ExprCharacterLiteral(0x%02x)", node->value);
		else
			PrintIndented(ctx, name, node, "ExprCharacterLiteral(\'%c\')", node->value);
	}
	else if(ExprStringLiteral *node = getType<ExprStringLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprStringLiteral(\'%.*s\')", node->length, node->value);
	}
	else if(ExprIntegerLiteral *node = getType<ExprIntegerLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprIntegerLiteral(%lld)", node->value);
	}
	else if(ExprRationalLiteral *node = getType<ExprRationalLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprRationalLiteral(%f)", node->value);
	}
	else if(ExprTypeLiteral *node = getType<ExprTypeLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprTypeLiteral(%.*s)", FMT_ISTR(node->value->name));
	}
	else if(ExprNullptrLiteral *node = getType<ExprNullptrLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprNullptrLiteral()");
	}
	else if(ExprFunctionIndexLiteral *node = getType<ExprFunctionIndexLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprFunctionIndexLiteral(%.*s: f%04x)", FMT_ISTR(node->function->name->name), node->function->uniqueId);
	}
	else if(ExprPassthrough *node = getType<ExprPassthrough>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprPassthrough()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprArray *node = getType<ExprArray>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprArray()");

		for(ExprBase *value = node->values.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprPreModify *node = getType<ExprPreModify>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprPreModify(%s)", node->isIncrement ? "++" : "--");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprPostModify *node = getType<ExprPostModify>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprPostModify(%s)", node->isIncrement ? "++" : "--");

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
		case EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
			category = "EXPR_CAST_ARRAY_PTR_TO_UNSIZED";
			break;
		case EXPR_CAST_PTR_TO_AUTO_PTR:
			category = "EXPR_CAST_PTR_TO_AUTO_PTR";
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
		default:
			assert(!"unknown category");
		}

		PrintEnterBlock(ctx, name, node, "ExprTypeCast(%s)", category);

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprUnaryOp *node = getType<ExprUnaryOp>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprUnaryOp(%s)", GetOpName(node->op));

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprBinaryOp *node = getType<ExprBinaryOp>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprBinaryOp(%s)", GetOpName(node->op));

		PrintGraph(ctx, node->lhs, "lhs");
		PrintGraph(ctx, node->rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(ExprGetAddress *node = getType<ExprGetAddress>(expression))
	{
		PrintIndented(ctx, name, node, "ExprGetAddress(%.*s: v%04x)", FMT_ISTR(node->variable->variable->name->name), node->variable->variable->uniqueId);
	}
	else if(ExprDereference *node = getType<ExprDereference>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprDereference()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprUnboxing *node = getType<ExprUnboxing>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprUnboxing()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprConditional *node = getType<ExprConditional>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprConditional()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->trueBlock, "trueBlock");
		PrintGraph(ctx, node->falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(ExprAssignment *node = getType<ExprAssignment>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprAssignment()");

		PrintGraph(ctx, node->lhs, "lhs");
		PrintGraph(ctx, node->rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(ExprMemberAccess *node = getType<ExprMemberAccess>(expression))
	{
		if(node->member)
			PrintEnterBlock(ctx, name, node, "ExprMemberAccess(%.*s: v%04x)", FMT_ISTR(node->member->variable->name->name), node->member->variable->uniqueId);
		else
			PrintEnterBlock(ctx, name, node, "ExprMemberAccess(%%missing%%)");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprArrayIndex *node = getType<ExprArrayIndex>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprArrayIndex()");

		PrintGraph(ctx, node->value, "value");
		PrintGraph(ctx, node->index, "index");

		PrintLeaveBlock(ctx);
	}
	else if(ExprReturn *node = getType<ExprReturn>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprReturn()");

		PrintGraph(ctx, node->value, "value");

		PrintGraph(ctx, node->coroutineStateUpdate, "coroutineStateUpdate");

		PrintGraph(ctx, node->closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprYield *node = getType<ExprYield>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprYield()");

		PrintGraph(ctx, node->value, "value");

		PrintGraph(ctx, node->coroutineStateUpdate, "coroutineStateUpdate");

		PrintGraph(ctx, node->closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableDefinition *node = getType<ExprVariableDefinition>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprVariableDefinition(%.*s %.*s: v%04x)", FMT_ISTR(node->variable->variable->type->name), FMT_ISTR(node->variable->variable->name->name), node->variable->variable->uniqueId);

		PrintGraph(ctx, node->initializer, "initializer");

		PrintLeaveBlock(ctx);
	}
	else if(ExprZeroInitialize *node = getType<ExprZeroInitialize>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprZeroInitialize()");

		PrintGraph(ctx, node->address, "address");

		PrintLeaveBlock(ctx);
	}
	else if(ExprArraySetup *node = getType<ExprArraySetup>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprArraySetup()");

		PrintGraph(ctx, node->lhs, "lhs");
		PrintGraph(ctx, node->initializer, "initializer");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableDefinitions *node = getType<ExprVariableDefinitions>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprVariableDefinitions()");

		for(ExprBase *value = node->definitions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableAccess *node = getType<ExprVariableAccess>(expression))
	{
		PrintIndented(ctx, name, node, "ExprVariableAccess(%.*s: v%04x)", FMT_ISTR(node->variable->name->name), node->variable->uniqueId);
	}
	else if(ExprFunctionContextAccess *node = getType<ExprFunctionContextAccess>(expression))
	{
		PrintIndented(ctx, name, node, "ExprFunctionContextAccess(%.*s: f%04x)", FMT_ISTR(node->function->name->name), node->function->uniqueId);
	}
	else if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(expression))
	{
		if(ctx.skipFunctionDefinitions)
		{
			PrintIndented(ctx, name, node, "ExprFunctionDefinition(%s%.*s: f%04x);", node->function->isPrototype ? "prototype, " : "", FMT_ISTR(node->function->name->name), node->function->uniqueId);
			return;
		}

		if(node->function->isPrototype)
		{
			PrintEnterBlock(ctx, name, node, "ExprFunctionDefinition(%s%.*s: f%04x)", node->function->isPrototype ? "prototype, " : "", FMT_ISTR(node->function->name->name), node->function->uniqueId);
			
			if(FunctionData *implementation = node->function->implementation)
				PrintIndented(ctx, InplaceStr("implementation"), implementation->type, "%.*s: f%04x", FMT_ISTR(implementation->name->name), implementation->uniqueId);

			PrintLeaveBlock(ctx);

			return;
		}

		ctx.skipFunctionDefinitions = true;

		PrintEnterBlock(ctx, name, node, "ExprFunctionDefinition(%s%.*s: f%04x)", node->function->isPrototype ? "prototype, " : "", FMT_ISTR(node->function->name->name), node->function->uniqueId);

		PrintGraph(ctx, node->contextArgument, "contextArgument");

		PrintEnterBlock(ctx, InplaceStr("arguments"), 0);

		for(ExprBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->coroutineStateRead, "coroutineStateRead");

		PrintEnterBlock(ctx, InplaceStr("expressions"), 0);

		for(ExprBase *expr = node->expressions.head; expr; expr = expr->next)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);

		ctx.skipFunctionDefinitions = false;
	}
	else if(ExprGenericFunctionPrototype *node = getType<ExprGenericFunctionPrototype>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprGenericFunctionPrototype(%.*s: f%04x)", FMT_ISTR(node->function->name->name), node->function->uniqueId);

		PrintEnterBlock(ctx, InplaceStr("contextVariables"), 0);

		for(ExprBase *expr = node->contextVariables.head; expr; expr = expr->next)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprFunctionAccess(%.*s: f%04x)", FMT_ISTR(node->function->name->name), node->function->uniqueId);

		PrintGraph(ctx, node->context, "context");

		PrintLeaveBlock(ctx);
	}
	else if(ExprFunctionOverloadSet *node = getType<ExprFunctionOverloadSet>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprFunctionOverloadSet()");

		PrintEnterBlock(ctx, InplaceStr("functions"), 0);

		for(FunctionHandle *arg = node->functions.head; arg; arg = arg->next)
			PrintIndented(ctx, name, arg->function->type, "%.*s: f%04x", FMT_ISTR(arg->function->name->name), arg->function->uniqueId);

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->context, "context");

		PrintLeaveBlock(ctx);
	}
	else if(ExprShortFunctionOverloadSet *node = getType<ExprShortFunctionOverloadSet>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprShortFunctionOverloadSet()");

		PrintEnterBlock(ctx, InplaceStr("functions"), 0);

		for(ShortFunctionHandle *arg = node->functions.head; arg; arg = arg->next)
		{
			PrintIndented(ctx, name, arg->function->type, "%.*s: f%04x", FMT_ISTR(arg->function->name->name), arg->function->uniqueId);

			PrintGraph(ctx, arg->context, "context");
		}

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprFunctionCall *node = getType<ExprFunctionCall>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprFunctionCall()");

		PrintGraph(ctx, node->function, "function");

		PrintEnterBlock(ctx, InplaceStr("arguments"), 0);

		for(ExprBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprAliasDefinition *node = getType<ExprAliasDefinition>(expression))
	{
		PrintIndented(ctx, name, node, "ExprAliasDefinition(%.*s %.*s: a%04x)", FMT_ISTR(node->alias->type->name), FMT_ISTR(node->alias->name->name), node->alias->uniqueId);
	}
	else if(ExprClassPrototype *node = getType<ExprClassPrototype>(expression))
	{
		PrintIndented(ctx, name, node, "ExprClassPrototype(%.*s)", FMT_ISTR(node->classType->name));
	}
	else if(ExprGenericClassPrototype *node = getType<ExprGenericClassPrototype>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprGenericClassPrototype(%.*s)", FMT_ISTR(node->genericProtoType->name));

		PrintEnterBlock(ctx, InplaceStr("instances"), 0);

		for(ExprBase *value = node->genericProtoType->instances.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprClassDefinition *node = getType<ExprClassDefinition>(expression))
	{
		if(node->classType->baseClass)
			PrintEnterBlock(ctx, name, node, "ExprClassDefinition(%.*s: %.*s)", FMT_ISTR(node->classType->name), FMT_ISTR(node->classType->baseClass->name));
		else
			PrintEnterBlock(ctx, name, node, "ExprClassDefinition(%.*s%s)", FMT_ISTR(node->classType->name), node->classType->extendable ? " extendable" : "");

		PrintEnterBlock(ctx, InplaceStr("variables"), 0);

		for(MemberHandle *value = node->classType->members.head; value; value = value->next)
			PrintIndented(ctx, InplaceStr(), value->variable->type, "%.*s: v%04x @ 0x%x", FMT_ISTR(value->variable->name->name), value->variable->uniqueId, value->variable->offset);

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("functions"), 0);

		for(ExprBase *value = node->functions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("constants"), 0);

		for(ConstantData *value = node->classType->constants.head; value; value = value->next)
			PrintGraph(ctx, value->value, value->name->name);

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprEnumDefinition *node = getType<ExprEnumDefinition>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprEnumDefinition(%.*s)", FMT_ISTR(node->enumType->name));

		PrintEnterBlock(ctx, InplaceStr("constants"), 0);

		for(ConstantData *value = node->enumType->constants.head; value; value = value->next)
			PrintGraph(ctx, value->value, value->name->name);

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->toInt, "toInt");
		PrintGraph(ctx, node->toEnum, "toEnum");

		PrintLeaveBlock(ctx);
	}
	else if(ExprIfElse *node = getType<ExprIfElse>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprIfElse()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->trueBlock, "trueBlock");
		PrintGraph(ctx, node->falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(ExprFor *node = getType<ExprFor>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprFor()");

		PrintGraph(ctx, node->initializer, "initializer");
		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->increment, "increment");
		PrintGraph(ctx, node->body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(ExprWhile *node = getType<ExprWhile>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprWhile()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(ExprDoWhile *node = getType<ExprDoWhile>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprDoWhile()");

		PrintGraph(ctx, node->body, "body");
		PrintGraph(ctx, node->condition, "condition");

		PrintLeaveBlock(ctx);
	}
	else if(ExprSwitch *node = getType<ExprSwitch>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprSwitch()");

		PrintGraph(ctx, node->condition, "condition");

		PrintEnterBlock(ctx, InplaceStr("cases"), 0);

		for(ExprBase *value = node->cases.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("blocks"), 0);

		for(ExprBase *value = node->blocks.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->defaultBlock, "defaultBlock");

		PrintLeaveBlock(ctx);
	}
	else if(ExprBreak *node = getType<ExprBreak>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprBreak(%d)", node->depth);

		PrintGraph(ctx, node->closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprContinue *node = getType<ExprContinue>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprContinue(%d)", node->depth);

		PrintGraph(ctx, node->closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprBlock *node = getType<ExprBlock>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprBlock()");

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintGraph(ctx, node->closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprSequence *node = getType<ExprSequence>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprSequence()");

		for(unsigned i = 0; i < node->expressions.size(); i++)
			PrintGraph(ctx, node->expressions[i], "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprModule *node = getType<ExprModule>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprModule()");

		if(!ctx.skipImported)
			PrintGraph(ctx, node->moduleScope, true);

		PrintGraph(ctx, node->moduleScope, false);

		PrintEnterBlock(ctx, InplaceStr("definitions"), 0);

		for(unsigned i = 0; i < node->definitions.size(); i++)
			PrintGraph(ctx, node->definitions[i], "");

		PrintLeaveBlock(ctx);

		ctx.skipFunctionDefinitions = true;

		PrintEnterBlock(ctx, InplaceStr("setup"), 0);

		for(ExprBase *value = node->setup.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("expressions"), 0);

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(!expression)
	{
		PrintIndent(ctx);

		ctx.output.Print(name.begin, unsigned(name.end - name.begin));
		ctx.output.Print(": null\n");
	}
	else
	{
		assert(!"unknown type");
	}
}

void PrintGraph(ExpressionGraphContext &ctx, ExprBase *expression, const char *name)
{
	PrintGraph(ctx, expression, InplaceStr(name));

	ctx.output.Flush();
}

void DumpGraph(ExprBase *tree)
{
	ExpressionContext exprCtx(0, 0);
	OutputContext outputCtx;

	char outputBuf[4096];
	outputCtx.outputBuf = outputBuf;
	outputCtx.outputBufSize = 4096;

	char tempBuf[4096];
	outputCtx.tempBuf = tempBuf;
	outputCtx.tempBufSize = 4096;

	outputCtx.stream = OutputContext::FileOpen("expr_graph.txt");
	outputCtx.writeStream = OutputContext::FileWrite;

	ExpressionGraphContext exprGraphCtx(exprCtx, outputCtx);

	PrintGraph(exprGraphCtx, tree, "");

	OutputContext::FileClose(outputCtx.stream);
	outputCtx.stream = NULL;
}

void DumpGraph(ScopeData *scope)
{
	ExpressionContext exprCtx(0, 0);
	OutputContext outputCtx;

	char outputBuf[4096];
	outputCtx.outputBuf = outputBuf;
	outputCtx.outputBufSize = 4096;

	char tempBuf[4096];
	outputCtx.tempBuf = tempBuf;
	outputCtx.tempBufSize = 4096;

	outputCtx.stream = OutputContext::FileOpen("expr_graph.txt");
	outputCtx.writeStream = OutputContext::FileWrite;

	ExpressionGraphContext exprGraphCtx(exprCtx, outputCtx);

	PrintGraph(exprGraphCtx, scope, true);

	PrintGraph(exprGraphCtx, scope, false);

	OutputContext::FileClose(outputCtx.stream);
	outputCtx.stream = NULL;
}
