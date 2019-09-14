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

#define REGVM_DEBUG(x)
//#define REGVM_DEBUG(x) x

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

				REGVM_DEBUG(regFilePtr[rvrrGlobals].activeType = rvrPointer);
				REGVM_DEBUG(regFilePtr[rvrrFrame].activeType = rvrPointer);
				REGVM_DEBUG(regFilePtr[rvrrConstants].activeType = rvrPointer);

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

		REGVM_DEBUG(regFilePtr[rvrrGlobals].activeType = rvrPointer);
		REGVM_DEBUG(regFilePtr[rvrrFrame].activeType = rvrPointer);
		REGVM_DEBUG(regFilePtr[rvrrConstants].activeType = rvrPointer);

		regFilePtr[rvrrGlobals].ptrValue = uintptr_t(dataStack.data);
		regFilePtr[rvrrFrame].ptrValue = uintptr_t(dataStack.data);
		regFilePtr[rvrrConstants].ptrValue = uintptr_t(exLinker->exRegVmConstants.data);

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
		REGVM_DEBUG(lastResult.activeType = rvrInt);

		lastResult.intValue = tempStackPtr[0];
		break;
	case rvrDouble:
		REGVM_DEBUG(lastResult.activeType = rvrDouble);

		memcpy(&lastResult.doubleValue, tempStackPtr, sizeof(double));
		break;
	case rvrLong:
		REGVM_DEBUG(lastResult.activeType = rvrLong);

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

