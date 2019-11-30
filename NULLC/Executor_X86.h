#pragma once

#include "stdafx.h"
#include "InstructionTreeRegVm.h"

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
typedef struct DCCallVM_ DCCallVM;
#endif

class Linker;

struct x86Instruction;

struct ExternTypeInfo;
struct ExternFuncInfo;

struct OutputContext;

struct CodeGenRegVmContext;

const int REGVM_X86_ERROR_BUFFER_SIZE = 1024;

class ExecutorX86
{
public:
	ExecutorX86(Linker *linker);
	~ExecutorX86();

	bool	Initialize();

	void	ClearNative();
	bool	TranslateToNative(bool enableLogFiles, OutputContext &output);
	void	SaveListing(OutputContext &output);

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

private:
	bool	InitExecution();

	CodeGenRegVmContext *codeGenCtx;

	bool	codeRunning;

	RegVmReturnType	lastResultType;
	RegVmRegister	lastResult;

	char	execError[REGVM_X86_ERROR_BUFFER_SIZE];
	char	execResult[64];

	// Linker and linker data
	Linker		*exLinker;

	FastVector<ExternTypeInfo>	&exTypes;
	FastVector<ExternFuncInfo>	&exFunctions;
	FastVector<RegVmCmd>		&exRegVmCode;
	FastVector<unsigned int>	&exRegVmConstants;
	FastVector<bool>			codeJumpTargets;

	// Data stack
	unsigned int	minStackSize;

	FastVector<char, true, true>	dataStack;

	FastVector<unsigned>	callStack;
	unsigned	currentFrame;

	unsigned	lastFinalReturn;

	// Stack for call argument/return result data
	unsigned		*tempStackArrayBase;
	unsigned		*tempStackLastTop;
	unsigned		*tempStackArrayEnd;

	// Register file
	RegVmRegister	*regFileArrayBase;
	RegVmRegister	*regFileLastTop;
	RegVmRegister	*regFileArrayEnd;

	bool			callContinue;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	DCCallVM		*dcCallVM;
#endif

	// Native code data
	static const unsigned codeLaunchHeaderSize = 128;
	unsigned char codeLaunchHeader[codeLaunchHeaderSize];
	unsigned oldCodeLaunchHeaderProtect;

	FastVector<x86Instruction, true, true>	instList;

	unsigned char	*binCode;
	uintptr_t		binCodeStart;
	unsigned int	binCodeSize, binCodeReserved;

	unsigned int	lastInstructionCount;

	//unsigned int	*callstackTop;

	unsigned int	oldJumpTargetCount;
	unsigned int	oldFunctionSize;
	unsigned int	oldCodeBodyProtect;

public:
	FastVector<unsigned char*>	instAddress;

	struct RegVmStateContext
	{

	};

	RegVmStateContext vmState;

	void *breakFunctionContext;
	unsigned (*breakFunction)(void*, unsigned);

	struct Breakpoint
	{
		Breakpoint(): instIndex(0), oldOpcode(0), oneHit(false){}
		Breakpoint(unsigned int instIndex, unsigned char oldOpcode, bool oneHit): instIndex(instIndex), oldOpcode(oldOpcode), oneHit(oneHit){}
		unsigned int	instIndex;
		unsigned char	oldOpcode;
		bool			oneHit;
	};
	FastVector<Breakpoint>		breakInstructions;

	FastVector<unsigned int>	functionAddress;
	struct FunctionListInfo
	{
		FunctionListInfo(): list(NULL), count(0){}
		FunctionListInfo(unsigned *list, unsigned count): list(list), count(count){}
		unsigned	*list;
		unsigned	count;
	};
	FastVector<FunctionListInfo>	oldFunctionLists;

private:
	ExecutorX86(const ExecutorX86&);
	ExecutorX86& operator=(const ExecutorX86&);
};
