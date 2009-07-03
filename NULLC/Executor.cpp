#include "stdafx.h"
#include "Executor.h"

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

Executor::Executor(Linker* linker): exLinker(linker), exFunctions(linker->exFunctions),
			exFuncInfo(linker->exFuncInfo), exCode(linker->exCode), exTypes(linker->exTypes)
{
	DBG(m_FileStream.open("log.txt", std::ios::binary));

	m_RunCallback = NULL;

	genStackBase = NULL;
	genStackPtr = NULL;
	genStackTop = NULL;
}

Executor::~Executor()
{
	m_RunCallback = NULL;

	delete[] genStackBase;
}

#define genStackSize (genStackTop-genStackPtr)

void Executor::Run(const char* funcName) throw()
{
	if(!exCode.size())
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

	CmdID	cmd;

	//unsigned int	cmdCount = 0;
	bool	done = false;

	CmdFlag		cFlag;
	OperFlag	oFlag;

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	unsigned int typeSizeS[] = { 4, 8, 4, 8 };
	//unsigned int typeSizeD[] = { 1, 2, 4, 8, 4, 8 };
#endif

	execError[0] = 0;

	unsigned int funcPos = 0;
	if(funcName)
	{
		unsigned int fnameHash = GetStringHash(funcName);
		for(int i = (int)exFunctions.size()-1; i >= 0; i--)
		{
			if(exFunctions[i]->nameHash == fnameHash)
			{
				funcPos = exFunctions[i]->address;
				break;
			}
		}
		if(funcPos == 0)
		{
			sprintf(execError, "ERROR: starting function %s not found", funcName);
			done = true;
		}
	}

	// General stack
	if(!genStackBase)
	{
		genStackBase = new unsigned int[128];		// Will grow
		genStackTop = genStackBase + 128;
	}
	genStackPtr = genStackTop - 1;

#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
	unsigned int insCallCount[255];
	memset(insCallCount, 0, 255*4);
#endif
	char *cmdStreamBase = &exCode[0];
	char *cmdStream = &exCode[exLinker->offsetToGlobalCode];
	char *cmdStreamEnd = &exCode[exCode.size()];
#define cmdStreamPos (cmdStream-cmdStreamBase)

	if(funcName)
		cmdStream = &exCode[funcPos];

	while(cmdStream+2 < cmdStreamEnd && !done)
	{
		cmd = *(CmdID*)(cmdStream);
		DBG(unsigned int pos2 = (unsigned int)(cmdStream - cmdStreamBase));
		cmdStream += 2;

		if(genStackPtr <= genStackBase)
		{
			unsigned int *oldStack = genStackBase;
			unsigned int oldSize = (unsigned int)(genStackTop-genStackBase);
			genStackBase = new unsigned int[oldSize+128];
			genStackTop = genStackBase + oldSize + 128;
			memcpy(genStackBase+128, oldStack, oldSize * sizeof(unsigned int));
			delete[] oldStack;

			genStackPtr = genStackTop - oldSize;
		}
#ifdef NULLC_VM_DEBUG
		if(genStackSize < 0)
		{
			done = true;
			assert(!"stack underflow");
		}
#endif
		#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
			insCallCount[cmd]++;
		#endif

		unsigned int	highDW = 0, lowDW = 0;

		switch(cmd)
		{
		case cmdPushCharAbs:
			cmdStream += 2;

			genStackPtr--;
			*genStackPtr = genParams[*(int*)cmdStream];

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4), STYPE_INT | DTYPE_CHAR, 0, 0, 0));
			break;
		case cmdPushShortAbs:
			cmdStream += 2;

			genStackPtr--;
			*genStackPtr =  *((short*)(&genParams[*(int*)cmdStream]));

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4), STYPE_INT | DTYPE_SHORT, 0, 0, 0));
			break;
		case cmdPushIntAbs:
			cmdStream += 2;

			genStackPtr--;
			*genStackPtr = *((int*)(&genParams[*(int*)cmdStream]));

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4), STYPE_INT | DTYPE_INT, 0, 0, 0));
			break;
		case cmdPushFloatAbs:
			cmdStream += 2;

			genStackPtr -= 2;
			*(double*)(genStackPtr) = (double)*((float*)(&genParams[*(int*)cmdStream]));

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_DOUBLE); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4), STYPE_DOUBLE | DTYPE_FLOAT, 0, 0, 0));
			break;
		case cmdPushDorLAbs:
			cmdStream += 2;

			genStackPtr -= 2;
			*(double*)(genStackPtr) = *((double*)(&genParams[*(int*)cmdStream]));

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_DOUBLE); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4), STYPE_DOUBLE | DTYPE_DOUBLE, 0, 0, 0));
			break;
		case cmdPushCmplxAbs:
			cmdStream += 2;

			{
				unsigned int currShift = *(unsigned int*)(cmdStream + 4);
				while(currShift >= 4)
				{
					currShift -= 4;
					genStackPtr--;
					*genStackPtr = *((unsigned int*)(&genParams[*(int*)cmdStream + currShift]));
				}
				DBG(genStackTypes.push_back((asmStackType)(*(unsigned int*)(cmdStream + 4)|0x80000000)));
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream), STYPE_COMPLEX_TYPE | DTYPE_COMPLEX_TYPE, 0, 0, 0));
			}

			cmdStream += 8;
			break;

		case cmdPushCharRel:
			cmdStream += 2;

			genStackPtr--;
			*genStackPtr = genParams[*(int*)cmdStream + paramTop.back()];

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4) + paramTop.back(), STYPE_INT | DTYPE_CHAR | bitAddrRel, 0, 0, 0));
			break;
		case cmdPushShortRel:
			cmdStream += 2;

			genStackPtr--;
			*genStackPtr =  *((short*)(&genParams[*(int*)cmdStream + paramTop.back()]));

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4) + paramTop.back(), STYPE_INT | DTYPE_SHORT | bitAddrRel, 0, 0, 0));
			break;
		case cmdPushIntRel:
			cmdStream += 2;

			genStackPtr--;
			*genStackPtr = *((int*)(&genParams[*(int*)cmdStream + paramTop.back()]));

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4) + paramTop.back(), STYPE_INT | DTYPE_INT | bitAddrRel, 0, 0, 0));
			break;
		case cmdPushFloatRel:
			cmdStream += 2;

			genStackPtr -= 2;
			*(double*)(genStackPtr) = (double)*((float*)(&genParams[*(int*)cmdStream + paramTop.back()]));

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_DOUBLE); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4) + paramTop.back(), STYPE_DOUBLE | DTYPE_FLOAT | bitAddrRel, 0, 0, 0));
			break;
		case cmdPushDorLRel:
			cmdStream += 2;

			genStackPtr -= 2;
			*(double*)(genStackPtr) = *((double*)(&genParams[*(int*)cmdStream + paramTop.back()]));

			cmdStream += 4;
			DBG(genStackTypes.push_back(STYPE_DOUBLE); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)(cmdStream-4) + paramTop.back(), STYPE_DOUBLE | DTYPE_DOUBLE | bitAddrRel, 0, 0, 0));
			break;
		case cmdPushCmplxRel:
			cmdStream += 2;

			{
				int valind = *(int*)cmdStream + paramTop.back();
				unsigned int currShift = *(unsigned int*)(cmdStream + 4);
				while(currShift >= 4)
				{
					currShift -= 4;
					genStackPtr--;
					*genStackPtr = *((unsigned int*)(&genParams[valind + currShift]));
				}
				DBG(genStackTypes.push_back((asmStackType)(*(unsigned int*)(cmdStream + 4)|0x80000000)));
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind + paramTop.back(), STYPE_COMPLEX_TYPE | DTYPE_COMPLEX_TYPE | bitAddrRel, 0, 0, 0));
			}

			cmdStream += 8;
			break;

		case cmdPushCharStk:
			cmdStream += 2;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)cmdStream + *genStackPtr, STYPE_INT | DTYPE_CHAR | bitShiftStk, 0, 0, 0));

			*genStackPtr = genParams[*(int*)cmdStream + *genStackPtr];

			cmdStream += 4;
			break;
		case cmdPushShortStk:
			cmdStream += 2;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)cmdStream + *genStackPtr, STYPE_INT | DTYPE_SHORT | bitShiftStk, 0, 0, 0));

			*genStackPtr =  *((short*)(&genParams[*(int*)cmdStream + *genStackPtr]));

			cmdStream += 4;
			break;
		case cmdPushIntStk:
			cmdStream += 2;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)cmdStream + *genStackPtr, STYPE_INT | DTYPE_INT | bitShiftStk, 0, 0, 0));

			*genStackPtr = *((int*)(&genParams[*(int*)cmdStream + *genStackPtr]));

			cmdStream += 4;
			break;
		case cmdPushFloatStk:
			cmdStream += 2;
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)cmdStream + *genStackPtr, STYPE_DOUBLE | DTYPE_FLOAT | bitShiftStk, 0, 0, 0));

			genStackPtr--;
			*(double*)(genStackPtr) = (double)*((float*)(&genParams[*(int*)cmdStream + *(genStackPtr+1)]));

			cmdStream += 4;
			break;
		case cmdPushDorLStk:
			DBG(genStackTypes.push_back(STYPE_INT); PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)cmdStream + *genStackPtr, STYPE_DOUBLE | DTYPE_DOUBLE | bitShiftStk, 0, 0, 0));
			cmdStream += 2;

			genStackPtr--;
			*(double*)(genStackPtr) = *((double*)(&genParams[*(int*)cmdStream + *(genStackPtr+1)]));

			cmdStream += 4;
			break;
		case cmdPushCmplxStk:
			cmdStream += 2;
			DBG(genStackTypes.push_back((asmStackType)(*(unsigned int*)(cmdStream + 4)|0x80000000)));
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, *(int*)cmdStream + *genStackPtr, STYPE_COMPLEX_TYPE | DTYPE_COMPLEX_TYPE | bitShiftStk, 0, 0, 0));

			{
				unsigned int shift = *(int*)cmdStream + *genStackPtr;
				genStackPtr++;
				unsigned int currShift = *(unsigned int*)(cmdStream + 4);
				while(currShift >= 4)
				{
					currShift -= 4;
					genStackPtr--;
					*genStackPtr = *((unsigned int*)(&genParams[shift + currShift]));
				}
			}

			cmdStream += 8;
			break;

		case cmdDTOF:
			*((float*)(genStackPtr+1)) = float(*(double*)(genStackPtr));
			genStackPtr++;
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, 0, 0, 0));
			break;
		case cmdMovRTaP:
			{
				int valind;
				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;

				asmDataType dt = flagDataType(cFlag);

				valind = *(int*)cmdStream;
				cmdStream += 4;

				unsigned int sizeOfVar = 0;
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					sizeOfVar = *(unsigned int*)cmdStream;
					cmdStream += 4;
				}
				unsigned int sizeOfVarConst = sizeOfVar;

				valind += genParams.size();

				if(valind + sizeOfVarConst > genParams.size())
					genParams.reserve(genParams.size()+128);
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					unsigned int currShift = sizeOfVar;
					while(sizeOfVar >= 4)
					{
						currShift -= 4;
						*((unsigned int*)(&genParams[valind+currShift])) = *(genStackPtr+sizeOfVar/4-1);
						sizeOfVar -= 4;
					}
					genStackPtr += sizeOfVarConst / 4;
					assert(sizeOfVar == 0);
				}else if(dt == DTYPE_FLOAT){
					*((float*)(&genParams[valind])) = float(*(double*)(genStackPtr));
					genStackPtr += 2;
				}else if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG){
					*((unsigned int*)(&genParams[valind])) = *genStackPtr;
					*((unsigned int*)(&genParams[valind+4])) = *(genStackPtr+1);
					genStackPtr += 2;
				}else if(dt == DTYPE_INT){
					*((unsigned int*)(&genParams[valind])) = *genStackPtr;
					genStackPtr++;
				}else if(dt == DTYPE_SHORT){
					*((short*)(&genParams[valind])) = *(short*)(genStackPtr);
					genStackPtr++;
				}else if(dt == DTYPE_CHAR){
					genParams[valind] = *(char*)(genStackPtr);
					genStackPtr++;
				}

				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0, sizeOfVarConst));
			}
			break;
		case cmdPushImmt:
			{
				unsigned short sdata;
				unsigned char cdata;

				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;

				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);

				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					highDW = *(unsigned int*)cmdStream;
					lowDW = *(unsigned int*)(cmdStream+4);
					cmdStream += 8;
				}else if(dt == DTYPE_FLOAT || dt == DTYPE_INT){
					lowDW = *(unsigned int*)cmdStream;
					cmdStream += 4;
				}else if(dt == DTYPE_SHORT){
					sdata = *(unsigned short*)cmdStream;
					cmdStream += 2;
					lowDW = (sdata>0?sdata:sdata|0xFFFF0000);
				}else if(dt == DTYPE_CHAR){
					cdata = *(unsigned char*)cmdStream;
					cmdStream++;
					lowDW = cdata;
				}
				
				if(dt == DTYPE_FLOAT && st == STYPE_DOUBLE)	//expand float to double
				{
					genStackPtr -= 2;
					
					union
					{
					    unsigned int ui;
					    float f;
					} u;
					
					u.ui = lowDW;
					
					*(double*)(genStackPtr) = u.f;
				}else if(st == STYPE_DOUBLE || st == STYPE_LONG)
				{
					genStackPtr--;
					*genStackPtr = lowDW;
					genStackPtr--;
					*genStackPtr = highDW;
				}else{
					genStackPtr--;
					*genStackPtr = lowDW;
				}

				DBG(genStackTypes.push_back(st));
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, cFlag, 0, highDW, lowDW));
			}
			break;
		case cmdPush:
			assert(!"cmdPush is illegal in VM");
			break;
		case cmdPop:
			{
				unsigned int varSize = *(unsigned int*)cmdStream;
				cmdStream += 4;

				genStackPtr += varSize >> 2;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
				unsigned int sizeOfVar = varSize;
				unsigned int count = genStackTypes.back() & 0x80000000 ? genStackTypes.back() & ~0x80000000 : typeSizeS[genStackTypes.back()];
				for(unsigned int n = 0; n < sizeOfVar/count; n++)
					genStackTypes.pop_back();
#endif
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, sizeOfVar, 0, 0));
			}
			break;
		case cmdMov:
			{
				int valind = -1, shift = 0;
				unsigned short sdata;
				unsigned char cdata;
				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;
				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);

				valind = *(int*)cmdStream;
				cmdStream += 4;

				if(flagShiftStk(cFlag))
				{
					shift = *genStackPtr;
					genStackPtr++;

					//if(int(shift) < 0)
					//	throw std::string("ERROR: array index out of bounds (negative)");
					DBG(genStackTypes.pop_back());
				}

				unsigned int sizeOfVar = 0;
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					sizeOfVar = *(unsigned int*)cmdStream;
					cmdStream += 4;
				}

				if(flagAddrRel(cFlag))
					valind += paramTop.back();
				if(flagShiftStk(cFlag))
					valind += shift;

				if(dt == DTYPE_COMPLEX_TYPE)
				{
					unsigned int currShift = sizeOfVar;
					while(currShift >= 4)
					{
						currShift -= 4;
						*((unsigned int*)(&genParams[valind+currShift])) = *(genStackPtr+(currShift>>2));
					}
					assert(currShift == 0);
				}else if(dt == DTYPE_FLOAT && st == STYPE_DOUBLE)
				{
					*((float*)(&genParams[valind])) = float(*(double*)(genStackPtr));
				}else if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					*((unsigned int*)(&genParams[valind])) = *genStackPtr;
					*((unsigned int*)(&genParams[valind+4])) = *(genStackPtr+1);
				}else if(dt == DTYPE_FLOAT || dt == DTYPE_INT)
				{
					*((unsigned int*)(&genParams[valind])) = *genStackPtr;
				}else if(dt == DTYPE_SHORT)
				{
					sdata = (unsigned short)(*genStackPtr);
					*((unsigned short*)(&genParams[valind])) = sdata;
				}else if(dt == DTYPE_CHAR)
				{
					cdata = (unsigned char)(*genStackPtr);
					genParams[valind] = cdata;
				}

				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0, sizeOfVar));
			}
			break;
		case cmdCTI:
		{
			unsigned int	uintVal, uintVal2;
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			uintVal = *(unsigned int*)cmdStream;
			cmdStream += 4;
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				uintVal2 = int(*(double*)(genStackPtr));
				genStackPtr++;
				break;
			case OTYPE_LONG:
				uintVal2 = int(*(long long*)(genStackPtr));
				genStackPtr++;
				break;
			case OTYPE_INT:
				uintVal2 = *genStackPtr;
				break;
			default:
				uintVal2 = 0;
			}
			*genStackPtr = uintVal*uintVal2;
			DBG(genStackTypes.pop_back());
			DBG(genStackTypes.push_back(STYPE_INT));
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0));
		}
			break;
		case cmdRTOI:
			{
				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;
				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);
				DBG(genStackTypes.pop_back());

				if(st == STYPE_DOUBLE && dt == DTYPE_INT)
				{
					int temp = int(*(double*)(genStackPtr));
					genStackPtr++;
					*genStackPtr = temp;
					DBG(genStackTypes.push_back(STYPE_INT));
				}else if(st == STYPE_DOUBLE && dt == DTYPE_LONG){
					*(long long*)(genStackPtr) = (long long)*(double*)(genStackPtr);
					DBG(genStackTypes.push_back(STYPE_LONG));
				}
				
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, cFlag, 0));
			}
			break;
		case cmdITOR:
			{
				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;
				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);
				DBG(genStackTypes.pop_back());

				if(st == STYPE_INT && dt == DTYPE_DOUBLE)
				{
					double temp = double(*(int*)genStackPtr);
					genStackPtr--;
					*(double*)(genStackPtr) = temp;
					DBG(genStackTypes.push_back(STYPE_DOUBLE));
				}
				if(st == STYPE_LONG && dt == DTYPE_DOUBLE)
				{
					double temp = double(*(long long*)(genStackPtr));
					*(double*)(genStackPtr) = temp;
					DBG(genStackTypes.push_back(STYPE_DOUBLE));
				}

				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, cFlag, 0));
			}
			break;
		case cmdCallStd:
			{
				unsigned int valind = *(unsigned int*)(cmdStream);
				cmdStream += sizeof(unsigned int);

				if(exFunctions[valind]->funcPtr == NULL)
				{
					double val = *(double*)(genStackPtr);
					DBG(genStackTypes.pop_back());

					if(exFunctions[valind]->nameHash == GetStringHash("cos"))
						val = cos(val);
					else if(exFunctions[valind]->nameHash == GetStringHash("sin"))
						val = sin(val);
					else if(exFunctions[valind]->nameHash == GetStringHash("tan"))
						val = tan(val);
					else if(exFunctions[valind]->nameHash == GetStringHash("ctg"))
						val = 1.0/tan(val);
					else if(exFunctions[valind]->nameHash == GetStringHash("ceil"))
						val = ceil(val);
					else if(exFunctions[valind]->nameHash == GetStringHash("floor"))
						val = floor(val);
					else if(exFunctions[valind]->nameHash == GetStringHash("sqrt"))
						val = sqrt(val);
					else{
						done = true;
						printf(execError, "ERROR: there is no such function: %s", exFunctions[valind]->name);
						break;
					}

					if(fabs(val) < 1e-10)
						val = 0.0;
					*(double*)(genStackPtr) = val;
					DBG(genStackTypes.push_back(STYPE_DOUBLE));
				}else{
				    done = !RunExternalFunction(valind);
				}
				DBG(m_FileStream << pos2 << dec << " CALLS " << exFunctions[valind]->name << ";");
			}
			break;
		case cmdSwap:
		{
			unsigned int valind;
			cFlag = *(CmdFlag*)cmdStream;
			cmdStream += 2;
			switch(cFlag)
			{
			case (STYPE_DOUBLE)+(DTYPE_DOUBLE):
			case (STYPE_LONG)+(DTYPE_LONG):
				valind = *genStackPtr;
				*genStackPtr = *(genStackPtr+2);
				*(genStackPtr+2) = valind;

				valind = *(genStackPtr+1);
				*(genStackPtr+1) = *(genStackPtr+3);
				*(genStackPtr+3) = valind;
				break;
			case (STYPE_DOUBLE)+(DTYPE_INT):
			case (STYPE_LONG)+(DTYPE_INT):
				valind = *(genStackPtr);
				*(genStackPtr) = *(genStackPtr+1);
				*(genStackPtr+1) = valind;

				valind = *(genStackPtr+1);
				*(genStackPtr+1) = *(genStackPtr+2);
				*(genStackPtr+2) = valind;
				break;
			case (STYPE_INT)+(DTYPE_DOUBLE):
			case (STYPE_INT)+(DTYPE_LONG):
				valind = *(genStackPtr+1);
				*(genStackPtr+1) = *(genStackPtr+2);
				*(genStackPtr+2) = valind;

				valind = *(genStackPtr);
				*(genStackPtr) = *(genStackPtr+1);
				*(genStackPtr+1) = valind;
				break;
			case (STYPE_INT)+(DTYPE_INT):
				valind = *(genStackPtr);
				*(genStackPtr) = *(genStackPtr+1);
				*(genStackPtr+1) = valind;
				break;
			default:
				done = true;
				strcpy(execError, "ERROR: cmdSwap, unimplemented type combo");
			}
			DBG(st = genStackTypes[genStackTypes.size()-2]);
			DBG(genStackTypes[genStackTypes.size()-2] = genStackTypes[genStackTypes.size()-1]);
			DBG(genStackTypes[genStackTypes.size()-1] = st);

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, cFlag, 0));
		}
			break;
		case cmdCopy:
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
			case OTYPE_LONG:
				genStackPtr -= 2;
				*genStackPtr = *(genStackPtr+2);
				*(genStackPtr+1) = *(genStackPtr+3);
				break;
			case OTYPE_INT:
				genStackPtr--;
				*genStackPtr = *(genStackPtr+1);
				break;
			}
			DBG(genStackTypes.push_back(genStackTypes[genStackTypes.size()-1]));

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, oFlag));
			break;
		case cmdJmp:
			cmdStream = cmdStreamBase + *(int*)cmdStream;
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
			break;
		case cmdJmpZ:
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			if(oFlag == OTYPE_DOUBLE){
				if(*(double*)(genStackPtr) == 0.0)
					cmdStream = cmdStreamBase + *(int*)cmdStream;
				else
					cmdStream += 4;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_LONG){
				if(*(long long*)(genStackPtr) == 0L)
					cmdStream = cmdStreamBase + *(int*)cmdStream;
				else
					cmdStream += 4;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_INT){
				if(*genStackPtr == 0)
					cmdStream = cmdStreamBase + *(int*)cmdStream;
				else
					cmdStream += 4;
				genStackPtr++;
			}
			DBG(genStackTypes.pop_back());
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
			break;
		case cmdJmpNZ:
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			if(oFlag == OTYPE_DOUBLE){
				if(*(double*)(genStackPtr) != 0.0)
					cmdStream = cmdStreamBase + *(int*)cmdStream;
				else
					cmdStream += 4;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_LONG){
				if(*(long long*)(genStackPtr) == 0L)
					cmdStream = cmdStreamBase + *(int*)cmdStream;
				else
					cmdStream += 4;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_INT){
				if(*genStackPtr != 0)
					cmdStream = cmdStreamBase + *(int*)cmdStream;
				else
					cmdStream += 4;
				genStackPtr++;
			}
			DBG(genStackTypes.pop_back());
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
			break;
		case cmdPushVTop:
			size_t valtop;
			valtop = genParams.size();
			paramTop.push_back(valtop);

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, (unsigned int)valtop, 0, 0));
			break;
		case cmdPopVTop:
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, paramTop.back(), 0, 0));
			genParams.shrink(paramTop.back());
			paramTop.pop_back();
			break;
		case cmdCall:
			{
				unsigned short retFlag;
				unsigned int fAddress = *(unsigned int*)cmdStream;
				cmdStream += 4;
				retFlag = *(unsigned short*)cmdStream;
				cmdStream += 2;

				if(fAddress == CALL_BY_POINTER)
				{
					fAddress = *genStackPtr;
					genStackPtr++;
				}
				fcallStack.push_back(cmdStream);// callStack.push_back(CallStackInfo(cmdStream, (unsigned int)genStackSize, uintVal));
				cmdStream = cmdStreamBase + fAddress;
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0, retFlag));
			}
			break;
		case cmdReturn:
			{
				unsigned short	retFlag, popCnt;
				retFlag = *(unsigned short*)cmdStream;
				cmdStream += 2;
				popCnt = *(unsigned short*)cmdStream;
				cmdStream += 2;
				if(retFlag & bitRetError)
				{
					done = true;
					strcpy(execError, "ERROR: function didn't return a value");
					break;
				}
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, popCnt, 0, 0, retFlag));
				for(int pops = 0; pops < (popCnt > 0 ? popCnt : 1); pops++)
				{
					genParams.shrink(paramTop.back());
					paramTop.pop_back();
				}
				if(fcallStack.size() == 0)
				{
					retType = (OperFlag)(retFlag & 0x0FFF);
					done = true;
					break;
				}
				cmdStream = fcallStack.back();
				fcallStack.pop_back();
			}
			break;
		case cmdPushV:
			genParams.resize(genParams.size() + *(int*)cmdStream);
			cmdStream += 4;
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
			break;
		case cmdNop:
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, 0));
			break;

		case cmdITOL:
			if((int)(*genStackPtr) < 0)
			{
				unsigned int valind = *genStackPtr;
				*genStackPtr = 0xFFFFFFFF;
				genStackPtr--;
				*genStackPtr = valind;
			}else{
				unsigned int valind = *genStackPtr;
				*genStackPtr = 0;
				genStackPtr--;
				*genStackPtr = valind;
			}
			DBG(genStackTypes.back() = STYPE_LONG);
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, 0));
			break;
		case cmdLTOI:
			genStackPtr++;
			*genStackPtr = *(genStackPtr-1);
			DBG(genStackTypes.back() = STYPE_INT);
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, 0));
			break;
		case cmdSetRange:
		{
			cFlag = *(CmdFlag*)cmdStream;
			cmdStream += 2;
			unsigned int start = *(unsigned int*)cmdStream;
			cmdStream += 4;
			unsigned int count = *(unsigned int*)cmdStream;
			cmdStream += 4;
			
			start += paramTop.back();
			for(unsigned int varNum = 0; varNum < count; varNum++)
			{
				switch(cFlag)
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
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, cFlag, 0, uintVal2));
		}
			break;
		case cmdGetAddr:
			genStackPtr--;
			*genStackPtr = *(unsigned int*)cmdStream + paramTop.back();
			cmdStream += 4;

			DBG(genStackTypes.push_back(STYPE_INT));
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0));
			break;
		case cmdFuncAddr:
		{
			unsigned int valind = *(unsigned int*)(cmdStream);
			cmdStream += sizeof(unsigned int);

			assert(sizeof(exFunctions[valind]->funcPtr) == 4);

			genStackPtr--;
			if(exFunctions[valind]->funcPtr == NULL)
				*genStackPtr = exFunctions[valind]->address;
			else
				*genStackPtr = (unsigned int)((unsigned long long)(exFunctions[valind]->funcPtr));
			DBG(genStackTypes.push_back(STYPE_INT));
		}
			break;
		case cmdNeg:
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				*(double*)(genStackPtr) = -*(double*)(genStackPtr);
				break;
			case OTYPE_LONG:
				*(long long*)(genStackPtr) = -*(long long*)(genStackPtr);
				break;
			case OTYPE_INT:
				*(int*)(genStackPtr) = -*(int*)(genStackPtr);
				break;
			default:
				done = true;
				strcpy(execError, "ERROR: Operation is not implemented");
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, oFlag));
			break;
		case cmdBitNot:
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			switch(oFlag)
			{
			case OTYPE_LONG:
				*(long long*)(genStackPtr) = ~*(long long*)(genStackPtr);
				break;
			case OTYPE_INT:
				*(int*)(genStackPtr) = ~*(int*)(genStackPtr);
				break;
			default:
				done = true;
				strcpy(execError, "ERROR: Operation is not implemented");
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, oFlag));
			break;
		case cmdLogNot:
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				*(double*)(genStackPtr) = fabs(*(double*)(genStackPtr)) < 1e-10;
				break;
			case OTYPE_LONG:
				*(long long*)(genStackPtr) = !*(long long*)(genStackPtr);
				break;
			case OTYPE_INT:
				*(int*)(genStackPtr) = !*(int*)(genStackPtr);
				break;
			default:
				done = true;
				strcpy(execError, "ERROR: Operation is not implemented");
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, oFlag));
			break;
		case cmdIncAt:
		case cmdDecAt:
			{
				int valind = 0, shift = 0, size = 0;
				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;
				asmDataType dt = flagDataType(cFlag);	//Data type

				if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
				{
					valind = *(int*)cmdStream;
					cmdStream += 4;
				}
				if(flagShiftStk(cFlag))
				{
					shift = *genStackPtr;
					genStackPtr++;

					if(shift < 0)
					{
						done = true;
						strcpy(execError, "ERROR: array index out of bounds (negative)");
						break;
					}
				}
				if(flagSizeOn(cFlag))
				{
					size = *(int*)cmdStream;
					cmdStream += 4;

					if(shift >= size)
					{
						done = true;
						strcpy(execError, "ERROR: array index out of bounds (overflow)");
						break;
					}
				}
				if(flagSizeStk(cFlag))
				{
					size = *genStackPtr;
					genStackPtr++;

					if(shift >= size)
					{
						done = true;
						strcpy(execError, "ERROR: array index out of bounds (overflow)");
						break;
					}
				}

				if(flagAddrRel(cFlag))
					valind += paramTop.back();
				if(flagShiftStk(cFlag))
					valind += shift;

				if(flagPushBefore(cFlag))
				{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						genStackPtr -= 2;
						*genStackPtr = *(int*)(&genParams[valind]);
						*(genStackPtr+1) = *(int*)(&genParams[valind+4]);
					}else if(dt == DTYPE_FLOAT){
						double res = (double)(*((float*)(&genParams[valind])));
						genStackPtr -= 2;
						*(double*)(genStackPtr) = res;
					}else if(dt == DTYPE_INT){
						genStackPtr--;
						*genStackPtr = *((int*)(&genParams[valind]));
					}else if(dt == DTYPE_SHORT){
						genStackPtr--;
						*genStackPtr = *((short*)(&genParams[valind]));
					}else if(dt == DTYPE_CHAR){
						genStackPtr--;
						*genStackPtr = *((char*)(&genParams[valind]));
					}

					DBG(genStackTypes.push_back(stackTypeForDataType(dt)));
				}

				switch(cmd + (dt << 16))
				{
				case cmdIncAt+(DTYPE_DOUBLE<<16):
					*((double*)(&genParams[valind])) += 1.0;
					break;
				case cmdIncAt+(DTYPE_FLOAT<<16):
					*((float*)(&genParams[valind])) += 1.0f;
					break;
				case cmdIncAt+(DTYPE_LONG<<16):
					*((long long*)(&genParams[valind])) += 1;
					break;
				case cmdIncAt+(DTYPE_INT<<16):
					*((int*)(&genParams[valind])) += 1;
					break;
				case cmdIncAt+(DTYPE_SHORT<<16):
					*((short*)(&genParams[valind])) += 1;
					break;
				case cmdIncAt+(DTYPE_CHAR<<16):
					*((unsigned char*)(&genParams[valind])) += 1;
					break;

				case cmdDecAt+(DTYPE_DOUBLE<<16):
					*((double*)(&genParams[valind])) -= 1.0;
					break;
				case cmdDecAt+(DTYPE_FLOAT<<16):
					*((float*)(&genParams[valind])) -= 1.0f;
					break;
				case cmdDecAt+(DTYPE_LONG<<16):
					*((long long*)(&genParams[valind])) -= 1;
					break;
				case cmdDecAt+(DTYPE_INT<<16):
					*((int*)(&genParams[valind])) -= 1;
					break;
				case cmdDecAt+(DTYPE_SHORT<<16):
					*((short*)(&genParams[valind])) -= 1;
					break;
				case cmdDecAt+(DTYPE_CHAR<<16):
					*((unsigned char*)(&genParams[valind])) -= 1;
					break;
				}

				if(flagPushAfter(cFlag))
				{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						genStackPtr -= 2;
						*genStackPtr = *(int*)(&genParams[valind]);
						*(genStackPtr+1) = *(int*)(&genParams[valind+4]);
					}else if(dt == DTYPE_FLOAT){
						double res = (double)(*((float*)(&genParams[valind])));
						genStackPtr -= 2;
						*(double*)(genStackPtr) = res;
					}else if(dt == DTYPE_INT){
						genStackPtr--;
						*genStackPtr = *((int*)(&genParams[valind]));
					}else if(dt == DTYPE_SHORT){
						genStackPtr--;
						*genStackPtr = *((short*)(&genParams[valind]));
					}else if(dt == DTYPE_CHAR){
						genStackPtr--;
						*genStackPtr = *((char*)(&genParams[valind]));
					}

					DBG(genStackTypes.push_back(stackTypeForDataType(dt)));
				}
			
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0));
			}
			break;
		case cmdAdd:
		case cmdSub:
		case cmdMul:
		case cmdDiv:
		case cmdPow:
		case cmdMod:
		case cmdLess:
		case cmdGreater:
		case cmdLEqual:
		case cmdGEqual:
		case cmdEqual:
		case cmdNEqual:
		case cmdShl:
		case cmdShr:
		case cmdBitAnd:
		case cmdBitOr:
		case cmdBitXor:
		case cmdLogAnd:
		case cmdLogOr:
		case cmdLogXor:
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			switch(cmd + (oFlag << 16))
			{
			case cmdAdd+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) += *(double*)(genStackPtr);
				break;
			case cmdAdd+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) += *(long long*)(genStackPtr);
				break;
			case cmdAdd+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) += *(int*)(genStackPtr);
				break;
			case cmdSub+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) -= *(double*)(genStackPtr);
				break;
			case cmdSub+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) -= *(long long*)(genStackPtr);
				break;
			case cmdSub+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) -= *(int*)(genStackPtr);
				break;
			case cmdMul+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) *= *(double*)(genStackPtr);
				break;
			case cmdMul+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) *= *(long long*)(genStackPtr);
				break;
			case cmdMul+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) *= *(int*)(genStackPtr);
				break;
			case cmdDiv+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) /= *(double*)(genStackPtr);
				break;
			case cmdDiv+(OTYPE_LONG<<16):
				if(*(long long*)(genStackPtr))
				{
					*(long long*)(genStackPtr+2) /= *(long long*)(genStackPtr);
				}else{
					strcpy(execError, "ERROR: Integer division by zero");
					done = true;
				}
				break;
			case cmdDiv+(OTYPE_INT<<16):
				if(*(int*)(genStackPtr))
				{
					*(int*)(genStackPtr+1) /= *(int*)(genStackPtr);
				}else{
					strcpy(execError, "ERROR: Integer division by zero");
					done = true;
				}
				break;
			case cmdPow+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) = pow(*(double*)(genStackPtr+2), *(double*)(genStackPtr));
				break;
			case cmdPow+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = vmLongPow(*(long long*)(genStackPtr+2), *(long long*)(genStackPtr));
				break;
			case cmdPow+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = (int)pow((double)*(int*)(genStackPtr+1), (double)*(int*)(genStackPtr));
				break;
			case cmdMod+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) = fmod(*(double*)(genStackPtr+2), *(double*)(genStackPtr));
				break;
			case cmdMod+(OTYPE_LONG<<16):
				if(*(long long*)(genStackPtr))
				{
					*(long long*)(genStackPtr+2) %= *(long long*)(genStackPtr);
				}else{
					strcpy(execError, "ERROR: Integer division by zero");
					done = true;
				}
				break;
			case cmdMod+(OTYPE_INT<<16):
				if(*(int*)(genStackPtr))
				{
					*(int*)(genStackPtr+1) %= *(int*)(genStackPtr);
				}else{
					strcpy(execError, "ERROR: Integer division by zero");
					done = true;
				}				
				break;
			case cmdLess+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) < *(double*)(genStackPtr);
				break;
			case cmdLess+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) < *(long long*)(genStackPtr);
				break;
			case cmdLess+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) < *(int*)(genStackPtr);
				break;
			case cmdGreater+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) > *(double*)(genStackPtr);
				break;
			case cmdGreater+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) > *(long long*)(genStackPtr);
				break;
			case cmdGreater+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) > *(int*)(genStackPtr);
				break;
			case cmdLEqual+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) <= *(double*)(genStackPtr);
				break;
			case cmdLEqual+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) <= *(long long*)(genStackPtr);
				break;
			case cmdLEqual+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) <= *(int*)(genStackPtr);
				break;
			case cmdGEqual+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) >= *(double*)(genStackPtr);
				break;
			case cmdGEqual+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) >= *(long long*)(genStackPtr);
				break;
			case cmdGEqual+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) >= *(int*)(genStackPtr);
				break;
			case cmdEqual+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) == *(double*)(genStackPtr);
				break;
			case cmdEqual+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) == *(long long*)(genStackPtr);
				break;
			case cmdEqual+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) == *(int*)(genStackPtr);
				break;
			case cmdNEqual+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr+2) = *(double*)(genStackPtr+2) != *(double*)(genStackPtr);
				break;
			case cmdNEqual+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) != *(long long*)(genStackPtr);
				break;
			case cmdNEqual+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) != *(int*)(genStackPtr);
				break;
			case cmdShl+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) << *(long long*)(genStackPtr);
				break;
			case cmdShl+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) << *(int*)(genStackPtr);
				break;
			case cmdShr+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) >> *(long long*)(genStackPtr);
				break;
			case cmdShr+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) >> *(int*)(genStackPtr);
				break;
			case cmdBitAnd+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) & *(long long*)(genStackPtr);
				break;
			case cmdBitAnd+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) & *(int*)(genStackPtr);
				break;
			case cmdBitOr+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) | *(long long*)(genStackPtr);
				break;
			case cmdBitOr+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) | *(int*)(genStackPtr);
				break;
			case cmdBitXor+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) ^ *(long long*)(genStackPtr);
				break;
			case cmdBitXor+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) ^ *(int*)(genStackPtr);
				break;
			case cmdLogAnd+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) && *(long long*)(genStackPtr);
				break;
			case cmdLogAnd+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) && *(int*)(genStackPtr);
				break;
			case cmdLogOr+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = *(long long*)(genStackPtr+2) || *(long long*)(genStackPtr);
				break;
			case cmdLogOr+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = *(int*)(genStackPtr+1) || *(int*)(genStackPtr);
				break;
			case cmdLogXor+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr+2) = !!(*(long long*)(genStackPtr+2)) ^ !!(*(long long*)(genStackPtr));
				break;
			case cmdLogXor+(OTYPE_INT<<16):
				*(int*)(genStackPtr+1) = !!(*(int*)(genStackPtr+1)) ^ !!(*(int*)(genStackPtr));
				break;
			default:
				done = true;
				strcpy(execError, "ERROR: Operation is not implemented");
			}
			if(oFlag == OTYPE_INT)
			{
				genStackPtr++;
			}else{
				genStackPtr += 2;
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, oFlag));
			DBG(genStackTypes.pop_back());
			
			break;
		}

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
		unsigned int typeSizeS[] = { 1, 2, 0, 2 };
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
		}
		m_FileStream << ";\r\n" << std::flush;
