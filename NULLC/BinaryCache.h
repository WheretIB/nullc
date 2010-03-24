#pragma once
#include "stdafx.h"

namespace BinaryCache
{
	void Initialize();
	void Terminate();

	void		PutBytecode(const char* path, const char* bytecode);
	const char*	GetBytecode(const char* path);

	void		LastBytecode(const char* bytecode);

	void		SetImportPath(const char* path);
	const char*	GetImportPath();

	struct	CodeDescriptor
	{
		const char		*name;
		unsigned int	nameHash;
		const char		*binary;
	};
}