RegVmReturnType ExecutorRegVm::RunCode(RegVmCmd *instruction, RegVmRegister * const regFilePtr, unsigned *tempStackPtr, ExecutorRegVm *rvm, RegVmCmd *codeBase)
{
	(void)codeBase;

	for(;;)
	{
		const RegVmCmd &cmd = *instruction;

#if defined(NULLC_REG_VM_PROFILE_INSTRUCTIONS)
		unsigned *executions = rvm->exLinker->exRegVmExecCount.data;

		executions[unsigned(instruction - codeBase)]++;

		unsigned *instructionExecutions = rvm->exLinker->exRegVmInstructionExecCount.data;

		instructionExecutions[cmd.code]++;
#endif

		switch(cmd.code)
		{
		case rviNop:
			instruction = rvm->ExecNop(cmd, instruction, regFilePtr);

			if(!instruction)
				return rvrError;
			break;
		case rviLoadByte:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = *(char*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviLoadWord:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = *(short*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviLoadDword:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviLoadLong:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = vmLoadLong((void*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));
			instruction++;
			break;
		case rviLoadFloat:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = *(float*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviLoadDouble:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviLoadImm:
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = cmd.argument;
			instruction++;
			break;
		case rviLoadImmLong:
			REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = ((uint64_t)cmd.argument << 32ull) | (unsigned)regFilePtr[cmd.rA].intValue;
			instruction++;
			break;
		case rviLoadImmDouble:
			REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			{
				uint64_t bits = ((uint64_t)cmd.argument << 32ull) | (unsigned)regFilePtr[cmd.rA].intValue;

				memcpy(&regFilePtr[cmd.rA].doubleValue, &bits, sizeof(double));
			}
			instruction++;
			break;
		case rviStoreByte:
			REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(char*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = (char)regFilePtr[cmd.rA].intValue;
			instruction++;
			break;
		case rviStoreWord:
			REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(short*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = (short)regFilePtr[cmd.rA].intValue;
			instruction++;
			break;
		case rviStoreDword:
			REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = regFilePtr[cmd.rA].intValue;
			instruction++;
			break;
		case rviStoreLong:
			REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = regFilePtr[cmd.rA].longValue;
			instruction++;
			break;
		case rviStoreFloat:
			REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(float*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = (float)regFilePtr[cmd.rA].doubleValue;
			instruction++;
			break;
		case rviStoreDouble:
			REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			*(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) = regFilePtr[cmd.rA].doubleValue;
			instruction++;
			break;
		case rviCombinedd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = ((uint64_t)regFilePtr[cmd.rC].intValue << 32ull) | (unsigned)regFilePtr[cmd.rB].intValue;
			instruction++;
			break;
		case rviBreakupdd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);
			REGVM_DEBUG(regFilePtr[cmd.rB].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = (int)(regFilePtr[cmd.rC].longValue >> 32ull);
			regFilePtr[cmd.rB].intValue = (int)(regFilePtr[cmd.rC].longValue);
			instruction++;
			break;
		case rviMov:
			memcpy(&regFilePtr[cmd.rA], &regFilePtr[cmd.rC], sizeof(RegVmRegister));
			instruction++;
			break;
		case rviDtoi:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = (int)regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviDtol:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = (long long)regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviDtof:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			{
				float tmp = (float)regFilePtr[cmd.rC].doubleValue;

				memcpy(&regFilePtr[cmd.rA].intValue, &tmp, sizeof(float));
			}
			instruction++;
			break;
		case rviItod:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			regFilePtr[cmd.rA].doubleValue = (double)regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviLtod:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			regFilePtr[cmd.rA].doubleValue = (double)regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviItol:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = (long long)regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviLtoi:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = (int)regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviIndex:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(assert(regFilePtr[(cmd.argument >> 16) & 0xff].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrPointer);

			if(regFilePtr[cmd.rB].intValue >= regFilePtr[(cmd.argument >> 16) & 0xff].intValue)
				return rvm->ExecError(instruction, "ERROR: array index out of bounds");

			regFilePtr[cmd.rA].ptrValue = regFilePtr[cmd.rC].ptrValue + regFilePtr[cmd.rB].intValue * (cmd.argument & 0xffff);
			instruction++;
			break;
		case rviGetAddr:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrPointer);

			regFilePtr[cmd.rA].ptrValue = regFilePtr[cmd.rC].ptrValue + cmd.argument;
			instruction++;
			break;
		case rviSetRange:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));

			switch(RegVmSetRangeType(cmd.rB))
			{
			case rvsrDouble:
				REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrDouble));

				for(unsigned i = 0; i < cmd.argument; i++)
					((double*)regFilePtr[cmd.rC].ptrValue)[i] = regFilePtr[cmd.rA].doubleValue;
				break;
			case rvsrFloat:
				REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrDouble));

				for(unsigned i = 0; i < cmd.argument; i++)
					((float*)regFilePtr[cmd.rC].ptrValue)[i] = (float)regFilePtr[cmd.rA].doubleValue;
				break;
			case rvsrLong:
				REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrLong));

				for(unsigned i = 0; i < cmd.argument; i++)
					((long long*)regFilePtr[cmd.rC].ptrValue)[i] = regFilePtr[cmd.rA].longValue;
				break;
			case rvsrInt:
				REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrInt));

				for(unsigned i = 0; i < cmd.argument; i++)
					((int*)regFilePtr[cmd.rC].ptrValue)[i] = regFilePtr[cmd.rA].intValue;
				break;
			case rvsrShort:
				REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrInt));

				for(unsigned i = 0; i < cmd.argument; i++)
					((short*)regFilePtr[cmd.rC].ptrValue)[i] = (short)regFilePtr[cmd.rA].intValue;
				break;
			case rvsrChar:
				REGVM_DEBUG(assert(regFilePtr[cmd.rA].activeType == rvrInt));

				for(unsigned i = 0; i < cmd.argument; i++)
					((char*)regFilePtr[cmd.rC].ptrValue)[i] = (char)regFilePtr[cmd.rA].intValue;
				break;
			default:
				assert(!"unknown type");
			}
			instruction++;
			break;
		case rviJmp:
#ifdef _M_X64
			instruction = codeBase + cmd.argument - 1;
#else
			instruction = rvm->codeBase + cmd.argument - 1;
