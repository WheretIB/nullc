#include "stdafx.h"
#ifdef NULLC_BUILD_X86_JIT

#include "Executor_X86.h"
#include "CodeGen_X86.h"
#include "Translator_X86.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace NULLC
{
	// Parameter stack range
	void	*stackBaseAddress;
	void	*stackEndAddress;	// if NULL, range is not upper bound (until allocation fails)
	// Flag that shows that Executor manages memory allocation and deallocation
	bool	stackManaged;
	// If memory is not maneged, Executor will place a page guard at the end and restore old protection later
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
		//	Disabled check that exception happened in NULLC code (disabled for the reason that some NULLC external functions throw exception that weren't handled)
		//if(expInfo->ContextRecord->Eip < binCodeStart || expInfo->ContextRecord->Eip > binCodeEnd)
		//	return (DWORD)EXCEPTION_CONTINUE_SEARCH;

		// Save part of state for later use
		expEAXstate = expInfo->ContextRecord->Eax;
		expECXstate = expInfo->ContextRecord->Ecx;
		expESPstate = expInfo->ContextRecord->Esp;
		expCodePublic = expCode;

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

	typedef void (*codegenCallback)(VMCmd);
	codegenCallback cgFuncs[cmdEnumCount];

	typedef BOOL (WINAPI *PSTSG)(PULONG);
	PSTSG pSetThreadStackGuarantee = NULL;
}

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
}
ExecutorX86::~ExecutorX86()
{
	if(NULLC::stackManaged)
	{
		// If stack is managed by Executor, free memory
		VirtualFree(NULLC::stackBaseAddress, 0, MEM_RELEASE);
	}else{
		// Otherwise, remove page guard, restoring old protection value
		VirtualProtect((char*)NULLC::stackEndAddress - 8192, 4096, NULLC::stackProtect, &NULLC::stackProtect);
	}

	NULLC::dealloc(binCode);

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

	HMODULE hDLL = LoadLibrary("kernel32");
	pSetThreadStackGuarantee = (PSTSG)GetProcAddress(hDLL, "SetThreadStackGuarantee");

	codeRunning = false;

	// Default mode - stack is managed by Executor and starts from 0x20000000
	return SetStackPlacement((void*)0x20000000, NULL, false);
}

bool ExecutorX86::SetStackPlacement(void* start, void* end, unsigned int flagMemoryAllocated)
{
	// If old memory was allocated here using VirtualAlloc
	if(NULLC::stackManaged)
	{
		// If stack is managed by Executor, free memory
		VirtualFree(NULLC::stackBaseAddress, 0, MEM_RELEASE);
	}else{
		// Otherwise, remove page guard, restoring old protection value
		VirtualProtect((char*)NULLC::stackEndAddress - 8192, 4096, NULLC::stackProtect, &NULLC::stackProtect);
	}
	NULLC::stackBaseAddress = start;
	NULLC::stackEndAddress = end;
	NULLC::stackManaged = !flagMemoryAllocated;
	if(NULLC::stackManaged)
	{
		NULLC::stackGrowSize = 128 * 4096;
		NULLC::stackGrowCommit = 64 * 4096;

		char *paramData = NULL;
		// Request memory at address
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
		VirtualProtect((char*)NULLC::stackEndAddress - 8192, 4096, PAGE_NOACCESS, &NULLC::stackProtect);

		NULLC::parameterHead = paramBase = (char*)NULLC::stackBaseAddress + sizeof(NULLC::DataStackHeader);
		NULLC::paramDataBase = static_cast<unsigned int>(reinterpret_cast<long long>(NULLC::stackBaseAddress));
		NULLC::dataHead = (NULLC::DataStackHeader*)NULLC::stackBaseAddress;

		NULLC::commitedStack = (unsigned int)((char*)NULLC::stackEndAddress - (char*)NULLC::stackBaseAddress - 12 * 1024);
	}
	return true;
}

// Returns value as close as real ESP, at the program execution start
void* getESP(){ __asm{ lea eax, [ebp-32] } }

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
	NULLC::dataHead->nextElement = NULL;

	genStackPtr = genStackTop = getESP();

	if(NULLC::pSetThreadStackGuarantee)
	{
		unsigned long extraStack = 4096;
		NULLC::pSetThreadStackGuarantee(&extraStack);
	}
	memset(NULLC::stackBaseAddress, 0, sizeof(NULLC::DataStackHeader));
}

