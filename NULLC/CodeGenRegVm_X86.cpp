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

// TODO: special handling for rvrrGlobals

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
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sBYTE, rR15, offset, sourceReg); // Store int value
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
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sWORD, rR15, offset, sourceReg); // Store int value
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

		if (value == 0)
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

void GenCodeStoreInt64ToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86Reg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rR15, offset, sourceReg); // Store int value
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
		EMIT_OP_RPTR_REG(ctx.ctx, o_movss, sDWORD, rR15, offset, sourceReg); // Store int value
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

void GenCodeStoreDoubleToPointer(CodeGenRegVmContext &ctx, x86Reg tempReg, x86XmmReg sourceReg, unsigned char reg, unsigned offset)
{
	if(reg == rvrrFrame)
	{
		EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rR15, offset, sourceReg); // Store int value
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, tempReg, sQWORD, rREG, reg * 8); // Load target pointer
		EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, tempReg, offset, sourceReg); // Store value to target with an offset
	}
}

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
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadWord(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	GenCodeLoadInt16FromPointer(ctx, rRAX, rEAX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadDword(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	GenCodeLoadInt32FromPointer(ctx, rRAX, rEAX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadLong(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	GenCodeLoadInt64FromPointer(ctx, rRAX, rRAX, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long to target
#else
	assert(!"not implemented");
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
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg target = ctx.ctx.GetXmmReg();

	GenCodeLoadDoubleFromPointer(ctx, rRAX, target, cmd.rC, cmd.argument);

	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, target); // Store double to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadImm(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, cmd.argument); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadImmLong(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRAX, ((uint64_t)cmd.argument << 32ull));
	EMIT_OP_REG_REG(ctx.ctx, o_xor64, rRDX, rRDX);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load int value
	EMIT_OP_REG_REG(ctx.ctx, o_or64, rRAX, rRDX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long to target
#else
	assert(!"not implemented");
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
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreWord(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value

	GenCodeStoreInt16ToPointer(ctx, rRAX, rEDX, cmd.rC, cmd.argument);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreDword(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value

	GenCodeStoreInt32ToPointer(ctx, rRAX, rEDX, cmd.rC, cmd.argument);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreLong(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDX, sQWORD, rREG, cmd.rA * 8); // Load value

	GenCodeStoreInt64ToPointer(ctx, rRAX, rRDX, cmd.rC, cmd.argument);
#else
	assert(!"not implemented");
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
	assert(!"not implemented");
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
	assert(!"not implemented");
#endif
}

void GenCodeCmdCombinedd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBreakupdd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMov(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	assert(cmd.rA != cmd.rC);

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store to target
#else
	assert(!"not implemented");
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
	assert(!"not implemented");
#endif
}

void GenCodeCmdDtoi(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_cvttsd2si, rEAX, sQWORD, rREG, cmd.rC * 8); // Load double as int
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store value
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDtol(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_cvttsd2si64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load double as long
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store value
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDtof(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsd2ss, temp, sQWORD, rREG, cmd.rC * 8); // Load double as float
	EMIT_OP_RPTR_REG(ctx.ctx, o_movss, sDWORD, rREG, cmd.rA * 8, temp); // Store value
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdItod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsi2sd, temp, sDWORD, rREG, cmd.rC * 8); // Load int as double
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, temp); // Store value
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLtod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg temp = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsi2sd, temp, sQWORD, rREG, cmd.rC * 8); // Load long as double
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, temp); // Store value
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdItol(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_movsxd, rRAX, sDWORD, rREG, cmd.rC * 8); // Load int as long with sign extension
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store value
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLtoi(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load lower int part of a long number
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store value
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdIndex(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	// TODO: index bounds check
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDX, sQWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load index with zero extension to use in lea (top RAX bits are cleared)

	// Multiply index by size and add to source pointer
	unsigned size = (cmd.argument & 0xffff);

	if(size == 1)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rRAX, sQWORD, rRAX, 1, rRDX, 0);
	}
	else if(size == 2)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rRAX, sQWORD, rRAX, 2, rRDX, 0);
	}
	else if(size == 4)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rRAX, sQWORD, rRAX, 4, rRDX, 0);
	}
	else if(size == 8)
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rRAX, sQWORD, rRAX, 8, rRDX, 0);
	}
	else if(size == 16)
	{
		EMIT_OP_REG_NUM(ctx.ctx, o_shl, rEAX, 4); // 32 bit shift is ok, top bits are zero
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rRAX, sQWORD, rRAX, 1, rRDX, 0);
	}
	else
	{
		EMIT_OP_REG_NUM(ctx.ctx, o_imul, rEAX, size); // 32 bit multiplication is ok, top bits are zero
		EMIT_OP_REG_RPTR(ctx.ctx, o_lea, rRAX, sQWORD, rRAX, 1, rRDX, 0);
	}

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGetAddr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_NUM(ctx.ctx, o_add64, rRAX, cmd.argument); // Add offset
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store to target
#else
	assert(!"not implemented");
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
	assert(!"not implemented");
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
	assert(!"not implemented");
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

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load value
	EMIT_OP_REG_REG(ctx.ctx, o_test, rEAX, rEAX);
	EMIT_OP_LABEL(ctx.ctx, o_jz, LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdJmpnz(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load value
	EMIT_OP_REG_REG(ctx.ctx, o_test, rEAX, rEAX);
	EMIT_OP_LABEL(ctx.ctx, o_jnz, LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
#else
	assert(!"not implemented");
#endif
}

void CallWrap(CodeGenRegVmStateContext *vmState, unsigned functionId)
{
	CodeGenRegVmContext &ctx = *vmState->ctx;

	ExternFuncInfo &target = vmState->ctx->exFunctions[functionId];

	vmState->callStackTop->instruction = vmState->callInstructionPos + 1;
	vmState->callStackTop++;

	// TODO: only for callptr
	/*if(functionId == 0)
	{
		ctx.x86rvm->Stop("ERROR: invalid function pointer");
		return;
	}*/

	unsigned address = target.regVmAddress;

	if(address == ~0u)
	{
		if(target.funcPtrWrap)
		{
			target.funcPtrWrap(target.funcPtrWrapTarget, (char*)vmState->tempStackArrayBase, (char*)vmState->tempStackArrayBase);
		}
		else
		{
#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
			RunRawExternalFunction(ctx.x86rvm->dcCallVM, ctx.exFunctions[functionId], ctx.exLocals, ctx.exTypes, vmState->tempStackArrayBase);
#else
			ctx.x86rvm->Stop("ERROR: external raw function calls are disabled");
#endif
		}

		if(!ctx.x86rvm->callContinue)
			return;

		vmState->callStackTop--;
	}
	else
	{
		unsigned prevDataSize = unsigned(vmState->dataStackTop - vmState->dataStackBase);

		unsigned argumentsSize = target.bytesToPop;

		if(unsigned(vmState->dataStackTop - vmState->dataStackBase) + argumentsSize >= unsigned(vmState->dataStackEnd - vmState->dataStackBase))
		{
			ctx.x86rvm->Stop("ERROR: stack overflow");
			return;
		}

		// Copy function arguments to new stack frame
		memcpy(vmState->dataStackTop, vmState->tempStackArrayBase, argumentsSize);

		unsigned stackSize = (target.stackSize + 0xf) & ~0xf;

		RegVmRegister *prevRegFilePtr = vmState->regFileLastPtr;
		RegVmRegister *prevRegFileTop = vmState->regFileLastTop;

		RegVmRegister *newRegFilePtr = prevRegFileTop;

		vmState->regFileLastPtr = newRegFilePtr;
		vmState->regFileLastTop = newRegFilePtr + target.regVmRegisters;

		if(vmState->regFileLastTop >= vmState->regFileArrayEnd)
		{
			ctx.x86rvm->Stop("ERROR: register overflow");
			return;
		}

		if(unsigned(vmState->dataStackTop - vmState->dataStackBase) + stackSize >= unsigned(vmState->dataStackEnd - vmState->dataStackBase))
		{
			ctx.x86rvm->Stop("ERROR: stack overflow");
			return;
		}

		vmState->dataStackTop += stackSize;

		assert(argumentsSize <= stackSize);

		if(stackSize - argumentsSize)
			memset(vmState->dataStackBase + prevDataSize + argumentsSize, 0, stackSize - argumentsSize);

		newRegFilePtr[rvrrGlobals].ptrValue = uintptr_t(vmState->dataStackBase);
		newRegFilePtr[rvrrFrame].ptrValue = uintptr_t(vmState->dataStackBase + prevDataSize);
		newRegFilePtr[rvrrConstants].ptrValue = uintptr_t(ctx.exRegVmConstants);
		newRegFilePtr[rvrrRegisters].ptrValue = uintptr_t(newRegFilePtr);

		memset(newRegFilePtr + rvrrCount, 0, (vmState->regFileLastTop - newRegFilePtr - rvrrCount) * sizeof(RegVmRegister));

		unsigned char *codeStart = vmState->instAddress[target.regVmAddress];

		typedef	void (*nullcFunc)(unsigned char *codeStart, RegVmRegister *regFilePtr);
		nullcFunc gate = (nullcFunc)(uintptr_t)vmState->codeLaunchHeader;
		gate(codeStart, newRegFilePtr);

		if(!ctx.x86rvm->callContinue)
			return;

		vmState->regFileLastPtr = prevRegFilePtr;
		vmState->regFileLastTop = prevRegFileTop;

		vmState->dataStackTop = vmState->dataStackBase + prevDataSize;

		vmState->callStackTop--;
	}
}

unsigned* GetCodeCmdCallPrologue(CodeGenRegVmContext &ctx, unsigned microcodePos)
{
	// Push arguments
	unsigned *microcode = ctx.exRegVmConstants + microcodePos;

	x86Reg rTempStack = rRBP;

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rTempStack, (uintptr_t)&ctx.vmState->tempStackArrayBase);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rTempStack, sQWORD, rTempStack, 0);

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

	microcode++;

	return microcode;
}

