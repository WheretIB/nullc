#include "Executor_RegVm.h"

#include "Executor_Common.h"

#include "nullc.h"
#include "nullc_debug.h"
#include "StdLib.h"

#if defined(_MSC_VER)
#pragma warning(disable: 4702) // unreachable code
#endif

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
#define dcAllocMem NULLC::alloc
#define dcFreeMem  NULLC::dealloc

#include "../external/dyncall/dyncall.h"
#endif

namespace
{
	int vmIntPow(int power, int number)
	{
		if(power < 0)
			return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

		int result = 1;
		while(power)
		{
			if(power & 1)
			{
				result *= number;
				power--;
			}
			number *= number;
			power >>= 1;
		}
		return result;
	}

	long long vmLongPow(long long power, long long number)
	{
		if(power < 0)
			return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

		long long result = 1;
		while(power)
		{
			if(power & 1)
			{
				result *= number;
				power--;
			}
			number *= number;
			power >>= 1;
		}
		return result;
	}

	long long vmLoadLong(void* target)
	{
		long long value;
		memcpy(&value, target, sizeof(long long));
		return value;
	}

	void vmStoreLong(void* target, long long value)
	{
		memcpy(target, &value, sizeof(long long));
	}

	double vmLoadDouble(void* target)
	{
		double value;
		memcpy(&value, target, sizeof(double));
		return value;
	}

	void vmStoreDouble(void* target, double value)
	{
		memcpy(target, &value, sizeof(double));
	}

	char* vmLoadPointer(void* target)
	{
		char* value;
		memcpy(&value, target, sizeof(char*));
		return value;
	}

	void vmStorePointer(void* target, char* value)
	{
		memcpy(target, &value, sizeof(char*));
	}
}

ExecutorRegVm::ExecutorRegVm(Linker* linker) : exLinker(linker), exTypes(linker->exTypes), exFunctions(linker->exFunctions)
{
	memset(execError, 0, REGVM_ERROR_BUFFER_SIZE);
	memset(execResult, 0, 64);

	codeRunning = false;

	lastResultType = rvrError;

	symbols = NULL;

	codeBase = NULL;

	minStackSize = 1 * 1024 * 1024;

	currentFrame = 0;

	tempStackArrayBase = NULL;
	tempStackLastTop = NULL;
	tempStackArrayEnd = NULL;

	regFileArrayBase = NULL;
	regFileLastTop = NULL;
	regFileArrayEnd = NULL;

	callContinue = true;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	dcCallVM = NULL;
#endif

	breakFunctionContext = NULL;
	breakFunction = NULL;
}

ExecutorRegVm::~ExecutorRegVm()
{
	NULLC::dealloc(tempStackArrayBase);

	NULLC::dealloc(regFileArrayBase);

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	if(dcCallVM)
		dcFree(dcCallVM);
#endif
}

void ExecutorRegVm::InitExecution()
{
	if(!exLinker->exRegVmCode.size())
	{
		strcpy(execError, "ERROR: no code to run");
		return;
	}

	callStack.clear();

	lastFinalReturn = 0;

	CommonSetLinker(exLinker);

	dataStack.reserve(minStackSize);
	dataStack.clear();
	dataStack.resize((exLinker->globalVarSize + 0xf) & ~0xf);

	SetUnmanagableRange(dataStack.data, dataStack.max);

	execError[0] = 0;

	callContinue = true;

	// Add return after the last instruction to end execution of code with no return at the end
	exLinker->exRegVmCode.push_back(RegVmCmd(rviReturn, 0, rvrError, 0, 0));
	exLinker->exRegVmExecCount.push_back(0);

	if(!tempStackArrayBase)
	{
		tempStackArrayBase = (unsigned*)NULLC::alloc(sizeof(unsigned) * 1024 * 16);
		memset(tempStackArrayBase, 0, sizeof(unsigned) * 1024 * 16);
		tempStackArrayEnd = tempStackArrayBase + 1024 * 16;
	}

	tempStackLastTop = tempStackArrayBase;

	if(!regFileArrayBase)
	{
		regFileArrayBase = (RegVmRegister*)NULLC::alloc(sizeof(RegVmRegister) * 1024 * 16);
		memset(regFileArrayBase, 0, sizeof(RegVmRegister) * 1024 * 16);
		regFileArrayEnd = regFileArrayBase + 1024 * 16;
	}

	regFileLastTop = regFileArrayBase;

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	if(!dcCallVM)
	{
		dcCallVM = dcNewCallVM(4096);
		dcMode(dcCallVM, DC_CALL_C_DEFAULT);
	}
#endif
}

void ExecutorRegVm::Run(unsigned functionID, const char *arguments)
{
	if(!codeRunning || functionID == ~0u)
		InitExecution();

	codeRunning = true;

	RegVmReturnType retType = rvrVoid;

	codeBase = &exLinker->exRegVmCode[0];
	RegVmCmd *instruction = &exLinker->exRegVmCode[exLinker->regVmOffsetToGlobalCode];

	bool errorState = false;

	// We will know that return is global if call stack size is equal to current
	unsigned prevLastFinalReturn = lastFinalReturn;
	lastFinalReturn = callStack.size();

	unsigned prevDataSize = dataStack.size();

	RegVmRegister *regFilePtr = regFileLastTop;
	RegVmRegister *regFileTop = regFilePtr + 256;

	unsigned *tempStackPtr = tempStackLastTop;

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
				if(!RunExternalFunction(functionID, tempStackPtr))
					errorState = true;
			}

			// This will disable NULLC code execution while leaving error check and result retrieval
			instruction = NULL;
		}
		else
		{
			instruction = &exLinker->exRegVmCode[funcPos];

			unsigned argumentsSize = target.bytesToPop;

			// Keep stack frames aligned to 16 byte boundary
			unsigned alignOffset = (dataStack.size() % 16 != 0) ? (16 - (dataStack.size() % 16)) : 0;

			if(dataStack.size() + alignOffset + argumentsSize >= dataStack.max)
			{
				callStack.push_back(instruction + 1);
				instruction = NULL;
				strcpy(execError, "ERROR: stack overflow");
				retType = rvrError;
			}
			else
			{
				// Copy arguments to new stack frame
				memcpy((char*)(dataStack.data + dataStack.size() + alignOffset), arguments, argumentsSize);

				unsigned stackSize = (target.stackSize + 0xf) & ~0xf;

				regFilePtr = regFileLastTop;
				regFileTop = regFilePtr + target.regVmRegisters;


				assert(dataStack.size() % 16 == 0);

				if(dataStack.size() + stackSize >= dataStack.max)
				{
					callStack.push_back(instruction + 1);
					instruction = NULL;
					strcpy(execError, "ERROR: stack overflow");
					retType = rvrError;
				}
				else
				{
					dataStack.resize(dataStack.size() + stackSize);

					assert(argumentsSize <= stackSize);

					if(stackSize - argumentsSize)
						memset(dataStack.data + prevDataSize + argumentsSize, 0, stackSize - argumentsSize);

					regFilePtr[rvrrGlobals].ptrValue = uintptr_t(dataStack.data);
					regFilePtr[rvrrFrame].ptrValue = uintptr_t(dataStack.data + prevDataSize);
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
		assert(dataStack.size() >= exLinker->globalVarSize);
		memset(dataStack.data, 0, exLinker->globalVarSize);


		regFilePtr[rvrrGlobals].ptrValue = uintptr_t(dataStack.data);
		regFilePtr[rvrrFrame].ptrValue = uintptr_t(dataStack.data);
		regFilePtr[rvrrConstants].ptrValue = uintptr_t(exLinker->exRegVmConstants.data);
		regFilePtr[rvrrRegisters].ptrValue = uintptr_t(regFilePtr);

		memset(regFilePtr + rvrrCount, 0, (regFileTop - regFilePtr - rvrrCount) * sizeof(regFilePtr[0]));
	}

	RegVmRegister *prevRegFileLastTop = regFileLastTop;

	regFileLastTop = regFileTop;

	tempStackLastTop = tempStackPtr;

	RegVmReturnType resultType = instruction ? RunCode(instruction, regFilePtr, tempStackPtr, this, codeBase) : retType;

	regFileLastTop = prevRegFileLastTop;

	assert(tempStackLastTop == tempStackPtr);

	dataStack.shrink(prevDataSize);

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
			currPos += NULLC::SafeSprintf(currPos, REGVM_ERROR_BUFFER_SIZE - int(currPos - execError), "\r\nCall stack:\r\n");

			BeginCallStack();
			while(unsigned address = GetNextAddress())
				currPos += PrintStackFrame(address, currPos, REGVM_ERROR_BUFFER_SIZE - int(currPos - execError), false);
		}

		lastFinalReturn = prevLastFinalReturn;

		// Ascertain that execution stops when there is a chain of nullcRunFunction
		callContinue = false;
		codeRunning = false;

		return;
	}

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

