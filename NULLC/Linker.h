#pragma once
#include "stdafx.h"
#include "ParseClass.h"

#include "Bytecode.h"

const int LINK_ERROR_BUFFER_SIZE = 512;

class Linker
{
public:
	Linker();
	~Linker();

	void	CleanCode();
	bool	LinkCode(const char *bytecode, int redefinitions);

	const char*	GetLinkError();

public:
	char		linkError[LINK_ERROR_BUFFER_SIZE];

	FastVector<ExternTypeInfo>	exTypes;
	FastVector<ExternVarInfo>	exVariables;
	FastVector<ExternFuncInfo>	exFunctions;
	FastVector<ExternLocalInfo>	exLocals;
	FastVector<ExternModuleInfo>	exModules;
	FastVector<VMCmd>			exCode;
	FastVector<char>			exSymbols;
	unsigned int				globalVarSize;
	unsigned int				offsetToGlobalCode;

	FastVector<unsigned int>	typeRemap;
	FastVector<unsigned int>	funcRemap;
};

