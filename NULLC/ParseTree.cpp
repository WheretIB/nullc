#include "ParseTree.h"

#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>

#include "Lexer.h"

namespace
{
	jmp_buf errorHandler;

	void Stop(ParseContext &ctx, const char *pos, const char *msg, va_list args)
	{
		ctx.errorPos = pos;

		char errorText[4096];

		vsnprintf(errorText, 4096, msg, args);
		errorText[4096 - 1] = '\0';

		ctx.errorMsg = InplaceStr(errorText);

		longjmp(errorHandler, 1);
	}

	void Stop(ParseContext &ctx, const char *pos, const char *msg, ...)
	{
		va_list args;
		va_start(args, msg);

		Stop(ctx, pos, msg, args);
	}

	void AssertAt(ParseContext &ctx, LexemeType type, const char *msg, ...)
	{
		if(!ctx.At(type))
		{
			va_list args;
			va_start(args, msg);

			Stop(ctx, ctx.Position(), msg, args);
		}
	}

	void AssertConsume(ParseContext &ctx, LexemeType type, const char *msg, ...)
	{
		if(!ctx.Consume(type))
		{
			va_list args;
			va_start(args, msg);

			Stop(ctx, ctx.Position(), msg, args);
		}
	}

	void AssertConsume(ParseContext &ctx, const char *str, const char *msg, ...)
	{
		if(!ctx.Consume(str))
		{
			va_list args;
			va_start(args, msg);

			Stop(ctx, ctx.Position(), msg, args);
		}
	}

	template<typename T>
	bool isType(SynBase *node)
	{
		return node->typeID == typename T::myTypeID;
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
	}

	return SYN_MODIFY_ASSIGN_UNKNOWN;
}

ParseContext::ParseContext()
{
	errorPos = 0;
}

LexemeType ParseContext::Peek()
{
	return currentLexeme->type;
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

const char* ParseContext::Position()
{
	return currentLexeme->pos;
}

SynBase* ParseType(ParseContext &ctx);
SynBase* ParsePostExpressions(ParseContext &ctx, SynBase *node);
SynBase* ParseTernaryExpr(ParseContext &ctx);
SynBase* ParseAssignment(ParseContext &ctx);
SynTypedef* ParseTypedef(ParseContext &ctx);
SynBase* ParseExpression(ParseContext &ctx);
IntrusiveList<SynBase> ParseExpressions(ParseContext &ctx);
SynFunctionDefinition* ParseFunctionDefinition(ParseContext &ctx);
SynShortFunctionDefinition* ParseShortFunctionDefinition(ParseContext &ctx);
SynVariableDefinition* ParseVariableDefinition(ParseContext &ctx);
SynVariableDefinitions* ParseVariableDefinitions(ParseContext &ctx);
SynAccessor* ParseAccessorDefinition(ParseContext &ctx);
IntrusiveList<SynCallArgument> ParseCallArguments(ParseContext &ctx);

SynBase* ParseTerminalType(ParseContext &ctx)
{
	const char *start = ctx.Position();

	Lexeme *lexeme = ctx.currentLexeme;

	if(ctx.At(lex_string))
	{
		InplaceStr name = ctx.Consume();

		if(ctx.Consume(lex_less))
		{
			IntrusiveList<SynBase> types;

			SynBase *type = ParseType(ctx);

			if(!type)
			{
				// Backtrack
				ctx.currentLexeme = lexeme;

				return NULL;
			}

			types.push_back(type);

			while(ctx.Consume(lex_comma))
			{
				type = ParseType(ctx);

				if(!type)
					Stop(ctx, ctx.Position(), "ERROR: typename required after ','");

				types.push_back(type);
			}

			if(!ctx.Consume(lex_greater))
			{
				if(types.size() > 1)
				{
					Stop(ctx, ctx.Position(), "ERROR: '>' expected after generic type alias list");
				}
				else
				{
					// Backtrack
					ctx.currentLexeme = lexeme;

					return NULL;
				}
			}

			return new SynTypeGenericInstance(start, name, types);
		}

		return new SynTypeSimple(start, name);
	}

	if(ctx.Consume(lex_auto))
		return new SynTypeAuto(start);

	if(ctx.Consume(lex_generic))
		return new SynTypeGeneric(start);

	if(ctx.Consume(lex_typeof))
	{
		AssertConsume(ctx, lex_oparen, "ERROR: typeof must be followed by '('");

		SynBase *value = ParseAssignment(ctx);

		AssertConsume(ctx, lex_cparen, "ERROR: ')' not found after expression in typeof");

		SynBase *node = new SynTypeof(start, value);

		return ParsePostExpressions(ctx, node);
	}

	if(ctx.Consume(lex_at))
	{
		AssertAt(ctx, lex_string, "ERROR: type alias required after '@'");

		InplaceStr name = ctx.Consume();

		return new SynTypeAlias(start, name);
	}

	return NULL;
}

SynBase* ParseType(ParseContext &ctx)
{
	const char *start = ctx.Position();

	SynBase *base = ParseTerminalType(ctx);
	
	if(!base)
		return NULL;

	while(ctx.At(lex_obracket) || ctx.At(lex_ref))
	{
		if(ctx.Consume(lex_obracket))
		{
			SynBase *size = ParseTernaryExpr(ctx);

			AssertConsume(ctx, lex_cbracket, "ERROR: matching ']' not found");

			base = new SynTypeArray(start, base, size);
		}
		else if(ctx.Consume(lex_ref))
		{
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
							Stop(ctx, ctx.Position(), "ERROR: type is expected after ','");

						arguments.push_back(argument);
					}
				}

				AssertConsume(ctx, lex_cparen, "ERROR: matching ')' not found");

				base = new SynTypeFunction(start, base, arguments);
			}
			else
			{
				base = new SynTypeReference(start, base);
			}
		}
	}

	return base;
}

