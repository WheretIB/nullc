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

ParseContext::ParseContext()
{
	errorPos = 0;
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

	return lhs;
}

SynBase* ParseTernaryExpr(ParseContext &ctx)
{
	SynBase *condition = ParseArithmetic(ctx);

	return condition;
}

SynBase* ParseAssignment(ParseContext &ctx)
{
	SynBase *lhs = ParseTernaryExpr(ctx);

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

SynBase* ParseExpression(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.At(lex_return))
		return ParseReturn(ctx);

	return NULL;
}

SynModuleImport* ParseImport(ParseContext &ctx)
{
	const char *start = ctx.Position();

	if(ctx.Consume(lex_import))
	{
		AssertAt(ctx, lex_string, "ERROR: string expected after import");

		Stop(ctx, ctx.Position(), "ERROR: not implemented");

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
