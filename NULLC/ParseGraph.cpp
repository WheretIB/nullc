#include "ParseGraph.h"

#include <stdarg.h>

#include "ParseTree.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

void PrintIndent(ParseGraphContext &ctx)
{
	for(unsigned i = 0; i < ctx.depth; i++)
		ctx.output.Print("  ");
}

NULLC_PRINT_FORMAT_CHECK(3, 4) void PrintIndented(ParseGraphContext &ctx, const char *name, const char *format, ...)
{
	PrintIndent(ctx);

	if(*name)
	{
		ctx.output.Print(name);
		ctx.output.Print(": ");
	}

	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);

	ctx.output.Print("\n");
}

NULLC_PRINT_FORMAT_CHECK(3, 4) void PrintEnterBlock(ParseGraphContext &ctx, const char *name, const char *format, ...)
{
	PrintIndent(ctx);

	if(*name)
	{
		ctx.output.Print(name);
		ctx.output.Print(": ");
	}

	va_list args;
	va_start(args, format);

	ctx.output.Print(format, args);

	va_end(args);

	ctx.output.Print("{\n");

	ctx.depth++;
}

void PrintEnterBlock(ParseGraphContext &ctx, const char *name)
{
	PrintIndent(ctx);

	if(*name)
	{
		ctx.output.Print(name);
		ctx.output.Print(": ");
	}

	ctx.output.Print("{\n");

	ctx.depth++;
}

void PrintLeaveBlock(ParseGraphContext &ctx)
{
	ctx.depth--;

	PrintIndent(ctx);

	ctx.output.Print("}\n");
}

