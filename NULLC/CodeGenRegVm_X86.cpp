#include "CodeGenRegVm_X86.h"

#include "Bytecode.h"
#include "CodeGen_X86.h"
#include "Executor_Common.h"
#include "Executor_X86.h"
#include "InstructionTreeRegVm.h"
#include "StdLib.h"

#if defined(_M_X64)
const x86Reg rREG = rRBX;
#else
const x86Reg rREG = rEBX;
#endif

#if defined(__linux)
const x86Reg rArg1 = rRDI;
const x86Reg rArg2 = rRSI;
const x86Reg rArg3 = rRDX;
#else
const x86Reg rArg1 = rECX;
const x86Reg rArg2 = rEDX;
const x86Reg rArg3 = rR8;
#endif

// TODO: special handling for rvrrGlobals

#if defined(_M_X64)

void GenCodeLoadInt8FromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg targetReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, targetReg, sBYTE, rR15, offset); // Load byte value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, targetReg, sBYTE, tempReg, offset); // Load byte value with sign extension
	}
}

void GenCodeStoreInt8ToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sBYTE, rR15, offset, sourceReg); // Store byte value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sBYTE, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

void GenCodeLoadInt16FromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg targetReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, targetReg, sWORD, rR15, offset); // Load short value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, targetReg, sWORD, tempReg, offset); // Load short value with sign extension
	}
}

void GenCodeStoreInt16ToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sWORD, rR15, offset, sourceReg); // Store short value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

void GenCodeLoadInt32FromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg targetReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrRegisters)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetReg, sDWORD, rREG, offset); // Load int value
	}
	else if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetReg, sDWORD, rR15, offset); // Load int value
	}
	else if(reg == rvrrConstants)
	{
		// Load int immediate
		int value = ctx.exRegVmConstants[offset >> 2];

		if(value == 0)
			EMIT_OP_REG_REG(ctx.ctx, o_xor, targetReg, targetReg);
		else
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, targetReg, value);
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetReg, sDWORD, tempReg, offset); // Load int value
	}
}

x86Reg GenCodeLoadInt32FromPointerIntoRegister(CodeGenRegVmContext &ctx, unsigned char reg, unsigned offset)
{
	if(reg == rvrrRegisters)
	{
		x86Reg targetReg = ctx.ctx.FindRegAtMemory(sDWORD, rNONE, 1, rREG, offset, true);

		if(targetReg != rRegCount)
			return targetReg;

		targetReg = ctx.ctx.GetReg();

		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetReg, sDWORD, rREG, offset); // Load double value

		return targetReg;
	}

	if(reg == rvrrFrame)
	{
		x86Reg targetReg = ctx.ctx.FindRegAtMemory(sDWORD, rNONE, 1, rR15, offset, true);

		if(targetReg != rRegCount)
			return targetReg;

		targetReg = ctx.ctx.GetReg();

		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetReg, sDWORD, rR15, offset); // Load double value

		return targetReg;
	}

	if(reg == rvrrConstants)
	{
		x86Reg targetReg = ctx.ctx.FindRegAtMemory(sDWORD, rNONE, 1, rR14, offset, true);

		if(targetReg != rRegCount)
			return targetReg;

		targetReg = ctx.ctx.GetReg();

		// Load int immediate
		int value = ctx.exRegVmConstants[offset >> 2];

		if(value == 0)
			EMIT_OP_REG_REG(ctx.ctx, o_xor, targetReg, targetReg);
		else
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, targetReg, value);

		return targetReg;
	}

	x86Reg targetReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, rREG, reg * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetReg, sDWORD, targetReg, offset); // Load int value

	return targetReg;
}

void GenCodeStoreInt32ToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rR15, offset, sourceReg); // Store int value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

void GenCodeLoadInt64FromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg targetReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrRegisters)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, rREG, offset); // Load long value
	}
	else if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, rR15, offset); // Load long value
	}
	else if(reg == rvrrConstants)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, rR14, offset); // Load long value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, tempReg, offset); // Load long value
	}
}

x86Reg GenCodeLoadInt64FromPointerIntoRegister(CodeGenRegVmContext &ctx, unsigned char reg, unsigned offset)
{
	if(reg == rvrrRegisters)
	{
		x86Reg targetReg = ctx.ctx.FindRegAtMemory(sQWORD, rNONE, 1, rREG, offset, true);

		if(targetReg != rRegCount)
			return targetReg;

		targetReg = ctx.ctx.GetReg();

		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, rREG, offset); // Load long value

		return targetReg;
	}

	if(reg == rvrrFrame)
	{
		x86Reg targetReg = ctx.ctx.FindRegAtMemory(sQWORD, rNONE, 1, rR15, offset, true);

		if(targetReg != rRegCount)
			return targetReg;

		targetReg = ctx.ctx.GetReg();

		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, rR15, offset); // Load long value

		return targetReg;
	}

	if(reg == rvrrConstants)
	{
		x86Reg targetReg = ctx.ctx.FindRegAtMemory(sQWORD, rNONE, 1, rR14, offset, true);

		if(targetReg != rRegCount)
			return targetReg;

		targetReg = ctx.ctx.GetReg();

		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, rR14, offset); // Load long value

		return targetReg;
	}

	x86Reg targetReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, rREG, reg * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, targetReg, offset); // Load long value

	return targetReg;
}

void GenCodeStoreInt64ToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rR15, offset, sourceReg); // Store long value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

void GenCodeLoadFloatFromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86XmmReg targetReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_cvtss2sd, targetReg, sDWORD, rR15, offset); // Load float value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_cvtss2sd, targetReg, sDWORD, tempReg, offset); // Load float value
	}
}

void GenCodeStoreFloatToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86XmmReg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_movss, sDWORD, rR15, offset, sourceReg); // Store float value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_movss, sDWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

void GenCodeLoadDoubleFromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86XmmReg targetReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrRegisters)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, rREG, offset); // Load double value
	}
	else if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, rR15, offset); // Load double value
	}
	else if(reg == rvrrConstants)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, rR14, offset); // Load double value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, tempReg, offset); // Load double value
	}
}

x86XmmReg GenCodeLoadDoubleFromPointerIntoRegister(CodeGenRegVmContext &ctx, x86Reg tempReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrRegisters)
	{
		x86XmmReg targetReg = ctx.ctx.FindXmmRegAtMemory(sQWORD, rNONE, 1, rREG, offset, true);

		if(targetReg != rXmmRegCount)
			return targetReg;

		targetReg = ctx.ctx.GetXmmReg();

		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, rREG, offset); // Load double value

		return targetReg;
	}

	if(reg == rvrrFrame)
	{
		x86XmmReg targetReg = ctx.ctx.FindXmmRegAtMemory(sQWORD, rNONE, 1, rR15, offset, true);

		if(targetReg != rXmmRegCount)
			return targetReg;

		targetReg = ctx.ctx.GetXmmReg();

		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, rR15, offset); // Load double value

		return targetReg;
	}

	if(reg == rvrrConstants)
	{
		x86XmmReg targetReg = ctx.ctx.FindXmmRegAtMemory(sQWORD, rNONE, 1, rR14, offset, true);

		if(targetReg != rXmmRegCount)
			return targetReg;

		targetReg = ctx.ctx.GetXmmReg();

		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, rR14, offset); // Load double value

		return targetReg;
	}

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load source pointer

	x86XmmReg targetReg = ctx.ctx.FindXmmRegAtMemory(sQWORD, rNONE, 1, tempReg, offset, true);

	if(targetReg != rXmmRegCount)
		return targetReg;

	targetReg = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, tempReg, offset); // Load double value

	return targetReg;
}

void GenCodeStoreDoubleToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86XmmReg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rR15, offset, sourceReg); // Store double value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

#else

void x86GenCodeLoadInt8FromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg targetReg, unsigned char reg, unsigned offset)
{
	assert(reg != rvrrRegisters && reg != rvrrConstants);

	if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, targetReg, sBYTE, rESI, offset); // Load byte value
	}
	else if(reg == rvrrGlobals)
	{
		assert(ctx.vmState->dataStackBase);

		EMIT_OP_REG_ADDR(ctx.ctx, o_movsx, targetReg, sBYTE, uintptr_t(ctx.vmState->dataStackBase) + offset); // Load byte value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, targetReg, sBYTE, tempReg, offset); // Load byte value with sign extension
	}
}

void x86GenCodeStoreInt8ToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sBYTE, rESI, offset, sourceReg); // Store byte value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sBYTE, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

void x86GenCodeLoadInt16FromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg targetReg, unsigned char reg, unsigned offset)
{
	assert(reg != rvrrRegisters && reg != rvrrConstants);

	if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, targetReg, sWORD, rESI, offset); // Load short value
	}
	else if(reg == rvrrGlobals)
	{
		assert(ctx.vmState->dataStackBase);

		EMIT_OP_REG_ADDR(ctx.ctx, o_movsx, targetReg, sWORD, uintptr_t(ctx.vmState->dataStackBase) + offset); // Load short value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, targetReg, sWORD, tempReg, offset); // Load short value with sign extension
	}
}

void x86GenCodeStoreInt16ToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sWORD, rESI, offset, sourceReg); // Store short value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

void x86GenCodeLoadInt32FromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg targetReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrRegisters)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetReg, sDWORD, rREG, offset); // Load int value
	}
	else if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetReg, sDWORD, rESI, offset); // Load int value
	}
	else if(reg == rvrrGlobals)
	{
		assert(ctx.vmState->dataStackBase);

		EMIT_OP_REG_ADDR(ctx.ctx, o_mov, targetReg, sDWORD, uintptr_t(ctx.vmState->dataStackBase) + offset); // Load int value
	}
	else if(reg == rvrrConstants)
	{
		// Load int immediate
		int value = ctx.exRegVmConstants[offset >> 2];

		if(value == 0)
			EMIT_OP_REG_REG(ctx.ctx, o_xor, targetReg, targetReg);
		else
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, targetReg, value);
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetReg, sDWORD, tempReg, offset); // Load int value
	}
}

void x86GenCodeStoreInt32ToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rESI, offset, sourceReg); // Store int value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

