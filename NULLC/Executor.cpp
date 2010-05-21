#include "stdafx.h"
#include "Executor.h"

#include "StdLib.h"

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	#define DBG(x) x
#else
	#define DBG(x)
#endif

long long vmLongPow(long long num, long long pow)
{
	if(pow < 0)
		return (num == 1 ? 1 : 0);
	if(pow == 0)
		return 1;
	if(pow == 1)
		return num;
	if(pow > 64)
		return num;
	long long res = 1;
	int power = (int)pow;
	while(power)
	{
		if(power & 0x01)
		{
			res *= num;
			power--;
		}
		num *= num;
		power >>= 1;
	}
	return res;
}

#if defined(_M_X64)
#ifdef _WIN64
	#include <Windows.h>
	#define NULLC_X64_IREGARGS 4
	#define NULLC_X64_FREGARGS 4
#else
	#include <sys/mman.h>
	#include <limits.h>
	#ifndef PAGESIZE
		#define PAGESIZE 4096
	#endif
	#define NULLC_X64_IREGARGS 6
	#define NULLC_X64_FREGARGS 8
#endif

static const unsigned char gatePrologue[32] =
{
#ifdef _WIN64
	0x4C, 0x89, 0x44, 0x24, 0x18,	// mov qword [rsp+18h], r8	// spill r8
	0x48, 0x89, 0x54, 0x24, 0x10,	// mov qword [rsp+10h], rdx	// spill RDX
	0x48, 0x89, 0x4C, 0x24, 0x08,	// mov qword [rsp+8h], rcx	// spill RCX
#endif
	0x57,	// push RDI
	0x50,	// push RAX
	0x53,	// push RBX
	0x56,	// push RSI
	0x56,	// push RSI again for stack alignment
#ifdef _WIN64
	0x48, 0x8b, 0301,// mov RAX, RCX
	0x48, 0x8b, 0332,// mov RBX, RDX
	0x4d, 0x8b, 0330,// mov R11, R8
#else
	0x48, 0x8b, 0307,// mov RAX, RDI
	0x48, 0x8b, 0336,// mov RBX, RSI
	0x4c, 0x8b, 0332,// mov R11, RDX
#endif
};

unsigned int Executor::CreateFunctionGateway(FastVector<unsigned char>& code, unsigned int funcID)
{
	unsigned int dwordsToPop = (exFunctions[funcID].bytesToPop >> 2);
	unsigned int tmp;

	/*printf("Creating gateway for function %s(", exLinker->exSymbols.data + exFunctions[funcID].offsetToName);
	// Check every function local
	for(unsigned int i = 0; i < exFunctions[funcID].localCount; i++)
	{
		// Get information about local
		ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + i];
		printf("%s, ", exLinker->exSymbols.data + exTypes[lInfo.type].offsetToName);
	}
	printf(") arg count %d\n", exFunctions[funcID].paramCount + (exFunctions[funcID].isNormal ? 0 : 1));*/

	// Create function call code, clear it out, copy call prologue to the beginning
#ifdef _WIN64
	code.push_back(gatePrologue, 29);
#else
	code.push_back(gatePrologue, 14);
#endif

	// complex type return is handled by passing pointer to a place, where the return value will be placed. This uses first register.
	unsigned int rvs = exFunctions[funcID].retType == ExternFuncInfo::RETURN_UNKNOWN ? 1 : 0;

#ifndef _WIN
	// in AMD64, int and FP registers are independant, so we have to count, how much arguments to save in registers
	unsigned int usedIRegs = rvs, usedFRegs = 0;
	// This to variables will help later to know we've reached the boundary of arguments that are placed in registers
	unsigned int argsToIReg = 0, argsToFReg = 0;
	for(unsigned int k = 0; k < exFunctions[funcID].paramCount + (exFunctions[funcID].isNormal ? 0 : 1); k++)
	{
		ExternTypeInfo::TypeCategory typeCat = ExternTypeInfo::TYPE_LONG;
		unsigned int typeSize = 8;
		if((unsigned int)k != exFunctions[funcID].paramCount)
		{
			ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + k];
			ExternTypeInfo &lType = exLinker->exTypes[lInfo.type];
			typeCat = lType.type;
			typeSize = lType.size;
			// Aggregate types are passed in registers
			if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize != 0 && typeSize <= 4)
				typeCat = ExternTypeInfo::TYPE_INT;
			if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize == 8)
				typeCat = ExternTypeInfo::TYPE_LONG;
		}
		switch(typeCat)
		{
		case ExternTypeInfo::TYPE_FLOAT:
		case ExternTypeInfo::TYPE_DOUBLE:
			if(usedIRegs < NULLC_X64_FREGARGS)
			{
				usedFRegs++;
				argsToFReg = k;
			}
			break;
		case ExternTypeInfo::TYPE_CHAR:
		case ExternTypeInfo::TYPE_SHORT:
		case ExternTypeInfo::TYPE_INT:
		case ExternTypeInfo::TYPE_LONG:
			if(usedIRegs < NULLC_X64_IREGARGS)
			{
				usedIRegs++;
				argsToIReg = k;
			}
			break;
		case ExternTypeInfo::TYPE_COMPLEX:
			if(typeSize != 0 && typeSize <= 8)
				assert(!"parameter type unsupported (small aggregate)");
			
			break;
		default:
			assert(!"parameter type unsupported");
		}
	}
	//printf("Used Iregs: %d Fregs: %d\n", usedIRegs, usedFRegs);
	//printf("Arguments to Ireg limit: %d Freg limit: %d\n", argsToIReg, argsToFReg);
