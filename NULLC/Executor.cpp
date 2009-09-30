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
	if(pow == 0)
		return 1;
	if(pow == 1)
		return num;
	if(pow > 36)
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

	m_RunCallback = NULL;

	genStackBase = NULL;
	genStackPtr = NULL;
	genStackTop = NULL;
}

Executor::~Executor()
{
	DBG(fclose(executeLog));
	m_RunCallback = NULL;

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

	genParams.reserve(1024 * 1024);
	genParams.clear();
	genParams.resize(exLinker->globalVarSize);

	execError[0] = 0;

	unsigned int funcPos = 0;
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
		if(funcPos == 0)
		{
			sprintf(execError, "ERROR: starting function %s not found", funcName);
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

	genStackPtr--;
	*genStackPtr = 0;

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
			*genStackPtr =  *((short*)(&genParams[cmd.argument + (paramBase * cmd.flag)]));
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
			*genStackPtr = *((char*)NULL + cmd.argument + *genStackPtr);
			break;
		case cmdPushShortStk:
			*genStackPtr =  *(short*)((char*)NULL + cmd.argument + *genStackPtr);
			break;
		case cmdPushIntStk:
			*genStackPtr = *(int*)((char*)NULL + cmd.argument + *genStackPtr);
			break;
		case cmdPushFloatStk:
			genStackPtr--;
			*(double*)(genStackPtr) = (double)*(float*)((char*)NULL + cmd.argument + *(genStackPtr+1));
			break;
		case cmdPushDorLStk:
			genStackPtr--;
			*(double*)(genStackPtr) = *(double*)((char*)NULL + cmd.argument + *(genStackPtr+1));
			break;
		case cmdPushCmplxStk:
		{
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
			genStackPtr++;
			*((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned char)(*genStackPtr);
			break;
		case cmdMovShortStk:
			genStackPtr++;
			*(unsigned short*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned short)(*genStackPtr);
			break;
		case cmdMovIntStk:
			genStackPtr++;
			*(int*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (int)(*genStackPtr);
			break;
		case cmdMovFloatStk:
			genStackPtr++;
			*(float*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (float)*(double*)(genStackPtr);
			break;
		case cmdMovDorLStk:
			genStackPtr++;
			*(long long*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = *(long long*)(genStackPtr);
			break;
		case cmdMovCmplxStk:
		{
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
			genParams.reserve(genParams.size() + cmd.argument);
			break;

		case cmdPopCharTop:
			genParams[cmd.argument + genParams.size()] = *(char*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdPopShortTop:
			*((short*)(&genParams[cmd.argument + genParams.size()])) = *(short*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdPopIntTop:
			*((unsigned int*)(&genParams[cmd.argument + genParams.size()])) = *genStackPtr;
			genStackPtr++;
			break;
		case cmdPopFloatTop:
			*((float*)(&genParams[cmd.argument + genParams.size()])) = float(*(double*)(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdPopDorLTop:
			*((unsigned int*)(&genParams[cmd.argument + genParams.size()])) = *genStackPtr;
			*((unsigned int*)(&genParams[cmd.argument + genParams.size() + 4])) = *(genStackPtr+1);
			genStackPtr += 2;
			break;
		case cmdPopCmplxTop:
			memcpy((char*)&genParams[cmd.argument + genParams.size()], genStackPtr, cmd.helper);
			genStackPtr += cmd.helper >> 2;
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
			if(*genStackPtr >= (unsigned int)cmd.argument)
			{
				cmdStreamEnd = NULL;
				strcpy(execError, "ERROR: array index out of bounds");
				break;
			}
			*(int*)(genStackPtr+1) += cmd.helper * (*genStackPtr);
			genStackPtr++;
			break;
		case cmdIndexStk:
			if(*genStackPtr >= *(genStackPtr+2))
			{
				cmdStreamEnd = NULL;
				strcpy(execError, "ERROR: array index out of bounds");
				break;
			}
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
			*genStackPtr = cmd.argument + paramBase * cmd.helper + (int)(long long)&genParams[0];
			break;
		case cmdFuncAddr:
			assert(sizeof(exFunctions[cmd.argument].funcPtr) == 4);

			genStackPtr--;
			if(exFunctions[cmd.argument].funcPtr == NULL)
				*genStackPtr = exFunctions[cmd.argument].address;
			else
				*genStackPtr = (unsigned int)((unsigned long long)(exFunctions[cmd.argument].funcPtr));
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
			if(genStackPtr <= genStackBase+8)
			{
				cmdStreamEnd = NULL;
				printf(execError, "ERROR: stack overflow");
				break;
			}
			unsigned int fAddress = cmd.argument;
			if(fAddress == CALL_BY_POINTER)
			{
				fAddress = *genStackPtr;
				genStackPtr++;
			}
			fcallStack.push_back(cmdStream);
			cmdStream = cmdStreamBase + fAddress;
		}
			break;

		case cmdCallStd:
			if(!RunExternalFunction(cmd.argument))
				cmdStreamEnd = NULL;
			break;

		case cmdReturn:
			if(cmd.flag & bitRetError)
			{
				cmdStreamEnd = NULL;
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
			if(fcallStack.size() == 0)
			{
				retType = (asmOperType)cmd.flag;
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
			genParams.resize(genParams.size() + cmd.argument);
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
		
		}

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
		fprintf(executeLog, ";\r\n");
		fflush(executeLog);
#endif
	}
	// Print call stack on error
	if(cmdStreamEnd == NULL)
	{
		unsigned int line = 0;
		unsigned int i = (unsigned int)(cmdStream - cmdStreamBase);
		while((line < CodeInfo::cmdInfoList.sourceInfo.size() - 1) && (i >= CodeInfo::cmdInfoList.sourceInfo[line + 1].byteCodePos))
				line++;

		char buf[512];
		sprintf(buf, " (at %.*s)", CodeInfo::cmdInfoList.sourceInfo[line].sourceEnd - CodeInfo::cmdInfoList.sourceInfo[line].sourcePos, CodeInfo::cmdInfoList.sourceInfo[line].sourcePos);
		strcat(execError, buf);
		
		/*char *currPos = execError + strlen(execError);
		while(fcallStack.size())
		{
			int address = int(fcallStack.back() - cmdStreamBase);
			if(address != -1)
			{
				for(unsigned int i = 0; i < 
			}else{
				if((currPos - execError) + strlen("unknown (-1)") < ERROR_BUFFER_SIZE)
					currPos = strcpy(execError, "unknown (-1)");
			}
		}*/
	}
}

#ifdef _MSC_VER
// X86 implementation
bool Executor::RunExternalFunction(unsigned int funcID)
{
	unsigned int bytesToPop = exFunctions[funcID].bytesToPop;

	unsigned int *stackStart = (genStackPtr+bytesToPop/4-1);
	for(unsigned int i = 0; i < bytesToPop/4; i++)
	{
		__asm mov eax, dword ptr[stackStart]
		__asm push dword ptr[eax];
		stackStart--;
	}
	genStackPtr += bytesToPop/4;

	void* fPtr = exFunctions[funcID].funcPtr;
	if(exFunctions[funcID].retType == ExternFuncInfo::RETURN_VOID)
	{
		((void (*)())fPtr)();
	}else if(exFunctions[funcID].retType == ExternFuncInfo::RETURN_INT){
		genStackPtr--;
		*genStackPtr = ((int (*)())fPtr)();
	}else if(exFunctions[funcID].retType == ExternFuncInfo::RETURN_DOUBLE){
		genStackPtr -= 2;
		*(double*)genStackPtr = ((double (*)())fPtr)();
	}else if(exFunctions[funcID].retType == ExternFuncInfo::RETURN_LONG){
		genStackPtr -= 2;
		*(long long*)genStackPtr = ((long long (*)())fPtr)();
	}
	__asm add esp, bytesToPop;
	return true;
}
#elif defined(__CELLOS_LV2__)
// PS3 implementation
typedef unsigned int (*SimpleFunctionPtr)(
	unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
	double, double, double, double, double, double, double, double
	);

bool Executor::RunExternalFunction(unsigned int funcID)
{
	// cast function pointer so we can call it
	SimpleFunctionPtr code = (SimpleFunctionPtr)exFunctions[funcID]->funcPtr;

	// call function
	#define R(i) *(const unsigned int*)(const void*)(genStackPtr + exFuncInfo[funcID].rOffsets[i])
	#define F(i) *(const double*)(const void*)(genStackPtr + exFuncInfo[funcID].fOffsets[i])

	unsigned int result = code(R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7));

	#undef F
	#undef R

	if (exTypes[exFunctions[funcID]->retType]->size != 0)
	{
		genStackPtr--;
		*genStackPtr = result;
	}

	return true;
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
		sprintf(execResult, "%f", *(double*)(genStackPtr));
		break;
	case OTYPE_LONG:
#ifdef _MSC_VER
		sprintf(execResult, "%I64dL", *(long long*)(genStackPtr));
#else
		sprintf(execResult, "%lld", *(long long*)(genStackPtr));
#endif
		break;
	case OTYPE_INT:
		sprintf(execResult, "%d", *(int*)(genStackPtr));
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

void Executor::SetCallback(bool (*Func)(unsigned int))
{
	m_RunCallback = Func;
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