void x86GenCodeLoadInt64FromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg targetRegA, x86Reg targetRegB, unsigned char reg, unsigned offset)
{
	assert(tempReg != targetRegA);

	if(reg == rvrrRegisters)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetRegA, sDWORD, rREG, offset); // Load int value
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetRegB, sDWORD, rREG, offset + 4);
	}
	else if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetRegA, sDWORD, rESI, offset); // Load long value
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetRegB, sDWORD, rESI, offset + 4);
	}
	else if(reg == rvrrGlobals)
	{
		assert(ctx.vmState->dataStackBase);

		EMIT_OP_REG_ADDR(ctx.ctx, o_mov, targetRegA, sDWORD, uintptr_t(ctx.vmState->dataStackBase) + offset); // Load long value
		EMIT_OP_REG_ADDR(ctx.ctx, o_mov, targetRegB, sDWORD, uintptr_t(ctx.vmState->dataStackBase) + offset + 4);
	}
	else if(reg == rvrrConstants)
	{
		// Load int immediate
		int value = ctx.exRegVmConstants[offset >> 2];

		if(value == 0)
			EMIT_OP_REG_REG(ctx.ctx, o_xor, targetRegA, targetRegA);
		else
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, targetRegA, value);

		value = ctx.exRegVmConstants[(offset >> 2) + 1];

		if(value == 0)
			EMIT_OP_REG_REG(ctx.ctx, o_xor, targetRegB, targetRegB);
		else
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, targetRegB, value);
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load source pointer

		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetRegA, sDWORD, tempReg, offset); // Load long value
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, targetRegB, sDWORD, tempReg, offset + 4);
	}
}

void x86GenCodeStoreInt64ToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg sourceRegA, x86Reg sourceRegB, unsigned char reg, unsigned offset)
{
	assert(tempReg != sourceRegA);
	assert(tempReg != sourceRegB);

	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rESI, offset, sourceRegA); // Store int value
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rESI, offset + 4, sourceRegB); // Store int value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load target pointer

		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, tempReg, offset, sourceRegA); // Store value to target with an offset
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, tempReg, offset + 4, sourceRegB); // Store value to target with an offset
	}
}

void x86GenCodeLoadFloatFromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86XmmReg targetReg, unsigned char reg, unsigned offset)
{
	assert(reg != rvrrRegisters && reg != rvrrConstants);

	if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_cvtss2sd, targetReg, sDWORD, rESI, offset); // Load float value
	}
	else if(reg == rvrrGlobals)
	{
		assert(ctx.vmState->dataStackBase);

		EMIT_OP_REG_ADDR(ctx.ctx, o_cvtss2sd, targetReg, sDWORD, uintptr_t(ctx.vmState->dataStackBase) + offset); // Load float value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_cvtss2sd, targetReg, sDWORD, tempReg, offset); // Load float value
	}
}

void x86GenCodeStoreFloatToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86XmmReg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_movss, sDWORD, rESI, offset, sourceReg); // Store float value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_movss, sDWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

void x86GenCodeLoadDoubleFromPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86XmmReg targetReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrRegisters)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, rREG, offset); // Load double value
	}
	else if(reg == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, rESI, offset); // Load double value
	}
	else if(reg == rvrrGlobals)
	{
		assert(ctx.vmState->dataStackBase);

		EMIT_OP_REG_ADDR(ctx.ctx, o_movsd, targetReg, sQWORD, uintptr_t(ctx.vmState->dataStackBase) + offset); // Load double value
	}
	else if(reg == rvrrConstants)
	{
		// TODO: like old function pointer tables, jit has to store a copy of all constants so that the buffer won't relocate after a link (or patch all code locations like this one)
		EMIT_OP_REG_ADDR(ctx.ctx, o_movsd, targetReg, sQWORD, uintptr_t(ctx.exRegVmConstants) + offset); // Load double value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load source pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, targetReg, sQWORD, tempReg, offset); // Load double value
	}
}

void x86GenCodeStoreDoubleToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86XmmReg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rESI, offset, sourceReg); // Store double value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, tempReg, sDWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

#endif

void GenCodeCmdNop(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));
}

void GenCodeCmdLoadByte(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	GenCodeLoadInt8FromPointer(ctx, rRAX, rEAX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	x86GenCodeLoadInt8FromPointer(ctx, rEAX, rEAX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdLoadWord(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	GenCodeLoadInt16FromPointer(ctx, rRAX, rEAX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	x86GenCodeLoadInt16FromPointer(ctx, rEAX, rEAX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdLoadDword(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg target = GenCodeLoadInt32FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, target); // Store int to target
#else
	x86GenCodeLoadInt32FromPointer(ctx, rEAX, rEAX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdLoadLong(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg target = GenCodeLoadInt64FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, target); // Store long to target
#else
	x86GenCodeLoadInt64FromPointer(ctx, rEDX, rEAX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store long to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX);
#endif
}

void GenCodeCmdLoadFloat(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg target = ctx.ctx.GetXmmReg();

	GenCodeLoadFloatFromPointer(ctx, rRAX, target, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, target); // Store double to target
#else
	x86XmmReg target = ctx.ctx.GetXmmReg();

	x86GenCodeLoadFloatFromPointer(ctx, rEAX, target, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, target); // Store double to target
#endif
}

void GenCodeCmdLoadDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg target = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, target); // Store double to target
#else
	x86XmmReg target = ctx.ctx.GetXmmReg();

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, target, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, target); // Store double to target
#endif
}

void GenCodeCmdLoadImm(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, cmd.argument); // Store int to target
#else
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, cmd.argument); // Store int to target
#endif
}

void GenCodeCmdLoadImmLong(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	// TODO: should be as simple as on x86
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRAX, ((uint64_t)cmd.argument << 32ull));
	EMIT_OP_REG_REG(ctx.ctx, o_xor64, rRDX, rRDX);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load int value
	EMIT_OP_REG_REG(ctx.ctx, o_or64, rRAX, rRDX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long to target
#else
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, cmd.argument); // Store int to top of the target
#endif
}

void GenCodeCmdLoadImmDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreByte(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value

	GenCodeStoreInt8ToPointer(ctx, rRAX, rEDX, cmd.rC, cmd.argument);
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value

	x86GenCodeStoreInt8ToPointer(ctx, rEAX, rEDX, cmd.rC, cmd.argument);
#endif
}

void GenCodeCmdStoreWord(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value

	GenCodeStoreInt16ToPointer(ctx, rRAX, rEDX, cmd.rC, cmd.argument);
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value

	x86GenCodeStoreInt16ToPointer(ctx, rEAX, rEDX, cmd.rC, cmd.argument);
#endif
}

void GenCodeCmdStoreDword(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value

	GenCodeStoreInt32ToPointer(ctx, rRAX, rEDX, cmd.rC, cmd.argument);
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value

	x86GenCodeStoreInt32ToPointer(ctx, rEAX, rEDX, cmd.rC, cmd.argument);
#endif
}

void GenCodeCmdStoreLong(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDX, sQWORD, rREG, cmd.rA * 8); // Load value

	GenCodeStoreInt64ToPointer(ctx, rRAX, rRDX, cmd.rC, cmd.argument);
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rA * 8); // Load value
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8 + 4);

	x86GenCodeStoreInt64ToPointer(ctx, rECX, rEAX, rEDX, cmd.rC, cmd.argument);
#endif
}

void GenCodeCmdStoreFloat(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsd2ss, temp, sQWORD, rREG, cmd.rA * 8); // Load value

	GenCodeStoreFloatToPointer(ctx, rRAX, temp, cmd.rC, cmd.argument);
#else
	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsd2ss, temp, sQWORD, rREG, cmd.rA * 8); // Load value

	x86GenCodeStoreFloatToPointer(ctx, rEAX, temp, cmd.rC, cmd.argument);
#endif
}

void GenCodeCmdStoreDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, temp, sQWORD, rREG, cmd.rA * 8); // Load value

	GenCodeStoreDoubleToPointer(ctx, rRAX, temp, cmd.rC, cmd.argument);
#else
	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, temp, sQWORD, rREG, cmd.rA * 8); // Load value

	x86GenCodeStoreDoubleToPointer(ctx, rEAX, temp, cmd.rC, cmd.argument);
#endif
}

void GenCodeCmdCombinedd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load low value
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rC * 8); // Load high value

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX);
}

void GenCodeCmdBreakupdd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load low value
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rC * 8 + 4); // Load high value

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rB * 8, rEAX); // Store to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEDX);
}

void GenCodeCmdMov(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	assert(cmd.rA != cmd.rC);

#if defined(_M_X64)
	x86Reg sourceReg = ctx.ctx.FindRegAtMemory(sQWORD, rNONE, 1, rREG, cmd.rC * 8, true);

	if(sourceReg != rRegCount)
	{
		ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, sourceReg); // Store to target
		return;
	}

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load source
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store to target
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8 + 4); // Load source
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEAX); // Store to target
#endif
}

void GenCodeCmdMovMult(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	assert(cmd.rA != cmd.rC);

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store to target

	if(((cmd.argument >> 16) & 0xff) != (cmd.argument >> 24))
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, ((cmd.argument >> 16) & 0xff) * 8); // Load source
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, (cmd.argument >> 24) * 8, rRAX); // Store to target
	}

	if((cmd.argument & 0xff) != ((cmd.argument >> 8) & 0xff))
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, (cmd.argument & 0xff) * 8); // Load source
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, ((cmd.argument >> 8) & 0xff) * 8, rRAX); // Store to target
	}
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load source
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store to target
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8 + 4); // Load source
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEAX); // Store to target

	if(((cmd.argument >> 16) & 0xff) != (cmd.argument >> 24))
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, ((cmd.argument >> 16) & 0xff) * 8); // Load source
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, (cmd.argument >> 24) * 8, rEAX); // Store to target
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, ((cmd.argument >> 16) & 0xff) * 8 + 4); // Load source
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, (cmd.argument >> 24) * 8 + 4, rEAX); // Store to target
	}

	if((cmd.argument & 0xff) != ((cmd.argument >> 8) & 0xff))
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, (cmd.argument & 0xff) * 8); // Load source
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, ((cmd.argument >> 8) & 0xff) * 8, rEAX); // Store to target
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, (cmd.argument & 0xff) * 8 + 4); // Load source
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, ((cmd.argument >> 8) & 0xff) * 8 + 4, rEAX); // Store to target
	}
