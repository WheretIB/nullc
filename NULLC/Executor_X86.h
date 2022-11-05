#pragma once

#include "stdafx.h"

#include "Array.h"
#include "CodeGenRegVm_X86.h"
#include "InstructionTreeRegVm.h"

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
typedef struct DCCallVM_ DCCallVM;
#endif

#if _MSC_VER <= 1600
typedef struct _RUNTIME_FUNCTION RUNTIME_FUNCTION;
#else
typedef struct _IMAGE_RUNTIME_FUNCTION_ENTRY RUNTIME_FUNCTION;
#endif

class Linker;

struct x86Instruction;

struct ExternTypeInfo;
struct ExternFuncInfo;

struct OutputContext;

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

	bool	Run(unsigned int functionID, const char *arguments);
	void	Stop(const char* error);
	void	Stop(NULLCRef error);
	void	Resume();

	bool	SetStackSize(unsigned bytes);

	unsigned	GetResultType();
	NULLCRef	GetResultObject();
	const char*	GetResult();
	int			GetResultInt();
	double		GetResultDouble();
	long long	GetResultLong();

	const char*	GetErrorMessage();
	NULLCRef	GetErrorObject();

	char*		GetVariableData(unsigned int *count);

	unsigned	GetCallStackAddress(unsigned frame);

	void*		GetStackStart();
	void*		GetStackEnd();

	void	SetBreakFunction(void *context, unsigned (*callback)(void*, unsigned));
	void	ClearBreakpoints();
	bool	AddBreakpoint(unsigned int instruction, bool oneHit);
	bool	RemoveBreakpoint(unsigned int instruction);

	unsigned	GetInstructionAtAddress(void *address);
	bool		IsCodeLaunchHeader(void *address);

private:
	bool	InitExecution();

	CodeGenRegVmContext *codeGenCtx;

	bool	codeRunning;

	static const unsigned execResultSize = 512;
	char	execResult[execResultSize];

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

	unsigned	lastFinalReturn;

public:
	CodeGenRegVmStateContext vmState;

	char	*execErrorBuffer;

	const char	*execErrorMessage;
	NULLCRef	execErrorObject;

	unsigned	execErrorFinalReturnDepth;

	unsigned char	*binCode;
	unsigned		binCodeSize;
	unsigned		binCodeReserved;

	struct ExpiredCodeBlock
	{
		unsigned char *code;
		unsigned codeSize;

#ifdef _M_X64
		RUNTIME_FUNCTION *unwindTable;
#endif
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

	struct ExpiredFunctionAddressList
	{
		unsigned char **data;
		unsigned count;
	};
	FastVector<ExpiredFunctionAddressList> expiredFunctionAddressLists;

	FastVector<unsigned> globalCodeRanges;

#ifdef _M_X64
	FastVector<RUNTIME_FUNCTION> functionWin64UnwindTable;
#endif

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
