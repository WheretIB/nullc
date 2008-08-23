#include "stdafx.h"
#include "Executor_X86.h"

ExecutorX86::ExecutorX86(CommandList* cmds, std::vector<VariableInfo>* varinfo)
{
	cmdList = cmds;
	varInfo = varinfo;
}
ExecutorX86::~ExecutorX86()
{

}

bool ExecutorX86::Run()
{
	GenListing();
	
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

	char *data = new char[1000000];
	UINT res1 = 0;
	__asm
	{
		pusha ;
		mov eax, binCodeStart ;
		mov ebx, data ;
		mov ecx, 0h ;
		mov edi, 18h ;
		call eax ;
		mov dword ptr [res1], eax;
		//push 0h;
		popa ;
	}
	delete[] data;

	return false;
}
void ExecutorX86::GenListing()
{
	logASM.str("");

	UINT pos = 0, pos2 = 0;
	CmdID	cmd;
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
		//logASM << "\r\n";
	}

	logASM << "use32\r\n";
	UINT typeSizeD[] = { 1, 2, 4, 8, 4, 8 };

	//logASM << "pop eax ; ����� ����� �������� � eax\r\n";
	//logASM << "mov [ebx], eax ; �������� � ��������\r\n";

	int pushLabels = 1;
	int movLabels = 1;
	int skipLabels = 1;
	int aluLabels = 1;

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
		//xchg al,[char]
		pos2 = pos;
		pos += 2;
		switch(cmd)
		{
		case cmdCallStd:
			break;
		case cmdPushVTop:
			logASM << "  ; PUSHT\r\n";
			logASM << "push ecx ; ��������� ������� ���� ����� ����������\r\n";
			logASM << "mov ecx, edi ; ���������� ����� ���� ����� ����������, �� ������� �����\r\n";
			break;
		case cmdPopVTop:
			logASM << "  ; POPT\r\n";
			logASM << "mov edi, ecx ; ������������ ���������� ������ ����� ����������\r\n";
			logASM << "pop ecx ; ������������ ���������� ���� ����� ����������\r\n";
			break;
		case cmdCall:
			logASM << "  ; CALL\r\n";
			cmdList->GetData(pos, &valind, sizeof(UINT));
			pos += sizeof(UINT);
			logASM << "call function" << valind << "\r\n";
			break;
		case cmdProlog:
			logASM << "xchg eax, [esp]\r\n";
			logASM << "push eax ; ����� �������, eip ������� �� �������, �� ��� ��� ������ ���������� eax �� �������\r\n";
			//logASM << "xchg eax, [ebx] ; ����� �������, eip � [ebx], � ���������� �������� � �����\r\n";
			break;
		case cmdReturn:
			logASM << "  ; RET\r\n";
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetData(pos, &valind, sizeof(UINT));
			pos += sizeof(UINT);
			if(oFlag == OTYPE_DOUBLE || oFlag == OTYPE_LONG)
			{
				logASM << "mov edx, eax \r\n";
				logASM << "pop eax ; ������ ��� ����� ��������� ��� � eax � edx\r\n";
			}
			logASM << "pop dword [ebx+4h] ; ������ �������� ��������\r\n";
			//if(oFlag == OTYPE_INT)
			//	logASM << "pop eax \r\n";
			for(UINT pops = 0; pops < valind; pops++)
			{
				logASM << "mov edi, ecx ; ������������ ���������� ������ ����� ����������\r\n";
				logASM << "pop ecx ; ������������ ���������� ���� ����� ����������\r\n";
			}
			if(oFlag == OTYPE_DOUBLE || oFlag == OTYPE_LONG)
			{
				logASM << "xchg edx, [esp] \r\n";
				logASM << "push edx ; ���, ������ ��������� ����� �� ������� � � eax\r\n";
			}
			//logASM << "push edx ; ��, ������ ��������� ��-���� ����� � ����� � eax\r\n";
			//logASM << "push dword [ebx] ;\r\n";
			logASM << "ret ; ������������ �� �������\r\n";
			break;
		case cmdPushV:
			logASM << "  ; PUSHV\r\n";
			cmdList->GetData(pos, &valind, sizeof(int));
			pos += sizeof(int);
			logASM << "add edi, " << pos << " ; �������� ����� ��� ����� ���������� � �����\r\n";
			break;
		case cmdNop:
			logASM << "  ; NOP\r\n";
			logASM << "nop\r\n";
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
				logASM << "push eax ; ������� double ������� � ����\r\n";
				logASM << "mov eax, dword [esi+8] ; ��������� �� ������� doubletolong\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop eax ; ������ double �� �����\r\n";
				logASM << "call eax ; �������. ������ ������ 32���� ���������� - eax, ������� - edx\r\n";
				break;
			case OTYPE_LONG:
				logASM << "pop edx ; ������ ������� ���� long �� �����\r\n";
				break;
			case OTYPE_INT:
				break;
			}
			logASM << "imul eax, 0" << hex << valind << dec << "h ;������� ����� �� ������ ����������\r\n";
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

				// �� ��� �������� �� ����� ����� �����������...
				if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ���� ���������� ���������, ����� �� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					if(valind != 0)
						logASM << "lea edx, [ebx + 0" << hex << valind << dec << "h] ; ����� ��������� �� ���� ���������� � ������� �� ������������ ������\r\n";
					else
						logASM << "mov edx, ebx ; ����� ��������� �� ���� ���������� (opt: addr+shift==0)\r\n";
				}else if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ���� ���������� ���������, ����� �� ����� � �����, � ������� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << valind << dec << "h] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ���� ���������� ���������, ����� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(shift != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << shift << dec << "h] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: shift==0)\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ���� ���������� ���������, ����� ����� � �����, � ������� ����� � �����
					logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
					logASM << "add edx, eax ; ������� �� ��� ���� ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ���� ������������� ���������, ����� �� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					if(valind != 0)
						logASM << "lea edx, [ebx + ecx + 0" << hex << valind << dec << "h] ; ����� ��������� �� ���� ���������� � ������� �� ���� ����� ���������� � �� ������������ ������\r\n";
					else
						logASM << "lea edx, [ebx + ecx] ; ����� ��������� �� ���� ���������� � ������� �� ���� ����� ����������(opt: addr+shift==0)\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ���� ������������� ���������, ����� �� ����� � �����, � ������� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << valind << dec << "h] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
					logASM << "add edx, ecx ; ������� �� ���� ����� ����������";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ���� ������������� ���������, ����� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(shift != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << shift << dec << "h] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: shift==0)\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
					logASM << "add edx, ecx ; ������� �� ���� ����� ����������";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ���� ������������� ���������, ����� ����� � �����, � ������� ����� � �����
					logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
					logASM << "add edx, eax ; ������� �� ��� ���� ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
					logASM << "add edx, ecx ; ������� �� ���� ����� ����������";
				}else if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag))
				{
					// ���� ���������� ��������� � ����� �� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + 0" << hex << valind << dec << "h] ; ����� ��������� �� ���� ���������� � ������� �� ������������ ������\r\n";
					else
						logASM << "mov edx, ebx ; ����� ��������� �� ���� ���������� (opt: addr==0)\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag))
				{
					// ���� ���������� ��������� � ����� ����� � �����
					logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ����������\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag))
				{
					// ���� ������������� ��������� � ����� �� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + ecx + 0" << hex << valind << dec << "h] ; ����� ��������� �� ���� ���������� � ������� �� ���� ����� ���������� � �� ������������ ������\r\n";
					else
						logASM << "lea edx, [ebx + ecx] ; ����� ��������� �� ���� ���������� � ������� �� ���� ����� ����������(opt: addr+shift==0)\r\n";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag))
				{
					// ���� ������������� ��������� � ����� ����� � �����
					logASM << "lea edx, [ebx + eax] ; ����� ��������� �� ���� ���������� � ������� �� ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
					logASM << "add edx, ecx ; ������� �� ���� ����� ����������";
				}
				
				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					logASM << "cmp edx, 0" << hex << size << dec << "h ; ������� � ������������ �������\r\n";
					logASM << "jb pushLabel" << pushLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "mov esi, [esi+4h] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
					logASM << "call esi ; ������� �\r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					pushLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp edx, eax ; ������� � ������������ ������� � �����\r\n";
					logASM << "jb pushLabel" << pushLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "mov esi, [esi+4h] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
					logASM << "call esi ; ������� �\r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					pushLabels++;
				}
				logASM << "push eax ; ��������� ������� ��� ����� ��������\r\n";
				if(flagNoAddr(cFlag))
				{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						cmdList->GetUINT(pos, highDW); pos += 4;
						cmdList->GetUINT(pos, lowDW); pos += 4;
						logASM << "mov eax, 0" << hex << highDW << dec << "h\r\n";
						logASM << "push eax\r\n";
						logASM << "mov eax, 0" << hex << lowDW << dec << "h ; �������� double ��� long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						// ����� ����� ��� double
						cmdList->GetUINT(pos, lowDW); pos += 4;
						double res = (double)*((float*)(&lowDW));
						logASM << "mov eax, 0" << hex << *((UINT*)(&res)) << dec << "h\r\n";
						logASM << "push eax\r\n";
						logASM << "mov eax, 0" << hex << *((UINT*)(&res)+1) << dec << "h ; �������� float ��� double\r\n";
					}
					if(dt == DTYPE_INT)
					{
						cmdList->GetUINT(pos, lowDW); pos += 4;
						logASM << "mov eax, 0" << hex << lowDW << dec << "h ; �������� int\r\n";
					}
					if(dt == DTYPE_SHORT)
					{
						cmdList->GetUSHORT(pos, sdata); pos += 2;
						lowDW = (sdata > 0 ? sdata : sdata | 0xFFFF0000);
						logASM << "mov eax, 0" << hex << lowDW << dec << "h ; �������� short\r\n";
					}
					if(dt == DTYPE_CHAR)
					{
						cmdList->GetUCHAR(pos, cdata); pos += 1;
						lowDW = cdata;
						logASM << "mov eax, 0" << hex << lowDW << dec << "h ; �������� char\r\n";
					}
				}else{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						logASM << "mov eax, [edx]\r\n";
						logASM << "push eax\r\n";
						logASM << "mov eax, [edx+4h] ; �������� double ��� long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						logASM << "push eax ; \r\n";
						logASM << "push eax ; ��������� ����� ��� double\r\n";
						logASM << "fld dword [edx] ; �������� float � fpu ����\r\n";
						logASM << "fstp qword [esp] ; �������� double � ������� ����\r\n";
						logASM << "pop eax\r\n";
					}
					if(dt == DTYPE_INT)
						logASM << "mov eax, [edx] ; �������� int\r\n";
					if(dt == DTYPE_SHORT)
						logASM << "movsx eax, word [edx] ; �������� short\r\n";
					if(dt == DTYPE_CHAR)
						logASM << "movsx eax, byte [edx] ; �������� char\r\n";
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

				if(flagAddrStk(cFlag) || flagShiftStk(cFlag) || flagSizeStk(cFlag))
				{
					if(st == STYPE_DOUBLE || st == STYPE_LONG)
					{
						logASM << "push eax ; ������� double ��� long ������� � ���� \r\n";
						logASM << "add esp, 8 ; �������� ������� �� ������ \r\n";
						logASM << "pop eax ; �������� � eax ��������� �� ������� �������� \r\n";
					}
					if(st == STYPE_INT)
					{
						logASM << "push eax ; ������� int ������� � ����\r\n";
						logASM << "add esp, 4 ; �������� ������� �� ������ \r\n";
						logASM << "pop eax ; �������� � eax ��������� �� ������� �������� \r\n";
					}
				}

				// �� ��� �������� �� ����� ����� �����������...
				if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ���� ���������� ���������, ����� �� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					//valind += typeSizeD[(cFlag>>2)&0x00000007]; // ������� ��� ������ ����������
					if(valind != 0)
						logASM << "mov edx, " << valind << " ; �������� ����������� �����\r\n";
					else
						logASM << "xor edx, edx ; ����������� ����� == 0\r\n";
				}else if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ���� ���������� ���������, ����� �� ����� � �����, � ������� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [eax + " << valind << "] ; ����� = ����������� ����� + ����� � �����\r\n";
					else
						logASM << "mov edx, eax ; ����� �� ���������� � ����� (opt: addr==0)\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ���� ���������� ���������, ����� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(shift != 0)
						logASM << "lea edx, [eax + " << shift << "] ; ����� = ����� � ����� + ����������� �����\r\n";
					else
						logASM << "mov edx, eax ; ����� = ����� � ����� (opt: shift==0)\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ���� ���������� ���������, ����� ����� � �����, � ������� ����� � �����
					logASM << "mov edx, eax ; ����� = ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
					logASM << "add edx, eax ; ����� = ����� � ����� + ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ���� ������������� ���������, ����� �� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					if(valind != 0)
						logASM << "lea edx, [ecx + " << valind << "] ; ����� = ���� ����� ���������� + ����������� ����� + ����������� �����\r\n";
					else
						logASM << "mov edx, ecx ; ����� = ���� ����� ���������� (opt: addr+shift==0)\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ���� ������������� ���������, ����� �� ����� � �����, � ������� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [eax + ecx + " << valind << "] ; ����� = ���� ����� ���������� + ����� � ����� + ����������� �����\r\n";
					else
						logASM << "lea edx, [eax + ecx] ; ����� = ���� ����� ���������� + ����� � ����� (opt: addr==0)\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ���� ������������� ���������, ����� ����� � �����, � ������� ����� � �������
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(shift != 0)
						logASM << "lea edx, [eax + ecx + " << shift << "] ; ����� = ���� ����� ���������� + ����������� ����� + ����� � �����\r\n";
					else
						logASM << "lea edx, [eax + ecx] ; ����� = ���� ����� ���������� + ����� � ����� (opt: shift==0)\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ���� ������������� ���������, ����� ����� � �����, � ������� ����� � �����
					logASM << "lea edx, [eax + ecx] ; ����� = ���� ����� ���������� + ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
					logASM << "add edx, eax ; ����� = ���� ����� ���������� + ����� � ����� + ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag))
				{
					// ���� ���������� ��������� � ����� �� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "mov edx, " << valind << "] ; ����� = ����������� �����\r\n";
					else
						logASM << "xor edx, edx ; ����� = 0 (opt: addr==0)\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag))
				{
					// ���� ���������� ��������� � ����� ����� � �����
					logASM << "mov edx, eax ; ����� = ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag))
				{
					// ���� ������������� ��������� � ����� �� ����� � �����
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ecx + " << valind << "] ; ����� = ���� ����� ���������� + ����������� �����\r\n";
					else
						logASM << "mov edx, ecx ; ����� = ���� ����� ���������� (opt: addr+shift==0)\r\n";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag))
				{
					// ���� ������������� ��������� � ����� ����� � �����
					logASM << "lea edx, [eax + ecx] ; ����� = ���� ����� ���������� + ����� � �����\r\n";
					logASM << "pop eax ; ������ �������������� �����\r\n";
				}

				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					logASM << "cmp edx, " << size << " ; ������� � ������������ �������\r\n";
					logASM << "jb movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "mov esi, [esi+4] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
					logASM << "call esi ; ������� �\r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					logASM << "pop eax ; ������ �������������� ������\r\n";
					movLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp edx, eax ; ������� � ������������ ������� � �����\r\n";
					logASM << "jb movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "mov esi, [esi+4] ; ������ ��������� �� ������ ��������� ������� (invalidOffset)\r\n";
					logASM << "call esi ; ������� �\r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					logASM << "pop eax ; ������ �������������� ������\r\n";
					movLabels++;
				}
				
				logASM << "add edx, " << typeSizeD[(cFlag>>2)&0x00000007] << " ; �������� ������\r\n";
				logASM << "cmp edi, edx ; ������� �� �������� �� ������ ����� ����������\r\n";
				logASM << "jge skipResize" << skipLabels << "\r\n";
				logASM << "mov edi, edx \r\n";
				logASM << "  skipResize" << skipLabels << ":\r\n";
				logASM << "lea edx, [edx + ebx - " << typeSizeD[(cFlag>>2)&0x00000007] << "] ; ����������� ����� � �������� �����\r\n";
				//logASM << "sub edx, " << typeSizeD[(cFlag>>2)&0x00000007] << " ; ���������� �����\r\n";
				skipLabels++;

				UINT restoreShift = 0;
				if(flagAddrStk(cFlag))
					restoreShift += 4;
				if(flagShiftStk(cFlag))
					restoreShift += 4;
				if(flagSizeStk(cFlag))
					restoreShift += 4;
				if(restoreShift != 0)
				{
					if(st == STYPE_DOUBLE || st == STYPE_LONG)
						restoreShift += 12;
					if(st == STYPE_INT)
						restoreShift += 8;

					logASM << "sub esp, " << restoreShift << " ; �������� ������� �� ������ \r\n";
					logASM << "pop eax ";
				}
				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					logASM << "mov [edx+4h], eax \r\n";
					logASM << "pop eax\r\n";
					logASM << "mov [edx], eax ; ��������� double ��� long long ����������.\r\n";
					logASM << "push eax \r\n";
					logASM << "mov eax, [edx+4h] ; �������� �������� � �����, ��� ����\r\n";
				}
				if(dt == DTYPE_FLOAT)
				{
					logASM << "fld qword [esp] ; �������� double �� ����� � fpu ����\r\n";
					logASM << "fstp qword [edx] ; ��������� float ����������\r\n";
				}
				if(dt == DTYPE_INT)
					logASM << "mov [edx], eax ; ��������� int ����������\r\n";
				if(dt == DTYPE_SHORT)
					logASM << "mov word [edx], eax ; ��������� short ����������\r\n";
				if(dt == DTYPE_CHAR)
					logASM << "mov byte [edx], eax ; ��������� char ����������\r\n";
				
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
					logASM << "push eax ; ������� double ������� � ����\r\n";
					logASM << "mov eax, dword [esi+8] ; ��������� �� ������� doubletolong\r\n";
					logASM << "pop eax \r\n";
					logASM << "pop eax ; ������ double �� �����\r\n";
					logASM << "call eax ; �������. ������ ������ 32���� ���������� - eax, ������� - edx\r\n";
				}else if(st == STYPE_DOUBLE && dt == DTYPE_LONG){
					logASM << "push eax ; ������� double ������� � ����\r\n";
					logASM << "mov eax, dword [esi+8] ; ��������� �� ������� doubletolong\r\n";
					logASM << "pop eax \r\n";
					logASM << "pop eax ; ������ double �� �����\r\n";
					logASM << "call eax ; �������. ������ ������ 32���� ���������� - eax, ������� - edx\r\n";
					logASM << "push edx ; ��� � ��������� long long � �����!";
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
					logASM << "push eax ; ������� ����� ������� � ����\r\n";
					logASM << "fild dword [esp] ; �������� � double\r\n";
					logASM << "push eax ; ��������� ����� ��� double\r\n";
					logASM << "fstp qword [esp] ; ��������� double � ����\r\n";
					logASM << "pop eax \r\n";
				}
				if(st == STYPE_LONG && dt == DTYPE_DOUBLE)
				{
					logASM << "push eax ; ������� ����� ������� � ����\r\n";
					logASM << "fild qword [esp] ; �������� � double\r\n";
					logASM << "fstp qword [esp] ; ��������� double � ����\r\n";
					logASM << "pop eax \r\n";
				}
			}
			break;
		case cmdITOL:
			logASM << "  ; ITOL\r\n";
			logASM << "cdq ; ��������� �� long � edx\r\n";
			logASM << "push edx ; �������� ������� � ����\r\n";
			break;
		case cmdLTOI:
			logASM << "  ; LTOI\r\n";
			logASM << "pop edx ; ������ ������� ���� long �� �����\r\n";
			break;
		case cmdSwap:
			logASM << "  ; SWAP\r\n";
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			switch(cFlag)
			{
			case (STYPE_DOUBLE)+(DTYPE_DOUBLE):
			case (STYPE_LONG)+(DTYPE_LONG):
				logASM << "xchg eax, [esp+4h]\r\n";
				logASM << "mov edx, [esp]\r\n";
				logASM << "xchg edx, [esp+8h]\r\n";
				logASM << "mov [esp], edx ; �������� ������� ��� long ��� double\r\n";
				break;
			case (STYPE_DOUBLE)+(DTYPE_INT):
			case (STYPE_LONG)+(DTYPE_INT):
				logASM << "xchg eax, [esp+4h]\r\n";
				logASM << "xchg eax, [esp] ; �������� ������� double � int\r\n";
				break;
			case (STYPE_INT)+(DTYPE_DOUBLE):
			case (STYPE_INT)+(DTYPE_LONG):
				logASM << "xchg eax, [esp]\r\n";
				logASM << "xchg eax, [esp+4h] ; �������� ������� int � double\r\n";
				break;
			case (STYPE_INT)+(DTYPE_INT):
				logASM << "xchg eax, [esp] ; �������� ������� ��� int\r\n";
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
				logASM << "push eax\r\n";
				logASM << "push edx ; ����������� long ��� double\r\n";
				break;
			case OTYPE_INT:
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
				logASM << "push eax ; ������� double ������� � ����\r\n";
				logASM << "fldz ; ������� ���� � fpu ����\r\n";
				logASM << "fcomp qword [esp] ; �������\r\n"; 
				logASM << "fnstsw ax ; ��������� ��� �������� �� fpu ���������� ������� � ax\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop eax ; ������ double �� �����\r\n";
				logASM << "test ah, 44h ; MSVS � ���-�� ����������\r\n";
				logASM << "jp gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "mov edx, eax \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; ���������� long == 0\r\n";
				logASM << "pop eax \r\n";
				logASM << "jne gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "mov edx, eax \r\n";
				logASM << "pop eax \r\n";
				logASM << "test edx, edx ; ���������� int == 0\r\n";
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
				logASM << "push eax ; ������� double ������� � ����\r\n";
				logASM << "fldz ; ������� ���� � fpu ����\r\n";
				logASM << "fcomp qword [esp] ; �������\r\n"; 
				logASM << "fnstsw ax ; ��������� ��� �������� �� fpu ���������� ������� � ax\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop eax ; ������ double �� �����\r\n";
				logASM << "test ah, 44h ; MSVS � ���-�� ����������\r\n";
				logASM << "jnp gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "mov edx, eax \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; ���������� long == 0\r\n";
				logASM << "pop eax \r\n";
				logASM << "je gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "mov edx, eax \r\n";
				logASM << "pop eax \r\n";
				logASM << "test edx, edx \r\n";
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
				logASM << ";TODO:  cmdAdd double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) += *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdAdd+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdAdd long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) += *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdAdd+(OTYPE_INT<<16):
				logASM << "  ; ADD int\r\n";
				logASM << "pop edx \r\n";
				logASM << "add eax, edx \r\n";
				break;
			case cmdSub+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdSub double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) -= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdSub+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdSub long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) -= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdSub+(OTYPE_INT<<16):
				logASM << "  ; SUB int\r\n";
				logASM << "xchg eax, edx \r\n";
				logASM << "pop edx \r\n";
				logASM << "sub eax, edx \r\n";
				break;
			case cmdMul+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdMul double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) *= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdMul+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdMul long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) *= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdMul+(OTYPE_INT<<16):
				logASM << "  ; MUL int\r\n";
				logASM << "pop edx \r\n";
				logASM << "imul edx ; ��������� � eax\r\n";
				break;
			case cmdDiv+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdDiv double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) /= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdDiv+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdDiv long\r\n";
			/*	if(*((long long*)(&genStack[genStack.size()-2])))
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
				}*/
				break;
			case cmdDiv+(OTYPE_INT<<16):
				logASM << "  ; DIV int\r\n";
				logASM << "pop edx \r\n";
				logASM << "idiv edx ; � �������� �� 0?\r\n";
				break;
			case cmdPow+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdPow double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) = pow(*((double*)(&genStack[genStack.size()-4])), *((double*)(&genStack[genStack.size()-2])));
				break;
			case cmdPow+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdPow long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = (long long)pow((double)*((long long*)(&genStack[genStack.size()-4])), (double)*((long long*)(&genStack[genStack.size()-2])));
				break;
			case cmdPow+(OTYPE_INT<<16):
				logASM << ";TODO:  cmdPow int\r\n";
			//	*((int*)(&genStack[genStack.size()-2])) = (int)pow((double)(*((int*)(&genStack[genStack.size()-2]))), *((int*)(&genStack[genStack.size()-1])));
				break;
			case cmdMod+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdMod double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) = fmod(*((double*)(&genStack[genStack.size()-4])), *((double*)(&genStack[genStack.size()-2])));
				break;
			case cmdMod+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdMod long\r\n";
			/*	if(*((long long*)(&genStack[genStack.size()-2])))
					*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) % *((long long*)(&genStack[genStack.size()-2]));
				else
					*((long long*)(&genStack[genStack.size()-4])) = 0;*/
				break;
			case cmdMod+(OTYPE_INT<<16):
				logASM << ";TODO:  cmdMod int\r\n";
			/*	if(*((int*)(&genStack[genStack.size()-1])))
					*((int*)(&genStack[genStack.size()-2])) %= *((int*)(&genStack[genStack.size()-1]));
				else
					*((int*)(&genStack[genStack.size()-2])) = 0;*/

				break;
			case cmdLess+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdLess double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) < *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdLess+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdLess long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) < *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLess+(OTYPE_INT<<16):
				logASM << "  ; LES int\r\n";
				logASM << "pop edx \r\n";
				logASM << "cmp eax, edx ; \r\n";
				logASM << "setl al";
				logASM << "movzx eax, al\r\n";
				break;
			case cmdGreater+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdGreater double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) > *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdGreater+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdGreater long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) > *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdGreater+(OTYPE_INT<<16):
				logASM << "  ; GRT int\r\n";
				logASM << "pop edx \r\n";
				logASM << "cmp eax, edx ; \r\n";
				logASM << "setg al";
				logASM << "movzx eax, al\r\n";
				break;
			case cmdLEqual+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdLEqual double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) <= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdLEqual+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdLEqual long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) <= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLEqual+(OTYPE_INT<<16):
				logASM << "  ; LEQL int\r\n";
				logASM << "pop edx \r\n";
				logASM << "cmp eax, edx ; \r\n";
				logASM << "setle al";
				logASM << "movzx eax, al\r\n";
				break;
			case cmdGEqual+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdGEqual double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) >= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdGEqual+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdGEqual long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) >= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdGEqual+(OTYPE_INT<<16):
				logASM << "  ; GEQL int\r\n";
				logASM << "pop edx \r\n";
				logASM << "cmp eax, edx ; \r\n";
				logASM << "setge al";
				logASM << "movzx eax, al\r\n";
				break;
			case cmdEqual+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdEqual double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) == *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdEqual+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdEqual long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) == *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdEqual+(OTYPE_INT<<16):
				logASM << "  ; EQL int\r\n";
				logASM << "pop edx \r\n";
				logASM << "cmp eax, edx ; \r\n";
				logASM << "sete al";
				logASM << "movzx eax, al\r\n";
				break;
			case cmdNEqual+(OTYPE_DOUBLE<<16):
				logASM << ";TODO:  cmdNEqual double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) != *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdNEqual+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdNEqual long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) != *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdNEqual+(OTYPE_INT<<16):
				logASM << "  ; NEQL int\r\n";
				logASM << "pop edx \r\n";
				logASM << "cmp eax, edx ; \r\n";
				logASM << "setne al";
				logASM << "movzx eax, al\r\n";
				break;
			case cmdShl+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdShl long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) << *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdShl+(OTYPE_INT<<16):
				logASM << "  ; SHL int\r\n";
				logASM << "pop edx \r\n";
				logASM << "sal eax, edx ; \r\n";
				break;
			case cmdShr+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdShr long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) >> *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdShr+(OTYPE_INT<<16):
				logASM << "  ; SHR int\r\n";
				logASM << "pop edx \r\n";
				logASM << "sar eax, edx ; \r\n";
				break;
			case cmdBitAnd+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdBitAnd long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) & *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitAnd+(OTYPE_INT<<16):
				logASM << "  ; BAND int\r\n";
				logASM << "pop edx \r\n";
				logASM << "and eax, edx ; \r\n";
				break;
			case cmdBitOr+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdBitOr long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) | *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitOr+(OTYPE_INT<<16):
				logASM << "  ; BOR int\r\n";
				logASM << "pop edx \r\n";
				logASM << "or eax, edx ; \r\n";
				break;
			case cmdBitXor+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdBitXor long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) ^ *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitXor+(OTYPE_INT<<16):
				logASM << "  ; BXOR int\r\n";
				logASM << "pop edx \r\n";
				logASM << "xor eax, edx ; \r\n";
				break;
			case cmdLogAnd+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdLogAnd long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) && *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLogAnd+(OTYPE_INT<<16):
				logASM << ";TODO:  cmdLogAnd int\r\n";
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) && *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLogOr+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdLogOr long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) || *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLogOr+(OTYPE_INT<<16):
				logASM << ";TODO:  cmdLogOr int\r\n";
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) || *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLogXor+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdLogXor long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = !!(*((long long*)(&genStack[genStack.size()-4]))) ^ !!(*((long long*)(&genStack[genStack.size()-2])));
				break;
			case cmdLogXor+(OTYPE_INT<<16):
				logASM << ";TODO:  cmdLogXor int\r\n";
			//	*((int*)(&genStack[genStack.size()-2])) = !!(*((int*)(&genStack[genStack.size()-2]))) ^ !!(*((int*)(&genStack[genStack.size()-1])));
				break;
			default:
				throw string("Operation is not implemented");
			}
			if(oFlag == OTYPE_INT)
			{
			//	genStack.pop_back();
			}else{
			//	genStack.pop_back(); genStack.pop_back();
			}
		}
		if(cmd >= cmdNeg && cmd <= cmdLogNot)
		{
			switch(cmd)
			{
			case cmdNeg:
				logASM <<";TODO: NEG\r\n";
				break;
			case cmdInc:
				logASM <<";TODO: INC\r\n";
				break;
			case cmdDec:
				logASM <<";TODO: DEC\r\n";
				break;
			case cmdBitNot:
				logASM <<";TODO: BNOT\r\n";
				break;
			case cmdLogNot:
				logASM <<";TODO: LNOT\r\n";
				break;
			}
		}
		if(cmd >= cmdIncAt && cmd < cmdDecAt)
		{
			switch(cmd)
			{
			case cmdIncAt:
				logASM <<";TODO: INCAT\r\n";
				break;
			case cmdDecAt:
				logASM <<";TODO: DECAT\r\n";
				break;
			}
		}
		//logASM << "\r\n";
	}

	ofstream m_FileStream("asmX86.txt", std::ios::binary);
	m_FileStream << logASM.str();
	m_FileStream.flush();
}
string ExecutorX86::GetListing()
{
	return logASM.str();
}

string ExecutorX86::GetResult()
{
	return "";
}

string ExecutorX86::GetVarInfo()
{
	return "";
}