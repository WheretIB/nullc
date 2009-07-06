#include "stdafx.h"
#ifdef NULLC_BUILD_X86_JIT

#include "Executor_X86.h"
#include "StdLib_X86.h"
#include "Translator_X86.h"
#include "Optimizer_x86.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

unsigned int paramDataBase;
unsigned int reservedStack;
unsigned int commitedStack;
unsigned int stackGrowSize;
unsigned int stackGrowCommit;

ExecutorX86::ExecutorX86(Linker *linker): exLinker(linker), exFunctions(linker->exFunctions),
			exFuncInfo(linker->exFuncInfo), exCode(linker->exCode), exTypes(linker->exTypes)
{
	binCode = NULL;
}
ExecutorX86::~ExecutorX86()
{
	VirtualFree(reinterpret_cast<void*>(0x20000000), 0, MEM_RELEASE);

	delete[] binCode;
}

int runResult = 0;
int runResult2 = 0;
asmOperType runResultType = OTYPE_DOUBLE;

unsigned int stackReallocs;

unsigned int expCodePublic;
unsigned int expAllocCode;
unsigned int expECXstate;
DWORD CanWeHandleSEH(unsigned int expCode, _EXCEPTION_POINTERS* expInfo)
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

			return (DWORD)EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	return (DWORD)EXCEPTION_CONTINUE_SEARCH;
}

