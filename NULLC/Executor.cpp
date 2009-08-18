#include "stdafx.h"
#include "Executor.h"

#include "CodeInfo.h"

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	#define DBG(x) x
#else
	#define DBG(x)
#endif

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
	paramTop.clear();
	fcallStack.clear();

	paramTop.push_back(0);

	genParams.clear();
	genStackTypes.clear();

	double tempVal = 0.0;
	genParams.push_back((char*)(&tempVal), 8);
	tempVal = 3.1415926535897932384626433832795;
	genParams.push_back((char*)(&tempVal), 8);
	tempVal = 2.7182818284590452353602874713527;
	genParams.push_back((char*)(&tempVal), 8);
	
	genParams.resize(exLinker->globalVarSize);

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	//unsigned int typeSizeS[] = { 4, 8, 4, 8 };
	//unsigned int typeSizeD[] = { 1, 2, 4, 8, 4, 8 };
#endif

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
	
	while(cmdStream < cmdStreamEnd)
	{
		const VMCmd &cmd = *cmdStream;
		//const unsigned int argument = cmd.argument;
		DBG(PrintInstructionText(executeLog, cmd, paramTop.back(), genParams.size()));
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
			*genStackPtr = genParams[cmd.argument + (paramTop.back() * cmd.flag)];
			break;
		case cmdPushShort:
			genStackPtr--;
			*genStackPtr =  *((short*)(&genParams[cmd.argument + (paramTop.back() * cmd.flag)]));
			break;
		case cmdPushInt:
			genStackPtr--;
			*genStackPtr = *((int*)(&genParams[cmd.argument + (paramTop.back() * cmd.flag)]));
			break;
		case cmdPushFloat:
			genStackPtr -= 2;
			*(double*)(genStackPtr) = (double)*((float*)(&genParams[cmd.argument + (paramTop.back() * cmd.flag)]));
			break;
		case cmdPushDorL:
			genStackPtr -= 2;
			*(double*)(genStackPtr) = *((double*)(&genParams[cmd.argument + (paramTop.back() * cmd.flag)]));
			break;
		case cmdPushCmplx:
		{
			int valind = cmd.argument + (paramTop.back() * cmd.flag);
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
			*genStackPtr = genParams[cmd.argument + *genStackPtr];
			break;
		case cmdPushShortStk:
			*genStackPtr =  *((short*)(&genParams[cmd.argument + *genStackPtr]));
			break;
		case cmdPushIntStk:
			*genStackPtr = *((int*)(&genParams[cmd.argument + *genStackPtr]));
			break;
		case cmdPushFloatStk:
			genStackPtr--;
			*(double*)(genStackPtr) = (double)*((float*)(&genParams[cmd.argument + *(genStackPtr+1)]));
			break;
		case cmdPushDorLStk:
			genStackPtr--;
			*(double*)(genStackPtr) = *((double*)(&genParams[cmd.argument + *(genStackPtr+1)]));
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
				*genStackPtr = *((unsigned int*)(&genParams[shift + currShift]));
			}
		}
			break;

		case cmdPushImmt:
			genStackPtr--;
			*genStackPtr = cmd.argument;
			break;

		case cmdMovChar:
			genParams[cmd.argument + (paramTop.back() * cmd.flag)] = (unsigned char)(*genStackPtr);
			break;
		case cmdMovShort:
			*((unsigned short*)(&genParams[cmd.argument + (paramTop.back() * cmd.flag)])) = (unsigned short)(*genStackPtr);
			break;
		case cmdMovInt:
			*((int*)(&genParams[cmd.argument + (paramTop.back() * cmd.flag)])) = (int)(*genStackPtr);
			break;
		case cmdMovFloat:
			*((float*)(&genParams[cmd.argument + (paramTop.back() * cmd.flag)])) = (float)*(double*)(genStackPtr);
			break;
		case cmdMovDorL:
			*((long long*)(&genParams[cmd.argument + (paramTop.back() * cmd.flag)])) = *(long long*)(genStackPtr);
			break;
		case cmdMovCmplx:
		{
			int valind = cmd.argument + (paramTop.back() * cmd.flag);
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
			genParams[cmd.argument + *(genStackPtr-1)] = (unsigned char)(*genStackPtr);
			break;
		case cmdMovShortStk:
			genStackPtr++;
			*((unsigned short*)(&genParams[cmd.argument + *(genStackPtr-1)])) = (unsigned short)(*genStackPtr);
			break;
		case cmdMovIntStk:
			genStackPtr++;
			*((int*)(&genParams[cmd.argument + *(genStackPtr-1)])) = (int)(*genStackPtr);
			break;
		case cmdMovFloatStk:
			genStackPtr++;
			*((float*)(&genParams[cmd.argument + *(genStackPtr-1)])) = (float)*(double*)(genStackPtr);
			break;
		case cmdMovDorLStk:
			genStackPtr++;
			*((long long*)(&genParams[cmd.argument + *(genStackPtr-1)])) = *(long long*)(genStackPtr);
			break;
		case cmdMovCmplxStk:
		{
			unsigned int shift = cmd.argument + *genStackPtr;
			genStackPtr++;
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				*((unsigned int*)(&genParams[shift + currShift])) = *(genStackPtr+(currShift>>2));
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
			//genStackPtr += cmd.argument >> 2;
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

		case cmdImmtMulD:
			*(genStackPtr+1) = cmd.argument * int(*(double*)(genStackPtr));
			genStackPtr++;
			break;
		case cmdImmtMulL:
			*(genStackPtr+1) = cmd.argument * int(*(long long*)(genStackPtr));
			genStackPtr++;
			break;
		case cmdImmtMulI:
			*genStackPtr = cmd.argument * (*genStackPtr);
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
			*genStackPtr = cmd.argument + paramTop.back();
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

			unsigned int start = cmd.argument + paramTop.back();

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

		case cmdJmpZI:
			if(*genStackPtr == 0)
				cmdStream = cmdStreamBase + cmd.argument;
			genStackPtr++;
			break;
		case cmdJmpZD:
			if(*(double*)(genStackPtr) == 0.0)
				cmdStream = cmdStreamBase + cmd.argument;
			genStackPtr += 2;
			break;
		case cmdJmpZL:
			if(*(long long*)(genStackPtr) == 0L)
				cmdStream = cmdStreamBase + cmd.argument;
			genStackPtr += 2;
			break;

		case cmdJmpNZI:
			if(*genStackPtr != 0)
				cmdStream = cmdStreamBase + cmd.argument;
			genStackPtr++;
			break;
		case cmdJmpNZD:
			if(*(double*)(genStackPtr) != 0.0)
				cmdStream = cmdStreamBase + cmd.argument;
			genStackPtr += 2;
			break;
		case cmdJmpNZL:
			if(*(long long*)(genStackPtr) != 0L)
				cmdStream = cmdStreamBase + cmd.argument;
			genStackPtr += 2;
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
		{
			unsigned int valind = cmd.argument;
			if(exFunctions[valind].funcPtr == NULL)
			{
				double val = *(double*)(genStackPtr);

				if(exFunctions[valind].nameHash == GetStringHash("cos"))
					val = cos(val);
				else if(exFunctions[valind].nameHash == GetStringHash("sin"))
					val = sin(val);
				else if(exFunctions[valind].nameHash == GetStringHash("tan"))
					val = tan(val);
				else if(exFunctions[valind].nameHash == GetStringHash("ctg"))
					val = 1.0/tan(val);
				else if(exFunctions[valind].nameHash == GetStringHash("ceil"))
					val = ceil(val);
				else if(exFunctions[valind].nameHash == GetStringHash("floor"))
					val = floor(val);
				else if(exFunctions[valind].nameHash == GetStringHash("sqrt"))
					val = sqrt(val);
				else{
					cmdStreamEnd = NULL;
					printf(execError, "ERROR: Build-in function not found");
					break;
				}

				if(fabs(val) < 1e-10)
					val = 0.0;
				*(double*)(genStackPtr) = val;
			}else{
				if(!RunExternalFunction(valind))
					cmdStreamEnd = NULL;
			}
		}
			break;

		case cmdReturn:
			if(cmd.flag & bitRetError)
			{
				cmdStreamEnd = NULL;
				strcpy(execError, "ERROR: function didn't return a value");
				break;
			}
			// TODO: move (cmd.argument > 0 ? cmd.argument : 1) to compilation stage
			for(unsigned int pops = 0; pops < (cmd.argument > 0 ? cmd.argument : 1); pops++)
			{
				genParams.shrink(paramTop.back());
				paramTop.pop_back();
			}
			if(fcallStack.size() == 0)
			{
				retType = (cmd.helper&bitRetSimple) ? (asmOperType)(cmd.helper^bitRetSimple) : OTYPE_FLOAT_DEPRECATED;
				cmdStream = cmdStreamEnd;
				break;
			}
			cmdStream = fcallStack.back();
			fcallStack.pop_back();
			break;

		case cmdPushVTop:
			paramTop.push_back(genParams.size());
			break;
		case cmdPopVTop:
			genParams.shrink(paramTop.back());
			paramTop.pop_back();
			break;

		case cmdPushV:
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
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) < *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdGreaterL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) > *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdLEqualL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) <= *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdGEqualL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) >= *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdEqualL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) == *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdNEqualL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) != *(long long*)(genStackPtr);
			genStackPtr += 2;
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
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) && *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdLogOrL:
			*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) || *(long long*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdLogXorL:
			*(long long*)(genStackPtr+2) = !!(*(long long*)(genStackPtr+2)) ^ !!(*(long long*)(genStackPtr));
			genStackPtr += 2;
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
			*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) < *(double*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdGreaterD:
			*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) > *(double*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdLEqualD:
			*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) <= *(double*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdGEqualD:
			*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) >= *(double*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdEqualD:
			*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) == *(double*)(genStackPtr);
			genStackPtr += 2;
			break;
		case cmdNEqualD:
			*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) != *(double*)(genStackPtr);
			genStackPtr += 2;
			break;

		case cmdNeg:
			*(int*)(genStackPtr) = -*(int*)(genStackPtr);
			break;
		case cmdBitNot:
			*(int*)(genStackPtr) = ~*(int*)(genStackPtr);
			break;
		case cmdLogNot:
			*(int*)(genStackPtr) = !*(int*)(genStackPtr);
			break;

		case cmdNegL:
			*(long long*)(genStackPtr) = -*(long long*)(genStackPtr);
			break;
		case cmdBitNotL:
			*(long long*)(genStackPtr) = ~*(long long*)(genStackPtr);
			break;
		case cmdLogNotL:
			*(long long*)(genStackPtr) = !*(long long*)(genStackPtr);
			break;

		case cmdNegD:
			*(double*)(genStackPtr) = -*(double*)(genStackPtr);
			break;
		case cmdLogNotD:
			*(double*)(genStackPtr) = fabs(*(double*)(genStackPtr)) < 1e-10;
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
				*genStackPtr = *((char*)(&genParams[shift + cmd.argument]));
			}
			*((char*)(&genParams[shift + cmd.argument])) += (char)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr--;
				*genStackPtr = *((char*)(&genParams[shift + cmd.argument]));
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
				*genStackPtr = *((short*)(&genParams[shift + cmd.argument]));
			}
			*((short*)(&genParams[shift + cmd.argument])) += (short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr--;
				*genStackPtr = *((short*)(&genParams[shift + cmd.argument]));
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
				*genStackPtr = *((int*)(&genParams[shift + cmd.argument]));
			}
			*((int*)(&genParams[shift + cmd.argument])) += (int)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr--;
				*genStackPtr = *((int*)(&genParams[shift + cmd.argument]));
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
				*genStackPtr = *(int*)(&genParams[shift + cmd.argument]);
				*(genStackPtr+1) = *(int*)(&genParams[shift + cmd.argument+4]);
			}
			*((long long*)(&genParams[shift + cmd.argument])) += (long long)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr -= 2;
				*genStackPtr = *(int*)(&genParams[shift + cmd.argument]);
				*(genStackPtr+1) = *(int*)(&genParams[shift + cmd.argument+4]);
			}
		}
			break;
		case cmdAddAtFloatStk:
		{
			unsigned int shift = *genStackPtr;
			genStackPtr++;
			if(cmd.flag == bitPushBefore)
			{
				double res = (double)(*((float*)(&genParams[shift + cmd.argument])));
				genStackPtr -= 2;
				*(double*)(genStackPtr) = res;
			}
			*((float*)(&genParams[shift + cmd.argument])) += (float)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				double res = (double)(*((float*)(&genParams[shift + cmd.argument])));
				genStackPtr -= 2;
				*(double*)(genStackPtr) = res;
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
				*genStackPtr = *(int*)(&genParams[shift + cmd.argument]);
				*(genStackPtr+1) = *(int*)(&genParams[shift + cmd.argument+4]);
			}
			*((double*)(&genParams[shift + cmd.argument])) += (double)(short)cmd.helper;
			if(cmd.flag == bitPushAfter)
			{
				genStackPtr -= 2;
				*genStackPtr = *(int*)(&genParams[shift + cmd.argument]);
				*(genStackPtr+1) = *(int*)(&genParams[shift + cmd.argument+4]);
			}
		}
			break;
		
		}

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
		/*unsigned int typeSizeS[] = { 1, 2, 0, 2 };
		m_FileStream << " stack size " << genStackSize << "; stack vals " << genStackTypes.size() << "; param size " << genParams.size() << ";  // ";
		assert(genStackTypes.size() < (1 << 16));
		for(unsigned int i = 0, k = 0; i < genStackTypes.size(); i++)
		{
			if(genStackTypes[i] & 0x80000000)
			{
				m_FileStream << "complex " << (genStackTypes[i] & ~0x80000000) << " bytes";
				k += genStackTypes[i] & ~0x80000000;
			}else{
				if(genStackTypes[i] == STYPE_DOUBLE)
					m_FileStream << "double " << *((double*)(genStackPtr+k)) << ", ";
				if(genStackTypes[i] == STYPE_LONG)
					m_FileStream << "long " << *((long*)(genStackPtr+k)) << ", ";
				if(genStackTypes[i] == STYPE_INT)
					m_FileStream << "int " << *((int*)(genStackPtr+k)) << ", ";
				k += typeSizeS[genStackTypes[i]];
			}
		}*/
		fprintf(executeLog, ";\r\n");
		fflush(executeLog);
