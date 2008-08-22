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
	system("fasm.exe asmX86.txt");

	STARTUPINFO stInfo;
	PROCESS_INFORMATION prInfo;

	// Compile using fasm
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.cb = sizeof(stInfo);
	stInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USEPOSITION;
	stInfo.wShowWindow = SW_HIDE;
	stInfo.dwX = 900;
	stInfo.dwY = 200;
	memset(&prInfo, 0, sizeof(prInfo));

	if(!CreateProcess("fasm.exe", "asmX86.txt", NULL, NULL, false, 0, NULL, ".\\", &stInfo, &prInfo))
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
	__asm
	{
		pusha ;
		mov eax, binCodeStart ;
		call eax ;
		pop eax;
		popa ;
	}

	return false;
}
void ExecutorX86::GenListing()
{
	logASM.str("");

	UINT pos = 0, pos2 = 0;
	CmdID	cmd;
	char	name[512];
	UINT	valind;

	CmdFlag cFlag;
	OperFlag oFlag;
	asmStackType st, sdt;
	asmDataType dt;

	vector<int> instrNeedLabel;	// нужен ли перед инструкцией лейбл метки
	vector<int> funcNeedLabel;	// нужен ли перед инструкцией лейбл функции

	//Узнаем, кому нужны лейблы
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
		case cmdReturn:
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

	pos = 0;
	pos2 = 0;
	while(cmdList->GetData(pos, cmd))
	{
		for(int i = 0; i < instrNeedLabel.size(); i++)
		{
			if(pos == instrNeedLabel[i])
			{
				logASM << "  gLabel" << pos << ": \r\n";
				break;
			}
		}
		for(int i = 0; i < funcNeedLabel.size(); i++)
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
			logASM << "push ecx ; сохранили текущую базу стека переменных\r\n";
			logASM << "mov ecx, edi ; установили новую базу стека переменных, по размеру стека\r\n";
			break;
		case cmdPopVTop:
			logASM << "  ; POPT\r\n";
			logASM << "mov edi, ecx ; восстановили предыдущий размер стека переменных\r\n";
			logASM << "pop ecx ; восстановили предыдущую базу стека переменных\r\n";
			break;
		case cmdCall:
			logASM << "  ; CALL\r\n";
			cmdList->GetData(pos, &valind, sizeof(UINT));
			pos += sizeof(UINT);
			logASM << "call function" << valind << "\r\n";
			break;
		case cmdReturn:
			logASM << "  ; RET\r\n";
			logASM << "ret ; возвращаемся из функции\r\n";
			break;
		case cmdPushV:
			logASM << "  ; PUSHV\r\n";
			cmdList->GetData(pos, &valind, sizeof(int));
			pos += sizeof(int);
			logASM << "lea edi, [edi + 0" << hex << pos << dec << "h] ; добавили место под новые переменные в стеке\r\n";
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
				logASM << "push eax ; положим double целиком в стек\r\n";
				logASM << "mov eax, dword [esi+8] ; указатель на функцию doubletolong\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop eax ; убрали double со стека\r\n";
				logASM << "call eax ; вызовем. теперь нижние 32бита результата - eax, верхние - edx\r\n";
				break;
			case OTYPE_LONG:
				logASM << "pop edx ; убрали старшие биты long со стека\r\n";
				break;
			case OTYPE_INT:
				break;
			}
			logASM << "imul eax, 0" << hex << valind << dec << "h ;умножим адрес на размер переменной\r\n";
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

				// На все варианты на свете найдём оптимизацию...
				if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// Если абсолютная адресация, адрес не лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					if(valind != 0)
						logASM << "lea edx, [ebx + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем по константному сдвигу\r\n";
					else
						logASM << "mov edx, ebx ; возмём указатель на стек переменных (opt: addr+shift==0)\r\n";
				}else if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// Если абсолютная адресация, адрес не лежит в стеке, и имеется сдвиг в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// Если абсолютная адресация, адрес лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(shift != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << shift << dec << "h] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: shift==0)\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// Если абсолютная адресация, адрес лежит в стеке, и имеется сдвиг в стеке
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
					logASM << "add edx, eax ; сдвинем на ещё одно число в стеке\r\n";
					logASM << "pop eax ; убрали использованный сдвиг\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// Если относительная адресация, адрес не лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					if(valind != 0)
						logASM << "lea edx, [ebx + ecx + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем до базы стека переменных и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + ecx] ; возмём указатель на стек переменных и сдвинем до базы стека переменных(opt: addr+shift==0)\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// Если относительная адресация, адрес не лежит в стеке, и имеется сдвиг в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
					logASM << "pop eax ; убрали использованный сдвиг\r\n";
					logASM << "add edx, ecx ; сдвинем на базу стека переменных";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// Если относительная адресация, адрес лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(shift != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << shift << dec << "h] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: shift==0)\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
					logASM << "add edx, ecx ; сдвинем на базу стека переменных";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// Если относительная адресация, адрес лежит в стеке, и имеется сдвиг в стеке
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
					logASM << "add edx, eax ; сдвинем на ещё одно число в стеке\r\n";
					logASM << "pop eax ; убрали использованный сдвиг\r\n";
					logASM << "add edx, ecx ; сдвинем на базу стека переменных";
				}else if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag))
				{
					// Если абсолютная адресация и адрес не лежит в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем по константному сдвигу\r\n";
					else
						logASM << "mov edx, ebx ; возмём указатель на стек переменных (opt: addr==0)\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag))
				{
					// Если абсолютная адресация и адрес лежит в стеке
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag))
				{
					// Если относительная адресация и адрес не лежит в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + ecx + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем на базу стека переменных и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + ecx] ; возмём указатель на стек переменных и сдвинем на базу стека переменных(opt: addr+shift==0)\r\n";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag))
				{
					// Если относительная адресация и адрес лежит в стеке
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
					logASM << "add edx, ecx ; сдвинем на базу стека переменных";
				}
				
				static int pushLabels = 1;
				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					logASM << "cmp edx, 0" << hex << size << dec << "h ; сравним с максимальным сдвигом\r\n";
					logASM << "jb pushLabel" << pushLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "mov esi, [esi+4h] ; возьмём указатель на вторую системную функцию (invalidOffset)\r\n";
					logASM << "call esi ; вызовем её\r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					pushLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp edx, eax ; сравним с максимальным сдвигом в стеке\r\n";
					logASM << "jb pushLabel" << pushLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "mov esi, [esi+4h] ; возьмём указатель на вторую системную функцию (invalidOffset)\r\n";
					logASM << "call esi ; вызовем её\r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					pushLabels++;
				}
				logASM << "push eax ; освободим вершину для новых значений\r\n";
				if(flagNoAddr(cFlag))
				{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						cmdList->GetUINT(pos, highDW); pos += 4;
						cmdList->GetUINT(pos, lowDW); pos += 4;
						logASM << "mov eax, 0" << hex << highDW << dec << "h\r\n";
						logASM << "push eax\r\n";
						logASM << "mov eax, 0" << hex << lowDW << dec << "h ; положили double или long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						// Кладём флоат как double
						cmdList->GetUINT(pos, lowDW); pos += 4;
						double res = (double)*((float*)(&lowDW));
						logASM << "mov eax, 0" << hex << *((UINT*)(&res)) << dec << "h\r\n";
						logASM << "push eax\r\n";
						logASM << "mov eax, 0" << hex << *((UINT*)(&res)+1) << dec << "h ; положили float как double\r\n";
					}
					if(dt == DTYPE_INT)
					{
						cmdList->GetUINT(pos, lowDW); pos += 4;
						logASM << "mov eax, 0" << hex << lowDW << dec << "h ; положили int\r\n";
					}
					if(dt == DTYPE_SHORT)
					{
						cmdList->GetUSHORT(pos, sdata); pos += 2;
						lowDW = (sdata > 0 ? sdata : sdata | 0xFFFF0000);
						logASM << "mov eax, 0" << hex << lowDW << dec << "h ; положили short\r\n";
					}
					if(dt == DTYPE_CHAR)
					{
						cmdList->GetUCHAR(pos, cdata); pos += 1;
						lowDW = cdata;
						logASM << "mov eax, 0" << hex << lowDW << dec << "h ; положили char\r\n";
					}
				}else{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						logASM << "mov eax, [edx]\r\n";
						logASM << "push eax\r\n";
						logASM << "mov eax, [edx+4h] ; положили double или long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						logASM << "push eax ; \r\n";
						logASM << "push eax ; освободим место под double\r\n";
						logASM << "fld dword [edx] ; поместим float в fpu стек\r\n";
						logASM << "fstp qword [esp] ; поместим double в обычный стек\r\n";
						logASM << "pop eax\r\n";
					}
					if(dt == DTYPE_INT)
						logASM << "mov eax, [edx] ; положили int\r\n";
					if(dt == DTYPE_SHORT)
						logASM << "movsx eax, word [edx] ; положили short\r\n";
					if(dt == DTYPE_CHAR)
						logASM << "movsx eax, byte [edx] ; положили char\r\n";
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

				// На все варианты на свете найдём оптимизацию...
				if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// Если абсолютная адресация, адрес не лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					if(valind != 0)
						logASM << "lea edx, [ebx + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем по константному сдвигу\r\n";
					else
						logASM << "mov edx, ebx ; возмём указатель на стек переменных (opt: addr+shift==0)\r\n";
				}else if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// Если абсолютная адресация, адрес не лежит в стеке, и имеется сдвиг в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// Если абсолютная адресация, адрес лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(shift != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << shift << dec << "h] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: shift==0)\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// Если абсолютная адресация, адрес лежит в стеке, и имеется сдвиг в стеке
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
					logASM << "add edx, eax ; сдвинем на ещё одно число в стеке\r\n";
					logASM << "pop eax ; убрали использованный сдвиг\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// Если относительная адресация, адрес не лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					if(valind != 0)
						logASM << "lea edx, [ebx + ecx + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем до базы стека переменных и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + ecx] ; возмём указатель на стек переменных и сдвинем до базы стека переменных(opt: addr+shift==0)\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// Если относительная адресация, адрес не лежит в стеке, и имеется сдвиг в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
					logASM << "pop eax ; убрали использованный сдвиг\r\n";
					logASM << "add edx, ecx ; сдвинем на базу стека переменных";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// Если относительная адресация, адрес лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, shift);
					pos += 4;
					if(shift != 0)
						logASM << "lea edx, [ebx + eax + 0" << hex << shift << dec << "h] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: shift==0)\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
					logASM << "add edx, ecx ; сдвинем на базу стека переменных";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// Если относительная адресация, адрес лежит в стеке, и имеется сдвиг в стеке
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
					logASM << "add edx, eax ; сдвинем на ещё одно число в стеке\r\n";
					logASM << "pop eax ; убрали использованный сдвиг\r\n";
					logASM << "add edx, ecx ; сдвинем на базу стека переменных";
				}else if(flagAddrAbs(cFlag) && !flagAddrStk(cFlag))
				{
					// Если абсолютная адресация и адрес не лежит в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем по константному сдвигу\r\n";
					else
						logASM << "mov edx, ebx ; возмём указатель на стек переменных (opt: addr==0)\r\n";
				}else if(flagAddrAbs(cFlag) && flagAddrStk(cFlag))
				{
					// Если абсолютная адресация и адрес лежит в стеке
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
				}else if(flagAddrRel(cFlag) && !flagAddrStk(cFlag))
				{
					// Если относительная адресация и адрес не лежит в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(valind != 0)
						logASM << "lea edx, [ebx + ecx + 0" << hex << valind << dec << "h] ; возмём указатель на стек переменных и сдвинем на базу стека переменных и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [ebx + ecx] ; возмём указатель на стек переменных и сдвинем на базу стека переменных(opt: addr+shift==0)\r\n";
				}else if(flagAddrRel(cFlag) && flagAddrStk(cFlag))
				{
					// Если относительная адресация и адрес лежит в стеке
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке\r\n";
					logASM << "pop eax ; убрали использованный адрес\r\n";
					logASM << "add edx, ecx ; сдвинем на базу стека переменных";
				}

				static int movLabels = 1;
				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					logASM << "cmp edx, 0" << hex << size << dec << "h ; сравним с максимальным сдвигом\r\n";
					logASM << "jb movLabel" << movLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "mov esi, [esi+4h] ; возьмём указатель на вторую системную функцию (invalidOffset)\r\n";
					logASM << "call esi ; вызовем её\r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					movLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp edx, eax ; сравним с максимальным сдвигом в стеке\r\n";
					logASM << "jb movLabel" << movLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "mov esi, [esi+4h] ; возьмём указатель на вторую системную функцию (invalidOffset)\r\n";
					logASM << "call esi ; вызовем её\r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					movLabels++;
				}
				
				static int skipLabels = 1;
				logASM << "add edx, 0" << typeSizeD[(cFlag>>2)&0x00000007] << "h ; прибавим размер\r\n";
				logASM << "cmp edi, edx ; сравним не превышен ли размер стека переменных\r\n";
				logASM << "jge skipResize" << skipLabels << "\r\n";
				logASM << "mov edi, edx \r\n";
				logASM << "  skipResize" << skipLabels << ":\r\n";
				//logASM << "sub edx, 0" << typeSizeD[(cFlag>>2)&0x00000007] << "h ; востановим адрес\r\n";
				skipLabels++;

				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					logASM << "mov [edx+4h], eax \r\n";
					logASM << "pop eax\r\n";
					logASM << "mov [edx], eax ; присвоили double или long long переменной.\r\n";
					logASM << "push eax \r\n";
					logASM << "mov eax, [edx+4h] ; оставили значение в стеке, как было\r\n";
				}
				if(dt == DTYPE_FLOAT)
				{
					logASM << "fld qword [esp] ; поместим double из стека в fpu стек\r\n";
					logASM << "fstp qword [edx] ; присвоили float переменной\r\n";
				}
				if(dt == DTYPE_INT)
					logASM << "mov [edx], eax ; присвоили int переменной\r\n";
				if(dt == DTYPE_SHORT)
					logASM << "mov word [edx], eax ; присвоили short переменной\r\n";
				if(dt == DTYPE_CHAR)
					logASM << "mov byte [edx], eax ; присвоили char переменной\r\n";
				
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
				logASM << "pop eax ; убрали double или long\r\n";
			}else{
				logASM << "pop eax ; убрали int\r\n";
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
					logASM << "push eax ; положим double целиком в стек\r\n";
					logASM << "mov eax, dword [esi+8] ; указатель на функцию doubletolong\r\n";
					logASM << "pop eax \r\n";
					logASM << "pop eax ; убрали double со стека\r\n";
					logASM << "call eax ; вызовем. теперь нижние 32бита результата - eax, верхние - edx\r\n";
				}else if(st == STYPE_DOUBLE && dt == DTYPE_LONG){
					logASM << "push eax ; положим double целиком в стек\r\n";
					logASM << "mov eax, dword [esi+8] ; указатель на функцию doubletolong\r\n";
					logASM << "pop eax \r\n";
					logASM << "pop eax ; убрали double со стека\r\n";
					logASM << "call eax ; вызовем. теперь нижние 32бита результата - eax, верхние - edx\r\n";
					logASM << "push edx ; вот и составили long long в стеке!";
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
					logASM << "push eax ; загоним число целиком в стек\r\n";
					logASM << "fild dword [esp] ; переведём в double\r\n";
					logASM << "push eax ; освободим место под double\r\n";
					logASM << "fstp qword [esp] ; скопируем double в стек\r\n";
					logASM << "pop eax \r\n";
				}
				if(st == STYPE_LONG && dt == DTYPE_DOUBLE)
				{
					logASM << "push eax ; загоним число целиком в стек\r\n";
					logASM << "fild qword [esp] ; переведём в double\r\n";
					logASM << "fstp qword [esp] ; скопируем double в стек\r\n";
					logASM << "pop eax \r\n";
				}
			}
			break;
		case cmdITOL:
			logASM << "  ; ITOL\r\n";
			logASM << "cdq ; расширили до long в edx\r\n";
			logASM << "push edx ; положили старшие в стек\r\n";
			break;
		case cmdLTOI:
			logASM << "  ; LTOI\r\n";
			logASM << "pop edx ; убрали старшие биты long со стека\r\n";
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
				logASM << "mov [esp], edx ; поменяли местами два long или double\r\n";
				break;
			case (STYPE_DOUBLE)+(DTYPE_INT):
			case (STYPE_LONG)+(DTYPE_INT):
				logASM << "xchg eax, [esp+4h]\r\n";
				logASM << "xchg eax, [esp] ; поменяли местами double и int\r\n";
				break;
			case (STYPE_INT)+(DTYPE_DOUBLE):
			case (STYPE_INT)+(DTYPE_LONG):
				logASM << "xchg eax, [esp]\r\n";
				logASM << "xchg eax, [esp+4h] ; поменяли местами int и double\r\n";
				break;
			case (STYPE_INT)+(DTYPE_INT):
				logASM << "xchg eax, [esp] ; поменяли местами два int\r\n";
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
				logASM << "push edx ; скопировали long или double\r\n";
				break;
			case OTYPE_INT:
				logASM << "push eax ; скопировали int\r\n";
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
				logASM << "push eax ; положим double целиком в стек\r\n";
				logASM << "fldz ; положим ноль в fpu стек\r\n";
				logASM << "fcomp qword [esp] ; сравним\r\n"; 
				logASM << "fnstsw ax ; результат без проверок на fpu исключения положим в ax\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop eax ; убрали double со стека\r\n";
				logASM << "test ah, 44h ; MSVS с чем-то сравнивает\r\n";
				logASM << "jp gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "mov edx, eax \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; сравниваем long == 0\r\n";
				logASM << "pop eax \r\n";
				logASM << "jne gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "mov edx, eax \r\n";
				logASM << "pop eax \r\n";
				logASM << "test edx, edx ; сравниваем int == 0\r\n";
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
				logASM << "push eax ; положим double целиком в стек\r\n";
				logASM << "fldz ; положим ноль в fpu стек\r\n";
				logASM << "fcomp qword [esp] ; сравним\r\n"; 
				logASM << "fnstsw ax ; результат без проверок на fpu исключения положим в ax\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop eax ; убрали double со стека\r\n";
				logASM << "test ah, 44h ; MSVS с чем-то сравнивает\r\n";
				logASM << "jnp gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "mov edx, eax \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; сравниваем long == 0\r\n";
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
			//	*((double*)(&genStack[genStack.size()-4])) += *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdAdd+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) += *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdAdd+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) += *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdSub+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) -= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdSub+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) -= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdSub+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) -= *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdMul+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) *= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdMul+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) *= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdMul+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) *= *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdDiv+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) /= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdDiv+(OTYPE_LONG<<16):
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
			/*	if(*((int*)(&genStack[genStack.size()-1])))
					*((int*)(&genStack[genStack.size()-2])) /= *((int*)(&genStack[genStack.size()-1]));
				else{
					if(*((int*)(&genStack[genStack.size()-2])) > 0)
						*((int*)(&genStack[genStack.size()-2])) = (1 << 31) - 1;
					else if(*((int*)(&genStack[genStack.size()-2])) < 0)
						*((int*)(&genStack[genStack.size()-2])) = (1 << 31);
					else
						*((int*)(&genStack[genStack.size()-2])) = 0;
				}*/
				break;
			case cmdPow+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) = pow(*((double*)(&genStack[genStack.size()-4])), *((double*)(&genStack[genStack.size()-2])));
				break;
			case cmdPow+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = (long long)pow((double)*((long long*)(&genStack[genStack.size()-4])), (double)*((long long*)(&genStack[genStack.size()-2])));
				break;
			case cmdPow+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = (int)pow((double)(*((int*)(&genStack[genStack.size()-2]))), *((int*)(&genStack[genStack.size()-1])));
				break;
			case cmdMod+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) = fmod(*((double*)(&genStack[genStack.size()-4])), *((double*)(&genStack[genStack.size()-2])));
				break;
			case cmdMod+(OTYPE_LONG<<16):
			/*	if(*((long long*)(&genStack[genStack.size()-2])))
					*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) % *((long long*)(&genStack[genStack.size()-2]));
				else
					*((long long*)(&genStack[genStack.size()-4])) = 0;*/
				break;
			case cmdMod+(OTYPE_INT<<16):
			/*	if(*((int*)(&genStack[genStack.size()-1])))
					*((int*)(&genStack[genStack.size()-2])) %= *((int*)(&genStack[genStack.size()-1]));
				else
					*((int*)(&genStack[genStack.size()-2])) = 0;*/

				break;
			case cmdLess+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) < *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdLess+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) < *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLess+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) < *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdGreater+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) > *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdGreater+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) > *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdGreater+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) > *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLEqual+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) <= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdLEqual+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) <= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLEqual+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) <= *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdGEqual+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) >= *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdGEqual+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) >= *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdGEqual+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) >= *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdEqual+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) == *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdEqual+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) == *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdEqual+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) == *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdNEqual+(OTYPE_DOUBLE<<16):
			//	*((double*)(&genStack[genStack.size()-4])) = *((double*)(&genStack[genStack.size()-4])) != *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdNEqual+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) != *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdNEqual+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) != *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdShl+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) << *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdShl+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) << *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdShr+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) >> *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdShr+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) >> *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdBitAnd+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) & *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitAnd+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) & *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdBitOr+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) | *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitOr+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) | *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdBitXor+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) ^ *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitXor+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) ^ *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLogAnd+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) && *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLogAnd+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) && *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLogOr+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) || *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdLogOr+(OTYPE_INT<<16):
			//	*((int*)(&genStack[genStack.size()-2])) = *((int*)(&genStack[genStack.size()-2])) || *((int*)(&genStack[genStack.size()-1]));
				break;
			case cmdLogXor+(OTYPE_LONG<<16):
			//	*((long long*)(&genStack[genStack.size()-4])) = !!(*((long long*)(&genStack[genStack.size()-4]))) ^ !!(*((long long*)(&genStack[genStack.size()-2])));
				break;
			case cmdLogXor+(OTYPE_INT<<16):
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