SynArray* ParseArray(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_ofigure))
	{
		IntrusiveList<SynBase> values;

		SynBase *value = ParseTernaryExpr(ctx);

		if(!value)
			Stop(ctx, ctx.Position(), "ERROR: value not found after '{'");

		values.push_back(value);

		while(ctx.Consume(lex_comma))
		{
			value = ParseTernaryExpr(ctx);

			if(!value)
				Stop(ctx, ctx.Position(), "ERROR: value not found after ','");

			values.push_back(value);
		}

		AssertConsume(ctx, lex_cfigure, "ERROR: '}' not found after inline array");

		return new SynArray(start, values);
	}

	return NULL;
}

SynBase* ParseSizeof(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_sizeof))
	{
		AssertConsume(ctx, lex_oparen, "ERROR: sizeof must be followed by '('");

		SynBase *value = ParseAssignment(ctx);

		if(!value)
			Stop(ctx, ctx.Position(), "ERROR: expression or type not found after sizeof(");

		AssertConsume(ctx, lex_cparen, "ERROR: ')' not found after expression in sizeof");

		return new SynSizeof(start, value);
	}

	return NULL;
}

SynNumber* ParseNumber(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.At(lex_number))
	{
		InplaceStr value = ctx.Consume();

		InplaceStr suffix;

		if(ctx.At(lex_string))
			suffix = ctx.Consume();

		return new SynNumber(start, value, suffix);
	}

	return NULL;
}

SynAlign* ParseAlign(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_align))
	{
		AssertConsume(ctx, lex_oparen, "ERROR: '(' expected after align");

		AssertAt(ctx, lex_number, "ERROR: alignment value not found after align(");

		SynNumber *value = ParseNumber(ctx);

		AssertConsume(ctx, lex_cparen, "ERROR: ')' expected after alignment value");

		return new SynAlign(start, value);
	}
	else if(ctx.Consume(lex_noalign))
	{
		return new SynAlign(start, NULL);
	}

	return NULL;
}

SynNew* ParseNew(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_new))
	{
		SynBase *type = NULL;

		if(ctx.Consume(lex_oparen))
		{
			type = ParseType(ctx);

			if(!type)
				Stop(ctx, ctx.Position(), "ERROR: type name expected after 'new'");

			AssertConsume(ctx, lex_cparen, "ERROR: matching ')' not found after '('");
		}
		else
		{
			type = ParseType(ctx);

			if(!type)
				Stop(ctx, ctx.Position(), "ERROR: type name expected after 'new'");
		}

		IntrusiveList<SynCallArgument> arguments;

		if(ctx.Consume(lex_obracket))
		{
			SynBase *count = ParseTernaryExpr(ctx);

			if(!count)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after '['");

			AssertConsume(ctx, lex_cbracket, "ERROR: ']' not found after expression");

			return new SynNew(start, type, arguments, count);
		}

		if(ctx.Consume(lex_oparen))
		{
			arguments = ParseCallArguments(ctx);

			AssertConsume(ctx, lex_cparen, "ERROR: ')' not found after function parameter list");
		}
		
		return new SynNew(start, type, arguments, NULL);
	}

	return NULL;
}