void ExecutorRegVm::Stop(const char* error)
{
	codeRunning = false;

	callContinue = false;
	NULLC::SafeSprintf(execError, REGVM_ERROR_BUFFER_SIZE, "%s", error);
}

bool ExecutorRegVm::SetStackSize(unsigned bytes)
{
	if(codeRunning)
		return false;

	minStackSize = bytes;

	return true;
}

#if (defined(__clang__) || defined(__GNUC__)) && !defined(NULLC_REG_VM_PROFILE_INSTRUCTIONS)
#define USE_COMPUTED_GOTO
#endif

RegVmReturnType ExecutorRegVm::RunCode(RegVmCmd *instruction, RegVmRegister * const regFilePtr, unsigned *tempStackPtr, ExecutorRegVm *rvm, RegVmCmd *codeBase)
{
	(void)codeBase;

#if defined(USE_COMPUTED_GOTO)
	static void* switchTable[] = {
		&&case_rviNop,
		&&case_rviLoadByte,
		&&case_rviLoadWord,
		&&case_rviLoadDword,
		&&case_rviLoadLong,
		&&case_rviLoadFloat,
		&&case_rviLoadDouble,
		&&case_rviLoadImm,
		&&case_rviLoadImmLong,
		&&case_rviLoadImmDouble,
		&&case_rviStoreByte,
		&&case_rviStoreWord,
		&&case_rviStoreDword,
		&&case_rviStoreLong,
		&&case_rviStoreFloat,
		&&case_rviStoreDouble,
		&&case_rviCombinedd,
		&&case_rviBreakupdd,
		&&case_rviMov,
		&&case_rviMovMult,
		&&case_rviDtoi,
		&&case_rviDtol,
		&&case_rviDtof,
		&&case_rviItod,
		&&case_rviLtod,
		&&case_rviItol,
		&&case_rviLtoi,
		&&case_rviIndex,
		&&case_rviGetAddr,
		&&case_rviSetRange,
		&&case_rviMemCopy,
		&&case_rviJmp,
		&&case_rviJmpz,
		&&case_rviJmpnz,
		&&case_rviPush,
		&&case_rviPushQword,
		&&case_rviPushImm,
		&&case_rviPushImmq,
		&&case_rviNop, // pushmem
		&&case_rviPop,
		&&case_rviPopq,
		&&case_rviNop, // popmem
		&&case_rviCall,
		&&case_rviCallPtr,
		&&case_rviReturn,
		&&case_rviAddImm,
		&&case_rviAdd,
		&&case_rviSub,
		&&case_rviMul,
		&&case_rviDiv,
		&&case_rviPow,
		&&case_rviMod,
		&&case_rviLess,
		&&case_rviGreater,
		&&case_rviLequal,
		&&case_rviGequal,
		&&case_rviEqual,
		&&case_rviNequal,
		&&case_rviShl,
		&&case_rviShr,
		&&case_rviBitAnd,
		&&case_rviBitOr,
		&&case_rviBitXor,
		&&case_rviLogXor,
		&&case_rviAddImml,
		&&case_rviAddl,
		&&case_rviSubl,
		&&case_rviMull,
		&&case_rviDivl,
		&&case_rviPowl,
		&&case_rviModl,
		&&case_rviLessl,
		&&case_rviGreaterl,
		&&case_rviLequall,
		&&case_rviGequall,
		&&case_rviEquall,
		&&case_rviNequall,
		&&case_rviShll,
		&&case_rviShrl,
		&&case_rviBitAndl,
		&&case_rviBitOrl,
		&&case_rviBitXorl,
		&&case_rviLogXorl,
		&&case_rviAddd,
		&&case_rviSubd,
		&&case_rviMuld,
		&&case_rviDivd,
		&&case_rviAddf,
		&&case_rviSubf,
		&&case_rviMulf,
		&&case_rviDivf,
		&&case_rviPowd,
		&&case_rviModd,
		&&case_rviLessd,
		&&case_rviGreaterd,
		&&case_rviLequald,
		&&case_rviGequald,
		&&case_rviEquald,
		&&case_rviNequald,
		&&case_rviNeg,
		&&case_rviNegl,
		&&case_rviNegd,
		&&case_rviBitNot,
		&&case_rviBitNotl,
		&&case_rviLogNot,
		&&case_rviLogNotl,
		&&case_rviConvertPtr,
	};

#define SWITCH goto *switchTable[instruction->code];
#define CASE(x) case_##x:
#define BREAK goto *switchTable[instruction->code]
#else
#define SWITCH switch(cmd.code)
#define CASE(x) case x:
#define BREAK break
#endif

	for(;;)
	{
#if defined(USE_COMPUTED_GOTO)
#define cmd (*instruction)
#else
		const RegVmCmd &cmd = *instruction;
#endif

#if defined(NULLC_REG_VM_PROFILE_INSTRUCTIONS)
		unsigned *executions = rvm->exLinker->exRegVmExecCount.data;

		executions[unsigned(instruction - codeBase)]++;

		unsigned *instructionExecutions = rvm->exLinker->exRegVmInstructionExecCount.data;

		instructionExecutions[cmd.code]++;
#endif

		SWITCH
		{
		CASE(rviNop)
			instruction = rvm->ExecNop(cmd, instruction, regFilePtr);

			if(!instruction)
				return rvrError;

			BREAK;
		CASE(rviLoadByte)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = *(char*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLoadWord)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = *(short*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLoadDword)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLoadLong)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = vmLoadLong((void*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));
			instruction++;
			BREAK;
		CASE(rviLoadFloat)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = *(float*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLoadDouble)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLoadImm)
			regFilePtr[cmd.rA].intValue = cmd.argument;
			instruction++;
			BREAK;
		CASE(rviLoadImmLong)
			regFilePtr[cmd.rA].longValue = ((uint64_t)cmd.argument << 32ull) | (unsigned)regFilePtr[cmd.rA].intValue;
			instruction++;
			BREAK;
		CASE(rviLoadImmDouble)
			{
				uint64_t bits = ((uint64_t)cmd.argument << 32ull) | (unsigned)regFilePtr[cmd.rA].intValue;

				memcpy(&regFilePtr[cmd.rA].doubleValue, &bits, sizeof(double));
			}
			instruction++;
			BREAK;
		CASE(rviStoreByte)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(char*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = (char)regFilePtr[cmd.rA].intValue;
			instruction++;
			BREAK;
		CASE(rviStoreWord)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(short*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = (short)regFilePtr[cmd.rA].intValue;
			instruction++;
			BREAK;
		CASE(rviStoreDword)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = regFilePtr[cmd.rA].intValue;
			instruction++;
			BREAK;
		CASE(rviStoreLong)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = regFilePtr[cmd.rA].longValue;
			instruction++;
			BREAK;
		CASE(rviStoreFloat)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(float*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = (float)regFilePtr[cmd.rA].doubleValue;
			instruction++;
			BREAK;
		CASE(rviStoreDouble)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = regFilePtr[cmd.rA].doubleValue;
			instruction++;
			BREAK;
		CASE(rviCombinedd)
			regFilePtr[cmd.rA].longValue = ((uint64_t)regFilePtr[cmd.rC].intValue << 32ull) | (unsigned)regFilePtr[cmd.rB].intValue;
			instruction++;
			BREAK;
		CASE(rviBreakupdd)
			regFilePtr[cmd.rA].intValue = (int)(regFilePtr[cmd.rC].longValue >> 32ull);
			regFilePtr[cmd.rB].intValue = (int)(regFilePtr[cmd.rC].longValue);
			instruction++;
			BREAK;
		CASE(rviMov)
			memcpy(&regFilePtr[cmd.rA], &regFilePtr[cmd.rC], sizeof(RegVmRegister));
			instruction++;
			BREAK;
		CASE(rviMovMult)
			memcpy(&regFilePtr[cmd.rA], &regFilePtr[cmd.rC], sizeof(RegVmRegister));
			memcpy(&regFilePtr[cmd.argument >> 24], &regFilePtr[(cmd.argument >> 16) & 0xff], sizeof(RegVmRegister));
			memcpy(&regFilePtr[(cmd.argument >> 8) & 0xff], &regFilePtr[cmd.argument & 0xff], sizeof(RegVmRegister));
			instruction++;
			BREAK;
		CASE(rviDtoi)
			regFilePtr[cmd.rA].intValue = (int)regFilePtr[cmd.rC].doubleValue;
			instruction++;
			BREAK;
		CASE(rviDtol)
			regFilePtr[cmd.rA].longValue = (long long)regFilePtr[cmd.rC].doubleValue;
			instruction++;
			BREAK;
		CASE(rviDtof)
			{
				float tmp = (float)regFilePtr[cmd.rC].doubleValue;

				memcpy(&regFilePtr[cmd.rA].intValue, &tmp, sizeof(float));
			}
			instruction++;
			BREAK;
		CASE(rviItod)
			regFilePtr[cmd.rA].doubleValue = (double)regFilePtr[cmd.rC].intValue;
			instruction++;
			BREAK;
		CASE(rviLtod)
			regFilePtr[cmd.rA].doubleValue = (double)regFilePtr[cmd.rC].longValue;
			instruction++;
			BREAK;
		CASE(rviItol)
			regFilePtr[cmd.rA].longValue = (long long)regFilePtr[cmd.rC].intValue;
			instruction++;
			BREAK;
		CASE(rviLtoi)
			regFilePtr[cmd.rA].intValue = (int)regFilePtr[cmd.rC].longValue;
			instruction++;
			BREAK;
		CASE(rviIndex)
			if(regFilePtr[cmd.rB].intValue >= regFilePtr[(cmd.argument >> 16) & 0xff].intValue)
				return rvm->ExecError(instruction, "ERROR: array index out of bounds");

			regFilePtr[cmd.rA].ptrValue = regFilePtr[cmd.rC].ptrValue + regFilePtr[cmd.rB].intValue * (cmd.argument & 0xffff);
			instruction++;
			BREAK;
		CASE(rviGetAddr)
			regFilePtr[cmd.rA].ptrValue = regFilePtr[cmd.rC].ptrValue + cmd.argument;
			instruction++;
			BREAK;
		CASE(rviSetRange)
			switch(RegVmSetRangeType(cmd.rB))
			{
			case rvsrDouble:

				for(unsigned i = 0; i < cmd.argument; i++)
					((double*)regFilePtr[cmd.rC].ptrValue)[i] = regFilePtr[cmd.rA].doubleValue;
				break;
			case rvsrFloat:

				for(unsigned i = 0; i < cmd.argument; i++)
					((float*)regFilePtr[cmd.rC].ptrValue)[i] = (float)regFilePtr[cmd.rA].doubleValue;
				break;
			case rvsrLong:

				for(unsigned i = 0; i < cmd.argument; i++)
					((long long*)regFilePtr[cmd.rC].ptrValue)[i] = regFilePtr[cmd.rA].longValue;
				break;
			case rvsrInt:

				for(unsigned i = 0; i < cmd.argument; i++)
					((int*)regFilePtr[cmd.rC].ptrValue)[i] = regFilePtr[cmd.rA].intValue;
				break;
			case rvsrShort:

				for(unsigned i = 0; i < cmd.argument; i++)
					((short*)regFilePtr[cmd.rC].ptrValue)[i] = (short)regFilePtr[cmd.rA].intValue;
				break;
			case rvsrChar:

				for(unsigned i = 0; i < cmd.argument; i++)
					((char*)regFilePtr[cmd.rC].ptrValue)[i] = (char)regFilePtr[cmd.rA].intValue;
				break;
			default:
				assert(!"unknown type");
			}
			instruction++;
			BREAK;
		CASE(rviMemCopy)
			memcpy((void*)regFilePtr[cmd.rA].ptrValue, (void*)regFilePtr[cmd.rC].ptrValue, cmd.argument);
			instruction++;
			BREAK;
		CASE(rviJmp)
#ifdef _M_X64
			instruction = codeBase + cmd.argument - 1;
#else
			instruction = rvm->codeBase + cmd.argument - 1;
#endif
			instruction++;
			BREAK;
		CASE(rviJmpz)
			if(regFilePtr[cmd.rC].intValue == 0)
			{
#ifdef _M_X64
				instruction = codeBase + cmd.argument - 1;
#else
				instruction = rvm->codeBase + cmd.argument - 1;
#endif
			}
			instruction++;
			BREAK;
		CASE(rviJmpnz)
			if(regFilePtr[cmd.rC].intValue != 0)
			{
#ifdef _M_X64
				instruction = codeBase + cmd.argument - 1;
#else
				instruction = rvm->codeBase + cmd.argument - 1;
#endif
			}
			instruction++;
			BREAK;
		CASE(rviPop)
			regFilePtr[cmd.rA].intValue = tempStackPtr[cmd.argument];
			instruction++;
			BREAK;
		CASE(rviPopq)
			regFilePtr[cmd.rA].longValue = vmLoadLong(&tempStackPtr[cmd.argument]);
			instruction++;
			BREAK;
		CASE(rviPush)
			*tempStackPtr++ = regFilePtr[cmd.rC].intValue;
			instruction++;
			BREAK;
		CASE(rviPushQword)
			memcpy(tempStackPtr, &regFilePtr[cmd.rC].longValue, sizeof(long long));
			tempStackPtr += 2;
			instruction++;
			BREAK;
		CASE(rviPushImm)
			*tempStackPtr = cmd.argument;
			tempStackPtr += 1;
			instruction++;
			BREAK;
		CASE(rviPushImmq)
			vmStoreLong(tempStackPtr, cmd.argument);
			tempStackPtr += 2;
			instruction++;
			BREAK;
		CASE(rviCall)
			tempStackPtr = rvm->ExecCall((cmd.rA << 16) | (cmd.rB << 8) | cmd.rC, cmd.argument, instruction, regFilePtr, tempStackPtr);

			if(!tempStackPtr)
				return rvrError;

			instruction++;
			BREAK;
		CASE(rviCallPtr)

			if(regFilePtr[cmd.rC].intValue == 0)
				return rvm->ExecError(instruction, "ERROR: invalid function pointer");

			tempStackPtr = rvm->ExecCall(cmd.argument, regFilePtr[cmd.rC].intValue, instruction, regFilePtr, tempStackPtr);

			if(!tempStackPtr)
				return rvrError;

			instruction++;
			BREAK;
		CASE(rviReturn)
			return rvm->ExecReturn(cmd, instruction, regFilePtr, tempStackPtr);
		CASE(rviAddImm)
			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue + (int)cmd.argument;
			instruction++;
			BREAK;
		CASE(rviAdd)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue + *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviSub)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue - *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviMul)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue * *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviDiv)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			if(*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue / *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviPow)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = vmIntPow(*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument), regFilePtr[cmd.rB].intValue);
			instruction++;
			BREAK;
		CASE(rviMod)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			if(*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue % *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLess)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue < *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviGreater)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue > *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLequal)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue <= *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviGequal)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue >= *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviEqual)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue == *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviNequal)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue != *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviShl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue << *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviShr)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue >> *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviBitAnd)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue & *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviBitOr)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue | *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviBitXor)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue ^ *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLogXor)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = (regFilePtr[cmd.rB].intValue != 0) != (*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) != 0);
			instruction++;
			BREAK;
		CASE(rviAddImml)
			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue + (int)cmd.argument;
			instruction++;
			BREAK;
		CASE(rviAddl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue + *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviSubl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue - *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviMull)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue * *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviDivl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			if(*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue / *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviPowl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = vmLongPow(*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument), regFilePtr[cmd.rB].longValue);
			instruction++;
			BREAK;
		CASE(rviModl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			if(*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue % *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLessl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue < *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviGreaterl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue > *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLequall)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue <= *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviGequall)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue >= *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviEquall)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue == *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviNequall)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue != *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviShll)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue << *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviShrl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue >> *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviBitAndl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue & *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviBitOrl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue | *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviBitXorl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue ^ *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLogXorl)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = (regFilePtr[cmd.rB].longValue != 0) != (*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) != 0);
			instruction++;
			BREAK;
		CASE(rviAddd)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue + *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviSubd)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue - *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviMuld)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue * *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviDivd)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue / *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviAddf)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue + *(float*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviSubf)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue - *(float*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviMulf)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue * *(float*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviDivf)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue / *(float*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviPowd)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = pow(regFilePtr[cmd.rB].doubleValue, *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));
			instruction++;
			BREAK;
		CASE(rviModd)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = fmod(regFilePtr[cmd.rB].doubleValue, *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));
			instruction++;
			BREAK;
		CASE(rviLessd)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue < *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviGreaterd)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue > *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviLequald)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue <= *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviGequald)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue >= *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviEquald)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue == *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviNequald)
			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue != *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			BREAK;
		CASE(rviNeg)
			regFilePtr[cmd.rA].intValue = -regFilePtr[cmd.rC].intValue;
			instruction++;
			BREAK;
		CASE(rviNegl)
			regFilePtr[cmd.rA].longValue = -regFilePtr[cmd.rC].longValue;
			instruction++;
			BREAK;
		CASE(rviNegd)
			regFilePtr[cmd.rA].doubleValue = -regFilePtr[cmd.rC].doubleValue;
			instruction++;
			BREAK;
		CASE(rviBitNot)
			regFilePtr[cmd.rA].intValue = ~regFilePtr[cmd.rC].intValue;
			instruction++;
			BREAK;
		CASE(rviBitNotl)
			regFilePtr[cmd.rA].longValue = ~regFilePtr[cmd.rC].longValue;
			instruction++;
			BREAK;
		CASE(rviLogNot)
			regFilePtr[cmd.rA].intValue = !regFilePtr[cmd.rC].intValue;
			instruction++;
			BREAK;
		CASE(rviLogNotl)
			regFilePtr[cmd.rA].intValue = !regFilePtr[cmd.rC].longValue;
			instruction++;
			BREAK;
		CASE(rviConvertPtr)
			if(!rvm->ExecConvertPtr(cmd, instruction, regFilePtr))
				return rvrError;

			instruction++;
			BREAK;
