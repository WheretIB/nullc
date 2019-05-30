#include "Executor_RegVm.h"

//#include "Executor_Common.h"

void CommonSetLinker(Linker* linker);
bool AreMembersAligned(ExternTypeInfo *lType, Linker *exLinker);
bool HasIntegerMembersInRange(ExternTypeInfo &type, unsigned fromOffset, unsigned toOffset, Linker *linker);

void	SetUnmanagableRange(char* base, unsigned int size);
int		IsPointerUnmanaged(NULLCRef ptr);
void	MarkUsedBlocks();
void	ResetGC();

#include "StdLib.h"

#define dcAllocMem NULLC::alloc
#define dcFreeMem  NULLC::dealloc

#include "../external/dyncall/dyncall.h"

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

ExecutorRegVm::ExecutorRegVm(Linker* linker): exLinker(linker), exTypes(linker->exTypes), exFunctions(linker->exFunctions)
{
	codeRunning = false;

	lastResultType = rvrError;

	symbols = NULL;

	codeBase = NULL;

	currentFrame = 0;

	tempStackBase = NULL;
	tempStackPtr = NULL;
	tempStackEnd = NULL;

	regFileBase = NULL;
	regFilePtr = NULL;
	regFileEnd = NULL;

	callContinue = true;

	dcCallVM = NULL;

	breakFunctionContext = NULL;
	breakFunction = NULL;
}

ExecutorRegVm::~ExecutorRegVm()
{
	NULLC::dealloc(tempStackBase);

	NULLC::dealloc(regFileBase);

	if(dcCallVM)
		dcFree(dcCallVM);
}

//#define RUNTIME_ERROR(test, desc)	if(test){ fcallStack.push_back(cmdStream); cmdStream = NULL; strcpy(execError, desc); break; }

void ExecutorRegVm::InitExecution()
{
	if(!exLinker->exRegVmCode.size())
	{
		strcpy(execError, "ERROR: no code to run");
		return;
	}

	callStack.clear();

	CommonSetLinker(exLinker);

	dataStack.reserve(4096);
	dataStack.clear();
	dataStack.resize((exLinker->globalVarSize + 0xf) & ~0xf);

	SetUnmanagableRange(dataStack.data, dataStack.max);

	execError[0] = 0;

	callContinue = true;

	// Add return after the last instruction to end execution of code with no return at the end
	exLinker->exRegVmCode.push_back(RegVmCmd(rviReturn, 0, rvrError, 0, 0));

	if(!tempStackBase)
	{
		tempStackBase = (unsigned*)NULLC::alloc(sizeof(unsigned) * 1024 * 16);
		tempStackEnd = tempStackBase + 1024 * 16;
	}
	tempStackPtr = tempStackBase;

	if(!regFileBase)
	{
		regFileBase = (RegVmRegister*)NULLC::alloc(sizeof(RegVmRegister) * 1024 * 16);
		regFileEnd = regFileBase + 1024 * 16;
	}
	regFilePtr = regFileBase;

	if(!dcCallVM)
	{
		dcCallVM = dcNewCallVM(4096);
		dcMode(dcCallVM, DC_CALL_C_DEFAULT);
	}
}

