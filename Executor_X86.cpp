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

bool ExecutorX86::Compile()
{
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
			logASM << "push ecx ; сохранили текущий размер стека переменных\r\n";
			logASM << "mov ecx, edi ; установили новую базу стека переменных\r\n";
			break;
		case cmdPopVTop:
			logASM << "pop ecx ; восстановили размер стека переменных\r\n";
			logASM << "mov edi, ecx ; восстановили базу стека переменных\r\n";
			break;
		case cmdCall:
			cmdList->GetData(pos, &valind, sizeof(UINT));
			pos += sizeof(UINT);
			logASM << "call function" << valind << "\r\n";
			break;
		case cmdReturn:
			logASM << "ret ; возвращаемся из функции\r\n";
			break;
		case cmdPushV:
			cmdList->GetData(pos, &valind, sizeof(int));
			pos += sizeof(int);
			logASM << "lea edi, [edi + 0" << hex << pos << dec << "h] ; добавили место под новые переменные в стеке\r\n";
			break;
		case cmdNop:
			logASM << "nop\r\n";
			break;
		case cmdCTI:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << "  ; cmdCTI, тут ещё следует разобратся imul eax, 0" << hex << valind << dec << "h ;умножим адрес на размер переменной\r\n";
			break;
		case cmdPush:
			{
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
						logASM << "push eax ; освободим место под double\r\n";
						logASM << "fld dword [edx] ; поместим float в fpu стек\r\n";
						logASM << "fstp qword [ebp-8h] ; поместим double в обычный стек\r\n";
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
				
				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					logASM << "mov [edx+4h], eax \r\n";
					logASM << "pop eax\r\n";
					logASM << "mov [edx], eax ; присвоили double или long long переменной.\r\n";
					logASM << "push eax eax\r\n";
					logASM << "mov eax, [edx+4h] ; оставили значение в стеке, как было\r\n";
				}
				if(dt == DTYPE_FLOAT)
				{
					logASM << "fld qword [ebp-8h] ; поместим double из стека в fpu стек\r\n";
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
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << ";TODO: RTOI\r\n";

			//asmStackType st = flagStackType(cFlag);
			//asmDataType dt = flagDataType(cFlag);
			
			/*if(st == STYPE_DOUBLE && dt == DTYPE_INT)
			{
				00412D39  push        ebp  
					00412D3A  mov         ebp,esp 
					00412D3C  sub         esp,8 
					00412D3F  and         esp,0FFFFFFF8h 
					00412D42  fstp        qword ptr [esp] 
				00412D45  cvttsd2si   eax,mmword ptr [esp] 
				00412D4A  leave            
					00412D4B  ret              
				int temp = (int)*((double*)(&genStack[genStack.size()-2]));
				genStack.pop_back();
				*((int*)(&genStack[genStack.size()-1])) = temp;
				genStackTypes.push_back(STYPE_INT);
			}else if(st == STYPE_DOUBLE && dt == DTYPE_LONG){
				*((long long*)(&genStack[genStack.size()-2])) = (long long)*((double*)(&genStack[genStack.size()-2]));
				genStackTypes.push_back(STYPE_LONG);
			}*/

			break;
		case cmdITOR:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << ";TODO: ITOR\r\n";
			break;
		case cmdITOL:
			logASM << ";TODO: ITOL\r\n";
			break;
		case cmdLTOI:
			logASM << ";TODO: LTOI\r\n";
			break;
		case cmdSwap:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << ";TODO: SWAP\r\n";
			break;
		case cmdCopy:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << ";TODO: COPY\r\n";
			break;
		case cmdJmp:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << "jmp gLabel" << valind << "\r\n";
			break;
		case cmdJmpZ:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << ";TODO: JMPZ\r\n";
			break;
		case cmdJmpNZ:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM <<";TODO: JMPNZ\r\n";
			break;
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
		{
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			switch(cmd)
			{
			case cmdAdd:
				logASM <<";TODO: ADD\r\n";
				break;
			case cmdSub:
				logASM <<";TODO: SUB\r\n";
				break;
			case cmdMul:
				logASM <<";TODO: MUL\r\n";
				break;
			case cmdDiv:
				logASM <<";TODO: DIV\r\n";
				break;
			case cmdPow:
				logASM <<";TODO: POW\r\n";
				break;
			case cmdMod:
				logASM <<";TODO: MOD\r\n";
				break;
			case cmdLess:
				logASM <<";TODO: LESS\r\n";
				break;
			case cmdGreater:
				logASM <<";TODO: GRT\r\n";
				break;
			case cmdLEqual:
				logASM <<";TODO: LE\r\n";
				break;
			case cmdGEqual:
				logASM <<";TODO: GE\r\n";
				break;
			case cmdEqual:
				logASM <<";TODO: EQUAL\r\n";
				break;
			case cmdNEqual:
				logASM <<";TODO: NEQUAL\r\n";
				break;
			case cmdShl:
				logASM <<";TODO: SHL\r\n";
				break;
			case cmdShr:
				logASM <<";TODO: SHR\r\n";
				break;
			case cmdBitAnd:
				logASM <<";TODO: BAND\r\n";
				break;
			case cmdBitOr:
				logASM <<";TODO: BOR\r\n";
				break;
			case cmdBitXor:
				logASM <<";TODO: BXOR\r\n";
				break;
			case cmdLogAnd:
				logASM <<";TODO: LAND\r\n";
				break;
			case cmdLogOr:
				logASM <<";TODO: LOR\r\n";
				break;
			case cmdLogXor:
				logASM <<";TODO: LXOR\r\n";
				break;
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

bool ExecutorX86::Run()
{
	return false;
}
string ExecutorX86::GetResult()
{
	return "";
}