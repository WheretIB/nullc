#include "ParseTree.h"

#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>

#include "BinaryCache.h"
#include "Bytecode.h"
#include "Lexer.h"
#include "Trace.h"

void AddErrorLocationInfo(const char *codeStart, const char *errorPos, char *errorBuf, unsigned errorBufSize);

namespace
{
	void ReportAt(ParseContext &ctx, Lexeme *begin, Lexeme *end, const char *pos, const char *msg, va_list args)
	{
		if(ctx.errorBuf && ctx.errorBufSize)
		{
			if(ctx.errorCount == 0)
			{
				ctx.errorPos = pos;
				ctx.errorBufLocation = ctx.errorBuf;
			}

			// Don't report multiple errors at the same position
			if(!ctx.errorInfo.empty() && ctx.errorInfo.back()->pos == pos)
				return;

			const char *messageStart = ctx.errorBufLocation;

			vsnprintf(ctx.errorBufLocation, ctx.errorBufSize - unsigned(ctx.errorBufLocation - ctx.errorBuf), msg, args);
			ctx.errorBuf[ctx.errorBufSize - 1] = '\0';

			ctx.errorBufLocation += strlen(ctx.errorBufLocation);

			const char *messageEnd = ctx.errorBufLocation;

			ctx.errorInfo.push_back(new (ctx.get<ErrorInfo>()) ErrorInfo(ctx.allocator, messageStart, messageEnd, begin, end, pos));

			AddErrorLocationInfo(ctx.code, pos, ctx.errorBufLocation, ctx.errorBufSize - unsigned(ctx.errorBufLocation - ctx.errorBuf));

			ctx.errorBufLocation += strlen(ctx.errorBufLocation);
		}

		ctx.errorCount++;

		if(ctx.errorCount == 100)
		{
			NULLC::SafeSprintf(ctx.errorBufLocation, ctx.errorBufSize - unsigned(ctx.errorBufLocation - ctx.errorBuf), "ERROR: error limit reached");

			ctx.errorBufLocation += strlen(ctx.errorBufLocation);

			assert(ctx.errorHandlerActive);

			longjmp(ctx.errorHandler, 1);
		}
	}

	NULLC_PRINT_FORMAT_CHECK(3, 4) void Report(ParseContext &ctx, Lexeme *pos, const char *msg, ...)
	{
		va_list args;
		va_start(args, msg);

		if(pos == ctx.Last() && pos != ctx.First())
			ReportAt(ctx, pos - 1, pos - 1, (pos - 1)->pos + (pos - 1)->length, msg, args);
		else
			ReportAt(ctx, pos, pos, pos->pos, msg, args);

		va_end(args);
	}

	void StopAt(ParseContext &ctx, Lexeme *begin, Lexeme *end, const char *pos, const char *msg, va_list args)
	{
		ReportAt(ctx, begin, end, pos, msg, args);

		assert(ctx.errorHandlerActive);

		longjmp(ctx.errorHandler, 1);
	}

	NULLC_PRINT_FORMAT_CHECK(3, 4) void Stop(ParseContext &ctx, Lexeme *pos, const char *msg, ...)
	{
		va_list args;
		va_start(args, msg);

		if(pos == ctx.Last() && pos != ctx.First())
			StopAt(ctx, pos - 1, pos - 1, (pos - 1)->pos + (pos - 1)->length, msg, args);
		else
			StopAt(ctx, pos, pos, pos->pos, msg, args);
	}

	NULLC_PRINT_FORMAT_CHECK(3, 4) bool CheckAt(ParseContext &ctx, LexemeType type, const char *msg, ...)
	{
		if(!ctx.At(type))
		{
			va_list args;
			va_start(args, msg);

			Lexeme *curr = ctx.Current();

			if(curr == ctx.Last() && curr != ctx.First())
				ReportAt(ctx, curr - 1, curr - 1, (curr - 1)->pos + (curr - 1)->length, msg, args);
			else
				ReportAt(ctx, curr, curr, curr->pos, msg, args);

			va_end(args);

			return false;
		}

		return true;
	}

	NULLC_PRINT_FORMAT_CHECK(3, 4) bool CheckConsume(ParseContext &ctx, LexemeType type, const char *msg, ...)
	{
		if(!ctx.Consume(type))
		{
			va_list args;
			va_start(args, msg);

			Lexeme *curr = ctx.Current();

			if(curr == ctx.Last() && curr != ctx.First())
				ReportAt(ctx, curr - 1, curr - 1, (curr - 1)->pos + (curr - 1)->length, msg, args);
			else
				ReportAt(ctx, curr, curr, curr->pos, msg, args);

			va_end(args);

			return false;
		}

		return true;
	}

	NULLC_PRINT_FORMAT_CHECK(3, 4) bool CheckConsume(ParseContext &ctx, const char *str, const char *msg, ...)
	{
		if(!ctx.Consume(str))
		{
			va_list args;
			va_start(args, msg);

			Lexeme *curr = ctx.Current();

			if(curr == ctx.Last() && curr != ctx.First())
				ReportAt(ctx, curr - 1, curr - 1, (curr - 1)->pos + (curr - 1)->length, msg, args);
			else
				ReportAt(ctx, curr, curr, curr->pos, msg, args);

			va_end(args);

			return false;
		}

		return true;
	}
}

SynUnaryOpType GetUnaryOpType(LexemeType type)
{
	switch(type)
	{
	case lex_add:
		return SYN_UNARY_OP_PLUS;
	case lex_sub:
		return SYN_UNARY_OP_NEGATE;
	case lex_bitnot:
		return SYN_UNARY_OP_BIT_NOT;
	case lex_lognot:
		return SYN_UNARY_OP_LOGICAL_NOT;
	default:
		break;
	}

	return SYN_UNARY_OP_UNKNOWN;
}

SynBinaryOpType GetBinaryOpType(LexemeType type)
{
	switch(type)
	{
	case lex_add:
		return SYN_BINARY_OP_ADD;
	case lex_sub:
		return SYN_BINARY_OP_SUB;
	case lex_mul:
		return SYN_BINARY_OP_MUL;
	case lex_div:
		return SYN_BINARY_OP_DIV;
	case lex_mod:
		return SYN_BINARY_OP_MOD;
	case lex_pow:
		return SYN_BINARY_OP_POW;
	case lex_less:
		return SYN_BINARY_OP_LESS;
	case lex_lequal:
		return SYN_BINARY_OP_LESS_EQUAL;
	case lex_shl:
		return SYN_BINARY_OP_SHL;
	case lex_greater:
		return SYN_BINARY_OP_GREATER;
	case lex_gequal:
		return SYN_BINARY_OP_GREATER_EQUAL;
	case lex_shr:
		return SYN_BINARY_OP_SHR;
	case lex_equal:
		return SYN_BINARY_OP_EQUAL;
	case lex_nequal:
		return SYN_BINARY_OP_NOT_EQUAL;
	case lex_bitand:
		return SYN_BINARY_OP_BIT_AND;
	case lex_bitor:
		return SYN_BINARY_OP_BIT_OR;
	case lex_bitxor:
		return SYN_BINARY_OP_BIT_XOR;
	case lex_logand:
		return SYN_BINARY_OP_LOGICAL_AND;
	case lex_logor:
		return SYN_BINARY_OP_LOGICAL_OR;
	case lex_logxor:
		return SYN_BINARY_OP_LOGICAL_XOR;
	case lex_in:
		return SYN_BINARY_OP_IN;
	default:
		break;
	}

	return SYN_BINARY_OP_UNKNOWN;
}

unsigned GetBinaryOpPrecedence(SynBinaryOpType op)
{
	switch(op)
	{
	case SYN_BINARY_OP_ADD:
		return 2;
	case SYN_BINARY_OP_SUB:
		return 2;
	case SYN_BINARY_OP_MUL:
		return 1;
	case SYN_BINARY_OP_DIV:
		return 1;
	case SYN_BINARY_OP_MOD:
		return 1;
	case SYN_BINARY_OP_POW:
		return 0;
	case SYN_BINARY_OP_LESS:
		return 4;
	case SYN_BINARY_OP_LESS_EQUAL:
		return 4;
	case SYN_BINARY_OP_SHL:
		return 3;
	case SYN_BINARY_OP_GREATER:
		return 4;
	case SYN_BINARY_OP_GREATER_EQUAL:
		return 4;
	case SYN_BINARY_OP_SHR:
		return 3;
	case SYN_BINARY_OP_EQUAL:
		return 5;
	case SYN_BINARY_OP_NOT_EQUAL:
		return 5;
	case SYN_BINARY_OP_BIT_AND:
		return 6;
	case SYN_BINARY_OP_BIT_OR:
		return 8;
	case SYN_BINARY_OP_BIT_XOR:
		return 7;
	case SYN_BINARY_OP_LOGICAL_AND:
		return 9;
	case SYN_BINARY_OP_LOGICAL_OR:
		return 11;
	case SYN_BINARY_OP_LOGICAL_XOR:
		return 10;
	case SYN_BINARY_OP_IN:
		return 12;
	default:
		break;
	}

	return 0;
}

SynModifyAssignType GetModifyAssignType(LexemeType type)
{
	switch(type)
	{
	case lex_addset:
		return SYN_MODIFY_ASSIGN_ADD;
	case lex_subset:
		return SYN_MODIFY_ASSIGN_SUB;
	case lex_mulset:
		return SYN_MODIFY_ASSIGN_MUL;
	case lex_divset:
		return SYN_MODIFY_ASSIGN_DIV;
	case lex_powset:
		return SYN_MODIFY_ASSIGN_POW;
	case lex_modset:
		return SYN_MODIFY_ASSIGN_MOD;
	case lex_shlset:
		return SYN_MODIFY_ASSIGN_SHL;
	case lex_shrset:
		return SYN_MODIFY_ASSIGN_SHR;
	case lex_andset:
		return SYN_MODIFY_ASSIGN_BIT_AND;
	case lex_orset:
		return SYN_MODIFY_ASSIGN_BIT_OR;
	case lex_xorset:
		return SYN_MODIFY_ASSIGN_BIT_XOR;
	default:
		break;
	}

	return SYN_MODIFY_ASSIGN_UNKNOWN;
}

ParseContext::ParseContext(Allocator *allocator, int optimizationLevel, ArrayView<InplaceStr> activeImports): lexer(allocator), binaryOpStack(allocator), namespaceList(allocator), optimizationLevel(optimizationLevel), activeImports(allocator), errorInfo(allocator), nonTypeLocations(allocator), nonFunctionDefinitionLocations(allocator), allocator(allocator)
{
	code = NULL;

	moduleRoot = NULL;

	bytecodeBuilder = NULL;

	firstLexeme = NULL;
	currentLexeme = NULL;
	lastLexeme = NULL;

	expressionGroupDepth = 0;
	expressionBlockDepth = 0;
	statementBlockDepth = 0;

	errorHandlerActive = false;
	errorPos = NULL;
	errorCount = 0;
	errorBuf = NULL;
	errorBufSize = 0;
	errorBufLocation = NULL;

	currentNamespace = NULL;

	for(unsigned i = 0; i < activeImports.size(); i++)
		this->activeImports.push_back(activeImports[i]);
}

LexemeType ParseContext::Peek()
{
	return currentLexeme->type;
}

InplaceStr ParseContext::Value()
{
	return InplaceStr(currentLexeme->pos, currentLexeme->length);
}

bool ParseContext::At(LexemeType type)
{
	return currentLexeme->type == type;
}

bool ParseContext::Consume(LexemeType type)
{
	if(currentLexeme->type == type)
	{
		Skip();
		return true;
	}

	return false;
}

bool ParseContext::Consume(const char *str)
{
	if(InplaceStr(currentLexeme->pos, currentLexeme->length) == InplaceStr(str))
	{
		Skip();
		return true;
	}

	return false;
}

InplaceStr ParseContext::Consume()
{
	InplaceStr str(currentLexeme->pos, currentLexeme->length);

	Skip();

	return str;
}

void ParseContext::Skip()
{
	if(currentLexeme->type != lex_none)
		currentLexeme++;
}

Lexeme* ParseContext::First()
{
	return firstLexeme;
}

Lexeme* ParseContext::Current()
{
	return currentLexeme;
}

Lexeme* ParseContext::Previous()
{
	assert(currentLexeme > firstLexeme);

	return currentLexeme - 1;
}

Lexeme* ParseContext::Last()
{
	return lastLexeme;
}

const char* ParseContext::Position()
{
	return currentLexeme->pos;
}

const char* ParseContext::LastEnding()
{
	assert(currentLexeme > firstLexeme);

	return (currentLexeme - 1)->pos + (currentLexeme - 1)->length;
}