#endif
}

void GenCodeCmdDtoi(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	EMIT_OP_REG_RPTR(ctx.ctx, o_cvttsd2si, rEAX, sQWORD, rREG, cmd.rC * 8); // Load double as int
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store value
}

void x86DtolWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].longValue = (long long)(regFilePtr[cmd.rC].doubleValue);
}

void GenCodeCmdDtol(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x86DtolWrap = x86DtolWrap;

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_cvttsd2si64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load double as long
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store value
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86DtolWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void GenCodeCmdDtof(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsd2ss, temp, sQWORD, rREG, cmd.rC * 8); // Load double as float
	EMIT_OP_RPTR_REG(ctx.ctx, o_movss, sDWORD, rREG, cmd.rA * 8, temp); // Store value
}

void GenCodeCmdItod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsi2sd, temp, sDWORD, rREG, cmd.rC * 8); // Load int as double
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, temp); // Store value
}

void x86LtodWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].doubleValue = double(regFilePtr[cmd.rC].longValue);
}

void GenCodeCmdLtod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x86LtodWrap = x86LtodWrap;

#if defined(_M_X64)
	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsi2sd64, temp, sQWORD, rREG, cmd.rC * 8); // Load long as double
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, temp); // Store value
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86LtodWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void GenCodeCmdItol(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_movsxd, rRAX, sDWORD, rREG, cmd.rC * 8); // Load int as long with sign extension
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store value
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load int
	EMIT_OP(ctx.ctx, o_cdq);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store value
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX); // Store value
#endif
}

void GenCodeCmdLtoi(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load lower int part of a long number
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store value
}

void ErrorOutOfBoundsWrap(CodeGenRegVmStateContext *vmState)
{
	CodeGenRegVmContext &ctx = *vmState->ctx;

	vmState->callStackTop->instruction = vmState->callInstructionPos + 1;
	vmState->callStackTop++;

	ctx.x86rvm->Stop("ERROR: array index out of bounds");
	longjmp(vmState->errorHandler, 1);
}

void GenCodeCmdIndex(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->errorOutOfBoundsWrap = ErrorOutOfBoundsWrap;

#if defined(_M_X64)
	x86Reg indexReg = ctx.ctx.GetReg();
	x86Reg pointerReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, indexReg, sDWORD, rREG, cmd.rB * 8); // Load index with zero extension to use in lea (top RAX bits are cleared)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rECX, sDWORD, rREG, ((cmd.argument >> 16) & 0xff) * 8); // Load size

	EMIT_OP_REG_REG(ctx.ctx, o_cmp, indexReg, rECX);
	EMIT_OP_LABEL(ctx.ctx, o_jb, ctx.labelCount, false);

	EMIT_OP_NUM(ctx.ctx, o_set_tracking, 0);

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg1, uintptr_t(ctx.vmState));
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->callInstructionPos) - uintptr_t(ctx.vmState)), ctx.currInstructionPos);
	EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->errorOutOfBoundsWrap) - uintptr_t(ctx.vmState)));

	EMIT_OP_NUM(ctx.ctx, o_set_tracking, 1);

	EMIT_LABEL(ctx.ctx, ctx.labelCount, false);
	ctx.labelCount++;

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, pointerReg, sQWORD, rREG, cmd.rC * 8); // Load source pointer

	// Multiply index by size and add to source pointer
	unsigned size = (cmd.argument & 0xffff);

	if(size == 1)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, pointerReg, sQWORD, indexReg, 1, pointerReg, 0);
	}
	else if(size == 2)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, pointerReg, sQWORD, indexReg, 2, pointerReg, 0);
	}
	else if(size == 4)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, pointerReg, sQWORD, indexReg, 4, pointerReg, 0);
	}
	else if(size == 8)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, pointerReg, sQWORD, indexReg, 8, pointerReg, 0);
	}
	else if(size == 16)
	{
		EMIT_OP_REG_NUM(ctx.ctx, o_shl, indexReg, 4); // 32 bit shift is ok, top bits are zero
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, pointerReg, sQWORD, indexReg, 1, pointerReg, 0);
	}
	else
	{
		EMIT_OP_REG_NUM(ctx.ctx, o_imul, indexReg, size); // 32 bit multiplication is ok, top bits are zero
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, pointerReg, sQWORD, indexReg, 1, pointerReg, 0);
	}

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, pointerReg); // Store to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load inde
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rECX, sDWORD, rREG, ((cmd.argument >> 16) & 0xff) * 8); // Load size

	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rECX);
	EMIT_OP_LABEL(ctx.ctx, o_jb, ctx.labelCount, false);

	EMIT_OP_NUM(ctx.ctx, o_set_tracking, 0);

	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, uintptr_t(&ctx.vmState->callInstructionPos), ctx.currInstructionPos);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->errorOutOfBoundsWrap));

	EMIT_OP_NUM(ctx.ctx, o_set_tracking, 1);

	EMIT_LABEL(ctx.ctx, ctx.labelCount, false);
	ctx.labelCount++;

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rC * 8); // Load source pointer

	// Multiply index by size and add to source pointer
	unsigned size = (cmd.argument & 0xffff);

	if(size == 1)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rEAX, sDWORD, rEAX, 1, rEDX, 0);
	}
	else if(size == 2)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rEAX, sDWORD, rEAX, 2, rEDX, 0);
	}
	else if(size == 4)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rEAX, sDWORD, rEAX, 4, rEDX, 0);
	}
	else if(size == 8)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rEAX, sDWORD, rEAX, 8, rEDX, 0);
	}
	else if(size == 16)
	{
		EMIT_OP_REG_NUM(ctx.ctx, o_shl, rEAX, 4); // 32 bit shift is ok, top bits are zero
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rEAX, sDWORD, rEAX, 1, rEDX, 0);
	}
	else
	{
		EMIT_OP_REG_NUM(ctx.ctx, o_imul, rEAX, size); // 32 bit multiplication is ok, top bits are zero
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rEAX, sDWORD, rEAX, 1, rEDX, 0);
	}

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store to target
#endif
}

void GenCodeCmdGetAddr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg targetReg = ctx.ctx.GetReg();

	if(cmd.rC == rvrrFrame)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, targetReg, sQWORD, rR15, cmd.argument); // Add offset
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, targetReg); // Store to target
	}
	else if(cmd.rC == rvrrConstants)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, targetReg, sQWORD, rR14, cmd.argument); // Add offset
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, targetReg); // Store to target
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, targetReg, sQWORD, rREG, cmd.rC * 8); // Load source pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_add64, targetReg, cmd.argument); // Add offset
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, targetReg); // Store to target
	}
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rEAX, cmd.argument); // Add offset
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store to target
#endif
}

void GenCodeCmdSetRange(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	switch(RegVmSetRangeType(cmd.rB))
	{
	case rvsrDouble:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rA * 8); // Load double value as long
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDI, sQWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosq);
		break;
	case rvsrFloat:
		EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsd2ss, rXMM0, sQWORD, rREG, cmd.rA * 8); // Load double as float
		EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, rXMM0); // Move to integer register
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDI, sQWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosd);
		break;
	case rvsrLong:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rA * 8); // Load long value
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDI, sQWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosq);
		break;
	case rvsrInt:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rA * 8); // Load integer
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDI, sQWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosd);
		break;
	case rvsrShort:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rA * 8); // Load integer
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDI, sQWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosw);
		break;
	case rvsrChar:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rA * 8); // Load integer
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDI, sQWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosb);
		break;
	default:
		assert(!"unknown type");
	}
#else
	switch(RegVmSetRangeType(cmd.rB))
	{
	case rvsrDouble:
	case rvsrLong:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rA * 8);
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8 + 4);

		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rECX, sDWORD, rEDI, cmd.argument * 8);

		EMIT_LABEL(ctx.ctx, ctx.labelCount);
		EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEDI, rECX);
		EMIT_OP_LABEL(ctx.ctx, o_je, ctx.labelCount + 1);

		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rEDI, 0, rEAX);
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rEDI, 4, rEDX);

		EMIT_OP_REG_NUM(ctx.ctx, o_add, rEDI, 8);
		EMIT_REG_READ(ctx.ctx, rEDI); // Mark that register is used (by next iteration)

		EMIT_OP_LABEL(ctx.ctx, o_jmp, ctx.labelCount);
		EMIT_LABEL(ctx.ctx, ctx.labelCount + 1);

		ctx.labelCount += 2;
		break;
	case rvsrFloat:
		EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsd2ss, rXMM0, sQWORD, rREG, cmd.rA * 8); // Load double as float
		EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, rXMM0); // Move to integer register
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosd);
		break;
	case rvsrInt:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rA * 8); // Load integer
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosd);
		break;
	case rvsrShort:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rA * 8); // Load integer
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosw);
		break;
	case rvsrChar:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rA * 8); // Load integer
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rC * 8); // Load target pointer
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument);
		EMIT_OP(ctx.ctx, o_rep_stosb);
		break;
	default:
		assert(!"unknown type");
	}
#endif
}

void GenCodeCmdMemCopy(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	assert(cmd.argument % 4 == 0);

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRSI, sQWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDI, sQWORD, rREG, cmd.rA * 8); // Load target pointer
	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument >> 2);
	EMIT_OP(ctx.ctx, o_rep_movsd);
#else
	EMIT_OP_REG_REG(ctx.ctx, o_mov, rEDX, rESI);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rESI, sDWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rA * 8); // Load target pointer
	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, cmd.argument >> 2);
	EMIT_OP(ctx.ctx, o_rep_movsd);
	EMIT_OP_REG_REG(ctx.ctx, o_mov, rESI, rEDX);
	EMIT_REG_READ(ctx.ctx, rESI);
#endif
}

