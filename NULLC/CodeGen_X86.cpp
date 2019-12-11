#include "CodeGen_X86.h"

//#undef NULLC_OPTIMIZE_X86

#ifdef NULLC_BUILD_X86_JIT

#include "Executor_Common.h"
#include "StdLib.h"

unsigned CodeGenGenericContext::MemFind(const x86Argument &address)
{
	for(unsigned i = 0; i < memoryStateSize; i++)
	{
		MemCache &entry = memCache[i];

		if(entry.value.type != x86Argument::argNone && entry.address.type != x86Argument::argNone &&
			entry.address.ptrSize == address.ptrSize && entry.address.ptrNum == address.ptrNum &&
			entry.address.ptrBase == address.ptrBase && entry.address.ptrIndex == address.ptrIndex)
		{
			return i + 1;
		}
	}
	return 0;
}

void CodeGenGenericContext::MemRead(const x86Argument &address)
{
	(void)address;

	for(unsigned i = 0; i < memoryStateSize; i++)
	{
		MemCache &entry = memCache[i];

		entry.read = true;
	}
}

void CodeGenGenericContext::MemWrite(const x86Argument &address, const x86Argument &value)
{
	for(unsigned i = 0; i < memoryStateSize; i++)
	{
		MemCache &entry = memCache[i];

		if(entry.value.type != x86Argument::argNone && entry.address.type != x86Argument::argNone && entry.address.ptrBase == address.ptrBase && entry.address.ptrIndex == address.ptrIndex && entry.address.ptrSize != address.ptrSize)
		{
			unsigned dataSize = entry.address.ptrSize == sBYTE ? 1 : (entry.address.ptrSize == sWORD ? 2 : (entry.address.ptrSize == sDWORD ? 4 : 8));
			unsigned argSize = address.ptrSize == sBYTE ? 1 : (address.ptrSize == sWORD ? 2 : (address.ptrSize == sDWORD ? 4 : 8));

			if(unsigned(address.ptrNum) + argSize <= unsigned(entry.address.ptrNum) || unsigned(address.ptrNum) >= unsigned(entry.address.ptrNum) + dataSize)
				continue;

			entry.address.type = x86Argument::argNone;
		}
	}

	if(unsigned index = MemFind(address))
	{
		index--;

		if(index != 0)
		{
			MemCache tmp = memCache[index - 1];
			memCache[index - 1] = memCache[index];
			memCache[index] = tmp;

			memCache[index - 1].value = value;
			memCache[index - 1].location = unsigned(x86Op - x86Base);
			memCache[index - 1].read = false;
		}
		else
		{
			memCache[0].value = value;
			memCache[0].location = unsigned(x86Op - x86Base);
			memCache[0].read = false;
		}
	}
	else
	{
		unsigned newIndex = memCacheEntries < memoryStateSize ? memCacheEntries : memoryStateSize - 1;

		if(memCacheEntries < memoryStateSize)
			memCacheEntries++;
		else
			memCacheEntries = memoryStateSize >> 1;	// Wrap to the middle

		memCache[newIndex].address = address;
		memCache[newIndex].value = value;
		memCache[newIndex].location = unsigned(x86Op - x86Base);
		memCache[newIndex].read = false;
	}
}

void CodeGenGenericContext::MemUpdate(unsigned index)
{
	if(index != 0)
	{
		MemCache tmp = memCache[index - 1];
		memCache[index - 1] = memCache[index];
		memCache[index] = tmp;
	}
}

void CodeGenGenericContext::InvalidateState()
{
#if defined(NULLC_OPTIMIZE_X86)
	for(unsigned i = 0; i < rRegCount; i++)
		genReg[i].type = x86Argument::argNone;

	for(unsigned i = 0; i < rXmmRegCount; i++)
		xmmReg[i].type = x86Argument::argNone;

	for(unsigned i = 0; i < memoryStateSize; i++)
		memCache[i].address.type = x86Argument::argNone;

	memCacheEntries = 0;
#endif

	currFreeXmmReg = rXMM0;
}

void CodeGenGenericContext::InvalidateDependand(x86Reg dreg)
{
	for(unsigned i = 0; i < rRegCount; i++)
	{
		if(genReg[i].type == x86Argument::argReg && genReg[i].reg == dreg)
			genReg[i].type = x86Argument::argPtrLabel;

		if(genReg[i].type == x86Argument::argPtr && (genReg[i].ptrBase == dreg || genReg[i].ptrIndex == dreg))
			genReg[i].type = x86Argument::argPtrLabel;
	}

	for(unsigned i = 0; i < memoryStateSize; i++)
	{
		MemCache &entry = memCache[i];

		if(entry.address.type == x86Argument::argReg && entry.address.reg == dreg)
			entry.address.type = x86Argument::argNone;

		if(entry.address.type == x86Argument::argPtr && (entry.address.ptrBase == dreg || entry.address.ptrIndex == dreg))
			entry.address.type = x86Argument::argNone;

		if(entry.value.type == x86Argument::argReg && entry.value.reg == dreg)
			entry.value.type = x86Argument::argNone;

		if(entry.value.type == x86Argument::argPtr && (entry.value.ptrBase == dreg || entry.value.ptrIndex == dreg))
			entry.value.type = x86Argument::argNone;
	}
}

