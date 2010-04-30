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

Executor::Executor(Linker* linker): exLinker(linker), exFunctions(linker->exFunctions), exTypes(linker->exTypes), breakCode(128)
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
	genParams.resize(exLinker->globalVarSize);

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
		unsigned int funcPos = ~0ul;
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
			if(cmd.flag == 0)
			{
				RUNTIME_ERROR(breakFunction == NULL, "ERROR: break function isn't set");
				fcallStack.push_back(cmdStream);
				breakFunction((unsigned int)(cmdStream - cmdStreamBase));
				fcallStack.pop_back();
				cmdStream = &breakCode[cmd.argument];
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
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			*genStackPtr = *((char*)NULL + cmd.argument + *genStackPtr);
			break;
		case cmdPushShortStk:
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			*genStackPtr = *(short*)((char*)NULL + cmd.argument + *genStackPtr);
			break;
		case cmdPushIntStk:
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			*genStackPtr = *(int*)((char*)NULL + cmd.argument + *genStackPtr);
			break;
		case cmdPushFloatStk:
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr--;
			*(double*)(genStackPtr) = (double)*(float*)((char*)NULL + cmd.argument + *(genStackPtr+1));
			break;
		case cmdPushDorLStk:
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr--;
			*(double*)(genStackPtr) = *(double*)((char*)NULL + cmd.argument + *(genStackPtr+1));
			break;
		case cmdPushCmplxStk:
		{
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			unsigned int shift = cmd.argument + *genStackPtr;
			genStackPtr++;
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				genStackPtr--;
				*genStackPtr = *(unsigned int*)((char*)NULL + shift + currShift);
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
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned char)(*genStackPtr);
			break;
		case cmdMovShortStk:
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*(unsigned short*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned short)(*genStackPtr);
			break;
		case cmdMovIntStk:
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*(int*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (int)(*genStackPtr);
			break;
		case cmdMovFloatStk:
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*(float*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (float)*(double*)(genStackPtr);
			break;
		case cmdMovDorLStk:
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			genStackPtr++;
			*(long long*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = *(long long*)(genStackPtr);
			break;
		case cmdMovCmplxStk:
		{
			RUNTIME_ERROR(*genStackPtr == 0, "ERROR: null pointer access");
			unsigned int shift = cmd.argument + *genStackPtr;
			genStackPtr++;
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				*(unsigned int*)((char*)NULL + shift + currShift) = *(genStackPtr+(currShift>>2));
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
			*(int*)(genStackPtr+1) += cmd.helper * (*genStackPtr);
			genStackPtr++;
			break;
		case cmdIndexStk:
			RUNTIME_ERROR(*genStackPtr >= *(genStackPtr+2), "ERROR: array index out of bounds");
			*(int*)(genStackPtr+2) = *(genStackPtr+1) + cmd.helper * (*genStackPtr);
			genStackPtr += 2;
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
			genStackPtr--;
			*genStackPtr = cmd.argument + paramBase * cmd.helper + (int)(intptr_t)&genParams[0];
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
				unsigned int alignOffset = (genParams.size() % 16 != 0) ? (16 - (genParams.size() % 16)) : 0;
				genParams.reserve(genParams.size() + alignOffset + paramSize);
				memcpy((char*)&genParams[genParams.size() + alignOffset], genStackPtr, paramSize);
				genStackPtr += paramSize >> 2;

				if(genParams.size() + alignOffset + paramSize >= oldSize)
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

				int alignOffset = (genParams.size() % 16 != 0) ? (16 - (genParams.size() % 16)) : 0;
				genParams.reserve(genParams.size() + alignOffset + paramSize);
				memcpy((char*)&genParams[genParams.size() + alignOffset], genStackPtr, paramSize);
				genStackPtr += paramSize >> 2;
				RUNTIME_ERROR(genStackPtr <= genStackBase+8, "ERROR: stack overflow");

				genStackPtr++;

				fcallStack.push_back(cmdStream);
				cmdStream = cmdStreamBase + exFunctions[fID].address;

				if(genParams.size() + alignOffset + paramSize >= oldSize)
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
			// Align on a 16-byte boundary
			paramBase += (paramBase % 16 != 0) ? (16 - (paramBase % 16)) : 0;
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
			ClosureCreate(&genParams[paramBase], cmd.helper, cmd.argument, (ExternFuncInfo::Upvalue*)(intptr_t)*genStackPtr);
			genStackPtr++;
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

#if defined(_MSC_VER) || (defined(__GNUC__) && !defined(__CELLOS_LV2__))
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
	#ifndef _M_X64
		__asm{ mov eax, dword ptr[stackStart] }
		__asm{ push dword ptr[eax] }
	#else
		strcpy(execError, "ERROR: no external call on x64");
		return false;
	#endif
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
	}
	genStackPtr = newStackPtr;
#ifdef __GNUC__
	asm("movl %0, %%eax"::"r"(dwordsToPop):"%eax");
	asm("leal (%esp, %eax, 0x4), %esp");
#else
	#ifndef _M_X64
		__asm mov eax, dwordsToPop
		__asm lea esp, [eax * 4 + esp]
	#else
		strcpy(execError, "ERROR: no external call on x64");
		return false;
	#endif
#endif
	return callContinue;
}

#elif defined(__CELLOS_LV2__)
// PS3 implementation
#define MAKE_FUNC_PTR_TYPE(retType, typeName) typedef retType (*typeName)(			\
	unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,	\
	double, double, double, double, double, double, double, double					\
	);
MAKE_FUNC_PTR_TYPE(void, VoidFunctionPtr)
MAKE_FUNC_PTR_TYPE(int, IntFunctionPtr)
MAKE_FUNC_PTR_TYPE(double, DoubleFunctionPtr)
MAKE_FUNC_PTR_TYPE(long long, LongFunctionPtr)

bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	unsigned int dwordsToPop = (exFunctions[funcID].bytesToPop >> 2);

	// call function
	#define R(i) *(const unsigned int*)(const void*)(genStackPtr + exFunctions[funcID].rOffsets[i])
	#define F(i) *(const double*)(const void*)(genStackPtr + exFunctions[funcID].fOffsets[i])

	switch(exFunctions[funcID].retType)
	{
	case ExternFuncInfo::RETURN_VOID:
		// cast function pointer so we can call it and call it
		((VoidFunctionPtr)exFunctions[funcID].funcPtr)(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));
		break;
	case ExternFuncInfo::RETURN_INT:
		genStackPtr--;
		// cast function pointer so we can call it and call it
		*(genStackPtr+dwordsToPop) = ((IntFunctionPtr)exFunctions[funcID].funcPtr)(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));
		break;
	case ExternFuncInfo::RETURN_DOUBLE:
		genStackPtr -= 2;
		// cast function pointer so we can call it and call it
		*(double*)(genStackPtr+dwordsToPop) = ((DoubleFunctionPtr)exFunctions[funcID].funcPtr)(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));
		break;
	case ExternFuncInfo::RETURN_LONG:
		genStackPtr -= 2;
		// cast function pointer so we can call it and call it
		*(long long*)(genStackPtr+dwordsToPop) = ((LongFunctionPtr)exFunctions[funcID].funcPtr)(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));
	}
	genStackPtr += dwordsToPop;

	#undef F
	#undef R

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
#ifdef _MSC_VER
		SafeSprintf(execResult, 64, "%I64dL", combined);
#else
		SafeSprintf(execResult, 64, "%lldL", combined);
#endif
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

char* Executor::GetVariableData()
{
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

void Executor::SetBreakFunction(void (*callback)(unsigned int))
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
	breakCode.push_back(VMCmd(cmdNop, 1, 0, instruction + 1));
	exLinker->exCode[instruction].cmd = cmdNop;
	exLinker->exCode[instruction].flag = 0;
	exLinker->exCode[instruction].argument = pos;
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
