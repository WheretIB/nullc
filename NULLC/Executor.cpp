#include "stdafx.h"
#include "Executor.h"

#include "CodeInfo.h"

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	#define DBG(x) x
#else
	#define DBG(x)
#endif

#include "CodeInfo.h"

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

	// $$$ Temporal solution to prevent stack from resizing, until there will be code that fixes pointers to stack variables. 
	genParams.reserve(1024 * 1024);
	genParams.clear();
	genParams.resize(exLinker->globalVarSize);

	execError[0] = 0;
	callContinue = true;

	retType = (asmOperType)-1;

	unsigned int funcPos = ~0ul;
	if(funcName)
	{
		unsigned int fnameHash = GetStringHash(funcName);
		for(int i = (int)exFunctions.size()-1; i >= 0; i--)
		{
			if(exFunctions[i].nameHash == fnameHash)
			{
				funcPos = exFunctions[i].address;
				break;
			}
		}
		if(funcPos == ~0ul)
		{
			SafeSprintf(execError, ERROR_BUFFER_SIZE, "ERROR: starting function %s not found", funcName);
			return;
		}
	}

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
	VMCmd *cmdStreamBase = &exLinker->exCode[0];
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
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			*genStackPtr = *((char*)NULL + cmd.argument + *genStackPtr);
			break;
		case cmdPushShortStk:
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			*genStackPtr = *(short*)((char*)NULL + cmd.argument + *genStackPtr);
			break;
		case cmdPushIntStk:
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			*genStackPtr = *(int*)((char*)NULL + cmd.argument + *genStackPtr);
			break;
		case cmdPushFloatStk:
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			genStackPtr--;
			*(double*)(genStackPtr) = (double)*(float*)((char*)NULL + cmd.argument + *(genStackPtr+1));
			break;
		case cmdPushDorLStk:
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			genStackPtr--;
			*(double*)(genStackPtr) = *(double*)((char*)NULL + cmd.argument + *(genStackPtr+1));
			break;
		case cmdPushCmplxStk:
		{
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
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
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			genStackPtr++;
			*((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned char)(*genStackPtr);
			break;
		case cmdMovShortStk:
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			genStackPtr++;
			*(unsigned short*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned short)(*genStackPtr);
			break;
		case cmdMovIntStk:
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			genStackPtr++;
			*(int*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (int)(*genStackPtr);
			break;
		case cmdMovFloatStk:
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			genStackPtr++;
			*(float*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (float)*(double*)(genStackPtr);
			break;
		case cmdMovDorLStk:
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
			genStackPtr++;
			*(long long*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = *(long long*)(genStackPtr);
			break;
		case cmdMovCmplxStk:
		{
			RUNTIME_ERROR(*genStackPtr == NULL, "ERROR: null pointer access");
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

		case cmdReserveV:
		{
			int alignOffset = (genParams.size() % 16 != 0) ? (16 - (genParams.size() % 16)) : 0;
			genParams.reserve(genParams.size() + alignOffset + cmd.argument);
			memcpy((char*)&genParams[genParams.size() + alignOffset], genStackPtr, cmd.argument);
			genStackPtr += cmd.argument >> 2;
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
			if(exFunctions[cmd.argument].funcPtr == NULL)
			{
				*genStackPtr = exFunctions[cmd.argument].address;
			}else{
				assert(sizeof(exFunctions[cmd.argument].funcPtr) == 4);
				*genStackPtr = (unsigned int)(intptr_t)(exFunctions[cmd.argument].funcPtr);
			}
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
			unsigned int fAddress = cmd.argument;
			if(fAddress == CALL_BY_POINTER)
			{
				fAddress = *genStackPtr;
				genStackPtr++;
				// External function call by pointer
				RUNTIME_ERROR(genStackPtr[-2] == ~0u, "ERROR: External function pointers are unsupported");
			}
			RUNTIME_ERROR(fAddress == 0, "ERROR: Invalid function pointer");
			fcallStack.push_back(cmdStream);
			cmdStream = cmdStreamBase + fAddress;
		}
			break;

		case cmdCallStd:
			if(!RunExternalFunction(cmd.argument))
				cmdStreamEnd = NULL;
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
			genParams.resize(paramBase + cmd.argument);
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

		case cmdAddAtCharStk:
		{
			unsigned int shift = *genStackPtr;
			genStackPtr++;
			if(cmd.flag == bitPushBefore)
			{
				genStackPtr--;
				*genStackPtr = *((char*)NULL + shift + cmd.argument);
			}
			char *address = ((char*)NULL + shift + cmd.argument);
			*address = *address + (char)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr--;
				*genStackPtr = *((char*)NULL + shift + cmd.argument);
			}
		}
			break;
		case cmdAddAtShortStk:
		{
			unsigned int shift = *genStackPtr;
			genStackPtr++;
			if(cmd.flag == bitPushBefore)
			{
				genStackPtr--;
				*genStackPtr = *(short*)((char*)NULL + shift + cmd.argument);
			}
			short *address = (short*)((char*)NULL + shift + cmd.argument);
			*address = *address + (short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr--;
				*genStackPtr = *(short*)((char*)NULL + shift + cmd.argument);
			}
		}
			break;
		case cmdAddAtIntStk:
		{
			unsigned int shift = *genStackPtr;
			genStackPtr++;
			if(cmd.flag == bitPushBefore)
			{
				genStackPtr--;
				*genStackPtr = *(int*)((char*)NULL + shift + cmd.argument);
			}
			*(int*)((char*)NULL + shift + cmd.argument) += (int)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr--;
				*genStackPtr = *(int*)((char*)NULL + shift + cmd.argument);
			}
		}
			break;
		case cmdAddAtLongStk:
		{
			unsigned int shift = *genStackPtr;
			genStackPtr++;
			if(cmd.flag == bitPushBefore)
			{
				genStackPtr -= 2;
				*(long long*)(genStackPtr) = *(long long*)((char*)NULL + shift + cmd.argument);
			}
			*(long long*)((char*)NULL + shift + cmd.argument) += (long long)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr -= 2;
				*(long long*)(genStackPtr) = *(long long*)((char*)NULL + shift + cmd.argument);
			}
		}
			break;
		case cmdAddAtFloatStk:
		{
			unsigned int shift = *genStackPtr;
			genStackPtr++;
			if(cmd.flag == bitPushBefore)
			{
				genStackPtr -= 2;
				*(double*)(genStackPtr) = (double)*(float*)((char*)NULL + shift + cmd.argument);
			}
			*(float*)((char*)NULL + shift + cmd.argument) += (float)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr -= 2;
				*(double*)(genStackPtr) = (double)*(float*)((char*)NULL + shift + cmd.argument);
			}
		}
			break;
		case cmdAddAtDoubleStk:
		{
			unsigned int shift = *genStackPtr;
			genStackPtr++;
			if(cmd.flag == bitPushBefore)
			{
				genStackPtr -= 2;
				*(double*)(genStackPtr) = *(double*)((char*)NULL + shift + cmd.argument);
			}
			*(double*)((char*)NULL + shift + cmd.argument) += (double)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr -= 2;
				*(double*)(genStackPtr) = *(double*)((char*)NULL + shift + cmd.argument);
			}
		}
			break;
		case cmdCreateClosure:
		{
			ExternFuncInfo::Upvalue *closure = (ExternFuncInfo::Upvalue*)(intptr_t)*genStackPtr;
			genStackPtr++;

			ExternFuncInfo &func = exFunctions[cmd.argument];
			ExternLocalInfo *externals = &exLinker->exLocals[func.offsetToFirstLocal + func.localCount];
			for(unsigned int i = 0; i < func.externCount; i++)
			{
				ExternFuncInfo *varParent = &exFunctions[externals[i].closeFuncList & ~0x80000000];
				if(externals[i].closeFuncList & 0x80000000)
				{
					closure->ptr = (unsigned int*)(externals[i].target + paramBase + &genParams[0]);
				}else{
					unsigned int *prevClosure = (unsigned int*)(intptr_t)*(int*)(&genParams[cmd.helper + paramBase]);
					closure->ptr = (unsigned int*)(intptr_t)prevClosure[externals[i].target >> 2];
				}
				closure->next = varParent->externalList;
				closure->size = externals[i].size;
				varParent->externalList = closure;
				closure = (ExternFuncInfo::Upvalue*)((int*)closure + ((externals[i].size >> 2) < 3 ? 3 : (externals[i].size >> 2)));
			}
		}
			break;
		case cmdCloseUpvals:
		{
			ExternFuncInfo &func = exFunctions[cmd.argument];
			ExternFuncInfo::Upvalue *curr = func.externalList;
			while(curr && (char*)curr->ptr >= paramBase + &genParams[0])
			{
				ExternFuncInfo::Upvalue *next = curr->next;
				unsigned int size = curr->size;

				memcpy(&curr->next, curr->ptr, size);
				curr->ptr = (unsigned int*)&curr->next;
				curr = next;
			}
			func.externalList = curr;
		}
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
			int funcID = -1;
			for(unsigned int i = 0; i < exFunctions.size(); i++)
				if(address >= exFunctions[i].address && address <= (exFunctions[i].address + exFunctions[i].codeSize))
					funcID = i;
			if(funcID != -1)
				currPos += SafeSprintf(currPos, ERROR_BUFFER_SIZE - int(currPos - execError), "%s", &exLinker->exSymbols[exFunctions[funcID].offsetToName]);
			else
				currPos += SafeSprintf(currPos, ERROR_BUFFER_SIZE - int(currPos - execError), "%s", address == -1 ? "external" : "global scope");
			if(address != -1)
			{
				unsigned int line = 0;
				unsigned int i = address - 1;
				while((line < CodeInfo::cmdInfoList.sourceInfo.size() - 1) && (i >= CodeInfo::cmdInfoList.sourceInfo[line + 1].byteCodePos))
						line++;
				const char *codeStart = CodeInfo::cmdInfoList.sourceInfo[line].sourcePos;
				while(*codeStart == ' ' || *codeStart == '\t')
					codeStart++;
				int codeLength = (int)(CodeInfo::cmdInfoList.sourceInfo[line].sourceEnd - codeStart) - 1;
				currPos += SafeSprintf(currPos, ERROR_BUFFER_SIZE - int(currPos - execError), " (at %.*s)\r\n", codeLength, codeStart);
			}
#ifdef NULLC_STACK_TRACE_WITH_LOCALS
			if(funcID != -1)
			{
				for(unsigned int i = 0; i < exFunctions[funcID].localCount + exFunctions[funcID].externCount; i++)
				{
					ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + i];
					const char *typeName = &exLinker->exSymbols[exTypes[lInfo.type].offsetToName];
					const char *localName = &exLinker->exSymbols[lInfo.offsetToName];
					const char *localType = lInfo.paramType == ExternLocalInfo::PARAMETER ? "param" : (lInfo.paramType == ExternLocalInfo::EXTERNAL ? "extern" : "local");
					const char *offsetType = (lInfo.paramType == ExternLocalInfo::PARAMETER || lInfo.paramType == ExternLocalInfo::LOCAL) ? "base" :
						(lInfo.closeFuncList & 0x80000000 ? "local" : "closure");
					currPos += SafeSprintf(currPos, ERROR_BUFFER_SIZE - int(currPos - execError), " %s %d: %s %s (at %s+%d size %d)\r\n",
						localType, i, typeName, localName, offsetType, lInfo.offset, exTypes[lInfo.type].size);
				}
			}
#endif

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
bool Executor::RunExternalFunction(unsigned int funcID)
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
	genStackPtr += (bytesToPop >> 2);

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

bool Executor::RunExternalFunction(unsigned int funcID)
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
bool Executor::RunExternalFunction(unsigned int funcID)
{
	strcpy(execError, "ERROR: External function call failed");
	return false;
}
#endif

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