void CodeGenGenericContext::InvalidateDependand(x86XmmReg dreg)
{
	for(unsigned i = 0; i < rXmmRegCount; i++)
	{
		if(xmmReg[i].type == x86Argument::argReg && xmmReg[i].xmmArg == dreg)
			xmmReg[i].type = x86Argument::argPtrLabel;
	}

	for(unsigned i = 0; i < memoryStateSize; i++)
	{
		MemCache &entry = memCache[i];

		if(entry.value.type == x86Argument::argXmmReg && entry.value.xmmArg == dreg)
			entry.value.type = x86Argument::argNone;
	}
}

void CodeGenGenericContext::InvalidateAddressValue(x86Argument arg)
{
	assert(arg.type == x86Argument::argPtr);

	for(unsigned i = 0; i < rRegCount; i++)
	{
		x86Argument &data = genReg[i];

		if(data.type == x86Argument::argPtr)
		{
			// Can't rewrite constants
			if(data.ptrIndex == rNONE && data.ptrBase == rR14)
				continue;

			// If register contains data from register memory block, no memory update using a different register can overwrite it
			if(data.ptrIndex == rNONE && data.ptrBase == rEBX && (arg.ptrIndex != rNONE || arg.ptrBase != rEBX))
				continue;

			// If register doesn't contain data from a register memory block, register memory update can't overwrite it
			if((data.ptrIndex != rNONE || data.ptrBase != rEBX) && arg.ptrIndex == rNONE && arg.ptrBase == rEBX)
				continue;

			if(data.ptrIndex == rNONE && arg.ptrIndex == rNONE && data.ptrBase == arg.ptrBase)
			{
				assert(data.ptrSize != sNONE);
				assert(arg.ptrSize != sNONE);

				unsigned dataSize = data.ptrSize == sBYTE ? 1 : (data.ptrSize == sWORD ? 2 : (data.ptrSize == sDWORD ? 4 : 8));
				unsigned argSize = arg.ptrSize == sBYTE ? 1 : (arg.ptrSize == sWORD ? 2 : (arg.ptrSize == sDWORD ? 4 : 8));

				if(unsigned(arg.ptrNum) + argSize <= unsigned(data.ptrNum) || unsigned(arg.ptrNum) >= unsigned(data.ptrNum) + dataSize)
					continue;
			}

			data.type = x86Argument::argNone;
		}
	}

	for(unsigned i = 0; i < rXmmRegCount; i++)
	{
		x86Argument &data = xmmReg[i];

		if(data.type == x86Argument::argPtr)
		{
			// Can't rewrite constants
			if(data.ptrIndex == rNONE && data.ptrBase == rR14)
				continue;

			if(data.ptrIndex == rNONE && arg.ptrIndex == rNONE && data.ptrBase == arg.ptrBase)
			{
				assert(data.ptrSize == sDWORD || data.ptrSize == sQWORD);
				assert(data.ptrSize == sDWORD || data.ptrSize == sQWORD);

				unsigned dataSize = data.ptrSize == sDWORD ? 4 : 8;
				unsigned argSize = arg.ptrSize == sDWORD ? 4 : 8;

				if(unsigned(arg.ptrNum) + argSize <= unsigned(data.ptrNum) || unsigned(arg.ptrNum) >= unsigned(data.ptrNum) + dataSize)
					continue;
			}

			data.type = x86Argument::argNone;
		}
	}
}

void CodeGenGenericContext::KillUnreadRegisters()
{
	for(unsigned i = 0; i < rRegCount; i++)
		EMIT_REG_KILL(*this, x86Reg(i));

	for(unsigned i = 0; i < rXmmRegCount; i++)
		EMIT_REG_KILL(*this, x86XmmReg(i));
}

void CodeGenGenericContext::ReadRegister(x86Reg reg)
{
	genRegRead[reg] = true;
}

void CodeGenGenericContext::ReadRegister(x86XmmReg reg)
{
	xmmRegRead[reg] = true;
}

void CodeGenGenericContext::OverwriteRegisterWithValue(x86Reg reg, x86Argument arg)
{
	// Destination is updated
	EMIT_REG_KILL(*this, reg);
	InvalidateDependand(reg);

	genReg[reg] = arg;
	genRegUpdate[reg] = unsigned(x86Op - x86Base);
	genRegRead[reg] = false;
}

void CodeGenGenericContext::OverwriteRegisterWithUnknown(x86Reg reg)
{
	// Destination is updated
	EMIT_REG_KILL(*this, reg);
	InvalidateDependand(reg);

	genReg[reg].type = x86Argument::argNone;
	genRegUpdate[reg] = unsigned(x86Op - x86Base);
	genRegRead[reg] = false;
}

void CodeGenGenericContext::OverwriteRegisterWithValue(x86XmmReg reg, x86Argument arg)
{
	// Destination is updated
	EMIT_REG_KILL(*this, reg);
	InvalidateDependand(reg);

	xmmReg[reg] = arg;
	xmmRegUpdate[reg] = unsigned(x86Op - x86Base);
	xmmRegRead[reg] = false;
}

void CodeGenGenericContext::OverwriteRegisterWithUnknown(x86XmmReg reg)
{
	// Destination is updated
	EMIT_REG_KILL(*this, reg);
	InvalidateDependand(reg);

	xmmReg[reg].type = x86Argument::argNone;
	xmmRegUpdate[reg] = unsigned(x86Op - x86Base);
	xmmRegRead[reg] = false;
}

void CodeGenGenericContext::ReadAndModifyRegister(x86Reg reg)
{
	InvalidateDependand(reg);

	genReg[reg].type = x86Argument::argNone;
	genRegUpdate[reg] = unsigned(x86Op - x86Base);
	genRegRead[reg] = false;
}

