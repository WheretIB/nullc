import nullcdef;
import parsetree;
import stringutil;
import reflist;
import arrayview;
import lexer;
import common;
import std.vector;
import std.io;
import std.hashmap;
import std.string;

class ParseContext
{
	void ParseContext(int optimizationLevel, ArrayView<InplaceStr> activeImports);

	LexemeType Peek();
	InplaceStr Value();
	bool At(LexemeType type);
	bool Consume(LexemeType type);
	bool Consume(char[] str);
	InplaceStr Consume();
	void Skip();

	LexemeRef First();
	LexemeRef Current();
	LexemeRef Previous();
	LexemeRef Last();

	StringRef Position();
	StringRef LastEnding();

	SynNamespaceElement ref IsNamespace(SynNamespaceElement ref parent, InplaceStr name);
	SynNamespaceElement ref PushNamespace(SynIdentifier ref name);
	void PopNamespace();

	char[] code;

	ByteCode ref ref(string, string, bool, StringRef ref, string ref, int, ArrayView<InplaceStr>) bytecodeBuilder;

	Lexer lexer;

	LexemeRef firstLexeme;
	LexemeRef currentLexeme;
	LexemeRef lastLexeme;

	vector<SynBinaryOpElement> binaryOpStack;

	int expressionGroupDepth;
	int expressionBlockDepth;
	int statementBlockDepth;

	vector<SynNamespaceElement ref> namespaceList;
	SynNamespaceElement ref currentNamespace;

	int optimizationLevel;
	vector<InplaceStr> activeImports;

	bool errorHandlerActive;
	StringRef errorPos;
	int errorCount;
	string ref errorBuf;

	vector<ErrorInfo ref> errorInfo;

	hashmap<int, bool> nonTypeLocations;
	hashmap<int, bool> nonFunctionDefinitionLocations;
}

void ParseContext::ParseContext(int optimizationLevel, ArrayView<InplaceStr> activeImports)
{
	this.optimizationLevel = optimizationLevel;

	code = nullptr;

	bytecodeBuilder = nullptr;

	expressionGroupDepth = 0;
	expressionBlockDepth = 0;
	statementBlockDepth = 0;

	errorHandlerActive = false;
	errorCount = 0;
	errorBuf = nullptr;

	currentNamespace = nullptr;

	for(int i = 0; i < activeImports.size(); i++)
		this.activeImports.push_back(activeImports[i]);
}

LexemeType ParseContext::Peek()
{
	return currentLexeme.type;
}

InplaceStr ParseContext::Value()
{
	return InplaceStr(currentLexeme.pos, currentLexeme.length);
}

bool ParseContext::At(LexemeType type)
{
	return currentLexeme.type == type;
}

bool ParseContext::Consume(LexemeType type)
{
	if(currentLexeme.type == type)
	{
		Skip();
		return true;
	}

	return false;
}

bool ParseContext::Consume(char[] str)
{
	if(InplaceStr(currentLexeme.pos, currentLexeme.length) == InplaceStr(str))
	{
		Skip();
		return true;
	}

	return false;
}

InplaceStr ParseContext::Consume()
{
	InplaceStr str = InplaceStr(currentLexeme.pos, currentLexeme.length);

	Skip();

	return str;
}

void ParseContext::Skip()
{
	if(currentLexeme.type != LexemeType.lex_none)
		currentLexeme.advance();
}

LexemeRef ParseContext::First()
{
	return firstLexeme;
}

LexemeRef ParseContext::Current()
{
	return currentLexeme;
}

LexemeRef ParseContext::Previous()
{
	assert(currentLexeme > firstLexeme);

	return currentLexeme - 1;
}

LexemeRef ParseContext::Last()
{
	return lastLexeme;
}

StringRef ParseContext::Position()
{
	return currentLexeme.pos;
}

StringRef ParseContext::LastEnding()
{
	assert(currentLexeme > firstLexeme);

	return (currentLexeme - 1).pos + (currentLexeme - 1).length;
}

SynNamespaceElement ref ParseContext::IsNamespace(SynNamespaceElement ref parent, InplaceStr name)
{
	// In the context of a parent namespace, we only look for immediate children
	if(parent)
	{
		// Search for existing namespace in the same context
		for(int i = 0; i < namespaceList.size(); i++)
		{
			SynNamespaceElement ref ns = namespaceList[i];

			if(ns.parent == parent && ns.name.name == name)
				return ns;
		}

		// Try from context of the parent namespace
		if(parent.parent)
			return IsNamespace(parent.parent, name);

		return nullptr;
	}

	// Go from the bottom of the namespace stack, trying to find the namespace there
	SynNamespaceElement ref current = currentNamespace;

	for(;1;)
	{
		for(int i = 0; i < namespaceList.size(); i++)
		{
			SynNamespaceElement ref ns = namespaceList[i];

			if(ns.parent == current && ns.name.name == name)
				return ns;
		}

		if(current)
			current = current.parent;
		else
			break;
	}

	return nullptr;
}

SynNamespaceElement ref ParseContext::PushNamespace(SynIdentifier ref name)
{
	SynNamespaceElement ref current = currentNamespace;

	// Search for existing namespace in the same context
	for(SynNamespaceElement ref ns in namespaceList)
	{
		if(ns.parent == current && ns.name.name == name.name)
		{
			currentNamespace = ns;
			return ns;
		}
	}

	// Create new namespace
	SynNamespaceElement ref ns = new SynNamespaceElement(current, name);
	namespaceList.push_back(ns);

	currentNamespace = ns;
	return ns;
}

void ParseContext::PopNamespace()
{
	assert(currentNamespace != nullptr);

	currentNamespace = currentNamespace.parent;
}

void ReportAt(ParseContext ref ctx, LexemeRef begin, LexemeRef end, StringRef pos, char[] msg, auto ref[] args)
{
	if(ctx.errorBuf)
	{
		/*if(ctx.errorCount == 0)
		{
			ctx.errorPos = pos;
			ctx.errorBufLocation = ctx.errorBuf;
		}

		// Don't report multiple errors at the same position
		if(!ctx.errorInfo.empty() && ctx.errorInfo.back().pos == pos)
			return;

		StringRef messageStart = ctx.errorBufLocation;

		vsnprintf(ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf), msg, args);
		ctx.errorBuf[ctx.errorBufSize - 1] = '\0';

		ctx.errorBufLocation += strlen(ctx.errorBufLocation);

		StringRef messageEnd = ctx.errorBufLocation;

		ctx.errorInfo.push_back(new ErrorInfo(ctx.allocator, messageStart, messageEnd, begin, end, pos));

		AddErrorLocationInfo(ctx.code, pos, ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf));

		ctx.errorBufLocation += strlen(ctx.errorBufLocation);*/
	}

	ctx.errorCount++;

	/*if(ctx.errorCount == 100)
	{
		SafeSprintf(ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf), "ERROR: error limit reached");

		ctx.errorBufLocation += strlen(ctx.errorBufLocation);

		assert(ctx.errorHandlerActive);

		longjmp(ctx.errorHandler, 1);
	}*/
}

void Report(ParseContext ref ctx, LexemeRef pos, char[] msg, auto ref[] args)
{
	if(pos == ctx.Last() && pos != ctx.First())
		ReportAt(ctx, pos - 1, pos - 1, (pos - 1).pos + (pos - 1).length, msg, args);
	else
		ReportAt(ctx, pos, pos, pos.pos, msg, args);
}

void StopAt(ParseContext ref ctx, LexemeRef begin, LexemeRef end, StringRef pos, char[] msg, auto ref[] args)
{
	ReportAt(ctx, begin, end, pos, msg, args);

	assert(ctx.errorHandlerActive);

	io.out << msg << io.endl;

	for(int i = 0; i < 16 && i + pos.pos < pos.string.size; i++)
		io.out << pos[i];
	io.out << io.endl;

	// TODO: continue?
	assert(0, "Parse Error");

	//longjmp(ctx.errorHandler, 1);
}

void Stop(ParseContext ref ctx, LexemeRef pos, char[] msg, auto ref[] args)
{
	if(pos == ctx.Last() && pos != ctx.First())
		StopAt(ctx, pos - 1, pos - 1, (pos - 1).pos + (pos - 1).length, msg, args);
	else
		StopAt(ctx, pos, pos, pos.pos, msg, args);
}

bool CheckAt(ParseContext ref ctx, LexemeType type, char[] msg, auto ref[] args)
{
	if(!ctx.At(type))
	{
		LexemeRef curr = ctx.Current();

		if(curr == ctx.Last() && curr != ctx.First())
			ReportAt(ctx, curr - 1, curr - 1, (curr - 1).pos + (curr - 1).length, msg, args);
		else
			ReportAt(ctx, curr, curr, curr.pos, msg, args);

		return false;
	}

	return true;
}

bool CheckConsume(ParseContext ref ctx, LexemeType type, char[] msg, auto ref[] args)
{
	if(!ctx.Consume(type))
	{
		LexemeRef curr = ctx.Current();

		if(curr == ctx.Last() && curr != ctx.First())
			ReportAt(ctx, curr - 1, curr - 1, (curr - 1).pos + (curr - 1).length, msg, args);
		else
			ReportAt(ctx, curr, curr, curr.pos, msg, args);

		return false;
	}

	return true;
}

bool CheckConsume(ParseContext ref ctx, char[] str, char[] msg, auto ref[] args)
{
	if(!ctx.Consume(str))
	{
		LexemeRef curr = ctx.Current();

		if(curr == ctx.Last() && curr != ctx.First())
			ReportAt(ctx, curr - 1, curr - 1, (curr - 1).pos + (curr - 1).length, msg, args);
		else
			ReportAt(ctx, curr, curr, curr.pos, msg, args);

		return false;
	}

	return true;
}

SynBase ref ParseType(ParseContext ref ctx, bool ref shrBorrow, bool onlyType);
SynBase ref ParsePostExpressions(ParseContext ref ctx, SynBase ref node);
SynBase ref ParseTerminal(ParseContext ref ctx);
SynBase ref ParseTernaryExpr(ParseContext ref ctx);
SynBase ref ParseAssignment(ParseContext ref ctx);
SynTypedef ref ParseTypedef(ParseContext ref ctx);
SynBase ref ParseExpression(ParseContext ref ctx);
RefList<SynBase> ParseExpressions(ParseContext ref ctx);
SynFunctionDefinition ref ParseFunctionDefinition(ParseContext ref ctx);
SynShortFunctionDefinition ref ParseShortFunctionDefinition(ParseContext ref ctx);
SynVariableDefinition ref ParseVariableDefinition(ParseContext ref ctx);
SynVariableDefinitions ref ParseVariableDefinitions(ParseContext ref ctx, bool classMembers);
SynAccessor ref ParseAccessorDefinition(ParseContext ref ctx);
SynConstantSet ref ParseConstantSet(ParseContext ref ctx);
SynClassStaticIf ref ParseClassStaticIf(ParseContext ref ctx, bool nested);
RefList<SynCallArgument> ParseCallArguments(ParseContext ref ctx);

