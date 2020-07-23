#pragma once

#include "stdafx.h"
#include "Executor_Common.h"
#include "InstructionTreeRegVm.h"

struct LlvmExecutionContext;

typedef struct DCCallVM_ DCCallVM;

class ExecutorLLVM
{
public:
	ExecutorLLVM(Linker* linker);
	~ExecutorLLVM();

	bool	TranslateToNative();

	bool	Run(unsigned functionID, const char *arguments);
	void	Stop(const char* error);
	void	Stop(NULLCRef error);
	void	Resume();

	bool	SetStackSize(unsigned bytes);

	unsigned	GetResultType();
	NULLCRef	GetResultObject();

	const char*	GetResult();
	int			GetResultInt();
	double		GetResultDouble();
	long long	GetResultLong();

	const char*	GetErrorMessage();
	NULLCRef	GetErrorObject();

	char*		GetVariableData(unsigned int *count);

	unsigned	GetCallStackAddress(unsigned frame);

	void*		GetStackStart();
	void*		GetStackEnd();

public:
#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	DCCallVM		*dcCallVM;
#endif

private:
	bool	codeRunning;

	char	*execErrorBuffer;

	const char	*execErrorMessage;
	NULLCRef	execErrorObject;

	static const unsigned execResultSize = 512;
	char		execResult[execResultSize];

	Linker		*exLinker;

	LlvmExecutionContext *ctx;

	void operator=(ExecutorLLVM& r);
};
