#include "CodeGen_X86.h"
#include "StdLib_X86.h"

#ifdef NULLC_BUILD_X86_JIT

x86Instruction	*x86Op = NULL;
ExternFuncInfo	*x86Functions = NULL;
int				*x86Continue = NULL;

bool			x86LookBehind = true;

unsigned int optiCount = 0;
unsigned int GetOptimizationCount()
{
	return optiCount;
}

#ifdef NULLC_LOG_FILES
void EMIT_COMMENT(const char* text)
{
	x86Op->name = o_other;
	x86Op->comment = text;
	x86Op++;
}
#else
#define EMIT_COMMENT(x)
#endif

void EMIT_LABEL(unsigned int labelID)
{
	x86Op->name = o_label;
	x86Op->labelID = labelID;
	x86Op++;
}
void EMIT_OP(x86Command op)
{
	x86Op->name = op;
	x86Op++;
}
void EMIT_OP_LABEL(x86Command op, unsigned int labelID)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argLabel;
	x86Op->argA.labelID = labelID;
	x86Op++;
}
void EMIT_OP_REG(x86Command op, x86Reg reg1)
{
#ifdef NULLC_OPTIMIZE_X86
	if(x86LookBehind && op == o_pop)
	{
		x86Instruction &prev = x86Op[-1];
		// Optimizations for "push num ... pop reg", "push [location] ... pop reg" and "push regA ... pop regB"
		if(prev.name == o_push && (prev.argA.type == x86Argument::argNumber || prev.argA.type == x86Argument::argReg || prev.argA.type == x86Argument::argPtr))
		{
			prev.name = o_mov;
			prev.argB = prev.argA;
			prev.argA = x86Argument(reg1);
			if(prev.argA == prev.argB)
			{
				optiCount++;
				x86Op--;
			}
			optiCount++;
			return;
		}
	}
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op++;
}
void EMIT_OP_FPUREG(x86Command op, x87Reg reg1)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argFPReg;
	x86Op->argA.fpArg = reg1;
	x86Op++;
}
void EMIT_OP_NUM(x86Command op, unsigned int num)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argNumber;
	x86Op->argA.num = num;
	x86Op++;
}

void EMIT_OP_ADDR(x86Command op, x86Size size, unsigned int addr)
{
#ifdef NULLC_OPTIMIZE_X86
	if(x86LookBehind && op == o_pop)
	{
		x86Instruction &prev = x86Op[-1];
		// Optimizations for "push num ... pop [location]" and "push register ... pop [location]"
		if(prev.name == o_push && (prev.argA.type == x86Argument::argNumber || prev.argA.type == x86Argument::argReg))
		{
			prev.name = o_mov;
			prev.argB = prev.argA;
			prev.argA = x86Argument(size, addr);

			optiCount++;
			return;
		}
	}
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrNum = addr;
	x86Op++;
}
void EMIT_OP_RPTR(x86Command op, x86Size size, x86Reg reg2, unsigned int shift)
{
#ifdef NULLC_OPTIMIZE_X86
	if(x86LookBehind && op == o_pop)
	{
		x86Instruction &prev = x86Op[-1];
		// Optimizations for "push num ... pop [reg+location]" and "push register ... pop [reg+location]"
		if(prev.name == o_push && (prev.argA.type == x86Argument::argNumber || prev.argA.type == x86Argument::argReg))
		{
			prev.name = o_mov;
			prev.argB = prev.argA;
			prev.argA = x86Argument(size, reg2, shift);
			optiCount++;
			return;
		}
	}
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrReg[0] = reg2;
	x86Op->argA.ptrNum = shift;
	x86Op++;
}
void EMIT_OP_REG_NUM(x86Command op, x86Reg reg1, unsigned int num)
{
#ifdef NULLC_OPTIMIZE_X86
	if(x86LookBehind)
	{
		x86Instruction &prev = x86Op[-1];
		// sub reg, num; add reg, num
		if(op == o_sub && prev.name == o_add && prev.argA.type == x86Argument::argReg && prev.argB.type == x86Argument::argNumber)
		{
			if(reg1 == prev.argA.reg && (int)num == prev.argB.num)
			{
				prev.argA.type = x86Argument::argNone;
				prev.argB.type = x86Argument::argNone;
				x86Op--;
				optiCount++;
				return;
			}
		}
		// add reg, num; sub reg, num
		if(op == o_add && prev.name == o_sub && prev.argA.type == x86Argument::argReg && prev.argB.type == x86Argument::argNumber)
		{
			if(reg1 == prev.argA.reg && (int)num == prev.argB.num)
			{
				prev.argA.type = x86Argument::argNone;
				prev.argB.type = x86Argument::argNone;
				x86Op--;
				optiCount++;
				return;
			}
		}
	}
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argNumber;
	x86Op->argB.num = num;
	x86Op++;
}
void EMIT_OP_REG_REG(x86Command op, x86Reg reg1, x86Reg reg2)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
	x86Op++;
}
void EMIT_OP_REG_ADDR(x86Command op, x86Reg reg1, x86Size size, unsigned int addr)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argPtr;
	x86Op->argB.ptrSize = size;
	x86Op->argB.ptrNum = addr;
	x86Op++;
}
void EMIT_OP_REG_RPTR(x86Command op, x86Reg reg1, x86Size size, x86Reg reg2, unsigned int shift)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argPtr;
	x86Op->argB.ptrSize = size;
	x86Op->argB.ptrReg[0] = reg2;
	x86Op->argB.ptrNum = shift;
	x86Op++;
}
void EMIT_OP_REG_LABEL(x86Command op, x86Reg reg1, unsigned int labelID, unsigned int shift)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argPtrLabel;
	x86Op->argB.labelID = labelID;
	x86Op->argB.ptrNum = shift;
	x86Op++;
}
void EMIT_OP_ADDR_REG(x86Command op, x86Size size, unsigned int addr, x86Reg reg2)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrNum = addr;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
	x86Op++;
}