#pragma warning(disable: 4731)
void ExecutorX86::Run(unsigned int functionID, const char *arguments)
{
	int	callStackExtra[2];
	if(!codeRunning || functionID == ~0u)
	{
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
				__asm{ mov eax, dword ptr[stackStart] }
				__asm{ push dword ptr[eax] }
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
			__asm{ mov eax, dwordsToPop }
			__asm{ lea esp, [eax * 4 + esp] }
			return;
		}else{
			varSize += NULLC::dataHead->lastEDI;
			memcpy(paramBase + varSize, arguments, exFunctions[functionID].bytesToPop);
			binCodeStart += exFunctions[functionID].startInByteCode;
		}
	}else{
		binCodeStart += globalStartInBytecode;
		while(NULLC::commitedStack < exLinker->globalVarSize)
		{
			if(NULLC::ExtendMemory() != (DWORD)EXCEPTION_CONTINUE_EXECUTION)
			{
				strcpy(execError, "ERROR: allocated stack overflow");
				return;
			}
		}
		memset(NULLC::parameterHead, 0, exLinker->globalVarSize);
	}

	unsigned int res1 = 0;
	unsigned int res2 = 0;
	unsigned int resT = 0;

	NULLC::abnormalTermination = false;

	__try 
	{
		__asm
		{
			pusha ; // Save all registers
			mov eax, binCodeStart ;
			
			// Align stack to a 8-byte boundary
			lea ebx, [esp+8];
			and ebx, 0fh;
			mov ecx, 16;
			sub ecx, ebx;
			sub esp, ecx;

			push ecx; // Save alignment size
			push ebp; // Save stack base pointer (must be restored before popa)

			mov edi, varSize ;
			mov ebp, 0h ;

			call eax ; // Return type is placed in ebx

			pop ebp; // Restore stack base
			pop ecx;
			add esp, ecx;

			mov dword ptr [res1], eax;
			mov dword ptr [res2], edx;
			mov dword ptr [resT], ebx;

			popa ;
		}
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
			SafeSprintf(execError, 512, "ERROR: cannot convert from %s ref to %s ref", &exLinker->exSymbols[exLinker->exTypes[NULLC::expEAXstate].offsetToName], &exLinker->exSymbols[exLinker->exTypes[NULLC::expECXstate].offsetToName]);
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
				SafeSprintf(execError, 512, "ERROR: access violation at address 0x%d", NULLC::expECXstate);
		}
	}

	NULLC::runResult = res1;
	NULLC::runResult2 = res2;
	if(functionID == -1)
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
	}

	// Call stack management
	if(codeRunning && functionID != ~0u && exFunctions[functionID].startInByteCode != ~0u)
	{
		// Restore previous state
		NULLC::dataHead->nextElement = callStackExtra[1];
	}
}
#pragma warning(default: 4731)

void ExecutorX86::Stop(const char* error)
{
	callContinue = false;
	SafeSprintf(execError, 512, error);
}

