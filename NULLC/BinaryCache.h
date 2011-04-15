#pragma once
#include "stdafx.h"

struct Lexeme;

namespace BinaryCache
{
	void Initialize();
	void Terminate();

	void		PutBytecode(const char* path, const char* bytecode, Lexeme* lexStart, unsigned lexCount);
	const char*	GetBytecode(const char* path);
	Lexeme*		GetLexems(const char* path, unsigned& count);
	void		RemoveBytecode(const char* path);
	const char*	EnumerateModules(unsigned id);

	void		LastBytecode(const char* bytecode);

	void		SetImportPath(const char* path);
	const char*	GetImportPath();

	struct	CodeDescriptor
	{
		const char		*name;
		unsigned int	nameHash;
		const char		*binary;
		Lexeme			*lexemes;
		unsigned		lexemeCount;
	};
}
