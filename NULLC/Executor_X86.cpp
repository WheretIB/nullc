#include "stdafx.h"
#ifdef NULLC_BUILD_X86_JIT

#include "Executor_X86.h"
#include "StdLib_X86.h"
#include "Translator_X86.h"
#include "Optimizer_x86.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "CodeInfo.h"
using namespace CodeInfo;

UINT paramDataBase;
UINT reservedStack;
UINT commitedStack;
UINT stackGrowSize;
UINT stackGrowCommit;

ExecutorX86::ExecutorX86()
{
	stackGrowSize = 128*4096;
	stackGrowCommit = 64*4096;
	// Request memory at address
	if(NULL == (paramData = (char*)VirtualAlloc(reinterpret_cast<void*>(0x20000000), stackGrowSize, MEM_RESERVE, PAGE_NOACCESS)))
		throw std::string("ERROR: Failed to reserve memory");
	if(!VirtualAlloc(reinterpret_cast<void*>(0x20000000), stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
		throw std::string("ERROR: Failed to commit memory");

	reservedStack = stackGrowSize;
	commitedStack = stackGrowCommit;
	
	paramDataBase = paramBase = static_cast<UINT>(reinterpret_cast<long long>(paramData));

	binCode = new unsigned char[200000];
	memset(binCode, 0x90, 20);
	binCodeStart = static_cast<UINT>(reinterpret_cast<long long>(&binCode[20]));
	binCodeSize = 0;
}
ExecutorX86::~ExecutorX86()
{
	VirtualFree(reinterpret_cast<void*>(0x20000000), 0, MEM_RELEASE);

	delete[] binCode;
}

int runResult = 0;
int runResult2 = 0;
OperFlag runResultType = OTYPE_DOUBLE;

UINT stackReallocs;

UINT expCodePublic;
UINT expAllocCode;
UINT expECXstate;
DWORD CanWeHandleSEH(UINT expCode, _EXCEPTION_POINTERS* expInfo)
{
	expECXstate = expInfo->ContextRecord->Ecx;
	expCodePublic = expCode;
	if(expCode == EXCEPTION_INT_DIVIDE_BY_ZERO || expCode == EXCEPTION_BREAKPOINT || expCode == EXCEPTION_STACK_OVERFLOW)
		return EXCEPTION_EXECUTE_HANDLER;
	if(expCode == EXCEPTION_ACCESS_VIOLATION)
	{
		if(expInfo->ExceptionRecord->ExceptionInformation[1] > paramDataBase &&
			expInfo->ExceptionRecord->ExceptionInformation[1] < expInfo->ContextRecord->Edi+paramDataBase+64*1024)
		{
			// ��������, �� �������� �� �� ����� ��������� ������
			if(reservedStack > 512*1024*1024)
			{
				expAllocCode = 4;
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// �������� ������������� ��������� �������� ����������������� ������
			if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+commitedStack)), stackGrowSize-stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
			{
				expAllocCode = 1; // failed to commit all old memory
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// ������������� ��� ������ ����� ����� ����������� �����
			if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+reservedStack)), stackGrowSize, MEM_RESERVE, PAGE_NOACCESS))
			{
				expAllocCode = 2; // failed to reserve new memory
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// �������� ������������� ���� ����������������� ������ ����� ��������� ��������
			if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+reservedStack)), stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
			{
				expAllocCode = 3; // failed to commit new memory
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// ������� ����������
			commitedStack = reservedStack;
			reservedStack += stackGrowSize;
			commitedStack += stackGrowCommit;
			stackReallocs++;

			return (DWORD)EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	return (DWORD)EXCEPTION_CONTINUE_SEARCH;
}

#pragma warning(disable: 4731)
UINT ExecutorX86::Run(const char* funcName)
{
	stackReallocs = 0;

	*(double*)(paramData) = 0.0;
	*(double*)(paramData+8) = 3.1415926535897932384626433832795;
	*(double*)(paramData+16) = 2.7182818284590452353602874713527;

	LARGE_INTEGER pFreq, pCntS, pCntE;
	QueryPerformanceFrequency(&pFreq);

	UINT binCodeStart = static_cast<UINT>(reinterpret_cast<long long>(&binCode[20]));

	UINT startPos = 20;
	unsigned char	oldCode[16];
	if(funcName)
	{
		UINT funcPos = (unsigned int)-1;
		for(unsigned int i = 0; i < funcInfo.size(); i++)
		{
			if(strcmp(funcInfo[i]->name.c_str(), funcName) == 0)
			{
				funcPos = CodeInfo::funcInfo[i]->address;
				break;
			}
		}
		if(funcPos == -1)
			throw std::string("Cannot find starting function");
		UINT marker = 'N' << 24 | funcPos;

		while(*(UINT*)(binCode+startPos) != marker && startPos < binCodeSize)
			startPos++;
		startPos -= 5;
		memcpy(oldCode, binCode+startPos, 9);
		binCode[startPos+0] = 0x90;
		binCode[startPos+1] = 0x90;
		binCode[startPos+2] = 0x90;
		binCode[startPos+3] = 0x90; // nop */binCode[startPos+0] = 0x55; // push ebp
		binCode[startPos+4] = 0x89; // mov ebp, edi
		binCode[startPos+5] = 0xFD;
		binCode[startPos+6] = 0x83; // add edi, 4
		binCode[startPos+7] = 0xC7;
		binCode[startPos+8] = 0x04;
	}

	UINT res1 = 0;
	UINT res2 = 0;
	UINT resT = 0;
	__try 
	{
		QueryPerformanceCounter(&pCntS);
		__asm
		{
			pusha ; // �������� ��� ��������
			mov eax, binCodeStart ;

			// ����������� ���� �� ������� 8 ����
			lea ebx, [esp+8];
			and ebx, 0fh;
			mov ecx, 16;
			sub ecx, ebx;
			sub esp, ecx;

			push ecx; // �������� �� ������� �������� ����
			push ebp; // �������� ���� ����� (� ������� ����������� �� popa)

			mov ebp, 0h ;
			mov edi, 18h ;

			call eax ; // � ebx ��� ������������ ��������

			pop ebp; // ���������� ���� �����
			pop ecx;
			add esp, ecx;

			mov dword ptr [res1], eax;
			mov dword ptr [res2], edx;
			mov dword ptr [resT], ebx;

			popa ;
		}
		QueryPerformanceCounter(&pCntE);
	}__except(CanWeHandleSEH(GetExceptionCode(), GetExceptionInformation())){
		if(expCodePublic == EXCEPTION_INT_DIVIDE_BY_ZERO)
			throw std::string("ERROR: integer division by zero");
		if(expCodePublic == EXCEPTION_BREAKPOINT && expECXstate != 0xFFFFFFFF)
			throw std::string("ERROR: array index out of bounds");
		if(expCodePublic == EXCEPTION_BREAKPOINT && expECXstate == 0xFFFFFFFF)
			throw std::string("ERROR: function didn't return a value");
		if(expCodePublic == EXCEPTION_STACK_OVERFLOW)
			throw std::string("ERROR: stack overflow");
		if(expCodePublic == EXCEPTION_ACCESS_VIOLATION)
		{
			if(expAllocCode == 1)
				throw std::string("ERROR: Failed to commit old stack memory");
			if(expAllocCode == 2)
				throw std::string("ERROR: Failed to reserve new stack memory");
			if(expAllocCode == 3)
				throw std::string("ERROR: Failed to commit new stack memory");
			if(expAllocCode == 4)
				throw std::string("ERROR: No more memory (512Mb maximum exceeded)");
		}
	}
	UINT runTime = UINT(double(pCntE.QuadPart - pCntS.QuadPart) / double(pFreq.QuadPart) * 1000.0);

	runResult = res1;
	runResult2 = res2;
	runResultType = (OperFlag)resT;

	if(funcName)
		memcpy(binCode+startPos, oldCode, 9);

	return runTime;
}
#pragma warning(default: 4731)

