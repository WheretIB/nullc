#include "stdafx.h"

#ifdef NULLC_BUILD_X86_JIT

#include "Executor_Common.h"

#include "nullc.h"
#include "nullc_debug.h"
#include "Executor_X86.h"
#include "CodeGen_X86.h"
#include "CodeGenRegVm_X86.h"
#include "Translator_X86.h"
#include "Linker.h"
#include "StdLib.h"
#include "InstructionTreeRegVmLowerGraph.h"
#include "Trace.h"
#include "StrAlgo.h"

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
#define UWOP_SAVE_XMM128 8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME 10

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
	UNWIND_CODE unwindCode[10];
};

struct UNWIND_INFO_FUNCTION
{
	unsigned char version : 3;
	unsigned char flags : 5;
	unsigned char sizeOfProlog;
	unsigned char countOfCodes;
	unsigned char frameRegister : 4;
	unsigned char frameOffset : 4;
	UNWIND_CODE unwindCode[4];
};

#else

#include <sys/mman.h>
#ifndef PAGESIZE
	// $ sysconf()
	#define PAGESIZE 4096
#endif
#include <signal.h>

#if _MSC_VER <= 1600
typedef struct _RUNTIME_FUNCTION
{
} RUNTIME_FUNCTION;
#else
typedef struct _IMAGE_RUNTIME_FUNCTION_ENTRY
{
} RUNTIME_FUNCTION;
#endif


#endif

extern "C"
{
	NULLC_DEBUG_EXPORT uintptr_t nullcModuleStartAddress = 0;
	NULLC_DEBUG_EXPORT uintptr_t nullcModuleEndAddress = 0;

	NULLC_DEBUG_EXPORT uintptr_t nullcJitContextMainDataAddress = 0;
}

namespace NULLC
{
	ExecutorX86	*currExecutor = NULL;

	unsigned GetInstructionFromAddress(uintptr_t address)
	{
		unsigned lowerBound = 0;
		unsigned upperBound = currExecutor->instAddress.size() - 1;
		unsigned index = 0;

		while(lowerBound <= upperBound)
		{
			index = (lowerBound + upperBound) >> 1;

			if(address < uintptr_t(currExecutor->instAddress.data[index]))
				upperBound = index - 1;
			else if(address > uintptr_t(currExecutor->instAddress.data[index]))
				lowerBound = index + 1;
			else
				break;
		}

		if(index != 0 && address < uintptr_t(currExecutor->instAddress.data[index]))
			index--;

		return index;
	}

	void AddCrashStackFrame(unsigned instruction)
	{
		if(instruction != ~0u)
		{
			currExecutor->vmState.callStackTop->instruction = instruction;
			currExecutor->vmState.callStackTop++;
		}
	}

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
		if(expCode == EXCEPTION_ACCESS_VIOLATION && expInfo->ExceptionRecord->ExceptionInformation[1] >= uintptr_t(currExecutor->vmState.callStackEnd) && expInfo->ExceptionRecord->ExceptionInformation[1] <= uintptr_t(currExecutor->vmState.callStackEnd) + 8192)
		{
			currExecutor->Stop("ERROR: call stack overflow");

			return (DWORD)EXCEPTION_EXECUTE_HANDLER;
		}

		if(expCode == EXCEPTION_ACCESS_VIOLATION && expInfo->ExceptionRecord->ExceptionInformation[1] >= uintptr_t(currExecutor->vmState.dataStackEnd) && expInfo->ExceptionRecord->ExceptionInformation[1] <= uintptr_t(currExecutor->vmState.dataStackEnd) + 8192)
		{
			currExecutor->Stop("ERROR: stack overflow");

			return (DWORD)EXCEPTION_EXECUTE_HANDLER;
		}

		if(expCode == EXCEPTION_ACCESS_VIOLATION && expInfo->ExceptionRecord->ExceptionInformation[1] >= uintptr_t(currExecutor->vmState.regFileArrayEnd) && expInfo->ExceptionRecord->ExceptionInformation[1] <= uintptr_t(currExecutor->vmState.regFileArrayEnd) + 8192)
		{
			currExecutor->Stop("ERROR: register overflow");

			return (DWORD)EXCEPTION_EXECUTE_HANDLER;
		}

		uintptr_t address = uintptr_t(expInfo->ContextRecord->RegisterIp);

		// Check that exception happened in NULLC code
		bool isInternal = address >= uintptr_t(currExecutor->binCode) && address <= uintptr_t(currExecutor->binCode + currExecutor->binCodeSize);

		unsigned internalInstruction = isInternal ? GetInstructionFromAddress(address) : ~0u;

		for(unsigned i = 0; i < currExecutor->expiredCodeBlocks.size(); i++)
		{
			if(address >= uintptr_t(currExecutor->expiredCodeBlocks[i].code) && address <= uintptr_t(currExecutor->expiredCodeBlocks[i].code + currExecutor->expiredCodeBlocks[i].codeSize))
			{
				isInternal = true;
				break;
			}
		}

		if(currExecutor->vmState.jitCodeActive)
			isInternal = true;

		if(!isInternal)
			return (DWORD)EXCEPTION_CONTINUE_SEARCH;

		if(*(unsigned char*)(intptr_t)expInfo->ContextRecord->RegisterIp == 0xcc)
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

			currExecutor->vmState.callStackTop->instruction = currExecutor->breakInstructions[index].instIndex;
			currExecutor->vmState.callStackTop++;

			/*unsigned command = */currExecutor->breakFunction(currExecutor->breakFunctionContext, currExecutor->breakInstructions[index].instIndex);
			//printf("Returned command %d\n", command);
			*currExecutor->instAddress[currExecutor->breakInstructions[index].instIndex] = currExecutor->breakInstructions[index].oldOpcode;

			currExecutor->vmState.callStackTop--;

			return (DWORD)EXCEPTION_CONTINUE_EXECUTION;
		}

		if(expCode == EXCEPTION_INT_DIVIDE_BY_ZERO)
		{
			AddCrashStackFrame(internalInstruction);

			currExecutor->Stop("ERROR: integer division by zero");

			return (DWORD)EXCEPTION_EXECUTE_HANDLER;
		}

		if(expCode == EXCEPTION_INT_OVERFLOW)
		{
			AddCrashStackFrame(internalInstruction);

			currExecutor->Stop("ERROR: integer overflow");

			return (DWORD)EXCEPTION_EXECUTE_HANDLER;
		}

		if(expCode == EXCEPTION_ACCESS_VIOLATION && expInfo->ExceptionRecord->ExceptionInformation[1] < 0x00010000)
		{
			AddCrashStackFrame(internalInstruction);

			currExecutor->Stop("ERROR: null pointer access");

			return (DWORD)EXCEPTION_EXECUTE_HANDLER;
		}

		return (DWORD)EXCEPTION_CONTINUE_SEARCH;
	}
#else

