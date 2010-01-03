#pragma once
#include "stdafx.h"

#include "Instruction_X86.h"

#include "ParseClass.h"

#include "Executor_Common.h"

class ExecutorX86
{
public:
	ExecutorX86(Linker *linker);
	~ExecutorX86();

	bool	Initialize();

	bool	TranslateToNative();

	void	Run(const char* funcName = NULL);
	void	Stop(const char* error);

	const char*	GetResult();
	const char*	GetExecError();

	char*	GetVariableData();

	void			BeginCallStack();
	unsigned int	GetNextAddress();
private:
	char	execError[512];
	char	execResult[64];

	Linker		*exLinker;

	FastVector<ExternTypeInfo>	&exTypes;
	FastVector<ExternFuncInfo>	&exFunctions;
	FastVector<VMCmd>			&exCode;

	FastVector<x86Instruction, true, true>	instList;
	FastVector<unsigned char*>	instAddress;

	unsigned int		globalStartInBytecode;

	char			*paramBase;

	unsigned char	*binCode;
	unsigned int	binCodeStart;
	unsigned int	binCodeSize, binCodeReserved;

	int			callContinue;

	unsigned int	*callstackTop;

	void operator=(ExecutorX86& r){ (void)r; assert(false); }
};