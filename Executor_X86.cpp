#include "stdafx.h"
#include "Executor_X86.h"

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

bool ExecutorX86::Run()
{
	//GenListing();
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
	__asm
	{
		pusha ;
		mov eax, binCodeStart ;
		//mov ebx, data ;
		push ebp;
		mov ebp, 0h ;
		mov edi, 18h ;
		call eax ;
		pop eax;
		pop ebp;
		mov dword ptr [res1], eax;
		//push 0h;
		popa ;
	}
	runResult = res1;

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

	int pushLabels = 1;
	int movLabels = 1;
	int skipLabels = 1;
	int aluLabels = 1;

	bool skipPopEAXOnIntALU = false;

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
		//xchg al,[char]
		pos2 = pos;
		pos += 2;
		switch(cmd)
		{
		case cmdCallStd:
			break;
		case cmdPushVTop:
			logASM << "  ; PUSHT\r\n";
			logASM << "push ebp ; сохранили текущую базу стека переменных\r\n";
			logASM << "mov ebp, edi ; установили новую базу стека переменных, по размеру стека\r\n";
			break;
		case cmdPopVTop:
			logASM << "  ; POPT\r\n";
			logASM << "mov edi, ebp ; восстановили предыдущий размер стека переменных\r\n";
			logASM << "pop ebp ; восстановили предыдущую базу стека переменных\r\n";
			break;
		case cmdCall:
			logASM << "  ; CALL\r\n";
			cmdList->GetData(pos, &valind, sizeof(UINT));
			pos += sizeof(UINT);
			logASM << "call function" << valind << "\r\n";
			break;
		case cmdProlog:
			if(firstProlog)
			{
				logASM << "push ebp";
			}else{
				logASM << "mov ebp, edi ; установили новую базу стека переменных, по размеру стека\r\n";
				pos += 2; //пропустить PUSHT
			}
			firstProlog = !firstProlog;
			//logASM << "xchg eax, [esp]\r\n";
			//logASM << "push eax ; таким образом, eip остаётся на вершине, но под ним теперь содержимое eax до функции\r\n";
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
				logASM << "pop eax ; на время поместим double и long в регистры\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "pop eax ; на время поместим int в регистр\r\n";
			}
			logASM << "pop ebx ; сохранили eip\r\n";
			for(UINT pops = 0; pops < valind; pops++)
			{
				logASM << "mov edi, ebp ; восстановили предыдущий размер стека переменных\r\n";
				logASM << "pop ebp ; восстановили предыдущую базу стека переменных\r\n";
			}
			
			if(oFlag == OTYPE_DOUBLE || oFlag == OTYPE_LONG)
			{
				logASM << "push edx ; \r\n";
				logASM << "push eax ; поместим число обратно в стек\r\n";
				logASM << "push ebx ; поместим eip обратно в стек\r\n";
				/*logASM << "xchg [esp], eax ; поменяем часть числа и eip\r\n";
				logASM << "push edx ; поместим число обратно в стек\r\n";
				logASM << "push ebx ; поместим eip обратно в стек\r\n";*/
			}else if(oFlag == OTYPE_INT){
				logASM << "push eax ; поместим число обратно в стек\r\n";
				logASM << "push ebx ; поместим eip обратно в стек\r\n";
				/*logASM << "xchg [esp], eax ; поменяем число и eip\r\n";
				logASM << "push eax ; поместим eip обратно в стек\r\n";*/
			}

			logASM << "ret ; возвращаемся из функции\r\n";
			break;
		case cmdPushV:
			logASM << "  ; PUSHV\r\n";
			cmdList->GetData(pos, &valind, sizeof(int));
			pos += sizeof(int);
			logASM << "add edi, " << valind << " ; добавили место под новые переменные в стеке\r\n";
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
				logASM << "mov eax, dword [esi+8] ; указатель на функцию doubletolong\r\n";
				logASM << "call eax ; вызовем. теперь нижние 32бита результата - eax, верхние - edx\r\n";
				logASM << "pop ebx \r\n";
				logASM << "mov [esp], eax ; заменили double int'ом\r\n";
				break;
			case OTYPE_LONG:
				logASM << "pop edx ; убрали старшие биты long со стека\r\n";
				break;
			case OTYPE_INT:
				break;
			}
			logASM << "pop eax ; расчёт в eax\r\n";
			logASM << "imul " << valind << " ; умножим адрес на размер переменной\r\n";
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

				// Если читается из переменной и...
				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ...если адрес не лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					if(!(flagSizeOn(cFlag) || flagSizeStk(cFlag)))
						needEDX = texts[0];
					numEDX = valind;
					if(flagSizeOn(cFlag) || flagSizeStk(cFlag))
					{
						if(valind != 0)
							logASM << "mov edx, " << valind << " ; возмём указатель на стек переменных и сдвинем по константному сдвигу\r\n";
						else
							logASM << "xor edx, edx ; возмём указатель на стек переменных (opt: addr+shift==0)\r\n";
					}
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ...если адрес не лежит в стеке, и имеется сдвиг в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << "pop eax ; взяли сдвиг\r\n";
					if(valind != 0)
						logASM << "lea edx, [eax + " << valind << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "mov edx, eax ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ...если адрес лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, shift);
					pos += 4;
					logASM << "pop eax ; взяли адрес\r\n";
					if(shift != 0)
						logASM << "lea edx, [eax + " << shift << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "mov edx, eax ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: shift==0)\r\n";
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ...если адрес лежит в стеке, и имеется сдвиг в стеке
					logASM << "pop eax ; взяли адрес\r\n";
					logASM << "pop ebx ; взяли сдвиг\r\n";
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке\r\n";
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag))
				{
					// ...если адрес не лежит в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(!(flagSizeOn(cFlag) || flagSizeStk(cFlag)))
						needEDX = texts[0];
					numEDX = valind;
					if(flagSizeOn(cFlag) || flagSizeStk(cFlag))
					{
						if(valind != 0)
							logASM << "mov edx, " << valind << " ; возмём указатель на стек переменных и сдвинем по константному сдвигу\r\n";
						else
							logASM << "xor edx, edx ; возмём указатель на стек переменных (opt: addr==0)\r\n";
					}
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagAddrStk(cFlag))
				{
					// ...если адрес лежит в стеке
					logASM << "pop eax ; взяли адрес\r\n";
					logASM << "mov edx, eax ; возмём указатель на стек переменных\r\n";
				}

				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					logASM << "cmp edx, " << size << " ; сравним с максимальным сдвигом\r\n";
					logASM << "jb pushLabel" << pushLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "mov esi, [esi+4h] ; возьмём указатель на вторую системную функцию (invalidOffset)\r\n";
					logASM << "call esi ; вызовем её\r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					pushLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "pop eax ; взяли максимальный сдвиг\r\n";
					logASM << "cmp edx, eax ; сравним с текущим\r\n";
					logASM << "jb pushLabel" << pushLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "mov esi, [esi+4h] ; возьмём указатель на вторую системную функцию (invalidOffset)\r\n";
					logASM << "call esi ; вызовем её\r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					pushLabels++;
				}

				if(flagNoAddr(cFlag))
				{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						cmdList->GetUINT(pos, highDW); pos += 4;
						cmdList->GetUINT(pos, lowDW); pos += 4;
						logASM << "push " << highDW << "\r\n";
						logASM << "push " << lowDW << " ; положили double или long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						// Кладём флоат как double
						cmdList->GetUINT(pos, lowDW); pos += 4;
						double res = (double)*((float*)(&lowDW));
						logASM << "push " << *((UINT*)(&res)) << "\r\n";
						logASM << "push " << *((UINT*)(&res)+1) << " ; положили float как double\r\n";
					}
					if(dt == DTYPE_INT)
					{
						//look at the next command
						cmdList->GetData(pos+4, cmdNext);
						if(cmdNext >= cmdAdd && cmdNext <= cmdLogXor)
						{
							needPush = texts[4];
							skipPopEAXOnIntALU = true;
						}

						cmdList->GetUINT(pos, lowDW); pos += 4;
						logASM << needPush << lowDW << " ; положили int\r\n";
					}
					if(dt == DTYPE_SHORT)
					{
						cmdList->GetUSHORT(pos, sdata); pos += 2;
						lowDW = (sdata > 0 ? sdata : sdata | 0xFFFF0000);
						logASM << "push " << lowDW << " ; положили short\r\n";
					}
					if(dt == DTYPE_CHAR)
					{
						cmdList->GetUCHAR(pos, cdata); pos += 1;
						lowDW = cdata;
						logASM << "push " << lowDW << " ; положили char\r\n";
					}
				}else{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX << "]\r\n";
						logASM << "push dword [" << needEDX << needEBP << paramBase+4+numEDX << "] ; положили double или long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						logASM << "push eax ; \r\n";
						logASM << "push eax ; освободим место под double\r\n";
						logASM << "fld dword [" << needEDX << needEBP << paramBase+numEDX << "] ; поместим float в fpu стек\r\n";
						logASM << "fstp qword [esp] ; поместим double в обычный стек\r\n";
					}
					if(dt == DTYPE_INT)
					{
						//look at the next command
						cmdList->GetData(pos, cmdNext);
						if(cmdNext >= cmdAdd && cmdNext <= cmdLogXor)
						{
							needPush = texts[4];
							skipPopEAXOnIntALU = true;
						}
						logASM << needPush << "dword [" << needEDX << needEBP << paramBase+numEDX << "] ; положили int\r\n";
					}
					if(dt == DTYPE_SHORT)
					{
						logASM << "movsx eax, word [" << needEDX << needEBP << paramBase+numEDX << "] ; положили short\r\n";
						logASM << "push eax \r\n";
					}
					if(dt == DTYPE_CHAR)
					{
						logASM << "movsx eax, byte [" << needEDX << needEBP << paramBase+numEDX << "] ; положили char\r\n";
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
					// Регистров много, не паримся!
					if(st == STYPE_DOUBLE || st == STYPE_LONG)
					{
						logASM << "pop ebx ; \r\n";
						logASM << "pop ecx ; считаем double или long long в регистры \r\n";
					}else if(st == STYPE_INT)
					{
						logASM << "pop ebx ; считаем int в регистр\r\n";
					}
				}

				UINT numEDX = 0;
				bool knownEDX = false;
				bool cantOptimiseEDX = flagSizeOn(cFlag) || flagSizeStk(cFlag);

				// Если читается из переменной и...
				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ...если адрес не лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, valind);
					pos += 4;
					cmdList->GetINT(pos, shift);
					pos += 4;
					valind += shift;
					if(cantOptimiseEDX)
					{
						if(valind != 0)
							logASM << "mov edx, " << valind << " ; возмём указатель на стек переменных и сдвинем по константному сдвигу\r\n";
						else
							logASM << "xor edx, edx ; возмём указатель на стек переменных (opt: addr+shift==0)\r\n";
					}else{
						knownEDX = true;
						numEDX = valind;
					}
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ...если адрес не лежит в стеке, и имеется сдвиг в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << "pop eax ; взяли сдвиг\r\n";
					if(valind != 0)
						logASM << "lea edx, [eax + " << valind << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "mov edx, eax ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagAddrStk(cFlag) && flagShiftOn(cFlag))
				{
					// ...если адрес лежит в стеке, и имеется сдвиг в команде
					cmdList->GetINT(pos, shift);
					pos += 4;
					logASM << "pop eax ; взяли адрес\r\n";
					if(shift != 0)
						logASM << "lea edx, [eax + " << shift << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "mov edx, eax ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: shift==0)\r\n";
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagAddrStk(cFlag) && flagShiftStk(cFlag))
				{
					// ...если адрес лежит в стеке, и имеется сдвиг в стеке
					logASM << "pop eax ; взяли адрес\r\n";
					logASM << "pop ebx ; взяли сдвиг\r\n";
					logASM << "lea edx, [ebx + eax] ; возмём указатель на стек переменных и сдвинем на число в стеке\r\n";
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && !flagAddrStk(cFlag))
				{
					// ...если адрес не лежит в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(cantOptimiseEDX)
					{
						if(valind != 0)
							logASM << "mov edx, " << valind << " ; возмём указатель на стек переменных и сдвинем по константному сдвигу\r\n";
						else
							logASM << "xor edx, edx ; возмём указатель на стек переменных (opt: addr==0)\r\n";
					}else{
						knownEDX = true;
						numEDX = valind;
					}
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagAddrStk(cFlag))
				{
					// ...если адрес лежит в стеке
					logASM << "pop eax ; взяли адрес\r\n";
					logASM << "mov edx, eax ; возмём указатель на стек переменных\r\n";
				}

				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					logASM << "cmp edx, " << size << " ; сравним с максимальным сдвигом\r\n";
					logASM << "jb movLabel" << movLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "mov esi, [esi+4] ; возьмём указатель на вторую системную функцию (invalidOffset)\r\n";
					logASM << "call esi ; вызовем её\r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					logASM << "pop eax ; убрали использованный размер\r\n";
					movLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp edx, eax ; сравним с максимальным сдвигом в стеке\r\n";
					logASM << "jb movLabel" << movLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "mov esi, [esi+4] ; возьмём указатель на вторую системную функцию (invalidOffset)\r\n";
					logASM << "call esi ; вызовем её\r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					logASM << "pop eax ; убрали использованный размер\r\n";
					movLabels++;
				}
				
				//UINT varSize = typeSizeD[(cFlag>>2)&0x00000007];
				//if(knownEDX)
				//{
				//	if(flagAddrAbs(cFlag))
				//		logASM << "mov edx, " << /*varSize+*/numEDX << " ; прибавим размер\r\n";
				//	else
				//		logASM << "lea edx, [ebp + " << /*varSize+*/numEDX << "] ; прибавим размер\r\n";
				//}else{
				//	if(!flagAddrAbs(cFlag))
				//		logASM << "lea edx, [edx + ebp] ; прибавим размер\r\n";
					/*if(flagAddrAbs(cFlag))
						logASM << "add edx, " << varSize << " ; прибавим размер\r\n";
					else
						logASM << "lea edx, [edx + ebp + " << varSize << "] ; прибавим размер\r\n";*/
				//}
				//logASM << "cmp edi, edx ; сравним не превышен ли размер стека переменных\r\n";
				//logASM << "jge skipResize" << skipLabels << "\r\n";
				//logASM << "mov edi, edx \r\n";
				//logASM << "  skipResize" << skipLabels << ":\r\n";
				//skipLabels++;

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
						logASM << "mov [" << needEDX << needEBP << final+4 << "], ebx \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ecx ; присвоили double или long long переменной.\r\n";
						logASM << "push ecx \r\n";
						logASM << "push ebx ; оставили значение в стеке, как было\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov ecx, [esp+4] \r\n";
						logASM << "mov [" << needEDX << needEBP << final+4 << "], ebx \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ecx ; присвоили double или long long переменной.\r\n";
					}
				}
				if(dt == DTYPE_FLOAT)
				{
					if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
						logASM << "push ebx \r\n";
					logASM << "fld qword [esp] ; поместим double из стека в fpu стек\r\n";
					logASM << "fstp dword [" << needEDX << needEBP << final << "] ; присвоили float переменной\r\n";
				}
				if(dt == DTYPE_INT)
				{
					if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
					{
						logASM << "push ebx \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx ; присвоили int переменной\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx ; присвоили int переменной\r\n";
					}
				}
				if(dt == DTYPE_SHORT)
				{
					if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
					{
						logASM << "push ebx \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; присвоили short переменной\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; присвоили short переменной\r\n";
					}
				}
				if(dt == DTYPE_CHAR)
				{
					if(flagAddrStk(cFlag) | flagShiftStk(cFlag) | flagSizeStk(cFlag))
					{
						logASM << "push ebx \r\n";
						logASM << "mov byte [" << needEDX << needEBP << final << "], bl ; присвоили char переменной\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov byte [" << needEDX << needEBP << final << "], bl ; присвоили char переменной\r\n";
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
					logASM << "mov eax, dword [esi+8] ; указатель на функцию doubletolong\r\n";
					logASM << "call eax ; вызовем. теперь нижние 32бита результата - eax, верхние - edx\r\n";
					logASM << "pop ebx \r\n";
					logASM << "pop ebx ; убрали double со стека\r\n";
					logASM << "push eax ; положим int в стек\r\n";
				}else if(st == STYPE_DOUBLE && dt == DTYPE_LONG){
					logASM << "mov eax, dword [esi+8] ; указатель на функцию doubletolong\r\n";
					logASM << "call eax ; вызовем. теперь нижние 32бита результата - eax, верхние - edx\r\n";
					logASM << "xchg eax, [esp] \r\n";
					logASM << "xchg edx, [esp-4] ; составили long long в стеке\r\n";
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
					logASM << "fild dword [esp] ; переведём в double\r\n";
					logASM << "push eax ; освободим место под double\r\n";
					logASM << "fstp qword [esp] ; скопируем double в стек\r\n";
				}
				if(st == STYPE_LONG && dt == DTYPE_DOUBLE)
				{
					logASM << "fild qword [esp] ; переведём в double\r\n";
					logASM << "fstp qword [esp] ; скопируем double в стек\r\n";
				}
			}
			break;
		case cmdITOL:
			logASM << "  ; ITOL\r\n";
			logASM << "pop eax ; взяли int\r\n";
			logASM << "cdq ; расширили до long в edx\r\n";
			logASM << "push edx ; положили старшие в стек\r\n";
			logASM << "push eax ; полжили младшие в стек\r\n";
			break;
		case cmdLTOI:
			logASM << "  ; LTOI\r\n";
			logASM << "pop eax ; взяли младшие биты\r\n";
			logASM << "xchg eax, [esp] ; заменили старшие младшими\r\n";
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
				logASM << "push eax ; поменяли местами два long или double\r\n";
				break;
			case (STYPE_DOUBLE)+(DTYPE_INT):
			case (STYPE_LONG)+(DTYPE_INT):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp+4h]\r\n";
				logASM << "xchg eax, [esp]\r\n";
				logASM << "push eax ; поменяли местами (long или double) и int\r\n";
				break;
			case (STYPE_INT)+(DTYPE_DOUBLE):
			case (STYPE_INT)+(DTYPE_LONG):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp]\r\n";
				logASM << "xchg eax, [esp+4h]\r\n";
				logASM << "push eax ; поменяли местами int и (long или double)\r\n";
				break;
			case (STYPE_INT)+(DTYPE_INT):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp]\r\n";
				logASM << "push eax ; поменяли местами два int\r\n";
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
				logASM << "push edx ; скопировали long или double\r\n";
				break;
			case OTYPE_INT:
				logASM << "mov eax, [esp]\r\n";
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
				logASM << "fldz ; положим ноль в fpu стек\r\n";
				logASM << "fcomp qword [esp] ; сравним\r\n"; 
				logASM << "fnstsw ax ; результат без проверок на fpu исключения положим в ax\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop eax ; убрали double со стека\r\n";
				logASM << "test ah, 44h ; MSVS с чем-то сравнивает\r\n";
				logASM << "jp gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "pop edx \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; сравниваем long == 0\r\n";
				logASM << "jne gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "pop eax \r\n";
				logASM << "test eax, eax ; сравниваем int == 0\r\n";
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
				logASM << "fldz ; положим ноль в fpu стек\r\n";
				logASM << "fcomp qword [esp] ; сравним\r\n"; 
				logASM << "fnstsw ax ; результат без проверок на fpu исключения положим в ax\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop eax ; убрали double со стека\r\n";
				logASM << "test ah, 44h ; MSVS с чем-то сравнивает\r\n";
				logASM << "jnp gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "pop edx \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; сравниваем long == 0\r\n";
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
				logASM << ";TODO:  cmdAdd double\r\n";
			//	*((double*)(&genStack[genStack.size()-4])) += *((double*)(&genStack[genStack.size()-2]));
				break;
			case cmdAdd+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdAdd long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) += *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdAdd+(OTYPE_INT<<16):
				logASM << "  ; ADD int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "add [esp], eax \r\n";
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
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "sub [esp], eax \r\n";
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
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "imul edx \r\n";
				logASM << "push eax \r\n";
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
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "idiv edx ; а проверка на 0?\r\n";
				logASM << "push eax \r\n";
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
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setl cl \r\n";
				logASM << "mov [esp], ecx\r\n";
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
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setg cl \r\n";
				logASM << "mov [esp], ecx\r\n";
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
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setle cl \r\n";
				logASM << "mov [esp], ecx\r\n";
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
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setge cl \r\n";
				logASM << "mov [esp], ecx\r\n";
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
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "sete cl \r\n";
				logASM << "mov [esp], ecx\r\n";
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
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setne cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;
			case cmdShl+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdShl long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) << *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdShl+(OTYPE_INT<<16):
				logASM << "  ; SHL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "sal edx, eax ; \r\n";
				logASM << "push edx \r\n";
				break;
			case cmdShr+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdShr long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) >> *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdShr+(OTYPE_INT<<16):
				logASM << "  ; SHR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "sar edx, eax ; \r\n";
				logASM << "push edx \r\n";
				break;
			case cmdBitAnd+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdBitAnd long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) & *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitAnd+(OTYPE_INT<<16):
				logASM << "  ; BAND int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "and [esp], eax ; \r\n";
				break;
			case cmdBitOr+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdBitOr long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) | *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitOr+(OTYPE_INT<<16):
				logASM << "  ; BOR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "or [esp], eax ; \r\n";
				break;
			case cmdBitXor+(OTYPE_LONG<<16):
				logASM << ";TODO:  cmdBitXor long\r\n";
			//	*((long long*)(&genStack[genStack.size()-4])) = *((long long*)(&genStack[genStack.size()-4])) ^ *((long long*)(&genStack[genStack.size()-2]));
				break;
			case cmdBitXor+(OTYPE_INT<<16):
				logASM << "  ; BXOR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor [esp], eax ; \r\n";
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
			skipPopEAXOnIntALU = false;
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
	char buf[100];
	_itoa(runResult, buf, 10);
	return string(buf);
}

string ExecutorX86::GetVarInfo()
{
	return "";
}