void CodeGenGenericContext::ReadAndModifyRegister(x86XmmReg reg)
{
	InvalidateDependand(reg);

	xmmReg[reg].type = x86Argument::argNone;
	xmmRegUpdate[reg] = unsigned(x86Op - x86Base);
	xmmRegRead[reg] = false;
}

void CodeGenGenericContext::RedirectAddressComputation(x86Reg &index, int &multiplier, x86Reg &base, unsigned &shift)
{
	// If selected base register contains the value of another register, use it instead
	if(genReg[base].type == x86Argument::argReg)
	{
		base = genReg[base].reg;
	}

	// If the base register contains a known value, add it to the offset and don't use the base register
	if(genReg[base].type == x86Argument::argNumber)
	{
		shift += genReg[base].num;
		base = rNONE;
	}

	// If the index register contains a known value, add it to the offset and don't use the index register
	if(genReg[index].type == x86Argument::argNumber)
	{
		shift += genReg[index].num * multiplier;
		multiplier = 1;
		index = rNONE;
	}
}

x86Reg CodeGenGenericContext::RedirectRegister(x86Reg reg)
{
	// If a register holds another 
	if(genReg[reg].type == x86Argument::argReg)
		return genReg[reg].reg;

	return reg;
}

x86XmmReg CodeGenGenericContext::RedirectRegister(x86XmmReg reg)
{
	// If a register holds another 
	if(xmmReg[reg].type == x86Argument::argXmmReg)
		return xmmReg[reg].xmmArg;

	return reg;
}

x86Reg CodeGenGenericContext::GetReg()
{
	static x86Reg regs[] = { rRAX, rRDX, rEDI, rESI, rR8, rR9, rR10, rR11 };

	// Simple rotation
	x86Reg res = regs[currFreeReg];

	if(res == rR11)
		currFreeReg = 0;
	else
		currFreeReg += 1;

	return res;
}

x86XmmReg CodeGenGenericContext::GetXmmReg()
{
	// Simple rotation
	x86XmmReg res = currFreeXmmReg;

#if defined(_MSC_VER) && defined(_M_X64)
	x86XmmReg lastXmmReg = rXMM5; // Less volatile registers than on x86
#elif defined(_MSC_VER)
	x86XmmReg lastXmmReg = rXMM7;
#elif defined(_M_X64)
	x86XmmReg lastXmmReg = rXMM15;
#else
	x86XmmReg lastXmmReg = rXMM7;
#endif

	if(currFreeXmmReg == lastXmmReg)
		currFreeXmmReg = rXMM0;
	else
		currFreeXmmReg = x86XmmReg(currFreeXmmReg + 1);

	return res;
}

void EMIT_COMMENT(CodeGenGenericContext &ctx, const char* text)
{
#if !defined(NDEBUG)
	ctx.x86Op->name = o_other;
	ctx.x86Op->comment = text;
	ctx.x86Op++;
#else
	(void)ctx;
	(void)text;
#endif
}

