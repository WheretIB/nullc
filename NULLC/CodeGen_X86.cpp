#include "CodeGen_X86.h"

#include "StdLib_X86.h"

FastVector<x86Instruction, true, true>	*myInstList;
x86Instruction	*x86Op = NULL;

void Emit(int comment, const char* text)
{
#ifdef NULLC_LOG_FILES
	myInstList->push_back(x86Instruction(comment, text));
#else
	(void)comment;
	(void)text;
#endif
}

void EMIT_LABEL(const char* Label)
{
	x86Op = myInstList->push_back();
	x86Op->name = o_label;
	assert(strlen(Label) < 16); strncpy(x86Op->labelName, Label, 16);
}
void EMIT_OP(x86Command op)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
}
void EMIT_OP_LABEL(x86Command op, const char* Label)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argLabel;
	assert(strlen(Label) < 16);
	strncpy(x86Op->argA.labelName, Label, 16);
}
void EMIT_OP_REG(x86Command op, x86Reg reg1)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
}
void EMIT_OP_FPUREG(x86Command op, x87Reg reg1)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argFPReg;
	x86Op->argA.fpArg = reg1;
}
void EMIT_OP_NUM(x86Command op, unsigned int num)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argNumber;
	x86Op->argA.num = num;
}

void EMIT_OP_ADDR(x86Command op, x86Size size, unsigned int addr)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrNum = addr;
}
void EMIT_OP_RPTR(x86Command op, x86Size size, x86Reg reg2, unsigned int shift)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrReg[0] = reg2;
	x86Op->argA.ptrNum = shift;
}
void EMIT_OP_REG_NUM(x86Command op, x86Reg reg1, unsigned int num)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argNumber;
	x86Op->argB.num = num;
}
void EMIT_OP_REG_REG(x86Command op, x86Reg reg1, x86Reg reg2)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
}
void EMIT_OP_REG_ADDR(x86Command op, x86Reg reg1, x86Size size, unsigned int addr)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argPtr;
	x86Op->argB.ptrSize = size;
	x86Op->argB.ptrNum = addr;
}
void EMIT_OP_REG_RPTR(x86Command op, x86Reg reg1, x86Size size, x86Reg reg2, unsigned int shift)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argPtr;
	x86Op->argB.ptrSize = size;
	x86Op->argB.ptrReg[0] = reg2;
	x86Op->argB.ptrNum = shift;
}
void EMIT_OP_REG_LABEL(x86Command op, x86Reg reg1, const char* Label, unsigned int shift)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argPtrLabel;
	assert(strlen(Label) < 16);
	strncpy(x86Op->argB.labelName, Label, 16);
	x86Op->argB.ptrNum = shift;
}
void EMIT_OP_ADDR_REG(x86Command op, x86Size size, unsigned int addr, x86Reg reg2)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrNum = addr;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
}

void EMIT_OP_RPTR_REG(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, x86Reg reg2)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrReg[0] = reg1;
	x86Op->argA.ptrNum = shift;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
}

void EMIT_OP_RPTR_NUM(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, unsigned int num)
{
	x86Op = myInstList->push_back();
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrReg[0] = reg1;
	x86Op->argA.ptrNum = shift;
	x86Op->argB.type = x86Argument::argNumber;
	x86Op->argB.num = num;
}


#include <stdarg.h>

char* InlFmt(const char *str, ...)
{
	static char storage[64];
	va_list args;
	va_start(args, str);
	vsprintf(storage, str, args); 
	return storage;
}

static unsigned int paramBase = 0;
static unsigned int aluLabels = 1;

void SetParamBase(unsigned int base)
{
	paramBase = base;
}

void SetInstructionList(FastVector<x86Instruction, true, true> *instList)
{
	myInstList = instList;
}

void GenCodeCmdNop(VMCmd cmd)
{
	(void)cmd;
	EMIT_OP(o_nop);
}

void GenCodeCmdPushCharAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH char abs");

	EMIT_OP_REG_ADDR(o_movsx, rEAX, sBYTE, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushShortAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH short abs");

	EMIT_OP_REG_ADDR(o_movsx, rEAX, sWORD, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushIntAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH int abs");

	EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase);
}

void GenCodeCmdPushFloatAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH float abs");

	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_ADDR(o_fld, sDWORD, cmd.argument+paramBase);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdPushDorLAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "PUSH double abs" : "MOV long abs");

	EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase+4);
	EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase);
}

void GenCodeCmdPushCmplxAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH complex abs");

	unsigned int currShift = cmd.helper;
	while(currShift >= 4)
	{
		currShift -= 4;
		EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase+currShift);
	}
	assert(currShift == 0);
}


void GenCodeCmdPushCharRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH char rel");

	EMIT_OP_REG_RPTR(o_movsx, rEAX, sBYTE, rEBP, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushShortRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH short rel");

	EMIT_OP_REG_RPTR(o_movsx, rEAX, sWORD, rEBP, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushIntRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH int rel");

	EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase);
}

void GenCodeCmdPushFloatRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH float rel");

	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fld, sDWORD, rEBP, cmd.argument+paramBase);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdPushDorLRel(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "PUSH double rel" : "PUSH long rel");

	EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase+4);
	EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase);
}

void GenCodeCmdPushCmplxRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH complex rel");
	unsigned int currShift = cmd.helper;
	while(currShift >= 4)
	{
		currShift -= 4;
		EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase+currShift);
	}
	assert(currShift == 0);
}


void GenCodeCmdPushCharStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH char stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_movsx, rEAX, sBYTE, rEDX, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushShortStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH short stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_movsx, rEAX, sWORD, rEDX, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushIntStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH int stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument+paramBase);
}

void GenCodeCmdPushFloatStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH float stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fld, sDWORD, rEDX, cmd.argument+paramBase);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdPushDorLStk(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "PUSH double stack" : "PUSH long stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument+paramBase+4);
	EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument+paramBase);
}

void GenCodeCmdPushCmplxStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH complex stack");
	unsigned int currShift = cmd.helper;
	EMIT_OP_REG(o_pop, rEDX);
	while(currShift >= 4)
	{
		currShift -= 4;
		EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument+paramBase+currShift);
	}
	assert(currShift == 0);
}


void GenCodeCmdPushImmt(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSHIMMT");
	
	EMIT_OP_NUM(o_push, cmd.argument);
}


void GenCodeCmdMovCharAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV char abs");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_ADDR_REG(o_mov, sBYTE, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovShortAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV short abs");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_ADDR_REG(o_mov, sWORD, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovIntAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV int abs");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_ADDR_REG(o_mov, sDWORD, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovFloatAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV float abs");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_ADDR(o_fstp, sDWORD, cmd.argument+paramBase);
}

void GenCodeCmdMovDorLAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "MOV double abs" : "MOV long abs");

	if(cmd.flag)
	{
		EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
		EMIT_OP_ADDR(o_fstp, sQWORD, cmd.argument+paramBase);
	}else{
		EMIT_OP_ADDR(o_pop, sDWORD, cmd.argument+paramBase);
		EMIT_OP_ADDR(o_pop, sDWORD, cmd.argument+paramBase + 4);
		EMIT_OP_REG_NUM(o_sub, rESP, 8);
	}
}

void GenCodeCmdMovCmplxAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV complex abs");
	unsigned int currShift = 0;
	while(currShift < cmd.helper)
	{
		EMIT_OP_ADDR(o_pop, sDWORD, cmd.argument+paramBase + currShift);
		currShift += 4;
	}
	EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
	assert(currShift == cmd.helper);
}


void GenCodeCmdMovCharRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV char rel");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sBYTE, rEBP, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovShortRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV short rel");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sWORD, rEBP, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovIntRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV int rel");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBP, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovFloatRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV float rel");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sDWORD, rEBP, cmd.argument+paramBase);
}

void GenCodeCmdMovDorLRel(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "MOV double rel" : "MOV long rel");

	if(cmd.flag)
	{
		EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
		EMIT_OP_RPTR(o_fstp, sQWORD, rEBP, cmd.argument+paramBase);
	}else{
		EMIT_OP_RPTR(o_pop, sDWORD, rEBP, cmd.argument+paramBase);
		EMIT_OP_RPTR(o_pop, sDWORD, rEBP, cmd.argument+paramBase + 4);
		EMIT_OP_REG_NUM(o_sub, rESP, 8);
	}
}

void GenCodeCmdMovCmplxRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV complex rel");
	unsigned int currShift = 0;
	while(currShift < cmd.helper)
	{
		EMIT_OP_RPTR(o_pop, sDWORD, rEBP, cmd.argument+paramBase + currShift);
		currShift += 4;
	}
	EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
	assert(currShift == cmd.helper);
}


void GenCodeCmdMovCharStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV char stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sBYTE, rEDX, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovShortStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV short stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sWORD, rEDX, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovIntStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV int stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rEDX, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdMovFloatStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV float stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sDWORD, rEDX, cmd.argument+paramBase);
}

void GenCodeCmdMovDorLStk(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "MOV double stack" : "MOV long stack");

	EMIT_OP_REG(o_pop, rEDX);
	if(cmd.flag)
	{
		EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
		EMIT_OP_RPTR(o_fstp, sQWORD, rEDX, cmd.argument+paramBase);
	}else{
		EMIT_OP_RPTR(o_pop, sDWORD, rEDX, cmd.argument+paramBase);
		EMIT_OP_RPTR(o_pop, sDWORD, rEDX, cmd.argument+paramBase + 4);
		EMIT_OP_REG_NUM(o_sub, rESP, 8);
	}
}

void GenCodeCmdMovCmplxStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV complex stack");
	EMIT_OP_REG(o_pop, rEDX);
	unsigned int currShift = 0;
	while(currShift < cmd.helper)
	{
		EMIT_OP_RPTR(o_pop, sDWORD, rEDX, cmd.argument+paramBase + currShift);
		currShift += 4;
	}
	EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
	assert(currShift == cmd.helper);
}


void GenCodeCmdReserveV(VMCmd cmd)
{
	(void)cmd;
}

void GenCodeCmdPopCharTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP char top");

	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_RPTR_REG(o_mov, sBYTE, rEDI, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdPopShortTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP short top");

	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_RPTR_REG(o_mov, sWORD, rEDI, cmd.argument+paramBase, rEBX);
}

void GenCodeCmdPopIntTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP int top");

	EMIT_OP_RPTR(o_pop, sDWORD, rEDI, cmd.argument+paramBase);
}

void GenCodeCmdPopFloatTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP float top");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sDWORD, rEDI, cmd.argument+paramBase);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdPopDorLTop(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "POP double top" : "POP long top");

	EMIT_OP_RPTR(o_pop, sDWORD, rEDI, cmd.argument+paramBase);
	EMIT_OP_RPTR(o_pop, sDWORD, rEDI, cmd.argument+paramBase + 4);
}

void GenCodeCmdPopCmplxTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP complex top");
	unsigned int currShift = 0;
	while(currShift < cmd.helper)
	{
		EMIT_OP_RPTR(o_pop, sDWORD, rEDI, cmd.argument+paramBase + currShift);
		currShift += 4;
	}
	assert(currShift == cmd.helper);
}



void GenCodeCmdPop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP");

	EMIT_OP_REG_NUM(o_add, rESP, cmd.argument);
}


void GenCodeCmdDtoI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DTOI");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fistp, sDWORD, rESP, 4);
	EMIT_OP_REG_NUM(o_add, rESP, 4);
}

void GenCodeCmdDtoL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DTOL");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fistp, sQWORD, rESP, 0);
}

void GenCodeCmdDtoF(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DTOF");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sDWORD, rESP, 4);
	EMIT_OP_REG_NUM(o_add, rESP, 4);
}

void GenCodeCmdItoD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "ITOD");

	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_REG(o_push, rEAX);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdLtoD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LTOD");

	EMIT_OP_RPTR(o_fild, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdItoL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "ITOL");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP(o_cdq);
	EMIT_OP_REG(o_push, rEDX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdLtoI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LTOI");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_RPTR(o_xchg, rEAX, sDWORD, rESP, 0);
}


void GenCodeCmdImmtMul(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.cmd == cmdImmtMulD ? "IMUL double" : (cmd.cmd == cmdImmtMulL ? "IMUL long" : "IMUL int"));
	
	if(cmd.cmd == cmdImmtMulD)
	{
		EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
		EMIT_OP_RPTR(o_fistp, sDWORD, rESP, 4);
		EMIT_OP_REG(o_pop, rEAX);
	}else if(cmd.cmd == cmdImmtMulL){
		EMIT_OP_REG(o_pop, rEAX);
	}
	
	if(cmd.argument == 2)
	{
		EMIT_OP_RPTR_NUM(o_shl, sDWORD, rESP, 0, 1);
	}else if(cmd.argument == 4){
		EMIT_OP_RPTR_NUM(o_shl, sDWORD, rESP, 0, 2);
	}else if(cmd.argument == 8){
		EMIT_OP_RPTR_NUM(o_shl, sDWORD, rESP, 0, 3);
	}else if(cmd.argument == 16){
		EMIT_OP_RPTR_NUM(o_shl, sDWORD, rESP, 0, 4);
	}else{
		EMIT_OP_REG(o_pop, rEAX);
		EMIT_OP_REG_NUM(o_imul, rEAX, cmd.argument);
		EMIT_OP_REG(o_push, rEAX);
	}
}

