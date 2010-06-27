#pragma once

#include "stdafx.h"

#include "Instruction_X86.h"
#include "ParseClass.h"
#include "Bytecode.h"

const unsigned int JUMP_NEAR = (unsigned int)(1 << 31);
// jump ID markers for assembly printout
const unsigned int LABEL_GLOBAL = 1 << 30;
const unsigned int LABEL_ALU = 0;

#ifdef NULLC_LOG_FILES
void EMIT_COMMENT(const char* text);
#else
#define EMIT_COMMENT(x)
#endif

void EMIT_LABEL(unsigned int labelID, int invalidate);
void EMIT_OP(x86Command op);
void EMIT_OP_LABEL(x86Command op, unsigned int labelID, int invalidate, int longJump);
void EMIT_OP_REG(x86Command op, x86Reg reg1);
void EMIT_OP_FPUREG(x86Command op, x87Reg reg1);
void EMIT_OP_NUM(x86Command op, unsigned int num);

void EMIT_OP_RPTR(x86Command op, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift);
void EMIT_OP_RPTR(x86Command op, x86Size size, x86Reg reg2, unsigned int shift);
void EMIT_OP_ADDR(x86Command op, x86Size size, unsigned int addr);

void EMIT_OP_REG_NUM(x86Command op, x86Reg reg1, unsigned int num);
void EMIT_OP_REG_REG(x86Command op, x86Reg reg1, x86Reg reg2);

void EMIT_OP_REG_RPTR(x86Command op, x86Reg reg1, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift);
void EMIT_OP_REG_RPTR(x86Command op, x86Reg reg1, x86Size size, x86Reg reg2, unsigned int shift);
void EMIT_OP_REG_ADDR(x86Command op, x86Reg reg1, x86Size size, unsigned int addr);

void EMIT_OP_REG_LABEL(x86Command op, x86Reg reg1, unsigned int labelID, unsigned int shift);

void EMIT_OP_RPTR_REG(x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned int shift, x86Reg reg2);
void EMIT_OP_RPTR_REG(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, x86Reg reg2);
void EMIT_OP_ADDR_REG(x86Command op, x86Size size, unsigned int addr, x86Reg reg2);

void EMIT_OP_RPTR_NUM(x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned int shift, unsigned int num);
void EMIT_OP_RPTR_NUM(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, unsigned int num);

void SetParamBase(unsigned int base);
void SetFunctionList(ExternFuncInfo* list, unsigned int* funcAddresses);
void SetContinuePtr(int* continueVar);

#ifdef __linux
const int EXCEPTION_INT_DIVIDE_BY_ZERO = 1;
const int EXCEPTION_STOP_EXECUTION = 2;
const int EXCEPTION_FUNCTION_NO_RETURN = 3;
const int EXCEPTION_ARRAY_OUT_OF_BOUNDS = 4;
const int EXCEPTION_INVALID_FUNCTION = 5;
const int EXCEPTION_CONVERSION_ERROR = 6;
const int EXCEPTION_ALLOCATED_STACK_OVERFLOW = 7;
const int EXCEPTION_INVALID_POINTER = 8;
const int EXCEPTION_FAILED_TO_RESERVE = 9;
void SetLongJmpTarget(sigjmp_buf target);
#endif

void SetLastInstruction(x86Instruction *pos, x86Instruction *base);
x86Instruction* GetLastInstruction();

unsigned int	GetLastALULabel();

void OptimizationLookBehind(bool allow);
unsigned int GetOptimizationCount();

void GenCodeCmdNop(VMCmd cmd);

void GenCodeCmdPushChar(VMCmd cmd);
void GenCodeCmdPushShort(VMCmd cmd);
void GenCodeCmdPushInt(VMCmd cmd);
void GenCodeCmdPushFloat(VMCmd cmd);
void GenCodeCmdPushDorL(VMCmd cmd);
void GenCodeCmdPushCmplx(VMCmd cmd);

void GenCodeCmdPushCharStk(VMCmd cmd);
void GenCodeCmdPushShortStk(VMCmd cmd);
void GenCodeCmdPushIntStk(VMCmd cmd);
void GenCodeCmdPushFloatStk(VMCmd cmd);
void GenCodeCmdPushDorLStk(VMCmd cmd);
void GenCodeCmdPushCmplxStk(VMCmd cmd);

void GenCodeCmdPushImmt(VMCmd cmd);

void GenCodeCmdMovChar(VMCmd cmd);
void GenCodeCmdMovShort(VMCmd cmd);
void GenCodeCmdMovInt(VMCmd cmd);
void GenCodeCmdMovFloat(VMCmd cmd);
void GenCodeCmdMovDorL(VMCmd cmd);
void GenCodeCmdMovCmplx(VMCmd cmd);

void GenCodeCmdMovCharStk(VMCmd cmd);
void GenCodeCmdMovShortStk(VMCmd cmd);
void GenCodeCmdMovIntStk(VMCmd cmd);
void GenCodeCmdMovFloatStk(VMCmd cmd);
void GenCodeCmdMovDorLStk(VMCmd cmd);
void GenCodeCmdMovCmplxStk(VMCmd cmd);

void GenCodeCmdPop(VMCmd cmd);

