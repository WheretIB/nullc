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

SynType* ParseType(ParseContext &ctx)
{
	if(ctx.At(lex_string))
	{
		InplaceStr name = ctx.Consume();

		return new SynTypeSimple(name);
	}

	return NULL;
}

SynBase* ParseTerminal(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_true))
		return new SynBool(start, true);

	if(ctx.Consume(lex_false))
		return new SynBool(start, false);

	if(ctx.At(lex_number))
		return new SynNumber(start, ctx.Consume());

	if(ctx.Consume(lex_nullptr))
		return new SynNullptr(start);

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
	SynBase *condition = ParseArithmetic(ctx);

	if(!condition)
		return NULL;

	return condition;
}

SynBase* ParseAssignment(ParseContext &ctx)
{
	SynBase *lhs = ParseTernaryExpr(ctx);

	if(!lhs)
		return NULL;

	return lhs;
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

SynTypedef* ParseTypedef(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_typedef))
	{
		SynType *type = ParseType(ctx);

		if(!type)
			Stop(ctx, ctx.Position(), "ERROR: typename expected after typedef");

		AssertAt(ctx, lex_string, "ERROR: alias name expected after typename in typedef expression");

		InplaceStr alias = ctx.Consume();

		AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after typedef");

		return new SynTypedef(start, type, alias);
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

	if(SynType *type = ParseType(ctx))
	{
		IntrusiveList<SynVariableDefinition> definitions;

		SynVariableDefinition *definition = ParseVariableDefinition(ctx);

		if(!definition)
			return NULL;

		definitions.push_back(definition);

		while(ctx.Consume(lex_comma))
		{
			definition = ParseVariableDefinition(ctx);

			if(!definition)
				Stop(ctx, ctx.Position(), "ERROR: next variable definition excepted after ','");

			definitions.push_back(definition);
		}

		AssertConsume(ctx, lex_semicolon, "ERROR: ';' not found after variable definition");

		return new SynVariableDefinitions(start, type, definitions);
	}

	return NULL;
}

SynBase* ParseExpression(ParseContext &ctx)
{
	//const char *start = ctx.Position();

	if(ctx.At(lex_return))
		return ParseReturn(ctx);

	if(ctx.At(lex_typedef))
		return ParseTypedef(ctx);

	if(SynBase *node = ParseVariableDefinitions(ctx))
		return node;
	
	return NULL;
}

SynModuleImport* ParseImport(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_import))
	{
		AssertAt(ctx, lex_string, "ERROR: string expected after import");

		Stop(ctx, start, "ERROR: not implemented");

		return NULL;
	}

	return NULL;
}

SynBase* ParseModule(ParseContext &ctx)
{
	const char *start = ctx.Position();

	IntrusiveList<SynModuleImport> imports;

	while(SynModuleImport *import = ParseImport(ctx))
		imports.push_back(import);

	IntrusiveList<SynBase> expressions;

	while(SynBase* expression = ParseExpression(ctx))
		expressions.push_back(expression);

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