void EMIT_LABEL(CodeGenGenericContext &ctx, unsigned labelID, int invalidate)
{
	if(invalidate)
		ctx.InvalidateState();

	ctx.x86Op->name = o_label;
	ctx.x86Op->labelID = labelID;
	ctx.x86Op->argA.type = x86Argument::argNone;
	ctx.x86Op->argA.num = invalidate;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP(CodeGenGenericContext &ctx, x86Command op)
{
#ifdef NULLC_OPTIMIZE_X86
	switch(op)
	{
	case o_ret:
		// Mark return registers as implicitly used
		ctx.ReadRegister(rEAX);
		ctx.ReadRegister(rEDX);

		ctx.KillUnreadRegisters();

		ctx.InvalidateState();
		break;
	case o_rep_movsd:
		assert(ctx.genReg[rECX].type == x86Argument::argNumber);

		ctx.InvalidateState();

		// Implicit register reads
		ctx.ReadRegister(rECX);
		ctx.ReadRegister(rESI);
		ctx.ReadRegister(rEDI);
		break;
	case o_rep_stosb:
	case o_rep_stosw:
	case o_rep_stosd:
	case o_rep_stosq:
		assert(ctx.genReg[rECX].type == x86Argument::argNumber);

		ctx.InvalidateState();

		// Implicit register reads
		ctx.ReadRegister(rECX);
		ctx.ReadRegister(rEAX);
		ctx.ReadRegister(rEDI);
		break;
	case o_cdq:
	case o_cqo:
		ctx.ReadRegister(rEAX);

		ctx.OverwriteRegisterWithUnknown(rEDX);
		break;
	case o_use32:
	case o_other:
		break;
	default:
		assert(!"unknown instruction");
	}
#else
	if(op == o_ret)
		ctx.InvalidateState();
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argNone;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP_LABEL(CodeGenGenericContext &ctx, x86Command op, unsigned labelID, int invalidate, int longJump)
{
#ifdef NULLC_OPTIMIZE_X86
	switch(op)
	{
	case o_jmp:
	case o_ja:
	case o_jae:
	case o_jb:
	case o_jbe:
	case o_je:
	case o_jg:
	case o_jl:
	case o_jne:
	case o_jnp:
	case o_jp:
	case o_jge:
	case o_jle:
		// TODO: setcc followed by test and jump can be optimized

		if(invalidate)
		{
			if(longJump)
				ctx.KillUnreadRegisters();

			ctx.InvalidateState();
		}
		break;
	case o_call:
		ctx.KillUnreadRegisters();

		ctx.InvalidateState();
		break;
	default:
		assert(!"unknown instruction");
	}
#else
	switch(op)
	{
	case o_jmp:
	case o_ja:
	case o_jae:
	case o_jb:
	case o_jbe:
	case o_je:
	case o_jg:
	case o_jl:
	case o_jne:
	case o_jnp:
	case o_jp:
	case o_jge:
	case o_jle:
		if(invalidate)
			ctx.InvalidateState();
		break;
	case o_call:
		ctx.InvalidateState();
		break;
	}

	(void)longJump;
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argLabel;
	ctx.x86Op->argA.labelID = labelID;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op->argB.num = invalidate;
	ctx.x86Op->argB.ptrNum = longJump;
	ctx.x86Op++;
}

void EMIT_OP_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1)
{
#ifdef NULLC_OPTIMIZE_X86
	switch(op)
	{
	case o_call:
		ctx.ReadRegister(reg1);

		ctx.KillUnreadRegisters();

		ctx.InvalidateState();
		break;
	case o_setl:
	case o_setg:
	case o_setle:
	case o_setge:
	case o_sete:
	case o_setne:
	case o_setz:
	case o_setnz:
		ctx.ReadAndModifyRegister(reg1); // Since instruction sets only lower bits, it 'reads' the rest
		break;
	case o_imul:
	case o_imul64:
		// TODO: check if direct operation with memory is possible

		ctx.ReadRegister(reg1);

		ctx.ReadAndModifyRegister(rEAX);
		ctx.OverwriteRegisterWithUnknown(rEDX); // Don't care about top bits here
		break;
	case o_idiv:
	case o_idiv64:
		// TODO: check if direct operation with memory is possible

		ctx.ReadRegister(reg1);

		ctx.ReadAndModifyRegister(rEAX);
		ctx.ReadAndModifyRegister(rEDX);

		// Implicitly read the result so when one of the registers is killed, it will not remove the instruction (there's room for improvement)
		ctx.ReadRegister(rEAX);
		ctx.ReadRegister(rEDX);
		break;
	case o_neg:
	case o_not:
	case o_neg64:
	case o_not64:
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_push:
		ctx.ReadRegister(reg1);
		break;
	case o_pop:
		ctx.OverwriteRegisterWithUnknown(reg1);
		break;
	case o_sal:
	case o_sar:
	case o_sal64:
	case o_sar64:
		ctx.ReadRegister(rECX);

		ctx.ReadAndModifyRegister(reg1);
		break;
	default:
		assert(!"unknown instruction");
	}
#else
	if(op == o_call)
		ctx.InvalidateState();
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP_NUM(CodeGenGenericContext &ctx, x86Command op, unsigned num)
{
#ifdef NULLC_OPTIMIZE_X86
	switch(op)
	{
	case o_push:
	case o_int:
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argNumber;
	ctx.x86Op->argA.num = num;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift)
{
#ifdef NULLC_OPTIMIZE_X86
	ctx.RedirectAddressComputation(index, multiplier, base, shift);

	// Register reads
	ctx.ReadRegister(base);
	ctx.ReadRegister(index);

	x86Argument arg = x86Argument(size, index, multiplier, base, shift);

	ctx.MemRead(arg);

	switch(op)
	{
	case o_push:
	case o_pop:
		break;
	case o_neg:
	case o_not:
	case o_neg64:
	case o_not64:
		ctx.InvalidateAddressValue(arg);

		ctx.MemWrite(arg, x86Argument());
		break;
	case o_idiv:
	case o_idiv64:
		ctx.ReadAndModifyRegister(rEAX);
		ctx.ReadAndModifyRegister(rEDX);

		// Implicitly read the result so when one of the registers is killed, it will not remove the instruction (there's room for improvement)
		ctx.ReadRegister(rEAX);
		ctx.ReadRegister(rEDX);
		break;
	case o_call:
		if(!ctx.skipInvalidate)
		{
			ctx.KillUnreadRegisters();

			ctx.InvalidateState();
		}
		ctx.skipInvalidate = false;
		break;
	default:
		assert(!"unknown instruction");
	}
#else
	if(op == o_call)
		ctx.InvalidateState();
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argPtr;
	ctx.x86Op->argA.ptrSize = size;
	ctx.x86Op->argA.ptrIndex = index;
	ctx.x86Op->argA.ptrMult = multiplier;
	ctx.x86Op->argA.ptrBase = base;
	ctx.x86Op->argA.ptrNum = shift;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg2, unsigned shift)
{
	EMIT_OP_RPTR(ctx, op, size, rNONE, 1, reg2, shift);
}

void EMIT_OP_ADDR(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned addr)
{
	EMIT_OP_RPTR(ctx, op, size, rNONE, 1, rNONE, addr);
}

void EMIT_OP_REG_NUM(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, unsigned num)
{
	if(op == o_movsx)
		op = o_mov;

#ifdef NULLC_OPTIMIZE_X86
	switch(op)
	{
	case o_mov:
		// Skip move if the target already contains the same number
		if(ctx.genReg[reg1].type == x86Argument::argNumber && ctx.genReg[reg1].num == int(num))
		{
			ctx.optimizationCount++;
			return;
		}

		ctx.OverwriteRegisterWithValue(reg1, x86Argument(num));
		break;
	case o_add:
	case o_sub:
	case o_add64:
	case o_sub64:
		// Stack frame setup
		if(reg1 == rESP)
			break;

		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_adc:
	case o_sbb:
	case o_imul:
	case o_and:
	case o_or:
	case o_xor:
	case o_shl:
	case o_sal:
	case o_sar:
	case o_imul64:
	case o_and64:
	case o_or64:
	case o_xor64:
	case o_sal64:
	case o_sar64:
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_cmp:
	case o_cmp64:
		ctx.ReadRegister(reg1);
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argNumber;
	ctx.x86Op->argB.num = num;
	ctx.x86Op++;
}

void EMIT_OP_REG_NUM64(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, unsigned long long num)
{
#ifdef NULLC_OPTIMIZE_X86
	switch(op)
	{
	case o_mov64:
		// Skip move if the target already contains the same number
		if(ctx.genReg[reg1].type == x86Argument::argImm64 && ctx.genReg[reg1].imm64Arg == num)
		{
			ctx.optimizationCount++;
			return;
		}

		ctx.OverwriteRegisterWithValue(reg1, x86Argument(num));
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argImm64;
	ctx.x86Op->argB.imm64Arg = num;
	ctx.x86Op++;
}

void EMIT_OP_REG_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Reg reg2)
{
#ifdef NULLC_OPTIMIZE_X86
	switch(op)
	{
	case o_xor:
	case o_xor64:
		if(reg1 == reg2)
		{
			EMIT_REG_KILL(ctx, reg1);
			ctx.InvalidateDependand(reg1);
		}
		else
		{
			reg2 = ctx.RedirectRegister(reg2);

			ctx.ReadRegister(reg2);
			ctx.ReadAndModifyRegister(reg1);
		}
		break;
	case o_cmp:
	case o_cmp64:
		// TODO: compare directly with memory, compare with immediate (x86)

		reg2 = ctx.RedirectRegister(reg2);

		ctx.ReadRegister(reg1);
		ctx.ReadRegister(reg2);
		break;
	case o_test:
		reg2 = ctx.RedirectRegister(reg2);

		ctx.ReadRegister(reg1);
		ctx.ReadRegister(reg2);
		break;
	case o_add:
	case o_sub:
		reg2 = ctx.RedirectRegister(reg2);

		if(ctx.genReg[reg2].type == x86Argument::argNumber)
		{
			EMIT_OP_REG_NUM(ctx, op, reg1, ctx.genReg[reg2].num);
			return;
		}

		// TODO: if there is a known number in destination, we can perform a lea

		ctx.ReadRegister(reg2);
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_adc:
	case o_sbb:
		reg2 = ctx.RedirectRegister(reg2);

		ctx.ReadRegister(reg2);
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_add64:
	case o_sub64:
		reg2 = ctx.RedirectRegister(reg2);

		ctx.ReadRegister(reg2);
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_sal:
	case o_sar:
	case o_sal64:
	case o_sar64:
		// Can't redirect source register since the instruction has it fixed to ecx

		ctx.ReadRegister(reg2);
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_and:
	case o_or:
		reg2 = ctx.RedirectRegister(reg2);

		// Load source directly from memory
		if(ctx.genReg[reg2].type == x86Argument::argPtr && ctx.genReg[reg2].ptrSize == sDWORD)
		{
			EMIT_OP_REG_RPTR(ctx, op, reg1, ctx.genReg[reg2].ptrSize, ctx.genReg[reg2].ptrIndex, ctx.genReg[reg2].ptrMult, ctx.genReg[reg2].ptrBase, ctx.genReg[reg2].ptrNum);
			return;
		}

		ctx.ReadRegister(reg2);
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_and64:
	case o_or64:
		reg2 = ctx.RedirectRegister(reg2);

		// Load source directly from memory
		if(ctx.genReg[reg2].type == x86Argument::argPtr && ctx.genReg[reg2].ptrSize == sQWORD)
		{
			EMIT_OP_REG_RPTR(ctx, op, reg1, ctx.genReg[reg2].ptrSize, ctx.genReg[reg2].ptrIndex, ctx.genReg[reg2].ptrMult, ctx.genReg[reg2].ptrBase, ctx.genReg[reg2].ptrNum);
			return;
		}

		ctx.ReadRegister(reg2);
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_imul:
		// Load source directly from memory
		if(ctx.genReg[reg2].type == x86Argument::argPtr && ctx.genReg[reg2].ptrSize == sDWORD)
		{
			EMIT_OP_REG_RPTR(ctx, op, reg1, ctx.genReg[reg2].ptrSize, ctx.genReg[reg2].ptrIndex, ctx.genReg[reg2].ptrMult, ctx.genReg[reg2].ptrBase, ctx.genReg[reg2].ptrNum);
			return;
		}

		ctx.ReadRegister(reg2);
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_imul64:
		// Load source directly from memory
		if(ctx.genReg[reg2].type == x86Argument::argPtr && ctx.genReg[reg2].ptrSize == sQWORD)
		{
			EMIT_OP_REG_RPTR(ctx, op, reg1, ctx.genReg[reg2].ptrSize, ctx.genReg[reg2].ptrIndex, ctx.genReg[reg2].ptrMult, ctx.genReg[reg2].ptrBase, ctx.genReg[reg2].ptrNum);
			return;
		}

		ctx.ReadRegister(reg2);
		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_mov:
	case o_mov64:
		reg2 = ctx.RedirectRegister(reg2);

		// Skip self-assignment
		if(reg1 == reg2)
		{
			ctx.optimizationCount++;
			return;
		}

		ctx.ReadRegister(reg2);

		ctx.OverwriteRegisterWithValue(reg1, x86Argument(reg2));
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argReg;
	ctx.x86Op->argB.reg = reg2;
	ctx.x86Op++;
}

void EMIT_OP_REG_REG(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86XmmReg reg2)
{
#ifdef NULLC_OPTIMIZE_X86
	switch(op)
	{
	case o_movsd:
		reg2 = ctx.RedirectRegister(reg2);

		// Skip self-assignment
		if(reg1 == reg2)
		{
			ctx.optimizationCount++;
			return;
		}

		ctx.ReadRegister(reg2);

		ctx.OverwriteRegisterWithValue(reg1, x86Argument(reg2));
		break;
	case o_addsd:
	case o_subsd:
	case o_mulsd:
	case o_divsd:
	case o_cmpeqsd:
	case o_cmpltsd:
	case o_cmplesd:
	case o_cmpneqsd:
		reg2 = ctx.RedirectRegister(reg2);

		ctx.ReadRegister(reg2);
		ctx.ReadAndModifyRegister(reg1);
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argXmmReg;
	ctx.x86Op->argA.xmmArg = reg1;
	ctx.x86Op->argB.type = x86Argument::argXmmReg;
	ctx.x86Op->argB.xmmArg = reg2;
	ctx.x86Op++;
}

void EMIT_OP_REG_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86XmmReg reg2)
{
#ifdef NULLC_OPTIMIZE_X86
	switch(op)
	{
	case o_movd:
		reg2 = ctx.RedirectRegister(reg2);

		ctx.ReadRegister(reg2);
		ctx.OverwriteRegisterWithValue(reg1, x86Argument(reg2));
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argXmmReg;
	ctx.x86Op->argB.xmmArg = reg2;
	ctx.x86Op++;
}

void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift)
{
#ifdef NULLC_OPTIMIZE_X86
	ctx.RedirectAddressComputation(index, multiplier, base, shift);

	// Register reads
	ctx.ReadRegister(base);
	ctx.ReadRegister(index);

	x86Argument address = x86Argument(size, index, multiplier, base, shift);

	switch(op)
	{
	case o_mov:
	case o_mov64:
		if(unsigned memIndex = ctx.MemFind(address))
		{
			memIndex--;

			if(ctx.memCache[memIndex].value.type == x86Argument::argReg)
			{
				EMIT_OP_REG_REG(ctx, op, reg1, ctx.memCache[memIndex].value.reg);
				ctx.MemUpdate(memIndex);
				return;
			}
			else if(ctx.memCache[memIndex].value.type == x86Argument::argNumber)
			{
				EMIT_OP_REG_NUM(ctx, op, reg1, ctx.memCache[memIndex].value.num);
				ctx.MemUpdate(memIndex);
				return;
			}
		}

		// If another register contains data from memory
		for(unsigned i = 0; i < rRegCount; i++)
		{
			if(ctx.genReg[i].type == x86Argument::argPtr && ctx.genReg[i] == address)
			{
				EMIT_OP_REG_REG(ctx, op, reg1, (x86Reg)i);
				return;
			}
		}

		ctx.MemRead(address);

		// If write doesn't invalidate the source registers, mark that register contains value from source address
		if(reg1 != base && reg1 != index)
			ctx.OverwriteRegisterWithValue(reg1, address);
		else
			ctx.OverwriteRegisterWithUnknown(reg1);
		break;
	case o_movsx:
	case o_movsxd:
		ctx.MemRead(address);

		ctx.OverwriteRegisterWithUnknown(reg1);
		break;
	case o_lea:
		ctx.OverwriteRegisterWithUnknown(reg1);
		break;
	case o_cvttsd2si:
	case o_cvttsd2si64:
	case o_add:
	case o_sub:
	case o_imul:
	case o_and:
	case o_or:
	case o_xor:
	case o_add64:
	case o_sub64:
	case o_imul64:
	case o_and64:
	case o_or64:
	case o_xor64:
		ctx.MemRead(address);

		ctx.ReadAndModifyRegister(reg1);
		break;
	case o_cmp:
	case o_cmp64:
		ctx.MemRead(address);

		ctx.ReadRegister(reg1);
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argPtr;
	ctx.x86Op->argB.ptrSize = size;
	ctx.x86Op->argB.ptrIndex = index;
	ctx.x86Op->argB.ptrMult = multiplier;
	ctx.x86Op->argB.ptrBase = base;
	ctx.x86Op->argB.ptrNum = shift;
	ctx.x86Op++;
}

void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift)
{
#ifdef NULLC_OPTIMIZE_X86
	x86Argument newArg = x86Argument(size, index, multiplier, base, shift);

	switch(op)
	{
	case o_cvtss2sd:
	case o_cvtsd2ss:
	case o_cvtsi2sd:
		// Register reads
		ctx.ReadRegister(base);
		ctx.ReadRegister(index);

		ctx.MemRead(x86Argument(size, index, multiplier, base, shift));

		ctx.OverwriteRegisterWithUnknown(reg1);
		break;
	case o_movss:
	case o_movsd:
		if(unsigned memIndex = ctx.MemFind(newArg))
		{
			memIndex--;

			if(ctx.memCache[memIndex].value.type == x86Argument::argXmmReg)
			{
				EMIT_OP_REG_REG(ctx, op, reg1, ctx.memCache[memIndex].value.xmmArg);
				ctx.MemUpdate(memIndex);
				return;
			}
		}

		// If another register contains data from memory
		for(unsigned i = 0; i < rXmmRegCount; i++)
		{
			if(ctx.xmmReg[i].type == x86Argument::argPtr && ctx.xmmReg[i] == newArg)
			{
				EMIT_OP_REG_REG(ctx, op, reg1, (x86XmmReg)i);
				return;
			}
		}

		// Register reads
		ctx.ReadRegister(base);
		ctx.ReadRegister(index);

		ctx.MemRead(x86Argument(size, index, multiplier, base, shift));

		ctx.OverwriteRegisterWithValue(reg1, newArg);
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argXmmReg;
	ctx.x86Op->argA.xmmArg = reg1;
	ctx.x86Op->argB.type = x86Argument::argPtr;
	ctx.x86Op->argB.ptrSize = size;
	ctx.x86Op->argB.ptrIndex = index;
	ctx.x86Op->argB.ptrMult = multiplier;
	ctx.x86Op->argB.ptrBase = base;
	ctx.x86Op->argB.ptrNum = shift;
	ctx.x86Op++;
}

void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, x86Reg reg2, unsigned shift)
{
	EMIT_OP_REG_RPTR(ctx, op, reg1, size, rNONE, 1, reg2, shift);
}

void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Size size, x86Reg reg2, unsigned shift)
{
	EMIT_OP_REG_RPTR(ctx, op, reg1, size, rNONE, 1, reg2, shift);
}

void EMIT_OP_REG_ADDR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, unsigned addr)
{
	EMIT_OP_REG_RPTR(ctx, op, reg1, size, rNONE, 1, rNONE, addr);
}

void EMIT_OP_REG_ADDR(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Size size, unsigned addr)
{
	EMIT_OP_REG_RPTR(ctx, op, reg1, size, rNONE, 1, rNONE, addr);
}

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift, x86Reg reg2)
{
#ifdef NULLC_OPTIMIZE_X86
	ctx.RedirectAddressComputation(index, multiplier, base, shift);

	reg2 = ctx.RedirectRegister(reg2);

	if(size == sDWORD && ctx.genReg[reg2].type == x86Argument::argNumber)
	{
		EMIT_OP_RPTR_NUM(ctx, op, size, index, multiplier, base, shift, ctx.genReg[reg2].num);
		return;
	}

	// Register reads
	ctx.ReadRegister(base);
	ctx.ReadRegister(index);
	ctx.ReadRegister(reg2);

	x86Argument arg = x86Argument(size, index, multiplier, base, shift);

	switch(op)
	{
	case o_mov:
	case o_mov64:
		assert(base != rESP);

		ctx.InvalidateAddressValue(arg);

		// If the source register value was unknown, now we know that it is stored at destination memory location
		// If the source register value contained a value from an address, now we know that it contains the value from the destination address
		if(ctx.genReg[reg2].type == x86Argument::argNone || ctx.genReg[reg2].type == x86Argument::argPtr)
			ctx.genReg[reg2] = arg;

		if(unsigned memIndex = ctx.MemFind(arg))
		{
			memIndex--;

			if(!ctx.memCache[memIndex].read)
			{
				// Remove dead store
				x86Instruction *curr = ctx.memCache[memIndex].location + ctx.x86Base;

				if(curr->name != o_none)
				{
					curr->name = o_none;
					ctx.optimizationCount++;
				}
			}
		}

		// Track target memory value
		ctx.MemWrite(arg, x86Argument(reg2));
		break;
	case o_add:
	case o_sub:
	case o_adc:
	case o_sbb:
	case o_and:
	case o_or:
	case o_xor:
	case o_add64:
	case o_sub64:
	case o_and64:
	case o_or64:
	case o_xor64:
		ctx.MemRead(arg);

		ctx.InvalidateAddressValue(arg);
		
		ctx.MemWrite(arg, x86Argument());
		break;
	case o_cmp:
	case o_cmp64:
		ctx.MemRead(arg);
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argPtr;
	ctx.x86Op->argA.ptrSize = size;
	ctx.x86Op->argA.ptrIndex = index;
	ctx.x86Op->argA.ptrMult = multiplier;
	ctx.x86Op->argA.ptrBase = base;
	ctx.x86Op->argA.ptrNum = shift;
	ctx.x86Op->argB.type = x86Argument::argReg;
	ctx.x86Op->argB.reg = reg2;
	ctx.x86Op++;
}

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift, x86XmmReg reg2)
{
#ifdef NULLC_OPTIMIZE_X86
	ctx.RedirectAddressComputation(index, multiplier, base, shift);

	reg2 = ctx.RedirectRegister(reg2);

	x86Argument arg = x86Argument(size, index, multiplier, base, shift);

	// Register reads
	ctx.ReadRegister(base);
	ctx.ReadRegister(index);
	ctx.ReadRegister(reg2);

	switch(op)
	{
	case o_movss:
	case o_movsd:
		// No tracking for stack
		if(base == rESP)
			break;

		ctx.InvalidateAddressValue(arg);

		// If the source register value was unknown, now we know that it is stored at destination memory location
		// If the source register value contained a value from an address, now we know that it contains the value from the destination address
		if(ctx.xmmReg[reg2].type == x86Argument::argNone || ctx.xmmReg[reg2].type == x86Argument::argPtr)
			ctx.xmmReg[reg2] = arg;

		if(unsigned memIndex = ctx.MemFind(arg))
		{
			memIndex--;

			if(!ctx.memCache[memIndex].read)
			{
				// Remove dead store
				x86Instruction *curr = ctx.memCache[memIndex].location + ctx.x86Base;

				if(curr->name != o_none)
				{
					curr->name = o_none;
					ctx.optimizationCount++;
				}
			}
		}

		// Track target memory value
		ctx.MemWrite(arg, x86Argument(reg2));
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argPtr;
	ctx.x86Op->argA.ptrSize = size;
	ctx.x86Op->argA.ptrIndex = index;
	ctx.x86Op->argA.ptrMult = multiplier;
	ctx.x86Op->argA.ptrBase = base;
	ctx.x86Op->argA.ptrNum = shift;
	ctx.x86Op->argB.type = x86Argument::argXmmReg;
	ctx.x86Op->argB.xmmArg = reg2;
	ctx.x86Op++;
}

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned shift, x86Reg reg2)
{
	EMIT_OP_RPTR_REG(ctx, op, size, rNONE, 1, reg1, shift, reg2);
}

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned shift, x86XmmReg reg2)
{
	EMIT_OP_RPTR_REG(ctx, op, size, rNONE, 1, reg1, shift, reg2);
}

void EMIT_OP_ADDR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned addr, x86Reg reg2)
{
	EMIT_OP_RPTR_REG(ctx, op, size, rNONE, 1, rNONE, addr, reg2);
}