SynCallArgument* ParseCallArgument(ParseContext &ctx)
{
	const char *start = ctx.Position();

	Lexeme *lexeme = ctx.currentLexeme;

	if(ctx.At(lex_string))
	{
		InplaceStr name = ctx.Consume();

		if(ctx.Consume(lex_colon))
		{
			SynBase *value = ParseAssignment(ctx);

			if(!value)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after ':' in function parameter list");

			return new SynCallArgument(start, name, value);
		}
		else
		{
			// Backtrack
			ctx.currentLexeme = lexeme;
		}
	}

	if(SynBase *value = ParseAssignment(ctx))
	{
		return new SynCallArgument(start, InplaceStr(), value);
	}

	return NULL;
}

IntrusiveList<SynCallArgument> ParseCallArguments(ParseContext &ctx)
{
	IntrusiveList<SynCallArgument> arguments;

	if(SynCallArgument *argument = ParseCallArgument(ctx))
	{
		arguments.push_back(argument);

		while(ctx.Consume(lex_comma))
		{
			argument = ParseCallArgument(ctx);

			if(!argument)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after ',' in function parameter list");

			arguments.push_back(argument);
		}
	}

	return arguments;
}

SynBase* ParsePostExpressions(ParseContext &ctx, SynBase *node)
{
	while(ctx.At(lex_point) || ctx.At(lex_obracket) || ctx.At(lex_oparen))
	{
		const char *pos = ctx.Position();

		if(ctx.Consume(lex_point))
		{
			AssertAt(ctx, lex_string, "ERROR: member name expected after '.'");

			InplaceStr member = ctx.Consume();

			node = new SynMemberAccess(pos, node, member);
		}
		else if(ctx.Consume(lex_obracket))
		{
			IntrusiveList<SynCallArgument> arguments = ParseCallArguments(ctx);

			AssertConsume(ctx, lex_cbracket, "ERROR: ']' not found after expression");

			node = new SynArrayIndex(pos, node, arguments);
		}
		else if(ctx.Consume(lex_oparen))
		{
			IntrusiveList<SynCallArgument> arguments = ParseCallArguments(ctx);

			AssertConsume(ctx, lex_cparen, "ERROR: ')' not found after function parameter list");

			node = new SynArrayIndex(pos, node, arguments);
		}
		else
		{
			Stop(ctx, ctx.Position(), "ERROR: not implemented");
		}
	}

	const char *pos = ctx.Position();

	if(ctx.Consume(lex_inc))
		node = new SynPostModify(pos, node, true);
	else if(ctx.Consume(lex_dec))
		node = new SynPostModify(pos, node, false);

	return node;
}

SynBase* ParseComplexTerminal(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_mul))
	{
		SynBase *node = ParseComplexTerminal(ctx);

		if(!node)
			Stop(ctx, ctx.Position(), "ERROR: expression not found after '*'");

		return new SynDereference(start, node);
	}

	SynBase *node = NULL;

	if(ctx.Consume(lex_oparen))
	{
		node = ParseAssignment(ctx);

		if(!node)
			Stop(ctx, ctx.Position(), "ERROR: expression not found after '('");

		AssertConsume(ctx, lex_cparen, "ERROR: closing ')' not found after '('");
	}

	if(!node && ctx.At(lex_quotedstring))
		node = new SynString(start, ctx.Consume());

	if(!node)
		node = ParseArray(ctx);

	if(!node)
		node = ParseType(ctx);

	if(!node && ctx.At(lex_string))
		node = new SynIdentifier(start, ctx.Consume());

	if(!node)
		return NULL;
		
	return ParsePostExpressions(ctx, node);
}

