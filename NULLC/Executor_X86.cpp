#include "stdafx.h"

#ifdef NULLC_BUILD_X86_JIT

#include "Executor_X86.h"
#include "CodeGen_X86.h"
#include "CodeGenRegVm_X86.h"
#include "Translator_X86.h"
#include "Linker.h"
#include "Executor_Common.h"
#include "InstructionTreeRegVmLowerGraph.h"

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
#define dcAllocMem NULLC::alloc
#define dcFreeMem  NULLC::dealloc

#include "../external/dyncall/dyncall.h"
#endif

#ifndef __linux

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define UNW_FLAG_NHANDLER 0x0
#define UNW_FLAG_EHANDLER 0x1
#define UNW_FLAG_UHANDLER 0x2
#define UNW_FLAG_CHAININFO 0x4

#define UWOP_PUSH_NONVOL 0
#define UWOP_ALLOC_LARGE 1
#define UWOP_ALLOC_SMALL 2
#define UWOP_SET_FPREG 3
#define UWOP_SAVE_NONVOL 4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128 6
#define UWOP_SAVE_XMM128_FAR 7
#define UWOP_PUSH_MACHFRAME 8

#define UWOP_REGISTER_RAX 0
#define UWOP_REGISTER_RCX 1
#define UWOP_REGISTER_RDX 2
#define UWOP_REGISTER_RBX 3
#define UWOP_REGISTER_RSP 4
#define UWOP_REGISTER_RBP 5
#define UWOP_REGISTER_RSI 6
#define UWOP_REGISTER_RDI 7

struct UNWIND_CODE
{
	unsigned char offsetInPrologue;
	unsigned char operationCode : 4;
	unsigned char operationInfo : 4;
};

struct UNWIND_INFO_ENTRY
{
	unsigned char version : 3;
	unsigned char flags : 5;
	unsigned char sizeOfProlog;
	unsigned char countOfCodes;
	unsigned char frameRegister : 4;
	unsigned char frameOffset : 4;
	UNWIND_CODE unwindCode[8];
};

struct UNWIND_INFO_FUNCTION
{
	unsigned char version : 3;
	unsigned char flags : 5;
	unsigned char sizeOfProlog;
	unsigned char countOfCodes;
	unsigned char frameRegister : 4;
	unsigned char frameOffset : 4;
	UNWIND_CODE unwindCode[2];
};

#else

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
	//char	*stackBaseAddress;
	//char	*stackEndAddress;

	// Four global variables
	/*struct DataStackHeader
	{
		uintptr_t	unused1;
		uintptr_t	lastEDI;
		uintptr_t	instructionPtr;
		uintptr_t	nextElement;
	};*/

	//DataStackHeader	*dataHead;
	//char* parameterHead;

	// Hidden pointer to the beginning of NULLC parameter stack, skipping DataStackHeader
	uintptr_t paramDataBase;

	// Binary code range in hidden pointers
	uintptr_t binCodeStart;
	uintptr_t binCodeEnd;

	// Code run result - two DWORDs for parts of result and a type flag
	//int runResult = 0;
	//int runResult2 = 0;
	//RegVmReturnType runResultType = rvrVoid;

	// Call stack is made up by a linked list, starting from last frame, this array will hold call stack in correct order
	//const unsigned STACK_TRACE_DEPTH = 1024;
	//unsigned stackTrace[STACK_TRACE_DEPTH];

	// Signal that call stack contains stack of execution that ended in SEH handler with a fatal exception
	volatile bool abnormalTermination;

	// Part of state that SEH handler saves for future use
	unsigned int expCodePublic;
	unsigned int expAllocCode;
	uintptr_t expEAXstate;
	uintptr_t expECXstate;
	uintptr_t expESPstate;

	ExecutorX86	*currExecutor = NULL;

#ifndef __linux