void GenCodeCmdCopyDorL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "COPY qword");

	EMIT_OP_REG_RPTR(o_mov, rEDX, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 4);
	EMIT_OP_REG(o_push, rEAX);
	EMIT_OP_REG(o_push, rEDX);
}

void GenCodeCmdCopyI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "COPY dword");

	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 0);
	EMIT_OP_REG(o_push, rEAX);
}


void GenCodeCmdGetAddr(VMCmd cmd)
{
	Emit(INST_COMMENT, "GETADDR");

	if(cmd.argument)
	{
		EMIT_OP_REG_RPTR(o_lea, rEAX, sDWORD, rEBP, cmd.argument);
		EMIT_OP_REG(o_push, rEAX);
	}else{
		EMIT_OP_REG(o_push, rEBP);
	}
}

void GenCodeCmdSetRange(VMCmd cmd)
{
	unsigned int elCount = myInstList->back().argA.num;
	myInstList->pop_back();

	Emit(INST_COMMENT, "SETRANGE");

	//assert(exCode[pos-2].cmd == cmdPushImmt);	// previous command must be cmdPushImmt

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
	EMIT_LABEL(InlFmt("loopStart%d", aluLabels));
	EMIT_OP_REG_REG(o_cmp, rEBX, rECX);
	EMIT_OP_LABEL(o_jg, InlFmt("loopEnd%d", aluLabels));

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
	EMIT_OP_LABEL(o_jmp, InlFmt("loopStart%d", aluLabels));
	EMIT_LABEL(InlFmt("loopEnd%d", aluLabels));
	if(cmd.helper == DTYPE_FLOAT)
		EMIT_OP_FPUREG(o_fstp, rST0);
	aluLabels++;
}


void GenCodeCmdJmp(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMP");
	EMIT_OP_LABEL(o_jmp, InlFmt("near gLabel%d", cmd.argument));
}


void GenCodeCmdJmpZI(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPZ int");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_test, rEAX, rEAX);
	EMIT_OP_LABEL(o_jz, InlFmt("near gLabel%d", cmd.argument));
}

void GenCodeCmdJmpZD(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPZ double");

	EMIT_OP(o_fldz);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 0);
	EMIT_OP_REG(o_fnstsw, rEAX);
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jnp, InlFmt("near gLabel%d", cmd.argument));
}

void GenCodeCmdJmpZL(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPZ long");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_or, rEDX, rEAX);
	EMIT_OP_LABEL(o_jne, InlFmt("near gLabel%d", cmd.argument));
}


void GenCodeCmdJmpNZI(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPNZ int");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_test, rEAX, rEAX);
	EMIT_OP_LABEL(o_jnz, InlFmt("near gLabel%d", cmd.argument));
}

void GenCodeCmdJmpNZD(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPNZ double");

	EMIT_OP(o_fldz);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 0);
	EMIT_OP_REG(o_fnstsw, rEAX);
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jp, InlFmt("near gLabel%d", cmd.argument));
}

void GenCodeCmdJmpNZL(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPNZ long");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_or, rEDX, rEAX);
	EMIT_OP_LABEL(o_je, InlFmt("near gLabel%d", cmd.argument));
}


