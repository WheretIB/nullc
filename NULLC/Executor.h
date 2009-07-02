#pragma once
#include "stdafx.h"
#include "ParseClass.h"

#include "Linker.h"

class Executor
{
public:
	Executor(Linker* linker);
	~Executor();

	void	Run(const char* funcName = NULL) throw();

	const char*	GetResult() throw();
	const char*	GetExecError() throw();

	char*	GetVariableData();

	void	SetCallback(bool (*Func)(unsigned int));
private:
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	ofstream			m_FileStream;
#endif
	char		execError[512];
	char		execResult[64];

	Linker		*exLinker;

	FastVector<ExternTypeInfo*>	&exTypes;
	FastVector<ExternFuncInfo*>	&exFunctions;
	FastVector<ExternalFunctionInfo>	&exFuncInfo;
	FastVector<char>			&exCode;

	FastVector<asmStackType>	genStackTypes;

	FastVector<char, true>	genParams;
	FastVector<unsigned int>	paramTop;
	FastVector<char*>	fcallStack;

	unsigned int	*genStackBase;
	unsigned int	*genStackPtr;
	unsigned int	*genStackTop;

	OperFlag	retType;

	bool (*m_RunCallback)(unsigned int);
	
	bool RunExternalFunction(unsigned int funcID);

	void operator=(Executor& r){ (void)r; assert(false); }
};

void PrintInstructionText(ostream* stream, CmdID cmd, unsigned int pos2, unsigned int valind, const CmdFlag cFlag, const OperFlag oFlag, unsigned int dw0=0, unsigned int dw1=0);
