#include "stdafx.h"
#include "Executor.h"

#include "CodeInfo.h"

const unsigned int CALL_BY_POINTER = (unsigned int)-1;

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	#define DBG(x) x
#else
	#define DBG(x)
#endif

Executor::Executor()DBG(: m_FileStream("log.txt", std::ios::binary))
{
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
	
	//UINT pos = 0, pos2 = 0;
	CmdID	cmd;
	double	val = 0.0;
	UINT	uintVal, uintVal2;

	int		valind;
	//UINT	cmdCount = 0;
	bool	done = false;

	CmdFlag		cFlag;
	OperFlag	oFlag;
	asmStackType st;

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	UINT typeSizeS[] = { 4, 8, 4, 8 };
	//UINT typeSizeD[] = { 1, 2, 4, 8, 4, 8 };
#endif

	FunctionInfo *funcInfoPtr = NULL;

	execError[0] = 0;

	UINT funcPos = 0;
	if(funcName)
	{
		for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		{
			if(strcmp(CodeInfo::funcInfo[i]->name.c_str(), funcName) == 0)
			{
				funcPos = CodeInfo::funcInfo[i]->address;
				*(CmdID*)(&CodeInfo::cmdList->bytecode[funcPos-6]) = cmdFEnter;
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
		genStackBase = new UINT[128];		// Will grow
		genStackTop = genStackBase + 128;
	}
	genStackPtr = genStackTop - 1;

#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
	unsigned int insCallCount[255];
	memset(insCallCount, 0, 255*4);
#endif
	char *cmdStreamBase = CodeInfo::cmdList->bytecode;
	char *cmdStream = CodeInfo::cmdList->bytecode;
	char *cmdStreamEnd = CodeInfo::cmdList->bytecode + CodeInfo::cmdList->max;
#define cmdStreamPos (cmdStream-cmdStreamBase)

	while(cmdStream+2 < cmdStreamEnd && !done)
	{
		cmd = *(CmdID*)(cmdStream);
		DBG(pos2 = cmdStream - cmdStreamBase);
		cmdStream += 2;

		if(genStackPtr <= genStackBase)
		{
			UINT *oldStack = genStackBase;
			UINT oldSize = (unsigned int)(genStackTop-genStackBase);
			genStackBase = new UINT[oldSize+128];
			genStackTop = genStackBase + oldSize + 128;
			memcpy(genStackBase+128, oldStack, oldSize);

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

		UINT	highDW = 0, lowDW = 0;

		switch(cmd)
		{
		case cmdFEnter:
			paramTop.push_back(genParams.size());
			genParams.resize(genParams.size()+4);
			break;
		case cmdMovRTaP:
			{
				int valind;
				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;

				asmDataType dt = flagDataType(cFlag);

				valind = *(int*)cmdStream;
				cmdStream += 4;

				UINT sizeOfVar = 0;
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					sizeOfVar = *(unsigned int*)cmdStream;
					cmdStream += 4;
				}
				UINT sizeOfVarConst = sizeOfVar;

				valind += genParams.size();

				if(valind + sizeOfVarConst > genParams.size())
					genParams.reserve(genParams.size()+128);
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					UINT currShift = sizeOfVar;
					while(sizeOfVar >= 4)
					{
						currShift -= 4;
						*((UINT*)(&genParams[valind+currShift])) = *(genStackPtr+sizeOfVar/4-1);
						sizeOfVar -= 4;
					}
					genStackPtr += sizeOfVarConst / 4;
					assert(sizeOfVar == 0);
				}else if(dt == DTYPE_FLOAT){
					*((float*)(&genParams[valind])) = float(*(double*)(genStackPtr));
					genStackPtr += 2;
				}else if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG){
					*((UINT*)(&genParams[valind])) = *genStackPtr;
					*((UINT*)(&genParams[valind+4])) = *(genStackPtr+1);
					genStackPtr += 2;
				}else if(dt == DTYPE_INT){
					*((UINT*)(&genParams[valind])) = *genStackPtr;
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
				USHORT sdata;
				UCHAR cdata;

				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;

				st = flagStackType(cFlag);
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
					*(double*)(genStackPtr) = (double)(*((float*)(&lowDW)));
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
			{
				int valind = -1, shift = 0;
				USHORT sdata;
				UCHAR cdata;
				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;
				st = flagStackType(cFlag);
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

				UINT sizeOfVar = 0;
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					sizeOfVar = *(unsigned int*)cmdStream;
					cmdStream += 4;
				}

				if(flagAddrRel(cFlag))
					valind += paramTop.back();
				if(flagShiftStk(cFlag))
					valind += shift;

				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					highDW = *((UINT*)(&genParams[valind]));
					lowDW = *((UINT*)(&genParams[valind+4]));
				}
				if(dt == DTYPE_FLOAT || dt == DTYPE_INT){ lowDW = *((UINT*)(&genParams[valind])); }
				if(dt == DTYPE_SHORT)
				{
					sdata = *((USHORT*)(&genParams[valind]));
					lowDW = (short)(sdata) > 0 ? sdata : sdata | 0xFFFF0000;
				}
				if(dt == DTYPE_CHAR){ cdata = genParams[valind]; lowDW = cdata; }
				
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					UINT currShift = sizeOfVar;
					while(sizeOfVar >= 4)
					{
						currShift -= 4;
						genStackPtr--;
						*genStackPtr = *((UINT*)(&genParams[valind+currShift]));
						sizeOfVar -= 4;
					}
					lowDW = sizeOfVar;
				}else if(dt == DTYPE_FLOAT && st == STYPE_DOUBLE)	//expand float to double
				{
					genStackPtr -= 2;
					*(double*)(genStackPtr) = (double)(*((float*)(&lowDW)));
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
				DBG(if(st == STYPE_COMPLEX_TYPE))
				DBG(genStackTypes.back() = (asmStackType)(sizeOfVar|0x80000000));

				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0, highDW, lowDW));
			}
			break;
		case cmdPop:
			{
				UINT varSize = *(unsigned int*)cmdStream;
				cmdStream += 4;

				genStackPtr += varSize >> 2;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
				UINT sizeOfVar = varSize;
				UINT count = genStackTypes.back() & 0x80000000 ? genStackTypes.back() & ~0x80000000 : typeSizeS[genStackTypes.back()];
				for(unsigned int n = 0; n < sizeOfVar/count; n++)
					genStackTypes.pop_back();
#endif
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, sizeOfVar, 0, 0));
			}
			break;
		case cmdMov:
			{
				int valind = -1, shift = 0;
				USHORT sdata;
				UCHAR cdata;
				cFlag = *(CmdFlag*)cmdStream;
				cmdStream += 2;
				st = flagStackType(cFlag);
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

				UINT sizeOfVar = 0;
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
					UINT currShift = sizeOfVar;
					while(currShift >= 4)
					{
						currShift -= 4;
						*((UINT*)(&genParams[valind+currShift])) = *(genStackPtr+(currShift>>2));
					}
					assert(currShift == 0);
				}else if(dt == DTYPE_FLOAT && st == STYPE_DOUBLE)
				{
					*((float*)(&genParams[valind])) = float(*(double*)(genStackPtr));
				}else if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					*((UINT*)(&genParams[valind])) = *genStackPtr;
					*((UINT*)(&genParams[valind+4])) = *(genStackPtr+1);
				}else if(dt == DTYPE_FLOAT || dt == DTYPE_INT)
				{
					*((UINT*)(&genParams[valind])) = *genStackPtr;
				}else if(dt == DTYPE_SHORT)
				{
					sdata = (unsigned short)(*genStackPtr);
					*((USHORT*)(&genParams[valind])) = sdata;
				}else if(dt == DTYPE_CHAR)
				{
					cdata = (unsigned char)(*genStackPtr);
					genParams[valind] = cdata;
				}

				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0, sizeOfVarConst));
			}
			break;
		case cmdCTI:
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
				funcInfoPtr = *(FunctionInfo**)(cmdStream);
				cmdStream += sizeof(FunctionInfo*);
				if(!funcInfoPtr)
				{
					done = true;
					strcpy(execError, "ERROR: std function info is invalid");
					break;
				}

				if(funcInfoPtr->funcPtr == NULL)
				{
					val = *(double*)(genStackPtr);
					DBG(genStackTypes.pop_back());

					if(funcInfoPtr->name == "cos")
						val = cos(val);
					else if(funcInfoPtr->name == "sin")
						val = sin(val);
					else if(funcInfoPtr->name == "tan")
						val = tan(val);
					else if(funcInfoPtr->name == "ctg")
						val = 1.0/tan(val);
					else if(funcInfoPtr->name == "ceil")
						val = ceil(val);
					else if(funcInfoPtr->name == "floor")
						val = floor(val);
					else if(funcInfoPtr->name == "sqrt")
						val = sqrt(val);
					else{
						done = true;
						printf(execError, "ERROR: there is no such function: %s", funcInfoPtr->name.c_str());
						break;
					}

					if(fabs(val) < 1e-10)
						val = 0.0;
					*(double*)(genStackPtr) = val;
					DBG(genStackTypes.push_back(STYPE_DOUBLE));
				}else{
				    done = !RunExternalFunction(funcInfoPtr);
				}
				DBG(m_FileStream << pos2 << dec << " CALLS " << funcInfoPtr->name << ";");
			}
			break;
		case cmdSwap:
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
			valind = *(int*)cmdStream;
			cmdStream = cmdStreamBase + valind;
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
			break;
		case cmdJmpZ:
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			valind = *(int*)cmdStream;
			cmdStream += 4;
			if(oFlag == OTYPE_DOUBLE){
				if(*(double*)(genStackPtr) == 0.0)
					cmdStream = cmdStreamBase + valind;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_LONG){
				if(*(long long*)(genStackPtr) == 0L)
					cmdStream = cmdStreamBase + valind;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_INT){
				if(*genStackPtr == 0)
					cmdStream = cmdStreamBase + valind;
				genStackPtr++;
			}
			DBG(genStackTypes.pop_back());
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
			break;
		case cmdJmpNZ:
			oFlag = *(OperFlag*)cmdStream;
			cmdStream++;
			valind = *(int*)cmdStream;
			cmdStream += 4;
			if(oFlag == OTYPE_DOUBLE){
				if(*(double*)(genStackPtr) != 0.0)
					cmdStream = cmdStreamBase + valind;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_LONG){
				if(*(long long*)(genStackPtr) == 0L)
					cmdStream = cmdStreamBase + valind;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_INT){
				if(*genStackPtr != 0)
					cmdStream = cmdStreamBase + valind;
				genStackPtr++;
			}
			DBG(genStackTypes.pop_back());
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
			break;
		case cmdPushVTop:
			size_t valtop;
			valtop = genParams.size();
			paramTop.push_back(valtop);

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, (UINT)valtop, 0, 0));
			break;
		case cmdPopVTop:
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, paramTop.back(), 0, 0));
			genParams.shrink(paramTop.back());
			paramTop.pop_back();
			break;
		case cmdCall:
			{
				USHORT retFlag;
				uintVal = *(unsigned int*)cmdStream;
				cmdStream += 4;
				retFlag = *(unsigned short*)cmdStream;
				cmdStream += 2;

				if(uintVal == CALL_BY_POINTER)
				{
					uintVal = *genStackPtr;
					genStackPtr++;
				}
				fcallStack.push_back(cmdStream);// callStack.push_back(CallStackInfo(cmdStream, (UINT)genStackSize, uintVal));
				cmdStream = cmdStreamBase + uintVal;
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0, retFlag));
			}
			break;
		case cmdReturn:
			{
				USHORT	retFlag, popCnt;
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
			valind = *(int*)cmdStream;
			cmdStream += 4;
			genParams.resize(genParams.size()+valind);
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
			break;
		case cmdNop:
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, 0));
			break;

		case cmdITOL:
			if((int)(*genStackPtr) < 0)
			{
				valind = *genStackPtr;
				*genStackPtr = 0xFFFFFFFF;
				genStackPtr--;
				*genStackPtr = valind;
			}else{
				valind = *genStackPtr;
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
			cFlag = *(CmdFlag*)cmdStream;
			cmdStream += 2;
			uintVal = *(unsigned int*)cmdStream;
			cmdStream += 4;
			uintVal2 = *(unsigned int*)cmdStream;
			cmdStream += 4;
			
			uintVal += paramTop.back();
			for(UINT varNum = 0; varNum < uintVal2; varNum++)
			{
				switch(cFlag)
				{
				case DTYPE_DOUBLE:
					*((double*)(&genParams[uintVal])) = *(double*)(genStackPtr);
					uintVal += 8;
					break;
				case DTYPE_FLOAT:
					*((float*)(&genParams[uintVal])) = float(*(double*)(genStackPtr));
					uintVal += 4;
					break;
				case DTYPE_LONG:
					*((long long*)(&genParams[uintVal])) = *(long long*)(genStackPtr);
					uintVal += 8;
					break;
				case DTYPE_INT:
					*((int*)(&genParams[uintVal])) = int(*genStackPtr);
					uintVal += 4;
					break;
				case DTYPE_SHORT:
					*((short*)(&genParams[uintVal])) = short(*genStackPtr);
					uintVal += 2;
					break;
				case DTYPE_CHAR:
					*((char*)(&genParams[uintVal])) = char(*genStackPtr);
					uintVal += 1;
					break;
				}
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, cFlag, 0, uintVal2));
			break;
		case cmdGetAddr:
			uintVal = *(unsigned int*)cmdStream;
			cmdStream += 4;

			genStackPtr--;
			*genStackPtr = uintVal + paramTop.back();
			DBG(genStackTypes.push_back(STYPE_INT));
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0));
			break;
		case cmdFuncAddr:
			funcInfoPtr = *(FunctionInfo**)cmdStream;
			cmdStream += sizeof(FunctionInfo*);
			if(!funcInfoPtr)
			{
				done = true;
				strcpy(execError, "ERROR: std function info is invalid");
				break;
			}

			assert(sizeof(funcInfoPtr->funcPtr) == 4);

			genStackPtr--;
			if(funcInfoPtr->funcPtr == NULL)
				*genStackPtr = funcInfoPtr->address;
			else
				*genStackPtr = (unsigned int)((unsigned long long)(funcInfoPtr->funcPtr));
			DBG(genStackTypes.push_back(STYPE_INT));
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
				*(long long*)(genStackPtr+2) = (long long)pow((double)*(long long*)(genStackPtr+2), (double)*(long long*)(genStackPtr));
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
		UINT typeSizeS[] = { 1, 2, 0, 2 };
		m_FileStream << " stack size " << genStackSize << "; stack vals " << genStackTypes.size() << "; param size " << genParams.size() << ";  // ";
		for(UINT i = 0, k = 0; i < genStackTypes.size(); i++)
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

	if(funcName && funcPos)
		*(CmdID*)(&CodeInfo::cmdList->bytecode[funcPos-6]) = cmdJmp;
}