SynNamespaceElement* ParseContext::IsNamespace(SynNamespaceElement *parent, InplaceStr name)
{
	// In the context of a parent namespace, we only look for immediate children
	if(parent)
	{
		// Search for existing namespace in the same context
		for(unsigned i = 0; i < namespaceList.size(); i++)
		{
			SynNamespaceElement *ns = namespaceList[i];

			if(ns->parent == parent && ns->name->name == name)
				return ns;
		}

		// Try from context of the parent namespace
		if(parent->parent)
			return IsNamespace(parent->parent, name);

		return NULL;
	}

	// Go from the bottom of the namespace stack, trying to find the namespace there
	SynNamespaceElement *current = currentNamespace;

	for(;;)
	{
		for(unsigned i = 0; i < namespaceList.size(); i++)
		{
			SynNamespaceElement *ns = namespaceList[i];

			if(ns->parent == current && ns->name->name == name)
				return ns;
		}

		if(current)
			current = current->parent;
		else
			break;
	}

	return NULL;
}

SynNamespaceElement* ParseContext::PushNamespace(SynIdentifier *name)
{
	SynNamespaceElement *current = currentNamespace;

	// Search for existing namespace in the same context
	for(unsigned i = 0; i < namespaceList.size(); i++)
	{
		SynNamespaceElement *ns = namespaceList[i];

		if(ns->parent == current && ns->name->name == name->name)
		{
			currentNamespace = ns;
			return ns;
		}
	}

	// Create new namespace
	SynNamespaceElement *ns = new (get<SynNamespaceElement>()) SynNamespaceElement(current, name);
	namespaceList.push_back(ns);

	currentNamespace = ns;
	return ns;
}

void ParseContext::PopNamespace()
{
	assert(currentNamespace);

	currentNamespace = currentNamespace->parent;
}

SynBase* ParseType(ParseContext &ctx, bool *shrBorrow = 0, bool onlyType = false);
SynBase* ParsePostExpressions(ParseContext &ctx, SynBase *node);
SynBase* ParseTerminal(ParseContext &ctx);
SynBase* ParseTernaryExpr(ParseContext &ctx);
SynBase* ParseAssignment(ParseContext &ctx);
SynTypedef* ParseTypedef(ParseContext &ctx);
SynBase* ParseStatement(ParseContext &ctx);
SynBase* ParseExpression(ParseContext &ctx);
IntrusiveList<SynBase> ParseExpressions(ParseContext &ctx);
SynFunctionDefinition* ParseFunctionDefinition(ParseContext &ctx);
SynShortFunctionDefinition* ParseShortFunctionDefinition(ParseContext &ctx);
SynVariableDefinition* ParseVariableDefinition(ParseContext &ctx);
SynVariableDefinitions* ParseVariableDefinitions(ParseContext &ctx, bool classMembers);
SynAccessor* ParseAccessorDefinition(ParseContext &ctx);
SynConstantSet* ParseConstantSet(ParseContext &ctx);
SynClassStaticIf* ParseClassStaticIf(ParseContext &ctx, bool nested);
IntrusiveList<SynCallArgument> ParseCallArguments(ParseContext &ctx);

SynBase* ParseTerminalType(ParseContext &ctx, bool &shrBorrow)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.At(lex_identifier))
	{
		InplaceStr name = ctx.Consume();

		IntrusiveList<SynIdentifier> namespacePath;

		SynNamespaceElement *ns = NULL;

		while(ctx.At(lex_point) && (ns = ctx.IsNamespace(ns, name)) != NULL)
		{
			ctx.Skip();

			if(!CheckAt(ctx, lex_identifier, "ERROR: namespace member is expected after '.'"))
			{
				namespacePath.push_back(new (ctx.get<SynIdentifier>()) SynIdentifier(start, ctx.Previous(), name));

				break;
			}

			namespacePath.push_back(new (ctx.get<SynIdentifier>()) SynIdentifier(start, ctx.Previous(), name));

			name = ctx.Consume();
		}

		if(ctx.Consume(lex_less))
		{
			IntrusiveList<SynBase> types;

			SynBase *type = ParseType(ctx, &shrBorrow);

			if(!type)
			{
				if(ctx.Peek() == lex_greater)
					Report(ctx, ctx.Current(), "ERROR: typename required after '<'");

				// Backtrack
				ctx.currentLexeme = start;

				return NULL;
			}

			types.push_back(type);

			while(ctx.Consume(lex_comma))
			{
				type = ParseType(ctx, &shrBorrow);

				if(!type)
				{
					Report(ctx, ctx.Current(), "ERROR: typename required after ','");

					break;
				}

				types.push_back(type);
			}

			bool closed = ctx.Consume(lex_greater);

			if(!closed && ctx.At(lex_shr))
			{
				if(shrBorrow)
					ctx.Skip();

				shrBorrow = !shrBorrow;

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

					return NULL;
				}
			}

			return new (ctx.get<SynTypeGenericInstance>()) SynTypeGenericInstance(start, ctx.Previous(), new (ctx.get<SynTypeSimple>()) SynTypeSimple(start, ctx.Previous(), namespacePath, name), types);
		}

		return new (ctx.get<SynTypeSimple>()) SynTypeSimple(start, ctx.Previous(), namespacePath, name);
	}

	if(ctx.Consume(lex_auto))
		return new (ctx.get<SynTypeAuto>()) SynTypeAuto(start, ctx.Previous());

	if(ctx.Consume(lex_generic))
		return new (ctx.get<SynTypeGeneric>()) SynTypeGeneric(start, ctx.Previous());

	if(ctx.Consume(lex_typeof))
	{
		CheckConsume(ctx, lex_oparen, "ERROR: typeof must be followed by '('");

		SynBase *value = ParseAssignment(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after typeof(");

			value = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after expression in typeof");

		SynBase *node = new (ctx.get<SynTypeof>()) SynTypeof(start, ctx.Previous(), value);

		return ParsePostExpressions(ctx, node);
	}

	if(ctx.Consume(lex_at))
	{
		if(!ctx.At(lex_identifier))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return NULL;
		}

		InplaceStr name = ctx.Consume();
		SynIdentifier *nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		return new (ctx.get<SynTypeAlias>()) SynTypeAlias(start, ctx.Previous(), nameIdentifier);
	}

	return NULL;
}

SynBase* ParseType(ParseContext &ctx, bool *shrBorrow, bool onlyType)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.nonTypeLocations.find(unsigned(start - ctx.firstLexeme) + 1))
		return NULL;

	bool shrBorrowTerminal = shrBorrow ? *shrBorrow : false;

	SynBase *base = ParseTerminalType(ctx, shrBorrowTerminal);
	
	if(!base)
		return NULL;

	if(shrBorrowTerminal)
	{
		if(!shrBorrow)
		{
			// Backtrack
			ctx.currentLexeme = start;
			ctx.nonTypeLocations.insert(unsigned(start - ctx.firstLexeme) + 1, true);

			return NULL;
		}
		else
		{
			*shrBorrow = true;
		}
	}

	while(ctx.At(lex_obracket) || ctx.At(lex_ref))
	{
		if(ctx.At(lex_obracket))
		{
			IntrusiveList<SynBase> sizes;

			while(ctx.Consume(lex_obracket))
			{
				Lexeme *sizeStart = ctx.currentLexeme;

				SynBase *size = ParseTernaryExpr(ctx);

				if(size && !ctx.At(lex_cbracket))
				{
					if(onlyType)
						Report(ctx, ctx.Current(), "ERROR: ']' not found after expression");

					// Backtrack
					ctx.currentLexeme = start;
					ctx.nonTypeLocations.insert(unsigned(start - ctx.firstLexeme) + 1, true);

					return NULL;
				}

				bool hasClose = CheckConsume(ctx, lex_cbracket, "ERROR: matching ']' not found");

				if(size)
					sizes.push_back(size);
				else
					sizes.push_back(new (ctx.get<SynNothing>()) SynNothing(sizeStart, hasClose ? ctx.Previous() : ctx.Current()));
			}

			base = new (ctx.get<SynTypeArray>()) SynTypeArray(start, ctx.Previous(), base, sizes);
		}
		else if(ctx.Consume(lex_ref))
		{
			Lexeme *refLexeme = ctx.currentLexeme;

			if(ctx.Consume(lex_oparen))
			{
				IntrusiveList<SynBase> arguments;

				if(SynBase *argument = ParseType(ctx))
				{
					arguments.push_back(argument);

					while(ctx.Consume(lex_comma))
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

				if(!ctx.Consume(lex_cparen))
				{
					// Backtrack
					ctx.currentLexeme = refLexeme;

					return new (ctx.get<SynTypeReference>()) SynTypeReference(start, ctx.Previous(), base);
				}

				base = new (ctx.get<SynTypeFunction>()) SynTypeFunction(start, ctx.Previous(), base, arguments);
			}
			else
			{
				base = new (ctx.get<SynTypeReference>()) SynTypeReference(start, ctx.Previous(), base);
			}
		}
	}

	return base;
}

SynBase* ParseArray(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_ofigure))
	{
		if(ctx.At(lex_for))
		{
			IntrusiveList<SynBase> expressions = ParseExpressions(ctx);

			CheckConsume(ctx, lex_cfigure, "ERROR: '}' not found after inline array");

			return new (ctx.get<SynGenerator>()) SynGenerator(start, ctx.Previous(), expressions);
		}

		const unsigned blockLimit = 256;

		if(ctx.expressionBlockDepth >= blockLimit)
			Stop(ctx, ctx.Current(), "ERROR: reached nested array limit of %d", blockLimit);

		ctx.expressionBlockDepth++;

		IntrusiveList<SynBase> values;

		SynBase *value = ParseTernaryExpr(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: value not found after '{'");

			value = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		values.push_back(value);

		while(ctx.Consume(lex_comma))
		{
			value = ParseTernaryExpr(ctx);

			if(!value)
				Report(ctx, ctx.Current(), "ERROR: value not found after ','");
			else
				values.push_back(value);
		}

		ctx.expressionBlockDepth--;

		CheckConsume(ctx, lex_cfigure, "ERROR: '}' not found after inline array");

		return new (ctx.get<SynArray>()) SynArray(start, ctx.Previous(), values);
	}

	return NULL;
}

SynBase* ParseString(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	bool rawLiteral = ctx.Consume(lex_at);

	if(ctx.At(lex_quotedstring))
	{
		InplaceStr str = ctx.Consume();

		if(str.length() == 1 || str.begin[str.length() - 1] != '\"')
			Report(ctx, start, "ERROR: unclosed string constant");

		return new (ctx.get<SynString>()) SynString(start, ctx.Previous(), rawLiteral, str);
	}

	if(rawLiteral)
	{
		// Backtrack
		ctx.currentLexeme = start;
	}

	return NULL;
}

SynBase* ParseSizeof(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_sizeof))
	{
		CheckConsume(ctx, lex_oparen, "ERROR: sizeof must be followed by '('");

		SynBase *value = ParseAssignment(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression or type not found after sizeof(");

			value = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after expression in sizeof");

		return new (ctx.get<SynSizeof>()) SynSizeof(start, ctx.Previous(), value);
	}

	return NULL;
}

SynNumber* ParseNumber(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.At(lex_number))
	{
		InplaceStr value = ctx.Consume();

		InplaceStr suffix;

		if(ctx.At(lex_identifier))
			suffix = ctx.Consume();

		return new (ctx.get<SynNumber>()) SynNumber(start, ctx.Previous(), value, suffix);
	}

	return NULL;
}

SynAlign* ParseAlign(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_align))
	{
		CheckConsume(ctx, lex_oparen, "ERROR: '(' expected after align");

		SynBase *value = NULL;

		if(CheckAt(ctx, lex_number, "ERROR: alignment value not found after align("))
			value = ParseNumber(ctx);
		else
			value = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());

		CheckConsume(ctx, lex_cparen, "ERROR: ')' expected after alignment value");

		return new (ctx.get<SynAlign>()) SynAlign(start, ctx.Previous(), value);
	}
	else if(ctx.Consume(lex_noalign))
	{
		return new (ctx.get<SynAlign>()) SynAlign(start, ctx.Previous(), NULL);
	}

	return NULL;
}

