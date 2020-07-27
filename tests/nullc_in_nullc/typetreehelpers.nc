import std.memory;
import typetree;

int strlen(char[] str)
{
	int count = 0;

	for(i in str)
	{
		if(i == 0)
			break;

		count++;
	}

	return count;
}

void memcpy(StringRef dst, InplaceStr src)
{
	memory.copy(dst.string, dst.pos, src.data, src.begin, src.end - src.begin);
}

void memcpy(StringRef dst, char[] src, int length)
{
	memory.copy(dst.string, dst.pos, src, 0, length);
}

void strcpy(StringRef dst, char[] src)
{
	if(src.size > 0)
		memory.copy(dst.string, dst.pos, src, 0, src.size - 1);
}

StringRef strstr(StringRef haystack, char[] needle)
{
    int last = haystack.length() - (needle.size - 1);

	for(int i = 0; i < last; i++)
	{
		bool wrong = false;

		for(int k = 0; k < needle.size - 1 && !wrong; k++)
		{
			if(haystack[i + k] != needle[k])
				wrong = true;
		}
		if(!wrong)
			return haystack + i;
	}

	return StringRef();
}

StringRef strstr(InplaceStr haystack, char[] needle)
{
    int last = haystack.length() - (needle.size - 1);

	for(int i = 0; i < last; i++)
	{
		bool wrong = false;

		for(int k = 0; k < needle.size - 1 && !wrong; k++)
		{
			if(haystack[i + k] != needle[k])
				wrong = true;
		}
		if(!wrong)
			return StringRef(haystack.data, haystack.begin + i);
	}

	return StringRef();
}

StringRef NameWriteUnsigned(StringRef pos, int value)
{
	char[16] reverse;
	StringRef curr = StringRef(reverse, 0);

	curr.advance((char)((value % 10) + '0'));

	while(value /= 10)
		curr.advance((char)((value % 10) + '0'));

	do
	{
		curr.rewind();
		pos.advance(curr[0]);
	}
	while(curr != StringRef(reverse, 0));

	return pos;
}

InplaceStr GetOperatorName(InplaceStr name)
{
	if(name == InplaceStr("+"))
		return InplaceStr("__operatorAdd");

	if(name == InplaceStr("-"))
		return InplaceStr("__operatorSub");

	if(name == InplaceStr("*"))
		return InplaceStr("__operatorMul");

	if(name == InplaceStr("/"))
		return InplaceStr("__operatorDiv");

	if(name == InplaceStr("%"))
		return InplaceStr("__operatorMod");

	if(name == InplaceStr("**"))
		return InplaceStr("__operatorPow");

	if(name == InplaceStr("<"))
		return InplaceStr("__operatorLess");

	if(name == InplaceStr(">"))
		return InplaceStr("__operatorGreater");

	if(name == InplaceStr("<="))
		return InplaceStr("__operatorLEqual");

	if(name == InplaceStr(">="))
		return InplaceStr("__operatorGEqual");

	if(name == InplaceStr("=="))
		return InplaceStr("__operatorEqual");

	if(name == InplaceStr("!="))
		return InplaceStr("__operatorNEqual");

	if(name == InplaceStr("<<"))
		return InplaceStr("__operatorShiftLeft");

	if(name == InplaceStr(">>"))
		return InplaceStr("__operatorShiftRight");

	if(name == InplaceStr("&"))
		return InplaceStr("__operatorBitAnd");

	if(name == InplaceStr("|"))
		return InplaceStr("__operatorBitOr");

	if(name == InplaceStr("^"))
		return InplaceStr("__operatorBitXor");

	if(name == InplaceStr("&&"))
		return InplaceStr("__operatorLogAnd");

	if(name == InplaceStr("||"))
		return InplaceStr("__operatorLogOr");

	if(name == InplaceStr("^^"))
		return InplaceStr("__operatorLogXor");

	if(name == InplaceStr("="))
		return InplaceStr("__operatorSet");

	if(name == InplaceStr("+="))
		return InplaceStr("__operatorAddSet");

	if(name == InplaceStr("-="))
		return InplaceStr("__operatorSubSet");

	if(name == InplaceStr("*="))
		return InplaceStr("__operatorMulSet");

	if(name == InplaceStr("/="))
		return InplaceStr("__operatorDivSet");

	if(name == InplaceStr("**="))
		return InplaceStr("__operatorPowSet");

	if(name == InplaceStr("%="))
		return InplaceStr("__operatorModSet");

	if(name == InplaceStr("<<="))
		return InplaceStr("__operatorShlSet");

	if(name == InplaceStr(">>="))
		return InplaceStr("__operatorShrSet");

	if(name == InplaceStr("&="))
		return InplaceStr("__operatorAndSet");

	if(name == InplaceStr("|="))
		return InplaceStr("__operatorOrSet");

	if(name == InplaceStr("^="))
		return InplaceStr("__operatorXorSet");

	if(name == InplaceStr("[]"))
		return InplaceStr("__operatorIndex");

	if(name == InplaceStr("!"))
		return InplaceStr("__operatorLogNot");

	if(name == InplaceStr("~"))
		return InplaceStr("__operatorBitNot");

	if(name == InplaceStr("()"))
		return InplaceStr("__operatorFuncCall");

	return InplaceStr();
}