#endif
	
	// normal function doesn't accept context, so start of the stack skips those 2 dwords
	unsigned int currentShift = (dwordsToPop - (exFunctions[funcID].isNormal ? 2 : 0)) * 4;
	unsigned int needpop = 0;
	int i = exFunctions[funcID].paramCount - (exFunctions[funcID].isNormal ? 1 : 0);
	for(; i >= 0; i--)
	{
		// By default, suppose we have last hidden argument, that is a pointer, represented as long number of size 8
		ExternTypeInfo::TypeCategory typeCat = ExternTypeInfo::TYPE_LONG;
		unsigned int typeSize = 8;
		// If this is not the last argument, update data above
		if((unsigned int)i != exFunctions[funcID].paramCount)
		{
			ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + i];
			ExternTypeInfo &lType = exLinker->exTypes[lInfo.type];
			typeCat = lType.type;
			typeSize = lType.size;
			// Aggregate types are passed in registers
			if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize != 0 && typeSize <= 4)
				typeCat = ExternTypeInfo::TYPE_INT;
			if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize == 8)
				typeCat = ExternTypeInfo::TYPE_LONG;
		}
		switch(typeCat)
		{
		case ExternTypeInfo::TYPE_FLOAT:
		case ExternTypeInfo::TYPE_DOUBLE:
#ifdef _WIN64
			if(rvs + i > NULLC_X64_FREGARGS - 1)
#else
			if((unsigned)i > argsToFReg)
#endif
			{
				// mov rsi, [rax+shift]
				if(typeCat == ExternTypeInfo::TYPE_DOUBLE)
					code.push_back(0x48);	// 64bit mode
				code.push_back(0x8b);
				code.push_back(0264);	// modR/M mod = 10 (32-bit shift), spare = 6 (RSI is destination), r/m = 4 (SIB active)
				code.push_back(0040);	// sib	scale = 0 (1), index = 4 (NONE), base = 0 (EAX)
				tmp = currentShift - (typeCat == ExternTypeInfo::TYPE_DOUBLE ? 8 : 4);
				code.push_back((unsigned char*)&tmp, 4);

				// push rsi
				code.push_back(0x56);
				needpop += 8;
			}else{
				// movsd/movss xmm, [rax+shift]
				code.push_back(typeCat == ExternTypeInfo::TYPE_DOUBLE ? 0xf2 : 0xf3);
				code.push_back(0x0f);
				code.push_back(0x10);
#ifdef _WIN64
				code.push_back((unsigned char)(0200 | (i << 3)));	// modR/M mod = 10 (32-bit shift), spare = XMMn, r/m = 0 (RAX is base)
#else
				assert(usedFRegs - 1 < NULLC_X64_FREGARGS);
				code.push_back((unsigned char)(0200 | ((usedFRegs - 1) << 3)));
				usedFRegs--;
#endif
				tmp = currentShift - (typeCat == ExternTypeInfo::TYPE_DOUBLE ? 8 : 4);
				code.push_back((unsigned char*)&tmp, 4);
			}
			currentShift -= (typeCat == ExternTypeInfo::TYPE_DOUBLE ? 8 : 4);
			break;
		case ExternTypeInfo::TYPE_CHAR:
		case ExternTypeInfo::TYPE_SHORT:
		case ExternTypeInfo::TYPE_INT:
		case ExternTypeInfo::TYPE_LONG:
#ifdef _WIN64
			if(rvs + i > NULLC_X64_IREGARGS - 1)
#else
			if((unsigned)i > argsToIReg)
#endif
			{
				// mov rsi, [rax+shift]
				if(typeCat == ExternTypeInfo::TYPE_LONG)
					code.push_back(0x48);	// 64bit mode
				code.push_back(0x8b);
				code.push_back(0264);	// modR/M mod = 10 (32-bit shift), spare = 6 (RSI is destination), r/m = 4 (SIB active)
				code.push_back(0040);	// sib	scale = 0 (1), index = 4 (NONE), base = 0 (EAX)
				tmp = currentShift - (typeCat == ExternTypeInfo::TYPE_LONG ? 8 : 4);
				code.push_back((unsigned char*)&tmp, 4);

				// push rsi
				code.push_back(0x56);
				needpop += 8;
			}else{
				unsigned int regID = rvs + i;
#ifndef _WIN64
				assert(usedIRegs - 1 < NULLC_X64_IREGARGS);
				regID = (usedIRegs - 1);
				usedIRegs--;
#endif
				if(regID == (NULLC_X64_IREGARGS - 2) || regID == (NULLC_X64_IREGARGS - 1))
					code.push_back(typeCat == ExternTypeInfo::TYPE_LONG ? 0x4c : 0x44);
				else if(typeCat == ExternTypeInfo::TYPE_LONG)
					code.push_back(0x48);	// 64bit mode
				code.push_back(0x8b);
#ifdef _WIN64
				unsigned regCodes[] = { 0214, 0224, 0204, 0214 }; // rcx, rdx, r8, r9
#else
				unsigned regCodes[] = { 0274, 0264, 0224, 0214, 0204, 0214 }; // rdi, rsi, rdx, rcx, r8, r9
#endif
				code.push_back((unsigned char)regCodes[regID]);
				code.push_back(0040);
				tmp = currentShift - (typeCat == ExternTypeInfo::TYPE_LONG ? 8 : 4);
				code.push_back((unsigned char*)&tmp, 4);
			}
			currentShift -= (typeCat == ExternTypeInfo::TYPE_LONG ? 8 : 4);
			break;
		case ExternTypeInfo::TYPE_COMPLEX:
			if(typeSize != 0 && typeSize <= 8)
				assert(!"parameter type unsupported (small aggregate)");
			// lea r12, [rax+shift]
			code.push_back(0x4c);	// 64bit mode
			code.push_back(0x8d);
			code.push_back(0244);	// modR/M mod = 10 (32-bit shift), spare = 4 (R12 destination), r/m = 4 (SIB active)
			code.push_back(0040);	// sib	scale = 0 (1), index = 4 (NONE), base = 0 (EAX)
			
			tmp = currentShift - typeSize;
			code.push_back((unsigned char*)&tmp, 4);

			if(rvs + i > NULLC_X64_IREGARGS - 1)
			{
				// push r12
				code.push_back(0x4c);	// 64bit mode
				code.push_back(0x54);
				needpop += 8;
			}else{
				// mov reg, r12
				if(rvs + i == (NULLC_X64_IREGARGS - 2) || rvs + i == (NULLC_X64_IREGARGS - 1))
					code.push_back(0x4d);	// 64bit mode
				else
					code.push_back(0x49);	// 64bit mode
				code.push_back(0x8b);
#ifdef _WIN64
				unsigned regCodes[] = { 0314, 0324, 0304, 0314 }; // rcx, rdx, r8, r9
#else
				unsigned regCodes[] = { 0374, 0364, 0324, 0314, 0304, 0314 }; // rdi, rsi, rdx, rcx, r8, r9
#endif
				code.push_back((unsigned char)regCodes[rvs + i]);
			}
			currentShift -= typeSize;
			break;
		default:
			assert(!"parameter type unsupported");
		}
	}
	// for complex return value, pass a pointer in rcx
	if(rvs)
	{
		code.push_back(0x48);	// 64bit mode
		code.push_back(0x8b);
		code.push_back(0313);	// modR/M mod = 11 (register), spare = 1 (RCX is a destination), r/m = 3 (RBX is source)
	}
#ifdef _WIN64
	// sub rsp, 32
	code.push_back(0x48);
	code.push_back(0x81);
	code.push_back(0354);	// modR/M mod = 11 (register), spare = 5 (sub), r/m = 4 (RSP is changed)
	tmp = 32;
	code.push_back((unsigned char*)&tmp, 4);
#endif

	// call r11
	code.push_back(0x49);
	code.push_back(0xff);
	code.push_back(0323);	// modR/M mod = 11 (register), spare = 2, r/m = 3 (R11 is source)

	// add rsp, 32 + needpop
	code.push_back(0x48);
	code.push_back(0x81);
	code.push_back(0304);	// modR/M mod = 11 (register), spare = 0 (add), r/m = 4 (RSP is changed)
#ifdef _WIN64
	tmp = 32 + needpop;
#else
	tmp = needpop;
#endif
	code.push_back((unsigned char*)&tmp, 4);

	// handle return value
	ExternFuncInfo::ReturnType retType = (ExternFuncInfo::ReturnType)exFunctions[funcID].retType;
	unsigned int retTypeID = exLinker->exTypeExtra[exTypes[exFunctions[funcID].funcType].memberOffset];
	unsigned int retTypeSize = exTypes[retTypeID].size;
	if(retType == ExternFuncInfo::RETURN_UNKNOWN)
	{
		if(retTypeSize == 0)
			retType = ExternFuncInfo::RETURN_VOID;
		else if(retTypeSize <= 4)
			retType = ExternFuncInfo::RETURN_INT;
		else if(retTypeSize == 8)
			retType = ExternFuncInfo::RETURN_LONG;
	}
	unsigned int returnShift = 0;
	switch(retType)
	{
	case ExternFuncInfo::RETURN_DOUBLE:
		
		// float type is #2
		if(retTypeID == 2)
		{
			// cvtss2sd xmm0, xmm0
			code.push_back(0xf3);
			code.push_back(0x0f);
			code.push_back(0x5a);
			code.push_back(0xc0);
		}
		returnShift = 2;
		// movsd qword [rbx], xmm0
		code.push_back(0xf2);
		code.push_back(0x0f);
		code.push_back(0x11);
		code.push_back(0003);	// modR/M mod = 00 (no shift), spare = XMM0, r/m = 3 (RBX is base)
		break;
	case ExternFuncInfo::RETURN_LONG:
		returnShift = 1;
		// mov qword [rbx], rax
		code.push_back(0x48);	// 64bit mode
	case ExternFuncInfo::RETURN_INT:
		returnShift += 1;
		// mov dword [rbx], eax
		code.push_back(0x89);
		code.push_back(0003);	// modR/M mod = 00 (no shift), spare = 0 (RAX is source), r/m = 3 (RBX is base)
		break;
	case ExternFuncInfo::RETURN_VOID:
		break;
	case ExternFuncInfo::RETURN_UNKNOWN:
		if(retTypeSize <= 8)
			assert(!"return type unsupported (small aggregate)");
		returnShift = retTypeSize / 4;
		break;
	}

	code.push_back(0x5e);	// pop RSI
	code.push_back(0x5e);	// pop RSI (pushed twice for alignment)
	code.push_back(0x5b);	// pop RBX
	code.push_back(0x58);	// pop RAX
	code.push_back(0x5f);	// pop RDI
	code.push_back(0xc3);	// ret

	return returnShift;
}