void GetCodeCmdCallEpilogue(CodeGenRegVmContext &ctx, unsigned *microcode, unsigned char resultReg, unsigned char resultType)
{
	x86Reg rTempStack = rRBP;

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
}

void GenCodeCmdCall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->callWrap = CallWrap;

#if defined(_M_X64)
	// TODO: ERROR: call argument buffer overflow

	unsigned *microcode = GetCodeCmdCallPrologue(ctx, (cmd.rA << 16) | (cmd.rB << 8) | cmd.rC);

	unsigned char resultReg = *microcode++ & 0xff;
	unsigned char resultType = *microcode++ & 0xff;

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRCX, uintptr_t(ctx.vmState));
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->callInstructionPos) - uintptr_t(ctx.vmState)), ctx.currInstructionPos);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->callWrap) - uintptr_t(ctx.vmState)));
	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEDX, cmd.argument);
	EMIT_REG_READ(ctx.ctx, rRCX);
	EMIT_REG_READ(ctx.ctx, rEDX);
	EMIT_OP_REG(ctx.ctx, o_call, rRAX);

	GetCodeCmdCallEpilogue(ctx, microcode, resultReg, resultType);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdCallPtr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	// TODO: complex instruction with microcode and exceptions

	unsigned *microcode = GetCodeCmdCallPrologue(ctx, cmd.argument);

	unsigned char resultReg = *microcode++ & 0xff;
	unsigned char resultType = *microcode++ & 0xff;

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRCX, uintptr_t(ctx.vmState));
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->callInstructionPos) - uintptr_t(ctx.vmState)), ctx.currInstructionPos);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->callWrap) - uintptr_t(ctx.vmState)));
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rC * 8); // Get function id
	EMIT_REG_READ(ctx.ctx, rRCX);
	EMIT_REG_READ(ctx.ctx, rEDX);
	EMIT_OP_REG(ctx.ctx, o_call, rRAX);

	GetCodeCmdCallEpilogue(ctx, microcode, resultReg, resultType);