bool ExecutorX86::Initialize() throw()
{
	stackGrowSize = 128*4096;
	stackGrowCommit = 64*4096;

	// Request memory at address
	if(NULL == (paramData = (char*)VirtualAlloc(reinterpret_cast<void*>(0x20000000), stackGrowSize, MEM_RESERVE, PAGE_NOACCESS)))
	{
		strcpy(execError, "ERROR: Failed to reserve memory");
		return false;
	}
	if(!VirtualAlloc(reinterpret_cast<void*>(0x20000000), stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
	{
		strcpy(execError, "ERROR: Failed to commit memory");
		return false;
	}

	reservedStack = stackGrowSize;
	commitedStack = stackGrowCommit;
	
	paramDataBase = paramBase = static_cast<unsigned int>(reinterpret_cast<long long>(paramData));

	binCode = new unsigned char[200000];
	memset(binCode, 0x90, 20);
	binCodeStart = static_cast<unsigned int>(reinterpret_cast<long long>(&binCode[20]));
	binCodeSize = 0;

	return true;
}

char* InlFmt(const char *str, ...)
{
	static char storage[64];
	va_list args;
	va_start(args, str);
	vsprintf(storage, str, args); 
	return storage;
}

#pragma warning(disable: 4731)
void ExecutorX86::Run(const char* funcName) throw()
{
	if(!exCode.size())
	{
		strcpy(execError, "ERROR: no code to run");
		return;
	}

	execError[0] = 0;

	stackReallocs = 0;

	*(double*)(paramData) = 0.0;
	*(double*)(paramData+8) = 3.1415926535897932384626433832795;
	*(double*)(paramData+16) = 2.7182818284590452353602874713527;

	unsigned int binCodeStart = static_cast<unsigned int>(reinterpret_cast<long long>(&binCode[20]));

	if(funcName)
	{
		unsigned int funcPos = (unsigned int)-1;
		unsigned int fnameHash = GetStringHash(funcName);
		for(int i = (int)exFunctions.size()-1; i >= 0; i--)
		{
			if(exFunctions[i]->nameHash == fnameHash)
			{
				funcPos = exFuncInfo[i].startInByteCode;
				break;
			}
		}
		if(funcPos == -1)
		{
			strcpy(execError, "Cannot find starting function");
			return;
		}
		binCodeStart += funcPos;
	}else{
		binCodeStart += globalStartInBytecode;
	}

	unsigned int varSize = exLinker->globalVarSize;

	unsigned int res1 = 0;
	unsigned int res2 = 0;
	unsigned int resT = 0;
	__try 
	{
		__asm
		{
			pusha ; // Сохраним все регистры
			mov eax, binCodeStart ;
			
			// Выравниваем стек на границу 8 байт
			lea ebx, [esp+8];
			and ebx, 0fh;
			mov ecx, 16;
			sub ecx, ebx;
			sub esp, ecx;

			push ecx; // Сохраним на сколько сдвинули стек
			push ebp; // Сохраним базу стека (её придётся востановить до popa)

			mov edi, varSize ;
			mov ebp, 0h ;

			call eax ; // в ebx тип вернувшегося значения

			pop ebp; // Востановим базу стека
			pop ecx;
			add esp, ecx;

			mov dword ptr [res1], eax;
			mov dword ptr [res2], edx;
			mov dword ptr [resT], ebx;

			popa ;
		}
	}__except(CanWeHandleSEH(GetExceptionCode(), GetExceptionInformation())){
		if(expCodePublic == EXCEPTION_INT_DIVIDE_BY_ZERO)
			strcpy(execError, "ERROR: integer division by zero");
		if(expCodePublic == EXCEPTION_BREAKPOINT && expECXstate != 0xFFFFFFFF)
			strcpy(execError, "ERROR: array index out of bounds");
		if(expCodePublic == EXCEPTION_BREAKPOINT && expECXstate == 0xFFFFFFFF)
			strcpy(execError, "ERROR: function didn't return a value");
		if(expCodePublic == EXCEPTION_STACK_OVERFLOW)
			strcpy(execError, "ERROR: stack overflow");
		if(expCodePublic == EXCEPTION_ACCESS_VIOLATION)
		{
			if(expAllocCode == 1)
				strcpy(execError, "ERROR: Failed to commit old stack memory");
			if(expAllocCode == 2)
				strcpy(execError, "ERROR: Failed to reserve new stack memory");
			if(expAllocCode == 3)
				strcpy(execError, "ERROR: Failed to commit new stack memory");
			if(expAllocCode == 4)
				strcpy(execError, "ERROR: No more memory (512Mb maximum exceeded)");
		}
	}

	runResult = res1;
	runResult2 = res2;
	runResultType = (asmOperType)resT;
}
#pragma warning(default: 4731)

bool ExecutorX86::TranslateToNative()
{
	execError[0] = 0;

	unsigned int pos = 0;

	vector<unsigned int> instrNeedLabel;	// нужен ли перед инструкцией лейбл метки
	vector<unsigned int> funcNeedLabel;	// нужен ли перед инструкцией лейбл функции

	globalStartInBytecode = 0xffffffff;
	for(unsigned int i = 0; i < exFunctions.size(); i++)
	{
		exFuncInfo[i].startInByteCode = 0xffffffff;
		if(exFunctions[i]->funcPtr == NULL && exFunctions[i]->address != -1)
			funcNeedLabel.push_back(exFunctions[i]->address);
	}

	//Узнаем, кому нужны лейблы
	while(pos < exCode.size())
	{
		const VMCmd &cmd = exCode[pos];
		if(cmd.cmd >= cmdJmp && cmd.cmd <= cmdJmpNZL)
			instrNeedLabel.push_back(cmd.argument);
		pos++;
	}

#define Emit instList.push_back(x86Instruction()), instList.back() = x86Instruction

	instList.clear();

	Emit(o_use32);
	unsigned int typeSizeD[] = { 1, 2, 4, 8, 4, 8 };

	int aluLabels = 1;
	int stackRelSize = 0, stackRelSizePrev = 0;

	pos = 0;
	while(pos < exCode.size())
	{
		const VMCmd &cmd = exCode[pos];
		const VMCmd &cmdNext = exCode[pos+1];
		for(unsigned int i = 0; i < instrNeedLabel.size(); i++)
		{
			if(pos == instrNeedLabel[i])
			{
				Emit(InlFmt("gLabel%d", pos));
				break;
			}
		}
		for(unsigned int i = 0; i < funcNeedLabel.size(); i++)
		{
			if(pos == funcNeedLabel[i])
			{
				Emit(o_dd, x86Argument((('N' << 24) | pos)));
				Emit(InlFmt("function%d", pos));
				break;
			}
		}

		if(pos == exLinker->offsetToGlobalCode)
		{
			Emit(o_dd, x86Argument((('G' << 24) | exLinker->offsetToGlobalCode)));
			Emit(o_push, x86Argument(rEBP));
		}

		stackRelSizePrev = stackRelSize;

	//	const char *descStr = cmdList->GetDescription(pos2);
	//	if(descStr)
	//		logASM << "\r\n  ; \"" << descStr << "\" codeinfo\r\n";
		pos++;

		switch(cmd.cmd)
		{
		case cmdNop:
			Emit(o_nop);
			break;
		case cmdPushCharAbs:
			Emit(INST_COMMENT, "PUSH char abs");

			Emit(o_movsx, x86Argument(rEAX), x86Argument(sBYTE, cmd.argument+paramBase));
			Emit(o_push, x86Argument(rEAX));
			stackRelSize += 4;
			break;
		case cmdPushShortAbs:
			Emit(INST_COMMENT, "PUSH short abs");

			Emit(o_movsx, x86Argument(rEAX), x86Argument(sWORD, cmd.argument+paramBase));
			Emit(o_push, x86Argument(rEAX));
			stackRelSize += 4;
			break;
		case cmdPushIntAbs:
			Emit(INST_COMMENT, "PUSH int abs");

			Emit(o_push, x86Argument(sDWORD, cmd.argument+paramBase));
			stackRelSize += 4;
			break;
		case cmdPushFloatAbs:
			Emit(INST_COMMENT, "PUSH float abs");

			Emit(o_sub, x86Argument(rESP), x86Argument(8));
			Emit(o_fld, x86Argument(sDWORD, cmd.argument+paramBase));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
			stackRelSize += 8;
			break;
		case cmdPushDorLAbs:
			Emit(INST_COMMENT, cmd.flag ? "PUSH double abs" : "MOV long abs");

			Emit(o_push, x86Argument(sDWORD, cmd.argument+paramBase+4));
			Emit(o_push, x86Argument(sDWORD, cmd.argument+paramBase));
			stackRelSize += 8;
			break;
		case cmdPushCmplxAbs:
			Emit(INST_COMMENT, "PUSH complex abs");
		{
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				Emit(o_push, x86Argument(sDWORD, cmd.argument+paramBase+currShift));
				stackRelSize += 4;
			}
			assert(currShift == 0);
		}
			break;

		case cmdPushCharRel:
			Emit(INST_COMMENT, "PUSH char rel");

			Emit(o_movsx, x86Argument(rEAX), x86Argument(sBYTE, rEBP, cmd.argument+paramBase));
			Emit(o_push, x86Argument(rEAX));
			stackRelSize += 4;
			break;
		case cmdPushShortRel:
			Emit(INST_COMMENT, "PUSH short rel");

			Emit(o_movsx, x86Argument(rEAX), x86Argument(sWORD, rEBP, cmd.argument+paramBase));
			Emit(o_push, x86Argument(rEAX));
			stackRelSize += 4;
			break;
		case cmdPushIntRel:
			Emit(INST_COMMENT, "PUSH int rel");

			Emit(o_push, x86Argument(sDWORD, rEBP, cmd.argument+paramBase));
			stackRelSize += 4;
			break;
		case cmdPushFloatRel:
			Emit(INST_COMMENT, "PUSH float rel");

			Emit(o_sub, x86Argument(rESP), x86Argument(8));
			Emit(o_fld, x86Argument(sDWORD, rEBP, cmd.argument+paramBase));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
			stackRelSize += 8;
			break;
		case cmdPushDorLRel:
			Emit(INST_COMMENT, cmd.flag ? "PUSH double rel" : "PUSH long rel");

			Emit(o_push, x86Argument(sDWORD, rEBP, cmd.argument+paramBase+4));
			Emit(o_push, x86Argument(sDWORD, rEBP, cmd.argument+paramBase));
			stackRelSize += 8;
			break;
		case cmdPushCmplxRel:
			Emit(INST_COMMENT, "PUSH complex rel");
		{
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				Emit(o_push, x86Argument(sDWORD, rEBP, cmd.argument+paramBase+currShift));
				stackRelSize += 4;
			}
			assert(currShift == 0);
		}
			break;

		case cmdPushCharStk:
			Emit(INST_COMMENT, "PUSH char stack");

			Emit(o_pop, x86Argument(rEDX));
			Emit(o_movsx, x86Argument(rEAX), x86Argument(sBYTE, rEDX, cmd.argument+paramBase));
			Emit(o_push, x86Argument(rEAX));
			break;
		case cmdPushShortStk:
			Emit(INST_COMMENT, "PUSH short stack");

			Emit(o_pop, x86Argument(rEDX));
			Emit(o_movsx, x86Argument(rEAX), x86Argument(sWORD, rEDX, cmd.argument+paramBase));
			Emit(o_push, x86Argument(rEAX));
			break;
		case cmdPushIntStk:
			Emit(INST_COMMENT, "PUSH int stack");

			Emit(o_pop, x86Argument(rEDX));
			Emit(o_push, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
			break;
		case cmdPushFloatStk:
			Emit(INST_COMMENT, "PUSH float stack");

			Emit(o_pop, x86Argument(rEDX));
			Emit(o_sub, x86Argument(rESP), x86Argument(8));
			Emit(o_fld, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
			stackRelSize += 4;
			break;
		case cmdPushDorLStk:
			Emit(INST_COMMENT, cmd.flag ? "PUSH double stack" : "PUSH long stack");

			Emit(o_pop, x86Argument(rEDX));
			Emit(o_push, x86Argument(sDWORD, rEDX, cmd.argument+paramBase+4));
			Emit(o_push, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
			stackRelSize += 4;
			break;
		case cmdPushCmplxStk:
			Emit(INST_COMMENT, "PUSH complex stack");
		{
			unsigned int currShift = cmd.helper;
			Emit(o_pop, x86Argument(rEDX));
			stackRelSize -= 4;
			while(currShift >= 4)
			{
				currShift -= 4;
				Emit(o_push, x86Argument(sDWORD, rEDX, cmd.argument+paramBase+currShift));
				stackRelSize += 4;
			}
			assert(currShift == 0);
		}
			break;

		case cmdPushImmt:
			Emit(INST_COMMENT, "PUSHIMMT");
			
			if(cmdNext.cmd != cmdSetRange)
			{
				Emit(o_push, x86Argument(cmd.argument));
				stackRelSize += 4;
			}
			break;

		case cmdMovCharAbs:
			Emit(INST_COMMENT, "MOV char abs");
	
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(sBYTE, cmd.argument+paramBase), x86Argument(rEBX));
			break;
		case cmdMovShortAbs:
			Emit(INST_COMMENT, "MOV short abs");
	
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(sWORD, cmd.argument+paramBase), x86Argument(rEBX));
			break;
		case cmdMovIntAbs:
			Emit(INST_COMMENT, "MOV int abs");
	
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(sDWORD, cmd.argument+paramBase), x86Argument(rEBX));
			break;
		case cmdMovFloatAbs:
			Emit(INST_COMMENT, "MOV float abs");

			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sDWORD, cmd.argument+paramBase));
			break;
		case cmdMovDorLAbs:
			Emit(INST_COMMENT, cmd.flag ? "MOV double abs" : "MOV long abs");
	
			if(cmd.flag)
			{
				Emit(o_fld, x86Argument(sQWORD, rESP, 0));
				Emit(o_fstp, x86Argument(sQWORD, cmd.argument+paramBase));
			}else{
				Emit(o_pop, x86Argument(sDWORD, cmd.argument+paramBase));
				Emit(o_pop, x86Argument(sDWORD, cmd.argument+paramBase + 4));
				Emit(o_sub, x86Argument(rESP), x86Argument(8));
			}
			break;
		case cmdMovCmplxAbs:
			Emit(INST_COMMENT, "MOV complex abs");
		{
			unsigned int currShift = 0;
			while(currShift < cmd.helper)
			{
				Emit(o_pop, x86Argument(sDWORD, cmd.argument+paramBase + currShift));
				currShift += 4;
			}
			Emit(o_sub, x86Argument(rESP), x86Argument(cmd.helper));
			assert(currShift == cmd.helper);
		}
			break;

		case cmdMovCharRel:
			Emit(INST_COMMENT, "MOV char rel");
	
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(sBYTE, rEBP, cmd.argument+paramBase), x86Argument(rEBX));
			break;
		case cmdMovShortRel:
			Emit(INST_COMMENT, "MOV short rel");
	
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(sWORD, rEBP, cmd.argument+paramBase), x86Argument(rEBX));
			break;
		case cmdMovIntRel:
			Emit(INST_COMMENT, "MOV int rel");
	
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(sDWORD, rEBP, cmd.argument+paramBase), x86Argument(rEBX));
			break;
		case cmdMovFloatRel:
			Emit(INST_COMMENT, "MOV float rel");

			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sDWORD, rEBP, cmd.argument+paramBase));
			break;
		case cmdMovDorLRel:
			Emit(INST_COMMENT, cmd.flag ? "MOV double rel" : "MOV long rel");
	
			if(cmd.flag)
			{
				Emit(o_fld, x86Argument(sQWORD, rESP, 0));
				Emit(o_fstp, x86Argument(sQWORD, rEBP, cmd.argument+paramBase));
			}else{
				Emit(o_pop, x86Argument(sDWORD, rEBP, cmd.argument+paramBase));
				Emit(o_pop, x86Argument(sDWORD, rEBP, cmd.argument+paramBase + 4));
				Emit(o_sub, x86Argument(rESP), x86Argument(8));
			}
			break;
		case cmdMovCmplxRel:
			Emit(INST_COMMENT, "MOV complex rel");
		{
			unsigned int currShift = 0;
			while(currShift < cmd.helper)
			{
				Emit(o_pop, x86Argument(sDWORD, rEBP, cmd.argument+paramBase + currShift));
				currShift += 4;
			}
			Emit(o_sub, x86Argument(rESP), x86Argument(cmd.helper));
			assert(currShift == cmd.helper);
		}
			break;

		case cmdMovCharStk:
			Emit(INST_COMMENT, "MOV char stack");
	
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(sBYTE, rEDX, cmd.argument+paramBase), x86Argument(rEBX));
			stackRelSize -= 4;
			break;
		case cmdMovShortStk:
			Emit(INST_COMMENT, "MOV short stack");
	
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(sWORD, rEDX, cmd.argument+paramBase), x86Argument(rEBX));
			stackRelSize -= 4;
			break;
		case cmdMovIntStk:
			Emit(INST_COMMENT, "MOV int stack");
	
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(sDWORD, rEDX, cmd.argument+paramBase), x86Argument(rEBX));
			stackRelSize -= 4;
			break;
		case cmdMovFloatStk:
			Emit(INST_COMMENT, "MOV float stack");

			Emit(o_pop, x86Argument(rEDX));
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
			stackRelSize -= 4;
			break;
		case cmdMovDorLStk:
			Emit(INST_COMMENT, cmd.flag ? "MOV double stack" : "MOV long stack");
	
			Emit(o_pop, x86Argument(rEDX));
			if(cmd.flag)
			{
				Emit(o_fld, x86Argument(sQWORD, rESP, 0));
				Emit(o_fstp, x86Argument(sQWORD, rEDX, cmd.argument+paramBase));
			}else{
				Emit(o_pop, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
				Emit(o_pop, x86Argument(sDWORD, rEDX, cmd.argument+paramBase + 4));
				Emit(o_sub, x86Argument(rESP), x86Argument(8));
			}
			stackRelSize -= 4;
			break;
		case cmdMovCmplxStk:
			Emit(INST_COMMENT, "MOV complex stack");
		{
			Emit(o_pop, x86Argument(rEDX));
			stackRelSize -= 4;
			unsigned int currShift = 0;
			while(currShift < cmd.helper)
			{
				Emit(o_pop, x86Argument(sDWORD, rEDX, cmd.argument+paramBase + currShift));
				currShift += 4;
			}
			Emit(o_sub, x86Argument(rESP), x86Argument(cmd.helper));
			assert(currShift == cmd.helper);
		}
			break;

		case cmdReserveV:
			break;

		case cmdPopCharTop:
			Emit(INST_COMMENT, "POP char top");
	
			Emit(o_pop, x86Argument(rEBX));
			Emit(o_mov, x86Argument(sBYTE, rEDI, cmd.argument+paramBase), x86Argument(rEBX));
			stackRelSize -= 4;
			break;
		case cmdPopShortTop:
			Emit(INST_COMMENT, "POP short top");
	
			Emit(o_pop, x86Argument(rEBX));
			Emit(o_mov, x86Argument(sWORD, rEDI, cmd.argument+paramBase), x86Argument(rEBX));
			stackRelSize -= 4;
			break;
		case cmdPopIntTop:
			Emit(INST_COMMENT, "POP int top");
	
			Emit(o_pop, x86Argument(sDWORD, rEDI, cmd.argument+paramBase));
			stackRelSize -= 4;
			break;
		case cmdPopFloatTop:
			Emit(INST_COMMENT, "POP float top");

			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sDWORD, rEDI, cmd.argument+paramBase));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			stackRelSize -= 8;
			break;
		case cmdPopDorLTop:
			Emit(INST_COMMENT, cmd.flag ? "POP double top" : "POP long top");
	
			Emit(o_pop, x86Argument(sDWORD, rEDI, cmd.argument+paramBase));
			Emit(o_pop, x86Argument(sDWORD, rEDI, cmd.argument+paramBase + 4));
			stackRelSize -= 8;
			break;
		case cmdPopCmplxTop:
			Emit(INST_COMMENT, "POP complex top");
		{
			unsigned int currShift = 0;
			while(currShift < cmd.helper)
			{
				Emit(o_pop, x86Argument(sDWORD, rEDI, cmd.argument+paramBase + currShift));
				currShift += 4;
			}
			assert(currShift == cmd.helper);
			stackRelSize -= cmd.helper;
		}
			break;


		case cmdPop:
			Emit(INST_COMMENT, "POP");

			Emit(o_add, x86Argument(rESP), x86Argument(cmd.argument));
			stackRelSize -= cmd.argument;
			break;

		case cmdDtoI:
			Emit(INST_COMMENT, "DTOI");

			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fistp, x86Argument(sDWORD, rESP, 4));
			Emit(o_add, x86Argument(rESP), x86Argument(4));
			stackRelSize -= 4;
			break;
		case cmdDtoL:
			Emit(INST_COMMENT, "DTOL");

			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fistp, x86Argument(sQWORD, rESP, 0));
			break;
		case cmdDtoF:
			Emit(INST_COMMENT, "DTOF");

			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sDWORD, rESP, 4));
			Emit(o_add, x86Argument(rESP), x86Argument(4));
			stackRelSize -= 4;
			break;
		case cmdItoD:
			Emit(INST_COMMENT, "ITOD");

			Emit(o_fild, x86Argument(sDWORD, rESP, 0));
			Emit(o_push, x86Argument(rEAX));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
			stackRelSize += 4;
			break;
		case cmdLtoD:
			Emit(INST_COMMENT, "LTOD");

			Emit(o_fild, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
			break;
		case cmdItoL:
			Emit(INST_COMMENT, "ITOL");

			Emit(o_pop, x86Argument(rEAX));
			Emit(o_cdq);
			Emit(o_push, x86Argument(rEDX));
			Emit(o_push, x86Argument(rEAX));
			stackRelSize += 4;
			break;
		case cmdLtoI:
			Emit(INST_COMMENT, "LTOI");

			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xchg, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
			stackRelSize -= 4;
			break;

		case cmdImmtMulD:
		case cmdImmtMulL:
		case cmdImmtMulI:
			Emit(INST_COMMENT, cmd.cmd == cmdImmtMulD ? "IMUL double" : (cmd.cmd == cmdImmtMulL ? "IMUL long" : "IMUL int"));
			
			if(cmd.cmd == cmdImmtMulD)
			{
				Emit(o_fld, x86Argument(sQWORD, rESP, 0));
				Emit(o_fistp, x86Argument(sDWORD, rESP, 4));
				Emit(o_pop, x86Argument(rEAX));
				stackRelSize -= 4;
			}else if(cmd.cmd == cmdImmtMulL){
				Emit(o_pop, x86Argument(rEAX));
				stackRelSize -= 4;
			}
			
			if(cmd.argument == 2)
			{
				Emit(o_shl, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			}else if(cmd.argument == 4){
				Emit(o_shl, x86Argument(sDWORD, rESP, 0), x86Argument(2));
			}else if(cmd.argument == 8){
				Emit(o_shl, x86Argument(sDWORD, rESP, 0), x86Argument(3));
			}else if(cmd.argument == 16){
				Emit(o_shl, x86Argument(sDWORD, rESP, 0), x86Argument(4));
			}else{
				Emit(o_pop, x86Argument(rEAX));
				Emit(o_imul, x86Argument(rEAX), x86Argument(cmd.argument));
				Emit(o_push, x86Argument(rEAX));
			}
			break;

		case cmdCopyDorL:
			Emit(INST_COMMENT, "COPY qword");

			Emit(o_mov, x86Argument(rEDX), x86Argument(sDWORD, rESP, 0));
			Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rESP, 4));
			Emit(o_push, x86Argument(rEAX));
			Emit(o_push, x86Argument(rEDX));
			stackRelSize += 8;
			break;
		case cmdCopyI:
			Emit(INST_COMMENT, "COPY dword");

			Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
			Emit(o_push, x86Argument(rEAX));
			stackRelSize += 4;
			break;

		case cmdGetAddr:
			Emit(INST_COMMENT, "GETADDR");

			if(cmd.argument)
			{
				Emit(o_lea, x86Argument(rEAX), x86Argument(sDWORD, rEBP, cmd.argument));
				Emit(o_push, x86Argument(rEAX));
			}else{
				Emit(o_push, x86Argument(rEBP));
			}
			stackRelSize += 4;
			break;
		case cmdFuncAddr:
			Emit(INST_COMMENT, "FUNCADDR");

			if(exFunctions[cmd.argument]->funcPtr == NULL)
			{
				Emit(o_lea, x86Argument(rEAX), x86Argument(InlFmt("function%d", exFunctions[cmd.argument]->address), binCodeStart));
				Emit(o_push, x86Argument(rEAX));
			}else{
				Emit(o_push, x86Argument((int)(long long)exFunctions[cmd.argument]->funcPtr));
			}
			stackRelSize += 4;
			break;

		case cmdSetRange:
			Emit(INST_COMMENT, "SETRANGE");

			assert(exCode[pos-2].cmd == cmdPushImmt);	// previous command must be cmdPushImmt

			// start address
			Emit(o_lea, x86Argument(rEBX), x86Argument(sDWORD, rEBP, paramBase + cmd.argument));
			// end address
			Emit(o_lea, x86Argument(rECX), x86Argument(sDWORD, rEBP, paramBase + cmd.argument + (exCode[pos-2].argument - 1) * typeSizeD[(cmd.helper>>2)&0x00000007]));
			if(cmd.helper == DTYPE_FLOAT)
			{
				Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			}else{
				Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
				Emit(o_mov, x86Argument(rEDX), x86Argument(sDWORD, rESP, 4));
			}
			Emit(InlFmt("loopStart%d", aluLabels));
			Emit(o_cmp, x86Argument(rEBX), x86Argument(rECX));
			Emit(o_jg, x86Argument(InlFmt("loopEnd%d", aluLabels)));

			switch(cmd.helper)
			{
			case DTYPE_DOUBLE:
			case DTYPE_LONG:
				Emit(o_mov, x86Argument(sDWORD, rEBX, 4), x86Argument(rEDX));
				Emit(o_mov, x86Argument(sDWORD, rEBX, 0), x86Argument(rEAX));
				break;
			case DTYPE_FLOAT:
				Emit(o_fst, x86Argument(sDWORD, rEBX, 0));
				break;
			case DTYPE_INT:
				Emit(o_mov, x86Argument(sDWORD, rEBX, 0), x86Argument(rEAX));
				break;
			case DTYPE_SHORT:
				Emit(o_mov, x86Argument(sWORD, rEBX, 0), x86Argument(rEAX));
				break;
			case DTYPE_CHAR:
				Emit(o_mov, x86Argument(sBYTE, rEBX, 0), x86Argument(rEAX));
				break;
			}
			Emit(o_add, x86Argument(rEBX), x86Argument(typeSizeD[(cmd.helper>>2)&0x00000007]));
			Emit(o_jmp, x86Argument(InlFmt("loopStart%d", aluLabels)));
			Emit(InlFmt("loopEnd%d", aluLabels));
			if(cmd.helper == DTYPE_FLOAT)
				Emit(o_fstp, x86Argument(rST0));
			aluLabels++;
			break;

		case cmdJmp:
			Emit(INST_COMMENT, "JMP");
			Emit(o_jmp, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
			break;

		case cmdJmpZI:
			Emit(INST_COMMENT, "JMPZ int");

			Emit(o_pop, x86Argument(rEAX));
			Emit(o_test, x86Argument(rEAX), x86Argument(rEAX));
			Emit(o_jz, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
			stackRelSize -= 4;
			break;
		case cmdJmpZD:
			Emit(INST_COMMENT, "JMPZ double");

			Emit(o_fldz);
			Emit(o_fcomp, x86Argument(sQWORD, rESP, 0));
			Emit(o_fnstsw, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEBX));
			Emit(o_pop, x86Argument(rEBX));
			Emit(o_test, x86Argument(rEAX), x86Argument(0x44));
			Emit(o_jnp, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
			stackRelSize -= 8;
			break;
		case cmdJmpZL:
			Emit(INST_COMMENT, "JMPZ long");

			Emit(o_pop, x86Argument(rEDX));
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_or, x86Argument(rEDX), x86Argument(rEAX));
			Emit(o_jne, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
			stackRelSize -= 8;
			break;

		case cmdJmpNZI:
			Emit(INST_COMMENT, "JMPNZ int");

			Emit(o_pop, x86Argument(rEAX));
			Emit(o_test, x86Argument(rEAX), x86Argument(rEAX));
			Emit(o_jnz, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
			stackRelSize -= 4;
			break;
		case cmdJmpNZD:
			Emit(INST_COMMENT, "JMPNZ double");

			Emit(o_fldz);
			Emit(o_fcomp, x86Argument(sQWORD, rESP, 0));
			Emit(o_fnstsw, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEBX));
			Emit(o_pop, x86Argument(rEBX));
			Emit(o_test, x86Argument(rEAX), x86Argument(0x44));
			Emit(o_jp, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
			stackRelSize -= 8;
			break;
		case cmdJmpNZL:
			Emit(INST_COMMENT, "JMPNZ long");

			Emit(o_pop, x86Argument(rEDX));
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_or, x86Argument(rEDX), x86Argument(rEAX));
			Emit(o_je, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
			stackRelSize -= 8;
			break;

		case cmdCall:
			Emit(INST_COMMENT, InlFmt("CALL %d ret %s %d", cmd.argument, (cmd.helper & bitRetSimple ? "simple " : ""), (cmd.helper & 0x0FFF)));

			if(cmd.argument == -1)
			{
				Emit(o_pop, x86Argument(rEAX));
				Emit(o_call, x86Argument(rEAX));
				stackRelSize -= 4;
			}else{
				Emit(o_call, x86Argument(InlFmt("function%d", cmd.argument)));
			}
			if(cmd.helper & bitRetSimple)
			{
				if((asmOperType)(cmd.helper & 0x0FFF) == OTYPE_INT)
				{
					Emit(o_push, x86Argument(rEAX));
					stackRelSize += 4;
				}else{	// double or long
					Emit(o_push, x86Argument(rEAX));
					Emit(o_push, x86Argument(rEDX));
					stackRelSize += 8;
				}
			}else{
				assert(cmd.helper % 4 == 0);
				if(cmd.helper == 4)
				{
					Emit(o_push, x86Argument(rEAX));
				}else if(cmd.helper == 8){
					Emit(o_push, x86Argument(rEAX));
					Emit(o_push, x86Argument(rEDX));
				}else if(cmd.helper == 12){
					Emit(o_push, x86Argument(rEAX));
					Emit(o_push, x86Argument(rEDX));
					Emit(o_push, x86Argument(rECX));
				}else if(cmd.helper == 16){
					Emit(o_push, x86Argument(rEAX));
					Emit(o_push, x86Argument(rEDX));
					Emit(o_push, x86Argument(rECX));
					Emit(o_push, x86Argument(rEBX));
				}else if(cmd.helper > 16){
					Emit(o_sub, x86Argument(rESP), x86Argument(cmd.helper));

					Emit(o_mov, x86Argument(rEBX), x86Argument(rEDI));

					Emit(o_lea, x86Argument(rESI), x86Argument(sDWORD, rEAX, paramBase));
					Emit(o_mov, x86Argument(rEDI), x86Argument(rESP));
					Emit(o_mov, x86Argument(rECX), x86Argument(cmd.helper >> 2));
					Emit(o_rep_movsd);

					Emit(o_mov, x86Argument(rEDI), x86Argument(rEBX));
				}
				stackRelSize += cmd.helper;
			}
			break;
		case cmdCallStd:
			if(exFunctions[cmd.argument]->nameLength < 31)
				Emit(INST_COMMENT, InlFmt("CALLSTD %s", exFunctions[cmd.argument]->name));
			else
				Emit(INST_COMMENT, InlFmt("CALLSTD %d", cmd.argument));

			if(exFunctions[cmd.argument]->funcPtr == NULL)
			{
				Emit(o_fld, x86Argument(sQWORD, rESP, 0));
				if(exFunctions[cmd.argument]->nameHash == GetStringHash("cos"))
				{
					Emit(o_fsincos);
					Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
					Emit(o_fstp, x86Argument(rST0));
				}else if(exFunctions[cmd.argument]->nameHash == GetStringHash("sin")){
					Emit(o_fsincos);
					Emit(o_fstp, x86Argument(rST0));
					Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
				}else if(exFunctions[cmd.argument]->nameHash == GetStringHash("tan")){
					Emit(o_fptan);
					Emit(o_fstp, x86Argument(rST0));
					Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
				}else if(exFunctions[cmd.argument]->nameHash == GetStringHash("ctg")){
					Emit(o_fptan);
					Emit(o_fdivrp);
					Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
				}else if(exFunctions[cmd.argument]->nameHash == GetStringHash("ceil")){
					Emit(o_push, x86Argument(rEAX));
					Emit(o_fstcw, x86Argument(sWORD, rESP, 0));
					Emit(o_mov, x86Argument(sWORD, rESP, 2), x86Argument(0x1BBF));
					Emit(o_fldcw, x86Argument(sWORD, rESP, 2));
					Emit(o_frndint);
					Emit(o_fldcw, x86Argument(sWORD, rESP, 0));
					Emit(o_fstp, x86Argument(sQWORD, rESP, 4));
					Emit(o_pop, x86Argument(rEAX));
				}else if(exFunctions[cmd.argument]->nameHash == GetStringHash("floor")){
					Emit(o_push, x86Argument(rEAX));
					Emit(o_fstcw, x86Argument(sWORD, rESP, 0));
					Emit(o_mov, x86Argument(sWORD, rESP, 2), x86Argument(0x17BF));
					Emit(o_fldcw, x86Argument(sWORD, rESP, 2));
					Emit(o_frndint);
					Emit(o_fldcw, x86Argument(sWORD, rESP, 0));
					Emit(o_fstp, x86Argument(sQWORD, rESP, 4));
					Emit(o_pop, x86Argument(rEAX));
				}else if(exFunctions[cmd.argument]->nameHash == GetStringHash("sqrt")){
					Emit(o_fsqrt);
					Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
					Emit(o_fstp, x86Argument(rST0));
				}else{
					strcpy(execError, "cmdCallStd with unknown standard function");
					return false;
				}
			}else{
				unsigned int bytesToPop = 0;
				for(unsigned int i = 0; i < exFunctions[cmd.argument]->paramCount; i++)
				{
					bytesToPop += exTypes[exFunctions[cmd.argument]->paramList[i]]->size > 4 ? exTypes[exFunctions[cmd.argument]->paramList[i]]->size : 4;
				}
				Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)exFunctions[cmd.argument]->funcPtr));
				Emit(o_call, x86Argument(rECX));
				Emit(o_add, x86Argument(rESP), x86Argument(bytesToPop));
				stackRelSize -= bytesToPop;
				if(exTypes[exFunctions[cmd.argument]->retType]->size != 0)
				{
					Emit(o_push, x86Argument(rEAX));
					stackRelSize += 4;
				}
			}
			break;
		case cmdReturn:
			Emit(INST_COMMENT, InlFmt("RET %d, %d %d", cmd.flag, cmd.argument, (cmd.helper & 0x0FFF)));

			if(cmd.flag & bitRetError)
			{
				Emit(o_mov, x86Argument(rECX), x86Argument(0xffffffff));
				Emit(o_int, x86Argument(3));
				break;
			}
			if(cmd.helper == 0)
			{
				Emit(o_mov, x86Argument(rEDI), x86Argument(rEBP));
				Emit(o_pop, x86Argument(rEBP));
				Emit(o_ret);
				stackRelSize = 0;
				break;
			}
			if(cmd.helper & bitRetSimple)
			{
				if((asmOperType)(cmd.helper & 0x0FFF) == OTYPE_INT)
				{
					Emit(o_pop, x86Argument(rEAX));
					stackRelSize -= 4;
				}else{
					Emit(o_pop, x86Argument(rEDX));
					Emit(o_pop, x86Argument(rEAX));
					stackRelSize -= 8;
				}
				for(unsigned int pops = 0; pops < (cmd.argument > 0 ? cmd.argument : 1); pops++)
				{
					Emit(o_mov, x86Argument(rEDI), x86Argument(rEBP));
					Emit(o_pop, x86Argument(rEBP));
					stackRelSize -= 4;
				}
				if(cmd.argument == 0)
					Emit(o_mov, x86Argument(rEBX), x86Argument(cmd.helper & 0x0FFF));
			}else{
				if(cmd.helper == 4)
				{
					Emit(o_pop, x86Argument(rEAX));
				}else if(cmd.helper == 8){
					Emit(o_pop, x86Argument(rEDX));
					Emit(o_pop, x86Argument(rEAX));
				}else if(cmd.helper == 12){
					Emit(o_pop, x86Argument(rECX));
					Emit(o_pop, x86Argument(rEDX));
					Emit(o_pop, x86Argument(rEAX));
				}else if(cmd.helper == 16){
					Emit(o_pop, x86Argument(rEBX));
					Emit(o_pop, x86Argument(rECX));
					Emit(o_pop, x86Argument(rEDX));
					Emit(o_pop, x86Argument(rEAX));
				}else{
					Emit(o_mov, x86Argument(rEBX), x86Argument(rEDI));
					Emit(o_mov, x86Argument(rESI), x86Argument(rESP));

					Emit(o_lea, x86Argument(rEDI), x86Argument(sDWORD, rEDI, paramBase));
					Emit(o_mov, x86Argument(rECX), x86Argument(cmd.helper >> 2));
					Emit(o_rep_movsd);

					Emit(o_mov, x86Argument(rEDI), x86Argument(rEBX));

					Emit(o_add, x86Argument(rESP), x86Argument(cmd.helper));
				}
				stackRelSize -= cmd.helper;
				for(unsigned int pops = 0; pops < cmd.argument-1; pops++)
				{
					Emit(o_mov, x86Argument(rEDI), x86Argument(rEBP));
					Emit(o_pop, x86Argument(rEBP));
					stackRelSize -= 4;
				}
				if(cmd.helper > 16)
					Emit(o_mov, x86Argument(rEAX), x86Argument(rEDI));
				Emit(o_mov, x86Argument(rEDI), x86Argument(rEBP));
				Emit(o_pop, x86Argument(rEBP));
				stackRelSize -= 4;
				if(cmd.argument == 0)
					Emit(o_mov, x86Argument(rEBX), x86Argument(16));
			}
			Emit(o_ret);
			stackRelSize = 0;
			break;

		case cmdPushVTop:
			Emit(INST_COMMENT, "PUSHT");

			Emit(o_push, x86Argument(rEBP));
			Emit(o_mov, x86Argument(rEBP), x86Argument(rEDI));
			stackRelSize += 4;
			break;
		case cmdPopVTop:
			Emit(INST_COMMENT, "POPT");

			Emit(o_mov, x86Argument(rEDI), x86Argument(rEBP));
			Emit(o_pop, x86Argument(rEBP));
			stackRelSize -= 4;
			break;
		
		case cmdPushV:
			Emit(INST_COMMENT, "PUSHV");

			Emit(o_add, x86Argument(rEDI), x86Argument(cmd.argument));
			break;

		case cmdAdd:
			Emit(INST_COMMENT, "ADD int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_add, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 4;
			break;
		case cmdSub:
			Emit(INST_COMMENT, "SUB int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_sub, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 4;
			break;
		case cmdMul:
			Emit(INST_COMMENT, "MUL int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_imul, x86Argument(rEDX));
			Emit(o_push, x86Argument(rEAX));
			stackRelSize -= 4;
			break;
		case cmdDiv:
			Emit(INST_COMMENT, "DIV int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xchg, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
			Emit(o_cdq);
			Emit(o_idiv, x86Argument(sDWORD, rESP, 0));
			Emit(o_xchg, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
			stackRelSize -= 4;
			break;
		case cmdPow:
			Emit(INST_COMMENT, "POW int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEBX));
			Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)intPow));
			Emit(o_call, x86Argument(rECX));
			Emit(o_push, x86Argument(rEDX));
			stackRelSize -= 4;
			break;
		case cmdMod:
			Emit(INST_COMMENT, "MOD int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xchg, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
			Emit(o_cdq);
			Emit(o_idiv, x86Argument(sDWORD, rESP, 0));
			Emit(o_xchg, x86Argument(rEDX), x86Argument(sDWORD, rESP, 0));
			stackRelSize -= 4;
			break;
		case cmdLess:
			Emit(INST_COMMENT, "LESS int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_setl, x86Argument(rECX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
			stackRelSize -= 4;
			break;
		case cmdGreater:
			Emit(INST_COMMENT, "GREATER int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_setg, x86Argument(rECX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
			stackRelSize -= 4;
			break;
		case cmdLEqual:
			Emit(INST_COMMENT, "LEQUAL int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_setle, x86Argument(rECX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
			stackRelSize -= 4;
			break;
		case cmdGEqual:
			Emit(INST_COMMENT, "GEQUAL int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_setge, x86Argument(rECX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
			stackRelSize -= 4;
			break;
		case cmdEqual:
			Emit(INST_COMMENT, "EQUAL int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_sete, x86Argument(rECX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
			stackRelSize -= 4;
			break;
		case cmdNEqual:
			Emit(INST_COMMENT, "NEQUAL int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_setne, x86Argument(rECX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
			stackRelSize -= 4;
			break;
		case cmdShl:
			Emit(INST_COMMENT, "SHL int");
			Emit(o_pop, x86Argument(rECX));
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_sal, x86Argument(rEAX), x86Argument(rECX));
			Emit(o_push, x86Argument(rEAX));
			stackRelSize -= 4;
			break;
		case cmdShr:
			Emit(INST_COMMENT, "SHR int");
			Emit(o_pop, x86Argument(rECX));
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_sar, x86Argument(rEAX), x86Argument(rECX));
			Emit(o_push, x86Argument(rEAX));
			stackRelSize -= 4;
			break;
		case cmdBitAnd:
			Emit(INST_COMMENT, "BAND int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_and, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 4;
			break;
		case cmdBitOr:
			Emit(INST_COMMENT, "BOR int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_or, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 4;
			break;
		case cmdBitXor:
			Emit(INST_COMMENT, "BXOR int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_xor, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 4;
			break;
		case cmdLogAnd:
			Emit(INST_COMMENT, "LAND int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_cmp, x86Argument(rEAX), x86Argument(0));
			Emit(o_je, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_je, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("pushedOne%d", aluLabels)));
			Emit(InlFmt("pushZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("pushedOne%d", aluLabels));
			aluLabels++;
			stackRelSize -= 4;
			break;
		case cmdLogOr:
			Emit(INST_COMMENT, "LOR int");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEBX));
			Emit(o_or, x86Argument(rEAX), x86Argument(rEBX));
			Emit(o_cmp, x86Argument(rEAX), x86Argument(0));
			Emit(o_je, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_push, x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("pushedOne%d", aluLabels)));
			Emit(InlFmt("pushZero%d", aluLabels));
			Emit(o_push, x86Argument(0));
			Emit(InlFmt("pushedOne%d", aluLabels));
			aluLabels++;
			stackRelSize -= 4;
			break;
		case cmdLogXor:
			Emit(INST_COMMENT, "LXOR int");
			Emit(o_xor, x86Argument(rEAX), x86Argument(rEAX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(o_setne, x86Argument(rEAX));
			Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			Emit(o_setne, x86Argument(rECX));
			Emit(o_xor, x86Argument(rEAX), x86Argument(rECX));
			Emit(o_pop, x86Argument(rECX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 4;
			break;

		case cmdAddL:
			Emit(INST_COMMENT, "ADD long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_add, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_adc, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			stackRelSize -= 8;
			break;
		case cmdSubL:
			Emit(INST_COMMENT, "SUB long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_sub, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_sbb, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			stackRelSize -= 8;
			break;
		case cmdMulL:
			Emit(INST_COMMENT, "MUL long");
			Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longMul));
			Emit(o_call, x86Argument(rECX));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 8;
			break;
		case cmdDivL:
			Emit(INST_COMMENT, "DIV long");
			Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longDiv));
			Emit(o_call, x86Argument(rECX));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 8;
			break;
		case cmdPowL:
			Emit(INST_COMMENT, "POW long");
			Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longPow));
			Emit(o_call, x86Argument(rECX));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 8;
			break;
		case cmdModL:
			Emit(INST_COMMENT, "MOD long");
			Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longMod));
			Emit(o_call, x86Argument(rECX));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			stackRelSize -= 8;
			break;
		case cmdLessL:
			Emit(INST_COMMENT, "LESS long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_jg, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(o_jl, x86Argument(InlFmt("SetOne%d", aluLabels)));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_jae, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(InlFmt("SetOne%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("OneSet%d", aluLabels)));
			Emit(InlFmt("SetZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("OneSet%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdGreaterL:
			Emit(INST_COMMENT, "GREATER long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_jl, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(o_jg, x86Argument(InlFmt("SetOne%d", aluLabels)));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_jbe, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(InlFmt("SetOne%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("OneSet%d", aluLabels)));
			Emit(InlFmt("SetZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("OneSet%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdLEqualL:
			Emit(INST_COMMENT, "LEQUAL long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_jg, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(o_jl, x86Argument(InlFmt("SetOne%d", aluLabels)));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_ja, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(InlFmt("SetOne%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("OneSet%d", aluLabels)));
			Emit(InlFmt("SetZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("OneSet%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdGEqualL:
			Emit(INST_COMMENT, "GEQUAL long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_jl, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(o_jg, x86Argument(InlFmt("SetOne%d", aluLabels)));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_jb, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(InlFmt("SetOne%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("OneSet%d", aluLabels)));
			Emit(InlFmt("SetZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("OneSet%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdEqualL:
			Emit(INST_COMMENT, "EQUAL long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_jne, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_jne, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("OneSet%d", aluLabels)));
			Emit(InlFmt("SetZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("OneSet%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdNEqualL:
			Emit(INST_COMMENT, "NEQUAL long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			Emit(o_jne, x86Argument(InlFmt("SetOne%d", aluLabels)));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_je, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(InlFmt("SetOne%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("OneSet%d", aluLabels)));
			Emit(InlFmt("SetZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("OneSet%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdShlL:
			Emit(INST_COMMENT, "SHL long");
			Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longShl));
			Emit(o_call, x86Argument(rECX));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			stackRelSize -= 8;
			break;
		case cmdShrL:
			Emit(INST_COMMENT, "SHR long");
			Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longShr));
			Emit(o_call, x86Argument(rECX));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			stackRelSize -= 8;
			break;
		case cmdBitAndL:
			Emit(INST_COMMENT, "BAND long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_and, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_and, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			stackRelSize -= 8;
			break;
		case cmdBitOrL:
			Emit(INST_COMMENT, "BOR long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_or, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_or, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			stackRelSize -= 8;
			break;
		case cmdBitXorL:
			Emit(INST_COMMENT, "BXOR long");
			Emit(o_pop, x86Argument(rEAX));
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_xor, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			Emit(o_xor, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
			stackRelSize -= 8;
			break;
		case cmdLogAndL:
			Emit(INST_COMMENT, "LAND long");
			Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
			Emit(o_or, x86Argument(rEAX), x86Argument(sDWORD, rESP, 4));
			Emit(o_jz, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rESP, 8));
			Emit(o_or, x86Argument(rEAX), x86Argument(sDWORD, rESP, 12));
			Emit(o_jz, x86Argument(InlFmt("SetZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 8), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("OneSet%d", aluLabels)));
			Emit(InlFmt("SetZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 8), x86Argument(0));
			Emit(InlFmt("OneSet%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 12), x86Argument(0));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdLogOrL:
			Emit(INST_COMMENT, "LOR long");
			Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
			Emit(o_or, x86Argument(rEAX), x86Argument(sDWORD, rESP, 4));
			Emit(o_jnz, x86Argument(InlFmt("SetOne%d", aluLabels)));
			Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rESP, 8));
			Emit(o_or, x86Argument(rEAX), x86Argument(sDWORD, rESP, 12));
			Emit(o_jnz, x86Argument(InlFmt("SetOne%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 8), x86Argument(0));
			Emit(o_jmp, x86Argument(InlFmt("ZeroSet%d", aluLabels)));
			Emit(InlFmt("SetOne%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 8), x86Argument(1));
			Emit(InlFmt("ZeroSet%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 12), x86Argument(0));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdLogXorL:
			Emit(INST_COMMENT, "LXOR long");
			Emit(o_xor, x86Argument(rEAX), x86Argument(rEAX));
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_or, x86Argument(rEBX), x86Argument(sDWORD, rESP, 4));
			Emit(o_setnz, x86Argument(rEAX));
			Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 8));
			Emit(o_or, x86Argument(rEBX), x86Argument(sDWORD, rESP, 12));
			Emit(o_setnz, x86Argument(rECX));
			Emit(o_xor, x86Argument(rEAX), x86Argument(rECX));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			aluLabels++;
			stackRelSize -= 8;
			break;

		case cmdAddD:
			Emit(INST_COMMENT, "ADD double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 8));
			Emit(o_fadd, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			stackRelSize -= 8;
			break;
		case cmdSubD:
			Emit(INST_COMMENT, "SUB double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 8));
			Emit(o_fsub, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			stackRelSize -= 8;
			break;
		case cmdMulD:
			Emit(INST_COMMENT, "MUL double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 8));
			Emit(o_fmul, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			stackRelSize -= 8;
			break;
		case cmdDivD:
			Emit(INST_COMMENT, "DIV double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 8));
			Emit(o_fdiv, x86Argument(sQWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			stackRelSize -= 8;
			break;
		case cmdPowD:
			Emit(INST_COMMENT, "POW double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fld, x86Argument(sQWORD, rESP, 8));
			Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)doublePow));
			Emit(o_call, x86Argument(rECX));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			stackRelSize -= 8;
			break;
		case cmdModD:
			Emit(INST_COMMENT, "MOD double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fld, x86Argument(sQWORD, rESP, 8));
			Emit(o_fprem);
			Emit(o_fstp, x86Argument(rST1));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			stackRelSize -= 8;
			break;
		case cmdLessD:
			Emit(INST_COMMENT, "LESS double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fcomp, x86Argument(sQWORD, rESP, 8));
			Emit(o_fnstsw);
			Emit(o_test, x86Argument(rEAX), x86Argument(0x41));
			Emit(o_jne, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("pushedOne%d", aluLabels)));
			Emit(InlFmt("pushZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("pushedOne%d", aluLabels));
			Emit(o_fild, x86Argument(sDWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdGreaterD:
			Emit(INST_COMMENT, "GREATER double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fcomp, x86Argument(sQWORD, rESP, 8));
			Emit(o_fnstsw);
			Emit(o_test, x86Argument(rEAX), x86Argument(0x05));
			Emit(o_jp, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("pushedOne%d", aluLabels)));
			Emit(InlFmt("pushZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("pushedOne%d", aluLabels));
			Emit(o_fild, x86Argument(sDWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdLEqualD:
			Emit(INST_COMMENT, "LEQUAL double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fcomp, x86Argument(sQWORD, rESP, 8));
			Emit(o_fnstsw);
			Emit(o_test, x86Argument(rEAX), x86Argument(0x01));
			Emit(o_jne, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("pushedOne%d", aluLabels)));
			Emit(InlFmt("pushZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("pushedOne%d", aluLabels));
			Emit(o_fild, x86Argument(sDWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdGEqualD:
			Emit(INST_COMMENT, "GEQUAL double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fcomp, x86Argument(sQWORD, rESP, 8));
			Emit(o_fnstsw);
			Emit(o_test, x86Argument(rEAX), x86Argument(0x41));
			Emit(o_jp, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("pushedOne%d", aluLabels)));
			Emit(InlFmt("pushZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("pushedOne%d", aluLabels));
			Emit(o_fild, x86Argument(sDWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdEqualD:
			Emit(INST_COMMENT, "EQUAL double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fcomp, x86Argument(sQWORD, rESP, 8));
			Emit(o_fnstsw);
			Emit(o_test, x86Argument(rEAX), x86Argument(0x44));
			Emit(o_jp, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("pushedOne%d", aluLabels)));
			Emit(InlFmt("pushZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("pushedOne%d", aluLabels));
			Emit(o_fild, x86Argument(sDWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			aluLabels++;
			stackRelSize -= 8;
			break;
		case cmdNEqualD:
			Emit(INST_COMMENT, "NEQUAL double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fcomp, x86Argument(sQWORD, rESP, 8));
			Emit(o_fnstsw);
			Emit(o_test, x86Argument(rEAX), x86Argument(0x44));
			Emit(o_jnp, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("pushedOne%d", aluLabels)));
			Emit(InlFmt("pushZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("pushedOne%d", aluLabels));
			Emit(o_fild, x86Argument(sDWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
			Emit(o_add, x86Argument(rESP), x86Argument(8));
			aluLabels++;
			stackRelSize -= 8;
			break;

		case cmdNeg:
			Emit(INST_COMMENT, "NEG int");
			Emit(o_neg, x86Argument(sDWORD, rESP, 0));
			break;
		case cmdBitNot:
			Emit(INST_COMMENT, "BNOT int");
			Emit(o_not, x86Argument(sDWORD, rESP, 0));
			break;
		case cmdLogNot:
			Emit(INST_COMMENT, "LNOT int");
			Emit(o_xor, x86Argument(rEAX), x86Argument(rEAX));
			Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(o_sete, x86Argument(rEAX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			break;

		case cmdNegL:
			Emit(INST_COMMENT, "NEG long");
			Emit(o_neg, x86Argument(sDWORD, rESP, 0));
			Emit(o_adc, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			Emit(o_neg, x86Argument(sDWORD, rESP, 4));
			break;
		case cmdBitNotL:
			Emit(INST_COMMENT, "BNOT long");
			Emit(o_not, x86Argument(sDWORD, rESP, 0));
			Emit(o_not, x86Argument(sDWORD, rESP, 4));
			break;
		case cmdLogNotL:
			Emit(INST_COMMENT, "LNOT long");
			Emit(o_xor, x86Argument(rEAX), x86Argument(rEAX));
			Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 4));
			Emit(o_or, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
			Emit(o_setz, x86Argument(rEAX));
			Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(0));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
			break;

		case cmdNegD:
			Emit(INST_COMMENT, "NEG double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fchs);
			Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
			break;
		case cmdLogNotD:
			Emit(INST_COMMENT, "LNOT double");
			Emit(o_fldz);
			Emit(o_fcomp, x86Argument(sQWORD, rESP, 0));
			Emit(o_fnstsw);
			Emit(o_test, x86Argument(rEAX), x86Argument(0x44));
			Emit(o_jp, x86Argument(InlFmt("pushZero%d", aluLabels)));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_jmp, x86Argument(InlFmt("pushedOne%d", aluLabels)));
			Emit(InlFmt("pushZero%d", aluLabels));
			Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			Emit(InlFmt("pushedOne%d", aluLabels));
			Emit(o_fild, x86Argument(sDWORD, rESP, 0));
			Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
			aluLabels++;
			break;
		
		case cmdIncI:
			Emit(INST_COMMENT, "INC int");
			Emit(o_add, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			break;
		case cmdIncD:
			Emit(INST_COMMENT, "INC double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fld1);
			Emit(o_faddp);
			Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
			break;
		case cmdIncL:
			Emit(INST_COMMENT, "INC long");
			Emit(o_add, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_adc, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			break;

		case cmdDecI:
			Emit(INST_COMMENT, "DEC int");
			Emit(o_sub, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			break;
		case cmdDecD:
			Emit(INST_COMMENT, "DEC double");
			Emit(o_fld, x86Argument(sQWORD, rESP, 0));
			Emit(o_fld1);
			Emit(o_fsubp);
			Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
			break;
		case cmdDecL:
			Emit(INST_COMMENT, "DEC long");
			Emit(o_sub, x86Argument(sDWORD, rESP, 0), x86Argument(1));
			Emit(o_sbb, x86Argument(sDWORD, rESP, 0), x86Argument(0));
			break;

		case cmdAddAtCharStk:
			Emit(INST_COMMENT, "ADDAT char stack");
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_movsx, x86Argument(rEAX), x86Argument(sBYTE, rEDX, cmd.argument+paramBase));
			if(cmd.flag == bitPushBefore)
				Emit(o_push, x86Argument(rEAX));
			Emit(cmd.helper == 1 ? o_add : o_sub, x86Argument(rEAX), x86Argument(1));
			Emit(o_mov, x86Argument(sBYTE, rEDX, cmd.argument+paramBase), x86Argument(rEAX));
			if(cmd.flag == bitPushAfter)
				Emit(o_push, x86Argument(rEAX));
			if(!cmd.flag)
				stackRelSize -= 4;
			break;
		case cmdAddAtShortStk:
			Emit(INST_COMMENT, "ADDAT short stack");
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_movsx, x86Argument(rEAX), x86Argument(sWORD, rEDX, cmd.argument+paramBase));
			if(cmd.flag == bitPushBefore)
				Emit(o_push, x86Argument(rEAX));
			Emit(cmd.helper == 1 ? o_add : o_sub, x86Argument(rEAX), x86Argument(1));
			Emit(o_mov, x86Argument(sWORD, rEDX, cmd.argument+paramBase), x86Argument(rEAX));
			if(cmd.flag == bitPushAfter)
				Emit(o_push, x86Argument(rEAX));
			if(!cmd.flag)
				stackRelSize -= 4;
			break;
		case cmdAddAtIntStk:
			Emit(INST_COMMENT, "ADDAT int stack");
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
			if(cmd.flag == bitPushBefore)
				Emit(o_push, x86Argument(rEAX));
			Emit(cmd.helper == 1 ? o_add : o_sub, x86Argument(rEAX), x86Argument(1));
			Emit(o_mov, x86Argument(sDWORD, rEDX, cmd.argument+paramBase), x86Argument(rEAX));
			if(cmd.flag == bitPushAfter)
				Emit(o_push, x86Argument(rEAX));
			if(!cmd.flag)
				stackRelSize -= 4;
			break;
		case cmdAddAtLongStk:
			Emit(INST_COMMENT, "ADDAT long stack");
			Emit(o_pop, x86Argument(rECX));
			Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rECX, cmd.argument+paramBase));
			Emit(o_mov, x86Argument(rEDX), x86Argument(sDWORD, rECX, cmd.argument+paramBase+4));
			if(cmd.flag == bitPushBefore)
			{
				Emit(o_push, x86Argument(rEAX));
				Emit(o_push, x86Argument(rEDX));
				stackRelSize += 4;
			}
			Emit(cmd.helper == 1 ? o_add : o_sub, x86Argument(rEAX), x86Argument(1));
			Emit(cmd.helper == 1 ? o_adc : o_sbb, x86Argument(rEDX), x86Argument(0));
			Emit(o_mov, x86Argument(sDWORD, rECX, cmd.argument+paramBase), x86Argument(rEAX));
			Emit(o_mov, x86Argument(sDWORD, rECX, cmd.argument+paramBase+4), x86Argument(rEDX));
			if(cmd.flag == bitPushAfter)
			{
				Emit(o_push, x86Argument(rEAX));
				Emit(o_push, x86Argument(rEDX));
				stackRelSize += 4;
			}
			break;
		case cmdAddAtFloatStk:
			Emit(INST_COMMENT, "ADDAT float stack");
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_fld, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
			if(cmd.flag == bitPushBefore)
				Emit(o_fld, x86Argument(rST0));
			Emit(o_fld1);
			Emit(cmd.helper == 1 ? o_faddp : o_fsubp);
			if(cmd.flag == bitPushAfter)
			{
				Emit(o_fst, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
				Emit(o_sub, x86Argument(rESP), x86Argument(8));
				Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
				stackRelSize += 4;
			}else{
				Emit(o_fstp, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
			}
			if(cmd.flag == bitPushBefore)
			{
				Emit(o_sub, x86Argument(rESP), x86Argument(8));
				Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
				stackRelSize += 4;
			}
			break;
		case cmdAddAtDoubleStk:
			Emit(INST_COMMENT, "ADDAT double stack");
			Emit(o_pop, x86Argument(rEDX));
			Emit(o_fld, x86Argument(sQWORD, rEDX, cmd.argument+paramBase));
			if(cmd.flag == bitPushBefore)
				Emit(o_fld, x86Argument(rST0));
			Emit(o_fld1);
			Emit(cmd.helper == 1 ? o_faddp : o_fsubp);
			if(cmd.flag == bitPushAfter)
			{
				Emit(o_fst, x86Argument(sQWORD, rEDX, cmd.argument+paramBase));
				Emit(o_sub, x86Argument(rESP), x86Argument(8));
				Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
				stackRelSize += 4;
			}else{
				Emit(o_fstp, x86Argument(sQWORD, rEDX, cmd.argument+paramBase));
			}
			if(cmd.flag == bitPushBefore)
			{
				Emit(o_sub, x86Argument(rESP), x86Argument(8));
				Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
				stackRelSize += 4;
			}
			break;
		}
		if(stackRelSize == 0 && stackRelSizePrev != 0)
			Emit(INST_COMMENT, "=====Stack restored=====");
		//Emit(INST_COMMENT, InlFmt("====Stack size: %d", stackRelSize));
	}
	Emit(InlFmt("gLabel%d", pos));
	Emit(o_pop, x86Argument(rEBP));
	Emit(o_ret);

#ifdef NULLC_LOG_FILES
	FILE *noptAsm = fopen("asmX86_noopt.txt", "wb");
	char instBuf[128];
	for(unsigned int i = 0; i < instList.size(); i++)
	{
		instList[i].Decode(instBuf);
		fprintf(noptAsm, "%s\r\n", instBuf);
	}
	fclose(noptAsm);
#endif

#ifdef NULLC_LOG_FILES
	DeleteFile("asmX86.txt");
	FILE *fAsm = fopen("asmX86.txt", "wb");
#endif
	if(optimize)
	{
		OptimizerX86 optiMan;
		optiMan.Optimize(instList);
	}
#ifdef NULLC_LOG_FILES
	for(unsigned int i = 0; i < instList.size(); i++)
	{
		if(instList[i].name == o_other)
			continue;
		instList[i].Decode(instBuf);
		fprintf(fAsm, "%s\r\n", instBuf);
	}
	fclose(fAsm);
#endif

	// Translate to x86
	unsigned char *bytecode = binCode+20;//new unsigned char[16000];
	unsigned char *code = bytecode;

	x86ClearLabels();

	for(unsigned int i = 0; i < instList.size(); i++)
	{
		//if(code-bytecode >= 0x0097)
		//	__asm int 3;
		x86Instruction cmd = instList[i];
		switch(cmd.name)
		{
		case o_none:
			break;
		case o_mov:
			if(cmd.argA.type != x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argNumber)
					code += x86MOV(code, cmd.argA.reg, cmd.argB.num);
				else if(cmd.argB.type == x86Argument::argPtr)
					code += x86MOV(code, cmd.argA.reg, cmd.argB.ptrReg[0], sDWORD, cmd.argB.ptrNum);
				else
					code += x86MOV(code, cmd.argA.reg, cmd.argB.reg);
			}else{
				if(cmd.argB.type == x86Argument::argNumber)
					code += x86MOV(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.num);
				else
					code += x86MOV(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrReg[1], cmd.argA.ptrNum, cmd.argB.reg);
			}
			break;
		case o_movsx:
			code += x86MOVSX(code, cmd.argA.reg, cmd.argB.ptrSize, cmd.argB.ptrReg[0], cmd.argB.ptrReg[1], cmd.argB.ptrNum);
			break;
		case o_push:
			if(cmd.argA.type == x86Argument::argNumber)
				code += x86PUSH(code, cmd.argA.num);
			else if(cmd.argA.type == x86Argument::argPtr)
				code += x86PUSH(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrReg[1], cmd.argA.ptrNum);
			else
				code += x86PUSH(code, cmd.argA.reg);
			break;
		case o_pop:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86POP(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrReg[1], cmd.argA.ptrNum);
			else
				code += x86POP(code, cmd.argA.reg);
			break;
		case o_lea:
			if(cmd.argB.type == x86Argument::argPtrLabel)
			{
				code += x86LEA(code, cmd.argA.reg, cmd.argB.labelName, cmd.argB.ptrNum);
			}else{
				if(cmd.argB.ptrMult != 1)
					code += x86LEA(code, cmd.argA.reg, cmd.argB.ptrReg[0], cmd.argB.ptrMult, cmd.argB.ptrNum);
				else
					code += x86LEA(code, cmd.argA.reg, cmd.argB.ptrReg[0], cmd.argB.ptrNum);
			}
			break;
		case o_xchg:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86XCHG(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.reg);
			else if(cmd.argB.type == x86Argument::argPtr)
				code += x86XCHG(code, sDWORD, cmd.argB.ptrReg[0], cmd.argB.ptrNum, cmd.argA.reg);
			else
				code += x86XCHG(code, cmd.argA.reg, cmd.argB.reg);
			break;
		case o_cdq:
			code += x86CDQ(code);
			break;
		case o_rep_movsd:
			code += x86REP_MOVSD(code);
			break;

		case o_jmp:
			code += x86JMP(code, cmd.argA.labelName, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_ja:
			code += x86Jcc(code, cmd.argA.labelName, condA, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jae:
			code += x86Jcc(code, cmd.argA.labelName, condAE, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jb:
			code += x86Jcc(code, cmd.argA.labelName, condB, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jbe:
			code += x86Jcc(code, cmd.argA.labelName, condBE, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jc:
			code += x86Jcc(code, cmd.argA.labelName, condC, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_je:
			code += x86Jcc(code, cmd.argA.labelName, condE, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jz:
			code += x86Jcc(code, cmd.argA.labelName, condZ, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jg:
			code += x86Jcc(code, cmd.argA.labelName, condG, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jl:
			code += x86Jcc(code, cmd.argA.labelName, condL, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jne:
			code += x86Jcc(code, cmd.argA.labelName, condNE, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jnp:
			code += x86Jcc(code, cmd.argA.labelName, condNP, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jnz:
			code += x86Jcc(code, cmd.argA.labelName, condNZ, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jp:
			code += x86Jcc(code, cmd.argA.labelName, condP, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_call:
			if(cmd.argA.type == x86Argument::argLabel)
				code += x86CALL(code, cmd.argA.labelName);
			else
				code += x86CALL(code, cmd.argA.reg);
			break;
		case o_ret:
			code += x86RET(code);
			break;

		case o_fld:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argA.ptrReg[1] != x86Argument::argNone)
					code += x86FLD(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrReg[1], cmd.argA.ptrNum);
				else
					code += x86FLD(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			}else{
				code += x86FLD(code, (x87Reg)cmd.argA.fpArg);
			}
			break;
		case o_fild:
			code += x86FILD(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0]);
			break;
		case o_fistp:
			code += x86FISTP(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_fst:
			if(cmd.argA.ptrReg[1] != x86Argument::argNone)
				code += x86FST(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrReg[1], cmd.argA.ptrNum);
			else
				code += x86FST(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_fstp:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argA.ptrReg[1] != x86Argument::argNone)
					code += x86FSTP(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrReg[1], cmd.argA.ptrNum);
				else
					code += x86FSTP(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			}else{
				code += x86FSTP(code, (x87Reg)cmd.argA.fpArg);
			}
			break;
		case o_fnstsw:
			code += x86FNSTSW(code);
			break;
		case o_fstcw:
			code += x86FSTCW(code);
			break;
		case o_fldcw:
			code += x86FLDCW(code, cmd.argA.ptrNum);
			break;

		case o_neg:
			code += x86NEG(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_add:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADD(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86ADD(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.num);
			}else{
				code += x86ADD(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_adc:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADC(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86ADC(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.num);
			}else{
				code += x86ADC(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_sub:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86SUB(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86SUB(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.num);
			}else{
				code += x86SUB(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_sbb:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86SBB(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86SBB(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.num);
			}else{
				code += x86SBB(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_imul:
			if(cmd.argB.type != x86Argument::argNone)
				code += x86IMUL(code, cmd.argA.reg, cmd.argB.num);
			else
				code += x86IMUL(code, cmd.argA.reg);
			break;
		case o_idiv:
			code += x86IDIV(code, sDWORD, cmd.argA.ptrReg[0]);
			break;
		case o_shl:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86SHL(code, sDWORD, cmd.argA.ptrReg[0], cmd.argB.num);
			else
				code += x86SHL(code, cmd.argA.reg, cmd.argB.num);
			break;
		case o_sal:
			code += x86SAL(code);
			break;
		case o_sar:
			code += x86SAR(code);
			break;
		case o_not:
			code += x86NOT(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_and:
			code += x86AND(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.reg);
			break;
		case o_or:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86OR(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.reg);
			else if(cmd.argB.type == x86Argument::argPtr)
				code += x86OR(code, cmd.argA.reg, sDWORD, cmd.argB.ptrReg[0], cmd.argB.ptrNum);
			else
				code += x86OR(code, cmd.argA.reg, cmd.argB.reg);
			break;
		case o_xor:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86XOR(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.reg);
			else
				code += x86XOR(code, cmd.argA.reg, cmd.argB.reg);
			break;
		case o_cmp:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argNumber)
					code += x86CMP(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.num);
				else
					code += x86CMP(code, sDWORD, cmd.argA.ptrReg[0], cmd.argA.ptrNum, cmd.argB.reg);
			}else{
				if(cmd.argB.type == x86Argument::argNumber)
					code += x86CMP(code, cmd.argA.reg, cmd.argB.num);
				else
					code += x86CMP(code, cmd.argA.reg, cmd.argB.reg);
			}
			break;
		case o_test:
			if(cmd.argB.type == x86Argument::argNumber)
				code += x86TESTah(code, (char)cmd.argB.num);
			else
				code += x86TEST(code, cmd.argA.reg, cmd.argB.reg);
			break;

		case o_setl:
			code += x86SETcc(code, condL, cmd.argA.reg);
			break;
		case o_setg:
			code += x86SETcc(code, condG, cmd.argA.reg);
			break;
		case o_setle:
			code += x86SETcc(code, condLE, cmd.argA.reg);
			break;
		case o_setge:
			code += x86SETcc(code, condGE, cmd.argA.reg);
			break;
		case o_sete:
			code += x86SETcc(code, condE, cmd.argA.reg);
			break;
		case o_setne:
			code += x86SETcc(code, condNE, cmd.argA.reg);
			break;
		case o_setz:
			code += x86SETcc(code, condZ, cmd.argA.reg);
			break;
		case o_setnz:
			code += x86SETcc(code, condNZ, cmd.argA.reg);
			break;

		case o_fadd:
			code += x86FADD(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_faddp:
			code += x86FADDP(code);
			break;
		case o_fmul:
			code += x86FMUL(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_fmulp:
			code += x86FMULP(code);
			break;
		case o_fsub:
			code += x86FSUB(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_fsubr:
			code += x86FSUBR(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_fsubp:
			code += x86FSUBP(code);
			break;
		case o_fsubrp:
			code += x86FSUBRP(code);
			break;
		case o_fdiv:
			code += x86FDIV(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_fdivr:
			code += x86FDIVR(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_fdivrp:
			code += x86FDIVRP(code);
			break;
		case o_fchs:
			code += x86FCHS(code);
			break;
		case o_fprem:
			code += x86FPREM(code);
			break;
		case o_fcomp:
			code += x86FCOMP(code, cmd.argA.ptrSize, cmd.argA.ptrReg[0], cmd.argA.ptrNum);
			break;
		case o_fldz:
			code += x86FLDZ(code);
			break;
		case o_fld1:
			code += x86FLD1(code);
			break;
		case o_fsincos:
			code += x86FSINCOS(code);
			break;
		case o_fptan:
			code += x86FPTAN(code);
			break;
		case o_fsqrt:
			code += x86FSQRT(code);
			break;
		case o_frndint:
			code += x86FRNDINT(code);
			break;

		case o_int:
			code += x86INT(code, 3);
			break;
		case o_dd:
			*(int*)code = cmd.argA.num;
#ifdef NULLC_X86_CMP_FASM
			code += 4;
#endif
			for(unsigned int i = 0; i < exFunctions.size(); i++)
			{
				int marker = (('N' << 24) | exFunctions[i]->address);
				if(marker == cmd.argA.num)
					exFuncInfo[i].startInByteCode = (int)(code-bytecode);
			}
			if(cmd.argA.num == int(('G' << 24) | exLinker->offsetToGlobalCode))
			{
				globalStartInBytecode = (int)(code-bytecode);
			}
			break;
		case o_label:
			x86AddLabel(code, cmd.labelName);
			break;
		case o_use32:
			break;
		case o_other:
			break;
		}
	}
	binCodeSize = (unsigned int)(code-bytecode);

#ifdef NULLC_X86_CMP_FASM
	FILE *fMyCode = fopen("asmX86my.bin", "wb");
	fwrite(bytecode, 1, code-bytecode, fMyCode);
	fclose(fMyCode);

	// debug
	unsigned char *bytecodeCopy = new unsigned char[code-bytecode+1];
	memcpy(bytecodeCopy, bytecode, code-bytecode);

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
	{
		strcpy(execError, "Failed to create process");
		return false;
	}

	if(WAIT_TIMEOUT == WaitForSingleObject(prInfo.hProcess, 5000))
	{
		strcpy(execError, "Compilation to x86 binary took too much time (timeout=5sec)");
		return false;
	}

	CloseHandle(prInfo.hProcess);
	CloseHandle(prInfo.hThread);

	FILE *fCode = fopen("asmX86.bin", "rb");
	if(!fCode)
	{
		strcpy(execError, "Failed to open output file");
		return false;
	}
	
	fseek(fCode, 0, SEEK_END);
	unsigned int size = ftell(fCode);
	fseek(fCode, 0, SEEK_SET);
	if(size > 200000)
	{
		strcpy(execError, "Byte code is too big (size > 200000)");
		return false;
	}
	fread(binCode+20, 1, size, fCode);
	binCodeSize = size;

	for(int i = 0; i < code-bytecode; i++)
		if(binCode[i+20] != bytecodeCopy[i])
			__asm int 3;

	delete[] bytecodeCopy;
#endif NULLC_X86_CMP_FASM
	return true;
}

const char* ExecutorX86::GetResult() throw()
{
	long long combined = 0;
	*((int*)(&combined)) = runResult2;
	*((int*)(&combined)+1) = runResult;

	switch(runResultType)
	{
	case OTYPE_DOUBLE:
		sprintf(execResult, "%f", *(double*)(&combined));
		break;
	case OTYPE_LONG:
		sprintf(execResult, "%I64dL", combined);
		break;
	case OTYPE_INT:
		sprintf(execResult, "%d", runResult);
		break;
	}
	return execResult;
}

const char*	ExecutorX86::GetExecError() throw()
{
	return execError;
}

void ExecutorX86::SetOptimization(int toggle) throw()
{
	optimize = toggle;
}

char* ExecutorX86::GetVariableData() throw()
{
	return paramData;
}

#endif