void GenCodeCmdJmp(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	EMIT_OP_LABEL(ctx.ctx, o_jmp, LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
}

void GenCodeCmdJmpz(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	x86Reg sourceReg = ctx.ctx.FindRegAtMemory(sDWORD, rNONE, 1, rREG, cmd.rC * 8, true);

	if(sourceReg == rRegCount)
	{
		sourceReg = rEAX;

		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, sourceReg, sDWORD, rREG, cmd.rC * 8); // Load value

		ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);
		ctx.ctx.KillLateUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);
	}
	else
	{
		ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);
		ctx.ctx.KillLateUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

		if(ctx.ctx.x86LookBehind)
		{
			// Transform "xor, cmp, setcc, test, jz/jnz" into "cmp, jmpcc"
			x86Instruction *setInst = ctx.ctx.x86Op;

			while(setInst->name == o_none || setInst->name == o_other)
				setInst--;

			if((setInst->name >= o_setl && setInst->name <= o_setnz) && setInst->argA.type == x86Argument::argReg && setInst->argA.reg == sourceReg)
			{
				x86Command setCmd = setInst->name;

				x86Instruction *cmpInst = setInst - 1;

				while(cmpInst->name == o_none || cmpInst->name == o_other)
					cmpInst--;

				if(cmpInst->name == o_cmp)
				{
					x86Instruction *xorInst = cmpInst - 1;

					while(xorInst->name == o_none || xorInst->name == o_other)
						xorInst--;

					if(xorInst->name == o_xor && xorInst->argA.type == x86Argument::argReg && xorInst->argA.reg == sourceReg && xorInst->argB.type == x86Argument::argReg && xorInst->argB.reg == sourceReg)
					{
						// Keep xor if it's used by compare
						if(!((cmpInst->argA.type == x86Argument::argReg && cmpInst->argA.reg == sourceReg) || (cmpInst->argB.type == x86Argument::argReg && cmpInst->argB.reg == sourceReg)))
							xorInst->name = o_none;

						setInst->name = o_none;

						static const x86Command jump[] = { o_jge, o_jle, o_jg, o_jl, o_jne, o_je, o_jnz, o_jz };

						ctx.ctx.optimizationCount++;

						EMIT_OP_LABEL(ctx.ctx, jump[setCmd - o_setl], LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
						return;
					}
				}
			}
		}
	}

	EMIT_OP_REG_REG(ctx.ctx, o_test, sourceReg, sourceReg);
	EMIT_OP_LABEL(ctx.ctx, o_jz, LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
}

void GenCodeCmdJmpnz(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	x86Reg sourceReg = ctx.ctx.FindRegAtMemory(sDWORD, rNONE, 1, rREG, cmd.rC * 8, true);

	if(sourceReg == rRegCount)
	{
		sourceReg = rEAX;

		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, sourceReg, sDWORD, rREG, cmd.rC * 8); // Load value

		ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);
		ctx.ctx.KillLateUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);
	}
	else
	{
		ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);
		ctx.ctx.KillLateUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

		if(ctx.ctx.x86LookBehind)
		{
			// Transform "xor, cmp, setcc, test, jz/jnz" into "cmp, jmpcc"
			x86Instruction *setInst = ctx.ctx.x86Op;

			while(setInst->name == o_none || setInst->name == o_other)
				setInst--;

			if((setInst->name >= o_setl && setInst->name <= o_setnz) && setInst->argA.type == x86Argument::argReg && setInst->argA.reg == sourceReg)
			{
				x86Command setCmd = setInst->name;

				x86Instruction *cmpInst = setInst - 1;

				while(cmpInst->name == o_none || cmpInst->name == o_other)
					cmpInst--;

				if(cmpInst->name == o_cmp)
				{
					x86Instruction *xorInst = cmpInst - 1;

					while(xorInst->name == o_none || xorInst->name == o_other)
						xorInst--;

					if(xorInst->name == o_xor && xorInst->argA.type == x86Argument::argReg && xorInst->argA.reg == sourceReg && xorInst->argB.type == x86Argument::argReg && xorInst->argB.reg == sourceReg)
					{
						// Keep xor if it's used by compare
						if(!((cmpInst->argA.type == x86Argument::argReg && cmpInst->argA.reg == sourceReg) || (cmpInst->argB.type == x86Argument::argReg && cmpInst->argB.reg == sourceReg)))
							xorInst->name = o_none;

						setInst->name = o_none;

						static const x86Command jump[] = { o_jl, o_jg, o_jle, o_jge, o_je, o_jne, o_jz, o_jnz };

						ctx.ctx.optimizationCount++;

						EMIT_OP_LABEL(ctx.ctx, jump[setCmd - o_setl], LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
						return;
					}
				}
			}
		}
	}

	EMIT_OP_REG_REG(ctx.ctx, o_test, sourceReg, sourceReg);
	EMIT_OP_LABEL(ctx.ctx, o_jnz, LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
}

void CallWrap(CodeGenRegVmStateContext *vmState, unsigned functionId)
{
	CodeGenRegVmContext &ctx = *vmState->ctx;

	ExternFuncInfo &target = vmState->ctx->exFunctions[functionId];

	if(target.regVmAddress == -1)
	{
		if(target.funcPtrWrap)
		{
			target.funcPtrWrap(target.funcPtrWrapTarget, (char*)vmState->tempStackArrayBase, (char*)vmState->dataStackTop);
		}
		else
		{
#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
			RunRawExternalFunction(ctx.x86rvm->dcCallVM, ctx.exFunctions[functionId], ctx.exLocals, ctx.exTypes, ctx.exTypeExtra, (unsigned*)vmState->dataStackTop, (unsigned*)vmState->tempStackArrayBase);
#else
			ctx.x86rvm->Stop("ERROR: external raw function calls are disabled");
#endif
		}

		if(!ctx.x86rvm->callContinue)
			longjmp(vmState->errorHandler, 1);
	}
	else
	{
		unsigned char *codeStart = vmState->instAddress[target.regVmAddress];

		typedef	void (*nullcFunc)(unsigned char *codeStart, RegVmRegister *regFilePtr);
		nullcFunc gate = (nullcFunc)(uintptr_t)vmState->codeLaunchHeader;
		gate(codeStart, vmState->regFileLastPtr);
	}
}

void CallPtrWrap(CodeGenRegVmStateContext *vmState, unsigned functionId)
{
	CodeGenRegVmContext &ctx = *vmState->ctx;

	if(functionId == 0)
	{
		ctx.x86rvm->Stop("ERROR: invalid function pointer");
		longjmp(vmState->errorHandler, 1);
	}

	CallWrap(vmState, functionId);
}

#define nullcOffsetOf(obj, field) unsigned(uintptr_t(&obj->field) - uintptr_t(obj))

unsigned* GetCodeCmdCallPrologue(CodeGenRegVmContext &ctx, unsigned microcodePos)
{
	// Push arguments
	unsigned *microcode = ctx.exRegVmConstants + microcodePos;

#if defined(_M_X64)
	x86Reg rTempStack = rRBP;

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rTempStack, sQWORD, rR13, nullcOffsetOf(ctx.vmState, dataStackTop));

	unsigned tempStackPtrOffset = 0;

	while(*microcode != rvmiCall)
	{
		switch(*microcode++)
		{
		case rvmiPush:
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, *microcode++ * 8);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset, rEAX);
			tempStackPtrOffset += sizeof(int);
			break;
		case rvmiPushQword:
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, *microcode++ * 8);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rTempStack, tempStackPtrOffset, rRAX);
			tempStackPtrOffset += sizeof(long long);
			break;
		case rvmiPushImm:
			EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset, *microcode++);
			tempStackPtrOffset += sizeof(int);
			break;
		case rvmiPushImmq:
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEAX, *microcode++);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rTempStack, tempStackPtrOffset, rRAX);
			tempStackPtrOffset += sizeof(long long);
			break;
		case rvmiPushMem:
		{
			unsigned reg = *microcode++;
			unsigned offset = *microcode++;
			unsigned size = *microcode++;

			EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRSI, sQWORD, rREG, reg * 8);
			EMIT_OP_REG_NUM(ctx.ctx, o_add64, rRSI, offset);
			EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rRDI, sQWORD, rTempStack, tempStackPtrOffset);
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, size >> 2);
			EMIT_OP(ctx.ctx, o_rep_movsd);

			tempStackPtrOffset += size;
		}
		break;
		}
	}

	// Add call stack frame
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDX, sQWORD, rR13, nullcOffsetOf(ctx.vmState, callStackTop));
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rRDX, nullcOffsetOf(ctx.vmState->callStackTop, instruction), ctx.currInstructionPos + 1); // vmState->callStackTop->instruction = vmState->callInstructionPos + 1;
	EMIT_OP_RPTR_NUM(ctx.ctx, o_add64, sQWORD, rR13, nullcOffsetOf(ctx.vmState, callStackTop), sizeof(CodeGenRegVmCallStackEntry)); // vmState->callStackTop++;

	//vmState->regFileLastTop
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDX, sQWORD, rR13, nullcOffsetOf(ctx.vmState, regFileLastTop));

	//vmState->regFileLastTop[rvrrGlobals].ptrValue = uintptr_t(vmState->dataStackBase);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRBX, rvrrGlobals * 8);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rRDX, rvrrGlobals * 8, rRAX);

	//vmState->regFileLastTop[rvrrFrame].ptrValue = uintptr_t(vmState->dataStackTop);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rRDX, rvrrFrame * 8, rTempStack);

	//vmState->regFileLastTop[rvrrConstants].ptrValue = uintptr_t(ctx.exRegVmConstants);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rRDX, rvrrConstants * 8, rR14);

	//vmState->regFileLastTop[rvrrRegisters].ptrValue = uintptr_t(vmState->regFileLastTop);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rRDX, rvrrRegisters * 8, rRDX);

	//vmState->regFileLastPtr = vmState->regFileLastTop;
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rR13, nullcOffsetOf(ctx.vmState, regFileLastPtr), rRDX);
#else
	x86Reg rTempStack = rEDX;

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rTempStack, sDWORD, uintptr_t(&ctx.vmState->dataStackTop));

	unsigned tempStackPtrOffset = 0;

	while(*microcode != rvmiCall)
	{
		switch(*microcode++)
		{
		case rvmiPush:
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, *microcode++ * 8);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset, rEAX);
			tempStackPtrOffset += sizeof(int);
			break;
		case rvmiPushQword:
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, *microcode * 8);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset, rEAX);
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, *microcode++ * 8 + 4);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset + 4, rEAX);
			tempStackPtrOffset += sizeof(long long);
			break;
		case rvmiPushImm:
			EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset, *microcode++);
			tempStackPtrOffset += sizeof(int);
			break;
		case rvmiPushImmq:
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEAX, *microcode++);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset, rEAX);
			EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset + 4, 0);
			tempStackPtrOffset += sizeof(long long);
			break;
		case rvmiPushMem:
		{
			unsigned reg = *microcode++;
			unsigned offset = *microcode++;
			unsigned size = *microcode++;

			EMIT_OP_REG_REG(ctx.ctx, o_mov, rEDX, rESI); // Save frame register
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rESI, sDWORD, rREG, reg * 8);
			EMIT_OP_REG_NUM(ctx.ctx, o_add, rESI, offset);
			EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rEDI, sDWORD, rTempStack, tempStackPtrOffset);
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, size >> 2);
			EMIT_OP(ctx.ctx, o_rep_movsd);
			EMIT_OP_REG_REG(ctx.ctx, o_mov, rESI, rEDX); // Restore frame register
			EMIT_REG_READ(ctx.ctx, rESI);

			tempStackPtrOffset += size;
		}
		break;
		}
	}
