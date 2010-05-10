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

	void	SetBreakFunction(void (*callback)(unsigned int));
	void	ClearBreakpoints();
	bool	AddBreakpoint(unsigned int instruction);

	bool	SetStackPlacement(void* start, void* end, unsigned int flagMemoryAllocated);
private:
	void	InitExecution();

	bool	codeRunning;

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
	void			*genStackTop, *genStackPtr;

	unsigned char	*binCode;
	unsigned int	binCodeStart;
	unsigned int	binCodeSize, binCodeReserved;

	int				callContinue;

	unsigned int	*callstackTop;

	void (*breakFunction)(unsigned int);
	struct Breakpoint
	{
		unsigned int	instIndex;
		unsigned char	oldOpcodeA;
		unsigned char	oldOpcodeB;
	};
	FastVector<unsigned int>	breakInstructions;

	void operator=(ExecutorX86& r){ (void)r; assert(false); }
};
