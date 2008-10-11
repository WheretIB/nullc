#include "stdafx.h"
#include "Executor.h"
#include <MMSystem.h>

#ifdef _DEBUG
#define DBG(x) x
//#define DBG(x)
#else
#define DBG(x)
#endif

Executor::Executor(CommandList* cmds, std::vector<VariableInfo>* varinfo): m_FileStream("log.txt", std::ios::binary)
{
	m_cmds	= cmds;
	m_VarInfo = varinfo;
	m_RunCallback = NULL;
}

Executor::~Executor()
{

}

UINT Executor::Run()
{
	paramTop.clear();
	callStack.clear();

	paramTop.push_back(0);

	genStack.clear();
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

	UINT startTime = timeGetTime();

	while(m_cmds->GetSHORT(pos, cmd) && !done)
	{
		cmdCount++;
		if(m_RunCallback && cmdCount % 5000000 == 0)
			if(!m_RunCallback(cmdCount))
			{
				done = true;
				throw std::string("User have canceled the execution");
			}
		pos2 = pos;
		pos += 2;
		//DBG(m_FileStream << pos2 << " ");
		switch(cmd)
		{
		case cmdCallStd:
			{
				size_t len;
				m_cmds->GetData(pos, len);
				pos += sizeof(size_t);
				if(len >= 511)
					throw std::string("ERROR: standard function can't have length>512");
				m_cmds->GetData(pos, name, len);
				pos += (UINT)len;
				name[len] = 0;
				if(memcmp(name, "clock", 5) != 0)
				{
					val = *((double*)(&genStack[genStack.size()-2]));
					genStack.pop_back(); genStack.pop_back();
					genStackTypes.pop_back();
				}
				if(memcmp(name, "cos", 3) == 0)
					val = cos(val/180.0*3.14159265358);
				else if(memcmp(name, "sin", 3) == 0)
					val = sin(val/180.0*3.14159265358);
				else if(memcmp(name, "tan", 3) == 0)
					val = tan(val/180.0*3.14159265358);
				else if(memcmp(name, "ctg", 3) == 0)
					val = 1.0/tan(val/180.0*3.14159265358);
				else if(memcmp(name, "ceil", 4) == 0)
					val = ceil(val);
				else if(memcmp(name, "floor", 5) == 0)
					val = floor(val);
				else if(memcmp(name, "sqrt", 4) == 0)
					val = sqrt(val);
				else if(memcmp(name, "clock", 5) == 0)
					uintVal = GetTickCount();
				else
					throw std::string("ERROR: there is no such function: ") + name;
				if(fabs(val) < 1e-10)
					val = 0.0;
				if(memcmp(name, "clock", 5) != 0)
				{
					genStack.push_back((UINT*)(&val), 2);
					genStackTypes.push_back(STYPE_DOUBLE);
				}else{
					genStack.push_back(uintVal);
					genStackTypes.push_back(STYPE_INT);
				}
				DBG(m_FileStream << pos2 << dec << " CALLS " << name << ";");
			}
			break;
		case cmdSwap:
			m_cmds->GetUSHORT(pos, cFlag);
			pos += 2;
			switch(cFlag)
			{
			case (STYPE_DOUBLE)+(DTYPE_DOUBLE):
			case (STYPE_LONG)+(DTYPE_LONG):
				valind = genStack[genStack.size()-2];
				genStack[genStack.size()-2] = genStack[genStack.size()-4];
				genStack[genStack.size()-4] = valind;
				valind = genStack[genStack.size()-1];
				genStack[genStack.size()-1] = genStack[genStack.size()-3];
				genStack[genStack.size()-3] = valind;
				break;
			case (STYPE_DOUBLE)+(DTYPE_INT):
			case (STYPE_LONG)+(DTYPE_INT):
				valind = genStack[genStack.size()-2];
				genStack[genStack.size()-2] = genStack[genStack.size()-1];
				genStack[genStack.size()-1] = valind;
				valind = genStack[genStack.size()-3];
				genStack[genStack.size()-3] = genStack[genStack.size()-2];
				genStack[genStack.size()-2] = valind;
				break;
			case (STYPE_INT)+(DTYPE_DOUBLE):
			case (STYPE_INT)+(DTYPE_LONG):
				valind = genStack[genStack.size()-3];
				genStack[genStack.size()-3] = genStack[genStack.size()-2];
				genStack[genStack.size()-2] = valind;
				valind = genStack[genStack.size()-1];
				genStack[genStack.size()-1] = genStack[genStack.size()-2];
				genStack[genStack.size()-2] = valind;
				break;
			case (STYPE_INT)+(DTYPE_INT):
				valind = genStack[genStack.size()-1];
				genStack[genStack.size()-1] = genStack[genStack.size()-2];
				genStack[genStack.size()-2] = valind;
				break;
			default:
				throw std::string("cmdSwap, unimplemented type combo");
			}
			st = genStackTypes[genStackTypes.size()-2];
			genStackTypes[genStackTypes.size()-2] = genStackTypes[genStackTypes.size()-1];
			genStackTypes[genStackTypes.size()-1] = st;

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, cFlag, 0));
			break;
		case cmdCopy:
			m_cmds->GetUCHAR(pos, oFlag);
			pos += 1;
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
			case OTYPE_LONG:
				genStack.push_back(genStack[genStack.size()-2]);
				genStack.push_back(genStack[genStack.size()-2]);
				break;
			case OTYPE_INT:
				genStack.push_back(genStack[genStack.size()-1]);
				break;
			}
			genStackTypes.push_back(genStackTypes[genStackTypes.size()-1]);

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
				m_cmds->GetUINT(pos, uintVal);
				pos += 4;
				m_cmds->GetUSHORT(pos, retFlag);
				pos += 2;
				callStack.push_back(CallStackInfo(pos, (UINT)genStack.size(), uintVal));
				pos = uintVal;
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0, retFlag));
			}
			break;
		case cmdReturn:
			{
				USHORT	retFlag, popCnt;
				m_cmds->GetUSHORT(pos, retFlag);
				pos += 2;
				m_cmds->GetUSHORT(pos, popCnt);
				pos += 2;
				if(retFlag & bitRetError)
					throw std::string("ERROR: function didn't return a value");
				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, popCnt, 0, 0, retFlag));
				for(int pops = 0; pops < popCnt; pops++)
				{
					while(genParams.size() > paramTop.back())
						genParams.pop_back();
					paramTop.pop_back();
				}
				if(callStack.size() == 0)
				{
					done = true;
					break;
				}
				pos = callStack.back().cmd;
				callStack.pop_back();
			}
			break;
		case cmdPushV:
			int valind;
			m_cmds->GetINT(pos, valind);
			pos += sizeof(UINT);
			genParams.resize(genParams.size()+valind);
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
			break;
		case cmdNop:
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, 0));
			break;

		case cmdITOL:
			if((int)genStack.back() < 0)
				genStack.push_back(0xFFFFFFFF);
			else
				genStack.push_back(0);
			genStackTypes.back() = STYPE_LONG;
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, 0));
			break;
		case cmdLTOI:
			genStack.pop_back();
			genStackTypes.back() = STYPE_INT;
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, 0));
			break;
		case cmdCTI:
			m_cmds->GetUCHAR(pos, oFlag);
			pos += 1;
			m_cmds->GetUINT(pos, uintVal);
			pos += sizeof(UINT);
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				uintVal2 = int(*((double*)(&genStack[genStack.size()-2])));
				genStack.pop_back(); genStack.pop_back();
				break;
			case OTYPE_LONG:
				uintVal2 = int(*((long long*)(&genStack[genStack.size()-2])));
				genStack.pop_back(); genStack.pop_back();
				break;
			case OTYPE_INT:
				uintVal2 = *((int*)(&genStack[genStack.size()-1]));
				genStack.pop_back();
				break;
			}
			genStackTypes.pop_back();
			genStack.push_back(uintVal*uintVal2);
			genStackTypes.push_back(STYPE_INT);
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0));
			break;
		case cmdSetRange:
			m_cmds->GetUSHORT(pos, cFlag);
			pos += 2;
			m_cmds->GetUINT(pos, uintVal);
			pos += 4;
			uintVal += paramTop.back();
			m_cmds->GetUINT(pos, uintVal2);
			pos += 4;
			for(UINT varNum = 0; varNum < uintVal2; varNum++)
			{
				switch(cFlag)
				{
				case DTYPE_DOUBLE:
					*((double*)(&genParams[uintVal])) = *((double*)(&genStack[genStack.size()-2]));
					uintVal += 8;
					break;
				case DTYPE_FLOAT:
					*((float*)(&genParams[uintVal])) = (float)*((double*)(&genStack[genStack.size()-2]));
					uintVal += 4;
					break;
				case DTYPE_LONG:
					*((long long*)(&genParams[uintVal])) = *((long long*)(&genStack[genStack.size()-2]));
					uintVal += 8;
					break;
				case DTYPE_INT:
					*((int*)(&genParams[uintVal])) = *((int*)(&genStack[genStack.size()-1]));
					uintVal += 4;
					break;
				case DTYPE_SHORT:
					*((short*)(&genParams[uintVal])) = *((int*)(&genStack[genStack.size()-1]));
					uintVal += 2;
					break;
				case DTYPE_CHAR:
					*((char*)(&genParams[uintVal])) = *((int*)(&genStack[genStack.size()-1]));
					uintVal += 1;
					break;
				}
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, cFlag, 0, uintVal2));
			break;
		case cmdGetAddr:
			m_cmds->GetUINT(pos, uintVal);
			pos += 4;
			genStack.push_back(uintVal + paramTop.back());
			genStackTypes.push_back(STYPE_INT);
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, uintVal, 0, 0));
			break;
		}

		//New commands
		if(cmd == cmdPush || cmd == cmdMov)
		{
			int valind = -1, shift, size;
			UINT	highDW = 0, lowDW = 0;
			USHORT sdata;
			UCHAR cdata;
			m_cmds->GetUSHORT(pos, cFlag);
			pos += 2;
			st = flagStackType(cFlag);
			asmDataType dt = flagDataType(cFlag);

			if(flagAddrRel(cFlag) || flagAddrAbs(cFlag) || flagAddrRelTop(cFlag))
			{
				m_cmds->GetINT(pos, valind);
				pos += 4;
			}
			if(flagShiftStk(cFlag))
			{
				shift = genStack.back();
				genStack.pop_back();
				genStackTypes.pop_back();
			}
			if(flagSizeOn(cFlag))
			{
				m_cmds->GetINT(pos, size);
				pos += 4;
			}
			if(flagSizeStk(cFlag))
			{
				size = genStack.back();
				genStack.pop_back();
				genStackTypes.pop_back();
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
				m_cmds->GetUINT(pos, sizeOfVar);
				pos += 4;
			}

			if(flagAddrRel(cFlag))
				valind += paramTop.back();
			if(flagShiftStk(cFlag))
				valind += shift;
			if(flagAddrRelTop(cFlag))
				valind += genParams.size();

			if(cmd == cmdMov)
			{
				if(flagAddrRelTop(cFlag) && valind+typeSizeD[dt] > genParams.size())
					genParams.reserve(genParams.size()+64);
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					UINT currShift = 0, varSize = sizeOfVar;
					while(varSize >= 4)
					{
						*((UINT*)(&genParams[valind+currShift])) = genStack[genStack.size()-varSize/4];
						varSize -= 4;
						currShift += 4;
					}
					assert(varSize == 0);
				}else if(dt == DTYPE_FLOAT && st == STYPE_DOUBLE)
				{
					UINT arr[2] = { genStack[genStack.size()-2], genStack[genStack.size()-1] };
					float res = (float)(*((double*)(&arr[0])));
					*((float*)(&genParams[valind])) = res;
				}else if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					*((UINT*)(&genParams[valind])) = genStack[genStack.size()-2];
					*((UINT*)(&genParams[valind+4])) = genStack[genStack.size()-1];
				}else if(dt == DTYPE_FLOAT || dt == DTYPE_INT)
				{
					*((UINT*)(&genParams[valind])) = genStack[genStack.size()-1];
				}else if(dt == DTYPE_SHORT)
				{
					sdata = genStack[genStack.size()-1];
					*((USHORT*)(&genParams[valind])) = sdata;
				}else if(dt == DTYPE_CHAR)
				{
					cdata = genStack[genStack.size()-1];
					genParams[valind] = cdata;
				}

				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0, sizeOfVar));
			}else{
				if(flagNoAddr(cFlag)){
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						m_cmds->GetUINT(pos, highDW); pos += 4;
						m_cmds->GetUINT(pos, lowDW); pos += 4;
					}
					if(dt == DTYPE_FLOAT || dt == DTYPE_INT){ m_cmds->GetUINT(pos, lowDW); pos += 4; }
					if(dt == DTYPE_SHORT){ m_cmds->GetUSHORT(pos, sdata); pos += 2; lowDW = (sdata>0?sdata:sdata|0xFFFF0000); }
					if(dt == DTYPE_CHAR){ m_cmds->GetUCHAR(pos, cdata); pos += 1; lowDW = cdata; }
				}else{
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
				}
				
				if(dt == DTYPE_FLOAT && st == STYPE_DOUBLE)	//expand float to double
				{
					double res = (double)(*((float*)(&lowDW)));
					genStack.push_back((UINT*)(&res), 2);
				}else if(st == STYPE_DOUBLE || st == STYPE_LONG)
				{
					genStack.push_back(highDW);
					genStack.push_back(lowDW);
				}else{
					genStack.push_back(lowDW);
				}

				if(dt == DTYPE_COMPLEX_TYPE)
				{
					UINT currShift = 0, varSize = sizeOfVar;
					while(varSize >= 4)
					{
						genStack.push_back(*((UINT*)(&genParams[valind+currShift])));
						varSize -= 4;
						currShift += 4;
					}
					assert(varSize == 0);
					lowDW = sizeOfVar;
				}

				genStackTypes.push_back(st);
				if(st == STYPE_COMPLEX_TYPE)
					genStackTypes.back() = (asmStackType)(sizeOfVar|0x80000000);

				DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0, highDW, lowDW));
			}
		}else if(cmd == cmdRTOI){
			m_cmds->GetUSHORT(pos, cFlag);
			pos += 2;
			asmStackType st = flagStackType(cFlag);
			asmDataType dt = flagDataType(cFlag);
			genStackTypes.pop_back();

			if(st == STYPE_DOUBLE && dt == DTYPE_INT)
			{
				int temp = (int)*((double*)(&genStack[genStack.size()-2]));
				genStack.pop_back();
				*((int*)(&genStack[genStack.size()-1])) = temp;
				genStackTypes.push_back(STYPE_INT);
			}else if(st == STYPE_DOUBLE && dt == DTYPE_LONG){
				*((long long*)(&genStack[genStack.size()-2])) = (long long)*((double*)(&genStack[genStack.size()-2]));
				genStackTypes.push_back(STYPE_LONG);
			}
			
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, cFlag, 0));
		}else if(cmd == cmdITOR){
			m_cmds->GetUSHORT(pos, cFlag);
			pos += 2;
			asmStackType st = flagStackType(cFlag);
			asmDataType dt = flagDataType(cFlag);
			genStackTypes.pop_back();

			if(st == STYPE_INT && dt == DTYPE_DOUBLE)
			{
				double temp = (double)*((int*)(&genStack[genStack.size()-1]));
				genStack.pop_back();
				genStack.push_back((UINT*)(&temp), 2);
				genStackTypes.push_back(STYPE_DOUBLE);
			}
			if(st == STYPE_LONG && dt == DTYPE_DOUBLE)
			{
				double temp = (double)*((long long*)(&genStack[genStack.size()-2]));
				*((double*)(&genStack[genStack.size()-2])) = temp;
				genStackTypes.push_back(STYPE_DOUBLE);
			}

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, cFlag, 0));
		}else if(cmd == cmdJmp){
			m_cmds->GetINT(pos, valind);
			pos = valind;
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
		}else if(cmd == cmdJmpZ){
			m_cmds->GetUCHAR(pos, oFlag);
			pos += 1;
			m_cmds->GetINT(pos, valind);
			pos += 4;
			if(oFlag == OTYPE_DOUBLE){
				if(*((double*)(&genStack[genStack.size()-2])) == 0.0)
					pos = valind;
				genStack.pop_back(); genStack.pop_back();
			}else if(oFlag == OTYPE_LONG){
				if(*((long long*)(&genStack[genStack.size()-1])) == 0L)
					pos = valind;
				genStack.pop_back(); genStack.pop_back();
			}else if(oFlag == OTYPE_INT){
				if(*((int*)(&genStack[genStack.size()-1])) == 0)
					pos = valind;
				genStack.pop_back();
			}
			genStackTypes.pop_back();
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
		}else if(cmd == cmdJmpNZ){
			m_cmds->GetUCHAR(pos, oFlag);
			pos += 1;
			m_cmds->GetINT(pos, valind);
			pos += 4;
			if(oFlag == OTYPE_DOUBLE){
				if(*((double*)(&genStack[genStack.size()-2])) != 0.0)
					pos = valind;
				genStack.pop_back(); genStack.pop_back();
			}else if(oFlag == OTYPE_LONG){
				if(*((long long*)(&genStack[genStack.size()-1])) == 0L)
					pos = valind;
				genStack.pop_back(); genStack.pop_back();
			}else if(oFlag == OTYPE_INT){
				if(*((int*)(&genStack[genStack.size()-1])) != 0)
					pos = valind;
				genStack.pop_back();
			}
			genStackTypes.pop_back();
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, 0, 0));
		}else if(cmd == cmdPop){
			m_cmds->GetUSHORT(pos, cFlag);
			pos += 2;
			asmStackType st = flagStackType(cFlag);
			UINT sizeOfVar = 0;
			if(st == STYPE_DOUBLE || st == STYPE_LONG)
			{
				genStack.pop_back();
				genStack.pop_back();
			}else if(st == STYPE_COMPLEX_TYPE){
				UINT varSize;
				m_cmds->GetUINT(pos, varSize);
				pos += 4;
				sizeOfVar = varSize;
				while(varSize > 0)
				{
					genStack.pop_back();
					varSize -= 4;
				}
			}else{
				genStack.pop_back();
			}
			genStackTypes.pop_back();

			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, sizeOfVar, cFlag, 0));
		}else if(cmd >= cmdAdd && cmd <= cmdLogXor){
			m_cmds->GetUCHAR(pos, oFlag);
			pos += 1;
			switch(cmd + (oFlag << 16))
			{
			case cmdAdd+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) += *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdAdd+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) += *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdAdd+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) += *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdSub+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) -= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdSub+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) -= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdSub+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) -= *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdMul+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) *= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdMul+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) *= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdMul+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) *= *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdDiv+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) /= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdDiv+(OTYPE_LONG<<16):
				if(*((long long*)(&genStack[genStack.size()-2])))
					*((long long*)(&genStack[genStack.size()-4])) /= *((long long*)(&genStack[genStack.size()-2]));
				else{
					if(*((long long*)(&genStack[genStack.size()-4])) > 0)
					{
						*((int*)(&genStack[genStack.size()-3])) = 0x7FFFFFFF;
						*((int*)(&genStack[genStack.size()-4])) = 0xFFFFFFFF;
					}else if(*((long long*)(&genStack[genStack.size()-4])) < 0){
						*((int*)(&genStack[genStack.size()-3])) = 0x80000000;
						*((int*)(&genStack[genStack.size()-4])) = 0;
					}else
						*((long long*)(&genStack[genStack.size()-4])) = 0;
				}
				break;
			case cmdDiv+(OTYPE_INT<<16):
				if(*((int*)(&genStack[genStack.size()-1])))
					*((int*)(&genStack[genStack.size()-2])) /= *((int*)(&genStack[genStack.size()-1]));
				else{
					if(*((int*)(&genStack[genStack.size()-2])) > 0)
						*((int*)(&genStack[genStack.size()-2])) = (1 << 31) - 1;
					else if(*((int*)(&genStack[genStack.size()-2])) < 0)
						*((int*)(&genStack[genStack.size()-2])) = (1 << 31);
					else
						*((int*)(&genStack[genStack.size()-2])) = 0;
				}
				break;
			case cmdPow+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) = pow(*((double*)(&genStack[genStack.size()-4])), *((double*)(&genStack[genStack.size()-2])));
				break;
			case cmdPow+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = (long long)pow((double)*((long long*)(&genStack[genStack.size()-4])), (double)*((long long*)(&genStack[genStack.size()-2])));
				break;
			case cmdPow+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = (int)pow((double)(*((int*)(&genStack[genStack.size()-2]))), *((int*)(&genStack[genStack.size()-1])));
				break;
			case cmdMod+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) = fmod(*((double*)(&genStack[genStack.size()-4])), *((double*)(&genStack[genStack.size()-2])));
				break;
			case cmdMod+(OTYPE_LONG<<16):
				if(*((long long*)(&genStack[genStack.size()-2])))
					*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) % *((long long*)(&genStack[genStack.size()-2]));
				else
					*((long long*)(&genStack[genStack.size()-4])) = 0;
				break;
			case cmdMod+(OTYPE_INT<<16):
				if(*((int*)(&genStack[genStack.size()-1])))
					*((int*)(&genStack[genStack.size()-2])) %= *((int*)(&genStack[genStack.size()-1]));
				else
					*((int*)(&genStack[genStack.size()-2])) = 0;
				
				break;
			case cmdLess+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) < *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdLess+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) < *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLess+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) < *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdGreater+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) > *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdGreater+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) > *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdGreater+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) > *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLEqual+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) <= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdLEqual+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) <= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLEqual+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) <= *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdGEqual+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) >= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdGEqual+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) >= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdGEqual+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) >= *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdEqual+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) == *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdEqual+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) == *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdEqual+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) == *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdNEqual+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) != *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdNEqual+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) != *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdNEqual+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) != *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdShl+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) << *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdShl+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) << *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdShr+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) >> *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdShr+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) >> *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdBitAnd+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) & *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitAnd+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) & *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdBitOr+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) | *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitOr+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) | *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdBitXor+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) ^ *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitXor+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) ^ *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLogAnd+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) && *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLogAnd+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) && *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLogOr+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) || *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLogOr+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) || *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLogXor+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-4])) = !!(*((long long*)(&genStack[genStack.size()-4]))) ^ !!(*((long long*)(&genStack[genStack.size()-2])));
				break;
			case cmdLogXor+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-2])) = !!(*((int*)(&genStack[genStack.size()-2]))) ^ !!(*((int*)(&genStack[genStack.size()-1])));
				break;
			default:
				throw string("Operation is not implemented");
			}
			if(oFlag == OTYPE_INT)
			{
				genStack.pop_back();
			}else{
				genStack.pop_back(); genStack.pop_back();
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, oFlag));
			genStackTypes.pop_back();
			
		}else if(cmd >= cmdNeg && cmd <= cmdLogNot){
			m_cmds->GetUCHAR(pos, oFlag);
			pos += 1;
			switch(cmd + (oFlag << 16))
			{
			case cmdNeg+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-2])) = -*((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdNeg+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-2])) = -*((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdNeg+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-1])) = -*((int*)(&genStack[genStack.size()-1]));
				break;

			case cmdLogNot+(OTYPE_DOUBLE<<16):
				*((double*)(&genStack[genStack.size()-2])) = fabs(*((double*)(&genStack[genStack.size()-2]))) < 1e-10;
				break;
			case cmdLogNot+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-2])) = !*((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLogNot+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-1])) = !*((int*)(&genStack[genStack.size()-1]));
				break;

			case cmdBitNot+(OTYPE_LONG<<16):
				*((long long*)(&genStack[genStack.size()-2])) = ~*((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitNot+(OTYPE_INT<<16):
				*((int*)(&genStack[genStack.size()-1])) = ~*((int*)(&genStack[genStack.size()-1]));
				break;
			default:
				throw string("Operation is not implemented");
			}
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, 0, 0, oFlag));
		}else if(cmd == cmdIncAt || cmd == cmdDecAt)
		{
			int valind, shift, size;
			m_cmds->GetUSHORT(pos, cFlag);
			pos += 2;
			asmDataType dt = flagDataType(cFlag);	//Data type

			if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
			{
				m_cmds->GetINT(pos, valind);
				pos += 4;
			}
			if(flagShiftStk(cFlag)){
				shift = genStack.back();
				genStack.pop_back();
				genStackTypes.pop_back();
			}
			if(flagSizeOn(cFlag)){
				m_cmds->GetINT(pos, size);
				pos += 4;
			}
			if(flagSizeStk(cFlag)){
				size = genStack.back();
				genStack.pop_back();
				genStackTypes.pop_back();
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
			DBG(PrintInstructionText(&m_FileStream, cmd, pos2, valind, cFlag, 0));
		}

		UINT typeSizeS[] = { 1, 2, 0, 2 };
#ifdef _DEBUG
		m_FileStream << "  " << genStack.size() << ";" << genStackTypes.size() << "; // ";
		for(UINT i = 0, k = 0; i < genStackTypes.size(); i++)
		{
			if(genStackTypes[i] & 0x80000000)
			{
				m_FileStream << "complex " << (genStackTypes[i] & ~0x80000000) << " bytes";
				k += genStackTypes[i] & ~0x80000000;
			}
			if(genStackTypes[i] == STYPE_DOUBLE)
				m_FileStream << "double " << *((double*)(&genStack[k])) << ", ";
			if(genStackTypes[i] == STYPE_LONG)
				m_FileStream << "long " << *((long*)(&genStack[k])) << ", ";
			if(genStackTypes[i] == STYPE_INT)
				m_FileStream << "int " << *((int*)(&genStack[k])) << ", ";
			k += typeSizeS[genStackTypes[i]];
		}
		m_FileStream << ";\r\n" << std::flush;
#endif
	}
	UINT runTime = timeGetTime() - startTime;

	m_ostr << "There are " << (UINT)genStackTypes.size() << " values in the stack\r\n";
	if((UINT)genStackTypes.size() == 0)
		m_ostr << "It's bad.\r\nÏèøè return ñöóêî!";//Did you forget 'return'?";
	else if((UINT)genStackTypes.size() == 1)
		m_ostr << "Good.";
	else if((UINT)genStackTypes.size() > 1)
		m_ostr << "It's bug.\r\nReport to NULL_PTR";
	m_ostr << "\r\n\r\n";
	m_ostr << "Variables active: " << (UINT)genParams.size() << "\r\n";
	return runTime;
}

string Executor::GetResult()
{
	if((UINT)genStackTypes.size() == 0)
		throw std::string("There are no values on the stack");
	if((UINT)genStackTypes.size() != 1)
		throw std::string("There are more than one value on the stack");
	ostringstream tempStream;
	switch(genStackTypes[0])
	{
	case STYPE_DOUBLE:
		tempStream << *((double*)(&genStack[0]));
		break;
	case STYPE_LONG:
		tempStream << *((long long*)(&genStack[0])) << 'L';
		break;
	case STYPE_INT:
		tempStream << *((int*)(&genStack[0]));
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