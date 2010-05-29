#pragma once
#ifndef NULLC_LINKER_H
#define NULLC_LINKER_H

#include "stdafx.h"
#include "HashMap.h"
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
	FastVector<unsigned int>	exTypeExtra;
	FastVector<ExternVarInfo>	exVariables;
	FastVector<ExternFuncInfo>	exFunctions;
	FastVector<ExternLocalInfo>	exLocals;
	FastVector<ExternModuleInfo>	exModules;
	FastVector<VMCmd>			exCode;
	FastVector<char>			exSymbols;
	FastVector<unsigned int>	exCodeInfo;
	FastVector<char>			exSource;
	FastVector<ExternFuncInfo::Upvalue*>	exCloseLists;
	unsigned int				globalVarSize;
	unsigned int				offsetToGlobalCode;

	FastVector<unsigned int>	jumpTargets;

	FastVector<unsigned int>	functionAddress;

	FastVector<unsigned int>	typeRemap;
	FastVector<unsigned int>	funcRemap;
	FastVector<unsigned int>	moduleRemap;

	HashMap<unsigned int>		typeMap;
	HashMap<unsigned int>		funcMap;
};

#endif
