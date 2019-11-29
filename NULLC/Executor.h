#pragma once

#include "stdafx.h"
#include "Bytecode.h"
#include "Executor_Common.h"
#include "InstructionSet.h"

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
typedef struct DCCallVM_ DCCallVM;
#endif

const int ERROR_BUFFER_SIZE = 1024;

class Executor
{
public:
	Executor(Linker* linker);
	~Executor();

	void	Run(unsigned int functionID, const char *arguments);
	void	Stop(const char* error);

	bool	SetStackSize(unsigned bytes);

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

	void	SetBreakFunction(void *context, unsigned (*callback)(void*, unsigned));
	void	ClearBreakpoints();
	bool	AddBreakpoint(unsigned int instruction, bool oneHit);
	bool	RemoveBreakpoint(unsigned int instruction);

	void	UpdateInstructionPointer();
private:
	void	InitExecution();

	bool	codeRunning;

	asmOperType		lastResultType;
	int				lastResultInt;
	long long		lastResultLong;
	double			lastResultDouble;

	char		execError[ERROR_BUFFER_SIZE];
	char		execResult[64];

	Linker		*exLinker;

	FastVector<ExternTypeInfo>	&exTypes;
	FastVector<ExternFuncInfo>	&exFunctions;
	char			*symbols;

	unsigned int	minStackSize;

	FastVector<char, true, true>	genParams;
	FastVector<VMCmd*>	fcallStack;

	VMCmd			*cmdBase;
	unsigned int	currentFrame;

	unsigned int	paramBase;

	unsigned int	*genStackBase;
	unsigned int	*genStackPtr;
	unsigned int	*genStackTop;

	bool			callContinue;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	DCCallVM		*dcCallVM;
#endif

	void *breakFunctionContext;
	unsigned (*breakFunction)(void*, unsigned);

	FastVector<VMCmd>	breakCode;

#ifdef NULLC_VM_CALL_STACK_UNWRAP
	FastVector<unsigned>	funcIDStack;
	bool RunCallStackHelper(unsigned funcID, unsigned extraPopDW, unsigned callStackPos);
#endif

	bool RunExternalFunction(unsigned int funcID, unsigned int extraPopDW);

	static const unsigned int	EXEC_BREAK_SIGNAL = 0;
	static const unsigned int	EXEC_BREAK_RETURN = 1;
	static const unsigned int	EXEC_BREAK_ONCE = 2;

private:
	Executor(const Executor&);
	Executor& operator=(const Executor&);
};
