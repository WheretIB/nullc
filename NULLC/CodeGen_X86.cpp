#include "CodeGen_X86.h"

#include "StdLib_X86.h"

FastVector<x86Instruction>	*myInstList;

void Emit(const char* Label)
{
	myInstList->push_back(x86Instruction(Label));
}
void Emit(int comment, const char* text)
{
#ifdef NULLC_LOG_FILES
	myInstList->push_back(x86Instruction(comment, text));
#else
	(void)comment;
	(void)text;
#endif
}
void Emit(x86Command Name)
{
	myInstList->push_back(x86Instruction(Name));
}
void Emit(x86Command Name, const x86Argument& a)
{
	myInstList->push_back(x86Instruction(Name, a));
}
void Emit(x86Command Name, const x86Argument& a, const x86Argument& b)
{
	myInstList->push_back(x86Instruction(Name, a, b));
}
void Emit(x86Command Name, const x86Argument& a, const x86Argument& b, const x86Argument& c)
{
	myInstList->push_back(x86Instruction(Name, a, b, c));
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

static unsigned int stackRelSize = 0;
static unsigned int paramBase = 0;
static unsigned int aluLabels = 1;

void ResetStackTracking()
{
	stackRelSize = 0;
}

unsigned int GetStackTrackInfo()
{
	return stackRelSize;
}

void SetParamBase(unsigned int base)
{
	paramBase = base;
}

void SetInstructionList(FastVector<x86Instruction> *instList)
{
	myInstList = instList;
}

void GenCodeCmdNop(VMCmd cmd)
{
	(void)cmd;
	Emit(o_nop);
}

void GenCodeCmdPushCharAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH char abs");

	Emit(o_movsx, x86Argument(rEAX), x86Argument(sBYTE, cmd.argument+paramBase));
	Emit(o_push, x86Argument(rEAX));
	stackRelSize += 4;
}

void GenCodeCmdPushShortAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH short abs");

	Emit(o_movsx, x86Argument(rEAX), x86Argument(sWORD, cmd.argument+paramBase));
	Emit(o_push, x86Argument(rEAX));
	stackRelSize += 4;
}

void GenCodeCmdPushIntAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH int abs");

	Emit(o_push, x86Argument(sDWORD, cmd.argument+paramBase));
	stackRelSize += 4;
}

void GenCodeCmdPushFloatAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH float abs");

	Emit(o_sub, x86Argument(rESP), x86Argument(8));
	Emit(o_fld, x86Argument(sDWORD, cmd.argument+paramBase));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
	stackRelSize += 8;
}

void GenCodeCmdPushDorLAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "PUSH double abs" : "MOV long abs");

	Emit(o_push, x86Argument(sDWORD, cmd.argument+paramBase+4));
	Emit(o_push, x86Argument(sDWORD, cmd.argument+paramBase));
	stackRelSize += 8;
}

void GenCodeCmdPushCmplxAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH complex abs");

	unsigned int currShift = cmd.helper;
	while(currShift >= 4)
	{
		currShift -= 4;
		Emit(o_push, x86Argument(sDWORD, cmd.argument+paramBase+currShift));
		stackRelSize += 4;
	}
	assert(currShift == 0);
}


void GenCodeCmdPushCharRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH char rel");

	Emit(o_movsx, x86Argument(rEAX), x86Argument(sBYTE, rEBP, cmd.argument+paramBase));
	Emit(o_push, x86Argument(rEAX));
	stackRelSize += 4;
}

void GenCodeCmdPushShortRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH short rel");

	Emit(o_movsx, x86Argument(rEAX), x86Argument(sWORD, rEBP, cmd.argument+paramBase));
	Emit(o_push, x86Argument(rEAX));
	stackRelSize += 4;
}

void GenCodeCmdPushIntRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH int rel");

	Emit(o_push, x86Argument(sDWORD, rEBP, cmd.argument+paramBase));
	stackRelSize += 4;
}

void GenCodeCmdPushFloatRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH float rel");

	Emit(o_sub, x86Argument(rESP), x86Argument(8));
	Emit(o_fld, x86Argument(sDWORD, rEBP, cmd.argument+paramBase));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
	stackRelSize += 8;
}