SynBase ref ParseTerminalType(ParseContext ref ctx, bool ref shrBorrow)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.At(LexemeType.lex_identifier))
	{
		InplaceStr name = ctx.Consume();

		RefList<SynIdentifier> namespacePath;

		SynNamespaceElement ref ns = nullptr;

		while(ctx.At(LexemeType.lex_dblcolon) && (ns = ctx.IsNamespace(ns, name)) != nullptr)
		{
			ctx.Skip();

			if(!CheckAt(ctx, LexemeType.lex_identifier, "ERROR: namespace member is expected after '::'"))
			{
				namespacePath.push_back(new SynIdentifier(start, ctx.Previous(), name));

				break;
			}

			namespacePath.push_back(new SynIdentifier(start, ctx.Previous(), name));

			name = ctx.Consume();
		}

		if(ctx.Consume(LexemeType.lex_less))
		{
			RefList<SynBase> types;

			SynBase ref type = ParseType(ctx, shrBorrow, false);

			if(!type)
			{
				if(ctx.Peek() == LexemeType.lex_greater)
					Report(ctx, ctx.Current(), "ERROR: typename required after '<'");

				// Backtrack
				ctx.currentLexeme = start;

				return nullptr;
			}

			types.push_back(type);

			while(ctx.Consume(LexemeType.lex_comma))
			{
				type = ParseType(ctx, shrBorrow, false);

				if(!type)
				{
					Report(ctx, ctx.Current(), "ERROR: typename required after ','");

					break;
				}

				types.push_back(type);
			}

			bool closed = ctx.Consume(LexemeType.lex_greater);

			if(!closed && ctx.At(LexemeType.lex_shr))
			{
				if(shrBorrow)
					ctx.Skip();

				*shrBorrow = !*shrBorrow;

				closed = true;
			}

			if(!closed)
			{
				if(types.size() > 1)
				{
					Report(ctx, ctx.Current(), "ERROR: '>' expected after generic type alias list");
				}
				else
				{
					// Backtrack
					ctx.currentLexeme = start;

					return nullptr;
				}
			}

			return new SynTypeGenericInstance(start, ctx.Previous(), new SynTypeSimple(start, ctx.Previous(), namespacePath, name), types);
		}

		return new SynTypeSimple(start, ctx.Previous(), namespacePath, name);
	}

	if(ctx.Consume(LexemeType.lex_auto))
		return new SynTypeAuto(start, ctx.Previous());

	if(ctx.Consume(LexemeType.lex_generic))
		return new SynTypeGeneric(start, ctx.Previous());

	if(ctx.Consume(LexemeType.lex_typeof))
	{
		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: typeof must be followed by '('");

		SynBase ref value = ParseAssignment(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after typeof(");

			value = new SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after expression in typeof");

		SynBase ref node = new SynTypeof(start, ctx.Previous(), value);

		return ParsePostExpressions(ctx, node);
	}

	if(ctx.Consume(LexemeType.lex_at))
	{
		if(!ctx.At(LexemeType.lex_identifier))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return nullptr;
		}

		InplaceStr name = ctx.Consume();
		SynIdentifier ref nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		return new SynTypeAlias(start, ctx.Previous(), nameIdentifier);
	}

	return nullptr;
}

SynBase ref ParseType(ParseContext ref ctx, bool ref shrBorrow = nullptr, bool onlyType = false)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.nonTypeLocations.find(int(start - ctx.firstLexeme) + 1))
		return nullptr;

	bool shrBorrowTerminal = shrBorrow ? *shrBorrow : false;

	SynBase ref base = ParseTerminalType(ctx, shrBorrowTerminal);

	if(!base)
		return nullptr;

	if(shrBorrowTerminal)
	{
		if(!shrBorrow)
		{
			// Backtrack
			ctx.currentLexeme = start;
			ctx.nonTypeLocations.insert(int(start - ctx.firstLexeme) + 1, true);

			return nullptr;
		}
		else
		{
			*shrBorrow = true;
		}
	}

	while(ctx.At(LexemeType.lex_obracket) || ctx.At(LexemeType.lex_ref))
	{
		if(ctx.At(LexemeType.lex_obracket))
		{
			RefList<SynBase> sizes;

			while(ctx.Consume(LexemeType.lex_obracket))
			{
				LexemeRef sizeStart = ctx.currentLexeme;

				SynBase ref size = ParseTernaryExpr(ctx);

				if(size && !ctx.At(LexemeType.lex_cbracket))
				{
					if(onlyType)
						Report(ctx, ctx.Current(), "ERROR: ']' not found after expression");

					// Backtrack
					ctx.currentLexeme = start;
					ctx.nonTypeLocations.insert(int(start - ctx.firstLexeme) + 1, true);

					return nullptr;
				}

				bool hasClose = CheckConsume(ctx, LexemeType.lex_cbracket, "ERROR: matching ']' not found");

				if(size)
					sizes.push_back(size);
				else
					sizes.push_back(new SynNothing(sizeStart, hasClose ? ctx.Previous() : ctx.Current()));
			}

			base = new SynTypeArray(start, ctx.Previous(), base, sizes);
		}
		else if(ctx.Consume(LexemeType.lex_ref))
		{
			LexemeRef refLexeme = ctx.currentLexeme;

			if(ctx.Consume(LexemeType.lex_oparen))
			{
				RefList<SynBase> arguments;

				if(SynBase ref argument = ParseType(ctx))
				{
					arguments.push_back(argument);

					while(ctx.Consume(LexemeType.lex_comma))
					{
						argument = ParseType(ctx);

						if(!argument)
						{
							Report(ctx, ctx.Current(), "ERROR: type is expected after ','");

							break;
						}

						arguments.push_back(argument);
					}
				}

				if(!ctx.Consume(LexemeType.lex_cparen))
				{
					// Backtrack
					ctx.currentLexeme = refLexeme;

					return new SynTypeReference(start, ctx.Previous(), base);
				}

				base = new SynTypeFunction(start, ctx.Previous(), base, arguments);
			}
			else
			{
				base = new SynTypeReference(start, ctx.Previous(), base);
			}
		}
	}

	return base;
}

SynBase ref ParseArray(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_ofigure))
	{
		if(ctx.At(LexemeType.lex_for))
		{
			RefList<SynBase> expressions = ParseExpressions(ctx);

			CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' not found after inline array");

			return new SynGenerator(start, ctx.Previous(), expressions);
		}

		int blockLimit = 256;

		if(ctx.expressionBlockDepth >= blockLimit)
			Stop(ctx, ctx.Current(), "ERROR: reached nested array limit of %d", blockLimit);

		ctx.expressionBlockDepth++;

		RefList<SynBase> values;

		SynBase ref value = ParseTernaryExpr(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: value not found after '{'");

			value = new SynError(ctx.Current(), ctx.Current());
		}

		values.push_back(value);

		while(ctx.Consume(LexemeType.lex_comma))
		{
			value = ParseTernaryExpr(ctx);

			if(!value)
				Report(ctx, ctx.Current(), "ERROR: value not found after ','");
			else
				values.push_back(value);
		}

		ctx.expressionBlockDepth--;

		CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' not found after inline array");

		return new SynArray(start, ctx.Previous(), values);
	}

	return nullptr;
}

SynBase ref ParseString(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	bool rawLiteral = ctx.Consume(LexemeType.lex_at);

	if(ctx.At(LexemeType.lex_quotedstring))
	{
		InplaceStr str = ctx.Consume();

		if(str.length() == 1 || str[str.length() - 1] != '\"')
			Report(ctx, start, "ERROR: unclosed string constant");

		return new SynString(start, ctx.Previous(), rawLiteral, str);
	}

	if(rawLiteral)
	{
		// Backtrack
		ctx.currentLexeme = start;
	}

	return nullptr;
}

SynBase ref ParseSizeof(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_sizeof))
	{
		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: sizeof must be followed by '('");

		SynBase ref value = ParseAssignment(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression or type not found after sizeof(");

			value = new SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after expression in sizeof");

		return new SynSizeof(start, ctx.Previous(), value);
	}

	return nullptr;
}

SynNumber ref ParseNumber(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.At(LexemeType.lex_number))
	{
		InplaceStr value = ctx.Consume();

		InplaceStr suffix;

		if(ctx.At(LexemeType.lex_identifier))
			suffix = ctx.Consume();

		return new SynNumber(start, ctx.Previous(), value, suffix);
	}

	return nullptr;
}

SynAlign ref ParseAlign(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_align))
	{
		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: '(' expected after align");

		SynBase ref value = nullptr;

		if(CheckAt(ctx, LexemeType.lex_number, "ERROR: alignment value not found after align("))
			value = ParseNumber(ctx);
		else
			value = new SynError(ctx.Current(), ctx.Current());

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' expected after alignment value");

		return new SynAlign(start, ctx.Previous(), value);
	}
	else if(ctx.Consume(LexemeType.lex_noalign))
	{
		return new SynAlign(start, ctx.Previous(), nullptr);
	}

	return nullptr;
}

SynNew ref ParseNew(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_new))
	{
		SynBase ref type = nullptr;

		bool explicitType = ctx.Consume(LexemeType.lex_oparen);

		if(explicitType)
		{
			type = ParseType(ctx);

			if(!type)
			{
				Report(ctx, ctx.Current(), "ERROR: type name expected after 'new'");

				type = new SynError(ctx.Current(), ctx.Current());
			}

			CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: matching ')' not found after '('");
		}
		else
		{
			type = ParseType(ctx, nullptr, true);

			if(!type)
			{
				Report(ctx, ctx.Current(), "ERROR: type name expected after 'new'");

				type = new SynError(ctx.Current(), ctx.Current());
			}
		}

		RefList<SynCallArgument> arguments;

		RefList<SynBase> constructor;

		if(ctx.Consume(LexemeType.lex_obracket))
		{
			SynBase ref count = ParseTernaryExpr(ctx);

			if(!count)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '['");

				count = new SynError(ctx.Current(), ctx.Current());
			}

			CheckConsume(ctx, LexemeType.lex_cbracket, "ERROR: ']' not found after expression");

			return new SynNew(start, ctx.Previous(), type, arguments, count, constructor);
		}
		else if(!explicitType && isType with<SynTypeArray>(type))
		{
			SynTypeArray ref arrayType = getType with<SynTypeArray>(type);

			// Try to extract last array type extent as a size
			SynBase ref prevSize = nullptr;
			SynBase ref count = arrayType.sizes.head;

			while(count.next)
			{
				prevSize = count;
				count = count.next;
			}

			// Check if the extent is real
			if(!isType with<SynNothing>(count))
			{
				if(prevSize)
					prevSize.next = nullptr;
				else
					type = arrayType.type;

				return new SynNew(start, ctx.Previous(), type, arguments, count, constructor);
			}
		}

		if(ctx.Consume(LexemeType.lex_oparen))
		{
			arguments = ParseCallArguments(ctx);

			CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after function argument list");
		}

		if(ctx.Consume(LexemeType.lex_ofigure))
		{
			constructor = ParseExpressions(ctx);

			CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' not found after custom constructor body");
		}

		return new SynNew(start, ctx.Previous(), type, arguments, nullptr, constructor);
	}

	return nullptr;
}

SynCallArgument ref ParseCallArgument(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.At(LexemeType.lex_identifier))
	{
		InplaceStr name = ctx.Consume();
		SynIdentifier ref nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		if(ctx.Consume(LexemeType.lex_colon))
		{
			SynBase ref value = ParseAssignment(ctx);

			if(!value)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after ':' in function argument list");

				value = new SynError(ctx.Current(), ctx.Current());
			}

			return new SynCallArgument(start, ctx.Previous(), nameIdentifier, value);
		}
		else
		{
			// Backtrack
			ctx.currentLexeme = start;
		}
	}

	if(SynBase ref value = ParseAssignment(ctx))
	{
		return new SynCallArgument(start, ctx.Previous(), nullptr, value);
	}

	return nullptr;
}