void EMIT_OP_ADDR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned addr, x86XmmReg reg2)
{
	EMIT_OP_RPTR_REG(ctx, op, size, rNONE, 1, rNONE, addr, reg2);
}

void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned shift, unsigned num)
{
#ifdef NULLC_OPTIMIZE_X86
	ctx.RedirectAddressComputation(index, multiplier, base, shift);

	x86Argument arg = x86Argument(size, index, multiplier, base, shift);

	// Register reads
	ctx.ReadRegister(base);
	ctx.ReadRegister(index);

	switch(op)
	{
	case o_mov:
	case o_mov64:
		assert(base != rESP);

		ctx.InvalidateAddressValue(arg);

		if(unsigned memIndex = ctx.MemFind(arg))
		{
			memIndex--;

			if(!ctx.memCache[memIndex].read)
			{
				// Remove dead store
				x86Instruction *curr = ctx.memCache[memIndex].location + ctx.x86Base;

				if(curr->name != o_none)
				{
					curr->name = o_none;
					ctx.optimizationCount++;
				}
			}
		}

		// Track target memory value
		ctx.MemWrite(arg, x86Argument(num));
		break;
	case o_add:
	case o_sub:
	case o_adc:
	case o_sbb:
	case o_shl:
	case o_sal:
	case o_sar:
	case o_and:
	case o_or:
	case o_xor:
	case o_add64:
	case o_sub64:
	case o_sal64:
	case o_sar64:
	case o_and64:
	case o_or64:
	case o_xor64:
		ctx.MemRead(arg);

		ctx.InvalidateAddressValue(arg);

		ctx.MemWrite(arg, x86Argument());
		break;
	case o_cmp:
	case o_cmp64:
		ctx.MemRead(arg);
		break;
	default:
		assert(!"unknown instruction");
	}
#endif

	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argPtr;
	ctx.x86Op->argA.ptrSize = size;
	ctx.x86Op->argA.ptrIndex = index;
	ctx.x86Op->argA.ptrMult = multiplier;
	ctx.x86Op->argA.ptrBase = base;
	ctx.x86Op->argA.ptrNum = shift;
	ctx.x86Op->argB.type = x86Argument::argNumber;
	ctx.x86Op->argB.num = num;
	ctx.x86Op++;
}

