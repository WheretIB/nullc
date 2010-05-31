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

	void	Run(unsigned int functionID, const char *arguments);
	void	Stop(const char* error);

	const char*	GetResult();
	int			GetResultInt();
	double		GetResultDouble();
	long long	GetResultLong();

	const char*	GetExecError();

	char*	GetVariableData(unsigned int *count);

	void			BeginCallStack();
	unsigned int	GetNextAddress();

	void*			GetStackStart();
	void*			GetStackEnd();

	void	SetBreakFunction(unsigned (*callback)(unsigned int));
	void	ClearBreakpoints();
	bool	AddBreakpoint(unsigned int instruction, bool oneHit);
	bool	RemoveBreakpoint(unsigned int instruction);

	void	UpdateInstructionPointer();
private:
	unsigned int	CreateFunctionGateway(FastVector<unsigned char>	&code, unsigned int funcID);
	void	InitExecution();

	bool	codeRunning;

	asmOperType		lastResultType;
	unsigned int	lastResultH, lastResultL;

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	FILE*		executeLog;
#endif
	char		execError[ERROR_BUFFER_SIZE];
	char		execResult[64];

	Linker		*exLinker;

	FastVector<ExternTypeInfo>	&exTypes;
	FastVector<ExternFuncInfo>	&exFunctions;
	char			*symbols;

	FastVector<unsigned char>	gateCode;

	FastVector<char, true, true>	genParams;
	FastVector<VMCmd*>	fcallStack;

	VMCmd			*cmdBase;
	unsigned int	currentFrame;

	unsigned int	paramBase;

	unsigned int	*genStackBase;
	unsigned int	*genStackPtr;
	unsigned int	*genStackTop;

	bool			callContinue;

	unsigned (*breakFunction)(unsigned int);
	FastVector<VMCmd>	breakCode;

	bool RunExternalFunction(unsigned int funcID, unsigned int extraPopDW);

	void FixupPointer(char* ptr, const ExternTypeInfo& type);
	void FixupArray(char* ptr, const ExternTypeInfo& type);
	void FixupClass(char* ptr, const ExternTypeInfo& type);
	void FixupFunction(char* ptr);
	void FixupVariable(char* ptr, const ExternTypeInfo& type);

	bool ExtendParameterStack(char* oldBase, unsigned int oldSize, VMCmd *current);

	void operator=(Executor& r){ (void)r; assert(false); }

	static const unsigned int	EXEC_BREAK_SIGNAL = 0;
	static const unsigned int	EXEC_BREAK_RETURN = 1;
	static const unsigned int	EXEC_BREAK_ONE_HIT_WONDER = 2;
};

void PrintInstructionText(FILE* stream, VMCmd cmd, unsigned int rel, unsigned int top);
