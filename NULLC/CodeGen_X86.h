#pragma once

#include "stdafx.h"

#include "Instruction_X86.h"
#include "ParseClass.h"
#include "Bytecode.h"

char* InlFmt(const char *str, ...);

void ResetStackTracking();
unsigned int GetStackTrackInfo();

void SetParamBase(unsigned int base);

void GenCodeCmdNop(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdPushCharAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushShortAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushIntAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushFloatAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushDorLAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushCmplxAbs(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdPushCharRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushShortRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushIntRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushFloatRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushDorLRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushCmplxRel(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdPushCharStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushShortStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushIntStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushFloatStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushDorLStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPushCmplxStk(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdPushImmt(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdMovCharAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovShortAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovIntAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovFloatAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovDorLAbs(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovCmplxAbs(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdMovCharRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovShortRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovIntRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovFloatRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovDorLRel(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovCmplxRel(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdMovCharStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovShortStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovIntStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovFloatStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovDorLStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMovCmplxStk(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdReserveV(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdPopCharTop(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPopShortTop(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPopIntTop(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPopFloatTop(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPopDorLTop(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPopCmplxTop(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPop(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdDtoI(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdDtoL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdDtoF(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdItoD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLtoD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdItoL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLtoI(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdImmtMul(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdCopyDorL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdCopyI(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdGetAddr(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdSetRange(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdJmp(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdJmpZI(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdJmpZD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdJmpZL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdJmpNZI(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdJmpNZD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdJmpNZL(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdCall(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdReturn(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdPushVTop(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPopVTop(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdPushV(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdAdd(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdSub(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMul(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdDiv(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPow(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMod(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLess(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdGreater(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLEqual(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdGEqual(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdEqual(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdNEqual(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdShl(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdShr(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdBitAnd(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdBitOr(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdBitXor(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLogAnd(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLogOr(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLogXor(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdAddL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdSubL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMulL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdDivL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPowL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdModL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLessL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdGreaterL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLEqualL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdGEqualL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdEqualL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdNEqualL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdShlL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdShrL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdBitAndL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdBitOrL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdBitXorL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLogAndL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLogOrL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLogXorL(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdAddD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdSubD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdMulD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdDivD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdPowD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdModD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLessD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdGreaterD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLEqualD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdGEqualD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdEqualD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdNEqualD(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdNeg(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdBitNot(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLogNot(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdNegL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdBitNotL(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLogNotL(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdNegD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdLogNotD(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdIncI(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdIncD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdIncL(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdDecI(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdDecD(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdDecL(VMCmd cmd, FastVector<x86Instruction> &instList);

void GenCodeCmdAddAtCharStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdAddAtShortStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdAddAtIntStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdAddAtLongStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdAddAtFloatStk(VMCmd cmd, FastVector<x86Instruction> &instList);
void GenCodeCmdAddAtDoubleStk(VMCmd cmd, FastVector<x86Instruction> &instList);