void PrintGraph(ParseGraphContext &ctx, SynBase *syntax, const char *name)
{
	if(ctx.depth > 1024)
	{
		PrintIndented(ctx, name, "{...}");
		return;
	}

	if(isType<SynError>(syntax))
	{
		PrintIndented(ctx, name, "SynError()");
	}
	else if(isType<SynNothing>(syntax))
	{
		PrintIndented(ctx, name, "SynNothing()");
	}
	else if(SynIdentifier *node = getType<SynIdentifier>(syntax))
	{
		PrintIndented(ctx, name, "SynIdentifier(%.*s)", FMT_ISTR(node->name));
	}
	else if(isType<SynTypeAuto>(syntax))
	{
		PrintIndented(ctx, name, "SynTypeAuto()");
	}
	else if(isType<SynTypeGeneric>(syntax))
	{
		PrintIndented(ctx, name, "SynTypeGeneric()");
	}
	else if(SynTypeSimple *node = getType<SynTypeSimple>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeSimple(%.*s)", FMT_ISTR(node->name));

		for(SynIdentifier *part = node->path.head; part; part = getType<SynIdentifier>(part->next))
			PrintGraph(ctx, part, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeAlias *node = getType<SynTypeAlias>(syntax))
	{
		PrintIndented(ctx, name, "SynTypeAlias(%.*s)", FMT_ISTR(node->name->name));
	}
	else if(SynTypeArray *node = getType<SynTypeArray>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeArray()");

		PrintGraph(ctx, node->type, "type");

		PrintEnterBlock(ctx, "sizes");

		for(SynBase *size = node->sizes.head; size; size = size->next)
			PrintGraph(ctx, size, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeReference *node = getType<SynTypeReference>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeReference()");

		PrintGraph(ctx, node->type, "type");

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeFunction *node = getType<SynTypeFunction>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeFunction()");

		PrintGraph(ctx, node->returnType, "returnType");

		PrintEnterBlock(ctx, "arguments");

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeGenericInstance *node = getType<SynTypeGenericInstance>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeGenericInstance()");

		PrintGraph(ctx, node->baseType, "baseType");

		PrintEnterBlock(ctx, "types");

		for(SynBase *type = node->types.head; type; type = type->next)
			PrintGraph(ctx, type, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeof *node = getType<SynTypeof>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeof()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynBool *node = getType<SynBool>(syntax))
	{
		PrintIndented(ctx, name, "SynBool(%s)", node->value ? "true" : "false");
	}
	else if(SynNumber *node = getType<SynNumber>(syntax))
	{
		PrintIndented(ctx, name, "SynNumber(%.*s)", FMT_ISTR(node->value));
	}
	else if(isType<SynNullptr>(syntax))
	{
		PrintIndented(ctx, name, "SynNullptr()");
	}
	else if(SynCharacter *node = getType<SynCharacter>(syntax))
	{
		PrintIndented(ctx, name, "SynCharacter(%.*s)", FMT_ISTR(node->value));
	}
	else if(SynString *node = getType<SynString>(syntax))
	{
		PrintIndented(ctx, name, "SynString(%.*s)", FMT_ISTR(node->value));
	}
	else if(SynArray *node = getType<SynArray>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynArray()");

		for(SynBase *value = node->values.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynGenerator *node = getType<SynGenerator>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynGenerator()");

		for(SynBase *value = node->expressions.head; value; value = value->next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynAlign *node = getType<SynAlign>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynAlign()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynTypedef *node = getType<SynTypedef>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypedef(%.*s)", FMT_ISTR(node->alias->name));

		PrintGraph(ctx, node->type, "type");

		PrintLeaveBlock(ctx);
	}
	else if(SynMemberAccess *node = getType<SynMemberAccess>(syntax))
	{
		if(node->member)
			PrintEnterBlock(ctx, name, "SynMemberAccess(%.*s)", FMT_ISTR(node->member->name));
		else
			PrintEnterBlock(ctx, name, "SynMemberAccess(%%missing%%)");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynCallArgument *node = getType<SynCallArgument>(syntax))
	{
		if(node->name)
			PrintEnterBlock(ctx, name, "SynCallArgument(%.*s)", FMT_ISTR(node->name->name));
		else
			PrintEnterBlock(ctx, name, "SynCallArgument()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynArrayIndex *node = getType<SynArrayIndex>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynArrayIndex()");

		PrintGraph(ctx, node->value, "value");

		PrintEnterBlock(ctx, "arguments");

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynFunctionCall *node = getType<SynFunctionCall>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynFunctionCall()");

		PrintGraph(ctx, node->value, "value");

		PrintEnterBlock(ctx, "aliases");

		for(SynBase *arg = node->aliases.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "arguments");

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynPreModify *node = getType<SynPreModify>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynPreModify(%s)", node->isIncrement ? "++" : "--");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynPostModify *node = getType<SynPostModify>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynPostModify(%s)", node->isIncrement ? "++" : "--");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynGetAddress *node = getType<SynGetAddress>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynGetAddress()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynDereference *node = getType<SynDereference>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynDereference()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynSizeof *node = getType<SynSizeof>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynSizeof()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynNew *node = getType<SynNew>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynNew()");

		PrintGraph(ctx, node->type, "type");

		PrintEnterBlock(ctx, "arguments");

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->count, "count");

		PrintEnterBlock(ctx, "constructor");

		for(SynBase *arg = node->constructor.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynConditional *node = getType<SynConditional>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynConditional()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->trueBlock, "trueBlock");
		PrintGraph(ctx, node->falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(SynReturn *node = getType<SynReturn>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynReturn()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynYield *node = getType<SynYield>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynYield()");

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynBreak *node = getType<SynBreak>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynBreak()");

		PrintGraph(ctx, node->number, "number");

		PrintLeaveBlock(ctx);
	}
	else if(SynContinue *node = getType<SynContinue>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynContinue()");

		PrintGraph(ctx, node->number, "number");

		PrintLeaveBlock(ctx);
	}
	else if(SynBlock *node = getType<SynBlock>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynBlock()");

		for(SynBase *expr = node->expressions.head; expr; expr = expr->next)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynIfElse *node = getType<SynIfElse>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynIfElse()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->trueBlock, "trueBlock");
		PrintGraph(ctx, node->falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(SynFor *node = getType<SynFor>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynFor()");

		PrintGraph(ctx, node->initializer, "initializer");
		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->increment, "increment");
		PrintGraph(ctx, node->body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(SynForEachIterator *node = getType<SynForEachIterator>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynForEachIterator(%.*s)", FMT_ISTR(node->name->name));

		PrintGraph(ctx, node->type, "type");
		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynForEach *node = getType<SynForEach>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynForEach()");

		PrintEnterBlock(ctx, "iterators");

		for(SynBase *arg = node->iterators.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(SynWhile *node = getType<SynWhile>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynWhile()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(SynDoWhile *node = getType<SynDoWhile>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynDoWhile()");

		PrintEnterBlock(ctx, "expressions");

		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->condition, "condition");

		PrintLeaveBlock(ctx);
	}
	else if(SynSwitchCase *node = getType<SynSwitchCase>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynSwitchCase()");

		PrintGraph(ctx, node->value, "value");

		PrintEnterBlock(ctx, "expressions");

		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynSwitch *node = getType<SynSwitch>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynSwitch()");

		PrintGraph(ctx, node->condition, "condition");

		PrintEnterBlock(ctx, "cases");

		for(SynBase *arg = node->cases.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynUnaryOp *node = getType<SynUnaryOp>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynUnaryOp(%s)", GetOpName(node->type));

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynBinaryOp *node = getType<SynBinaryOp>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynBinaryOp(%s)", GetOpName(node->type));

		PrintGraph(ctx, node->lhs, "lhs");
		PrintGraph(ctx, node->rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(SynAssignment *node = getType<SynAssignment>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynAssignment()");

		PrintGraph(ctx, node->lhs, "lhs");
		PrintGraph(ctx, node->rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(SynModifyAssignment *node = getType<SynModifyAssignment>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynModifyAssignment(%s)", GetOpName(node->type));

		PrintGraph(ctx, node->lhs, "lhs");
		PrintGraph(ctx, node->rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(SynVariableDefinition *node = getType<SynVariableDefinition>(syntax))
	{
		if(node->initializer)
		{
			PrintEnterBlock(ctx, name, "SynVariableDefinition(%.*s)", FMT_ISTR(node->name->name));

			PrintGraph(ctx, node->initializer, "initializer");

			PrintLeaveBlock(ctx);
		}
		else
		{
			PrintIndented(ctx, name, "SynVariableDefinition(%.*s)", FMT_ISTR(node->name->name));
		}
	}
	else if(SynVariableDefinitions *node = getType<SynVariableDefinitions>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynVariableDefinitions()");

		PrintGraph(ctx, node->align, "align");
		PrintGraph(ctx, node->type, "type");

		PrintEnterBlock(ctx, "definitions");

		for(SynBase *arg = node->definitions.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynAccessor *node = getType<SynAccessor>(syntax))
	{
		if(node->setName)
			PrintEnterBlock(ctx, name, "SynAccessor(%.*s, %.*s)", FMT_ISTR(node->name->name), FMT_ISTR(node->setName->name));
		else
			PrintEnterBlock(ctx, name, "SynAccessor(%.*s)", FMT_ISTR(node->name->name));

		PrintGraph(ctx, node->getBlock, "getBlock");
		PrintGraph(ctx, node->setBlock, "setBlock");

		PrintLeaveBlock(ctx);
	}
	else if(SynFunctionArgument *node = getType<SynFunctionArgument>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynFunctionArgument(%s%.*s)", node->isExplicit ? "explicit, " : "", FMT_ISTR(node->name->name));

		PrintGraph(ctx, node->type, "type");
		PrintGraph(ctx, node->initializer, "initializer");

		PrintLeaveBlock(ctx);
	}
	else if(SynFunctionDefinition *node = getType<SynFunctionDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynFunctionDefinition(%s%s%s%.*s)", node->prototype ? "prototype, " : "", node->coroutine ? "coroutine, " : "", node->accessor ? "accessor, " : "", FMT_ISTR(node->name->name));

		PrintGraph(ctx, node->parentType, "parentType");
		PrintGraph(ctx, node->returnType, "returnType");

		PrintEnterBlock(ctx, "aliases");

		for(SynBase *arg = node->aliases.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "arguments");

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "expressions");

		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynShortFunctionArgument *node = getType<SynShortFunctionArgument>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynShortFunctionArgument(%.*s)", FMT_ISTR(node->name->name));

		PrintGraph(ctx, node->type, "type");

		PrintLeaveBlock(ctx);
	}
	else if(SynShortFunctionDefinition *node = getType<SynShortFunctionDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynShortFunctionDefinition()");

		PrintEnterBlock(ctx, "arguments");

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "expressions");

		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynConstant *node = getType<SynConstant>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynConstant(%.*s)", FMT_ISTR(node->name->name));

		PrintGraph(ctx, node->value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynConstantSet *node = getType<SynConstantSet>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynConstantSet()");

		PrintGraph(ctx, node->type, "type");

		PrintEnterBlock(ctx, "constants");

		for(SynBase *arg = node->constants.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynClassPrototype *node = getType<SynClassPrototype>(syntax))
	{
		PrintIndented(ctx, name, "SynClassPrototype(%.*s)", FMT_ISTR(node->name->name));
	}
	else if(SynClassStaticIf *node = getType<SynClassStaticIf>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynClassStaticIf()");

		PrintGraph(ctx, node->condition, "condition");
		PrintGraph(ctx, node->trueBlock, "trueBlock");
		PrintGraph(ctx, node->falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(SynClassElements *node = getType<SynClassElements>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynClassElements()");

		if(node->typedefs.head)
		{
			PrintEnterBlock(ctx, "typedefs");

			for(SynBase *arg = node->typedefs.head; arg; arg = arg->next)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node->functions.head)
		{
			PrintEnterBlock(ctx, "functions");

			for(SynBase *arg = node->functions.head; arg; arg = arg->next)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node->accessors.head)
		{
			PrintEnterBlock(ctx, "accessors");

			for(SynBase *arg = node->accessors.head; arg; arg = arg->next)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node->members.head)
		{
			PrintEnterBlock(ctx, "members");

			for(SynBase *arg = node->members.head; arg; arg = arg->next)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node->constantSets.head)
		{
			PrintEnterBlock(ctx, "constantSets");

			for(SynBase *arg = node->constantSets.head; arg; arg = arg->next)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node->staticIfs.head)
		{
			PrintEnterBlock(ctx, "staticIfs");

			for(SynBase *arg = node->staticIfs.head; arg; arg = arg->next)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		PrintLeaveBlock(ctx);
	}
	else if(SynClassDefinition *node = getType<SynClassDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynClassDefinition(%.*s%s)", FMT_ISTR(node->name->name), node->extendable ? ", extendable" : "");

		PrintGraph(ctx, node->align, "align");

		PrintEnterBlock(ctx, "aliases");

		for(SynBase *arg = node->aliases.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node->baseClass, "baseClass");
		PrintGraph(ctx, node->elements, "elements");

		PrintLeaveBlock(ctx);
	}
	else if(SynEnumDefinition *node = getType<SynEnumDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynEnumDefinition(%.*s)", FMT_ISTR(node->name->name));

		for(SynBase *arg = node->values.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynNamespaceDefinition *node = getType<SynNamespaceDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynNamespaceDefinition()");

		PrintEnterBlock(ctx, "path");

		for(SynIdentifier *part = node->path.head; part; part = getType<SynIdentifier>(part->next))
			PrintGraph(ctx, part, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "expressions");

		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynModuleImport *node = getType<SynModuleImport>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynModuleImport()");

		for(SynIdentifier *part = node->path.head; part; part = getType<SynIdentifier>(part->next))
			PrintGraph(ctx, part, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynModule *node = getType<SynModule>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynModule()");

		PrintEnterBlock(ctx, "imports");

		for(SynModuleImport *import = node->imports.head; import; import = getType<SynModuleImport>(import->next))
			PrintGraph(ctx, import, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "expressions");

		for(SynBase *expr = node->expressions.head; expr; expr = expr->next)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);

		ctx.output.Flush();
	}
	else if(!syntax)
	{
		PrintIndented(ctx, name, "null");
	}
	else
	{
		assert(!"unknown type");
	}
}
