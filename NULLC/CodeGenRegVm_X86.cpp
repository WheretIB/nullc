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
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, rEAX, sBYTE, rRAX, cmd.argument);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadWord(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_movsx, rEAX, sWORD, rRAX, cmd.argument);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadDword(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov, rEAX, sDWORD, rRAX, cmd.argument);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, rEAX);
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadLong(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rREG, cmd.rC * 8);
	EMIT_OP_REG_RPTR(ctx.ctx, o_mov64, rRAX, sQWORD, rRAX, cmd.argument);
	EMIT_OP_RPTR_REG(ctx.ctx, o_mov64, sQWORD, rREG, cmd.rA * 8, rRAX);
	EMIT_REG_KILL(ctx.ctx, rEAX);
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadFloat(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLoadImm(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

#if defined(_M_X64)
	EMIT_OP_RPTR_NUM(ctx.ctx, o_mov, sDWORD, rREG, cmd.rA * 8, cmd.argument);
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

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdStoreDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
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

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDtol(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdDtof(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdItod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLtod(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdItol(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLtoi(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdIndex(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGetAddr(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
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

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdJmpnz(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
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

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdAdd(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdSub(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdMul(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
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

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGreater(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdLequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdGequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdEqual(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
#else
	assert(!"not implemented");
#endif
}

void GenCodeCmdNequal(CodeGenRegVmContext &ctx, RegVmCmd cmd)
{
	EMIT_COMMENT(ctx.ctx, GetInstructionName(RegVmInstructionCode(cmd.code)));

	//

#if defined(_M_X64)
	assert(!"not implemented");
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