void GenCodeCmdCall(VMCmd cmd)
{
	Emit(INST_COMMENT, InlFmt("CALL %d ret %s %d", cmd.argument, (cmd.helper & bitRetSimple ? "simple " : ""), (cmd.helper & 0x0FFF)));

	if(cmd.argument == -1)
	{
		EMIT_OP_REG(o_pop, rEAX);
		EMIT_OP_REG(o_call, rEAX);
	}else{
		EMIT_OP_LABEL(o_call, InlFmt("function%d", cmd.argument));
	}
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

void GenCodeCmdReturn(VMCmd cmd)
{
	Emit(INST_COMMENT, InlFmt("RET %d, %d %d", cmd.flag, cmd.argument, (cmd.helper & 0x0FFF)));

	if(cmd.flag & bitRetError)
	{
		EMIT_OP_REG_NUM(o_mov, rECX, 0xffffffff);
		EMIT_OP_NUM(o_int, 3);
		return;
	}
	if(cmd.helper == 0)
	{
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
		EMIT_OP(o_ret);
		return;
	}
	if(cmd.helper & bitRetSimple)
	{
		if((asmOperType)(cmd.helper & 0x0FFF) == OTYPE_INT)
		{
			EMIT_OP_REG(o_pop, rEAX);
		}else{
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}
		for(unsigned int pops = 0; pops < (cmd.argument > 0 ? cmd.argument : 1); pops++)
		{
			EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
			EMIT_OP_REG(o_pop, rEBP);
		}
		if(cmd.argument == 0)
			EMIT_OP_REG_NUM(o_mov, rEBX, cmd.helper & 0x0FFF);
	}else{
		if(cmd.helper == 4)
		{
			EMIT_OP_REG(o_pop, rEAX);
		}else if(cmd.helper == 8){
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}else if(cmd.helper == 12){
			EMIT_OP_REG(o_pop, rECX);
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}else if(cmd.helper == 16){
			EMIT_OP_REG(o_pop, rEBX);
			EMIT_OP_REG(o_pop, rECX);
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}else{
			EMIT_OP_REG_REG(o_mov, rEBX, rEDI);
			EMIT_OP_REG_REG(o_mov, rESI, rESP);

			EMIT_OP_REG_RPTR(o_lea, rEDI, sDWORD, rEDI, paramBase);
			EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
			EMIT_OP(o_rep_movsd);

			EMIT_OP_REG_REG(o_mov, rEDI, rEBX);

			EMIT_OP_REG_NUM(o_add, rESP, cmd.helper);
		}
		for(unsigned int pops = 0; pops < cmd.argument-1; pops++)
		{
			EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
			EMIT_OP_REG(o_pop, rEBP);
		}
		if(cmd.helper > 16)
			EMIT_OP_REG_REG(o_mov, rEAX, rEDI);
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
		if(cmd.argument == 0)
			EMIT_OP_REG_NUM(o_mov, rEBX, 16);
	}
	EMIT_OP(o_ret);
}


void GenCodeCmdPushVTop(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "PUSHT");

	EMIT_OP_REG(o_push, rEBP);
	EMIT_OP_REG_REG(o_mov, rEBP, rEDI);
}

void GenCodeCmdPopVTop(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "POPT");

	EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
	EMIT_OP_REG(o_pop, rEBP);
}


void GenCodeCmdPushV(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSHV");

	EMIT_OP_REG_NUM(o_add, rEDI, cmd.argument);
}


void GenCodeCmdAdd(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "ADD int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_add, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdSub(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SUB int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_sub, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdMul(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MUL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG(o_imul, rEDX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdDiv(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DIV int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_RPTR(o_xchg, rEAX, sDWORD, rESP, 0);
	EMIT_OP(o_cdq);
	EMIT_OP_RPTR(o_idiv, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_xchg, rEAX, sDWORD, rESP, 0);
}

void GenCodeCmdPow(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "POW int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(long long)intPow);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG(o_push, rEDX);
}

void GenCodeCmdMod(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MOD int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_RPTR(o_xchg, rEAX, sDWORD, rESP, 0);
	EMIT_OP(o_cdq);
	EMIT_OP_RPTR(o_idiv, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_xchg, rEDX, sDWORD, rESP, 0);
}

