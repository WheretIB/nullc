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

Executor::Executor(Linker* linker): exLinker(linker), exFunctions(linker->exFunctions), exTypes(linker->exTypes)
{
	DBG(executeLog = fopen("log.txt", "wb"));

	genStackBase = NULL;
	genStackPtr = NULL;
	genStackTop = NULL;
}

Executor::~Executor()
{
	DBG(fclose(executeLog));

	delete[] genStackBase;
}

#define genStackSize (genStackTop-genStackPtr)

void Executor::Run(const char* funcName)
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

	// If global code is executed, reset all global variables
	if(!funcName)
		memset(&genParams[0], 0, exLinker->globalVarSize);

	execError[0] = 0;
	callContinue = true;

	retType = (asmOperType)-1;

	unsigned int funcPos = ~0ul;
	int functionID = -1;
	if(funcName)
	{
		unsigned int fnameHash = GetStringHash(funcName);
		for(int i = (int)exFunctions.size()-1; i >= 0; i--)
		{
			if(exFunctions[i].nameHash == fnameHash)
			{
				functionID = i;
				funcPos = exFunctions[i].address;
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
				break;
			}
		}
		if(funcPos == ~0ul)
		{
			SafeSprintf(execError, ERROR_BUFFER_SIZE, "ERROR: starting function %s not found", funcName);
			return;
		}
	}
	runningFunction = functionID;

	// General stack
	if(!genStackBase)
	{
		genStackBase = new unsigned int[2048];		// Should be enough, but can grow
		genStackTop = genStackBase + 2048;
	}
	genStackPtr = genStackTop - 1;

#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
	unsigned int insCallCount[255];
	memset(insCallCount, 0, 255*4);
	unsigned int insExecuted = 0;
#endif
	VMCmd *cmdStreamBase = cmdBase = &exLinker->exCode[0];
	VMCmd *cmdStream = &exLinker->exCode[exLinker->offsetToGlobalCode];
	VMCmd *cmdStreamEnd = &exLinker->exCode[0]+exLinker->exCode.size();
#define cmdStreamPos (cmdStream-cmdStreamBase)

	if(funcName)
		cmdStream = &exLinker->exCode[funcPos];

	paramBase = 0;

	if(!funcName)
	{
		genStackPtr--;
		*genStackPtr = 0;
	}

#define RUNTIME_ERROR(test, desc)	if(test){ cmdStreamEnd = NULL; strcpy(execError, desc); break; }

	while(cmdStream < cmdStreamEnd)
	{
		const VMCmd &cmd = *cmdStream;
		//const unsigned int argument = cmd.argument;
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
			assert(!"cmdNop looks like error");
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
			genStackPtr--;
			*genStackPtr = cmd.argument;
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

			if(fAddress == -1)
			{
				if(!RunExternalFunction(cmd.argument, 0))
					cmdStreamEnd = NULL;
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
			RUNTIME_ERROR(fID == 0, "ERROR: Invalid function pointer");

			if(exFunctions[fID].address == -1)
			{
				if(!RunExternalFunction(fID, 1))
					cmdStreamEnd = NULL;
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
			RUNTIME_ERROR(cmd.flag & bitRetError, "ERROR: function didn't return a value");
			{
				unsigned int *retValue = genStackPtr;
				genStackPtr = (unsigned int*)((char*)(genStackPtr) + cmd.argument);

				genParams.shrink(paramBase);
				paramBase = *genStackPtr;
				genStackPtr++;

				genStackPtr = (unsigned int*)((char*)(genStackPtr) - cmd.argument);
				memmove(genStackPtr, retValue, cmd.argument);
			}
			if(fcallStack.size() == 0)
			{
				if(retType == -1)
					retType = (asmOperType)(int)cmd.flag;
				cmdStream = cmdStreamEnd;
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
				strcpy(execError, "ERROR: Integer division by zero");
				cmdStreamEnd = NULL;
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
				strcpy(execError, "ERROR: Integer division by zero");
				cmdStreamEnd = NULL;
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
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) && *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdLogOr:
			*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) || *(int*)(genStackPtr);
			genStackPtr++;
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
				strcpy(execError, "ERROR: Integer division by zero");
				cmdStreamEnd = NULL;
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
				strcpy(execError, "ERROR: Integer division by zero");
				cmdStreamEnd = NULL;
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
			*(int*)(genStackPtr+3) = *(long long*)(genStackPtr+2) && *(long long*)(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdLogOrL:
			*(int*)(genStackPtr+3) = *(long long*)(genStackPtr+2) || *(long long*)(genStackPtr);
			genStackPtr += 3;
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
			CloseUpvalues(&genParams[paramBase], cmd.helper, cmd.argument);
			break;

		case cmdConvertPtr:
			if(*genStackPtr != cmd.argument)
			{
				SafeSprintf(execError, 1024, "ERROR: Cannot convert from %s ref to %s ref", &exLinker->exSymbols[exLinker->exTypes[*genStackPtr].offsetToName], &exLinker->exSymbols[exLinker->exTypes[cmd.argument].offsetToName]);
				cmdStreamEnd = NULL;
			}
			genStackPtr++;
			break;
		}

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
		fprintf(executeLog, ";\r\n");
		fflush(executeLog);
#endif
	}
	// Print call stack on error
	if(cmdStreamEnd == NULL)
	{
		char *currPos = execError + strlen(execError);

		currPos += SafeSprintf(currPos, ERROR_BUFFER_SIZE - int(currPos - execError), "\r\nCall stack:\r\n");
		int address = int(cmdStream - cmdStreamBase);
		do
		{
			currPos += PrintStackFrame(address, currPos, ERROR_BUFFER_SIZE - int(currPos - execError));

			if(!fcallStack.size())
				break;
			address = int(fcallStack.back() - cmdStreamBase);
			fcallStack.pop_back();
		}while(true);
	}
}

