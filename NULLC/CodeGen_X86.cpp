#include "CodeGen_X86.h"

#ifdef NULLC_BUILD_X86_JIT

#include "StdLib_X86.h"
#include "Executor_Common.h"
#include "StdLib.h"

#ifdef NULLC_OPTIMIZE_X86
namespace NULLC
{
	const unsigned int STACK_STATE_SIZE = 8;
	x86Argument		stack[STACK_STATE_SIZE];
	unsigned int	stackUpdate[STACK_STATE_SIZE];
	bool			stackRead[STACK_STATE_SIZE];
	unsigned int	stackTop = 0;

	unsigned int	lastInvalidate = 0;

	x86Argument		reg[9];			// Holds current register value
	unsigned int	regUpdate[9];	// Marks the last instruction that wrote to the register
	bool			regRead[9];		// Marks if there was a read from the register after last write

	struct MemCache
	{
		x86Argument	address;
		x86Argument value;
	};
	const unsigned int MEMORY_STATE_SIZE = 16;
	MemCache	mCache[MEMORY_STATE_SIZE];
	unsigned int	mCacheEntries = 0;

	unsigned MemFind(const x86Argument &address)
	{
		for(unsigned int i = 0; i < MEMORY_STATE_SIZE; i++)
		{
			if(mCache[i].value.type != x86Argument::argNone && mCache[i].address.type != x86Argument::argNone &&
				mCache[i].address.ptrSize == address.ptrSize && mCache[i].address.ptrNum == address.ptrNum &&
				mCache[i].address.ptrBase == address.ptrBase && mCache[i].address.ptrIndex == address.ptrIndex)
			{
				return i;
			}
		}
		return ~0u;
	}

	void MemWrite(const x86Argument &address, const x86Argument &value)
	{
		unsigned int index = MemFind(address);
		if(index != ~0u)
		{
			if(index != 0)
			{
				MemCache tmp = mCache[index-1];
				mCache[index-1] = mCache[index];
				mCache[index] = tmp;
				mCache[index-1].value = value;
			}else{
				mCache[0].value = value;
			}
		}else{
			unsigned int newIndex = mCacheEntries < MEMORY_STATE_SIZE ? mCacheEntries : MEMORY_STATE_SIZE - 1;
			if(mCacheEntries < MEMORY_STATE_SIZE)
				mCacheEntries++;
			else
				mCacheEntries = MEMORY_STATE_SIZE >> 1;	// Wrap to the middle

			mCache[newIndex].address = address;
			mCache[newIndex].value = value;
		}
	}

	void MemUpdate(unsigned int index)
	{
		(void)index;
		if(index != 0)
		{
			MemCache tmp = mCache[index-1];
			mCache[index-1] = mCache[index];
			mCache[index] = tmp;
		}
	}

	void InvalidateState()
	{
		stackTop = 0;
		for(unsigned int i = 0; i < STACK_STATE_SIZE; i++)
			stack[i].type = x86Argument::argNone;
		for(unsigned int i = 0; i < 9; i++)
			reg[i].type = x86Argument::argNone;
		for(unsigned int i = 0; i < MEMORY_STATE_SIZE; i++)
			mCache[i].address.type = x86Argument::argNone;
		mCacheEntries = 0;
	}

	void InvalidateDependand(x86Reg dreg)
	{
		for(unsigned int i = 0; i < NULLC::STACK_STATE_SIZE; i++)
		{
			if(stack[i].type == x86Argument::argReg && stack[i].reg == dreg)
				stack[i].type = x86Argument::argNone;
			if(stack[i].type == x86Argument::argPtr && (stack[i].ptrBase == dreg || stack[i].ptrIndex == dreg))
				stack[i].type = x86Argument::argNone;
		}
		for(unsigned int i = 0; i < 9; i++)
		{
			if(reg[i].type == x86Argument::argReg && reg[i].reg == dreg)
				reg[i].type = x86Argument::argPtrLabel;
			if(reg[i].type == x86Argument::argPtr && (reg[i].ptrBase == dreg || reg[i].ptrIndex == dreg))
				reg[i].type = x86Argument::argPtrLabel;
		}
		for(unsigned int i = 0; i < NULLC::MEMORY_STATE_SIZE; i++)
		{
			if(mCache[i].address.type == x86Argument::argReg && mCache[i].address.reg == dreg)
				mCache[i].address.type = x86Argument::argNone;
			if(mCache[i].address.type == x86Argument::argPtr && (mCache[i].address.ptrBase == dreg || mCache[i].address.ptrIndex == dreg))
				mCache[i].address.type = x86Argument::argNone;
			if(mCache[i].value.type == x86Argument::argReg && mCache[i].value.reg == dreg)
				mCache[i].value.type = x86Argument::argNone;
			if(mCache[i].value.type == x86Argument::argPtr && (mCache[i].value.ptrBase == dreg || mCache[i].value.ptrIndex == dreg))
				mCache[i].value.type = x86Argument::argNone;
		}
	}
}
#endif

void EMIT_COMMENT(CodeGenGenericContext &ctx, const char* text)
{
#if !defined(NDEBUG)
	ctx.x86Op->name = o_other;
	ctx.x86Op->comment = text;
	ctx.x86Op++;
#else
	(void)text;
#endif
}