#if !defined(USE_COMPUTED_GOTO)
		default:
#if defined(_MSC_VER)
			__assume(false);
#elif defined(__GNUC__)
			__builtin_unreachable();
#endif
#endif
		}
	}

#undef SWITCH
#undef CASE
#undef BREAK

#if defined(USE_COMPUTED_GOTO)
#undef cmd
#endif

	return rvrError;
}

bool ExecutorRegVm::RunExternalFunction(unsigned funcID, unsigned *callStorage)
{
#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	ExternFuncInfo &func = exFunctions[funcID];

	assert(func.funcPtrRaw);

	void* fPtr = (void*)func.funcPtrRaw;
	unsigned retType = func.retType;

	unsigned *stackStart = callStorage;

	dcReset(dcCallVM);

#if defined(_WIN64)
	bool returnByPointer = func.returnShift > 1;
#elif !defined(_M_X64)
	bool returnByPointer = true;
#elif defined(__aarch64__)
	ExternTypeInfo &funcType = exTypes[func.funcType];

	ExternMemberInfo &member = exLinker->exTypeExtra[funcType.memberOffset];
	ExternTypeInfo &returnType = exLinker->exTypes[member.type];

	bool returnByPointer = false;

	bool opaqueType = returnType.subCat != ExternTypeInfo::CAT_CLASS || returnType.memberCount == 0;

	bool firstQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 0, 8, exLinker);
	bool secondQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 8, 16, exLinker);