#endif

Executor::Executor(Linker* linker): exLinker(linker), exFunctions(linker->exFunctions), exTypes(linker->exTypes), breakCode(128), gateCode(4096)
{
	DBG(executeLog = fopen("log.txt", "wb"));

	genStackBase = NULL;
	genStackPtr = NULL;
	genStackTop = NULL;

	callContinue = true;

	paramBase = 0;
	currentFrame = 0;
	cmdBase = NULL;

	symbols = NULL;

	codeRunning = false;

	breakFunction = NULL;
}

Executor::~Executor()
{
	DBG(fclose(executeLog));

	NULLC::dealloc(genStackBase);
}

#define genStackSize (genStackTop-genStackPtr)
#define RUNTIME_ERROR(test, desc)	if(test){ fcallStack.push_back(cmdStream); cmdStream = NULL; strcpy(execError, desc); break; }

void Executor::InitExecution()
{
	if(!exLinker->exCode.size())
	{
		strcpy(execError, "ERROR: no code to run");
		return;
	}
	fcallStack.clear();

	CommonSetLinker(exLinker);

	genParams.reserve(4096);
	genParams.clear();
	genParams.resize((exLinker->globalVarSize + 0xf) & ~0xf);

	SetUnmanagableRange(genParams.data, genParams.max);

	execError[0] = 0;
	callContinue = true;

	// Add return after the last instruction to end execution of code with no return at the end
	exLinker->exCode[exLinker->exCode.size()] = VMCmd(cmdReturn, bitRetError, 0, 1);

	// General stack
	if(!genStackBase)
	{
		genStackBase = (unsigned int*)NULLC::alloc(sizeof(unsigned int) * 8192 * 2);	// Should be enough, but can grow
		genStackTop = genStackBase + 8192 * 2;
	}
	genStackPtr = genStackTop - 1;

	paramBase = 0;

#if defined(_M_X64)
	// Generate gateway code for all functions
	gateCode.clear();
	for(unsigned int i = 0; i < exFunctions.size(); i++)
	{
		exFunctions[i].startInByteCode = gateCode.size();
		exFunctions[i].returnShift = (unsigned short)CreateFunctionGateway(gateCode, i);
	}
#ifdef _WIN64
	DWORD old;
	VirtualProtect(gateCode.data, gateCode.size(), PAGE_EXECUTE_READWRITE, &old);
#else
	char *p = (char*)((intptr_t)((char*)gateCode.data + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
	if(mprotect(p, ((gateCode.size() + PAGESIZE - 1) & ~(PAGESIZE - 1)) + PAGESIZE, PROT_READ | PROT_WRITE | PROT_EXEC))
		asm("int $0x3");
#endif

#endif
}

void Executor::Run(unsigned int functionID, const char *arguments)
{
	if(!codeRunning || functionID == ~0u)
		InitExecution();
	codeRunning = true;

	asmOperType retType = (asmOperType)-1;

	VMCmd *cmdStreamBase = cmdBase = &exLinker->exCode[0];
	VMCmd *cmdStream = &exLinker->exCode[exLinker->offsetToGlobalCode];
#define cmdStreamPos (cmdStream-cmdStreamBase)

	// By default error is flagged, normal return will clear it
	bool	errorState = true;
	// We will know that return is global if call stack size is equal to current
	unsigned int	finalReturn = fcallStack.size();

	if(functionID != ~0u)
	{
		unsigned int funcPos = ~0u;
		funcPos = exFunctions[functionID].address;
		if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_VOID)
		{
			retType = OTYPE_COMPLEX;
		}else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_INT){
			retType = OTYPE_INT;
		}else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_DOUBLE){
			retType = OTYPE_DOUBLE;
		}else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_LONG){
			retType = OTYPE_LONG;
		}
		if(funcPos == ~0u)
		{
			// Copy all arguments
			memcpy(genStackPtr - (exFunctions[functionID].bytesToPop >> 2), arguments, exFunctions[functionID].bytesToPop);
			genStackPtr -= (exFunctions[functionID].bytesToPop >> 2);
			// Call function
			if(RunExternalFunction(functionID, 0))
				errorState = false;
			// This will disable NULLC code execution while leaving error check and result retrieval
			cmdStream = NULL;
		}else{
			cmdStream = &exLinker->exCode[funcPos];

			// Copy from argument buffer to next stack frame
			char* oldBase = &genParams[0];
			unsigned int oldSize = genParams.max;

			unsigned int paramSize = exFunctions[functionID].bytesToPop;
			unsigned int alignOffset = (genParams.size() % 16 != 0) ? (16 - (genParams.size() % 16)) : 0;
			genParams.reserve(genParams.size() + alignOffset + paramSize);
			memcpy((char*)&genParams[genParams.size() + alignOffset], arguments, paramSize);

			// Ensure that stack is resized, if needed
			if(genParams.size() + alignOffset + paramSize >= oldSize)
				ExtendParameterStack(oldBase, oldSize, cmdStream);
		}
	}else{
		// If global code is executed, reset all global variables
		memset(&genParams[0], 0, exLinker->globalVarSize);
	}

#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
	unsigned int insCallCount[255];
	memset(insCallCount, 0, 255*4);
	unsigned int insExecuted = 0;