#endif
	}
}

#ifdef _MSC_VER
// X86 implementation
bool Executor::RunExternalFunction(unsigned int funcID)
{
    unsigned int bytesToPop = exFuncInfo[funcID].bytesToPop;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	unsigned int typeSizeS[] = { 4, 8, 4, 8 };
    unsigned int paramSize = bytesToPop;
    while(paramSize > 0)
    {
        paramSize -= genStackTypes.back() & 0x80000000 ? genStackTypes.back() & ~0x80000000 : typeSizeS[genStackTypes.back()];;
        genStackTypes.pop_back();
    }
#endif
    unsigned int *stackStart = (genStackPtr+bytesToPop/4-1);
    for(unsigned int i = 0; i < bytesToPop/4; i++)
    {
        __asm mov eax, dword ptr[stackStart]
        __asm push dword ptr[eax];
        stackStart--;
    }
    genStackPtr += bytesToPop/4;

    void* fPtr = exFunctions[funcID]->funcPtr;
    unsigned int fRes;
    __asm{
        mov ecx, fPtr;
        call ecx;
        add esp, bytesToPop;
        mov fRes, eax;
    }
    if(exTypes[exFunctions[funcID]->retType]->size != 0)
    {
        genStackPtr--;
        *genStackPtr = fRes;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
        if(exTypes[exFunctions[funcID]->retType]->type == TypeInfo::TYPE_COMPLEX)
            genStackTypes.push_back((asmStackType)(0x80000000 | exTypes[exFunctions[funcID]->retType]->size));
        else
            genStackTypes.push_back(podTypeToStackType[exTypes[exFunctions[funcID]->retType]->type]);
#endif
    }
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

const char* Executor::GetResult() throw()
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

const char*	Executor::GetExecError() throw()
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
void PrintInstructionText(ostream* stream, CmdID cmd, unsigned int pos2, unsigned int valind, const CmdFlag cFlag, const OperFlag oFlag, unsigned int dw0, unsigned int dw1)
{
	asmStackType st = flagStackType(cFlag);
	asmDataType dt = flagDataType(cFlag);
	char*	typeInfoS[] = { "int", "long", "complex", "double" };
	char*	typeInfoD[] = { "char", "short", "int", "long", "float", "double", "complex" };

	unsigned int	DWords[] = { dw0, dw1 };

	size_t beginPos = stream->tellp();
	(*stream) << pos2;
	char temp[32];
	sprintf(temp, "%d", pos2);
	unsigned int addSp = 5 - (unsigned int)strlen(temp);
	for(unsigned int i = 0; i < addSp; i++)
		(*stream) << ' ';
	switch(cmd)
	{
	case cmdPushCharAbs:
	case cmdPushShortAbs:
	case cmdPushIntAbs:
	case cmdPushFloatAbs:
	case cmdPushDorLAbs:
	case cmdPushCmplxAbs:
	case cmdPushCharRel:
	case cmdPushShortRel:
	case cmdPushIntRel:
	case cmdPushFloatRel:
	case cmdPushDorLRel:
	case cmdPushCmplxRel:
	case cmdPushCharStk:
	case cmdPushShortStk:
	case cmdPushIntStk:
	case cmdPushFloatStk:
	case cmdPushDorLStk:
	case cmdPushCmplxStk:
	case cmdPush:
		(*stream) << " PUSH ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "<-";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007];

		(*stream) << " PTR[";
		(*stream) << valind << "] //";
		
		if(flagAddrRel(cFlag))
			(*stream) << "rel+top";
		if(flagAddrRelTop(cFlag))
			(*stream) << "max+top";
		if(flagShiftStk(cFlag))
			(*stream) << "+shiftstk";
		if(st == STYPE_COMPLEX_TYPE)
			(*stream) << " sizeof " << dw1;
		break;
	case cmdPushVTop:
		(*stream) << " PUSHT " << valind << ";";
		break;
	case cmdPopVTop:
		(*stream) << " POPT " << valind << ";";
		break;
	case cmdCall:
		//(*stream) << " CALL " << valind << " size: " << dw0 << ";";
		(*stream) << " CALL " << valind << " ret " << (dw0 & bitRetSimple ? "simple " : "") << "size: ";
		if(dw0 & bitRetSimple)
		{
			OperFlag oFlag = (OperFlag)(dw0 & 0x0FFF);
			if(oFlag == OTYPE_DOUBLE)
				(*stream) << "double";
			if(oFlag == OTYPE_LONG)
				(*stream) << "long";
			if(oFlag == OTYPE_INT)
				(*stream) << "int";
		}else{
			(*stream) << (dw0&0x0FFF) << "";
		}
		break;
	case cmdReturn:
		(*stream) << " RET " << valind;
		if(dw0 & bitRetError)
			(*stream) << " error;";
		if(dw0 & bitRetSimple)
		{
			OperFlag oFlag = (OperFlag)(dw0 & 0x0FFF);
			if(oFlag == OTYPE_DOUBLE)
				(*stream) << " double;";
			else if(oFlag == OTYPE_LONG)
				(*stream) << " long;";
			else if(oFlag == OTYPE_INT)
				(*stream) << " int;";
		}else{
			(*stream) << " " << dw0 << " bytes;";
		}
		break;
	case cmdPushV:
		(*stream) << " PUSHV " << valind << ";";
		break;
	case cmdNop:
		(*stream) << " NOP;";
		break;
	case cmdPop:
		(*stream) << " POP ";
		//(*stream) << typeInfoS[cFlag&0x00000003];
		if(valind)
			(*stream) << " sizeof " << valind;
		break;
	case cmdRTOI:
		(*stream) << " RTOI ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
		break;
	case cmdITOR:
		(*stream) << " ITOR ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
		break;
	case cmdITOL:
		(*stream) << " ITOL";
		break;
	case cmdLTOI:
		(*stream) << " LTOI";
		break;
	case cmdSwap:
		(*stream) << " SWAP ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "<->";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007];
		break;
	case cmdCopy:
		(*stream) << " COPY ";
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double;";
			break;
		case OTYPE_LONG:
			(*stream) << " long;";
			break;
		case OTYPE_INT:
			(*stream) << " int;";
			break;
		}
		break;
	case cmdJmp:
		(*stream) << " JMP " << valind;
		break;
	case cmdJmpZ:
		(*stream) << " JMPZ";
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double ";
			break;
		case OTYPE_LONG:
			(*stream) << " long ";
			break;
		case OTYPE_INT:
			(*stream) << " int ";
			break;
		}
		(*stream) << valind << ';';
		break;
	case cmdJmpNZ:
		(*stream) << " JMPNZ";
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double ";
			break;
		case OTYPE_LONG:
			(*stream) << " long ";
			break;
		case OTYPE_INT:
			(*stream) << " int ";
			break;
		}
		(*stream) << valind << ';';
		break;
	case cmdCTI:
		(*stream) << " CTI addr*";
		(*stream) << valind;
		break;
	case cmdMovRTaP:
		(*stream) << " MOVRTAP ";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";

		(*stream) << valind << "] //+max";

		if(dt == DTYPE_COMPLEX_TYPE)
			(*stream) << " sizeof " << dw0;
		break;
	case cmdMov:
		(*stream) << " MOV ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "->";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";

		(*stream) << valind << "] //";
		
		if(flagAddrRel(cFlag))
			(*stream) << "rel+top";
		if(flagAddrRelTop(cFlag))
			(*stream) << "max+top";
		if(flagShiftStk(cFlag))
			(*stream) << "+shiftstk";
		if(st == STYPE_COMPLEX_TYPE)
			(*stream) << " sizeof " << dw0;
		break;
	case cmdPushImmt:
		(*stream) << " PUSHIMMT ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "<-";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007];

		if(dt == DTYPE_DOUBLE)
			(*stream) << " (" << *((double*)(&DWords[0])) << ')';
		if(dt == DTYPE_LONG)
			(*stream) << " (" << *((long*)(&DWords[0])) << ')';
		if(dt == DTYPE_FLOAT)
			(*stream) << " (" << *((float*)(&DWords[1])) << ')';
		if(dt == DTYPE_INT)
			(*stream) << " (" << *((int*)(&DWords[1])) << ')';
		if(dt == DTYPE_SHORT)
			(*stream) << " (" << *((short*)(&DWords[1])) << ')';
		if(dt == DTYPE_CHAR)
			(*stream) << " (" << *((char*)(&DWords[1])) << ')';
		break;
	case cmdSetRange:
		(*stream) << " SETRANGE" << typeInfoD[(cFlag>>2)&0x00000007] << " " << valind << " " << dw0;
		break;
	case cmdGetAddr:
		(*stream) << " GETADDR " << valind;
	}
	if(cmd >= cmdAdd && cmd <= cmdLogXor)
	{
		(*stream) << ' ';
		switch(cmd)
		{
		case cmdAdd:
			(*stream) << "ADD";
			break;
		case cmdSub:
			(*stream) << "SUB";
			break;
		case cmdMul:
			(*stream) << "MUL";
			break;
		case cmdDiv:
			(*stream) << "DIV";
			break;
		case cmdPow:
			(*stream) << "POW";
			break;
		case cmdMod:
			(*stream) << "MOD";
			break;
		case cmdLess:
			(*stream) << "LES";
			break;
		case cmdGreater:
			(*stream) << "GRT";
			break;
		case cmdLEqual:
			(*stream) << "LEQL";
			break;
		case cmdGEqual:
			(*stream) << "GEQL";
			break;
		case cmdEqual:
			(*stream) << "EQL";
			break;
		case cmdNEqual:
			(*stream) << "NEQL";
			break;
		case cmdShl:
			(*stream) << "SHL";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: SHL used on float");
			break;
		case cmdShr:
			(*stream) << "SHR";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: SHR used on float");
			break;
		case cmdBitAnd:
			(*stream) << "BAND";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: BAND used on float");
			break;
		case cmdBitOr:
			(*stream) << "BOR";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: BOR used on float");
			break;
		case cmdBitXor:
			(*stream) << "BXOR";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: BXOR used on float");
			break;
		case cmdLogAnd:
			(*stream) << "LAND";
			break;
		case cmdLogOr:
			(*stream) << "LOR";
			break;
		case cmdLogXor:
			(*stream) << "LXOR";
			break;
		}
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double;";
			break;
		case OTYPE_LONG:
			(*stream) << " long;";
			break;
		case OTYPE_INT:
			(*stream) << " int;";
			break;
		default:
			(*stream) << "ERROR: OperFlag expected after instruction";
		}
	}
	if(cmd >= cmdNeg && cmd <= cmdLogNot)
	{
		(*stream) << ' ';
		switch(cmd)
		{
		case cmdNeg:
			(*stream) << "NEG";
			break;
		case cmdBitNot:
			(*stream) << "BNOT";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: BNOT used on float");
			break;
		case cmdLogNot:
			(*stream) << "LNOT;";
			break;
		}
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double;";
			break;
		case OTYPE_LONG:
			(*stream) << " long;";
			break;
		case OTYPE_INT:
			(*stream) << " int;";
			break;
		default:
			(*stream) << "ERROR: OperFlag expected after ";
		}
	}
	if(cmd >= cmdIncAt && cmd <= cmdDecAt)
	{
		if(cmd == cmdIncAt)
			(*stream) << " INCAT ";
		if(cmd == cmdDecAt)
			(*stream) << " DECAT ";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
		
		(*stream) << valind << "] //";
		
		if(flagAddrRel(cFlag))
			(*stream) << "rel+top";
		if(flagShiftStk(cFlag))
			(*stream) << "+shiftstk";
		
		if(flagSizeStk(cFlag))
			(*stream) << " size: stack";
		if(flagSizeOn(cFlag))
			(*stream) << " size: instr";
	}
	
	// Add end alignment
	// ƒобавить выравнивание
	size_t endPos = stream->tellp();
	int putSize = (int)(endPos - beginPos);
	int alignLen = 55-putSize;
	if(alignLen > 0)
		for(int i = 0; i < alignLen; i++)
			(*stream) << ' ';
	
}
#endif
