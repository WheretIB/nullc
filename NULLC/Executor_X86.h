#pragma once

#include "stdafx.h"

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
typedef struct DCCallVM_ DCCallVM;
#endif

class Linker;

struct RegVmCmd;

struct x86Instruction;

struct ExternTypeInfo;
struct ExternFuncInfo;

struct OutputContext;

struct CodeGenRegVmContext;

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
	bool	InitStack();
	bool	InitExecution();

	CodeGenRegVmContext *codeGenCtx;

	bool	codeRunning;

	char	execError[512];
	char	execResult[64];

	Linker		*exLinker;

	FastVector<ExternTypeInfo>	&exTypes;
	FastVector<ExternFuncInfo>	&exFunctions;
	FastVector<RegVmCmd>		&exRegVmCode;
	FastVector<unsigned int>	&exRegVmConstants;
	FastVector<bool>			codeJumpTargets;

	FastVector<x86Instruction, true, true>	instList;

	unsigned int	globalStartInBytecode;

	unsigned int	minStackSize;

	char			*paramBase;
	void			*genStackTop, *genStackPtr;

	unsigned char	*binCode;
	uintptr_t		binCodeStart;
	unsigned int	binCodeSize, binCodeReserved;

	unsigned int	lastInstructionCount;

	int				callContinue;

	unsigned int	*callstackTop;

	unsigned int	oldJumpTargetCount;
	unsigned int	oldFunctionSize;

	unsigned int	oldCodeHeadProtect;
	unsigned int	oldCodeBodyProtect;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	DCCallVM		*dcCallVM;
#endif

public:
	FastVector<unsigned char*>	instAddress;

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
