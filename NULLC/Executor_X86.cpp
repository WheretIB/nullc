#include "stdafx.h"
#ifdef NULLC_BUILD_X86_JIT

#include "Executor_X86.h"
#include "CodeGen_X86.h"
#include "Translator_X86.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

unsigned int paramDataBase;
unsigned int reservedStack;
unsigned int commitedStack;
unsigned int stackGrowSize;
unsigned int stackGrowCommit;

ExecutorX86::ExecutorX86(Linker *linker): exLinker(linker), exFunctions(linker->exFunctions),
			exCode(linker->exCode), exTypes(linker->exTypes)
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

typedef void (*codegenCallback)(VMCmd);
codegenCallback cgFuncs[cmdAddAtDoubleStk+1];

bool ExecutorX86::Initialize()
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

	cgFuncs[cmdNop] = GenCodeCmdNop;

	cgFuncs[cmdPushChar] = GenCodeCmdPushChar;
	cgFuncs[cmdPushShort] = GenCodeCmdPushShort;
	cgFuncs[cmdPushInt] = GenCodeCmdPushInt;
	cgFuncs[cmdPushFloat] = GenCodeCmdPushFloat;
	cgFuncs[cmdPushDorL] = GenCodeCmdPushDorL;
	cgFuncs[cmdPushCmplx] = GenCodeCmdPushCmplx;

	cgFuncs[cmdPushCharStk] = GenCodeCmdPushCharStk;
	cgFuncs[cmdPushShortStk] = GenCodeCmdPushShortStk;
	cgFuncs[cmdPushIntStk] = GenCodeCmdPushIntStk;
	cgFuncs[cmdPushFloatStk] = GenCodeCmdPushFloatStk;
	cgFuncs[cmdPushDorLStk] = GenCodeCmdPushDorLStk;
	cgFuncs[cmdPushCmplxStk] = GenCodeCmdPushCmplxStk;

	cgFuncs[cmdPushImmt] = GenCodeCmdPushImmt;

	cgFuncs[cmdMovChar] = GenCodeCmdMovChar;
	cgFuncs[cmdMovShort] = GenCodeCmdMovShort;
	cgFuncs[cmdMovInt] = GenCodeCmdMovInt;
	cgFuncs[cmdMovFloat] = GenCodeCmdMovFloat;
	cgFuncs[cmdMovDorL] = GenCodeCmdMovDorL;
	cgFuncs[cmdMovCmplx] = GenCodeCmdMovCmplx;

	cgFuncs[cmdMovCharStk] = GenCodeCmdMovCharStk;
	cgFuncs[cmdMovShortStk] = GenCodeCmdMovShortStk;
	cgFuncs[cmdMovIntStk] = GenCodeCmdMovIntStk;
	cgFuncs[cmdMovFloatStk] = GenCodeCmdMovFloatStk;
	cgFuncs[cmdMovDorLStk] = GenCodeCmdMovDorLStk;
	cgFuncs[cmdMovCmplxStk] = GenCodeCmdMovCmplxStk;

	cgFuncs[cmdReserveV] = GenCodeCmdReserveV;

	cgFuncs[cmdPopCharTop] = GenCodeCmdPopCharTop;
	cgFuncs[cmdPopShortTop] = GenCodeCmdPopShortTop;
	cgFuncs[cmdPopIntTop] = GenCodeCmdPopIntTop;
	cgFuncs[cmdPopFloatTop] = GenCodeCmdPopFloatTop;
	cgFuncs[cmdPopDorLTop] = GenCodeCmdPopDorLTop;
	cgFuncs[cmdPopCmplxTop] = GenCodeCmdPopCmplxTop;

	cgFuncs[cmdPop] = GenCodeCmdPop;

	cgFuncs[cmdDtoI] = GenCodeCmdDtoI;
	cgFuncs[cmdDtoL] = GenCodeCmdDtoL;
	cgFuncs[cmdDtoF] = GenCodeCmdDtoF;
	cgFuncs[cmdItoD] = GenCodeCmdItoD;
	cgFuncs[cmdLtoD] = GenCodeCmdLtoD;
	cgFuncs[cmdItoL] = GenCodeCmdItoL;
	cgFuncs[cmdLtoI] = GenCodeCmdLtoI;

	cgFuncs[cmdIndex] = GenCodeCmdIndex;
	cgFuncs[cmdIndexStk] = GenCodeCmdIndex;

	cgFuncs[cmdCopyDorL] = GenCodeCmdCopyDorL;
	cgFuncs[cmdCopyI] = GenCodeCmdCopyI;

	cgFuncs[cmdGetAddr] = GenCodeCmdGetAddr;

	cgFuncs[cmdSetRange] = GenCodeCmdSetRange;

	cgFuncs[cmdJmp] = GenCodeCmdJmp;

	cgFuncs[cmdJmpZ] = GenCodeCmdJmpZ;
	cgFuncs[cmdJmpNZ] = GenCodeCmdJmpNZ;

	cgFuncs[cmdCall] = GenCodeCmdCall;

	cgFuncs[cmdReturn] = GenCodeCmdReturn;

	cgFuncs[cmdPushVTop] = GenCodeCmdPushVTop;

	cgFuncs[cmdAdd] = GenCodeCmdAdd;
	cgFuncs[cmdSub] = GenCodeCmdSub;
	cgFuncs[cmdMul] = GenCodeCmdMul;
	cgFuncs[cmdDiv] = GenCodeCmdDiv;
	cgFuncs[cmdPow] = GenCodeCmdPow;
	cgFuncs[cmdMod] = GenCodeCmdMod;
	cgFuncs[cmdLess] = GenCodeCmdLess;
	cgFuncs[cmdGreater] = GenCodeCmdGreater;
	cgFuncs[cmdLEqual] = GenCodeCmdLEqual;
	cgFuncs[cmdGEqual] = GenCodeCmdGEqual;
	cgFuncs[cmdEqual] = GenCodeCmdEqual;
	cgFuncs[cmdNEqual] = GenCodeCmdNEqual;
	cgFuncs[cmdShl] = GenCodeCmdShl;
	cgFuncs[cmdShr] = GenCodeCmdShr;
	cgFuncs[cmdBitAnd] = GenCodeCmdBitAnd;
	cgFuncs[cmdBitOr] = GenCodeCmdBitOr;
	cgFuncs[cmdBitXor] = GenCodeCmdBitXor;
	cgFuncs[cmdLogAnd] = GenCodeCmdLogAnd;
	cgFuncs[cmdLogOr] = GenCodeCmdLogOr;
	cgFuncs[cmdLogXor] = GenCodeCmdLogXor;

	cgFuncs[cmdAddL] = GenCodeCmdAddL;
	cgFuncs[cmdSubL] = GenCodeCmdSubL;
	cgFuncs[cmdMulL] = GenCodeCmdMulL;
	cgFuncs[cmdDivL] = GenCodeCmdDivL;
	cgFuncs[cmdPowL] = GenCodeCmdPowL;
	cgFuncs[cmdModL] = GenCodeCmdModL;
	cgFuncs[cmdLessL] = GenCodeCmdLessL;
	cgFuncs[cmdGreaterL] = GenCodeCmdGreaterL;
	cgFuncs[cmdLEqualL] = GenCodeCmdLEqualL;
	cgFuncs[cmdGEqualL] = GenCodeCmdGEqualL;
	cgFuncs[cmdEqualL] = GenCodeCmdEqualL;
	cgFuncs[cmdNEqualL] = GenCodeCmdNEqualL;
	cgFuncs[cmdShlL] = GenCodeCmdShlL;
	cgFuncs[cmdShrL] = GenCodeCmdShrL;
	cgFuncs[cmdBitAndL] = GenCodeCmdBitAndL;
	cgFuncs[cmdBitOrL] = GenCodeCmdBitOrL;
	cgFuncs[cmdBitXorL] = GenCodeCmdBitXorL;
	cgFuncs[cmdLogAndL] = GenCodeCmdLogAndL;
	cgFuncs[cmdLogOrL] = GenCodeCmdLogOrL;
	cgFuncs[cmdLogXorL] = GenCodeCmdLogXorL;

	cgFuncs[cmdAddD] = GenCodeCmdAddD;
	cgFuncs[cmdSubD] = GenCodeCmdSubD;
	cgFuncs[cmdMulD] = GenCodeCmdMulD;
	cgFuncs[cmdDivD] = GenCodeCmdDivD;
	cgFuncs[cmdPowD] = GenCodeCmdPowD;
	cgFuncs[cmdModD] = GenCodeCmdModD;
	cgFuncs[cmdLessD] = GenCodeCmdLessD;
	cgFuncs[cmdGreaterD] = GenCodeCmdGreaterD;
	cgFuncs[cmdLEqualD] = GenCodeCmdLEqualD;
	cgFuncs[cmdGEqualD] = GenCodeCmdGEqualD;
	cgFuncs[cmdEqualD] = GenCodeCmdEqualD;
	cgFuncs[cmdNEqualD] = GenCodeCmdNEqualD;

	cgFuncs[cmdNeg] = GenCodeCmdNeg;
	cgFuncs[cmdNegL] = GenCodeCmdNegL;
	cgFuncs[cmdNegD] = GenCodeCmdNegD;

	cgFuncs[cmdBitNot] = GenCodeCmdBitNot;
	cgFuncs[cmdBitNotL] = GenCodeCmdBitNotL;

	cgFuncs[cmdLogNot] = GenCodeCmdLogNot;
	cgFuncs[cmdLogNotL] = GenCodeCmdLogNotL;

	cgFuncs[cmdIncI] = GenCodeCmdIncI;
	cgFuncs[cmdIncD] = GenCodeCmdIncD;
	cgFuncs[cmdIncL] = GenCodeCmdIncL;

	cgFuncs[cmdDecI] = GenCodeCmdDecI;
	cgFuncs[cmdDecD] = GenCodeCmdDecD;
	cgFuncs[cmdDecL] = GenCodeCmdDecL;

	cgFuncs[cmdAddAtCharStk] = GenCodeCmdAddAtCharStk;
	cgFuncs[cmdAddAtShortStk] = GenCodeCmdAddAtShortStk;
	cgFuncs[cmdAddAtIntStk] = GenCodeCmdAddAtIntStk;
	cgFuncs[cmdAddAtLongStk] = GenCodeCmdAddAtLongStk;
	cgFuncs[cmdAddAtFloatStk] = GenCodeCmdAddAtFloatStk;
	cgFuncs[cmdAddAtDoubleStk] = GenCodeCmdAddAtDoubleStk;
	return true;
}

