#pragma once
#include "stdafx.h"
#include "ParseClass.h"

#include "Linker.h"

class Executor
{
public:
	Executor(Linker* linker);
	~Executor();

	void	Run(const char* funcName = NULL);

	const char*	GetResult();
	const char*	GetExecError();

	char*	GetVariableData();

	void	SetCallback(bool (*Func)(unsigned int));
private:
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	FILE*		executeLog;
#endif
	char		execError[512];
	char		execResult[64];

	Linker		*exLinker;

	FastVector<ExternTypeInfo>	&exTypes;
	FastVector<ExternFuncInfo*>	&exFunctions;
	FastVector<ExternalFunctionInfo>	&exFuncInfo;

	FastVector<asmStackType>	genStackTypes;

	FastVector<char, true>	genParams;
	FastVector<unsigned int>	paramTop;
	FastVector<VMCmd*>	fcallStack;

	unsigned int	*genStackBase;
	unsigned int	*genStackPtr;
	unsigned int	*genStackTop;

	asmOperType	retType;

	bool (*m_RunCallback)(unsigned int);
	
	bool RunExternalFunction(unsigned int funcID);

	void operator=(Executor& r){ (void)r; assert(false); }
};

void PrintInstructionText(FILE* stream, VMCmd cmd, unsigned int rel, unsigned int top);