#endif
			instruction++;
			break;
		case rviJmpz:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));

			if(regFilePtr[cmd.rC].intValue == 0)
			{
#ifdef _M_X64
				instruction = codeBase + cmd.argument - 1;
#else
				instruction = rvm->codeBase + cmd.argument - 1;
#endif
			}
			instruction++;
			break;
		case rviJmpnz:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));

			if(regFilePtr[cmd.rC].intValue != 0)
			{
#ifdef _M_X64
				instruction = codeBase + cmd.argument - 1;
#else
				instruction = rvm->codeBase + cmd.argument - 1;
#endif
			}
			instruction++;
			break;
		case rviPop:
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = tempStackPtr[cmd.argument];
			instruction++;
			break;
		case rviPopq:
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = vmLoadLong(&tempStackPtr[cmd.argument]);
			instruction++;
			break;
		case rviPush:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));

			*tempStackPtr++ = regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviPushQword:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong || regFilePtr[cmd.rC].activeType == rvrDouble));

			memcpy(tempStackPtr, &regFilePtr[cmd.rC].longValue, sizeof(long long));
			tempStackPtr += 2;
			instruction++;
			break;
		case rviPushImm:
			*tempStackPtr = cmd.argument;
			tempStackPtr += 1;
			instruction++;
			break;
		case rviPushImmq:
			vmStoreLong(tempStackPtr, cmd.argument);
			tempStackPtr += 2;
			instruction++;
			break;
		case rviCall:
			tempStackPtr = rvm->ExecCall(cmd.rA, cmd.rB, cmd.argument, instruction, regFilePtr, tempStackPtr);

			if(!tempStackPtr)
				return rvrError;

			instruction++;
			break;
		case rviCallPtr:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));

			if(regFilePtr[cmd.rC].intValue == 0)
				return rvm->ExecError(instruction, "ERROR: invalid function pointer");

			tempStackPtr = rvm->ExecCall(cmd.rA, cmd.rB, regFilePtr[cmd.rC].intValue, instruction, regFilePtr, tempStackPtr);

			if(!tempStackPtr)
				return rvrError;

			instruction++;
			break;
		case rviReturn:
			return rvm->ExecReturn(cmd, instruction);
		case rviAdd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue + regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviAddImm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue + (int)cmd.argument;
			instruction++;
			break;
		case rviSub:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue - regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviMul:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue * regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviDiv:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if(regFilePtr[cmd.rC].intValue == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue / regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviAddm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue + *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviSubm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue - *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviMulm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue * *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviDivm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			if(*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue / *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviPow:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = vmIntPow(regFilePtr[cmd.rC].intValue, regFilePtr[cmd.rB].intValue);
			instruction++;
			break;
		case rviMod:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if(regFilePtr[cmd.rC].intValue == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue % regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviLess:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue < regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviGreater:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue > regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviLequal:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue <= regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviGequal:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue >= regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviEqual:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue == regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviNequal:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue != regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviShl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue << regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviShr:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue >> regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviShlm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue << *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviShrm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue >> *(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviBitAnd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue & regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviBitOr:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue | regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviBitXor:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].intValue ^ regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviLogXor:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = (regFilePtr[cmd.rB].intValue != 0) != (regFilePtr[cmd.rC].intValue != 0);
			instruction++;
			break;
		case rviAddl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue + regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviAddImml:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue + (int)cmd.argument;
			instruction++;
			break;
		case rviSubl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue - regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviMull:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue * regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviDivl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			if(regFilePtr[cmd.rC].longValue == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue / regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviAddlm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue + *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviSublm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue - *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviMullm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue * *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviDivlm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			if(*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument) == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue / *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviPowl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = vmLongPow(regFilePtr[cmd.rC].longValue, regFilePtr[cmd.rB].longValue);
			instruction++;
			break;
		case rviModl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			if(regFilePtr[cmd.rC].longValue == 0)
				return rvm->ExecError(instruction, "ERROR: integer division by zero");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue % regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviLessl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue < regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviGreaterl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue > regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviLequall:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue <= regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviGequall:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue >= regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviEquall:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue == regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviNequall:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].longValue != regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviShll:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue << regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviShrl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue >> regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviShllm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue << *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviShrlm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue >> *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviBitAndl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue & regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviBitOrl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue | regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviBitXorl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue ^ regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviLogXorl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrLong));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = (regFilePtr[cmd.rB].longValue != 0) != (regFilePtr[cmd.rC].longValue != 0);
			instruction++;
			break;
		case rviAddd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue + regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviSubd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue - regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviMuld:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue * regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviDivd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue / regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviAdddm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue + *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviSubdm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue - *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviMuldm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue * *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviDivdm:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			if((uintptr_t)regFilePtr[cmd.rC].ptrValue < 0x00010000)
				return rvm->ExecError(instruction, "ERROR: null pointer access");

			regFilePtr[cmd.rA].doubleValue = regFilePtr[cmd.rB].doubleValue / *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);
			instruction++;
			break;
		case rviPowd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			regFilePtr[cmd.rA].doubleValue = pow(regFilePtr[cmd.rB].doubleValue, regFilePtr[cmd.rC].doubleValue);
			instruction++;
			break;
		case rviModd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			regFilePtr[cmd.rA].doubleValue = fmod(regFilePtr[cmd.rB].doubleValue, regFilePtr[cmd.rC].doubleValue);
			instruction++;
			break;
		case rviLessd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue < regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviGreaterd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue > regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviLequald:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue <= regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviGequald:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue >= regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviEquald:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue == regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviNequald:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrDouble));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = regFilePtr[cmd.rB].doubleValue != regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviNeg:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = -regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviNegl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = -regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviNegd:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrDouble));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrDouble);

			regFilePtr[cmd.rA].doubleValue = -regFilePtr[cmd.rC].doubleValue;
			instruction++;
			break;
		case rviBitNot:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = ~regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviBitNotl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrLong);

			regFilePtr[cmd.rA].longValue = ~regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviLogNot:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrInt));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = !regFilePtr[cmd.rC].intValue;
			instruction++;
			break;
		case rviLogNotl:
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrLong));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrInt);

			regFilePtr[cmd.rA].intValue = !regFilePtr[cmd.rC].longValue;
			instruction++;
			break;
		case rviConvertPtr:
			REGVM_DEBUG(assert(regFilePtr[cmd.rB].activeType == rvrInt));
			REGVM_DEBUG(assert(regFilePtr[cmd.rC].activeType == rvrPointer));
			REGVM_DEBUG(regFilePtr[cmd.rA].activeType = rvrPointer);

			if(!rvm->ExecConvertPtr(cmd, instruction, regFilePtr))
				return rvrError;

			instruction++;
			break;
		case rviCheckRet:
			rvm->ExecCheckedReturn(cmd, regFilePtr, tempStackPtr);
			instruction++;
			break;
		default:
#if defined(_MSC_VER)
			__assume(false);
#elif defined(__GNUC__)
			__builtin_unreachable();
#endif
		}
	}

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

unsigned* ExecutorRegVm::ExecCall(unsigned char resultReg, unsigned char resultType, unsigned functionId, RegVmCmd * const instruction, RegVmRegister * const regFilePtr, unsigned *tempStackPtr)
{
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
			REGVM_DEBUG(regFilePtr[resultReg].activeType = rvrDouble);

			memcpy(&regFilePtr[resultReg].doubleValue, tempStackPtr, sizeof(double));
			break;
		case rvrLong:
			REGVM_DEBUG(regFilePtr[resultReg].activeType = rvrLong);

			memcpy(&regFilePtr[resultReg].longValue, tempStackPtr, sizeof(long long));
			break;
		case rvrInt:
			REGVM_DEBUG(regFilePtr[resultReg].activeType = rvrInt);

			regFilePtr[resultReg].intValue = *tempStackPtr;
			break;
		default:
			break;
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

	REGVM_DEBUG(regFileTop[rvrrGlobals].activeType = rvrPointer);
	REGVM_DEBUG(regFileTop[rvrrFrame].activeType = rvrPointer);
	REGVM_DEBUG(regFileTop[rvrrConstants].activeType = rvrPointer);

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
		REGVM_DEBUG(regFilePtr[resultReg].activeType = rvrDouble);

		memcpy(&regFilePtr[resultReg].doubleValue, tempStackPtr, sizeof(double));
		break;
	case rvrLong:
		REGVM_DEBUG(regFilePtr[resultReg].activeType = rvrLong);

		memcpy(&regFilePtr[resultReg].longValue, tempStackPtr, sizeof(long long));
		break;
	case rvrInt:
		REGVM_DEBUG(regFilePtr[resultReg].activeType = rvrInt);

		regFilePtr[resultReg].intValue = *tempStackPtr;
		break;
	default:
		break;
	}

	return tempStackPtr;
}

RegVmReturnType ExecutorRegVm::ExecReturn(const RegVmCmd cmd, RegVmCmd * const instruction)
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

void ExecutorRegVm::ExecCheckedReturn(const RegVmCmd cmd, RegVmRegister * const regFilePtr, unsigned * const tempStackPtr)
{
	uintptr_t frameBase = regFilePtr[rvrrFrame].ptrValue;
	uintptr_t frameEnd = regFilePtr[rvrrGlobals].ptrValue + dataStack.size();

	ExternTypeInfo &type = exLinker->exTypes[cmd.argument];

	char *returnValuePtr = (char*)tempStackPtr - type.size;

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
	REGVM_DEBUG(assert(lastResult.activeType == rvrInt));

	return lastResult.intValue;
}

double ExecutorRegVm::GetResultDouble()
{
	assert(lastResultType == rvrDouble);
	REGVM_DEBUG(assert(lastResult.activeType == rvrDouble));

	return lastResult.doubleValue;
}

long long ExecutorRegVm::GetResultLong()
{
	assert(lastResultType == rvrLong);
	REGVM_DEBUG(assert(lastResult.activeType == rvrLong));

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