void GenCodeCmdPushDorLRel(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "PUSH double rel" : "PUSH long rel");

	Emit(o_push, x86Argument(sDWORD, rEBP, cmd.argument+paramBase+4));
	Emit(o_push, x86Argument(sDWORD, rEBP, cmd.argument+paramBase));
	stackRelSize += 8;
}

void GenCodeCmdPushCmplxRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH complex rel");
	unsigned int currShift = cmd.helper;
	while(currShift >= 4)
	{
		currShift -= 4;
		Emit(o_push, x86Argument(sDWORD, rEBP, cmd.argument+paramBase+currShift));
		stackRelSize += 4;
	}
	assert(currShift == 0);
}


void GenCodeCmdPushCharStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH char stack");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_movsx, x86Argument(rEAX), x86Argument(sBYTE, rEDX, cmd.argument+paramBase));
	Emit(o_push, x86Argument(rEAX));
}

void GenCodeCmdPushShortStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH short stack");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_movsx, x86Argument(rEAX), x86Argument(sWORD, rEDX, cmd.argument+paramBase));
	Emit(o_push, x86Argument(rEAX));
}

void GenCodeCmdPushIntStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH int stack");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_push, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
}

void GenCodeCmdPushFloatStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH float stack");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_sub, x86Argument(rESP), x86Argument(8));
	Emit(o_fld, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
	stackRelSize += 4;
}

void GenCodeCmdPushDorLStk(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "PUSH double stack" : "PUSH long stack");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_push, x86Argument(sDWORD, rEDX, cmd.argument+paramBase+4));
	Emit(o_push, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
	stackRelSize += 4;
}

void GenCodeCmdPushCmplxStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSH complex stack");
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


void GenCodeCmdPushImmt(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSHIMMT");
	
	Emit(o_push, x86Argument(cmd.argument));
	stackRelSize += 4;
}


void GenCodeCmdMovCharAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV char abs");

	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(sBYTE, cmd.argument+paramBase), x86Argument(rEBX));
}

void GenCodeCmdMovShortAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV short abs");

	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(sWORD, cmd.argument+paramBase), x86Argument(rEBX));
}

void GenCodeCmdMovIntAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV int abs");

	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(sDWORD, cmd.argument+paramBase), x86Argument(rEBX));
}

void GenCodeCmdMovFloatAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV float abs");

	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sDWORD, cmd.argument+paramBase));
}

void GenCodeCmdMovDorLAbs(VMCmd cmd)
{
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
}

void GenCodeCmdMovCmplxAbs(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV complex abs");
	unsigned int currShift = 0;
	while(currShift < cmd.helper)
	{
		Emit(o_pop, x86Argument(sDWORD, cmd.argument+paramBase + currShift));
		currShift += 4;
	}
	Emit(o_sub, x86Argument(rESP), x86Argument(cmd.helper));
	assert(currShift == cmd.helper);
}


void GenCodeCmdMovCharRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV char rel");

	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(sBYTE, rEBP, cmd.argument+paramBase), x86Argument(rEBX));
}

void GenCodeCmdMovShortRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV short rel");

	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(sWORD, rEBP, cmd.argument+paramBase), x86Argument(rEBX));
}

void GenCodeCmdMovIntRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV int rel");

	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(sDWORD, rEBP, cmd.argument+paramBase), x86Argument(rEBX));
}

void GenCodeCmdMovFloatRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV float rel");

	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sDWORD, rEBP, cmd.argument+paramBase));
}

void GenCodeCmdMovDorLRel(VMCmd cmd)
{
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
}

void GenCodeCmdMovCmplxRel(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV complex rel");
	unsigned int currShift = 0;
	while(currShift < cmd.helper)
	{
		Emit(o_pop, x86Argument(sDWORD, rEBP, cmd.argument+paramBase + currShift));
		currShift += 4;
	}
	Emit(o_sub, x86Argument(rESP), x86Argument(cmd.helper));
	assert(currShift == cmd.helper);
}


void GenCodeCmdMovCharStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV char stack");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(sBYTE, rEDX, cmd.argument+paramBase), x86Argument(rEBX));
	stackRelSize -= 4;
}

void GenCodeCmdMovShortStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV short stack");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(sWORD, rEDX, cmd.argument+paramBase), x86Argument(rEBX));
	stackRelSize -= 4;
}

void GenCodeCmdMovIntStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV int stack");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(sDWORD, rEDX, cmd.argument+paramBase), x86Argument(rEBX));
	stackRelSize -= 4;
}

void GenCodeCmdMovFloatStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV float stack");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sDWORD, rEDX, cmd.argument+paramBase));
	stackRelSize -= 4;
}

void GenCodeCmdMovDorLStk(VMCmd cmd)
{
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
}

void GenCodeCmdMovCmplxStk(VMCmd cmd)
{
	Emit(INST_COMMENT, "MOV complex stack");
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


void GenCodeCmdReserveV(VMCmd cmd)
{
	(void)cmd;
}

void GenCodeCmdPopCharTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP char top");

	Emit(o_pop, x86Argument(rEBX));
	Emit(o_mov, x86Argument(sBYTE, rEDI, cmd.argument+paramBase), x86Argument(rEBX));
	stackRelSize -= 4;
}

void GenCodeCmdPopShortTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP short top");

	Emit(o_pop, x86Argument(rEBX));
	Emit(o_mov, x86Argument(sWORD, rEDI, cmd.argument+paramBase), x86Argument(rEBX));
	stackRelSize -= 4;
}

void GenCodeCmdPopIntTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP int top");

	Emit(o_pop, x86Argument(sDWORD, rEDI, cmd.argument+paramBase));
	stackRelSize -= 4;
}

void GenCodeCmdPopFloatTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP float top");

	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sDWORD, rEDI, cmd.argument+paramBase));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	stackRelSize -= 8;
}

void GenCodeCmdPopDorLTop(VMCmd cmd)
{
	Emit(INST_COMMENT, cmd.flag ? "POP double top" : "POP long top");

	Emit(o_pop, x86Argument(sDWORD, rEDI, cmd.argument+paramBase));
	Emit(o_pop, x86Argument(sDWORD, rEDI, cmd.argument+paramBase + 4));
	stackRelSize -= 8;
}

void GenCodeCmdPopCmplxTop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP complex top");
	unsigned int currShift = 0;
	while(currShift < cmd.helper)
	{
		Emit(o_pop, x86Argument(sDWORD, rEDI, cmd.argument+paramBase + currShift));
		currShift += 4;
	}
	assert(currShift == cmd.helper);
	stackRelSize -= cmd.helper;
}



void GenCodeCmdPop(VMCmd cmd)
{
	Emit(INST_COMMENT, "POP");

	Emit(o_add, x86Argument(rESP), x86Argument(cmd.argument));
	stackRelSize -= cmd.argument;
}


void GenCodeCmdDtoI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DTOI");

	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fistp, x86Argument(sDWORD, rESP, 4));
	Emit(o_add, x86Argument(rESP), x86Argument(4));
	stackRelSize -= 4;
}

void GenCodeCmdDtoL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DTOL");

	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fistp, x86Argument(sQWORD, rESP, 0));
}

void GenCodeCmdDtoF(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DTOF");

	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sDWORD, rESP, 4));
	Emit(o_add, x86Argument(rESP), x86Argument(4));
	stackRelSize -= 4;
}

void GenCodeCmdItoD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "ITOD");

	Emit(o_fild, x86Argument(sDWORD, rESP, 0));
	Emit(o_push, x86Argument(rEAX));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
	stackRelSize += 4;
}

void GenCodeCmdLtoD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LTOD");

	Emit(o_fild, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
}

void GenCodeCmdItoL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "ITOL");

	Emit(o_pop, x86Argument(rEAX));
	Emit(o_cdq);
	Emit(o_push, x86Argument(rEDX));
	Emit(o_push, x86Argument(rEAX));
	stackRelSize += 4;
}