SynNew* ParseNew(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_new))
	{
		SynBase *type = NULL;

		bool explicitType = ctx.Consume(lex_oparen);

		if(explicitType)
		{
			type = ParseType(ctx);

			if(!type)
			{
				Report(ctx, ctx.Current(), "ERROR: type name expected after 'new'");

				type = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}

			CheckConsume(ctx, lex_cparen, "ERROR: matching ')' not found after '('");
		}
		else
		{
			type = ParseType(ctx, NULL, true);

			if(!type)
			{
				Report(ctx, ctx.Current(), "ERROR: type name expected after 'new'");

				type = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}
		}

		IntrusiveList<SynCallArgument> arguments;

		IntrusiveList<SynBase> constructor;

		if(ctx.Consume(lex_obracket))
		{
			SynBase *count = ParseTernaryExpr(ctx);

			if(!count)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '['");

				count = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}

			CheckConsume(ctx, lex_cbracket, "ERROR: ']' not found after expression");

			return new (ctx.get<SynNew>()) SynNew(start, ctx.Previous(), type, arguments, count, constructor);
		}
		else if(!explicitType && isType<SynTypeArray>(type))
		{
			SynTypeArray *arrayType = getType<SynTypeArray>(type);

			// Try to extract last array type extent as a size
			SynBase *prevSize = NULL;
			SynBase *count = arrayType->sizes.head;

			while(count->next)
			{
				prevSize = count;
				count = count->next;
			}

			// Check if the extent is real
			if(!isType<SynNothing>(count))
			{
				if(prevSize)
					prevSize->next = NULL;
				else
					type = arrayType->type;

				return new (ctx.get<SynNew>()) SynNew(start, ctx.Previous(), type, arguments, count, constructor);
			}
		}

		if(ctx.Consume(lex_oparen))
		{
			arguments = ParseCallArguments(ctx);

			CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after function argument list");
		}

		if(ctx.Consume(lex_ofigure))
		{
			constructor = ParseExpressions(ctx);

			CheckConsume(ctx, lex_cfigure, "ERROR: '}' not found after custom constructor body");
		}

		return new (ctx.get<SynNew>()) SynNew(start, ctx.Previous(), type, arguments, NULL, constructor);
	}

	return NULL;
}

SynCallArgument* ParseCallArgument(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.At(lex_identifier))
	{
		InplaceStr name = ctx.Consume();
		SynIdentifier *nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		if(ctx.Consume(lex_colon))
		{
			SynBase *value = ParseAssignment(ctx);

			if(!value)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after ':' in function argument list");

				value = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}

			return new (ctx.get<SynCallArgument>()) SynCallArgument(start, ctx.Previous(), nameIdentifier, value);
		}
		else
		{
			// Backtrack
			ctx.currentLexeme = start;
		}
	}

	if(SynBase *value = ParseAssignment(ctx))
	{
		return new (ctx.get<SynCallArgument>()) SynCallArgument(start, ctx.Previous(), NULL, value);
	}

	return NULL;
}

IntrusiveList<SynCallArgument> ParseCallArguments(ParseContext &ctx)
{
	IntrusiveList<SynCallArgument> arguments;

	if(SynCallArgument *argument = ParseCallArgument(ctx))
	{
		arguments.push_back(argument);

		bool namedCall = argument->name != NULL;

		while(ctx.Consume(lex_comma))
		{
			argument = ParseCallArgument(ctx);

			if(!argument)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after ',' in function argument list");

				arguments.push_back(new (ctx.get<SynCallArgument>()) SynCallArgument(ctx.Current(), ctx.Current(), NULL, new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current())));
				break;
			}

			if(namedCall && !argument->name)
			{
				Report(ctx, ctx.Current(), "ERROR: function argument name expected after ','");

				break;
			}

			namedCall |= argument->name != NULL;

			arguments.push_back(argument);
		}
	}

	return arguments;
}

SynBase* ParsePostExpressions(ParseContext &ctx, SynBase *node)
{
	while(ctx.At(lex_point) || ctx.At(lex_obracket) || ctx.At(lex_oparen) || ctx.At(lex_with))
	{
		Lexeme *pos = ctx.currentLexeme;

		if(ctx.Consume(lex_point))
		{
			if(ctx.At(lex_return) || CheckAt(ctx, lex_identifier, "ERROR: member name expected after '.'"))
			{
				InplaceStr member = ctx.Consume();
				SynIdentifier *memberIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), member);

				node = new (ctx.get<SynMemberAccess>()) SynMemberAccess(node->begin, ctx.Previous(), node, memberIdentifier);
			}
			else
			{
				node = new (ctx.get<SynMemberAccess>()) SynMemberAccess(ctx.Previous(), ctx.Previous(), node, NULL);
			}
		}
		else if(ctx.Consume(lex_obracket))
		{
			IntrusiveList<SynCallArgument> arguments = ParseCallArguments(ctx);

			CheckConsume(ctx, lex_cbracket, "ERROR: ']' not found after expression");

			node = new (ctx.get<SynArrayIndex>()) SynArrayIndex(pos, ctx.Previous(), node, arguments);
		}
		else if(ctx.Consume(lex_with))
		{
			IntrusiveList<SynBase> aliases;

			CheckConsume(ctx, lex_less, "ERROR: '<' not found before explicit generic type alias list");

			bool shrBorrow = false;

			do
			{
				SynBase *type = ParseType(ctx, &shrBorrow);

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
			while(ctx.Consume(lex_comma));

			CheckConsume(ctx, shrBorrow ? lex_shr : lex_greater, "ERROR: '>' not found after explicit generic type alias list");

			CheckConsume(ctx, lex_oparen, "ERROR: '(' is expected at this point");

			IntrusiveList<SynCallArgument> arguments = ParseCallArguments(ctx);

			CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after function argument list");

			node = new (ctx.get<SynFunctionCall>()) SynFunctionCall(pos, ctx.Previous(), node, aliases, arguments);
		}
		else if(ctx.Consume(lex_oparen))
		{
			IntrusiveList<SynBase> aliases;
			IntrusiveList<SynCallArgument> arguments = ParseCallArguments(ctx);

			CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after function argument list");

			node = new (ctx.get<SynFunctionCall>()) SynFunctionCall(pos, ctx.Previous(), node, aliases, arguments);
		}
		else
		{
			Stop(ctx, ctx.Current(), "ERROR: not implemented");
		}
	}

	Lexeme *pos = ctx.currentLexeme;

	if(ctx.Consume(lex_inc))
		node = new (ctx.get<SynPostModify>()) SynPostModify(pos, ctx.Previous(), node, true);
	else if(ctx.Consume(lex_dec))
		node = new (ctx.get<SynPostModify>()) SynPostModify(pos, ctx.Previous(), node, false);

	return node;
}

SynBase* ParseComplexTerminal(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_mul))
	{
		SynBase *node = ParseTerminal(ctx);

		if(!node)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after '*'");

			node = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		return new (ctx.get<SynDereference>()) SynDereference(start, ctx.Previous(), node);
	}

	SynBase *node = NULL;

	if(ctx.Consume(lex_oparen))
	{
		const unsigned groupLimit = 256;

		if(ctx.expressionGroupDepth >= groupLimit)
			Stop(ctx, ctx.Current(), "ERROR: reached nested '(' limit of %d", groupLimit);

		ctx.expressionGroupDepth++;

		node = ParseAssignment(ctx);

		ctx.expressionGroupDepth--;

		if(!node)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after '('");

			node = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, lex_cparen, "ERROR: closing ')' not found after '('");
	}

	if(!node)
		node = ParseString(ctx);

	if(!node)
		node = ParseArray(ctx);

	if(!node)
		node = ParseType(ctx);

	if(!node && ctx.At(lex_identifier))
	{
		InplaceStr value = ctx.Consume();

		node = new (ctx.get<SynIdentifier>()) SynIdentifier(start, ctx.Previous(), value);
	}

	if(!node && ctx.Consume(lex_at))
	{
		bool isOperator = (ctx.Peek() >= lex_add && ctx.Peek() <= lex_in) || (ctx.Peek() >= lex_set && ctx.Peek() <= lex_xorset) || ctx.Peek() == lex_bitnot || ctx.Peek() == lex_lognot;

		if(!isOperator)
		{
			Report(ctx, ctx.Current(), "ERROR: name expected after '@'");

			return new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		InplaceStr value = ctx.Consume();

		node = new (ctx.get<SynIdentifier>()) SynIdentifier(start, ctx.Previous(), value);
	}

	if(!node)
		return NULL;
		
	return ParsePostExpressions(ctx, node);
}

SynBase* ParseTerminal(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_true))
		return new (ctx.get<SynBool>()) SynBool(start, ctx.Previous(), true);

	if(ctx.Consume(lex_false))
		return new (ctx.get<SynBool>()) SynBool(start, ctx.Previous(), false);

	if(ctx.Consume(lex_nullptr))
		return new (ctx.get<SynNullptr>()) SynNullptr(start, ctx.Previous());

	if(ctx.Consume(lex_bitand))
	{
		SynBase *node = ParseComplexTerminal(ctx);

		if(!node)
		{
			Report(ctx, ctx.Current(), "ERROR: variable not found after '&'");

			node = new (ctx.get<SynError>()) SynError(start, ctx.Previous());
		}

		return new (ctx.get<SynGetAddress>()) SynGetAddress(start, ctx.Previous(), node);
	}

	if(ctx.At(lex_semiquotedchar))
	{
		InplaceStr str = ctx.Consume();

		if(str.length() == 1 || str.begin[str.length() - 1] != '\'')
			Stop(ctx, start, "ERROR: unclosed character constant");
		else if((str.length() > 3 && str.begin[1] != '\\') || str.length() > 4)
			Stop(ctx, start, "ERROR: only one character can be inside single quotes");
		else if(str.length() < 3)
			Stop(ctx, start, "ERROR: empty character constant");

		return new (ctx.get<SynCharacter>()) SynCharacter(start, ctx.Previous(), str);
	}

	if(SynUnaryOpType type = GetUnaryOpType(ctx.Peek()))
	{
		InplaceStr name = ctx.Consume();

		SynBase *value = ParseTerminal(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after '%.*s'", name.length(), name.begin);

			value = new (ctx.get<SynError>()) SynError(start, ctx.Previous());
		}

		return new (ctx.get<SynUnaryOp>()) SynUnaryOp(start, ctx.Previous(), type, value);
	}

	if(ctx.Consume(lex_dec))
	{
		SynBase *value = ParseTerminal(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: variable not found after '--'");

			value = new (ctx.get<SynError>()) SynError(start, ctx.Previous());
		}

		return new (ctx.get<SynPreModify>()) SynPreModify(start, ctx.Previous(), value, false);
	}

	if(ctx.Consume(lex_inc))
	{
		SynBase *value = ParseTerminal(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: variable not found after '++'");

			value = new (ctx.get<SynError>()) SynError(start, ctx.Previous());
		}

		return new (ctx.get<SynPreModify>()) SynPreModify(start, ctx.Previous(), value, true);
	}

	if(SynNumber *node = ParseNumber(ctx))
		return node;

	if(SynNew *node = ParseNew(ctx))
		return node;

	if(SynBase *node = ParseSizeof(ctx))
		return node;

	if(SynBase *node = ParseFunctionDefinition(ctx))
		return node;

	if(SynBase *node = ParseShortFunctionDefinition(ctx))
		return node;

	if(SynBase *node = ParseComplexTerminal(ctx))
		return node;

	return NULL;
}

SynBase* ParseArithmetic(ParseContext &ctx)
{
	SynBase *lhs = ParseTerminal(ctx);

	if(!lhs)
		return NULL;

	unsigned startSize = ctx.binaryOpStack.size();

	while(SynBinaryOpType binaryOp = GetBinaryOpType(ctx.Peek()))
	{
		Lexeme *start = ctx.currentLexeme;

		ctx.Skip();

		while(ctx.binaryOpStack.size() > startSize && GetBinaryOpPrecedence(ctx.binaryOpStack.back().type) <= GetBinaryOpPrecedence(binaryOp))
		{
			SynBinaryOpElement &lastOp = ctx.binaryOpStack.back();

			lhs = new (ctx.get<SynBinaryOp>()) SynBinaryOp(lastOp.begin, lastOp.end, lastOp.type, lastOp.value, lhs);

			ctx.binaryOpStack.pop_back();
		}

		ctx.binaryOpStack.push_back(SynBinaryOpElement(start, ctx.Previous(), binaryOp, lhs));

		lhs = ParseTerminal(ctx);

		if(!lhs)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after binary operation");

			lhs = new (ctx.get<SynError>()) SynError(start, ctx.Previous());
		}
	}

	while(ctx.binaryOpStack.size() > startSize)
	{
		SynBinaryOpElement &lastOp = ctx.binaryOpStack.back();

		lhs = new (ctx.get<SynBinaryOp>()) SynBinaryOp(lastOp.begin, lastOp.end, lastOp.type, lastOp.value, lhs);

		ctx.binaryOpStack.pop_back();
	}

	return lhs;
}