void Executor::Stop(const char* error)
{
	callContinue = false;
	SafeSprintf(execError, ERROR_BUFFER_SIZE, error);
}

#if defined(_MSC_VER) || (defined(__GNUC__) && !defined(__CELLOS_LV2__))
// X86 implementation
bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	unsigned int bytesToPop = exFunctions[funcID].bytesToPop;
	void* fPtr = exFunctions[funcID].funcPtr;
	unsigned int retType = exFunctions[funcID].retType;

	unsigned int *stackStart = (genStackPtr + (bytesToPop >> 2) - 1);
	for(unsigned int i = 0; i < (bytesToPop >> 2); i++)
	{
#ifdef __GNUC__
		asm("movl %0, %%eax"::"r"(stackStart):"%eax");
		asm("pushl (%eax)");
#else
	#ifndef _M_X64
		__asm{ mov eax, dword ptr[stackStart] }
		__asm{ push dword ptr[eax] }
	#else
		strcpy(execError, "ERROR: No external call on x64");
		return false;
	#endif
#endif
		stackStart--;
	}
	genStackPtr += (bytesToPop >> 2) + extraPopDW;

	switch(retType)
	{
	case ExternFuncInfo::RETURN_VOID:
		((void (*)())fPtr)();
		break;
	case ExternFuncInfo::RETURN_INT:
		genStackPtr--;
		*genStackPtr = ((int (*)())fPtr)();
		break;
	case ExternFuncInfo::RETURN_DOUBLE:
		genStackPtr -= 2;
		*(double*)genStackPtr = ((double (*)())fPtr)();
		break;
	case ExternFuncInfo::RETURN_LONG:
		genStackPtr -= 2;
		*(long long*)genStackPtr = ((long long (*)())fPtr)();
	}
#ifdef __GNUC__
	asm("addl %0, %%esp"::"r"(bytesToPop):"%esp");
#else
	#ifndef _M_X64
		__asm add esp, bytesToPop;
	#else
		strcpy(execError, "ERROR: No external call on x64");
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
	strcpy(execError, "ERROR: External function call failed");
	return false;
}
#endif