void ExecutorRegVm::Run(unsigned functionID, const char *arguments)
{
	if(!codeRunning || functionID == ~0u)
		InitExecution();
	codeRunning = true;

	asmOperType retType = (asmOperType)-1;

	codeBase = &exLinker->exRegVmCode[0];
	RegVmCmd *instruction = &exLinker->exRegVmCode[exLinker->regVmOffsetToGlobalCode];

	// By default error is flagged, normal return will clear it
	bool errorState = true;

	// We will know that return is global if call stack size is equal to current
	unsigned finalReturn = callStack.size();

	if(functionID != ~0u)
	{
		unsigned funcPos = ~0u;
		funcPos = exFunctions[functionID].regVmAddress;

		if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_VOID)
			retType = OTYPE_COMPLEX;
		else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_INT)
			retType = OTYPE_INT;
		else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_DOUBLE)
			retType = OTYPE_DOUBLE;
		else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_LONG)
			retType = OTYPE_LONG;

		if(funcPos == ~0u)
		{
			// TODO
			/*
			// Copy all arguments
			memcpy(genStackPtr - (exFunctions[functionID].bytesToPop >> 2), arguments, exFunctions[functionID].bytesToPop);
			genStackPtr -= (exFunctions[functionID].bytesToPop >> 2);
			*/

			// Call function
			if(RunExternalFunction(functionID, 0))
				errorState = false;

			// This will disable NULLC code execution while leaving error check and result retrieval
			instruction = NULL;
		}
		else
		{
			instruction = &exLinker->exRegVmCode[funcPos];

			// TODO
			/*// Copy from argument buffer to next stack frame
			char* oldBase = genParams.data;
			unsigned oldSize = genParams.max;

			unsigned paramSize = exFunctions[functionID].bytesToPop;
			// Keep stack frames aligned to 16 byte boundary
			unsigned alignOffset = (genParams.size() % 16 != 0) ? (16 - (genParams.size() % 16)) : 0;
			// Reserve new stack frame
			genParams.reserve(genParams.size() + alignOffset + paramSize);
			// Copy arguments to new stack frame
			memcpy((char*)(genParams.data + genParams.size() + alignOffset), arguments, paramSize);

			// Ensure that stack is resized, if needed
			if(genParams.size() + alignOffset + paramSize >= oldSize)
				ExtendParameterStack(oldBase, oldSize, cmdStream);*/
		}
	}
	else
	{
		// If global code is executed, reset all global variables
		assert(dataStack.size() >= exLinker->globalVarSize);
		memset(dataStack.data, 0, exLinker->globalVarSize);
	}

#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
	unsigned instCallCount[256];
	memset(instCallCount, 0, 256 * sizeof(unsigned));
	unsigned instExecuted = 0;