void EMIT_LABEL(CodeGenGenericContext &ctx, unsigned int labelID, int invalidate)
{
#ifdef NULLC_OPTIMIZE_X86
	if(invalidate)
		NULLC::InvalidateState();
#else
	(void)invalidate;
#endif
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
	if(op >= o_jmp && op <= o_ret)
		NULLC::InvalidateState();
	if(op == o_ret)
	{
		NULLC::regRead[rEAX] = NULLC::regRead[rEBX] = NULLC::regRead[rECX] = NULLC::regRead[rEDX] = true;
	}else if(op == o_rep_movsd){
		NULLC::regRead[rECX] = NULLC::regRead[rESI] = NULLC::regRead[rEDI] = true;

		assert(NULLC::reg[rECX].type == x86Argument::argNumber);

		NULLC::InvalidateState();
	}else if(op == o_cdq){
		if(NULLC::reg[rEAX].type == x86Argument::argNumber)
		{
			EMIT_OP_REG_NUM(o_mov, rEDX, NULLC::reg[rEAX].num >= 0 ? 0 : ~0u);
			return;
		}
		NULLC::regRead[rEAX] = true;
		EMIT_REG_KILL(ctx, rEDX);
		NULLC::InvalidateDependand(rEDX);
		NULLC::reg[rEDX].type = x86Argument::argNone;
		NULLC::regRead[rEDX] = false;
		NULLC::regUpdate[rEDX] = (unsigned int)(ctx.x86Op - x86Base);
	}
#endif
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argNone;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP_LABEL(CodeGenGenericContext &ctx, x86Command op, unsigned int labelID, int invalidate, int longJump)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op == o_call)
	{
		EMIT_REG_KILL(ctx, rEAX);
		EMIT_REG_KILL(ctx, rEBX);
		EMIT_REG_KILL(ctx, rECX);
		EMIT_REG_KILL(ctx, rEDX);
		EMIT_REG_KILL(ctx, rESI);
	}
	if(op >= o_jmp && op <= o_ret && invalidate)
	{
		if(longJump)
		{
			EMIT_REG_KILL(ctx, rEAX);
			EMIT_REG_KILL(ctx, rEBX);
			EMIT_REG_KILL(ctx, rECX);
			EMIT_REG_KILL(ctx, rEDX);
			EMIT_REG_KILL(ctx, rESI);
		}
		NULLC::InvalidateState();
	}

	if((ctx.x86Op - x86Base - 2) > (int)NULLC::lastInvalidate && ctx.x86Op[-2].name >= o_setl && ctx.x86Op[-2].name <= o_setnz && ctx.x86Op[-1].name == o_test &&
		ctx.x86Op[-2].argA.reg == ctx.x86Op[-1].argA.reg && ctx.x86Op[-1].argA.reg == ctx.x86Op[-1].argB.reg)
	{
		if(op == o_jz)
		{
			static const x86Command jump[] = { o_jge, o_jle, o_jg, o_jl, o_jne, o_je, o_jnz, o_jz };
			x86Command curr = jump[ctx.x86Op[-2].name - o_setl];
			if(ctx.x86Op[-4].name == o_xor && ctx.x86Op[-4].argA.reg == ctx.x86Op[-4].argB.reg)
				ctx.x86Op[-4].name = o_none;
			ctx.x86Op[-2].name = o_none;
			ctx.x86Op[-1].name = o_none;
			optiCount += 2;
			EMIT_OP_LABEL(curr, labelID, invalidate, longJump);
			return;
		}else if(op == o_jnz){
			static const x86Command jump[] = { o_jl, o_jg, o_jle, o_jge, o_je, o_jne, o_jz, o_jnz };
			x86Command curr = jump[ctx.x86Op[-2].name - o_setl];
			if(ctx.x86Op[-4].name == o_xor && ctx.x86Op[-4].argA.reg == ctx.x86Op[-4].argB.reg)
				ctx.x86Op[-4].name = o_none;
			ctx.x86Op[-2].name = o_none;
			ctx.x86Op[-1].name = o_none;
			optiCount += 2;
			EMIT_OP_LABEL(curr, labelID, invalidate, longJump);
			return;
		}
	}
#else
	(void)invalidate;
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

void EMIT_CALL_REG(CodeGenGenericContext &ctx, x86Reg reg1)
{
#ifdef NULLC_OPTIMIZE_X86
	NULLC::regRead[reg1] = true;
	NULLC::reg[rECX].type = x86Argument::argNone;
#endif
	ctx.x86Op->name = o_call;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op++;
}

void EMIT_OP_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op == o_neg && NULLC::reg[reg1].type == x86Argument::argNumber)
	{
		EMIT_OP_REG_NUM(o_mov, reg1, -NULLC::reg[reg1].num);
		return;
	}
	if(op == o_not && NULLC::reg[reg1].type == x86Argument::argNumber)
	{
		EMIT_OP_REG_NUM(o_mov, reg1, ~NULLC::reg[reg1].num);
		return;
	}
	if(op == o_idiv && NULLC::reg[rEAX].type == x86Argument::argNumber &&
		NULLC::reg[reg1].type == x86Argument::argNumber && NULLC::reg[reg1].num)
	{
		EMIT_OP_REG_NUM(o_mov, rEDX, NULLC::reg[rEAX].num % NULLC::reg[reg1].num);
		EMIT_OP_REG_NUM(o_mov, rEAX, NULLC::reg[rEAX].num / NULLC::reg[reg1].num);
		return;
	}
	if(op == o_call)
	{
		if(reg1 != rEAX)
			EMIT_REG_KILL(ctx, rEAX);
		if(reg1 != rEBX)
			EMIT_REG_KILL(ctx, rEBX);
		if(reg1 != rECX)
			EMIT_REG_KILL(ctx, rECX);
		if(reg1 != rEDX)
			EMIT_REG_KILL(ctx, rEDX);
		NULLC::InvalidateState();
	}
	if(op == o_push)
	{
		if(NULLC::reg[reg1].type == x86Argument::argNumber)
		{
			EMIT_OP_NUM(op, NULLC::reg[reg1].num);
			return;
		}else if(NULLC::reg[reg1].type == x86Argument::argReg){
			EMIT_OP_REG(op, NULLC::reg[reg1].reg);
			return;
		}
		unsigned int index = (++NULLC::stackTop) % NULLC::STACK_STATE_SIZE;
		NULLC::stack[index] = x86Argument(reg1);
		NULLC::stackRead[index] = false;
		NULLC::stackUpdate[index] = (unsigned int)(ctx.x86Op - x86Base);

		NULLC::InvalidateDependand(rESP);
	}else{
		if(op == o_pop)
		{
			unsigned int index = (16 + NULLC::stackTop) % NULLC::STACK_STATE_SIZE;

			if(NULLC::reg[reg1].type != x86Argument::argNone && NULLC::reg[reg1] == NULLC::stack[index])
			{
				EMIT_OP_REG_NUM(o_add, rESP, 4);
				optiCount++;
				return;
			}

			x86Argument &target = NULLC::stack[index];
			if(target.type == x86Argument::argPtr)
			{
				EMIT_OP_REG_NUM(o_add, rESP, 4);
				EMIT_OP_REG_RPTR(o_mov, reg1, target.ptrSize, target.ptrIndex, target.ptrMult, target.ptrBase, target.ptrNum);
				return;
			}
			if(target.type == x86Argument::argReg)
			{
				if(reg1 != target.reg)
					EMIT_OP_REG_REG(o_mov, reg1, target.reg);
				else
					optiCount++;
				EMIT_OP_REG_NUM(o_add, rESP, 4);
				return;
			}
			if(target.type == x86Argument::argNumber)
			{
				EMIT_OP_REG_NUM(o_mov, reg1, target.num);
				EMIT_OP_REG_NUM(o_add, rESP, 4);
				return;
			}
			EMIT_REG_KILL(ctx, reg1);
			NULLC::reg[reg1] = NULLC::stack[index];
			NULLC::stackTop--;
			NULLC::stack[index].type = x86Argument::argNone;

			NULLC::InvalidateDependand(rESP);
		}
		NULLC::InvalidateDependand(reg1);
	}
	if(op == o_imul || op == o_idiv)
	{
		NULLC::InvalidateDependand(rEAX);
		NULLC::reg[rEAX].type = x86Argument::argNone;
		NULLC::InvalidateDependand(rEDX);
		NULLC::reg[rEDX].type = x86Argument::argNone;
		if(op == o_idiv && NULLC::reg[reg1].type == x86Argument::argPtr)
		{
			EMIT_OP_RPTR(o_idiv, NULLC::reg[reg1].ptrSize, NULLC::reg[reg1].ptrBase, NULLC::reg[reg1].ptrNum);
			return;
		}
	}else{
		NULLC::regUpdate[reg1] = (unsigned int)(ctx.x86Op - x86Base);
	}
	if(op == o_neg || op == o_not)
		NULLC::reg[reg1].type = x86Argument::argNone;
	NULLC::regRead[reg1] = (op == o_push || op == o_imul || op == o_idiv || op == o_neg || op == o_not || op == o_nop);