RefList<SynCallArgument> ParseCallArguments(ParseContext ref ctx)
{
	RefList<SynCallArgument> arguments;

	if(SynCallArgument ref argument = ParseCallArgument(ctx))
	{
		arguments.push_back(argument);

		bool namedCall = argument.name != nullptr;

		while(ctx.Consume(LexemeType.lex_comma))
		{
			argument = ParseCallArgument(ctx);

			if(!argument)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after ',' in function argument list");

				arguments.push_back(new SynCallArgument(ctx.Current(), ctx.Current(), nullptr, new SynError(ctx.Current(), ctx.Current())));
				break;
			}

			if(namedCall && !argument.name)
			{
				Report(ctx, ctx.Current(), "ERROR: function argument name expected after ','");

				break;
			}

			namedCall |= argument.name != nullptr;

			arguments.push_back(argument);
		}
	}

	return arguments;
}

SynBase ref ParsePostExpressions(ParseContext ref ctx, SynBase ref node)
{
	while(ctx.At(LexemeType.lex_point) || ctx.At(LexemeType.lex_obracket) || ctx.At(LexemeType.lex_oparen) || ctx.At(LexemeType.lex_with))
	{
		LexemeRef pos = ctx.currentLexeme;

		if(ctx.Consume(LexemeType.lex_point))
		{
			if(ctx.At(LexemeType.lex_return) || CheckAt(ctx, LexemeType.lex_identifier, "ERROR: member name expected after '.'"))
			{
				InplaceStr member = ctx.Consume();
				SynIdentifier ref memberIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), member);

				node = new SynMemberAccess(node.begin, ctx.Previous(), node, memberIdentifier);
			}
			else
			{
				node = new SynMemberAccess(ctx.Previous(), ctx.Previous(), node, nullptr);
			}
		}
		else if(ctx.Consume(LexemeType.lex_obracket))
		{
			RefList<SynCallArgument> arguments = ParseCallArguments(ctx);

			CheckConsume(ctx, LexemeType.lex_cbracket, "ERROR: ']' not found after expression");

			node = new SynArrayIndex(pos, ctx.Previous(), node, arguments);
		}
		else if(ctx.Consume(LexemeType.lex_with))
		{
			RefList<SynBase> aliases;

			CheckConsume(ctx, LexemeType.lex_less, "ERROR: '<' not found before explicit generic type alias list");

			bool shrBorrow = false;

			do
			{
				SynBase ref type = ParseType(ctx, &shrBorrow);

				if(!type)
				{
					if(aliases.empty())
						Report(ctx, ctx.Current(), "ERROR: type name is expected after 'with'");
					else
						Report(ctx, ctx.Current(), "ERROR: type name is expected after ','");

					break;
				}

				aliases.push_back(type);
			}
			while(ctx.Consume(LexemeType.lex_comma));

			CheckConsume(ctx, shrBorrow ? LexemeType.lex_shr : LexemeType.lex_greater, "ERROR: '>' not found after explicit generic type alias list");

			CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: '(' is expected at this point");

			RefList<SynCallArgument> arguments = ParseCallArguments(ctx);

			CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after function argument list");

			node = new SynFunctionCall(pos, ctx.Previous(), node, aliases, arguments);
		}
		else if(ctx.Consume(LexemeType.lex_oparen))
		{
			RefList<SynBase> aliases;
			RefList<SynCallArgument> arguments = ParseCallArguments(ctx);

			CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after function argument list");

			node = new SynFunctionCall(pos, ctx.Previous(), node, aliases, arguments);
		}
		else
		{
			Stop(ctx, ctx.Current(), "ERROR: not implemented");
		}
	}

	LexemeRef pos = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_inc))
		node = new SynPostModify(pos, ctx.Previous(), node, true);
	else if(ctx.Consume(LexemeType.lex_dec))
		node = new SynPostModify(pos, ctx.Previous(), node, false);

	return node;
}

SynBase ref ParseComplexTerminal(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_mul))
	{
		SynBase ref node = ParseTerminal(ctx);

		if(!node)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after '*'");

			node = new SynError(ctx.Current(), ctx.Current());
		}

		return new SynDereference(start, ctx.Previous(), node);
	}

	SynBase ref node = nullptr;

	if(ctx.Consume(LexemeType.lex_oparen))
	{
		int groupLimit = 256;

		if(ctx.expressionGroupDepth >= groupLimit)
			Stop(ctx, ctx.Current(), "ERROR: reached nested '(' limit of %d", groupLimit);

		ctx.expressionGroupDepth++;

		node = ParseAssignment(ctx);

		ctx.expressionGroupDepth--;

		if(!node)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after '('");

			node = new SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: closing ')' not found after '('");
	}

	if(!node)
		node = ParseString(ctx);

	if(!node)
		node = ParseArray(ctx);

	if(!node)
		node = ParseType(ctx);

	if(!node && ctx.At(LexemeType.lex_identifier))
	{
		InplaceStr value = ctx.Consume();

		node = new SynIdentifier(start, ctx.Previous(), value);
	}

	if(!node && ctx.Consume(LexemeType.lex_at))
	{
		bool isOperator = (ctx.Peek() >= LexemeType.lex_add && ctx.Peek() <= LexemeType.lex_in) || (ctx.Peek() >= LexemeType.lex_set && ctx.Peek() <= LexemeType.lex_xorset) || ctx.Peek() == LexemeType.lex_bitnot || ctx.Peek() == LexemeType.lex_lognot;

		if(!isOperator)
		{
			Report(ctx, ctx.Current(), "ERROR: name expected after '@'");

			return new SynError(ctx.Current(), ctx.Current());
		}

		InplaceStr value = ctx.Consume();

		node = new SynIdentifier(start, ctx.Previous(), value);
	}

	if(!node)
		return nullptr;

	return ParsePostExpressions(ctx, node);
}

SynBase ref ParseTerminal(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_true))
		return new SynBool(start, ctx.Previous(), true);

	if(ctx.Consume(LexemeType.lex_false))
		return new SynBool(start, ctx.Previous(), false);

	if(ctx.Consume(LexemeType.lex_nullptr))
		return new SynNullptr(start, ctx.Previous());

	if(ctx.Consume(LexemeType.lex_bitand))
	{
		SynBase ref node = ParseComplexTerminal(ctx);

		if(!node)
		{
			Report(ctx, ctx.Current(), "ERROR: variable not found after '&'");

			node = new SynError(start, ctx.Previous());
		}

		return new SynGetAddress(start, ctx.Previous(), node);
	}

	if(ctx.At(LexemeType.lex_semiquotedchar))
	{
		InplaceStr str = ctx.Consume();

		if(str.length() == 1 || str[str.length() - 1] != '\'')
			Stop(ctx, start, "ERROR: unclosed character constant");
		else if((str.length() > 3 && str[1] != '\\') || str.length() > 4)
			Stop(ctx, start, "ERROR: only one character can be inside single quotes");
		else if(str.length() < 3)
			Stop(ctx, start, "ERROR: empty character constant");

		return new SynCharacter(start, ctx.Previous(), str);
	}

	if(SynUnaryOpType type = GetUnaryOpType(ctx.Peek()))
	{
		InplaceStr name = ctx.Consume();

		SynBase ref value = ParseTerminal(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after '%.*s'", name.length(), name.begin);

			value = new SynError(start, ctx.Previous());
		}

		return new SynUnaryOp(start, ctx.Previous(), type, value);
	}

	if(ctx.Consume(LexemeType.lex_dec))
	{
		SynBase ref value = ParseTerminal(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: variable not found after '--'");

			value = new SynError(start, ctx.Previous());
		}

		return new SynPreModify(start, ctx.Previous(), value, false);
	}

	if(ctx.Consume(LexemeType.lex_inc))
	{
		SynBase ref value = ParseTerminal(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: variable not found after '++'");

			value = new SynError(start, ctx.Previous());
		}

		return new SynPreModify(start, ctx.Previous(), value, true);
	}

	if(SynNumber ref node = ParseNumber(ctx))
		return node;

	if(SynNew ref node = ParseNew(ctx))
		return node;

	if(SynBase ref node = ParseSizeof(ctx))
		return node;

	if(SynBase ref node = ParseFunctionDefinition(ctx))
		return node;

	if(SynBase ref node = ParseShortFunctionDefinition(ctx))
		return node;

	if(SynBase ref node = ParseComplexTerminal(ctx))
		return node;

	return nullptr;
}

SynBase ref ParseArithmetic(ParseContext ref ctx)
{
	SynBase ref lhs = ParseTerminal(ctx);

	if(!lhs)
		return nullptr;

	int startSize = ctx.binaryOpStack.size();

	SynBinaryOpType binaryOp = GetBinaryOpType(ctx.Peek());

	while(binaryOp)
	{
		LexemeRef start = ctx.currentLexeme;

		ctx.Skip();

		while(ctx.binaryOpStack.size() > startSize && GetBinaryOpPrecedence(ctx.binaryOpStack.back().type) <= GetBinaryOpPrecedence(binaryOp))
		{
			SynBinaryOpElement lastOp = *ctx.binaryOpStack.back();

			lhs = new SynBinaryOp(lastOp.begin, lastOp.end, lastOp.type, lastOp.value, lhs);

			ctx.binaryOpStack.pop_back();
		}

		ctx.binaryOpStack.push_back(SynBinaryOpElement(start, ctx.Previous(), binaryOp, lhs));

		lhs = ParseTerminal(ctx);

		if(!lhs)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after binary operation");

			lhs = new SynError(start, ctx.Previous());
		}

		binaryOp = GetBinaryOpType(ctx.Peek());
	}

	while(ctx.binaryOpStack.size() > startSize)
	{
		SynBinaryOpElement lastOp = *ctx.binaryOpStack.back();

		lhs = new SynBinaryOp(lastOp.begin, lastOp.end, lastOp.type, lastOp.value, lhs);

		ctx.binaryOpStack.pop_back();
	}

	return lhs;
}

SynBase ref ParseTernaryExpr(ParseContext ref ctx)
{
	if(SynBase ref value = ParseArithmetic(ctx))
	{
		LexemeRef pos = ctx.currentLexeme;

		while(ctx.Consume(LexemeType.lex_questionmark))
		{
			SynBase ref trueBlock = ParseAssignment(ctx);

			if(!trueBlock)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '?'");

				trueBlock = new SynError(pos, ctx.Previous());
			}

			CheckConsume(ctx, LexemeType.lex_colon, "ERROR: ':' not found after expression in ternary operator");

			SynBase ref falseBlock = ParseAssignment(ctx);

			if(!falseBlock)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after ':'");

				falseBlock = new SynError(pos, ctx.Previous());
			}

			value = new SynConditional(pos, ctx.Previous(), value, trueBlock, falseBlock);
		}

		return value;
	}

	return nullptr;
}

SynBase ref ParseAssignment(ParseContext ref ctx)
{
	if(SynBase ref lhs = ParseTernaryExpr(ctx))
	{
		LexemeRef pos = ctx.currentLexeme;

		if(ctx.Consume(LexemeType.lex_set))
		{
			SynBase ref rhs = ParseAssignment(ctx);

			if(!rhs)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

				rhs = new SynError(ctx.Current(), ctx.Current());
			}

			return new SynAssignment(pos, ctx.Previous(), lhs, rhs);
		}
		else if(SynModifyAssignType modifyType = GetModifyAssignType(ctx.Peek()))
		{
			InplaceStr name = ctx.Consume();

			SynBase ref rhs = ParseAssignment(ctx);

			if(!rhs)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '%.*s' operator", name.length(), name.begin);

				rhs = new SynError(ctx.Current(), ctx.Current());
			}

			return new SynModifyAssignment(pos, ctx.Previous(), modifyType, lhs, rhs);
		}

		return lhs;
	}

	return nullptr;
}

