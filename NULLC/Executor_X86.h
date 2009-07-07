#pragma once
#include "stdafx.h"

#include "Instruction_X86.h"

#include "ParseClass.h"

#include "Linker.h"

class ExecutorX86
{
public:
	ExecutorX86(Linker *linker);
	~ExecutorX86();

	bool	Initialize() throw();

	bool	TranslateToNative() throw();

	void	Run(const char* funcName = NULL) throw();
	const char*	GetResult() throw();

	const char*	GetExecError() throw();

	char*	GetVariableData() throw();

	void	SetOptimization(int toggle) throw();
private:
	char	execError[256];
	char	execResult[64];

	Linker		*exLinker;

	FastVector<ExternTypeInfo*>	&exTypes;
	FastVector<ExternFuncInfo*>	&exFunctions;
	FastVector<ExternalFunctionInfo>	&exFuncInfo;
	FastVector<VMCmd>			&exCode;

	FastVector<x86Instruction>	instList;

	int	optimize;
	unsigned int		globalStartInBytecode;

	char			*paramData;
	unsigned int	paramBase;

	unsigned char	*binCode;
	unsigned int	binCodeStart;
	unsigned int	binCodeSize;

	void operator=(ExecutorX86& r){ (void)r; assert(false); }
};