namespace ExPriv
{
	char *oldBase;
	char *newBase;
	unsigned int oldSize;
}
//int calls = 0, fixed = 0;
void Executor::FixupPointer(char*& ptr, const ExternTypeInfo& type)
{
	//calls++;
	if(ptr > (char*)0x00010000)
	{
		if(ptr >= ExPriv::oldBase && ptr < (ExPriv::oldBase + ExPriv::oldSize))
		{
			//fixed++;
//			printf("\tFixing from %p to %p\r\n", ptr, ptr - ExPriv::oldBase + ExPriv::newBase);
			ptr = ptr - ExPriv::oldBase + ExPriv::newBase;
		}else{
			ExternTypeInfo &subType = exTypes[type.subType];
//			printf("\tGlobal pointer %s %p\r\n", symbols + subType.offsetToName, ptr);
			unsigned int *marker = (unsigned int*)(ptr - 4);
//			printf("\tMarker is %d\r\n", *marker);
			if(*marker == 42)
			{
				*marker = 0;
				switch(subType.subCat)
				{
				case ExternTypeInfo::CAT_ARRAY:
//					printf("\tChecking pointer to %s [at %p]\r\n", symbols + subType.offsetToName, ptr);
					FixupArray((unsigned int)(ptr - ExPriv::newBase), subType);
					break;
				case ExternTypeInfo::CAT_POINTER:
//					printf("\tChecking pointer to %s [at %d]\r\n", symbols + subType.offsetToName, ptr);
					FixupPointer(*(char**)(ptr), subType);
					break;
				case ExternTypeInfo::CAT_CLASS:
//					printf("\tChecking pointer to %s [at %d]\r\n", symbols + subType.offsetToName, ptr);
					FixupClass((unsigned int)(ptr - ExPriv::newBase), subType);
					break;
				default:
//					printf("\tSkipping pointer to %s [at %d]\r\n", symbols + subType.offsetToName, ptr);
					break;
				}
			}
		}
	}
}

void Executor::FixupArray(unsigned int offset, const ExternTypeInfo& type)
{
	ExternTypeInfo &subType = exTypes[type.subType];
	if(type.arrSize == -1)
	{
		FixupPointer(*(char**)(ExPriv::newBase + offset), subType);
		return;
	}
	switch(subType.subCat)
	{
	case ExternTypeInfo::CAT_ARRAY:
//		printf("Checking array of %s [at %d]\r\n", symbols + subType.offsetToName, offset);
		for(unsigned int i = 0; i < type.arrSize; i++, offset += subType.size)
			FixupArray(offset, subType);
		break;
	case ExternTypeInfo::CAT_POINTER:
//		printf("Checking array of %s [at %d]\r\n", symbols + subType.offsetToName, offset);
		for(unsigned int i = 0; i < type.arrSize; i++, offset += subType.size)
			FixupPointer(*(char**)(ExPriv::newBase + offset), subType);
		break;
	case ExternTypeInfo::CAT_CLASS:
//		printf("Checking array of %s [at %d]\r\n", symbols + subType.offsetToName, offset);
		for(unsigned int i = 0; i < type.arrSize; i++, offset += subType.size)
			FixupClass(offset, subType);
		break;
	default:
//		printf("Skipping array of %s [at %d]\r\n", symbols + subType.offsetToName, offset);
		break;
	}
}

void Executor::FixupClass(unsigned int offset, const ExternTypeInfo& type)
{
	unsigned int *memberList = &exLinker->exTypeExtra[0];
	char *str = symbols + type.offsetToName;
	const char *memberName = symbols + type.offsetToName + strlen(str) + 1;
	for(unsigned int n = 0; n < type.memberCount; n++)
	{
		unsigned int strLength = (unsigned int)strlen(memberName) + 1;
		ExternTypeInfo &subType = exTypes[memberList[type.memberOffset + n]];
		switch(subType.subCat)
		{
		case ExternTypeInfo::CAT_ARRAY:
//			printf("Checking member %s %s [at %d]\r\n", symbols + subType.offsetToName, memberName, offset);
			FixupArray(offset, subType);
			break;
		case ExternTypeInfo::CAT_POINTER:
//			printf("Checking member %s %s [at %d]\r\n", symbols + subType.offsetToName, memberName, offset);
			FixupPointer(*(char**)(ExPriv::newBase + offset), subType);
			break;
		case ExternTypeInfo::CAT_CLASS:
//			printf("Checking member %s %s [at %d]\r\n", symbols + subType.offsetToName, memberName, offset);
			FixupClass(offset, subType);
			break;
		default:
//			printf("Skipping member %s %s [at %d]\r\n", symbols + subType.offsetToName, memberName, offset);
			break;
		}
		memberName += strLength;
		offset += subType.size;
	}
}