SynClassElements ref ParseClassElements(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	RefList<SynTypedef> typedefs;
	RefList<SynFunctionDefinition> functions;
	RefList<SynAccessor> accessors;
	RefList<SynVariableDefinitions> members;
	RefList<SynConstantSet> constantSets;
	RefList<SynClassStaticIf> staticIfs;

	for(;1;)
	{
		if(SynTypedef ref node = ParseTypedef(ctx))
		{
			typedefs.push_back(node);
		}
		else if(SynClassStaticIf ref node = ParseClassStaticIf(ctx, false))
		{
			staticIfs.push_back(node);
		}
		else if(SynFunctionDefinition ref node = ParseFunctionDefinition(ctx))
		{
			functions.push_back(node);
		}
		else if(SynAccessor ref node = ParseAccessorDefinition(ctx))
		{
			accessors.push_back(node);
		}
		else if(SynVariableDefinitions ref node = ParseVariableDefinitions(ctx, true))
		{
			members.push_back(node);
		}
		else if(SynConstantSet ref node = ParseConstantSet(ctx))
		{
			constantSets.push_back(node);
		}
		else
		{
			break;
		}
	}

	LexemeRef end = start == ctx.currentLexeme ? start : ctx.Previous();

	return new SynClassElements(start, end, typedefs, functions, accessors, members, constantSets, staticIfs);
}

SynBase ref ParseClassDefinition(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	SynAlign ref alignment = ParseAlign(ctx);

        bool isStruct = ctx.Consume(LexemeType.lex_struct);

	if(isStruct || ctx.Consume(LexemeType.lex_class))
	{
		SynIdentifier ref nameIdentifier = nullptr;

		if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: class name expected"))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else
		{
			nameIdentifier = new SynIdentifier(InplaceStr());
		}

		if(ctx.Consume(LexemeType.lex_semicolon))
		{
			if(alignment)
				Report(ctx, ctx.Current(), "ERROR: can't specify alignment of a class prototype");

			return new SynClassPrototype(start, ctx.Previous(), nameIdentifier);
		}

		RefList<SynIdentifier> aliases;

		if(ctx.Consume(LexemeType.lex_less))
		{
			if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: generic type alias required after '<'"))
			{
				InplaceStr alias = ctx.Consume();

				aliases.push_back(new SynIdentifier(ctx.Previous(), ctx.Previous(), alias));

				while(ctx.Consume(LexemeType.lex_comma))
				{
					if(!CheckAt(ctx, LexemeType.lex_identifier, "ERROR: generic type alias required after ','"))
						break;

					alias = ctx.Consume();

					aliases.push_back(new SynIdentifier(ctx.Previous(), ctx.Previous(), alias));
				}
			}

			CheckConsume(ctx, LexemeType.lex_greater, "ERROR: '>' expected after generic type alias list");
		}

		bool isExtendable = ctx.Consume(LexemeType.lex_extendable);

		SynBase ref baseClass = nullptr;

		if(ctx.Consume(LexemeType.lex_colon))
		{
			baseClass = ParseType(ctx);

			if(!baseClass)
				Report(ctx, ctx.Current(), "ERROR: base type name is expected at this point");
		}

		CheckConsume(ctx, LexemeType.lex_ofigure, "ERROR: '{' not found after class name");

		/*TRACE_SCOPE("parse", "ParseClassBody");

		if(nameIdentifier)
			TRACE_LABEL2(nameIdentifier.name.begin, nameIdentifier.name.end);*/

		SynClassElements ref elements = ParseClassElements(ctx);

		CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' not found after class definition");

		return new SynClassDefinition(start, ctx.Previous(), alignment, nameIdentifier, aliases, isExtendable, isStruct, baseClass, elements);
	}

	// Backtrack
	ctx.currentLexeme = start;

	return nullptr;
}

SynEnumDefinition ref ParseEnumDefinition(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_enum))
	{
		SynIdentifier ref nameIdentifier = nullptr;

		if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: enum name expected"))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else
		{
			nameIdentifier = new SynIdentifier(InplaceStr());
		}

		CheckConsume(ctx, LexemeType.lex_ofigure, "ERROR: '{' not found after enum name");

		RefList<SynConstant> values;

		do
		{
			if(values.empty())
			{
				if(!CheckAt(ctx, LexemeType.lex_identifier, "ERROR: enumeration name expected after '{'"))
					break;
			}
			else
			{
				if(!CheckAt(ctx, LexemeType.lex_identifier, "ERROR: enumeration name expected after ','"))
					break;
			}

			LexemeRef pos = ctx.currentLexeme;

			InplaceStr name = ctx.Consume();
			SynIdentifier ref nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);

			SynBase ref value = nullptr;

			if(ctx.Consume(LexemeType.lex_set))
			{
				value = ParseTernaryExpr(ctx);

				if(!value)
				{
					Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

					value = new SynError(start, ctx.Previous());
				}
			}

			values.push_back(new SynConstant(pos, ctx.Previous(), nameIdentifier, value));
		}
		while(ctx.Consume(LexemeType.lex_comma));

		CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' not found after enum definition");

		return new SynEnumDefinition(start, ctx.Previous(), nameIdentifier, values);
	}

	return nullptr;
}

SynNamespaceDefinition ref ParseNamespaceDefinition(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_namespace))
	{
		RefList<SynIdentifier> path;

		if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: namespace name required"))
		{
			InplaceStr value = ctx.Consume();
			path.push_back(new SynIdentifier(ctx.Previous(), ctx.Previous(), value));
		}

		while(ctx.Consume(LexemeType.lex_dblcolon))
		{
			if(!CheckAt(ctx, LexemeType.lex_identifier, "ERROR: namespace name required after '::'"))
				break;

			InplaceStr value = ctx.Consume();
			path.push_back(new SynIdentifier(ctx.Previous(), ctx.Previous(), value));
		}

		CheckConsume(ctx, LexemeType.lex_ofigure, "ERROR: '{' not found after namespace name");

		SynNamespaceElement ref currentNamespace = ctx.currentNamespace;

		for(SynIdentifier ref el = path.head; el; el = (SynIdentifier ref)(el.next))
			ctx.PushNamespace(el);

		RefList<SynBase> code = ParseExpressions(ctx);

		ctx.currentNamespace = currentNamespace;

		CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' not found after namespace body");

		return new SynNamespaceDefinition(start, ctx.Previous(), path, code);
	}

	return nullptr;
}

SynReturn ref ParseReturn(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_return))
	{
		// Optional
		SynBase ref value = ParseAssignment(ctx);

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: return statement must be followed by an expression or ';'");

		return new SynReturn(start, ctx.Previous(), value);
	}

	return nullptr;
}

SynYield ref ParseYield(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_yield))
	{
		// Optional
		SynBase ref value = ParseAssignment(ctx);

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: yield statement must be followed by an expression or ';'");

		return new SynYield(start, ctx.Previous(), value);
	}

	return nullptr;
}

SynBreak ref ParseBreak(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_break))
	{
		// Optional
		SynNumber ref node = ParseNumber(ctx);

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: break statement must be followed by ';' or a constant");

		return new SynBreak(start, ctx.Previous(), node);
	}

	return nullptr;
}

SynContinue ref ParseContinue(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_continue))
	{
		// Optional
		SynNumber ref node = ParseNumber(ctx);

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: continue statement must be followed by ';' or a constant");

		return new SynContinue(start, ctx.Previous(), node);
	}

	return nullptr;
}

SynLabel ref ParseLabel(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_continue))
	{
		// Optional
		SynNumber ref node = ParseNumber(ctx);

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: continue statement must be followed by ';' or a constant");

		return new SynLabel(start, ctx.Previous(), node);
	}

	return nullptr;
}

SynGoto ref ParseGoto(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_continue))
	{
		// Optional
		SynNumber ref node = ParseNumber(ctx);

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: continue statement must be followed by ';' or a constant");

		return new SynGoto(start, ctx.Previous(), node);
	}

	return nullptr;
}

SynTypedef ref ParseTypedef(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_typedef))
	{
		SynBase ref type = ParseType(ctx);

		if(!type)
		{
			Report(ctx, ctx.Current(), "ERROR: typename expected after typedef");

			type = new SynError(start, ctx.Previous());
		}

		SynIdentifier ref aliasIdentifier = nullptr;

		if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: alias name expected after typename in typedef expression"))
		{
			InplaceStr alias = ctx.Consume();
			aliasIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), alias);
		}
		else
		{
			aliasIdentifier = new SynIdentifier(InplaceStr());
		}

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after typedef");

		return new SynTypedef(start, ctx.Previous(), type, aliasIdentifier);
	}

	return nullptr;
}

SynBlock ref ParseBlock(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_ofigure))
	{
		int blockLimit = 256;

		if(ctx.statementBlockDepth >= blockLimit)
			Stop(ctx, ctx.Current(), "ERROR: reached nested '{' limit of %d", blockLimit);

		ctx.statementBlockDepth++;

		RefList<SynBase> expressions = ParseExpressions(ctx);

		ctx.statementBlockDepth--;

		CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: closing '}' not found");

		return new SynBlock(start, ctx.Previous(), expressions);
	}

	return nullptr;
}

SynIfElse ref ParseIfElse(ParseContext ref ctx, bool forceStaticIf)
{
	LexemeRef start = ctx.currentLexeme;

	bool staticIf = forceStaticIf || ctx.Consume(LexemeType.lex_at);

	if(ctx.Consume(LexemeType.lex_if))
	{
		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: '(' not found after 'if'");

		LexemeRef conditionPos = ctx.currentLexeme;

		SynBase ref condition = nullptr;

		if(!staticIf)
		{
			if(SynBase ref type = ParseType(ctx))
			{
				RefList<SynVariableDefinition> definitions;

				if(SynVariableDefinition ref definition = ParseVariableDefinition(ctx))
				{
					definitions.push_back(definition);

					condition = new SynVariableDefinitions(start, ctx.Previous(), nullptr, type, definitions);
				}
				else
				{
					ctx.currentLexeme = conditionPos;
				}
			}
		}

		if(!condition)
			condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: condition not found in 'if' statement");

			condition = new SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: closing ')' not found after 'if' condition");

		SynBase ref trueBlock = ParseExpression(ctx);

		if(!trueBlock)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after 'if'");

			trueBlock = new SynError(ctx.Current(), ctx.Current());
		}

		SynBase ref falseBlock = nullptr;

		if(staticIf && ctx.At(LexemeType.lex_semicolon))
			ctx.Skip();

		if(ctx.Consume(LexemeType.lex_else))
		{
			if(ctx.At(LexemeType.lex_if))
				falseBlock = ParseIfElse(ctx, staticIf);
			else
				falseBlock = ParseExpression(ctx);

			if(!falseBlock)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after 'else'");

				falseBlock = new SynError(ctx.Current(), ctx.Current());
			}

			if(staticIf && ctx.At(LexemeType.lex_semicolon))
				ctx.Skip();
		}

		return new SynIfElse(start, ctx.Previous(), staticIf, condition, trueBlock, falseBlock);
	}

	if(staticIf)
	{
		// Backtrack
		ctx.currentLexeme = start;
	}

	return nullptr;
}

