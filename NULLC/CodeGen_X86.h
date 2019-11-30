#pragma once

#include "stdafx.h"
#include "Instruction_X86.h"

struct CodeGenGenericContext
{
	CodeGenGenericContext()
	{
		x86Op = NULL;
		x86Base = NULL;

		x86LookBehind = true;

		optimizationCount = 0;
	}

	void SetLastInstruction(x86Instruction *pos, x86Instruction *base)
	{
		x86Op = pos;
		x86Base = base;
	}

	x86Instruction* GetLastInstruction()
	{
		return x86Op;
	}

	x86Instruction *x86Op;
	x86Instruction *x86Base;

	bool x86LookBehind;

	unsigned optimizationCount;
};

void EMIT_COMMENT(CodeGenGenericContext &ctx, const char* text);
void EMIT_LABEL(CodeGenGenericContext &ctx, unsigned int labelID, int invalidate = true);

void EMIT_OP(CodeGenGenericContext &ctx, x86Command op);
void EMIT_OP_LABEL(CodeGenGenericContext &ctx, x86Command op, unsigned int labelID, int invalidate = true, int longJump = false);
void EMIT_CALL_REG(CodeGenGenericContext &ctx, x86Reg reg1);
void EMIT_OP_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1);
void EMIT_OP_FPUREG(CodeGenGenericContext &ctx, x86Command op, x87Reg reg1);
void EMIT_OP_NUM(CodeGenGenericContext &ctx, x86Command op, unsigned int num);

void EMIT_OP_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift);
void EMIT_OP_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg2, unsigned int shift);
void EMIT_OP_ADDR(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned int addr);

void EMIT_OP_REG_NUM(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, unsigned int num);
void EMIT_OP_REG_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Reg reg2);

void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift);
void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, x86Reg reg2, unsigned int shift);
void EMIT_OP_REG_ADDR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, unsigned int addr);

void EMIT_OP_REG_LABEL(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, unsigned int labelID, unsigned int shift);

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned int shift, x86Reg reg2);
void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned int shift, x86Reg reg2);
void EMIT_OP_ADDR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned int addr, x86Reg reg2);

void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned int shift, unsigned int num);
void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned int shift, unsigned int num);
void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned int addr, unsigned int number);

void EMIT_REG_READ(CodeGenGenericContext &ctx, x86Reg reg); // Call to mark that register was implicitly used
void EMIT_REG_KILL(CodeGenGenericContext &ctx, x86Reg reg); // Call to signal that the register value will no longer be used

void SetOptimizationLookBehind(CodeGenGenericContext &ctx, bool allow);
