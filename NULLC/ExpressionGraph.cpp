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

void PrintIndented(ExpressionGraphContext &ctx, const char *name, TypeBase *type, const char *format, ...)
{
	PrintIndent(ctx);

	if(*name)
		fprintf(ctx.file, "%s: ", name);

	if(type)
		fprintf(ctx.file, "%.*s ", FMT_ISTR(type->name));

	va_list args;
	va_start(args, format);

	vfprintf(ctx.file, format, args);

	va_end(args);

	fprintf(ctx.file, "\n");
}

void PrintEnterBlock(ExpressionGraphContext &ctx, const char *name, TypeBase *type, const char *format, ...)
{
	PrintIndent(ctx);

	if(*name)
		fprintf(ctx.file, "%s: ", name);

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

void PrintGraph(ExpressionGraphContext &ctx, ExprBase *expression, const char *name)
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
		PrintEnterBlock(ctx, name, node->type, "ExprTypeCast()");

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
	else if(ExprModifyAssignment *node = getType<ExprModifyAssignment>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprModifyAssignment(%s)", GetOpName(node->op));

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

		for(ExprVariableDefinition *value = node->definitions.head; value; value = getType<ExprVariableDefinition>(value->next))
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableAccess *node = getType<ExprVariableAccess>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprVariableAccess(%.*s: v%04x)", FMT_ISTR(node->variable->name), node->variable->uniqueId);
	}
	else if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprFunctionDefinition(%s%.*s: f%04x)", node->prototype ? "prototype, " : "", FMT_ISTR(node->function->name), node->function->uniqueId);

		PrintEnterBlock(ctx, "arguments", 0, "");

		for(ExprVariableDefinition *arg = node->arguments.head; arg; arg = getType<ExprVariableDefinition>(arg->next))
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "expressions", 0, "");

		for(ExprBase *expr = node->expressions.head; expr; expr = expr->next)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprGenericFunctionPrototype *node = getType<ExprGenericFunctionPrototype>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprGenericFunctionPrototype(%.*s: f%04x)", FMT_ISTR(node->function->name), node->function->uniqueId);
	}
	else if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(expression))
	{
		PrintIndented(ctx, name, node->type, "ExprFunctionAccess(%.*s: f%04x)", FMT_ISTR(node->function->name), node->function->uniqueId);
	}
	else if(ExprFunctionCall *node = getType<ExprFunctionCall>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprFunctionCall()");

		PrintGraph(ctx, node->function, "function");

		PrintEnterBlock(ctx, "arguments", 0, "");

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

		PrintEnterBlock(ctx, "instances", 0, "");

		for(ExprBase *value = node->genericProtoType->instances.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprClassDefinition *node = getType<ExprClassDefinition>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprClassDefinition(%.*s)", FMT_ISTR(node->classType->name));

		PrintEnterBlock(ctx, "variables", 0, "");

		for(VariableHandle *value = node->classType->members.head; value; value = value->next)
			PrintIndented(ctx, "", value->variable->type, "%.*s: v%04x", FMT_ISTR(value->variable->name), value->variable->uniqueId);

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "functions", 0, "");

		for(ExprBase *value = node->functions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

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
	else if(ExprBlock *node = getType<ExprBlock>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprBlock()");

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprModule *node = getType<ExprModule>(expression))
	{
		PrintEnterBlock(ctx, name, node->type, "ExprModule()");

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

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
