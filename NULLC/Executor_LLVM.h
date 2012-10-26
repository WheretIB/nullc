#pragma once
#include "stdafx.h"
#include "Executor_Common.h"

class ExecutorLLVM
{
public:
	ExecutorLLVM(Linker* linker);
	~ExecutorLLVM();

	bool	TranslateToNative();

	void	Run(unsigned int functionID, const char *arguments);
	void	Stop(const char* error);

	const char*	GetResult();
	int			GetResultInt();
	double		GetResultDouble();
	long long	GetResultLong();

	const char*	GetExecError();

	char*		GetVariableData(unsigned int *count);

	void			BeginCallStack();
	unsigned int	GetNextAddress();

	void*			GetStackStart();
	void*			GetStackEnd();
private:
	void	InitExecution();

	bool *mapped;
	unsigned	GetFunctionID(const char* name, unsigned length);

	bool	codeRunning;

	static const int ERROR_BUFFER_SIZE = 1024;
	char		execError[ERROR_BUFFER_SIZE];
	char		execResult[64];

	Linker		*exLinker;

	void operator=(ExecutorLLVM& r);
};