#ifdef _DEBUG
	if(op != o_push && op != o_pop && op != o_call && op != o_imul && op < o_setl && op > o_setnz)
		assert(!"invalid instruction");
#endif

#endif
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP_FPUREG(CodeGenGenericContext &ctx, x86Command op, x87Reg reg1)
{
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argFpReg;
	ctx.x86Op->argA.fpArg = reg1;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP_NUM(CodeGenGenericContext &ctx, x86Command op, unsigned int num)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op == o_push)
	{
		unsigned int index = (++NULLC::stackTop) % NULLC::STACK_STATE_SIZE;
		NULLC::stack[index] = x86Argument(num);
		NULLC::stackRead[index] = false;
		NULLC::stackUpdate[index] = (unsigned int)(ctx.x86Op - x86Base);

		NULLC::InvalidateDependand(rESP);
	}
	if(op == o_int)
		NULLC::regRead[rECX] = true;

#ifdef _DEBUG
	if(op != o_push && op != o_int)
		assert(!"invalid instruction");
#endif

#endif
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argNumber;
	ctx.x86Op->argA.num = num;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift)
{
#ifdef NULLC_OPTIMIZE_X86
	if(NULLC::reg[base].type == x86Argument::argReg)
		base = NULLC::reg[base].reg;
	if(op != o_push && index == rNONE && NULLC::reg[base].type == x86Argument::argPtr && NULLC::reg[base].ptrSize == sNONE)
	{
		EMIT_OP_RPTR(op, size, NULLC::reg[base].ptrIndex, NULLC::reg[base].ptrMult, NULLC::reg[base].ptrBase, NULLC::reg[base].ptrNum + shift);
		return;
	}

	if(op == o_push)
	{
		unsigned int sIndex  = (++NULLC::stackTop) % NULLC::STACK_STATE_SIZE;
		NULLC::stack[sIndex] = x86Argument(size, index, mult, base, shift);
		NULLC::stackRead[sIndex] = false;
		NULLC::stackUpdate[sIndex] = (unsigned int)(ctx.x86Op - x86Base);

		NULLC::InvalidateDependand(rESP);
	}else if(op == o_pop){

		unsigned int sIndex = (NULLC::stackTop) % NULLC::STACK_STATE_SIZE;
		x86Argument &target = NULLC::stack[sIndex];
		if(target.type == x86Argument::argPtr)
		{
			unsigned int cIndex = NULLC::MemFind(target);
			if(cIndex != ~0u)
			{
				if(NULLC::mCache[cIndex].value.type == x86Argument::argNumber)
				{
					EMIT_OP_RPTR_NUM(o_mov, size, index, mult, base, shift, NULLC::mCache[cIndex].value.num);
					EMIT_OP_REG_NUM(o_add, rESP, 4);
					NULLC::MemUpdate(cIndex);
					return;
				}
				if(NULLC::mCache[cIndex].value.type == x86Argument::argReg && op != o_imul && op != o_cmp)
				{
					EMIT_OP_RPTR_REG(o_mov, size, index, mult, base, shift, NULLC::mCache[cIndex].value.reg);
					EMIT_OP_REG_NUM(o_add, rESP, 4);
					NULLC::MemUpdate(cIndex);
					return;
				}
			}
		}
		if(target.type == x86Argument::argReg)
		{
			EMIT_OP_RPTR_REG(o_mov, size, index, mult, base, shift, target.reg);
			EMIT_OP_REG_NUM(o_add, rESP, 4);
			return;
		}
		if(target.type == x86Argument::argNumber)
		{
			EMIT_OP_RPTR_NUM(o_mov, size, index, mult, base, shift, target.num);
			EMIT_OP_REG_NUM(o_add, rESP, 4);
			return;
		}
		NULLC::stackTop--;
		NULLC::stack[sIndex].type = x86Argument::argNone;

		NULLC::InvalidateDependand(rESP);
	}else if(size == sDWORD && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4)){

		if(x86LookBehind && op == o_fstp && size == sDWORD && shift == 0 &&
			ctx.x86Op[-1].name == o_sub && ctx.x86Op[-1].argA.reg == rESP && ctx.x86Op[-1].argB.num == 4 &&
			ctx.x86Op[-2].name == o_fld && ctx.x86Op[-2].argA.ptrSize == sDWORD)
		{
			x86Instruction &fld = ctx.x86Op[-2];
			EMIT_OP_REG_NUM(o_add, rESP, 4);
			EMIT_OP_RPTR(o_push, sDWORD, fld.argA.ptrIndex, fld.argA.ptrMult, fld.argA.ptrBase, fld.argA.ptrNum);
			fld.name = o_none;
			return;
		}
		x86Argument &target = NULLC::stack[(16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE];
		target.type = x86Argument::argNone;
	}else if((op == o_fld || op == o_fild || op == o_fadd || op == o_fsub || op == o_fmul || op == o_fdiv || op == o_fcomp) && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4)){
		if(x86LookBehind && op == o_fld && ctx.x86Op[-1].name == o_fstp && ctx.x86Op[-1].argA.ptrSize == size && ctx.x86Op[-1].argA.ptrBase == base && ctx.x86Op[-1].argA.ptrNum == (int)shift)
		{
			ctx.x86Op[-1].name = o_fst;
			optiCount++;
			return;
		}
		if(x86LookBehind && (op == o_fadd || op == o_fsub || op == o_fmul || op == o_fdiv) && ctx.x86Op[-2].name == o_fstp && shift == 0 && ctx.x86Op[-2].argA.ptrNum == 0)
		{
			ctx.x86Op[-2].name = o_fst;
			if(op == o_fadd)
				EMIT_OP(o_faddp);
			else if(op == o_fsub)
				EMIT_OP(o_fsubrp);
			else if(op == o_fmul)
				EMIT_OP(o_fmulp);
			else if(op == o_fdiv)
				EMIT_OP(o_fdivrp);
			return;
		}
		unsigned int sIndex1 = (16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE;
		unsigned int sIndex2 = (16 + NULLC::stackTop - (shift >> 2) - 1) % NULLC::STACK_STATE_SIZE;

		if(op == o_fld && size == sQWORD && NULLC::stack[sIndex1].type == x86Argument::argNumber && NULLC::stack[sIndex2].type == x86Argument::argNumber)
		{
			if(NULLC::stack[sIndex1].num == 0 && NULLC::stack[sIndex2].num == 0)
			{
				EMIT_OP(o_fldz);
				return;
			}
			if(NULLC::stack[sIndex1].num == 0 && NULLC::stack[sIndex2].num == 1072693248)
			{
				EMIT_OP(o_fld1);
				return;
			}
		}
		if(NULLC::stack[sIndex1].type == x86Argument::argPtr)
		{
			if(NULLC::stack[sIndex1].ptrBase == rNONE)
			{
				EMIT_OP_ADDR(op, size, NULLC::stack[sIndex1].ptrNum);
				return;
			}
			index = NULLC::stack[sIndex1].ptrIndex;
			mult = NULLC::stack[sIndex1].ptrMult;
			base = NULLC::stack[sIndex1].ptrBase;
			shift = NULLC::stack[sIndex1].ptrNum;
		}else{
			NULLC::stackRead[sIndex1] = true;
			if(size == sQWORD)
				NULLC::stackRead[sIndex2] = true;
		}
	}else if((op == o_fstp || op == o_fst || op == o_fistp) && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4)){
		unsigned int target = (16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE;
		NULLC::stack[target].type = op == o_fistp ? x86Argument::argPtrLabel : x86Argument::argFPReg;
		NULLC::stackRead[target] = false;
		NULLC::stackUpdate[target] = (unsigned int)(ctx.x86Op - x86Base);

		if(size == sQWORD)
		{
			target = (16 + NULLC::stackTop - (shift >> 2) - 1) % NULLC::STACK_STATE_SIZE;
			NULLC::stack[target].type = op == o_fistp ? x86Argument::argPtrLabel : x86Argument::argFPReg;
			NULLC::stackRead[target] = false;
			NULLC::stackUpdate[target] = (unsigned int)(ctx.x86Op - x86Base);
		}
	}else if(op == o_idiv || op == o_imul){
		// still invalidate eax and edx
		NULLC::InvalidateDependand(rEAX);
		NULLC::reg[rEAX].type = x86Argument::argNone;
		NULLC::InvalidateDependand(rEDX);
		NULLC::reg[rEDX].type = x86Argument::argNone;
	}
	
	NULLC::regRead[index] = true;
	NULLC::regRead[base] = true;

	if(op == o_call)
	{
		EMIT_REG_KILL(ctx, rEAX);
		EMIT_REG_KILL(ctx, rEBX);
		EMIT_REG_KILL(ctx, rECX);
		EMIT_REG_KILL(ctx, rEDX);
		EMIT_REG_KILL(ctx, rESI);
		NULLC::InvalidateState();
	}

#ifdef _DEBUG
	if(op != o_push && op != o_pop && op != o_neg && op != o_not && op != o_idiv && op != o_fstp && op != o_fld && op != o_fadd && op != o_fsub && op != o_fmul && op != o_fdiv && op != o_fcomp && op != o_fild && op != o_fistp && op != o_fst && op != o_call && op != o_jmp)
		assert(!"invalid instruction");
#endif

#endif
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argPtr;
	ctx.x86Op->argA.ptrSize = size;
	ctx.x86Op->argA.ptrIndex = index;
	ctx.x86Op->argA.ptrMult = mult;
	ctx.x86Op->argA.ptrBase = base;
	ctx.x86Op->argA.ptrNum = shift;
	ctx.x86Op->argB.type = x86Argument::argNone;
	ctx.x86Op++;
}