SynBase* ParseTernaryExpr(ParseContext &ctx)
{
	if(SynBase *value = ParseArithmetic(ctx))
	{
		Lexeme *pos = ctx.currentLexeme;

		while(ctx.Consume(lex_questionmark))
		{
			SynBase *trueBlock = ParseAssignment(ctx);

			if(!trueBlock)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '?'");

				trueBlock = new (ctx.get<SynError>()) SynError(pos, ctx.Previous());
			}

			CheckConsume(ctx, lex_colon, "ERROR: ':' not found after expression in ternary operator");

			SynBase *falseBlock = ParseAssignment(ctx);

			if(!falseBlock)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after ':'");

				falseBlock = new (ctx.get<SynError>()) SynError(pos, ctx.Previous());
			}

			value = new (ctx.get<SynConditional>()) SynConditional(pos, ctx.Previous(), value, trueBlock, falseBlock);
		}

		return value;
	}

	return NULL;
}

SynBase* ParseAssignment(ParseContext &ctx)
{
	if(SynBase *lhs = ParseTernaryExpr(ctx))
	{
		Lexeme *pos = ctx.currentLexeme;

		if(ctx.Consume(lex_set))
		{
			SynBase *rhs = ParseAssignment(ctx);

			if(!rhs)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

				rhs = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}

			return new (ctx.get<SynAssignment>()) SynAssignment(pos, ctx.Previous(), lhs, rhs);
		}
		else if(SynModifyAssignType modifyType = GetModifyAssignType(ctx.Peek()))
		{
			InplaceStr name = ctx.Consume();

			SynBase *rhs = ParseAssignment(ctx);

			if(!rhs)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '%.*s' operator", name.length(), name.begin);

				rhs = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}

			return new (ctx.get<SynModifyAssignment>()) SynModifyAssignment(pos, ctx.Previous(), modifyType, lhs, rhs);
		}

		return lhs;
	}

	return NULL;
}

SynClassElements* ParseClassElements(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	IntrusiveList<SynTypedef> typedefs;
	IntrusiveList<SynFunctionDefinition> functions;
	IntrusiveList<SynAccessor> accessors;
	IntrusiveList<SynVariableDefinitions> members;
	IntrusiveList<SynConstantSet> constantSets;
	IntrusiveList<SynClassStaticIf> staticIfs;

	for(;;)
	{
		if(SynTypedef *node = ParseTypedef(ctx))
		{
			typedefs.push_back(node);
		}
		else if(SynClassStaticIf *node = ParseClassStaticIf(ctx, false))
		{
			staticIfs.push_back(node);
		}
		else if(SynFunctionDefinition *node = ParseFunctionDefinition(ctx))
		{
			functions.push_back(node);
		}
		else if(SynAccessor *node = ParseAccessorDefinition(ctx))
		{
			accessors.push_back(node);
		}
		else if(SynVariableDefinitions *node = ParseVariableDefinitions(ctx, true))
		{
			members.push_back(node);
		}
		else if(SynConstantSet *node = ParseConstantSet(ctx))
		{
			constantSets.push_back(node);
		}
		else
		{
			break;
		}
	}

	Lexeme *end = start == ctx.currentLexeme ? start : ctx.Previous();

	return new (ctx.get<SynClassElements>()) SynClassElements(start, end, typedefs, functions, accessors, members, constantSets, staticIfs);
}

SynBase* ParseClassDefinition(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	SynAlign *align = ParseAlign(ctx);

	if(ctx.Consume(lex_class))
	{
		SynIdentifier *nameIdentifier = NULL;

		if(CheckAt(ctx, lex_identifier, "ERROR: class name expected"))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else
		{
			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(InplaceStr());
		}

		if(ctx.Consume(lex_semicolon))
		{
			if(align)
				Report(ctx, ctx.Current(), "ERROR: can't specify alignment of a class prototype");

			return new (ctx.get<SynClassPrototype>()) SynClassPrototype(start, ctx.Previous(), nameIdentifier);
		}

		IntrusiveList<SynIdentifier> aliases;

		if(ctx.Consume(lex_less))
		{
			if(CheckAt(ctx, lex_identifier, "ERROR: generic type alias required after '<'"))
			{
				InplaceStr alias = ctx.Consume();

				aliases.push_back(new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), alias));

				while(ctx.Consume(lex_comma))
				{
					if(!CheckAt(ctx, lex_identifier, "ERROR: generic type alias required after ','"))
						break;

					alias = ctx.Consume();

					aliases.push_back(new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), alias));
				}
			}

			CheckConsume(ctx, lex_greater, "ERROR: '>' expected after generic type alias list");
		}

		bool extendable = ctx.Consume(lex_extendable);

		SynBase *baseClass = NULL;

		if(ctx.Consume(lex_colon))
		{
			baseClass = ParseType(ctx);

			if(!baseClass)
				Report(ctx, ctx.Current(), "ERROR: base type name is expected at this point");
		}

		CheckConsume(ctx, lex_ofigure, "ERROR: '{' not found after class name");

		TRACE_SCOPE("parse", "ParseClassBody");

		if(nameIdentifier)
			TRACE_LABEL2(nameIdentifier->name.begin, nameIdentifier->name.end);

		SynClassElements *elements = ParseClassElements(ctx);

		CheckConsume(ctx, lex_cfigure, "ERROR: '}' not found after class definition");

		return new (ctx.get<SynClassDefinition>()) SynClassDefinition(start, ctx.Previous(), align, nameIdentifier, aliases, extendable, baseClass, elements);
	}

	// Backtrack
	ctx.currentLexeme = start;

	return NULL;
}

SynEnumDefinition* ParseEnumDefinition(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_enum))
	{
		SynIdentifier *nameIdentifier = NULL;

		if(CheckAt(ctx, lex_identifier, "ERROR: enum name expected"))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else
		{
			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(InplaceStr());
		}

		CheckConsume(ctx, lex_ofigure, "ERROR: '{' not found after enum name");

		IntrusiveList<SynConstant> values;

		do
		{
			if(values.empty())
			{
				if(!CheckAt(ctx, lex_identifier, "ERROR: enumeration name expected after '{'"))
					break;
			}
			else
			{
				if(!CheckAt(ctx, lex_identifier, "ERROR: enumeration name expected after ','"))
					break;
			}

			Lexeme *pos = ctx.currentLexeme;

			InplaceStr name = ctx.Consume();
			SynIdentifier *nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);

			SynBase *value = NULL;

			if(ctx.Consume(lex_set))
			{
				value = ParseTernaryExpr(ctx);

				if(!value)
				{
					Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

					value = new (ctx.get<SynError>()) SynError(start, ctx.Previous());
				}
			}

			values.push_back(new (ctx.get<SynConstant>()) SynConstant(pos, ctx.Previous(), nameIdentifier, value));
		}
		while(ctx.Consume(lex_comma));

		CheckConsume(ctx, lex_cfigure, "ERROR: '}' not found after enum definition");

		return new (ctx.get<SynEnumDefinition>()) SynEnumDefinition(start, ctx.Previous(), nameIdentifier, values);
	}

	return NULL;
}

SynNamespaceDefinition* ParseNamespaceDefinition(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_namespace))
	{
		IntrusiveList<SynIdentifier> path;

		if(CheckAt(ctx, lex_identifier, "ERROR: namespace name required"))
		{
			InplaceStr value = ctx.Consume();
			path.push_back(new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), value));
		}

		while(ctx.Consume(lex_point))
		{
			if(!CheckAt(ctx, lex_identifier, "ERROR: namespace name required after '.'"))
				break;

			InplaceStr value = ctx.Consume();
			path.push_back(new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), value));
		}

		CheckConsume(ctx, lex_ofigure, "ERROR: '{' not found after namespace name");

		SynNamespaceElement *currentNamespace = ctx.currentNamespace;

		for(SynIdentifier *el = path.head; el; el = (SynIdentifier*)el->next)
			ctx.PushNamespace(el);

		IntrusiveList<SynBase> code = ParseExpressions(ctx);

		ctx.currentNamespace = currentNamespace;

		CheckConsume(ctx, lex_cfigure, "ERROR: '}' not found after namespace body");

		return new (ctx.get<SynNamespaceDefinition>()) SynNamespaceDefinition(start, ctx.Previous(), path, code);
	}

	return NULL;
}

SynReturn* ParseReturn(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_return))
	{
		// Optional
		SynBase *value = ParseAssignment(ctx);

		CheckConsume(ctx, lex_semicolon, "ERROR: return statement must be followed by an expression or ';'");

		return new (ctx.get<SynReturn>()) SynReturn(start, ctx.Previous(), value);
	}

	return NULL;
}

SynYield* ParseYield(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_yield))
	{
		// Optional
		SynBase *value = ParseAssignment(ctx);

		CheckConsume(ctx, lex_semicolon, "ERROR: yield statement must be followed by an expression or ';'");

		return new (ctx.get<SynYield>()) SynYield(start, ctx.Previous(), value);
	}

	return NULL;
}

SynBreak* ParseBreak(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_break))
	{
		// Optional
		SynNumber *node = ParseNumber(ctx);

		CheckConsume(ctx, lex_semicolon, "ERROR: break statement must be followed by ';' or a constant");

		return new (ctx.get<SynBreak>()) SynBreak(start, ctx.Previous(), node);
	}

	return NULL;
}

SynContinue* ParseContinue(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_continue))
	{
		// Optional
		SynNumber *node = ParseNumber(ctx);

		CheckConsume(ctx, lex_semicolon, "ERROR: continue statement must be followed by ';' or a constant");

		return new (ctx.get<SynContinue>()) SynContinue(start, ctx.Previous(), node);
	}

	return NULL;
}

SynTypedef* ParseTypedef(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_typedef))
	{
		SynBase *type = ParseType(ctx);

		if(!type)
		{
			Report(ctx, ctx.Current(), "ERROR: typename expected after typedef");

			type = new (ctx.get<SynError>()) SynError(start, ctx.Previous());
		}

		SynIdentifier *aliasIdentifier = NULL;

		if(CheckAt(ctx, lex_identifier, "ERROR: alias name expected after typename in typedef expression"))
		{
			InplaceStr alias = ctx.Consume();
			aliasIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), alias);
		}
		else
		{
			aliasIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(InplaceStr());
		}

		CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after typedef");

		return new (ctx.get<SynTypedef>()) SynTypedef(start, ctx.Previous(), type, aliasIdentifier);
	}

	return NULL;
}

SynBlock* ParseBlock(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_ofigure))
	{
		const unsigned blockLimit = 256;

		if(ctx.statementBlockDepth >= blockLimit)
			Stop(ctx, ctx.Current(), "ERROR: reached nested '{' limit of %d", blockLimit);

		ctx.statementBlockDepth++;

		IntrusiveList<SynBase> expressions = ParseExpressions(ctx);

		ctx.statementBlockDepth--;

		CheckConsume(ctx, lex_cfigure, "ERROR: closing '}' not found");

		return new (ctx.get<SynBlock>()) SynBlock(start, ctx.Previous(), expressions);
	}

	return NULL;
}

SynIfElse* ParseIfElse(ParseContext &ctx, bool forceStaticIf)
{
	Lexeme *start = ctx.currentLexeme;

	bool staticIf = forceStaticIf || ctx.Consume(lex_at);

	if(ctx.Consume(lex_if))
	{
		CheckConsume(ctx, lex_oparen, "ERROR: '(' not found after 'if'");

		Lexeme *conditionPos = ctx.currentLexeme;

		SynBase *condition = NULL;

		if(!staticIf)
		{
			if(SynBase *type = ParseType(ctx))
			{
				IntrusiveList<SynVariableDefinition> definitions;

				if(SynVariableDefinition *definition = ParseVariableDefinition(ctx))
				{
					definitions.push_back(definition);

					condition = new (ctx.get<SynVariableDefinitions>()) SynVariableDefinitions(start, ctx.Previous(), NULL, type, definitions);
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

			condition = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, lex_cparen, "ERROR: closing ')' not found after 'if' condition");

		SynBase *trueBlock = ParseExpression(ctx);

		if(!trueBlock)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after 'if'");

			trueBlock = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		SynBase *falseBlock = NULL;

		if(staticIf && ctx.At(lex_semicolon))
			ctx.Skip();

		if(ctx.Consume(lex_else))
		{
			if(ctx.At(lex_if))
				falseBlock = ParseIfElse(ctx, staticIf);
			else
				falseBlock = ParseExpression(ctx);

			if(!falseBlock)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after 'else'");

				falseBlock = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}

			if(staticIf && ctx.At(lex_semicolon))
				ctx.Skip();
		}

		return new (ctx.get<SynIfElse>()) SynIfElse(start, ctx.Previous(), staticIf, condition, trueBlock, falseBlock);
	}

	if(staticIf)
	{
		// Backtrack
		ctx.currentLexeme = start;
	}

	return NULL;
}

SynForEachIterator* ParseForEachIterator(ParseContext &ctx, bool isFirst)
{
	Lexeme *start = ctx.currentLexeme;

	// Optional type
	SynBase *type = ParseType(ctx);

	// Must be followed by a type
	if(!ctx.At(lex_identifier))
	{
		// Backtrack
		ctx.currentLexeme = start;

		type = NULL;
	}

	if(ctx.At(lex_identifier))
	{
		InplaceStr name = ctx.Consume();
		SynIdentifier *nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		if(isFirst && !ctx.At(lex_in))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return NULL;
		}

		CheckConsume(ctx, lex_in, "ERROR: 'in' expected after variable name");

		SynBase *value = ParseTernaryExpr(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression expected after 'in'");

			value = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		return new (ctx.get<SynForEachIterator>()) SynForEachIterator(start, ctx.Previous(), type, nameIdentifier, value);
	}

	return NULL;
}

SynForEach* ParseForEach(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_for))
	{
		CheckConsume(ctx, lex_oparen, "ERROR: '(' not found after 'for'");

		IntrusiveList<SynForEachIterator> iterators;

		SynForEachIterator *iterator = ParseForEachIterator(ctx, true);

		if(!iterator)
		{
			// Backtrack
			ctx.currentLexeme = start;

			return NULL;
		}

		iterators.push_back(iterator);

		while(ctx.Consume(lex_comma))
		{
			iterator = ParseForEachIterator(ctx, false);

			if(!iterator)
			{
				Report(ctx, ctx.Current(), "ERROR: variable name or type expected before 'in'");

				break;
			}

			iterators.push_back(iterator);
		}

		CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after 'for' statement");

		SynBase *body = ParseExpression(ctx);

		if(!body)
		{
			Report(ctx, ctx.Current(), "ERROR: body not found after 'for' header");

			body = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		return new (ctx.get<SynForEach>()) SynForEach(start, ctx.Previous(), iterators, body);
	}

	return NULL;
}