SynForEachIterator ref ParseForEachIterator(ParseContext ref ctx, bool isFirst)
{
	LexemeRef start = ctx.currentLexeme;

	// Optional type
	SynBase ref type = ParseType(ctx);

	// Must be followed by a type
	if(!ctx.At(LexemeType.lex_identifier))
	{
		// Backtrack
		ctx.currentLexeme = start;

		type = nullptr;
	}

	if(ctx.At(LexemeType.lex_identifier))
	{
		InplaceStr name = ctx.Consume();
		SynIdentifier ref nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		if(isFirst && !ctx.At(LexemeType.lex_in))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return nullptr;
		}

		CheckConsume(ctx, LexemeType.lex_in, "ERROR: 'in' expected after variable name");

		SynBase ref value = ParseTernaryExpr(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression expected after 'in'");

			value = new SynError(ctx.Current(), ctx.Current());
		}

		return new SynForEachIterator(start, ctx.Previous(), type, nameIdentifier, value);
	}

	return nullptr;
}

SynForEach ref ParseForEach(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_for))
	{
		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: '(' not found after 'for'");

		RefList<SynForEachIterator> iterators;

		SynForEachIterator ref iterator = ParseForEachIterator(ctx, true);

		if(!iterator)
		{
			// Backtrack
			ctx.currentLexeme = start;

			return nullptr;
		}

		iterators.push_back(iterator);

		while(ctx.Consume(LexemeType.lex_comma))
		{
			iterator = ParseForEachIterator(ctx, false);

			if(!iterator)
			{
				Report(ctx, ctx.Current(), "ERROR: variable name or type expected before 'in'");

				break;
			}

			iterators.push_back(iterator);
		}

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after 'for' statement");

		SynBase ref body = ParseExpression(ctx);

		if(!body)
		{
			Report(ctx, ctx.Current(), "ERROR: body not found after 'for' header");

			body = new SynError(ctx.Current(), ctx.Current());
		}

		return new SynForEach(start, ctx.Previous(), iterators, body);
	}

	return nullptr;
}

SynFor ref ParseFor(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_for))
	{
		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: '(' not found after 'for'");

		SynBase ref initializer = nullptr;

		if(ctx.At(LexemeType.lex_ofigure))
		{
			initializer = ParseBlock(ctx);

			CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after initializer in 'for'");
		}
		else if(SynBase ref node = ParseVariableDefinitions(ctx, false))
		{
			initializer = node;
		}
		else if(SynBase ref node = ParseAssignment(ctx))
		{
			initializer = node;

			CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after initializer in 'for'");
		}
		else if(!ctx.Consume(LexemeType.lex_semicolon))
		{
			Report(ctx, ctx.Current(), "ERROR: ';' not found after initializer in 'for'");
		}

		SynBase ref condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: condition not found in 'for' statement");

			condition = new SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after condition in 'for'");

		SynBase ref increment = nullptr;

		if(ctx.At(LexemeType.lex_ofigure))
			increment = ParseBlock(ctx);
		else if(SynBase ref node = ParseAssignment(ctx))
			increment = node;

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after 'for' statement");

		SynBase ref body = ParseExpression(ctx);

		if(!body)
		{
			Report(ctx, ctx.Current(), "ERROR: body not found after 'for' header");

			body = new SynError(ctx.Current(), ctx.Current());
		}

		return new SynFor(start, ctx.Previous(), initializer, condition, increment, body);
	}

	return nullptr;
}

SynWhile ref ParseWhile(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_while))
	{
		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: '(' not found after 'while'");

		SynBase ref condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: expression expected after 'while('");

			condition = new SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: closing ')' not found after expression in 'while' statement");

		SynBase ref body = ParseExpression(ctx);

		if(!body && !ctx.Consume(LexemeType.lex_semicolon))
			Report(ctx, ctx.Current(), "ERROR: body not found after 'while' header");

		return new SynWhile(start, ctx.Previous(), condition, body);
	}

	return nullptr;
}

SynDoWhile ref ParseDoWhile(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_do))
	{
		RefList<SynBase> expressions;

		if(ctx.Consume(LexemeType.lex_ofigure))
		{
			expressions = ParseExpressions(ctx);

			CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: closing '}' not found");
		}
		else if(SynBase ref body = ParseExpression(ctx))
		{
			expressions.push_back(body);
		}
		else
		{
			Report(ctx, ctx.Current(), "ERROR: expression expected after 'do'");

			expressions.push_back(new SynError(ctx.Current(), ctx.Current()));
		}

		CheckConsume(ctx, LexemeType.lex_while, "ERROR: 'while' expected after 'do' statement");

		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: '(' not found after 'while'");

		SynBase ref condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: expression expected after 'while('");

			condition = new SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: closing ')' not found after expression in 'while' statement");

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: while(...) should be followed by ';'");

		return new SynDoWhile(start, ctx.Previous(), expressions, condition);
	}

	return nullptr;
}

SynSwitch ref ParseSwitch(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_switch))
	{
		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: '(' not found after 'switch'");

		SynBase ref condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after 'switch('");

			condition = new SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: closing ')' not found after expression in 'switch' statement");

		CheckConsume(ctx, LexemeType.lex_ofigure, "ERROR: '{' not found after 'switch(...)'");

		bool hadDefautltCase = false;

		RefList<SynSwitchCase> cases;

		while(ctx.At(LexemeType.lex_case) || ctx.At(LexemeType.lex_default))
		{
			LexemeRef pos = ctx.currentLexeme;

			SynBase ref value = nullptr;

			if(ctx.Consume(LexemeType.lex_case))
			{
				if(hadDefautltCase)
					Report(ctx, ctx.Current(), "ERROR: default switch case can't be followed by more cases");

				value = ParseAssignment(ctx);

				if(!value)
				{
					Report(ctx, ctx.Current(), "ERROR: expression expected after 'case'");

					value = new SynError(ctx.Current(), ctx.Current());
				}
			}
			else
			{
				if(hadDefautltCase)
					Report(ctx, ctx.Current(), "ERROR: default switch case is already defined");

				ctx.Skip();

				hadDefautltCase = true;
			}

			CheckConsume(ctx, LexemeType.lex_colon, "ERROR: ':' expected");

			RefList<SynBase> expressions = ParseExpressions(ctx);

			cases.push_back(new SynSwitchCase(pos, ctx.Previous(), value, expressions));
		}

		CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' not found after 'switch' statement");

		return new SynSwitch(start, ctx.Previous(), condition, cases);
	}

	return nullptr;
}

SynVariableDefinition ref ParseVariableDefinition(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.At(LexemeType.lex_identifier))
	{
		InplaceStr name = ctx.Consume();
		SynIdentifier ref nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		if(name.length() >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			Stop(ctx, ctx.Current(), "ERROR: variable name length is limited to %d symbols", NULLC_MAX_VARIABLE_NAME_LENGTH);

		SynBase ref initializer = nullptr;

		if(ctx.Consume(LexemeType.lex_set))
		{
			initializer = ParseAssignment(ctx);

			if(!initializer)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

				initializer = new SynError(ctx.Current(), ctx.Current());
			}
		}

		return new SynVariableDefinition(start, ctx.Previous(), nameIdentifier, initializer);
	}

	return nullptr;
}

SynVariableDefinitions ref ParseVariableDefinitions(ParseContext ref ctx, bool classMembers)
{
	LexemeRef start = ctx.currentLexeme;

	SynAlign ref alignment = ParseAlign(ctx);

	if(SynBase ref type = ParseType(ctx))
	{
		RefList<SynVariableDefinition> definitions;

		SynVariableDefinition ref definition = ParseVariableDefinition(ctx);

		if(!definition)
		{
			if(alignment)
				Report(ctx, ctx.Current(), "ERROR: variable or class definition is expected after alignment specifier");

			// Backtrack
			ctx.currentLexeme = start;

			return nullptr;
		}

		definitions.push_back(definition);

		while(ctx.Consume(LexemeType.lex_comma))
		{
			definition = ParseVariableDefinition(ctx);

			if(!definition)
			{
				if(classMembers)
					Report(ctx, ctx.Current(), "ERROR: member name expected after ','");
				else
					Report(ctx, ctx.Current(), "ERROR: next variable definition excepted after ','");

				break;
			}

			definitions.push_back(definition);
		}

		if(classMembers)
		{
			CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after class member list");
		}
		else if(!ctx.Consume(LexemeType.lex_semicolon))
		{
			if(ctx.Peek() == LexemeType.lex_obracket)
				CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: array size must be specified after type name");
			else
				CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after variable definition");
		}

		return new SynVariableDefinitions(start, ctx.Previous(), alignment, type, definitions);
	}

	// Backtrack
	ctx.currentLexeme = start;

	return nullptr;
}

SynAccessor ref ParseAccessorDefinition(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(SynBase ref type = ParseType(ctx))
	{
		if(!CheckAt(ctx, LexemeType.lex_identifier, "ERROR: class member name expected after type"))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return nullptr;
		}

		InplaceStr name = ctx.Consume();
		SynIdentifier ref nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		if(!ctx.Consume(LexemeType.lex_ofigure))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return nullptr;
		}

		CheckConsume(ctx, "get", "ERROR: 'get' is expected after '{'");

		SynBase ref getBlock = ParseBlock(ctx);

		if(!getBlock)
		{
			Report(ctx, ctx.Current(), "ERROR: function body expected after 'get'");

			getBlock = new SynError(ctx.Current(), ctx.Current());
		}

		SynBase ref setBlock = nullptr;
		SynIdentifier ref setNameIdentifier = nullptr;

		if(ctx.Consume("set"))
		{
			if(ctx.Consume(LexemeType.lex_oparen))
			{
				if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: r-value name not found after '('"))
				{
					InplaceStr setName = ctx.Consume();
					setNameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), setName);
				}

				CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after r-value");
			}

			setBlock = ParseBlock(ctx);

			if(!setBlock)
			{
				Report(ctx, ctx.Current(), "ERROR: function body expected after 'set'");

				setBlock = new SynError(ctx.Current(), ctx.Current());
			}
		}

		CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' is expected after property");

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after class member list");

		return new SynAccessor(start, ctx.Previous(), type, nameIdentifier, getBlock, setBlock, setNameIdentifier);
	}

	return nullptr;
}

SynConstantSet ref ParseConstantSet(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_const))
	{
		SynBase ref type = ParseType(ctx);

		if(!type)
		{
			Report(ctx, ctx.Current(), "ERROR: type name expected after const");

			type = new SynError(ctx.Current(), ctx.Current());
		}

		RefList<SynConstant> constantSet;

		LexemeRef pos = ctx.currentLexeme;

		SynIdentifier ref nameIdentifier = nullptr;

		if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: constant name expected after type"))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else
		{
			pos = ctx.Previous();

			nameIdentifier = new SynIdentifier(InplaceStr());
		}

		CheckConsume(ctx, LexemeType.lex_set, "ERROR: '=' not found after constant name");

		SynBase ref value = ParseTernaryExpr(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

			value = new SynError(ctx.Current(), ctx.Current());
		}

		constantSet.push_back(new SynConstant(pos, ctx.Previous(), nameIdentifier, value));

		while(ctx.Consume(LexemeType.lex_comma))
		{
			pos = ctx.currentLexeme;

			if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: constant name expected after ','"))
			{
				InplaceStr name = ctx.Consume();
				nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else
			{
				pos = ctx.Previous();

				nameIdentifier = new SynIdentifier(InplaceStr());
			}

			value = nullptr;

			if(ctx.Consume(LexemeType.lex_set))
			{
				value = ParseTernaryExpr(ctx);

				if(!value)
				{
					Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

					value = new SynError(ctx.Current(), ctx.Current());
				}
			}

			constantSet.push_back(new SynConstant(pos, ctx.Previous(), nameIdentifier, value));
		}

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after constants");

		return new SynConstantSet(start, ctx.Previous(), type, constantSet);
	}

	return nullptr;
}