void EMIT_OP_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg2, unsigned int shift)
{
	EMIT_OP_RPTR(ctx, op, size, rNONE, 1, reg2, shift);
}

void EMIT_OP_ADDR(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned int addr)
{
	EMIT_OP_RPTR(ctx, op, size, rNONE, 1, rNONE, addr);
}

void EMIT_OP_REG_NUM(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, unsigned int num)
{
	if(op == o_movsx)
		op = o_mov;
#ifdef NULLC_OPTIMIZE_X86
	if(op == o_add && NULLC::reg[reg1].type == x86Argument::argNumber)
	{
		EMIT_OP_REG_NUM(o_mov, reg1, NULLC::reg[reg1].num + (int)num);
		return;
	}
	if(op == o_sub && NULLC::reg[reg1].type == x86Argument::argNumber)
	{
		EMIT_OP_REG_NUM(o_mov, reg1, NULLC::reg[reg1].num - (int)num);
		return;
	}
	if(op == o_sub && (int)num < 0)
	{
		op = o_add;
		num = -(int)num;
	}
	if(op == o_add && (int)num < 0)
	{
		op = o_sub;
		num = -(int)num;
	}

	if(op == o_add && reg1 == rESP)
	{
		unsigned int bytes = num >> 2;
		unsigned int removed = 0;

		while(bytes--)
		{
			unsigned int index = (NULLC::stackTop--) % NULLC::STACK_STATE_SIZE;

			if(!NULLC::stackRead[index] && NULLC::stack[index].type != x86Argument::argNone)
			{
				x86Instruction *curr = NULLC::stackUpdate[index] + x86Base;
				
				int pos = 0;
				if(curr->name == o_push)
				{
					removed += 4;
					x86Instruction *start = curr;
					while(++start < ctx.x86Op)
					{
						if(start->name == o_none)
							continue;
						if(start->name == o_push)
							pos += 4;
						if(start->name == o_pop)
							pos -= 4;
						if(start->name == o_add && start->argA.reg == rESP)
						{
							pos -= start->argB.num;
							continue;
						}
						if(start->name == o_sub && start->argA.reg == rESP)
						{
							pos += start->argB.num;
							continue;
						}
						// if esp was used directly, signal an error
						assert(start->argA.type != x86Argument::argReg || start->argA.reg != rESP);
						assert(start->argB.type != x86Argument::argReg || start->argB.reg != rESP);
						
						if(start->argA.type == x86Argument::argPtr && start->argA.ptrBase == rESP && start->argA.ptrNum >= pos)
							start->argA.ptrNum -= 4;
						if(start->argB.type == x86Argument::argPtr && start->argB.ptrBase == rESP && start->argB.ptrNum >= pos)
							start->argB.ptrNum -= 4;
					}
				}
				if(curr->name != o_fstp)
				{
					curr->name = o_none;
				}else{
					curr->argA = x86Argument(rST0);
				}
				optiCount++;
			}
			NULLC::stack[index].type = x86Argument::argNone;

			NULLC::InvalidateDependand(rESP);
		}
		num -= removed;
		if(!num)
		{
			optiCount++;
			return;
		}
	}
	if(op == o_sub && reg1 == rESP)
	{
		unsigned int bytes = num >> 2;
		while(bytes--)
			NULLC::stack[(++NULLC::stackTop) % NULLC::STACK_STATE_SIZE].type = x86Argument::argNone;

		NULLC::InvalidateDependand(rESP);
	}

	if((op == o_add || op == o_sub) && reg1 == rESP)
	{
		x86Instruction &prev = x86Base[NULLC::regUpdate[rESP]];
		if((prev.name == o_add || prev.name == o_sub) && prev.argB.type == x86Argument::argNumber && NULLC::regUpdate[rESP] > NULLC::lastInvalidate)
		{
			x86Instruction *curr = &prev;
			bool safe = true;
			while(++curr < ctx.x86Op && safe)
			{
				if(curr->name == o_none)
					continue;
				if(curr->name == o_pop || curr->name == o_push || curr->name == o_label)
					safe = false;
				if(curr->name >= o_jmp && curr->name <= o_ret)
					safe = false;
				if(curr->argA.type == x86Argument::argPtr && curr->argA.ptrBase == rESP)
					safe = false;
				if(curr->argB.type == x86Argument::argPtr && curr->argB.ptrBase == rESP)
					safe = false;
				if(curr->argA.type == x86Argument::argReg && curr->argA.reg == rESP)
					safe = false;
				if(curr->argB.type == x86Argument::argReg && curr->argB.reg == rESP)
					safe = false;
			}
			if(safe)
			{
				prev.argB.num = op == o_add ? (prev.name == o_sub ? prev.argB.num - num : prev.argB.num + num) : (prev.name == o_add ? prev.argB.num - num : prev.argB.num + num);
				optiCount++;
				if(prev.argB.num == 0)
				{
					prev.name = o_none;
					optiCount++;
				}
				return;
			}
		}
		NULLC::regUpdate[rESP] = (unsigned int)(ctx.x86Op - x86Base);

		NULLC::InvalidateDependand(rESP);
	}else if(op == o_mov){
		if(NULLC::reg[reg1].type == x86Argument::argNumber && NULLC::reg[reg1].num == (int)num)
		{
			optiCount++;
			return;
		}
		EMIT_REG_KILL(ctx, reg1);
		NULLC::InvalidateDependand(reg1);
		NULLC::reg[reg1] = x86Argument(num);
		NULLC::regUpdate[reg1] = (unsigned int)(ctx.x86Op - x86Base);
		NULLC::regRead[reg1] = false;
	}else if(op == o_cmp){
		if(NULLC::reg[reg1].type == x86Argument::argReg)
			reg1 = NULLC::reg[reg1].reg;
		NULLC::regRead[reg1] = true;
	}else{
		NULLC::InvalidateDependand(reg1);
		NULLC::reg[reg1].type = x86Argument::argNone;
	}

#ifdef _DEBUG
	if(op != o_add && op != o_sub && op != o_mov && op != o_test && op != o_imul && op != o_shl && op != o_cmp)
		assert(!"invalid instruction");
#endif

#endif
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argNumber;
	ctx.x86Op->argB.num = num;
	ctx.x86Op++;
}