void GenCodeCmdLtoI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LTOI");

	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xchg, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
	stackRelSize -= 4;
}


void GenCodeCmdImmtMul(VMCmd cmd)
{
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
}

void GenCodeCmdCopyDorL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "COPY qword");

	Emit(o_mov, x86Argument(rEDX), x86Argument(sDWORD, rESP, 0));
	Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rESP, 4));
	Emit(o_push, x86Argument(rEAX));
	Emit(o_push, x86Argument(rEDX));
	stackRelSize += 8;
}

void GenCodeCmdCopyI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "COPY dword");

	Emit(o_mov, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
	Emit(o_push, x86Argument(rEAX));
	stackRelSize += 4;
}


void GenCodeCmdGetAddr(VMCmd cmd)
{
	Emit(INST_COMMENT, "GETADDR");

	if(cmd.argument)
	{
		Emit(o_lea, x86Argument(rEAX), x86Argument(sDWORD, rEBP, cmd.argument));
		Emit(o_push, x86Argument(rEAX));
	}else{
		Emit(o_push, x86Argument(rEBP));
	}
	stackRelSize += 4;
}

void GenCodeCmdSetRange(VMCmd cmd)
{
	unsigned int elCount = myInstList->back().argA.num;
	myInstList->pop_back();

	Emit(INST_COMMENT, "SETRANGE");

	//assert(exCode[pos-2].cmd == cmdPushImmt);	// previous command must be cmdPushImmt

	unsigned int typeSizeD[] = { 1, 2, 4, 8, 4, 8 };

	// start address
	Emit(o_lea, x86Argument(rEBX), x86Argument(sDWORD, rEBP, paramBase + cmd.argument));
	// end address
	Emit(o_lea, x86Argument(rECX), x86Argument(sDWORD, rEBP, paramBase + cmd.argument + (elCount - 1) * typeSizeD[(cmd.helper>>2)&0x00000007]));
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
}