SynBase* ParseTerminal(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_true))
		return new SynBool(start, true);

	if(ctx.Consume(lex_false))
		return new SynBool(start, false);

	if(ctx.Consume(lex_nullptr))
		return new SynNullptr(start);

	if(ctx.Consume(lex_bitand))
	{
		SynBase *node = ParseComplexTerminal(ctx);

		if(!node)
			Stop(ctx, ctx.Position(), "ERROR: variable not found after '&'");

		return new SynGetAddress(start, node);
	}

	if(ctx.At(lex_semiquotedchar))
		return new SynCharacter(start, ctx.Consume());

	if(SynUnaryOpType type = GetUnaryOpType(ctx.Peek()))
	{
		InplaceStr name = ctx.Consume();

		SynBase *value = ParseTerminal(ctx);

		if(!value)
			Stop(ctx, ctx.Position(), "ERROR: expression not found after '%.*s'", name.length(), name.begin);

		return new SynUnaryOp(start, type, value);
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
		const char *start = ctx.Position();

		ctx.Skip();

		while(ctx.binaryOpStack.size() > startSize && GetBinaryOpPrecedence(ctx.binaryOpStack.back().type) <= GetBinaryOpPrecedence(binaryOp))
		{
			lhs = new SynBinaryOp(ctx.binaryOpStack.back().pos, ctx.binaryOpStack.back().type, lhs, ctx.binaryOpStack.back().value);

			ctx.binaryOpStack.pop_back();
		}

		SynBase *value = ParseTerminal(ctx);

		if(!value)
			Stop(ctx, ctx.Position(), "ERROR: terminal expression not found after binary operation");

		ctx.binaryOpStack.push_back(SynBinaryOpElement(start, binaryOp, value));
	}

	while(ctx.binaryOpStack.size() > startSize)
	{
		lhs = new SynBinaryOp(ctx.binaryOpStack.back().pos, ctx.binaryOpStack.back().type, lhs, ctx.binaryOpStack.back().value);

		ctx.binaryOpStack.pop_back();
	}

	return lhs;
}

SynBase* ParseTernaryExpr(ParseContext &ctx)
{
	if(SynBase *value = ParseArithmetic(ctx))
	{
		const char *pos = ctx.Position();

		while(ctx.Consume(lex_questionmark))
		{
			SynBase *trueBlock = ParseAssignment(ctx);

			if(!trueBlock)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after '?'");

			AssertConsume(ctx, lex_colon, "ERROR: ':' not found after expression in ternary operator");

			SynBase *falseBlock = ParseAssignment(ctx);

			if(!falseBlock)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after ':'");

			value = new SynIfElse(pos, false, value, trueBlock, falseBlock);
		}

		return value;
	}

	return NULL;
}

SynBase* ParseAssignment(ParseContext &ctx)
{
	if(SynBase *lhs = ParseTernaryExpr(ctx))
	{
		const char *pos = ctx.Position();

		if(ctx.Consume(lex_set))
		{
			SynBase *rhs = ParseAssignment(ctx);

			if(!rhs)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after '='");

			return new SynAssignment(pos, lhs, rhs);
		}
		else if(SynModifyAssignType modifyType = GetModifyAssignType(ctx.Peek()))
		{
			InplaceStr name = ctx.Consume();

			SynBase *rhs = ParseAssignment(ctx);

			if(!rhs)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after '%.*s' operator", name.length(), name.begin);

			return new SynModifyAssignment(pos, modifyType, lhs, rhs);
		}

		return lhs;
	}

	return NULL;
}

SynBase* ParseClassDefinition(ParseContext &ctx)
{
	const char *start = ctx.Position();

	Lexeme *lexeme = ctx.currentLexeme;

	SynAlign *align = ParseAlign(ctx);

	if(ctx.Consume(lex_class))
	{
		AssertAt(ctx, lex_string, "ERROR: class name expected");

		InplaceStr name = ctx.Consume();

		if(ctx.Consume(lex_semicolon))
			return new SynClassPototype(start, name);

		IntrusiveList<SynIdentifier> aliases;

		if(ctx.Consume(lex_less))
		{
			AssertAt(ctx, lex_string, "ERROR: generic type alias required after '<'");

			const char *pos = ctx.Position();
			InplaceStr alias = ctx.Consume();

			aliases.push_back(new SynIdentifier(pos, alias));

			while(ctx.Consume(lex_comma))
			{
				AssertAt(ctx, lex_string, "ERROR: generic type alias required after ','");

				pos = ctx.Position();
				alias = ctx.Consume();

				aliases.push_back(new SynIdentifier(pos, alias));
			}

			AssertConsume(ctx, lex_greater, "ERROR: '>' expected after generic type alias list");
		}

		bool extendable = ctx.Consume(lex_extendable);

		SynBase *baseClass = NULL;

		if(ctx.Consume(lex_colon))
		{
			baseClass = ParseType(ctx);

			if(!baseClass)
				Stop(ctx, ctx.Position(), "ERROR: base type name is expected at this point");
		}

		AssertConsume(ctx, lex_ofigure, "ERROR: '{' not found after class name");

		IntrusiveList<SynTypedef> typedefs;
		IntrusiveList<SynFunctionDefinition> functions;
		IntrusiveList<SynAccessor> accessors;
		IntrusiveList<SynVariableDefinitions> members;

		for(;;)
		{
			if(SynTypedef *node = ParseTypedef(ctx))
			{
				typedefs.push_back(node);
			}
			else if(SynFunctionDefinition *node = ParseFunctionDefinition(ctx))
			{
				functions.push_back(node);
			}
			else if(SynAccessor *node = ParseAccessorDefinition(ctx))
			{
				accessors.push_back(node);
			}
			else if(SynVariableDefinitions *node = ParseVariableDefinitions(ctx))
			{
				members.push_back(node);
			}
			else
			{
				break;
			}
		}

		AssertConsume(ctx, lex_cfigure, "ERROR: '{' not found after class name");

		return new SynClassDefinition(start, align, name, aliases, extendable, baseClass, typedefs, functions, accessors, members);
	}

	// Backtrack
	ctx.currentLexeme = lexeme;

	return NULL;
}

