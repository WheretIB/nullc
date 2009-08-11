#pragma once
#include "stdafx.h"
#include "ParseClass.h"

#include "Bytecode.h"

class Linker
{
public:
	Linker();
	~Linker();

	void	CleanCode();
	bool	LinkCode(const char *bytecode, int redefinitions);

	const char*	GetLinkError();

public:
	char		linkError[512];

	FastVector<ExternTypeInfo>	exTypes;
	FastVector<ExternVarInfo>	exVariables;
	FastVector<ExternFuncInfo>	exFunctions;
	FastVector<VMCmd>			exCode;
	unsigned int				globalVarSize;
	unsigned int				offsetToGlobalCode;
};