SynFor* ParseFor(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_for))
	{
		CheckConsume(ctx, lex_oparen, "ERROR: '(' not found after 'for'");

		SynBase *initializer = NULL;

		if(ctx.At(lex_ofigure))
		{
			initializer = ParseBlock(ctx);

			CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after initializer in 'for'");
		}
		else if(SynBase *node = ParseVariableDefinitions(ctx, false))
		{
			initializer = node;
		}
		else if(SynBase *node = ParseAssignment(ctx))
		{
			initializer = node;

			CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after initializer in 'for'");
		}
		else if(!ctx.Consume(lex_semicolon))
		{
			Report(ctx, ctx.Current(), "ERROR: ';' not found after initializer in 'for'");
		}

		SynBase *condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: condition not found in 'for' statement");

			condition = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after condition in 'for'");

		SynBase *increment = NULL;

		if(ctx.At(lex_ofigure))
			increment = ParseBlock(ctx);
		else if(SynBase *node = ParseAssignment(ctx))
			increment = node;

		CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after 'for' statement");

		SynBase *body = ParseExpression(ctx);

		if(!body)
		{
			Report(ctx, ctx.Current(), "ERROR: body not found after 'for' header");

			body = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		return new (ctx.get<SynFor>()) SynFor(start, ctx.Previous(), initializer, condition, increment, body);
	}

	return NULL;
}

SynWhile* ParseWhile(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_while))
	{
		CheckConsume(ctx, lex_oparen, "ERROR: '(' not found after 'while'");

		SynBase *condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: expression expected after 'while('");

			condition = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, lex_cparen, "ERROR: closing ')' not found after expression in 'while' statement");

		SynBase *body = ParseExpression(ctx);

		if(!body && !ctx.Consume(lex_semicolon))
			Report(ctx, ctx.Current(), "ERROR: body not found after 'while' header");

		return new (ctx.get<SynWhile>()) SynWhile(start, ctx.Previous(), condition, body);
	}

	return NULL;
}

SynDoWhile* ParseDoWhile(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_do))
	{
		IntrusiveList<SynBase> expressions;

		if(ctx.Consume(lex_ofigure))
		{
			expressions = ParseExpressions(ctx);

			CheckConsume(ctx, lex_cfigure, "ERROR: closing '}' not found");
		}
		else if(SynBase *body = ParseExpression(ctx))
		{
			expressions.push_back(body);
		}
		else
		{
			Report(ctx, ctx.Current(), "ERROR: expression expected after 'do'");

			expressions.push_back(new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current()));
		}

		CheckConsume(ctx, lex_while, "ERROR: 'while' expected after 'do' statement");

		CheckConsume(ctx, lex_oparen, "ERROR: '(' not found after 'while'");

		SynBase *condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: expression expected after 'while('");

			condition = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, lex_cparen, "ERROR: closing ')' not found after expression in 'while' statement");

		CheckConsume(ctx, lex_semicolon, "ERROR: while(...) should be followed by ';'");

		return new (ctx.get<SynDoWhile>()) SynDoWhile(start, ctx.Previous(), expressions, condition);
	}

	return NULL;
}

SynSwitch* ParseSwitch(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_switch))
	{
		CheckConsume(ctx, lex_oparen, "ERROR: '(' not found after 'switch'");

		SynBase *condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after 'switch('");

			condition = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, lex_cparen, "ERROR: closing ')' not found after expression in 'switch' statement");

		CheckConsume(ctx, lex_ofigure, "ERROR: '{' not found after 'switch(...)'");

		bool hadDefautltCase = false;

		IntrusiveList<SynSwitchCase> cases;

		while(ctx.At(lex_case) || ctx.At(lex_default))
		{
			Lexeme *pos = ctx.currentLexeme;

			SynBase *value = NULL;

			if(ctx.Consume(lex_case))
			{
				if(hadDefautltCase)
					Report(ctx, ctx.Current(), "ERROR: default switch case can't be followed by more cases");

				value = ParseAssignment(ctx);

				if(!value)
				{
					Report(ctx, ctx.Current(), "ERROR: expression expected after 'case'");

					value = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
				}
			}
			else
			{
				if(hadDefautltCase)
					Report(ctx, ctx.Current(), "ERROR: default switch case is already defined");

				ctx.Skip();

				hadDefautltCase = true;
			}

			CheckConsume(ctx, lex_colon, "ERROR: ':' expected");

			IntrusiveList<SynBase> expressions = ParseExpressions(ctx);

			cases.push_back(new (ctx.get<SynSwitchCase>()) SynSwitchCase(pos, ctx.Previous(), value, expressions));
		}

		CheckConsume(ctx, lex_cfigure, "ERROR: '}' not found after 'switch' statement");

		return new (ctx.get<SynSwitch>()) SynSwitch(start, ctx.Previous(), condition, cases);
	}

	return NULL;
}

SynVariableDefinition* ParseVariableDefinition(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.At(lex_identifier))
	{
		InplaceStr name = ctx.Consume();
		SynIdentifier *nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		if(name.length() >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			Stop(ctx, ctx.Current(), "ERROR: variable name length is limited to %d symbols", NULLC_MAX_VARIABLE_NAME_LENGTH);

		SynBase *initializer = NULL;

		if(ctx.Consume(lex_set))
		{
			initializer = ParseAssignment(ctx);

			if(!initializer)
			{
				Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

				initializer = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}
		}

		return new (ctx.get<SynVariableDefinition>()) SynVariableDefinition(start, ctx.Previous(), nameIdentifier, initializer);
	}

	return NULL;
}

SynVariableDefinitions* ParseVariableDefinitions(ParseContext &ctx, bool classMembers)
{
	Lexeme *start = ctx.currentLexeme;

	SynAlign *align = ParseAlign(ctx);

	if(SynBase *type = ParseType(ctx))
	{
		IntrusiveList<SynVariableDefinition> definitions;

		SynVariableDefinition *definition = ParseVariableDefinition(ctx);

		if(!definition)
		{
			if(align)
				Report(ctx, ctx.Current(), "ERROR: variable or class definition is expected after alignment specifier");

			// Backtrack
			ctx.currentLexeme = start;

			return NULL;
		}

		definitions.push_back(definition);

		while(ctx.Consume(lex_comma))
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
			CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after class member list");
		}
		else if(!ctx.Consume(lex_semicolon))
		{
			if(ctx.Peek() == lex_obracket)
				CheckConsume(ctx, lex_semicolon, "ERROR: array size must be specified after type name");
			else
				CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after variable definition");
		}

		return new (ctx.get<SynVariableDefinitions>()) SynVariableDefinitions(start, ctx.Previous(), align, type, definitions);
	}
	
	// Backtrack
	ctx.currentLexeme = start;

	return NULL;
}

SynAccessor* ParseAccessorDefinition(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(SynBase *type = ParseType(ctx))
	{
		if(!CheckAt(ctx, lex_identifier, "ERROR: class member name expected after type"))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return NULL;
		}

		InplaceStr name = ctx.Consume();
		SynIdentifier *nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);

		if(!ctx.Consume(lex_ofigure))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return NULL;
		}

		CheckConsume(ctx, "get", "ERROR: 'get' is expected after '{'");

		SynBase *getBlock = ParseBlock(ctx);

		if(!getBlock)
		{
			Report(ctx, ctx.Current(), "ERROR: function body expected after 'get'");

			getBlock = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		SynBase *setBlock = NULL;
		SynIdentifier *setNameIdentifier = NULL;

		if(ctx.Consume("set"))
		{
			if(ctx.Consume(lex_oparen))
			{
				if(CheckAt(ctx, lex_identifier, "ERROR: r-value name not found after '('"))
				{
					InplaceStr setName = ctx.Consume();
					setNameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), setName);
				}

				CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after r-value");
			}

			setBlock = ParseBlock(ctx);

			if(!setBlock)
			{
				Report(ctx, ctx.Current(), "ERROR: function body expected after 'set'");

				setBlock = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}
		}

		CheckConsume(ctx, lex_cfigure, "ERROR: '}' is expected after property");

		CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after class member list");

		return new (ctx.get<SynAccessor>()) SynAccessor(start, ctx.Previous(), type, nameIdentifier, getBlock, setBlock, setNameIdentifier);
	}

	return NULL;
}