SynNamespaceDefinition* ParseNamespaceDefinition(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_namespace))
	{
		IntrusiveList<SynIdentifier> path;

		AssertAt(ctx, lex_string, "ERROR: namespace name required");

		const char *pos = ctx.Position();

		path.push_back(new SynIdentifier(pos, ctx.Consume()));

		while(ctx.Consume(lex_point))
		{
			AssertAt(ctx, lex_string, "ERROR: namespace name required after '.'");

			pos = ctx.Position();

			path.push_back(new SynIdentifier(pos, ctx.Consume()));
		}

		AssertConsume(ctx, lex_ofigure, "ERROR: '{' not found after namespace name");

		IntrusiveList<SynBase> code = ParseExpressions(ctx);

		AssertConsume(ctx, lex_cfigure, "ERROR: '}' not found after namespace body");

		return new SynNamespaceDefinition(start, path, code);
	}

	return NULL;
}

SynReturn* ParseReturn(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_return))
	{
		// Optional
		SynBase *value = ParseAssignment(ctx);

		AssertConsume(ctx, lex_semicolon, "ERROR: return statement must be followed by ';'");

		return new SynReturn(start, value);
	}

	return NULL;
}

SynYield* ParseYield(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_yield))
	{
		// Optional
		SynBase *value = ParseAssignment(ctx);

		AssertConsume(ctx, lex_semicolon, "ERROR: yield statement must be followed by ';'");

		return new SynYield(start, value);
	}

	return NULL;
}

SynBreak* ParseBreak(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_break))
	{
		// Optional
		SynNumber *node = ParseNumber(ctx);

		AssertConsume(ctx, lex_semicolon, "ERROR: break statement must be followed by ';'");

		return new SynBreak(start, node);
	}

	return NULL;
}

SynContinue* ParseContinue(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_break))
	{
		// Optional
		SynNumber *node = ParseNumber(ctx);

		AssertConsume(ctx, lex_semicolon, "ERROR: break statement must be followed by ';'");

		return new SynContinue(start, node);
	}

	return NULL;
}

SynTypedef* ParseTypedef(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_typedef))
	{
		SynBase *type = ParseType(ctx);

		if(!type)
			Stop(ctx, ctx.Position(), "ERROR: typename expected after typedef");

		AssertAt(ctx, lex_string, "ERROR: alias name expected after typename in typedef expression");

		InplaceStr alias = ctx.Consume();

		AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after typedef");

		return new SynTypedef(start, type, alias);
	}

	return NULL;
}

SynBlock* ParseBlock(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_ofigure))
	{
		IntrusiveList<SynBase> expressions = ParseExpressions(ctx);

		AssertConsume(ctx, lex_cfigure, "ERROR: closing '}' not found");

		return new SynBlock(start, expressions);
	}

	return NULL;
}

SynIfElse* ParseIfElse(ParseContext &ctx)
{
	const char *start = ctx.Position();

	bool staticIf = ctx.Consume(lex_at);

	if(ctx.Consume(lex_if))
	{
		AssertConsume(ctx, lex_oparen, "ERROR: '(' not found after 'if'");

		SynBase *condition = ParseAssignment(ctx);

		if(!condition)
			Stop(ctx, ctx.Position(), "ERROR: condition not found in 'if' statement");

		AssertConsume(ctx, lex_cparen, "ERROR: closing ')' not found after 'if' condition");

		SynBase *trueBlock = ParseExpression(ctx);
		SynBase *falseBlock = NULL;

		if(ctx.Consume(lex_else))
		{
			falseBlock = ParseExpression(ctx);

			if(!falseBlock)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after 'else'");
		}

		return new SynIfElse(start, staticIf, condition, trueBlock, falseBlock);
	}

	return NULL;
}