#ifdef _MSC_VER
// X86 implementation
bool Executor::RunExternalFunction(const FunctionInfo* funcInfo)
{
    if (funcInfo->retType->size > 4)
    {
        strcpy(execError, "ERROR: user functions with return type size larger than 4 bytes are not supported");
        return false;
    }
    UINT bytesToPop = funcInfo->externalInfo.bytesToPop;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
    UINT paramSize = bytesToPop;
    while(paramSize > 0)
    {
        paramSize -= genStackTypes.back() & 0x80000000 ? genStackTypes.back() & ~0x80000000 : typeSizeS[genStackTypes.back()];;
        genStackTypes.pop_back();
    }
#endif
    UINT *stackStart = (genStackPtr+bytesToPop/4-1);
    for(UINT i = 0; i < bytesToPop/4; i++)
    {
        __asm mov eax, dword ptr[stackStart]
        __asm push dword ptr[eax];
        stackStart--;
    }
    genStackPtr += bytesToPop/4;

    void* fPtr = funcInfo->funcPtr;
    UINT fRes;
    __asm{
        mov ecx, fPtr;
        call ecx;
        add esp, bytesToPop;
        mov fRes, eax;
    }
    if(funcInfo->retType->size != 0)
    {
        genStackPtr--;
        *genStackPtr = fRes;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
        if(funcInfo->retType->type == TypeInfo::TYPE_COMPLEX)
            genStackTypes.push_back((asmStackType)(0x80000000 | funcInfo->retType->size));
        else
            genStackTypes.push_back(podTypeToStackType[funcInfo->retType->type]);
#endif
    }
}
#elif defined(__CELLOS_LV2__)
// PS3 implementation
bool Executor::RunExternalFunction(const FunctionInfo* funcInfo)
{
    strcpy(execError, "ERROR: user functions are not supported");
    return false;
}
#endif