void GenCodeCmdJmp(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMP");
	Emit(o_jmp, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
}


void GenCodeCmdJmpZI(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPZ int");

	Emit(o_pop, x86Argument(rEAX));
	Emit(o_test, x86Argument(rEAX), x86Argument(rEAX));
	Emit(o_jz, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
	stackRelSize -= 4;
}

void GenCodeCmdJmpZD(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPZ double");

	Emit(o_fldz);
	Emit(o_fcomp, x86Argument(sQWORD, rESP, 0));
	Emit(o_fnstsw, x86Argument(rEAX));
	Emit(o_pop, x86Argument(rEBX));
	Emit(o_pop, x86Argument(rEBX));
	Emit(o_test, x86Argument(rEAX), x86Argument(0x44));
	Emit(o_jnp, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
	stackRelSize -= 8;
}

void GenCodeCmdJmpZL(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPZ long");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_or, x86Argument(rEDX), x86Argument(rEAX));
	Emit(o_jne, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
	stackRelSize -= 8;
}


void GenCodeCmdJmpNZI(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPNZ int");

	Emit(o_pop, x86Argument(rEAX));
	Emit(o_test, x86Argument(rEAX), x86Argument(rEAX));
	Emit(o_jnz, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
	stackRelSize -= 4;
}

void GenCodeCmdJmpNZD(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPNZ double");

	Emit(o_fldz);
	Emit(o_fcomp, x86Argument(sQWORD, rESP, 0));
	Emit(o_fnstsw, x86Argument(rEAX));
	Emit(o_pop, x86Argument(rEBX));
	Emit(o_pop, x86Argument(rEBX));
	Emit(o_test, x86Argument(rEAX), x86Argument(0x44));
	Emit(o_jp, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
	stackRelSize -= 8;
}

void GenCodeCmdJmpNZL(VMCmd cmd)
{
	Emit(INST_COMMENT, "JMPNZ long");

	Emit(o_pop, x86Argument(rEDX));
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_or, x86Argument(rEDX), x86Argument(rEAX));
	Emit(o_je, x86Argument(InlFmt("near gLabel%d", cmd.argument)));
	stackRelSize -= 8;
}


void GenCodeCmdCall(VMCmd cmd)
{
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
}

void GenCodeCmdReturn(VMCmd cmd)
{
	Emit(INST_COMMENT, InlFmt("RET %d, %d %d", cmd.flag, cmd.argument, (cmd.helper & 0x0FFF)));

	if(cmd.flag & bitRetError)
	{
		Emit(o_mov, x86Argument(rECX), x86Argument(0xffffffff));
		Emit(o_int, x86Argument(3));
		return;
	}
	if(cmd.helper == 0)
	{
		Emit(o_mov, x86Argument(rEDI), x86Argument(rEBP));
		Emit(o_pop, x86Argument(rEBP));
		Emit(o_ret);
		stackRelSize = 0;
		return;
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
}


void GenCodeCmdPushVTop(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "PUSHT");

	Emit(o_push, x86Argument(rEBP));
	Emit(o_mov, x86Argument(rEBP), x86Argument(rEDI));
	stackRelSize += 4;
}

void GenCodeCmdPopVTop(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "POPT");

	Emit(o_mov, x86Argument(rEDI), x86Argument(rEBP));
	Emit(o_pop, x86Argument(rEBP));
	stackRelSize -= 4;
}


void GenCodeCmdPushV(VMCmd cmd)
{
	Emit(INST_COMMENT, "PUSHV");

	Emit(o_add, x86Argument(rEDI), x86Argument(cmd.argument));
}


void GenCodeCmdAdd(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "ADD int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_add, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	stackRelSize -= 4;
}

void GenCodeCmdSub(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SUB int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_sub, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	stackRelSize -= 4;
}

void GenCodeCmdMul(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MUL int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_pop, x86Argument(rEDX));
	Emit(o_imul, x86Argument(rEDX));
	Emit(o_push, x86Argument(rEAX));
	stackRelSize -= 4;
}

void GenCodeCmdDiv(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DIV int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xchg, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
	Emit(o_cdq);
	Emit(o_idiv, x86Argument(sDWORD, rESP, 0));
	Emit(o_xchg, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
	stackRelSize -= 4;
}

void GenCodeCmdPow(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "POW int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_pop, x86Argument(rEBX));
	Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)intPow));
	Emit(o_call, x86Argument(rECX));
	Emit(o_push, x86Argument(rEDX));
	stackRelSize -= 4;
}

void GenCodeCmdMod(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MOD int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xchg, x86Argument(rEAX), x86Argument(sDWORD, rESP, 0));
	Emit(o_cdq);
	Emit(o_idiv, x86Argument(sDWORD, rESP, 0));
	Emit(o_xchg, x86Argument(rEDX), x86Argument(sDWORD, rESP, 0));
	stackRelSize -= 4;
}

void GenCodeCmdLess(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LESS int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
	Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_setl, x86Argument(rECX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
	stackRelSize -= 4;
}

void GenCodeCmdGreater(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "GREATER int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
	Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_setg, x86Argument(rECX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
	stackRelSize -= 4;
}

void GenCodeCmdLEqual(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LEQUAL int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
	Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_setle, x86Argument(rECX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
	stackRelSize -= 4;
}

void GenCodeCmdGEqual(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "GEQUAL int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
	Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_setge, x86Argument(rECX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
	stackRelSize -= 4;
}

void GenCodeCmdEqual(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "EQUAL int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
	Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_sete, x86Argument(rECX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
	stackRelSize -= 4;
}

void GenCodeCmdNEqual(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEQUAL int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xor, x86Argument(rECX), x86Argument(rECX));
	Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_setne, x86Argument(rECX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rECX));
	stackRelSize -= 4;
}

void GenCodeCmdShl(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SHL int");
	Emit(o_pop, x86Argument(rECX));
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_sal, x86Argument(rEAX), x86Argument(rECX));
	Emit(o_push, x86Argument(rEAX));
	stackRelSize -= 4;
}

void GenCodeCmdShr(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SHR int");
	Emit(o_pop, x86Argument(rECX));
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_sar, x86Argument(rEAX), x86Argument(rECX));
	Emit(o_push, x86Argument(rEAX));
	stackRelSize -= 4;
}

void GenCodeCmdBitAnd(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BAND int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_and, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	stackRelSize -= 4;
}

void GenCodeCmdBitOr(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BOR int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_or, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	stackRelSize -= 4;
}

void GenCodeCmdBitXor(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BXOR int");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_xor, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	stackRelSize -= 4;
}

void GenCodeCmdLogAnd(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdLogOr(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdLogXor(VMCmd cmd)
{
	(void)cmd;
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
}


void GenCodeCmdAddL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "ADD long");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_pop, x86Argument(rEDX));
	Emit(o_add, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_adc, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
	stackRelSize -= 8;
}

void GenCodeCmdSubL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SUB long");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_pop, x86Argument(rEDX));
	Emit(o_sub, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_sbb, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
	stackRelSize -= 8;
}

void GenCodeCmdMulL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MUL long");
	Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longMul));
	Emit(o_call, x86Argument(rECX));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	stackRelSize -= 8;
}