void GenCodeCmdDtoI(VMCmd cmd);
void GenCodeCmdDtoL(VMCmd cmd);
void GenCodeCmdDtoF(VMCmd cmd);
void GenCodeCmdItoD(VMCmd cmd);
void GenCodeCmdLtoD(VMCmd cmd);
void GenCodeCmdItoL(VMCmd cmd);
void GenCodeCmdLtoI(VMCmd cmd);

void GenCodeCmdIndex(VMCmd cmd);

void GenCodeCmdCopyDorL(VMCmd cmd);
void GenCodeCmdCopyI(VMCmd cmd);

void GenCodeCmdGetAddr(VMCmd cmd);
void GenCodeCmdFuncAddr(VMCmd cmd);

void GenCodeCmdSetRange(VMCmd cmd);

void GenCodeCmdJmp(VMCmd cmd);
void GenCodeCmdJmpZ(VMCmd cmd);
void GenCodeCmdJmpNZ(VMCmd cmd);

void GenCodeCmdCall(VMCmd cmd);
void GenCodeCmdCallPtr(VMCmd cmd);
void GenCodeCmdReturn(VMCmd cmd);

void GenCodeCmdPushVTop(VMCmd cmd);
void GenCodeCmdPopVTop(VMCmd cmd);

void GenCodeCmdPushV(VMCmd cmd);

void GenCodeCmdAdd(VMCmd cmd);
void GenCodeCmdSub(VMCmd cmd);
void GenCodeCmdMul(VMCmd cmd);
void GenCodeCmdDiv(VMCmd cmd);
void GenCodeCmdPow(VMCmd cmd);
void GenCodeCmdMod(VMCmd cmd);
void GenCodeCmdLess(VMCmd cmd);
void GenCodeCmdGreater(VMCmd cmd);
void GenCodeCmdLEqual(VMCmd cmd);
void GenCodeCmdGEqual(VMCmd cmd);
void GenCodeCmdEqual(VMCmd cmd);
void GenCodeCmdNEqual(VMCmd cmd);
void GenCodeCmdShl(VMCmd cmd);
void GenCodeCmdShr(VMCmd cmd);
void GenCodeCmdBitAnd(VMCmd cmd);
void GenCodeCmdBitOr(VMCmd cmd);
void GenCodeCmdBitXor(VMCmd cmd);
void GenCodeCmdLogAnd(VMCmd cmd);
void GenCodeCmdLogOr(VMCmd cmd);
void GenCodeCmdLogXor(VMCmd cmd);

void GenCodeCmdAddL(VMCmd cmd);
void GenCodeCmdSubL(VMCmd cmd);
void GenCodeCmdMulL(VMCmd cmd);
void GenCodeCmdDivL(VMCmd cmd);
void GenCodeCmdPowL(VMCmd cmd);
void GenCodeCmdModL(VMCmd cmd);
void GenCodeCmdLessL(VMCmd cmd);
void GenCodeCmdGreaterL(VMCmd cmd);
void GenCodeCmdLEqualL(VMCmd cmd);
void GenCodeCmdGEqualL(VMCmd cmd);
void GenCodeCmdEqualL(VMCmd cmd);
void GenCodeCmdNEqualL(VMCmd cmd);
void GenCodeCmdShlL(VMCmd cmd);
void GenCodeCmdShrL(VMCmd cmd);
void GenCodeCmdBitAndL(VMCmd cmd);
void GenCodeCmdBitOrL(VMCmd cmd);
void GenCodeCmdBitXorL(VMCmd cmd);
void GenCodeCmdLogAndL(VMCmd cmd);
void GenCodeCmdLogOrL(VMCmd cmd);
void GenCodeCmdLogXorL(VMCmd cmd);

void GenCodeCmdAddD(VMCmd cmd);
void GenCodeCmdSubD(VMCmd cmd);
void GenCodeCmdMulD(VMCmd cmd);
void GenCodeCmdDivD(VMCmd cmd);
void GenCodeCmdPowD(VMCmd cmd);
void GenCodeCmdModD(VMCmd cmd);
void GenCodeCmdLessD(VMCmd cmd);
void GenCodeCmdGreaterD(VMCmd cmd);
void GenCodeCmdLEqualD(VMCmd cmd);
void GenCodeCmdGEqualD(VMCmd cmd);
void GenCodeCmdEqualD(VMCmd cmd);
void GenCodeCmdNEqualD(VMCmd cmd);

void GenCodeCmdNeg(VMCmd cmd);
void GenCodeCmdBitNot(VMCmd cmd);
void GenCodeCmdLogNot(VMCmd cmd);

void GenCodeCmdNegL(VMCmd cmd);
void GenCodeCmdBitNotL(VMCmd cmd);
void GenCodeCmdLogNotL(VMCmd cmd);

void GenCodeCmdNegD(VMCmd cmd);

void GenCodeCmdIncI(VMCmd cmd);
void GenCodeCmdIncD(VMCmd cmd);
void GenCodeCmdIncL(VMCmd cmd);

void GenCodeCmdDecI(VMCmd cmd);
void GenCodeCmdDecD(VMCmd cmd);
void GenCodeCmdDecL(VMCmd cmd);

void SetClosureCreateFunc(void (*f)());
void GenCodeCmdCreateClosure(VMCmd cmd);

void SetUpvaluesCloseFunc(void (*f)());
void GenCodeCmdCloseUpvalues(VMCmd cmd);

void GenCodeCmdConvertPtr(VMCmd cmd);

void GenCodeCmdYield(VMCmd cmd);