#else
	assert(!"not implemented");
#endif
}

void CheckedReturnWrap(CodeGenRegVmStateContext *vmState, unsigned microcodePos)
{
	CodeGenRegVmContext &ctx = *vmState->ctx;

	RegVmRegister *regFilePtr = vmState->regFileLastPtr;

	unsigned dataSize = unsigned(vmState->dataStackTop - vmState->dataStackBase);

	uintptr_t frameBase = regFilePtr[rvrrFrame].ptrValue;
	uintptr_t frameEnd = regFilePtr[rvrrGlobals].ptrValue + dataSize;

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

void GenCodeCmdReturn(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	// TODO: ERROR: function didn't return a value

	if(cmd.rB == rvrError)
	{
		EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEAX, cmd.rB);
		EMIT_OP_REG_NUM(ctx.ctx, o_add64, rRSP, 32);
		EMIT_OP(ctx.ctx, o_ret);
		return;
	}

	if(cmd.rB != rvrVoid)
	{
		unsigned *microcode = ctx.exRegVmConstants + cmd.argument;

		unsigned typeId = *microcode++;
		unsigned typeSize = *microcode++;

		(void)typeId;
		(void)typeSize;

		// TODO: ERROR: call return buffer overflow

		x86Reg rTempStack = rRBP;

		if(*microcode != rvmiReturn)
		{
			EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rTempStack, (uintptr_t)&ctx.vmState->tempStackArrayBase);
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rTempStack, sQWORD, rTempStack, 0);
		}

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
			ctx.vmState->checkedReturnWrap = CheckedReturnWrap;

			EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRCX, uintptr_t(ctx.vmState));
			EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->checkedReturnWrap) - uintptr_t(ctx.vmState)));
			EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEDX, cmd.argument);
			EMIT_REG_READ(ctx.ctx, rRCX);
			EMIT_REG_READ(ctx.ctx, rEDX);
			EMIT_OP_REG(ctx.ctx, o_call, rRAX);
		}
	}

	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEAX, cmd.rB);
	EMIT_OP_REG_NUM(ctx.ctx, o_add64, rRSP, 32);
	EMIT_OP(ctx.ctx, o_ret);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAddImm(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	if(cmd.rA == cmd.rB)
	{
		EMIT_OP_RPTR_NUM(ctx.ctx, o_add, sDWORD, rREG, cmd.rA * 8, cmd.argument); // Modify inplace
	}
	else
	{
		EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load value
		EMIT_OP_REG_NUM(ctx.ctx, o_add, rEAX, cmd.argument);
		EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store to target
	}
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAdd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_add, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdSub(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_sub, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMul(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_imul, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDiv(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cdq);

	GenCodeLoadInt32FromPointer(ctx, rRCX, rECX, cmd.rC, cmd.argument);

	EMIT_OP_REG(ctx.ctx, o_idiv, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void PowWrap(RegVmRegister *regFilePtr, uintptr_t cmdValue)
{
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].intValue = VmIntPow(*(int*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument), regFilePtr[cmd.rB].intValue);
}

void GenCodeCmdPow(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->powWrap = PowWrap;

#if defined(_M_X64)
	uintptr_t cmdValue;
	memcpy(&cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRCX, uintptr_t(ctx.vmState));
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->powWrap) - uintptr_t(ctx.vmState)));
	EMIT_OP_REG_REG(ctx.ctx, o_mov64, rRCX, rRBX);
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRDX, cmdValue);
	EMIT_REG_READ(ctx.ctx, rRCX);
	EMIT_REG_READ(ctx.ctx, rEDX);
	EMIT_OP_REG(ctx.ctx, o_call, rRAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cdq);

	GenCodeLoadInt32FromPointer(ctx, rRCX, rECX, cmd.rC, cmd.argument);

	EMIT_OP_REG(ctx.ctx, o_idiv, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEDX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLess(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setl, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGreater(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setg, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setle, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setge, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdEqual(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setne, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdShl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rECX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_sal, rEAX, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdShr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rECX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_sar, rEAX, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitAnd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_and, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitOr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_or, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitXor(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rEDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAddImml(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_NUM(ctx.ctx, o_add64, rEAX, cmd.argument);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAddl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_add64, rRAX, rRDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdSubl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_sub64, rRAX, rRDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMull(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_imul64, rRAX, rRDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDivl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cqo);

	GenCodeLoadInt64FromPointer(ctx, rRCX, rRCX, cmd.rC, cmd.argument);

	EMIT_OP_REG(ctx.ctx, o_idiv64, rRCX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void PowlWrap(RegVmRegister *regFilePtr, uintptr_t cmdValue)
{
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].longValue = VmLongPow(*(long long*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument), regFilePtr[cmd.rB].longValue);
}

void GenCodeCmdPowl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->powlWrap = PowlWrap;

#if defined(_M_X64)
	uintptr_t cmdValue;
	memcpy(&cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRCX, uintptr_t(ctx.vmState));
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->powlWrap) - uintptr_t(ctx.vmState)));
	EMIT_OP_REG_REG(ctx.ctx, o_mov64, rRCX, rRBX);
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRDX, cmdValue);
	EMIT_REG_READ(ctx.ctx, rRCX);
	EMIT_REG_READ(ctx.ctx, rEDX);
	EMIT_OP_REG(ctx.ctx, o_call, rRAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdModl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP(ctx.ctx, o_cqo);

	GenCodeLoadInt64FromPointer(ctx, rRCX, rRCX, cmd.rC, cmd.argument);

	EMIT_OP_REG(ctx.ctx, o_idiv64, rRCX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRDX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLessl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setl, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGreaterl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setg, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLequall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setle, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGequall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setge, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdEquall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNequall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load long lhs value

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp64, rRAX, rRDX);
	EMIT_OP_REG(ctx.ctx, o_setne, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdShll(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rECX, cmd.rC, cmd.argument); // Load lower int bits ot the rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_sal64, rRAX, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdShrl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt32FromPointer(ctx, rRDX, rECX, cmd.rC, cmd.argument); // Load lower int bits ot the rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_sar64, rRAX, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitAndl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_and64, rRAX, rRDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitOrl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_or64, rRAX, rRDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitXorl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rB * 8); // Load lhs

	GenCodeLoadInt64FromPointer(ctx, rRDX, rRDX, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_xor64, rRAX, rRDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAddd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_addsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdSubd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_subsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMuld(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_mulsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDivd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_divsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	assert(!"not implemented");
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

	EMIT_OP_REG_REG(ctx.ctx, o_addsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	assert(!"not implemented");
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

	EMIT_OP_REG_REG(ctx.ctx, o_subsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	assert(!"not implemented");
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

	EMIT_OP_REG_REG(ctx.ctx, o_mulsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	assert(!"not implemented");
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

	EMIT_OP_REG_REG(ctx.ctx, o_divsd, lhs, rhs);
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, lhs); // Store double to target
#else
	assert(!"not implemented");
#endif
}

void PowdWrap(RegVmRegister *regFilePtr, uintptr_t cmdValue)
{
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].doubleValue = pow(regFilePtr[cmd.rB].doubleValue, *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));
}

void GenCodeCmdPowd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->powdWrap = PowdWrap;

#if defined(_M_X64)
	uintptr_t cmdValue;
	memcpy(&cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRCX, uintptr_t(ctx.vmState));
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->powdWrap) - uintptr_t(ctx.vmState)));
	EMIT_OP_REG_REG(ctx.ctx, o_mov64, rRCX, rRBX);
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRDX, cmdValue);
	EMIT_REG_READ(ctx.ctx, rRCX);
	EMIT_REG_READ(ctx.ctx, rEDX);
	EMIT_OP_REG(ctx.ctx, o_call, rRAX);
#else
	assert(!"not implemented");
#endif
}

