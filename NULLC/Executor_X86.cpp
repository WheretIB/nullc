#include "stdafx.h"
#ifdef NULLC_BUILD_X86_JIT

#include "Executor_X86.h"
#include "CodeGen_X86.h"
#include "Translator_X86.h"

#ifndef __linux
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#else
	typedef unsigned int DWORD;
	#include <sys/mman.h>
	#ifndef PAGESIZE
		// $ sysconf()
		#define PAGESIZE 4096
	#endif
	#include <signal.h>
#endif

namespace NULLC
{
	// Parameter stack range
	void	*stackBaseAddress;
	void	*stackEndAddress;	// if NULL, range is not upper bound (until allocation fails)
	// Flag that shows that Executor manages memory allocation and deallocation
	bool	stackManaged;
	// If memory is not managed, Executor will place a page guard at the end and restore old protection later
	DWORD	stackProtect;

	// Four global variables
	struct DataStackHeader
	{
		unsigned int	unused1;
		unsigned int	lastEDI;
		unsigned int	instructionPtr;
		unsigned int	nextElement;
	};

	DataStackHeader	*dataHead;
	char* parameterHead;

	// Hidden pointer to the beginning if NULLC parameter stack, skipping DataStackHeader
	unsigned int paramDataBase;
	unsigned int reservedStack;	// Reserved stack size
	unsigned int commitedStack;	// Committed stack size
	unsigned int stackGrowSize;	// The amount by which stack grows
	unsigned int stackGrowCommit;	// The amount that is committed

	// Binary code range in hidden pointers
	unsigned int binCodeStart, binCodeEnd;

	// Code run result - two DWORDs for parts of result and a type flag
	int runResult = 0;
	int runResult2 = 0;
	asmOperType runResultType = OTYPE_DOUBLE;

	// Call stack is made up by a linked list, starting from last frame, this array will hold call stack in correct order
	const unsigned int STACK_TRACE_DEPTH = 1024;
	unsigned int stackTrace[STACK_TRACE_DEPTH];
	// Signal that call stack contains stack of execution that ended in SEH handler with a fatal exception
	volatile bool abnormalTermination;

	// Parameter stack reallocation count
	unsigned int stackReallocs;

	// Part of state that SEH handler saves for future use
	unsigned int expCodePublic;
	unsigned int expAllocCode;
	unsigned int expEAXstate;
	unsigned int expECXstate;
	unsigned int expESPstate;

	ExecutorX86	*currExecutor = NULL;

#ifndef __linux
	int ExtendMemory()
	{
		// External stack cannot be extended, end execution with error
		if(!stackManaged)
		{
			expAllocCode = 5;
			return EXCEPTION_EXECUTE_HANDLER;
		}
		if(stackEndAddress && (char*)stackBaseAddress + commitedStack + stackGrowSize > (char*)stackEndAddress)
		{
			expAllocCode = 2;
			return EXCEPTION_EXECUTE_HANDLER;
		}
		// Check that we haven't exceeded available memory
		if(reservedStack > 512*1024*1024)
		{
			expAllocCode = 4;
			return EXCEPTION_EXECUTE_HANDLER;
		}
		// Allow the use of last reserved memory page
		if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+commitedStack)), stackGrowSize-stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
		{
			expAllocCode = 1; // failed to commit all old memory
			return EXCEPTION_EXECUTE_HANDLER;
		}
		// Reserve new memory right after the block
		if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+reservedStack)), stackGrowSize, MEM_RESERVE, PAGE_NOACCESS))
		{
			expAllocCode = 2; // failed to reserve new memory
			return EXCEPTION_EXECUTE_HANDLER;
		}
		// Allow access to all new reserved memory, except for the last memory page
		if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+reservedStack)), stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
		{
			expAllocCode = 3; // failed to commit new memory
			return EXCEPTION_EXECUTE_HANDLER;
		}
		// Update variables
		commitedStack = reservedStack;
		reservedStack += stackGrowSize;
		commitedStack += stackGrowCommit;
		stackReallocs++;

		SetUnmanagableRange(parameterHead, reservedStack);

		return (DWORD)EXCEPTION_CONTINUE_EXECUTION;
	}

	DWORD CanWeHandleSEH(unsigned int expCode, _EXCEPTION_POINTERS* expInfo)
	{
		// Check that exception happened in NULLC code (division by zero and int overflow still catched)
		bool externalCode = expInfo->ContextRecord->Eip < binCodeStart || expInfo->ContextRecord->Eip > binCodeEnd;
		bool managedMemoryEnd = expInfo->ExceptionRecord->ExceptionInformation[1] > paramDataBase &&
			expInfo->ExceptionRecord->ExceptionInformation[1] < expInfo->ContextRecord->Edi+paramDataBase + 64 * 1024;
		if(externalCode && (expCode == EXCEPTION_BREAKPOINT || expCode == EXCEPTION_STACK_OVERFLOW || (expCode == EXCEPTION_ACCESS_VIOLATION && !managedMemoryEnd)))
			return (DWORD)EXCEPTION_CONTINUE_SEARCH;

		// Save part of state for later use
		expEAXstate = expInfo->ContextRecord->Eax;
		expECXstate = expInfo->ContextRecord->Ecx;
		expESPstate = expInfo->ContextRecord->Esp;
		expCodePublic = expCode;
		expAllocCode = ~0u;

		if(!externalCode && *(unsigned char*)(intptr_t)expInfo->ContextRecord->Eip == 0xcc)
		{
			unsigned index = ~0u;
			for(unsigned i = 0; i < currExecutor->breakInstructions.size() && index == ~0u; i++)
			{
				if((intptr_t)currExecutor->instAddress[currExecutor->breakInstructions[i].instIndex] == expInfo->ContextRecord->Eip)
					index = i;
			}
			//printf("Found at index %d\n", index);
			if(index == ~0u)
				return EXCEPTION_CONTINUE_SEARCH;
			//printf("Returning execution (%d)\n", currExecutor->breakInstructions[index].instIndex);
			//Sleep(8000);

			unsigned array[2] = { expInfo->ContextRecord->Eip, 0 };
			NULLC::dataHead->instructionPtr = (unsigned)(uintptr_t)&array[1];

			/*unsigned command = */currExecutor->breakFunction(currExecutor->breakInstructions[index].instIndex);
			//printf("Returned command %d\n", command);
			*currExecutor->instAddress[currExecutor->breakInstructions[index].instIndex] = currExecutor->breakInstructions[index].oldOpcode;
			return (DWORD)EXCEPTION_CONTINUE_EXECUTION;
		}
		// Call stack should be unwind only once on top level error, since every function in external function call chain will signal an exception if there was an exception before.
		if(!NULLC::abnormalTermination)
		{
			// Create call stack
			dataHead->instructionPtr = expInfo->ContextRecord->Eip;
			unsigned int *paramData = &dataHead->nextElement;
			int count = 0;
			while(count < (STACK_TRACE_DEPTH - 1) && paramData)
			{
				stackTrace[count++] = paramData[-1];
				paramData = (unsigned int*)(long long)(*paramData);
			}
			stackTrace[count] = 0;
			dataHead->nextElement = NULL;
		}

		if(expCode == EXCEPTION_INT_DIVIDE_BY_ZERO || expCode == EXCEPTION_BREAKPOINT || expCode == EXCEPTION_STACK_OVERFLOW ||
			expCode == EXCEPTION_INT_OVERFLOW || (expCode == EXCEPTION_ACCESS_VIOLATION && expInfo->ExceptionRecord->ExceptionInformation[1] < 0x00010000))
		{
#ifndef __DMC__
			// Restore stack guard
			if(expCode == EXCEPTION_STACK_OVERFLOW)
				_resetstkoflw();
#endif
			// Save address of access violation
			if(expCode == EXCEPTION_ACCESS_VIOLATION)
				expECXstate = (unsigned int)expInfo->ExceptionRecord->ExceptionInformation[1];

			// Mark that execution terminated abnormally
			NULLC::abnormalTermination = true;

			return EXCEPTION_EXECUTE_HANDLER;
		}
		if(expCode == EXCEPTION_ACCESS_VIOLATION)
		{
			// If access violation is in some considerable boundaries out of parameter stack, extend it
			if(expInfo->ExceptionRecord->ExceptionInformation[1] > paramDataBase &&
				expInfo->ExceptionRecord->ExceptionInformation[1] < expInfo->ContextRecord->Edi+paramDataBase + 64 * 1024)
			{
				return ExtendMemory();
			}
		}

		return (DWORD)EXCEPTION_CONTINUE_SEARCH;
	}

	typedef BOOL (WINAPI *PSTSG)(PULONG);
	PSTSG pSetThreadStackGuarantee = NULL;
