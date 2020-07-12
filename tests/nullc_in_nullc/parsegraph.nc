import parsetree;
import stringutil;
import reflist;
import std.file;

class ParseGraphContext
{
	File output;

	int depth;
}

void Print(ParseGraphContext ref ctx, char[] format, auto ref[] args)
{
	if(args.size == 0)
	{
		ctx.output.Print(format);
	}
	else
	{
		int arg = 0;
		
		for(int i = 0; i < format.size; i++)
		{
			if(format[i] == 0)
				continue;
				
			if(format[i] == '%')
			{
				i++;
				
				if(format[i] == '.')
					i++;
					
				if(format[i] == '*')
					i++;
				
				if(format[i] == 's')
				{
					if(args[arg].type == char[])
					{
						char[] str = args[arg];
						
						ctx.output.Print(str);
					}
					else if(args[arg].type == InplaceStr)
					{
						InplaceStr str = args[arg];
						
						for(int k = str.begin; k < str.end; k++)
							ctx.output.Write(str.data[k]);
					}

					arg++;
				}
			}
			else
			{
				ctx.output.Write(format[i]);
			}
		}
	}
}

void PrintIndent(ParseGraphContext ref ctx)
{
	for(int i = 0; i < ctx.depth; i++)
		ctx.output.Print("  ");
}

void PrintIndented(ParseGraphContext ref ctx, char[] name, char[] format, auto ref[] args)
{
	PrintIndent(ctx);

	if(name[0])
	{
		ctx.output.Print(name);
		ctx.output.Print(": ");
	}

	Print(ctx, format, args);

	ctx.output.Print("\n");
}

void PrintEnterBlock(ParseGraphContext ref ctx, char[] name, char[] format, auto ref[] args)
{
	PrintIndent(ctx);

	if(name[0])
	{
		ctx.output.Print(name);
		ctx.output.Print(": ");
	}

	Print(ctx, format, args);

	ctx.output.Print("{\n");

	ctx.depth++;
}

void PrintEnterBlock(ParseGraphContext ref ctx, char[] name)
{
	PrintIndent(ctx);

	if(name[0])
	{
		ctx.output.Print(name);
		ctx.output.Print(": ");
	}

	ctx.output.Print("{\n");

	ctx.depth++;
}

void PrintLeaveBlock(ParseGraphContext ref ctx)
{
	ctx.depth--;

	PrintIndent(ctx);

	ctx.output.Print("}\n");
}