void ModdWrap(RegVmRegister *regFilePtr, uintptr_t cmdValue)
{
	RegVmCmd cmd;
	memcpy(&cmd, &cmdValue, sizeof(cmd));

	regFilePtr[cmd.rA].doubleValue = fmod(regFilePtr[cmd.rB].doubleValue, *(double*)(uintptr_t)(regFilePtr[cmd.rC].ptrValue + cmd.argument));
}

void GenCodeCmdModd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->moddWrap = ModdWrap;

#if defined(_M_X64)
	uintptr_t cmdValue;
	memcpy(&cmdValue, &cmd, sizeof(cmdValue));

	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRCX, uintptr_t(ctx.vmState));
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->moddWrap) - uintptr_t(ctx.vmState)));
	EMIT_OP_REG_REG(ctx.ctx, o_mov64, rRCX, rRBX);
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRDX, cmdValue);
	EMIT_REG_READ(ctx.ctx, rRCX);
	EMIT_REG_READ(ctx.ctx, rEDX);
	EMIT_OP_REG(ctx.ctx, o_call, rRAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLessd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpltsd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGreaterd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpltsd, rhs, lhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, rhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLequald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmplesd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGequald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmplesd, rhs, lhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, rhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdEquald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpeqsd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNequald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	x86XmmReg lhs = ctx.ctx.GetXmmReg();
	x86XmmReg rhs = ctx.ctx.GetXmmReg();

	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, lhs, sQWORD, rREG, cmd.rB * 8); // Load double value

	GenCodeLoadDoubleFromPointer(ctx, rRAX, rhs, cmd.rC, cmd.argument);

	EMIT_OP_REG_REG(ctx.ctx, o_cmpneqsd, lhs, rhs);
	EMIT_OP_REG_REG(ctx.ctx, o_movd, rEAX, lhs);
	EMIT_OP_REG_NUM(ctx.ctx, o_and, rEAX, 0x01);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNeg(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load int value
	EMIT_OP_REG(ctx.ctx, o_neg, rEAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNegl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load long value
	EMIT_OP_REG(ctx.ctx, o_neg64, rRAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNegd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load double as a long bit pattern
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRDX, 0x8000000000000000ull);
	EMIT_OP_REG_REG(ctx.ctx, o_xor64, rRAX, rRDX); // Switch top bit
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long bit pattern to target double
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitNot(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load int value
	EMIT_OP_REG(ctx.ctx, o_not, rEAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitNotl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load int value
	EMIT_OP_REG(ctx.ctx, o_not64, rRAX);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLogNot(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rC * 8); // Load int value

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rECX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLogNotl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load long value

	EMIT_OP_REG_REG(ctx.ctx, o_xor64, rRCX, rRCX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rRAX, rRCX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rECX); // Store int to target
#else
	assert(!"not implemented");
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
	}
}

void GenCodeCmdConvertPtr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	ctx.vmState->convertPtrWrap = ConvertPtrWrap;

#if defined(_M_X64)
	EMIT_OP_REG_NUM64(ctx.ctx, o_mov64, rRCX, uintptr_t(ctx.vmState));
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->callInstructionPos) - uintptr_t(ctx.vmState)), ctx.currInstructionPos);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRCX, unsigned(uintptr_t(&ctx.vmState->convertPtrWrap) - uintptr_t(ctx.vmState)));
	EMIT_OP_REG_NUM(ctx.ctx, o_mov, rEDX, cmd.argument);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rR8, sDWORD, rREG, cmd.rB * 8); // Load typeid
	EMIT_REG_READ(ctx.ctx, rRCX);
	EMIT_REG_READ(ctx.ctx, rEDX);
	EMIT_REG_READ(ctx.ctx, rR8);
	EMIT_OP_REG(ctx.ctx, o_call, rRAX);

	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Get source pointer
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Move to target
#else
	assert(!"not implemented");
#endif
}