#else
	sigjmp_buf errorHandler;
	
	struct JmpBufData
	{
		char data[sizeof(sigjmp_buf)];
	};
	void HandleError(int signum, struct sigcontext ctx)
	{
		bool externalCode = ctx.eip < binCodeStart || ctx.eip > binCodeEnd;
		if(signum == SIGFPE)
		{
			expCodePublic = EXCEPTION_INT_DIVIDE_BY_ZERO;
			siglongjmp(errorHandler, expCodePublic);
		}
		if(signum == SIGTRAP)
		{
			expCodePublic = EXCEPTION_ARRAY_OUT_OF_BOUNDS;
			siglongjmp(errorHandler, expCodePublic);
		}
		if(signum == SIGSEGV)
		{
			if((void*)ctx.cr2 >= NULLC::stackBaseAddress && (void*)ctx.cr2 <= NULLC::stackEndAddress)
			{
				expCodePublic = stackManaged ? EXCEPTION_FAILED_TO_RESERVE : EXCEPTION_ALLOCATED_STACK_OVERFLOW;

				siglongjmp(errorHandler, expCodePublic);
			}
			if(!externalCode && ctx.cr2 < 0x00010000)
			{
				expCodePublic = EXCEPTION_INVALID_POINTER;
				siglongjmp(errorHandler, expCodePublic);
			}
		}
		signal(signum, SIG_DFL);
		raise(signum);
	}
#endif

	typedef void (*codegenCallback)(VMCmd);
	codegenCallback cgFuncs[cmdEnumCount];

	void UpdateFunctionPointer(unsigned dest, unsigned source)
	{
		currExecutor->functionAddress[dest * 2 + 0] = currExecutor->functionAddress[source * 2 + 0];	// function address
		currExecutor->functionAddress[dest * 2 + 1] = currExecutor->functionAddress[source * 2 + 1];	// function class
		for(unsigned i = 0; i < currExecutor->oldFunctionLists.size(); i++)
		{
			if(currExecutor->oldFunctionLists[i].count < dest * 2)
				continue;
			currExecutor->oldFunctionLists[i].list[dest * 2 + 0] = currExecutor->functionAddress[source * 2 + 0];	// function address
			currExecutor->oldFunctionLists[i].list[dest * 2 + 1] = currExecutor->functionAddress[source * 2 + 1];	// function class
		}
	}
}

// code header
static const unsigned char codeHead[] = {
	0x8B, 0xC4,						// mov         eax,esp

	0x8B, 0x50, 0x10,				// mov         edx,dword ptr [eax+10h] 
	0x89, 0x02,						// mov         dword ptr [edx],eax 

	0x60,							// pushad
	0x8B, 0x78, 0x04,				// mov         edi,dword ptr [eax+4]
	0xBD, 0x00, 0x00, 0x00, 0x00,	// mov         ebp,0
	0x8B, 0x40, 0x0C,				// mov         eax,dword ptr [eax+0Ch]

	0x8D, 0x5C, 0x24, 0x04,			// lea         ebx,[esp+4]
	0x83, 0xE3, 0x0F,				// and         ebx,0Fh
	0xB9, 0x10, 0x00, 0x00, 0x00,	// mov         ecx,10h
	0x2B, 0xCB,						// sub         ecx,ebx
	0x2B, 0xE1,						// sub         esp,ecx
	0x51,							// push        ecx 
	0xFF, 0xD0,						// call eax

	0x59,							// pop         ecx
	0x03, 0xE1,						// add         esp,ecx

	0x8B, 0x4C, 0x24, 0x28,			// mov         ecx,dword ptr [esp+28h]
	0x89, 0x01,						// mov         dword ptr [ecx],eax
	0x89, 0x51, 0x04,				// mov         dword ptr [ecx+4],edx
	0x89, 0x59, 0x08,				// mov         dword ptr [ecx+8],ebx
	0x61,							// popad
	0xC3,							// ret
};