#endif

	microcode++;

	return microcode;
}

void GetCodeCmdCallEpilogue(CodeGenRegVmContext &ctx, unsigned *microcode, unsigned char resultReg, unsigned char resultType)
{
#if defined(_M_X64)
	//vmState->regFileLastPtr = prevRegFilePtr;
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rR13, nullcOffsetOf(ctx.vmState, regFileLastPtr), rRBX);

	EMIT_OP_RPTR_NUM(ctx.ctx, o_sub64, sQWORD, rR13, nullcOffsetOf(ctx.vmState, callStackTop), sizeof(CodeGenRegVmCallStackEntry)); // vmState->callStackTop--;

	x86Reg rTempStack = rRBP;

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rTempStack, sQWORD, rR13, nullcOffsetOf(ctx.vmState, tempStackArrayBase));

	switch(resultType)
	{
	case rvrDouble:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rTempStack, 0);
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, resultReg * 8, rRAX);
		break;
	case rvrLong:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rTempStack, 0);
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, resultReg * 8, rRAX);
		break;
	case rvrInt:
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rTempStack, 0);
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, resultReg * 8, rEAX);
		break;
	default:
		break;
	}

	unsigned tempStackPtrOffset = 0;

	while(*microcode != rvmiReturn)
	{
		switch(*microcode++)
		{
		case rvmiPop:
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rTempStack, tempStackPtrOffset);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, *microcode++ * 8, rEAX);
			tempStackPtrOffset += sizeof(int);
			break;
		case rvmiPopq:
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rTempStack, tempStackPtrOffset);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, *microcode++ * 8, rRAX);
			tempStackPtrOffset += sizeof(long long);
			break;
		case rvmiPopMem:
		{
			unsigned reg = *microcode++;
			unsigned offset = *microcode++;
			unsigned size = *microcode++;

			EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rRSI, sQWORD, rTempStack, tempStackPtrOffset);
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDI, sQWORD, rREG, reg * 8);
			EMIT_OP_REG_NUM(ctx.ctx, o_add64, rRDI, offset);
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, size >> 2);
			EMIT_OP(ctx.ctx, o_rep_movsd);

			tempStackPtrOffset += size;
		}
		break;
		}
	}
#else
	switch(resultType)
	{
	case rvrDouble:
	case rvrLong:
		EMIT_OP_REG_ADDR(ctx.ctx, o_mov, rEAX, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase));
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, resultReg * 8, rEAX);
		EMIT_OP_REG_ADDR(ctx.ctx, o_mov, rEAX, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + 4);
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, resultReg * 8 + 4, rEAX);
		break;
	case rvrInt:
		EMIT_OP_REG_ADDR(ctx.ctx, o_mov, rEAX, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase));
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, resultReg * 8, rEAX);
		break;
	default:
		break;
	}

	unsigned tempStackPtrOffset = 0;

	while(*microcode != rvmiReturn)
	{
		switch(*microcode++)
		{
		case rvmiPop:
			EMIT_OP_REG_ADDR(ctx.ctx, o_mov, rEAX, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, *microcode++ * 8, rEAX);
			tempStackPtrOffset += sizeof(int);
			break;
		case rvmiPopq:
			EMIT_OP_REG_ADDR(ctx.ctx, o_mov, rEAX, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, *microcode * 8, rEAX);
			EMIT_OP_REG_ADDR(ctx.ctx, o_mov, rEAX, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset + 4);
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, *microcode++ * 8 + 4, rEAX);
			tempStackPtrOffset += sizeof(long long);
			break;
		case rvmiPopMem:
		{
			unsigned reg = *microcode++;
			unsigned offset = *microcode++;
			unsigned size = *microcode++;

			EMIT_OP_REG_REG(ctx.ctx, o_mov, rEDX, rESI); // Save frame register
			EMIT_OP_REG_ADDR(ctx.ctx, o_lea, rESI, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset);
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, reg * 8);
			EMIT_OP_REG_NUM(ctx.ctx, o_add, rEDI, offset);
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, size >> 2);
			EMIT_OP(ctx.ctx, o_rep_movsd);
			EMIT_OP_REG_REG(ctx.ctx, o_mov, rESI, rEDX); // Restore frame register
			EMIT_REG_READ(ctx.ctx, rESI);

			tempStackPtrOffset += size;
		}
		break;
		}
	}
#endif
}

void GenCodeCmdCall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->callWrap = CallWrap;

	unsigned *microcode = GetCodeCmdCallPrologue(ctx, (cmd.rA << 16) | (cmd.rB << 8) | cmd.rC);

	unsigned char resultReg = *microcode++ & 0xff;
	unsigned char resultType = *microcode++ & 0xff;

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rR13, nullcOffsetOf(ctx.vmState, functionAddress));
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRAX, cmd.argument * sizeof(void*));
	EMIT_OP_REG_NUM(ctx.ctx, o_cmp64, rRAX, 0);

	EMIT_OP_LABEL(ctx.ctx, o_je, ctx.labelCount, false);

	EMIT_OP_REG(ctx.ctx, o_call, rRAX);
	EMIT_OP_LABEL(ctx.ctx, o_jmp, ctx.labelCount + 1, false);

	EMIT_LABEL(ctx.ctx, ctx.labelCount, false);
	ctx.labelCount++;

	EMIT_OP_REG_REG(ctx.ctx, o_mov64, rArg1, rR13);
	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rArg2, cmd.argument);
	EMIT_REG_READ(ctx.ctx, rArg1);
	EMIT_REG_READ(ctx.ctx, rArg2);
	EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, nullcOffsetOf(ctx.vmState, callWrap));

	EMIT_LABEL(ctx.ctx, ctx.labelCount, false);
	ctx.labelCount++;
#else
	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEDI, uintptr_t(ctx.vmState));

	// Add call stack frame
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rEDI, nullcOffsetOf(ctx.vmState, callStackTop));
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rRDX, nullcOffsetOf(ctx.vmState->callStackTop, instruction), ctx.currInstructionPos + 1); // vmState->callStackTop->instruction = vmState->callInstructionPos + 1;
	EMIT_OP_RPTR_NUM(ctx.ctx, o_add, sDWORD, rEDI, nullcOffsetOf(ctx.vmState, callStackTop), sizeof(CodeGenRegVmCallStackEntry)); // vmState->callStackTop++;

	assert(!"setup registers");

	EMIT_OP_NUM(ctx.ctx, o_push, cmd.argument);
	EMIT_OP_REG(ctx.ctx, o_push, rEDI);
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->callWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 8);

	// Remove call stack frame
	EMIT_OP_RPTR_NUM(ctx.ctx, o_sub, sDWORD, rEDI, nullcOffsetOf(ctx.vmState, callStackTop), sizeof(CodeGenRegVmCallStackEntry)); // vmState->callStackTop++;
#endif

	GetCodeCmdCallEpilogue(ctx, microcode, resultReg, resultType);
}

void GenCodeCmdCallPtr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->callPtrWrap = CallPtrWrap;

	unsigned *microcode = GetCodeCmdCallPrologue(ctx, cmd.argument);

	unsigned char resultReg = *microcode++ & 0xff;
	unsigned char resultType = *microcode++ & 0xff;

#if defined(_M_X64)
	EMIT_OP_REG_REG(ctx.ctx, o_mov64, rArg1, rR13);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rArg2, sDWORD, rREG, cmd.rC * 8); // Get function id
	EMIT_REG_READ(ctx.ctx, rArg1);
	EMIT_REG_READ(ctx.ctx, rArg2);
	EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->callPtrWrap) - uintptr_t(ctx.vmState)));
#else
	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEDI, uintptr_t(ctx.vmState));

	// Add call stack frame
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rEDI, nullcOffsetOf(ctx.vmState, callStackTop));
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rRDX, nullcOffsetOf(ctx.vmState->callStackTop, instruction), ctx.currInstructionPos + 1); // vmState->callStackTop->instruction = vmState->callInstructionPos + 1;
	EMIT_OP_RPTR_NUM(ctx.ctx, o_add, sDWORD, rEDI, nullcOffsetOf(ctx.vmState, callStackTop), sizeof(CodeGenRegVmCallStackEntry)); // vmState->callStackTop++;

	assert(!"setup registers");

	EMIT_OP_RPTR(ctx.ctx, o_push, sDWORD, rREG, cmd.rC * 8); // Get function id
	EMIT_OP_REG(ctx.ctx, o_push, rEDI);
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->callPtrWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 8);

	// Remove call stack frame
	EMIT_OP_RPTR_NUM(ctx.ctx, o_sub, sDWORD, rEDI, nullcOffsetOf(ctx.vmState, callStackTop), sizeof(CodeGenRegVmCallStackEntry)); // vmState->callStackTop++;
#endif

	GetCodeCmdCallEpilogue(ctx, microcode, resultReg, resultType);
}

