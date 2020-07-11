#pragma once

#include "Array.h"
#include "Bytecode.h"
#include "InstructionTreeRegVm.h"

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
typedef struct DCCallVM_ DCCallVM;
#endif

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

	unsigned	GetCallStackAddress(unsigned frame);

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

	static const unsigned execResultSize = 512;
	char		execResult[execResultSize];

	// Linker and linker data
	Linker		*exLinker;

	FastVector<ExternTypeInfo>	&exTypes;
	FastVector<ExternFuncInfo>	&exFunctions;
	char			*symbols;

	// Placement and layout of dataStack, codeBase and callStack members is used in nullc_debugger_component
	FastVector<char, true, true>	dataStack;

	RegVmCmd	*codeBase;

	FastVector<RegVmCmd*>	callStack;

	unsigned	minStackSize;
	unsigned	lastFinalReturn;

	// Stack for call argument/return result data
	unsigned	*tempStackArrayBase;
	unsigned	*tempStackArrayEnd;

	// Register file
	RegVmRegister	*regFileArrayBase;
	RegVmRegister	*regFileLastTop;
	RegVmRegister	*regFileArrayEnd;

	bool		callContinue;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	DCCallVM	*dcCallVM;
#endif

	void *breakFunctionContext;
	unsigned (*breakFunction)(void*, unsigned);

	FastVector<RegVmCmd>	breakCode;

	static RegVmReturnType RunCode(RegVmCmd *instruction, RegVmRegister * const regFilePtr, ExecutorRegVm *rvm, RegVmCmd *codeBase);

	bool RunExternalFunction(unsigned funcID, unsigned *callStorage);

	RegVmCmd* ExecNop(const RegVmCmd cmd, RegVmCmd * const instruction, RegVmRegister * const regFilePtr);
	bool ExecCall(unsigned microcodePos, unsigned functionId, RegVmCmd * const instruction, RegVmRegister * const regFilePtr);
	RegVmReturnType ExecReturn(const RegVmCmd cmd, RegVmCmd * const instruction, RegVmRegister * const regFilePtr);
	bool ExecConvertPtr(const RegVmCmd cmd, RegVmCmd * const instruction, RegVmRegister * const regFilePtr);
	void ExecCheckedReturn(unsigned typeId, RegVmRegister * const regFilePtr);

	RegVmReturnType ExecError(RegVmCmd * const instruction, const char *errorMessage);

	static const unsigned EXEC_BREAK_SIGNAL = 0;
	static const unsigned EXEC_BREAK_RETURN = 1;
	static const unsigned EXEC_BREAK_ONCE = 2;

private:
	ExecutorRegVm(const ExecutorRegVm&);
	ExecutorRegVm& operator=(const ExecutorRegVm&);
};