InplaceStr GetReferenceTypeName(ExpressionContext ref ctx, TypeBase ref type)
{
	int typeNameLength = int(type.name.end - type.name.begin);

	int nameLength = int(typeNameLength + 4);
	char[] name = new char[(nameLength + 1)];

    StringRef pos = StringRef(name, 0);

	if(typeNameLength)
		memcpy(pos, type.name);

	memcpy(pos + typeNameLength, " ref", 5);

	return InplaceStr(name);
}

void sprintf(char[] buffer, char[] format, auto ref[] args)
{
	int arg = 0;
	int pos = 0;

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
					
					if(str.size > 0)
					{
						memory.copy(buffer, pos, str, 0, str.size - 1);
						pos += str.size - 1;
					}
				}
				else if(args[arg].type == InplaceStr)
				{
					InplaceStr str = args[arg];
					
					for(int k = str.begin; k < str.end; k++)
						buffer[pos++] = str.data[k];
				}
				else if(args[arg].type == StringRef)
				{
					StringRef str = args[arg];

					int strPos = 0;
					while(str.string[str.pos + strPos])
						buffer[pos++] = str.string[str.pos + strPos++];
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

			if(format[i] == 'd' || format[i] == 'u')
			{
				if(args[arg].type == int)
				{
					int value = args[arg];
					
					char[] str;

					if(format[i] == 'u')
						str = as_unsigned(value).str();
					else
						str = value.str();

					if(str.size > 0)
					{
						if(width != -1 && width > str.size - 1)
						{
							char[] tmp = new char[width + 1];

							if(zeroWidth)
							{
								for(el in tmp)
									el = '0';

								tmp[tmp.size - 1] = 0;
							}

							memory.copy(tmp, tmp.size - str.size, str, 0, str.size - 1);

							str = tmp;
						}

						memory.copy(buffer, pos, str, 0, str.size - 1);
						pos += str.size - 1;
					}
				}
				else if(args[arg].type == long)
				{
					long value = args[arg];
					
					char[] str = value.str();

					if(str.size > 0)
					{
						memory.copy(buffer, pos, str, 0, str.size - 1);
						pos += str.size - 1;
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
			buffer[pos++] = format[i];
		}
	}
}

InplaceStr GetArrayTypeName(ExpressionContext ref ctx, TypeBase ref type, long length)
{
	int nameLength = int(type.name.length() + strlen("[]") + 21);
	char[] name = new char[(nameLength + 1)];
	sprintf(name, "%.*s[%lld]", FMT_ISTR(type.name), length);

	return InplaceStr(name);
}

InplaceStr GetUnsizedArrayTypeName(ExpressionContext ref ctx, TypeBase ref type)
{
	int nameLength = int(type.name.length() + strlen("[]"));
	char[] name = new char[(nameLength + 1)];

	StringRef pos = StringRef(name, 0);

	memcpy(pos, type.name);
	pos += type.name.length();

	memcpy(pos, "[]", 2);
	pos += 2;

    pos[0] = 0;

	return InplaceStr(name);
}

InplaceStr GetFunctionTypeName(ExpressionContext ref ctx, TypeBase ref returnType, RefList<TypeHandle> arguments)
{
	int nameLength = int(returnType.name.length() + strlen(" ref()"));

	for(TypeHandle ref arg = arguments.head; arg; arg = arg.next)
		nameLength += arg.type.name.length() + 1;

	char[] name = new char[(nameLength + 1)];

	StringRef pos = StringRef(name, 0);

	if(returnType.name)
		memcpy(pos, returnType.name);
	pos += returnType.name.length();

	memcpy(pos, " ref(", 5);
	pos += 5;

	for(TypeHandle ref arg = arguments.head; arg; arg = arg.next)
	{
		if(arg.type.name)
			memcpy(pos, arg.type.name);
		pos += arg.type.name.length();

		if(arg.next)
			pos.advance(',');
	}

	pos.advance(')');
	pos.advance(0);

	return InplaceStr(name);
}

InplaceStr GetGenericClassTypeName(ExpressionContext ref ctx, TypeBase ref proto, RefList<TypeHandle> generics)
{
	int nameLength = int(proto.name.length() + strlen("<>"));

	for(TypeHandle ref arg = generics.head; arg; arg = arg.next)
	{
		if(arg.type.isGeneric)
			nameLength += int(strlen("generic") + 1);
		else
			nameLength += arg.type.name.length() + 1;
	}

	char[] name = new char[(nameLength + 1)];

	StringRef pos = StringRef(name, 0);

	memcpy(pos, proto.name);
	pos += proto.name.length();

	strcpy(pos, "<");
	pos += 1;

	for(TypeHandle ref arg = generics.head; arg; arg = arg.next)
	{
		if(arg.type.isGeneric)
		{
			strcpy(pos, "generic");
			pos += strlen("generic");
		}
		else
		{
			memcpy(pos, arg.type.name);
			pos += arg.type.name.length();
		}

		if(arg.next)
			pos.advance(',');
	}

	pos.advance('>');
	pos.advance(0);

	return InplaceStr(name);
}

InplaceStr GetFunctionSetTypeName(ExpressionContext ref ctx, RefList<TypeHandle> types)
{
	assert(!types.empty());

	int nameLength = 0;

	for(TypeHandle ref arg = types.head; arg; arg = arg.next)
		nameLength += int(arg.type.name.length() + strlen(" or "));

	char[] name = new char[(nameLength + 1)];

	StringRef pos = StringRef(name, 0);

	for(TypeHandle ref arg = types.head; arg; arg = arg.next)
	{
		memcpy(pos, arg.type.name);
		pos += arg.type.name.length();

		if(arg.next)
		{
			strcpy(pos, " or ");
			pos += strlen(" or ");
		}
	}

	pos.advance(0);

	return InplaceStr(name);
}

InplaceStr GetArgumentSetTypeName(ExpressionContext ref ctx, RefList<TypeHandle> types)
{
	int nameLength = 2;

	for(TypeHandle ref arg = types.head; arg; arg = arg.next)
		nameLength += arg.type.name.length() + 1;

	char[] name = new char[(nameLength + 1)];

	StringRef pos = StringRef(name, 0);

	pos.advance('(');

	for(TypeHandle ref arg = types.head; arg; arg = arg.next)
	{
		memcpy(pos, arg.type.name);
		pos += arg.type.name.length();

		if(arg.next)
			pos.advance(',');
	}

	pos.advance(')');
	pos.advance(0);

	return InplaceStr(name);
}

InplaceStr GetMemberSetTypeName(ExpressionContext ref ctx, TypeBase ref type)
{
	int nameLength = int(type.name.length() + strlen(" members"));
	char[] name = new char[(nameLength + 1)];

	StringRef pos = StringRef(name, 0);

	memcpy(pos, type.name);
	pos += type.name.length();

	strcpy(pos, " members");
	pos += strlen(" members");

	pos.advance(0);

	return InplaceStr(name);
}

InplaceStr GetGenericAliasTypeName(ExpressionContext ref ctx, InplaceStr baseName)
{
	int nameLength = baseName.length() + 1;
	char[] name = new char[(nameLength + 1)];

	StringRef pos = StringRef(name, 0);

	pos.advance('@');

	memcpy(pos, baseName);
	pos += baseName.length();

	pos.advance(0);

	return InplaceStr(name);
}

InplaceStr GetFunctionContextTypeName(ExpressionContext ref ctx, InplaceStr functionName, int index)
{
	if((functionName[0] <= '@' && functionName[0] != '$') || functionName[0] == '|' || functionName[0] == '^' || functionName[0] == '~')
	{
		InplaceStr operatorName = GetOperatorName(functionName);

		if(!operatorName.empty())
			functionName = operatorName;
	}

	int nameLength = functionName.length() + 32;
	char[] name = new char[(nameLength + 1)];

	StringRef pos = StringRef(name, 0);

	pos.advance('_');
	pos.advance('_');

	memcpy(pos, functionName);
	pos += functionName.length();

	pos.advance('_');
	pos = NameWriteUnsigned(pos, index);

	strcpy(pos, "_cls");
	pos += strlen("_cls");

	pos.advance(0);

	return InplaceStr(name);
}

InplaceStr GetFunctionContextVariableName(ExpressionContext ref ctx, FunctionData ref function, int index)
{
	InplaceStr functionName = function.name.name;

	if((functionName[0] <= '@' && functionName[0] != '$') || functionName[0] == '|' || functionName[0] == '^' || functionName[0] == '~')
	{
		InplaceStr operatorName = GetOperatorName(functionName);

		if(!operatorName.empty())
			functionName = operatorName;
	}

	int nameLength = functionName.length() + 32;
	char[] name = new char[(nameLength + 1)];

	StringRef pos = StringRef(name, 0);

	pos.advance('$');

	memcpy(pos, functionName);
	pos += functionName.length();

	pos.advance('_');
	pos = NameWriteUnsigned(pos, function.type.name.hash());

	pos.advance('_');
	pos = NameWriteUnsigned(pos, index);

	strcpy(pos, "_ext");
	pos += strlen("_ext");

	pos.advance(0);

	return InplaceStr(name);
}

InplaceStr GetFunctionTableName(ExpressionContext ref ctx, FunctionData ref function)
{
	assert(function.scope.ownerType != nullptr);

	StringRef pos = strstr(function.name.name, "::");

	assert(pos.string != nullptr);

	int nameLength = function.name.name.length() + 32;
	char[] name = new char[(nameLength + 1)];
	sprintf(name, "$vtbl%010u%s", function.type.name.hash(), pos + 2);

	return InplaceStr(name);
}

InplaceStr GetFunctionContextMemberName(ExpressionContext ref ctx, InplaceStr prefix, InplaceStr suffix, int index)
{
	int nameLength = prefix.length() + 1 + suffix.length() + (index != 0 ? 16 : 0) + 1;
	char[] name = new char[(nameLength)];

	StringRef pos = StringRef(name, 0);

	memcpy(pos, prefix);
	pos += prefix.length();

	pos.advance('_');

	memcpy(pos, suffix);
	pos += suffix.length();

	if(index != 0)
	{
		pos.advance('_');
		pos = NameWriteUnsigned(pos, index);
	}

	pos.advance(0);

	return InplaceStr(name);
}

FunctionData ref GetFunctionOwner(ScopeData ref scopeData)
{
	// Temporary scopes have no owner
	if(scopeData.type == ScopeType.SCOPE_TEMPORARY)
		return nullptr;

	// Walk up, but if we reach a type or namespace owner, stop - we're not in a context of a function
	for(ScopeData ref curr = scopeData; curr; curr = curr.scope)
	{
		if(curr.ownerType)
			return nullptr;

		if(curr.ownerNamespace)
			return nullptr;

		if(FunctionData ref function = curr.ownerFunction)
			return function;
	}

	return nullptr;
}

InplaceStr GetFunctionVariableUpvalueName(ExpressionContext ref ctx, VariableData ref variable)
{
	FunctionData ref function = GetFunctionOwner(variable.scope);

	InplaceStr functionName = function ? function.name.name : InplaceStr("global");

	if((functionName[0] <= '@' && functionName[0] != '$') || functionName[0] == '|' || functionName[0] == '^' || functionName[0] == '~')
	{
		InplaceStr operatorName = GetOperatorName(functionName);

		if(!operatorName.empty())
			functionName = operatorName;
	}

	int nameLength = functionName.length() + variable.name.name.length() + 24;
	char[] name = new char[(nameLength)];

	StringRef pos = StringRef(name, 0);

	strcpy(pos, "$upvalue_");
	pos += strlen("$upvalue_");

	memcpy(pos, functionName);
	pos += functionName.length();

	pos.advance('_');

	memcpy(pos, variable.name.name);
	pos += variable.name.name.length();

	pos.advance('_');
	pos = NameWriteUnsigned(pos, variable.uniqueId);

	pos.advance(0);

	return InplaceStr(name);
}

InplaceStr GetTypeNameInScope(ExpressionContext ref ctx, ScopeData ref scope, InplaceStr str)
{
	bool foundNamespace = false;

	int nameLength = str.length();

	for(ScopeData ref curr = scope; curr; curr = curr.scope)
	{
		if((curr.ownerType || curr.ownerFunction) && !foundNamespace)
			break;

		if(curr.ownerNamespace)
		{
			nameLength += curr.ownerNamespace.name.name.length() + 1;

			foundNamespace = true;
		}
	}

	if(!foundNamespace)
		return str;

	char[] name = new char[(nameLength + 1)];

	// Format a string back-to-front
	StringRef pos = StringRef(name, nameLength);

	pos[0] = 0;

	if(int strLength = str.length())
	{
		pos -= strLength;
		memcpy(pos, str);
	}

	for(ScopeData ref curr = scope; curr; curr = curr.scope)
	{
		if(curr.ownerNamespace)
		{
			InplaceStr nsName = curr.ownerNamespace.name.name;

			pos -= 1;
			pos[0] = '.';

			pos -= nsName.length();
			memcpy(pos, nsName);
		}
	}

	assert(pos == StringRef(name, 0));

	return InplaceStr(name);
}

InplaceStr GetVariableNameInScope(ExpressionContext ref ctx, ScopeData ref scope, InplaceStr str)
{
	return GetTypeNameInScope(ctx, scope, str);
}

InplaceStr GetFunctionNameInScope(ExpressionContext ref ctx, ScopeData ref scope, TypeBase ref parentType, InplaceStr str, bool isOperator, bool isAccessor)
{
	if(parentType)
	{
		char[] name = new char[(parentType.name.length() + 2 + str.length() + (isAccessor ? 1 : 0) + 1)];

		sprintf(name, "%.*s::%.*s%s", FMT_ISTR(parentType.name), FMT_ISTR(str), isAccessor ? "$" : "");

		return InplaceStr(name);
	}

	if(isOperator)
		return str;

	assert(!isAccessor);

	bool foundNamespace = false;

	int nameLength = str.length();

	for(ScopeData ref curr = scope; curr; curr = curr.scope)
	{
		// Temporary scope is not evaluated
		if(curr.type == ScopeType.SCOPE_TEMPORARY)
			return str;

		// Function scope, just use the name
		if(curr.ownerFunction && !foundNamespace)
			return str;

		if(curr.ownerType)
			assert(foundNamespace);

		if(curr.ownerNamespace)
		{
			nameLength += curr.ownerNamespace.name.name.length() + 1;

			foundNamespace = true;
		}
	}

	char[] name = new char[(nameLength + 1)];

	if(!foundNamespace)
		return str;

	// Format a string back-to-front
	StringRef pos = StringRef(name, nameLength);

	pos[0] = 0;

	pos -= str.length();
	memcpy(pos, str);

	for(ScopeData ref curr = scope; curr; curr = curr.scope)
	{
		if(curr.ownerNamespace)
		{
			InplaceStr nsName = curr.ownerNamespace.name.name;

			pos -= 1;
			pos[0] = '.';

			pos -= nsName.length();
			memcpy(pos, nsName);
		}
	}

	assert(pos == StringRef(name, 0));

	return InplaceStr(name);
}

InplaceStr GetTemporaryName(ExpressionContext ref ctx, int index, char[] suffix)
{
	char[16] buf;

	StringRef curr = StringRef(buf, 0);

	curr.advance((char)((index % 10) + '0'));

	while(index /= 10)
		curr.advance((char)((index % 10) + '0'));

	int suffixLength = suffix ? strlen(suffix) : 0;

	char[] name = new char[(16 + suffixLength)];

	StringRef pos = StringRef(name, 0);

	memcpy(pos, "$temp", 5);
	pos += 5;

	do
	{
		curr.rewind();
		pos.advance(curr[0]);
	}
	while(curr != StringRef(buf, 0));

	if(suffix)
	{
		pos.advance('_');

		memcpy(pos, suffix, suffixLength);
		pos += suffixLength;
	}

	pos[0] = 0;

	return InplaceStr(name, 0, pos.pos);
}

int GetAlignmentOffset(long offset, int alignment)
{
	// If alignment is set and address is not aligned
	if(alignment != 0 && offset % alignment != 0)
		return alignment - (offset % alignment);

	return 0;
}