void EMIT_OP_REG_NUM64(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, uintptr_t num)
{
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
	if(op == o_xor && reg1 == reg2)
	{
		EMIT_REG_KILL(ctx, reg1);
		NULLC::InvalidateDependand(reg1);
	}else if(op == o_test && reg1 == reg2 && NULLC::reg[reg1].type == x86Argument::argReg){
		reg1 = reg2 = NULLC::reg[reg1].reg;
	}else if(op != o_sal && op != o_sar){
		if(NULLC::reg[reg2].type == x86Argument::argReg)
			reg2 = NULLC::reg[reg2].reg;
	}

	if(op == o_mov || op == o_movsx)
	{
		if(reg1 == reg2)
		{
			optiCount++;
			return;
		}
		EMIT_REG_KILL(ctx, reg1);
		NULLC::InvalidateDependand(reg1);

		NULLC::reg[reg1] = x86Argument(reg2);
		NULLC::regUpdate[reg1] = (unsigned int)(ctx.x86Op - x86Base);
		NULLC::regRead[reg1] = false;
	}else if((op == o_add || op == o_sub) && NULLC::reg[reg2].type == x86Argument::argNumber){
		EMIT_OP_REG_NUM(op, reg1, NULLC::reg[reg2].num);
		return;
	}else if(op == o_add && NULLC::reg[reg1].type == x86Argument::argNumber){
		EMIT_OP_REG_RPTR(o_lea, reg1, sNONE, rNONE, 1, reg2, NULLC::reg[reg1].num);
		return;
	}else if(op == o_cmp){
		if(NULLC::reg[reg2].type == x86Argument::argPtr && NULLC::reg[reg2].ptrSize == sDWORD)
		{
			EMIT_OP_REG_RPTR(o_cmp, reg1, sDWORD, NULLC::reg[reg2].ptrIndex, NULLC::reg[reg2].ptrMult, NULLC::reg[reg2].ptrBase, NULLC::reg[reg2].ptrNum);
			return;
		}
		if(NULLC::reg[reg1].type == x86Argument::argPtr && NULLC::reg[reg1].ptrSize == sDWORD)
		{
			EMIT_OP_RPTR_REG(o_cmp, sDWORD, NULLC::reg[reg1].ptrIndex, NULLC::reg[reg1].ptrMult, NULLC::reg[reg1].ptrBase, NULLC::reg[reg1].ptrNum, reg2);
			return;
		}
		if(NULLC::reg[reg2].type == x86Argument::argNumber)
		{
			EMIT_OP_REG_NUM(o_cmp, reg1, NULLC::reg[reg2].num);
			return;
		}

		NULLC::regRead[reg1] = true;
	}else if(op == o_imul){
		if(NULLC::reg[reg1].type == x86Argument::argNumber && NULLC::reg[reg2].type == x86Argument::argNumber)
		{
			EMIT_OP_REG_NUM(o_mov, reg1, NULLC::reg[reg1].num * NULLC::reg[reg2].num);
			return;
		}
		if(NULLC::reg[reg2].type == x86Argument::argPtr)
		{
			EMIT_OP_REG_RPTR(o_imul, reg1, sDWORD, NULLC::reg[reg2].ptrIndex, NULLC::reg[reg2].ptrMult, NULLC::reg[reg2].ptrBase, NULLC::reg[reg2].ptrNum);
			return;
		}
		NULLC::InvalidateDependand(reg1);
		NULLC::reg[reg1].type = x86Argument::argNone;
	}else{
		NULLC::reg[reg1].type = x86Argument::argNone;
	}
	NULLC::regRead[reg2] = true;
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
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argXmmReg;
	ctx.x86Op->argA.xmmArg = reg1;
	ctx.x86Op->argB.type = x86Argument::argXmmReg;
	ctx.x86Op->argB.xmmArg = reg2;
	ctx.x86Op++;
}