void EMIT_OP_RPTR_REG(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, x86Reg reg2)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrReg[0] = reg1;
	x86Op->argA.ptrNum = shift;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
	x86Op++;
}

void EMIT_OP_RPTR_NUM(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, unsigned int num)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrReg[0] = reg1;
	x86Op->argA.ptrNum = shift;
	x86Op->argB.type = x86Argument::argNumber;
	x86Op->argB.num = num;
	x86Op++;
}

void OptimizationLookBehind(bool allow)
{
	x86LookBehind = allow;
}

static unsigned int paramBase = 0;
static unsigned int aluLabels = LABEL_ALU;

void SetParamBase(unsigned int base)
{
	paramBase = base;
	aluLabels = LABEL_ALU;
}

void SetFunctionList(ExternFuncInfo *list)
{
	x86Functions = list;
}

void SetContinuePtr(int* continueVar)
{
	x86Continue = continueVar;
}

void SetLastInstruction(x86Instruction *pos)
{
	x86Op = pos;
}

x86Instruction* GetLastInstruction()
{
	return x86Op;
}

unsigned int GetLastALULabel()
{
	return aluLabels;
}

void GenCodeCmdNop(VMCmd cmd)
{
	(void)cmd;
	EMIT_OP(o_nop);
}

void GenCodeCmdPushChar(VMCmd cmd)
{
	EMIT_COMMENT("PUSH char");

	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_REG_ADDR(o_movsx, rEAX, sBYTE, cmd.argument+paramBase);
	else
		EMIT_OP_REG_RPTR(o_movsx, rEAX, sBYTE, rEBP, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushShort(VMCmd cmd)
{
	EMIT_COMMENT("PUSH short");

	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_REG_ADDR(o_movsx, rEAX, sWORD, cmd.argument+paramBase);
	else
		EMIT_OP_REG_RPTR(o_movsx, rEAX, sWORD, rEBP, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushInt(VMCmd cmd)
{
	EMIT_COMMENT("PUSH int");

	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase);
	else
		EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase);
}

void GenCodeCmdPushFloat(VMCmd cmd)
{
	EMIT_COMMENT("PUSH float");

	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR(o_fld, sDWORD, cmd.argument+paramBase);
	else
		EMIT_OP_RPTR(o_fld, sDWORD, rEBP, cmd.argument+paramBase);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdPushDorL(VMCmd cmd)
{
	EMIT_COMMENT("PUSH DorL");

	if(cmd.flag == ADDRESS_ABOLUTE)
	{
		EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase+4);
		EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase);
	}else{
		EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase+4);
		EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase);
	}
}

void GenCodeCmdPushCmplx(VMCmd cmd)
{
	EMIT_COMMENT("PUSH complex");
	if(cmd.helper == 0)
		return;
	if(cmd.helper <= 32)
	{
		unsigned int currShift = cmd.helper;
		while(currShift >= 4)
		{
			currShift -= 4;
			if(cmd.flag == ADDRESS_ABOLUTE)
				EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase+currShift);
			else
				EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase+currShift);
		}
		assert(currShift == 0);
	}else{
		EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		if(cmd.flag == ADDRESS_ABOLUTE)
			EMIT_OP_REG_NUM(o_mov, rESI, cmd.argument+paramBase);
		else
			EMIT_OP_REG_RPTR(o_lea, rESI, sDWORD, rEBP, cmd.argument+paramBase);
		EMIT_OP_REG_REG(o_mov, rEDI, rESP);
		EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
	}
}

void GenCodeCmdPushCharStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH char stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_REG_RPTR(o_movsx, rEAX, sBYTE, rEDX, cmd.argument);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushShortStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH short stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_REG_RPTR(o_movsx, rEAX, sWORD, rEDX, cmd.argument);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushIntStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH int stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument);
}

void GenCodeCmdPushFloatStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH float stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fld, sDWORD, rEDX, cmd.argument);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdPushDorLStk(VMCmd cmd)
{
	EMIT_COMMENT(cmd.flag ? "PUSH double stack" : "PUSH long stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument+4);
	EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument);
}

void GenCodeCmdPushCmplxStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH complex stack");
	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	if(cmd.helper == 0)
		return;
	if(cmd.helper <= 32)
	{
		unsigned int currShift = cmd.helper;
		while(currShift >= 4)
		{
			currShift -= 4;
			EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument+currShift);
		}
		assert(currShift == 0);
	}else{
		EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		EMIT_OP_REG_RPTR(o_lea, rESI, sDWORD, rEDX, cmd.argument);
		EMIT_OP_REG_REG(o_mov, rEDI, rESP);
		EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
	}
}


void GenCodeCmdPushImmt(VMCmd cmd)
{
	EMIT_COMMENT("PUSHIMMT");
	
	EMIT_OP_NUM(o_push, cmd.argument);
}


void GenCodeCmdMovChar(VMCmd cmd)
{
	EMIT_COMMENT("MOV char");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR_REG(o_mov, sBYTE, cmd.argument+paramBase, rEBX);
	else
		EMIT_OP_RPTR_REG(o_mov, sBYTE, rEBP, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovShort(VMCmd cmd)
{
	EMIT_COMMENT("MOV short");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR_REG(o_mov, sWORD, cmd.argument+paramBase, rEBX);
	else
		EMIT_OP_RPTR_REG(o_mov, sWORD, rEBP, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovInt(VMCmd cmd)
{
	EMIT_COMMENT("MOV int");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR_REG(o_mov, sDWORD, cmd.argument+paramBase, rEBX);
	else
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBP, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovFloat(VMCmd cmd)
{
	EMIT_COMMENT("MOV float");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR(o_fstp, sDWORD, cmd.argument+paramBase);
	else
		EMIT_OP_RPTR(o_fstp, sDWORD, rEBP, cmd.argument+paramBase);
}

void GenCodeCmdMovDorL(VMCmd cmd)
{
	EMIT_COMMENT("MOV DorL");

	if(cmd.flag == ADDRESS_ABOLUTE)
	{
		EMIT_OP_ADDR(o_pop, sDWORD, cmd.argument+paramBase);
		EMIT_OP_ADDR(o_pop, sDWORD, cmd.argument+paramBase + 4);
	}else{
		EMIT_OP_RPTR(o_pop, sDWORD, rEBP, cmd.argument+paramBase);
		EMIT_OP_RPTR(o_pop, sDWORD, rEBP, cmd.argument+paramBase + 4);
	}
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
}

void GenCodeCmdMovCmplx(VMCmd cmd)
{
	EMIT_COMMENT("MOV complex");
	if(cmd.helper == 0)
		return;
	if(cmd.helper <= 32)
	{
		unsigned int currShift = 0;
		while(currShift < cmd.helper)
		{
			if(cmd.flag == ADDRESS_ABOLUTE)
				EMIT_OP_ADDR(o_pop, sDWORD, cmd.argument+paramBase + currShift);
			else
				EMIT_OP_RPTR(o_pop, sDWORD, rEBP, cmd.argument+paramBase + currShift);
			currShift += 4;
		}
		EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
		assert(currShift == cmd.helper);
	}else{
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		EMIT_OP_REG_REG(o_mov, rESI, rESP);
		if(cmd.flag == ADDRESS_ABOLUTE)
			EMIT_OP_REG_NUM(o_mov, rEDI, cmd.argument+paramBase);
		else
			EMIT_OP_REG_RPTR(o_lea, rEDI, sDWORD, rEBP, cmd.argument+paramBase);
		EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
	}
}

void GenCodeCmdMovCharStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV char stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sBYTE, rEDX, cmd.argument, rEBX);
}

void GenCodeCmdMovShortStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV short stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sWORD, rEDX, cmd.argument, rEBX);
}

void GenCodeCmdMovIntStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV int stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rEDX, cmd.argument, rEBX);
}

void GenCodeCmdMovFloatStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV float stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sDWORD, rEDX, cmd.argument);
}

void GenCodeCmdMovDorLStk(VMCmd cmd)
{
	EMIT_COMMENT(cmd.flag ? "MOV double stack" : "MOV long stack");

	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	if(cmd.flag)
	{
		EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
		EMIT_OP_RPTR(o_fstp, sQWORD, rEDX, cmd.argument);
	}else{
		EMIT_OP_RPTR(o_pop, sDWORD, rEDX, cmd.argument);
		EMIT_OP_RPTR(o_pop, sDWORD, rEDX, cmd.argument + 4);
		EMIT_OP_REG_NUM(o_sub, rESP, 8);
	}
}

void GenCodeCmdMovCmplxStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV complex stack");
	EMIT_OP_REG(o_pop, rEDX);

	EMIT_OP_REG_REG(o_test, rEDX, rEDX);
	EMIT_OP_LABEL(o_jnz, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	if(cmd.helper == 0)
		return;
	if(cmd.helper <= 32)
	{
		unsigned int currShift = 0;
		while(currShift < cmd.helper)
		{
			EMIT_OP_RPTR(o_pop, sDWORD, rEDX, cmd.argument + currShift);
			currShift += 4;
		}
		EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
		assert(currShift == cmd.helper);
	}else{
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		EMIT_OP_REG_REG(o_mov, rESI, rESP);
		EMIT_OP_REG_RPTR(o_lea, rEDI, sDWORD, rEDX, cmd.argument);
		EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
	}
}

void GenCodeCmdPop(VMCmd cmd)
{
	EMIT_COMMENT("POP");

	EMIT_OP_REG_NUM(o_add, rESP, cmd.argument);
}


void GenCodeCmdDtoI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DTOI");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fistp, sDWORD, rESP, 4);
	EMIT_OP_REG_NUM(o_add, rESP, 4);
}

void GenCodeCmdDtoL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DTOL");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fistp, sQWORD, rESP, 0);
}

void GenCodeCmdDtoF(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DTOF");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sDWORD, rESP, 4);
	EMIT_OP_REG_NUM(o_add, rESP, 4);
}

void GenCodeCmdItoD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ITOD");

	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_REG(o_push, rEAX);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdLtoD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LTOD");

	EMIT_OP_RPTR(o_fild, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdItoL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ITOL");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP(o_cdq);
	EMIT_OP_REG(o_push, rEDX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdLtoI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LTOI");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_RPTR(o_xchg, rEAX, sDWORD, rESP, 0);
}


void GenCodeCmdIndex(VMCmd cmd)
{
	EMIT_COMMENT("IMUL int");

	if(cmd.cmd == cmdIndex)
	{
		EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, 0, cmd.argument);
	}else{
		EMIT_OP_REG_RPTR(o_mov, rECX, sDWORD, rESP, 8);
		EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rECX);
	}
	EMIT_OP_LABEL(o_jb, aluLabels);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;

	EMIT_OP_REG(o_pop, rEAX);	// Take index
	// Multiply it
	if(cmd.helper == 2)
	{
		EMIT_OP_REG_NUM(o_shl, rEAX, 1);
	}else if(cmd.helper == 4){
		EMIT_OP_REG_NUM(o_shl, rEAX, 2);
	}else if(cmd.helper == 8){
		EMIT_OP_REG_NUM(o_shl, rEAX, 3);
	}else if(cmd.helper == 16){
		EMIT_OP_REG_NUM(o_shl, rEAX, 4);
	}else{
		EMIT_OP_REG_NUM(o_imul, rEAX, cmd.helper);
	}
	if(cmd.cmd == cmdIndex)
	{
		EMIT_OP_RPTR_REG(o_add, sDWORD, rESP, 0, rEAX);	// Add it to address
	}else{
		EMIT_OP_REG(o_pop, rEDX);
		EMIT_OP_REG_REG(o_add, rEAX, rEDX);	// Add it to address
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
	}
}

void GenCodeCmdCopyDorL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("COPY qword");

	EMIT_OP_REG_RPTR(o_mov, rEDX, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 4);
	EMIT_OP_REG(o_push, rEAX);
	EMIT_OP_REG(o_push, rEDX);
}

void GenCodeCmdCopyI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("COPY dword");

	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 0);
	EMIT_OP_REG(o_push, rEAX);
}


