#pragma once
#ifndef NULLC_LINKER_H
#define NULLC_LINKER_H

#include "stdafx.h"
#include "Bytecode.h"
#include "HashMap.h"

struct RegVmCmd;

struct OutputContext;

const int LINK_ERROR_BUFFER_SIZE = 512;

class Linker
{
public:
	Linker();
	~Linker();

	void	CleanCode();
	bool	LinkCode(const char *bytecode, const char *moduleName);
	bool	SaveRegVmListing(OutputContext &output, bool withProfileInfo);

	const char*	GetLinkError();

	void	FixupCallMicrocode(unsigned microcode, unsigned oldGlobalSize);
public:
	char		linkError[LINK_ERROR_BUFFER_SIZE];

	FastVector<ExternTypeInfo>		exTypes;
	FastVector<ExternMemberInfo>	exTypeExtra;
	FastVector<ExternConstantInfo>	exTypeConstants;
	FastVector<ExternVarInfo>		exVariables;
	FastVector<ExternFuncInfo>		exFunctions;
	FastVector<unsigned>			exFunctionExplicitTypeArrayOffsets;
	FastVector<unsigned>			exFunctionExplicitTypes;
	FastVector<ExternLocalInfo>		exLocals;
	FastVector<ExternModuleInfo>	exModules;
	FastVector<char>				exSymbols;
	FastVector<char>				exSource;
	FastVector<unsigned int>		exDependencies;

	FastVector<RegVmCmd>			exRegVmCode;
	FastVector<ExternSourceInfo>	exRegVmSourceInfo;
	FastVector<unsigned int>		exRegVmExecCount;
	FixedArray<unsigned int, 256>	exRegVmInstructionExecCount;
	FastVector<unsigned int>		exRegVmConstants;
	FastVector<unsigned char>		exRegVmRegKillInfo;

	FastVector<unsigned int>		regVmJumpTargets;

	FastVector<RegVmCmd*>			expiredRegVmCode;
	FastVector<unsigned*>			expiredRegVmConstants;

	unsigned int					globalVarSize;

#ifdef NULLC_LLVM_SUPPORT
	FastVector<unsigned int>	llvmModuleSizes;
	FastVector<char>			llvmModuleCodes;

	FastVector<unsigned int>	llvmTypeRemapSizes;
	FastVector<unsigned int>	llvmTypeRemapOffsets;
	FastVector<unsigned int>	llvmTypeRemapValues;

	FastVector<unsigned int>	llvmFuncRemapSizes;
	FastVector<unsigned int>	llvmFuncRemapOffsets;
	FastVector<unsigned int>	llvmFuncRemapValues;
#endif

	FastVector<unsigned int>	typeRemap;
	FastVector<unsigned int>	funcRemap;
	FastVector<unsigned int>	moduleRemap;

	HashMap<unsigned int>		typeMap;
	HashMap<unsigned int>		funcMap;

	unsigned debugOutputIndent;
};

#endif