void GenCodeCmdLess(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LESS int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setl, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdGreater(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "GREATER int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setg, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdLEqual(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LEQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setle, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdGEqual(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "GEQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setge, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdEqual(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "EQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_sete, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdNEqual(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_REG(o_setne, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rECX);
}

void GenCodeCmdShl(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SHL int");
	EMIT_OP_REG(o_pop, rECX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_sal, rEAX, rECX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdShr(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SHR int");
	EMIT_OP_REG(o_pop, rECX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_sar, rEAX, rECX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdBitAnd(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BAND int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_and, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdBitOr(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BOR int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_or, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdBitXor(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BXOR int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_RPTR_REG(o_xor, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdLogAnd(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LAND int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_NUM(o_cmp, rEAX, 0);
	EMIT_OP_LABEL(o_je, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_je, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("pushedOne%d", aluLabels));
	EMIT_LABEL(InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("pushedOne%d", aluLabels));
	aluLabels++;
}

void GenCodeCmdLogOr(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LOR int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG_REG(o_or, rEAX, rEBX);
	EMIT_OP_REG_NUM(o_cmp, rEAX, 0);
	EMIT_OP_LABEL(o_je, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_NUM(o_push, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("pushedOne%d", aluLabels));
	EMIT_LABEL(InlFmt("pushZero%d", aluLabels));
	EMIT_OP_NUM(o_push, 0);
	EMIT_LABEL(InlFmt("pushedOne%d", aluLabels));
	aluLabels++;
}

void GenCodeCmdLogXor(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LXOR int");
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
	Emit(INST_COMMENT, "ADD long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_add, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_adc, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdSubL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SUB long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_sub, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_sbb, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdMulL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MUL long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(long long)longMul);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdDivL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DIV long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(long long)longDiv);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdPowL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "POW long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(long long)longPow);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdModL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MOD long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(long long)longMod);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdLessL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LESS long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jg, InlFmt("SetZero%d", aluLabels));
	EMIT_OP_LABEL(o_jl, InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jae, InlFmt("SetZero%d", aluLabels));
	EMIT_LABEL(InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("OneSet%d", aluLabels));
	EMIT_LABEL(InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("OneSet%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	aluLabels++;
}

void GenCodeCmdGreaterL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "GREATER long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jl, InlFmt("SetZero%d", aluLabels));
	EMIT_OP_LABEL(o_jg, InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jbe, InlFmt("SetZero%d", aluLabels));
	EMIT_LABEL(InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("OneSet%d", aluLabels));
	EMIT_LABEL(InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("OneSet%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	aluLabels++;
}

void GenCodeCmdLEqualL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LEQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jg, InlFmt("SetZero%d", aluLabels));
	EMIT_OP_LABEL(o_jl, InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_ja, InlFmt("SetZero%d", aluLabels));
	EMIT_LABEL(InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("OneSet%d", aluLabels));
	EMIT_LABEL(InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("OneSet%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	aluLabels++;
}

void GenCodeCmdGEqualL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "GEQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jl, InlFmt("SetZero%d", aluLabels));
	EMIT_OP_LABEL(o_jg, InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jb, InlFmt("SetZero%d", aluLabels));
	EMIT_LABEL(InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("OneSet%d", aluLabels));
	EMIT_LABEL(InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("OneSet%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	aluLabels++;
}

void GenCodeCmdEqualL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "EQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jne, InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jne, InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("OneSet%d", aluLabels));
	EMIT_LABEL(InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("OneSet%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	aluLabels++;
}

void GenCodeCmdNEqualL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jne, InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_je, InlFmt("SetZero%d", aluLabels));
	EMIT_LABEL(InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("OneSet%d", aluLabels));
	EMIT_LABEL(InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("OneSet%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	aluLabels++;
}

void GenCodeCmdShlL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SHL long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(long long)longShl);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdShrL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SHR long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(long long)longShr);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdBitAndL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BAND long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_and, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_and, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdBitOrL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BOR long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_or, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_or, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdBitXorL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BXOR long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_xor, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_xor, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdLogAndL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LAND long");
	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_or, rEAX, sDWORD, rESP, 4);
	EMIT_OP_LABEL(o_jz, InlFmt("SetZero%d", aluLabels));
	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 8);
	EMIT_OP_REG_RPTR(o_or, rEAX, sDWORD, rESP, 12);
	EMIT_OP_LABEL(o_jz, InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 8, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("OneSet%d", aluLabels));
	EMIT_LABEL(InlFmt("SetZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 8, 0);
	EMIT_LABEL(InlFmt("OneSet%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 12, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	aluLabels++;
}

void GenCodeCmdLogOrL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LOR long");
	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_or, rEAX, sDWORD, rESP, 4);
	EMIT_OP_LABEL(o_jnz, InlFmt("SetOne%d", aluLabels));
	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 8);
	EMIT_OP_REG_RPTR(o_or, rEAX, sDWORD, rESP, 12);
	EMIT_OP_LABEL(o_jnz, InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 8, 0);
	EMIT_OP_LABEL(o_jmp, InlFmt("ZeroSet%d", aluLabels));
	EMIT_LABEL(InlFmt("SetOne%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 8, 1);
	EMIT_LABEL(InlFmt("ZeroSet%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 12, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	aluLabels++;
}

void GenCodeCmdLogXorL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LXOR long");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_or, rEBX, sDWORD, rESP, 4);
	EMIT_OP_REG(o_setnz, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 8);
	EMIT_OP_REG_RPTR(o_or, rEBX, sDWORD, rESP, 12);
	EMIT_OP_REG(o_setnz, rECX);
	EMIT_OP_REG_REG(o_xor, rEAX, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
	aluLabels++;
}


void GenCodeCmdAddD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "ADD double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fadd, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdSubD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SUB double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fsub, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdMulD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MUL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fmul, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdDivD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DIV double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fdiv, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdPowD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "POW double");
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
	Emit(INST_COMMENT, "MOD double");
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
	Emit(INST_COMMENT, "LESS double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x41);
	EMIT_OP_LABEL(o_jne, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("pushedOne%d", aluLabels));
	EMIT_LABEL(InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("pushedOne%d", aluLabels));
	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	aluLabels++;
}

void GenCodeCmdGreaterD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "GREATER double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x05);
	EMIT_OP_LABEL(o_jp, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("pushedOne%d", aluLabels));
	EMIT_LABEL(InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("pushedOne%d", aluLabels));
	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	aluLabels++;
}

void GenCodeCmdLEqualD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LEQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x01);
	EMIT_OP_LABEL(o_jne, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("pushedOne%d", aluLabels));
	EMIT_LABEL(InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("pushedOne%d", aluLabels));
	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	aluLabels++;
}

void GenCodeCmdGEqualD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "GEQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x41);
	EMIT_OP_LABEL(o_jp, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("pushedOne%d", aluLabels));
	EMIT_LABEL(InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("pushedOne%d", aluLabels));
	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	aluLabels++;
}

void GenCodeCmdEqualD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "EQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jp, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("pushedOne%d", aluLabels));
	EMIT_LABEL(InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("pushedOne%d", aluLabels));
	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	aluLabels++;
}

void GenCodeCmdNEqualD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jnp, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("pushedOne%d", aluLabels));
	EMIT_LABEL(InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("pushedOne%d", aluLabels));
	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	aluLabels++;
}


void GenCodeCmdNeg(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEG int");
	EMIT_OP_RPTR(o_neg, sDWORD, rESP, 0);
}

void GenCodeCmdBitNot(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BNOT int");
	EMIT_OP_RPTR(o_not, sDWORD, rESP, 0);
}

void GenCodeCmdLogNot(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LNOT int");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, 0, 0);
	EMIT_OP_REG(o_sete, rEAX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}


void GenCodeCmdNegL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEG long");
	EMIT_OP_RPTR(o_neg, sDWORD, rESP, 0);
	EMIT_OP_RPTR_NUM(o_adc, sDWORD, rESP, 4, 0);
	EMIT_OP_RPTR(o_neg, sDWORD, rESP, 4);
}

void GenCodeCmdBitNotL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BNOT long");
	EMIT_OP_RPTR(o_not, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_not, sDWORD, rESP, 4);
}

void GenCodeCmdLogNotL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LNOT long");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 4);
	EMIT_OP_REG_RPTR(o_or, rEBX, sDWORD, rESP, 0);
	EMIT_OP_REG(o_setz, rEAX);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}


void GenCodeCmdNegD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEG double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP(o_fchs);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdLogNotD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LNOT double");
	EMIT_OP(o_fldz);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 0);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jp, InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, InlFmt("pushedOne%d", aluLabels));
	EMIT_LABEL(InlFmt("pushZero%d", aluLabels));
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(InlFmt("pushedOne%d", aluLabels));
	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
	aluLabels++;
}


void GenCodeCmdIncI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "INC int");
	EMIT_OP_RPTR_NUM(o_add, sDWORD, rESP, 0, 1);
}