void GenCodeCmdGetAddr(VMCmd cmd)
{
	EMIT_COMMENT("GETADDR");

	if(cmd.helper)
		EMIT_OP_REG_RPTR(o_lea, rEAX, sDWORD, rEBP, cmd.argument + paramBase);
	else
		EMIT_OP_REG_NUM(o_mov, rEAX, cmd.argument + paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdFuncAddr(VMCmd cmd)
{
	EMIT_COMMENT("FUNCADDR");

	if(x86Functions[cmd.argument].funcPtr == NULL)
	{
		EMIT_OP_REG_LABEL(o_lea, rEAX, LABEL_GLOBAL + x86Functions[cmd.argument].address, 0);
		EMIT_OP_REG(o_push, rEAX);
	}else{
		EMIT_OP_NUM(o_push, (int)(intptr_t)x86Functions[cmd.argument].funcPtr);
	}
}

void GenCodeCmdSetRange(VMCmd cmd)
{
	unsigned int elCount = x86Op[-1].argA.num;

	EMIT_COMMENT("SETRANGE");

	EMIT_OP_REG(o_pop, rEBX);
	unsigned int typeSizeD[] = { 1, 2, 4, 8, 4, 8 };

	// start address
	EMIT_OP_REG_RPTR(o_lea, rEBX, sDWORD, rEBP, paramBase + cmd.argument);
	// end address
	EMIT_OP_REG_RPTR(o_lea, rECX, sDWORD, rEBP, paramBase + cmd.argument + (elCount - 1) * typeSizeD[(cmd.helper>>2)&0x00000007]);
	if(cmd.helper == DTYPE_FLOAT)
	{
		EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	}else{
		EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 0);
		EMIT_OP_REG_RPTR(o_mov, rEDX, sDWORD, rESP, 4);
	}
	EMIT_LABEL(aluLabels);
	EMIT_OP_REG_REG(o_cmp, rEBX, rECX);
	EMIT_OP_LABEL(o_jg, aluLabels + 1);

	switch(cmd.helper)
	{
	case DTYPE_DOUBLE:
	case DTYPE_LONG:
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBX, 4, rEDX);
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBX, 0, rEAX);
		break;
	case DTYPE_FLOAT:
		EMIT_OP_RPTR(o_fst, sDWORD, rEBX, 0);
		break;
	case DTYPE_INT:
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBX, 0, rEAX);
		break;
	case DTYPE_SHORT:
		EMIT_OP_RPTR_REG(o_mov, sWORD, rEBX, 0, rEAX);
		break;
	case DTYPE_CHAR:
		EMIT_OP_RPTR_REG(o_mov, sBYTE, rEBX, 0, rEAX);
		break;
	}
	EMIT_OP_REG_NUM(o_add, rEBX, typeSizeD[(cmd.helper>>2)&0x00000007]);
	EMIT_OP_LABEL(o_jmp, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	if(cmd.helper == DTYPE_FLOAT)
		EMIT_OP_FPUREG(o_fstp, rST0);
	aluLabels += 2;
}


void GenCodeCmdJmp(VMCmd cmd)
{
	EMIT_COMMENT("JMP");
	EMIT_OP_LABEL(o_jmp, LABEL_GLOBAL | JUMP_NEAR | cmd.argument);
}


void GenCodeCmdJmpZ(VMCmd cmd)
{
	EMIT_COMMENT("JMPZ int");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_test, rEAX, rEAX);
	EMIT_OP_LABEL(o_jz, LABEL_GLOBAL | JUMP_NEAR | cmd.argument);
}

void GenCodeCmdJmpNZ(VMCmd cmd)
{
	EMIT_COMMENT("JMPNZ int");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_test, rEAX, rEAX);
	EMIT_OP_LABEL(o_jnz, LABEL_GLOBAL | JUMP_NEAR | cmd.argument);
}

void GenCodeCmdCallEpilog(unsigned int size)
{
	if(size == 0)
		return;
	if(size <= 32)
	{
		unsigned int currShift = 0;
		while(currShift < size)
		{
			EMIT_OP_RPTR(o_pop, sDWORD, rEDI, paramBase + currShift);
			currShift += 4;
		}
		assert(currShift == size);
	}else{
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		EMIT_OP_REG_REG(o_mov, rESI, rESP);
		EMIT_OP_REG_RPTR(o_lea, rEDI, sDWORD, rEDI, paramBase);
		EMIT_OP_REG_NUM(o_mov, rECX, size >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
		EMIT_OP_REG_NUM(o_add, rESP, size);
	}
}


void GenCodeCmdCallProlog(VMCmd cmd)
{
	if(cmd.helper & bitRetSimple)
	{
		if((asmOperType)(cmd.helper & 0x0FFF) == OTYPE_INT)
		{
			EMIT_OP_REG(o_push, rEAX);
		}else{	// double or long
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEDX);
		}
	}else{
		assert(cmd.helper % 4 == 0);
		if(cmd.helper == 4)
		{
			EMIT_OP_REG(o_push, rEAX);
		}else if(cmd.helper == 8){
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEDX);
		}else if(cmd.helper == 12){
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEDX);
			EMIT_OP_REG(o_push, rECX);
		}else if(cmd.helper == 16){
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEDX);
			EMIT_OP_REG(o_push, rECX);
			EMIT_OP_REG(o_push, rEBX);
		}else if(cmd.helper > 16){
			EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);

			EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

			EMIT_OP_REG_RPTR(o_lea, rESI, sDWORD, rEAX, paramBase);
			EMIT_OP_REG_REG(o_mov, rEDI, rESP);
			EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
			EMIT_OP(o_rep_movsd);

			EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
		}
	}
}