string Executor::GetResult() throw()
{
	if(genStackSize == 0)
		return "No result value";
	if(genStackSize-1 > 2)
		return "There are more than one value on the stack";
	ostringstream tempStream;
	switch(retType)
	{
	case OTYPE_DOUBLE:
		tempStream << *(double*)(genStackPtr);
		break;
	case OTYPE_LONG:
		tempStream << *(long long*)(genStackPtr) << 'L';
		break;
	case OTYPE_INT:
		tempStream << *(int*)(genStackPtr);
		break;
	}

	return tempStream.str();
}

const char*	Executor::GetExecError()
{
	return execError;
}

char* Executor::GetVariableData()
{
	return &genParams[0];
}

void Executor::SetCallback(bool (*Func)(UINT))
{
	m_RunCallback = Func;
}


// распечатать инструкцию в читабельном виде в поток
void PrintInstructionText(ostream* stream, CmdID cmd, UINT pos2, UINT valind, const CmdFlag cFlag, const OperFlag oFlag, UINT dw0, UINT dw1)
{
	asmStackType st = flagStackType(cFlag);
	asmDataType dt = flagDataType(cFlag);
	char*	typeInfoS[] = { "int", "long", "complex", "double" };
	char*	typeInfoD[] = { "char", "short", "int", "long", "float", "double", "complex" };

	UINT	DWords[] = { dw0, dw1 };

	size_t beginPos = stream->tellp();
	(*stream) << pos2;
	char temp[32];
	sprintf(temp, "%d", pos2);
	UINT addSp = 5 - (UINT)strlen(temp);
	for(UINT i = 0; i < addSp; i++)
		(*stream) << ' ';
	switch(cmd)
	{
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
	case cmdPush:
		(*stream) << " PUSH ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "<-";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007];

		if(flagNoAddr(cFlag))
		{
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
		}else{
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
		}
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
	if(cmd >= cmdIncAt && cmd < cmdDecAt)
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