void GenCodeCmdIncD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "INC double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP(o_fld1);
	EMIT_OP(o_faddp);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdIncL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "INC long");
	EMIT_OP_RPTR_NUM(o_add, sDWORD, rESP, 0, 1);
	EMIT_OP_RPTR_NUM(o_adc, sDWORD, rESP, 0, 0);
}


void GenCodeCmdDecI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DEC int");
	EMIT_OP_RPTR_NUM(o_sub, sDWORD, rESP, 0, 1);
}

void GenCodeCmdDecD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DEC double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP(o_fld1);
	EMIT_OP(o_fsubp);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdDecL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DEC long");
	EMIT_OP_RPTR_NUM(o_sub, sDWORD, rESP, 0, 1);
	EMIT_OP_RPTR_NUM(o_sbb, sDWORD, rESP, 0, 0);
}


void GenCodeCmdAddAtCharStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "ADDAT char stack");
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_movsx, rEAX, sBYTE, rEDX, cmd.argument+paramBase);
	if(cmd.flag == bitPushBefore)
		EMIT_OP_REG(o_push, rEAX);
	EMIT_OP_REG_NUM(cmd.helper == 1 ? o_add : o_sub, rEAX, 1);
	EMIT_OP_RPTR_REG(o_mov, sBYTE, rEDX, cmd.argument+paramBase, rEAX);
	if(cmd.flag == bitPushAfter)
		EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdAddAtShortStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "ADDAT short stack");
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_movsx, rEAX, sWORD, rEDX, cmd.argument+paramBase);
	if(cmd.flag == bitPushBefore)
		EMIT_OP_REG(o_push, rEAX);
	EMIT_OP_REG_NUM(cmd.helper == 1 ? o_add : o_sub, rEAX, 1);
	EMIT_OP_RPTR_REG(o_mov, sWORD, rEDX, cmd.argument+paramBase, rEAX);
	if(cmd.flag == bitPushAfter)
		EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdAddAtIntStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "ADDAT int stack");
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rEDX, cmd.argument+paramBase);
	if(cmd.flag == bitPushBefore)
		EMIT_OP_REG(o_push, rEAX);
	EMIT_OP_REG_NUM(cmd.helper == 1 ? o_add : o_sub, rEAX, 1);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rEDX, cmd.argument+paramBase, rEAX);
	if(cmd.flag == bitPushAfter)
		EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdAddAtLongStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "ADDAT long stack");
	EMIT_OP_REG(o_pop, rECX);
	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rECX, cmd.argument+paramBase);
	EMIT_OP_REG_RPTR(o_mov, rEDX, sDWORD, rECX, cmd.argument+paramBase+4);
	if(cmd.flag == bitPushBefore)
	{
		EMIT_OP_REG(o_push, rEAX);
		EMIT_OP_REG(o_push, rEDX);
	}
	EMIT_OP_REG_NUM(cmd.helper == 1 ? o_add : o_sub, rEAX, 1);
	EMIT_OP_REG_NUM(cmd.helper == 1 ? o_adc : o_sbb, rEDX, 0);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rECX, cmd.argument+paramBase, rEAX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rECX, cmd.argument+paramBase+4, rEDX);
	if(cmd.flag == bitPushAfter)
	{
		EMIT_OP_REG(o_push, rEAX);
		EMIT_OP_REG(o_push, rEDX);
	}
}

void GenCodeCmdAddAtFloatStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "ADDAT float stack");
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR(o_fld, sDWORD, rEDX, cmd.argument+paramBase);
	if(cmd.flag == bitPushBefore)
		EMIT_OP_FPUREG(o_fld, rST0);
	EMIT_OP(o_fld1);
	EMIT_OP(cmd.helper == 1 ? o_faddp : o_fsubp);
	if(cmd.flag == bitPushAfter)
	{
		EMIT_OP_RPTR(o_fst, sDWORD, rEDX, cmd.argument+paramBase);
		EMIT_OP_REG_NUM(o_sub, rESP, 8);
		EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
	}else{
		EMIT_OP_RPTR(o_fstp, sDWORD, rEDX, cmd.argument+paramBase);
	}
	if(cmd.flag == bitPushBefore)
	{
		EMIT_OP_REG_NUM(o_sub, rESP, 8);
		EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
	}
}

void GenCodeCmdAddAtDoubleStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "ADDAT double stack");
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR(o_fld, sQWORD, rEDX, cmd.argument+paramBase);
	if(cmd.flag == bitPushBefore)
		EMIT_OP_FPUREG(o_fld, rST0);
	EMIT_OP(o_fld1);
	EMIT_OP(cmd.helper == 1 ? o_faddp : o_fsubp);
	if(cmd.flag == bitPushAfter)
	{
		EMIT_OP_RPTR(o_fst, sQWORD, rEDX, cmd.argument+paramBase);
		EMIT_OP_REG_NUM(o_sub, rESP, 8);
		EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
	}else{
		EMIT_OP_RPTR(o_fstp, sQWORD, rEDX, cmd.argument+paramBase);
	}
	if(cmd.flag == bitPushBefore)
	{
		EMIT_OP_REG_NUM(o_sub, rESP, 8);
		EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
	}
}
