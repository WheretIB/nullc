#pragma once
#include "stdafx.h"
#include "ParseClass.h"

#include "Bytecode.h"

struct ExternalFunctionInfo
{
	unsigned int startInByteCode;
#if defined(_MSC_VER)
	unsigned int bytesToPop;
#elif defined(__CELLOS_LV2__)
	unsigned int rOffsets[8];
	unsigned int fOffsets[8];
#endif
};

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

	FastVector<ExternTypeInfo*>	exTypes;
	FastVector<ExternVarInfo*>	exVariables;
	FastVector<ExternFuncInfo*>	exFunctions;
	FastVector<char>			exCode;
	unsigned int				globalVarSize;
	unsigned int				offsetToGlobalCode;

	FastVector<ExternalFunctionInfo>	exFuncInfo;
	bool CreateExternalInfo(ExternFuncInfo *fInfo, ExternalFunctionInfo& externalInfo);
};