void GenCodeCmdDivL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DIV long");
	Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longDiv));
	Emit(o_call, x86Argument(rECX));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	stackRelSize -= 8;
}

void GenCodeCmdPowL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "POW long");
	Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longPow));
	Emit(o_call, x86Argument(rECX));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	stackRelSize -= 8;
}

void GenCodeCmdModL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MOD long");
	Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longMod));
	Emit(o_call, x86Argument(rECX));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	stackRelSize -= 8;
}

void GenCodeCmdLessL(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdGreaterL(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdLEqualL(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdGEqualL(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdEqualL(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdNEqualL(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdShlL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SHL long");
	Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longShl));
	Emit(o_call, x86Argument(rECX));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	stackRelSize -= 8;
}

void GenCodeCmdShrL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SHR long");
	Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)longShr));
	Emit(o_call, x86Argument(rECX));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	stackRelSize -= 8;
}

void GenCodeCmdBitAndL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BAND long");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_pop, x86Argument(rEDX));
	Emit(o_and, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_and, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
	stackRelSize -= 8;
}

void GenCodeCmdBitOrL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BOR long");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_pop, x86Argument(rEDX));
	Emit(o_or, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_or, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
	stackRelSize -= 8;
}

void GenCodeCmdBitXorL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BXOR long");
	Emit(o_pop, x86Argument(rEAX));
	Emit(o_pop, x86Argument(rEDX));
	Emit(o_xor, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
	Emit(o_xor, x86Argument(sDWORD, rESP, 4), x86Argument(rEDX));
	stackRelSize -= 8;
}

void GenCodeCmdLogAndL(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdLogOrL(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdLogXorL(VMCmd cmd)
{
	(void)cmd;
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
}


void GenCodeCmdAddD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "ADD double");
	Emit(o_fld, x86Argument(sQWORD, rESP, 8));
	Emit(o_fadd, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	stackRelSize -= 8;
}

void GenCodeCmdSubD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "SUB double");
	Emit(o_fld, x86Argument(sQWORD, rESP, 8));
	Emit(o_fsub, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	stackRelSize -= 8;
}

void GenCodeCmdMulD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MUL double");
	Emit(o_fld, x86Argument(sQWORD, rESP, 8));
	Emit(o_fmul, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	stackRelSize -= 8;
}

void GenCodeCmdDivD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DIV double");
	Emit(o_fld, x86Argument(sQWORD, rESP, 8));
	Emit(o_fdiv, x86Argument(sQWORD, rESP, 0));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	stackRelSize -= 8;
}

void GenCodeCmdPowD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "POW double");
	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fld, x86Argument(sQWORD, rESP, 8));
	Emit(o_mov, x86Argument(rECX), x86Argument((int)(long long)doublePow));
	Emit(o_call, x86Argument(rECX));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	stackRelSize -= 8;
}

void GenCodeCmdModD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "MOD double");
	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fld, x86Argument(sQWORD, rESP, 8));
	Emit(o_fprem);
	Emit(o_fstp, x86Argument(rST1));
	Emit(o_fstp, x86Argument(sQWORD, rESP, 8));
	Emit(o_add, x86Argument(rESP), x86Argument(8));
	stackRelSize -= 8;
}

