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
	bool	LinkCode(const char *bytecode);

	const char*	GetLinkError();

#ifdef NULLC_BUILD_X86_JIT
	void	SetFunctionPointerUpdater(void (*)(unsigned, unsigned));
	void	UpdateFunctionPointer(unsigned dest, unsigned source);
#endif
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

	void (*fptrUpdater)(unsigned, unsigned);

#ifdef NULLC_LLVM_SUPPORT
	FastVector<unsigned int>	llvmModuleSizes;
	FastVector<char>			llvmModuleCodes;
#endif

	FastVector<unsigned int>	typeRemap;
	FastVector<unsigned int>	funcRemap;
	FastVector<unsigned int>	moduleRemap;

	HashMap<unsigned int>		typeMap;
	HashMap<unsigned int>		funcMap;
};

#endif