void ExecutorX86::GenListing()
{
	logASM.str("");

	UINT pos = 0, pos2 = 0;
	CmdID	cmd, cmdNext;
	UINT	valind, valind2;

	CmdFlag cFlag;
	OperFlag oFlag;
	asmStackType st;
	asmDataType dt;

	vector<unsigned int> instrNeedLabel;	// ����� �� ����� ����������� ����� �����
	vector<unsigned int> funcNeedLabel;	// ����� �� ����� ����������� ����� �������

	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		if(CodeInfo::funcInfo[i]->funcPtr == NULL && CodeInfo::funcInfo[i]->address != -1)
			funcNeedLabel.push_back(CodeInfo::funcInfo[i]->address);

	FunctionInfo *funcInfo;

	//������, ���� ����� ������
	while(cmdList->GetData(pos, cmd))
	{
		pos2 = pos;
		pos += 2;
		switch(cmd)
		{
		case cmdCallStd:
			size_t len;
			cmdList->GetData(pos, len);
			pos += sizeof(FunctionInfo*);
			break;
		case cmdPushVTop:
			break;
		case cmdPopVTop:
			break;
		case cmdCall:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			pos += 2;
			//funcNeedLabel.push_back(valind);
			break;
		case cmdFuncAddr:
			cmdList->GetData(pos, funcInfo);
			pos += sizeof(FunctionInfo*);
			//if(funcInfo->funcPtr == NULL)
			//	funcNeedLabel.push_back(funcInfo->address);
			break;
		case cmdReturn:
			pos += 4;
			break;
		case cmdPushV:
			pos += 4;
			break;
		case cmdNop:
			break;
		case cmdCTI:
			pos += 5;
			break;
		case cmdPushImmt:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			dt = flagDataType(cFlag);

			if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				pos += 8;
			if(dt == DTYPE_FLOAT || dt == DTYPE_INT)
				pos += 4;
			if(dt == DTYPE_SHORT)
				pos += 2;
			if(dt == DTYPE_CHAR)
				pos += 1;
			break;
		case cmdPush:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				dt = flagDataType(cFlag);

				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)))
					pos += 4;
				if(flagSizeOn(cFlag))
					pos += 4;
				
				if(flagNoAddr(cFlag))
				{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
						pos += 8;
					if(dt == DTYPE_FLOAT)
						pos += 4;
					if(dt == DTYPE_INT)
						pos += 4;
					if(dt == DTYPE_SHORT)
						pos += 2;
					if(dt == DTYPE_CHAR)
						pos += 1;
				}
				if(dt == DTYPE_COMPLEX_TYPE)
					pos += 4;
			}
			break;
		case cmdMov:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)))
					pos += 4;
				if(flagSizeOn(cFlag))
					pos += 4;
				if(flagDataType(cFlag) == DTYPE_COMPLEX_TYPE)
					pos += 4;
			}
			break;
		case cmdPop:
			cmdList->GetUSHORT(pos, cFlag);
			//pos += 2;
			//dt = flagDataType(cFlag);
			//if(dt == DTYPE_COMPLEX_TYPE)
			pos += 4;
			break;
		case cmdRTOI:
			pos += 2;
			break;
		case cmdITOR:
			pos += 2;
			break;
		case cmdITOL:
			break;
		case cmdLTOI:
			break;
		case cmdSwap:
			pos += 2;
			break;
		case cmdCopy:
			pos += 1;
			break;
		case cmdJmp:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			instrNeedLabel.push_back(valind);
			break;
		case cmdJmpZ:
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			instrNeedLabel.push_back(valind);
			break;
		case cmdJmpNZ:
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			instrNeedLabel.push_back(valind);
			break;
		case cmdSetRange:
			pos += 10;
			break;
		case cmdGetAddr:
			pos += 4;
			break;
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
			pos += 1;
		if(cmd >= cmdNeg && cmd <= cmdLogNot)
			pos += 1;
		if(cmd >= cmdIncAt && cmd < cmdDecAt)
		{
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;

			if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
				pos += 4;
			if(flagSizeOn(cFlag))
				pos += 4;
		}
	}

	logASM << "use32\r\n";
	UINT typeSizeD[] = { 1, 2, 4, 8, 4, 8 };

	int pushLabels = 1;
	int movLabels = 1;
	//int skipLabels = 1;
	int aluLabels = 1;

	bool skipPopEAXOnIntALU = false;
	bool skipFldESPOnDoubleALU = false;
	bool skipFldOnMov = false;

	bool skipPopEDXOnPush = false;
	bool indexInEaxOnCti = false;

	bool knownEDXOnPush = false;
	bool addEBPtoEDXOnPush = false;
	int edxValueForPush = 0;

	bool skipPop = false;

	UINT lastVarSize = 0;
	bool mulByVarSize = false;

	pos = 0;
	pos2 = 0;
	while(cmdList->GetData(pos, cmd))
	{
		for(UINT i = 0; i < instrNeedLabel.size(); i++)
		{
			if(pos == instrNeedLabel[i])
			{
				logASM << "  gLabel" << pos << ": \r\n";
				break;
			}
		}
		for(UINT i = 0; i < funcNeedLabel.size(); i++)
		{
			if(pos == funcNeedLabel[i])
			{
				//logASM << "  db " << "'N', " << (pos>>16) << ", " << (0xFF&(pos>>8)) << ", " << (pos&0xFF) << "; marker \r\n";
				logASM << "  dd " << (('N' << 24) | pos) << "; marker \r\n";
				logASM << "  function" << pos << ": \r\n";
				break;
			}
		}

		pos2 = pos;
		pos += 2;
		const char *descStr = cmdList->GetDescription(pos2);
		if(descStr)
			logASM << "\r\n  ; \"" << descStr << "\" codeinfo\r\n";

		switch(cmd)
		{
		case cmdDTOF:
			logASM << "  ; DTOF \r\n";
			logASM << "fld qword [esp] \r\n";
			logASM << "fstp dword [esp+4] \r\n";
			logASM << "add esp, 4 \r\n";
			break;
		case cmdCallStd:
			logASM << "  ; CALLSTD ";
			cmdList->GetData(pos, funcInfo);
			pos += sizeof(FunctionInfo*);
			if(!funcInfo)
				throw std::string("ERROR: std function info is invalid");

			if(funcInfo->funcPtr == NULL)
			{
				if(funcInfo->name == "cos")
				{
					logASM << "cos \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fsincos \r\n";
					logASM << "fstp qword [esp] \r\n";
					logASM << "fstp st \r\n";
				}else if(funcInfo->name == "sin"){
					logASM << "sin \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fsincos \r\n";
					logASM << "fstp st \r\n";
					logASM << "fstp qword [esp] \r\n";
				}else if(funcInfo->name == "tan"){
					logASM << "tan \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fptan \r\n";
					logASM << "fstp st \r\n";
					logASM << "fstp qword [esp] \r\n";
				}else if(funcInfo->name == "ctg"){
					logASM << "ctg \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fptan \r\n";
					logASM << "fdivrp \r\n";
					logASM << "fstp qword [esp] \r\n";
				}else if(funcInfo->name == "ceil"){
					logASM << "ceil \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "push eax ; ���� ������� ���� fpu \r\n";
					logASM << "fstcw word [esp] ; �������� ���� �������� \r\n";
					logASM << "mov word [esp+2], 1BBFh ; �������� ���� � ����������� � +inf \r\n";
					logASM << "fldcw word [esp+2] ; ��������� ��� \r\n";
					logASM << "frndint ; �������� �� ������ \r\n";
					logASM << "fldcw word [esp] ; ���������� ���� �������� \r\n";
					logASM << "fstp qword [esp+4] \r\n";
					logASM << "pop eax ; \r\n";
				}else if(funcInfo->name == "floor"){
					logASM << "floor \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "push eax ; ���� ������� ���� fpu \r\n";
					logASM << "fstcw word [esp] ; �������� ���� �������� \r\n";
					logASM << "mov word [esp+2], 17BFh ; �������� ���� � ����������� � -inf \r\n";
					logASM << "fldcw word [esp+2] ; ��������� ��� \r\n";
					logASM << "frndint ; �������� �� ������ \r\n";
					logASM << "fldcw word [esp] ; ���������� ���� �������� \r\n";
					logASM << "fstp qword [esp+4] \r\n";
					logASM << "pop eax ; \r\n";
				}else if(funcInfo->name == "sqrt"){
					logASM << "sqrt \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fsqrt \r\n";
					logASM << "fstp qword [esp] \r\n";
					logASM << "fstp st \r\n";
				}else{
					throw std::string("ERROR: there is no such function: ") + funcInfo->name;
				}
			}else{
				if(funcInfo->retType->size > 4 && funcInfo->retType->type != TypeInfo::TYPE_DOUBLE)
					throw std::string("ERROR: user functions with return type size larger than 4 bytes are not supported");
				UINT bytesToPop = 0;
				for(UINT i = 0; i < funcInfo->params.size(); i++)
				{
					bytesToPop += funcInfo->params[i].varType->size > 4 ? funcInfo->params[i].varType->size : 4;
				}
				logASM << funcInfo->name << "\r\n";
				logASM << "mov ecx, 0x" << funcInfo->funcPtr << " ; " << funcInfo->name << "() \r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, " << bytesToPop << " \r\n";
				if(funcInfo->retType->size != 0)
					logASM << "push eax \r\n";
			}
			break;
		case cmdPushVTop:
			logASM << "  ; PUSHT\r\n";
			logASM << "push ebp ; ��������� ������� ���� ����� ����������\r\n";
			logASM << "mov ebp, edi ; ���������� ����� ���� ����� ����������, �� ������� �����\r\n";
			break;
		case cmdPopVTop:
			logASM << "  ; POPT\r\n";
			logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
			logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
			break;
		case cmdCall:
			{
				RetFlag retFlag;
				cmdList->GetUINT(pos, valind);
				pos += 4;
				cmdList->GetUSHORT(pos, retFlag);
				pos += 2;
				logASM << "  ; CALL " << valind << " ret " << (retFlag & bitRetSimple ? "simple " : "") << "size: ";
				if(retFlag & bitRetSimple)
				{
					oFlag = (OperFlag)(retFlag & 0x0FFF);
					if(oFlag == OTYPE_DOUBLE)
						logASM << "double\r\n";
					if(oFlag == OTYPE_LONG)
						logASM << "long\r\n";
					if(oFlag == OTYPE_INT)
						logASM << "int\r\n";
				}else{
					logASM << (retFlag&0x0FFF) << "\r\n";
				}
				if(valind == -1)
				{
					logASM << "pop eax ;\r\n";
					logASM << "xchg [esp], eax ;\r\n";
					logASM << "call eax ; \r\n";
				}else{
					logASM << "call function" << valind << "\r\n";
				}
				if(!(retFlag & bitRetSimple) && retFlag > 16)
					logASM << "mov eax, edi ; �������� ������ edi\r\n";
				logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
				logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
				if(retFlag & bitRetSimple)
				{
					oFlag = (OperFlag)(retFlag & 0x0FFF);
					if(oFlag == OTYPE_INT)
						logASM << "push eax ; �������� int ������� � ����\r\n";
					if(oFlag == OTYPE_DOUBLE)
					{
						logASM << "push eax ; \r\n";
						logASM << "push edx ; �������� double ������� � ����\r\n";
					}
					if(oFlag == OTYPE_LONG)
					{
						logASM << "push eax ; \r\n";
						logASM << "push edx ; �������� long ������� � ����\r\n";
					}
				}else{
					if(retFlag != 0)
					{
						if(retFlag == 4)
						{
							logASM << "push eax ; �������� �����. ���������� � 4 ����� �� ��������\r\n";
						}else if(retFlag == 8){
							logASM << "push eax \r\n";
							logASM << "push edx ; �������� �����. ���������� � 8 ���� � ��������\r\n";
						}else if(retFlag == 12){
							logASM << "push eax \r\n";
							logASM << "push edx \r\n";
							logASM << "push ecx ; �������� �����. ���������� � 12 ���� � ��������\r\n";
						}else if(retFlag == 16){
							logASM << "push eax \r\n";
							logASM << "push edx \r\n";
							logASM << "push ecx \r\n";
							logASM << "push ebx ; �������� �����. ���������� � 16 ���� � ��������\r\n";
						}else{
							logASM << "sub esp, " << retFlag << "; ��������� � ����� ����� ��� ����������\r\n";

							logASM << "mov ebx, edi ; �������� ����� edi\r\n";
							
							logASM << "lea esi, [eax + " << paramBase << "] ; �������� ���� � ������� ����� ����������\r\n";
							logASM << "mov edi, esp ; ���������� �� ����� �� ������� ����� ����������\r\n";
							logASM << "mov ecx, " << retFlag/4 << " ; ������ ����������\r\n";
							logASM << "rep movsd ; ��������\r\n";

							logASM << "mov edi, ebx ; �������������� edi\r\n";
						}
					}
				}
			}
			break;
		case cmdReturn:
			{
				USHORT	retFlag, popCnt;
				logASM << "  ; RET\r\n";
				cmdList->GetUSHORT(pos, retFlag);
				pos += 2;
				cmdList->GetUSHORT(pos, popCnt);
				pos += 2;
				if(retFlag & bitRetError)
				{
					logASM << "mov ecx, " << 0xffffffff << " ; ������, ����� �� ������� �������\r\n";
					logASM << "int 3 ; ��������� ����������\r\n";
					break;
				}
				if(retFlag == 0)
				{
					logASM << "ret ; ������������ �� �������\r\n";
					break;
				}
				if(retFlag & bitRetSimple)
				{
					oFlag = (OperFlag)(retFlag & 0x0FFF);
					if(oFlag == OTYPE_DOUBLE)
					{
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �� ����� �������� double � ��������\r\n";
					}else if(oFlag == OTYPE_LONG){
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �� ����� �������� long � ��������\r\n";
					}else if(oFlag == OTYPE_INT){
						logASM << "pop eax ; �� ����� �������� int � �������\r\n";
					}
					for(int pops = 0; pops < popCnt-1; pops++)
					{
						logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
						logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
					}
					if(popCnt == 0)
						logASM << "mov ebx, " << (UINT)(oFlag) << " ; �������� oFlag ����� ������� �����, ����� ��� ��������\r\n";
				}else{
					if(retFlag == 4)
					{
						logASM << "pop eax ; �������� �����. ���������� � 4 ����� � �������\r\n";
					}else if(retFlag == 8){
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �������� �����. ���������� � 8 ���� � ��������\r\n";
					}else if(retFlag == 12){
						logASM << "pop ecx \r\n";
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �������� �����. ���������� � 12 ���� � ��������\r\n";
					}else if(retFlag == 16){
						logASM << "pop ebx \r\n";
						logASM << "pop ecx \r\n";
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �������� �����. ���������� � 12 ���� � ��������\r\n";
					}else{
						logASM << "mov ebx, edi ; �������� edi\r\n";

						logASM << "mov esi, esp ; �������� ���� �� ����� � ����\r\n";
						logASM << "lea edi, [edi + " << paramBase << "] ; ���������� �� ����� �� ������� ����� ����������\r\n";
						logASM << "mov ecx, " << retFlag/4 << " ; ������ ����������\r\n";
						logASM << "rep movsd ; ��������\r\n";

						logASM << "mov edi, ebx ; �������������� edi\r\n";

						logASM << "add esp, " << retFlag << "; ������� ���� �� �������� ���� ����� ����������\r\n";
					}
					for(int pops = 0; pops < popCnt-1; pops++)
					{
						logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
						logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
					}
					if(popCnt == 0)
						logASM << "mov ebx, " << 16 << " ; ���� ���������� return, �� ���������, ����� ��� ��������\r\n";
				}
				logASM << "ret ; ������������ �� �������\r\n";
			}
			break;
		case cmdPushV:
			logASM << "  ; PUSHV\r\n";
			cmdList->GetData(pos, &valind, sizeof(int));
			pos += sizeof(int);
			logASM << "add edi, " << valind << " ; �������� ����� ��� ����� ���������� � �����\r\n";
			break;
		case cmdNop:
			logASM << "  ; NOP\r\n";
			logASM << "nop \r\n";
			break;
		case cmdCTI:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << "  ; CTI " << valind << "\r\n";
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << "fld qword [esp] \r\n";
				logASM << "fistp dword[esp+4] \r\n";
				logASM << "pop eax ; �������� double int'��\r\n";
				break;
			case OTYPE_LONG:
				logASM << "pop edx ; ������ ������� ���� long �� �����\r\n";
				break;
			case OTYPE_INT:
				break;
			}
			//look at the next command
			cmdList->GetData(pos, cmdNext);
			if(valind != 1)
			{
				char *indexPlace = "dword [esp]";
				if(indexInEaxOnCti)
				{
					indexPlace = "eax";
					skipPopEAXOnIntALU = true;
				}
				if((cmdNext == cmdPush || cmdNext == cmdMov || cmdNext == cmdIncAt || cmdNext == cmdDecAt) && (valind == 2 || valind == 4 || valind == 8))
				{
					mulByVarSize = true;
					lastVarSize = valind;
				}else if(valind == 2)
				{
					logASM << "shl " << indexPlace << ", 1 ; ������� ����� �� ������ ����������\r\n";
				}else if(valind == 4){
					logASM << "shl " << indexPlace << ", 2 ; ������� ����� �� ������ ����������\r\n";
				}else if(valind == 8){
					logASM << "shl " << indexPlace << ", 3 ; ������� ����� �� ������ ����������\r\n";
				}else if(valind == 16){
					logASM << "shl " << indexPlace << ", 4 ; ������� ����� �� ������ ����������\r\n";
				}else{
					if(!indexInEaxOnCti)
						logASM << "pop eax ; ������ � eax\r\n";
					logASM << "imul eax, " << valind << " ; ������� ����� �� ������ ����������\r\n";
					if(!indexInEaxOnCti)
						logASM << "push eax \r\n";
				}
			}else{
				if(indexInEaxOnCti)
				{
					if(cmdNext != cmdAdd)
						logASM << "push eax \r\n";
					else
						skipPopEAXOnIntALU = true;
				}
			}
			indexInEaxOnCti = false;
			break;
		case cmdPushImmt:
			{
				logASM << "  ; PUSHIMMT\r\n";
				UINT	highDW = 0, lowDW = 0;
				USHORT sdata;
				UCHAR cdata;
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				st = flagStackType(cFlag);
				dt = flagDataType(cFlag);

				char *texts[] = { "", "edx + ", "ebp + ", "push ", "mov eax, " };
				char *needPush = texts[3];
				addEBPtoEDXOnPush = false;

				mulByVarSize = false;

				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					cmdList->GetUINT(pos, highDW); pos += 4;
					cmdList->GetUINT(pos, lowDW); pos += 4;
					logASM << "push " << lowDW << "\r\n";
					logASM << "push " << highDW << " ; �������� double ��� long long\r\n";
				}
				if(dt == DTYPE_FLOAT)
				{
					// ����� ����� ��� double
					cmdList->GetUINT(pos, lowDW); pos += 4;
					double res = (double)*((float*)(&lowDW));
					logASM << "push " << *((UINT*)(&res)+1) << "\r\n";
					logASM << "push " << *((UINT*)(&res)) << " ; �������� float ��� double\r\n";
				}
				if(dt == DTYPE_INT)
				{
					//look at the next command
					cmdList->GetData(pos+4, cmdNext);
					if(cmdNext >= cmdAdd && cmdNext <= cmdLogOr) // for binary commands except LogicalXOR
					{
						needPush = texts[4];
						skipPopEAXOnIntALU = true;
					}
					if(cmdNext == cmdPush || cmdNext == cmdMov || cmdNext == cmdIncAt || cmdNext == cmdDecAt)
					{
						CmdFlag lcFlag;
						cmdList->GetUSHORT(pos+6, lcFlag);
						if((flagAddrAbs(lcFlag) || flagAddrRel(lcFlag)) && flagShiftStk(lcFlag))
							knownEDXOnPush = true;
					}

					cmdList->GetUINT(pos, lowDW); pos += 4;
					if(knownEDXOnPush)
						edxValueForPush = (int)lowDW;
					else
						logASM << needPush << (int)lowDW << " ; �������� int\r\n";
				}
				if(dt == DTYPE_SHORT)
				{
					cmdList->GetUSHORT(pos, sdata); pos += 2;
					lowDW = (sdata > 0 ? sdata : sdata | 0xFFFF0000);
					logASM << "push " << lowDW << " ; �������� short\r\n";
				}
				if(dt == DTYPE_CHAR)
				{
					cmdList->GetUCHAR(pos, cdata); pos += 1;
					lowDW = cdata;
					logASM << "push " << lowDW << " ; �������� char\r\n";
				}
			}
			break;
		case cmdPush:
			{
				logASM << "  ; PUSH\r\n";
				int valind = -1, /*shift, */size;
				UINT	highDW = 0, lowDW = 0;
				USHORT sdata;
				UCHAR cdata;
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				st = flagStackType(cFlag);
				dt = flagDataType(cFlag);

				char *texts[] = { "", "edx + ", "ebp + ", "push ", "mov eax, " };
				char *needPush = texts[3];
				char *needEDX = texts[1];
				char *needEBP = texts[2];
				if(flagAddrAbs(cFlag) && !addEBPtoEDXOnPush)
					needEBP = texts[0];
				addEBPtoEDXOnPush = false;
				UINT numEDX = 0;

				// ���� �������� �� ���������� �...
				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagShiftStk(cFlag))
				{
					// ...���� ����� � ������� � ������� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(knownEDXOnPush)
					{
						if(mulByVarSize)
							numEDX = edxValueForPush * lastVarSize + valind;
						else
							numEDX = edxValueForPush + valind;
						needEDX = texts[0];
						knownEDXOnPush = false;
					}else{
						if(skipPopEDXOnPush)
						{
							if(mulByVarSize)
							{
								if(valind != 0)
									logASM << "lea edx, [edx*" << lastVarSize << " + " << valind << "]\r\n";
								else
									logASM << "lea edx, [edx*" << lastVarSize << "]\r\n";
							}else{
								numEDX = valind;
							}
							skipPopEDXOnPush = false;
						}else{
							if(mulByVarSize)
							{
								logASM << "pop eax ; ����� �����\r\n";
								if(valind != 0)
									logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
								else
									logASM << "lea edx, [eax*" << lastVarSize << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
							}else{
								if(valind != 0)
								{
									logASM << "pop edx ; ����� �����\r\n";
									numEDX = valind;
								}else{
									logASM << "pop edx ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
								}
							}
						}
					}
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)))
				{
					// ...���� ����� � �������
					cmdList->GetINT(pos, valind);
					pos += 4;

					needEDX = texts[0];
					numEDX = valind;
				}

				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					logASM << "cmp eax, " << (mulByVarSize ? size/lastVarSize : size) << " ; ������� ����� � ������������\r\n";
					logASM << "jb pushLabel" << pushLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "int 3 \r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					pushLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp [esp], eax ; ������� � ������������ ������� � �����\r\n";
					logASM << "ja pushLabel" << pushLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "int 3 \r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					logASM << "pop eax ; ������ �������������� ������\r\n";
					pushLabels++;
				}
				mulByVarSize = false;

				if(flagNoAddr(cFlag))
				{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						cmdList->GetUINT(pos, highDW); pos += 4;
						cmdList->GetUINT(pos, lowDW); pos += 4;
						logASM << "push " << lowDW << "\r\n";
						logASM << "push " << highDW << " ; �������� double ��� long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						// ����� ����� ��� double
						cmdList->GetUINT(pos, lowDW); pos += 4;
						double res = (double)*((float*)(&lowDW));
						logASM << "push " << *((UINT*)(&res)+1) << "\r\n";
						logASM << "push " << *((UINT*)(&res)) << " ; �������� float ��� double\r\n";
					}
					if(dt == DTYPE_INT)
					{
						//look at the next command
						cmdList->GetData(pos+4, cmdNext);
						if(cmdNext >= cmdAdd && cmdNext <= cmdLogOr) // for binary commands except LogicalXOR
						{
							needPush = texts[4];
							skipPopEAXOnIntALU = true;
						}
						if(cmdNext == cmdPush || cmdNext == cmdMov || cmdNext == cmdIncAt || cmdNext == cmdDecAt)
						{
							CmdFlag lcFlag;
							cmdList->GetUSHORT(pos+6, lcFlag);
							if((flagAddrAbs(lcFlag) || flagAddrRel(lcFlag)) && flagShiftStk(lcFlag))
								knownEDXOnPush = true;
						}

						cmdList->GetUINT(pos, lowDW); pos += 4;
						if(knownEDXOnPush)
							edxValueForPush = (int)lowDW;
						else
							logASM << needPush << (int)lowDW << " ; �������� int\r\n";
					}
					if(dt == DTYPE_SHORT)
					{
						cmdList->GetUSHORT(pos, sdata); pos += 2;
						lowDW = (sdata > 0 ? sdata : sdata | 0xFFFF0000);
						logASM << "push " << lowDW << " ; �������� short\r\n";
					}
					if(dt == DTYPE_CHAR)
					{
						cmdList->GetUCHAR(pos, cdata); pos += 1;
						lowDW = cdata;
						logASM << "push " << lowDW << " ; �������� char\r\n";
					}
				}else{
					UINT sizeOfVar = 0;
					if(dt == DTYPE_COMPLEX_TYPE)
					{
						cmdList->GetUINT(pos, sizeOfVar);
						pos += 4;
					}

					//look at the next command
					cmdList->GetData(pos, cmdNext);

					if(dt == DTYPE_COMPLEX_TYPE)
					{
						UINT currShift = sizeOfVar;
						while(sizeOfVar >= 4)
						{
							currShift -= 4;
							logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX+currShift << "] ; �������� ����� complex\r\n";
							sizeOfVar -= 4;
						}
						if(sizeOfVar)
						{
							logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX+currShift << "] ; �������� ����� complex\r\n";
							logASM << "add esp, " << 4-sizeOfVar << " ; ������ ������\r\n";
						}
					}
					if(dt == DTYPE_DOUBLE)
					{
						if(cmdNext >= cmdAdd && cmdNext <= cmdNEqual)
						{
							skipFldESPOnDoubleALU = true;
							logASM << "fld qword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� double ����� � FPU\r\n";
						}else if(cmdNext == cmdMov){
							logASM << "fld qword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� double ����� � FPU\r\n";
							skipFldOnMov = true;
						}else{
							logASM << "push dword [" << needEDX << needEBP << paramBase+4+numEDX << "]\r\n";
							logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� double\r\n";
						}
					}
					if(dt == DTYPE_LONG)
					{
						logASM << "push dword [" << needEDX << needEBP << paramBase+4+numEDX << "]\r\n";
						logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						if(cmdNext == cmdMov)
						{
							logASM << "fld dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� float � fpu ����\r\n";
							skipFldOnMov = true;
						}else{
							logASM << "sub esp, 8 ; ��������� ����� ��� double\r\n";
							logASM << "fld dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� float � fpu ����\r\n";
							logASM << "fstp qword [esp] ; �������� double � ������� ����\r\n";
						}
					}
					if(dt == DTYPE_INT)
					{
						if(cmdNext >= cmdAdd && cmdNext <= cmdLogOr) // for binary commands except LogicalXOR
						{
							needPush = texts[4];
							skipPopEAXOnIntALU = true;
						}
						if(cmdNext == cmdPush || cmdNext == cmdMov || cmdNext == cmdIncAt || cmdNext == cmdDecAt)
						{
							CmdFlag lcFlag;
							cmdList->GetUSHORT(pos+2, lcFlag);
							if((flagAddrAbs(lcFlag) || flagAddrRel(lcFlag)) && flagShiftStk(lcFlag))
							{
								skipPopEDXOnPush = true;
								needPush = "mov edx, ";
							}
						}
						if(cmdNext == cmdCTI)
						{
							indexInEaxOnCti = true;
							needPush = "mov eax, ";
						}
						logASM << needPush << "dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� int\r\n";
					}
					if(dt == DTYPE_SHORT)
					{
						logASM << "movsx eax, word [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� short\r\n";
						logASM << "push eax \r\n";
					}
					if(dt == DTYPE_CHAR)
					{
						logASM << "movsx eax, byte [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� char\r\n";
						logASM << "push eax \r\n";
					}
				}
			}
			break;
		case cmdMov:
			{
				logASM << "  ; MOV\r\n";
				int valind = -1, size;

				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				st = flagStackType(cFlag);
				dt = flagDataType(cFlag);

				UINT numEDX = 0;
				bool knownEDX = false;

				// ���� ������� ����� � �����
				if(flagShiftStk(cFlag))
				{
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(knownEDXOnPush)
					{
						if(mulByVarSize)
							numEDX = edxValueForPush * lastVarSize + valind;
						else
							numEDX = edxValueForPush + valind;
						knownEDX = true;
						knownEDXOnPush = false;
					}else{
						if(skipPopEDXOnPush)
						{
							if(mulByVarSize)
							{
								if(valind != 0)
									logASM << "lea edx, [edx*" << lastVarSize << " + " << valind << "]\r\n";
								else
									logASM << "lea edx, [edx*" << lastVarSize << "]\r\n";
							}else{
								numEDX = valind;
							}
							skipPopEDXOnPush = false;
						}else{
							if(mulByVarSize)
							{
								logASM << "pop eax ; ����� �����\r\n";
								if(valind != 0)
									logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
								else
									logASM << "lea edx, [eax*" << lastVarSize << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
							}else{
								if(valind != 0)
								{
									logASM << "pop edx ; ����� �����\r\n";
									numEDX = valind;
								}else{
									logASM << "pop edx ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
								}
							}
						}
					}
				}else{
					cmdList->GetINT(pos, valind);
					pos += 4;

					knownEDX = true;
					numEDX = valind;
				}

				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					logASM << "cmp eax, " << (mulByVarSize ? size/lastVarSize : size) << " ; ������� ����� � ������������\r\n";
					logASM << "jb movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "int 3 \r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					movLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp [esp], eax ; ������� � ������������ ������� � �����\r\n";
					logASM << "ja movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "int 3 \r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					logASM << "pop eax ; ������ �������������� ������\r\n";
					movLabels++;
				}
				mulByVarSize = false;

				char *texts[] = { "", "edx + ", "ebp + ", "edi + " };
				char *dontNeed = texts[0];
				char *needEDX = texts[1];
				char *needEBP = texts[2];
				char *useEDI = texts[3];
				if(knownEDX)
					needEDX = dontNeed;
				if(flagAddrAbs(cFlag) && !addEBPtoEDXOnPush)
					needEBP = dontNeed;
				addEBPtoEDXOnPush = false;
				if(flagAddrRelTop(cFlag))
					needEBP = useEDI;

				UINT final = paramBase+numEDX;

				UINT sizeOfVar = 0;
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					cmdList->GetUINT(pos, sizeOfVar);
					pos += 4;
				}

				//look at the next command
				cmdList->GetData(pos, cmdNext);
				if(cmdNext == cmdPop)
					skipPop = true;

				if(dt == DTYPE_COMPLEX_TYPE)
				{
					if(skipPop)
					{
						UINT currShift = 0;
						while(sizeOfVar >= 4)
						{
							logASM << "pop dword [" << needEDX << needEBP << final+currShift << "] ; ��������� ����� complex\r\n";
							sizeOfVar -= 4;
							currShift += 4;
						}
						assert(sizeOfVar == 0);
					}else{
						UINT currShift = sizeOfVar;
						while(sizeOfVar >= 4)
						{
							currShift -= 4;
							logASM << "mov ebx, [esp+" << sizeOfVar-4 << "] \r\n";
							logASM << "mov dword [" << needEDX << needEBP << final+currShift << "], ebx ; ��������� ����� complex\r\n";
							sizeOfVar -= 4;
						}
						assert(sizeOfVar == 0);
					}
				}
				if(dt == DTYPE_DOUBLE)
				{
					if(skipFldOnMov)
					{
						if(skipPop)
						{
							logASM << "fstp qword [" << needEDX << needEBP << final << "] ; ��������� double ����������\r\n";
							skipFldOnMov = false;
						}else{
							if(cmdNext == cmdMov)
							{
								logASM << "fst qword [" << needEDX << needEBP << final << "] ; ��������� double ����������\r\n";
							}else{
								logASM << "fst qword [" << needEDX << needEBP << final << "] ; ��������� double ����������\r\n";
								logASM << "sub esp, 8 ; ��������� ����� ��� double\r\n";
								logASM << "fstp qword [esp]\r\n";
								skipFldOnMov = false;
							}
						}
					}else{
						if(skipPop)
						{
							logASM << "pop dword [" << needEDX << needEBP << final << "] \r\n";
							logASM << "pop dword [" << needEDX << needEBP << final+4 << "] ; ��������� double ����������.\r\n";
						}else{
							logASM << "fld qword [esp] ; �������� double �� ����� � fpu ����\r\n";
							logASM << "fstp qword [" << needEDX << needEBP << final << "] ; ��������� double ����������\r\n";
						}
					}
				}
				if(dt == DTYPE_LONG)
				{
					if(skipPop)
					{
						logASM << "pop dword [" << needEDX << needEBP << final << "] \r\n";
						logASM << "pop dword [" << needEDX << needEBP << final+4 << "] ; ��������� long long ����������.\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov ecx, [esp+4] \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx \r\n";
						logASM << "mov [" << needEDX << needEBP << final+4 << "], ecx ; ��������� long long ����������.\r\n";
					}
				}
				if(dt == DTYPE_FLOAT)
				{
					if(skipFldOnMov)
					{
						if(skipPop)
						{
							logASM << "fstp dword [" << needEDX << needEBP << final << "] ; ��������� float ����������\r\n";
							skipFldOnMov = false;
						}else{
							if(cmdNext == cmdMov)
							{
								logASM << "fst dword [" << needEDX << needEBP << final << "] ; ��������� float ����������\r\n";
							}else{
								logASM << "fst dword [" << needEDX << needEBP << final << "] ; ��������� float ����������\r\n";
								logASM << "sub esp, 8 ; ��������� ����� ��� double\r\n";
								logASM << "fstp qword [esp]\r\n";
								skipFldOnMov = false;
							}
						}
					}else{
						logASM << "fld qword [esp] ; �������� double �� ����� � fpu ����\r\n";
						logASM << "fstp dword [" << needEDX << needEBP << final << "] ; ��������� float ����������\r\n";
						if(skipPop)
							logASM << "add esp, 8 ;\r\n";
					}
				}
				if(dt == DTYPE_INT)
				{
					if(skipPop)
					{
						logASM << "pop dword [" << needEDX << needEBP << final << "] ; ��������� int ����������\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx ; ��������� int ����������\r\n";
					}
				}
				if(dt == DTYPE_SHORT)
				{
					if(skipPop)
					{
						logASM << "pop ebx \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; ��������� short ����������\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; ��������� short ����������\r\n";
					}
				}
				if(dt == DTYPE_CHAR)
				{
					if(skipPop)
					{
						logASM << "pop ebx \r\n";
						logASM << "mov byte [" << needEDX << needEBP << final << "], bl ; ��������� char ����������\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov byte [" << needEDX << needEBP << final << "], bl ; ��������� char ����������\r\n";
					}
				}
			}
			break;
		case cmdPop:
			logASM << "  ; POP\r\n";
			//cmdList->GetUSHORT(pos, cFlag);
			//pos += 2;
			//st = flagStackType(cFlag);

			if(skipPop)
			{
				//if(st == STYPE_COMPLEX_TYPE)
				pos += 4;
				skipPop = false;
				break;
			}

			//if(

			//if(st == STYPE_DOUBLE || st == STYPE_LONG)
			//{
			//	logASM << "add esp, 8 ; ������ double ��� long\r\n";
			//}else if(st == STYPE_COMPLEX_TYPE){

			cmdList->GetUINT(pos, valind);
			pos += 4;
			if(valind == 4)
				logASM << "pop eax ; ������ int\r\n";
			else
				logASM << "add esp, " << valind << " ; ������ complex\r\n";

			//}else{
			//	logASM << "pop eax ; ������ int\r\n";
			//}
			break;
		case cmdRTOI:
			{
				logASM << "  ; RTOI\r\n";
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;

				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);

				if(st == STYPE_DOUBLE && dt == DTYPE_INT)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fistp dword [esp+4] \r\n";
					logASM << "add esp, 4 \r\n";
				}else if(st == STYPE_DOUBLE && dt == DTYPE_LONG){
					logASM << "fld qword [esp] \r\n";
					logASM << "fistp qword [esp] \r\n";
				}
			}
			break;
		case cmdITOR:
			{
				logASM << "  ; ITOR\r\n";
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);

				if(st == STYPE_INT && dt == DTYPE_DOUBLE)
				{
					logASM << "fild dword [esp] ; �������� � double\r\n";
					logASM << "push eax ; ��������� ����� ��� double\r\n";
					logASM << "fstp qword [esp] ; ��������� double � ����\r\n";
				}
				if(st == STYPE_LONG && dt == DTYPE_DOUBLE)
				{
					logASM << "fild qword [esp] ; �������� � double\r\n";
					logASM << "fstp qword [esp] ; ��������� double � ����\r\n";
				}
			}
			break;
		case cmdITOL:
			logASM << "  ; ITOL\r\n";
			logASM << "pop eax ; ����� int\r\n";
			logASM << "cdq ; ��������� �� long � edx\r\n";
			logASM << "push edx ; �������� ������� � ����\r\n";
			logASM << "push eax ; ������� ������� � ����\r\n";
			break;
		case cmdLTOI:
			logASM << "  ; LTOI\r\n";
			logASM << "pop eax ; ����� ������� ����\r\n";
			logASM << "xchg eax, [esp] ; �������� ������� ��������\r\n";
			break;
		case cmdSwap:
			logASM << "  ; SWAP\r\n";
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			switch(cFlag)
			{
			case (STYPE_DOUBLE)+(DTYPE_DOUBLE):
			case (STYPE_LONG)+(DTYPE_LONG):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp+4]\r\n";
				logASM << "pop edx \r\n";
				logASM << "xchg edx, [esp+4]\r\n";
				logASM << "push edx\r\n";
				logASM << "push eax ; �������� ������� ��� long ��� double\r\n";
				break;
			case (STYPE_DOUBLE)+(DTYPE_INT):
			case (STYPE_LONG)+(DTYPE_INT):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp+4h]\r\n";
				logASM << "xchg eax, [esp]\r\n";
				logASM << "push eax ; �������� ������� (long ��� double) � int\r\n";
				break;
			case (STYPE_INT)+(DTYPE_DOUBLE):
			case (STYPE_INT)+(DTYPE_LONG):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp]\r\n";
				logASM << "xchg eax, [esp+4h]\r\n";
				logASM << "push eax ; �������� ������� int � (long ��� double)\r\n";
				break;
			case (STYPE_INT)+(DTYPE_INT):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp]\r\n";
				logASM << "push eax ; �������� ������� ��� int\r\n";
				break;
			default:
				throw std::string("cmdSwap, unimplemented type combo");
			}
			break;
		case cmdCopy:
			logASM << "  ; COPY\r\n";
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
			case OTYPE_LONG:
				logASM << "mov edx, [esp]\r\n";
				logASM << "mov eax, [esp+4]\r\n";
				logASM << "push eax\r\n";
				logASM << "push edx ; ����������� long ��� double\r\n";
				break;
			case OTYPE_INT:
				logASM << "mov eax, [esp]\r\n";
				logASM << "push eax ; ����������� int\r\n";
				break;
			}
			break;
		case cmdJmp:
			logASM << "  ; JMP\r\n";
			cmdList->GetUINT(pos, valind);
			pos += 4;
			{
				bool jFar = false;
				for(unsigned int i = 0; i < funcNeedLabel.size(); i++)
					if(funcNeedLabel[i] == pos)
						jFar = true;
				logASM << "jmp " << (jFar ? "near " : "") << "gLabel" << valind << "\r\n";
			}
			break;
		case cmdJmpZ:
			logASM << "  ; JMPZ\r\n";
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;

			if(oFlag == OTYPE_DOUBLE)
			{
				logASM << "fldz ; ������� ���� � fpu ����\r\n";
				logASM << "fcomp qword [esp] ; �������\r\n"; 
				logASM << "fnstsw ax ; ��������� ��� �������� �� fpu ���������� ������� � ax\r\n";
				logASM << "pop ebx \r\n";
				logASM << "pop ebx ; ������ double �� �����\r\n";
				logASM << "test ah, 44h ; MSVS � ���-�� ����������\r\n";
				logASM << "jnp gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "pop edx \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; ���������� long == 0\r\n";
				logASM << "jne gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "pop eax \r\n";
				logASM << "test eax, eax ; ���������� int == 0\r\n";
				logASM << "jz gLabel" << valind << "\r\n";
			}
			break;
		case cmdJmpNZ:
			logASM << "  ; JMPNZ\r\n";
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			if(oFlag == OTYPE_DOUBLE)
			{
				logASM << "fldz ; ������� ���� � fpu ����\r\n";
				logASM << "fcomp qword [esp] ; �������\r\n"; 
				logASM << "fnstsw ax ; ��������� ��� �������� �� fpu ���������� ������� � ax\r\n";
				logASM << "pop ebx \r\n";
				logASM << "pop ebx ; ������ double �� �����\r\n";
				logASM << "test ah, 44h ; MSVS � ���-�� ����������\r\n";
				logASM << "jp gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "pop edx \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; ���������� long == 0\r\n";
				logASM << "je gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "pop eax \r\n";
				logASM << "test eax, eax \r\n";
				logASM << "jnz gLabel" << valind << "\r\n";
			}
			break;
		case cmdSetRange:
			logASM << "  ; SETRANGE\r\n";
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			cmdList->GetUINT(pos, valind2);
			pos += 4;
			logASM << "lea ebx, [ebp + " << paramBase+valind << "] ; ��������� �����\r\n";
			logASM << "lea ecx, [ebp + " << paramBase+valind+(valind2-1)*typeSizeD[(cFlag>>2)&0x00000007] << "] ; �������� �����\r\n";
			if(cFlag == DTYPE_FLOAT)
			{
				logASM << "fld qword [esp] ; float � ����\r\n";
			}else{
				logASM << "mov eax, [esp] \r\n";
				logASM << "mov edx, [esp+4] ; ���������� � ��������\r\n";
			}
			logASM << " loopStart" << aluLabels << ": \r\n";
			logASM << "cmp ebx, ecx \r\n";
			logASM << "jg loopEnd" << aluLabels << " \r\n";
			switch(cFlag)
			{
			case DTYPE_DOUBLE:
				logASM << "mov dword [ebx+4], edx \r\n";
				logASM << "mov dword [ebx], eax \r\n";
				break;
			case DTYPE_FLOAT:
				// ����� ��������������� float � ����
				logASM << "fst dword [ebx] \r\n";
				break;
			case DTYPE_LONG:
				logASM << "mov dword [ebx+4], edx \r\n";
				logASM << "mov dword [ebx], eax \r\n";
				break;
			case DTYPE_INT:
				logASM << "mov dword [ebx], eax \r\n";
				break;
			case DTYPE_SHORT:
				logASM << "mov word [ebx], ax \r\n";
				break;
			case DTYPE_CHAR:
				logASM << "mov byte [ebx], al \r\n";
				break;
			}
			logASM << "add ebx, " << typeSizeD[(cFlag>>2)&0x00000007] << " ; ������� ��������� �� ��������� �������\r\n";
			logASM << "jmp loopStart" << aluLabels << " \r\n";
			logASM << "  loopEnd" << aluLabels << ": \r\n";
			if(cFlag == DTYPE_FLOAT)
				logASM << "fstp st0 ; float �� �����\r\n";
			aluLabels++;
			break;
		case cmdGetAddr:
			logASM << "  ; GETADDR\r\n";
			cmdList->GetUINT(pos, valind);
			pos += 4;
			cmdList->GetData(pos, cmdNext);
			if(cmdNext == cmdPush)
			{
				CmdFlag lcFlag;
				cmdList->GetUSHORT(pos+2, lcFlag);
				if((flagAddrAbs(lcFlag) || flagAddrRel(lcFlag)) && flagShiftStk(lcFlag))
					knownEDXOnPush = true;
			}
			if(!knownEDXOnPush)
			{
				if(valind)
				{
					logASM << "lea eax, [ebp + " << (int)valind << "] ; �������� ����� ������������ ���� �����\r\n";
					logASM << "push eax ; �������� ����� � ����\r\n";
				}else{
					logASM << "push ebp ; �������� ����� � ���� (valind == 0)\r\n";
				}
			}else{
				addEBPtoEDXOnPush = true;
				edxValueForPush = (int)valind;
			}
			break;
		case cmdFuncAddr:
		{
			cmdList->GetData(pos, funcInfo);
			pos += sizeof(FunctionInfo*);
			if(!funcInfo)
				throw std::string("ERROR: std function info is invalid");

			if(funcInfo->funcPtr == NULL)
			{
				logASM << "lea eax, [function" << funcInfo->address << " + " << binCodeStart << "] ; ����� ������� \r\n";
				logASM << "push eax ; \r\n";
			}else{
				logASM << "push 0x" << funcInfo->funcPtr << " \r\n";
			}
			break;
		}
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
		{
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;

			//look at the next command
			cmdList->GetData(pos, cmdNext);

			bool skipFstpOnDoubleALU = false;
			if(cmdNext >= cmdAdd && cmdNext <= cmdNEqual)
				skipFstpOnDoubleALU = true;

			switch(cmd + (oFlag << 16))
			{
			case cmdAdd+(OTYPE_DOUBLE<<16):
				logASM << "  ; ADD  double\r\n";
			//	logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "fadd qword [esp] \r\n";
			//	logASM << "faddp \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdAdd+(OTYPE_LONG<<16):
				logASM << "  ; ADD long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "add [esp], eax \r\n";
				logASM << "adc [esp+4], edx \r\n";
				break;
			case cmdAdd+(OTYPE_INT<<16):
				logASM << "  ; ADD int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "add [esp], eax \r\n";
				break;

			case cmdSub+(OTYPE_DOUBLE<<16):
				logASM << "  ; SUB  double\r\n";
			//	logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				if(skipFldESPOnDoubleALU)
					logASM << "fsubr qword [esp] \r\n";
				else
					logASM << "fsub qword [esp] \r\n";
			//	logASM << "fsubrp \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdSub+(OTYPE_LONG<<16):
				logASM << "  ; SUB long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "sub [esp], eax \r\n";
				logASM << "sbb [esp+4], edx \r\n";
				break;
			case cmdSub+(OTYPE_INT<<16):
				logASM << "  ; SUB int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "sub [esp], eax \r\n";
				break;

			case cmdMul+(OTYPE_DOUBLE<<16):
				logASM << "  ; MUL  double\r\n";
			//	logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "fmul qword [esp] \r\n";
			//	logASM << "fmulp \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdMul+(OTYPE_LONG<<16):
				logASM << "  ; MUL long\r\n";
				logASM << "mov ecx, 0x" << longMul << " ; longMul(), result in edx:eax\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdMul+(OTYPE_INT<<16):
				logASM << "  ; MUL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "imul edx \r\n";
				logASM << "push eax \r\n";
				break;

			case cmdDiv+(OTYPE_DOUBLE<<16):
				logASM << "  ; DIV  double\r\n";
			//	logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				if(skipFldESPOnDoubleALU)
					logASM << "fdivr qword [esp] \r\n";
				else
					logASM << "fdiv qword [esp] \r\n";
			//	logASM << "fdivrp \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdDiv+(OTYPE_LONG<<16):
				logASM << "  ; DIV long\r\n";
				logASM << "mov ecx, 0x" << longDiv << " ; longDiv(), result in edx:eax\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdDiv+(OTYPE_INT<<16):
				logASM << "  ; DIV int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp] \r\n";
				logASM << "cdq \r\n";
				logASM << "idiv dword [esp] ; � �������� �� 0?\r\n";
				logASM << "xchg eax, [esp]\r\n";
				break;

			case cmdPow+(OTYPE_DOUBLE<<16):
				logASM << "  ; POW double\r\n";
				logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "mov ecx, 0x" << doublePow << " ; doublePow(), result in st0\r\n";
				logASM << "call ecx \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdPow+(OTYPE_LONG<<16):
				logASM << "  ; MOD long\r\n";
				logASM << "mov ecx, 0x" << longPow << " ; longPow(), result in edx:eax\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdPow+(OTYPE_INT<<16):
				logASM << "  ; POW int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop ebx \r\n";
				logASM << "mov ecx, 0x" << intPow << " ; intPow(), result in edx\r\n";
				logASM << "call ecx \r\n";
				logASM << "push edx \r\n";
				break;

			case cmdMod+(OTYPE_DOUBLE<<16):
				logASM << "  ; MOD  double\r\n";
				logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "fprem \r\n";
				logASM << "fstp st1 \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdMod+(OTYPE_LONG<<16):
				logASM << "  ; MOD long\r\n";
				logASM << "mov ecx, 0x" << longMod << " ; longMod(), result in edx:eax\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdMod+(OTYPE_INT<<16):
				logASM << "  ; MOD int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp] \r\n";
				logASM << "cdq \r\n";
				logASM << "idiv dword [esp] ; � �������� �� 0?\r\n";
				logASM << "xchg edx, [esp]\r\n";
				break;

			case cmdLess+(OTYPE_DOUBLE<<16):
				logASM << "  ; LES double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 41h ; �������� � '������'\r\n";
				logASM << "jne pushZero" << aluLabels << " ; ��, �� ������\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdLess+(OTYPE_LONG<<16):
				logASM << "  ; LES long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jg SetZero" << aluLabels << " \r\n";
				logASM << "jl SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "jae SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdLess+(OTYPE_INT<<16):
				logASM << "  ; LES int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setl cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdGreater+(OTYPE_DOUBLE<<16):
				logASM << "  ; GRT double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 5h ; �������� � '������'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� ������\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdGreater+(OTYPE_LONG<<16):
				logASM << "  ; GRT long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jl SetZero" << aluLabels << " \r\n";
				logASM << "jg SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "jbe SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdGreater+(OTYPE_INT<<16):
				logASM << "  ; GRT int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setg cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdLEqual+(OTYPE_DOUBLE<<16):
				logASM << "  ; LEQL double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 1h ; �������� � '������ ��� �����'\r\n";
				logASM << "jne pushZero" << aluLabels << " ; ��, �� ������ ��� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdLEqual+(OTYPE_LONG<<16):
				logASM << "  ; LEQL long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jg SetZero" << aluLabels << " \r\n";
				logASM << "jl SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "ja SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdLEqual+(OTYPE_INT<<16):
				logASM << "  ; LEQL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setle cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdGEqual+(OTYPE_DOUBLE<<16):
				logASM << "  ; GEQL double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 41h ; �������� � '������ ��� �����'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� ������ ��� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdGEqual+(OTYPE_LONG<<16):
				logASM << "  ; GEQL long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jl SetZero" << aluLabels << " \r\n";
				logASM << "jg SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "jb SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdGEqual+(OTYPE_INT<<16):
				logASM << "  ; GEQL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setge cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdEqual+(OTYPE_DOUBLE<<16):
				logASM << "  ; EQL double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 44h ; �������� � '�����'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdEqual+(OTYPE_LONG<<16):
				logASM << "  ; EQL long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jne SetZero" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "jne SetZero" << aluLabels << " \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdEqual+(OTYPE_INT<<16):
				logASM << "  ; EQL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "sete cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdNEqual+(OTYPE_DOUBLE<<16):
				logASM << "  ; NEQL double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 44h ; �������� � '�������'\r\n";
				logASM << "jnp pushZero" << aluLabels << " ; ��, �� �������\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdNEqual+(OTYPE_LONG<<16):
				logASM << "  ; NEQL long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jne SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "je SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdNEqual+(OTYPE_INT<<16):
				logASM << "  ; NEQL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setne cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdShl+(OTYPE_LONG<<16):
				logASM << "  ; SHL long\r\n";
				logASM << "mov ecx, 0x" << longShl << " ; longShl(), result in [esp+8]\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				break;
			case cmdShl+(OTYPE_INT<<16):
				logASM << "  ; SHL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop ecx \r\n";
				logASM << "xchg ecx, eax \r\n";
				logASM << "sal eax, cl ; \r\n";
				logASM << "push eax \r\n";
				break;

			case cmdShr+(OTYPE_LONG<<16):
				logASM << "  ; SHR long\r\n";
				logASM << "mov ecx, 0x" << longShr << " ; longShr(), result in [esp+8]\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				break;
			case cmdShr+(OTYPE_INT<<16):
				logASM << "  ; SHR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop ecx \r\n";
				logASM << "xchg ecx, eax \r\n";
				logASM << "sar eax, cl ; \r\n";
				logASM << "push eax \r\n";
				break;

			case cmdBitAnd+(OTYPE_LONG<<16):
				logASM << "  ; BAND long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "and [esp], eax ; \r\n";
				logASM << "and [esp+4], edx ; \r\n";
				break;
			case cmdBitAnd+(OTYPE_INT<<16):
				logASM << "  ; BAND int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "and [esp], eax ; \r\n";
				break;

			case cmdBitOr+(OTYPE_LONG<<16):
				logASM << "  ; BOR long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "or [esp], eax ; \r\n";
				logASM << "or [esp+4], edx ; \r\n";
				break;
			case cmdBitOr+(OTYPE_INT<<16):
				logASM << "  ; BOR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "or [esp], eax ; \r\n";
				break;

			case cmdBitXor+(OTYPE_LONG<<16):
				logASM << "  ; BXOR long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "xor [esp], eax ; \r\n";
				logASM << "xor [esp+4], edx ; \r\n";
				break;
			case cmdBitXor+(OTYPE_INT<<16):
				logASM << "  ; BXOR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor [esp], eax ; \r\n";
				break;

			case cmdLogAnd+(OTYPE_LONG<<16):
				logASM << "  ; LAND long\r\n";
				logASM << "mov eax, dword [esp] \r\n";
				logASM << "or eax, dword [esp+4] \r\n";
				logASM << "jz SetZero" << aluLabels << " \r\n";
				logASM << "mov eax, dword [esp+8] \r\n";
				logASM << "or eax, dword [esp+12] \r\n";
				logASM << "jz SetZero" << aluLabels << " \r\n";
				logASM << "mov dword [esp+8], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+8], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+12], 0 \r\n";
				logASM << "add esp, 8 \r\n";
				aluLabels++;
				break;
			case cmdLogAnd+(OTYPE_INT<<16):
				logASM << "  ; LAND int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "cmp eax, 0 \r\n";
				logASM << "je pushZero" << aluLabels << "\r\n";
				logASM << "cmp dword [esp], 0 \r\n";
				logASM << "je pushZero" << aluLabels << "\r\n";
				logASM << "mov dword [esp], 1 ; true\r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 ; false\r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				aluLabels++;
				break;

			case cmdLogOr+(OTYPE_LONG<<16):
				logASM << "  ; LOR long\r\n";
				logASM << "mov eax, dword [esp] \r\n";
				logASM << "or eax, dword [esp+4] \r\n";
				logASM << "jnz SetOne" << aluLabels << " \r\n";
				logASM << "mov eax, dword [esp+8] \r\n";
				logASM << "or eax, dword [esp+12] \r\n";
				logASM << "jnz SetOne" << aluLabels << " \r\n";
				logASM << "mov dword [esp+8], 0 \r\n";
				logASM << "jmp ZeroSet" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+8], 1 \r\n";
				logASM << "  ZeroSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+12], 0 \r\n";
				logASM << "add esp, 8 \r\n";
				aluLabels++;
				break;
			case cmdLogOr+(OTYPE_INT<<16):
				logASM << "  ; LOR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop ebx \r\n";
				logASM << "or eax, ebx \r\n";
				logASM << "cmp eax, 0 \r\n";
				logASM << "je pushZero" << aluLabels << "\r\n";
				logASM << "push 1 ; true\r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "push 0 ; false\r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				aluLabels++;
				break;

			case cmdLogXor+(OTYPE_LONG<<16):
				logASM << "  ; LXOR long\r\n";
				logASM << "xor eax, eax \r\n";
				logASM << "mov ebx, dword [esp] \r\n";
				logASM << "or ebx, dword [esp+4] \r\n";
				logASM << "setnz al \r\n";
				logASM << "xor ecx, ecx \r\n";
				logASM << "mov ebx, dword [esp+8] \r\n";
				logASM << "or ebx, dword [esp+12] \r\n";
				logASM << "setnz cl \r\n";
				logASM << "xor eax, ecx \r\n";
				logASM << "add esp, 8 \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				logASM << "mov dword [esp], eax \r\n";
				break;
			case cmdLogXor+(OTYPE_INT<<16):
				logASM << "  ; LXOR int\r\n";
				logASM << "xor eax, eax \r\n";
				logASM << "cmp dword [esp], 0 \r\n";
				logASM << "setne al \r\n";
				logASM << "xor ecx, ecx \r\n";
				logASM << "cmp dword [esp+4], 0 \r\n";
				logASM << "setne cl \r\n";
				logASM << "xor eax, ecx \r\n";
				logASM << "pop ecx \r\n";
				logASM << "mov dword [esp], eax \r\n";
				break;
			default:
				throw string("Operation is not implemented");
			}
			skipPopEAXOnIntALU = false;
			if(!skipFstpOnDoubleALU)
				skipFldESPOnDoubleALU = false;
		}
		if(cmd >= cmdNeg && cmd <= cmdLogNot)
		{
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			switch(cmd + (oFlag << 16))
			{
			case cmdNeg+(OTYPE_DOUBLE<<16):
				logASM << "  ; NEG double\r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "fchs \r\n";
				logASM << "fstp qword [esp] \r\n";
				break;
			case cmdNeg+(OTYPE_LONG<<16):
				logASM << "  ; NEG long\r\n";
				logASM << "neg dword [esp] \r\n";
				logASM << "adc dword [esp+4], 0 \r\n";
				logASM << "neg dword [esp+4] \r\n";
				break;
			case cmdNeg+(OTYPE_INT<<16):
				logASM << "  ; NEG int\r\n";
				logASM << "neg dword [esp] \r\n";
				break;

			case cmdLogNot+(OTYPE_DOUBLE<<16):
				logASM << "  ; LNOT double\r\n";
				logASM << "fldz \r\n";
				logASM << "fcomp qword [esp] ; ��������\r\n";
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 44h ; �������� � '�����'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fstp qword [esp] \r\n";
				aluLabels++;
				break;
			case cmdLogNot+(OTYPE_LONG<<16):
				logASM << "xor eax, eax \r\n";
				logASM << "mov ebx, dword [esp+4] \r\n";
				logASM << "or ebx, dword [esp] \r\n";
				logASM << "setz al \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				logASM << "mov dword [esp], eax \r\n";
				break;
			case cmdLogNot+(OTYPE_INT<<16):
				logASM << "  ; LNOT int\r\n";
				logASM << "xor eax, eax \r\n";
				logASM << "cmp dword [esp], 0 \r\n";
				logASM << "sete al \r\n";
				logASM << "mov dword [esp], eax \r\n";
				break;

			case cmdBitNot+(OTYPE_LONG<<16):
				logASM << "  ; BNOT long\r\n";
				logASM << "not dword [esp] \r\n";
				logASM << "not dword [esp+4] \r\n";
				break;
			case cmdBitNot+(OTYPE_INT<<16):
				logASM << "  ; BNOT int\r\n";
				logASM << "not dword [esp] \r\n";
				break;
			default:
				throw string("Operation is not implemented");
			}
		}
		if(cmd >= cmdIncAt && cmd <= cmdDecAt)
		{
			int valind = -1, size;

			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			dt = flagDataType(cFlag);

			if(cmd == cmdIncAt)
				logASM << "  ; INCAT ";
			else
				logASM << "  ; DECAT ";
			char *typeNameD[] = { "char", "short", "int", "long", "float", "double" };
			logASM << typeNameD[dt/4] << "\r\n";

			UINT numEDX = 0;
			bool knownEDX = false;
			
			// ���� ������� ����� � �����
			if(flagShiftStk(cFlag))
			{
				cmdList->GetINT(pos, valind);
				pos += 4;
				if(knownEDXOnPush)
				{
					if(mulByVarSize)
						numEDX = edxValueForPush * lastVarSize + valind;
					else
						numEDX = edxValueForPush + valind;
					knownEDX = true;
					knownEDXOnPush = false;
				}else{
					if(skipPopEDXOnPush)
					{
						if(mulByVarSize)
						{
							if(valind != 0)
								logASM << "lea edx, [edx*" << lastVarSize << " + " << valind << "]\r\n";
							else
								logASM << "lea edx, [edx*" << lastVarSize << "]\r\n";
						}else{
							numEDX = valind;
						}
						skipPopEDXOnPush = false;
					}else{
						if(mulByVarSize)
						{
							logASM << "pop eax ; ����� �����\r\n";
							if(valind != 0)
								logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
							else
								logASM << "lea edx, [eax*" << lastVarSize << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
						}else{
							if(valind != 0)
							{
								logASM << "pop edx ; ����� �����\r\n";
								numEDX = valind;
							}else{
								logASM << "pop edx ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
							}
						}
					}
				}
			}else{
				cmdList->GetINT(pos, valind);
				pos += 4;

				knownEDX = true;
				numEDX = valind;
			}

			if(flagSizeOn(cFlag))
			{
				cmdList->GetINT(pos, size);
				pos += 4;
				logASM << "cmp eax, " << (mulByVarSize ? size/lastVarSize : size) << " ; ������� ����� � ������������\r\n";
				logASM << "jb movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
				logASM << "int 3 \r\n";
				logASM << "  movLabel" << movLabels << ":\r\n";
				movLabels++;
			}
			if(flagSizeStk(cFlag))
			{
				logASM << "cmp [esp], eax ; ������� � ������������ ������� � �����\r\n";
				logASM << "ja movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
				logASM << "int 3 \r\n";
				logASM << "  movLabel" << movLabels << ":\r\n";
				logASM << "pop eax ; ������ �������������� ������\r\n";
				movLabels++;
			}
			mulByVarSize = false;

			char *texts[] = { "", "edx + ", "ebp + " };
			char *needEDX = texts[1];
			char *needEBP = texts[2];
			if(knownEDX)
				needEDX = texts[0];
			if(flagAddrAbs(cFlag) && !addEBPtoEDXOnPush)
				needEBP = texts[0];
			addEBPtoEDXOnPush = false;

			UINT final = paramBase+numEDX;
			switch(cmd + (dt << 16))
			{
			case cmdIncAt+(DTYPE_DOUBLE<<16):
				logASM << "fld qword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "fld st0\r\n";
				logASM << "fld1 \r\n";
				logASM << "faddp \r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "fst qword [" << needEDX << needEBP << final << "] ;\r\n";
					logASM << "sub esp, 8\r\n";
					logASM << "fstp qword [esp]\r\n";
				}else{
					logASM << "fstp qword [" << needEDX << needEBP << final << "] ;\r\n";
				}
				if(flagPushBefore(cFlag))
					logASM << "sub esp, 8\r\nfstp qword [esp]\r\n";
				break;
			case cmdIncAt+(DTYPE_FLOAT<<16):
				logASM << "fld dword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "fld st0\r\n";
				logASM << "fld1 \r\n";
				logASM << "faddp \r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "fst dword [" << needEDX << needEBP << final << "] ;\r\n";
					logASM << "sub esp, 8\r\n";
					logASM << "fstp qword [esp]\r\n";
				}else{
					logASM << "fstp dword [" << needEDX << needEBP << final << "] ;\r\n";
				}
				if(flagPushBefore(cFlag))
					logASM << "sub esp, 8\r\nfstp qword [esp]\r\n";
				break;
			case cmdIncAt+(DTYPE_LONG<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "mov edx, dword [" << needEDX << needEBP << final+4 << "] ;\r\n";
				if(flagPushBefore(cFlag))
				{
					logASM << "push edx\r\n";
					logASM << "push eax\r\n";
				}
				logASM << "add eax, 1 \r\n";
				logASM << "adc edx, 0 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				logASM << "mov dword [" << needEDX << needEBP << final+4 << "], edx ;\r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "push edx\r\n";
					logASM << "push eax\r\n";
				}
				break;
			case cmdIncAt+(DTYPE_INT<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			case cmdIncAt+(DTYPE_SHORT<<16):
				logASM << "movsx eax, word [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov word [" << needEDX << needEBP << final << "], ax ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			case cmdIncAt+(DTYPE_CHAR<<16):
				logASM << "movsx eax, byte [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov byte [" << needEDX << needEBP << final << "], al ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;

			case cmdDecAt+(DTYPE_DOUBLE<<16):
				logASM << "fld qword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "fld st0\r\n";
				logASM << "fld1 \r\n";
				logASM << "fsubp \r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "fst qword [" << needEDX << needEBP << final << "] ;\r\n";
					logASM << "sub esp, 8\r\n";
					logASM << "fstp qword [esp]\r\n";
				}else{
					logASM << "fstp qword [" << needEDX << needEBP << final << "] ;\r\n";
				}
				if(flagPushBefore(cFlag))
					logASM << "sub esp, 8\r\nfstp qword [esp]\r\n";
				break;
			case cmdDecAt+(DTYPE_FLOAT<<16):
				logASM << "fld dword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "fld st0\r\n";
				logASM << "fld1 \r\n";
				logASM << "fsubp \r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "fst dword [" << needEDX << needEBP << final << "] ;\r\n";
					logASM << "sub esp, 8\r\n";
					logASM << "fstp qword [esp]\r\n";
				}else{
					logASM << "fstp dword [" << needEDX << needEBP << final << "] ;\r\n";
				}
				if(flagPushBefore(cFlag))
					logASM << "sub esp, 8\r\nfstp qword [esp]\r\n";
				break;
			case cmdDecAt+(DTYPE_LONG<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "mov edx, dword [" << needEDX << needEBP << final+4 << "] ;\r\n";
				if(flagPushBefore(cFlag))
				{
					logASM << "push edx\r\n";
					logASM << "push eax\r\n";
				}
				logASM << "sub eax, 1 \r\n";
				logASM << "sbb edx, 0 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				logASM << "mov dword [" << needEDX << needEBP << final+4 << "], edx ;\r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "push edx\r\n";
					logASM << "push eax\r\n";
				}
				break;
			case cmdDecAt+(DTYPE_INT<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			case cmdDecAt+(DTYPE_SHORT<<16):
				logASM << "movsx eax, word [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov word [" << needEDX << needEBP << final << "], ax ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			case cmdDecAt+(DTYPE_CHAR<<16):
				logASM << "movsx eax, byte [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov byte [" << needEDX << needEBP << final << "], al ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			}
		}
	}

	std::string	logASMstr = logASM.str();

	ofstream noOptFile("asmX86_noopt.txt", std::ios::binary);
	noOptFile << logASMstr;
	noOptFile.flush();
	noOptFile.close();

	DeleteFile("asmX86.txt");
	ofstream m_FileStream("asmX86.txt", std::ios::binary | std::ios::out);
	if(optimize)
	{
		Optimizer_x86 optiMan;
		std::vector<std::string> *optiList = optiMan.Optimize(logASMstr.c_str(), (int)logASMstr.length());

		for(UINT i = 0; i < optiList->size(); i++)
			m_FileStream << (*optiList)[i] << "\r\n";
	}else{
		m_FileStream << logASMstr;
	}
	m_FileStream.flush();
	m_FileStream.close();

	STARTUPINFO stInfo;
	PROCESS_INFORMATION prInfo;

	// Compile using fasm
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.cb = sizeof(stInfo);
	stInfo.dwFlags = STARTF_USESHOWWINDOW;
	stInfo.wShowWindow = SW_HIDE;
	memset(&prInfo, 0, sizeof(prInfo));

	DeleteFile("asmX86.bin");

	if(!CreateProcess(NULL, "fasm.exe asmX86.txt", NULL, NULL, false, 0, NULL, ".\\", &stInfo, &prInfo))
		throw std::string("Failed to create process");

	if(WAIT_TIMEOUT == WaitForSingleObject(prInfo.hProcess, 5000))
		throw std::string("Compilation to x86 binary takes too much time (timeout=5sec)");

	CloseHandle(prInfo.hProcess);
	CloseHandle(prInfo.hThread);

	FILE *fCode = fopen("asmX86.bin", "rb");
	if(!fCode)
		throw std::string("Failed to open output file");
	
	fseek(fCode, 0, SEEK_END);
	UINT size = ftell(fCode);
	fseek(fCode, 0, SEEK_SET);
	if(size > 200000)
		throw std::string("Byte code is too big (size > 200000)");
	fread(binCode+20, 1, size, fCode);
	binCodeSize = size;
}

string ExecutorX86::GetListing()
{
	return logASM.str();
}

string ExecutorX86::GetResult()
{
	ostringstream tempStream;
	long long combined = 0;
	*((int*)(&combined)) = runResult2;
	*((int*)(&combined)+1) = runResult;

	switch(runResultType)
	{
	case OTYPE_DOUBLE:
		tempStream << *((double*)(&combined));
		break;
	case OTYPE_LONG:
		tempStream << combined << 'L';
		break;
	case OTYPE_INT:
		tempStream << runResult;
		break;
	}
	//tempStream << " (" << stackReallocs << " reallocs)";
	return tempStream.str();
}

void ExecutorX86::SetOptimization(int toggle)
{
	optimize = toggle;
}

char* ExecutorX86::GetVariableData()
{
	return paramData;
}

#endif