#define EXCEPTION_INT_DIVIDE_BY_ZERO 1
#define EXCEPTION_INVALID_POINTER 4

	sigjmp_buf errorHandler;
	
	struct JmpBufData
	{
		char data[sizeof(sigjmp_buf)];
	};

	void HandleError(int signum, siginfo_t *info, void *ucontext)
	{
		if(signum == SIGSEGV && uintptr_t(info->si_addr) >= uintptr_t(currExecutor->vmState.callStackEnd) && uintptr_t(info->si_addr) <= uintptr_t(currExecutor->vmState.callStackEnd) + 8192)
		{
			currExecutor->Stop("ERROR: call stack overflow");
			siglongjmp(errorHandler, 1);
		}

		if(signum == SIGSEGV && uintptr_t(info->si_addr) >= uintptr_t(currExecutor->vmState.dataStackEnd) && uintptr_t(info->si_addr) <= uintptr_t(currExecutor->vmState.dataStackEnd) + 8192)
		{
			currExecutor->Stop("ERROR: stack overflow");
			siglongjmp(errorHandler, 1);
		}

		if(signum == SIGSEGV && uintptr_t(info->si_addr) >= uintptr_t(currExecutor->vmState.regFileArrayEnd) && uintptr_t(info->si_addr) <= uintptr_t(currExecutor->vmState.regFileArrayEnd) + 8192)
		{
			currExecutor->Stop("ERROR: register overflow");
			siglongjmp(errorHandler, 1);
		}

#if defined(_M_X64)
		uintptr_t address = uintptr_t(((ucontext_t*)ucontext)->uc_mcontext.gregs[REG_RIP]);
#else
		uintptr_t address = uintptr_t(((ucontext_t*)ucontext)->uc_mcontext.gregs[REG_EIP]);
#endif

		// Check that exception happened in NULLC code
		bool isInternal = address >= uintptr_t(currExecutor->binCode) && address <= uintptr_t(currExecutor->binCode + currExecutor->binCodeSize);

		unsigned internalInstruction = isInternal ? GetInstructionFromAddress(address) : ~0u;

		for(unsigned i = 0; i < currExecutor->expiredCodeBlocks.size(); i++)
		{
			if(address >= uintptr_t(currExecutor->expiredCodeBlocks[i].code) && address <= uintptr_t(currExecutor->expiredCodeBlocks[i].code + currExecutor->expiredCodeBlocks[i].codeSize))
			{
				isInternal = true;
				break;
			}
		}

		if(currExecutor->vmState.jitCodeActive)
			isInternal = true;

		if(!isInternal)
		{
			signal(signum, SIG_DFL);
			raise(signum);
			return;
		}

		if(signum == SIGFPE)
		{
			AddCrashStackFrame(internalInstruction);

			currExecutor->Stop("ERROR: integer division by zero");

			siglongjmp(errorHandler, 1);
		}

		if(signum == SIGSEGV && uintptr_t(info->si_addr) < 0x00010000)
		{
			AddCrashStackFrame(internalInstruction);

			currExecutor->Stop("ERROR: null pointer access");

			siglongjmp(errorHandler, 1);
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

	void DenyMemoryPageRead(void *addr)
	{
		void *alignedAddr = (void*)((uintptr_t(addr) & ~4095) + 4096);

#ifndef __linux
		DWORD unusedProtect;
		VirtualProtect(alignedAddr, 4096, PAGE_NOACCESS, &unusedProtect);
#else
		mprotect(alignedAddr, 4096, PROT_NONE);
#endif
	}

	void AllowMemoryPageRead(void *addr)
	{
		void *alignedAddr = (void*)((uintptr_t(addr) & ~4095) + 4096);

#ifndef __linux
		DWORD unusedProtect;
		VirtualProtect(alignedAddr, 4096, PAGE_READWRITE, &unusedProtect);
#else
		mprotect(alignedAddr, 4096, PROT_READ | PROT_WRITE);
#endif
	}

	typedef void (*codegenCallback)(CodeGenRegVmContext &ctx, RegVmCmd);
	codegenCallback cgFuncs[rviConvertPtr + 1];
}

ExecutorX86::ExecutorX86(Linker *linker): exLinker(linker), exTypes(linker->exTypes), exFunctions(linker->exFunctions), exRegVmCode(linker->exRegVmCode), exRegVmConstants(linker->exRegVmConstants), exRegVmRegKillInfo(linker->exRegVmRegKillInfo)
{
	codeGenCtx = NULL;

	execErrorBuffer = (char*)NULLC::alloc(NULLC_ERROR_BUFFER_SIZE);
	*execErrorBuffer = 0;

	NULLC::fillMemory(execResult, 0, execResultSize);

	codeRunning = false;

	minStackSize = 1 * 1024 * 1024;

	lastFinalReturn = 0;

	callContinue = true;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	dcCallVM = NULL;
#endif

	breakFunctionContext = NULL;
	breakFunction = NULL;

	NULLC::fillMemory(codeLaunchHeader, 0, codeLaunchHeaderSize);
	oldCodeLaunchHeaderProtect = 0;

	codeLaunchHeaderLength = 0;
	codeLaunchUnwindOffset = 0;
	codeLaunchDataLength = 0;
	codeLaunchWin64UnwindTable = NULL;

	binCode = NULL;
	binCodeSize = 0;
	binCodeReserved = 0;

	lastInstructionCount = 0;

	oldJumpTargetCount = 0;
	oldRegKillInfoCount = 0;
	oldFunctionSize = 0;
	oldCodeBodyProtect = 0;

	NULLC::currExecutor = this;
}

ExecutorX86::~ExecutorX86()
{
	NULLC::dealloc(execErrorBuffer);

	NULLC::AllowMemoryPageRead(vmState.callStackEnd);
	NULLC::AllowMemoryPageRead(vmState.dataStackEnd);
	NULLC::AllowMemoryPageRead(vmState.regFileArrayEnd);

	NULLC::dealloc(vmState.dataStackBase);

	NULLC::dealloc(vmState.callStackBase);

	NULLC::dealloc(vmState.tempStackArrayBase);

	NULLC::dealloc(vmState.regFileArrayBase);

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	if(dcCallVM)
		dcFree(dcCallVM);
#endif

	ClearNative();

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

	NULLC::dealloc(codeLaunchWin64UnwindTable);

	NULLC::dealloc(binCode);

	NULLC::currExecutor = NULL;

	x86ResetLabels();
}

#define nullcOffsetOf(obj, field) unsigned(uintptr_t(&(obj)->field) - uintptr_t(obj))

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
	pos += x64SUB(pos, rRSP, 40);

#ifndef __linux
	pos += x64MOV(pos, rRBX, rRDX);
#else
	pos += x64MOV(pos, rRBX, rRSI);
#endif

	pos += x64MOV(pos, rR13, uintptr_t(&vmState));
	pos += x86MOV(pos, rR15, sQWORD, rNONE, 1, rR13, nullcOffsetOf(&vmState, dataStackTop));
	pos += x86MOV(pos, rR14, sQWORD, rNONE, 1, rR13, nullcOffsetOf(&vmState, exRegVmConstants));
	pos += x86MOV(pos, rRAX, sQWORD, rNONE, 1, rR13, nullcOffsetOf(&vmState, dataStackBase));
	pos += x86MOV(pos, sQWORD, rNONE, 1, rRBX, 0, rRAX);

#ifndef __linux
	pos += x86CALL(pos, rECX);
#else
	pos += x86CALL(pos, rRDI);
#endif

	// Restore registers
	pos += x64ADD(pos, rRSP, 40);
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
	assert(sizeof(UNWIND_INFO_ENTRY) == 4 + 10 * 2);

	UNWIND_INFO_ENTRY unwindInfo = { 0 };

	unwindInfo.version = 1;
	unwindInfo.flags = 0; // No EH
	unwindInfo.sizeOfProlog = 16;
	unwindInfo.countOfCodes = 9;
	unwindInfo.frameRegister = 0;
	unwindInfo.frameOffset = 0;

	unwindInfo.unwindCode[0].offsetInPrologue = 16;
	unwindInfo.unwindCode[0].operationCode = UWOP_ALLOC_SMALL;
	unwindInfo.unwindCode[0].operationInfo = (40 - 8) / 8;

	unwindInfo.unwindCode[1].offsetInPrologue = 12;
	unwindInfo.unwindCode[1].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[1].operationInfo = 15; // r15

	unwindInfo.unwindCode[2].offsetInPrologue = 10;
	unwindInfo.unwindCode[2].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[2].operationInfo = 14; // r14

	unwindInfo.unwindCode[3].offsetInPrologue = 8;
	unwindInfo.unwindCode[3].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[3].operationInfo = 13; // r13

	unwindInfo.unwindCode[4].offsetInPrologue = 6;
	unwindInfo.unwindCode[4].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[4].operationInfo = 12; // r12

	unwindInfo.unwindCode[5].offsetInPrologue = 4;
	unwindInfo.unwindCode[5].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[5].operationInfo = UWOP_REGISTER_RSI;

	unwindInfo.unwindCode[6].offsetInPrologue = 3;
	unwindInfo.unwindCode[6].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[6].operationInfo = UWOP_REGISTER_RDI;

	unwindInfo.unwindCode[7].offsetInPrologue = 2;
	unwindInfo.unwindCode[7].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[7].operationInfo = UWOP_REGISTER_RBX;

	unwindInfo.unwindCode[8].offsetInPrologue = 1;
	unwindInfo.unwindCode[8].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[8].operationInfo = UWOP_REGISTER_RBP;

	NULLC::copyMemory(pos, &unwindInfo, sizeof(unwindInfo));
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
	NULLC::MemProtect((void*)codeLaunchHeader, codeLaunchHeaderSize, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

	if(!vmState.callStackBase)
	{
		vmState.callStackBase = (CodeGenRegVmCallStackEntry*)NULLC::alloc(sizeof(CodeGenRegVmCallStackEntry) * 1024 * 2 + 8192); // Two extra pages for page guard
		NULLC::fillMemory(vmState.callStackBase, 0, sizeof(CodeGenRegVmCallStackEntry) * 1024 * 2);
		vmState.callStackEnd = vmState.callStackBase + 1024 * 2;
	}

	if(!vmState.tempStackArrayBase)
	{
		vmState.tempStackArrayBase = (unsigned*)NULLC::alloc(sizeof(unsigned) * 1024 * 16);
		NULLC::fillMemory(vmState.tempStackArrayBase, 0, sizeof(unsigned) * 1024 * 16);
		vmState.tempStackArrayEnd = vmState.tempStackArrayBase + 1024 * 16;
	}

	if(!vmState.dataStackBase)
	{
		vmState.dataStackBase = (char*)NULLC::alloc(sizeof(char) * minStackSize + 8192); // Two extra pages for page guard
		NULLC::fillMemory(vmState.dataStackBase, 0, sizeof(char) * minStackSize);
		vmState.dataStackEnd = vmState.dataStackBase + minStackSize;
	}

	if(!vmState.regFileArrayBase)
	{
		vmState.regFileArrayBase = (RegVmRegister*)NULLC::alloc(sizeof(RegVmRegister) * 1024 * 32 + 8192); // Two extra pages for page guard
		NULLC::fillMemory(vmState.regFileArrayBase, 0, sizeof(RegVmRegister) * 1024 * 32);
		vmState.regFileArrayEnd = vmState.regFileArrayBase + 1024 * 32;
	}

	NULLC::DenyMemoryPageRead(vmState.callStackEnd);
	NULLC::DenyMemoryPageRead(vmState.dataStackEnd);
	NULLC::DenyMemoryPageRead(vmState.regFileArrayEnd);

	//x86TestEncoding(codeLaunchHeader);

	return true;
}

bool ExecutorX86::InitExecution()
{
	vmState.callStackTop = vmState.callStackBase;

	lastFinalReturn = 0;

	CommonSetLinker(exLinker);

	vmState.dataStackTop = vmState.dataStackBase + ((exLinker->globalVarSize + 0xf) & ~0xf);

	if(vmState.dataStackTop >= vmState.dataStackEnd)
	{
		Stop("ERROR: allocated stack overflow");
		return false;
	}

	GC::SetUnmanagableRange(vmState.dataStackBase, unsigned(vmState.dataStackEnd - vmState.dataStackBase));

	execErrorMessage = NULL;

	execErrorObject.typeID = 0;
	execErrorObject.ptr = NULL;

	execErrorFinalReturnDepth = 0;

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

	if(!exLinker->fullLinkerData.empty())
	{
		nullcModuleStartAddress = uintptr_t(binCode);
		nullcModuleEndAddress = uintptr_t(binCode + binCodeSize);

		nullcJitContextMainDataAddress = uintptr_t(&vmState.dataStackBase);
	}

	return true;
}

bool ExecutorX86::Run(unsigned int functionID, const char *arguments)
{
	if(exRegVmCode.empty())
	{
		Stop("ERROR: module contains no code");
		return false;
	}

	bool firstRun = !codeRunning || functionID == ~0u;

	if(firstRun)
	{
		if(!InitExecution())
			return false;
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

	if(functionID != ~0u)
	{
		ExternFuncInfo &target = exFunctions[functionID];

		unsigned funcPos = ~0u;
		funcPos = target.regVmAddress;

		retType = (RegVmReturnType)GetFunctionVmReturnType(target, exTypes.data, exLinker->exTypeExtra.data);

		if(funcPos == ~0u)
		{
			// Can't return complex types here
			if(target.retType == ExternFuncInfo::RETURN_UNKNOWN)
			{
				Stop("ERROR: can't call external function with complex return type");
				return false;
			}

			// Copy all arguments
			NULLC::copyMemory(vmState.tempStackArrayBase, arguments, target.argumentSize);

			// Call function
			if(target.funcPtrWrap)
			{
				target.funcPtrWrap(target.funcPtrWrapTarget, (char*)vmState.tempStackArrayBase, (char*)vmState.tempStackArrayBase);

				if(!callContinue)
					errorState = true;
			}
			else
			{
#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
				RunRawExternalFunction(dcCallVM, exFunctions[functionID], exLinker->exLocals.data, exTypes.data, exLinker->exTypeExtra.data, vmState.tempStackArrayBase, vmState.tempStackArrayBase);

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

			unsigned argumentsSize = target.argumentSize;

			if(unsigned(vmState.dataStackTop - vmState.dataStackBase) + argumentsSize >= unsigned(vmState.dataStackEnd - vmState.dataStackBase))
			{
				CodeGenRegVmCallStackEntry *entry = vmState.callStackTop;

				entry->instruction = instructionPos + 1;

				vmState.callStackTop++;

				instructionPos = ~0u;
				Stop("ERROR: stack overflow");
				retType = rvrError;
			}
			else
			{
				// Copy arguments to new stack frame
				NULLC::copyMemory(vmState.dataStackTop, arguments, argumentsSize);

				unsigned stackSize = (target.stackSize + 0xf) & ~0xf;

				if(unsigned(vmState.dataStackTop - vmState.dataStackBase) + stackSize >= unsigned(vmState.dataStackEnd - vmState.dataStackBase))
				{
					CodeGenRegVmCallStackEntry *entry = vmState.callStackTop;

					entry->instruction = instructionPos + 1;

					vmState.callStackTop++;

					instructionPos = ~0u;
					Stop("ERROR: stack overflow");
					retType = rvrError;
				}
				else
				{
					regFilePtr[rvrrGlobals].ptrValue = uintptr_t(vmState.dataStackBase);
					regFilePtr[rvrrFrame].ptrValue = uintptr_t(vmState.dataStackBase + prevDataSize);
					regFilePtr[rvrrConstants].ptrValue = uintptr_t(exLinker->exRegVmConstants.data);
					regFilePtr[rvrrRegisters].ptrValue = uintptr_t(regFilePtr);
				}
			}
		}
	}
	else
	{
		// If global code is executed, reset all global variables
		assert(unsigned(vmState.dataStackTop - vmState.dataStackBase) >= exLinker->globalVarSize);
		NULLC::fillMemory(vmState.dataStackBase, 0, exLinker->globalVarSize);

		regFilePtr[rvrrGlobals].ptrValue = uintptr_t(vmState.dataStackBase);
		regFilePtr[rvrrFrame].ptrValue = uintptr_t(vmState.dataStackBase);
		regFilePtr[rvrrConstants].ptrValue = uintptr_t(exLinker->exRegVmConstants.data);
		regFilePtr[rvrrRegisters].ptrValue = uintptr_t(regFilePtr);

		NULLC::fillMemory(regFilePtr + rvrrCount, 0, (256 - rvrrCount) * sizeof(regFilePtr[0]));
	}

	RegVmRegister *prevRegFilePtr = vmState.regFileLastPtr;

	vmState.regFileLastPtr = regFilePtr;

	if(functionID == ~0u)
		vmState.regFileLastTop += 256;

	RegVmReturnType resultType = retType;

	if(instructionPos != ~0u)
	{
		vmState.jitCodeActive = true;

#ifdef __linux
		struct sigaction sa;
		struct sigaction sigFPE;
		struct sigaction sigTRAP;
		struct sigaction sigSEGV;

		if(firstRun)
		{
			sa.sa_sigaction = NULLC::HandleError;
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = SA_RESTART | SA_SIGINFO;

			sigaction(SIGFPE, &sa, &sigFPE);
			sigaction(SIGTRAP, &sa, &sigTRAP);
			sigaction(SIGSEGV, &sa, &sigSEGV);
		}

		int errorCode = 0;

		NULLC::JmpBufData data;
		NULLC::copyMemory(data.data, NULLC::errorHandler, sizeof(sigjmp_buf));

		if(!(errorCode = sigsetjmp(NULLC::errorHandler, 1)))
		{
			unsigned char *codeStart = instAddress[instructionPos];

			jmp_buf prevErrorHandler;
			NULLC::copyMemory(&prevErrorHandler, &vmState.errorHandler, sizeof(jmp_buf));

			if(!setjmp(vmState.errorHandler))
			{
				typedef	uintptr_t(*nullcFunc)(unsigned char *codeStart, RegVmRegister *regFilePtr);
				nullcFunc gate = (nullcFunc)(uintptr_t)codeLaunchHeader;
				resultType = (RegVmReturnType)gate(codeStart, regFilePtr);
			}
			else
			{
				resultType = rvrError;
			}

			NULLC::copyMemory(&vmState.errorHandler, &prevErrorHandler, sizeof(jmp_buf));
		}
		else
		{
			resultType = rvrError;
		}

		// Disable signal handlers only from top-level Run
		if(lastFinalReturn == 0)
		{
			sigaction(SIGFPE, &sigFPE, NULL);
			sigaction(SIGTRAP, &sigTRAP, NULL);
			sigaction(SIGSEGV, &sigSEGV, NULL);
		}

		NULLC::copyMemory(NULLC::errorHandler, data.data, sizeof(sigjmp_buf));
#else
		__try
		{
			unsigned char *codeStart = instAddress[instructionPos];

			jmp_buf prevErrorHandler;
			NULLC::copyMemory(&prevErrorHandler, &vmState.errorHandler, sizeof(jmp_buf));

			if(!setjmp(vmState.errorHandler))
			{
				typedef	uintptr_t(*nullcFunc)(unsigned char *codeStart, RegVmRegister *regFilePtr);
				nullcFunc gate = (nullcFunc)(uintptr_t)codeLaunchHeader;
				resultType = (RegVmReturnType)gate(codeStart, regFilePtr);
			}
			else
			{
				resultType = rvrError;
			}

			NULLC::copyMemory(&vmState.errorHandler, &prevErrorHandler, sizeof(jmp_buf));
		}
		__except(NULLC::CanWeHandleSEH(GetExceptionCode(), GetExceptionInformation()))
		{
			resultType = rvrError;
		}
#endif

		vmState.jitCodeActive = false;
	}

	vmState.regFileLastPtr = prevRegFilePtr;

	if(functionID == ~0u)
		vmState.regFileLastTop -= 256;

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
			char *currPos = execErrorBuffer + strlen(execErrorBuffer);
			currPos += NULLC::SafeSprintf(currPos, NULLC_ERROR_BUFFER_SIZE - int(currPos - execErrorBuffer), "\r\nCall stack:\r\n");

			unsigned currentFrame = 0;
			while(unsigned address = GetCallStackAddress(currentFrame++))
				currPos += PrintStackFrame(address, currPos, NULLC_ERROR_BUFFER_SIZE - int(currPos - execErrorBuffer), false);
		}

		execErrorFinalReturnDepth = lastFinalReturn;
		lastFinalReturn = prevLastFinalReturn;

		// Ascertain that execution stops when there is a chain of nullcRunFunction
		callContinue = false;
		codeRunning = false;

		return false;
	}

	if(lastFinalReturn == 0)
		codeRunning = false;

	lastFinalReturn = prevLastFinalReturn;

	if(functionID != ~0u)
	{
		ExternFuncInfo &target = exFunctions[functionID];
		ExternTypeInfo &targetType = exTypes[target.funcType];

		vmState.tempStackType = exLinker->exTypeExtra[targetType.memberOffset].type;
	}
	else
	{
		vmState.tempStackType = NULLC_TYPE_VOID;

		switch(retType)
		{
		case rvrInt:
			vmState.tempStackType = NULLC_TYPE_INT;
			break;
		case rvrDouble:
			vmState.tempStackType = NULLC_TYPE_DOUBLE;
			break;
		case rvrLong:
			vmState.tempStackType = NULLC_TYPE_LONG;
			break;
		default:
			break;
		}
	}

	return true;
}

void ExecutorX86::Stop(const char* error)
{
	codeRunning = false;

	callContinue = false;

	NULLC::SafeSprintf(execErrorBuffer, NULLC_ERROR_BUFFER_SIZE, "%s", error);

	execErrorMessage = execErrorBuffer;

	execErrorObject.typeID = 0;
	execErrorObject.ptr = NULL;
}

void ExecutorX86::Stop(NULLCRef error)
{
	codeRunning = false;

	callContinue = false;

	NULLC::SafeSprintf(execErrorBuffer, NULLC_ERROR_BUFFER_SIZE, "%s", exLinker->exSymbols.data + exTypes[error.typeID].offsetToName);

	execErrorMessage = execErrorBuffer;

	if(nullcIsStackPointer(error.ptr))
		execErrorObject = NULLC::CopyObject(error);
	else
		execErrorObject = error;
}

void ExecutorX86::Resume()
{
	vmState.callStackTop = vmState.callStackBase + execErrorFinalReturnDepth;

	codeRunning = true;

	callContinue = true;

	execErrorMessage = NULL;

	execErrorObject.typeID = 0;
	execErrorObject.ptr = NULL;
}

bool ExecutorX86::SetStackSize(unsigned bytes)
{
	if(codeRunning || !instList.empty())
		return false;

	minStackSize = bytes;

	NULLC::AllowMemoryPageRead(vmState.dataStackEnd);

	NULLC::dealloc(vmState.dataStackBase);

	vmState.dataStackBase = (char*)NULLC::alloc(sizeof(char) * minStackSize + 8192); // Two extra pages for page guard
	NULLC::fillMemory(vmState.dataStackBase, 0, sizeof(char) * minStackSize);
	vmState.dataStackEnd = vmState.dataStackBase + minStackSize;

	NULLC::DenyMemoryPageRead(vmState.dataStackEnd);

	return true;
}

void ExecutorX86::ClearNative()
{
	TRACE_SCOPE("x86", "ClearNative");

	if (instList.size())
		NULLC::fillMemory(instList.data, 0, sizeof(x86Instruction) * instList.size());
	instList.clear();

	binCodeSize = 0;
	lastInstructionCount = 0;

	globalCodeRanges.clear();

	for(unsigned i = 0; i < expiredCodeBlocks.size(); i++)
	{
		ExpiredCodeBlock &block = expiredCodeBlocks[i];

#ifndef __linux
		DWORD unusedProtect;
		VirtualProtect((void*)block.code, block.codeSize, oldCodeBodyProtect, &unusedProtect);
#else
		NULLC::MemProtect((void*)block.code, block.codeSize, PROT_READ | PROT_WRITE);
#endif

		NULLC::dealloc(block.code);

#if defined(_M_X64) && !defined(__linux)
		if(block.unwindTable)
		{
			RtlDeleteFunctionTable(block.unwindTable);
			NULLC::dealloc(block.unwindTable);
		}
#endif
	}

	expiredCodeBlocks.clear();

	for(unsigned i = 0; i < expiredFunctionAddressLists.size(); i++)
	{
		ExpiredFunctionAddressList &info = expiredFunctionAddressLists[i];

		NULLC::dealloc(info.data);
	}
	expiredFunctionAddressLists.clear();

#if defined(_M_X64) && !defined(__linux)
	// Remove function table for unwind information
	if(!functionWin64UnwindTable.empty())
		RtlDeleteFunctionTable(functionWin64UnwindTable.data);

	functionWin64UnwindTable.clear();
#endif

	oldJumpTargetCount = 0;
	oldRegKillInfoCount = 0;
	oldFunctionSize = 0;

	// Create new code generation context
	if(codeGenCtx)
		NULLC::destruct(codeGenCtx);
	codeGenCtx = NULL;

	codeRunning = false;
}

bool ExecutorX86::TranslateToNative(bool enableLogFiles, OutputContext &output)
{
	TRACE_SCOPE("x86", "TranslateToNative");

	if(instList.size())
		NULLC::fillMemory(instList.data, 0, sizeof(x86Instruction) * instList.size());
	instList.clear();
	instList.reserve(64);

	if(!codeGenCtx)
		codeGenCtx = NULLC::construct<CodeGenRegVmContext>();

	codeGenCtx->x86rvm = this;

	codeGenCtx->exFunctions = exFunctions.data;
	codeGenCtx->exTypes = exTypes.data;
	codeGenCtx->exTypeExtra = exLinker->exTypeExtra.data;
	codeGenCtx->exLocals = exLinker->exLocals.data;
	codeGenCtx->exRegVmConstants = exRegVmConstants.data;
	codeGenCtx->exRegVmConstantsEnd = exRegVmConstants.data + exRegVmConstants.count;
	codeGenCtx->exRegVmRegKillInfo = exRegVmRegKillInfo.data;
	codeGenCtx->exSymbols = exLinker->exSymbols.data;

	codeGenCtx->vmState = &vmState;

	vmState.ctx = codeGenCtx;
	vmState.exRegVmConstants = exRegVmConstants.data;

	codeGenCtx->ctx.SetLastInstruction(instList.data, instList.data);

	CommonSetLinker(exLinker);

	EMIT_OP(codeGenCtx->ctx, o_use32);

	codeJumpTargets.resize(exRegVmCode.size());
	if(codeJumpTargets.size())
		NULLC::fillMemory(&codeJumpTargets[lastInstructionCount], 0, (codeJumpTargets.size() - lastInstructionCount) * sizeof(codeJumpTargets[0]));

	// Mirror extra global return so that jump to global return can be marked (rviNop, because we will have some custom code)
	codeJumpTargets.push_back(0);
	for(unsigned i = oldJumpTargetCount, e = exLinker->regVmJumpTargets.size(); i != e; i++)
		codeJumpTargets[exLinker->regVmJumpTargets[i]] = 1;

	// Mark function locations
	for(unsigned i = 0, e = exLinker->exFunctions.size(); i != e; i++)
	{
		ExternFuncInfo &target = exLinker->exFunctions[i];

		if(target.regVmAddress != -1 && target.regVmCodeSize != 0 && (codeJumpTargets[target.regVmAddress] >> 8) == 0)
			codeJumpTargets[target.regVmAddress] |= 2 + (i << 8);
	}

	// Find instruction register kill info positions
	codeRegKillInfoOffsets.resize(exRegVmCode.size());
	for(unsigned i = lastInstructionCount, e = exRegVmCode.size(); i != e; i++)
	{
		codeRegKillInfoOffsets[i] = oldRegKillInfoCount;

		unsigned counts = exRegVmRegKillInfo[oldRegKillInfoCount];

		oldRegKillInfoCount += 1 + (counts >> 4) + (counts & 0xf);
	}
	assert(oldRegKillInfoCount == exRegVmRegKillInfo.size());

	if(codeRunning && exFunctions.size() >= functionAddress.max)
	{
		ExpiredFunctionAddressList info;

		info.data = functionAddress.data;
		info.count = functionAddress.count;

		expiredFunctionAddressLists.push_back(info);

		functionAddress.data = NULL;
		functionAddress.count = 0;
		functionAddress.max = 0;

		functionAddress.resize(exFunctions.size());

		for(unsigned int i = 0; i < oldFunctionSize; i++)
		{
			if(exFunctions[i].regVmAddress != -1)
				functionAddress[i] = instAddress[exFunctions[i].regVmAddress];
			else
				functionAddress[i] = 0;
		}
	}
	else
	{
		functionAddress.resize(exFunctions.size());
	}

	vmState.functionAddress = functionAddress.data;

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
		codeGenCtx->currInstructionRegKillOffset = codeRegKillInfoOffsets[pos];

		// Frame setup
		if((codeJumpTargets[pos] & 6) != 0)
		{
			if(codeJumpTargets[pos] & 4)
			{
				activeGlobalCodeStart = pos;

				codeGenCtx->currFunctionId = 0;
			}

			EMIT_OP_NUM(codeGenCtx->ctx, o_set_tracking, 0);

#if defined(_M_X64)
			EMIT_OP_REG(codeGenCtx->ctx, o_push, rRBX);
			EMIT_OP_REG(codeGenCtx->ctx, o_push, rR15);
			EMIT_OP_REG_NUM(codeGenCtx->ctx, o_sub64, rRSP, 40);
#else
			EMIT_OP_REG(codeGenCtx->ctx, o_push, rEBP);
			EMIT_OP_REG_REG(codeGenCtx->ctx, o_mov, rEBP, rESP);
			EMIT_OP_REG(codeGenCtx->ctx, o_push, rEBX);
			EMIT_OP_REG(codeGenCtx->ctx, o_push, rESI);
#endif

			EMIT_OP_NUM(codeGenCtx->ctx, o_set_tracking, 1);

			// Generate function prologue (register cleanup, data stack advance, data stack cleanup)
			if(codeJumpTargets[pos] & 2)
			{
				codeGenCtx->currFunctionId = codeJumpTargets[pos] >> 8;

				ExternFuncInfo &target = exLinker->exFunctions[codeGenCtx->currFunctionId];

				unsigned stackSize = (target.stackSize + 0xf) & ~0xf;
				unsigned argumentsSize = target.argumentSize;

#if defined(_M_X64)
				EMIT_OP_NUM(codeGenCtx->ctx, o_set_tracking, 0);

				EMIT_OP_REG_RPTR(codeGenCtx->ctx, o_mov64, rRBX, sQWORD, rR13, nullcOffsetOf(&vmState, regFileLastTop));
				EMIT_OP_REG_RPTR(codeGenCtx->ctx, o_mov64, rR15, sQWORD, rR13, nullcOffsetOf(&vmState, dataStackTop));

				EMIT_OP_REG_RPTR(codeGenCtx->ctx, o_mov64, rRAX, sQWORD, rR13, nullcOffsetOf(&vmState, dataStackBase));
				EMIT_OP_RPTR_REG(codeGenCtx->ctx, o_mov64, sQWORD, rRBX, 0, rRAX);

				// Advance frame top
				EMIT_OP_RPTR_NUM(codeGenCtx->ctx, o_add64, sQWORD, rR13, nullcOffsetOf(&vmState, dataStackTop), stackSize); // vmState->dataStackTop += stackSize;

				// Advance register top
				EMIT_OP_RPTR_NUM(codeGenCtx->ctx, o_add64, sQWORD, rR13, nullcOffsetOf(&vmState, regFileLastTop), target.regVmRegisters * 8); // vmState->regFileLastTop += target.regVmRegisters;

				EMIT_OP_NUM(codeGenCtx->ctx, o_set_tracking, 1);

				bool isRaxCleared = false;

				// Clear register values
				if (target.regVmRegisters > rvrrCount)
				{
					unsigned count = target.regVmRegisters - rvrrCount;

					if(count <= 8)
					{
						for(int regId = rvrrCount; regId < target.regVmRegisters; regId++)
							EMIT_OP_RPTR_NUM(codeGenCtx->ctx, o_mov64, sQWORD, rRBX, regId * 8, 0);
					}
					else
					{
						isRaxCleared = true;

						EMIT_OP_REG_REG(codeGenCtx->ctx, o_xor, rRAX, rRAX);
						EMIT_OP_REG_RPTR(codeGenCtx->ctx, o_lea, rRDI, sQWORD, rRBX, rvrrCount * 8);
						EMIT_OP_REG_NUM(codeGenCtx->ctx, o_mov, rECX, count);
						EMIT_OP(codeGenCtx->ctx, o_rep_stosq);
					}
				}

				// Clear data stack
				// TODO: use target.stackSize which is smaller?
				if(unsigned count = stackSize - argumentsSize)
				{
					assert(count % 4 == 0);

					if(count <= 16)
					{
						for(unsigned dataId = 0; dataId < count / 4; dataId++)
							EMIT_OP_RPTR_NUM(codeGenCtx->ctx, o_mov, sDWORD, rR15, argumentsSize + dataId * 4, 0);
					}
					else
					{
						if(!isRaxCleared)
							EMIT_OP_REG_REG(codeGenCtx->ctx, o_xor, rRAX, rRAX);

						EMIT_OP_REG_RPTR(codeGenCtx->ctx, o_lea, rRDI, sQWORD, rR15, argumentsSize);
						EMIT_OP_REG_NUM(codeGenCtx->ctx, o_mov, rECX, count / 4);
						EMIT_OP(codeGenCtx->ctx, o_rep_stosd);
					}
				}
#else
				EMIT_OP_NUM(codeGenCtx->ctx, o_set_tracking, 0);

				EMIT_OP_REG_ADDR(codeGenCtx->ctx, o_mov, rEBX, sDWORD, uintptr_t(&vmState.regFileLastTop));
				EMIT_OP_REG_RPTR(codeGenCtx->ctx, o_mov, rESI, sDWORD, rNONE, 1, rEBX, rvrrFrame * 8);

				// Advance frame top
				EMIT_OP_RPTR_NUM(codeGenCtx->ctx, o_add, sDWORD, uintptr_t(&vmState.dataStackTop), stackSize); // vmState->dataStackTop += stackSize;

				// Advance register top
				EMIT_OP_RPTR_NUM(codeGenCtx->ctx, o_add, sDWORD, uintptr_t(&vmState.regFileLastTop), target.regVmRegisters * 8); // vmState->regFileLastTop += target.regVmRegisters;

				EMIT_OP_NUM(codeGenCtx->ctx, o_set_tracking, 1);

				bool isEaxCleared = false;

				// Clear register values
				if(target.regVmRegisters > rvrrCount)
				{
					unsigned count = target.regVmRegisters - rvrrCount;

					if(count <= 4)
					{
						for(int regId = rvrrCount; regId < target.regVmRegisters; regId++)
						{
							EMIT_OP_RPTR_NUM(codeGenCtx->ctx, o_mov, sDWORD, rEBX, regId * 8, 0);
							EMIT_OP_RPTR_NUM(codeGenCtx->ctx, o_mov, sDWORD, rEBX, regId * 8, 4);
						}
					}
					else
					{
						isEaxCleared = true;

						EMIT_OP_REG_REG(codeGenCtx->ctx, o_xor, rEAX, rEAX);
						EMIT_OP_REG_RPTR(codeGenCtx->ctx, o_lea, rEDI, sDWORD, rEBX, rvrrCount * 8);
						EMIT_OP_REG_NUM(codeGenCtx->ctx, o_mov, rECX, count * 2);
						EMIT_OP(codeGenCtx->ctx, o_rep_stosd);
					}
				}

				// Clear data stack
				// TODO: use target.stackSize which is smaller?
				if(unsigned count = stackSize - argumentsSize)
				{
					assert(count % 4 == 0);

					if(count <= 16)
					{
						for(unsigned dataId = 0; dataId < count / 4; dataId++)
							EMIT_OP_RPTR_NUM(codeGenCtx->ctx, o_mov, sDWORD, rESI, argumentsSize + dataId * 4, 0);
					}
					else
					{
						if(!isEaxCleared)
							EMIT_OP_REG_REG(codeGenCtx->ctx, o_xor, rEAX, rEAX);

						EMIT_OP_REG_RPTR(codeGenCtx->ctx, o_lea, rEDI, sDWORD, rESI, argumentsSize);
						EMIT_OP_REG_NUM(codeGenCtx->ctx, o_mov, rECX, count / 4);
						EMIT_OP(codeGenCtx->ctx, o_rep_stosd);
					}
				}
#endif
			}
		}

		if(cmd.code == rviJmp && cmd.rA)
		{
			codeJumpTargets[cmd.argument] |= 4;

			if(activeGlobalCodeStart != 0)
				globalCodeRanges.push_back(pos);

			globalCodeRanges.push_back(cmd.argument);

			if(pos)
			{
#if defined(_M_X64)
				EMIT_OP_REG_NUM(codeGenCtx->ctx, o_add64, rRSP, 40);
				EMIT_OP_REG(codeGenCtx->ctx, o_pop, rR15);
				EMIT_OP_REG(codeGenCtx->ctx, o_pop, rRBX);
#else
				EMIT_OP_REG(codeGenCtx->ctx, o_pop, rESI);
				EMIT_OP_REG(codeGenCtx->ctx, o_pop, rEBX);
				EMIT_OP_REG_REG(codeGenCtx->ctx, o_mov, rESP, rEBP);
				EMIT_REG_READ(codeGenCtx->ctx, rESP);
				EMIT_OP_REG(codeGenCtx->ctx, o_pop, rEBP);
#endif
			}
		}

		pos++;

		NULLC::cgFuncs[cmd.code](*codeGenCtx, cmd);

		codeGenCtx->ctx.KillLateUnreadRegVmRegisters(exRegVmRegKillInfo.data + codeGenCtx->currInstructionRegKillOffset);
		codeGenCtx->ctx.UnlockRegisters();

		SetOptimizationLookBehind(codeGenCtx->ctx, true);
	}

	globalCodeRanges.push_back(pos);

	// Add extra global return if there is none
	codeGenCtx->ctx.GetLastInstruction()->instID = pos + 1;

	if((codeJumpTargets[exRegVmCode.size()] & 6) != 0)
	{
		EMIT_OP_NUM(codeGenCtx->ctx, o_set_tracking, 0);

#if defined(_M_X64)
		EMIT_OP_REG(codeGenCtx->ctx, o_push, rRBX);
		EMIT_OP_REG(codeGenCtx->ctx, o_push, rR15);
		EMIT_OP_REG_NUM(codeGenCtx->ctx, o_sub64, rRSP, 40);
#else
		EMIT_OP_REG(codeGenCtx->ctx, o_push, rEBP);
		EMIT_OP_REG_REG(codeGenCtx->ctx, o_mov, rEBP, rESP);
		EMIT_OP_REG(codeGenCtx->ctx, o_push, rEBX);
		EMIT_OP_REG(codeGenCtx->ctx, o_push, rESI);
#endif

		EMIT_OP_NUM(codeGenCtx->ctx, o_set_tracking, 1);
	}

	EMIT_OP_REG_REG(codeGenCtx->ctx, o_xor, rEAX, rEAX);

#if defined(_M_X64)
	EMIT_OP_REG_NUM(codeGenCtx->ctx, o_add64, rRSP, 40);
	EMIT_OP_REG(codeGenCtx->ctx, o_pop, rR15);
	EMIT_OP_REG(codeGenCtx->ctx, o_pop, rRBX);
#else
	EMIT_OP_REG(codeGenCtx->ctx, o_pop, rESI);
	EMIT_OP_REG(codeGenCtx->ctx, o_pop, rEBX);
	EMIT_OP_REG_REG(codeGenCtx->ctx, o_mov, rESP, rEBP);
	EMIT_REG_READ(codeGenCtx->ctx, rESP);
	EMIT_OP_REG(codeGenCtx->ctx, o_pop, rEBP);
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
		x86Instruction &inst = instList[i];

		// Skip trash
		if(inst.name == o_none)
		{
			EMIT_OP(codeGenCtx->ctx, o_none);
			continue;
		}
		// If invalidation flag is set
		if(inst.instID && codeJumpTargets[inst.instID - 1])
			SetOptimizationLookBehind(codeGenCtx->ctx, false);

		if(inst.name == o_label)
		{
			EMIT_LABEL(codeGenCtx->ctx, inst.labelID, inst.argA.num);
			SetOptimizationLookBehind(codeGenCtx->ctx, true);
			continue;
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
#endif

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
		if(binCode && !codeRunning)
			VirtualProtect((void*)binCode, oldBinCodeReserved, oldCodeBodyProtect, (DWORD*)&unusedProtect);
		VirtualProtect((void*)binCodeNew, binCodeReserved, PAGE_EXECUTE_READWRITE, (DWORD*)&oldCodeBodyProtect);
#else
		if(binCode && !codeRunning)
			NULLC::MemProtect((void*)binCode, oldBinCodeReserved, PROT_READ | PROT_WRITE);
		NULLC::MemProtect((void*)binCodeNew, binCodeReserved, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

		if(binCodeSize)
			NULLC::copyMemory(binCodeNew, binCode, binCodeSize);

		// If code is currently running, update all instruction pointers
		if(codeRunning)
		{
			codeRelocated = true;

			ExpiredCodeBlock block;

			block.code = binCode;
			block.codeSize = oldBinCodeReserved;

#ifdef _M_X64
			block.unwindTable = functionWin64UnwindTable.data;

			functionWin64UnwindTable.data = NULL;
			functionWin64UnwindTable.count = 0;
			functionWin64UnwindTable.max = 0;
#endif

			expiredCodeBlocks.push_back(block);
		}
		else
		{
			NULLC::dealloc(binCode);
		}

		for(unsigned i = 0; i < instAddress.size(); i++)
			instAddress[i] = (instAddress[i] - binCode) + binCodeNew;

		for(unsigned i = 0; i < functionAddress.size(); i++)
		{
			if(functionAddress[i])
				functionAddress[i] = uintptr_t(functionAddress[i] - binCode) + binCodeNew;
		}

		for(unsigned i = 0; i < expiredFunctionAddressLists.size(); i++)
		{
			ExpiredFunctionAddressList &info = expiredFunctionAddressLists[i];

			for(unsigned k = 0; k < info.count; k++)
			{
				if(info.data[k])
					info.data[k] = uintptr_t(info.data[k] - binCode) + binCodeNew;
			}
		}

		binCode = binCodeNew;
	}

	// Translate to x86
	unsigned char *code = binCode + binCodeSize;

	// Linking in new code, destroy final global return code sequence
	if(binCodeSize != 0)
	{
#if defined(_M_X64)
		code -= 10; // xor eax, eax; add rsp, 40; pop r15; pop rbx; ret;
#else
		code -= 8; // xor eax, eax; mov esp, ebp; pop esi; pop ebx; pop ebp; ret;
#endif
	}

	instAddress.resize(exRegVmCode.size() + 1); // Extra instruction for global return
	NULLC::fillMemory(instAddress.data + lastInstructionCount, 0, (exRegVmCode.size() - lastInstructionCount + 1) * sizeof(instAddress[0]));

	vmState.instAddress = instAddress.data;

	x86ClearLabels();
	x86ReserveLabels(codeGenCtx->labelCount);

	code = x86TranslateInstructionList(code, binCode + binCodeReserved, instList.data, instList.size(), instAddress.data);

	assert(binCodeSize < binCodeReserved);

	binCodeSize = unsigned(code - binCode);

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
	assert(sizeof(UNWIND_INFO_FUNCTION) == 4 + 4 * 2);

	UNWIND_INFO_FUNCTION unwindInfo = { 0 };

	unwindInfo.version = 1;
	unwindInfo.flags = 0; // No EH
	unwindInfo.sizeOfProlog = 7;
	unwindInfo.countOfCodes = 3;
	unwindInfo.frameRegister = 0;
	unwindInfo.frameOffset = 0;

	unwindInfo.unwindCode[0].offsetInPrologue = 7;
	unwindInfo.unwindCode[0].operationCode = UWOP_ALLOC_SMALL;
	unwindInfo.unwindCode[0].operationInfo = (40 - 8) / 8;

	unwindInfo.unwindCode[1].offsetInPrologue = 3;
	unwindInfo.unwindCode[1].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[1].operationInfo = 15; // r15

	unwindInfo.unwindCode[2].offsetInPrologue = 1;
	unwindInfo.unwindCode[2].operationCode = UWOP_PUSH_NONVOL;
	unwindInfo.unwindCode[2].operationInfo = UWOP_REGISTER_RBX;

	unsigned char *unwindPos = code;

	NULLC::copyMemory(code, &unwindInfo, sizeof(unwindInfo));
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
		unsigned char *codeEnd = instAddress[globalCodeRanges[i + 1]] + unwindInfo.sizeOfProlog; // Add prologue

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

	assert(unsigned(code - binCode) < binCodeReserved);

	x86SatisfyJumps(instAddress);

	for(unsigned int i = (codeRelocated ? 0 : oldFunctionSize); i < exFunctions.size(); i++)
	{
		if(exFunctions[i].regVmAddress != -1)
			functionAddress[i] = instAddress[exFunctions[i].regVmAddress];
		else
			functionAddress[i] = 0;
	}

	lastInstructionCount = exRegVmCode.size();

	oldJumpTargetCount = exLinker->regVmJumpTargets.size();
	oldRegKillInfoCount = exRegVmRegKillInfo.size();
	oldFunctionSize = exFunctions.size();

	return true;
}

void ExecutorX86::UpdateFunctionPointer(unsigned source, unsigned target)
{
	functionAddress[source] = functionAddress[target];

	for(unsigned i = 0; i < expiredFunctionAddressLists.size(); i++)
	{
		ExpiredFunctionAddressList &info = expiredFunctionAddressLists[i];

		if(source < info.count)
			info.data[source] = functionAddress[target];
	}
}

void ExecutorX86::SaveListing(OutputContext &output)
{
	TRACE_SCOPE("x86", "SaveListing");

	const unsigned instBufSize = 128;
	char instBuf[instBufSize];

	for(unsigned i = 0; i < instList.size(); i++)
	{
		x86Instruction &inst = instList[i];

		unsigned instID = inst.instID;

		if(instID && codeJumpTargets[instID - 1])
		{
			const char *functionName = NULL;
			unsigned functionId = 0;

			for(unsigned k = 0, e = exLinker->exFunctions.size(); k != e; k++)
			{
				ExternFuncInfo &funcInfo = exLinker->exFunctions[k];

				if(unsigned(funcInfo.regVmAddress) == instID - 1)
				{
					functionName = funcInfo.offsetToName + exLinker->exSymbols.data;
					functionId = k;
					break;
				}
			}

			output.Print("; ------------------- Invalidation ----------------\n");

			if(functionName)
				output.Printf("0x%x: ; %4d // %s#%d", 0xc0000000 | (instID - 1), instID - 1, functionName, functionId);
			else
				output.Printf("0x%x: ; %4d", 0xc0000000 | (instID - 1), instID - 1);

			output.Print('\n');
		}

		if(instID && instID - 1 < exRegVmCode.size())
		{
			RegVmCmd &cmd = exRegVmCode[instID - 1];

			output.Printf("; %4d: ", instID - 1);

			PrintInstruction(output, (char*)exRegVmConstants.data, exFunctions.data, exLinker->exSymbols.data, RegVmInstructionCode(cmd.code), cmd.rA, cmd.rB, cmd.rC, cmd.argument, NULL);

			unsigned regKillInfoOffset = codeRegKillInfoOffsets[instID - 1];
			unsigned regKillCounts = exRegVmRegKillInfo[regKillInfoOffset];

			if(regKillCounts)
			{
				output.Print(" // kill");

				regKillInfoOffset++;

				unsigned preKillCount = (regKillCounts >> 4);
				unsigned postKillCount = (regKillCounts & 0xf);

				if(preKillCount)
				{
					output.Print(" early[");

					for(unsigned k = 0; k < preKillCount; k++)
						output.Printf("%sr%d", k == 0 ? "" : ", ", exRegVmRegKillInfo[regKillInfoOffset++]);

					output.Print(']');
				}

				if(postKillCount)
				{
					output.Print(" late[");

					for(unsigned k = 0; k < postKillCount; k++)
						output.Printf("%sr%d", k == 0 ? "" : ", ", exRegVmRegKillInfo[regKillInfoOffset++]);

					output.Print(']');
				}
			}

			output.Print('\n');
		}

		inst.Decode(vmState, instBuf, instBufSize);

		output.Print(instBuf);
		output.Print('\n');
	}

	output.Flush();
}

unsigned ExecutorX86::GetResultType()
{
	return vmState.tempStackType;
}

NULLCRef ExecutorX86::GetResultObject()
{
	return GetExecutorResultObject(vmState.tempStackType, vmState.tempStackArrayBase);
}

const char* ExecutorX86::GetResult()
{
	return GetExecutorResult(execResult, execResultSize, vmState.tempStackType, vmState.tempStackArrayBase, exLinker->exSymbols.data, exTypes.data);
}

int ExecutorX86::GetResultInt()
{
	return GetExecutorResultInt(vmState.tempStackType, vmState.tempStackArrayBase);
}

double ExecutorX86::GetResultDouble()
{
	return GetExecutorResultDouble(vmState.tempStackType, vmState.tempStackArrayBase);
}

long long ExecutorX86::GetResultLong()
{
	return GetExecutorResultLong(vmState.tempStackType, vmState.tempStackArrayBase);
}

const char* ExecutorX86::GetErrorMessage()
{
	return execErrorMessage;
}

NULLCRef ExecutorX86::GetErrorObject()
{
	return execErrorObject;
}

char* ExecutorX86::GetVariableData(unsigned int *count)
{
	if(count)
		*count = unsigned(vmState.dataStackTop - vmState.dataStackBase);

	return vmState.dataStackBase;
}

unsigned int ExecutorX86::GetCallStackAddress(unsigned frame)
{
	return frame >= unsigned(vmState.callStackTop - vmState.callStackBase) ? 0 : vmState.callStackBase[frame].instruction;
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
		NULLC::SafeSprintf(execErrorBuffer, NULLC_ERROR_BUFFER_SIZE, "ERROR: break position out of code range");
		execErrorMessage = execErrorBuffer;
		return false;
	}

	while(instruction < instAddress.size() && !instAddress[instruction])
		instruction++;

	if(instruction >= instAddress.size())
	{
		NULLC::SafeSprintf(execErrorBuffer, NULLC_ERROR_BUFFER_SIZE, "ERROR: break position out of code range");
		execErrorMessage = execErrorBuffer;
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
		NULLC::SafeSprintf(execErrorBuffer, NULLC_ERROR_BUFFER_SIZE, "ERROR: break position out of code range");
		execErrorMessage = execErrorBuffer;
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
		NULLC::SafeSprintf(execErrorBuffer, NULLC_ERROR_BUFFER_SIZE, "ERROR: there is no breakpoint at instruction %d", instruction);
		execErrorMessage = execErrorBuffer;
		return false;
	}

	*instAddress[breakInstructions[index].instIndex] = breakInstructions[index].oldOpcode;
	return true;
}

unsigned ExecutorX86::GetInstructionAtAddress(void *address)
{
	bool isInternal = uintptr_t(address) >= uintptr_t(binCode) && uintptr_t(address) <= uintptr_t(binCode + binCodeSize);

	if(!isInternal)
		return ~0u;

	return NULLC::GetInstructionFromAddress(uintptr_t(address));
}

bool ExecutorX86::IsCodeLaunchHeader(void *address)
{
	return uintptr_t(address) >= uintptr_t(codeLaunchHeader) && uintptr_t(address) <= uintptr_t(codeLaunchHeader + codeLaunchHeaderLength);
}

#endif