bool Executor::ExtendParameterStack(char* oldBase, unsigned int oldSize, VMCmd *current)
{
//	printf("Old base: %p-%p\r\n", oldBase, oldBase + oldSize);
//	printf("New base: %p-%p\r\n", genParams.data, genParams.data + genParams.max);

	NULLC::MarkMemory(42);

	ExPriv::oldBase = oldBase;
	ExPriv::newBase = genParams.data;
	ExPriv::oldSize = oldSize;

	symbols = &exLinker->exSymbols[0];

	ExternVarInfo *vars = &exLinker->exVariables[0];
	ExternTypeInfo *types = &exLinker->exTypes[0];
	// Fix global variables
	for(unsigned int i = 0; i < exLinker->exVariables.size(); i++)
	{
		unsigned int offset = vars[i].offset;
		switch(types[vars[i].type].subCat)
		{
		case ExternTypeInfo::CAT_ARRAY:
//			printf("Checking global %s %s [at %d]\r\n", symbols + types[vars[i].type].offsetToName, symbols + vars[i].offsetToName, offset);
			FixupArray(offset, types[vars[i].type]);
			break;
		case ExternTypeInfo::CAT_POINTER:
//			printf("Checking global %s %s [at %d]\r\n", symbols + types[vars[i].type].offsetToName, symbols + vars[i].offsetToName, offset);
			FixupPointer(*(char**)(genParams.data + offset), types[vars[i].type]);
			break;
		case ExternTypeInfo::CAT_CLASS:
//			printf("Checking global %s %s [at %d]\r\n", symbols + types[vars[i].type].offsetToName, symbols + vars[i].offsetToName, offset);
			FixupClass(offset, types[vars[i].type]);
			break;
		default:
//			printf("Skipping global %s %s [at %d]\r\n", symbols + types[vars[i].type].offsetToName, symbols + vars[i].offsetToName, offset);
			break;
		}
	}
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
				switch(types[lInfo.type].subCat)
				{
				case ExternTypeInfo::CAT_ARRAY:
//					printf("Checking local %s %s [at %d+%d] [at %d] (%p)\r\n", symbols + types[lInfo.type].offsetToName, symbols + lInfo.offsetToName, offset, lInfo.offset, offset + lInfo.offset, *(int**)(genParams.data + offset + lInfo.offset));
					FixupArray(offset + lInfo.offset, types[lInfo.type]);
					break;
				case ExternTypeInfo::CAT_POINTER:
//					printf("Checking local %s %s [at %d+%d] [at %d] (%p)\r\n", symbols + types[lInfo.type].offsetToName, symbols + lInfo.offsetToName, offset, lInfo.offset, offset + lInfo.offset, *(int**)(genParams.data + offset + lInfo.offset));
					FixupPointer(*(char**)(genParams.data + offset + lInfo.offset), types[lInfo.type]);
					break;
				case ExternTypeInfo::CAT_CLASS:
//					printf("Checking local %s %s [at %d+%d] [at %d] (%p)\r\n", symbols + types[lInfo.type].offsetToName, symbols + lInfo.offsetToName, offset, lInfo.offset, offset + lInfo.offset, *(int**)(genParams.data + offset + lInfo.offset));
					FixupClass(offset + lInfo.offset, types[lInfo.type]);
					break;
				default:
//					printf("Skipping local %s %s [at %d+%d] [at %d] (%p)\r\n", symbols + types[lInfo.type].offsetToName, symbols + lInfo.offsetToName, offset, lInfo.offset, offset + lInfo.offset, *(int**)(genParams.data + offset + lInfo.offset));
					break;
				}
			}
			ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + exFunctions[funcID].localCount - 1];
			offset += lInfo.offset + lInfo.size;
		}
	}
	fcallStack.pop_back();
#ifdef _DEBUG
	NULLC::SweepMemory(42);
#endif

	return true;
}

const char* Executor::GetResult()
{
	if(genStackSize == 0)
	{
		strcpy(execResult, "No result value");
		return execResult;
	}
	if(genStackSize-1 > 2)
	{
		strcpy(execResult, "There are more than one value on the stack");
		return execResult;
	}
	switch(retType)
	{
	case OTYPE_DOUBLE:
		SafeSprintf(execResult, 64, "%f", *(double*)(genStackPtr));
		break;
	case OTYPE_LONG:
#ifdef _MSC_VER
		SafeSprintf(execResult, 64, "%I64dL", *(long long*)(genStackPtr));
#else
		SafeSprintf(execResult, 64, "%lld", *(long long*)(genStackPtr));
#endif
		break;
	case OTYPE_INT:
		SafeSprintf(execResult, 64, "%d", *(int*)(genStackPtr));
		break;
	default:
		SafeSprintf(execResult, 64, "no return value");
		break;
	}
	return execResult;
}

const char*	Executor::GetExecError()
{
	return execError;
}

char* Executor::GetVariableData()
{
	return &genParams[0];
}

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
// распечатать инструкцию в читабельном виде в поток
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
