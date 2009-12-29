#pragma once
#include "stdafx.h"
#include "ParseClass.h"

#include "Executor_Common.h"

const int ERROR_BUFFER_SIZE = 1024;

class Executor
{
public:
	Executor(Linker* linker);
	~Executor();

	void	Run(const char* funcName = NULL);
	void	Stop(const char* error);

	const char*	GetResult();
	const char*	GetExecError();

	char*	GetVariableData();
private:
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	FILE*		executeLog;
#endif
	char		execError[ERROR_BUFFER_SIZE];
	char		execResult[64];

	Linker		*exLinker;

	FastVector<ExternTypeInfo>	&exTypes;
	FastVector<ExternFuncInfo>	&exFunctions;
	char			*symbols;

	FastVector<char, true>	genParams;
	FastVector<VMCmd*>	fcallStack;

	unsigned int	runningFunction;

	unsigned int	paramBase;

	unsigned int	*genStackBase;
	unsigned int	*genStackPtr;
	unsigned int	*genStackTop;

	asmOperType		retType;

	bool			callContinue;

	bool RunExternalFunction(unsigned int funcID);

	void FixupArray(unsigned int offset, const ExternTypeInfo& type);
	void FixupClass(unsigned int offset, const ExternTypeInfo& type);

	bool ExtendParameterStack(char* oldBase, unsigned int oldSize);

	void operator=(Executor& r){ (void)r; assert(false); }
};

void PrintInstructionText(FILE* stream, VMCmd cmd, unsigned int rel, unsigned int top);