#else
	ExternTypeInfo &funcType = exTypes[func.funcType];

	ExternMemberInfo &member = exLinker->exTypeExtra[funcType.memberOffset];
	ExternTypeInfo &returnType = exLinker->exTypes[member.type];

	bool returnByPointer = func.returnShift > 4 || member.type == NULLC_TYPE_AUTO_REF || (returnType.subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(&returnType, exLinker));

	bool opaqueType = returnType.subCat != ExternTypeInfo::CAT_CLASS || returnType.memberCount == 0;

	bool firstQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 0, 8, exLinker);
	bool secondQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 8, 16, exLinker);
#endif

	unsigned ret[128];

	if(retType == ExternFuncInfo::RETURN_UNKNOWN && returnByPointer)
		dcArgPointer(dcCallVM, ret);

	for(unsigned i = 0; i < func.paramCount; i++)
	{
		// Get information about local
		ExternLocalInfo &lInfo = exLinker->exLocals[func.offsetToFirstLocal + i];

		ExternTypeInfo &tInfo = exTypes[lInfo.type];

		switch(tInfo.type)
		{
		case ExternTypeInfo::TYPE_COMPLEX:
#if defined(_WIN64)
			if(tInfo.size <= 4)
			{
				// This branch also handles 0 byte structs
				dcArgInt(dcCallVM, *(int*)stackStart);
				stackStart += 1;
			}
			else if(tInfo.size <= 8)
			{
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
				stackStart += 2;
			}
			else
			{
				dcArgPointer(dcCallVM, stackStart);
				stackStart += tInfo.size / 4;
			}
#elif defined(__aarch64__)
			if(tInfo.size <= 4)
			{
				// This branch also handles 0 byte structs
				dcArgInt(dcCallVM, *(int*)stackStart);
				stackStart += 1;
			}
			else if(tInfo.size <= 8)
			{
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
				stackStart += 2;
			}
			else if(tInfo.size <= 12)
			{
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
				dcArgInt(dcCallVM, *(int*)(stackStart + 2));
				stackStart += 3;
			}
			else if(tInfo.size <= 16)
			{
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart + 2));
				stackStart += 4;
			}
			else
			{
				dcArgPointer(dcCallVM, stackStart);
				stackStart += tInfo.size / 4;
			}
