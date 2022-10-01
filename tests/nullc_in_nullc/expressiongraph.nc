import std.file;
import stringutil;
import typetree;
import expressiontree;
import analyzer;

class ExpressionGraphContext
{
	void ExpressionGraphContext(ExpressionContext ref ctx/*, OutputContext &output*/)
	{
		this.ctx = ctx;
		//this.output = output;

		depth = 0;

		skipImported = false;
		skipFunctionDefinitions = false;
	}

	ExpressionContext ref ctx;

	//OutputContext ref output;
	File output;

	int depth;

	bool skipImported;
	bool skipFunctionDefinitions;
}

void PrintGraph(ExpressionGraphContext ref ctx, ExprBase ref expression, char[] name);

void Print(ExpressionGraphContext ref ctx, char[] format, auto ref[] args)
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

				bool zeroWidth = false;

				if(format[i] == '0')
				{
					zeroWidth = true;
					i++;
				}

				int width = -1;

				if(format[i] >= '0' && format[i] <= '9')
				{
					width = format[i] - '0';
					i++;
				}

				if(format[i] >= '0' && format[i] <= '9')
				{
					width = width * 10 + (format[i] - '0');
					i++;
				}
				
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
					else
					{
						assert(false, "unknown type");
					}

					arg++;
				}

				if(format[i] == 'l')
					i++;

				if(format[i] == 'l')
					i++;

				if(format[i] == 'd' || format[i] == 'u' || format[i] == 'x') // TODO: hexadecimal
				{
					if(args[arg].type == char || args[arg].type == int)
					{
						int value;

						if(args[arg].type == char)
							value = char(args[arg]);
						else if(args[arg].type == int)
							value = args[arg];

						char[] intStr;

						if(format[i] == 'u')
							intStr = as_unsigned(value).str();
						else
							intStr = value.str();

						if(intStr.size > 0)
						{
							if(width != -1 && width > intStr.size - 1)
							{
								char[] tmp = new char[width + 1];

								if(zeroWidth)
								{
									for(el in tmp)
										el = '0';

									tmp[tmp.size - 1] = 0;
								}

								memory::copy(tmp, tmp.size - intStr.size, intStr, 0, intStr.size - 1);

								intStr = tmp;
							}

							for(int k = 0; k < intStr.size - 1; k++)
								ctx.output.Write(intStr[k]);
						}
					}
					else if(args[arg].type == long)
					{
						long value = args[arg];

						char[] longStr = value.str();

						if(longStr.size > 0)
						{
							for(int k = 0; k < longStr.size - 1; k++)
								ctx.output.Write(longStr[k]);
						}
					}
					else
					{
						assert(false, "unknown type");
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

void PrintIndent(ExpressionGraphContext ref ctx)
{
	for(int i = 0; i < ctx.depth; i++)
		ctx.output.Print("  ");
}

void PrintIndented(ExpressionGraphContext ref ctx, InplaceStr name, TypeBase ref type, char[] format, auto ref[] args)
{
	PrintIndent(ctx);

	if(!name.empty())
	{
		ctx.output.Write(name.data, name.begin, name.end - name.begin);
		ctx.output.Print(": ");
	}

	if(type)
	{
		ctx.output.Write(type.name.data, type.name.begin, type.name.end - type.name.begin);
		ctx.output.Print(" ");
	}

	Print(ctx, format, args);

	ctx.output.Print("\n");
}

void PrintIndented(ExpressionGraphContext ref ctx, InplaceStr name, ExprBase ref node, char[] format, auto ref[] args)
{
	PrintIndent(ctx);

	if(!name.empty())
	{
		ctx.output.Write(name.data, name.begin, name.end - name.begin);
		ctx.output.Print(": ");
	}

	if(node.type)
	{
		ctx.output.Write(node.type.name.data, node.type.name.begin, node.type.name.end - node.type.name.begin);
		ctx.output.Print(" ");
	}

	Print(ctx, format, args);

	if(node.source && !node.source.isInternal)
	{
		//Print(ctx, " // %s [%d:%d]-[%d:%d]", GetParseTreeNodeName(node.source), node.source.begin.line + 1, node.source.begin.column, node.source.end.line + 1, node.source.end.column + node.source.end.length);
		Print(ctx, " // %s", GetParseTreeNodeName(node.source));

		if(ModuleData ref importModule = ctx.ctx.GetSourceOwner(node.source.begin))
			Print(ctx, " from '%.*s'", FMT_ISTR(importModule.name));
	}

	ctx.output.Print("\n");
}

void PrintEnterBlock(ExpressionGraphContext ref ctx, char[] name)
{
	PrintIndent(ctx);

	ctx.output.Print(name);
	ctx.output.Print("{\n");

	ctx.depth++;
}

void PrintEnterBlock(ExpressionGraphContext ref ctx, InplaceStr name, ExprBase ref node, char[] format, auto ref[] args)
{
	PrintIndent(ctx);

	if(!name.empty())
	{
		ctx.output.Write(name.data, name.begin, int(name.end - name.begin));
		ctx.output.Print(": ");
	}

	if(node.type)
	{
		ctx.output.Write(node.type.name.data, node.type.name.begin, int(node.type.name.end - node.type.name.begin));
		ctx.output.Print(" ");
	}

	Print(ctx, format, args);

	ctx.output.Print("{");

	if(node.source && !node.source.isInternal)
	{
		//Print(ctx, " // %s [%d:%d]-[%d:%d]", GetParseTreeNodeName(node.source), node.source.begin.line + 1, node.source.begin.column, node.source.end.line + 1, node.source.end.column + node.source.end.length);
		Print(ctx, " // %s", GetParseTreeNodeName(node.source));

		if(ModuleData ref importModule = ctx.ctx.GetSourceOwner(node.source.begin))
			Print(ctx, " from '%.*s'", FMT_ISTR(importModule.name));
	}

	ctx.output.Print("\n");

	ctx.depth++;
}

void PrintEnterBlock(ExpressionGraphContext ref ctx, InplaceStr name, TypeBase ref type)
{
	PrintIndent(ctx);

	if(!name.empty())
	{
		ctx.output.Write(name.data, name.begin, int(name.end - name.begin));
		ctx.output.Print(": ");
	}

	if(type)
	{
		ctx.output.Write(type.name.data, type.name.begin, int(type.name.end - type.name.begin));
		ctx.output.Print(" ");
	}

	ctx.output.Print("{\n");

	ctx.depth++;
}

void PrintLeaveBlock(ExpressionGraphContext ref ctx)
{
	ctx.depth--;

	PrintIndent(ctx);

	ctx.output.Print("}\n");
}

bool ContainsData(ScopeData ref scope, bool checkImported)
{
	for(int i = 0; i < scope.types.size(); i++)
	{
		TypeBase ref data = scope.types[i];

		bool imported = getType with<TypeClass>(data) && getType with<TypeClass>(data).importModule != nullptr;

		if(checkImported == imported)
			return true;
	}

	for(int i = 0; i < scope.functions.size(); i++)
	{
		bool imported = scope.functions[i].importModule != nullptr;

		if(checkImported == imported)
			return true;
	}

	bool ownerFunctionImported = scope.ownerFunction && scope.ownerFunction.importModule != nullptr;

	if(!scope.ownerFunction || checkImported == ownerFunctionImported)
	{
		for(int i = 0; i < scope.variables.size(); i++)
		{
			bool imported = scope.variables[i].importModule != nullptr;

			if(checkImported == imported)
				return true;
		}
	}

	for(int i = 0; i < scope.aliases.size(); i++)
	{
		bool imported = scope.aliases[i].importModule != nullptr;

		if(checkImported == imported)
			return true;
	}

	for(int i = 0; i < scope.scopes.size(); i++)
	{
		if(ContainsData(scope.scopes[i], checkImported))
			return true;
	}

	return false;
}

void PrintGraph(ExpressionGraphContext ref ctx, ScopeData ref scope, bool printImported)
{
	PrintIndent(ctx);

	char[] type = "";

	switch(scope.type)
	{
	case ScopeType.SCOPE_EXPLICIT:
		type = "SCOPE_EXPLICIT";
		break;
	case ScopeType.SCOPE_NAMESPACE:
		type = "SCOPE_NAMESPACE";
		break;
	case ScopeType.SCOPE_FUNCTION:
		type = "SCOPE_FUNCTION";
		break;
	case ScopeType.SCOPE_TYPE:
		type = "SCOPE_TYPE";
		break;
	case ScopeType.SCOPE_LOOP:
		type = "SCOPE_LOOP";
		break;
	case ScopeType.SCOPE_TEMPORARY:
		type = "SCOPE_TEMPORARY";
		break;
	default:
		assert(false, "unknown type");
	}

	if(FunctionData ref owner = scope.ownerFunction)
		Print(ctx, "ScopeData(%s, {%.*s: f%04x}: s%04x) @ 0x%x-0x%x {\n", type, FMT_ISTR(owner.name.name), owner.uniqueId, scope.uniqueId, int(scope.startOffset), int(scope.dataSize));
	else if(TypeBase ref owner = scope.ownerType)
		Print(ctx, "ScopeData(%s, {%.*s}: s%04x){\n", type, FMT_ISTR(owner.name), scope.uniqueId);
	else if(NamespaceData ref owner = scope.ownerNamespace)
		Print(ctx, "ScopeData(%s, {%.*s: n%04x}: s%04x){\n", type, FMT_ISTR(owner.name.name), owner.uniqueId, scope.uniqueId);
	else
		Print(ctx, "ScopeData(%s, {}: s%04x) @ 0x%x-0x%x {\n", type, scope.uniqueId, int(scope.startOffset), int(scope.dataSize));

	ctx.depth++;

	if(!scope.types.empty())
	{
		PrintEnterBlock(ctx, "types");

		for(int i = 0; i < scope.types.size(); i++)
		{
			TypeBase ref data = scope.types[i];

			bool imported = getType with<TypeClass>(data) && getType with<TypeClass>(data).importModule != nullptr;

			if(printImported != imported)
				continue;

			PrintIndent(ctx);

			if(data.isGeneric)
			{
				if(data.importModule)
					Print(ctx, "%.*s: generic from %.*s\n", FMT_ISTR(data.name), FMT_ISTR(data.importModule.name));
				else
					Print(ctx, "%.*s: generic\n", FMT_ISTR(data.name));
			}
			else
			{
				if(data.importModule)
					Print(ctx, "%.*s: size(%d) align(%d) padding(%d)%s from %.*s\n", FMT_ISTR(data.name), int(data.size), data.alignment, data.padding, data.hasPointers ? " gc_check" : "", FMT_ISTR(data.importModule.name));
				else
					Print(ctx, "%.*s: size(%d) align(%d) padding(%d)%s\n", FMT_ISTR(data.name), int(data.size), data.alignment, data.padding, data.hasPointers ? " gc_check" : "");
			}
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope.functions.empty())
	{
		PrintEnterBlock(ctx, "functions");

		for(int i = 0; i < scope.functions.size(); i++)
		{
			FunctionData ref data = scope.functions[i];

			bool imported = data.importModule != nullptr;

			if(printImported != imported)
				continue;

			PrintIndented(ctx, InplaceStr(), data.type, "%.*s: f%04x", FMT_ISTR(data.name.name), data.uniqueId);
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope.variables.empty())
	{
		PrintEnterBlock(ctx, "variables");

		for(int i = 0; i < scope.variables.size(); i++)
		{
			VariableData ref data = scope.variables[i];

			bool imported = data.importModule != nullptr;

			if(printImported != imported)
				continue;

			PrintIndented(ctx, InplaceStr(), data.type, "%.*s: v%04x @ 0x%x%s%s%s%s", FMT_ISTR(data.name.name), data.uniqueId, data.offset, data.isReadonly ? " readonly" : "", data.isReference ? " reference" : "", data.usedAsExternal ? " captured" : "", data.type.hasPointers ? " gc_check" : "");
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope.aliases.empty())
	{
		PrintEnterBlock(ctx, "aliases");

		for(int i = 0; i < scope.aliases.size(); i++)
		{
			AliasData ref data = scope.aliases[i];

			bool imported = data.importModule != nullptr;

			if(printImported != imported)
				continue;

			PrintIndented(ctx, InplaceStr("typedef"), data.type, "%.*s: a%04x", FMT_ISTR(data.name.name), data.uniqueId);
		}

		PrintLeaveBlock(ctx);
	}

	if(!scope.scopes.empty())
	{
		for(int i = 0; i < scope.scopes.size(); i++)
		{
			ScopeData ref data = scope.scopes[i];

			if(data.ownerFunction && data.ownerFunction.importModule != nullptr)
				continue;

			if(ContainsData(data, printImported))
				PrintGraph(ctx, data, printImported);
		}
	}

	ctx.depth--;

	PrintIndent(ctx);
	Print(ctx, "}\n");
}

void PrintGraph(ExpressionGraphContext ref ctx, ExprBase ref expression, InplaceStr name)
{
	if(ctx.depth > 1024)
	{
		PrintIndented(ctx, name, expression, "{...}");
		return;
	}

	if(ExprError ref node = getType with<ExprError>(expression))
	{
		if(!node.values.empty())
		{
			PrintEnterBlock(ctx, name, node, "ExprError()");

			for(int i = 0; i < node.values.size(); i++)
				PrintGraph(ctx, node.values[i], "");

			PrintLeaveBlock(ctx);
		}
		else
		{
			PrintIndented(ctx, name, node, "ExprError()");
		}
	}
	else if(ExprErrorTypeMemberAccess ref node = getType with<ExprErrorTypeMemberAccess>(expression))
	{
		PrintIndented(ctx, name, node, "ExprErrorTypeMemberAccess(%.*s)", FMT_ISTR(node.value.name));
	}
	else if(ExprVoid ref node = getType with<ExprVoid>(expression))
	{
		PrintIndented(ctx, name, node, "ExprVoid()");
	}
	else if(ExprBoolLiteral ref node = getType with<ExprBoolLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprBoolLiteral(%s)", node.value ? "true" : "false");
	}
	else if(ExprCharacterLiteral ref node = getType with<ExprCharacterLiteral>(expression))
	{
		if(node.value < ' ')
			PrintIndented(ctx, name, node, "ExprCharacterLiteral(0x%02x)", node.value);
		else
			PrintIndented(ctx, name, node, "ExprCharacterLiteral(\'%c\')", node.value);
	}
	else if(ExprStringLiteral ref node = getType with<ExprStringLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprStringLiteral(\'%.*s\')", node.length, node.value);
	}
	else if(ExprIntegerLiteral ref node = getType with<ExprIntegerLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprIntegerLiteral(%lld)", node.value);
	}
	else if(ExprRationalLiteral ref node = getType with<ExprRationalLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprRationalLiteral(%f)", node.value);
	}
	else if(ExprTypeLiteral ref node = getType with<ExprTypeLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprTypeLiteral(%.*s)", FMT_ISTR(node.value.name));
	}
	else if(ExprNullptrLiteral ref node = getType with<ExprNullptrLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprNullptrLiteral()");
	}
	else if(ExprFunctionIndexLiteral ref node = getType with<ExprFunctionIndexLiteral>(expression))
	{
		PrintIndented(ctx, name, node, "ExprFunctionIndexLiteral(%.*s: f%04x)", FMT_ISTR(node.function.name.name), node.function.uniqueId);
	}
	else if(ExprPassthrough ref node = getType with<ExprPassthrough>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprPassthrough()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprArray ref node = getType with<ExprArray>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprArray()");

		for(ExprBase ref value = node.values.head; value; value = value.next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprPreModify ref node = getType with<ExprPreModify>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprPreModify(%s)", node.isIncrement ? "++" : "--");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprPostModify ref node = getType with<ExprPostModify>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprPostModify(%s)", node.isIncrement ? "++" : "--");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprTypeCast ref node = getType with<ExprTypeCast>(expression))
	{
		char[] category = "";

		switch(node.category)
		{
		case ExprTypeCastCategory.EXPR_CAST_NUMERICAL:
			category = "EXPR_CAST_NUMERICAL";
			break;
		case ExprTypeCastCategory.EXPR_CAST_PTR_TO_BOOL:
			category = "EXPR_CAST_PTR_TO_BOOL";
			break;
		case ExprTypeCastCategory.EXPR_CAST_UNSIZED_TO_BOOL:
			category = "EXPR_CAST_UNSIZED_TO_BOOL";
			break;
		case ExprTypeCastCategory.EXPR_CAST_FUNCTION_TO_BOOL:
			category = "EXPR_CAST_FUNCTION_TO_BOOL";
			break;
		case ExprTypeCastCategory.EXPR_CAST_NULL_TO_PTR:
			category = "EXPR_CAST_NULL_TO_PTR";
			break;
		case ExprTypeCastCategory.EXPR_CAST_NULL_TO_AUTO_PTR:
			category = "EXPR_CAST_NULL_TO_AUTO_PTR";
			break;
		case ExprTypeCastCategory.EXPR_CAST_NULL_TO_UNSIZED:
			category = "EXPR_CAST_NULL_TO_UNSIZED";
			break;
		case ExprTypeCastCategory.EXPR_CAST_NULL_TO_AUTO_ARRAY:
			category = "EXPR_CAST_NULL_TO_AUTO_ARRAY";
			break;
		case ExprTypeCastCategory.EXPR_CAST_NULL_TO_FUNCTION:
			category = "EXPR_CAST_NULL_TO_FUNCTION";
			break;
		case ExprTypeCastCategory.EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
			category = "EXPR_CAST_ARRAY_PTR_TO_UNSIZED";
			break;
		case ExprTypeCastCategory.EXPR_CAST_PTR_TO_AUTO_PTR:
			category = "EXPR_CAST_PTR_TO_AUTO_PTR";
			break;
		case ExprTypeCastCategory.EXPR_CAST_AUTO_PTR_TO_PTR:
			category = "EXPR_CAST_AUTO_PTR_TO_PTR";
			break;
		case ExprTypeCastCategory.EXPR_CAST_UNSIZED_TO_AUTO_ARRAY:
			category = "EXPR_CAST_UNSIZED_TO_AUTO_ARRAY";
			break;
		case ExprTypeCastCategory.EXPR_CAST_REINTERPRET:
			category = "EXPR_CAST_REINTERPRET";
			break;
		default:
			assert(false, "unknown category");
		}

		PrintEnterBlock(ctx, name, node, "ExprTypeCast(%s)", category);

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprUnaryOp ref node = getType with<ExprUnaryOp>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprUnaryOp(%s)", GetOpName(node.op));

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprBinaryOp ref node = getType with<ExprBinaryOp>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprBinaryOp(%s)", GetOpName(node.op));

		PrintGraph(ctx, node.lhs, "lhs");
		PrintGraph(ctx, node.rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(ExprGetAddress ref node = getType with<ExprGetAddress>(expression))
	{
		PrintIndented(ctx, name, node, "ExprGetAddress(%.*s: v%04x)", FMT_ISTR(node.variable.variable.name.name), node.variable.variable.uniqueId);
	}
	else if(ExprDereference ref node = getType with<ExprDereference>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprDereference()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprUnboxing ref node = getType with<ExprUnboxing>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprUnboxing()");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprConditional ref node = getType with<ExprConditional>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprConditional()");

		PrintGraph(ctx, node.condition, "condition");
		PrintGraph(ctx, node.trueBlock, "trueBlock");
		PrintGraph(ctx, node.falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(ExprAssignment ref node = getType with<ExprAssignment>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprAssignment()");

		PrintGraph(ctx, node.lhs, "lhs");
		PrintGraph(ctx, node.rhs, "rhs");

		PrintLeaveBlock(ctx);
	}
	else if(ExprMemberAccess ref node = getType with<ExprMemberAccess>(expression))
	{
		if(node.member)
			PrintEnterBlock(ctx, name, node, "ExprMemberAccess(%.*s: v%04x)", FMT_ISTR(node.member.variable.name.name), node.member.variable.uniqueId);
		else
			PrintEnterBlock(ctx, name, node, "ExprMemberAccess(%%missing%%)");

		PrintGraph(ctx, node.value, "value");

		PrintLeaveBlock(ctx);
	}
	else if(ExprArrayIndex ref node = getType with<ExprArrayIndex>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprArrayIndex()");

		PrintGraph(ctx, node.value, "value");
		PrintGraph(ctx, node.index, "index");

		PrintLeaveBlock(ctx);
	}
	else if(ExprReturn ref node = getType with<ExprReturn>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprReturn()");

		PrintGraph(ctx, node.value, "value");

		PrintGraph(ctx, node.coroutineStateUpdate, "coroutineStateUpdate");

		PrintGraph(ctx, node.closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprYield ref node = getType with<ExprYield>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprYield()");

		PrintGraph(ctx, node.value, "value");

		PrintGraph(ctx, node.coroutineStateUpdate, "coroutineStateUpdate");

		PrintGraph(ctx, node.closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableDefinition ref node = getType with<ExprVariableDefinition>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprVariableDefinition(%.*s %.*s: v%04x)", FMT_ISTR(node.variable.variable.type.name), FMT_ISTR(node.variable.variable.name.name), node.variable.variable.uniqueId);

		PrintGraph(ctx, node.initializer, "initializer");

		PrintLeaveBlock(ctx);
	}
	else if(ExprArraySetup ref node = getType with<ExprArraySetup>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprArraySetup()");

		PrintGraph(ctx, node.lhs, "lhs");
		PrintGraph(ctx, node.initializer, "initializer");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableDefinitions ref node = getType with<ExprVariableDefinitions>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprVariableDefinitions()");

		for(ExprBase ref value = node.definitions.head; value; value = value.next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(expression))
	{
		PrintIndented(ctx, name, node, "ExprVariableAccess(%.*s: v%04x)", FMT_ISTR(node.variable.name.name), node.variable.uniqueId);
	}
	else if(ExprFunctionContextAccess ref node = getType with<ExprFunctionContextAccess>(expression))
	{
		PrintIndented(ctx, name, node, "ExprFunctionContextAccess(%.*s: f%04x)", FMT_ISTR(node.function.name.name), node.function.uniqueId);
	}
	else if(ExprFunctionDefinition ref node = getType with<ExprFunctionDefinition>(expression))
	{
		if(ctx.skipFunctionDefinitions)
		{
			PrintIndented(ctx, name, node, "ExprFunctionDefinition(%s%.*s: f%04x);", node.function.isPrototype ? "prototype, " : "", FMT_ISTR(node.function.name.name), node.function.uniqueId);
			return;
		}

		if(node.function.isPrototype)
		{
			PrintEnterBlock(ctx, name, node, "ExprFunctionDefinition(%s%.*s: f%04x)", node.function.isPrototype ? "prototype, " : "", FMT_ISTR(node.function.name.name), node.function.uniqueId);
			
			if(FunctionData ref implementation = node.function.implementation)
				PrintIndented(ctx, InplaceStr("implementation"), implementation.type, "%.*s: f%04x", FMT_ISTR(implementation.name.name), implementation.uniqueId);

			PrintLeaveBlock(ctx);

			return;
		}

		ctx.skipFunctionDefinitions = true;

		PrintEnterBlock(ctx, name, node, "ExprFunctionDefinition(%s%.*s: f%04x)", node.function.isPrototype ? "prototype, " : "", FMT_ISTR(node.function.name.name), node.function.uniqueId);

		PrintGraph(ctx, node.contextArgument, "contextArgument");

		PrintEnterBlock(ctx, InplaceStr("arguments"), nullptr);

		for(ExprBase ref arg = node.arguments.head; arg; arg = arg.next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node.coroutineStateRead, "coroutineStateRead");

		PrintEnterBlock(ctx, InplaceStr("expressions"), nullptr);

		for(ExprBase ref expr = node.expressions.head; expr; expr = expr.next)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);

		ctx.skipFunctionDefinitions = false;
	}
	else if(ExprGenericFunctionPrototype ref node = getType with<ExprGenericFunctionPrototype>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprGenericFunctionPrototype(%.*s: f%04x)", FMT_ISTR(node.function.name.name), node.function.uniqueId);

		PrintEnterBlock(ctx, InplaceStr("contextVariables"), nullptr);

		for(ExprBase ref expr = node.contextVariables.head; expr; expr = expr.next)
			PrintGraph(ctx, expr, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprFunctionAccess ref node = getType with<ExprFunctionAccess>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprFunctionAccess(%.*s: f%04x)", FMT_ISTR(node.function.name.name), node.function.uniqueId);

		PrintGraph(ctx, node.context, "context");

		PrintLeaveBlock(ctx);
	}
	else if(ExprFunctionOverloadSet ref node = getType with<ExprFunctionOverloadSet>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprFunctionOverloadSet()");

		PrintEnterBlock(ctx, InplaceStr("functions"), nullptr);

		for(FunctionHandle ref arg = node.functions.head; arg; arg = arg.next)
			PrintIndented(ctx, name, arg.function.type, "%.*s: f%04x", FMT_ISTR(arg.function.name.name), arg.function.uniqueId);

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node.context, "context");

		PrintLeaveBlock(ctx);
	}
	else if(ExprShortFunctionOverloadSet ref node = getType with<ExprShortFunctionOverloadSet>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprShortFunctionOverloadSet()");

		PrintEnterBlock(ctx, InplaceStr("functions"), nullptr);

		for(ShortFunctionHandle ref arg = node.functions.head; arg; arg = arg.next)
		{
			PrintIndented(ctx, name, arg.function.type, "%.*s: f%04x", FMT_ISTR(arg.function.name.name), arg.function.uniqueId);

			PrintGraph(ctx, arg.context, "context");
		}

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprFunctionCall ref node = getType with<ExprFunctionCall>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprFunctionCall()");

		PrintGraph(ctx, node.function, "function");

		PrintEnterBlock(ctx, InplaceStr("arguments"), nullptr);

		for(ExprBase ref arg = node.arguments.head; arg; arg = arg.next)
			PrintGraph(ctx, arg, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprAliasDefinition ref node = getType with<ExprAliasDefinition>(expression))
	{
		PrintIndented(ctx, name, node, "ExprAliasDefinition(%.*s %.*s: a%04x)", FMT_ISTR(node.alias.type.name), FMT_ISTR(node.alias.name.name), node.alias.uniqueId);
	}
	else if(ExprClassPrototype ref node = getType with<ExprClassPrototype>(expression))
	{
		PrintIndented(ctx, name, node, "ExprClassPrototype(%.*s)", FMT_ISTR(node.classType.name));
	}
	else if(ExprGenericClassPrototype ref node = getType with<ExprGenericClassPrototype>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprGenericClassPrototype(%.*s)", FMT_ISTR(node.genericProtoType.name));

		PrintEnterBlock(ctx, InplaceStr("instances"), nullptr);

		for(ExprBase ref value in node.genericProtoType.instances)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprClassDefinition ref node = getType with<ExprClassDefinition>(expression))
	{
		if(node.classType.baseClass)
			PrintEnterBlock(ctx, name, node, "ExprClassDefinition(%.*s: %.*s)", FMT_ISTR(node.classType.name), FMT_ISTR(node.classType.baseClass.name));
		else
			PrintEnterBlock(ctx, name, node, "ExprClassDefinition(%.*s%s)", FMT_ISTR(node.classType.name), node.classType.isExtendable ? " extendable" : "");

		PrintEnterBlock(ctx, InplaceStr("variables"), nullptr);

		for(MemberHandle ref value = node.classType.members.head; value; value = value.next)
			PrintIndented(ctx, InplaceStr(), value.variable.type, "%.*s: v%04x @ 0x%x", FMT_ISTR(value.variable.name.name), value.variable.uniqueId, value.variable.offset);

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("functions"), nullptr);

		for(ExprBase ref value = node.functions.head; value; value = value.next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("constants"), nullptr);

		for(ConstantData ref value = node.classType.constants.head; value; value = value.next)
			PrintGraph(ctx, value.value, value.name.name);

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(ExprEnumDefinition ref node = getType with<ExprEnumDefinition>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprEnumDefinition(%.*s)", FMT_ISTR(node.enumType.name));

		PrintEnterBlock(ctx, InplaceStr("constants"), nullptr);

		for(ConstantData ref value = node.enumType.constants.head; value; value = value.next)
			PrintGraph(ctx, value.value, value.name.name);

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node.toInt, "toInt");
		PrintGraph(ctx, node.toEnum, "toEnum");

		PrintLeaveBlock(ctx);
	}
	else if(ExprIfElse ref node = getType with<ExprIfElse>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprIfElse()");

		PrintGraph(ctx, node.condition, "condition");
		PrintGraph(ctx, node.trueBlock, "trueBlock");
		PrintGraph(ctx, node.falseBlock, "falseBlock");

		PrintLeaveBlock(ctx);
	}
	else if(ExprFor ref node = getType with<ExprFor>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprFor()");

		PrintGraph(ctx, node.initializer, "initializer");
		PrintGraph(ctx, node.condition, "condition");
		PrintGraph(ctx, node.increment, "increment");
		PrintGraph(ctx, node.body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(ExprWhile ref node = getType with<ExprWhile>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprWhile()");

		PrintGraph(ctx, node.condition, "condition");
		PrintGraph(ctx, node.body, "body");

		PrintLeaveBlock(ctx);
	}
	else if(ExprDoWhile ref node = getType with<ExprDoWhile>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprDoWhile()");

		PrintGraph(ctx, node.body, "body");
		PrintGraph(ctx, node.condition, "condition");

		PrintLeaveBlock(ctx);
	}
	else if(ExprSwitch ref node = getType with<ExprSwitch>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprSwitch()");

		PrintGraph(ctx, node.condition, "condition");

		PrintEnterBlock(ctx, InplaceStr("cases"), nullptr);

		for(ExprBase ref value = node.cases.head; value; value = value.next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("blocks"), nullptr);

		for(ExprBase ref value = node.blocks.head; value; value = value.next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintGraph(ctx, node.defaultBlock, "defaultBlock");

		PrintLeaveBlock(ctx);
	}
	else if(ExprBreak ref node = getType with<ExprBreak>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprBreak(%d)", node.depth);

		PrintGraph(ctx, node.closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprContinue ref node = getType with<ExprContinue>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprContinue(%d)", node.depth);

		PrintGraph(ctx, node.closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprBlock ref node = getType with<ExprBlock>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprBlock()");

		for(ExprBase ref value = node.expressions.head; value; value = value.next)
			PrintGraph(ctx, value, "");

		PrintGraph(ctx, node.closures, "closures");

		PrintLeaveBlock(ctx);
	}
	else if(ExprSequence ref node = getType with<ExprSequence>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprSequence()");

		for(int i = 0; i < node.expressions.size(); i++)
			PrintGraph(ctx, node.expressions[i], "");

		PrintLeaveBlock(ctx);
	}
	else if(ExprModule ref node = getType with<ExprModule>(expression))
	{
		PrintEnterBlock(ctx, name, node, "ExprModule()");

		if(!ctx.skipImported)
			PrintGraph(ctx, node.moduleScope, true);

		PrintGraph(ctx, node.moduleScope, false);

		PrintEnterBlock(ctx, InplaceStr("definitions"), nullptr);

		for(int i = 0; i < node.definitions.size(); i++)
			PrintGraph(ctx, node.definitions[i], "");

		PrintLeaveBlock(ctx);

		ctx.skipFunctionDefinitions = true;

		PrintEnterBlock(ctx, InplaceStr("setup"), nullptr);

		for(ExprBase ref value = node.setup.head; value; value = value.next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintEnterBlock(ctx, InplaceStr("expressions"), nullptr);

		for(ExprBase ref value = node.expressions.head; value; value = value.next)
			PrintGraph(ctx, value, "");

		PrintLeaveBlock(ctx);

		PrintLeaveBlock(ctx);
	}
	else if(!expression)
	{
		PrintIndent(ctx);

		ctx.output.Write(name.data, name.begin, name.end - name.begin);
		ctx.output.Print(": null\n");
	}
	else
	{
		assert(false, "unknown type");
	}
}

void PrintGraph(ExpressionGraphContext ref ctx, ExprBase ref expression, char[] name)
{
	PrintGraph(ctx, expression, InplaceStr(name));

	//ctx.output.Flush();
}
/*
void DumpGraph(ExprBase ref tree)
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
	outputCtx.stream = nullptr;
}

void DumpGraph(ScopeData ref scope)
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
	outputCtx.stream = nullptr;
}
*/