void EMIT_OP_REG_REG(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86XmmReg reg2)
{
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argXmmReg;
	ctx.x86Op->argB.xmmArg = reg2;
	ctx.x86Op++;
}

void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift)
{
#ifdef NULLC_OPTIMIZE_X86
	if(NULLC::reg[base].type == x86Argument::argReg)
		base = NULLC::reg[base].reg;
	if(NULLC::reg[base].type == x86Argument::argNumber)
	{
		shift += NULLC::reg[base].num;
		base = rNONE;
	}
	if(NULLC::reg[index].type == x86Argument::argNumber)
	{
		shift += NULLC::reg[index].num * mult;
		mult = 1;
		index = rNONE;
	}

	if(index == rNONE && NULLC::reg[base].type == x86Argument::argPtr && NULLC::reg[base].ptrSize == sNONE)
	{
		EMIT_OP_REG_RPTR(op, reg1, size, NULLC::reg[base].ptrIndex, NULLC::reg[base].ptrMult, NULLC::reg[base].ptrBase, NULLC::reg[base].ptrNum + shift);
		return;
	}

	x86Argument newArg = x86Argument(size, index, mult, base, shift);

	if(op != o_movsx)
	{
		unsigned int cIndex = NULLC::MemFind(newArg);
		if(cIndex != ~0u)
		{
			if(NULLC::mCache[cIndex].value.type == x86Argument::argNumber)
			{
				EMIT_OP_REG_NUM(op, reg1, NULLC::mCache[cIndex].value.num);
				NULLC::MemUpdate(cIndex);
				return;
			}
			if(NULLC::mCache[cIndex].value.type == x86Argument::argReg && op != o_imul && op != o_cmp)
			{
				EMIT_OP_REG_REG(op, reg1, NULLC::mCache[cIndex].value.reg);
				NULLC::MemUpdate(cIndex);
				return;
			}
		}
	}
	if(op == o_mov && base != rESP)
	{
		for(unsigned int i = rEAX; i <= rEDX; i++)
		{
			if(NULLC::reg[i].type == x86Argument::argPtr && NULLC::reg[i] == newArg)
			{
				EMIT_OP_REG_REG(op, reg1, (x86Reg)i);
				return;
			}
		}
	}
#ifdef _DEBUG
	if(op != o_mov && op != o_lea && op != o_movsx && op != o_or && op != o_imul && op != o_cmp)
		assert(!"invalid instruction");
#endif

	if(op == o_mov && size == sDWORD && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4))
	{
		x86Argument &target = NULLC::stack[(16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE];
		if(target.type == x86Argument::argPtr)
		{
			EMIT_OP_REG_RPTR(op, reg1, target.ptrSize, target.ptrIndex, target.ptrMult, target.ptrBase, target.ptrNum);
			return;
		}
		if(target.type == x86Argument::argReg)
		{
			EMIT_OP_REG_REG(op, reg1, target.reg);
			return;
		}
		if(target.type == x86Argument::argNumber)
		{
			EMIT_OP_REG_NUM(op, reg1, target.num);
			return;
		}
	}
	NULLC::regRead[base] = true;
	NULLC::regRead[index] = true;

	if(op == o_mov || op == o_movsx || op == o_lea)
	{
		EMIT_REG_KILL(ctx, reg1);
		NULLC::InvalidateDependand(reg1);
	}
	if(op == o_mov || op == o_movsx || op == o_lea)
	{
		if(reg1 != base && reg1 != index)
			NULLC::reg[reg1] = newArg;
		else
			NULLC::reg[reg1].type = x86Argument::argNone;
		NULLC::regUpdate[reg1] = (unsigned int)(ctx.x86Op - x86Base);
		NULLC::regRead[reg1] = false;
	}else if(op == o_imul){
		NULLC::reg[reg1].type = x86Argument::argNone;
	}else if(op == o_cmp || op == o_or){
		NULLC::regRead[reg1] = true;
	}

	if(size == sDWORD && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4))
		NULLC::stackRead[(16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE] = true;
#endif
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argPtr;
	ctx.x86Op->argB.ptrSize = size;
	ctx.x86Op->argB.ptrIndex = index;
	ctx.x86Op->argB.ptrMult = mult;
	ctx.x86Op->argB.ptrBase = base;
	ctx.x86Op->argB.ptrNum = shift;
	ctx.x86Op++;
}