SynClassStaticIf ref ParseClassStaticIf(ParseContext ref ctx, bool nested)
{
	LexemeRef start = ctx.currentLexeme;

	if(nested || ctx.Consume(LexemeType.lex_at))
	{
		if(!ctx.Consume(LexemeType.lex_if))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return nullptr;
		}

		CheckConsume(ctx, LexemeType.lex_oparen, "ERROR: '(' not found after 'if'");

		SynBase ref condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: condition not found in 'if' statement");

			condition = new SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: closing ')' not found after 'if' condition");

		bool hasBlock = ctx.Consume(LexemeType.lex_ofigure);

		SynClassElements ref trueBlock = ParseClassElements(ctx);

		if(hasBlock)
			CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: closing '}' not found");

		SynClassElements ref falseBlock = nullptr;

		if(ctx.Consume(LexemeType.lex_else))
		{
			if(ctx.At(LexemeType.lex_if))
			{
				LexemeRef pos = ctx.currentLexeme;

				RefList<SynClassStaticIf> staticIfs;

				staticIfs.push_back(ParseClassStaticIf(ctx, true));

				falseBlock = new SynClassElements(pos, ctx.Previous(), RefList<SynTypedef>(), RefList<SynFunctionDefinition>(), RefList<SynAccessor>(), RefList<SynVariableDefinitions>(), RefList<SynConstantSet>(), staticIfs);
			}
			else
			{
				hasBlock = ctx.Consume(LexemeType.lex_ofigure);

				falseBlock = ParseClassElements(ctx);

				if(hasBlock)
					CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: closing '}' not found");
			}
		}

		return new SynClassStaticIf(start, ctx.Previous(), condition, trueBlock, falseBlock);
	}

	return nullptr;
}

SynFunctionArgument ref ParseFunctionArgument(ParseContext ref ctx, bool lastExplicit, SynBase ref lastType)
{
	LexemeRef start = ctx.currentLexeme;

	bool isExplicit = ctx.Consume("explicit");

	if(SynBase ref type = ParseType(ctx))
	{
		if(!ctx.At(LexemeType.lex_identifier) && lastType)
		{
			if(isExplicit)
				Stop(ctx, ctx.Current(), "ERROR: variable name not found after type in function variable list");

			// Backtrack
			ctx.currentLexeme = start;

			isExplicit = lastExplicit;
			type = lastType;
		}

		SynIdentifier ref nameIdentifier = nullptr;

		if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: variable name not found after type in function variable list"))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else
		{
			nameIdentifier = new SynIdentifier(InplaceStr());
		}

		SynBase ref initializer = nullptr;

		if(ctx.Consume(LexemeType.lex_set))
		{
			initializer = ParseTernaryExpr(ctx);

			if(!initializer)
			{
				Report(ctx, ctx.Current(), "ERROR: default argument value not found after '='");

				initializer = new SynError(ctx.Current(), ctx.Current());
			}
		}

		return new SynFunctionArgument(start, start == ctx.currentLexeme ? ctx.Current() : ctx.Previous(), isExplicit, type, nameIdentifier, initializer);
	}

	if(isExplicit)
		Stop(ctx, ctx.Current(), "ERROR: type name not found after 'explicit' specifier");

	return nullptr;
}

RefList<SynFunctionArgument> ParseFunctionArguments(ParseContext ref ctx)
{
	RefList<SynFunctionArgument> arguments;

	if(SynFunctionArgument ref argument = ParseFunctionArgument(ctx, false, nullptr))
	{
		arguments.push_back(argument);

		while(ctx.Consume(LexemeType.lex_comma))
		{
			argument = ParseFunctionArgument(ctx, arguments.tail.isExplicit, arguments.tail.type);

			if(!argument)
			{
				Report(ctx, ctx.Current(), "ERROR: argument name not found after ',' in function argument list");

				break;
			}

			arguments.push_back(argument);
		}
	}

	return arguments;
}