ExecutorX86::ExecutorX86(Linker *linker): exLinker(linker), exFunctions(linker->exFunctions),
			exCode(linker->exCode), exTypes(linker->exTypes)
{
	binCode = NULL;
	binCodeStart = NULL;
	binCodeSize = 0;
	binCodeReserved = 0;

	paramBase = NULL;
	globalStartInBytecode = 0;
	callContinue = 1;

	// Parameter stack must be aligned
	assert(sizeof(NULLC::DataStackHeader) % 16 == 0);

	NULLC::stackBaseAddress = NULL;
	NULLC::stackEndAddress = NULL;
	NULLC::stackManaged = false;

	NULLC::currExecutor = this;

	linker->SetFunctionPointerUpdater(NULLC::UpdateFunctionPointer);

#ifdef __linux
	SetLongJmpTarget(NULLC::errorHandler);
#endif
}
ExecutorX86::~ExecutorX86()
{
#ifdef __linux
	if(NULLC::stackManaged)
	{
		// If stack is managed by Executor, free memory
		NULLC::alignedDealloc(NULLC::stackBaseAddress);
	}else if(NULLC::stackEndAddress){
		// Otherwise, remove page guard, restoring old protection value
		char *p = (char*)((intptr_t)((char*)NULLC::stackEndAddress + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
		if(mprotect(p - 8192, PAGESIZE, PROT_READ | PROT_WRITE))
			asm("int $0x3");
	}
#else
	if(NULLC::stackManaged)
	{
		// If stack is managed by Executor, free memory
		VirtualFree(NULLC::stackBaseAddress, 0, MEM_RELEASE);
	}else{
		// Otherwise, remove page guard, restoring old protection value
		VirtualProtect((char*)NULLC::stackEndAddress - 8192, 4096, NULLC::stackProtect, &NULLC::stackProtect);
	}
#endif

	// Disable execution of code head and code body
#ifndef __linux
	DWORD unusedProtect;
	VirtualProtect((void*)codeHead, sizeof(codeHead), oldCodeHeadProtect, &unusedProtect);
	if(binCode)
		VirtualProtect((void*)binCode, binCodeSize, oldCodeBodyProtect, &unusedProtect);
#else
	char *p = (char*)((intptr_t)((char*)codeHead + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
	if(mprotect(p, ((sizeof(codeHead) / sizeof(codeHead[0]) + PAGESIZE - 1) & ~(PAGESIZE - 1)) + PAGESIZE, PROT_READ | PROT_WRITE))
		asm("int $0x3");
	if(binCode)
	{
		p = (char*)((intptr_t)((char*)binCode + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
		if(mprotect(p, ((binCodeReserved + PAGESIZE - 1) & ~(PAGESIZE - 1)) + PAGESIZE, PROT_READ | PROT_WRITE))
			asm("int $0x3");
	}
#endif

	NULLC::dealloc(binCode);
	binCode = NULL;

	NULLC::currExecutor = NULL;

	for(unsigned i = 0; i < oldFunctionLists.size(); i++)
		NULLC::dealloc(oldFunctionLists[i].list);
	oldFunctionLists.clear();
	functionAddress.clear();

	x86ResetLabels();
}

bool ExecutorX86::Initialize()
{
	using namespace NULLC;

	cgFuncs[cmdNop] = GenCodeCmdNop;

	cgFuncs[cmdPushChar] = GenCodeCmdPushChar;
	cgFuncs[cmdPushShort] = GenCodeCmdPushShort;
	cgFuncs[cmdPushInt] = GenCodeCmdPushInt;
	cgFuncs[cmdPushFloat] = GenCodeCmdPushFloat;
	cgFuncs[cmdPushDorL] = GenCodeCmdPushDorL;
	cgFuncs[cmdPushCmplx] = GenCodeCmdPushCmplx;

	cgFuncs[cmdPushCharStk] = GenCodeCmdPushCharStk;
	cgFuncs[cmdPushShortStk] = GenCodeCmdPushShortStk;
	cgFuncs[cmdPushIntStk] = GenCodeCmdPushIntStk;
	cgFuncs[cmdPushFloatStk] = GenCodeCmdPushFloatStk;
	cgFuncs[cmdPushDorLStk] = GenCodeCmdPushDorLStk;
	cgFuncs[cmdPushCmplxStk] = GenCodeCmdPushCmplxStk;

	cgFuncs[cmdPushImmt] = GenCodeCmdPushImmt;

	cgFuncs[cmdMovChar] = GenCodeCmdMovChar;
	cgFuncs[cmdMovShort] = GenCodeCmdMovShort;
	cgFuncs[cmdMovInt] = GenCodeCmdMovInt;
	cgFuncs[cmdMovFloat] = GenCodeCmdMovFloat;
	cgFuncs[cmdMovDorL] = GenCodeCmdMovDorL;
	cgFuncs[cmdMovCmplx] = GenCodeCmdMovCmplx;

	cgFuncs[cmdMovCharStk] = GenCodeCmdMovCharStk;
	cgFuncs[cmdMovShortStk] = GenCodeCmdMovShortStk;
	cgFuncs[cmdMovIntStk] = GenCodeCmdMovIntStk;
	cgFuncs[cmdMovFloatStk] = GenCodeCmdMovFloatStk;
	cgFuncs[cmdMovDorLStk] = GenCodeCmdMovDorLStk;
	cgFuncs[cmdMovCmplxStk] = GenCodeCmdMovCmplxStk;

	cgFuncs[cmdPop] = GenCodeCmdPop;

	cgFuncs[cmdDtoI] = GenCodeCmdDtoI;
	cgFuncs[cmdDtoL] = GenCodeCmdDtoL;
	cgFuncs[cmdDtoF] = GenCodeCmdDtoF;
	cgFuncs[cmdItoD] = GenCodeCmdItoD;
	cgFuncs[cmdLtoD] = GenCodeCmdLtoD;
	cgFuncs[cmdItoL] = GenCodeCmdItoL;
	cgFuncs[cmdLtoI] = GenCodeCmdLtoI;

	cgFuncs[cmdIndex] = GenCodeCmdIndex;
	cgFuncs[cmdIndexStk] = GenCodeCmdIndex;

	cgFuncs[cmdCopyDorL] = GenCodeCmdCopyDorL;
	cgFuncs[cmdCopyI] = GenCodeCmdCopyI;

	cgFuncs[cmdGetAddr] = GenCodeCmdGetAddr;
	cgFuncs[cmdFuncAddr] = GenCodeCmdFuncAddr;

	cgFuncs[cmdSetRange] = GenCodeCmdSetRange;

	cgFuncs[cmdJmp] = GenCodeCmdJmp;

	cgFuncs[cmdJmpZ] = GenCodeCmdJmpZ;
	cgFuncs[cmdJmpNZ] = GenCodeCmdJmpNZ;

	cgFuncs[cmdCall] = GenCodeCmdCall;
	cgFuncs[cmdCallPtr] = GenCodeCmdCallPtr;

	cgFuncs[cmdReturn] = GenCodeCmdReturn;
	cgFuncs[cmdYield] = GenCodeCmdYield;

	cgFuncs[cmdPushVTop] = GenCodeCmdPushVTop;

	cgFuncs[cmdAdd] = GenCodeCmdAdd;
	cgFuncs[cmdSub] = GenCodeCmdSub;
	cgFuncs[cmdMul] = GenCodeCmdMul;
	cgFuncs[cmdDiv] = GenCodeCmdDiv;
	cgFuncs[cmdPow] = GenCodeCmdPow;
	cgFuncs[cmdMod] = GenCodeCmdMod;
	cgFuncs[cmdLess] = GenCodeCmdLess;
	cgFuncs[cmdGreater] = GenCodeCmdGreater;
	cgFuncs[cmdLEqual] = GenCodeCmdLEqual;
	cgFuncs[cmdGEqual] = GenCodeCmdGEqual;
	cgFuncs[cmdEqual] = GenCodeCmdEqual;
	cgFuncs[cmdNEqual] = GenCodeCmdNEqual;
	cgFuncs[cmdShl] = GenCodeCmdShl;
	cgFuncs[cmdShr] = GenCodeCmdShr;
	cgFuncs[cmdBitAnd] = GenCodeCmdBitAnd;
	cgFuncs[cmdBitOr] = GenCodeCmdBitOr;
	cgFuncs[cmdBitXor] = GenCodeCmdBitXor;
	cgFuncs[cmdLogAnd] = GenCodeCmdLogAnd;
	cgFuncs[cmdLogOr] = GenCodeCmdLogOr;
	cgFuncs[cmdLogXor] = GenCodeCmdLogXor;

	cgFuncs[cmdAddL] = GenCodeCmdAddL;
	cgFuncs[cmdSubL] = GenCodeCmdSubL;
	cgFuncs[cmdMulL] = GenCodeCmdMulL;
	cgFuncs[cmdDivL] = GenCodeCmdDivL;
	cgFuncs[cmdPowL] = GenCodeCmdPowL;
	cgFuncs[cmdModL] = GenCodeCmdModL;
	cgFuncs[cmdLessL] = GenCodeCmdLessL;
	cgFuncs[cmdGreaterL] = GenCodeCmdGreaterL;
	cgFuncs[cmdLEqualL] = GenCodeCmdLEqualL;
	cgFuncs[cmdGEqualL] = GenCodeCmdGEqualL;
	cgFuncs[cmdEqualL] = GenCodeCmdEqualL;
	cgFuncs[cmdNEqualL] = GenCodeCmdNEqualL;
	cgFuncs[cmdShlL] = GenCodeCmdShlL;
	cgFuncs[cmdShrL] = GenCodeCmdShrL;
	cgFuncs[cmdBitAndL] = GenCodeCmdBitAndL;
	cgFuncs[cmdBitOrL] = GenCodeCmdBitOrL;
	cgFuncs[cmdBitXorL] = GenCodeCmdBitXorL;
	cgFuncs[cmdLogAndL] = GenCodeCmdLogAndL;
	cgFuncs[cmdLogOrL] = GenCodeCmdLogOrL;
	cgFuncs[cmdLogXorL] = GenCodeCmdLogXorL;

	cgFuncs[cmdAddD] = GenCodeCmdAddD;
	cgFuncs[cmdSubD] = GenCodeCmdSubD;
	cgFuncs[cmdMulD] = GenCodeCmdMulD;
	cgFuncs[cmdDivD] = GenCodeCmdDivD;
	cgFuncs[cmdPowD] = GenCodeCmdPowD;
	cgFuncs[cmdModD] = GenCodeCmdModD;
	cgFuncs[cmdLessD] = GenCodeCmdLessD;
	cgFuncs[cmdGreaterD] = GenCodeCmdGreaterD;
	cgFuncs[cmdLEqualD] = GenCodeCmdLEqualD;
	cgFuncs[cmdGEqualD] = GenCodeCmdGEqualD;
	cgFuncs[cmdEqualD] = GenCodeCmdEqualD;
	cgFuncs[cmdNEqualD] = GenCodeCmdNEqualD;

	cgFuncs[cmdNeg] = GenCodeCmdNeg;
	cgFuncs[cmdNegL] = GenCodeCmdNegL;
	cgFuncs[cmdNegD] = GenCodeCmdNegD;

	cgFuncs[cmdBitNot] = GenCodeCmdBitNot;
	cgFuncs[cmdBitNotL] = GenCodeCmdBitNotL;

	cgFuncs[cmdLogNot] = GenCodeCmdLogNot;
	cgFuncs[cmdLogNotL] = GenCodeCmdLogNotL;

	cgFuncs[cmdIncI] = GenCodeCmdIncI;
	cgFuncs[cmdIncD] = GenCodeCmdIncD;
	cgFuncs[cmdIncL] = GenCodeCmdIncL;

	cgFuncs[cmdDecI] = GenCodeCmdDecI;
	cgFuncs[cmdDecD] = GenCodeCmdDecD;
	cgFuncs[cmdDecL] = GenCodeCmdDecL;

	cgFuncs[cmdCreateClosure] = GenCodeCmdCreateClosure;
	cgFuncs[cmdCloseUpvals] = GenCodeCmdCloseUpvalues;

	cgFuncs[cmdConvertPtr] = GenCodeCmdConvertPtr;

	cgFuncs[cmdCheckedRet] = GenCodeCmdCheckedRet;

#ifndef __linux
	HMODULE hDLL = LoadLibrary("kernel32");
	pSetThreadStackGuarantee = (PSTSG)GetProcAddress(hDLL, "SetThreadStackGuarantee");
#endif

	codeRunning = false;

	// Enable execution of code head
#ifndef __linux
	VirtualProtect((void*)codeHead, sizeof(codeHead), PAGE_EXECUTE_READWRITE, (DWORD*)&oldCodeHeadProtect);
#else
	char *p = (char*)((intptr_t)((char*)codeHead + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
	if(mprotect(p, ((sizeof(codeHead) / sizeof(codeHead[0]) + PAGESIZE - 1) & ~(PAGESIZE - 1)) + PAGESIZE, PROT_READ | PROT_EXEC))
		asm("int $0x3");
#endif

	// Default mode - stack is managed by Executor and starts from 0x20000000
	return SetStackPlacement((void*)0x20000000, NULL, false);
}

bool ExecutorX86::SetStackPlacement(void* start, void* end, unsigned int flagMemoryAllocated)
{
#ifdef __linux
	if(NULLC::stackManaged && NULLC::stackBaseAddress)
	{
		char *p = (char*)((intptr_t)((char*)NULLC::stackEndAddress + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
		if(mprotect(p - 8192, PAGESIZE, PROT_READ | PROT_WRITE))
			asm("int $0x3");
		// If stack is managed by Executor, free memory
		NULLC::alignedDealloc(NULLC::stackBaseAddress);
	}else if(NULLC::stackEndAddress){
		// Otherwise, remove page guard, restoring old protection value
		char *p = (char*)((intptr_t)((char*)NULLC::stackEndAddress + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
		if(mprotect(p - 8192, PAGESIZE, PROT_READ | PROT_WRITE))
			asm("int $0x3");
	}
#else
	// If old memory was allocated here using VirtualAlloc
	if(NULLC::stackManaged)
	{
		// If stack is managed by Executor, free memory
		VirtualFree(NULLC::stackBaseAddress, 0, MEM_RELEASE);
	}else{
		// Otherwise, remove page guard, restoring old protection value
		VirtualProtect((char*)NULLC::stackEndAddress - 8192, 4096, NULLC::stackProtect, &NULLC::stackProtect);
	}
#endif
	NULLC::stackBaseAddress = start;
	NULLC::stackEndAddress = end;
	NULLC::stackManaged = !flagMemoryAllocated;
	if(NULLC::stackManaged)
	{
		NULLC::stackGrowSize = 128 * 4096;
		NULLC::stackGrowCommit = 64 * 4096;

		char *paramData = NULL;
		// Request memory at address
#ifdef __linux
		if(NULLC::stackEndAddress)
			NULLC::stackGrowCommit = NULLC::stackGrowSize = int((char*)end - (char*)start);
		if(NULL == (paramData = (char*)NULLC::alignedAlloc(NULLC::stackGrowSize)))
		{
			strcpy(execError, "ERROR: failed to allocate memory");
			return false;
		}
		NULLC::stackBaseAddress = paramData;
		NULLC::stackEndAddress = (char*)paramData + NULLC::stackGrowSize;
		char *p = (char*)((intptr_t)((char*)NULLC::stackEndAddress + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
		if(mprotect(p - 8192, PAGESIZE, 0))
			asm("int $0x3");
#else
		if(NULL == (paramData = (char*)VirtualAlloc(NULLC::stackBaseAddress, NULLC::stackGrowSize, MEM_RESERVE, PAGE_NOACCESS)))
		{
			strcpy(execError, "ERROR: failed to reserve memory");
			return false;
		}
		if(!VirtualAlloc(NULLC::stackBaseAddress, NULLC::stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
		{
			strcpy(execError, "ERROR: failed to commit memory");
			return false;
		}
#endif
		NULLC::reservedStack = NULLC::stackGrowSize;
		NULLC::commitedStack = NULLC::stackGrowCommit;

		NULLC::parameterHead = paramBase = paramData + sizeof(NULLC::DataStackHeader);
		NULLC::paramDataBase = static_cast<unsigned int>(reinterpret_cast<long long>(paramData));
		NULLC::dataHead = (NULLC::DataStackHeader*)paramData;

		SetUnmanagableRange(NULLC::parameterHead, NULLC::reservedStack);
	}else{
		// If memory was allocated, setup parameters in a way that stack will not get out of range
		if(NULLC::stackEndAddress < (char*)NULLC::stackBaseAddress + 8192)
		{
			strcpy(execError, "ERROR: Stack memory range is too small or base pointer exceeds end pointer");
			return false;
		}
#ifdef __linux
		char *p = (char*)((intptr_t)((char*)NULLC::stackEndAddress + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
		if(mprotect(p - 8192, PAGESIZE, 0))
			asm("int $0x3");
#else
		VirtualProtect((char*)NULLC::stackEndAddress - 8192, 4096, PAGE_NOACCESS, &NULLC::stackProtect);
#endif

		NULLC::parameterHead = paramBase = (char*)NULLC::stackBaseAddress + sizeof(NULLC::DataStackHeader);
		NULLC::paramDataBase = static_cast<unsigned int>(reinterpret_cast<long long>(NULLC::stackBaseAddress));
		NULLC::dataHead = (NULLC::DataStackHeader*)NULLC::stackBaseAddress;

		NULLC::commitedStack = (unsigned int)((char*)NULLC::stackEndAddress - (char*)NULLC::stackBaseAddress - 12 * 1024);
	}
	return true;
}

void ExecutorX86::InitExecution()
{
	if(!exCode.size())
	{
		strcpy(execError, "ERROR: no code to run");
		return;
	}
	SetUnmanagableRange(NULLC::parameterHead, NULLC::reservedStack);

	execError[0] = 0;
	callContinue = 1;

	NULLC::stackReallocs = 0;
	NULLC::dataHead->lastEDI = 0;
	NULLC::dataHead->instructionPtr = NULL;
	NULLC::dataHead->nextElement = NULL;

#ifndef __linux
	if(NULLC::pSetThreadStackGuarantee)
	{
		unsigned long extraStack = 4096;
		NULLC::pSetThreadStackGuarantee(&extraStack);
	}
#endif
	memset(NULLC::stackBaseAddress, 0, sizeof(NULLC::DataStackHeader));
}

void ExecutorX86::Run(unsigned int functionID, const char *arguments)
{
	int	callStackExtra[2];
	bool firstRun = false;
	if(!codeRunning || functionID == ~0u)
	{
		firstRun = true;
		InitExecution();
	}else if(functionID != ~0u && exFunctions[functionID].startInByteCode != ~0u){
		// Instruction pointer is expected one DWORD above pointer to next element
		callStackExtra[0] = ((int*)(intptr_t)NULLC::dataHead->instructionPtr)[-1];
		// Set next element
		callStackExtra[1] = NULLC::dataHead->nextElement;
		// Now this structure is current element
		NULLC::dataHead->nextElement = (int)(intptr_t)&callStackExtra[1];
	}
	bool wasCodeRunning = codeRunning;
	codeRunning = true;

	unsigned int binCodeStart = static_cast<unsigned int>(reinterpret_cast<long long>(&binCode[16]));
	unsigned int varSize = (exLinker->globalVarSize + 0xf) & ~0xf;

	if(functionID != ~0u)
	{
		if(exFunctions[functionID].startInByteCode == ~0u)
		{
			unsigned int dwordsToPop = (exFunctions[functionID].bytesToPop >> 2);
			void* fPtr = exFunctions[functionID].funcPtr;
			unsigned int retType = exFunctions[functionID].retType;

			unsigned int *stackStart = ((unsigned int*)arguments) + dwordsToPop - 1;
			for(unsigned int i = 0; i < dwordsToPop; i++)
			{
#ifdef __GNUC__
				asm("movl %0, %%eax"::"r"(stackStart):"%eax");
				asm("pushl (%eax)");
#else
				__asm{ mov eax, dword ptr[stackStart] }
				__asm{ push dword ptr[eax] }
#endif
				stackStart--;
			}
			switch(retType)
			{
			case ExternFuncInfo::RETURN_VOID:
				NULLC::runResultType = OTYPE_COMPLEX;
				((void (*)())fPtr)();
				break;
			case ExternFuncInfo::RETURN_INT:
				NULLC::runResultType = OTYPE_INT;
				NULLC::runResult = ((int (*)())fPtr)();
				break;
			case ExternFuncInfo::RETURN_DOUBLE:
			{
				double tmp = ((double (*)())fPtr)();
				NULLC::runResultType = OTYPE_DOUBLE;
				NULLC::runResult2 = ((int*)&tmp)[0];
				NULLC::runResult = ((int*)&tmp)[1];
			}
				break;
			case ExternFuncInfo::RETURN_LONG:
			{
				long long tmp = ((long long (*)())fPtr)();
				NULLC::runResultType = OTYPE_LONG;
				NULLC::runResult2 = ((int*)&tmp)[0];
				NULLC::runResult = ((int*)&tmp)[1];
			}
				break;
			}
#ifdef __GNUC__
			asm("movl %0, %%eax"::"r"(dwordsToPop):"%eax");
			asm("leal (%esp, %eax, 0x4), %esp");
#else
			__asm{ mov eax, dwordsToPop }
			__asm{ lea esp, [eax * 4 + esp] }
#endif
			return;
		}else{
			if(NULLC::dataHead->lastEDI)
				varSize = NULLC::dataHead->lastEDI;
			memcpy(paramBase + varSize, arguments, exFunctions[functionID].bytesToPop);
			binCodeStart = functionAddress[functionID * 2];
		}
	}else{
		binCodeStart += globalStartInBytecode;
#ifndef __linux
		while(NULLC::commitedStack < exLinker->globalVarSize)
		{
			if(NULLC::ExtendMemory() != (DWORD)EXCEPTION_CONTINUE_EXECUTION)
			{
				strcpy(execError, "ERROR: allocated stack overflow");
				return;
			}
		}
#else
		if(NULLC::commitedStack < exLinker->globalVarSize)
		{
			strcpy(execError, "ERROR: allocated stack overflow");
			return;
		}
#endif
		memset(NULLC::parameterHead, 0, exLinker->globalVarSize);
	}

	unsigned int res1 = 0;
	unsigned int res2 = 0;
	unsigned int resT = 0;

	NULLC::abnormalTermination = false;

#ifdef __linux
	struct sigaction sa;
	struct sigaction sigFPE;
	struct sigaction sigTRAP;
	struct sigaction sigSEGV;
	if(firstRun)
	{
		sa.sa_handler = (void (*)(int))NULLC::HandleError;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;

		sigaction(SIGFPE, &sa, &sigFPE);
		sigaction(SIGTRAP, &sa, &sigTRAP);
		sigaction(SIGSEGV, &sa, &sigSEGV);
	}
	int errorCode = 0;

	NULLC::JmpBufData data;
	memcpy(data.data, NULLC::errorHandler, sizeof(sigjmp_buf));
	if(!(errorCode = sigsetjmp(NULLC::errorHandler, 1)))
	{
		unsigned savedSize = NULLC::dataHead->lastEDI;
		void *dummy = NULL;
		typedef	void (*nullcFunc)(int /*varSize*/, int* /*returnStruct*/, unsigned /*codeStart*/, void** /*genStackTop*/);
		nullcFunc gate = (nullcFunc)(intptr_t)codeHead;
		int returnStruct[3] = { 1, 2, 3 };
		gate(varSize, returnStruct, binCodeStart, firstRun ? &genStackTop : &dummy);
		res1 = returnStruct[0];
		res2 = returnStruct[1];
		resT = returnStruct[2];
		NULLC::dataHead->lastEDI = savedSize;
	}else{
		if(errorCode == EXCEPTION_INT_DIVIDE_BY_ZERO)
			strcpy(execError, "ERROR: integer division by zero");
		else if(errorCode == EXCEPTION_FUNCTION_NO_RETURN)
			strcpy(execError, "ERROR: function didn't return a value");
		else if(errorCode == EXCEPTION_ARRAY_OUT_OF_BOUNDS)
			strcpy(execError, "ERROR: array index out of bounds");
		else if(errorCode == EXCEPTION_INVALID_FUNCTION)
			strcpy(execError, "ERROR: invalid function pointer");
		else if((errorCode & 0xff) == EXCEPTION_CONVERSION_ERROR)
			SafeSprintf(execError, 512, "ERROR: cannot convert from %s ref to %s ref",
			&exLinker->exSymbols[exLinker->exTypes[NULLC::dataHead->unused1].offsetToName],
			&exLinker->exSymbols[exLinker->exTypes[errorCode >> 8].offsetToName]);
		else if(errorCode == EXCEPTION_ALLOCATED_STACK_OVERFLOW)
			strcpy(execError, "ERROR: allocated stack overflow");
		else if(errorCode == EXCEPTION_INVALID_POINTER)
			strcpy(execError, "ERROR: null pointer access");
		else if(errorCode == EXCEPTION_FAILED_TO_RESERVE)
			strcpy(execError, "ERROR: failed to reserve new stack memory");

		if(!NULLC::abnormalTermination && NULLC::dataHead->instructionPtr)
		{
			// Create call stack
			unsigned int *paramData = &NULLC::dataHead->nextElement;
			int count = 0;
			while((unsigned)count < (NULLC::STACK_TRACE_DEPTH - 1) && paramData)
			{
				NULLC::stackTrace[count++] = paramData[-1];
				paramData = (unsigned int*)(long long)(*paramData);
			}
			NULLC::stackTrace[count] = 0;
			NULLC::dataHead->nextElement = NULL;
		}
		NULLC::dataHead->instructionPtr = NULL;
		NULLC::abnormalTermination = true;
	}
	// Disable signal handlers only from top-level Run
	if(!wasCodeRunning)
	{
		sigaction(SIGFPE, &sigFPE, NULL);
		sigaction(SIGTRAP, &sigTRAP, NULL);
		sigaction(SIGSEGV, &sigSEGV, NULL);
	}

	memcpy(NULLC::errorHandler, data.data, sizeof(sigjmp_buf));
#else
	__try
	{
		unsigned savedSize = NULLC::dataHead->lastEDI;
		void *dummy = NULL;
		typedef	void (*nullcFunc)(int /*varSize*/, int* /*returnStruct*/, unsigned /*codeStart*/, void** /*genStackTop*/);
		nullcFunc gate = (nullcFunc)(intptr_t)codeHead;
		int returnStruct[3] = { 1, 2, 3 };
		gate(varSize, returnStruct, binCodeStart, firstRun ? &genStackTop : &dummy);
		res1 = returnStruct[0];
		res2 = returnStruct[1];
		resT = returnStruct[2];
		NULLC::dataHead->lastEDI = savedSize;
	}__except(NULLC::CanWeHandleSEH(GetExceptionCode(), GetExceptionInformation())){
		if(NULLC::expCodePublic == EXCEPTION_INT_DIVIDE_BY_ZERO)
			strcpy(execError, "ERROR: integer division by zero");
		else if(NULLC::expCodePublic == EXCEPTION_INT_OVERFLOW)
			strcpy(execError, "ERROR: integer overflow");
		else if(NULLC::expCodePublic == EXCEPTION_BREAKPOINT && NULLC::expECXstate == 0)
			strcpy(execError, "ERROR: array index out of bounds");
		else if(NULLC::expCodePublic == EXCEPTION_BREAKPOINT && NULLC::expECXstate == 0xFFFFFFFF)
			strcpy(execError, "ERROR: function didn't return a value");
		else if(NULLC::expCodePublic == EXCEPTION_BREAKPOINT && NULLC::expECXstate == 0xDEADBEEF)
			strcpy(execError, "ERROR: invalid function pointer");
		else if(NULLC::expCodePublic == EXCEPTION_BREAKPOINT && NULLC::expECXstate != NULLC::expESPstate)
			SafeSprintf(execError, 512, "ERROR: cannot convert from %s ref to %s ref",
			NULLC::expEAXstate >= exLinker->exTypes.size() ? "%unknown%" : &exLinker->exSymbols[exLinker->exTypes[NULLC::expEAXstate].offsetToName],
			NULLC::expECXstate >= exLinker->exTypes.size() ? "%unknown%" : &exLinker->exSymbols[exLinker->exTypes[NULLC::expECXstate].offsetToName]);
		else if(NULLC::expCodePublic == EXCEPTION_STACK_OVERFLOW)
			strcpy(execError, "ERROR: stack overflow");
		else if(NULLC::expCodePublic == EXCEPTION_ACCESS_VIOLATION)
		{
			if(NULLC::expAllocCode == 1)
				strcpy(execError, "ERROR: failed to commit old stack memory");
			else if(NULLC::expAllocCode == 2)
				strcpy(execError, "ERROR: failed to reserve new stack memory");
			else if(NULLC::expAllocCode == 3)
				strcpy(execError, "ERROR: failed to commit new stack memory");
			else if(NULLC::expAllocCode == 4)
				strcpy(execError, "ERROR: no more memory (512Mb maximum exceeded)");
			else if(NULLC::expAllocCode == 5)
				strcpy(execError, "ERROR: allocated stack overflow");
			else
				strcpy(execError, "ERROR: null pointer access");
		}
	}
#endif

	NULLC::runResult = res1;
	NULLC::runResult2 = res2;
	if(functionID == ~0u)
	{
		NULLC::runResultType = (asmOperType)resT;
	}else{
		if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_VOID)
		{
			NULLC::runResultType = OTYPE_COMPLEX;
		}else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_INT){
			NULLC::runResultType = OTYPE_INT;
		}else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_DOUBLE){
			NULLC::runResultType = OTYPE_DOUBLE;
		}else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_LONG){
			NULLC::runResultType = OTYPE_LONG;
		}
	}

	if(!wasCodeRunning)
	{
		if(execError[0] != '\0')
		{
			char *currPos = execError + strlen(execError);
			currPos += SafeSprintf(currPos, 512 - int(currPos - execError), "\r\nCall stack:\r\n");

			BeginCallStack();
			while(unsigned int address = GetNextAddress())
				currPos += PrintStackFrame(address, currPos, 512 - int(currPos - execError));
		}
		codeRunning = false;
		NULLC::dataHead->instructionPtr = NULL;
	}

	// Call stack management
	if(codeRunning && functionID != ~0u && exFunctions[functionID].startInByteCode != ~0u)
	{
		// Restore previous state
		NULLC::dataHead->nextElement = callStackExtra[1];
	}
}

void ExecutorX86::Stop(const char* error)
{
	callContinue = false;
	SafeSprintf(execError, 512, error);
}

void ExecutorX86::ClearNative()
{
	memset(instList.data, 0, sizeof(x86Instruction) * instList.size());
	instList.clear();

	binCodeSize = 0;
	lastInstructionCount = 0;
	for(unsigned i = 0; i < oldFunctionLists.size(); i++)
		NULLC::dealloc(oldFunctionLists[i].list);
	oldFunctionLists.clear();

	functionAddress.clear();

	oldJumpTargetCount = 0;
	oldFunctionSize = 0;
}

bool ExecutorX86::TranslateToNative()
{
	execError[0] = 0;

	globalStartInBytecode = 0xffffffff;

	if(codeRunning && functionAddress.max <= exFunctions.size() * 2)
	{
		unsigned *newStorage = (unsigned*)NULLC::alloc(exFunctions.size() * 3 * sizeof(unsigned));
		oldFunctionLists.push_back(FunctionListInfo(functionAddress.data, functionAddress.count));
		functionAddress.data = newStorage;
		functionAddress.count = exFunctions.size() * 2;
		functionAddress.max = exFunctions.size() * 3;
	}else{
		functionAddress.resize(exFunctions.size() * 2);
	}

	memset(instList.data, 0, sizeof(x86Instruction) * instList.size());
	instList.clear();

	SetParamBase((unsigned int)(long long)paramBase);
	SetFunctionList(exFunctions.data, functionAddress.data);
	SetContinuePtr(&callContinue);
	SetLastInstruction(instList.data, instList.data);

	CommonSetLinker(exLinker);

	EMIT_OP(o_use32);

	// Mirror extra global return so that jump to global return can be marked (cmdNop, because we will have some custom code)
	exCode.push_back(VMCmd(cmdNop));
	for(unsigned int i = oldJumpTargetCount, e = exLinker->jumpTargets.size(); i != e; i++)
		exCode[exLinker->jumpTargets[i]].cmd |= 0x80;
	// Remove cmdNop, because we don't want to generate code for it
	exCode.pop_back();

	OptimizationLookBehind(false);
	unsigned int pos = lastInstructionCount;
	while(pos < exCode.size())
	{
		VMCmd &cmd = exCode[pos];

		unsigned int currSize = (int)(GetLastInstruction() - instList.data);
		instList.count = currSize;
		if(currSize + 64 >= instList.max)
			instList.grow(currSize + 64);
		SetLastInstruction(instList.data + currSize, instList.data);

		GetLastInstruction()->instID = pos + 1;

		if(cmd.cmd & 0x80)
		{
			OptimizationLookBehind(false);
			cmd.cmd &= ~0x80;
		}

		pos++;
		NULLC::cgFuncs[cmd.cmd](cmd);

		OptimizationLookBehind(true);
	}
	// Add extra global return if there is none
	GetLastInstruction()->instID = pos + 1;
	EMIT_OP_REG(o_pop, rEBP);
	EMIT_OP_REG_NUM(o_mov, rEBX, ~0u);
	EMIT_OP(o_ret);
	instList.resize((int)(GetLastInstruction() - &instList[0]));

	// Once again, mirror extra global return so that jump to global return can be marked (cmdNop, because we will have some custom code)
	exCode.push_back(VMCmd(cmdNop));
#ifdef NULLC_OPTIMIZE_X86
	// Second optimization pass, just feed generated instructions again
	// But at first, mark invalidation instructions
	for(unsigned int i = oldJumpTargetCount, e = exLinker->jumpTargets.size(); i != e; i++)
		exCode[exLinker->jumpTargets[i]].cmd |= 0x80;

	// Set iterator at beginning
	SetLastInstruction(instList.data, instList.data);
	OptimizationLookBehind(false);
	// Now regenerate instructions
	for(unsigned int i = 0; i < instList.size(); i++)
	{
		// Skip trash
		if(instList[i].name == o_other || instList[i].name == o_none)
		{
			//EMIT_OP(o_none);
			if(instList[i].instID && (exCode[instList[i].instID-1].cmd & 0x80))
			{
				GetLastInstruction()->instID = instList[i].instID;
				EMIT_OP(o_none);
				OptimizationLookBehind(false);
			}
			continue;
		}
		// If invalidation flag is set
		if(instList[i].instID && (exCode[instList[i].instID-1].cmd & 0x80))
			OptimizationLookBehind(false);
		GetLastInstruction()->instID = instList[i].instID;

		x86Instruction &inst = instList[i];
		if(inst.name == o_label)
		{
			EMIT_LABEL(inst.labelID, inst.argA.num);
			OptimizationLookBehind(true);
			continue;
		}
		switch(inst.argA.type)
		{
		case x86Argument::argNone:
			EMIT_OP(inst.name);
			break;
		case x86Argument::argNumber:
			EMIT_OP_NUM(inst.name, inst.argA.num);
			break;
		case x86Argument::argFPReg:
			EMIT_OP_FPUREG(inst.name, inst.argA.fpArg);
			break;
		case x86Argument::argLabel:
			EMIT_OP_LABEL(inst.name, inst.argA.labelID, inst.argB.num, inst.argB.ptrNum);
			break;
		case x86Argument::argReg:
			switch(inst.argB.type)
			{
			case x86Argument::argNone:
				EMIT_OP_REG(inst.name, inst.argA.reg);
				break;
			case x86Argument::argNumber:
				EMIT_OP_REG_NUM(inst.name, inst.argA.reg, inst.argB.num);
				break;
			case x86Argument::argReg:
				EMIT_OP_REG_REG(inst.name, inst.argA.reg, inst.argB.reg);
				break;
			case x86Argument::argPtr:
				EMIT_OP_REG_RPTR(inst.name, inst.argA.reg, inst.argB.ptrSize, inst.argB.ptrIndex, inst.argB.ptrMult, inst.argB.ptrBase, inst.argB.ptrNum);
				break;
			case x86Argument::argPtrLabel:
				EMIT_OP_REG_LABEL(inst.name, inst.argA.reg, inst.argB.labelID, inst.argB.ptrNum);
				break;
			}
			break;
		case x86Argument::argPtr:
			switch(inst.argB.type)
			{
			case x86Argument::argNone:
				EMIT_OP_RPTR(inst.name, inst.argA.ptrSize, inst.argA.ptrIndex, inst.argA.ptrMult, inst.argA.ptrBase, inst.argA.ptrNum);
				break;
			case x86Argument::argNumber:
				EMIT_OP_RPTR_NUM(inst.name, inst.argA.ptrSize, inst.argA.ptrIndex, inst.argA.ptrMult, inst.argA.ptrBase, inst.argA.ptrNum, inst.argB.num);
				break;
			case x86Argument::argReg:
				EMIT_OP_RPTR_REG(inst.name, inst.argA.ptrSize, inst.argA.ptrIndex, inst.argA.ptrMult, inst.argA.ptrBase, inst.argA.ptrNum, inst.argB.reg);
				break;
			}
			break;
		}
		OptimizationLookBehind(true);
	}
	unsigned int currSize = (int)(GetLastInstruction() - &instList[0]);
	for(unsigned int i = currSize; i < instList.size(); i++)
	{
		instList[i].name = o_other;
		instList[i].instID = 0;
	}
#endif

#ifdef NULLC_LOG_FILES
	static unsigned int instCount = 0;
	for(unsigned int i = 0; i < instList.size(); i++)
		if(instList[i].name != o_none && instList[i].name != o_other)
			instCount++;
	printf("So far, %d optimizations (%d instructions)\r\n", GetOptimizationCount(), instCount);

	FILE *fAsm = fopen("asmX86.txt", "wb");
	char instBuf[128];
	for(unsigned int i = 0, e = exLinker->jumpTargets.size(); i != e; i++)
		exCode[exLinker->jumpTargets[i]].cmd |= 0x80;
	for(unsigned int i = 0; i < instList.size(); i++)
	{
		if(instList[i].name == o_other)
			continue;
		if(instList[i].instID && (exCode[instList[i].instID-1].cmd & 0x80))
		{
			fprintf(fAsm, "; ------------------- Invalidation ----------------\r\n");
			fprintf(fAsm, "0x%x\r\n", 0xc0000000 | (instList[i].instID - 1));
		}
		instList[i].Decode(instBuf);
		fprintf(fAsm, "%s\r\n", instBuf);
	}
	for(unsigned int i = 0, e = exLinker->jumpTargets.size(); i != e; i++)
		exCode[exLinker->jumpTargets[i]].cmd &= ~0x80;
	fclose(fAsm);
#endif
	exCode.pop_back();

	bool codeRelocated = false;
	if((binCodeSize + instList.size() * 6) > binCodeReserved)
	{
		unsigned int oldBinCodeReserved = binCodeReserved;
		binCodeReserved = binCodeSize + (instList.size()) * 6 + 4096;	// Average instruction size is 6 bytes.
		unsigned char *binCodeNew = (unsigned char*)NULLC::alloc(binCodeReserved);

		// Disable execution of old code body and enable execution of new code body
#ifndef __linux
		DWORD unusedProtect;
		if(binCode)
			VirtualProtect((void*)binCode, oldBinCodeReserved, oldCodeBodyProtect, (DWORD*)&unusedProtect);
		VirtualProtect((void*)binCodeNew, binCodeReserved, PAGE_EXECUTE_READWRITE, (DWORD*)&oldCodeBodyProtect);
#else
		if(binCode)
		{
			char *p = (char*)((intptr_t)((char*)binCode + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
			if(mprotect(p, ((oldBinCodeReserved + PAGESIZE - 1) & ~(PAGESIZE - 1)) + PAGESIZE, PROT_READ | PROT_WRITE))
				asm("int $0x3");
		}
		char *p = (char*)((intptr_t)((char*)binCodeNew + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
		if(mprotect(p, ((binCodeReserved + PAGESIZE - 1) & ~(PAGESIZE - 1)) + PAGESIZE, PROT_READ | PROT_WRITE | PROT_EXEC))
			asm("int $0x3");
#endif

		if(binCodeSize)
			memcpy(binCodeNew + 16, binCode + 16, binCodeSize);
		NULLC::dealloc(binCode);
		// If code is currently running, fix call stack (return addresses)
		if(codeRunning)
		{
			codeRelocated = true;
			// This must be an external function call
			assert(NULLC::dataHead->instructionPtr);
			
			unsigned *retvalpos = (unsigned*)(uintptr_t)NULLC::dataHead->instructionPtr - 1;
			if(*retvalpos >= NULLC::binCodeStart && *retvalpos <= NULLC::binCodeEnd)
				*retvalpos = (*retvalpos - NULLC::binCodeStart) + (unsigned)(uintptr_t)(binCodeNew + 16);

			unsigned *paramData = &NULLC::dataHead->nextElement;
			while(paramData)
			{
				unsigned *retvalpos = paramData - 1;
				if(*retvalpos >= NULLC::binCodeStart && *retvalpos <= NULLC::binCodeEnd)
					*retvalpos = (*retvalpos - NULLC::binCodeStart) + (unsigned)(uintptr_t)(binCodeNew + 16);
				paramData = (unsigned int*)(long long)(*paramData);
			}
		}
		for(unsigned i = 0; i < instAddress.size(); i++)
			instAddress[i] = (instAddress[i] - NULLC::binCodeStart) + (unsigned)(uintptr_t)(binCodeNew + 16);
		binCode = binCodeNew;
		binCodeStart = (unsigned int)(intptr_t)(binCode + 16);
	}
	SetBinaryCodeBase(binCode);
	NULLC::binCodeStart = binCodeStart;
	NULLC::binCodeEnd = binCodeStart + binCodeReserved;

	// Translate to x86
	unsigned char *bytecode = binCode + 16 + binCodeSize;
	unsigned char *code = bytecode + (!binCodeSize ? 0 : -7 /* we must destroy the pop ebp; mov ebx, code; ret; sequence */);

	instAddress.resize(exCode.size() + 1); // Extra instruction for global return
	memset(instAddress.data + lastInstructionCount, 0, (exCode.size() - lastInstructionCount + 1) * sizeof(unsigned int));

	x86ClearLabels();
	x86ReserveLabels(GetLastALULabel());

	x86Instruction *curr = &instList[0];

	for(unsigned int i = 0, e = instList.size(); i != e; i++)
	{
		x86Instruction &cmd = *curr;	// Syntax sugar + too lazy to rewrite switch contents
		if(cmd.instID)
		{
			instAddress[cmd.instID - 1] = code;	// Save VM instruction address in x86 bytecode

			if(int(cmd.instID - 1) == (int)exLinker->offsetToGlobalCode)
				code += x86PUSH(code, rEBP);
		}
		switch(cmd.name)
		{
		case o_none:
			break;
		case o_mov:
			if(cmd.argA.type != x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argNumber)
					code += x86MOV(code, cmd.argA.reg, cmd.argB.num);
				else if(cmd.argB.type == x86Argument::argPtr)
					code += x86MOV(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				else
					code += x86MOV(code, cmd.argA.reg, cmd.argB.reg);
			}else{
				if(cmd.argB.type == x86Argument::argNumber)
					code += x86MOV(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else
					code += x86MOV(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
			}
			break;
		case o_movsx:
			code += x86MOVSX(code, cmd.argA.reg, cmd.argB.ptrSize, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			break;
		case o_push:
			if(cmd.argA.type == x86Argument::argNumber)
				code += x86PUSH(code, cmd.argA.num);
			else if(cmd.argA.type == x86Argument::argPtr)
				code += x86PUSH(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else
				code += x86PUSH(code, cmd.argA.reg);
			break;
		case o_pop:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86POP(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else
				code += x86POP(code, cmd.argA.reg);
			break;
		case o_lea:
			if(cmd.argB.type == x86Argument::argPtrLabel)
			{
				code += x86LEA(code, cmd.argA.reg, cmd.argB.labelID, (unsigned int)(intptr_t)bytecode);
			}else{
				code += x86LEA(code, cmd.argA.reg, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			}
			break;
		case o_cdq:
			code += x86CDQ(code);
			break;
		case o_rep_movsd:
			code += x86REP_MOVSD(code);
			break;

		case o_jmp:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86JMP(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else
				code += x86JMP(code, cmd.argA.labelID, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_ja:
			code += x86Jcc(code, cmd.argA.labelID, condA, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jae:
			code += x86Jcc(code, cmd.argA.labelID, condAE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jb:
			code += x86Jcc(code, cmd.argA.labelID, condB, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jbe:
			code += x86Jcc(code, cmd.argA.labelID, condBE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_je:
			code += x86Jcc(code, cmd.argA.labelID, condE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jg:
			code += x86Jcc(code, cmd.argA.labelID, condG, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jl:
			code += x86Jcc(code, cmd.argA.labelID, condL, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jne:
			code += x86Jcc(code, cmd.argA.labelID, condNE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jnp:
			code += x86Jcc(code, cmd.argA.labelID, condNP, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jp:
			code += x86Jcc(code, cmd.argA.labelID, condP, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jge:
			code += x86Jcc(code, cmd.argA.labelID, condGE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jle:
			code += x86Jcc(code, cmd.argA.labelID, condLE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_call:
			if(cmd.argA.type == x86Argument::argLabel)
				code += x86CALL(code, cmd.argA.labelID);
			else if(cmd.argA.type == x86Argument::argPtr)
				code += x86CALL(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else
				code += x86CALL(code, cmd.argA.reg);
			break;
		case o_ret:
			code += x86RET(code);
			break;

		case o_fld:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86FLD(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else
				code += x86FLD(code, (x87Reg)cmd.argA.fpArg);
			break;
		case o_fild:
			code += x86FILD(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_fistp:
			code += x86FISTP(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_fst:
			code += x86FST(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_fstp:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				code += x86FSTP(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			}else{
				code += x86FSTP(code, (x87Reg)cmd.argA.fpArg);
			}
			break;
		case o_fnstsw:
			code += x86FNSTSW(code);
			break;
		case o_fstcw:
			code += x86FSTCW(code);
			break;
		case o_fldcw:
			code += x86FLDCW(code, cmd.argA.ptrNum);
			break;

		case o_neg:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86NEG(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else
				code += x86NEG(code, cmd.argA.reg);
			break;
		case o_add:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADD(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86ADD(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}else{
				if(cmd.argB.type == x86Argument::argPtr)
					code += x86ADD(code, cmd.argA.reg, cmd.argB.ptrSize, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				else if(cmd.argB.type == x86Argument::argReg)
					code += x86ADD(code, cmd.argA.reg, cmd.argB.reg);
				else
					code += x86ADD(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_adc:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADC(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86ADC(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}else{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADC(code, cmd.argA.reg, cmd.argB.reg);
				else
					code += x86ADC(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_sub:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86SUB(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86SUB(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}else{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86SUB(code, cmd.argA.reg, cmd.argB.reg);
				else
					code += x86SUB(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_sbb:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86SBB(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86SBB(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}else{
				code += x86SBB(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_imul:
			if(cmd.argB.type == x86Argument::argNumber)
				code += x86IMUL(code, cmd.argA.reg, cmd.argB.num);
			else if(cmd.argB.type == x86Argument::argReg)
				code += x86IMUL(code, cmd.argA.reg, cmd.argB.reg);
			else if(cmd.argB.type == x86Argument::argPtr)
				code += x86IMUL(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			else
				code += x86IMUL(code, cmd.argA.reg);
			break;
		case o_idiv:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86IDIV(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else
				code += x86IDIV(code, cmd.argA.reg);
			break;
		case o_shl:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86SHL(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			else
				code += x86SHL(code, cmd.argA.reg, cmd.argB.num);
			break;
		case o_sal:
			code += x86SAL(code);
			break;
		case o_sar:
			code += x86SAR(code);
			break;
		case o_not:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86NOT(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else
				code += x86NOT(code, cmd.argA.reg);
			break;
		case o_and:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86AND(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else if(cmd.argB.type == x86Argument::argNumber)
					code += x86AND(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}else{
				code += x86AND(code, cmd.argA.reg, cmd.argB.reg);
			}
			break;
		case o_or:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86OR(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86OR(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}else if(cmd.argB.type == x86Argument::argPtr){
				code += x86OR(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			}else{
				code += x86OR(code, cmd.argA.reg, cmd.argB.reg);
			}
			break;
		case o_xor:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86XOR(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86XOR(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}else{
				code += x86XOR(code, cmd.argA.reg, cmd.argB.reg);
			}
			break;
		case o_cmp:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argNumber)
					code += x86CMP(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else
					code += x86CMP(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
			}else{
				if(cmd.argB.type == x86Argument::argPtr)
					code += x86CMP(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				else if(cmd.argB.type == x86Argument::argNumber)
					code += x86CMP(code, cmd.argA.reg, cmd.argB.num);
				else
					code += x86CMP(code, cmd.argA.reg, cmd.argB.reg);
			}
			break;
		case o_test:
			if(cmd.argB.type == x86Argument::argNumber)
				code += x86TESTah(code, (char)cmd.argB.num);
			else
				code += x86TEST(code, cmd.argA.reg, cmd.argB.reg);
			break;

		case o_setl:
			code += x86SETcc(code, condL, cmd.argA.reg);
			break;
		case o_setg:
			code += x86SETcc(code, condG, cmd.argA.reg);
			break;
		case o_setle:
			code += x86SETcc(code, condLE, cmd.argA.reg);
			break;
		case o_setge:
			code += x86SETcc(code, condGE, cmd.argA.reg);
			break;
		case o_sete:
			code += x86SETcc(code, condE, cmd.argA.reg);
			break;
		case o_setne:
			code += x86SETcc(code, condNE, cmd.argA.reg);
			break;
		case o_setz:
			code += x86SETcc(code, condZ, cmd.argA.reg);
			break;
		case o_setnz:
			code += x86SETcc(code, condNZ, cmd.argA.reg);
			break;

		case o_fadd:
			code += x86FADD(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_faddp:
			code += x86FADDP(code);
			break;
		case o_fmul:
			code += x86FMUL(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_fmulp:
			code += x86FMULP(code);
			break;
		case o_fsub:
			code += x86FSUB(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_fsubr:
			code += x86FSUBR(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_fsubp:
			code += x86FSUBP(code);
			break;
		case o_fsubrp:
			code += x86FSUBRP(code);
			break;
		case o_fdiv:
			code += x86FDIV(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_fdivr:
			code += x86FDIVR(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_fdivrp:
			code += x86FDIVRP(code);
			break;
		case o_fchs:
			code += x86FCHS(code);
			break;
		case o_fprem:
			code += x86FPREM(code);
			break;
		case o_fcomp:
			code += x86FCOMP(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			break;
		case o_fldz:
			code += x86FLDZ(code);
			break;
		case o_fld1:
			code += x86FLD1(code);
			break;
		case o_fsincos:
			code += x86FSINCOS(code);
			break;
		case o_fptan:
			code += x86FPTAN(code);
			break;
		case o_fsqrt:
			code += x86FSQRT(code);
			break;
		case o_frndint:
			code += x86FRNDINT(code);
			break;

		case o_int:
			code += x86INT(code, 3);
			break;
		case o_label:
			x86AddLabel(code, cmd.labelID);
			break;
		case o_use32:
			break;
		case o_other:
			break;
		}
		curr++;
	}
	assert(binCodeSize < binCodeReserved);
	binCodeSize = (unsigned int)(code - (binCode + 16));

	x86SatisfyJumps(instAddress);

	for(unsigned int i = (codeRelocated ? 0 : oldFunctionSize); i < exFunctions.size(); i++)
	{
		if(exFunctions[i].address != -1)
		{
			exFunctions[i].startInByteCode = (int)(instAddress[exFunctions[i].address] - (binCode + 16));
			functionAddress[i * 2 + 0] = (unsigned int)(uintptr_t)instAddress[exFunctions[i].address];
			functionAddress[i * 2 + 1] = 0;
		}else{
			exFunctions[i].startInByteCode = 0xffffffff;
			functionAddress[i * 2 + 0] = (unsigned int)(uintptr_t)exFunctions[i].funcPtr;
			functionAddress[i * 2 + 1] = 1;
		}
	}
	if(codeRelocated && oldFunctionLists.size())
	{
		for(unsigned i = 0; i < oldFunctionLists.size(); i++)
			memcpy(oldFunctionLists[i].list, functionAddress.data, oldFunctionLists[i].count * sizeof(unsigned));
	}
	globalStartInBytecode = (int)(instAddress[exLinker->offsetToGlobalCode] - (binCode + 16));

	lastInstructionCount = exCode.size();

	oldJumpTargetCount = exLinker->jumpTargets.size();
	oldFunctionSize = exFunctions.size();

	return true;
}

const char* ExecutorX86::GetResult()
{
	long long combined = (long long)(((unsigned long long)(unsigned)NULLC::runResult << 32ull) + (unsigned long long)(unsigned)NULLC::runResult2);
	
	switch(NULLC::runResultType)
	{
	case OTYPE_DOUBLE:
		SafeSprintf(execResult, 64, "%f", *(double*)(&combined));
		break;
	case OTYPE_LONG:
		SafeSprintf(execResult, 64, "%lldL", combined);
		break;
	case OTYPE_INT:
		SafeSprintf(execResult, 64, "%d", NULLC::runResult);
		break;
	default:
		SafeSprintf(execResult, 64, "no return value");
		break;
	}
	return execResult;
}
int ExecutorX86::GetResultInt()
{
	assert(NULLC::runResultType == OTYPE_INT);
	return NULLC::runResult;
}
double ExecutorX86::GetResultDouble()
{
	assert(NULLC::runResultType == OTYPE_DOUBLE);
	long long combined = (long long)(((unsigned long long)(unsigned)NULLC::runResult << 32ull) + (unsigned long long)(unsigned)NULLC::runResult2);
	return *(double*)(&combined);
}
long long ExecutorX86::GetResultLong()
{
	assert(NULLC::runResultType == OTYPE_LONG);
	long long combined = (long long)(((unsigned long long)(unsigned)NULLC::runResult << 32ull) + (unsigned long long)(unsigned)NULLC::runResult2);
	return combined;
}

const char*	ExecutorX86::GetExecError()
{
	return execError;
}

char* ExecutorX86::GetVariableData(unsigned int *count)
{
	if(count)
		*count = NULLC::dataHead->lastEDI;
	return paramBase;
}

void ExecutorX86::BeginCallStack()
{
	int count = 0;
	if(!NULLC::abnormalTermination)
	{
		if(NULLC::dataHead->instructionPtr)
		{
			genStackPtr = (void*)(intptr_t)NULLC::dataHead->instructionPtr;
			NULLC::dataHead->instructionPtr = ((int*)(intptr_t)NULLC::dataHead->instructionPtr)[-1];
			unsigned int *paramData = &NULLC::dataHead->nextElement;
			while((unsigned)count < (NULLC::STACK_TRACE_DEPTH - 1) && paramData)
			{
				NULLC::stackTrace[count++] = paramData[-1];
				paramData = (unsigned int*)(long long)(*paramData);
			}
		}
		NULLC::stackTrace[count] = 0;
		NULLC::dataHead->instructionPtr = (unsigned int)(intptr_t)genStackPtr;
	}else{
		while((unsigned)count < NULLC::STACK_TRACE_DEPTH && NULLC::stackTrace[count++]);
		count--;
	}

	callstackTop = NULLC::stackTrace + count - 1;
}

unsigned int ExecutorX86::GetNextAddress()
{
	if(int(callstackTop - NULLC::stackTrace) < 0)
		return 0;
	unsigned int address = 0;
	for(; address < instAddress.size(); address++)
	{
		if(*callstackTop < (unsigned int)(long long)instAddress[address])
			break;
	}
	callstackTop--;
	return address - 1;
}

void* ExecutorX86::GetStackStart()
{
	return genStackPtr;
}
void* ExecutorX86::GetStackEnd()
{
	return genStackTop;
}

void ExecutorX86::SetBreakFunction(unsigned (*callback)(unsigned int))
{
	breakFunction = callback;
}

void ExecutorX86::ClearBreakpoints()
{
	for(unsigned i = 0; i < breakInstructions.size(); i++)
	{
		if(*instAddress[breakInstructions[i].instIndex] == 0xcc)
			*instAddress[breakInstructions[i].instIndex] = breakInstructions[i].oldOpcode;
	}
	breakInstructions.clear();
}

bool ExecutorX86::AddBreakpoint(unsigned int instruction, bool oneHit)
{
	if(instruction > instAddress.size())
	{
		SafeSprintf(execError, 512, "ERROR: break position out of code range");
		return false;
	}
	while(instruction < instAddress.size() && !instAddress[instruction])
		instruction++;
	if(instruction >= instAddress.size())
	{
		SafeSprintf(execError, 512, "ERROR: break position out of code range");
		return false;
	}
	breakInstructions.push_back(Breakpoint(instruction, *instAddress[instruction], oneHit));
	*instAddress[instruction] = 0xcc;
	return true;
}

bool ExecutorX86::RemoveBreakpoint(unsigned int instruction)
{
	if(instruction > instAddress.size())
	{
		SafeSprintf(execError, 512, "ERROR: break position out of code range");
		return false;
	}
	unsigned index = ~0u;
	for(unsigned i = 0; i < breakInstructions.size() && index == ~0u; i++)
		if(breakInstructions[i].instIndex == instruction)
			index = i;
	if(index == ~0u || *instAddress[breakInstructions[index].instIndex] != 0xcc)
	{
		SafeSprintf(execError, 512, "ERROR: there is no breakpoint at instruction %d", instruction);
		return false;
	}
	*instAddress[breakInstructions[index].instIndex] = breakInstructions[index].oldOpcode;
	return true;
}

#endif
