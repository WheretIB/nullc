#pragma once

#include "stdafx.h"
#include "InstructionTreeRegVm.h"
#include "CodeGenRegVm_X86.h"

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
typedef struct DCCallVM_ DCCallVM;
#endif

typedef struct _IMAGE_RUNTIME_FUNCTION_ENTRY RUNTIME_FUNCTION;

class Linker;

struct x86Instruction;

struct ExternTypeInfo;
struct ExternFuncInfo;

struct OutputContext;

const int REGVM_X86_ERROR_BUFFER_SIZE = 1024;

class ExecutorX86
{
public:
	ExecutorX86(Linker *linker);
	~ExecutorX86();

	bool	Initialize();

	void	ClearNative();
	bool	TranslateToNative(bool enableLogFiles, OutputContext &output);
	void	UpdateFunctionPointer(unsigned source, unsigned target);
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
	FastVector<unsigned char>	&exRegVmRegKillInfo;
	FastVector<unsigned int>	codeJumpTargets;
	FastVector<unsigned int>	codeRegKillInfoOffsets;

	// Data stack
	unsigned int	minStackSize;

	unsigned	currentFrame;

	unsigned	lastFinalReturn;

public:
	CodeGenRegVmStateContext vmState;

	unsigned char	*binCode;
	unsigned		binCodeSize;
	unsigned		binCodeReserved;

	struct ExpiredCodeBlock
	{
		unsigned char *code;
		unsigned codeSize;
		RUNTIME_FUNCTION *unwindTable;
	};

	FastVector<ExpiredCodeBlock>	expiredCodeBlocks;

private:
	// Native code data
	static const unsigned codeLaunchHeaderSize = 4096;
	unsigned char codeLaunchHeader[codeLaunchHeaderSize];
	unsigned codeLaunchHeaderLength;
	unsigned codeLaunchUnwindOffset;
	unsigned codeLaunchDataLength;
	unsigned oldCodeLaunchHeaderProtect;
	RUNTIME_FUNCTION *codeLaunchWin64UnwindTable;

	FastVector<x86Instruction, true, true>	instList;

	unsigned int	lastInstructionCount;

	unsigned int	oldJumpTargetCount;
	unsigned int	oldRegKillInfoCount;
	unsigned int	oldFunctionSize;
	unsigned int	oldCodeBodyProtect;

public:
	bool			callContinue;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	DCCallVM		*dcCallVM;
#endif

	FastVector<unsigned char*> instAddress;
	FastVector<unsigned char*> functionAddress;

	FastVector<unsigned> globalCodeRanges;
	FastVector<RUNTIME_FUNCTION> functionWin64UnwindTable;

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

private:
	ExecutorX86(const ExecutorX86&);
	ExecutorX86& operator=(const ExecutorX86&);
};