#endif
	}
}

#ifdef _MSC_VER
// X86 implementation
bool Executor::RunExternalFunction(unsigned int funcID)
{
	unsigned int bytesToPop = exFunctions[funcID].bytesToPop;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	/*unsigned int typeSizeS[] = { 4, 8, 4, 8 };
	unsigned int paramSize = bytesToPop;
	while(paramSize > 0)
	{
		paramSize -= genStackTypes.back() & 0x80000000 ? genStackTypes.back() & ~0x80000000 : typeSizeS[genStackTypes.back()];
		genStackTypes.pop_back();
	}*/
#endif
	unsigned int *stackStart = (genStackPtr+bytesToPop/4-1);
	for(unsigned int i = 0; i < bytesToPop/4; i++)
	{
		__asm mov eax, dword ptr[stackStart]
		__asm push dword ptr[eax];
		stackStart--;
	}
	genStackPtr += bytesToPop/4;

	void* fPtr = exFunctions[funcID].funcPtr;
	unsigned int fRes;
	__asm{
		mov eax, fPtr;
		call eax;
		add esp, bytesToPop;
		mov fRes, eax;
	}
	if(exFunctions[funcID].retSize != 0)
	{
		genStackPtr--;
		*genStackPtr = fRes;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
		/*if(exTypes[exFunctions[funcID]->retType]->type == TypeInfo::TYPE_COMPLEX)
			genStackTypes.push_back((asmStackType)(0x80000000 | exTypes[exFunctions[funcID]->retType]->size));
		else
			genStackTypes.push_back(exTypes[exFunctions[funcID]->retType]->stackType);*/
#endif
	}
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
