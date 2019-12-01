#include "CodeGenRegVm_X86.h"

#include "CodeGen_X86.h"
#include "InstructionTreeRegVm.h"

#if defined(_M_X64)
const x86Reg rREG = rRBX;
#else
const x86Reg rREG = rEBX;
#endif

// TODO: special handling for rvrrGlobals
// TODO: special handling for rvrrFrame (x64 only, reserve extra register)
// TODO: special handling for rvrrConstants
// TODO: special handling for rvrrRegisters

void GenCodeCmdNop(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));
}

void GenCodeCmdLoadByte(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, rEAX, sBYTE, rRAX, cmd.argument); // Load byte value with sign extension
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadWord(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, rEAX, sWORD, rRAX, cmd.argument); // Load short value with sign extension
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadDword(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rRAX, cmd.argument); // Load int value
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadLong(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRAX, cmd.argument); // Load long value
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX); // Store long to target
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadFloat(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtss2sd, rXMM0, sDWORD, rRAX, cmd.argument); // Load float as double
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, rXMM0); // Store double to target
	EMIT_REG_KILL(ctx.ctx, rXMM0);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load source pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, rXMM0, sQWORD, rRAX, cmd.argument); // Load double value
	EMIT_OP_RPTR_REG(ctx.ctx, o_movsd, sQWORD, rREG, cmd.rA * 8, rXMM0); // Store double to target
	EMIT_REG_KILL(ctx.ctx, rXMM0);
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

	//

#if defined(_M_X64)
	assert(!"not implemented");
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
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load target pointer
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sBYTE, rRAX, cmd.argument, rEDX); // Store value to target with an offset
	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreWord(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load target pointer
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sWORD, rRAX, cmd.argument, rEDX); // Store value to target with an offset
	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreDword(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEDX, sDWORD, rREG, cmd.rA * 8); // Load value
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load target pointer
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rRAX, cmd.argument, rEDX); // Store value to target with an offset
	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreLong(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRDX, sQWORD, rREG, cmd.rA * 8); // Load value
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load target pointer
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rRAX, cmd.argument, rRDX); // Store value to target with an offset
	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreFloat(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsd2ss, rXMM0, sQWORD, rREG, cmd.rA * 8); // Load value
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load target pointer
	EMIT_OP_RPTR_REG(ctx.ctx, o_movss, sDWORD, rRAX, cmd.argument, rXMM0); // Store value to target with an offset
	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rXMM0);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_movsd, rXMM0, sQWORD, rREG, cmd.rA * 8); // Load value
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load target pointer
	EMIT_OP_RPTR_REG(ctx.ctx, o_movss, sDWORD, rRAX, cmd.argument, rXMM0); // Store value to target with an offset
	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rXMM0);
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

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMovMult(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
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
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDtol(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_cvttsd2si64, rRAX, sQWORD, rREG, cmd.rC * 8); // Load double as long
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sQWORD, rREG, cmd.rA * 8, rRAX); // Store value
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDtof(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsd2ss, rXMM0, sQWORD, rREG, cmd.rC * 8); // Load double as float
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rXMM0); // Store value
	EMIT_REG_KILL(ctx.ctx, rXMM0);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdItod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsi2sd, rXMM0, sDWORD, rREG, cmd.rC * 8); // Load int as double
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sQWORD, rREG, cmd.rA * 8, rXMM0); // Store value
	EMIT_REG_KILL(ctx.ctx, rXMM0);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLtod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_cvtsi2sd, rXMM0, sQWORD, rREG, cmd.rC * 8); // Load long as double
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sQWORD, rREG, cmd.rA * 8, rXMM0); // Store value
	EMIT_REG_KILL(ctx.ctx, rXMM0);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdItol(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_movsxd, rRAX, sQWORD, rREG, cmd.rC * 8); // Load int as long with sign extension
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sQWORD, rREG, cmd.rA * 8, rRAX); // Store value
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLtoi(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rRAX, sQWORD, rREG, cmd.rC * 8); // Load int as long with sign extension
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rRAX); // Store value
	EMIT_REG_KILL(ctx.ctx, rEAX);
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
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sDWORD, rREG, cmd.rB * 8); // Load index with zero extension to use in lea

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
	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
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
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdSetRange(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMemCopy(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
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

void GenCodeCmdCall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	// TODO: complex instruction with microcode and exceptions

	//assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdCallPtr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	// TODO: complex instruction with microcode and exceptions

	//assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdReturn(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	// TODO: complex instruction with microcode and exceptions

	//assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAddImm(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load value
	EMIT_OP_REG_NUM(ctx.ctx, o_add, rEAX, cmd.argument);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store to target
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAdd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rEDX, sQWORD, rREG, cmd.rC * 8); // Load rhs pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rEDX, cmd.argument); // Load int rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_add, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target

	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdSub(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rEDX, sQWORD, rREG, cmd.rC * 8); // Load rhs pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rEDX, cmd.argument); // Load int rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_sub, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target

	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMul(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rEDX, sQWORD, rREG, cmd.rC * 8); // Load rhs pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rEDX, cmd.argument); // Load int rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_imul, rEAX, rEDX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target

	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDiv(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdPow(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLess(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rEDX, sQWORD, rREG, cmd.rC * 8); // Load rhs pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rEDX, cmd.argument); // Load int rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setl, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target

	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
	EMIT_REG_KILL(ctx.ctx, rECX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGreater(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rEDX, sQWORD, rREG, cmd.rC * 8); // Load rhs pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rEDX, cmd.argument); // Load int rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setg, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target

	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
	EMIT_REG_KILL(ctx.ctx, rECX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rEDX, sQWORD, rREG, cmd.rC * 8); // Load rhs pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rEDX, cmd.argument); // Load int rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setle, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target

	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
	EMIT_REG_KILL(ctx.ctx, rECX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rEDX, sQWORD, rREG, cmd.rC * 8); // Load rhs pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rEDX, cmd.argument); // Load int rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setge, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target

	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
	EMIT_REG_KILL(ctx.ctx, rECX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdEqual(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rEDX, sQWORD, rREG, cmd.rC * 8); // Load rhs pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rEDX, cmd.argument); // Load int rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_sete, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target

	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
	EMIT_REG_KILL(ctx.ctx, rECX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rREG, cmd.rB * 8); // Load lhs
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rEDX, sQWORD, rREG, cmd.rC * 8); // Load rhs pointer
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rEDX, cmd.argument); // Load int rhs value

	EMIT_OP_REG_REG(ctx.ctx, o_xor, rECX, rECX);
	EMIT_OP_REG_REG(ctx.ctx, o_cmp, rEAX, rEDX);
	EMIT_OP_REG(ctx.ctx, o_setne, rECX);

	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX); // Store int to target

	EMIT_REG_KILL(ctx.ctx, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEDX);
	EMIT_REG_KILL(ctx.ctx, rECX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdShl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdShr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitAnd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitOr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitXor(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAddImml(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAddl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdSubl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMull(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDivl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdPowl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdModl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLessl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGreaterl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLequall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGequall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdEquall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNequall(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdShll(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdShrl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitAndl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitOrl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitXorl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAddd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdSubd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMuld(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDivd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAddf(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdSubf(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMulf(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDivf(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdPowd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdModd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLessd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGreaterd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLequald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGequald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdEquald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNequald(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNeg(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNegl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNegd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitNot(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdBitNotl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLogNot(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLogNotl(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdConvertPtr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}