void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift)
{
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argXmmReg;
	ctx.x86Op->argA.xmmArg = reg1;
	ctx.x86Op->argB.type = x86Argument::argPtr;
	ctx.x86Op->argB.ptrSize = size;
	ctx.x86Op->argB.ptrIndex = index;
	ctx.x86Op->argB.ptrMult = mult;
	ctx.x86Op->argB.ptrBase = base;
	ctx.x86Op->argB.ptrNum = shift;
	ctx.x86Op++;
}

void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, x86Reg reg2, unsigned int shift)
{
	EMIT_OP_REG_RPTR(ctx, op, reg1, size, rNONE, 1, reg2, shift);
}


void EMIT_OP_REG_RPTR(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Size size, x86Reg reg2, unsigned int shift)
{
	EMIT_OP_REG_RPTR(ctx, op, reg1, size, rNONE, 1, reg2, shift);
}

void EMIT_OP_REG_ADDR(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, x86Size size, unsigned int addr)
{
	EMIT_OP_REG_RPTR(ctx, op, reg1, size, rNONE, 1, rNONE, addr);
}

void EMIT_OP_REG_ADDR(CodeGenGenericContext &ctx, x86Command op, x86XmmReg reg1, x86Size size, unsigned int addr)
{
	EMIT_OP_REG_RPTR(ctx, op, reg1, size, rNONE, 1, rNONE, addr);
}

void EMIT_OP_REG_LABEL(CodeGenGenericContext &ctx, x86Command op, x86Reg reg1, unsigned int labelID, unsigned int shift)
{
#ifdef NULLC_OPTIMIZE_X86
	NULLC::InvalidateDependand(reg1);
	NULLC::reg[reg1].type = x86Argument::argNone;
#endif
	ctx.x86Op->name = op;
	ctx.x86Op->argA.type = x86Argument::argReg;
	ctx.x86Op->argA.reg = reg1;
	ctx.x86Op->argB.type = x86Argument::argPtrLabel;
	ctx.x86Op->argB.labelID = labelID;
	ctx.x86Op->argB.ptrNum = shift;
	ctx.x86Op++;
}

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned int shift, x86Reg reg2)
{
#ifdef NULLC_OPTIMIZE_X86
	if(NULLC::reg[base].type == x86Argument::argReg)
		base = NULLC::reg[base].reg;
	if(NULLC::reg[base].type == x86Argument::argNumber)
	{
		shift += NULLC::reg[base].num;
		base = rNONE;
	}
	if(NULLC::reg[index].type == x86Argument::argNumber)
	{
		shift += NULLC::reg[index].num * multiplier;
		multiplier = 1;
		index = rNONE;
	}

	if(index == rNONE && NULLC::reg[base].type == x86Argument::argPtr && NULLC::reg[base].ptrSize == sNONE)
	{
		EMIT_OP_RPTR_REG(op, size, NULLC::reg[base].ptrIndex, NULLC::reg[base].ptrMult, NULLC::reg[base].ptrBase, NULLC::reg[base].ptrNum + shift, reg2);
		return;
	}

	if(size == sDWORD && NULLC::reg[reg2].type == x86Argument::argNumber)
	{
		EMIT_OP_RPTR_NUM(op, size, index, multiplier, base, shift, NULLC::reg[reg2].num);
		return;
	}else if(NULLC::reg[reg2].type == x86Argument::argReg){
		reg2 = NULLC::reg[reg2].reg;
	}
	x86Argument arg(size, index, multiplier, base, shift);
	if(x86LookBehind &&
		(ctx.x86Op[-1].name == o_add || ctx.x86Op[-1].name == o_sub) && ctx.x86Op[-1].argA.type == x86Argument::argReg && ctx.x86Op[-1].argA.reg == reg2 && ctx.x86Op[-1].argB.type == x86Argument::argNumber &&
		ctx.x86Op[-2].name == o_mov && ctx.x86Op[-2].argA.type == x86Argument::argReg && ctx.x86Op[-2].argA.reg == reg2 && ctx.x86Op[-2].argB == arg)
	{
		x86Command origCmd = ctx.x86Op[-1].name;
		int num = ctx.x86Op[-1].argB.num;
		ctx.x86Op -= 2;
		EMIT_OP_RPTR_NUM(origCmd, size, index, multiplier, base, shift, num);
		EMIT_OP_REG_RPTR(o_mov, reg2, size, index, multiplier, base, shift);
		return;
	}
	if(size == sDWORD && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4))
	{
		unsigned int stackIndex = (16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE;
		x86Argument &target = NULLC::stack[stackIndex];
		if(op == o_mov)
		{
			target = x86Argument(reg2);
			NULLC::stackRead[stackIndex] = false;
			NULLC::stackUpdate[stackIndex] = (unsigned int)(ctx.x86Op - x86Base);
		}else{
			target.type = x86Argument::argNone;
		}
	}
	NULLC::regRead[reg2] = true;
	NULLC::regRead[base] = true;
	NULLC::regRead[index] = true;

	if(op == o_mov && base != rESP)
	{
		
		for(unsigned int i = rEAX; i <= rEDX; i++)
		{
			if(NULLC::reg[i].type == x86Argument::argPtr && NULLC::reg[i] == arg)
				NULLC::reg[i].type = x86Argument::argNone;
		}
		if(NULLC::reg[reg2].type == x86Argument::argNone || NULLC::reg[reg2].type == x86Argument::argPtr)
			NULLC::reg[reg2] = arg;
		NULLC::MemWrite(arg, x86Argument(reg2));
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

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned int shift, x86XmmReg reg2)
{
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

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned int shift, x86Reg reg2)
{
	EMIT_OP_RPTR_REG(ctx, op, size, rNONE, 1, reg1, shift, reg2);
}

void EMIT_OP_RPTR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned int shift, x86XmmReg reg2)
{
	EMIT_OP_RPTR_REG(ctx, op, size, rNONE, 1, reg1, shift, reg2);
}

