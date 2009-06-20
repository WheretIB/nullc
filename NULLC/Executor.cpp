#include "stdafx.h"
#include "Executor.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <MMSystem.h>

#include "CodeInfo.h"
using namespace CodeInfo;

#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	#define DBG(x) x
#else
	#define DBG(x)
#endif

Executor::Executor(): m_FileStream("log.txt", std::ios::binary)
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

UINT Executor::Run(const char* funcName)
{
	paramTop.clear();
	callStack.clear();

	paramTop.push_back(0);

	genParams.clear();
	genStackTypes.clear();

	double tempVal = 0.0;
	genParams.push_back((char*)(&tempVal), 8);
	tempVal = 3.1415926535897932384626433832795;
	genParams.push_back((char*)(&tempVal), 8);
	tempVal = 2.7182818284590452353602874713527;
	genParams.push_back((char*)(&tempVal), 8);
	
	UINT pos = 0, pos2 = 0;
	CmdID	cmd;
	double	val = 0.0;
	UINT	uintVal, uintVal2;
	char	name[512];
	int		valind;
	UINT	cmdCount = 0;
	bool	done = false;

	CmdFlag		cFlag;
	OperFlag	oFlag;
	asmStackType st;
	char*	typeInfoS[] = { "int", "long", "float", "double" };
	char*	typeInfoD[] = { "char", "short", "int", "long", "float", "double" };
	UINT typeSizeS[] = { 4, 8, 4, 8 };
	UINT typeSizeD[] = { 1, 2, 4, 8, 4, 8 };
	FunctionInfo *funcInfoPtr = NULL;

	UINT startTime = timeGetTime();

	UINT funcPos = 0;
	if(funcName)
	{
		for(unsigned int i = 0; i < funcInfo.size(); i++)
		{
			if(strcmp(funcInfo[i]->name.c_str(), funcName) == 0)
			{
				funcPos = CodeInfo::funcInfo[i]->address;
				break;
			}
		}
	}

	bool intDivByZero = false;

	// General stack
	if(!genStackBase)
	{
		genStackBase = new UINT[64];		// Will grow
		genStackTop = genStackBase + 64;
	}
	genStackPtr = genStackTop - 1;

#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
	unsigned int insCallCount[255];
	memset(insCallCount, 0, 255*4);
#endif
	while(cmdList->GetSHORT(pos, cmd) && !done)
	{
		if(genStackSize >= (genStackTop-genStackBase))
		{
			UINT *oldStack = genStackBase;
			UINT oldSize = (genStackTop-genStackBase);
			genStackBase = new UINT[oldSize+64];
			genStackTop = genStackBase + oldSize + 64;
			memcpy(genStackBase+64, oldStack, oldSize);

			genStackPtr = genStackTop - oldSize;
		}
		if(genStackSize < 0)
		{
			done = true;
			assert(0 == "stack underflow");
		}
		#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
			insCallCount[cmd]++;
		#endif

		cmdCount++;
		if(m_RunCallback && cmdCount % 5000000 == 0)
			if(!m_RunCallback(cmdCount))
			{
				done = true;
				throw std::string("User have canceled the execution");
			}
		pos2 = pos;
		pos += 2;

		if(funcName && pos >= funcPos)
		{
			funcName = NULL;
			pos = funcPos;
			cmdList->GetSHORT(pos, cmd);
			pos2 = pos;
			pos += 2;

			// cmdPushVTop
			UINT valtop = genParams.size();
			paramTop.push_back(valtop);

			// cmdPushV 4
			genParams.resize(genParams.size()+4);
		}

		switch(cmd)
		{
		case cmdCallStd:
			{
				cmdList->GetData(pos, funcInfoPtr);
				pos += sizeof(FunctionInfo*);
				if(!funcInfoPtr)
					throw std::string("ERROR: std function info is invalid");

				if(funcInfoPtr->funcPtr == NULL)
				{
					if(funcInfoPtr->name != "clock")
					{
						val = *(double*)(genStackPtr);
						DBG(genStackTypes.pop_back());
					}
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
					else if(funcInfoPtr->name == "clock")
						uintVal = GetTickCount();
					else
						throw std::string("ERROR: there is no such function: ") + funcInfoPtr->name;

					if(fabs(val) < 1e-10)
						val = 0.0;
					if(funcInfoPtr->name != "clock")
					{
						*(double*)(genStackPtr) = val;
						DBG(genStackTypes.push_back(STYPE_DOUBLE));
					}else{
						genStackPtr--;
						*genStackPtr = uintVal;
						DBG(genStackTypes.push_back(STYPE_INT));
					}
				}else{
					if(funcInfoPtr->retType->size > 4)
						throw std::string("ERROR: user functions with return type size larger than 4 bytes are not supported");
					UINT bytesToPop = 0;
					for(UINT i = 0; i < funcInfoPtr->params.size(); i++)
					{
						UINT paramSize = funcInfoPtr->params[i].varType->size > 4 ? funcInfoPtr->params[i].varType->size : 4;
						bytesToPop += paramSize;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
						while(paramSize > 0)
						{
							paramSize -= genStackTypes.back() & 0x80000000 ? genStackTypes.back() & ~0x80000000 : typeSizeS[genStackTypes.back()];;
							genStackTypes.pop_back();
						}
#endif
					}
					for(UINT i = 0; i < bytesToPop/4; i++)
					{
						UINT data = *(genStackPtr+bytesToPop/4-i-1);
						__asm push data;
					}
					genStackPtr += bytesToPop/4;

					void* fPtr = funcInfoPtr->funcPtr;
					__asm{
						mov ecx, fPtr;
						call ecx;
						add esp, bytesToPop;
					}
					UINT fRes;
					__asm mov fRes, eax;
					if(funcInfoPtr->retType->size == 4)
					{
						genStackPtr--;
						*genStackPtr = fRes;
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
						if(funcInfoPtr->retType->type == TypeInfo::TYPE_COMPLEX)
							genStackTypes.push_back((asmStackType)(0x80000000 | funcInfoPtr->retType->size));
						else
							genStackTypes.push_back(podTypeToStackType[funcInfoPtr->retType->type]);
#endif
					}
				}
				DBG(m_FileStream << pos2 << dec << " CALLS " << funcInfoPtr->name << ";");
			}
			break;
		case cmdSwap:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
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
				throw std::string("cmdSwap, unimplemented type combo");
			}
			DBG(st = genStackTypes[genStackTypes.size()-2]);
			DBG(genStackTypes[genStackTypes.size()-2] = genStackTypes[genStackTypes.size()-1]);
			DBG(genStackTypes[genStackTypes.size()-1] = st);

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, cFlag, 0));
			break;
		case cmdCopy:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
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
		case cmdPushVTop:
			size_t valtop;
			valtop = genParams.size();
			paramTop.push_back(valtop);

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, (UINT)valtop, 0, 0));
			break;
		case cmdPopVTop:
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, paramTop.back(), 0, 0));
			while(genParams.size() > paramTop.back())
				genParams.pop_back();
			paramTop.pop_back();
			break;
		case cmdCall:
			{
				USHORT retFlag;
				cmdList->GetUINT(pos, uintVal);
				pos += 4;
				cmdList->GetUSHORT(pos, retFlag);
				pos += 2;
				if(uintVal == -1)
				{
					uintVal = *genStackPtr;
					genStackPtr++;
				}
				callStack.push_back(CallStackInfo(pos, (UINT)genStackSize, uintVal));
				pos = uintVal;
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0, retFlag));
			}
			break;
		case cmdReturn:
			{
				USHORT	retFlag, popCnt;
				cmdList->GetUSHORT(pos, retFlag);
				pos += 2;
				cmdList->GetUSHORT(pos, popCnt);
				pos += 2;
				if(retFlag & bitRetError)
					throw std::string("ERROR: function didn't return a value");
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, popCnt, 0, 0, retFlag));
				for(int pops = 0; pops < (popCnt > 0 ? popCnt : 1); pops++)
				{
					while(genParams.size() > paramTop.back())
						genParams.pop_back();
					paramTop.pop_back();
				}
				if(callStack.size() == 0)
				{
					retType = (OperFlag)(retFlag & 0x0FFF);
					done = true;
					break;
				}
				pos = callStack.back().cmd;
				callStack.pop_back();
			}
			break;
		case cmdPushV:
			int valind;
			cmdList->GetINT(pos, valind);
			pos += sizeof(UINT);
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
		case cmdCTI:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, uintVal);
			pos += sizeof(UINT);
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
			}
			*genStackPtr = uintVal*uintVal2;
			DBG(genStackTypes.pop_back());
			DBG(genStackTypes.push_back(STYPE_INT));
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0));
			break;
		case cmdSetRange:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			cmdList->GetUINT(pos, uintVal);
			pos += 4;
			uintVal += paramTop.back();
			cmdList->GetUINT(pos, uintVal2);
			pos += 4;
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
					*((int*)(&genParams[uintVal])) = *genStackPtr;
					uintVal += 4;
					break;
				case DTYPE_SHORT:
					*((short*)(&genParams[uintVal])) = *genStackPtr;
					uintVal += 2;
					break;
				case DTYPE_CHAR:
					*((char*)(&genParams[uintVal])) = *genStackPtr;
					uintVal += 1;
					break;
				}
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, cFlag, 0, uintVal2));
			break;
		case cmdGetAddr:
			cmdList->GetUINT(pos, uintVal);
			pos += 4;
			genStackPtr--;
			*genStackPtr = uintVal + paramTop.back();
			DBG(genStackTypes.push_back(STYPE_INT));
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0));
			break;
		case cmdFuncAddr:
			cmdList->GetData(pos, funcInfoPtr);
			pos += sizeof(FunctionInfo*);
			if(!funcInfoPtr)
				throw std::string("ERROR: std function info is invalid");

			genStackPtr--;
			if(funcInfoPtr->funcPtr == NULL)
				*genStackPtr = funcInfoPtr->address;
			else
				*genStackPtr = reinterpret_cast<unsigned int>(funcInfoPtr->funcPtr);
			DBG(genStackTypes.push_back(STYPE_INT));
			break;
		}

		//New commands
		if(cmd == cmdPushImmt)
		{
			int valind = -1;
			UINT	highDW = 0, lowDW = 0;
			USHORT sdata;
			UCHAR cdata;
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			st = flagStackType(cFlag);
			asmDataType dt = flagDataType(cFlag);

			if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
			{
				cmdList->GetUINT(pos, highDW); pos += 4;
				cmdList->GetUINT(pos, lowDW); pos += 4;
			}else if(dt == DTYPE_FLOAT || dt == DTYPE_INT){
				cmdList->GetUINT(pos, lowDW); pos += 4;
			}else if(dt == DTYPE_SHORT){
				cmdList->GetUSHORT(pos, sdata); pos += 2; lowDW = (sdata>0?sdata:sdata|0xFFFF0000);
			}else if(dt == DTYPE_CHAR){
				cmdList->GetUCHAR(pos, cdata); pos += 1; lowDW = cdata;
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
			DBG(if(st == STYPE_COMPLEX_TYPE))
			DBG(genStackTypes.back() = (asmStackType)(sizeOfVar|0x80000000));

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0, highDW, lowDW));
		}else if(cmd == cmdPush || cmd == cmdMov)
		{
			int valind = -1, shift, size;
			UINT	highDW = 0, lowDW = 0;
			USHORT sdata;
			UCHAR cdata;
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			st = flagStackType(cFlag);
			asmDataType dt = flagDataType(cFlag);

			//if(flagAddrRel(cFlag) || flagAddrAbs(cFlag) || flagAddrRelTop(cFlag))
			//{
				cmdList->GetINT(pos, valind);
				pos += 4;
			//}
			if(flagShiftStk(cFlag))
			{
				shift = *genStackPtr;
				genStackPtr++;
				DBG(genStackTypes.pop_back());
			}
			if(flagSizeOn(cFlag))
			{
				cmdList->GetINT(pos, size);
				pos += 4;
			}
			if(flagSizeStk(cFlag))
			{
				size = *genStackPtr;
				genStackPtr++;
				DBG(genStackTypes.pop_back());
			}
			if(flagShiftStk(cFlag))
				if((int)(shift) < 0)
					throw std::string("ERROR: array index out of bounds (negative)");
			if(flagSizeOn(cFlag) || flagSizeStk(cFlag))
				if((int)(shift) >= size)
					throw std::string("ERROR: array index out of bounds (overflow)");

			UINT sizeOfVar = 0;
			if(dt == DTYPE_COMPLEX_TYPE)
			{
				cmdList->GetUINT(pos, sizeOfVar);
				pos += 4;
				typeSizeD[dt>>3] = sizeOfVar;
			}

			if(flagAddrRel(cFlag))
				valind += paramTop.back();
			if(flagShiftStk(cFlag))
				valind += shift;
			if(flagAddrRelTop(cFlag))
				valind += genParams.size();

			if(cmd == cmdMov)
			{
				if(flagAddrRelTop(cFlag) && valind+typeSizeD[dt>>3] > genParams.size())
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
					assert(sizeOfVar == 0);
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
					sdata = *genStackPtr;
					*((USHORT*)(&genParams[valind])) = sdata;
				}else if(dt == DTYPE_CHAR)
				{
					cdata = *genStackPtr;
					genParams[valind] = cdata;
				}

				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0, typeSizeD[dt>>3]));
			}else{
				/*if(flagNoAddr(cFlag)){
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						cmdList->GetUINT(pos, highDW); pos += 4;
						cmdList->GetUINT(pos, lowDW); pos += 4;
					}
					if(dt == DTYPE_FLOAT || dt == DTYPE_INT){ cmdList->GetUINT(pos, lowDW); pos += 4; }
					if(dt == DTYPE_SHORT){ cmdList->GetUSHORT(pos, sdata); pos += 2; lowDW = (sdata>0?sdata:sdata|0xFFFF0000); }
					if(dt == DTYPE_CHAR){ cmdList->GetUCHAR(pos, cdata); pos += 1; lowDW = cdata; }
				}else{*/
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
				//}
				
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
		}else if(cmd == cmdRTOI){
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
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
		}else if(cmd == cmdITOR){
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
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
		}else if(cmd == cmdJmp){
			cmdList->GetINT(pos, valind);
			pos = valind;
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
		}else if(cmd == cmdJmpZ){
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetINT(pos, valind);
			pos += 4;
			if(oFlag == OTYPE_DOUBLE){
				if(*(double*)(genStackPtr) == 0.0)
					pos = valind;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_LONG){
				if(*(long long*)(genStackPtr) == 0L)
					pos = valind;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_INT){
				if(*genStackPtr == 0)
					pos = valind;
				genStackPtr++;
			}
			DBG(genStackTypes.pop_back());
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
		}else if(cmd == cmdJmpNZ){
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetINT(pos, valind);
			pos += 4;
			if(oFlag == OTYPE_DOUBLE){
				if(*(double*)(genStackPtr) != 0.0)
					pos = valind;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_LONG){
				if(*(long long*)(genStackPtr) == 0L)
					pos = valind;
				genStackPtr += 2;
			}else if(oFlag == OTYPE_INT){
				if(*genStackPtr != 0)
					pos = valind;
				genStackPtr++;
			}
			DBG(genStackTypes.pop_back());
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
		}else if(cmd == cmdPop){
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			asmStackType st = flagStackType(cFlag);
			UINT sizeOfVar = 0;
			if(st == STYPE_DOUBLE || st == STYPE_LONG)
			{
				genStackPtr += 2;
				DBG(genStackTypes.pop_back());
			}else if(st == STYPE_COMPLEX_TYPE){
				UINT varSize;
				cmdList->GetUINT(pos, varSize);
				pos += 4;
				sizeOfVar = varSize;
				genStackPtr += varSize/4;
				DBG(UINT count = genStackTypes.back() & 0x80000000 ? genStackTypes.back() & ~0x80000000 : typeSizeS[genStackTypes.back()]);
				DBG(for(int n = 0; n < sizeOfVar/count; n++))
				DBG(	genStackTypes.pop_back());
			}else{
				genStackPtr++;
				DBG(genStackTypes.pop_back());
			}

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, sizeOfVar, cFlag, 0));
		}else if(cmd >= cmdAdd && cmd <= cmdLogXor){
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
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
					intDivByZero = true;
					done = true;
				}
				break;
			case cmdDiv+(OTYPE_INT<<16):
				if(*(int*)(genStackPtr))
				{
					*(int*)(genStackPtr+1) /= *(int*)(genStackPtr);
				}else{
					intDivByZero = true;
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
					intDivByZero = true;
					done = true;
				}
				break;
			case cmdMod+(OTYPE_INT<<16):
				if(*(int*)(genStackPtr))
				{
					*(int*)(genStackPtr+1) %= *(int*)(genStackPtr);
				}else{
					intDivByZero = true;
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
				throw string("Operation is not implemented");
			}
			if(oFlag == OTYPE_INT)
			{
				genStackPtr++;
			}else{
				genStackPtr += 2;
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, oFlag));
			DBG(genStackTypes.pop_back());
			
		}else if(cmd >= cmdNeg && cmd <= cmdLogNot){
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			switch(cmd + (oFlag << 16))
			{
			case cmdNeg+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr) = -*(double*)(genStackPtr);
				break;
			case cmdNeg+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr) = -*(long long*)(genStackPtr);
				break;
			case cmdNeg+(OTYPE_INT<<16):
				*(int*)(genStackPtr) = -*(int*)(genStackPtr);
				break;

			case cmdLogNot+(OTYPE_DOUBLE<<16):
				*(double*)(genStackPtr) = fabs(*(double*)(genStackPtr)) < 1e-10;
				break;
			case cmdLogNot+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr) = !*(long long*)(genStackPtr);
				break;
			case cmdLogNot+(OTYPE_INT<<16):
				*(int*)(genStackPtr) = !*(int*)(genStackPtr);
				break;

			case cmdBitNot+(OTYPE_LONG<<16):
				*(long long*)(genStackPtr) = ~*(long long*)(genStackPtr);
				break;
			case cmdBitNot+(OTYPE_INT<<16):
				*(int*)(genStackPtr) = ~*(int*)(genStackPtr);
				break;
			default:
				throw string("Operation is not implemented");
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, oFlag));
		}else if(cmd == cmdIncAt || cmd == cmdDecAt)
		{
			int valind, shift, size;
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			asmDataType dt = flagDataType(cFlag);	//Data type

			if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
			{
				cmdList->GetINT(pos, valind);
				pos += 4;
			}
			if(flagShiftStk(cFlag))
			{
				shift = *genStackPtr;
				genStackPtr++;
			}
			if(flagSizeOn(cFlag))
			{
				cmdList->GetINT(pos, size);
				pos += 4;
			}
			if(flagSizeStk(cFlag))
			{
				size = *genStackPtr;
				genStackPtr++;
			}
			if(flagShiftStk(cFlag))
				if((int)(shift) < 0)
					throw std::string("ERROR: array index out of bounds (negative)");
			if(flagSizeOn(cFlag) || flagSizeStk(cFlag))
				if((int)(shift) >= size)
					throw std::string("ERROR: array index out of bounds (overflow)");

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

		UINT typeSizeS[] = { 1, 2, 0, 2 };
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
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
	UINT runTime = timeGetTime() - startTime;

	if(intDivByZero)
		throw std::string("Integer division by zero");

	/*m_ostr << "There are " << (UINT)genStackTypes.size() << " values in the stack\r\n";
	if((UINT)genStackTypes.size() == 0)
		m_ostr << "It's bad.\r\n���� return �����!";//Did you forget 'return'?";
	else if((UINT)genStackTypes.size() == 1)
		m_ostr << "Good.";
	else if((UINT)genStackTypes.size() > 1)
		m_ostr << "It's bug.\r\nReport to NULL_PTR";
	m_ostr << "\r\n\r\n";
	m_ostr << "Variables active: " << (UINT)genParams.size() << "\r\n";*/
	return runTime;
}

string Executor::GetResult()
{
	if(genStackSize == 0)
		return "No result value";
	if(genStackSize-1 > 2)
		throw std::string("There are more than one value on the stack");
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

	return tempStream.str();;
}
string Executor::GetLog()
{
	return m_ostr.str();
}

char* Executor::GetVariableData()
{
	return &genParams[0];
}

void Executor::SetCallback(bool (*Func)(UINT))
{
	m_RunCallback = Func;
}