#elif defined(_M_X64)
			if(tInfo.size > 16 || lInfo.type == NULLC_TYPE_AUTO_REF || (tInfo.subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(&tInfo, exLinker)))
			{
				dcArgStack(dcCallVM, stackStart, (tInfo.size + 7) & ~7);
				stackStart += tInfo.size / 4;
			}
			else
			{
				bool opaqueType = tInfo.subCat != ExternTypeInfo::CAT_CLASS || tInfo.memberCount == 0;

				bool firstQwordInteger = opaqueType || HasIntegerMembersInRange(tInfo, 0, 8, exLinker);
				bool secondQwordInteger = opaqueType || HasIntegerMembersInRange(tInfo, 8, 16, exLinker);

				if(tInfo.size <= 4)
				{
					if(tInfo.size != 0)
					{
						if(firstQwordInteger)
							dcArgInt(dcCallVM, *(int*)stackStart);
						else
							dcArgFloat(dcCallVM, *(float*)stackStart);
					}
					else
					{
						stackStart += 1;
					}
				}
				else if(tInfo.size <= 8)
				{
					if(firstQwordInteger)
						dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
					else
						dcArgDouble(dcCallVM, vmLoadDouble(stackStart));
				}
				else
				{
					int requredIRegs = (firstQwordInteger ? 1 : 0) + (secondQwordInteger ? 1 : 0);

					if(dcFreeIRegs(dcCallVM) < requredIRegs || dcFreeFRegs(dcCallVM) < (2 - requredIRegs))
					{
						dcArgStack(dcCallVM, stackStart, (tInfo.size + 7) & ~7);
					}
					else
					{
						if(firstQwordInteger)
							dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
						else
							dcArgDouble(dcCallVM, vmLoadDouble(stackStart));

						if(secondQwordInteger)
							dcArgLongLong(dcCallVM, vmLoadLong(stackStart + 2));
						else
							dcArgDouble(dcCallVM, vmLoadDouble(stackStart + 2));
					}
				}

				stackStart += tInfo.size / 4;
			}
#else
			if(tInfo.size <= 4)
			{
				// This branch also handles 0 byte structs
				dcArgInt(dcCallVM, *(int*)stackStart);
				stackStart += 1;
			}
			else
			{
				for(unsigned k = 0; k < tInfo.size / 4; k++)
				{
					dcArgInt(dcCallVM, *(int*)stackStart);
					stackStart += 1;
				}
			}
