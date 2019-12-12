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

		lastInvalidate = 0;

		memset(genReg, 0, rRegCount * sizeof(x86Argument));
		memset(genRegUpdate, 0, rRegCount * sizeof(unsigned));
		memset(genRegRead, 0, rRegCount * sizeof(bool));

		memset(xmmReg, 0, rXmmRegCount * sizeof(x86Argument));
		memset(xmmRegUpdate, 0, rXmmRegCount * sizeof(unsigned));
		memset(xmmRegRead, 0, rXmmRegCount * sizeof(bool));

		memset(memCache, 0, memoryStateSize * sizeof(MemCache));

		memCacheEntries = 0;

		optimizationCount = 0;

		currFreeReg = 0;
		currFreeXmmReg = rXMM0;

		skipTracking = false;
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

	unsigned MemFind(const x86Argument &address);
	unsigned MemIntersectFind(const x86Argument &address);

	void MemRead(const x86Argument &address);
	void MemWrite(const x86Argument &address, const x86Argument &value);
	void MemUpdate(unsigned index);

	void InvalidateState();
	void InvalidateDependand(x86Reg dreg);
	void InvalidateDependand(x86XmmReg dreg);
	void InvalidateAddressValue(x86Argument arg);

	void KillRegister(x86Reg reg);
	void KillRegister(x86XmmReg reg);

	void KillUnreadRegisters();

	void ReadRegister(x86Reg reg);
	void ReadRegister(x86XmmReg reg);

	void OverwriteRegisterWithValue(x86Reg reg, x86Argument arg);
	void OverwriteRegisterWithUnknown(x86Reg reg);
	void OverwriteRegisterWithValue(x86XmmReg reg, x86Argument arg);
	void OverwriteRegisterWithUnknown(x86XmmReg reg);

	void ReadAndModifyRegister(x86Reg reg);
	void ReadAndModifyRegister(x86XmmReg reg);

	void RedirectAddressComputation(x86Reg &index, int &multiplier, x86Reg &base, unsigned &shift);

	x86Reg RedirectRegister(x86Reg reg);
	x86XmmReg RedirectRegister(x86XmmReg reg);

	x86Reg GetReg();
	x86XmmReg GetXmmReg();

	x86Instruction *x86Op;
	x86Instruction *x86Base;

	bool x86LookBehind;

	unsigned lastInvalidate;

	x86Argument genReg[rRegCount];		// Holds current register value
	unsigned genRegUpdate[rRegCount];	// Marks the last instruction that wrote to the register
	bool genRegRead[rRegCount];			// Marks if there was a read from the register after last write

	x86Argument xmmReg[rXmmRegCount];		// Holds current register value
	unsigned xmmRegUpdate[rXmmRegCount];	// Marks the last instruction that wrote to the register
	bool xmmRegRead[rXmmRegCount];			// Marks if there was a read from the register after last write

	struct MemCache
	{
		x86Argument	address;
		x86Argument value;
		unsigned location; // Location of the memory update
		bool read; // Mark if this location was read after last write
	};

	static const unsigned memoryStateSize = 16;
	MemCache memCache[memoryStateSize];
	unsigned memCacheEntries;

	unsigned optimizationCount;

	unsigned currFreeReg;
	x86XmmReg currFreeXmmReg;

	bool skipTracking;
};

void EMIT_COMMENT(CodeGenGenericContext &ctx, const char* text);
void EMIT_LABEL(CodeGenGenericContext &ctx, unsigned labelID, int invalidate = true);

void EMIT_OP(CodeGenGenericContext &ctx, x86Command op);
void EMIT_OP_LABEL(CodeGenGenericContext &ctx, x86Command op, unsigned labelID, int invalidate = true, int longJump = false);
void EMIT_OP_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1);
void EMIT_OP_REG(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1);
void EMIT_OP_NUM(CodeGenGenericContext &ctx, x86Command op, unsigned num);

void EMIT_OP_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift);
void EMIT_OP_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg2, unsigned shift);
void EMIT_OP_ADDR(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned addr);

void EMIT_OP_REG_NUM(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, unsigned num);
void EMIT_OP_REG_NUM64(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, unsigned long long num);
void EMIT_OP_REG_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Reg reg2);
void EMIT_OP_REG_REG(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86XmmReg reg2);
void EMIT_OP_REG_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86XmmReg reg2);
void EMIT_OP_REG_REG(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Reg reg2);

void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift);
void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift);
void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, x86Reg reg2, unsigned shift);
void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Size size, x86Reg reg2, unsigned shift);
void EMIT_OP_REG_ADDR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, unsigned addr);
void EMIT_OP_REG_ADDR(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Size size, unsigned addr);

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift, x86Reg reg2);
void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift, x86XmmReg reg2);
void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned shift, x86Reg reg2);
void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned shift, x86XmmReg reg2);
void EMIT_OP_ADDR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned addr, x86Reg reg2);
void EMIT_OP_ADDR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned addr, x86XmmReg reg2);

void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift, unsigned num);
void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned shift, unsigned num);
void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned addr, unsigned number);

// Call to mark that register was implicitly used
void EMIT_REG_READ(CodeGenGenericContext &ctx, x86Reg reg);
void EMIT_REG_READ(CodeGenGenericContext &ctx, x86XmmReg reg);

// Call to signal that the register value will no longer be used
void EMIT_REG_KILL(CodeGenGenericContext &ctx, x86Reg reg);
void EMIT_REG_KILL(CodeGenGenericContext &ctx, x86XmmReg reg);

void SetOptimizationLookBehind(CodeGenGenericContext &ctx, bool allow);
