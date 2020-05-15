#pragma once

#include "stdafx.h"
#include "Executor_Common.h"

struct LlvmExecutionContext;

typedef struct DCCallVM_ DCCallVM;

class ExecutorLLVM
{
public:
	ExecutorLLVM(Linker* linker);
	~ExecutorLLVM();

	bool	TranslateToNative();

	void	Run(unsigned int functionID, const char *arguments);
	void	Stop(const char* error);

	bool	SetStackSize(unsigned bytes);

	const char*	GetResult();
	int			GetResultInt();
	double		GetResultDouble();
	long long	GetResultLong();

	const char*	GetExecError();

	char*		GetVariableData(unsigned int *count);

	unsigned	GetCallStackAddress(unsigned frame);

	void*		GetStackStart();
	void*		GetStackEnd();

private:
	bool	codeRunning;

	static const int ERROR_BUFFER_SIZE = 1024;
	char		execError[ERROR_BUFFER_SIZE];
	char		execResult[64];

	Linker		*exLinker;

	LlvmExecutionContext *ctx;

	DCCallVM	*dcCallVM;

	void operator=(ExecutorLLVM& r);
};