void CheckedReturnWrap(CodeGenRegVmStateContext *vmState, unsigned microcodePos)
{
	CodeGenRegVmContext &ctx = *vmState->ctx;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	uintptr_t frameBase = regFilePtr[rvrrFrame].ptrValue;
	uintptr_t frameEnd = uintptr_t(vmState->dataStackEnd);

	char *returnValuePtr = (char*)vmState->tempStackArrayBase;

	void *ptr;
	memcpy(&ptr, returnValuePtr, sizeof(ptr));

	if(uintptr_t(ptr) >= frameBase && uintptr_t(ptr) <= frameEnd)
	{
		unsigned *microcode = ctx.exRegVmConstants + microcodePos;
		unsigned typeId = *microcode;

		ExternTypeInfo &type = ctx.exTypes[typeId];

		if(type.arrSize == ~0u)
		{
			unsigned length = *(int*)(returnValuePtr + sizeof(void*));

			char *copy = (char*)NULLC::AllocObject(ctx.exTypes[type.subType].size * length);
			memcpy(copy, ptr, unsigned(ctx.exTypes[type.subType].size * length));
			memcpy(returnValuePtr, &copy, sizeof(copy));
		}
		else
		{
			unsigned objSize = type.size;

			char *copy = (char*)NULLC::AllocObject(objSize);
			memcpy(copy, ptr, objSize);
			memcpy(returnValuePtr, &copy, sizeof(copy));
		}
	}
}

void ErrorNoReturnWrap(CodeGenRegVmStateContext *vmState)
{
	CodeGenRegVmContext &ctx = *vmState->ctx;

	vmState->callStackTop->instruction = vmState->callInstructionPos + 1;
	vmState->callStackTop++;

	ctx.x86rvm->Stop("ERROR: function didn't return a value");
	longjmp(vmState->errorHandler, 1);
}

void GenCodeCmdReturn(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->checkedReturnWrap = CheckedReturnWrap;
	ctx.vmState->errorNoReturnWrap = ErrorNoReturnWrap;

#if defined(_M_X64)
	if(cmd.rB == rvrError)
	{
		EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg1, uintptr_t(ctx.vmState));
		EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->callInstructionPos) - uintptr_t(ctx.vmState)), ctx.currInstructionPos);
		EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->errorNoReturnWrap) - uintptr_t(ctx.vmState)));

		return;
	}

	if(unsigned functionId = ctx.currFunctionId)
	{
		ExternFuncInfo &target = ctx.exFunctions[functionId];

		unsigned stackSize = (target.stackSize + 0xf) & ~0xf;

		// Restore frame top
		EMIT_OP_RPTR_NUM(ctx.ctx, o_sub64, sQWORD, rR13, nullcOffsetOf(ctx.vmState, dataStackTop), stackSize); // vmState->dataStackTop -= stackSize;

		// Restore register top
		EMIT_OP_RPTR_NUM(ctx.ctx, o_sub64, sQWORD, rR13, nullcOffsetOf(ctx.vmState, regFileLastTop), target.regVmRegisters * 8); // vmState->regFileLastTop -= target.regVmRegisters;
	}

	if(cmd.rB != rvrVoid)
	{
		unsigned *microcode = ctx.exRegVmConstants + cmd.argument;

		// Skip type id and type size
		microcode++;
		microcode++;

		x86Reg rTempStack = rRBP;

		if(*microcode != rvmiReturn)
			EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rTempStack, (uintptr_t)ctx.vmState->tempStackArrayBase);

		unsigned tempStackPtrOffset = 0;

		while(*microcode != rvmiReturn)
		{
			switch(*microcode++)
			{
			case rvmiPush:
				EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, *microcode++ * 8);
				EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset, rEAX);
				tempStackPtrOffset += sizeof(int);
				break;
			case rvmiPushQword:
				EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, *microcode++ * 8);
				EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rTempStack, tempStackPtrOffset, rRAX);
				tempStackPtrOffset += sizeof(long long);
				break;
			case rvmiPushImm:
				EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rTempStack, tempStackPtrOffset, *microcode++);
				tempStackPtrOffset += sizeof(int);
				break;
			case rvmiPushImmq:
				EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEAX, *microcode++);
				EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rTempStack, tempStackPtrOffset, rRAX);
				tempStackPtrOffset += sizeof(long long);
				break;
			case rvmiPushMem:
			{
				unsigned reg = *microcode++;
				unsigned offset = *microcode++;
				unsigned size = *microcode++;

				EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRSI, sQWORD, rREG, reg * 8);
				EMIT_OP_REG_NUM(ctx.ctx, o_add64, rRSI, offset);
				EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rRDI, sQWORD, rTempStack, tempStackPtrOffset);
				EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, size >> 2);
				EMIT_OP(ctx.ctx, o_rep_movsd);

				tempStackPtrOffset += size;
			}
			break;
			}
		}

		// Checked return value
		if(cmd.rC)
		{
			EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg1, uintptr_t(ctx.vmState));
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, rArg2, cmd.argument);
			EMIT_REG_READ(ctx.ctx, rArg2);
			EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->checkedReturnWrap) - uintptr_t(ctx.vmState)));
		}
	}

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);
	ctx.ctx.KillLateUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEAX, cmd.rB);
	EMIT_OP_REG_NUM(ctx.ctx, o_add64, rRSP, 40);
	EMIT_OP_REG(ctx.ctx, o_pop, rR15);
	EMIT_OP_REG(ctx.ctx, o_pop, rRBX);
	EMIT_OP(ctx.ctx, o_ret);
#else
	if(cmd.rB == rvrError)
	{
		EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, uintptr_t(&ctx.vmState->callInstructionPos), ctx.currInstructionPos);
		EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
		EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->errorNoReturnWrap));
		return;
	}

	if(unsigned functionId = ctx.currFunctionId)
	{
		ExternFuncInfo &target = ctx.exFunctions[functionId];

		unsigned stackSize = (target.stackSize + 0xf) & ~0xf;

		assert(!"not implemented"); // vmState->dataStackTop -= stackSize;
	}

	if(cmd.rB != rvrVoid)
	{
		unsigned *microcode = ctx.exRegVmConstants + cmd.argument;

		unsigned typeId = *microcode++;
		unsigned typeSize = *microcode++;

		(void)typeId;
		(void)typeSize;

		unsigned tempStackPtrOffset = 0;

		while(*microcode != rvmiReturn)
		{
			switch(*microcode++)
			{
			case rvmiPush:
				EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, *microcode++ * 8);
				EMIT_OP_ADDR_REG(ctx.ctx, o_mov, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset, rEAX);
				tempStackPtrOffset += sizeof(int);
				break;
			case rvmiPushQword:
				EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, *microcode * 8);
				EMIT_OP_ADDR_REG(ctx.ctx, o_mov, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset, rEAX);
				EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, *microcode++ * 8 + 4);
				EMIT_OP_ADDR_REG(ctx.ctx, o_mov, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset + 4, rEAX);
				tempStackPtrOffset += sizeof(long long);
				break;
			case rvmiPushImm:
				EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset, *microcode++);
				tempStackPtrOffset += sizeof(int);
				break;
			case rvmiPushImmq:
				EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEAX, *microcode++);
				EMIT_OP_ADDR_REG(ctx.ctx, o_mov, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset, rEAX);
				EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset + 4, 0);
				tempStackPtrOffset += sizeof(long long);
				break;
			case rvmiPushMem:
			{
				unsigned reg = *microcode++;
				unsigned offset = *microcode++;
				unsigned size = *microcode++;

				EMIT_OP_REG_REG(ctx.ctx, o_mov, rEDX, rESI); // Save frame register
				EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rESI, sDWORD, rREG, reg * 8);
				EMIT_OP_REG_NUM(ctx.ctx, o_add, rESI, offset);
				EMIT_OP_REG_ADDR(ctx.ctx, o_lea, rEDI, sDWORD, uintptr_t(ctx.vmState->tempStackArrayBase) + tempStackPtrOffset);
				EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, size >> 2);
				EMIT_OP(ctx.ctx, o_rep_movsd);
				EMIT_OP_REG_REG(ctx.ctx, o_mov, rESI, rEDX); // Restore frame register
				EMIT_REG_READ(ctx.ctx, rESI);

				tempStackPtrOffset += size;
			}
			break;
			}
		}

		// Checked return value
		if(cmd.rC)
		{
			EMIT_OP_NUM(ctx.ctx, o_push, cmd.argument);
			EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
			EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->checkedReturnWrap));
			EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 8);
		}
	}

	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEAX, cmd.rB);
	EMIT_OP(ctx.ctx, o_ret);
#endif
}

void GenCodeCmdAddImm(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	if(cmd.rA == cmd.rB)
	{
		x86Reg sourceReg = ctx.ctx.FindRegAtMemory(sQWORD, rNONE, 1, rREG, cmd.rA * 8, true);

		if(sourceReg == rRegCount)
		{
			EMIT_OP_RPTR_NUM(ctx.ctx, o_add, sDWORD, rREG, cmd.rA * 8, cmd.argument); // Modify inplace
		}
		else
		{
			ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

			EMIT_OP_REG_NUM(ctx.ctx, o_add, sourceReg, cmd.argument); // Modify inplace
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, sourceReg); // Store int to target
		}
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load value

		ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

		EMIT_OP_REG_NUM(ctx.ctx, o_add, rEAX, cmd.argument);
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store to target
	}
}

void GenCodeCmdAdd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86Reg rhsReg = GenCodeLoadInt32FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_add, lhsReg, rhsReg);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_add, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdSub(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86Reg rhsReg = GenCodeLoadInt32FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_sub, lhsReg, rhsReg);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_sub, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdMul(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86Reg rhsReg = GenCodeLoadInt32FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_imul, lhsReg, rhsReg);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_imul, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdDiv(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cdq);

	GenCodeLoadInt32FromPointer(ctx, rRCX, rECX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG(ctx.ctx, o_idiv, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cdq);

	x86GenCodeLoadInt32FromPointer(ctx, rECX, rECX, cmd.rC, cmd.argument);

	EMIT_OP_REG(ctx.ctx, o_idiv, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void x64PowWrap(CodeGenRegVmStateContext *vmState, uintptr_t cmdValue)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].intValue = VmIntPow(*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument), regFilePtr[cmd.rB].intValue);

	vmState->instWrapperActive = false;
}