void EMIT_OP_ADDR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned int addr, x86Reg reg2)
{
	EMIT_OP_RPTR_REG(ctx, op, size, rNONE, 1, rNONE, addr, reg2);
}

void EMIT_OP_ADDR_REG(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned int addr, x86XmmReg reg2)
{
	EMIT_OP_RPTR_REG(ctx, op, size, rNONE, 1, rNONE, addr, reg2);
}

void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned int shift, unsigned int num)
{
#ifdef NULLC_OPTIMIZE_X86
	if(NULLC::reg[base].type == x86Argument::argReg)
		base = NULLC::reg[base].reg;
	if(NULLC::reg[base].type == x86Argument::argNumber)
	{
		shift += NULLC::reg[base].num;
		base = rNONE;
	}
	if(NULLC::reg[index].type == x86Argument::argNumber)
	{
		shift += NULLC::reg[index].num * multiplier;
		multiplier = 1;
		index = rNONE;
	}

	if(index == rNONE && NULLC::reg[base].type == x86Argument::argPtr && NULLC::reg[base].ptrSize == sNONE)
	{
		EMIT_OP_RPTR_NUM(op, size, NULLC::reg[base].ptrIndex, NULLC::reg[base].ptrMult, NULLC::reg[base].ptrBase, NULLC::reg[base].ptrNum + shift, num);
		return;
	}

	if(size == sDWORD && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4))
	{
		unsigned int stackIndex = (16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE;
		x86Argument &target = NULLC::stack[stackIndex];
		if(op == o_mov)
		{
			target = x86Argument(num);
			NULLC::stackRead[stackIndex] = false;
			NULLC::stackUpdate[stackIndex] = (unsigned int)(ctx.x86Op - x86Base);
		}else{
			target.type = x86Argument::argNone;
		}
	}
	NULLC::regRead[base] = true;
	NULLC::regRead[index] = true;
	if(op == o_mov && base != rESP)
	{
		x86Argument arg(size, index, multiplier, base, shift);
		for(unsigned int i = rEAX; i <= rEDX; i++)
		{
			if(NULLC::reg[i].type == x86Argument::argPtr && NULLC::reg[i] == arg)
				NULLC::reg[i].type = x86Argument::argNone;
		}
		NULLC::MemWrite(arg, x86Argument(num));
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

void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, x86Reg reg1, unsigned int shift, unsigned int num)
{
	EMIT_OP_RPTR_NUM(ctx, op, size, rNONE, 1, reg1, shift, num);
}

void EMIT_OP_RPTR_NUM(CodeGenGenericContext &ctx, x86Command op, x86Size size, unsigned int addr, unsigned int number)
{
	EMIT_OP_RPTR_NUM(ctx, op, size, rNONE, 1, rNONE, addr, number);
}

void EMIT_REG_READ(CodeGenGenericContext &ctx, x86Reg reg)
{
	(void)ctx;
	(void)reg;

#ifdef NULLC_OPTIMIZE_X86
	EMIT_OP_REG(ctx, o_nop, reg);
#endif
}

void EMIT_REG_KILL(CodeGenGenericContext &ctx, x86Reg reg)
{
	(void)ctx;

#ifdef NULLC_OPTIMIZE_X86
	// Eliminate dead stores to the register
	if(!NULLC::regRead[reg] && NULLC::reg[reg].type != x86Argument::argNone)
	{
		x86Instruction *curr = NULLC::regUpdate[reg] + x86Base;
		if(curr->name != o_none)
		{
			curr->name = o_none;
			optiCount++;
		}
	}

	// Invalidate the register value
	NULLC::reg[reg].type = x86Argument::argNone;
#else
	(void)reg;
#endif
}

void EMIT_REG_KILL(CodeGenGenericContext &ctx, x86XmmReg reg)
{
	(void)ctx;
	(void)reg;
}

void SetOptimizationLookBehind(CodeGenGenericContext &ctx, bool allow)
{
	ctx.x86LookBehind = allow;

#ifdef NULLC_OPTIMIZE_X86
	if(!allow)
	{
		EMIT_REG_KILL(ctx, rEAX);
		EMIT_REG_KILL(ctx, rEBX);
		EMIT_REG_KILL(ctx, rECX);
		EMIT_REG_KILL(ctx, rEDX);
		EMIT_REG_KILL(ctx, rESI);
		NULLC::lastInvalidate = (unsigned int)(ctx.x86Op - x86Base);
		NULLC::InvalidateState();
		NULLC::regUpdate[rESP] = 0;
	}
#endif
}

#endif