#endif

	while(instruction)
	{
		const RegVmCmd cmd = *instruction;
		instruction++;

#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
		instCallCount[cmd.code]++;
		instExecuted++;
#endif

		switch(cmd.code)
		{
		case rviNop:
			printf("unhandled rviNop\n");
			break;
		case rviLoadByte:
			printf("unhandled rviLoadByte\n");
			break;
		case rviLoadWord:
			printf("unhandled rviLoadWord\n");
			break;
		case rviLoadDword:
			printf("unhandled rviLoadDword\n");
			break;
		case rviLoadQword:
			printf("unhandled rviLoadQword\n");
			break;
		case rviLoadFloat:
			printf("unhandled rviLoadFloat\n");
			break;
		case rviLoadImm:
			printf("unhandled rviLoadImm\n");
			break;
		case rviLoadImmHigh:
			printf("unhandled rviLoadImmHigh\n");
			break;
		case rviStoreByte:
			printf("unhandled rviStoreByte\n");
			break;
		case rviStoreWord:
			printf("unhandled rviStoreWord\n");
			break;
		case rviStoreDword:
			printf("unhandled rviStoreDword\n");
			break;
		case rviStoreQword:
			printf("unhandled rviStoreQword\n");
			break;
		case rviStoreFloat:
			printf("unhandled rviStoreFloat\n");
			break;
		case rviCombinedd:
			printf("unhandled rviCombinedd\n");
			break;
		case rviMov:
			printf("unhandled rviMov\n");
			break;
		case rviDtoi:
			printf("unhandled rviDtoi\n");
			break;
		case rviDtol:
			printf("unhandled rviDtol\n");
			break;
		case rviDtof:
			printf("unhandled rviDtof\n");
			break;
		case rviItod:
			printf("unhandled rviItod\n");
			break;
		case rviLtod:
			printf("unhandled rviLtod\n");
			break;
		case rviItol:
			printf("unhandled rviItol\n");
			break;
		case rviLtoi:
			printf("unhandled rviLtoi\n");
			break;
		case rviIndex:
			printf("unhandled rviIndex\n");
			break;
		case rviGetAddr:
			printf("unhandled rviGetAddr\n");
			break;
		case rviSetRange:
			printf("unhandled rviSetRange\n");
			break;
		case rviJmp:
			printf("unhandled rviJmp\n");
			break;
		case rviJmpz:
			printf("unhandled rviJmpz\n");
			break;
		case rviJmpnz:
			printf("unhandled rviJmpnz\n");
			break;
		case rviPop:
			printf("unhandled rviPop\n");
			break;
		case rviPopq:
			printf("unhandled rviPopq\n");
			break;
		case rviPush:
			printf("unhandled rviPush\n");
			break;
		case rviPushq:
			printf("unhandled rviPushq\n");
			break;
		case rviPushImm:
			printf("unhandled rviPushImm\n");
			break;
		case rviPushImmq:
			printf("unhandled rviPushImmq\n");
			break;
		case rviCall:
			printf("unhandled rviCall\n");
			break;
		case rviCallPtr:
			printf("unhandled rviCallPtr\n");
			break;
		case rviReturn:
			printf("unhandled rviReturn\n");
			break;
		case rviPushvtop:
			printf("unhandled rviPushvtop\n");
			break;
		case rviAdd:
			printf("unhandled rviAdd\n");
			break;
		case rviSub:
			printf("unhandled rviSub\n");
			break;
		case rviMul:
			printf("unhandled rviMul\n");
			break;
		case rviDiv:
			printf("unhandled rviDiv\n");
			break;
		case rviPow:
			printf("unhandled rviPow\n");
			break;
		case rviMod:
			printf("unhandled rviMod\n");
			break;
		case rviLess:
			printf("unhandled rviLess\n");
			break;
		case rviGreater:
			printf("unhandled rviGreater\n");
			break;
		case rviLequal:
			printf("unhandled rviLequal\n");
			break;
		case rviGequal:
			printf("unhandled rviGequal\n");
			break;
		case rviEqual:
			printf("unhandled rviEqual\n");
			break;
		case rviNequal:
			printf("unhandled rviNequal\n");
			break;
		case rviShl:
			printf("unhandled rviShl\n");
			break;
		case rviShr:
			printf("unhandled rviShr\n");
			break;
		case rviBitAnd:
			printf("unhandled rviBitAnd\n");
			break;
		case rviBitOr:
			printf("unhandled rviBitOr\n");
			break;
		case rviBitXor:
			printf("unhandled rviBitXor\n");
			break;
		case rviLogXor:
			printf("unhandled rviLogXor\n");
			break;
		case rviAddl:
			printf("unhandled rviAddl\n");
			break;
		case rviSubl:
			printf("unhandled rviSubl\n");
			break;
		case rviMull:
			printf("unhandled rviMull\n");
			break;
		case rviDivl:
			printf("unhandled rviDivl\n");
			break;
		case rviPowl:
			printf("unhandled rviPowl\n");
			break;
		case rviModl:
			printf("unhandled rviModl\n");
			break;
		case rviLessl:
			printf("unhandled rviLessl\n");
			break;
		case rviGreaterl:
			printf("unhandled rviGreaterl\n");
			break;
		case rviLequall:
			printf("unhandled rviLequall\n");
			break;
		case rviGequall:
			printf("unhandled rviGequall\n");
			break;
		case rviEquall:
			printf("unhandled rviEquall\n");
			break;
		case rviNequall:
			printf("unhandled rviNequall\n");
			break;
		case rviShll:
			printf("unhandled rviShll\n");
			break;
		case rviShrl:
			printf("unhandled rviShrl\n");
			break;
		case rviBitAndl:
			printf("unhandled rviBitAndl\n");
			break;
		case rviBitOrl:
			printf("unhandled rviBitOrl\n");
			break;
		case rviBitXorl:
			printf("unhandled rviBitXorl\n");
			break;
		case rviLogXorl:
			printf("unhandled rviLogXorl\n");
			break;
		case rviAddd:
			printf("unhandled rviAddd\n");
			break;
		case rviSubd:
			printf("unhandled rviSubd\n");
			break;
		case rviMuld:
			printf("unhandled rviMuld\n");
			break;
		case rviDivd:
			printf("unhandled rviDivd\n");
			break;
		case rviPowd:
			printf("unhandled rviPowd\n");
			break;
		case rviModd:
			printf("unhandled rviModd\n");
			break;
		case rviLessd:
			printf("unhandled rviLessd\n");
			break;
		case rviGreaterd:
			printf("unhandled rviGreaterd\n");
			break;
		case rviLequald:
			printf("unhandled rviLequald\n");
			break;
		case rviGequald:
			printf("unhandled rviGequald\n");
			break;
		case rviEquald:
			printf("unhandled rviEquald\n");
			break;
		case rviNequald:
			printf("unhandled rviNequald\n");
			break;
		case rviNeg:
			printf("unhandled rviNeg\n");
			break;
		case rviNegl:
			printf("unhandled rviNegl\n");
			break;
		case rviNegd:
			printf("unhandled rviNegd\n");
			break;
		case rviBitNot:
			printf("unhandled rviBitNot\n");
			break;
		case rviBitNotl:
			printf("unhandled rviBitNotl\n");
			break;
		case rviLogNot:
			printf("unhandled rviLogNot\n");
			break;
		case rviLogNotl:
			printf("unhandled rviLogNotl\n");
			break;
		case rviConvertPtr:
			printf("unhandled rviConvertPtr\n");
			break;
		case rviCheckRet:
			printf("unhandled rviCheckRet\n");
			break;
		default:
			assert(!"unknown instruction");
		}
	}

	// If there was an execution error
	if(errorState)
	{
		// Print call stack on error, when we get to the first function
		if(!finalReturn)
		{
			char *currPos = execError + strlen(execError);
			currPos += NULLC::SafeSprintf(currPos, REGVM_ERROR_BUFFER_SIZE - int(currPos - execError), "\r\nCall stack:\r\n");

			// TODO:
			/*BeginCallStack();
			while(unsigned address = GetNextAddress())
				currPos += PrintStackFrame(address, currPos, REGVM_ERROR_BUFFER_SIZE - int(currPos - execError), false);*/
		}

		// Ascertain that execution stops when there is a chain of nullcRunFunction
		callContinue = false;
		codeRunning = false;
		return;
	}
	
	// TODO:
	/*lastResultType = retType;
	lastResultInt = *(int*)genStackPtr;
	if(genStackTop - genStackPtr > 1)
	{
		lastResultLong = vmLoadLong(genStackPtr);
		lastResultDouble = vmLoadDouble(genStackPtr);
	}

	switch(retType)
	{
	case OTYPE_DOUBLE:
	case OTYPE_LONG:
		genStackPtr += 2;
		break;
	case OTYPE_INT:
		genStackPtr++;
		break;
	case OTYPE_COMPLEX:
		break;
	}

	// If the call was started from an internal function call, a value pushed on stack for correct global return is still on stack
	if(!codeRunning && functionID != ~0u)
		genStackPtr++;*/
}

