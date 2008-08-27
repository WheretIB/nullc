#include "stdafx.h"
#include "Executor_X86.h"
#include "StdLib_X86.h"

ExecutorX86::ExecutorX86(CommandList* cmds, std::vector<VariableInfo>* varinfo)
{
	cmdList = cmds;
	varInfo = varinfo;
	paramData = new char[1000000];
	paramBase = static_cast<UINT>(reinterpret_cast<long long>(paramData));
}
ExecutorX86::~ExecutorX86()
{
	delete[] paramData;
}

int runResult = 0;
int runResult2 = 0;
OperFlag runResultType = OTYPE_DOUBLE;

bool ExecutorX86::Run()
{
	//GenListing();
	*(double*)(paramData) = 0.0;
	*(double*)(paramData+8) = 3.1415926535897932384626433832795;
	*(double*)(paramData+16) = 2.7182818284590452353602874713527;
	
	FILE *fCode = fopen("asmX86.bin", "rb");
	if(!fCode)
		throw std::string("Failed to open output file");
	char binCode[40000];
	fseek(fCode, 0, SEEK_END);
	UINT size = ftell(fCode);
	fseek(fCode, 0, SEEK_SET);
	if(size > 40000)
		throw std::string("Bytecode is too big (size > 40000)");
	fread(binCode, 1, size, fCode);

	typedef void (*codeFunc)();
	codeFunc funcMain = (codeFunc)(&binCode[0]);
	UINT binCodeStart = static_cast<UINT>(reinterpret_cast<long long>(&binCode[0]));

	UINT res1 = 0;
	UINT res2 = 0;
	UINT resT = 0;
	__asm
	{
		pusha ; // �������� ��� ��������
		mov eax, binCodeStart ;

		push ebp; // �������� ���� ����� (� ������� ����������� �� popa)

		mov ebp, 0h ;
		mov edi, 18h ;
		call eax ; // � ebx ��� ������������ ��������

		pop eax; // ����� ������ dword

		cmp ebx, 3 ; // oFlag == 3, ������ int
		je justAnInt ; // ��������� ������ ������ ����� ��� long � double
		pop edx; // ������ ������ dword

		justAnInt:

		pop ebp; // ���������� ���� �����
		mov dword ptr [res1], eax;
		mov dword ptr [res2], edx;
		mov dword ptr [resT], ebx;

		popa ;
	}
	runResult = res1;
	runResult2 = res2;
	runResultType = resT;

	return false;
}
void ExecutorX86::GenListing()
{
	logASM.str("");

	UINT pos = 0, pos2 = 0;
	CmdID	cmd, cmdNext;
	//char	name[512];
	UINT	valind;

	CmdFlag cFlag;
	OperFlag oFlag;
	asmStackType st;//, sdt;
	asmDataType dt;

	vector<int> instrNeedLabel;	// ����� �� ����� ����������� ����� �����
	vector<int> funcNeedLabel;	// ����� �� ����� ����������� ����� �������

	//������, ���� ����� ������
	while(cmdList->GetData(pos, cmd))
	{
		pos2 = pos;
		pos += 2;
		switch(cmd)
		{
		case cmdCallStd:
			break;
		case cmdPushVTop:
			break;
		case cmdPopVTop:
			break;
		case cmdCall:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			funcNeedLabel.push_back(valind);
			break;
		case cmdProlog:
			break;
		case cmdReturn:
			pos += 5;
			break;
		case cmdPushV:
			pos += 4;
			break;
		case cmdNop:
			break;
		case cmdCTI:
			pos += 5;
			break;
		case cmdPush:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				dt = flagDataType(cFlag);

				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag))
					pos += 4;
				if(flagShiftOn(cFlag))
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
			}
			break;
		case cmdMov:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag))
					pos += 4;
				if(flagShiftOn(cFlag))
					pos += 4;
				if(flagSizeOn(cFlag))
					pos += 4;
			}
			break;
		case cmdPop:
			pos += 2;
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
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
			pos += 1;
		if(cmd >= cmdNeg && cmd <= cmdLogNot)
			pos += 1;
		if(cmd >= cmdIncAt && cmd < cmdDecAt)
		{
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;

			if(!flagAddrStk(cFlag))
			{
				if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
					pos += 4;
				if(flagShiftOn(cFlag))
					pos += 4;
				if(flagSizeOn(cFlag))
					pos += 4;
			}
		}
	}

	logASM << "use32\r\n";
	UINT typeSizeD[] = { 1, 2, 4, 8, 4, 8 };

	int pushLabels = 1;
	int movLabels = 1;
	int skipLabels = 1;
	int aluLabels = 1;

	bool skipPopEAXOnIntALU = false;
	UINT lastVarSize = 0;
	bool mulByVarSize = false;

	bool firstProlog = true;

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
				logASM << "  function" << pos << ": \r\n";
				break;
			}
		}

		pos2 = pos;
		pos += 2;
		switch(cmd)
		{
		case cmdCallStd:
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
			logASM << "  ; CALL\r\n";
			cmdList->GetData(pos, &valind, sizeof(UINT));
			pos += sizeof(UINT);
			logASM << "call function" << valind << "\r\n";
			break;
		case cmdProlog:
			logASM << "  ; PROLOG \r\n";
			if(firstProlog)
			{
				logASM << "push ebp ; first part of PUSHT\r\n";
			}else{
				logASM << "mov ebp, edi ; second part of PUSHT\r\n";
				pos += 2; //���������� PUSHT
			}
			firstProlog = !firstProlog;
			break;
		case cmdReturn:
			logASM << "  ; RET\r\n";
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetData(pos, &valind, sizeof(UINT));
			pos += sizeof(UINT);
			if(oFlag == OTYPE_DOUBLE || oFlag == OTYPE_LONG)
			{
				logASM << "pop edx \r\n";
				logASM << "pop eax ; �� ����� �������� double � long � ��������\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "pop eax ; �� ����� �������� int � �������\r\n";
			}
			logASM << "pop ebx ; ��������� eip\r\n";
			for(UINT pops = 0; pops < valind; pops++)
			{
				logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
				logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
			}
			
			if(oFlag == OTYPE_DOUBLE || oFlag == OTYPE_LONG)
			{
				logASM << "push eax ; \r\n";
				logASM << "push edx ; �������� ����� ������� � ����\r\n";
				logASM << "push ebx ; �������� eip ������� � ����\r\n";
				/*logASM << "xchg [esp], eax ; �������� ����� ����� � eip\r\n";
				logASM << "push edx ; �������� ����� ������� � ����\r\n";
				logASM << "push ebx ; �������� eip ������� � ����\r\n";*/
			}else if(oFlag == OTYPE_INT){
				logASM << "push eax ; �������� ����� ������� � ����\r\n";
				logASM << "push ebx ; �������� eip ������� � ����\r\n";
				/*logASM << "xchg [esp], eax ; �������� ����� � eip\r\n";
				logASM << "push eax ; �������� eip ������� � ����\r\n";*/
			}

			logASM << "mov ebx, " << (UINT)(oFlag) << " ; �������� oFlag ����� ������� �����, ����� ��� ��������\r\n";
			logASM << "ret ; ������������ �� �������\r\n";
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
			logASM << "  ; CTI\r\n";
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
			if(valind != 1)
			{
				if(valind == 2 || valind == 4 || valind == 8)
				{
					mulByVarSize = true;
					lastVarSize = valind;
				}else{
					logASM << "pop eax ; ������ � eax\r\n";
					logASM << "imul eax, " << valind << " ; ������� ����� �� ������ ����������\r\n";
					logASM << "push eax \r\n";
				}
			}
			break;
		case cmdPush:
			{
				logASM << "  ; PUSH\r\n";
				int valind = -1, shift, size;
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
				if(flagAddrAbs(cFlag))
					needEBP = texts[0];
				UINT numEDX = 0;
				bool knownShift = false;

				// ���� �������� �� ���������� �...
				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ...���� ����� �� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(mulByVarSize)
						shift *= lastVarSize;
					mulByVarSize = false;
					valind += shift;
					knownShift = true;

					needEDX = texts[0];
					numEDX = valind;
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ...���� ����� �� ����� � �����, � ������� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << "pop eax ; ����� �����\r\n";
					if(mulByVarSize)
					{
						if(valind != 0)
							logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
						else
							logASM << "lea edx, [eax*" << lastVarSize << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
					}else{
						if(valind != 0)
							logASM << "lea edx, [eax + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
						else
							logASM << "mov edx, eax ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
					}
					mulByVarSize = false;
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ...���� ����� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(mulByVarSize)
						shift *= lastVarSize;
					mulByVarSize = false;
					logASM << "pop eax ; ����� �����\r\n";
					if(shift != 0)
						logASM << "lea edx, [eax + " << shift << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
					else
						logASM << "mov edx, eax ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: shift==0)\r\n";
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ...���� ����� ����� � �����, � ������� ����� � �����
					logASM << "pop eax ; ����� �����\r\n";
					logASM << "pop ebx ; ����� �����\r\n";
					if(mulByVarSize)
						logASM << "lea edx, [ebx*" << lastVarSize << " + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � �����\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � �����\r\n";
					mulByVarSize = false;
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag))
				{
					// ...���� ����� �� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					knownShift = true;

					needEDX = texts[0];
					numEDX = valind;
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagAddrStk(cFlag))
				{
					// ...���� ����� ����� � �����
					logASM << "pop eax ; ����� �����\r\n";
					logASM << "mov edx, eax ; ����� ��������� �� ���� ����������\r\n";
				}

				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					if(knownShift)
					{
						if(shift < 0)
							throw std::string("ERROR: array index out of bounds (negative)");
						if(shift > size)
							throw std::string("ERROR: array index out of bounds (overflow)");
					}else{
						logASM << "cmp eax, " << size << " ; ������� ����� � ������������\r\n";
						logASM << "jb pushLabel" << pushLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
						logASM << "mov esi, [esi+4] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
						logASM << "call esi ; ������� �\r\n";
						logASM << "  pushLabel" << pushLabels << ":\r\n";
						pushLabels++;
					}
				}
				if(flagSizeStk(cFlag))
				{
					if(knownShift)
						logASM << "cmp [esp], " << shift << " ; ������� � ������������ ������� � �����\r\n";
					else
						logASM << "cmp [esp], eax ; ������� � ������������ ������� � �����\r\n";
					logASM << "ja pushLabel" << pushLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "mov esi, [esi+4] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
					logASM << "call esi ; ������� �\r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					logASM << "pop eax ; ������ �������������� ������\r\n";
					pushLabels++;
				}

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

						cmdList->GetUINT(pos, lowDW); pos += 4;
						logASM << needPush << lowDW << " ; �������� int\r\n";
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
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						logASM << "push dword [" << needEDX << needEBP << paramBase+4+numEDX << "]\r\n";
						logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� double ��� long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						logASM << "push eax ; \r\n";
						logASM << "push eax ; ��������� ����� ��� double\r\n";
						logASM << "fld dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� float � fpu ����\r\n";
						logASM << "fstp qword [esp] ; �������� double � ������� ����\r\n";
					}
					if(dt == DTYPE_INT)
					{
						//look at the next command
						cmdList->GetData(pos, cmdNext);
						if(cmdNext >= cmdAdd && cmdNext <= cmdLogOr) // for binary commands except LogicalXOR
						{
							needPush = texts[4];
							skipPopEAXOnIntALU = true;
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
				int valind = -1, shift, size;
				UINT	highDW = 0, lowDW = 0;
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				st = flagStackType(cFlag);
				dt = flagDataType(cFlag);

				if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
				{
					// ��������� �����, �� �������!
					if(st == STYPE_DOUBLE || st == STYPE_LONG)
					{
						logASM << "pop ebx ; \r\n";
						logASM << "pop ecx ; ������� double ��� long long � �������� \r\n";
					}else if(st == STYPE_INT)
					{
						logASM << "pop ebx ; ������� int � �������\r\n";
					}
				}

				UINT numEDX = 0;
				bool knownEDX = false;
				bool knownShift = false;

				// ����...
				if(!flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ...���� ����� �� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(mulByVarSize)
						shift *= lastVarSize;
					mulByVarSize = false;
					valind += shift;

					knownEDX = true;
					numEDX = valind;
					knownShift = true;
				}else if(!flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ...���� ����� �� ����� � �����, � ������� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << "pop eax ; ����� �����\r\n";
					if(mulByVarSize)
					{
						if(valind != 0)
							logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
						else
							logASM << "lea edx, [eax*" << lastVarSize << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
					}else{
						if(valind != 0)
							logASM << "lea edx, [eax + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
						else
							logASM << "mov edx, eax ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
					}
					mulByVarSize = false;
				}else if(flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ...���� ����� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(mulByVarSize)
						shift *= lastVarSize;
					mulByVarSize = false;
					knownShift = true;

					logASM << "pop eax ; ����� �����\r\n";
					if(shift != 0)
						logASM << "lea edx, [eax + " << shift << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
					else
						logASM << "mov edx, eax ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: shift==0)\r\n";
				}else if(flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ...���� ����� ����� � �����, � ������� ����� � �����
					logASM << "pop eax ; ����� �����\r\n";
					logASM << "pop ebx ; ����� �����\r\n";
					if(mulByVarSize)
						logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � �����\r\n";
					else
						logASM << "lea edx, [ebx*" << lastVarSize << " + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � �����\r\n";
					mulByVarSize = false;
				}else if(!flagAddrStk(cFlag))
				{
					// ...���� ����� �� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;

					knownShift = true;
					knownEDX = true;
					numEDX = valind;
				}else if(flagAddrStk(cFlag))
				{
					// ...���� ����� ����� � �����
					logASM << "pop eax ; ����� �����\r\n";
					logASM << "mov edx, eax ; ����� ��������� �� ���� ����������\r\n";
				}

				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					if(knownShift)
					{
						if(shift < 0)
							throw std::string("ERROR: array index out of bounds (negative)");
						if(shift > size)
							throw std::string("ERROR: array index out of bounds (overflow)");
					}else{
						logASM << "cmp eax, " << size << " ; ������� ����� � ������������\r\n";
						logASM << "jb movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
						logASM << "mov esi, [esi+4] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
						logASM << "call esi ; ������� �\r\n";
						logASM << "  movLabel" << movLabels << ":\r\n";
						movLabels++;
					}
				}
				if(flagSizeStk(cFlag))
				{
					if(knownShift)
						logASM << "cmp [esp], " << shift << " ; ������� � ������������ ������� � �����\r\n";
					else
						logASM << "cmp [esp], eax ; ������� � ������������ ������� � �����\r\n";
					logASM << "ja movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "mov esi, [esi+4] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
					logASM << "call esi ; ������� �\r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					logASM << "pop eax ; ������ �������������� ������\r\n";
					movLabels++;
				}

				char *texts[] = { "", "edx + ", "ebp + " };
				char *needEDX = texts[1];
				char *needEBP = texts[2];
				if(knownEDX)
					needEDX = texts[0];
				if(flagAddrAbs(cFlag))
					needEBP = texts[0];

				UINT final = paramBase+numEDX;//-varSize;
				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
					{
						logASM << "mov [" << needEDX << needEBP << final << "], ebx \r\n";
						logASM << "mov [" << needEDX << needEBP << final+4 << "], ecx ; ��������� double ��� long long ����������.\r\n";
						logASM << "push ecx \r\n";
						logASM << "push ebx ; �������� �������� � �����, ��� ����\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov ecx, [esp+4] \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx \r\n";
						logASM << "mov [" << needEDX << needEBP << final+4 << "], ecx ; ��������� double ��� long long ����������.\r\n";
					}
				}
				if(dt == DTYPE_FLOAT)
				{
					if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
						logASM << "push ebx \r\n";
					logASM << "fld qword [esp] ; �������� double �� ����� � fpu ����\r\n";
					logASM << "fstp dword [" << needEDX << needEBP << final << "] ; ��������� float ����������\r\n";
				}
				if(dt == DTYPE_INT)
				{
					if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
					{
						logASM << "push ebx \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx ; ��������� int ����������\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx ; ��������� int ����������\r\n";
					}
				}
				if(dt == DTYPE_SHORT)
				{
					if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
					{
						logASM << "push ebx \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; ��������� short ����������\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; ��������� short ����������\r\n";
					}
				}
				if(dt == DTYPE_CHAR)
				{
					if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
					{
						logASM << "push ebx \r\n";
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
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			st = flagStackType(cFlag);
			if(st == STYPE_DOUBLE || st == STYPE_LONG)
			{
				logASM << "pop eax\r\n";
				logASM << "pop eax ; ������ double ��� long\r\n";
			}else{
				logASM << "pop eax ; ������ int\r\n";
			}
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
					logASM << "mov eax, dword [esi+8] ; ��������� �� ������� doubletolong\r\n";
					logASM << "call eax ; �������. ������ ������ 32���� ���������� - eax, ������� - edx\r\n";
					logASM << "pop ebx \r\n";
					logASM << "pop ebx ; ������ double �� �����\r\n";
					logASM << "push eax ; ������� int � ����\r\n";
				}else if(st == STYPE_DOUBLE && dt == DTYPE_LONG){
					logASM << "mov eax, dword [esi+8] ; ��������� �� ������� doubletolong\r\n";
					logASM << "call eax ; �������. ������ ������ 32���� ���������� - eax, ������� - edx\r\n";
					logASM << "xchg eax, [esp] \r\n";
					logASM << "xchg edx, [esp-4] ; ��������� long long � �����\r\n";
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
			logASM << "jmp gLabel" << valind << "\r\n";
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
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
		{
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			
			switch(cmd + (oFlag << 16))
			{
			case cmdAdd+(OTYPE_DOUBLE<<16):
				logASM << "  ; ADD  double\r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "fld qword [esp+8] \r\n";
				logASM << "faddp \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
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
				logASM << "fld qword [esp] \r\n";
				logASM << "fld qword [esp+8] \r\n";
				logASM << "fsubrp \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
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
				logASM << "fld qword [esp] \r\n";
				logASM << "fld qword [esp+8] \r\n";
				logASM << "fmulp \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
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
				logASM << "fld qword [esp] \r\n";
				logASM << "fld qword [esp+8] \r\n";
				logASM << "fdivrp \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
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
				logASM << "fld qword [esp+8] ; ���������� � �������, ��� ���������� ��������\r\n";
				logASM << "mov ecx, 0x" << doublePow << " ; doublePow(), result in st0\r\n";
				logASM << "call ecx \r\n";
				logASM << "fstp qword [esp+8] ; �������� ���������\r\n";
				logASM << "add esp, 8\r\n";
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
				logASM << "fld qword [esp+8] \r\n";
				logASM << "fprem \r\n";
				logASM << "fstp st1 \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
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
				logASM << "fld qword [esp] \r\n";
				logASM << "fcomp qword [esp+8] ; ��������\r\n";
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 41h ; �������� � '������'\r\n";
				logASM << "jne pushZero" << aluLabels << " ; ��, �� ������\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
				aluLabels++;
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
				logASM << "fld qword [esp] \r\n";
				logASM << "fcomp qword [esp+8] ; ��������\r\n";
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 5h ; �������� � '������'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� ������\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
				aluLabels++;
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
				logASM << "fld qword [esp] \r\n";
				logASM << "fcomp qword [esp+8] ; ��������\r\n";
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 1h ; �������� � '������ ��� �����'\r\n";
				logASM << "jne pushZero" << aluLabels << " ; ��, �� ������ ��� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
				aluLabels++;
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
				logASM << "fld qword [esp] \r\n";
				logASM << "fcomp qword [esp+8] ; ��������\r\n";
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 41h ; �������� � '������ ��� �����'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� ������ ��� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
				aluLabels++;
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
				logASM << "fld qword [esp] \r\n";
				logASM << "fcomp qword [esp+8] ; ��������\r\n";
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 44h ; �������� � '�����'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
				aluLabels++;
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
				logASM << "fld qword [esp] \r\n";
				logASM << "fcomp qword [esp+8] ; ��������\r\n";
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 44h ; �������� � '�������'\r\n";
				logASM << "jnp pushZero" << aluLabels << " ; ��, �� �������\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fstp qword [esp+8] \r\n";
				logASM << "add esp, 8\r\n";
				aluLabels++;
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
				logASM << "pop edx \r\n";
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
			int valind = -1, shift, size;
			UINT	highDW = 0, lowDW = 0;
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			dt = flagDataType(cFlag);

			UINT numEDX = 0;
			bool knownEDX = false;
			bool knownShift = false;

			// ����...
			if(!flagAddrStk(cFlag) && flagShiftOn(cFlag))
			{
				// ...���� ����� �� ����� � �����, � ������� ����� � �������
				cmdList->GetINT(pos, valind);
				pos += 4;
				cmdList->GetINT(pos, shift);
				pos += 4;
				if(mulByVarSize)
					shift *= lastVarSize;
				mulByVarSize = false;
				valind += shift;

				knownEDX = true;
				numEDX = valind;
				knownShift = true;
			}else if(!flagAddrStk(cFlag) && flagShiftStk(cFlag))
			{
				// ...���� ����� �� ����� � �����, � ������� ����� � �����
				cmdList->GetINT(pos, valind);
				pos += 4;
				logASM << "pop eax ; ����� �����\r\n";
				if(mulByVarSize)
				{
					if(valind != 0)
						logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
					else
						logASM << "lea edx, [eax*" << lastVarSize << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
				}else{
					if(valind != 0)
						logASM << "lea edx, [eax + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
					else
						logASM << "mov edx, eax ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
				}
				mulByVarSize = false;
			}else if(flagAddrStk(cFlag) && flagShiftOn(cFlag))
			{
				// ...���� ����� ����� � �����, � ������� ����� � �������
				cmdList->GetINT(pos, shift);
				pos += 4;
				if(mulByVarSize)
					shift *= lastVarSize;
				mulByVarSize = false;
				knownShift = true;

				logASM << "pop eax ; ����� �����\r\n";
				if(shift != 0)
					logASM << "lea edx, [eax + " << shift << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
				else
					logASM << "mov edx, eax ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: shift==0)\r\n";
			}else if(flagAddrStk(cFlag) && flagShiftStk(cFlag))
			{
				// ...���� ����� ����� � �����, � ������� ����� � �����
				logASM << "pop eax ; ����� �����\r\n";
				logASM << "pop ebx ; ����� �����\r\n";
				if(mulByVarSize)
					logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � �����\r\n";
				else
					logASM << "lea edx, [ebx*" << lastVarSize << " + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � �����\r\n";
				mulByVarSize = false;
			}else if(!flagAddrStk(cFlag))
			{
				// ...���� ����� �� ����� � �����
				cmdList->GetINT(pos, valind);
				pos += 4;

				knownShift = true;
				knownEDX = true;
				numEDX = valind;
			}else if(flagAddrStk(cFlag))
			{
				// ...���� ����� ����� � �����
				logASM << "pop eax ; ����� �����\r\n";
				logASM << "mov edx, eax ; ����� ��������� �� ���� ����������\r\n";
			}

			if(flagSizeOn(cFlag))
			{
				cmdList->GetINT(pos, size);
				pos += 4;
				if(knownShift)
				{
					if(shift < 0)
						throw std::string("ERROR: array index out of bounds (negative)");
					if(shift > size)
						throw std::string("ERROR: array index out of bounds (overflow)");
				}else{
					logASM << "cmp eax, " << size << " ; ������� ����� � ������������\r\n";
					logASM << "jb movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "mov esi, [esi+4] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
					logASM << "call esi ; ������� �\r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					movLabels++;
				}
			}
			if(flagSizeStk(cFlag))
			{
				if(knownShift)
					logASM << "cmp [esp], " << shift << " ; ������� � ������������ ������� � �����\r\n";
				else
					logASM << "cmp [esp], eax ; ������� � ������������ ������� � �����\r\n";
				logASM << "ja movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
				logASM << "mov esi, [esi+4] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
				logASM << "call esi ; ������� �\r\n";
				logASM << "  movLabel" << movLabels << ":\r\n";
				logASM << "pop eax ; ������ �������������� ������\r\n";
				movLabels++;
			}

			char *texts[] = { "", "edx + ", "ebp + " };
			char *needEDX = texts[1];
			char *needEBP = texts[2];
			if(knownEDX)
				needEDX = texts[0];
			if(flagAddrAbs(cFlag))
				needEBP = texts[0];

			UINT final = paramBase+numEDX;
			switch(cmd + (dt << 16))
			{
			case cmdIncAt+(DTYPE_DOUBLE<<16):
				logASM << "  ; INCAT double\r\n";
				logASM << "fld qword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "fld1 \r\n";
				logASM << "faddp \r\n";
				logASM << "fstp qword [" << needEDX << needEBP << final << "] ;\r\n";
				break;
			case cmdIncAt+(DTYPE_FLOAT<<16):
				logASM << "  ; INCAT float\r\n";
				logASM << "fld dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "fld1 \r\n";
				logASM << "faddp \r\n";
				logASM << "fstp dword [" << needEDX << needEBP << final << "] ;\r\n";
				break;
			case cmdIncAt+(DTYPE_LONG<<16):
				logASM << "  ; INCAT long\r\n";
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "mov edx, dword [" << needEDX << needEBP << final+4 << "] ;\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "adc edx, 0 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				logASM << "mov dword [" << needEDX << needEBP << final+4 << "], edx ;\r\n";
				break;
			case cmdIncAt+(DTYPE_INT<<16):
				logASM << "  ; INCAT int\r\n";
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				break;
			case cmdIncAt+(DTYPE_SHORT<<16):
				logASM << "  ; INCAT short\r\n";
				logASM << "movsx eax, word [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov word [" << needEDX << needEBP << final << "], ax ;\r\n";
				break;
			case cmdIncAt+(DTYPE_CHAR<<16):
				logASM << "  ; INCAT char\r\n";
				logASM << "movsx eax, byte [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov byte [" << needEDX << needEBP << final << "], al ;\r\n";
				break;

			case cmdDecAt+(DTYPE_DOUBLE<<16):
				logASM << "  ; DECAT double\r\n";
				logASM << "fld qword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "fld1 \r\n";
				logASM << "fsubp \r\n";
				logASM << "fstp qword [" << needEDX << needEBP << final << "] ;\r\n";
				break;
			case cmdDecAt+(DTYPE_FLOAT<<16):
				logASM << "  ; DECAT float\r\n";
				logASM << "fld dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "fld1 \r\n";
				logASM << "fsubp \r\n";
				logASM << "fstp dword [" << needEDX << needEBP << final << "] ;\r\n";
				break;
			case cmdDecAt+(DTYPE_LONG<<16):
				logASM << "  ; DECAT long\r\n";
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "mov edx, dword [" << needEDX << needEBP << final+4 << "] ;\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "sbb edx, 0 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				logASM << "mov dword [" << needEDX << needEBP << final+4 << "], edx ;\r\n";
				break;
			case cmdDecAt+(DTYPE_INT<<16):
				logASM << "  ; DECAT int\r\n";
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				break;
			case cmdDecAt+(DTYPE_SHORT<<16):
				logASM << "  ; DECAT short\r\n";
				logASM << "movsx eax, word [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov word [" << needEDX << needEBP << final << "], ax ;\r\n";
				break;
			case cmdDecAt+(DTYPE_CHAR<<16):
				logASM << "  ; DECAT char\r\n";
				logASM << "movsx eax, byte [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov byte [" << needEDX << needEBP << final << "], al ;\r\n";
				break;
			}
		}
	}

	ofstream m_FileStream("asmX86.txt", std::ios::binary);
	m_FileStream << logASM.str();
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
}
string ExecutorX86::GetListing()
{
	return logASM.str();
}

string ExecutorX86::GetResult()
{
	ostringstream tempStream;
	long long combined = 0;
	*((int*)(&combined)) = runResult;
	*((int*)(&combined)+1) = runResult2;

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
	return tempStream.str();
}

bool ExecutorX86::GetSimpleTypeInfo(ostringstream &varstr, TypeInfo* type, int address)
{
	if(type->type == TypeInfo::POD_INT)
	{
		varstr << *((int*)&paramData[address]);
	}else if(type->type == TypeInfo::POD_SHORT)
	{
		varstr << *((short*)&paramData[address]);
	}else if(type->type == TypeInfo::POD_CHAR)
	{
		varstr << "'" << *((unsigned char*)&paramData[address]) << "' (" << (int)(*((unsigned char*)&paramData[address])) << ")";
	}else if(type->type == TypeInfo::POD_FLOAT)
	{
		varstr << *((float*)&paramData[address]);
	}else if(type->type == TypeInfo::POD_LONG)
	{
		varstr << *((long long*)&paramData[address]);
	}else if(type->type == TypeInfo::POD_DOUBLE)
	{
		varstr << *((double*)&paramData[address]);
	}else{
		return false;
	}
	return true;
}

void ExecutorX86::GetComplexTypeInfo(ostringstream &varstr, TypeInfo* type, int address)
{
	for(UINT mn = 0; mn < type->memberData.size(); mn++)
	{
		varstr << "  " << type->memberData[mn].type->name << " " << type->memberData[mn].name << " = ";
		if(type->memberData[mn].type->type == TypeInfo::POD_VOID)
		{
			varstr << "ERROR: This type is void";
		}else if(type->memberData[mn].type->type == TypeInfo::NOT_POD)
		{
			varstr << "\r\n";
			GetComplexTypeInfo(varstr, type->memberData[mn].type, address+type->memberData[mn].offset);
		}else{
			if(!GetSimpleTypeInfo(varstr, type->memberData[mn].type, address+type->memberData[mn].offset))
				throw std::string("Executor::GetComplexTypeInfo() ERROR: unknown type of variable ") + type->memberData[mn].name;
		}
		varstr << "\r\n";
	}
}

string ExecutorX86::GetVarInfo()
{
	ostringstream varstr;
	std::vector<VariableInfo>&	varInfo = *this->varInfo;
	UINT address = 0;
	for(UINT i = 0; i < varInfo.size(); i++)
	{
		for(UINT n = 0; n < varInfo[i].count; n++)
		{
			varstr << address << ":" << (varInfo[i].isConst ? "const " : "") << varInfo[i].varType->name << (varInfo[i].isRef ? "ref " : " ") << varInfo[i].name;

			if(varInfo[i].count != 1)
				varstr << "[" << n << "]";
			varstr << " = ";
			if(varInfo[i].varType->type == TypeInfo::POD_VOID)
			{
				varstr << "ERROR: This type is void";
			}else if(varInfo[i].varType->type == TypeInfo::NOT_POD)
			{
				varstr << "" << "\r\n";
				GetComplexTypeInfo(varstr, varInfo[i].varType, address);
			}else{
				if(!GetSimpleTypeInfo(varstr, varInfo[i].varType, address))
					throw std::string("Executor::GetVarInfo() ERROR: unknown type of variable ") + varInfo[i].name;
				varstr << "\r\n";
			}
			address += varInfo[i].varType->size;
		}
	}
	return varstr.str();
}