#if defined(_M_X64)
#define RegisterIp Rip
#define RegisterAx Rax
#define RegisterCx Rcx
#define RegisterSp Rsp
#define RegisterDi Rdi
#else
#define RegisterIp Eip
#define RegisterAx Eax
#define RegisterCx Ecx
#define RegisterSp Esp
#define RegisterDi Edi
#endif

	DWORD CanWeHandleSEH(unsigned int expCode, _EXCEPTION_POINTERS* expInfo)
	{
		// Check that exception happened in NULLC code (division by zero and int overflow still catched)
		bool externalCode = expInfo->ContextRecord->RegisterIp < binCodeStart || expInfo->ContextRecord->RegisterIp > binCodeEnd;
		bool managedMemoryEnd = expInfo->ExceptionRecord->ExceptionInformation[1] > paramDataBase && expInfo->ExceptionRecord->ExceptionInformation[1] < expInfo->ContextRecord->RegisterDi + paramDataBase + 64 * 1024;

		if(externalCode && (expCode == EXCEPTION_BREAKPOINT || expCode == EXCEPTION_STACK_OVERFLOW || (expCode == EXCEPTION_ACCESS_VIOLATION && !managedMemoryEnd)))
			return (DWORD)EXCEPTION_CONTINUE_SEARCH;

		// Save part of state for later use
		expEAXstate = expInfo->ContextRecord->RegisterAx;
		expECXstate = expInfo->ContextRecord->RegisterCx;
		expESPstate = expInfo->ContextRecord->RegisterSp;
		expCodePublic = expCode;
		expAllocCode = ~0u;

		if(!externalCode && *(unsigned char*)(intptr_t)expInfo->ContextRecord->RegisterIp == 0xcc)
		{
			unsigned index = ~0u;
			for(unsigned i = 0; i < currExecutor->breakInstructions.size() && index == ~0u; i++)
			{
				if((uintptr_t)currExecutor->instAddress[currExecutor->breakInstructions[i].instIndex] == expInfo->ContextRecord->RegisterIp)
					index = i;
			}
			//printf("Found at index %d\n", index);
			if(index == ~0u)
				return EXCEPTION_CONTINUE_SEARCH;
			//printf("Returning execution (%d)\n", currExecutor->breakInstructions[index].instIndex);

			//??uintptr_t array[2] = { expInfo->ContextRecord->RegisterIp, 0 };
			//??NULLC::dataHead->instructionPtr = (uintptr_t)&array[1];

			/*unsigned command = */currExecutor->breakFunction(currExecutor->breakFunctionContext, currExecutor->breakInstructions[index].instIndex);
			//printf("Returned command %d\n", command);
			*currExecutor->instAddress[currExecutor->breakInstructions[index].instIndex] = currExecutor->breakInstructions[index].oldOpcode;
			return (DWORD)EXCEPTION_CONTINUE_EXECUTION;
		}

		if(expCode == EXCEPTION_INT_DIVIDE_BY_ZERO || expCode == EXCEPTION_BREAKPOINT || expCode == EXCEPTION_STACK_OVERFLOW || expCode == EXCEPTION_INT_OVERFLOW || (expCode == EXCEPTION_ACCESS_VIOLATION && expInfo->ExceptionRecord->ExceptionInformation[1] < 0x00010000))
		{
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
			if(managedMemoryEnd)
			{
				expAllocCode = 5;

				return EXCEPTION_EXECUTE_HANDLER;
			}
		}

		return (DWORD)EXCEPTION_CONTINUE_SEARCH;
	}

	//typedef BOOL (WINAPI *PSTSG)(PULONG);
	//PSTSG pSetThreadStackGuarantee = NULL;
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
				expCodePublic = EXCEPTION_ALLOCATED_STACK_OVERFLOW;

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

	int MemProtect(void *addr, unsigned size, int type)
	{
		char *alignedAddr = (char*)((intptr_t)((char*)addr + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
		char *alignedEnd = (char*)((intptr_t)((char*)addr + size + PAGESIZE - 1) & ~(PAGESIZE - 1));

		int result = mprotect(alignedAddr, alignedEnd - alignedAddr, type);

		return result;
	}
#endif

	typedef void (*codegenCallback)(CodeGenRegVmContext &ctx, RegVmCmd);
	codegenCallback cgFuncs[rviConvertPtr + 1];

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

ExecutorX86::ExecutorX86(Linker *linker): exLinker(linker), exFunctions(linker->exFunctions), exRegVmCode(linker->exRegVmCode), exRegVmConstants(linker->exRegVmConstants), exTypes(linker->exTypes)
{
	codeGenCtx = NULL;

	memset(execError, 0, REGVM_X86_ERROR_BUFFER_SIZE);
	memset(execResult, 0, 64);

	codeRunning = false;

	lastResultType = rvrError;

	minStackSize = 1 * 1024 * 1024;

	currentFrame = 0;

	lastFinalReturn = 0;

	callContinue = true;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	dcCallVM = NULL;
#endif

	breakFunctionContext = NULL;
	breakFunction = NULL;

	memset(codeLaunchHeader, 0, codeLaunchHeaderSize);
	oldCodeLaunchHeaderProtect = 0;

	codeLaunchHeaderLength = 0;
	codeLaunchUnwindOffset = 0;
	codeLaunchDataLength = 0;
	codeLaunchWin64UnwindTable = NULL;

	binCode = NULL;
	binCodeStart = 0;
	binCodeSize = 0;
	binCodeReserved = 0;

	lastInstructionCount = 0;

	//callstackTop = NULL;

	oldJumpTargetCount = 0;
	oldFunctionSize = 0;
	oldCodeBodyProtect = 0;

	// Parameter stack must be aligned
	//assert(sizeof(NULLC::DataStackHeader) % 16 == 0);

	//NULLC::stackBaseAddress = NULL;
	//NULLC::stackEndAddress = NULL;

	NULLC::currExecutor = this;

	linker->SetFunctionPointerUpdater(NULLC::UpdateFunctionPointer);

#ifdef __linux
	SetLongJmpTarget(NULLC::errorHandler);
#endif
}

ExecutorX86::~ExecutorX86()
{
	NULLC::dealloc(vmState.dataStackBase);

	NULLC::dealloc(vmState.callStackBase);

	NULLC::dealloc(vmState.tempStackArrayBase);

	NULLC::dealloc(vmState.regFileArrayBase);

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	if(dcCallVM)
		dcFree(dcCallVM);
#endif

	/*if(NULLC::stackBaseAddress)
	{
#ifndef __linux
		// Remove page guard, restoring old protection value
		DWORD unusedProtect;
		VirtualProtect((char*)NULLC::stackEndAddress - 8192, 4096, PAGE_READWRITE, &unusedProtect);
#else
		// Remove page guard, restoring old protection value
		NULLC::MemProtect((char*)NULLC::stackEndAddress - 8192, PAGESIZE, PROT_READ | PROT_WRITE);
#endif

		NULLC::alignedDealloc(NULLC::stackBaseAddress);
	}*/

	// Disable execution of code head and code body
#ifndef __linux
	DWORD unusedProtect;
	VirtualProtect((void*)codeLaunchHeader, codeLaunchHeaderSize, oldCodeLaunchHeaderProtect, &unusedProtect);
	if(binCode)
		VirtualProtect((void*)binCode, binCodeSize, oldCodeBodyProtect, &unusedProtect);
#else
	NULLC::MemProtect((void*)codeLaunchHeader, codeLaunchHeaderSize, PROT_READ | PROT_WRITE);
	if(binCode)
		NULLC::MemProtect((void*)binCode, binCodeSize, PROT_READ | PROT_WRITE);
#endif

	NULLC::dealloc(binCode);
	binCode = NULL;

	NULLC::currExecutor = NULL;

	for(unsigned i = 0; i < oldFunctionLists.size(); i++)
		NULLC::dealloc(oldFunctionLists[i].list);
	oldFunctionLists.clear();

	if(codeGenCtx)
		NULLC::destruct(codeGenCtx);
	codeGenCtx = NULL;

#ifndef __linux

#if defined(_M_X64)
	if(codeLaunchWin64UnwindTable)
	{
		RtlDeleteFunctionTable(codeLaunchWin64UnwindTable);
		NULLC::dealloc(codeLaunchWin64UnwindTable);
	}
#endif

#endif

	x86ResetLabels();
}

bool ExecutorX86::Initialize()
{
	using namespace NULLC;

	cgFuncs[rviNop] = GenCodeCmdNop;
	cgFuncs[rviLoadByte] = GenCodeCmdLoadByte;
	cgFuncs[rviLoadWord] = GenCodeCmdLoadWord;
	cgFuncs[rviLoadDword] = GenCodeCmdLoadDword;
	cgFuncs[rviLoadLong] = GenCodeCmdLoadLong;
	cgFuncs[rviLoadFloat] = GenCodeCmdLoadFloat;
	cgFuncs[rviLoadDouble] = GenCodeCmdLoadDouble;
	cgFuncs[rviLoadImm] = GenCodeCmdLoadImm;
	cgFuncs[rviLoadImmLong] = GenCodeCmdLoadImmLong;
	cgFuncs[rviLoadImmDouble] = GenCodeCmdLoadImmDouble;
	cgFuncs[rviStoreByte] = GenCodeCmdStoreByte;
	cgFuncs[rviStoreWord] = GenCodeCmdStoreWord;
	cgFuncs[rviStoreDword] = GenCodeCmdStoreDword;
	cgFuncs[rviStoreLong] = GenCodeCmdStoreLong;
	cgFuncs[rviStoreFloat] = GenCodeCmdStoreFloat;
	cgFuncs[rviStoreDouble] = GenCodeCmdStoreDouble;
	cgFuncs[rviCombinedd] = GenCodeCmdCombinedd;
	cgFuncs[rviBreakupdd] = GenCodeCmdBreakupdd;
	cgFuncs[rviMov] = GenCodeCmdMov;
	cgFuncs[rviMovMult] = GenCodeCmdMovMult;
	cgFuncs[rviDtoi] = GenCodeCmdDtoi;
	cgFuncs[rviDtol] = GenCodeCmdDtol;
	cgFuncs[rviDtof] = GenCodeCmdDtof;
	cgFuncs[rviItod] = GenCodeCmdItod;
	cgFuncs[rviLtod] = GenCodeCmdLtod;
	cgFuncs[rviItol] = GenCodeCmdItol;
	cgFuncs[rviLtoi] = GenCodeCmdLtoi;
	cgFuncs[rviIndex] = GenCodeCmdIndex;
	cgFuncs[rviGetAddr] = GenCodeCmdGetAddr;
	cgFuncs[rviSetRange] = GenCodeCmdSetRange;
	cgFuncs[rviMemCopy] = GenCodeCmdMemCopy;
	cgFuncs[rviJmp] = GenCodeCmdJmp;
	cgFuncs[rviJmpz] = GenCodeCmdJmpz;
	cgFuncs[rviJmpnz] = GenCodeCmdJmpnz;
	cgFuncs[rviCall] = GenCodeCmdCall;
	cgFuncs[rviCallPtr] = GenCodeCmdCallPtr;
	cgFuncs[rviReturn] = GenCodeCmdReturn;
	cgFuncs[rviAddImm] = GenCodeCmdAddImm;
	cgFuncs[rviAdd] = GenCodeCmdAdd;
	cgFuncs[rviSub] = GenCodeCmdSub;
	cgFuncs[rviMul] = GenCodeCmdMul;
	cgFuncs[rviDiv] = GenCodeCmdDiv;
	cgFuncs[rviPow] = GenCodeCmdPow;
	cgFuncs[rviMod] = GenCodeCmdMod;
	cgFuncs[rviLess] = GenCodeCmdLess;
	cgFuncs[rviGreater] = GenCodeCmdGreater;
	cgFuncs[rviLequal] = GenCodeCmdLequal;
	cgFuncs[rviGequal] = GenCodeCmdGequal;
	cgFuncs[rviEqual] = GenCodeCmdEqual;
	cgFuncs[rviNequal] = GenCodeCmdNequal;
	cgFuncs[rviShl] = GenCodeCmdShl;
	cgFuncs[rviShr] = GenCodeCmdShr;
	cgFuncs[rviBitAnd] = GenCodeCmdBitAnd;
	cgFuncs[rviBitOr] = GenCodeCmdBitOr;
	cgFuncs[rviBitXor] = GenCodeCmdBitXor;
	cgFuncs[rviAddImml] = GenCodeCmdAddImml;
	cgFuncs[rviAddl] = GenCodeCmdAddl;
	cgFuncs[rviSubl] = GenCodeCmdSubl;
	cgFuncs[rviMull] = GenCodeCmdMull;
	cgFuncs[rviDivl] = GenCodeCmdDivl;
	cgFuncs[rviPowl] = GenCodeCmdPowl;
	cgFuncs[rviModl] = GenCodeCmdModl;
	cgFuncs[rviLessl] = GenCodeCmdLessl;
	cgFuncs[rviGreaterl] = GenCodeCmdGreaterl;
	cgFuncs[rviLequall] = GenCodeCmdLequall;
	cgFuncs[rviGequall] = GenCodeCmdGequall;
	cgFuncs[rviEquall] = GenCodeCmdEquall;
	cgFuncs[rviNequall] = GenCodeCmdNequall;
	cgFuncs[rviShll] = GenCodeCmdShll;
	cgFuncs[rviShrl] = GenCodeCmdShrl;
	cgFuncs[rviBitAndl] = GenCodeCmdBitAndl;
	cgFuncs[rviBitOrl] = GenCodeCmdBitOrl;
	cgFuncs[rviBitXorl] = GenCodeCmdBitXorl;
	cgFuncs[rviAddd] = GenCodeCmdAddd;
	cgFuncs[rviSubd] = GenCodeCmdSubd;
	cgFuncs[rviMuld] = GenCodeCmdMuld;
	cgFuncs[rviDivd] = GenCodeCmdDivd;
	cgFuncs[rviAddf] = GenCodeCmdAddf;
	cgFuncs[rviSubf] = GenCodeCmdSubf;
	cgFuncs[rviMulf] = GenCodeCmdMulf;
	cgFuncs[rviDivf] = GenCodeCmdDivf;
	cgFuncs[rviPowd] = GenCodeCmdPowd;
	cgFuncs[rviModd] = GenCodeCmdModd;
	cgFuncs[rviLessd] = GenCodeCmdLessd;
	cgFuncs[rviGreaterd] = GenCodeCmdGreaterd;
	cgFuncs[rviLequald] = GenCodeCmdLequald;
	cgFuncs[rviGequald] = GenCodeCmdGequald;
	cgFuncs[rviEquald] = GenCodeCmdEquald;
	cgFuncs[rviNequald] = GenCodeCmdNequald;
	cgFuncs[rviNeg] = GenCodeCmdNeg;
	cgFuncs[rviNegl] = GenCodeCmdNegl;
	cgFuncs[rviNegd] = GenCodeCmdNegd;
	cgFuncs[rviBitNot] = GenCodeCmdBitNot;
	cgFuncs[rviBitNotl] = GenCodeCmdBitNotl;
	cgFuncs[rviLogNot] = GenCodeCmdLogNot;
	cgFuncs[rviLogNotl] = GenCodeCmdLogNotl;
	cgFuncs[rviConvertPtr] = GenCodeCmdConvertPtr;

	/*
#ifndef __linux
	if(HMODULE hDLL = LoadLibrary("kernel32"))
		pSetThreadStackGuarantee = (PSTSG)GetProcAddress(hDLL, "SetThreadStackGuarantee");
#endif*/

	// Create code launch header
	unsigned char *pos = codeLaunchHeader;

#if defined(_M_X64)
	// Save non-volatile registers
	pos += x86PUSH(pos, rRBP);
	pos += x86PUSH(pos, rRBX);
	pos += x86PUSH(pos, rRDI);
	pos += x86PUSH(pos, rRSI);
	pos += x86PUSH(pos, rR12);
	pos += x86PUSH(pos, rR13);
	pos += x86PUSH(pos, rR14);
	pos += x86PUSH(pos, rR15);

	pos += x64MOV(pos, rRBX, rRDX);
	pos += x86MOV(pos, rR15, sQWORD, rNONE, 1, rRBX, rvrrFrame * 8);
	pos += x86MOV(pos, rR14, sQWORD, rNONE, 1, rRBX, rvrrConstants * 8);
	pos += x86CALL(pos, rECX);

	// Restore registers
	pos += x86POP(pos, rR15);
	pos += x86POP(pos, rR14);
	pos += x86POP(pos, rR13);
	pos += x86POP(pos, rR12);
	pos += x86POP(pos, rRSI);
	pos += x86POP(pos, rRDI);
	pos += x86POP(pos, rRBX);
	pos += x86POP(pos, rRBP);

	pos += x86RET(pos);
#else
	// Setup stack frame
	pos += x86PUSH(pos, rEBP);
	pos += x86MOV(pos, rEBP, rESP);

	// Save registers
	pos += x86PUSH(pos, rEBX);
	pos += x86PUSH(pos, rECX);
	pos += x86PUSH(pos, rESI);
	pos += x86PUSH(pos, rEDI);

	pos += x86MOV(pos, rEAX, sDWORD, rNONE, 0, rEBP, 8); // Get nullc code address
	pos += x86MOV(pos, rEBX, sDWORD, rNONE, 0, rEBP, 12); // Get register file

	pos += x86MOV(pos, rESI, sDWORD, rNONE, 0, rEBX, rvrrFrame * 8); // Get frame pointer

	// Go into nullc code
	pos += x86CALL(pos, rEAX);

	// Restore registers
	pos += x86POP(pos, rEDI);
	pos += x86POP(pos, rESI);
	pos += x86POP(pos, rECX);
	pos += x86POP(pos, rEBX);

	// Destroy stack frame
	pos += x86MOV(pos, rESP, rEBP);
	pos += x86POP(pos, rEBP);
	pos += x86RET(pos);
#endif

	assert(pos <= codeLaunchHeader + codeLaunchHeaderSize);
	codeLaunchHeaderLength = unsigned(pos - codeLaunchHeader);

	// Enable execution of code head
#ifndef __linux
	VirtualProtect((void*)codeLaunchHeader, codeLaunchHeaderSize, PAGE_EXECUTE_READWRITE, (DWORD*)&oldCodeLaunchHeaderProtect);

#if defined(_M_X64)
	pos += 16 - unsigned(uintptr_t(pos) % 16);

	codeLaunchUnwindOffset = unsigned(pos - codeLaunchHeader);

	assert(sizeof(UNWIND_CODE) == 2);
	assert(sizeof(UNWIND_INFO_ENTRY) == 4 + 8 * 2);

	UNWIND_INFO_ENTRY unwindInfo = { 0 };

	unwindInfo.version = 1;
	unwindInfo.flags = 0; // No EH
	unwindInfo.sizeOfProlog = 12;
	unwindInfo.countOfCodes = 8;
	unwindInfo.frameRegister = 0;
	unwindInfo.frameOffset = 0;

	unwindInfo.unwindCode[0].offsetInPrologue = 12;
	unwindInfo.unwindCode[0].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[0].operationInfo = 15; // r15

	unwindInfo.unwindCode[1].offsetInPrologue = 10;
	unwindInfo.unwindCode[1].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[1].operationInfo = 14; // r14

	unwindInfo.unwindCode[2].offsetInPrologue = 8;
	unwindInfo.unwindCode[2].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[2].operationInfo = 13; // r13

	unwindInfo.unwindCode[3].offsetInPrologue = 6;
	unwindInfo.unwindCode[3].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[3].operationInfo = 12; // r12

	unwindInfo.unwindCode[4].offsetInPrologue = 4;
	unwindInfo.unwindCode[4].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[4].operationInfo = UWOP_REGISTER_RSI;

	unwindInfo.unwindCode[5].offsetInPrologue = 3;
	unwindInfo.unwindCode[5].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[5].operationInfo = UWOP_REGISTER_RDI;

	unwindInfo.unwindCode[6].offsetInPrologue = 2;
	unwindInfo.unwindCode[6].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[6].operationInfo = UWOP_REGISTER_RBX;

	unwindInfo.unwindCode[7].offsetInPrologue = 1;
	unwindInfo.unwindCode[7].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[7].operationInfo = UWOP_REGISTER_RBP;

	memcpy(pos, &unwindInfo, sizeof(unwindInfo));
	pos += sizeof(unwindInfo);

	assert(pos <= codeLaunchHeader + codeLaunchHeaderSize);

	uintptr_t baseAddress = (uintptr_t)codeLaunchHeader;

	codeLaunchWin64UnwindTable = (RUNTIME_FUNCTION*)NULLC::alloc(sizeof(RUNTIME_FUNCTION) * 1);

	codeLaunchWin64UnwindTable[0].BeginAddress = 0;
	codeLaunchWin64UnwindTable[0].EndAddress = codeLaunchHeaderLength;
	codeLaunchWin64UnwindTable[0].UnwindData = codeLaunchUnwindOffset;

	// Can't get RtlInstallFunctionTableCallback to work (it's not getting called)

	if(!RtlAddFunctionTable(codeLaunchWin64UnwindTable, 1, baseAddress))
		printf("Failed to install function table");
#endif

	codeLaunchDataLength = unsigned(pos - codeLaunchHeader);

#else
	NULLC::MemProtect((void*)codeLaunchHeader, codeLaunchHeaderSize, PROT_READ | PROT_EXEC);
#endif

	if(!vmState.callStackBase)
	{
		vmState.callStackBase = (CodeGenRegVmCallStackEntry*)NULLC::alloc(sizeof(CodeGenRegVmCallStackEntry) * 1024 * 8);
		memset(vmState.callStackBase, 0, sizeof(CodeGenRegVmCallStackEntry) * 1024 * 8);
		vmState.callStackEnd = vmState.callStackBase + 1024 * 8;
	}

	if(!vmState.tempStackArrayBase)
	{
		vmState.tempStackArrayBase = (unsigned*)NULLC::alloc(sizeof(unsigned) * 1024 * 32);
		memset(vmState.tempStackArrayBase, 0, sizeof(unsigned) * 1024 * 32);
		vmState.tempStackArrayEnd = vmState.tempStackArrayBase + 1024 * 32;
	}

	if(!vmState.dataStackBase)
	{
		vmState.dataStackBase = (char*)NULLC::alloc(sizeof(char) * minStackSize);
		memset(vmState.dataStackBase, 0, sizeof(char) * minStackSize);
		vmState.dataStackEnd = vmState.dataStackBase + minStackSize;
	}

	if(!vmState.regFileArrayBase)
	{
		vmState.regFileArrayBase = (RegVmRegister*)NULLC::alloc(sizeof(RegVmRegister) * 1024 * 32);
		memset(vmState.regFileArrayBase, 0, sizeof(RegVmRegister) * 1024 * 32);
		vmState.regFileArrayEnd = vmState.regFileArrayBase + 1024 * 32;
	}

	//x86TestEncoding(codeLaunchHeader);

	return true;
}

bool ExecutorX86::InitExecution()
{
	if(!exRegVmCode.size())
	{
		strcpy(execError, "ERROR: no code to run");
		return false;
	}

	vmState.callStackTop = vmState.callStackBase;

	lastFinalReturn = 0;

	CommonSetLinker(exLinker);

	vmState.dataStackTop = vmState.dataStackBase + ((exLinker->globalVarSize + 0xf) & ~0xf);

	SetUnmanagableRange(vmState.dataStackBase, unsigned(vmState.dataStackEnd - vmState.dataStackBase));

	execError[0] = 0;

	callContinue = true;

	vmState.regFileLastPtr = vmState.regFileArrayBase;
	vmState.regFileLastTop = vmState.regFileArrayBase;

	vmState.instAddress = instAddress.data;
	vmState.codeLaunchHeader = codeLaunchHeader;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	if(!dcCallVM)
	{
		dcCallVM = dcNewCallVM(4096);
		dcMode(dcCallVM, DC_CALL_C_DEFAULT);
	}
#endif

	/*NULLC::dataHead->lastEDI = 0;
	NULLC::dataHead->instructionPtr = 0;
	NULLC::dataHead->nextElement = 0;*/

	/*
#ifndef __linux
	if(NULLC::pSetThreadStackGuarantee)
	{
		unsigned long extraStack = 4096;
		NULLC::pSetThreadStackGuarantee(&extraStack);
	}
#endif*/

	//memset(NULLC::stackBaseAddress, 0, sizeof(NULLC::DataStackHeader));

	return true;
}

void ExecutorX86::Run(unsigned int functionID, const char *arguments)
{
	bool firstRun = !codeRunning || functionID == ~0u;

	if(firstRun)
	{
		if(!InitExecution())
			return;
	}

	codeRunning = true;

	RegVmReturnType retType = rvrVoid;

	unsigned instructionPos = 0;

	bool errorState = false;

	// We will know that return is global if call stack size is equal to current
	unsigned prevLastFinalReturn = lastFinalReturn;
	lastFinalReturn = unsigned(vmState.callStackTop - vmState.callStackBase);

	unsigned prevDataSize = unsigned(vmState.dataStackTop - vmState.dataStackBase);

	assert(prevDataSize % 16 == 0);

	RegVmRegister *regFilePtr = vmState.regFileLastTop;
	RegVmRegister *regFileTop = vmState.regFileLastTop + 256;

	unsigned *tempStackPtr = vmState.tempStackArrayBase;

	if(functionID != ~0u)
	{
		ExternFuncInfo &target = exFunctions[functionID];

		unsigned funcPos = ~0u;
		funcPos = target.regVmAddress;

		if(target.retType == ExternFuncInfo::RETURN_VOID)
			retType = rvrVoid;
		else if(target.retType == ExternFuncInfo::RETURN_INT)
			retType = rvrInt;
		else if(target.retType == ExternFuncInfo::RETURN_DOUBLE)
			retType = rvrDouble;
		else if(target.retType == ExternFuncInfo::RETURN_LONG)
			retType = rvrLong;

		if(funcPos == ~0u)
		{
			// Can't return complex types here
			if(target.retType == ExternFuncInfo::RETURN_UNKNOWN)
			{
				strcpy(execError, "ERROR: can't call external function with complex return type");
				return;
			}

			// Copy all arguments
			memcpy(tempStackPtr, arguments, target.bytesToPop);

			// Call function
			if(target.funcPtrWrap)
			{
				target.funcPtrWrap(target.funcPtrWrapTarget, (char*)tempStackPtr, (char*)tempStackPtr);

				if(!callContinue)
					errorState = true;
			}
			else
			{
#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
				RunRawExternalFunction(dcCallVM, exFunctions[functionID], exLinker->exLocals.data, exTypes.data, tempStackPtr);

				if(!callContinue)
					errorState = true;
#else
				Stop("ERROR: external raw function calls are disabled");

				errorState = true;
#endif
			}

			// This will disable NULLC code execution while leaving error check and result retrieval
			instructionPos = ~0u;
		}
		else
		{
			instructionPos = funcPos;

			unsigned argumentsSize = target.bytesToPop;

			if(unsigned(vmState.dataStackTop - vmState.dataStackBase) + argumentsSize >= unsigned(vmState.dataStackEnd - vmState.dataStackBase))
			{
				CodeGenRegVmCallStackEntry *entry = vmState.callStackTop;

				entry->instruction = instructionPos + 1;

				vmState.callStackTop++;

				instructionPos = ~0u;
				strcpy(execError, "ERROR: stack overflow");
				retType = rvrError;
			}
			else
			{
				// Copy arguments to new stack frame
				memcpy(vmState.dataStackTop, arguments, argumentsSize);

				unsigned stackSize = (target.stackSize + 0xf) & ~0xf;

				regFilePtr = vmState.regFileLastTop;
				regFileTop = vmState.regFileLastTop + target.regVmRegisters;

				if(unsigned(vmState.dataStackTop - vmState.dataStackBase) + stackSize >= unsigned(vmState.dataStackEnd - vmState.dataStackBase))
				{
					CodeGenRegVmCallStackEntry *entry = vmState.callStackTop;

					entry->instruction = instructionPos + 1;

					vmState.callStackTop++;

					instructionPos = ~0u;
					strcpy(execError, "ERROR: stack overflow");
					retType = rvrError;
				}
				else
				{
					vmState.dataStackTop += stackSize;

					assert(argumentsSize <= stackSize);

					if(stackSize - argumentsSize)
						memset(vmState.dataStackBase + prevDataSize + argumentsSize, 0, stackSize - argumentsSize);

					regFilePtr[rvrrGlobals].ptrValue = uintptr_t(vmState.dataStackBase);
					regFilePtr[rvrrFrame].ptrValue = uintptr_t(vmState.dataStackBase + prevDataSize);
					regFilePtr[rvrrConstants].ptrValue = uintptr_t(exLinker->exRegVmConstants.data);
					regFilePtr[rvrrRegisters].ptrValue = uintptr_t(regFilePtr);
				}

				memset(regFilePtr + rvrrCount, 0, (regFileTop - regFilePtr - rvrrCount) * sizeof(regFilePtr[0]));
			}
		}
	}
	else
	{
		// If global code is executed, reset all global variables
		assert(unsigned(vmState.dataStackTop - vmState.dataStackBase) >= exLinker->globalVarSize);
		memset(vmState.dataStackBase, 0, exLinker->globalVarSize);

		regFilePtr[rvrrGlobals].ptrValue = uintptr_t(vmState.dataStackBase);
		regFilePtr[rvrrFrame].ptrValue = uintptr_t(vmState.dataStackBase);
		regFilePtr[rvrrConstants].ptrValue = uintptr_t(exLinker->exRegVmConstants.data);
		regFilePtr[rvrrRegisters].ptrValue = uintptr_t(regFilePtr);

		memset(regFilePtr + rvrrCount, 0, (regFileTop - regFilePtr - rvrrCount) * sizeof(regFilePtr[0]));
	}

	RegVmRegister *prevRegFilePtr = vmState.regFileLastPtr;
	RegVmRegister *prevRegFileTop = vmState.regFileLastTop;

	vmState.regFileLastPtr = regFilePtr;
	vmState.regFileLastTop = regFileTop;

	RegVmReturnType resultType = retType;

	if(instructionPos != ~0u)
	{
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
			nullcFunc gate = (nullcFunc)(uintptr_t)codeLaunchHeader;
			int returnStruct[3] = { 1, 2, 3 };
			gate(varSize, returnStruct, funcBinCodeStart, firstRun ? &genStackTop : &dummy);
			res1 = returnStruct[0];
			res2 = returnStruct[1];
			resT = returnStruct[2];
			NULLC::dataHead->lastEDI = savedSize;
		}
		else
		{
			if(errorCode == EXCEPTION_INT_DIVIDE_BY_ZERO)
				strcpy(execError, "ERROR: integer division by zero");
			else if(errorCode == EXCEPTION_FUNCTION_NO_RETURN)
				strcpy(execError, "ERROR: function didn't return a value");
			else if(errorCode == EXCEPTION_ARRAY_OUT_OF_BOUNDS)
				strcpy(execError, "ERROR: array index out of bounds");
			else if(errorCode == EXCEPTION_INVALID_FUNCTION)
				strcpy(execError, "ERROR: invalid function pointer");
			else if((errorCode & 0xff) == EXCEPTION_CONVERSION_ERROR)
				NULLC::SafeSprintf(execError, 512, "ERROR: cannot convert from %s ref to %s ref",
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
					NULLC::stackTrace[count++] = unsigned(paramData[-1]);
					paramData = (unsigned int*)(long long)(*paramData);
				}
				NULLC::stackTrace[count] = 0;
				NULLC::dataHead->nextElement = 0;
			}
			NULLC::dataHead->instructionPtr = 0;
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
			unsigned char *codeStart = instAddress[instructionPos];

			typedef	uintptr_t (*nullcFunc)(unsigned char *codeStart, RegVmRegister *regFilePtr);
			nullcFunc gate = (nullcFunc)(uintptr_t)codeLaunchHeader;
			resultType = (RegVmReturnType)gate(codeStart, regFilePtr);
		}
		__except(NULLC::CanWeHandleSEH(GetExceptionCode(), GetExceptionInformation()))
		{
			if(NULLC::expCodePublic == EXCEPTION_INT_DIVIDE_BY_ZERO)
			{
				strcpy(execError, "ERROR: integer division by zero");
			}
			else if(NULLC::expCodePublic == EXCEPTION_INT_OVERFLOW)
			{
				strcpy(execError, "ERROR: integer overflow");
			}
			else if(NULLC::expCodePublic == EXCEPTION_BREAKPOINT && NULLC::expECXstate == 0)
			{
				strcpy(execError, "ERROR: array index out of bounds");
			}
			else if(NULLC::expCodePublic == EXCEPTION_BREAKPOINT && NULLC::expECXstate == 0xFFFFFFFF)
			{
				strcpy(execError, "ERROR: function didn't return a value");
			}
			else if(NULLC::expCodePublic == EXCEPTION_BREAKPOINT && NULLC::expECXstate == 0xDEADBEEF)
			{
				strcpy(execError, "ERROR: invalid function pointer");
			}
			else if(NULLC::expCodePublic == EXCEPTION_BREAKPOINT && NULLC::expECXstate != NULLC::expESPstate)
			{
				NULLC::SafeSprintf(execError, 512, "ERROR: cannot convert from %s ref to %s ref",
					NULLC::expEAXstate >= exLinker->exTypes.size() ? "%unknown%" : &exLinker->exSymbols[exLinker->exTypes[unsigned(NULLC::expEAXstate)].offsetToName],
					NULLC::expECXstate >= exLinker->exTypes.size() ? "%unknown%" : &exLinker->exSymbols[exLinker->exTypes[unsigned(NULLC::expECXstate)].offsetToName]);
			}
			else if(NULLC::expCodePublic == EXCEPTION_STACK_OVERFLOW)
			{
#ifndef __DMC__
				// Restore stack guard
				_resetstkoflw();
#endif

				strcpy(execError, "ERROR: stack overflow");
			}
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
	}

	vmState.regFileLastPtr = prevRegFilePtr;
	vmState.regFileLastTop = prevRegFileTop;

	vmState.dataStackTop = vmState.dataStackBase + prevDataSize;

	if(resultType == rvrError)
	{
		errorState = true;
	}
	else
	{
		if(retType == rvrVoid)
			retType = resultType;
		else
			assert(retType == resultType && "expected different result");
	}

	// If there was an execution error
	if(errorState)
	{
		// Print call stack on error, when we get to the first function
		if(lastFinalReturn == 0)
		{
			char *currPos = execError + strlen(execError);
			currPos += NULLC::SafeSprintf(currPos, REGVM_X86_ERROR_BUFFER_SIZE - int(currPos - execError), "\r\nCall stack:\r\n");

			BeginCallStack();
			while(unsigned address = GetNextAddress())
				currPos += PrintStackFrame(address, currPos, REGVM_X86_ERROR_BUFFER_SIZE - int(currPos - execError), false);
		}

		lastFinalReturn = prevLastFinalReturn;

		// Ascertain that execution stops when there is a chain of nullcRunFunction
		callContinue = false;
		codeRunning = false;

		return;
	}

	if(lastFinalReturn == 0)
		codeRunning = false;

	lastFinalReturn = prevLastFinalReturn;

	lastResultType = retType;

	switch(lastResultType)
	{
	case rvrInt:
		lastResult.intValue = tempStackPtr[0];
		break;
	case rvrDouble:
		memcpy(&lastResult.doubleValue, tempStackPtr, sizeof(double));
		break;
	case rvrLong:
		memcpy(&lastResult.longValue, tempStackPtr, sizeof(long long));
		break;
	default:
		break;
	}
}

void ExecutorX86::Stop(const char* error)
{
	codeRunning = false;

	callContinue = false;
	NULLC::SafeSprintf(execError, REGVM_X86_ERROR_BUFFER_SIZE, "%s", error);
}

bool ExecutorX86::SetStackSize(unsigned bytes)
{
	if(codeRunning || !instList.empty())
		return false;

	minStackSize = bytes;

	NULLC::dealloc(vmState.dataStackBase);

	vmState.dataStackBase = (char*)NULLC::alloc(sizeof(char) * minStackSize);
	memset(vmState.dataStackBase, 0, sizeof(char) * minStackSize);
	vmState.dataStackEnd = vmState.dataStackBase + minStackSize;

	return true;
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

	globalCodeRanges.clear();

#ifndef __linux

#if defined(_M_X64)
	// Create function table for unwind information
	if(!functionWin64UnwindTable.empty())
		RtlDeleteFunctionTable(functionWin64UnwindTable.data);

	functionWin64UnwindTable.clear();
#endif

#endif

	oldJumpTargetCount = 0;
	oldFunctionSize = 0;

	// Create new code generation context
	if(codeGenCtx)
		NULLC::destruct(codeGenCtx);
	codeGenCtx = NULL;

	codeRunning = false;
}

bool ExecutorX86::TranslateToNative(bool enableLogFiles, OutputContext &output)
{
	//globalStartInBytecode = 0xffffffff;

	if(functionAddress.max <= exFunctions.size() * 2)
	{
		unsigned *newStorage = (unsigned*)NULLC::alloc(exFunctions.size() * 3 * sizeof(unsigned));
		if(functionAddress.count != 0)
			oldFunctionLists.push_back(FunctionListInfo(functionAddress.data, functionAddress.count));
		memcpy(newStorage, functionAddress.data, functionAddress.count * sizeof(unsigned));
		functionAddress.data = newStorage;
		functionAddress.count = exFunctions.size() * 2;
		functionAddress.max = exFunctions.size() * 3;
	}
	else
	{
		functionAddress.resize(exFunctions.size() * 2);
	}

	memset(instList.data, 0, sizeof(x86Instruction) * instList.size());
	instList.clear();
	instList.reserve(64);

	if(!codeGenCtx)
		codeGenCtx = NULLC::construct<CodeGenRegVmContext>();

	codeGenCtx->x86rvm = this;

	codeGenCtx->exFunctions = exFunctions.data;
	codeGenCtx->exTypes = exTypes.data;
	codeGenCtx->exLocals = exLinker->exLocals.data;
	codeGenCtx->exRegVmConstants = exRegVmConstants.data;
	codeGenCtx->exSymbols = exLinker->exSymbols.data;

	codeGenCtx->vmState = &vmState;

	vmState.ctx = codeGenCtx;

	//SetParamBase((unsigned int)(long long)paramBase);
	//SetFunctionList(exFunctions.data, functionAddress.data);
	//SetContinuePtr(&callContinue);

	codeGenCtx->ctx.SetLastInstruction(instList.data, instList.data);

	CommonSetLinker(exLinker);

	EMIT_OP(codeGenCtx->ctx, o_use32);

	codeJumpTargets.resize(exRegVmCode.size());
	if(codeJumpTargets.size())
		memset(&codeJumpTargets[lastInstructionCount], 0, codeJumpTargets.size() - lastInstructionCount);

	// Mirror extra global return so that jump to global return can be marked (rviNop, because we will have some custom code)
	codeJumpTargets.push_back(0);
	for(unsigned i = oldJumpTargetCount, e = exLinker->regVmJumpTargets.size(); i != e; i++)
		codeJumpTargets[exLinker->regVmJumpTargets[i]] = 1;

	// Mark function locations
	for(unsigned i = 0, e = exLinker->exFunctions.size(); i != e; i++)
	{
		unsigned address = exLinker->exFunctions[i].regVmAddress;

		if(address != ~0u)
			codeJumpTargets[address] |= 2;
	}

	SetOptimizationLookBehind(codeGenCtx->ctx, false);

	unsigned activeGlobalCodeStart = 0;

	unsigned int pos = lastInstructionCount;
	while(pos < exRegVmCode.size())
	{
		RegVmCmd &cmd = exRegVmCode[pos];

		unsigned int currSize = (int)(codeGenCtx->ctx.GetLastInstruction() - instList.data);
		instList.count = currSize;
		if(currSize + 64 >= instList.max)
			instList.grow(currSize + 64);

		codeGenCtx->ctx.SetLastInstruction(instList.data + currSize, instList.data);

		codeGenCtx->ctx.GetLastInstruction()->instID = pos + 1;

		if(codeJumpTargets[pos])
			SetOptimizationLookBehind(codeGenCtx->ctx, false);

		codeGenCtx->currInstructionPos = pos;

		// Frame setup
		if((codeJumpTargets[pos] & 6) != 0)
		{
			if(codeJumpTargets[pos] & 4)
				activeGlobalCodeStart = pos;

#if defined(_M_X64)
			EMIT_OP_REG_NUM(codeGenCtx->ctx, o_sub64, rRSP, 32);
#endif
		}

		if(cmd.code == rviJmp && cmd.rA)
		{
			codeJumpTargets[cmd.argument] |= 4;

			if(activeGlobalCodeStart != 0)
				globalCodeRanges.push_back(pos);

			globalCodeRanges.push_back(cmd.argument);

#if defined(_M_X64)
			if(pos)
				EMIT_OP_REG_NUM(codeGenCtx->ctx, o_add64, rRSP, 32);
#endif
		}

		pos++;

		NULLC::cgFuncs[cmd.code](*codeGenCtx, cmd);

		SetOptimizationLookBehind(codeGenCtx->ctx, true);
	}

	if(activeGlobalCodeStart != 0)
		globalCodeRanges.push_back(pos);

	// Add extra global return if there is none
	codeGenCtx->ctx.GetLastInstruction()->instID = pos + 1;

	if((codeJumpTargets[exRegVmCode.size()] & 6) != 0)
	{
#if defined(_M_X64)
		EMIT_OP_REG_NUM(codeGenCtx->ctx, o_sub64, rRSP, 32);
#endif
	}

	EMIT_OP_REG_REG(codeGenCtx->ctx, o_xor, rEAX, rEAX);

#if defined(_M_X64)
	EMIT_OP_REG_NUM(codeGenCtx->ctx, o_add64, rRSP, 32);
#endif

	EMIT_OP(codeGenCtx->ctx, o_ret);

	// Remove rviNop, because we don't want to generate code for it
	codeJumpTargets.pop_back();

	instList.resize((int)(codeGenCtx->ctx.GetLastInstruction() - &instList[0]));

	// Once again, mirror extra global return so that jump to global return can be marked (cmdNop, because we will have some custom code)
	codeJumpTargets.push_back(false);

	if(enableLogFiles)
	{
		assert(!output.stream);
		output.stream = output.openStream("asmX86.txt");

		if(output.stream)
		{
			SaveListing(output);

			output.closeStream(output.stream);
			output.stream = NULL;
		}
	}

#if defined(NULLC_OPTIMIZE_X86) && 0
	// Second optimization pass, just feed generated instructions again

	// Set iterator at beginning
	codeGenCtx->ctx.SetLastInstruction(instList.data, instList.data);
	SetOptimizationLookBehind(codeGenCtx->ctx, false);
	// Now regenerate instructions
	for(unsigned int i = 0; i < instList.size(); i++)
	{
		// Skip trash
		if(instList[i].name == o_none)
		{
			if(instList[i].instID && codeJumpTargets[instList[i].instID - 1])
			{
				codeGenCtx->ctx.GetLastInstruction()->instID = instList[i].instID;
				EMIT_OP(codeGenCtx->ctx, o_none);
				SetOptimizationLookBehind(codeGenCtx->ctx, false);
			}
			continue;
		}
		// If invalidation flag is set
		if(instList[i].instID && codeJumpTargets[instList[i].instID - 1])
			SetOptimizationLookBehind(codeGenCtx->ctx, false);
		codeGenCtx->ctx.GetLastInstruction()->instID = instList[i].instID;

		x86Instruction &inst = instList[i];
		if(inst.name == o_label)
		{
			EMIT_LABEL(codeGenCtx->ctx, inst.labelID, inst.argA.num);
			SetOptimizationLookBehind(codeGenCtx->ctx, true);
			continue;
		}

		if(inst.name == o_call)
		{
			EMIT_REG_READ(codeGenCtx->ctx, rECX);
			EMIT_REG_READ(codeGenCtx->ctx, rEDX);
			EMIT_REG_READ(codeGenCtx->ctx, rR8);
		}

		switch(inst.argA.type)
		{
		case x86Argument::argNone:
			EMIT_OP(codeGenCtx->ctx, inst.name);
			break;
		case x86Argument::argNumber:
			EMIT_OP_NUM(codeGenCtx->ctx, inst.name, inst.argA.num);
			break;
		case x86Argument::argLabel:
			EMIT_OP_LABEL(codeGenCtx->ctx, inst.name, inst.argA.labelID, inst.argB.num, inst.argB.ptrNum);
			break;
		case x86Argument::argReg:
			switch(inst.argB.type)
			{
			case x86Argument::argNone:
				EMIT_OP_REG(codeGenCtx->ctx, inst.name, inst.argA.reg);
				break;
			case x86Argument::argNumber:
				EMIT_OP_REG_NUM(codeGenCtx->ctx, inst.name, inst.argA.reg, inst.argB.num);
				break;
			case x86Argument::argReg:
				EMIT_OP_REG_REG(codeGenCtx->ctx, inst.name, inst.argA.reg, inst.argB.reg);
				break;
			case x86Argument::argPtr:
				EMIT_OP_REG_RPTR(codeGenCtx->ctx, inst.name, inst.argA.reg, inst.argB.ptrSize, inst.argB.ptrIndex, inst.argB.ptrMult, inst.argB.ptrBase, inst.argB.ptrNum);
				break;
			case x86Argument::argImm64:
				EMIT_OP_REG_NUM64(codeGenCtx->ctx, inst.name, inst.argA.reg, inst.argB.imm64Arg);
				break;
			case x86Argument::argXmmReg:
				EMIT_OP_REG_REG(codeGenCtx->ctx, inst.name, inst.argA.reg, inst.argB.xmmArg);
				break;
			default:
				assert(!"unknown type");
				break;
			}
			break;
		case x86Argument::argPtr:
			switch(inst.argB.type)
			{
			case x86Argument::argNone:
				EMIT_OP_RPTR(codeGenCtx->ctx, inst.name, inst.argA.ptrSize, inst.argA.ptrIndex, inst.argA.ptrMult, inst.argA.ptrBase, inst.argA.ptrNum);
				break;
			case x86Argument::argNumber:
				EMIT_OP_RPTR_NUM(codeGenCtx->ctx, inst.name, inst.argA.ptrSize, inst.argA.ptrIndex, inst.argA.ptrMult, inst.argA.ptrBase, inst.argA.ptrNum, inst.argB.num);
				break;
			case x86Argument::argReg:
				EMIT_OP_RPTR_REG(codeGenCtx->ctx, inst.name, inst.argA.ptrSize, inst.argA.ptrIndex, inst.argA.ptrMult, inst.argA.ptrBase, inst.argA.ptrNum, inst.argB.reg);
				break;
			case x86Argument::argXmmReg:
				EMIT_OP_RPTR_REG(codeGenCtx->ctx, inst.name, inst.argA.ptrSize, inst.argA.ptrIndex, inst.argA.ptrMult, inst.argA.ptrBase, inst.argA.ptrNum, inst.argB.xmmArg);
				break;
			default:
				assert(!"unknown type");
				break;
			}
			break;
		case x86Argument::argXmmReg:
			switch(inst.argB.type)
			{
			case x86Argument::argXmmReg:
				EMIT_OP_REG_REG(codeGenCtx->ctx, inst.name, inst.argA.xmmArg, inst.argB.xmmArg);
				break;
			case x86Argument::argPtr:
				EMIT_OP_REG_RPTR(codeGenCtx->ctx, inst.name, inst.argA.xmmArg, inst.argB.ptrSize, inst.argB.ptrIndex, inst.argB.ptrMult, inst.argB.ptrBase, inst.argB.ptrNum);
				break;
			default:
				assert(!"unknown type");
				break;
			}
			break;
		default:
			assert(!"unknown type");
			break;
		}

		SetOptimizationLookBehind(codeGenCtx->ctx, true);
	}
	unsigned int currSize = (int)(codeGenCtx->ctx.GetLastInstruction() - &instList[0]);
	for(unsigned int i = currSize; i < instList.size(); i++)
	{
		instList[i].name = o_other;
		instList[i].instID = 0;
	}
#endif

	if(enableLogFiles)
	{
		assert(!output.stream);
		output.stream = output.openStream("asmX86_opt.txt");

		if(output.stream)
		{
			SaveListing(output);

			output.closeStream(output.stream);
			output.stream = NULL;
		}
	}

	codeJumpTargets.pop_back();

	bool codeRelocated = false;

	if((binCodeSize + instList.size() * 8) > binCodeReserved)
	{
		unsigned int oldBinCodeReserved = binCodeReserved;
		binCodeReserved = binCodeSize + (instList.size()) * 8 + 4096;	// Average instruction size is 8 bytes.
		unsigned char *binCodeNew = (unsigned char*)NULLC::alloc(binCodeReserved);

		// Disable execution of old code body and enable execution of new code body
#ifndef __linux
		DWORD unusedProtect;
		if(binCode)
			VirtualProtect((void*)binCode, oldBinCodeReserved, oldCodeBodyProtect, (DWORD*)&unusedProtect);
		VirtualProtect((void*)binCodeNew, binCodeReserved, PAGE_EXECUTE_READWRITE, (DWORD*)&oldCodeBodyProtect);
#else
		if(binCode)
			NULLC::MemProtect((void*)binCode, oldBinCodeReserved, PROT_READ | PROT_WRITE);
		NULLC::MemProtect((void*)binCodeNew, binCodeReserved, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

		if(binCodeSize)
			memcpy(binCodeNew + 16, binCode + 16, binCodeSize);
		NULLC::dealloc(binCode);
		// If code is currently running, fix call stack (return addresses)
		if(codeRunning)
		{
			codeRelocated = true;

			// This must be an external function call
			/*assert(NULLC::dataHead->instructionPtr);
			
			uintptr_t *retvalpos = (uintptr_t*)NULLC::dataHead->instructionPtr - 1;
			if(*retvalpos >= NULLC::binCodeStart && *retvalpos <= NULLC::binCodeEnd)
				*retvalpos = (*retvalpos - NULLC::binCodeStart) + (uintptr_t)(binCodeNew + 16);

			uintptr_t *paramData = &NULLC::dataHead->nextElement;
			while(paramData)
			{
				uintptr_t *retvalpos = paramData - 1;
				if(*retvalpos >= NULLC::binCodeStart && *retvalpos <= NULLC::binCodeEnd)
					*retvalpos = (*retvalpos - NULLC::binCodeStart) + (uintptr_t)(binCodeNew + 16);
				paramData = (uintptr_t*)(*paramData);
			}*/
		}
		for(unsigned i = 0; i < instAddress.size(); i++)
			instAddress[i] = (instAddress[i] - NULLC::binCodeStart) + (uintptr_t)(binCodeNew + 16);
		binCode = binCodeNew;
		binCodeStart = (uintptr_t)(binCode + 16);
	}

	//SetBinaryCodeBase(binCode);

	NULLC::binCodeStart = binCodeStart;
	NULLC::binCodeEnd = binCodeStart + binCodeReserved;

	// Translate to x86
	unsigned char *bytecode = binCode + 16 + binCodeSize;

#if defined(_M_X64)
	unsigned char *code = bytecode +(!binCodeSize ? 0 : -7 /* we must destroy the xor eax, eax; add rsp, 32; ret; sequence */);
#else
	unsigned char *code = bytecode +(!binCodeSize ? 0 : -4 /* we must destroy the xor eax, eax; ret; sequence */);
#endif

	instAddress.resize(exRegVmCode.size() + 1); // Extra instruction for global return
	memset(instAddress.data + lastInstructionCount, 0, (exRegVmCode.size() - lastInstructionCount + 1) * sizeof(instAddress[0]));

	x86ClearLabels();
	x86ReserveLabels(codeGenCtx->labelCount);

	code = x86TranslateInstructionList(code, binCode + binCodeReserved, instList.data, instList.size(), instAddress.data);

	assert(binCodeSize < binCodeReserved);

#ifndef __linux

#if defined(_M_X64)
	// Create function table for unwind information
	if(!functionWin64UnwindTable.empty())
		RtlDeleteFunctionTable(functionWin64UnwindTable.data);

	functionWin64UnwindTable.clear();

	// Align data block
	code += 16 - unsigned(uintptr_t(code) % 16);

	// Write the unwind data
	assert(sizeof(UNWIND_CODE) == 2);
	assert(sizeof(UNWIND_INFO_FUNCTION) == 4 + 2 * 2);

	UNWIND_INFO_FUNCTION unwindInfo = { 0 };

	unwindInfo.version = 1;
	unwindInfo.flags = 0; // No EH
	unwindInfo.sizeOfProlog = 4;
	unwindInfo.countOfCodes = 1;
	unwindInfo.frameRegister = 0;
	unwindInfo.frameOffset = 0;

	unwindInfo.unwindCode[0].offsetInPrologue = 4;
	unwindInfo.unwindCode[0].operationCode = UWOP_ALLOC_SMALL;
	unwindInfo.unwindCode[0].operationInfo = (32 - 8) / 8;

	unsigned char *unwindPos = code;

	memcpy(code, &unwindInfo, sizeof(unwindInfo));
	code += sizeof(unwindInfo);

	assert(code < binCode + binCodeReserved);

	for(unsigned i = 0, e = exLinker->exFunctions.size(); i != e; i++)
	{
		ExternFuncInfo &funcInfo = exLinker->exFunctions[i];

		if(funcInfo.regVmAddress != ~0u)
		{
			unsigned char *codeStart = instAddress[funcInfo.regVmAddress];
			unsigned char *codeEnd = instAddress[funcInfo.regVmAddress + funcInfo.regVmCodeSize];

			// Store function info
			RUNTIME_FUNCTION rtFunc;

			rtFunc.BeginAddress = unsigned(codeStart - binCode);
			rtFunc.EndAddress = unsigned(codeEnd - binCode);
			rtFunc.UnwindData = unsigned(unwindPos - binCode);

			functionWin64UnwindTable.push_back(rtFunc);
		}
	}

	for(unsigned i = 0, e = globalCodeRanges.size(); i != e; i += 2)
	{
		unsigned char *codeStart = instAddress[globalCodeRanges[i]];
		unsigned char *codeEnd = instAddress[globalCodeRanges[i + 1]] + 4; // add 'sub rsp, 32'

		// Store function info
		RUNTIME_FUNCTION rtFunc;

		rtFunc.BeginAddress = unsigned(codeStart - binCode);
		rtFunc.EndAddress = unsigned(codeEnd - binCode);
		rtFunc.UnwindData = unsigned(unwindPos - binCode);

		functionWin64UnwindTable.push_back(rtFunc);
	}

	if(!RtlAddFunctionTable(functionWin64UnwindTable.data, functionWin64UnwindTable.size(), uintptr_t(binCode)))
		assert(!"failed to install function table");
#endif

#endif

	binCodeSize = (unsigned int)(code - (binCode + 16));

	x86SatisfyJumps(instAddress);

	for(unsigned int i = (codeRelocated ? 0 : oldFunctionSize); i < exFunctions.size(); i++)
	{
		if(exFunctions[i].regVmAddress != -1)
		{
			exFunctions[i].startInByteCode = (int)(instAddress[exFunctions[i].regVmAddress] - (binCode + 16));

			functionAddress[i * 2 + 0] = (unsigned int)(uintptr_t)instAddress[exFunctions[i].regVmAddress];
			functionAddress[i * 2 + 1] = 0;
		}
		else if(exFunctions[i].funcPtrWrap)
		{
			exFunctions[i].startInByteCode = 0xffffffff;

			assert((uintptr_t)exFunctions[i].funcPtrWrapTarget > 1);

			functionAddress[i * 2 + 0] = (unsigned int)(uintptr_t)exFunctions[i].funcPtrWrap;
			functionAddress[i * 2 + 1] = (unsigned int)(uintptr_t)exFunctions[i].funcPtrWrapTarget;
		}
		else
		{
			exFunctions[i].startInByteCode = 0xffffffff;

			functionAddress[i * 2 + 0] = (unsigned int)(uintptr_t)exFunctions[i].funcPtrRaw;
			functionAddress[i * 2 + 1] = 1;
		}
	}
	if(codeRelocated && oldFunctionLists.size())
	{
		for(unsigned i = 0; i < oldFunctionLists.size(); i++)
			memcpy(oldFunctionLists[i].list, functionAddress.data, oldFunctionLists[i].count * sizeof(unsigned));
	}
	//globalStartInBytecode = (int)(instAddress[exLinker->regVmOffsetToGlobalCode] - (binCode + 16));

	lastInstructionCount = exRegVmCode.size();

	oldJumpTargetCount = exLinker->regVmJumpTargets.size();
	oldFunctionSize = exFunctions.size();

	return true;
}

void ExecutorX86::SaveListing(OutputContext &output)
{
	char instBuf[128];

	for(unsigned i = 0; i < instList.size(); i++)
	{
		if(instList[i].instID && codeJumpTargets[instList[i].instID - 1])
		{
			output.Print("; ------------------- Invalidation ----------------\n");
			output.Printf("0x%x: ; %4d\n", 0xc0000000 | (instList[i].instID - 1), instList[i].instID - 1);
		}

		if(instList[i].instID && instList[i].instID - 1 < exRegVmCode.size())
		{
			RegVmCmd &cmd = exRegVmCode[instList[i].instID - 1];

			output.Printf("; %4d: ", instList[i].instID - 1);

			PrintInstruction(output, (char*)exRegVmConstants.data, RegVmInstructionCode(cmd.code), cmd.rA, cmd.rB, cmd.rC, cmd.argument, NULL);

			output.Print('\n');
		}

		if(instList[i].name == o_other)
			continue;

		instList[i].Decode(vmState, instBuf);

		output.Print(instBuf);
		output.Print('\n');
	}

	output.Flush();
}

const char* ExecutorX86::GetResult()
{
	switch(lastResultType)
	{
	case rvrDouble:
		NULLC::SafeSprintf(execResult, 64, "%f", lastResult.doubleValue);
		break;
	case rvrLong:
		NULLC::SafeSprintf(execResult, 64, "%lldL", (long long)lastResult.longValue);
		break;
	case rvrInt:
		NULLC::SafeSprintf(execResult, 64, "%d", lastResult.intValue);
		break;
	case rvrVoid:
		NULLC::SafeSprintf(execResult, 64, "no return value");
		break;
	case rvrStruct:
		NULLC::SafeSprintf(execResult, 64, "complex return value");
		break;
	default:
		break;
	}

	return execResult;
}

int ExecutorX86::GetResultInt()
{
	assert(lastResultType == rvrInt);

	return lastResult.intValue;
}

double ExecutorX86::GetResultDouble()
{
	assert(lastResultType == rvrDouble);

	return lastResult.doubleValue;
}

long long ExecutorX86::GetResultLong()
{
	assert(lastResultType == rvrLong);

	return lastResult.longValue;
}

const char*	ExecutorX86::GetExecError()
{
	return execError;
}

char* ExecutorX86::GetVariableData(unsigned int *count)
{
	if(count)
		*count = unsigned(vmState.dataStackTop - vmState.dataStackBase);

	return vmState.dataStackBase;
}

void ExecutorX86::BeginCallStack()
{
	currentFrame = 0;
}

unsigned int ExecutorX86::GetNextAddress()
{
	return currentFrame == unsigned(vmState.callStackTop - vmState.callStackBase) ? 0 : vmState.callStackBase[currentFrame++].instruction;
}

void* ExecutorX86::GetStackStart()
{
	return vmState.regFileArrayBase;
}

void* ExecutorX86::GetStackEnd()
{
	return vmState.regFileLastTop;
}

void ExecutorX86::SetBreakFunction(void *context, unsigned (*callback)(void*, unsigned))
{
	breakFunctionContext = context;
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
		NULLC::SafeSprintf(execError, 512, "ERROR: break position out of code range");
		return false;
	}

	while(instruction < instAddress.size() && !instAddress[instruction])
		instruction++;

	if(instruction >= instAddress.size())
	{
		NULLC::SafeSprintf(execError, 512, "ERROR: break position out of code range");
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
		NULLC::SafeSprintf(execError, 512, "ERROR: break position out of code range");
		return false;
	}

	unsigned index = ~0u;
	for(unsigned i = 0; i < breakInstructions.size() && index == ~0u; i++)
	{
		if(breakInstructions[i].instIndex == instruction)
			index = i;
	}

	if(index == ~0u || *instAddress[breakInstructions[index].instIndex] != 0xcc)
	{
		NULLC::SafeSprintf(execError, 512, "ERROR: there is no breakpoint at instruction %d", instruction);
		return false;
	}

	*instAddress[breakInstructions[index].instIndex] = breakInstructions[index].oldOpcode;
	return true;
}

#endif