void GenCodeCmdLessD(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdGreaterD(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdLEqualD(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdGEqualD(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdEqualD(VMCmd cmd)
{
	(void)cmd;
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
}

void GenCodeCmdNEqualD(VMCmd cmd)
{
	(void)cmd;
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
}


void GenCodeCmdNeg(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEG int");
	Emit(o_neg, x86Argument(sDWORD, rESP, 0));
}

void GenCodeCmdBitNot(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BNOT int");
	Emit(o_not, x86Argument(sDWORD, rESP, 0));
}

void GenCodeCmdLogNot(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LNOT int");
	Emit(o_xor, x86Argument(rEAX), x86Argument(rEAX));
	Emit(o_cmp, x86Argument(sDWORD, rESP, 0), x86Argument(0));
	Emit(o_sete, x86Argument(rEAX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
}


void GenCodeCmdNegL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEG long");
	Emit(o_neg, x86Argument(sDWORD, rESP, 0));
	Emit(o_adc, x86Argument(sDWORD, rESP, 4), x86Argument(0));
	Emit(o_neg, x86Argument(sDWORD, rESP, 4));
}

void GenCodeCmdBitNotL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "BNOT long");
	Emit(o_not, x86Argument(sDWORD, rESP, 0));
	Emit(o_not, x86Argument(sDWORD, rESP, 4));
}

void GenCodeCmdLogNotL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "LNOT long");
	Emit(o_xor, x86Argument(rEAX), x86Argument(rEAX));
	Emit(o_mov, x86Argument(rEBX), x86Argument(sDWORD, rESP, 4));
	Emit(o_or, x86Argument(rEBX), x86Argument(sDWORD, rESP, 0));
	Emit(o_setz, x86Argument(rEAX));
	Emit(o_mov, x86Argument(sDWORD, rESP, 4), x86Argument(0));
	Emit(o_mov, x86Argument(sDWORD, rESP, 0), x86Argument(rEAX));
}


void GenCodeCmdNegD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "NEG double");
	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fchs);
	Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
}

void GenCodeCmdLogNotD(VMCmd cmd)
{
	(void)cmd;
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
}


void GenCodeCmdIncI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "INC int");
	Emit(o_add, x86Argument(sDWORD, rESP, 0), x86Argument(1));
}

void GenCodeCmdIncD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "INC double");
	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fld1);
	Emit(o_faddp);
	Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
}

void GenCodeCmdIncL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "INC long");
	Emit(o_add, x86Argument(sDWORD, rESP, 0), x86Argument(1));
	Emit(o_adc, x86Argument(sDWORD, rESP, 0), x86Argument(0));
}


void GenCodeCmdDecI(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DEC int");
	Emit(o_sub, x86Argument(sDWORD, rESP, 0), x86Argument(1));
}

void GenCodeCmdDecD(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DEC double");
	Emit(o_fld, x86Argument(sQWORD, rESP, 0));
	Emit(o_fld1);
	Emit(o_fsubp);
	Emit(o_fstp, x86Argument(sQWORD, rESP, 0));
}

void GenCodeCmdDecL(VMCmd cmd)
{
	(void)cmd;
	Emit(INST_COMMENT, "DEC long");
	Emit(o_sub, x86Argument(sDWORD, rESP, 0), x86Argument(1));
	Emit(o_sbb, x86Argument(sDWORD, rESP, 0), x86Argument(0));
}


void GenCodeCmdAddAtCharStk(VMCmd cmd)
{
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
}

void GenCodeCmdAddAtShortStk(VMCmd cmd)
{
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
}

void GenCodeCmdAddAtIntStk(VMCmd cmd)
{
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
}

void GenCodeCmdAddAtLongStk(VMCmd cmd)
{
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
}

void GenCodeCmdAddAtFloatStk(VMCmd cmd)
{
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
}

void GenCodeCmdAddAtDoubleStk(VMCmd cmd)
{
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
}