#endif

	while(cmdStream)
	{
		const VMCmd &cmd = *cmdStream;
		DBG(PrintInstructionText(executeLog, cmd, paramBase, genParams.size()));
		cmdStream++;

#ifdef NULLC_VM_DEBUG
		if(genStackSize < 0)
		{
			assert(!"stack underflow");
			break;
		}
#endif
		#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
			insCallCount[cmd.cmd]++;
			insExecuted++;
		#endif

		switch(cmd.cmd)
		{
		case cmdNop:
			if(cmd.flag == EXEC_BREAK_SIGNAL || cmd.flag == EXEC_BREAK_ONE_HIT_WONDER)
			{
				RUNTIME_ERROR(breakFunction == NULL, "ERROR: break function isn't set");
				unsigned int target = cmd.argument;
				fcallStack.push_back(cmdStream);
				RUNTIME_ERROR(cmdStream < cmdStreamBase || cmdStream > &exLinker->exCode[exLinker->exCode.size() + 1], "ERROR: break position is out of range");
				unsigned int response = breakFunction((unsigned int)(cmdStream - cmdStreamBase));
				fcallStack.pop_back();

				RUNTIME_ERROR(response == 4, "ERROR: execution was stopped after breakpoint");

				// Step command - set breakpoint on the next instruction, if there is no breakpoint already
				if(response)
				{
					// Next instruction for step command
					VMCmd *nextCommand = cmdStream;
					// Step command - handle unconditional jump step
					if(breakCode[target].cmd == cmdJmp)
						nextCommand = cmdStreamBase + breakCode[target].argument;
					// Step command - handle conditional "jump on false" step
					if(breakCode[target].cmd == cmdJmpZ && *genStackPtr == 0)
						nextCommand = cmdStreamBase + breakCode[target].argument;
					// Step command - handle conditional "jump on true" step
					if(breakCode[target].cmd == cmdJmpNZ && *genStackPtr != 0)
						nextCommand = cmdStreamBase + breakCode[target].argument;
					if(response == 2 && breakCode[target].cmd == cmdCall && exFunctions[breakCode[target].argument].address != -1)
						nextCommand = cmdStreamBase + exFunctions[breakCode[target].argument].address;
					if(response == 2 && breakCode[target].cmd == cmdCallPtr && genStackPtr[cmd.argument >> 2] && exFunctions[genStackPtr[cmd.argument >> 2]].address != -1)
						nextCommand = cmdStreamBase + exFunctions[genStackPtr[cmd.argument >> 2]].address;

					if(response == 3 && fcallStack.size() != finalReturn)
						nextCommand = fcallStack.back();

					if(nextCommand->cmd != cmdNop)
					{
						unsigned int pos = breakCode.size();
						breakCode.push_back(*nextCommand);
						nextCommand->cmd = cmdNop;
						nextCommand->flag = EXEC_BREAK_ONE_HIT_WONDER;
						nextCommand->argument = pos;
					}
				}
				// This flag means that breakpoint works only once
				if(cmd.flag == EXEC_BREAK_ONE_HIT_WONDER)
				{
					cmdStream[-1] = breakCode[target];
					cmdStream--;
					break;
				}
				// Jump to external code
				cmdStream = &breakCode[target];
				break;
			}
			cmdStream = cmdStreamBase + cmd.argument;
			break;
		case cmdPushChar:
			genStackPtr--;
			*genStackPtr = genParams[cmd.argument + (paramBase * cmd.flag)];
			break;
		case cmdPushShort:
			genStackPtr--;
			*genStackPtr = *((short*)(&genParams[cmd.argument + (paramBase * cmd.flag)]));
			break;
		case cmdPushInt:
			genStackPtr--;
			*genStackPtr = *((int*)(&genParams[cmd.argument + (paramBase * cmd.flag)]));
			break;
		case cmdPushFloat:
			genStackPtr -= 2;
			*(double*)(genStackPtr) = (double)*((float*)(&genParams[cmd.argument + (paramBase * cmd.flag)]));
			break;
		case cmdPushDorL:
			genStackPtr -= 2;
			*(double*)(genStackPtr) = *((double*)(&genParams[cmd.argument + (paramBase * cmd.flag)]));
			break;
		case cmdPushCmplx:
		{
			int valind = cmd.argument + (paramBase * cmd.flag);
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				genStackPtr--;
				*genStackPtr = *((unsigned int*)(&genParams[valind + currShift]));
			}
		}
			break;

		case cmdPushCharStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*genStackPtr = *(char*)(cmd.argument + *(char**)(genStackPtr-1));
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			*genStackPtr = *((char*)NULL + cmd.argument + *genStackPtr);
#endif
			break;
		case cmdPushShortStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*genStackPtr = *(short*)(cmd.argument + *(char**)(genStackPtr-1));
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			*genStackPtr = *(short*)((char*)NULL + cmd.argument + *genStackPtr);
#endif
			break;
		case cmdPushIntStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*genStackPtr = *(int*)(cmd.argument + *(char**)(genStackPtr-1));
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			*genStackPtr = *(int*)((char*)NULL + cmd.argument + *genStackPtr);
#endif
			break;
		case cmdPushFloatStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			*(double*)(genStackPtr) = (double)*(float*)(cmd.argument + *(char**)(genStackPtr));
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr--;
			*(double*)(genStackPtr) = (double)*(float*)((char*)NULL + cmd.argument + *(genStackPtr+1));
#endif
			break;
		case cmdPushDorLStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			*(long long*)(genStackPtr) = *(long long*)(cmd.argument + *(char**)(genStackPtr));
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr--;
			*(double*)(genStackPtr) = *(double*)((char*)NULL + cmd.argument + *(genStackPtr+1));
#endif
			break;
		case cmdPushCmplxStk:
		{
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			char *start = cmd.argument + *(char**)genStackPtr;
			genStackPtr += 2;
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			char *start = (char*)NULL + cmd.argument + *genStackPtr;
			genStackPtr++;
#endif
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				genStackPtr--;
				*genStackPtr = *(unsigned int*)(start + currShift);
			}
		}
			break;

		case cmdPushImmt:
			genStackPtr--;
			*genStackPtr = cmd.argument;
			break;

		case cmdMovChar:
			genParams[cmd.argument + (paramBase * cmd.flag)] = (unsigned char)(*genStackPtr);
			break;
		case cmdMovShort:
			*((unsigned short*)(&genParams[cmd.argument + (paramBase * cmd.flag)])) = (unsigned short)(*genStackPtr);
			break;
		case cmdMovInt:
			*((int*)(&genParams[cmd.argument + (paramBase * cmd.flag)])) = (int)(*genStackPtr);
			break;
		case cmdMovFloat:
			*((float*)(&genParams[cmd.argument + (paramBase * cmd.flag)])) = (float)*(double*)(genStackPtr);
			break;
		case cmdMovDorL:
			*((long long*)(&genParams[cmd.argument + (paramBase * cmd.flag)])) = *(long long*)(genStackPtr);
			break;
		case cmdMovCmplx:
		{
			int valind = cmd.argument + (paramBase * cmd.flag);
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				*((unsigned int*)(&genParams[valind + currShift])) = *(genStackPtr+(currShift>>2));
			}
			assert(currShift == 0);
		}
			break;

		case cmdMovCharStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr += 2;
			*(cmd.argument + *(char**)(genStackPtr-2)) = (unsigned char)(*genStackPtr);
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned char)(*genStackPtr);
#endif
			break;
		case cmdMovShortStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr += 2;
			*(unsigned short*)(cmd.argument + *(char**)(genStackPtr-2)) = (unsigned short)(*genStackPtr);
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*(unsigned short*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned short)(*genStackPtr);
#endif
			break;
		case cmdMovIntStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr += 2;
			*(int*)(cmd.argument + *(char**)(genStackPtr-2)) = (int)(*genStackPtr);
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*(int*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (int)(*genStackPtr);
#endif
			break;

		case cmdMovFloatStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr += 2;
			*(float*)(cmd.argument + *(char**)(genStackPtr-2)) = (float)*(double*)(genStackPtr);
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*(float*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (float)*(double*)(genStackPtr);
#endif
			break;
		case cmdMovDorLStk:
#ifdef _M_X64
			RUNTIME_ERROR(*(void**)genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr += 2;
			*(long long*)(cmd.argument + *(char**)(genStackPtr-2)) = *(long long*)(genStackPtr);
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*(long long*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = *(long long*)(genStackPtr);
#endif
			break;
		case cmdMovCmplxStk:
		{
#ifdef _M_X64
			RUNTIME_ERROR(*(char**)genStackPtr == 0, "ERROR: null pointer access");
			char *start = cmd.argument + *(char**)genStackPtr;
			genStackPtr += 2;
#else
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			char *start = (char*)NULL + cmd.argument + *genStackPtr;
			genStackPtr++;
#endif
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				*(unsigned int*)(start + currShift) = *(genStackPtr+(currShift>>2));
			}
			assert(currShift == 0);
		}
			break;

		case cmdPop:
			genStackPtr = (unsigned int*)((char*)(genStackPtr) + cmd.argument);
			break;

		case cmdDtoI:
			*(genStackPtr+1) = int(*(double*)(genStackPtr));
			genStackPtr++;
			break;
		case cmdDtoL:
			*(long long*)(genStackPtr) = (long long)*(double*)(genStackPtr);
			break;
		case cmdDtoF:
			*((float*)(genStackPtr+1)) = float(*(double*)(genStackPtr));
			genStackPtr++;
			break;
		case cmdItoD:
			genStackPtr--;
			*(double*)(genStackPtr) = double(*(int*)(genStackPtr+1));
			break;
		case cmdLtoD:
			*(double*)(genStackPtr) = double(*(long long*)(genStackPtr));
			break;
		case cmdItoL:
			genStackPtr--;
			*(long long*)(genStackPtr) = (long long)(*(int*)(genStackPtr+1));
			break;
		case cmdLtoI:
			genStackPtr++;
			*genStackPtr = *(genStackPtr-1);
			break;

		case cmdIndex:
			RUNTIME_ERROR(*genStackPtr >= (unsigned int)cmd.argument, "ERROR: array index out of bounds");
#ifdef _M_X64
			*(char**)(genStackPtr+1) += cmd.helper * (*genStackPtr);
#else
			*(char**)(genStackPtr+1) += cmd.helper * (*genStackPtr);
#endif
			genStackPtr++;
			break;
		case cmdIndexStk:
#ifdef _M_X64
			RUNTIME_ERROR(*genStackPtr >= *(genStackPtr+3), "ERROR: array index out of bounds");
			*(char**)(genStackPtr+2) = *(char**)(genStackPtr+1) + cmd.helper * (*genStackPtr);
			genStackPtr += 2;
#else
			RUNTIME_ERROR(*genStackPtr >= *(genStackPtr+2), "ERROR: array index out of bounds");
			*(int*)(genStackPtr+2) = *(genStackPtr+1) + cmd.helper * (*genStackPtr);
			genStackPtr += 2;
#endif
			break;

		case cmdCopyDorL:
			genStackPtr -= 2;
			*genStackPtr = *(genStackPtr+2);
			*(genStackPtr+1) = *(genStackPtr+3);
			break;
		case cmdCopyI:
			genStackPtr--;
			*genStackPtr = *(genStackPtr+1);
			break;

		case cmdGetAddr:
#ifdef _M_X64
			genStackPtr -= 2;
			*(void**)genStackPtr = cmd.argument + paramBase * cmd.helper + &genParams[0];
#else
			genStackPtr--;
			*genStackPtr = cmd.argument + paramBase * cmd.helper + (int)(intptr_t)&genParams[0];
#endif
			break;
		case cmdFuncAddr:
			break;

		case cmdSetRange:
		{
			unsigned int count = *genStackPtr;
			genStackPtr++;

			unsigned int start = cmd.argument + paramBase;

			for(unsigned int varNum = 0; varNum < count; varNum++)
			{
				switch(cmd.helper)
				{
				case DTYPE_DOUBLE:
					*((double*)(&genParams[start])) = *(double*)(genStackPtr);
					start += 8;
					break;
				case DTYPE_FLOAT:
					*((float*)(&genParams[start])) = float(*(double*)(genStackPtr));
					start += 4;
					break;
				case DTYPE_LONG:
					*((long long*)(&genParams[start])) = *(long long*)(genStackPtr);
					start += 8;
					break;
				case DTYPE_INT:
					*((int*)(&genParams[start])) = int(*genStackPtr);
					start += 4;
					break;
				case DTYPE_SHORT:
					*((short*)(&genParams[start])) = short(*genStackPtr);
					start += 2;
					break;
				case DTYPE_CHAR:
					*((char*)(&genParams[start])) = char(*genStackPtr);
					start += 1;
					break;
				}
			}
		}
			break;

		case cmdJmp:
			cmdStream = cmdStreamBase + cmd.argument;
			break;

		case cmdJmpZ:
			if(*genStackPtr == 0)
				cmdStream = cmdStreamBase + cmd.argument;
			genStackPtr++;
			break;

		case cmdJmpNZ:
			if(*genStackPtr != 0)
				cmdStream = cmdStreamBase + cmd.argument;
			genStackPtr++;
			break;

		case cmdCall:
		{
			RUNTIME_ERROR(genStackPtr <= genStackBase+8, "ERROR: stack overflow");
			unsigned int fAddress = exFunctions[cmd.argument].address;

			if(fAddress == CALL_BY_POINTER)
			{
				fcallStack.push_back(cmdStream);
				if(!RunExternalFunction(cmd.argument, 0))
					cmdStream = NULL;
				else
					fcallStack.pop_back();
			}else{
				fcallStack.push_back(cmdStream);
				cmdStream = cmdStreamBase + fAddress;

				char* oldBase = &genParams[0];
				unsigned int oldSize = genParams.max;

				unsigned int paramSize = exFunctions[cmd.argument].bytesToPop;
				assert(genParams.size() % 16 == 0);
				genParams.reserve(genParams.size() + paramSize);
				memcpy((char*)&genParams[genParams.size()], genStackPtr, paramSize);
				genStackPtr += paramSize >> 2;

				if(genParams.size() + paramSize >= oldSize)
					ExtendParameterStack(oldBase, oldSize, cmdStream);
			}
		}
			break;

		case cmdCallPtr:
		{
			unsigned int paramSize = cmd.argument;
			unsigned int fID = genStackPtr[paramSize >> 2];
			RUNTIME_ERROR(fID == 0, "ERROR: invalid function pointer");

			if(exFunctions[fID].address == -1)
			{
				fcallStack.push_back(cmdStream);
				if(!RunExternalFunction(fID, 1))
					cmdStream = NULL;
				else
					fcallStack.pop_back();
			}else{
				char* oldBase = &genParams[0];
				unsigned int oldSize = genParams.max;

				assert(genParams.size() % 16 == 0);
				genParams.reserve(genParams.size() + paramSize);
				memcpy((char*)&genParams[genParams.size()], genStackPtr, paramSize);
				genStackPtr += paramSize >> 2;
				RUNTIME_ERROR(genStackPtr <= genStackBase+8, "ERROR: stack overflow");

				genStackPtr++;

				fcallStack.push_back(cmdStream);
				cmdStream = cmdStreamBase + exFunctions[fID].address;

				if(genParams.size() + paramSize >= oldSize)
					ExtendParameterStack(oldBase, oldSize, cmdStream);
			}
		}
			break;

		case cmdReturn:
			if(cmd.flag & bitRetError)
			{
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
				codeRunning = false;
				errorState = !cmd.argument;
				if(errorState)
					strcpy(execError, "ERROR: function didn't return a value");
				break;
			}
			{
				unsigned int *retValue = genStackPtr;
				genStackPtr = (unsigned int*)((char*)(genStackPtr) + cmd.argument);

				genParams.shrink(paramBase);
				paramBase = *genStackPtr;
				genStackPtr++;

				genStackPtr = (unsigned int*)((char*)(genStackPtr) - cmd.argument);
				memmove(genStackPtr, retValue, cmd.argument);
			}
			if(fcallStack.size() == finalReturn)
			{
				if(retType == -1)
					retType = (asmOperType)(int)cmd.flag;
				cmdStream = NULL;
				errorState = false;
				if(finalReturn == 0)
					codeRunning = false;
				break;
			}
			cmdStream = fcallStack.back();
			fcallStack.pop_back();
			break;

		case cmdPushVTop:
			genStackPtr--;
			*genStackPtr = paramBase;
			paramBase = genParams.size();
			assert(paramBase % 16 == 0);
			if(paramBase + cmd.argument >= genParams.max)
			{
				char* oldBase = &genParams[0];
				unsigned int oldSize = genParams.max;
				genParams.reserve(paramBase + cmd.argument);
				ExtendParameterStack(oldBase, oldSize, cmdStream);
			}
			genParams.resize(paramBase + cmd.argument);
			memset(&genParams[paramBase + cmd.helper], 0, cmd.argument - cmd.helper);

			break;

		case cmdAdd:
			*(int*)(genStackPtr+1) += *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdSub:
			*(int*)(genStackPtr+1) -= *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdMul:
			*(int*)(genStackPtr+1) *= *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdDiv:
			if(*(int*)(genStackPtr))
			{
				*(int*)(genStackPtr+1) /= *(int*)(genStackPtr);
			}else{
				strcpy(execError, "ERROR: integer division by zero");
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr++;
			break;
		case cmdPow:
			*(int*)(genStackPtr+1) = (int)pow((double)*(int*)(genStackPtr+1), (double)*(int*)(genStackPtr));
			genStackPtr++;
			break;
		case cmdMod:
			if(*(int*)(genStackPtr))
			{
				*(int*)(genStackPtr+1) %= *(int*)(genStackPtr);
			}else{
				strcpy(execError, "ERROR: integer division by zero");
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr++;
			break;
		case cmdLess:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) < *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdGreater:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) > *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdLEqual:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) <= *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdGEqual:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) >= *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdEqual:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) == *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdNEqual:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) != *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdShl:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) << *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdShr:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) >> *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdBitAnd:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) & *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdBitOr:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) | *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdBitXor:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) ^ *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdLogAnd:
		case cmdLogOr:
			break;
		case cmdLogXor:
			*(int*)(genStackPtr+1) = !!(*(int*)(genStackPtr+1)) ^ !!(*(int*)(genStackPtr));
			genStackPtr++;
			break;

		case cmdAddL:
			*(long long*)(genStackPtr+2) += *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdSubL:
			*(long long*)(genStackPtr+2) -= *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdMulL:
			*(long long*)(genStackPtr+2) *= *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdDivL:
			if(*(long long*)(genStackPtr))
			{
				*(long long*)(genStackPtr+2) /= *(long long*)(genStackPtr);
			}else{
				strcpy(execError, "ERROR: integer division by zero");
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr += 2;
			break;
		case cmdPowL:
			*(long long*)(genStackPtr+2) = vmLongPow(*(long long*)(genStackPtr+2), *(long long*)(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdModL:
			if(*(long long*)(genStackPtr))
			{
				*(long long*)(genStackPtr+2) %= *(long long*)(genStackPtr);
			}else{
				strcpy(execError, "ERROR: integer division by zero");
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr += 2;
			break;
		case cmdLessL:
			*(int*)(genStackPtr+3) = *(long long*)(genStackPtr+2) < *(long long*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdGreaterL:
			*(int*)(genStackPtr+3) = *(long long*)(genStackPtr+2) > *(long long*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdLEqualL:
			*(int*)(genStackPtr+3) = *(long long*)(genStackPtr+2) <= *(long long*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdGEqualL:
			*(int*)(genStackPtr+3) = *(long long*)(genStackPtr+2) >= *(long long*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdEqualL:
			*(int*)(genStackPtr+3) = *(long long*)(genStackPtr+2) == *(long long*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdNEqualL:
			*(int*)(genStackPtr+3) = *(long long*)(genStackPtr+2) != *(long long*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdShlL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) << *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdShrL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) >> *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdBitAndL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) & *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdBitOrL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) | *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdBitXorL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) ^ *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdLogAndL:
		case cmdLogOrL:
			break;
		case cmdLogXorL:
			*(int*)(genStackPtr+3) = !!(*(long long*)(genStackPtr+2)) ^ !!(*(long long*)(genStackPtr));
			genStackPtr += 3;
			break;

		case cmdAddD:
			*(double*)(genStackPtr+2) += *(double*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdSubD:
			*(double*)(genStackPtr+2) -= *(double*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdMulD:
			*(double*)(genStackPtr+2) *= *(double*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdDivD:
			*(double*)(genStackPtr+2) /= *(double*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdPowD:
			*(double*)(genStackPtr+2) = pow(*(double*)(genStackPtr+2), *(double*)(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdModD:
			*(double*)(genStackPtr+2) = fmod(*(double*)(genStackPtr+2), *(double*)(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdLessD:
			*(int*)(genStackPtr+3) = *(double*)(genStackPtr+2) < *(double*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdGreaterD:
			*(int*)(genStackPtr+3) = *(double*)(genStackPtr+2) > *(double*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdLEqualD:
			*(int*)(genStackPtr+3) = *(double*)(genStackPtr+2) <= *(double*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdGEqualD:
			*(int*)(genStackPtr+3) = *(double*)(genStackPtr+2) >= *(double*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdEqualD:
			*(int*)(genStackPtr+3) = *(double*)(genStackPtr+2) == *(double*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdNEqualD:
			*(int*)(genStackPtr+3) = *(double*)(genStackPtr+2) != *(double*)(genStackPtr);
			genStackPtr += 3;
			break;

		case cmdNeg:
			*(int*)(genStackPtr) = -*(int*)(genStackPtr);
			break;
		case cmdNegL:
			*(long long*)(genStackPtr) = -*(long long*)(genStackPtr);
			break;
		case cmdNegD:
			*(double*)(genStackPtr) = -*(double*)(genStackPtr);
			break;

		case cmdBitNot:
			*(int*)(genStackPtr) = ~*(int*)(genStackPtr);
			break;
		case cmdBitNotL:
			*(long long*)(genStackPtr) = ~*(long long*)(genStackPtr);
			break;

		case cmdLogNot:
			*(int*)(genStackPtr) = !*(int*)(genStackPtr);
			break;
		case cmdLogNotL:
			*(int*)(genStackPtr+1) = !*(long long*)(genStackPtr);
			genStackPtr++;
			break;
		
		case cmdIncI:
			(*(int*)(genStackPtr))++;
			break;
		case cmdIncD:
			*(double*)(genStackPtr) += 1.0;
			break;
		case cmdIncL:
			(*(long long*)(genStackPtr))++;
			break;

		case cmdDecI:
			(*(int*)(genStackPtr))--;
			break;
		case cmdDecD:
			*(double*)(genStackPtr) -= 1.0;
			break;
		case cmdDecL:
			(*(long long*)(genStackPtr))--;
			break;

		case cmdCreateClosure:
#ifdef _M_X64
			ClosureCreate(&genParams[paramBase], cmd.helper, cmd.argument, *(ExternFuncInfo::Upvalue**)genStackPtr);
			genStackPtr += 2;
#else
			ClosureCreate(&genParams[paramBase], cmd.helper, cmd.argument, (ExternFuncInfo::Upvalue*)(intptr_t)*genStackPtr);
			genStackPtr++;
#endif
			break;
		case cmdCloseUpvals:
			CloseUpvalues(&genParams[paramBase], cmd.argument);
			break;

		case cmdConvertPtr:
			if(*genStackPtr != cmd.argument)
			{
				SafeSprintf(execError, 1024, "ERROR: cannot convert from %s ref to %s ref", &exLinker->exSymbols[exLinker->exTypes[*genStackPtr].offsetToName], &exLinker->exSymbols[exLinker->exTypes[cmd.argument].offsetToName]);
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr++;
			break;
#ifdef _M_X64
		case cmdPushPtrImmt:
			genStackPtr--;
			*genStackPtr = cmd.argument;
			genStackPtr--;
			*genStackPtr = cmd.argument;
			break;
#endif
		}

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
		fprintf(executeLog, ";\r\n");
		fflush(executeLog);
#endif
	}
	// If there was an execution error
	if(errorState)
	{
		// Print call stack on error, when we get to the first function
		if(!finalReturn)
		{
			char *currPos = execError + strlen(execError);
			currPos += SafeSprintf(currPos, ERROR_BUFFER_SIZE - int(currPos - execError), "\r\nCall stack:\r\n");

			BeginCallStack();
			while(unsigned int address = GetNextAddress())
				currPos += PrintStackFrame(address, currPos, ERROR_BUFFER_SIZE - int(currPos - execError));
		}
		// Ascertain that execution stops when there is a chain of nullcRunFunction
		callContinue = false;
		codeRunning = false;
		return;
	}
	
	lastResultType = retType;
	lastResultL = genStackPtr[0];

	switch(retType)
	{
	case OTYPE_DOUBLE:
	case OTYPE_LONG:
		lastResultH = genStackPtr[1];
		genStackPtr += 2;
		break;
	case OTYPE_INT:
		genStackPtr++;
		break;
	}
}

void Executor::Stop(const char* error)
{
	codeRunning = false;
	callContinue = false;
	SafeSprintf(execError, ERROR_BUFFER_SIZE, error);
}

#if !defined(__CELLOS_LV2__) && !defined(_M_X64)
// X86 implementation
bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	unsigned int dwordsToPop = (exFunctions[funcID].bytesToPop >> 2);
	//unsigned int bytesToPop = exFunctions[funcID].bytesToPop;
	void* fPtr = exFunctions[funcID].funcPtr;
	unsigned int retType = exFunctions[funcID].retType;

	unsigned int *stackStart = (genStackPtr + dwordsToPop - 1);
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
	unsigned int *newStackPtr = genStackPtr + dwordsToPop + extraPopDW;

	switch(retType)
	{
	case ExternFuncInfo::RETURN_VOID:
		((void (*)())fPtr)();
		break;
	case ExternFuncInfo::RETURN_INT:
		newStackPtr -= 1;
		*newStackPtr = ((int (*)())fPtr)();
		break;
	case ExternFuncInfo::RETURN_DOUBLE:
		newStackPtr -= 2;
		*(double*)newStackPtr = ((double (*)())fPtr)();
		break;
	case ExternFuncInfo::RETURN_LONG:
		newStackPtr -= 2;
		*(long long*)newStackPtr = ((long long (*)())fPtr)();
		break;
#ifdef NULLC_COMPLEX_RETURN
	case ExternFuncInfo::RETURN_UNKNOWN:
	{
		unsigned int ret[128];
		unsigned int *ptr = &ret[0];
		asm("movl %0, %%eax"::"r"(ptr):"%eax");
		asm("pushl %eax");

		((void (*)())fPtr)();
		
		// adjust new stack top
		newStackPtr -= exFunctions[funcID].returnShift;
		// copy return value on top of the stack
		memcpy(newStackPtr, ret, exFunctions[funcID].returnShift * 4);
	}
		break;
#endif
	}
	genStackPtr = newStackPtr;
#ifdef __GNUC__
	asm("movl %0, %%eax"::"r"(dwordsToPop):"%eax");
	asm("leal (%esp, %eax, 0x4), %esp");
#else
	__asm mov eax, dwordsToPop
	__asm lea esp, [eax * 4 + esp]
#endif
	return callContinue;
}

#elif defined(__CELLOS_LV2__)
// PS3 implementation
#define MAKE_FUNC_PTR_TYPE(retType, typeName) typedef retType (*typeName)(			\
	unsigned long long, unsigned long long, unsigned long long, unsigned long long,	\
	unsigned long long, unsigned long long, unsigned long long, unsigned long long,	\
	double, double, double, double, double, double, double, double					\
	);
MAKE_FUNC_PTR_TYPE(void, VoidFunctionPtr)
MAKE_FUNC_PTR_TYPE(int, IntFunctionPtr)
MAKE_FUNC_PTR_TYPE(double, DoubleFunctionPtr)
MAKE_FUNC_PTR_TYPE(long long, LongFunctionPtr)

bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	unsigned int dwordsToPop = (exFunctions[funcID].bytesToPop >> 2) + extraPopDW;

	struct BigReturnForce
	{
		unsigned	unused[128];
	};
	MAKE_FUNC_PTR_TYPE(BigReturnForce, BigReturnFunctionPtr)

	// call function
	#define R(i) *(const unsigned long long*)(const void*)(genStackPtr + exFunctions[funcID].rOffsets[i])
	#define F(i) *(const double*)(const void*)(genStackPtr + exFunctions[funcID].fOffsets[i])

	unsigned int *newStackPtr = genStackPtr + dwordsToPop;

	switch(exFunctions[funcID].retType)
	{
	case ExternFuncInfo::RETURN_VOID:
		// cast function pointer so we can call it and call it
		((VoidFunctionPtr)exFunctions[funcID].funcPtr)(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));
		break;
	case ExternFuncInfo::RETURN_INT:
		newStackPtr -= 1;
		// cast function pointer so we can call it and call it
		*newStackPtr = ((IntFunctionPtr)exFunctions[funcID].funcPtr)(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));
		break;
	case ExternFuncInfo::RETURN_DOUBLE:
		newStackPtr -= 2;
		// cast function pointer so we can call it and call it
		*(double*)newStackPtr = ((DoubleFunctionPtr)exFunctions[funcID].funcPtr)(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));
		break;
	case ExternFuncInfo::RETURN_LONG:
		newStackPtr -= 2;
		// cast function pointer so we can call it and call it
		*(long long*)newStackPtr = ((LongFunctionPtr)exFunctions[funcID].funcPtr)(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));
		break;
	case ExternFuncInfo::RETURN_UNKNOWN:
	{
		unsigned int ret[128];
		*(BigReturnForce*)ret = ((BigReturnFunctionPtr)exFunctions[funcID].funcPtr)(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));
		newStackPtr -= exFunctions[funcID].returnShift;
		memcpy(newStackPtr, ret, exFunctions[funcID].returnShift * 4);
	}
		break;
	}
	genStackPtr = newStackPtr;

	#undef F
	#undef R

	return callContinue;
}

#elif defined(_M_X64)

// X64 implementation
bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	unsigned int dwordsToPop = (exFunctions[funcID].bytesToPop >> 2);
	void* fPtr = exFunctions[funcID].funcPtr;

	unsigned int *stackStart = genStackPtr;
	unsigned int *newStackPtr = genStackPtr + dwordsToPop + extraPopDW;

	assert(exFunctions[funcID].returnShift < 128);	// maximum supported return type size is 512 bytes

	// buffer for return value
	unsigned int ret[128];

	//asm("int $0x3");
	// call external function through gateway
	void (*gate)(unsigned int*, unsigned int*, void*) = (void (*)(unsigned int*, unsigned int*, void*))(void*)(gateCode.data + exFunctions[funcID].startInByteCode);
	gate(stackStart, ret, fPtr);

	// adjust new stack top
	newStackPtr -= exFunctions[funcID].returnShift;

	// copy return value on top of the stack
	memcpy(newStackPtr, ret, exFunctions[funcID].returnShift * 4);

	// set new stack top
	genStackPtr = newStackPtr;

	return callContinue;
}

#else
bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	strcpy(execError, "ERROR: external function call failed");
	return false;
}
#endif

namespace ExPriv
{
	char *oldBase;
	char *newBase;
	unsigned int oldSize;
	unsigned int objectName = GetStringHash("auto ref");
}

void Executor::FixupPointer(char* ptr, const ExternTypeInfo& type)
{
	char **rPtr = (char**)ptr;
	if(*rPtr > (char*)0x00010000)
	{
		if(*rPtr >= ExPriv::oldBase && *rPtr < (ExPriv::oldBase + ExPriv::oldSize))
		{
//			printf("\tFixing from %p to %p\r\n", ptr, ptr - ExPriv::oldBase + ExPriv::newBase);

			*rPtr = *rPtr - ExPriv::oldBase + ExPriv::newBase;
		}else{
			ExternTypeInfo &subType = exTypes[type.subType];
//			printf("\tGlobal pointer %s %p (at %p)\r\n", symbols + subType.offsetToName, *rPtr, ptr);
			unsigned int *marker = (unsigned int*)(*rPtr)-1;
//			printf("\tMarker is %d\r\n", *marker);
			if(NULLC::IsBasePointer(*rPtr) && *marker == 42)
			{
				*marker = 0;
				if(type.subCat != ExternTypeInfo::CAT_NONE)
					FixupVariable(*rPtr, subType);
			}
		}
	}
}

void Executor::FixupArray(char* ptr, const ExternTypeInfo& type)
{
	ExternTypeInfo &subType = exTypes[type.subType];
	unsigned int size = type.arrSize;
	if(type.arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		// Get real array size
		size = *(int*)(ptr + 4);
		// Mark target data
		FixupPointer(ptr, subType);
		// Switch pointer to array data
		char **rPtr = (char**)ptr;
		ptr = *rPtr;
		// If initialized, return
		if(!ptr)
			return;
	}
	switch(subType.subCat)
	{
	case ExternTypeInfo::CAT_ARRAY:
		for(unsigned int i = 0; i < size; i++, ptr += subType.size)
			FixupArray(ptr, subType);
		break;
	case ExternTypeInfo::CAT_POINTER:
		for(unsigned int i = 0; i < size; i++, ptr += subType.size)
			FixupPointer(ptr, subType);
		break;
	case ExternTypeInfo::CAT_CLASS:
		for(unsigned int i = 0; i < size; i++, ptr += subType.size)
			FixupClass(ptr, subType);
		break;
	}
}

void Executor::FixupClass(char* ptr, const ExternTypeInfo& type)
{
	const ExternTypeInfo *realType = &type;
	if(type.nameHash == ExPriv::objectName)
	{
		// Get real variable type
		realType = &exTypes[*(int*)ptr];
		// Mark target data
		FixupPointer(ptr + 4, *realType);
		// Switch pointer to target
		char **rPtr = (char**)(ptr + 4);
		// Fixup target
		FixupVariable(*rPtr, *realType);
		// Exit
		return;
	}
	unsigned int *memberList = &exLinker->exTypeExtra[0];
	//char *str = symbols + type.offsetToName;
	//const char *memberName = symbols + type.offsetToName + strlen(str) + 1;
	for(unsigned int n = 0; n < realType->memberCount; n++)
	{
		//unsigned int strLength = (unsigned int)strlen(memberName) + 1;
		ExternTypeInfo &subType = exTypes[memberList[realType->memberOffset + n]];
		FixupVariable(ptr, subType);
		//memberName += strLength;
		ptr += subType.size;
	}
}

void Executor::FixupVariable(char* ptr, const ExternTypeInfo& type)
{
	switch(type.subCat)
	{
	case ExternTypeInfo::CAT_ARRAY:
		FixupArray(ptr, type);
		break;
	case ExternTypeInfo::CAT_POINTER:
		FixupPointer(ptr, type);
		break;
	case ExternTypeInfo::CAT_CLASS:
		FixupClass(ptr, type);
		break;
	}
}

bool Executor::ExtendParameterStack(char* oldBase, unsigned int oldSize, VMCmd *current)
{
//	printf("Old base: %p-%p\r\n", oldBase, oldBase + oldSize);
//	printf("New base: %p-%p\r\n", genParams.data, genParams.data + genParams.max);

	SetUnmanagableRange(genParams.data, genParams.max);

	NULLC::MarkMemory(42);

	ExPriv::oldBase = oldBase;
	ExPriv::newBase = genParams.data;
	ExPriv::oldSize = oldSize;

	symbols = &exLinker->exSymbols[0];

	ExternVarInfo *vars = &exLinker->exVariables[0];
	ExternTypeInfo *types = &exLinker->exTypes[0];
	// Fix global variables
	for(unsigned int i = 0; i < exLinker->exVariables.size(); i++)
		FixupVariable(genParams.data + vars[i].offset, types[vars[i].type]);

	int offset = exLinker->globalVarSize;
	int n = 0;
	fcallStack.push_back(current);
	// Fixup local variables
	for(; n < (int)fcallStack.size(); n++)
	{
		int address = int(fcallStack[n]-cmdBase);
		int funcID = -1;

		int debugMatch = 0;
		for(unsigned int i = 0; i < exFunctions.size(); i++)
		{
			if(address >= exFunctions[i].address && address < (exFunctions[i].address + exFunctions[i].codeSize))
			{
				funcID = i;
				debugMatch++;
			}
		}
		assert(debugMatch < 2);

		if(funcID != -1)
		{
			int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;
//			printf("In function %s (with offset of %d)\r\n", symbols + exFunctions[funcID].offsetToName, alignOffset);
			offset += alignOffset;
			for(unsigned int i = 0; i < exFunctions[funcID].localCount; i++)
			{
				ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + i];
				FixupVariable(genParams.data + offset + lInfo.offset, types[lInfo.type]);
			}
			if(exFunctions[funcID].localCount)
			{
				ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + exFunctions[funcID].localCount - 1];
				offset += lInfo.offset + lInfo.size;
			}else{
				offset += 4;	// There's one hidden parameter
			}
		}
	}
	fcallStack.pop_back();

	return true;
}

const char* Executor::GetResult()
{
	if(!codeRunning && genStackSize > 1)
	{
		strcpy(execResult, "There is more than one value on the stack");
		return execResult;
	}
	long long combined = 0;
	*((int*)(&combined)) = lastResultL;
	*((int*)(&combined)+1) = lastResultH;

	switch(lastResultType)
	{
	case OTYPE_DOUBLE:
		SafeSprintf(execResult, 64, "%f", *(double*)(&combined));
		break;
	case OTYPE_LONG:
		SafeSprintf(execResult, 64, "%lldL", combined);
		break;
	case OTYPE_INT:
		SafeSprintf(execResult, 64, "%d", lastResultL);
		break;
	default:
		SafeSprintf(execResult, 64, "no return value");
		break;
	}
	return execResult;
}
int Executor::GetResultInt()
{
	assert(lastResultType == OTYPE_INT);
	return lastResultL;
}
double Executor::GetResultDouble()
{
	assert(lastResultType == OTYPE_DOUBLE);
	long long combined = 0;
	*((int*)(&combined)) = lastResultL;
	*((int*)(&combined)+1) = lastResultH;
	return *(double*)(&combined);
}
long long Executor::GetResultLong()
{
	assert(lastResultType == OTYPE_LONG);
	long long combined = 0;
	*((int*)(&combined)) = lastResultL;
	*((int*)(&combined)+1) = lastResultH;
	return combined;
}

const char*	Executor::GetExecError()
{
	return execError;
}

char* Executor::GetVariableData(unsigned int *count)
{
	if(count)
		*count = genParams.size();
	return &genParams[0];
}

void Executor::BeginCallStack()
{
	currentFrame = 0;
}
unsigned int Executor::GetNextAddress()
{
	return currentFrame == fcallStack.size() ? 0 : (unsigned int)(fcallStack[currentFrame++] - cmdBase);
}

void* Executor::GetStackStart()
{
	return genStackPtr;
}
void* Executor::GetStackEnd()
{
	return genStackTop;
}

void Executor::SetBreakFunction(unsigned (*callback)(unsigned int))
{
	breakFunction = callback;
}

void Executor::ClearBreakpoints()
{
	// Check all instructions for break instructions
	for(unsigned int i = 0; i < exLinker->exCode.size(); i++)
	{
		// nop instruction is used for breaks
		// break structure: cmdOriginal, cmdNop
		if(exLinker->exCode[i].cmd == cmdNop)
			exLinker->exCode[i] = breakCode[exLinker->exCode[i].argument];	// replace it with original instruction
	}
	breakCode.clear();
}

bool Executor::AddBreakpoint(unsigned int instruction)
{
	if(instruction > exLinker->exCode.size())
	{
		SafeSprintf(execError, ERROR_BUFFER_SIZE, "ERROR: break position out of code range");
		return false;
	}
	unsigned int pos = breakCode.size();
	breakCode.push_back(exLinker->exCode[instruction]);
	breakCode.push_back(VMCmd(cmdNop, EXEC_BREAK_RETURN, 0, instruction + 1));
	exLinker->exCode[instruction].cmd = cmdNop;
	exLinker->exCode[instruction].flag = EXEC_BREAK_SIGNAL;
	exLinker->exCode[instruction].argument = pos;
	return true;
}

bool Executor::RemoveBreakpoint(unsigned int instruction)
{
	if(instruction > exLinker->exCode.size())
	{
		SafeSprintf(execError, ERROR_BUFFER_SIZE, "ERROR: break position out of code range");
		return false;
	}
	if(exLinker->exCode[instruction].cmd != cmdNop)
	{
		SafeSprintf(execError, ERROR_BUFFER_SIZE, "ERROR: there is no breakpoint at instruction %d", instruction);
		return false;
	}
	exLinker->exCode[instruction] = breakCode[exLinker->exCode[instruction].argument];
	return true;
}

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
// Print instruction info into stream in a readable format
void PrintInstructionText(FILE* logASM, VMCmd cmd, unsigned int rel, unsigned int top)
{
	char	buf[128];
	memset(buf, ' ', 128);
	char	*curr = buf;
	curr += cmd.Decode(buf);
	*curr = ' ';
	sprintf(&buf[50], " rel = %d; top = %d", rel, top);
	fwrite(buf, 1, strlen(buf), logASM);
}
#endif