SynForEachIterator* ParseForEachIterator(ParseContext &ctx, bool isFirst)
{
	const char *start = ctx.Position();

	Lexeme *lexeme = ctx.currentLexeme;

	// Optional type
	SynBase *type = ParseType(ctx);

	// Must be followed by a type
	if(!ctx.At(lex_string))
	{
		// Backtrack
		ctx.currentLexeme = lexeme;

		type = NULL;
	}

	if(ctx.At(lex_string))
	{
		InplaceStr name = ctx.Consume();

		if(isFirst && !ctx.At(lex_in))
		{
			// Backtrack
			ctx.currentLexeme = lexeme;

			return NULL;
		}

		AssertConsume(ctx, lex_in, "ERROR: 'in' expected after variable name");

		SynBase *value = ParseTernaryExpr(ctx);

		if(!value)
			Stop(ctx, ctx.Position(), "ERROR: expression expected after 'in'");

		return new SynForEachIterator(start, type, name, value);
	}

	return NULL;
}

SynForEach* ParseForEach(ParseContext &ctx)
{
	const char *start = ctx.Position();

	Lexeme *lexeme = ctx.currentLexeme;

	if(ctx.Consume(lex_for))
	{
		AssertConsume(ctx, lex_oparen, "ERROR: '(' not found after 'for'");

		IntrusiveList<SynForEachIterator> iterators;

		SynForEachIterator *iterator = ParseForEachIterator(ctx, true);

		if(!iterator)
		{
			// Backtrack
			ctx.currentLexeme = lexeme;

			return NULL;
		}

		iterators.push_back(iterator);

		while(ctx.Consume(lex_comma))
		{
			iterator = ParseForEachIterator(ctx, true);

			if(!iterator)
				Stop(ctx, ctx.Position(), "ERROR: variable name or type expected before 'in'");

			iterators.push_back(iterator);
		}

		AssertConsume(ctx, lex_cparen, "ERROR: ')' not found after 'for' statement");

		SynBase *body = ParseExpression(ctx);

		if(!body)
			Stop(ctx, ctx.Position(), "ERROR: body not found after 'for' header");

		return new SynForEach(start, iterators, body);
	}

	return NULL;
}

SynFor* ParseFor(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_for))
	{
		AssertConsume(ctx, lex_oparen, "ERROR: '(' not found after 'for'");

		SynBase *initializer = NULL;

		if(ctx.At(lex_ofigure))
		{
			initializer = ParseBlock(ctx);

			AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after initializer in 'for'");
		}
		else if(SynBase *node = ParseVariableDefinitions(ctx))
		{
			initializer = node;
		}
		else if(SynBase *node = ParseAssignment(ctx))
		{
			initializer = node;

			AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after initializer in 'for'");
		}
		else if(!ctx.Consume(lex_semicolon))
		{
			Stop(ctx, ctx.Position(), "ERROR: ';' not found after initializer in 'for'");
		}

		SynBase *condition = ParseAssignment(ctx);

		if(!condition)
			Stop(ctx, ctx.Position(), "ERROR: condition not found in 'for' statement");

		AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after condition in 'for'");

		SynBase *increment = NULL;

		if(ctx.At(lex_ofigure))
			increment = ParseBlock(ctx);
		else if(SynBase *node = ParseAssignment(ctx))
			increment = node;

		AssertConsume(ctx, lex_cparen, "ERROR: ')' not found after 'for' statement");

		SynBase *body = ParseExpression(ctx);

		if(!body)
			Stop(ctx, ctx.Position(), "ERROR: body not found after 'for' header");

		return new SynFor(start, initializer, condition, increment, body);
	}

	return NULL;
}

SynVariableDefinition* ParseVariableDefinition(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.At(lex_string))
	{
		InplaceStr name = ctx.Consume();
		SynBase *initializer = NULL;

		if(ctx.Consume(lex_set))
		{
			initializer = ParseAssignment(ctx);

			if(!initializer)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after '='");
		}

		return new SynVariableDefinition(start, name, initializer);
	}

	return NULL;
}

SynVariableDefinitions* ParseVariableDefinitions(ParseContext &ctx)
{
	const char *start = ctx.Position();

	Lexeme *lexeme = ctx.currentLexeme;

	SynAlign *align = ParseAlign(ctx);

	if(SynBase *type = ParseType(ctx))
	{
		IntrusiveList<SynVariableDefinition> definitions;

		SynVariableDefinition *definition = ParseVariableDefinition(ctx);

		if(!definition)
		{
			// Backtrack
			ctx.currentLexeme = lexeme;

			return NULL;
		}

		definitions.push_back(definition);

		while(ctx.Consume(lex_comma))
		{
			definition = ParseVariableDefinition(ctx);

			if(!definition)
				Stop(ctx, ctx.Position(), "ERROR: next variable definition excepted after ','");

			definitions.push_back(definition);
		}

		AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after variable definition");

		return new SynVariableDefinitions(start, align, type, definitions);
	}
	
	// Backtrack
	ctx.currentLexeme = lexeme;

	return NULL;
}

