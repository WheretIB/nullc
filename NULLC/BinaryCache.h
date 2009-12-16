#pragma once
#include "stdafx.h"

namespace BinaryCache
{
	void Initialize();
	void Terminate();

	char*	GetBytecode(const char* path);

	struct	CodeDescriptor
	{
		const char		*name;
		unsigned int	nameHash;
		char			*binary;
	};
}