bool ExecutorX86::TranslateToNative()
{
	execError[0] = 0;

	unsigned int pos = 0;

	globalStartInBytecode = 0xffffffff;
	for(unsigned int i = 0; i < exFunctions.size(); i++)
		exFunctions[i].startInByteCode = 0xffffffff;

	exLinker->functionAddress.resize(exFunctions.size() * 2);

	memset(&instList[0], 0, sizeof(x86Instruction) * instList.size());
	instList.clear();

	SetParamBase((unsigned int)(long long)paramBase);
	SetFunctionList(&exFunctions[0], &exLinker->functionAddress[0]);
	SetContinuePtr(&callContinue);
	SetLastInstruction(&instList[0], &instList[0]);
	SetClosureCreateFunc((void(*)())ClosureCreate);
	SetUpvaluesCloseFunc((void(*)())CloseUpvalues);

	CommonSetLinker(exLinker);

	EMIT_OP(o_use32);

	for(unsigned int i = 0, e = exLinker->jumpTargets.size(); i != e; i++)
		exCode[exLinker->jumpTargets[i]].cmd |= 0x80;
	OptimizationLookBehind(false);
	pos = 0;
	while(pos < exCode.size())
	{
		VMCmd &cmd = exCode[pos];

		unsigned int currSize = (int)(GetLastInstruction() - &instList[0]);
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
	GetLastInstruction()->instID = pos + 1;
	EMIT_OP_REG(o_pop, rEBP);
	EMIT_OP_REG_NUM(o_mov, rEBX, ~0u);
	EMIT_OP(o_ret);
	instList.resize((int)(GetLastInstruction() - &instList[0]));

#ifdef NULLC_OPTIMIZE_X86
	// Second optimization pass, just feed generated instructions again
	// But at first, mark invalidation instructions
	for(unsigned int i = 0, e = exLinker->jumpTargets.size(); i != e; i++)
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

	if(instList.size() * 4 > binCodeReserved)
	{
		NULLC::dealloc(binCode);
		binCodeReserved = (instList.size()) * 6 + 4096;	// Maximum instruction size is 6 bytes.
		binCode = (unsigned char*)NULLC::alloc(binCodeReserved);
		binCodeStart = (unsigned int)(intptr_t)(binCode + 16);
	}
	NULLC::binCodeStart = binCodeStart;
	NULLC::binCodeEnd = binCodeStart + binCodeReserved;

	// Translate to x86
	unsigned char *bytecode = binCode + 16;
	unsigned char *code = bytecode;

	instAddress.resize(exCode.size());
	memset(instAddress.data, 0, exCode.size() * sizeof(unsigned int));

	x86ClearLabels();
	x86ReserveLabels(GetLastALULabel());

	x86Instruction *curr = &instList[0];

	for(unsigned int i = 0, e = instList.size(); i != e; i++)
	{
		x86Instruction &cmd = *curr;	// Syntax sugar + too lazy to rewrite switch contents
		if(cmd.instID)
		{
			instAddress[cmd.instID - 1] = code;	// Save VM instruction address in x86 bytecode

			if(cmd.instID - 1 == (int)exLinker->offsetToGlobalCode)
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
	binCodeSize = (unsigned int)(code-bytecode);

	x86SatisfyJumps(instAddress);

	for(unsigned int i = 0; i < exFunctions.size(); i++)
	{
		if(exFunctions[i].address != -1)
		{
			exFunctions[i].startInByteCode = (int)(instAddress[exFunctions[i].address] - bytecode);
			exLinker->functionAddress[i * 2 + 0] = (unsigned int)(uintptr_t)instAddress[exFunctions[i].address];
			exLinker->functionAddress[i * 2 + 1] = 0;
		}else{
			exLinker->functionAddress[i * 2 + 0] = (unsigned int)(uintptr_t)exFunctions[i].funcPtr;
			exLinker->functionAddress[i * 2 + 1] = 1;
		}
	}
	globalStartInBytecode = (int)(instAddress[exLinker->offsetToGlobalCode] - bytecode);

	return true;
}

const char* ExecutorX86::GetResult()
{
	long long combined = 0;
	*((int*)(&combined)) = NULLC::runResult2;
	*((int*)(&combined)+1) = NULLC::runResult;

	switch(NULLC::runResultType)
	{
	case OTYPE_DOUBLE:
		SafeSprintf(execResult, 64, "%f", *(double*)(&combined));
		break;
	case OTYPE_LONG:
#ifdef _MSC_VER
		SafeSprintf(execResult, 64, "%I64dL", combined);
#else
		SafeSprintf(execResult, 64, "%lldL", combined);
#endif
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
	long long combined = 0;
	*((int*)(&combined)) = NULLC::runResult2;
	*((int*)(&combined)+1) = NULLC::runResult;
	return *(double*)(&combined);
}
long long ExecutorX86::GetResultLong()
{
	assert(NULLC::runResultType == OTYPE_LONG);
	long long combined = 0;
	*((int*)(&combined)) = NULLC::runResult2;
	*((int*)(&combined)+1) = NULLC::runResult;
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
		genStackPtr = (void*)(intptr_t)NULLC::dataHead->instructionPtr;
		NULLC::dataHead->instructionPtr = ((int*)(intptr_t)NULLC::dataHead->instructionPtr)[-1];
		unsigned int *paramData = &NULLC::dataHead->nextElement;
		while(count < (NULLC::STACK_TRACE_DEPTH - 1) && paramData)
		{
			NULLC::stackTrace[count++] = paramData[-1];
			paramData = (unsigned int*)(long long)(*paramData);
		}
		NULLC::stackTrace[count] = 0;
		NULLC::dataHead->nextElement = NULL;
	}else{
		while(count < NULLC::STACK_TRACE_DEPTH && NULLC::stackTrace[count++]);
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

void ExecutorX86::SetBreakFunction(void (*callback)(unsigned int))
{
	breakFunction = callback;
}

void ExecutorX86::ClearBreakpoints()
{
}

bool ExecutorX86::AddBreakpoint(unsigned int instruction)
{
	(void)instruction;
	return false;
}

#endif