SynAccessor* ParseAccessorDefinition(ParseContext &ctx)
{
	const char *start = ctx.Position();
	Lexeme *lexeme = ctx.currentLexeme;

	if(SynBase *type = ParseType(ctx))
	{
		AssertAt(ctx, lex_string, "ERROR: class member name expected after type");

		InplaceStr name = ctx.Consume();

		if(!ctx.Consume(lex_ofigure))
		{
			// Backtrack
			ctx.currentLexeme = lexeme;

			return NULL;
		}

		AssertConsume(ctx, "get", "ERROR: 'get' is expected after '{'");

		SynBase *getBlock = ParseBlock(ctx);

		if(!getBlock)
			Stop(ctx, ctx.Position(), "ERROR: function body expected after 'get'");

		SynBase *setBlock = NULL;
		InplaceStr setName;

		if(ctx.Consume("set"))
		{
			if(ctx.Consume(lex_oparen))
			{
				AssertAt(ctx, lex_string, "ERROR: r-value name not found after '('");

				setName = ctx.Consume();

				AssertConsume(ctx, lex_cparen, "ERROR: ')' not found after r-value");
			}

			setBlock = ParseBlock(ctx);

			if(!setBlock)
				Stop(ctx, ctx.Position(), "ERROR: function body expected after 'set'");
		}

		AssertConsume(ctx, lex_cfigure, "ERROR: '}' is expected after property");

		AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after class member list");

		return new SynAccessor(start, type, name, getBlock, setBlock, setName);
	}

	return NULL;
}

SynFunctionArgument* ParseFunctionArgument(ParseContext &ctx, SynBase *lastType)
{
	Lexeme *lexeme = ctx.currentLexeme;

	if(SynBase *type = ParseType(ctx))
	{
		if(!ctx.At(lex_string) && lastType)
		{
			// Backtrack
			ctx.currentLexeme = lexeme;

			type = lastType;
		}
		else
		{
			AssertAt(ctx, lex_string, "ERROR: variable name not found after type in function variable list");
		}

		const char *start = ctx.Position();
		InplaceStr name = ctx.Consume();

		SynBase *defaultValue = NULL;

		if(ctx.Consume(lex_set))
		{
			defaultValue = ParseTernaryExpr(ctx);

			if(!defaultValue)
				Stop(ctx, ctx.Position(), "ERROR: default parameter value not found after '='");
		}

		return new SynFunctionArgument(start, type, name, defaultValue);
	}

	return NULL;
}

IntrusiveList<SynFunctionArgument> ParseFunctionArguments(ParseContext &ctx)
{
	IntrusiveList<SynFunctionArgument> arguments;

	if(SynFunctionArgument *argument = ParseFunctionArgument(ctx, NULL))
	{
		arguments.push_back(argument);

		while(ctx.Consume(lex_comma))
		{
			argument = ParseFunctionArgument(ctx, arguments.tail->type);

			if(!argument)
				Stop(ctx, ctx.Position(), "ERROR: expression not found after ',' in function parameter list");

			arguments.push_back(argument);
		}
	}

	return arguments;
}