SynConstantSet* ParseConstantSet(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_const))
	{
		SynBase *type = ParseType(ctx);

		if(!type)
		{
			Report(ctx, ctx.Current(), "ERROR: type name expected after const");

			type = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		IntrusiveList<SynConstant> constantSet;

		Lexeme *pos = ctx.currentLexeme;

		SynIdentifier *nameIdentifier = NULL;

		if(CheckAt(ctx, lex_identifier, "ERROR: constant name expected after type"))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else
		{
			pos = ctx.Previous();

			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(InplaceStr());
		}

		CheckConsume(ctx, lex_set, "ERROR: '=' not found after constant name");

		SynBase *value = ParseTernaryExpr(ctx);

		if(!value)
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

			value = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		constantSet.push_back(new (ctx.get<SynConstant>()) SynConstant(pos, ctx.Previous(), nameIdentifier, value));

		while(ctx.Consume(lex_comma))
		{
			pos = ctx.currentLexeme;

			if(CheckAt(ctx, lex_identifier, "ERROR: constant name expected after ','"))
			{
				InplaceStr name = ctx.Consume();
				nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else
			{
				pos = ctx.Previous();

				nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(InplaceStr());
			}

			value = NULL;

			if(ctx.Consume(lex_set))
			{
				value = ParseTernaryExpr(ctx);

				if(!value)
				{
					Report(ctx, ctx.Current(), "ERROR: expression not found after '='");

					value = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
				}
			}

			constantSet.push_back(new (ctx.get<SynConstant>()) SynConstant(pos, ctx.Previous(), nameIdentifier, value));
		}

		CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after constants");

		return new (ctx.get<SynConstantSet>()) SynConstantSet(start, ctx.Previous(), type, constantSet);
	}

	return NULL;
}

SynClassStaticIf* ParseClassStaticIf(ParseContext &ctx, bool nested)
{
	Lexeme *start = ctx.currentLexeme;

	if(nested || ctx.Consume(lex_at))
	{
		if(!ctx.Consume(lex_if))
		{
			// Backtrack
			ctx.currentLexeme = start;

			return NULL;
		}

		CheckConsume(ctx, lex_oparen, "ERROR: '(' not found after 'if'");

		SynBase *condition = ParseAssignment(ctx);

		if(!condition)
		{
			Report(ctx, ctx.Current(), "ERROR: condition not found in 'if' statement");

			condition = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
		}

		CheckConsume(ctx, lex_cparen, "ERROR: closing ')' not found after 'if' condition");

		bool hasBlock = ctx.Consume(lex_ofigure);

		SynClassElements *trueBlock = ParseClassElements(ctx);

		if(hasBlock)
			CheckConsume(ctx, lex_cfigure, "ERROR: closing '}' not found");

		SynClassElements *falseBlock = NULL;

		if(ctx.Consume(lex_else))
		{
			if(ctx.At(lex_if))
			{
				Lexeme *pos = ctx.currentLexeme;

				IntrusiveList<SynClassStaticIf> staticIfs;

				staticIfs.push_back(ParseClassStaticIf(ctx, true));

				falseBlock = new (ctx.get<SynClassElements>()) SynClassElements(pos, ctx.Previous(), IntrusiveList<SynTypedef>(), IntrusiveList<SynFunctionDefinition>(), IntrusiveList<SynAccessor>(), IntrusiveList<SynVariableDefinitions>(), IntrusiveList<SynConstantSet>(), staticIfs);
			}
			else
			{
				hasBlock = ctx.Consume(lex_ofigure);

				falseBlock = ParseClassElements(ctx);

				if(hasBlock)
					CheckConsume(ctx, lex_cfigure, "ERROR: closing '}' not found");
			}
		}

		return new (ctx.get<SynClassStaticIf>()) SynClassStaticIf(start, ctx.Previous(), condition, trueBlock, falseBlock);
	}

	return NULL;
}

SynFunctionArgument* ParseFunctionArgument(ParseContext &ctx, bool lastExplicit, SynBase *lastType)
{
	Lexeme *start = ctx.currentLexeme;

	bool isExplicit = ctx.Consume("explicit");

	if(SynBase *type = ParseType(ctx))
	{
		if(!ctx.At(lex_identifier) && lastType)
		{
			if(isExplicit)
				Stop(ctx, ctx.Current(), "ERROR: variable name not found after type in function variable list");

			// Backtrack
			ctx.currentLexeme = start;

			isExplicit = lastExplicit;
			type = lastType;
		}

		SynIdentifier *nameIdentifier = NULL;

		if(CheckAt(ctx, lex_identifier, "ERROR: variable name not found after type in function variable list"))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else
		{
			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(InplaceStr());
		}

		SynBase *initializer = NULL;

		if(ctx.Consume(lex_set))
		{
			initializer = ParseTernaryExpr(ctx);

			if(!initializer)
			{
				Report(ctx, ctx.Current(), "ERROR: default argument value not found after '='");

				initializer = new (ctx.get<SynError>()) SynError(ctx.Current(), ctx.Current());
			}
		}

		return new (ctx.get<SynFunctionArgument>()) SynFunctionArgument(start, start == ctx.currentLexeme ? ctx.Current() : ctx.Previous(), isExplicit, type, nameIdentifier, initializer);
	}

	if(isExplicit)
		Stop(ctx, ctx.Current(), "ERROR: type name not found after 'explicit' specifier");

	return NULL;
}

IntrusiveList<SynFunctionArgument> ParseFunctionArguments(ParseContext &ctx)
{
	IntrusiveList<SynFunctionArgument> arguments;

	if(SynFunctionArgument *argument = ParseFunctionArgument(ctx, false, NULL))
	{
		arguments.push_back(argument);

		while(ctx.Consume(lex_comma))
		{
			argument = ParseFunctionArgument(ctx, arguments.tail->isExplicit, arguments.tail->type);

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

SynFunctionDefinition* ParseFunctionDefinition(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.nonFunctionDefinitionLocations.find(unsigned(start - ctx.firstLexeme) + 1))
		return NULL;

	bool coroutine = ctx.Consume(lex_coroutine);

	if(SynBase *returnType = ParseType(ctx))
	{
		// Check if this is a member function
		Lexeme *subLexeme = ctx.currentLexeme;

		SynBase *parentType = ParseType(ctx);
		bool accessor = false;

		if(parentType)
		{
			if(ctx.Consume(lex_dblcolon))
			{
				accessor = false;
			}
			else if(ctx.Consume(lex_point))
			{
				accessor = true;
			}
			else
			{
				// Backtrack
				ctx.currentLexeme = subLexeme;

				parentType = NULL;
			}
		}

		bool allowEmptyName = isType<SynTypeAuto>(returnType);

		bool isOperator = ctx.Consume(lex_operator);

		SynIdentifier *nameIdentifier = NULL;

		if(isOperator)
		{
			if(ctx.Consume(lex_obracket))
			{
				CheckConsume(ctx, lex_cbracket, "ERROR: ']' not found after '[' in operator definition");

				InplaceStr name = InplaceStr("[]");
				nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else if(ctx.Consume(lex_oparen))
			{
				CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after '(' in operator definition");

				InplaceStr name = InplaceStr("()");
				nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else if((ctx.Peek() >= lex_add && ctx.Peek() <= lex_in) || (ctx.Peek() >= lex_set && ctx.Peek() <= lex_xorset) || ctx.Peek() == lex_bitnot || ctx.Peek() == lex_lognot)
			{
				InplaceStr name = ctx.Consume();
				nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else
			{
				Stop(ctx, ctx.Current(), "ERROR: invalid operator name");
			}
		}
		else if(ctx.At(lex_identifier))
		{
			InplaceStr name = ctx.Consume();
			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
		}
		else if(parentType)
		{
			Stop(ctx, ctx.Current(), "ERROR: function name expected after ':' or '.'");
		}
		else if(coroutine && !allowEmptyName)
		{
			Stop(ctx, ctx.Current(), "ERROR: function name not found after return type");
		}

		IntrusiveList<SynIdentifier> aliases;

		if(nameIdentifier && ctx.Consume(lex_less))
		{
			do
			{
				if(aliases.empty())
					CheckConsume(ctx, lex_at, "ERROR: '@' is expected before explicit generic type alias");
				else
					CheckConsume(ctx, lex_at, "ERROR: '@' is expected after ',' in explicit generic type alias list");

				if(CheckAt(ctx, lex_identifier, "ERROR: explicit generic type alias is expected after '@'"))
				{
					InplaceStr name = ctx.Consume();
					aliases.push_back(new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name));
				}
			}
			while(ctx.Consume(lex_comma));

			CheckConsume(ctx, lex_greater, "ERROR: '>' not found after explicit generic type alias list");
		}

		if(parentType || coroutine || !aliases.empty())
			CheckAt(ctx, lex_oparen, "ERROR: '(' expected after function name");

		if((nameIdentifier == NULL && !allowEmptyName) || !ctx.Consume(lex_oparen))
		{
			// Backtrack
			ctx.currentLexeme = start;
			ctx.nonFunctionDefinitionLocations.insert(unsigned(start - ctx.firstLexeme) + 1, true);

			return NULL;
		}

		IntrusiveList<SynFunctionArgument> arguments = ParseFunctionArguments(ctx);

		CheckConsume(ctx, lex_cparen, "ERROR: ')' not found after function variable list");

		if(!nameIdentifier)
			nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(InplaceStr());

		IntrusiveList<SynBase> expressions;

		if(ctx.Consume(lex_semicolon))
			return new (ctx.get<SynFunctionDefinition>()) SynFunctionDefinition(start, ctx.Previous(), true, coroutine, parentType, accessor, returnType, isOperator, nameIdentifier, aliases, arguments, expressions);

		CheckConsume(ctx, lex_ofigure, "ERROR: '{' not found after function header");

		TRACE_SCOPE("parse", "ParseFunctionBody");

		if(nameIdentifier)
			TRACE_LABEL2(nameIdentifier->name.begin, nameIdentifier->name.end);

		expressions = ParseExpressions(ctx);

		CheckConsume(ctx, lex_cfigure, "ERROR: '}' not found after function body");

		return new (ctx.get<SynFunctionDefinition>()) SynFunctionDefinition(start, ctx.Previous(), false, coroutine, parentType, accessor, returnType, isOperator, nameIdentifier, aliases, arguments, expressions);
	}
	else if(coroutine)
	{
		Stop(ctx, ctx.Current(), "ERROR: function return type not found after 'coroutine'");
	}

	return NULL;
}

SynShortFunctionDefinition* ParseShortFunctionDefinition(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_less))
	{
		IntrusiveList<SynShortFunctionArgument> arguments;

		bool isFirst = true;

		do
		{
			if(isFirst && ctx.At(lex_greater))
				break;

			isFirst = false;

			Lexeme *pos = ctx.currentLexeme;

			Lexeme *lexeme = ctx.currentLexeme;

			SynBase *type = ParseType(ctx);

			if(!ctx.At(lex_identifier))
			{
				// Backtrack
				ctx.currentLexeme = lexeme;

				type = NULL;
			}

			bool hasName = false;

			if(arguments.empty())
				hasName = CheckAt(ctx, lex_identifier, "ERROR: function argument name not found after '<'");
			else
				hasName = CheckAt(ctx, lex_identifier, "ERROR: function argument name not found after ','");

			SynIdentifier *nameIdentifier = NULL;

			if(hasName)
			{
				InplaceStr name = ctx.Consume();
				nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), name);
			}
			else
			{
				pos = ctx.Previous();

				nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(InplaceStr());
			}

			arguments.push_back(new (ctx.get<SynShortFunctionArgument>()) SynShortFunctionArgument(pos, ctx.Previous(), type, nameIdentifier));
		}
		while(ctx.Consume(lex_comma));

		CheckConsume(ctx, lex_greater, "ERROR: '>' expected after short inline function argument list");

		IntrusiveList<SynBase> expressions;

		if(ctx.Consume(lex_ofigure))
		{
			expressions = ParseExpressions(ctx);

			CheckConsume(ctx, lex_cfigure, "ERROR: '}' not found after function body");
		}
		else if(SynBase *body = ParseStatement(ctx))
		{
			expressions.push_back(body);
		}
		else if(SynBase *body = ParseAssignment(ctx))
		{
			expressions.push_back(body);
		}
		else
		{
			Report(ctx, ctx.Current(), "ERROR: expression not found after function header");
		}

		return new (ctx.get<SynShortFunctionDefinition>()) SynShortFunctionDefinition(start, ctx.Previous(), arguments, expressions);
	}

	return NULL;
}

SynBase* ParseStatement(ParseContext &ctx)
{
	if(SynBase *node = ParseClassDefinition(ctx))
		return node;

	if(SynBase *node = ParseEnumDefinition(ctx))
		return node;

	if(SynBase *node = ParseNamespaceDefinition(ctx))
		return node;

	if(SynBase *node = ParseReturn(ctx))
		return node;

	if(SynBase *node = ParseYield(ctx))
		return node;

	if(SynBase *node = ParseBreak(ctx))
		return node;

	if(SynBase *node = ParseContinue(ctx))
		return node;

	if(SynBase *node = ParseTypedef(ctx))
		return node;

	if(SynBase *node = ParseBlock(ctx))
		return node;

	if(SynBase *node = ParseIfElse(ctx, false))
		return node;

	if(SynBase *node = ParseForEach(ctx))
		return node;

	if(SynBase *node = ParseFor(ctx))
		return node;

	if(SynBase *node = ParseWhile(ctx))
		return node;

	if(SynBase *node = ParseDoWhile(ctx))
		return node;

	if(SynBase *node = ParseSwitch(ctx))
		return node;

	if(SynBase *node = ParseFunctionDefinition(ctx))
		return node;

	if(SynBase *node = ParseVariableDefinitions(ctx, false))
		return node;

	return NULL;
}

SynBase* ParseExpression(ParseContext &ctx)
{
	if(SynBase *node = ParseStatement(ctx))
		return node;

	if(SynBase *node = ParseAssignment(ctx))
	{
		if(ctx.Peek() == lex_none && ctx.Current() != ctx.Last())
			Report(ctx, ctx.Current(), "ERROR: unknown lexeme");

		CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after expression");

		return node;
	}

	return NULL;
}

IntrusiveList<SynBase> ParseExpressions(ParseContext &ctx)
{
	IntrusiveList<SynBase> expressions;

	while(SynBase* expression = ParseExpression(ctx))
		expressions.push_back(expression);

	return expressions;
}

const char* GetBytecodeFromPath(ParseContext &ctx, Lexeme *start, IntrusiveList<SynIdentifier> parts)
{
	ctx.statistics.Start(NULLCTime::clockMicro());

	InplaceStr moduleName = GetModuleName(ctx.allocator, ctx.moduleRoot, parts);

	TRACE_SCOPE("parser", "GetBytecodeFromPath");
	TRACE_LABEL2(moduleName.begin, moduleName.end);

	const char *bytecode = BinaryCache::FindBytecode(moduleName.begin, false);

	if(!bytecode)
	{
		moduleName = GetModuleName(ctx.allocator, NULL, parts);

		bytecode = BinaryCache::FindBytecode(moduleName.begin, false);
	}

	for(unsigned i = 0; i < ctx.activeImports.size(); i++)
	{
		if(ctx.activeImports[i] == moduleName)
			Stop(ctx, start, "ERROR: found cyclic dependency on module '%.*s'", moduleName.length(), moduleName.begin);
	}

	ctx.activeImports.push_back(moduleName);

	if(!bytecode)
	{
		if(!ctx.bytecodeBuilder)
			Stop(ctx, start, "ERROR: import builder is not provided");

		if(ctx.errorCount == 0)
		{
			ctx.errorPos = start->pos;
			ctx.errorBufLocation = ctx.errorBuf;
		}

		const char *messageStart = ctx.errorBufLocation;

		// Separate allocator for each module
		ChunkedStackPool<65532> pool;
		GrowingAllocatorRef<ChunkedStackPool<65532>, 16384> allocator(pool);

		ctx.statistics.Finish("Extra", NULLCTime::clockMicro());

		const char *pos = NULL;
		bytecode = ctx.bytecodeBuilder(&allocator, moduleName, ctx.moduleRoot, false, &pos, ctx.errorBufLocation, ctx.errorBufSize - unsigned(ctx.errorBufLocation - ctx.errorBuf), ctx.optimizationLevel, ctx.activeImports, &ctx.statistics);

		ctx.statistics.Start(NULLCTime::clockMicro());

		if(!bytecode)
		{
			if(ctx.errorBuf && ctx.errorBufSize)
			{
				ctx.errorBufLocation += strlen(ctx.errorBufLocation);

				const char *messageEnd = ctx.errorBufLocation;

				ctx.errorInfo.push_back(new (ctx.get<ErrorInfo>()) ErrorInfo(ctx.allocator, messageStart, messageEnd, start, start, pos));

				AddErrorLocationInfo(ctx.code, pos, ctx.errorBufLocation, ctx.errorBufSize - unsigned(ctx.errorBufLocation - ctx.errorBuf));

				ctx.errorBufLocation += strlen(ctx.errorBufLocation);
			}

			ctx.errorCount++;
		}
	}

	ctx.statistics.Finish("Extra", NULLCTime::clockMicro());

	return bytecode;
}

void ImportModuleNamespaces(ParseContext &ctx, Lexeme *pos, ByteCode *bCode)
{
	TRACE_SCOPE("parser", "ImportModuleNamespaces");

	char *symbols = FindSymbols(bCode);

	// Import namespaces
	ExternNamespaceInfo *namespaceList = FindFirstNamespace(bCode);

	for(unsigned i = 0; i < bCode->namespaceCount; i++)
	{
		ExternNamespaceInfo &ns = namespaceList[i];

		const char *name = symbols + ns.offsetToName;

		SynNamespaceElement *parent = NULL;

		if(ns.parentHash != ~0u)
		{
			for(unsigned k = 0; k < ctx.namespaceList.size(); k++)
			{
				if(ctx.namespaceList[k]->fullNameHash == ns.parentHash)
				{
					parent = ctx.namespaceList[k];
					break;
				}
			}

			if(!parent)
				Stop(ctx, pos, "ERROR: namespace %s parent not found", name);
		}

		SynIdentifier *nameIdentifier = new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), InplaceStr(name));

		ctx.namespaceList.push_back(new (ctx.get<SynNamespaceElement>()) SynNamespaceElement(parent, nameIdentifier));
	}
}