void GenCodeCmdCall(VMCmd cmd)
{
	EMIT_COMMENT("CALL");

	if(x86Functions[cmd.argument].address != -1)
	{
		GenCodeCmdCallEpilog(x86Functions[cmd.argument].bytesToPop);

		EMIT_OP_ADDR(o_push, sDWORD, paramBase-4);
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-4, rESP);
		EMIT_OP_LABEL(o_call, LABEL_GLOBAL | JUMP_NEAR | x86Functions[cmd.argument].address);
		EMIT_OP_ADDR(o_pop, sDWORD, paramBase-4);
		
		GenCodeCmdCallProlog(cmd);
	}else{
		unsigned int bytesToPop = x86Functions[cmd.argument].bytesToPop;

		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-8, rESP);
		EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)x86Functions[cmd.argument].funcPtr);
		EMIT_OP_REG(o_call, rECX);

		static int continueLabel = 0;
		EMIT_OP_REG_ADDR(o_mov, rECX, sDWORD, (int)(intptr_t)x86Continue);
		EMIT_OP_REG_REG(o_test, rECX, rECX);
		EMIT_OP_LABEL(o_jnz, aluLabels);
		EMIT_OP_REG_REG(o_mov, rECX, rESP);	// esp is very likely to contain neither 0 or ~0, so we can distinguish
		EMIT_OP(o_int);						// array out of bounds and function with no return errors from this one
		EMIT_LABEL(aluLabels);
		aluLabels++;

		EMIT_OP_REG_NUM(o_add, rESP, bytesToPop);
		if(x86Functions[cmd.argument].retType == ExternFuncInfo::RETURN_INT)
		{
			EMIT_OP_REG(o_push, rEAX);
		}else if(x86Functions[cmd.argument].retType == ExternFuncInfo::RETURN_DOUBLE){
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
		}else if(x86Functions[cmd.argument].retType == ExternFuncInfo::RETURN_LONG){
			EMIT_OP_REG(o_push, rEDX);
			EMIT_OP_REG(o_push, rEAX);
		}
	}
}

void GenCodeCmdCallPtr(VMCmd cmd)
{
	EMIT_COMMENT("CALLPTR");

	assert(cmd.argument >= 4);
	EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, cmd.argument-4, ~0u);
	EMIT_OP_LABEL(o_jne, aluLabels);

	// external function call
	{
		EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, cmd.argument);
		EMIT_OP_REG(o_call, rEAX);
	 
		static int continueLabel = 0;
		EMIT_OP_REG_ADDR(o_mov, rECX, sDWORD, (int)(intptr_t)x86Continue);
		EMIT_OP_REG_REG(o_test, rECX, rECX);
		EMIT_OP_LABEL(o_jnz, aluLabels + 3);
		EMIT_OP_REG_REG(o_mov, rECX, rESP); // esp is very likely to contain neither 0 or ~0, so we can distinguish
		EMIT_OP(o_int);						// array out of bounds and function with no return errors from this one
		EMIT_LABEL(aluLabels + 3);
	 
		EMIT_OP_REG_NUM(o_add, rESP, cmd.argument + 4);
		if(cmd.helper == (bitRetSimple | OTYPE_INT))
		{
			EMIT_OP_REG(o_push, rEAX);
		}else if(cmd.helper == (bitRetSimple | OTYPE_DOUBLE)){
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
		}else if(cmd.helper == (bitRetSimple | OTYPE_LONG)){
			EMIT_OP_REG(o_push, rEDX);
			EMIT_OP_REG(o_push, rEAX);
		}

		EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	}

	// internal function call
	{
		EMIT_LABEL(aluLabels);

		GenCodeCmdCallEpilog(cmd.argument);

		EMIT_OP_REG(o_pop, rEAX);
		EMIT_OP_ADDR(o_push, sDWORD, paramBase-4);
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-4, rESP);

		EMIT_OP_REG_REG(o_test, rEAX, rEAX);
		EMIT_OP_LABEL(o_jnz, aluLabels + 1);
		EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
		EMIT_OP_NUM(o_int, 3);
		EMIT_LABEL(aluLabels + 1);

		EMIT_OP_REG(o_call, rEAX);
		EMIT_OP_ADDR(o_pop, sDWORD, paramBase-4);

		GenCodeCmdCallProlog(cmd);
	}

	EMIT_LABEL(aluLabels + 2);

	aluLabels += 4;
}