SynFunctionDefinition* ParseFunctionDefinition(ParseContext &ctx)
{
	const char *start = ctx.Position();

	Lexeme *lexeme = ctx.currentLexeme;

	bool coroutine = ctx.Consume(lex_coroutine);

	if(SynBase *returnType = ParseType(ctx))
	{
		// Check if this is a member function
		Lexeme *subLexeme = ctx.currentLexeme;

		SynBase *parentType = ParseType(ctx);
		bool accessor = false;

		if(parentType)
		{
			if(ctx.Consume(lex_colon))
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

		InplaceStr name;

		if(ctx.At(lex_string))
			name = ctx.Consume();
		else if(parentType)
			Stop(ctx, ctx.Position(), "ERROR: function name expected after ':' or '.'");
		else if(coroutine && !allowEmptyName)
			Stop(ctx, ctx.Position(), "ERROR: function name not found after return type");

		if(parentType || coroutine)
			AssertAt(ctx, lex_oparen, "ERROR: '(' expected after function name");

		if((name.begin == NULL && !allowEmptyName) || !ctx.Consume(lex_oparen))
		{
			// Backtrack
			ctx.currentLexeme = lexeme;

			return NULL;
		}

		IntrusiveList<SynFunctionArgument> arguments = ParseFunctionArguments(ctx);

		AssertConsume(ctx, lex_cparen, "ERROR: ')' not found after function variable list");

		IntrusiveList<SynBase> expressions;

		if(ctx.Consume(lex_semicolon))
			return new SynFunctionDefinition(start, true, coroutine, parentType, accessor, returnType, name, arguments, expressions);

		AssertConsume(ctx, lex_ofigure, "ERROR: '{' not found after function header");

		expressions = ParseExpressions(ctx);

		AssertConsume(ctx, lex_cfigure, "ERROR: '}' not found after function body");

		return new SynFunctionDefinition(start, false, coroutine, parentType, accessor, returnType, name, arguments, expressions);
	}
	else if(coroutine)
	{
		Stop(ctx, ctx.Position(), "ERROR: function return type not found after 'coroutine'");
	}

	return NULL;
}

SynShortFunctionDefinition* ParseShortFunctionDefinition(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_less))
	{
		IntrusiveList<SynShortFunctionArgument> arguments;

		do
		{
			const char *pos = ctx.Position();

			Lexeme *lexeme = ctx.currentLexeme;

			SynBase *type = ParseType(ctx);

			if(!ctx.At(lex_string))
			{
				// Backtrack
				ctx.currentLexeme = lexeme;

				type = NULL;
			}

			if(arguments.head == NULL)
				AssertAt(ctx, lex_string, "ERROR: function argument name not found after '<'");
			else
				AssertAt(ctx, lex_string, "ERROR: function argument name not found after ','");

			InplaceStr name = ctx.Consume();

			arguments.push_back(new SynShortFunctionArgument(pos, type, name));
		}
		while(ctx.Consume(lex_comma));

		AssertConsume(ctx, lex_greater, "ERROR: '>' expected after short inline function argument list");

		AssertConsume(ctx, lex_ofigure, "ERROR: '{' not found after function header");

		IntrusiveList<SynBase> expressions = ParseExpressions(ctx);

		AssertConsume(ctx, lex_cfigure, "ERROR: '}' not found after function body");

		return new SynShortFunctionDefinition(start, arguments, expressions);
	}

	return NULL;
}

SynBase* ParseExpression(ParseContext &ctx)
{
	if(SynBase *node = ParseClassDefinition(ctx))
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

	if(SynBase *node = ParseIfElse(ctx))
		return node;

	if(SynBase *node = ParseForEach(ctx))
		return node;

	if(SynBase *node = ParseFor(ctx))
		return node;

	if(SynBase *node = ParseFunctionDefinition(ctx))
		return node;

	if(SynBase *node = ParseVariableDefinitions(ctx))
		return node;

	if(SynBase *node = ParseAssignment(ctx))
	{
		AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after expression");

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

SynModuleImport* ParseImport(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_import))
	{
		IntrusiveList<SynIdentifier> path;

		AssertAt(ctx, lex_string, "ERROR: name expected after import");

		const char *pos = ctx.Position();

		path.push_back(new SynIdentifier(pos, ctx.Consume()));

		while(ctx.Consume(lex_point))
		{
			AssertAt(ctx, lex_string, "ERROR: name expected after '.'");

			pos = ctx.Position();

			path.push_back(new SynIdentifier(pos, ctx.Consume()));
		}

		AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after import expression");

		return new SynModuleImport(start, path);
	}

	return NULL;
}

IntrusiveList<SynModuleImport> ParseImports(ParseContext &ctx)
{
	IntrusiveList<SynModuleImport> imports;

	while(SynModuleImport *import = ParseImport(ctx))
		imports.push_back(import);

	return imports;
}

SynBase* ParseModule(ParseContext &ctx)
{
	const char *start = ctx.Position();

	IntrusiveList<SynModuleImport> imports = ParseImports(ctx);

	IntrusiveList<SynBase> expressions = ParseExpressions(ctx);

	return new SynModule(start, imports, expressions);
}

SynBase* Parse(ParseContext &ctx)
{
	SynBase *tree = ParseModule(ctx);

	if(!ctx.Consume(lex_none))
		Stop(ctx, ctx.Position(), "ERROR: unexpected symbol");

	return tree;
}

SynBase* Parse(ParseContext &ctx, const char *code)
{
	Lexer lexer;

	lexer.Lexify(code);

	if(!setjmp(errorHandler))
	{
		ctx.currentLexeme = lexer.GetStreamStart();

		return Parse(ctx);
	}

	return NULL;
}