void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned shift, unsigned num)
{
	EMIT_OP_RPTR_NUM(ctx, op, size, rNONE, 1, reg1, shift, num);
}

void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned addr, unsigned number)
{
	EMIT_OP_RPTR_NUM(ctx, op, size, rNONE, 1, rNONE, addr, number);
}

void EMIT_REG_READ(CodeGenGenericContext &ctx, x86Reg reg)
{
	(void)ctx;
	(void)reg;

#ifdef NULLC_OPTIMIZE_X86
	ctx.ReadRegister(reg);
#endif
}

void EMIT_REG_READ(CodeGenGenericContext &ctx, x86XmmReg reg)
{
	(void)ctx;
	(void)reg;

#ifdef NULLC_OPTIMIZE_X86
	ctx.ReadRegister(reg);
#endif
}

void EMIT_REG_KILL(CodeGenGenericContext &ctx, x86Reg reg)
{
#ifdef NULLC_OPTIMIZE_X86
	// Eliminate dead stores to the register
	if(!ctx.genRegRead[reg])
	{
		x86Instruction *curr = ctx.genRegUpdate[reg] + ctx.x86Base;

		if(curr->name != o_none)
		{
			curr->name = o_none;
			ctx.optimizationCount++;
		}
	}

	// Invalidate the register value
	ctx.genReg[reg].type = x86Argument::argNone;
#else
	(void)ctx;
	(void)reg;
#endif
}

void EMIT_REG_KILL(CodeGenGenericContext &ctx, x86XmmReg reg)
{
#ifdef NULLC_OPTIMIZE_X86
	// Eliminate dead stores to the register
	if(!ctx.xmmRegRead[reg])
	{
		x86Instruction *curr = ctx.xmmRegUpdate[reg] + ctx.x86Base;

		if(curr->name != o_none)
		{
			curr->name = o_none;
			ctx.optimizationCount++;
		}
	}

	// Invalidate the register value
	ctx.xmmReg[reg].type = x86Argument::argNone;
#else
	(void)ctx;
	(void)reg;
#endif
}

void SetOptimizationLookBehind(CodeGenGenericContext &ctx, bool allow)
{
	ctx.x86LookBehind = allow;

#ifdef NULLC_OPTIMIZE_X86
	if(!allow)
	{
		ctx.KillUnreadRegisters();

		ctx.lastInvalidate = unsigned(ctx.x86Op - ctx.x86Base);
		ctx.InvalidateState();

		ctx.genRegUpdate[rESP] = 0;
	}
#endif
}

#endif