SynModuleImport* ParseImport(ParseContext &ctx)
{
	Lexeme *start = ctx.currentLexeme;

	if(ctx.Consume(lex_import))
	{
		IntrusiveList<SynIdentifier> path;

		if(CheckAt(ctx, lex_identifier, "ERROR: name expected after import"))
		{
			InplaceStr value = ctx.Consume();
			path.push_back(new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), value));
		}

		while(ctx.Consume(lex_point))
		{
			if(CheckAt(ctx, lex_identifier, "ERROR: name expected after '.'"))
			{
				InplaceStr value = ctx.Consume();
				path.push_back(new (ctx.get<SynIdentifier>()) SynIdentifier(ctx.Previous(), ctx.Previous(), value));
			}
		}

		CheckConsume(ctx, lex_semicolon, "ERROR: ';' not found after import expression");

		if(const char *binary = GetBytecodeFromPath(ctx, start, path))
		{
			ImportModuleNamespaces(ctx, start, (ByteCode*)binary);

			ctx.activeImports.pop_back();

			return new (ctx.get<SynModuleImport>()) SynModuleImport(start, ctx.Previous(), path, (ByteCode*)binary);
		}
	}

	return NULL;
}

IntrusiveList<SynModuleImport> ParseImports(ParseContext &ctx)
{
	IntrusiveList<SynModuleImport> imports;

	while(ctx.At(lex_import))
	{
		if(SynModuleImport *import = ParseImport(ctx))
			imports.push_back(import);
	}

	return imports;
}

SynModule* ParseModule(ParseContext &ctx)
{
	TRACE_SCOPE("parser", "ParseModule");

	Lexeme *start = ctx.currentLexeme;

	// Ignore nested import timing
	ctx.statistics.finishTime = 0;

	IntrusiveList<SynModuleImport> imports = ParseImports(ctx);

	ctx.statistics.Start(NULLCTime::clockMicro());

	IntrusiveList<SynBase> expressions = ParseExpressions(ctx);

	while(!ctx.Consume(lex_none))
	{
		Report(ctx, ctx.Current(), "ERROR: unexpected symbol");

		ctx.Skip();

		while(SynBase* expression = ParseExpression(ctx))
			expressions.push_back(expression);
	}

	ctx.statistics.Finish("Parse", NULLCTime::clockMicro());

	if(expressions.empty())
	{
		Report(ctx, ctx.Current(), "ERROR: module contains no code");

		return new (ctx.get<SynModule>()) SynModule(start, ctx.Current(), imports, expressions);
	}

	return new (ctx.get<SynModule>()) SynModule(start, ctx.Previous(), imports, expressions);
}

SynModule* Parse(ParseContext &ctx, const char *code, const char *moduleRoot)
{
	TRACE_SCOPE("parser", "Parse");

	ctx.code = code;

	ctx.moduleRoot = moduleRoot;

	ctx.statistics.Start(NULLCTime::clockMicro());

	ctx.lexer.Lexify(code);

	ctx.statistics.Finish("Lexer", NULLCTime::clockMicro());

	unsigned traceDepth = NULLC::TraceGetDepth();

	if(!setjmp(ctx.errorHandler))
	{
		assert(ctx.lexer.GetStreamSize() != 0);

		ctx.errorHandlerActive = true;

		ctx.firstLexeme = ctx.lexer.GetStreamStart();
		ctx.currentLexeme = ctx.lexer.GetStreamStart();
		ctx.lastLexeme = ctx.lexer.GetStreamStart() + (ctx.lexer.GetStreamSize() - 1);

		SynModule *module = ParseModule(ctx);

		ctx.errorHandlerActive = false;

		ctx.code = NULL;

		ctx.moduleRoot = NULL;

		return module;
	}

	NULLC::TraceLeaveTo(traceDepth);

	assert(ctx.errorPos);

	ctx.code = NULL;

	ctx.moduleRoot = NULL;

	return NULL;
}