SynFunctionDefinition ref ParseFunctionDefinition(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.nonFunctionDefinitionLocations.find(int(start - ctx.firstLexeme) + 1))
		return nullptr;

	bool isCoroutine = ctx.Consume(LexemeType.lex_coroutine);

	if(SynBase ref returnType = ParseType(ctx))
	{
		// Check if this is a member function
		LexemeRef subLexeme = ctx.currentLexeme;

		SynBase ref parentType = ParseType(ctx);
		bool accessor = false;

		if(parentType)
		{
			if(ctx.Consume(LexemeType.lex_dblcolon))
			{
				accessor = false;
			}
			else if(ctx.Consume(LexemeType.lex_point))
			{
				accessor = true;
			}
			else
			{
				// Backtrack
				ctx.currentLexeme = subLexeme;

				parentType = nullptr;
			}
		}

		bool allowEmptyName = isType with<SynTypeAuto>(returnType);

		bool isOperator = ctx.Consume(LexemeType.lex_operator);

		SynIdentifier ref nameIdentifier = nullptr;

		if(isOperator)
		{
			if(ctx.Consume(LexemeType.lex_obracket))
			{
				CheckConsume(ctx, LexemeType.lex_cbracket, "ERROR: ']' not found after '[' in operator definition");

				InplaceStr name = InplaceStr("[]");
				nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else if(ctx.Consume(LexemeType.lex_oparen))
			{
				CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after '(' in operator definition");

				InplaceStr name = InplaceStr("()");
				nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else if((ctx.Peek() >= LexemeType.lex_add && ctx.Peek() <= LexemeType.lex_in) || (ctx.Peek() >= LexemeType.lex_set && ctx.Peek() <= LexemeType.lex_xorset) || ctx.Peek() == LexemeType.lex_bitnot || ctx.Peek() == LexemeType.lex_lognot)
			{
				InplaceStr name = ctx.Consume();
				nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else
			{
				Stop(ctx, ctx.Current(), "ERROR: invalid operator name");
			}
		}
		else if(ctx.At(LexemeType.lex_identifier))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else if(parentType)
		{
			Stop(ctx, ctx.Current(), "ERROR: function name expected after '::' or '.'");
		}
		else if(isCoroutine && !allowEmptyName)
		{
			Stop(ctx, ctx.Current(), "ERROR: function name not found after return type");
		}

		RefList<SynIdentifier> aliases;

		if(nameIdentifier && ctx.Consume(LexemeType.lex_less))
		{
			do
			{
				if(aliases.empty())
					CheckConsume(ctx, LexemeType.lex_at, "ERROR: '@' is expected before explicit generic type alias");
				else
					CheckConsume(ctx, LexemeType.lex_at, "ERROR: '@' is expected after ',' in explicit generic type alias list");

				if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: explicit generic type alias is expected after '@'"))
				{
					InplaceStr name = ctx.Consume();
					aliases.push_back(new SynIdentifier(ctx.Previous(), ctx.Previous(), name));
				}
			}
			while(ctx.Consume(LexemeType.lex_comma));

			CheckConsume(ctx, LexemeType.lex_greater, "ERROR: '>' not found after explicit generic type alias list");
		}

		if(parentType || isCoroutine || !aliases.empty())
			CheckAt(ctx, LexemeType.lex_oparen, "ERROR: '(' expected after function name");

		if((nameIdentifier == nullptr && !allowEmptyName) || !ctx.Consume(LexemeType.lex_oparen))
		{
			// Backtrack
			ctx.currentLexeme = start;
			ctx.nonFunctionDefinitionLocations.insert(int(start - ctx.firstLexeme) + 1, true);

			return nullptr;
		}

		RefList<SynFunctionArgument> arguments = ParseFunctionArguments(ctx);

		CheckConsume(ctx, LexemeType.lex_cparen, "ERROR: ')' not found after function variable list");

		if(!nameIdentifier)
			nameIdentifier = new SynIdentifier(InplaceStr());

		RefList<SynBase> expressions;

		if(ctx.Consume(LexemeType.lex_semicolon))
			return new SynFunctionDefinition(start, ctx.Previous(), true, isCoroutine, parentType, accessor, returnType, isOperator, nameIdentifier, aliases, arguments, expressions);

		CheckConsume(ctx, LexemeType.lex_ofigure, "ERROR: '{' not found after function header");

		/*TRACE_SCOPE("parse", "ParseFunctionBody");

		if(nameIdentifier)
			TRACE_LABEL2(nameIdentifier.name.begin, nameIdentifier.name.end);*/

		expressions = ParseExpressions(ctx);

		CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' not found after function body");

		return new SynFunctionDefinition(start, ctx.Previous(), false, isCoroutine, parentType, accessor, returnType, isOperator, nameIdentifier, aliases, arguments, expressions);
	}
	else if(isCoroutine)
	{
		Stop(ctx, ctx.Current(), "ERROR: function return type not found after 'coroutine'");
	}

	return nullptr;
}

SynShortFunctionDefinition ref ParseShortFunctionDefinition(ParseContext ref ctx)
{
	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_less))
	{
		RefList<SynShortFunctionArgument> arguments;

		bool isFirst = true;

		do
		{
			if(isFirst && ctx.At(LexemeType.lex_greater))
				break;

			isFirst = false;

			LexemeRef pos = ctx.currentLexeme;

			LexemeRef lexeme = ctx.currentLexeme;

			SynBase ref type = ParseType(ctx);

			if(!ctx.At(LexemeType.lex_identifier))
			{
				// Backtrack
				ctx.currentLexeme = lexeme;

				type = nullptr;
			}

			bool hasName = false;

			if(arguments.empty())
				hasName = CheckAt(ctx, LexemeType.lex_identifier, "ERROR: function argument name not found after '<'");
			else
				hasName = CheckAt(ctx, LexemeType.lex_identifier, "ERROR: function argument name not found after ','");

			SynIdentifier ref nameIdentifier = nullptr;

			if(hasName)
			{
				InplaceStr name = ctx.Consume();
				nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else
			{
				pos = ctx.Previous();

				nameIdentifier = new SynIdentifier(InplaceStr());
			}

			arguments.push_back(new SynShortFunctionArgument(pos, ctx.Previous(), type, nameIdentifier));
		}
		while(ctx.Consume(LexemeType.lex_comma));

		CheckConsume(ctx, LexemeType.lex_greater, "ERROR: '>' expected after short inline function argument list");

		CheckConsume(ctx, LexemeType.lex_ofigure, "ERROR: '{' not found after function header");

		RefList<SynBase> expressions = ParseExpressions(ctx);

		CheckConsume(ctx, LexemeType.lex_cfigure, "ERROR: '}' not found after function body");

		return new SynShortFunctionDefinition(start, ctx.Previous(), arguments, expressions);
	}

	return nullptr;
}

SynBase ref ParseExpression(ParseContext ref ctx)
{
	if(SynBase ref node = ParseClassDefinition(ctx))
		return node;

	if(SynBase ref node = ParseEnumDefinition(ctx))
		return node;

	if(SynBase ref node = ParseNamespaceDefinition(ctx))
		return node;

	if(SynBase ref node = ParseReturn(ctx))
		return node;

	if(SynBase ref node = ParseYield(ctx))
		return node;

	if(SynBase ref node = ParseBreak(ctx))
		return node;

	if(SynBase ref node = ParseContinue(ctx))
		return node;

	if(SynBase ref node = ParseGoto(ctx))
		return node;

	if(SynBase ref node = ParseLabel(ctx))
		return node;

	if(SynBase ref node = ParseTypedef(ctx))
		return node;

	if(SynBase ref node = ParseBlock(ctx))
		return node;

	if(SynBase ref node = ParseIfElse(ctx, false))
		return node;

	if(SynBase ref node = ParseForEach(ctx))
		return node;

	if(SynBase ref node = ParseFor(ctx))
		return node;

	if(SynBase ref node = ParseWhile(ctx))
		return node;

	if(SynBase ref node = ParseDoWhile(ctx))
		return node;

	if(SynBase ref node = ParseSwitch(ctx))
		return node;

	if(SynBase ref node = ParseFunctionDefinition(ctx))
		return node;

	if(SynBase ref node = ParseVariableDefinitions(ctx, false))
		return node;

	if(SynBase ref node = ParseAssignment(ctx))
	{
		if(ctx.Peek() == LexemeType.lex_none && ctx.Current() != ctx.Last())
			Report(ctx, ctx.Current(), "ERROR: unknown lexeme");

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after expression");

		return node;
	}

	return nullptr;
}

RefList<SynBase> ParseExpressions(ParseContext ref ctx)
{
	RefList<SynBase> expressions;

	SynBase ref expression = ParseExpression(ctx);

	while(expression)
	{
		expressions.push_back(expression);

		expression = ParseExpression(ctx);
	}

	return expressions;
}

InplaceStr GetModuleName(RefList<SynIdentifier> parts);

ByteCode ref GetBytecodeFromPath(ParseContext ref ctx, LexemeRef start, RefList<SynIdentifier> parts, int ref lexCount, vector<Lexeme> ref lexStream, int optimizationLevel, ArrayView<InplaceStr> activeImports)
{
	InplaceStr moduleName = GetModuleName(parts);

	//TRACE_SCOPE("parser", "GetBytecodeFromPath");
	//TRACE_LABEL2(moduleName.begin, moduleName.end);

	/*ByteCode ref bytecode = BinaryCache::FindBytecode(moduleName.begin, false);

	lexCount = 0;
	lexStream = BinaryCache::FindLexems(moduleName.begin, false, lexCount);

	if(!bytecode)
	{
		if(!ctx.bytecodeBuilder)
			Stop(ctx, start, "ERROR: import builder is not provided");

		if(ctx.errorCount == 0)
		{
			ctx.errorPos = start.pos;
			ctx.errorBufLocation = ctx.errorBuf;
		}

		const char *messageStart = ctx.errorBufLocation;

		const char *pos = nullptr;
		bytecode = ctx.bytecodeBuilder(ctx.allocator, moduleName, false, &pos, ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf), optimizationLevel, activeImports);

		if(!bytecode)
		{
			if(ctx.errorBuf && ctx.errorBufSize)
			{
				ctx.errorBufLocation += strlen(ctx.errorBufLocation);

				const char *messageEnd = ctx.errorBufLocation;

				ctx.errorInfo.push_back(new ErrorInfo(ctx.allocator, messageStart, messageEnd, start, start, pos));

				AddErrorLocationInfo(ctx.code, pos, ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf));

				ctx.errorBufLocation += strlen(ctx.errorBufLocation);
			}

			ctx.errorCount++;
		}
	}

	return bytecode;*/

	return nullptr;
}

void ImportModuleNamespaces(ParseContext ref ctx, LexemeRef pos, ByteCode ref bCode)
{
	//TRACE_SCOPE("parser", "ImportModuleNamespaces");

	/*char[] symbols = FindSymbols(bCode);

	// Import namespaces
	ExternNamespaceInfo *namespaceList = FindFirstNamespace(bCode);

	for(int i = 0; i < bCode.namespaceCount; i++)
	{
		ExternNamespaceInfo &ns = namespaceList[i];

		const char *name = symbols + ns.offsetToName;

		SynNamespaceElement ref parent = nullptr;

		if(ns.parentHash != ~0u)
		{
			for(int k = 0; k < ctx.namespaceList.size(); k++)
			{
				if(ctx.namespaceList[k].fullNameHash == ns.parentHash)
				{
					parent = ctx.namespaceList[k];
					break;
				}
			}

			if(!parent)
				Stop(ctx, pos, "ERROR: namespace %s parent not found", name);
		}

		SynIdentifier ref nameIdentifier = new SynIdentifier(ctx.Previous(), ctx.Previous(), InplaceStr(name));

		ctx.namespaceList.push_back(new SynNamespaceElement(parent, nameIdentifier));
	}*/
}

SynModuleImport ref ParseImport(ParseContext ref ctx)
{
	//TRACE_SCOPE("parser", "ParseImport");

	LexemeRef start = ctx.currentLexeme;

	if(ctx.Consume(LexemeType.lex_import))
	{
		RefList<SynIdentifier> path;

		if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: name expected after import"))
		{
			InplaceStr value = ctx.Consume();
			path.push_back(new SynIdentifier(ctx.Previous(), ctx.Previous(), value));
		}

		while(ctx.Consume(LexemeType.lex_point))
		{
			if(CheckAt(ctx, LexemeType.lex_identifier, "ERROR: name expected after '.'"))
			{
				InplaceStr value = ctx.Consume();
				path.push_back(new SynIdentifier(ctx.Previous(), ctx.Previous(), value));
			}
		}

		CheckConsume(ctx, LexemeType.lex_semicolon, "ERROR: ';' not found after import expression");

		InplaceStr moduleName = GetModuleName(path);

		for(int i = 0; i < ctx.activeImports.size(); i++)
		{
			if(ctx.activeImports[i] == moduleName)
				Stop(ctx, start, "ERROR: found cyclic dependency on module '%.*s'", moduleName.length(), moduleName.begin);
		}

		ctx.activeImports.push_back(moduleName);

		int lexCount = 0;
		vector<Lexeme> lexStream;

		if(ByteCode ref binary = GetBytecodeFromPath(ctx, start, path, lexCount, &lexStream, ctx.optimizationLevel, ViewOf(ctx.activeImports)))
		{
			ImportModuleNamespaces(ctx, start, binary);

			ctx.activeImports.pop_back();

			return new SynModuleImport(start, ctx.Previous(), path, binary);
		}
	}

	return nullptr;
}

RefList<SynModuleImport> ParseImports(ParseContext ref ctx)
{
	//TRACE_SCOPE("parser", "ParseImports");

	RefList<SynModuleImport> imports;

	while(ctx.At(LexemeType.lex_import))
	{
		if(SynModuleImport ref moduleImport = ParseImport(ctx))
			imports.push_back(moduleImport);
	}

	return imports;
}

SynModule ref ParseModule(ParseContext ref ctx)
{
	//TRACE_SCOPE("parser", "ParseModule");

	LexemeRef start = ctx.currentLexeme;

	RefList<SynModuleImport> imports = ParseImports(ctx);

	RefList<SynBase> expressions = ParseExpressions(ctx);

	if(!ctx.Consume(LexemeType.lex_none))
		Report(ctx, ctx.Current(), "ERROR: unexpected symbol");

	if(expressions.empty())
	{
		Report(ctx, ctx.Current(), "ERROR: module contains no code");

		return new SynModule(start, ctx.Current(), imports, expressions);
	}

	return new SynModule(start, ctx.Previous(), imports, expressions);
}

SynModule ref Parse(ParseContext ref ctx, char[] code)
{
	//TRACE_SCOPE("parser", "Parse");

	ctx.code = code;

	ctx.lexer.Lexify(code);

	//int traceDepth = NULLC::TraceGetDepth();

	//if(!setjmp(ctx.errorHandler))
	{
		assert(ctx.lexer.lexems.size() != 0);

		ctx.errorHandlerActive = true;

		ctx.firstLexeme = LexemeRef(ctx.lexer, 0);
		ctx.currentLexeme = LexemeRef(ctx.lexer, 0);
		ctx.lastLexeme = LexemeRef(ctx.lexer, ctx.lexer.lexems.size() - 1);

		SynModule ref module = ParseModule(ctx);

		ctx.errorHandlerActive = false;

		ctx.code = nullptr;

		return module;
	}

	//NULLC::TraceLeaveTo(traceDepth);

	//assert(ctx.errorPos);

	ctx.code = nullptr;

	return nullptr;
}

void VisitParseTreeNodes(SynBase ref syntax, void ref(SynBase ref) accept)
{
	if(!syntax)
		return;

	accept(syntax);

	if(SynTypeSimple ref node = getType with<SynTypeSimple>(syntax))
	{
		for(SynIdentifier ref part = node.path.head; part; part = getType with<SynIdentifier>(part.next))
			VisitParseTreeNodes(part, accept);
	}
	else if(SynTypeArray ref node = getType with<SynTypeArray>(syntax))
	{
		VisitParseTreeNodes(node.type, accept);

		for(SynBase ref size = node.sizes.head; size; size = size.next)
			VisitParseTreeNodes(size, accept);
	}
	else if(SynTypeReference ref node = getType with<SynTypeReference>(syntax))
	{
		VisitParseTreeNodes(node.type, accept);
	}
	else if(SynTypeFunction ref node = getType with<SynTypeFunction>(syntax))
	{
		VisitParseTreeNodes(node.returnType, accept);

		for(SynBase ref arg = node.arguments.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynTypeGenericInstance ref node = getType with<SynTypeGenericInstance>(syntax))
	{
		VisitParseTreeNodes(node.baseType, accept);

		for(SynBase ref type = node.types.head; type; type = type.next)
			VisitParseTreeNodes(type, accept);
	}
	else if(SynTypeof ref node = getType with<SynTypeof>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynArray ref node = getType with<SynArray>(syntax))
	{
		for(SynBase ref value = node.values.head; value; value = value.next)
			VisitParseTreeNodes(value, accept);
	}
	else if(SynGenerator ref node = getType with<SynGenerator>(syntax))
	{
		for(SynBase ref value = node.expressions.head; value; value = value.next)
			VisitParseTreeNodes(value, accept);
	}
	else if(SynAlign ref node = getType with<SynAlign>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynTypedef ref node = getType with<SynTypedef>(syntax))
	{
		VisitParseTreeNodes(node.type, accept);
	}
	else if(SynMemberAccess ref node = getType with<SynMemberAccess>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynCallArgument ref node = getType with<SynCallArgument>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynArrayIndex ref node = getType with<SynArrayIndex>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);

		for(SynBase ref arg = node.arguments.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynFunctionCall ref node = getType with<SynFunctionCall>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);

		for(SynBase ref arg = node.aliases.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		for(SynBase ref arg = node.arguments.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynPreModify ref node = getType with<SynPreModify>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynPostModify ref node = getType with<SynPostModify>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynGetAddress ref node = getType with<SynGetAddress>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynDereference ref node = getType with<SynDereference>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynSizeof ref node = getType with<SynSizeof>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynNew ref node = getType with<SynNew>(syntax))
	{
		VisitParseTreeNodes(node.type, accept);

		for(SynBase ref arg = node.arguments.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		VisitParseTreeNodes(node.count, accept);

		for(SynBase ref arg = node.constructor.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynConditional ref node = getType with<SynConditional>(syntax))
	{
		VisitParseTreeNodes(node.condition, accept);
		VisitParseTreeNodes(node.trueBlock, accept);
		VisitParseTreeNodes(node.falseBlock, accept);
	}
	else if(SynReturn ref node = getType with<SynReturn>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynYield ref node = getType with<SynYield>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynBreak ref node = getType with<SynBreak>(syntax))
	{
		VisitParseTreeNodes(node.number, accept);
	}
	else if(SynContinue ref node = getType with<SynContinue>(syntax))
	{
		VisitParseTreeNodes(node.number, accept);
	}
	else if(SynLabel ref node = getType with<SynLabel>(syntax))
	{
		VisitParseTreeNodes(node.number, accept);
	}
	else if(SynGoto ref node = getType with<SynGoto>(syntax))
	{
		VisitParseTreeNodes(node.number, accept);
	}
	else if(SynBlock ref node = getType with<SynBlock>(syntax))
	{
		for(SynBase ref expr = node.expressions.head; expr; expr = expr.next)
			VisitParseTreeNodes(expr, accept);
	}
	else if(SynIfElse ref node = getType with<SynIfElse>(syntax))
	{
		VisitParseTreeNodes(node.condition, accept);
		VisitParseTreeNodes(node.trueBlock, accept);
		VisitParseTreeNodes(node.falseBlock, accept);
	}
	else if(SynFor ref node = getType with<SynFor>(syntax))
	{
		VisitParseTreeNodes(node.initializer, accept);
		VisitParseTreeNodes(node.condition, accept);
		VisitParseTreeNodes(node.increment, accept);
		VisitParseTreeNodes(node.body, accept);
	}
	else if(SynForEachIterator ref node = getType with<SynForEachIterator>(syntax))
	{
		VisitParseTreeNodes(node.type, accept);
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynForEach ref node = getType with<SynForEach>(syntax))
	{
		for(SynBase ref arg = node.iterators.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		VisitParseTreeNodes(node.body, accept);
	}
	else if(SynWhile ref node = getType with<SynWhile>(syntax))
	{
		VisitParseTreeNodes(node.condition, accept);
		VisitParseTreeNodes(node.body, accept);
	}
	else if(SynDoWhile ref node = getType with<SynDoWhile>(syntax))
	{
		for(SynBase ref arg = node.expressions.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		VisitParseTreeNodes(node.condition, accept);
	}
	else if(SynSwitchCase ref node = getType with<SynSwitchCase>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);

		for(SynBase ref arg = node.expressions.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynSwitch ref node = getType with<SynSwitch>(syntax))
	{
		VisitParseTreeNodes(node.condition, accept);

		for(SynBase ref arg = node.cases.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynUnaryOp ref node = getType with<SynUnaryOp>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynBinaryOp ref node = getType with<SynBinaryOp>(syntax))
	{
		VisitParseTreeNodes(node.lhs, accept);
		VisitParseTreeNodes(node.rhs, accept);
	}
	else if(SynAssignment ref node = getType with<SynAssignment>(syntax))
	{
		VisitParseTreeNodes(node.lhs, accept);
		VisitParseTreeNodes(node.rhs, accept);
	}
	else if(SynModifyAssignment ref node = getType with<SynModifyAssignment>(syntax))
	{
		VisitParseTreeNodes(node.lhs, accept);
		VisitParseTreeNodes(node.rhs, accept);
	}
	else if(SynVariableDefinition ref node = getType with<SynVariableDefinition>(syntax))
	{
		VisitParseTreeNodes(node.initializer, accept);
	}
	else if(SynVariableDefinitions ref node = getType with<SynVariableDefinitions>(syntax))
	{
		VisitParseTreeNodes(node.alignment, accept);
		VisitParseTreeNodes(node.type, accept);

		for(SynBase ref arg = node.definitions.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynAccessor ref node = getType with<SynAccessor>(syntax))
	{
		VisitParseTreeNodes(node.getBlock, accept);
		VisitParseTreeNodes(node.setBlock, accept);
	}
	else if(SynFunctionArgument ref node = getType with<SynFunctionArgument>(syntax))
	{
		VisitParseTreeNodes(node.type, accept);
		VisitParseTreeNodes(node.initializer, accept);
	}
	else if(SynFunctionDefinition ref node = getType with<SynFunctionDefinition>(syntax))
	{
		VisitParseTreeNodes(node.parentType, accept);
		VisitParseTreeNodes(node.returnType, accept);

		for(SynBase ref arg = node.aliases.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		for(SynBase ref arg = node.arguments.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		for(SynBase ref arg = node.expressions.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynShortFunctionArgument ref node = getType with<SynShortFunctionArgument>(syntax))
	{
		VisitParseTreeNodes(node.type, accept);
	}
	else if(SynShortFunctionDefinition ref node = getType with<SynShortFunctionDefinition>(syntax))
	{
		for(SynBase ref arg = node.arguments.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		for(SynBase ref arg = node.expressions.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynConstant ref node = getType with<SynConstant>(syntax))
	{
		VisitParseTreeNodes(node.value, accept);
	}
	else if(SynConstantSet ref node = getType with<SynConstantSet>(syntax))
	{
		VisitParseTreeNodes(node.type, accept);

		for(SynBase ref arg = node.constants.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynClassStaticIf ref node = getType with<SynClassStaticIf>(syntax))
	{
		VisitParseTreeNodes(node.condition, accept);
		VisitParseTreeNodes(node.trueBlock, accept);
		VisitParseTreeNodes(node.falseBlock, accept);
	}
	else if(SynClassElements ref node = getType with<SynClassElements>(syntax))
	{
		for(SynBase ref arg = node.typedefs.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		for(SynBase ref arg = node.functions.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		for(SynBase ref arg = node.accessors.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		for(SynBase ref arg = node.members.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		for(SynBase ref arg = node.constantSets.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		for(SynBase ref arg = node.staticIfs.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynClassDefinition ref node = getType with<SynClassDefinition>(syntax))
	{
		VisitParseTreeNodes(node.alignment, accept);

		for(SynBase ref arg = node.aliases.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);

		VisitParseTreeNodes(node.baseClass, accept);
		VisitParseTreeNodes(node.elements, accept);
	}
	else if(SynEnumDefinition ref node = getType with<SynEnumDefinition>(syntax))
	{
		for(SynBase ref arg = node.values.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynNamespaceDefinition ref node = getType with<SynNamespaceDefinition>(syntax))
	{
		for(SynIdentifier ref part = node.path.head; part; part = getType with<SynIdentifier>(part.next))
			VisitParseTreeNodes(part, accept);

		for(SynBase ref arg = node.expressions.head; arg; arg = arg.next)
			VisitParseTreeNodes(arg, accept);
	}
	else if(SynModuleImport ref node = getType with<SynModuleImport>(syntax))
	{
		for(SynIdentifier ref part = node.path.head; part; part = getType with<SynIdentifier>(part.next))
			VisitParseTreeNodes(part, accept);
	}
	else if(SynModule ref node = getType with<SynModule>(syntax))
	{
		for(SynModuleImport ref moduleImport = node.imports.head; moduleImport; moduleImport = getType with<SynModuleImport>(moduleImport.next))
			VisitParseTreeNodes(moduleImport, accept);

		for(SynBase ref expr = node.expressions.head; expr; expr = expr.next)
			VisitParseTreeNodes(expr, accept);
	}
}

char[] GetParseTreeNodeName(SynBase ref syntax)
{
	switch(typeid(syntax))
	{
	case SynError:
		return "SynError";
	case SynNothing:
		return "SynNothing";
	case SynIdentifier:
		return "SynIdentifier";
	case SynTypeAuto:
		return "SynTypeAuto";
	case SynTypeGeneric:
		return "SynTypeGeneric";
	case SynTypeSimple:
		return "SynTypeSimple";
	case SynTypeAlias:
		return "SynTypeAlias";
	case SynTypeArray:
		return "SynTypeArray";
	case SynTypeReference:
		return "SynTypeReference";
	case SynTypeFunction:
		return "SynTypeFunction";
	case SynTypeGenericInstance:
		return "SynTypeGenericInstance";
	case SynTypeof:
		return "SynTypeof";
	case SynBool:
		return "SynBool";
	case SynNumber:
		return "SynNumber";
	case SynNullptr:
		return "SynNullptr";
	case SynCharacter:
		return "SynCharacter";
	case SynString:
		return "SynString";
	case SynArray:
		return "SynArray";
	case SynGenerator:
		return "SynGenerator";
	case SynAlign:
		return "SynAlign";
	case SynTypedef:
		return "SynTypedef";
	case SynMemberAccess:
		return "SynMemberAccess";
	case SynCallArgument:
		return "SynCallArgument";
	case SynArrayIndex:
		return "SynArrayIndex";
	case SynFunctionCall:
		return "SynFunctionCall";
	case SynPreModify:
		return "SynPreModify";
	case SynPostModify:
		return "SynPostModify";
	case SynGetAddress:
		return "SynGetAddress";
	case SynDereference:
		return "SynDereference";
	case SynSizeof:
		return "SynSizeof";
	case SynNew:
		return "SynNew";
	case SynConditional:
		return "SynConditional";
	case SynReturn:
		return "SynReturn";
	case SynYield:
		return "SynYield";
	case SynBreak:
		return "SynBreak";
	case SynContinue:
		return "SynContinue";
	case SynLabel:
		return "SynLabel";
	case SynGoto:
		return "SynGoto";
	case SynBlock:
		return "SynBlock";
	case SynIfElse:
		return "SynIfElse";
	case SynFor:
		return "SynFor";
	case SynForEachIterator:
		return "SynForEachIterator";
	case SynForEach:
		return "SynForEach";
	case SynWhile:
		return "SynWhile";
	case SynDoWhile:
		return "SynDoWhile";
	case SynSwitchCase:
		return "SynSwitchCase";
	case SynSwitch:
		return "SynSwitch";
	case SynUnaryOp:
		return "SynUnaryOp";
	case SynBinaryOp:
		return "SynBinaryOp";
	case SynAssignment:
		return "SynAssignment";
	case SynModifyAssignment:
		return "SynModifyAssignment";
	case SynVariableDefinition:
		return "SynVariableDefinition";
	case SynVariableDefinitions:
		return "SynVariableDefinitions";
	case SynAccessor:
		return "SynAccessor";
	case SynFunctionArgument:
		return "SynFunctionArgument";
	case SynFunctionDefinition:
		return "SynFunctionDefinition";
	case SynShortFunctionArgument:
		return "SynShortFunctionArgument";
	case SynShortFunctionDefinition:
		return "SynShortFunctionDefinition";
	case SynConstant:
		return "SynConstant";
	case SynConstantSet:
		return "SynConstantSet";
	case SynClassPrototype:
		return "SynClassPrototype";
	case SynClassStaticIf:
		return "SynClassStaticIf";
	case SynClassElements:
		return "SynClassElements";
	case SynClassDefinition:
		return "SynClassDefinition";
	case SynEnumDefinition:
		return "SynEnumDefinition";
	case SynNamespaceDefinition:
		return "SynNamespaceDefinition";
	case SynModuleImport:
		return "SynModuleImport";
	case SynModule:
		return "SynModule";
	default:
		break;
	}

	assert(false, "unknown type");
	return "unknown";
}

InplaceStr GetModuleName(RefList<SynIdentifier> parts)
{
	int pathLength = int(parts.size() - 1 + (".nc").size - 1);

	for(SynIdentifier ref part = parts.head; part; part = getType with<SynIdentifier>(part.next))
		pathLength += part.name.length();

	char[] path = new char[pathLength + 1];

	/*char *pos = path;

	for(SynIdentifier ref part = parts.head; part; part = getType with<SynIdentifier>(part.next))
	{
		memcpy(pos, part.name.begin, part.name.length());
		pos += part.name.length();

		if(part.next)
			*pos++ = '/';
	}

	strcpy(pos, ".nc");
	pos += strlen(".nc");

	*pos = 0;*/

	return InplaceStr(path);
}
