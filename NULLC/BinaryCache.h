#pragma once
#include "stdafx.h"

struct Lexeme;

namespace BinaryCache
{
	void Initialize();
	void Terminate();

	void		PutBytecode(const char* path, const char* bytecode, Lexeme* lexStart, unsigned lexCount);
	void		PutLexemes(const char* path, Lexeme* lexStart, unsigned lexCount);

	const char*	GetBytecode(const char* path);

	// Find module bytecode in any of the import paths or by name alone
	const char*	FindBytecode(const char* moduleName, bool addExtension);

	Lexeme*		GetLexems(const char* path, unsigned& count);

	// Find module source lexems in any of the import paths or by name alone
	Lexeme*		FindLexems(const char* moduleName, bool addExtension, unsigned& count);

	void		RemoveBytecode(const char* path);
	const char*	EnumerateModules(unsigned id);

	void		LastBytecode(const char* bytecode);

	void		ClearImportPaths();
	void		AddImportPath(const char* path);
	void		RemoveImportPath(const char* path);
	bool		HasImportPath(const char* path);
	const char*	EnumImportPath(unsigned pos);

	struct	CodeDescriptor
	{
		const char		*name;
		unsigned int	nameHash;
		const char		*binary;
		Lexeme			*lexemes;
		unsigned		lexemeCount;
	};
}