void VisitParseTreeNodes(SynBase *syntax, void *context, void(*accept)(void *context, SynBase *child))
{
	if(!syntax)
		return;

	accept(context, syntax);

	if(SynTypeSimple *node = getType<SynTypeSimple>(syntax))
	{
		for(SynIdentifier *part = node->path.head; part; part = getType<SynIdentifier>(part->next))
			VisitParseTreeNodes(part, context, accept);
	}
	else if(SynTypeArray *node = getType<SynTypeArray>(syntax))
	{
		VisitParseTreeNodes(node->type, context, accept);

		for(SynBase *size = node->sizes.head; size; size = size->next)
			VisitParseTreeNodes(size, context, accept);
	}
	else if(SynTypeReference *node = getType<SynTypeReference>(syntax))
	{
		VisitParseTreeNodes(node->type, context, accept);
	}
	else if(SynTypeFunction *node = getType<SynTypeFunction>(syntax))
	{
		VisitParseTreeNodes(node->returnType, context, accept);

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynTypeGenericInstance *node = getType<SynTypeGenericInstance>(syntax))
	{
		VisitParseTreeNodes(node->baseType, context, accept);

		for(SynBase *type = node->types.head; type; type = type->next)
			VisitParseTreeNodes(type, context, accept);
	}
	else if(SynTypeof *node = getType<SynTypeof>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynArray *node = getType<SynArray>(syntax))
	{
		for(SynBase *value = node->values.head; value; value = value->next)
			VisitParseTreeNodes(value, context, accept);
	}
	else if(SynGenerator *node = getType<SynGenerator>(syntax))
	{
		for(SynBase *value = node->expressions.head; value; value = value->next)
			VisitParseTreeNodes(value, context, accept);
	}
	else if(SynAlign *node = getType<SynAlign>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynTypedef *node = getType<SynTypedef>(syntax))
	{
		VisitParseTreeNodes(node->type, context, accept);
	}
	else if(SynMemberAccess *node = getType<SynMemberAccess>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynCallArgument *node = getType<SynCallArgument>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynArrayIndex *node = getType<SynArrayIndex>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynFunctionCall *node = getType<SynFunctionCall>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);

		for(SynBase *arg = node->aliases.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynPreModify *node = getType<SynPreModify>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynPostModify *node = getType<SynPostModify>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynGetAddress *node = getType<SynGetAddress>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynDereference *node = getType<SynDereference>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynSizeof *node = getType<SynSizeof>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynNew *node = getType<SynNew>(syntax))
	{
		VisitParseTreeNodes(node->type, context, accept);

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		VisitParseTreeNodes(node->count, context, accept);

		for(SynBase *arg = node->constructor.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynConditional *node = getType<SynConditional>(syntax))
	{
		VisitParseTreeNodes(node->condition, context, accept);
		VisitParseTreeNodes(node->trueBlock, context, accept);
		VisitParseTreeNodes(node->falseBlock, context, accept);
	}
	else if(SynReturn *node = getType<SynReturn>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynYield *node = getType<SynYield>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynBreak *node = getType<SynBreak>(syntax))
	{
		VisitParseTreeNodes(node->number, context, accept);
	}
	else if(SynContinue *node = getType<SynContinue>(syntax))
	{
		VisitParseTreeNodes(node->number, context, accept);
	}
	else if(SynBlock *node = getType<SynBlock>(syntax))
	{
		for(SynBase *expr = node->expressions.head; expr; expr = expr->next)
			VisitParseTreeNodes(expr, context, accept);
	}
	else if(SynIfElse *node = getType<SynIfElse>(syntax))
	{
		VisitParseTreeNodes(node->condition, context, accept);
		VisitParseTreeNodes(node->trueBlock, context, accept);
		VisitParseTreeNodes(node->falseBlock, context, accept);
	}
	else if(SynFor *node = getType<SynFor>(syntax))
	{
		VisitParseTreeNodes(node->initializer, context, accept);
		VisitParseTreeNodes(node->condition, context, accept);
		VisitParseTreeNodes(node->increment, context, accept);
		VisitParseTreeNodes(node->body, context, accept);
	}
	else if(SynForEachIterator *node = getType<SynForEachIterator>(syntax))
	{
		VisitParseTreeNodes(node->type, context, accept);
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynForEach *node = getType<SynForEach>(syntax))
	{
		for(SynBase *arg = node->iterators.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		VisitParseTreeNodes(node->body, context, accept);
	}
	else if(SynWhile *node = getType<SynWhile>(syntax))
	{
		VisitParseTreeNodes(node->condition, context, accept);
		VisitParseTreeNodes(node->body, context, accept);
	}
	else if(SynDoWhile *node = getType<SynDoWhile>(syntax))
	{
		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		VisitParseTreeNodes(node->condition, context, accept);
	}
	else if(SynSwitchCase *node = getType<SynSwitchCase>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);

		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynSwitch *node = getType<SynSwitch>(syntax))
	{
		VisitParseTreeNodes(node->condition, context, accept);

		for(SynBase *arg = node->cases.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynUnaryOp *node = getType<SynUnaryOp>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynBinaryOp *node = getType<SynBinaryOp>(syntax))
	{
		VisitParseTreeNodes(node->lhs, context, accept);
		VisitParseTreeNodes(node->rhs, context, accept);
	}
	else if(SynAssignment *node = getType<SynAssignment>(syntax))
	{
		VisitParseTreeNodes(node->lhs, context, accept);
		VisitParseTreeNodes(node->rhs, context, accept);
	}
	else if(SynModifyAssignment *node = getType<SynModifyAssignment>(syntax))
	{
		VisitParseTreeNodes(node->lhs, context, accept);
		VisitParseTreeNodes(node->rhs, context, accept);
	}
	else if(SynVariableDefinition *node = getType<SynVariableDefinition>(syntax))
	{
		VisitParseTreeNodes(node->initializer, context, accept);
	}
	else if(SynVariableDefinitions *node = getType<SynVariableDefinitions>(syntax))
	{
		VisitParseTreeNodes(node->align, context, accept);
		VisitParseTreeNodes(node->type, context, accept);

		for(SynBase *arg = node->definitions.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynAccessor *node = getType<SynAccessor>(syntax))
	{
		VisitParseTreeNodes(node->getBlock, context, accept);
		VisitParseTreeNodes(node->setBlock, context, accept);
	}
	else if(SynFunctionArgument *node = getType<SynFunctionArgument>(syntax))
	{
		VisitParseTreeNodes(node->type, context, accept);
		VisitParseTreeNodes(node->initializer, context, accept);
	}
	else if(SynFunctionDefinition *node = getType<SynFunctionDefinition>(syntax))
	{
		VisitParseTreeNodes(node->parentType, context, accept);
		VisitParseTreeNodes(node->returnType, context, accept);

		for(SynBase *arg = node->aliases.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynShortFunctionArgument *node = getType<SynShortFunctionArgument>(syntax))
	{
		VisitParseTreeNodes(node->type, context, accept);
	}
	else if(SynShortFunctionDefinition *node = getType<SynShortFunctionDefinition>(syntax))
	{
		for(SynBase *arg = node->arguments.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynConstant *node = getType<SynConstant>(syntax))
	{
		VisitParseTreeNodes(node->value, context, accept);
	}
	else if(SynConstantSet *node = getType<SynConstantSet>(syntax))
	{
		VisitParseTreeNodes(node->type, context, accept);

		for(SynBase *arg = node->constants.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynClassStaticIf *node = getType<SynClassStaticIf>(syntax))
	{
		VisitParseTreeNodes(node->condition, context, accept);
		VisitParseTreeNodes(node->trueBlock, context, accept);
		VisitParseTreeNodes(node->falseBlock, context, accept);
	}
	else if(SynClassElements *node = getType<SynClassElements>(syntax))
	{
		for(SynBase *arg = node->typedefs.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		for(SynBase *arg = node->functions.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		for(SynBase *arg = node->accessors.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		for(SynBase *arg = node->members.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		for(SynBase *arg = node->constantSets.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		for(SynBase *arg = node->staticIfs.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynClassDefinition *node = getType<SynClassDefinition>(syntax))
	{
		VisitParseTreeNodes(node->align, context, accept);

		for(SynBase *arg = node->aliases.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);

		VisitParseTreeNodes(node->baseClass, context, accept);
		VisitParseTreeNodes(node->elements, context, accept);
	}
	else if(SynEnumDefinition *node = getType<SynEnumDefinition>(syntax))
	{
		for(SynBase *arg = node->values.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynNamespaceDefinition *node = getType<SynNamespaceDefinition>(syntax))
	{
		for(SynIdentifier *part = node->path.head; part; part = getType<SynIdentifier>(part->next))
			VisitParseTreeNodes(part, context, accept);

		for(SynBase *arg = node->expressions.head; arg; arg = arg->next)
			VisitParseTreeNodes(arg, context, accept);
	}
	else if(SynModuleImport *node = getType<SynModuleImport>(syntax))
	{
		for(SynIdentifier *part = node->path.head; part; part = getType<SynIdentifier>(part->next))
			VisitParseTreeNodes(part, context, accept);
	}
	else if(SynModule *node = getType<SynModule>(syntax))
	{
		for(SynModuleImport *import = node->imports.head; import; import = getType<SynModuleImport>(import->next))
			VisitParseTreeNodes(import, context, accept);

		for(SynBase *expr = node->expressions.head; expr; expr = expr->next)
			VisitParseTreeNodes(expr, context, accept);
	}
}

const char* GetParseTreeNodeName(SynBase *syntax)
{
	switch(syntax->typeID)
	{
	case SynError::myTypeID:
		return "SynError";
	case SynNothing::myTypeID:
		return "SynNothing";
	case SynIdentifier::myTypeID:
		return "SynIdentifier";
	case SynTypeAuto::myTypeID:
		return "SynTypeAuto";
	case SynTypeGeneric::myTypeID:
		return "SynTypeGeneric";
	case SynTypeSimple::myTypeID:
		return "SynTypeSimple";
	case SynTypeAlias::myTypeID:
		return "SynTypeAlias";
	case SynTypeArray::myTypeID:
		return "SynTypeArray";
	case SynTypeReference::myTypeID:
		return "SynTypeReference";
	case SynTypeFunction::myTypeID:
		return "SynTypeFunction";
	case SynTypeGenericInstance::myTypeID:
		return "SynTypeGenericInstance";
	case SynTypeof::myTypeID:
		return "SynTypeof";
	case SynBool::myTypeID:
		return "SynBool";
	case SynNumber::myTypeID:
		return "SynNumber";
	case SynNullptr::myTypeID:
		return "SynNullptr";
	case SynCharacter::myTypeID:
		return "SynCharacter";
	case SynString::myTypeID:
		return "SynString";
	case SynArray::myTypeID:
		return "SynArray";
	case SynGenerator::myTypeID:
		return "SynGenerator";
	case SynAlign::myTypeID:
		return "SynAlign";
	case SynTypedef::myTypeID:
		return "SynTypedef";
	case SynMemberAccess::myTypeID:
		return "SynMemberAccess";
	case SynCallArgument::myTypeID:
		return "SynCallArgument";
	case SynArrayIndex::myTypeID:
		return "SynArrayIndex";
	case SynFunctionCall::myTypeID:
		return "SynFunctionCall";
	case SynPreModify::myTypeID:
		return "SynPreModify";
	case SynPostModify::myTypeID:
		return "SynPostModify";
	case SynGetAddress::myTypeID:
		return "SynGetAddress";
	case SynDereference::myTypeID:
		return "SynDereference";
	case SynSizeof::myTypeID:
		return "SynSizeof";
	case SynNew::myTypeID:
		return "SynNew";
	case SynConditional::myTypeID:
		return "SynConditional";
	case SynReturn::myTypeID:
		return "SynReturn";
	case SynYield::myTypeID:
		return "SynYield";
	case SynBreak::myTypeID:
		return "SynBreak";
	case SynContinue::myTypeID:
		return "SynContinue";
	case SynBlock::myTypeID:
		return "SynBlock";
	case SynIfElse::myTypeID:
		return "SynIfElse";
	case SynFor::myTypeID:
		return "SynFor";
	case SynForEachIterator::myTypeID:
		return "SynForEachIterator";
	case SynForEach::myTypeID:
		return "SynForEach";
	case SynWhile::myTypeID:
		return "SynWhile";
	case SynDoWhile::myTypeID:
		return "SynDoWhile";
	case SynSwitchCase::myTypeID:
		return "SynSwitchCase";
	case SynSwitch::myTypeID:
		return "SynSwitch";
	case SynUnaryOp::myTypeID:
		return "SynUnaryOp";
	case SynBinaryOp::myTypeID:
		return "SynBinaryOp";
	case SynAssignment::myTypeID:
		return "SynAssignment";
	case SynModifyAssignment::myTypeID:
		return "SynModifyAssignment";
	case SynVariableDefinition::myTypeID:
		return "SynVariableDefinition";
	case SynVariableDefinitions::myTypeID:
		return "SynVariableDefinitions";
	case SynAccessor::myTypeID:
		return "SynAccessor";
	case SynFunctionArgument::myTypeID:
		return "SynFunctionArgument";
	case SynFunctionDefinition::myTypeID:
		return "SynFunctionDefinition";
	case SynShortFunctionArgument::myTypeID:
		return "SynShortFunctionArgument";
	case SynShortFunctionDefinition::myTypeID:
		return "SynShortFunctionDefinition";
	case SynConstant::myTypeID:
		return "SynConstant";
	case SynConstantSet::myTypeID:
		return "SynConstantSet";
	case SynClassPrototype::myTypeID:
		return "SynClassPrototype";
	case SynClassStaticIf::myTypeID:
		return "SynClassStaticIf";
	case SynClassElements::myTypeID:
		return "SynClassElements";
	case SynClassDefinition::myTypeID:
		return "SynClassDefinition";
	case SynEnumDefinition::myTypeID:
		return "SynEnumDefinition";
	case SynNamespaceDefinition::myTypeID:
		return "SynNamespaceDefinition";
	case SynModuleImport::myTypeID:
		return "SynModuleImport";
	case SynModule::myTypeID:
		return "SynModule";
	default:
		break;
	}

	assert(!"unknown type");
	return "unknown";
}

const char* GetOpName(SynUnaryOpType type)
{
	switch(type)
	{
	case SYN_UNARY_OP_PLUS:
		return "+";
	case SYN_UNARY_OP_NEGATE:
		return "-";
	case SYN_UNARY_OP_BIT_NOT:
		return "~";
	case SYN_UNARY_OP_LOGICAL_NOT:
		return "!";
	default:
		break;
	}

	assert(!"unknown operation type");
	return "";
}

const char* GetOpName(SynBinaryOpType type)
{
	switch(type)
	{
	case SYN_BINARY_OP_ADD:
		return "+";
	case SYN_BINARY_OP_SUB:
		return "-";
	case SYN_BINARY_OP_MUL:
		return "*";
	case SYN_BINARY_OP_DIV:
		return "/";
	case SYN_BINARY_OP_MOD:
		return "%";
	case SYN_BINARY_OP_POW:
		return "**";
	case SYN_BINARY_OP_SHL:
		return "<<";
	case SYN_BINARY_OP_SHR:
		return ">>";
	case SYN_BINARY_OP_LESS:
		return "<";
	case SYN_BINARY_OP_LESS_EQUAL:
		return "<=";
	case SYN_BINARY_OP_GREATER:
		return ">";
	case SYN_BINARY_OP_GREATER_EQUAL:
		return ">=";
	case SYN_BINARY_OP_EQUAL:
		return "==";
	case SYN_BINARY_OP_NOT_EQUAL:
		return "!=";
	case SYN_BINARY_OP_BIT_AND:
		return "&";
	case SYN_BINARY_OP_BIT_OR:
		return "|";
	case SYN_BINARY_OP_BIT_XOR:
		return "^";
	case SYN_BINARY_OP_LOGICAL_AND:
		return "&&";
	case SYN_BINARY_OP_LOGICAL_OR:
		return "||";
	case SYN_BINARY_OP_LOGICAL_XOR:
		return "^^";
	case SYN_BINARY_OP_IN:
		return "in";
	default:
		break;
	}

	assert(!"unknown operation type");
	return "";
}

const char* GetOpName(SynModifyAssignType type)
{
	switch(type)
	{
	case SYN_MODIFY_ASSIGN_ADD:
		return "+=";
	case SYN_MODIFY_ASSIGN_SUB:
		return "-=";
	case SYN_MODIFY_ASSIGN_MUL:
		return "*=";
	case SYN_MODIFY_ASSIGN_DIV:
		return "/=";
	case SYN_MODIFY_ASSIGN_POW:
		return "**=";
	case SYN_MODIFY_ASSIGN_MOD:
		return "%=";
	case SYN_MODIFY_ASSIGN_SHL:
		return "<<=";
	case SYN_MODIFY_ASSIGN_SHR:
		return ">>=";
	case SYN_MODIFY_ASSIGN_BIT_AND:
		return "&=";
	case SYN_MODIFY_ASSIGN_BIT_OR:
		return "|=";
	case SYN_MODIFY_ASSIGN_BIT_XOR:
		return "^=";
	default:
		break;
	}

	assert(!"unknown operation type");
	return "";
}

InplaceStr GetModuleName(Allocator *allocator, const char *moduleRoot, IntrusiveList<SynIdentifier> parts)
{
	unsigned pathLength = unsigned(parts.size() - 1 + strlen(".nc"));

	if(moduleRoot)
		pathLength += unsigned(strlen(moduleRoot)) + 1;

	for(SynIdentifier *part = parts.head; part; part = getType<SynIdentifier>(part->next))
		pathLength += part->name.length();

	char *path = (char*)allocator->alloc(pathLength + 1);

	char *pos = path;

	if(moduleRoot)
	{
		strcpy(pos, moduleRoot);
		pos += strlen(moduleRoot);

		*pos++ = '/';
	}

	for(SynIdentifier *part = parts.head; part; part = getType<SynIdentifier>(part->next))
	{
		memcpy(pos, part->name.begin, part->name.length());
		pos += part->name.length();

		if(part->next)
			*pos++ = '/';
	}

	strcpy(pos, ".nc");
	pos += strlen(".nc");

	*pos = 0;

	return InplaceStr(path);
}