void PrintGraph(ParseGraphContext ref ctx, SynBase ref syntax, char[] name)
{
	if(ctx.depth > 1024)
	{
		PrintIndented(ctx, name, "{...}");
		return;
	}

	if(isType with<SynError>(syntax))
	{
		PrintIndented(ctx, name, "SynError()");
	}
	else if(isType with<SynNothing>(syntax))
	{
		PrintIndented(ctx, name, "SynNothing()");
	}
	else if(SynIdentifier ref node = getType with<SynIdentifier>(syntax))
	{
		PrintIndented(ctx, name, "SynIdentifier(%.*s)", FMT_ISTR(node.name));
	}
	else if(isType with<SynTypeAuto>(syntax))
	{
		PrintIndented(ctx, name, "SynTypeAuto()");
	}
	else if(isType with<SynTypeGeneric>(syntax))
	{
		PrintIndented(ctx, name, "SynTypeGeneric()");
	}
	else if(SynTypeSimple ref node = getType with<SynTypeSimple>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeSimple(%.*s)", FMT_ISTR(node.name));

		for(SynIdentifier ref part in node.path)
			PrintGraph(ctx, part, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeAlias ref node = getType with<SynTypeAlias>(syntax))
	{
		PrintIndented(ctx, name, "SynTypeAlias(%.*s)", FMT_ISTR(node.name.name));
	}
	else if(SynTypeArray ref node = getType with<SynTypeArray>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeArray()");

		PrintGraph(ctx, node.type, "type");

		PrintEnterBlock(ctx, "sizes");

		for(SynBase ref size in node.sizes)
			PrintGraph(ctx, size, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeReference ref node = getType with<SynTypeReference>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeReference()");

		PrintGraph(ctx, node.type, "type");

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeFunction ref node = getType with<SynTypeFunction>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeFunction()");

		PrintGraph(ctx, node.returnType, "returnType");

		PrintEnterBlock(ctx, "arguments");

		for(SynBase ref arg in node.arguments)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeGenericInstance ref node = getType with<SynTypeGenericInstance>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeGenericInstance()");

		PrintGraph(ctx, node.baseType, "baseType");

		PrintEnterBlock(ctx, "types");

		for(SynBase ref type in node.types)
			PrintGraph(ctx, type, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynTypeof ref node = getType with<SynTypeof>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypeof()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynBool ref node = getType with<SynBool>(syntax))
	{
		PrintIndented(ctx, name, "SynBool(%s)", node.value ? "true" : "false");
	}
	else if(SynNumber ref node = getType with<SynNumber>(syntax))
	{
		PrintIndented(ctx, name, "SynNumber(%.*s)", FMT_ISTR(node.value));
	}
	else if(isType with<SynNullptr>(syntax))
	{
		PrintIndented(ctx, name, "SynNullptr()");
	}
	else if(SynCharacter ref node = getType with<SynCharacter>(syntax))
	{
		PrintIndented(ctx, name, "SynCharacter(%.*s)", FMT_ISTR(node.value));
	}
	else if(SynString ref node = getType with<SynString>(syntax))
	{
		PrintIndented(ctx, name, "SynString(%.*s)", FMT_ISTR(node.value));
	}
	else if(SynArray ref node = getType with<SynArray>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynArray()");

		for(SynBase ref value in node.values)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynGenerator ref node = getType with<SynGenerator>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynGenerator()");

		for(SynBase ref value in node.expressions)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynAlign ref node = getType with<SynAlign>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynAlign()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynTypedef ref node = getType with<SynTypedef>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynTypedef(%.*s)", FMT_ISTR(node.alias.name));

		PrintGraph(ctx, node.type, "type");

		PrintLeaveBlock(ctx);
	}
	else if(SynMemberAccess ref node = getType with<SynMemberAccess>(syntax))
	{
		if(node.member)
			PrintEnterBlock(ctx, name, "SynMemberAccess(%.*s)", FMT_ISTR(node.member.name));
		else
			PrintEnterBlock(ctx, name, "SynMemberAccess(%%missing%%)");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynCallArgument ref node = getType with<SynCallArgument>(syntax))
	{
		if(node.name)
			PrintEnterBlock(ctx, name, "SynCallArgument(%.*s)", FMT_ISTR(node.name.name));
		else
			PrintEnterBlock(ctx, name, "SynCallArgument()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynArrayIndex ref node = getType with<SynArrayIndex>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynArrayIndex()");

		PrintGraph(ctx, node.value, "value");

		PrintEnterBlock(ctx, "arguments");

		for(SynBase ref arg in node.arguments)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynFunctionCall ref node = getType with<SynFunctionCall>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynFunctionCall()");

		PrintGraph(ctx, node.value, "value");

		PrintEnterBlock(ctx, "aliases");

		for(SynBase ref arg in node.aliases)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "arguments");

		for(SynBase ref arg in node.arguments)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynPreModify ref node = getType with<SynPreModify>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynPreModify(%s)", node.isIncrement ? "++" : "--");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynPostModify ref node = getType with<SynPostModify>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynPostModify(%s)", node.isIncrement ? "++" : "--");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynGetAddress ref node = getType with<SynGetAddress>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynGetAddress()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynDereference ref node = getType with<SynDereference>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynDereference()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynSizeof ref node = getType with<SynSizeof>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynSizeof()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynNew ref node = getType with<SynNew>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynNew()");

		PrintGraph(ctx, node.type, "type");

		PrintEnterBlock(ctx, "arguments");

		for(SynBase ref arg in node.arguments)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node.count, "count");

		PrintEnterBlock(ctx, "constructor");

		for(SynBase ref arg in node.constructor)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynConditional ref node = getType with<SynConditional>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynConditional()");

		PrintGraph(ctx, node.condition, "condition");
		PrintGraph(ctx, node.trueBlock, "trueBlock");
		PrintGraph(ctx, node.falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(SynReturn ref node = getType with<SynReturn>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynReturn()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynYield ref node = getType with<SynYield>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynYield()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynBreak ref node = getType with<SynBreak>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynBreak()");

		PrintGraph(ctx, node.number, "number");

		PrintLeaveBlock(ctx);
	}
	else if(SynContinue ref node = getType with<SynContinue>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynContinue()");

		PrintGraph(ctx, node.number, "number");

		PrintLeaveBlock(ctx);
	}
	else if(SynBlock ref node = getType with<SynBlock>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynBlock()");

		for(SynBase ref expr in node.expressions)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynIfElse ref node = getType with<SynIfElse>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynIfElse()");

		PrintGraph(ctx, node.condition, "condition");
		PrintGraph(ctx, node.trueBlock, "trueBlock");
		PrintGraph(ctx, node.falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(SynFor ref node = getType with<SynFor>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynFor()");

		PrintGraph(ctx, node.initializer, "initializer");
		PrintGraph(ctx, node.condition, "condition");
		PrintGraph(ctx, node.increment, "increment");
		PrintGraph(ctx, node.body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(SynForEachIterator ref node = getType with<SynForEachIterator>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynForEachIterator(%.*s)", FMT_ISTR(node.name.name));

		PrintGraph(ctx, node.type, "type");
		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynForEach ref node = getType with<SynForEach>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynForEach()");

		PrintEnterBlock(ctx, "iterators");

		for(SynBase ref arg in node.iterators)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node.body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(SynWhile ref node = getType with<SynWhile>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynWhile()");

		PrintGraph(ctx, node.condition, "condition");
		PrintGraph(ctx, node.body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(SynDoWhile ref node = getType with<SynDoWhile>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynDoWhile()");

		PrintEnterBlock(ctx, "expressions");

		for(SynBase ref arg in node.expressions)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node.condition, "condition");

		PrintLeaveBlock(ctx);
	}
	else if(SynSwitchCase ref node = getType with<SynSwitchCase>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynSwitchCase()");

		PrintGraph(ctx, node.value, "value");

		PrintEnterBlock(ctx, "expressions");

		for(SynBase ref arg in node.expressions)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynSwitch ref node = getType with<SynSwitch>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynSwitch()");

		PrintGraph(ctx, node.condition, "condition");

		PrintEnterBlock(ctx, "cases");

		for(SynBase ref arg in node.cases)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynUnaryOp ref node = getType with<SynUnaryOp>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynUnaryOp(%s)", GetOpName(node.type));

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynBinaryOp ref node = getType with<SynBinaryOp>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynBinaryOp(%s)", GetOpName(node.type));

		PrintGraph(ctx, node.lhs, "lhs");
		PrintGraph(ctx, node.rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(SynAssignment ref node = getType with<SynAssignment>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynAssignment()");

		PrintGraph(ctx, node.lhs, "lhs");
		PrintGraph(ctx, node.rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(SynModifyAssignment ref node = getType with<SynModifyAssignment>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynModifyAssignment(%s)", GetOpName(node.type));

		PrintGraph(ctx, node.lhs, "lhs");
		PrintGraph(ctx, node.rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(SynVariableDefinition ref node = getType with<SynVariableDefinition>(syntax))
	{
		if(node.initializer)
		{
			PrintEnterBlock(ctx, name, "SynVariableDefinition(%.*s)", FMT_ISTR(node.name.name));

			PrintGraph(ctx, node.initializer, "initializer");

			PrintLeaveBlock(ctx);
		}
		else
		{
			PrintIndented(ctx, name, "SynVariableDefinition(%.*s)", FMT_ISTR(node.name.name));
		}
	}
	else if(SynVariableDefinitions ref node = getType with<SynVariableDefinitions>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynVariableDefinitions()");

		PrintGraph(ctx, node.alignment, "align");
		PrintGraph(ctx, node.type, "type");

		PrintEnterBlock(ctx, "definitions");

		for(SynBase ref arg in node.definitions)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynAccessor ref node = getType with<SynAccessor>(syntax))
	{
		if(node.setName)
			PrintEnterBlock(ctx, name, "SynAccessor(%.*s, %.*s)", FMT_ISTR(node.name.name), FMT_ISTR(node.setName.name));
		else
			PrintEnterBlock(ctx, name, "SynAccessor(%.*s)", FMT_ISTR(node.name.name));

		PrintGraph(ctx, node.getBlock, "getBlock");
		PrintGraph(ctx, node.setBlock, "setBlock");

		PrintLeaveBlock(ctx);
	}
	else if(SynFunctionArgument ref node = getType with<SynFunctionArgument>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynFunctionArgument(%s%.*s)", node.isExplicit ? "explicit, " : "", FMT_ISTR(node.name.name));

		PrintGraph(ctx, node.type, "type");
		PrintGraph(ctx, node.initializer, "initializer");

		PrintLeaveBlock(ctx);
	}
	else if(SynFunctionDefinition ref node = getType with<SynFunctionDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynFunctionDefinition(%s%s%s%.*s)", node.isPrototype ? "prototype, " : "", node.isCoroutine ? "coroutine, " : "", node.isAccessor ? "accessor, " : "", FMT_ISTR(node.name.name));

		PrintGraph(ctx, node.parentType, "parentType");
		PrintGraph(ctx, node.returnType, "returnType");

		PrintEnterBlock(ctx, "aliases");

		for(SynBase ref arg in node.aliases)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "arguments");

		for(SynBase ref arg in node.arguments)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "expressions");

		for(SynBase ref arg in node.expressions)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynShortFunctionArgument ref node = getType with<SynShortFunctionArgument>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynShortFunctionArgument(%.*s)", FMT_ISTR(node.name.name));

		PrintGraph(ctx, node.type, "type");

		PrintLeaveBlock(ctx);
	}
	else if(SynShortFunctionDefinition ref node = getType with<SynShortFunctionDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynShortFunctionDefinition()");

		PrintEnterBlock(ctx, "arguments");

		for(SynBase ref arg in node.arguments)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "expressions");

		for(SynBase ref arg in node.expressions)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynConstant ref node = getType with<SynConstant>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynConstant(%.*s)", FMT_ISTR(node.name.name));

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(SynConstantSet ref node = getType with<SynConstantSet>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynConstantSet()");

		PrintGraph(ctx, node.type, "type");

		PrintEnterBlock(ctx, "constants");

		for(SynBase ref arg in node.constants)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynClassPrototype ref node = getType with<SynClassPrototype>(syntax))
	{
		PrintIndented(ctx, name, "SynClassPrototype(%.*s)", FMT_ISTR(node.name.name));
	}
	else if(SynClassStaticIf ref node = getType with<SynClassStaticIf>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynClassStaticIf()");

		PrintGraph(ctx, node.condition, "condition");
		PrintGraph(ctx, node.trueBlock, "trueBlock");
		PrintGraph(ctx, node.falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(SynClassElements ref node = getType with<SynClassElements>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynClassElements()");

		if(node.typedefs.head)
		{
			PrintEnterBlock(ctx, "typedefs");

			for(SynBase ref arg in node.typedefs)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node.functions.head)
		{
			PrintEnterBlock(ctx, "functions");

			for(SynBase ref arg in node.functions)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node.accessors.head)
		{
			PrintEnterBlock(ctx, "accessors");

			for(SynBase ref arg in node.accessors)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node.members.head)
		{
			PrintEnterBlock(ctx, "members");

			for(SynBase ref arg in node.members)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node.constantSets.head)
		{
			PrintEnterBlock(ctx, "constantSets");

			for(SynBase ref arg in node.constantSets)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		if(node.staticIfs.head)
		{
			PrintEnterBlock(ctx, "staticIfs");

			for(SynBase ref arg in node.staticIfs)
				PrintGraph(ctx, arg, "");

			PrintLeaveBlock(ctx);
		}

		PrintLeaveBlock(ctx);
	}
	else if(SynClassDefinition ref node = getType with<SynClassDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynClassDefinition(%.*s%s)", FMT_ISTR(node.name.name), node.isExtendable ? ", extendable" : "");

		PrintGraph(ctx, node.alignment, "align");

		PrintEnterBlock(ctx, "aliases");

		for(SynBase ref arg in node.aliases)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node.baseClass, "baseClass");
		PrintGraph(ctx, node.elements, "elements");

		PrintLeaveBlock(ctx);
	}
	else if(SynEnumDefinition ref node = getType with<SynEnumDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynEnumDefinition(%.*s)", FMT_ISTR(node.name.name));

		for(SynBase ref arg in node.values)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynNamespaceDefinition ref node = getType with<SynNamespaceDefinition>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynNamespaceDefinition()");

		PrintEnterBlock(ctx, "path");

		for(SynIdentifier ref part in node.path)
			PrintGraph(ctx, part, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "expressions");

		for(SynBase ref arg in node.expressions)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(SynModuleImport ref node = getType with<SynModuleImport>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynModuleImport()");

		for(SynIdentifier ref part in node.path)
			PrintGraph(ctx, part, "");

		PrintLeaveBlock(ctx);
	}
	else if(SynModule ref node = getType with<SynModule>(syntax))
	{
		PrintEnterBlock(ctx, name, "SynModule()");

		PrintEnterBlock(ctx, "imports");

		for(SynModuleImport ref moduleImport in node.imports)
			PrintGraph(ctx, moduleImport, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, "expressions");

		for(SynBase ref expr in node.expressions)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);

		//ctx.output.Flush();
	}
	else if(!syntax)
	{
		PrintIndented(ctx, name, "null");
	}
	else
	{
		assert(false, "unknown type");
	}
}
