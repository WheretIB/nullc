#pragma once

#include "Lexer.h"
#include "IntrusiveList.h"

struct ParseContext
{
	ParseContext();

	bool At(LexemeType type);
	bool Consume(LexemeType type);
	InplaceStr Consume();
	void Skip();

	const char* Position();

	Lexeme *currentLexeme;

	const char *errorPos;
	InplaceStr errorMsg;
};

struct SynBase
{
	SynBase(const char *pos): pos(pos), next(0)
	{
	}

	const char *pos;
	SynBase *next;
};

struct SynBool: SynBase
{
	SynBool(const char* pos, bool value): SynBase(pos), value(value)
	{
	}

	bool value;
};

struct SynNumber: SynBase
{
	SynNumber(const char* pos, InplaceStr value): SynBase(pos), value(value)
	{
	}

	InplaceStr value;
};

struct SynNullptr: SynBase
{
	SynNullptr(const char* pos): SynBase(pos)
	{
	}
};

struct SynReturn: SynBase
{
	SynReturn(const char* pos, SynBase* value): SynBase(pos), value(value)
	{
	}

	SynBase *value;
};

struct SynModuleImport: SynBase
{
	SynModuleImport(const char* pos, InplaceStr name): SynBase(pos), name(name)
	{
	}

	InplaceStr name;
};

struct SynModule: SynBase
{
	SynModule(const char* pos, IntrusiveList<SynModuleImport> imports, IntrusiveList<SynBase> expressions): SynBase(pos), imports(imports), expressions(expressions)
	{
	}

	IntrusiveList<SynModuleImport> imports;

	IntrusiveList<SynBase> expressions;
};

SynBase* Parse(ParseContext &context, const char *code);