#pragma warning(disable: 4731)
void ExecutorX86::Run(const char* funcName)
{
	if(!exCode.size())
	{
		strcpy(execError, "ERROR: no code to run");
		return;
	}

	execError[0] = 0;

	stackReallocs = 0;

	unsigned int binCodeStart = static_cast<unsigned int>(reinterpret_cast<long long>(&binCode[20]));

	if(funcName)
	{
		unsigned int funcPos = (unsigned int)-1;
		unsigned int fnameHash = GetStringHash(funcName);
		for(int i = (int)exFunctions.size()-1; i >= 0; i--)
		{
			if(exFunctions[i].nameHash == fnameHash)
			{
				funcPos = exFunctions[i].startInByteCode;
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

			mov edi, varSize ;
			mov ebp, 0h ;

			call eax ; // � ebx ��� ������������ ��������

			pop ebp; // ���������� ���� �����
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

	globalStartInBytecode = 0xffffffff;
	for(unsigned int i = 0; i < exFunctions.size(); i++)
		exFunctions[i].startInByteCode = 0xffffffff;

	memset(&instList[0], 0, sizeof(x86Instruction) * instList.size());
	instList.clear();

	SetParamBase(paramBase);
	SetLastInstruction(&instList[0]);

	EMIT_OP(o_use32);

	pos = 0;
	while(pos < exCode.size())
	{
		const VMCmd &cmd = exCode[pos];

		unsigned int currSize = (int)(GetLastInstruction() - &instList[0]);
		instList.m_size = currSize;
		if(currSize + 64 >= instList.max)
			instList.grow(currSize + 64);
		SetLastInstruction(instList.data + currSize);

		EMIT_OP(o_dd);

		pos++;

		if(cmd.cmd == cmdFuncAddr)
		{
			EMIT_COMMENT("FUNCADDR");

			if(exFunctions[cmd.argument].funcPtr == NULL)
			{
				EMIT_OP_REG_LABEL(o_lea, rEAX, LABEL_FUNCTION + exFunctions[cmd.argument].address, binCodeStart);
				EMIT_OP_REG(o_push, rEAX);
			}else{
				EMIT_OP_NUM(o_push, (int)(intptr_t)exFunctions[cmd.argument].funcPtr);
			}
		}else if(cmd.cmd == cmdCallStd){
			EMIT_COMMENT("CALLSTD");

			unsigned int bytesToPop = exFunctions[cmd.argument].bytesToPop;
			EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)exFunctions[cmd.argument].funcPtr);
			EMIT_OP_REG(o_call, rECX);
			EMIT_OP_REG_NUM(o_add, rESP, bytesToPop);
			if(exFunctions[cmd.argument].retType == ExternFuncInfo::RETURN_INT)
			{
				EMIT_OP_REG(o_push, rEAX);
			}else if(exFunctions[cmd.argument].retType == ExternFuncInfo::RETURN_DOUBLE){
				EMIT_OP_REG(o_push, rEAX);
				EMIT_OP_REG(o_push, rEAX);
				EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
			}else if(exFunctions[cmd.argument].retType == ExternFuncInfo::RETURN_LONG){
				EMIT_OP_REG(o_push, rEDX);
				EMIT_OP_REG(o_push, rEAX);
			}
		}else{
			cgFuncs[cmd.cmd](cmd);
		}
	}
	EMIT_LABEL(LABEL_GLOBAL + pos);
	EMIT_OP_REG(o_pop, rEBP);
	EMIT_OP(o_ret);
	instList.resize((int)(GetLastInstruction() - &instList[0]));

#ifdef NULLC_LOG_FILES
	FILE *fAsm = fopen("asmX86.txt", "wb");
	char instBuf[128];
	for(unsigned int i = 0; i < instList.size(); i++)
	{
		if(instList[i].name == o_dd || instList[i].name == o_other)
			continue;
		instList[i].Decode(instBuf);
		fprintf(fAsm, "%s\r\n", instBuf);
	}
	fclose(fAsm);
#endif

	// Translate to x86
	unsigned char *bytecode = binCode+20;
	unsigned char *code = bytecode;

	instAddress.resize(exCode.size());

	x86ClearLabels();

	pos = 0;

	x86Instruction *curr = &instList[0];

	for(unsigned int i = 0, e = instList.size(); i != e; i++)
	{
		x86Instruction &cmd = *curr;	// Syntax sugar + too lazy to rewrite switch contents
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
				code += x86LEA(code, cmd.argA.reg, cmd.argB.labelID, cmd.argB.ptrNum);
			}else{
				if(cmd.argB.ptrMult > 1)
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
			code += x86JMP(code, cmd.argA.labelID, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_ja:
			code += x86Jcc(code, cmd.argA.labelID, condA, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jae:
			code += x86Jcc(code, cmd.argA.labelID, condAE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jb:
			code += x86Jcc(code, cmd.argA.labelID, condB, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jbe:
			code += x86Jcc(code, cmd.argA.labelID, condBE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jc:
			code += x86Jcc(code, cmd.argA.labelID, condC, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_je:
			code += x86Jcc(code, cmd.argA.labelID, condE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jz:
			code += x86Jcc(code, cmd.argA.labelID, condZ, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jg:
			code += x86Jcc(code, cmd.argA.labelID, condG, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jl:
			code += x86Jcc(code, cmd.argA.labelID, condL, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jne:
			code += x86Jcc(code, cmd.argA.labelID, condNE, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jnp:
			code += x86Jcc(code, cmd.argA.labelID, condNP, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jnz:
			code += x86Jcc(code, cmd.argA.labelID, condNZ, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_jp:
			code += x86Jcc(code, cmd.argA.labelID, condP, cmd.argA.labelID & JUMP_NEAR ? true : false);
			break;
		case o_call:
			if(cmd.argA.type == x86Argument::argLabel)
				code += x86CALL(code, cmd.argA.labelID);
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
				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADD(code, cmd.argA.reg, cmd.argB.reg);
				else
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
			instAddress[pos] = code;	// Save VM instruction address in x86 bytecode

			if(pos == (int)exLinker->offsetToGlobalCode)
				code += x86PUSH(code, rEBP);
			pos++;
			break;
		case o_label:
			x86AddLabel(code, cmd.labelID);
			break;
		case o_use32:
			break;
		case o_other:
			break;
		}
		curr++;
	}
	binCodeSize = (unsigned int)(code-bytecode);

	x86SatisfyJumps(instAddress);

	for(unsigned int i = 0; i < exFunctions.size(); i++)
		if(exFunctions[i].address != -1)
			exFunctions[i].startInByteCode = (int)(instAddress[exFunctions[i].address] - bytecode);
	globalStartInBytecode = (int)(instAddress[exLinker->offsetToGlobalCode] - bytecode);

	return true;
}

const char* ExecutorX86::GetResult()
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

const char*	ExecutorX86::GetExecError()
{
	return execError;
}

char* ExecutorX86::GetVariableData()
{
	return paramData;
}

#endif