void x86PowWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].intValue = VmIntPow(*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument), regFilePtr[cmd.rB].intValue);

	vmState->instWrapperActive = false;
}

void GenCodeCmdPow(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x64PowWrap = x64PowWrap;
	ctx.vmState->x86PowWrap = x86PowWrap;

#if defined(_M_X64)
	uintptr_t cmdValue;
	memcpy(&cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg1, uintptr_t(ctx.vmState));
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg2, cmdValue);
	EMIT_REG_READ(ctx.ctx, rArg2);
	EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->x64PowWrap) - uintptr_t(ctx.vmState)));
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86PowWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void GenCodeCmdMod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cdq);

	GenCodeLoadInt32FromPointer(ctx, rRCX, rECX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG(ctx.ctx, o_idiv, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEDX); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cdq);

	x86GenCodeLoadInt32FromPointer(ctx, rECX, rECX, cmd.rC, cmd.argument);

	EMIT_OP_REG(ctx.ctx, o_idiv, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEDX); // Store int to target
#endif
}

void GenCodeCmdLess(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setl, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setl, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#endif
}

void GenCodeCmdGreater(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setg, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setg, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#endif
}

void GenCodeCmdLequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setle, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setle, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#endif
}

void GenCodeCmdGequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setge, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setge, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#endif
}

void GenCodeCmdEqual(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#endif
}

void GenCodeCmdNequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setne, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setne, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#endif
}

void GenCodeCmdShl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRCX, rECX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_sal, lhsReg, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rECX, rECX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_sal, rEAX, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdShr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRCX, rECX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_sar, lhsReg, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rECX, rECX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_sar, rEAX, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdBitAnd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86Reg rhsReg = GenCodeLoadInt32FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_and, lhsReg, rhsReg);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_and, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdBitOr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86Reg rhsReg = GenCodeLoadInt32FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_or, lhsReg, rhsReg);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_or, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdBitXor(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86Reg rhsReg = GenCodeLoadInt32FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, lhsReg, rhsReg);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	x86GenCodeLoadInt32FromPointer(ctx, rEDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdAddImml(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	if(cmd.rA == cmd.rB)
	{
		x86Reg sourceReg = ctx.ctx.FindRegAtMemory(sQWORD, rNONE, 1, rREG, cmd.rA * 8, true);

		if(sourceReg == rRegCount)
		{
			EMIT_OP_RPTR_NUM(ctx.ctx, o_add64, sQWORD, rREG, cmd.rA * 8, cmd.argument); // Modify inplace
		}
		else
		{
			ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

			EMIT_OP_REG_NUM(ctx.ctx, o_add64, sourceReg, cmd.argument); // Modify inplace
			EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, sourceReg); // Store int to target
		}
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs

		ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

		EMIT_OP_REG_NUM(ctx.ctx, o_add64, rEAX, cmd.argument);
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
	}
#else
	// Load long LHS value into ECX:EDI
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rB * 8 + 4);

	if(int(cmd.argument) < 0)
	{
		EMIT_OP_REG_NUM(ctx.ctx, o_sub, rEAX, -int(cmd.argument));
		EMIT_REG_READ(ctx.ctx, rEAX); // sbb implicitly reads the result of the last value-producing instruction
		EMIT_OP_REG_NUM(ctx.ctx, o_sbb, rEDX, 0);
	}
	else
	{
		EMIT_OP_REG_NUM(ctx.ctx, o_add, rEAX, cmd.argument);
		EMIT_REG_READ(ctx.ctx, rEAX); // adc implicitly reads the result of the last value-producing instruction
		EMIT_OP_REG_NUM(ctx.ctx, o_adc, rEDX, 0);
	}

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store long to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX);
#endif
}

void GenCodeCmdAddl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, lhsReg, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	x86Reg target = GenCodeLoadInt64FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_add64, lhsReg, target);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, lhsReg); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rEDX, rEAX, rEDX, cmd.rC, cmd.argument);

	// Load long LHS value into ECX:EDI
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rECX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rB * 8 + 4);

	EMIT_OP_REG_REG(ctx.ctx, o_add, rEAX, rECX);
	EMIT_REG_READ(ctx.ctx, rEAX); // adc implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG_REG(ctx.ctx, o_adc, rEDX, rEDI);

	// Send long in EAX:EDX
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store long to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX);
#endif
}

void GenCodeCmdSubl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, lhsReg, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	x86Reg target = GenCodeLoadInt64FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_sub64, lhsReg, target);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, lhsReg); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rEDX, rEAX, rEDX, cmd.rC, cmd.argument);

	// Load long LHS value into ECX:EDI
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rECX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rB * 8 + 4);

	EMIT_OP_REG_REG(ctx.ctx, o_sub, rECX, rEAX);
	EMIT_REG_READ(ctx.ctx, rECX); // sbb implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG_REG(ctx.ctx, o_sbb, rEDI, rEDX);

	// Send long in EAX:EDX
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store long to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDI);
#endif
}

void x86MullWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue * *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);

	vmState->instWrapperActive = false;
}

void GenCodeCmdMull(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x86MullWrap = x86MullWrap;

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, lhsReg, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	x86Reg target = GenCodeLoadInt64FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_imul64, lhsReg, target);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, lhsReg); // Store long to target
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86MullWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void x86DivlWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue / *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);

	vmState->instWrapperActive = false;
}

void GenCodeCmdDivl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x86DivlWrap = x86DivlWrap;

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cqo);

	GenCodeLoadInt64FromPointer(ctx, rRCX, rRCX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG(ctx.ctx, o_idiv64, rRCX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long to target
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86DivlWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void x64PowlWrap(CodeGenRegVmStateContext *vmState, uintptr_t cmdValue)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].longValue = VmLongPow(*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument), regFilePtr[cmd.rB].longValue);

	vmState->instWrapperActive = false;
}

void x86PowlWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].longValue = VmLongPow(*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument), regFilePtr[cmd.rB].longValue);

	vmState->instWrapperActive = false;
}

void GenCodeCmdPowl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x64PowlWrap = x64PowlWrap;
	ctx.vmState->x86PowlWrap = x86PowlWrap;

#if defined(_M_X64)
	uintptr_t cmdValue;
	memcpy(&cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg1, uintptr_t(ctx.vmState));
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg2, cmdValue);
	EMIT_REG_READ(ctx.ctx, rArg2);
	EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->x64PowlWrap) - uintptr_t(ctx.vmState)));
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86PowlWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void x86ModlWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue % *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);

	vmState->instWrapperActive = false;
}

void GenCodeCmdModl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x86ModlWrap = x86ModlWrap;

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cqo);

	GenCodeLoadInt64FromPointer(ctx, rRCX, rRCX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG(ctx.ctx, o_idiv64, rRCX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRDX); // Store long to target
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86ModlWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void GenCodeCmdLessl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setl, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rEDI, rEDX, rEDI, cmd.rC, cmd.argument);

	// Load long LHS value into ECX:EDI
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rECX, sDWORD, rREG, cmd.rB * 8 + 4);

	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG_REG(ctx.ctx, o_sbb, rECX, rEDI);
	EMIT_REG_READ(ctx.ctx, rECX); // setcc implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG(ctx.ctx, o_setl, rEAX);

	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 1);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdGreaterl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setg, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rECX, rEAX, rECX, cmd.rC, cmd.argument);

	// Load long LHS value into ECX:EDI
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rB * 8 + 4);

	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG_REG(ctx.ctx, o_sbb, rECX, rEDI);
	EMIT_REG_READ(ctx.ctx, rECX); // setcc implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG(ctx.ctx, o_setl, rEAX);

	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 1);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdLequall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setle, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rECX, rEAX, rECX, cmd.rC, cmd.argument);

	// Load long LHS value into ECX:EDI
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDI, sDWORD, rREG, cmd.rB * 8 + 4);

	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG_REG(ctx.ctx, o_sbb, rECX, rEDI);
	EMIT_REG_READ(ctx.ctx, rECX); // setcc implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG(ctx.ctx, o_setge, rEAX);

	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 1);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdGequall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setge, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rEDI, rEDX, rEDI, cmd.rC, cmd.argument);

	// Load long LHS value into ECX:EDI
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rECX, sDWORD, rREG, cmd.rB * 8 + 4);

	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG_REG(ctx.ctx, o_sbb, rECX, rEDI);
	EMIT_REG_READ(ctx.ctx, rECX); // setcc implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG(ctx.ctx, o_setge, rEAX);

	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 1);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdEquall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rEDX, rECX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rEAX, rEAX);
	EMIT_OP_REG_RPTR(ctx.ctx, o_xor, rECX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_xor, rEDX, sDWORD, rREG, cmd.rB * 8 + 4);
	EMIT_OP_REG_REG(ctx.ctx, o_or, rECX, rEDX);
	EMIT_REG_READ(ctx.ctx, rECX); // setcc implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG(ctx.ctx, o_sete, rEAX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdNequall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setne, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rEDX, rECX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rEAX, rEAX);
	EMIT_OP_REG_RPTR(ctx.ctx, o_xor, rECX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_xor, rEDX, sDWORD, rREG, cmd.rB * 8 + 4);
	EMIT_OP_REG_REG(ctx.ctx, o_or, rECX, rEDX);
	EMIT_REG_READ(ctx.ctx, rECX); // setcc implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG(ctx.ctx, o_setne, rEAX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void x86ShllWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue << *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);

	vmState->instWrapperActive = false;
}

void GenCodeCmdShll(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x86ShllWrap = x86ShllWrap;

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rECX, cmd.rC, cmd.argument); // Load lower int bits ot the rhs value

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_sal64, rRAX, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long to target
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86ShllWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void x86ShrlWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].longValue = regFilePtr[cmd.rB].longValue >> *(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument);

	vmState->instWrapperActive = false;
}