#endif
			break;
		case ExternTypeInfo::TYPE_VOID:
			return false;
		case ExternTypeInfo::TYPE_INT:
			dcArgInt(dcCallVM, *(int*)stackStart);
			stackStart += 1;
			break;
		case ExternTypeInfo::TYPE_FLOAT:
			dcArgFloat(dcCallVM, *(float*)stackStart);
			stackStart += 1;
			break;
		case ExternTypeInfo::TYPE_LONG:
			dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
			stackStart += 2;
			break;
		case ExternTypeInfo::TYPE_DOUBLE:
			dcArgDouble(dcCallVM, vmLoadDouble(stackStart));
			stackStart += 2;
			break;
		case ExternTypeInfo::TYPE_SHORT:
			dcArgShort(dcCallVM, *(short*)stackStart);
			stackStart += 1;
			break;
		case ExternTypeInfo::TYPE_CHAR:
			dcArgChar(dcCallVM, *(char*)stackStart);
			stackStart += 1;
			break;
		}
	}

	dcArgPointer(dcCallVM, (DCpointer)vmLoadPointer(stackStart));

	unsigned *newStackPtr = callStorage;

	switch(retType)
	{
	case ExternFuncInfo::RETURN_VOID:
		dcCallVoid(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_INT:
		*newStackPtr = dcCallInt(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_DOUBLE:
		if(func.returnShift == 1)
			vmStoreDouble(newStackPtr, dcCallFloat(dcCallVM, fPtr));
		else
			vmStoreDouble(newStackPtr, dcCallDouble(dcCallVM, fPtr));
		break;
	case ExternFuncInfo::RETURN_LONG:
		vmStoreLong(newStackPtr, dcCallLongLong(dcCallVM, fPtr));
		break;
	case ExternFuncInfo::RETURN_UNKNOWN:
#if defined(_WIN64)
		if(func.returnShift == 1)
		{
			*newStackPtr = dcCallInt(dcCallVM, fPtr);
		}
		else
		{
			dcCallVoid(dcCallVM, fPtr);

			// copy return value on top of the stack
			memcpy(newStackPtr, ret, func.returnShift * 4);
		}
#elif !defined(_M_X64)
		dcCallPointer(dcCallVM, fPtr);

		// copy return value on top of the stack
		memcpy(newStackPtr, ret, func.returnShift * 4);
#elif defined(__aarch64__)
		if(func.returnShift > 4)
		{
			DCcomplexbig res = dcCallComplexBig(dcCallVM, fPtr);

			memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
		}
		else
		{
			if(!firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexdd res = dcCallComplexDD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else if(firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexld res = dcCallComplexLD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else if(!firstQwordInteger && secondQwordInteger)
			{
				DCcomplexdl res = dcCallComplexDL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else
			{
				DCcomplexll res = dcCallComplexLL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
		}
#else
		if(returnByPointer)
		{
			dcCallPointer(dcCallVM, fPtr);

			// copy return value on top of the stack
			memcpy(newStackPtr, ret, func.returnShift * 4);
		}
		else
		{
			if(!firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexdd res = dcCallComplexDD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else if(firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexld res = dcCallComplexLD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else if(!firstQwordInteger && secondQwordInteger)
			{
				DCcomplexdl res = dcCallComplexDL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else
			{
				DCcomplexll res = dcCallComplexLL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
		}
#endif
		break;
	}

	return callContinue;
#else
	(void)funcID;
	(void)callStorage;

	Stop("ERROR: external raw function calls are disabled");

	return false;
#endif
}

RegVmCmd* ExecutorRegVm::ExecNop(const RegVmCmd cmd, RegVmCmd * const instruction, RegVmRegister * const regFilePtr)
{
	if(cmd.rB == EXEC_BREAK_SIGNAL || cmd.rB == EXEC_BREAK_ONCE)
	{
		if(breakFunction == NULL)
		{
			ExecError(instruction, "ERROR: break function isn't set");
			return NULL;
		}

		unsigned target = cmd.argument;
		callStack.push_back(instruction + 1);

		if(instruction < codeBase || instruction > exLinker->exRegVmCode.data + exLinker->exRegVmCode.size())
		{
			ExecError(instruction, "ERROR: break position is out of range");
			return NULL;
		}

		unsigned response = breakFunction(breakFunctionContext, unsigned(instruction - codeBase));
		callStack.pop_back();

		if(response == NULLC_BREAK_STOP)
		{
			ExecError(instruction, "ERROR: execution was stopped after breakpoint");
			return NULL;
		}

		// Step command - set breakpoint on the next instruction, if there is no breakpoint already
		if(response)
		{
			// Next instruction for step command
			RegVmCmd *nextCommand = instruction + 1;

			// Step command - handle unconditional jump step
			if(breakCode[target].code == rviJmp)
				nextCommand = codeBase + breakCode[target].argument;
			// Step command - handle conditional "jump on false" step
			if(breakCode[target].code == rviJmpz && regFilePtr[cmd.rC].intValue == 0)
				nextCommand = codeBase + breakCode[target].argument;
			// Step command - handle conditional "jump on true" step
			if(breakCode[target].code == rviJmpnz && regFilePtr[cmd.rC].intValue != 0)
				nextCommand = codeBase + breakCode[target].argument;
			// Step command - handle "return" step
			if(breakCode[target].code == rviReturn && callStack.size() != lastFinalReturn)
				nextCommand = callStack.back();

			if(response == NULLC_BREAK_STEP_INTO && breakCode[target].code == rviCall && exFunctions[breakCode[target].argument].regVmAddress != -1)
				nextCommand = codeBase + exFunctions[breakCode[target].argument].regVmAddress;
			if(response == NULLC_BREAK_STEP_INTO && breakCode[target].code == rviCallPtr && regFilePtr[cmd.rC].intValue && exFunctions[regFilePtr[cmd.rC].intValue].regVmAddress != -1)
				nextCommand = codeBase + exFunctions[regFilePtr[cmd.rC].intValue].regVmAddress;

			if(response == NULLC_BREAK_STEP_OUT && callStack.size() != lastFinalReturn)
				nextCommand = callStack.back();

			if(nextCommand->code != rviNop)
			{
				unsigned pos = breakCode.size();
				breakCode.push_back(*nextCommand);
				nextCommand->code = rviNop;
				nextCommand->rB = EXEC_BREAK_ONCE;
				nextCommand->argument = pos;
			}
		}
		// This flag means that breakpoint works only once
		if(cmd.rB == EXEC_BREAK_ONCE)
		{
			*instruction = breakCode[target];

			return instruction;
		}

		// Jump to external code
		return &breakCode[target];
	}

	return codeBase + cmd.argument;
}

unsigned* ExecutorRegVm::ExecCall(unsigned microcodePos, unsigned functionId, RegVmCmd * const instruction, RegVmRegister * const regFilePtr, unsigned *tempStackPtr)
{
	// Push arguments
	unsigned *microcode = exLinker->exRegVmConstants.data + microcodePos;

	while(*microcode != rviCall)
	{
		switch(*microcode++)
		{
		case rviPush:
			*tempStackPtr = regFilePtr[*microcode++].intValue;
			tempStackPtr += 1;
			break;
		case rviPushQword:
			memcpy(tempStackPtr, &regFilePtr[*microcode++].longValue, sizeof(long long));
			tempStackPtr += 2;
			break;
		case rviPushImm:
			*tempStackPtr = *microcode++;
			tempStackPtr += 1;
			break;
		case rviPushImmq:
			vmStoreLong(tempStackPtr, *microcode++);
			tempStackPtr += 2;
			break;
		case rviPushMem:
		{
			unsigned reg = *microcode++;
			unsigned offset = *microcode++;
			unsigned size = *microcode++;
			memcpy(tempStackPtr, (char*)regFilePtr[reg].ptrValue + offset, size);
			tempStackPtr += size >> 2;
		}
		break;
		}
	}

	microcode++;

	unsigned char resultReg = *microcode++ & 0xff;
	unsigned char resultType = *microcode++ & 0xff;

	ExternFuncInfo &target = exFunctions[functionId];

	unsigned address = target.regVmAddress;

	if(address == EXTERNAL_FUNCTION)
	{
		callStack.push_back(instruction + 1);

		// Take arguments
		tempStackPtr -= target.bytesToPop >> 2;

		unsigned *tempStackTop = tempStackLastTop;

		tempStackLastTop = tempStackPtr;

		if(target.funcPtrWrap)
		{
			target.funcPtrWrap(target.funcPtrWrapTarget, (char*)tempStackPtr, (char*)tempStackPtr);

			if(!callContinue)
				return NULL;
		}
		else
		{
			if(!RunExternalFunction(functionId, tempStackPtr))
				return NULL;
		}

		tempStackLastTop = tempStackTop;

		callStack.pop_back();

		switch(resultType)
		{
		case rvrDouble:
			memcpy(&regFilePtr[resultReg].doubleValue, tempStackPtr, sizeof(double));
			break;
		case rvrLong:
			memcpy(&regFilePtr[resultReg].longValue, tempStackPtr, sizeof(long long));
			break;
		case rvrInt:
			regFilePtr[resultReg].intValue = *tempStackPtr;
			break;
		default:
			break;
		}

		unsigned *curr = tempStackPtr;

		while(*microcode != rviReturn)
		{
			switch(*microcode++)
			{
			case rviPop:
				regFilePtr[*microcode++].intValue = *curr;
				curr += 1;
				break;
			case rviPopq:
				regFilePtr[*microcode++].longValue = vmLoadLong(curr);
				curr += 2;
				break;
			case rviPopMem:
			{
				unsigned reg = *microcode++;
				unsigned offset = *microcode++;
				unsigned size = *microcode++;
				memcpy((char*)regFilePtr[reg].ptrValue + offset, curr, size);
				curr += size >> 2;
			}
			break;
			}
		}

		return tempStackPtr;
	}

	callStack.push_back(instruction + 1);

	unsigned prevDataSize = dataStack.size();

	unsigned argumentsSize = target.bytesToPop;

	// Data stack is always aligned to 16 bytes
	assert(dataStack.size() % 16 == 0);

	if(dataStack.size() + argumentsSize >= dataStack.max)
	{
		codeRunning = false;
		strcpy(execError, "ERROR: stack overflow");

		return NULL;
	}

	// Take arguments
	tempStackPtr -= target.bytesToPop >> 2;

	// Copy function arguments to new stack frame
	memcpy((char*)(dataStack.data + dataStack.size()), tempStackPtr, argumentsSize);

	unsigned stackSize = (target.stackSize + 0xf) & ~0xf;

	RegVmRegister *regFileTop = regFileLastTop;

	regFileLastTop = regFileTop + target.regVmRegisters;

	if(regFileLastTop >= regFileArrayEnd)
	{
		codeRunning = false;
		strcpy(execError, "ERROR: register overflow");

		return NULL;
	}


	assert(dataStack.size() % 16 == 0);

	if(dataStack.size() + stackSize >= dataStack.max)
	{
		codeRunning = false;
		strcpy(execError, "ERROR: stack overflow");

		return NULL;
	}

	dataStack.resize(dataStack.size() + stackSize);

	assert(argumentsSize <= stackSize);

	if(stackSize - argumentsSize)
		memset(dataStack.data + prevDataSize + argumentsSize, 0, stackSize - argumentsSize);

	regFileTop[rvrrGlobals].ptrValue = uintptr_t(dataStack.data);
	regFileTop[rvrrFrame].ptrValue = uintptr_t(dataStack.data + prevDataSize);
	regFileTop[rvrrConstants].ptrValue = uintptr_t(exLinker->exRegVmConstants.data);
	regFileTop[rvrrRegisters].ptrValue = uintptr_t(regFileTop);

	memset(regFileTop + rvrrCount, 0, (regFileLastTop - regFileTop - rvrrCount) * sizeof(regFilePtr[0]));

	RegVmReturnType execResultType = RunCode(codeBase + address, regFileTop, tempStackPtr, this, codeBase);

	if(execResultType == rvrError)
		return NULL;

	assert(execResultType == resultType);

	regFileLastTop = regFileTop;

	dataStack.shrink(prevDataSize);

	switch(resultType)
	{
	case rvrDouble:
		memcpy(&regFilePtr[resultReg].doubleValue, tempStackPtr, sizeof(double));
		break;
	case rvrLong:
		memcpy(&regFilePtr[resultReg].longValue, tempStackPtr, sizeof(long long));
		break;
	case rvrInt:
		regFilePtr[resultReg].intValue = *tempStackPtr;
		break;
	default:
		break;
	}

	unsigned *curr = tempStackPtr;

	while(*microcode != rviReturn)
	{
		switch(*microcode++)
		{
		case rviPop:
			regFilePtr[*microcode++].intValue = *curr;
			curr += 1;
			break;
		case rviPopq:
			regFilePtr[*microcode++].longValue = vmLoadLong(curr);
			curr += 2;
			break;
		case rviPopMem:
		{
			unsigned reg = *microcode++;
			unsigned offset = *microcode++;
			unsigned size = *microcode++;
			memcpy((char*)regFilePtr[reg].ptrValue + offset, curr, size);
			curr += size >> 2;
		}
		break;
		}
	}

	return tempStackPtr;
}

RegVmReturnType ExecutorRegVm::ExecReturn(const RegVmCmd cmd, RegVmCmd * const instruction, RegVmRegister * const regFilePtr, unsigned *tempStackPtr)
{
	if(cmd.rB == rvrError)
	{
		bool errorState = !callStack.empty();

		callStack.push_back(instruction + 1);

		if(errorState)
			strcpy(execError, "ERROR: function didn't return a value");

		codeRunning = false;

		return errorState ? rvrError : rvrVoid;
	}

	if(cmd.rB != rvrVoid)
	{
		unsigned *microcode = exLinker->exRegVmConstants.data + cmd.argument;

		unsigned *tempStackPtrStart = tempStackPtr;
		unsigned typeId = *microcode++;

		while(*microcode != rviReturn)
		{
			switch(*microcode++)
			{
			case rviPush:
				*tempStackPtr = regFilePtr[*microcode++].intValue;
				tempStackPtr += 1;
				break;
			case rviPushQword:
				memcpy(tempStackPtr, &regFilePtr[*microcode++].longValue, sizeof(long long));
				tempStackPtr += 2;
				break;
			case rviPushImm:
				*tempStackPtr = *microcode++;
				tempStackPtr += 1;
				break;
			case rviPushImmq:
				vmStoreLong(tempStackPtr, *microcode++);
				tempStackPtr += 2;
				break;
			case rviPushMem:
			{
				unsigned reg = *microcode++;
				unsigned offset = *microcode++;
				unsigned size = *microcode++;
				memcpy(tempStackPtr, (char*)regFilePtr[reg].ptrValue + offset, size);
				tempStackPtr += size >> 2;
			}
			break;
			}
		}

		if(cmd.rC)
			ExecCheckedReturn(typeId, regFilePtr, tempStackPtrStart);
	}

	if(callStack.size() == lastFinalReturn)
	{
		if(lastFinalReturn == 0)
			codeRunning = false;

		return RegVmReturnType(cmd.rB);
	}

	callStack.pop_back();

	return RegVmReturnType(cmd.rB);
}

bool ExecutorRegVm::ExecConvertPtr(const RegVmCmd cmd, RegVmCmd * const instruction, RegVmRegister * const regFilePtr)
{
	unsigned typeId = regFilePtr[cmd.rB].intValue;

	if(!ConvertFromAutoRef(cmd.argument, typeId))
	{
		callStack.push_back(instruction + 1);

		codeRunning = false;

		NULLC::SafeSprintf(execError, 1024, "ERROR: cannot convert from %s ref to %s ref", &exLinker->exSymbols[exLinker->exTypes[typeId].offsetToName], &exLinker->exSymbols[exLinker->exTypes[cmd.argument].offsetToName]);

		return false;
	}

	regFilePtr[cmd.rA].ptrValue = regFilePtr[cmd.rC].ptrValue;

	return true;
}

void ExecutorRegVm::ExecCheckedReturn(unsigned typeId, RegVmRegister * const regFilePtr, unsigned * const tempStackPtr)
{
	uintptr_t frameBase = regFilePtr[rvrrFrame].ptrValue;
	uintptr_t frameEnd = regFilePtr[rvrrGlobals].ptrValue + dataStack.size();

	ExternTypeInfo &type = exLinker->exTypes[typeId];

	char *returnValuePtr = (char*)tempStackPtr;

	void *ptr = vmLoadPointer(returnValuePtr);

	if(uintptr_t(ptr) >= frameBase && uintptr_t(ptr) <= frameEnd)
	{
		if(type.arrSize == ~0u)
		{
			unsigned length = *(int*)(returnValuePtr + sizeof(void*));

			char *copy = (char*)NULLC::AllocObject(exLinker->exTypes[type.subType].size * length);
			memcpy(copy, ptr, unsigned(exLinker->exTypes[type.subType].size * length));
			vmStorePointer(returnValuePtr, copy);
		}
		else
		{
			unsigned objSize = type.size;

			char *copy = (char*)NULLC::AllocObject(objSize);
			memcpy(copy, ptr, objSize);
			vmStorePointer(returnValuePtr, copy);
		}
	}
}

RegVmReturnType ExecutorRegVm::ExecError(RegVmCmd * const instruction, const char *errorMessage)
{
	callStack.push_back(instruction + 1);

	codeRunning = false;

	strcpy(execError, errorMessage);

	return rvrError;
}

const char* ExecutorRegVm::GetResult()
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

int ExecutorRegVm::GetResultInt()
{
	assert(lastResultType == rvrInt);

	return lastResult.intValue;
}

double ExecutorRegVm::GetResultDouble()
{
	assert(lastResultType == rvrDouble);

	return lastResult.doubleValue;
}

long long ExecutorRegVm::GetResultLong()
{
	assert(lastResultType == rvrLong);

	return lastResult.longValue;
}

const char*	ExecutorRegVm::GetExecError()
{
	return execError;
}

char* ExecutorRegVm::GetVariableData(unsigned *count)
{
	if(count)
		*count = dataStack.size();

	return dataStack.data;
}

void ExecutorRegVm::BeginCallStack()
{
	currentFrame = 0;
}

unsigned ExecutorRegVm::GetNextAddress()
{
	return currentFrame == callStack.size() ? 0 : (unsigned)(callStack[currentFrame++] - codeBase);
}

void* ExecutorRegVm::GetStackStart()
{
	// TODO: what about temp stack?
	return regFileArrayBase;
}

void* ExecutorRegVm::GetStackEnd()
{
	// TODO: what about temp stack?
	return regFileLastTop;
}

void ExecutorRegVm::SetBreakFunction(void *context, unsigned (*callback)(void*, unsigned))
{
	breakFunctionContext = context;
	breakFunction = callback;
}

void ExecutorRegVm::ClearBreakpoints()
{
	// Check all instructions for break instructions
	for(unsigned i = 0; i < exLinker->exRegVmCode.size(); i++)
	{
		// nop instruction is used for breaks
		// break structure: cmdOriginal, cmdNop
		if(exLinker->exRegVmCode[i].code == rviNop)
			exLinker->exRegVmCode[i] = breakCode[exLinker->exRegVmCode[i].argument];	// replace it with original instruction
	}
	breakCode.clear();
}

bool ExecutorRegVm::AddBreakpoint(unsigned instruction, bool oneHit)
{
	if(instruction >= exLinker->exRegVmCode.size())
	{
		NULLC::SafeSprintf(execError, REGVM_ERROR_BUFFER_SIZE, "ERROR: break position out of code range");
		return false;
	}

	unsigned pos = breakCode.size();

	if(exLinker->exRegVmCode[instruction].code == rviNop)
	{
		NULLC::SafeSprintf(execError, REGVM_ERROR_BUFFER_SIZE, "ERROR: cannot set breakpoint on breakpoint");
		return false;
	}

	if(oneHit)
	{
		breakCode.push_back(exLinker->exRegVmCode[instruction]);
		exLinker->exRegVmCode[instruction].code = rviNop;
		exLinker->exRegVmCode[instruction].rB = EXEC_BREAK_ONCE;
		exLinker->exRegVmCode[instruction].argument = pos;
	}
	else
	{
		breakCode.push_back(exLinker->exRegVmCode[instruction]);
		breakCode.push_back(RegVmCmd(rviNop, 0, EXEC_BREAK_RETURN, 0, instruction + 1));

		exLinker->exRegVmCode[instruction].code = rviNop;
		exLinker->exRegVmCode[instruction].rB = EXEC_BREAK_SIGNAL;
		exLinker->exRegVmCode[instruction].argument = pos;
	}
	return true;
}

bool ExecutorRegVm::RemoveBreakpoint(unsigned instruction)
{
	if(instruction > exLinker->exRegVmCode.size())
	{
		NULLC::SafeSprintf(execError, REGVM_ERROR_BUFFER_SIZE, "ERROR: break position out of code range");
		return false;
	}

	if(exLinker->exRegVmCode[instruction].code != rviNop)
	{
		NULLC::SafeSprintf(execError, REGVM_ERROR_BUFFER_SIZE, "ERROR: there is no breakpoint at instruction %d", instruction);
		return false;
	}

	exLinker->exRegVmCode[instruction] = breakCode[exLinker->exRegVmCode[instruction].argument];
	return true;
}

void ExecutorRegVm::UpdateInstructionPointer()
{
	if(!codeBase || !callStack.size() || codeBase == &exLinker->exRegVmCode[0])
		return;

	for(unsigned i = 0; i < callStack.size(); i++)
	{
		int currentPos = int(callStack[i] - codeBase);

		assert(currentPos >= 0);

		callStack[i] = &exLinker->exRegVmCode[0] + currentPos;
	}

	codeBase = &exLinker->exRegVmCode[0];
}
