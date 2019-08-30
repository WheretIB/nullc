#pragma once

#include "Array.h"
#include "Bytecode.h"
#include "InstructionTreeRegVm.h"

typedef struct DCCallVM_ DCCallVM;

class Linker;

const int REGVM_ERROR_BUFFER_SIZE = 1024;

class ExecutorRegVm
{
public:
	ExecutorRegVm(Linker* linker);
	~ExecutorRegVm();

	void	Run(unsigned functionID, const char *arguments);
	void	Stop(const char* error);

	bool	SetStackSize(unsigned bytes);

	const char*	GetResult();
	int			GetResultInt();
	double		GetResultDouble();
	long long	GetResultLong();

	const char*	GetExecError();

	char*		GetVariableData(unsigned *count);

	void		BeginCallStack();
	unsigned	GetNextAddress();

	void*		GetStackStart();
	void*		GetStackEnd();

	void	SetBreakFunction(void *context, unsigned (*callback)(void*, unsigned));
	void	ClearBreakpoints();
	bool	AddBreakpoint(unsigned instruction, bool oneHit);
	bool	RemoveBreakpoint(unsigned instruction);

	void	UpdateInstructionPointer();

private:
	void	InitExecution();

	bool	codeRunning;

	RegVmReturnType	lastResultType;
	RegVmRegister	lastResult;

	char		execError[REGVM_ERROR_BUFFER_SIZE];
	char		execResult[64];

	// Linker and linker data
	Linker		*exLinker;

	FastVector<ExternTypeInfo>	&exTypes;
	FastVector<ExternFuncInfo>	&exFunctions;
	char			*symbols;

	RegVmCmd	*codeBase;

	unsigned	minStackSize;

	FastVector<char, true, true>	dataStack;

	FastVector<RegVmCmd*>	callStack;
	unsigned	currentFrame;

	unsigned	lastFinalReturn;

	// Stack for call argument/return result data
	unsigned	*tempStackArrayBase;
	unsigned	*tempStackLastTop;
	unsigned	*tempStackArrayEnd;

	// Register file
	RegVmRegister	*regFileArrayBase;
	RegVmRegister	*regFileLastTop;
	RegVmRegister	*regFileArrayEnd;

	bool		callContinue;

	DCCallVM	*dcCallVM;

	void *breakFunctionContext;
	unsigned (*breakFunction)(void*, unsigned);

	FastVector<RegVmCmd>	breakCode;

	static RegVmReturnType RunCode(RegVmCmd *instruction, RegVmRegister * const regFilePtr, unsigned *tempStackPtr, ExecutorRegVm *rvm, RegVmCmd *codeBase);

	bool RunExternalFunction(unsigned funcID, unsigned *callStorage);

	RegVmCmd* ExecNop(const RegVmCmd cmd, RegVmCmd * const instruction, RegVmRegister * const regFilePtr);
	unsigned* ExecCall(unsigned char resultReg, unsigned char resultType, unsigned functionId, RegVmCmd * const instruction, RegVmRegister * const regFilePtr, unsigned *tempStackPtr);
	RegVmReturnType ExecReturn(const RegVmCmd cmd, RegVmCmd * const instruction);
	bool ExecConvertPtr(const RegVmCmd cmd, RegVmCmd * const instruction, RegVmRegister * const regFilePtr);
	void ExecCheckedReturn(const RegVmCmd cmd, RegVmRegister * const regFilePtr, unsigned * const tempStackPtr);

	RegVmReturnType ExecError(RegVmCmd * const instruction, const char *errorMessage);

	static const unsigned EXEC_BREAK_SIGNAL = 0;
	static const unsigned EXEC_BREAK_RETURN = 1;
	static const unsigned EXEC_BREAK_ONCE = 2;

private:
	ExecutorRegVm(const ExecutorRegVm&);
	ExecutorRegVm& operator=(const ExecutorRegVm&);
};