void GenCodeCmdShrl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x86ShrlWrap = x86ShrlWrap;

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rECX, cmd.rC, cmd.argument); // Load lower int bits ot the rhs value

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_sar64, rRAX, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long to target
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86ShrlWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void GenCodeCmdBitAndl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, lhsReg, sQWORD, rREG, cmd.rB * 8); // Load lhs

	x86Reg target = GenCodeLoadInt64FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_and64, lhsReg, target);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, lhsReg); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rEDX, rEAX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_RPTR(ctx.ctx, o_and, rEAX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_and, rEDX, sDWORD, rREG, cmd.rB * 8 + 4);

	// Store long in EAX:EDX to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX);
#endif
}

void GenCodeCmdBitOrl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, lhsReg, sQWORD, rREG, cmd.rB * 8); // Load lhs

	x86Reg target = GenCodeLoadInt64FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_or64, lhsReg, target);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, lhsReg); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rEDX, rEAX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_RPTR(ctx.ctx, o_or, rEAX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_or, rEDX, sDWORD, rREG, cmd.rB * 8 + 4);

	// Store long in EAX:EDX to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX);
#endif
}

void GenCodeCmdBitXorl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, lhsReg, sQWORD, rREG, cmd.rB * 8); // Load lhs

	x86Reg target = GenCodeLoadInt64FromPointerIntoRegister(ctx, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor64, lhsReg, target);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, lhsReg); // Store long to target
#else
	// Load long RHS value into EAX:EDX
	x86GenCodeLoadInt64FromPointer(ctx, rEDX, rEAX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_RPTR(ctx.ctx, o_xor, rEAX, sDWORD, rREG, cmd.rB * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_xor, rEDX, sDWORD, rREG, cmd.rB * 8 + 4);

	// Store long in EAX:EDX to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX);
#endif
}

void GenCodeCmdAddd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_addsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_addsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#endif
}

void GenCodeCmdSubd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_subsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_subsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#endif
}

void GenCodeCmdMuld(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_mulsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_mulsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#endif
}

void GenCodeCmdDivd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_divsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_divsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#endif
}

void GenCodeCmdAddf(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadFloatFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_addsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadFloatFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_addsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#endif
}

void GenCodeCmdSubf(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadFloatFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_subsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadFloatFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_subsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#endif
}

void GenCodeCmdMulf(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadFloatFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_mulsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadFloatFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_mulsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#endif
}

void GenCodeCmdDivf(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadFloatFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_divsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadFloatFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_divsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#endif
}

void x64PowdWrap(CodeGenRegVmStateContext *vmState, uintptr_t cmdValue)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].doubleValue = pow(regFilePtr[cmd.rB].doubleValue, *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));

	vmState->instWrapperActive = false;
}

void x86PowdWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].doubleValue = pow(regFilePtr[cmd.rB].doubleValue, *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));

	vmState->instWrapperActive = false;
}

void GenCodeCmdPowd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x64PowdWrap = x64PowdWrap;
	ctx.vmState->x86PowdWrap = x86PowdWrap;

#if defined(_M_X64)
	uintptr_t cmdValue;
	memcpy(&cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg1, uintptr_t(ctx.vmState));
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg2, cmdValue);
	EMIT_REG_READ(ctx.ctx, rArg2);
	EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->x64PowdWrap) - uintptr_t(ctx.vmState)));
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86PowdWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void x64ModdWrap(CodeGenRegVmStateContext *vmState, uintptr_t cmdValue)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].doubleValue = fmod(regFilePtr[cmd.rB].doubleValue, *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));

	vmState->instWrapperActive = false;
}

void x86ModdWrap(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB)
{
	vmState->instWrapperActive = true;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned cmdValue[2] = { cmdValueA, cmdValueB };
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].doubleValue = fmod(regFilePtr[cmd.rB].doubleValue, *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));

	vmState->instWrapperActive = false;
}

void GenCodeCmdModd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->x64ModdWrap = x64ModdWrap;
	ctx.vmState->x86ModdWrap = x86ModdWrap;

#if defined(_M_X64)
	uintptr_t cmdValue;
	memcpy(&cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg1, uintptr_t(ctx.vmState));
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg2, cmdValue);
	EMIT_REG_READ(ctx.ctx, rArg2);
	EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->x64ModdWrap) - uintptr_t(ctx.vmState)));
#else
	unsigned cmdValue[2];
	memcpy(cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[1]);
	EMIT_OP_NUM(ctx.ctx, o_push, cmdValue[0]);
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->x86ModdWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);
#endif
}

void GenCodeCmdLessd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpltsd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpltsd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdGreaterd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpltsd, rhs, lhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, rhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpltsd, rhs, lhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, rhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdLequald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_cmplesd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmplesd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdGequald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_cmplesd, rhs, lhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, rhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmplesd, rhs, lhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, rhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdEquald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpeqsd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpeqsd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdNequald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86XmmReg rhs = GenCodeLoadDoubleFromPointerIntoRegister(ctx, rRAX, cmd.rC, cmd.argument);

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpneqsd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	x86GenCodeLoadDoubleFromPointer(ctx, rEAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpneqsd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdNeg(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rC * 8); // Load int value

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG(ctx.ctx, o_neg, lhsReg);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load int value
	EMIT_OP_REG(ctx.ctx, o_neg, rEAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdNegl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, lhsReg, sQWORD, rREG, cmd.rC * 8); // Load long value

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG(ctx.ctx, o_neg64, lhsReg);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, lhsReg); // Store long to target
#else
	// Load long value into EAX:EDX
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rC * 8 + 4);

	// Load 'zero' into ECX:EDI
	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_xor, rEDI, rEDI);

	EMIT_OP_REG_REG(ctx.ctx, o_sub, rECX, rEAX);
	EMIT_REG_READ(ctx.ctx, rECX); // sbb implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG_REG(ctx.ctx, o_sbb, rEDI, rEDX);

	// Store long in ECX:EDI to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDI);
#endif
}

void GenCodeCmdNegd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load double as a long bit pattern

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRDX, 0x8000000000000000ull);
	EMIT_OP_REG_REG(ctx.ctx, o_xor64, rRAX, rRDX); // Switch top bit
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long bit pattern to target double
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load double as a long bit patter
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rC * 8 + 4);

	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rECX, 0x80000000u);
	EMIT_OP_REG_REG(ctx.ctx, o_xor, rEDX, rECX); // Switch top bit

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store long bit pattern to target double
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX);
#endif
}

void GenCodeCmdBitNot(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, lhsReg, sDWORD, rREG, cmd.rC * 8); // Load int value

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG(ctx.ctx, o_not, lhsReg);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load int value
	EMIT_OP_REG(ctx.ctx, o_not, rEAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#endif
}

void GenCodeCmdBitNotl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86Reg lhsReg = ctx.ctx.GetReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, lhsReg, sQWORD, rREG, cmd.rC * 8); // Load int value

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG(ctx.ctx, o_not64, lhsReg);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, lhsReg); // Store int to target
#else
	// Load long value into EAX:EDX
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rC * 8 + 4);

	EMIT_OP_REG(ctx.ctx, o_not, rEAX);
	EMIT_OP_REG(ctx.ctx, o_not, rEDX);

	// Store long in EAX:EDX to target
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8 + 4, rEDX);
#endif
}

void GenCodeCmdLogNot(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load int value

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rECX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
}

void GenCodeCmdLogNotl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load long value

	ctx.ctx.KillEarlyUnreadRegVmRegisters(ctx.exRegVmRegKillInfo + ctx.currInstructionRegKillOffset);

	EMIT_OP_REG_REG(ctx.ctx, o_xor64, rRCX, rRCX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rRAX, rRCX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	// Load low part
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rRCX, rRCX);
	EMIT_OP_REG_RPTR(ctx.ctx, o_or, rEAX, sDWORD, rREG, cmd.rC * 8 + 4); // Or with high part
	EMIT_REG_READ(ctx.ctx, rEAX); // setcc implicitly reads the result of the last value-producing instruction
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#endif
}

void ConvertPtrWrap(CodeGenRegVmStateContext *vmState, unsigned targetTypeId, unsigned sourceTypeId)
{
	CodeGenRegVmContext &ctx = *vmState->ctx;

	if(!ConvertFromAutoRef(targetTypeId, sourceTypeId))
	{
		vmState->callStackTop->instruction = vmState->callInstructionPos + 1;
		vmState->callStackTop++;

		char execError[REGVM_X86_ERROR_BUFFER_SIZE];
		NULLC::SafeSprintf(execError, 1024, "ERROR: cannot convert from %s ref to %s ref", &ctx.exSymbols[ctx.exTypes[sourceTypeId].offsetToName], &ctx.exSymbols[ctx.exTypes[targetTypeId].offsetToName]);

		ctx.x86rvm->Stop(execError);
		longjmp(vmState->errorHandler, 1);
	}
}

void GenCodeCmdConvertPtr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->convertPtrWrap = ConvertPtrWrap;

#if defined(_M_X64)
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rArg1, uintptr_t(ctx.vmState));
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->callInstructionPos) - uintptr_t(ctx.vmState)), ctx.currInstructionPos);
	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rArg2, cmd.argument);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rArg3, sDWORD, rREG, cmd.rB * 8); // Load typeid
	EMIT_REG_READ(ctx.ctx, rArg2);
	EMIT_REG_READ(ctx.ctx, rArg3);
	EMIT_OP_RPTR(ctx.ctx, o_call, sQWORD, rArg1, unsigned(uintptr_t(&ctx.vmState->convertPtrWrap) - uintptr_t(ctx.vmState)));

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Get source pointer
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Move to target
#else
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, uintptr_t(&ctx.vmState->callInstructionPos), ctx.currInstructionPos);
	EMIT_OP_RPTR(ctx.ctx, o_push, sDWORD, rREG, cmd.rB * 8); // Source typeid
	EMIT_OP_NUM(ctx.ctx, o_push, cmd.argument); // Target typeid
	EMIT_OP_NUM(ctx.ctx, o_push, uintptr_t(ctx.vmState));
	EMIT_OP_ADDR(ctx.ctx, o_call, sDWORD, uintptr_t(&ctx.vmState->convertPtrWrap));
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rESP, 12);

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Get source pointer
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Move to target
#endif
}
