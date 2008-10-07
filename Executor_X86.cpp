#include "stdafx.h"
#include "Executor_X86.h"
#include "StdLib_X86.h"
#include "Optimizer_x86.h"
#include <MMSystem.h>

UINT paramDataBase;
UINT reservedStack;
UINT commitedStack;
UINT stackGrowSize;
UINT stackGrowCommit;

ExecutorX86::ExecutorX86(CommandList* cmds, std::vector<VariableInfo>* varinfo)
{
	cmdList = cmds;
	varInfo = varinfo;

	stackGrowSize = 128*4096;
	stackGrowCommit = 64*4096;
	// Request memory at address
	if(!(paramData = (char*)VirtualAlloc(reinterpret_cast<void*>(0x20000000), stackGrowSize, MEM_RESERVE, PAGE_NOACCESS)))
		throw std::string("ERROR: Failed to reserve memory");
	if(!VirtualAlloc(reinterpret_cast<void*>(0x20000000), stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
		throw std::string("ERROR: Failed to commit memory");

	reservedStack = stackGrowSize;
	commitedStack = stackGrowCommit;
	
	paramDataBase = paramBase = static_cast<UINT>(reinterpret_cast<long long>(paramData));
}
ExecutorX86::~ExecutorX86()
{
	VirtualFree(reinterpret_cast<void*>(0x20000000), reservedStack, MEM_RELEASE);
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
			// Проверим, не привысии ли мы объём доступной памяти
			if(reservedStack > 512*1024*1024)
			{
				expAllocCode = 4;
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// Разрешим использование последней страницы зарезервированной памяти
			if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+commitedStack)), stackGrowSize-stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
			{
				expAllocCode = 1; // failed to commit all old memory
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// Зарезервируем ещё память прямо после предыдущего блока
			if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+reservedStack)), stackGrowSize, MEM_RESERVE, PAGE_NOACCESS))
			{
				expAllocCode = 2; // failed to reserve new memory
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// Разрешим использование всей зарезервированной памяти кроме последней страницы
			if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+reservedStack)), stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
			{
				expAllocCode = 3; // failed to commit new memory
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// Обновим переменные
			commitedStack = reservedStack;
			reservedStack += stackGrowSize;
			commitedStack += stackGrowCommit;
			stackReallocs++;

			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

#pragma warning(disable: 4731)
UINT ExecutorX86::Run()
{
	stackReallocs = 0;

	*(double*)(paramData) = 0.0;
	*(double*)(paramData+8) = 3.1415926535897932384626433832795;
	*(double*)(paramData+16) = 2.7182818284590452353602874713527;
	
	FILE *fCode = fopen("asmX86.bin", "rb");
	if(!fCode)
		throw std::string("Failed to open output file");
	char binCode[200000];
	fseek(fCode, 0, SEEK_END);
	UINT size = ftell(fCode);
	fseek(fCode, 0, SEEK_SET);
	if(size > 200000)
		throw std::string("Byte code is too big (size > 200000)");
	fread(binCode, 1, size, fCode);

	typedef void (*codeFunc)();
	codeFunc funcMain = (codeFunc)(&binCode[0]);
	UINT binCodeStart = static_cast<UINT>(reinterpret_cast<long long>(&binCode[0]));

	UINT startTime = timeGetTime();
	UINT res1 = 0;
	UINT res2 = 0;
	UINT resT = 0;
	__try 
	{
		__asm
		{
			pusha ; // Сохраним все регистры
			mov eax, binCodeStart ;

			push ebp; // Сохраним базу стека (её придётся востановить до popa)

			mov ebp, 0h ;
			mov edi, 18h ;
			call eax ; // в ebx тип вернувшегося значения

		//	pop eax; // Возмём первый dword

			cmp ebx, 3 ; // oFlag == 3, значит int
			je justAnInt ; // пропустим взятие второй части для long и double
		//	pop edx; // Возьмём второй dword

			justAnInt:

			pop ebp; // Востановим базу стека
			mov dword ptr [res1], eax;
			mov dword ptr [res2], edx;
			mov dword ptr [resT], ebx;

			popa ;
		}
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
	UINT runTime = timeGetTime() - startTime;

	runResult = res1;
	runResult2 = res2;
	runResultType = resT;

	//just for fun, save the parameter data to bmp
	FILE *fBMP = fopen("funny.bmp", "wb");
	fwrite(paramData+24, 1, commitedStack-24, fBMP);
	fclose(fBMP);
	return runTime;
}
#pragma warning(default: 4731)

void ExecutorX86::GenListing()
{
	logASM.str("");

	UINT pos = 0, pos2 = 0;
	CmdID	cmd, cmdNext;
	char	name[512];
	UINT	valind, valind2;

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
			size_t len;
			cmdList->GetData(pos, len);
			pos += sizeof(size_t);
			pos += (UINT)len;
			break;
		case cmdPushVTop:
			break;
		case cmdPopVTop:
			break;
		case cmdCall:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			pos += 2;
			funcNeedLabel.push_back(valind);
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
	int skipLabels = 1;
	int aluLabels = 1;

	bool skipPopEAXOnIntALU = false;
	bool skipFldESPOnDoubleALU = false;
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
				logASM << "  function" << pos << ": \r\n";
				break;
			}
		}

		pos2 = pos;
		pos += 2;
		const char *descStr;
		if(descStr = cmdList->GetDescription(pos2))
		{
			logASM << "\r\n  ; \"" << descStr << "\" codeinfo\r\n";
		}
		switch(cmd)
		{
		case cmdCallStd:
			logASM << "  ; CALLSTD ";
			size_t len;
			cmdList->GetData(pos, len);
			pos += sizeof(size_t);
			if(len >= 511)
				throw std::string("ERROR: standard function can't have length>512");
			cmdList->GetData(pos, name, len);
			pos += (UINT)len;
			name[len] = 0;

			if(memcmp(name, "cos", 3) == 0)
			{
				logASM << "cos \r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "push 180 \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fdivp \r\n";
				logASM << "fldpi \r\n";
				logASM << "fmulp \r\n";
				logASM << "fsincos \r\n";
				logASM << "fstp qword [esp+4] \r\n";
				logASM << "fstp st \r\n";
				logASM << "pop eax \r\n";
			}else if(memcmp(name, "sin", 3) == 0){
				logASM << "sin \r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "push 180 \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fdivp \r\n";
				logASM << "fldpi \r\n";
				logASM << "fmulp \r\n";
				logASM << "fsincos \r\n";
				logASM << "fstp st \r\n";
				logASM << "fstp qword [esp+4] \r\n";
				logASM << "pop eax \r\n";
			}else if(memcmp(name, "tan", 3) == 0){
				logASM << "tan \r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "push 180 \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fdivp \r\n";
				logASM << "fldpi \r\n";
				logASM << "fmulp \r\n";
				logASM << "fptan \r\n";
				logASM << "fstp st \r\n";
				logASM << "fstp qword [esp+4] \r\n";
				logASM << "pop eax \r\n";
			}else if(memcmp(name, "ctg", 3) == 0){
				logASM << "ctg \r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "push 180 \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fdivp \r\n";
				logASM << "fldpi \r\n";
				logASM << "fmulp \r\n";
				logASM << "fptan \r\n";
				logASM << "fdivrp \r\n";
				logASM << "fstp qword [esp+4] \r\n";
				logASM << "pop eax \r\n";
			}else if(memcmp(name, "ceil", 4) == 0){
				logASM << "ceil \r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "push eax ; сюда положим флаг fpu \r\n";
				logASM << "fstcw word [esp] ; сохраним флаг контроля \r\n";
				logASM << "mov word [esp+2], 1BBFh ; сохраним свой с окурглением к +inf \r\n";
				logASM << "fldcw word [esp+2] ; установим его \r\n";
				logASM << "frndint ; округлим до целого \r\n";
				logASM << "fldcw word [esp] ; востановим флаг контроля \r\n";
				logASM << "fstp qword [esp+4] \r\n";
				logASM << "pop eax ; \r\n";
			}else if(memcmp(name, "floor", 5) == 0){
				logASM << "floor \r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "push eax ; сюда положим флаг fpu \r\n";
				logASM << "fstcw word [esp] ; сохраним флаг контроля \r\n";
				logASM << "mov word [esp+2], 17BFh ; сохраним свой с окурглением к -inf \r\n";
				logASM << "fldcw word [esp+2] ; установим его \r\n";
				logASM << "frndint ; округлим до целого \r\n";
				logASM << "fldcw word [esp] ; востановим флаг контроля \r\n";
				logASM << "fstp qword [esp+4] \r\n";
				logASM << "pop eax ; \r\n";
			}else if(memcmp(name, "sqrt", 4) == 0){
				logASM << "sqrt \r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "fsqrt \r\n";
				logASM << "fstp qword [esp] \r\n";
				logASM << "fstp st \r\n";
			}else if(memcmp(name, "clock", 5) == 0){
				logASM << "clock \r\n";
				logASM << "mov ecx, 0x" << GetTickCount << " ; GetTickCount() \r\n";
				logASM << "call ecx \r\n";
				logASM << "push eax \r\n";
			}else{
				throw std::string("ERROR: there is no such function: ") + name;
			}
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
			{
				RetFlag retFlag;
				cmdList->GetUINT(pos, valind);
				pos += 4;
				cmdList->GetUSHORT(pos, retFlag);
				pos += 2;
				logASM << "  ; CALL " << valind << " ret " << (retFlag & bitRetSimple ? "simple " : "") << "size: ";
				if(retFlag & bitRetSimple)
				{
					oFlag = retFlag & 0x0FFF;
					if(oFlag == OTYPE_DOUBLE)
						logASM << "double\r\n";
					if(oFlag == OTYPE_LONG)
						logASM << "long\r\n";
					if(oFlag == OTYPE_INT)
						logASM << "int\r\n";
				}else{
					logASM << (retFlag&0x0FFF) << "\r\n";
				}

				logASM << "call function" << valind << "\r\n";
				logASM << "mov edi, ebp ; восстановили предыдущий размер стека переменных\r\n";
				logASM << "pop ebp ; восстановили предыдущую базу стека переменных\r\n";
				if(retFlag & bitRetSimple)
				{
					oFlag = retFlag & 0x0FFF;
					if(oFlag == OTYPE_INT)
						logASM << "push eax ; поместим int обратно в стек\r\n";
					if(oFlag == OTYPE_DOUBLE)
					{
						logASM << "push eax ; \r\n";
						logASM << "push edx ; поместим double обратно в стек\r\n";
					}
					if(oFlag == OTYPE_LONG)
					{
						logASM << "push eax ; \r\n";
						logASM << "push edx ; поместим long обратно в стек\r\n";
					}
				}else{
					if(retFlag != 0)
						throw std::string("Complex type return is not supported [call]");
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
					logASM << "mov ecx, " << 0xffffffff << " ; укажем, вышли за пределы функции\r\n";
					logASM << "int 3 ; остановим выполнение\r\n";
					break;
				}
				if(retFlag == 0)
				{
					logASM << "ret ; возвращаемся из функции\r\n";
					break;
				}
				if(retFlag & bitRetSimple)
				{
					oFlag = retFlag & 0x0FFF;
					if(oFlag == OTYPE_DOUBLE)
					{
						logASM << "pop edx \r\n";
						logASM << "pop eax ; на время поместим double в регистры\r\n";
					}else if(oFlag == OTYPE_LONG){
						logASM << "pop edx \r\n";
						logASM << "pop eax ; на время поместим long в регистры\r\n";
					}else if(oFlag == OTYPE_INT){
						logASM << "pop eax ; на время поместим int в регистр\r\n";
					}
					for(int pops = 0; pops < popCnt-1; pops++)
					{
						logASM << "mov edi, ebp ; восстановили предыдущий размер стека переменных\r\n";
						logASM << "pop ebp ; восстановили предыдущую базу стека переменных\r\n";
					}
					if(popCnt == 0)
						logASM << "mov ebx, " << (UINT)(oFlag) << " ; поместим oFlag чтобы снаружи знали, какой тип вернулся\r\n";
				}else{
					throw std::string("Complex type return is not supported");
				}
				logASM << "ret ; возвращаемся из функции\r\n";
			}
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
			logASM << "  ; CTI " << valind << "\r\n";
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << "fld qword [esp] \r\n";
				logASM << "fistp dword[esp+4] \r\n";
				logASM << "pop eax ; заменили double int'ом\r\n";
				break;
			case OTYPE_LONG:
				logASM << "pop edx ; убрали старшие биты long со стека\r\n";
				break;
			case OTYPE_INT:
				break;
			}
			if(valind != 1)
			{
				//look at the next command
				cmdList->GetData(pos, cmdNext);
				if(cmdNext != cmdCopy && (valind == 2 || valind == 4 || valind == 8))
				{
					mulByVarSize = true;
					lastVarSize = valind;
				}else if(valind == 2)
				{
					logASM << "shl dword [esp], 1 ; умножим адрес на размер переменной\r\n";
				}else if(valind == 4){
					logASM << "shl dword [esp], 2 ; умножим адрес на размер переменной\r\n";
				}else if(valind == 8){
					logASM << "shl dword [esp], 3 ; умножим адрес на размер переменной\r\n";
				}else if(valind == 16){
					logASM << "shl dword [esp], 4 ; умножим адрес на размер переменной\r\n";
				}else{
					logASM << "pop eax ; расчёт в eax\r\n";
					logASM << "imul eax, " << valind << " ; умножим адрес на размер переменной\r\n";
					logASM << "push eax \r\n";
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
				if(flagAddrAbs(cFlag))
					needEBP = texts[0];
				UINT numEDX = 0;

				// Если читается из переменной и...
				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagShiftStk(cFlag))
				{
					// ...есть адрес в команде и имеется сдвиг в стеке
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(mulByVarSize)
					{
						logASM << "pop eax ; взяли сдвиг\r\n";
						if(valind != 0)
							logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
						else
							logASM << "lea edx, [eax*" << lastVarSize << "] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
					}else{
						if(valind != 0)
						{
							logASM << "pop eax ; взяли сдвиг\r\n";
							logASM << "lea edx, [eax + " << valind << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
						}else{
							logASM << "pop edx ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
						}
					}
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)))
				{
					// ...есть адрес в команде
					cmdList->GetINT(pos, valind);
					pos += 4;

					needEDX = texts[0];
					numEDX = valind;
				}

				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, size);
					pos += 4;
					logASM << "cmp eax, " << (mulByVarSize ? size/lastVarSize : size) << " ; сравним сдвиг с максимальным\r\n";
					logASM << "jb pushLabel" << pushLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "int 3 \r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					pushLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp [esp], eax ; сравним с максимальным сдвигом в стеке\r\n";
					logASM << "ja pushLabel" << pushLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "int 3 \r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					logASM << "pop eax ; убрали использованный размер\r\n";
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
						logASM << "push " << highDW << " ; положили double или long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						// Кладём флоат как double
						cmdList->GetUINT(pos, lowDW); pos += 4;
						double res = (double)*((float*)(&lowDW));
						logASM << "push " << *((UINT*)(&res)+1) << "\r\n";
						logASM << "push " << *((UINT*)(&res)) << " ; положили float как double\r\n";
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
					//look at the next command
					cmdList->GetData(pos, cmdNext);
					if(dt == DTYPE_DOUBLE)
					{
						if(cmdNext >= cmdAdd && cmdNext <= cmdNEqual)// && !skipFldESPOnDoubleALU)
						{
							skipFldESPOnDoubleALU = true;
							logASM << "fld qword [" << needEDX << needEBP << paramBase+numEDX << "] ; положили double прямо в FPU\r\n";
						}else{
							logASM << "push dword [" << needEDX << needEBP << paramBase+4+numEDX << "]\r\n";
							logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX << "] ; положили double\r\n";
						}
					}
					if(dt == DTYPE_LONG)
					{
						logASM << "push dword [" << needEDX << needEBP << paramBase+4+numEDX << "]\r\n";
						logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX << "] ; положили long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						logASM << "sub esp, 8 ; освободим место под double\r\n";
						logASM << "fld dword [" << needEDX << needEBP << paramBase+numEDX << "] ; поместим float в fpu стек\r\n";
						logASM << "fstp qword [esp] ; поместим double в обычный стек\r\n";
					}
					if(dt == DTYPE_INT)
					{
						if(cmdNext >= cmdAdd && cmdNext <= cmdLogOr) // for binary commands except LogicalXOR
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
				int valind = -1, /*shift, */size;
				UINT	highDW = 0, lowDW = 0;
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				st = flagStackType(cFlag);
				dt = flagDataType(cFlag);

				UINT numEDX = 0;
				bool knownEDX = false;

				// Если имеется сдвиг в стеке
				if(flagShiftStk(cFlag))
				{
					cmdList->GetINT(pos, valind);
					pos += 4;
					if(mulByVarSize)
					{
						logASM << "pop eax ; взяли сдвиг\r\n";
						if(valind != 0)
							logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
						else
							logASM << "lea edx, [eax*" << lastVarSize << "] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
					}else{
						if(valind != 0)
						{
							logASM << "pop eax ; взяли сдвиг\r\n";
							logASM << "lea edx, [eax + " << valind << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
						}else{
							logASM << "pop edx ; взяли сдвиг\r\n";
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
					logASM << "cmp eax, " << (mulByVarSize ? size/lastVarSize : size) << " ; сравним сдвиг с максимальным\r\n";
					logASM << "jb movLabel" << movLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "int 3 \r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					movLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp [esp], eax ; сравним с максимальным сдвигом в стеке\r\n";
					logASM << "ja movLabel" << movLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
					logASM << "int 3 \r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					logASM << "pop eax ; убрали использованный размер\r\n";
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
				if(flagAddrAbs(cFlag))
					needEBP = dontNeed;
				if(flagAddrRelTop(cFlag))
					needEBP = useEDI;

				UINT final = paramBase+numEDX;

				//look at the next command
				cmdList->GetData(pos, cmdNext);
				if(cmdNext == cmdPop)
					skipPop = true;

				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					if(skipPop)
					{
						logASM << "pop dword [" << needEDX << needEBP << final << "] \r\n";
						logASM << "pop dword [" << needEDX << needEBP << final+4 << "] ; присвоили double или long long переменной.\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov ecx, [esp+4] \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx \r\n";
						logASM << "mov [" << needEDX << needEBP << final+4 << "], ecx ; присвоили double или long long переменной.\r\n";
					}
				}
				if(dt == DTYPE_FLOAT)
				{
					logASM << "fld qword [esp] ; поместим double из стека в fpu стек\r\n";
					logASM << "fstp dword [" << needEDX << needEBP << final << "] ; присвоили float переменной\r\n";
					if(skipPop)
						logASM << "add esp, 8 ;\r\n";
				}
				if(dt == DTYPE_INT)
				{
					if(skipPop)
					{
						logASM << "pop dword [" << needEDX << needEBP << final << "] ; присвоили int переменной\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx ; присвоили int переменной\r\n";
					}
				}
				if(dt == DTYPE_SHORT)
				{
					if(skipPop)
					{
						logASM << "pop ebx \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; присвоили short переменной\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; присвоили short переменной\r\n";
					}
				}
				if(dt == DTYPE_CHAR)
				{
					if(skipPop)
					{
						logASM << "pop ebx \r\n";
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
			if(skipPop)
			{
				skipPop = false;
				break;
			}

			st = flagStackType(cFlag);
			if(st == STYPE_DOUBLE || st == STYPE_LONG)
				logASM << "add esp, 8 ; убрали double или long\r\n";
			else
				logASM << "pop eax ; убрали int\r\n";
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
				logASM << "pop ebx \r\n";
				logASM << "pop ebx ; убрали double со стека\r\n";
				logASM << "test ah, 44h ; MSVS с чем-то сравнивает\r\n";
				logASM << "jnp gLabel" << valind << "\r\n";
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
				logASM << "pop ebx \r\n";
				logASM << "pop ebx ; убрали double со стека\r\n";
				logASM << "test ah, 44h ; MSVS с чем-то сравнивает\r\n";
				logASM << "jp gLabel" << valind << "\r\n";
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
		case cmdSetRange:
			logASM << "  ; SETRANGE\r\n";
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			cmdList->GetUINT(pos, valind2);
			pos += 4;
			logASM << "lea ebx, [ebp + " << paramBase+valind << "] ; начальный адрес\r\n";
			logASM << "lea ecx, [ebp + " << paramBase+valind+valind2*typeSizeD[(cFlag>>2)&0x00000007] << "] ; конечный адрес\r\n";
			if(cFlag == DTYPE_FLOAT)
			{
				logASM << "fld qword [esp] ; float в стек\r\n";
			}else{
				logASM << "mov eax, [esp] \r\n";
				logASM << "mov edx, [esp+4] ; переменную в регистры\r\n";
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
				// Нужно сконвертировать float в дабл
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
			logASM << "add ebx, " << typeSizeD[(cFlag>>2)&0x00000007] << " ; сдвинем указатель на следующий элемент\r\n";
			logASM << "jmp loopStart" << aluLabels << " \r\n";
			logASM << "  loopEnd" << aluLabels << ": \r\n";
			if(cFlag == DTYPE_FLOAT)
				logASM << "fstp st0 ; float из стека\r\n";
			aluLabels++;
			break;
		case cmdGetAddr:
			logASM << "  ; GETADDR\r\n";
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << "lea eax, [ebp + " << valind << "] ; сдвинули адрес относительно бызы стека\r\n";
			logASM << "push eax ; положили адрес в стек\r\n";
			break;
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
				logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "faddp \r\n";
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
				logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "fsubrp \r\n";
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
				logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "fmulp \r\n";
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
				logASM << "add esp, 8 ; сносим один\r\n";
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
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "fdivrp \r\n";
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
				logASM << "add esp, 8 ; сносим один\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdDiv+(OTYPE_INT<<16):
				logASM << "  ; DIV int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp] \r\n";
				logASM << "cdq \r\n";
				logASM << "idiv dword [esp] ; а проверка на 0?\r\n";
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
				logASM << "add esp, 8 ; сносим один\r\n";
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
				logASM << "add esp, 8 ; сносим один\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdMod+(OTYPE_INT<<16):
				logASM << "  ; MOD int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp] \r\n";
				logASM << "cdq \r\n";
				logASM << "idiv dword [esp] ; а проверка на 0?\r\n";
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
				logASM << "fnstsw ax ; взяли флажок\r\n";
				logASM << "test ah, 41h ; сравнили с 'меньше'\r\n";
				logASM << "jne pushZero" << aluLabels << " ; не, не меньше\r\n";
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
				logASM << "fnstsw ax ; взяли флажок\r\n";
				logASM << "test ah, 5h ; сравнили с 'больше'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; не, не больше\r\n";
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
				logASM << "fnstsw ax ; взяли флажок\r\n";
				logASM << "test ah, 1h ; сравнили с 'меньше или равно'\r\n";
				logASM << "jne pushZero" << aluLabels << " ; не, не меньше или равно\r\n";
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
				logASM << "fnstsw ax ; взяли флажок\r\n";
				logASM << "test ah, 41h ; сравнили с 'больше или равно'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; не, не больше или равно\r\n";
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
				logASM << "fnstsw ax ; взяли флажок\r\n";
				logASM << "test ah, 44h ; сравнили с 'равно'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; не, не равно\r\n";
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
				logASM << "fnstsw ax ; взяли флажок\r\n";
				logASM << "test ah, 44h ; сравнили с 'неравно'\r\n";
				logASM << "jnp pushZero" << aluLabels << " ; не, не неравно\r\n";
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
				logASM << "add esp, 8 ; сносим один\r\n";
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
				logASM << "add esp, 8 ; сносим один\r\n";
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
				logASM << "fcomp qword [esp] ; сравнили\r\n";
				logASM << "fnstsw ax ; взяли флажок\r\n";
				logASM << "test ah, 44h ; сравнили с 'равно'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; не, не равно\r\n";
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
			int valind = -1, /*shift, */size;
			UINT	highDW = 0, lowDW = 0;
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
			
			// Если имеется сдвиг в стеке
			if(flagShiftStk(cFlag))
			{
				cmdList->GetINT(pos, valind);
				pos += 4;
				if(mulByVarSize)
				{
					logASM << "pop eax ; взяли сдвиг\r\n";
					if(valind != 0)
						logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					else
						logASM << "lea edx, [eax*" << lastVarSize << "] ; возмём указатель на стек переменных и сдвинем на число в стеке (opt: addr==0)\r\n";
				}else{
					if(valind != 0)
					{
						logASM << "pop eax ; взяли сдвиг\r\n";
						logASM << "lea edx, [eax + " << valind << "] ; возмём указатель на стек переменных и сдвинем на число в стеке и по константному сдвигу\r\n";
					}else{
						logASM << "pop edx ; взяли сдвиг\r\n";
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
				logASM << "cmp eax, " << (mulByVarSize ? size/lastVarSize : size) << " ; сравним сдвиг с максимальным\r\n";
				logASM << "jb movLabel" << movLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
				logASM << "int 3 \r\n";
				logASM << "  movLabel" << movLabels << ":\r\n";
				movLabels++;
			}
			if(flagSizeStk(cFlag))
			{
				logASM << "cmp [esp], eax ; сравним с максимальным сдвигом в стеке\r\n";
				logASM << "ja movLabel" << movLabels << " ; если сдвиг меньше максимума (и не отрицательный) то всё ок\r\n";
				logASM << "int 3 \r\n";
				logASM << "  movLabel" << movLabels << ":\r\n";
				logASM << "pop eax ; убрали использованный размер\r\n";
				movLabels++;
			}
			mulByVarSize = false;

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
				logASM << "fld qword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "fld1 \r\n";
				logASM << "faddp \r\n";
				logASM << "fstp qword [" << needEDX << needEBP << final << "] ;\r\n";
				break;
			case cmdIncAt+(DTYPE_FLOAT<<16):
				logASM << "fld dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "fld1 \r\n";
				logASM << "faddp \r\n";
				logASM << "fstp dword [" << needEDX << needEBP << final << "] ;\r\n";
				break;
			case cmdIncAt+(DTYPE_LONG<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "mov edx, dword [" << needEDX << needEBP << final+4 << "] ;\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "adc edx, 0 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				logASM << "mov dword [" << needEDX << needEBP << final+4 << "], edx ;\r\n";
				break;
			case cmdIncAt+(DTYPE_INT<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				break;
			case cmdIncAt+(DTYPE_SHORT<<16):
				logASM << "movsx eax, word [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov word [" << needEDX << needEBP << final << "], ax ;\r\n";
				break;
			case cmdIncAt+(DTYPE_CHAR<<16):
				logASM << "movsx eax, byte [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov byte [" << needEDX << needEBP << final << "], al ;\r\n";
				break;

			case cmdDecAt+(DTYPE_DOUBLE<<16):
				logASM << "fld qword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "fld1 \r\n";
				logASM << "fsubp \r\n";
				logASM << "fstp qword [" << needEDX << needEBP << final << "] ;\r\n";
				break;
			case cmdDecAt+(DTYPE_FLOAT<<16):
				logASM << "fld dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "fld1 \r\n";
				logASM << "fsubp \r\n";
				logASM << "fstp dword [" << needEDX << needEBP << final << "] ;\r\n";
				break;
			case cmdDecAt+(DTYPE_LONG<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "mov edx, dword [" << needEDX << needEBP << final+4 << "] ;\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "sbb edx, 0 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				logASM << "mov dword [" << needEDX << needEBP << final+4 << "], edx ;\r\n";
				break;
			case cmdDecAt+(DTYPE_INT<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				break;
			case cmdDecAt+(DTYPE_SHORT<<16):
				logASM << "movsx eax, word [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov word [" << needEDX << needEBP << final << "], ax ;\r\n";
				break;
			case cmdDecAt+(DTYPE_CHAR<<16):
				logASM << "movsx eax, byte [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov byte [" << needEDX << needEBP << final << "], al ;\r\n";
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

void ExecutorX86::SetOptimization(bool toggle)
{
	optimize = toggle;
}

bool ExecutorX86::GetSimpleTypeInfo(ostringstream &varstr, TypeInfo* type, int address)
{
	if(type->arrSize != 1)
		varstr << "{ ";
	for(UINT n = 0; n < type->arrSize; n++, address += (type->subType ? type->subType->size : type->size))
	{
		if(n > 100)
			continue;
		if(type->type == TypeInfo::TYPE_INT)
		{
			varstr << *((int*)&paramData[address]);
		}else if(type->type == TypeInfo::TYPE_SHORT)
		{
			varstr << *((short*)&paramData[address]);
		}else if(type->type == TypeInfo::TYPE_CHAR)
		{
			if(*((unsigned char*)&paramData[address]))
				varstr << "'" << *((unsigned char*)&paramData[address]) << "' (" << (int)(*((unsigned char*)&paramData[address])) << ")";
			else
				varstr << '0';
		}else if(type->type == TypeInfo::TYPE_FLOAT)
		{
			varstr << *((float*)&paramData[address]);
		}else if(type->type == TypeInfo::TYPE_LONG)
		{
			varstr << *((long long*)&paramData[address]);
		}else if(type->type == TypeInfo::TYPE_DOUBLE)
		{
			varstr << *((double*)&paramData[address]);
		}else{
			return false;
		}
		if(n != type->arrSize-1)
			varstr << ", ";
	}
	if(type->arrSize != 1)
		varstr << " }";
	varstr << "\r\n";
	return true;
}

void ExecutorX86::GetComplexTypeInfo(ostringstream &varstr, TypeInfo* type, int address)
{
	TypeInfo* subType = type;
	if(type->arrLevel != 0)
		subType = type->subType;
	for(UINT n = 0; n < type->arrSize; n++, address += subType->size)
	{
		if(n > 100)
			continue;
		for(UINT mn = 0; mn < subType->memberData.size(); mn++)
		{
			varstr << "  " << subType->memberData[mn].type->GetTypeName() << " " << subType->memberData[mn].name << " = ";
			if(subType->memberData[mn].type->type == TypeInfo::TYPE_VOID)
			{
				varstr << "ERROR: This type is void";
			}else if(subType->memberData[mn].type->type == TypeInfo::TYPE_COMPLEX)
			{
				varstr << "\r\n";
				GetComplexTypeInfo(varstr, subType->memberData[mn].type, address+subType->memberData[mn].offset);
			}else{
				if(!GetSimpleTypeInfo(varstr, subType->memberData[mn].type, address+subType->memberData[mn].offset))
					throw std::string("Executor::GetComplexTypeInfo() ERROR: unknown type of variable ") + subType->memberData[mn].name;
			}
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
		varstr << address << ":" << (varInfo[i].isConst ? "const " : "") << *varInfo[i].varType << " " << varInfo[i].name;

		varstr << " = ";
		if(varInfo[i].varType->type == TypeInfo::TYPE_VOID)
		{
			varstr << "ERROR: This type is void";
		}else if(varInfo[i].varType->type == TypeInfo::TYPE_COMPLEX)
		{
			varstr << "\r\n";
			GetComplexTypeInfo(varstr, varInfo[i].varType, address);
		}else{
			if(!GetSimpleTypeInfo(varstr, varInfo[i].varType, address))
				throw std::string("Executor::GetVarInfo() ERROR: unknown type of variable ") + varInfo[i].name;
		}
		address += varInfo[i].varType->size;
	}
	return varstr.str();
}