void ExecutorRegVm::Stop(const char* error)
{
	codeRunning = false;

	callContinue = false;
	NULLC::SafeSprintf(execError, REGVM_ERROR_BUFFER_SIZE, "%s", error);
}

bool ExecutorRegVm::RunExternalFunction(unsigned funcID, unsigned extraPopDW)
{
	ExternFuncInfo &func = exFunctions[funcID];

	unsigned dwordsToPop = (func.bytesToPop >> 2);

	void* fPtr = func.funcPtr;
	unsigned retType = func.retType;

	unsigned *newStackPtr = tempStackPtr - (dwordsToPop + extraPopDW);
	unsigned *stackStart = newStackPtr;

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
					}else{
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

	switch(retType)
	{
	case ExternFuncInfo::RETURN_VOID:
		dcCallVoid(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_INT:
		newStackPtr -= 1;
		*newStackPtr = dcCallInt(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_DOUBLE:
		newStackPtr -= 2;
		if(func.returnShift == 1)
			vmStoreDouble(newStackPtr, dcCallFloat(dcCallVM, fPtr));
		else
			vmStoreDouble(newStackPtr, dcCallDouble(dcCallVM, fPtr));
		break;
	case ExternFuncInfo::RETURN_LONG:
		newStackPtr -= 2;
		vmStoreLong(newStackPtr, dcCallLongLong(dcCallVM, fPtr));
		break;
	case ExternFuncInfo::RETURN_UNKNOWN:
#if defined(_WIN64)
		if(func.returnShift == 1)
		{
			newStackPtr -= 1;
			*newStackPtr = dcCallInt(dcCallVM, fPtr);
		}else{
			dcCallVoid(dcCallVM, fPtr);

			newStackPtr -= func.returnShift;
			// copy return value on top of the stack
			memcpy(newStackPtr, ret, func.returnShift * 4);
		}
#elif !defined(_M_X64)
		dcCallPointer(dcCallVM, fPtr);

		newStackPtr -= func.returnShift;
		// copy return value on top of the stack
		memcpy(newStackPtr, ret, func.returnShift * 4);
#elif defined(__aarch64__)
		if(func.returnShift > 4)
		{
			newStackPtr -= func.returnShift;

			DCcomplexbig res = dcCallComplexBig(dcCallVM, fPtr);

			memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
		}
		else
		{
			newStackPtr -= func.returnShift;

			if(!firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexdd res = dcCallComplexDD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}else if(firstQwordInteger && !secondQwordInteger){
				DCcomplexld res = dcCallComplexLD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}else if(!firstQwordInteger && secondQwordInteger){
				DCcomplexdl res = dcCallComplexDL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}else{
				DCcomplexll res = dcCallComplexLL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
		}
#else
		if(returnByPointer)
		{
			dcCallPointer(dcCallVM, fPtr);

			newStackPtr -= func.returnShift;
			// copy return value on top of the stack
			memcpy(newStackPtr, ret, func.returnShift * 4);
		}else{
			newStackPtr -= func.returnShift;

			if(!firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexdd res = dcCallComplexDD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}else if(firstQwordInteger && !secondQwordInteger){
				DCcomplexld res = dcCallComplexLD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}else if(!firstQwordInteger && secondQwordInteger){
				DCcomplexdl res = dcCallComplexDL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}else{
				DCcomplexll res = dcCallComplexLL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
		}
#endif
		break;
	}

	tempStackPtr = newStackPtr;

	return callContinue;
}

namespace
{
	char *oldBase;
	char *newBase;
	unsigned oldSize;
	unsigned newSize;
	unsigned objectName = NULLC::GetStringHash("auto ref");
	unsigned autoArrayName = NULLC::GetStringHash("auto[]");
}

#define RELOCATE_DEBUG_PRINT(...) (void)0
//#define RELOCATE_DEBUG_PRINT printf

void ExecutorRegVm::FixupPointer(char* ptr, const ExternTypeInfo& type, bool takeSubType)
{
	char *target = vmLoadPointer(ptr);

	if(target > (char*)0x00010000)
	{
		if(target >= oldBase && target < (oldBase + oldSize))
		{
			RELOCATE_DEBUG_PRINT("\tFixing from %p to %p\r\n", ptr, ptr - ExPriv::oldBase + ExPriv::newBase);

			vmStorePointer(ptr, target - oldBase + newBase);
		}
		else if(target >= newBase && target < (newBase + newSize))
		{
			const ExternTypeInfo &subType = takeSubType ? exTypes[type.subType] : type;
			(void)subType;
			RELOCATE_DEBUG_PRINT("\tStack%s pointer %s %p (at %p)\r\n", type.subType == 0 ? " opaque" : "", symbols + subType.offsetToName, target, ptr);
		}
		else
		{
			const ExternTypeInfo &subType = takeSubType ? exTypes[type.subType] : type;
			RELOCATE_DEBUG_PRINT("\tGlobal%s pointer %s %p (at %p) base %p\r\n", type.subType == 0 ? " opaque" : "", symbols + subType.offsetToName, target, ptr, NULLC::GetBasePointer(target));

			if(type.subType != 0 && NULLC::IsBasePointer(target))
			{
				markerType *marker = (markerType*)((char*)target - sizeof(markerType));
				RELOCATE_DEBUG_PRINT("\tMarker is %d", *marker);

				const uintptr_t OBJECT_VISIBLE		= 1 << 0;
				const uintptr_t OBJECT_FREED		= 1 << 1;
				const uintptr_t OBJECT_FINALIZABLE	= 1 << 2;
				const uintptr_t OBJECT_FINALIZED	= 1 << 3;
				const uintptr_t OBJECT_ARRAY		= 1 << 4;

				if(*marker & OBJECT_VISIBLE)
					RELOCATE_DEBUG_PRINT(" visible");
				if(*marker & OBJECT_FREED)
					RELOCATE_DEBUG_PRINT(" freed");
				if(*marker & OBJECT_FINALIZABLE)
					RELOCATE_DEBUG_PRINT(" finalizable");
				if(*marker & OBJECT_FINALIZED)
					RELOCATE_DEBUG_PRINT(" finalized");
				if(*marker & OBJECT_ARRAY)
					RELOCATE_DEBUG_PRINT(" array");

				RELOCATE_DEBUG_PRINT(" %s\r\n", symbols + exTypes[unsigned(*marker >> 8)].offsetToName);

				if(*marker & 1)
				{
					*marker &= ~1;
					if(type.subCat != ExternTypeInfo::CAT_NONE)
						FixupVariable(target, subType);
				}
			}
		}
	}
}

void ExecutorRegVm::FixupArray(char* ptr, const ExternTypeInfo& type)
{
	ExternTypeInfo *subType = type.nameHash == autoArrayName ? NULL : &exTypes[type.subType];
	unsigned size = type.arrSize;
	if(type.arrSize == ~0u)
	{
		// Get real array size
		size = *(int*)(ptr + NULLC_PTR_SIZE);

		// Switch pointer to array data
		char *target = vmLoadPointer(ptr);

		// If it points to stack, fix it and return
		if(target >= oldBase && target < (oldBase + oldSize))
		{
			vmStorePointer(ptr, target - oldBase + newBase);
			return;
		}

		ptr = target;

		// If uninitialized, return
		if(!ptr || ptr <= (char*)0x00010000)
			return;

		// Get base pointer
		unsigned *basePtr = (unsigned*)NULLC::GetBasePointer(ptr);
		markerType *marker = (markerType*)((char*)basePtr - sizeof(markerType));

		// If there is no base pointer or memory already marked, exit
		if(!basePtr || !(*marker & 1))
			return;

		// Mark memory as used
		*marker &= ~1;
	}
	else if(type.nameHash == autoArrayName)
	{
		NULLCAutoArray *data = (NULLCAutoArray*)ptr;

		// Get real variable type
		subType = &exTypes[data->typeID];

		// Skip uninitialized array
		if(!data->ptr)
			return;

		// If it points to stack, fix it
		if(data->ptr >= oldBase && data->ptr < (oldBase + oldSize))
			data->ptr = data->ptr - oldBase + newBase;

		// Mark target data
		FixupPointer(data->ptr, *subType, false);

		// Switch pointer to target
		ptr = data->ptr;

		// Get array size
		size = data->len;
	}

	if(!subType->pointerCount)
		return;

	switch(subType->subCat)
	{
	case ExternTypeInfo::CAT_NONE:
		break;
	case ExternTypeInfo::CAT_ARRAY:
		for(unsigned i = 0; i < size; i++, ptr += subType->size)
			FixupArray(ptr, *subType);
		break;
	case ExternTypeInfo::CAT_POINTER:
		for(unsigned i = 0; i < size; i++, ptr += subType->size)
			FixupPointer(ptr, *subType, true);
		break;
	case ExternTypeInfo::CAT_FUNCTION:
		for(unsigned i = 0; i < size; i++, ptr += subType->size)
			FixupFunction(ptr);
		break;
	case ExternTypeInfo::CAT_CLASS:
		for(unsigned i = 0; i < size; i++, ptr += subType->size)
			FixupClass(ptr, *subType);
		break;
	}
}

void ExecutorRegVm::FixupClass(char* ptr, const ExternTypeInfo& type)
{
	const ExternTypeInfo *realType = &type;

	if(type.nameHash == objectName)
	{
		// Get real variable type
		realType = &exTypes[*(int*)ptr];

		// Switch pointer to target
		char *target = vmLoadPointer(ptr + 4);

		// If it points to stack, fix it and return
		if(target >= oldBase && target < (oldBase + oldSize))
		{
			vmStorePointer(ptr + 4, target - oldBase + newBase);
			return;
		}
		ptr = target;

		// If uninitialized, return
		if(!ptr || ptr <= (char*)0x00010000)
			return;
		// Get base pointer
		unsigned *basePtr = (unsigned*)NULLC::GetBasePointer(ptr);
		markerType *marker = (markerType*)((char*)basePtr - sizeof(markerType));
		// If there is no base pointer or memory already marked, exit
		if(!basePtr || !(*marker & 1))
			return;
		// Mark memory as used
		*marker &= ~1;
		// Fixup target
		FixupVariable(target, *realType);
		// Exit
		return;
	}
	else if(type.nameHash == autoArrayName)
	{
		FixupArray(ptr, type);
		// Exit
		return;
	}

	// Get class member type list
	ExternMemberInfo *memberList = &exLinker->exTypeExtra[realType->memberOffset + realType->memberCount];
	char *str = symbols + type.offsetToName;
	const char *memberName = symbols + type.offsetToName + strlen(str) + 1;
	// Check pointer members
	for(unsigned n = 0; n < realType->pointerCount; n++)
	{
		// Get member type
		ExternTypeInfo &subType = exTypes[memberList[n].type];
		unsigned pos = memberList[n].offset;

		RELOCATE_DEBUG_PRINT("\tChecking member %s at offset %d\r\n", memberName, pos);

		// Check member
		FixupVariable(ptr + pos, subType);
		unsigned strLength = (unsigned)strlen(memberName) + 1;
		memberName += strLength;
	}
}

void ExecutorRegVm::FixupFunction(char* ptr)
{
	NULLCFuncPtr *fPtr = (NULLCFuncPtr*)ptr;

	// If there's no context, there's nothing to check
	if(!fPtr->context)
		return;

	const ExternFuncInfo &func = exFunctions[fPtr->id];

	// If function context type is valid
	if(func.contextType != ~0u)
		FixupPointer((char*)&fPtr->context, exTypes[func.contextType], true);
}

void ExecutorRegVm::FixupVariable(char* ptr, const ExternTypeInfo& type)
{
	if(!type.pointerCount)
		return;

	switch(type.subCat)
	{
	case ExternTypeInfo::CAT_NONE:
		break;
	case ExternTypeInfo::CAT_ARRAY:
		FixupArray(ptr, type);
		break;
	case ExternTypeInfo::CAT_POINTER:
		FixupPointer(ptr, type, true);
		break;
	case ExternTypeInfo::CAT_FUNCTION:
		FixupFunction(ptr);
		break;
	case ExternTypeInfo::CAT_CLASS:
		FixupClass(ptr, type);
		break;
	}
}

bool ExecutorRegVm::ExtendParameterStack(char* oldBase, unsigned oldSize, RegVmCmd *current)
{
	RELOCATE_DEBUG_PRINT("Old base: %p-%p\r\n", oldBase, oldBase + oldSize);
	RELOCATE_DEBUG_PRINT("New base: %p-%p\r\n", genParams.data, genParams.data + genParams.max);

	SetUnmanagableRange(dataStack.data, dataStack.max);

	NULLC::MarkMemory(1);

	oldBase = oldBase;
	newBase = dataStack.data;
	oldSize = oldSize;
	newSize = dataStack.max;

	symbols = exLinker->exSymbols.data;

	ExternVarInfo *vars = exLinker->exVariables.data;
	ExternTypeInfo *types = exLinker->exTypes.data;
	// Fix global variables
	for(unsigned i = 0; i < exLinker->exVariables.size(); i++)
	{
		ExternVarInfo &varInfo = vars[i];

		RELOCATE_DEBUG_PRINT("Global variable %s (with offset of %d)\r\n", symbols + varInfo.offsetToName, varInfo.offset);

		FixupVariable(dataStack.data + varInfo.offset, types[varInfo.type]);
	}

	int offset = exLinker->globalVarSize;
	int n = 0;

	callStack.push_back(RegVmCallFrame(current, dataStack.size(), unsigned(regFilePtr - regFileBase)));

	// Fixup local variables
	for(; n < (int)callStack.size(); n++)
	{
		int address = int(callStack[n].instruction - codeBase);
		int funcID = -1;

		for(unsigned i = 0; i < exFunctions.size(); i++)
		{
			if(address >= exFunctions[i].vmAddress && address < (exFunctions[i].vmAddress + exFunctions[i].vmCodeSize))
				funcID = i;
		}

		if(funcID != -1)
		{
			ExternFuncInfo &funcInfo = exFunctions[funcID];

			int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;
			RELOCATE_DEBUG_PRINT("In function %s (with offset of %d)\r\n", symbols + funcInfo.offsetToName, alignOffset);
			offset += alignOffset;

			unsigned offsetToNextFrame = funcInfo.bytesToPop;
			// Check every function local
			for(unsigned i = 0; i < funcInfo.localCount; i++)
			{
				// Get information about local
				ExternLocalInfo &lInfo = exLinker->exLocals[funcInfo.offsetToFirstLocal + i];

				RELOCATE_DEBUG_PRINT("Local %s %s (with offset of %d+%d)\r\n", symbols + types[lInfo.type].offsetToName, symbols + lInfo.offsetToName, offset, lInfo.offset);
				FixupVariable(dataStack.data + offset + lInfo.offset, types[lInfo.type]);
				if(lInfo.offset + lInfo.size > offsetToNextFrame)
					offsetToNextFrame = lInfo.offset + lInfo.size;
			}

			if(funcInfo.contextType != ~0u)
			{
				RELOCATE_DEBUG_PRINT("Local %s $context (with offset of %d+%d)\r\n", symbols + types[funcInfo.contextType].offsetToName, offset, funcInfo.bytesToPop - NULLC_PTR_SIZE);
				char *ptr = dataStack.data + offset + funcInfo.bytesToPop - NULLC_PTR_SIZE;

				// Fixup pointer itself
				char *target = vmLoadPointer(ptr);

				if(target >= oldBase && target < (oldBase + oldSize))
				{
					RELOCATE_DEBUG_PRINT("\tFixing from %p to %p\r\n", ptr, ptr - ExPriv::oldBase + ExPriv::newBase);
					vmStorePointer(ptr, target - oldBase + newBase);
				}

				// Fixup what it was pointing to
				if(char *fixedTarget = vmLoadPointer(ptr))
					FixupVariable(fixedTarget, types[funcInfo.contextType]);
			}

			offset += offsetToNextFrame;
			RELOCATE_DEBUG_PRINT("Moving offset to next frame by %d bytes\r\n", offsetToNextFrame);
		}
	}

	callStack.pop_back();

	return true;
}

const char* ExecutorRegVm::GetResult()
{
	// TODO:
	/*if(!codeRunning && genStackTop - genStackPtr > (int(lastResultType) == -1 ? 1 : 0))
	{
		NULLC::SafeSprintf(execResult, 64, "There is more than one value on the stack (%d)", int(genStackTop - genStackPtr));
		return execResult;
	}

	switch(lastResultType)
	{
	case OTYPE_DOUBLE:
		NULLC::SafeSprintf(execResult, 64, "%f", lastResultDouble);
		break;
	case OTYPE_LONG:
		NULLC::SafeSprintf(execResult, 64, "%lldL", lastResultLong);
		break;
	case OTYPE_INT:
		NULLC::SafeSprintf(execResult, 64, "%d", lastResultInt);
		break;
	default:
		NULLC::SafeSprintf(execResult, 64, "no return value");
		break;
	}*/

	return execResult;
}

int ExecutorRegVm::GetResultInt()
{
	assert(lastResultType == rvrInt);
	assert(lastResult.activeType == rvrInt); // Temp check

	return lastResult.intValue;
}

double ExecutorRegVm::GetResultDouble()
{
	assert(lastResultType == rvrDouble);
	assert(lastResult.activeType == rvrDouble); // Temp check

	return lastResult.doubleValue;
}

long long ExecutorRegVm::GetResultLong()
{
	assert(lastResultType == rvrLong);
	assert(lastResult.activeType == rvrLong); // Temp check

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
	return currentFrame == callStack.size() ? 0 : (unsigned)(callStack[currentFrame++].instruction - codeBase);
}

void* ExecutorRegVm::GetStackStart()
{
	// TODO: what about temp stack?
	return regFileBase;
}

void* ExecutorRegVm::GetStackEnd()
{
	// TODO: what about temp stack?
	return regFilePtr;
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
		int currentPos = int(callStack[i].instruction - codeBase);

		assert(currentPos >= 0);

		callStack[i].instruction = &exLinker->exRegVmCode[0] + currentPos;
	}

	codeBase = &exLinker->exRegVmCode[0];
}