void GenCodeCmdReturn(VMCmd cmd)
{
	EMIT_COMMENT("RET");

	if(cmd.flag & bitRetError)
	{
		EMIT_OP_REG_NUM(o_mov, rECX, 0xffffffff);
		EMIT_OP_NUM(o_int, 3);
		return;
	}
	if(cmd.argument == 0)
	{
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
		EMIT_OP(o_ret);
		return;
	}
	if(cmd.flag != OTYPE_COMPLEX)
	{
		if(cmd.flag == OTYPE_INT)
		{
			EMIT_OP_REG(o_pop, rEAX);
		}else{
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
		if(cmd.helper == 0)
			EMIT_OP_REG_NUM(o_mov, rEBX, cmd.flag);
	}else{
		if(cmd.argument == 4)
		{
			EMIT_OP_REG(o_pop, rEAX);
		}else if(cmd.argument == 8){
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}else if(cmd.argument == 12){
			EMIT_OP_REG(o_pop, rECX);
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}else if(cmd.argument == 16){
			EMIT_OP_REG(o_pop, rEBX);
			EMIT_OP_REG(o_pop, rECX);
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}else{
			EMIT_OP_REG_REG(o_mov, rEBX, rEDI);
			EMIT_OP_REG_REG(o_mov, rESI, rESP);

			EMIT_OP_REG_RPTR(o_lea, rEDI, sDWORD, rEDI, paramBase);
			EMIT_OP_REG_NUM(o_mov, rECX, cmd.argument >> 2);
			EMIT_OP(o_rep_movsd);

			EMIT_OP_REG_REG(o_mov, rEDI, rEBX);

			EMIT_OP_REG_NUM(o_add, rESP, cmd.argument);
		}
		if(cmd.argument > 16)
			EMIT_OP_REG_REG(o_mov, rEAX, rEDI);
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
		if(cmd.helper == 0)
			EMIT_OP_REG_NUM(o_mov, rEBX, 16);
	}
	EMIT_OP(o_ret);
}


void GenCodeCmdPushVTop(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("PUSHT");

	EMIT_OP_REG(o_push, rEBP);
	EMIT_OP_REG_REG(o_mov, rEBP, rEDI);

	if(cmd.argument)
		EMIT_OP_REG_NUM(o_add, rEDI, (cmd.argument & 0xfffffff0) + 16);
}

void GenCodeCmdAdd(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ADD int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_add, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdSub(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SUB int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_sub, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdMul(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MUL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG(o_imul, rEDX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdDiv(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DIV int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_RPTR(o_xchg, rEAX, sDWORD, rESP, 0);
	EMIT_OP(o_cdq);
	EMIT_OP_RPTR(o_idiv, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_xchg, rEAX, sDWORD, rESP, 0);
}

void GenCodeCmdPow(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("POW int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)intPow);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG(o_push, rEDX);
}

void GenCodeCmdMod(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MOD int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_RPTR(o_xchg, rEAX, sDWORD, rESP, 0);
	EMIT_OP(o_cdq);
	EMIT_OP_RPTR(o_idiv, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_xchg, rEDX, sDWORD, rESP, 0);
}

void GenCodeCmdLess(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LESS int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setl, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdGreater(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GREATER int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setg, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdLEqual(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LEQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setle, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdGEqual(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GEQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setge, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdEqual(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("EQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_sete, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdNEqual(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setne, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdShl(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SHL int");
	EMIT_OP_REG(o_pop, rECX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_sal, rEAX, rECX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdShr(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SHR int");
	EMIT_OP_REG(o_pop, rECX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_sar, rEAX, rECX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdBitAnd(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BAND int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_and, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdBitOr(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BOR int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_or, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdBitXor(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BXOR int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_xor, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdLogAnd(VMCmd cmd)
{
	(void)cmd;
	assert(!"Unknown command cmdLogAnd");
}

void GenCodeCmdLogOr(VMCmd cmd)
{
	(void)cmd;
	assert(!"Unknown command cmdLogOr");
}

void GenCodeCmdLogXor(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LXOR int");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, 0, 0);
	EMIT_OP_REG(o_setne, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, 4, 0);
	EMIT_OP_REG(o_setne, rECX);
	EMIT_OP_REG_REG(o_xor, rEAX, rECX);
	EMIT_OP_REG(o_pop, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}


void GenCodeCmdAddL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ADD long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_add, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_adc, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdSubL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SUB long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_sub, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_sbb, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdMulL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MUL long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longMul);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdDivL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DIV long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longDiv);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdPowL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("POW long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longPow);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdModL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MOD long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longMod);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdLessL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LESS long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jg, aluLabels);
	EMIT_OP_LABEL(o_jl, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jae, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdGreaterL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GREATER long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jl, aluLabels);
	EMIT_OP_LABEL(o_jg, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jbe, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdLEqualL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LEQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jg, aluLabels);
	EMIT_OP_LABEL(o_jl, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_ja, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdGEqualL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GEQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jl, aluLabels);
	EMIT_OP_LABEL(o_jg, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jb, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdEqualL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("EQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jne, aluLabels);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jne, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 2;
}

void GenCodeCmdNEqualL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jne, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_je, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdShlL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SHL long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longShl);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdShrL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SHR long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longShr);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdBitAndL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BAND long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_and, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_and, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdBitOrL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BOR long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_or, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_or, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdBitXorL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BXOR long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_xor, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_xor, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdLogAndL(VMCmd cmd)
{
	(void)cmd;
	assert(!"Unknown command cmdLogAndL");
}

void GenCodeCmdLogOrL(VMCmd cmd)
{
	(void)cmd;
	assert(!"Unknown command cmdLogOrL");
}

void GenCodeCmdLogXorL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LXOR long");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_or, rEBX, sDWORD, rESP, 4);
	EMIT_OP_REG(o_setnz, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 8);
	EMIT_OP_REG_RPTR(o_or, rEBX, sDWORD, rESP, 12);
	EMIT_OP_REG(o_setnz, rECX);
	EMIT_OP_REG_REG(o_xor, rEAX, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
	aluLabels++;
}


void GenCodeCmdAddD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ADD double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fadd, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdSubD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SUB double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fsub, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdMulD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MUL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fmul, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdDivD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DIV double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fdiv, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdPowD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("POW double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(long long)doublePow);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdModD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MOD double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP(o_fprem);
	EMIT_OP_FPUREG(o_fstp, rST1);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdLessD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LESS double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x41);
	EMIT_OP_LABEL(o_jne, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdGreaterD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GREATER double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x05);
	EMIT_OP_LABEL(o_jp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdLEqualD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LEQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x01);
	EMIT_OP_LABEL(o_jne, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdGEqualD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GEQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x41);
	EMIT_OP_LABEL(o_jp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdEqualD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("EQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdNEqualD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jnp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}


void GenCodeCmdNeg(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEG int");
	EMIT_OP_RPTR(o_neg, sDWORD, rESP, 0);
}

void GenCodeCmdBitNot(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BNOT int");
	EMIT_OP_RPTR(o_not, sDWORD, rESP, 0);
}

void GenCodeCmdLogNot(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LNOT int");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, 0, 0);
	EMIT_OP_REG(o_sete, rEAX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}


void GenCodeCmdNegL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEG long");
	EMIT_OP_RPTR(o_neg, sDWORD, rESP, 0);
	EMIT_OP_RPTR_NUM(o_adc, sDWORD, rESP, 4, 0);
	EMIT_OP_RPTR(o_neg, sDWORD, rESP, 4);
}

void GenCodeCmdBitNotL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BNOT long");
	EMIT_OP_RPTR(o_not, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_not, sDWORD, rESP, 4);
}

void GenCodeCmdLogNotL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LNOT long");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 4);
	EMIT_OP_REG_RPTR(o_or, rEBX, sDWORD, rESP, 0);
	EMIT_OP_REG(o_setz, rEAX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEAX);
	EMIT_OP_REG_NUM(o_add, rESP, 4);
}


void GenCodeCmdNegD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEG double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP(o_fchs);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdLogNotD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LNOT double");
	EMIT_OP(o_fldz);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 0);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}


void GenCodeCmdIncI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("INC int");
	EMIT_OP_RPTR_NUM(o_add, sDWORD, rESP, 0, 1);
}

void GenCodeCmdIncD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("INC double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP(o_fld1);
	EMIT_OP(o_faddp);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdIncL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("INC long");
	EMIT_OP_RPTR_NUM(o_add, sDWORD, rESP, 0, 1);
	EMIT_OP_RPTR_NUM(o_adc, sDWORD, rESP, 4, 0);
}


void GenCodeCmdDecI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DEC int");
	EMIT_OP_RPTR_NUM(o_sub, sDWORD, rESP, 0, 1);
}

void GenCodeCmdDecD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DEC double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP(o_fld1);
	EMIT_OP(o_fsubp);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdDecL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DEC long");
	EMIT_OP_RPTR_NUM(o_sub, sDWORD, rESP, 0, 1);
	EMIT_OP_RPTR_NUM(o_sbb, sDWORD, rESP, 4, 0);
}

void (*closureCreateFunc)() = NULL;
void SetClosureCreateFunc(void (*f)())
{
	closureCreateFunc = f;
}

void GenCodeCmdCreateClosure(VMCmd cmd)
{
	EMIT_COMMENT("CREATECLOSURE");

	EMIT_OP_NUM(o_push, cmd.argument);
	EMIT_OP_NUM(o_push, cmd.helper);
	EMIT_OP_REG_RPTR(o_lea, rEBX, sDWORD, rEBP, paramBase);
	EMIT_OP_REG(o_push, rEBX);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)closureCreateFunc);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 16);
}

void (*upvaluesCloseFunc)() = NULL;
void SetUpvaluesCloseFunc(void (*f)())
{
	upvaluesCloseFunc = f;
}

void GenCodeCmdCloseUpvalues(VMCmd cmd)
{
	EMIT_COMMENT("CLOSEUPVALUES");

	EMIT_OP_NUM(o_push, cmd.argument);
	EMIT_OP_REG_RPTR(o_lea, rEBX, sDWORD, rEBP, paramBase);
	EMIT_OP_REG(o_push, rEBX);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)upvaluesCloseFunc);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdConvertPtr(VMCmd cmd)
{
	EMIT_COMMENT("CONVERTPTR");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_NUM(o_cmp, rEAX, cmd.argument);
	EMIT_OP_LABEL(o_je, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, cmd.argument);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;
}

#endif
