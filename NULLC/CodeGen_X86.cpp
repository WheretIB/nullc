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

	x86Argument		reg[9];
	unsigned int	regUpdate[9];
	bool			regRead[9];

	x86Reg			regPool[] = { rEAX, rEBX, rECX, rEDX, rESI };
	unsigned int	regPoolPos = 0;
	x86Reg	AllocateRegister()
	{
		if(regPoolPos == 5)
			regPoolPos = 0;
		return regPool[regPoolPos++];
	}

	struct MemCache
	{
		x86Argument	address;
		x86Argument value;
	};
	const unsigned int MEMORY_STATE_SIZE = 16;
	MemCache	mCache[MEMORY_STATE_SIZE];
	unsigned int	mCacheEntries = 0;

	unsigned	MemFind(const x86Argument &address)
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
	void		MemWrite(const x86Argument &address, const x86Argument &value)
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
	void		MemUpdate(unsigned int index)
	{
		(void)index;
		if(index != 0)
		{
			MemCache tmp = mCache[index-1];
			mCache[index-1] = mCache[index];
			mCache[index] = tmp;
		}
	}

	void	InvalidateState()
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

	void	InvalidateDependand(x86Reg dreg)
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

x86Instruction	*x86Op = NULL, *x86Base = NULL;
const unsigned char	*x86BinaryBase = NULL;
ExternFuncInfo	*x86Functions = NULL;
unsigned int	*x86FuncAddr = NULL;
int				*x86Continue = NULL;

bool			x86LookBehind = true;

unsigned int optiCount = 0;
unsigned int GetOptimizationCount()
{
	return optiCount;
}

#ifdef NULLC_OPTIMIZE_X86
void KILL_REG_IMPL(x86Reg reg)
{
	if(!NULLC::regRead[reg] && NULLC::reg[reg].type != x86Argument::argNone)
	{
		x86Instruction *curr = NULLC::regUpdate[reg] + x86Base;
		if(curr->name != o_none)
		{
			curr->name = o_none;
			optiCount++;
		}
		NULLC::reg[reg].type = x86Argument::argNone;
	}
}
#define KILL_REG(reg)	KILL_REG_IMPL(reg)
#else
#define KILL_REG(reg)
#endif

#ifdef NULLC_LOG_FILES
void EMIT_COMMENT(const char* text)
{
	(void)text;
	/*x86Op->name = o_other;
	x86Op->comment = text;
	x86Op++;*/
}
#else
#define EMIT_COMMENT(x)
#endif

void EMIT_LABEL(unsigned int labelID, int invalidate = true)
{
#ifdef NULLC_OPTIMIZE_X86
	if(invalidate)
		NULLC::InvalidateState();
#else
	(void)invalidate;
#endif
	x86Op->name = o_label;
	x86Op->labelID = labelID;
	x86Op->argA.type = x86Argument::argNone;
	x86Op->argA.num = invalidate;
	x86Op->argB.type = x86Argument::argNone;
	x86Op++;
}
void EMIT_OP(x86Command op)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op >= o_jmp && op <= o_ret)
		NULLC::InvalidateState();
	if(op == o_ret)
	{
		NULLC::regRead[rEAX] = NULLC::regRead[rEBX] = NULLC::regRead[rECX] = NULLC::regRead[rEDX] = true;
	}else if(op == o_rep_movsd){
		NULLC::regRead[rECX] = NULLC::regRead[rESI] = NULLC::regRead[rEDI] = true;
		NULLC::InvalidateDependand(rESI);
		NULLC::reg[rESI].type = x86Argument::argNone;
		NULLC::InvalidateDependand(rEDI);
		NULLC::reg[rEDI].type = x86Argument::argNone;
		assert(NULLC::reg[rECX].type == x86Argument::argNumber);
		unsigned int stackEntryCount = NULLC::reg[rECX].num;
		stackEntryCount = stackEntryCount > NULLC::STACK_STATE_SIZE ? NULLC::STACK_STATE_SIZE : stackEntryCount;
		for(unsigned int i = 0; i < stackEntryCount; i++)
			NULLC::stackRead[(16 + NULLC::stackTop - i) % NULLC::STACK_STATE_SIZE] = true;
		NULLC::InvalidateDependand(rECX);
		NULLC::reg[rECX].type = x86Argument::argNone;
	}else if(op == o_cdq){
		if(NULLC::reg[rEAX].type == x86Argument::argNumber)
		{
			EMIT_OP_REG_NUM(o_mov, rEDX, NULLC::reg[rEAX].num >= 0 ? 0 : ~0u);
			return;
		}
		NULLC::regRead[rEAX] = true;
		KILL_REG(rEDX);
		NULLC::InvalidateDependand(rEDX);
		NULLC::reg[rEDX].type = x86Argument::argNone;
		NULLC::regRead[rEDX] = false;
		NULLC::regUpdate[rEDX] = (unsigned int)(x86Op - x86Base);
	}
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argNone;
	x86Op->argB.type = x86Argument::argNone;
	x86Op++;
}
void EMIT_OP_LABEL(x86Command op, unsigned int labelID, int invalidate = true, int longJump = false)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op == o_call)
	{
		KILL_REG(rEAX); KILL_REG(rEBX); KILL_REG(rECX); KILL_REG(rEDX); KILL_REG(rESI);
	}
	if(op >= o_jmp && op <= o_ret && invalidate)
	{
		if(longJump)
		{
			KILL_REG(rEAX); KILL_REG(rEBX); KILL_REG(rECX); KILL_REG(rEDX); KILL_REG(rESI);
		}
		NULLC::InvalidateState();
	}

	if((x86Op - x86Base - 2) > (int)NULLC::lastInvalidate && x86Op[-2].name >= o_setl && x86Op[-2].name <= o_setnz && x86Op[-1].name == o_test &&
		 x86Op[-2].argA.reg == x86Op[-1].argA.reg && x86Op[-1].argA.reg == x86Op[-1].argB.reg)
	{
		if(op == o_jz)
		{
			static const x86Command jump[] = { o_jge, o_jle, o_jg, o_jl, o_jne, o_je, o_jnz, o_jz };
			x86Command curr = jump[x86Op[-2].name - o_setl];
			if(x86Op[-4].name == o_xor && x86Op[-4].argA.reg == x86Op[-4].argB.reg)
				x86Op[-4].name = o_none;
			x86Op[-2].name = o_none;
			x86Op[-1].name = o_none;
			optiCount += 2;
			EMIT_OP_LABEL(curr, labelID, invalidate, longJump);
			return;
		}else if(op == o_jnz){
			static const x86Command jump[] = { o_jl, o_jg, o_jle, o_jge, o_je, o_jne, o_jz, o_jnz };
			x86Command curr = jump[x86Op[-2].name - o_setl];
			if(x86Op[-4].name == o_xor && x86Op[-4].argA.reg == x86Op[-4].argB.reg)
				x86Op[-4].name = o_none;
			x86Op[-2].name = o_none;
			x86Op[-1].name = o_none;
			optiCount += 2;
			EMIT_OP_LABEL(curr, labelID, invalidate, longJump);
			return;
		}
	}
#else
	(void)invalidate;
	(void)longJump;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argLabel;
	x86Op->argA.labelID = labelID;
	x86Op->argB.type = x86Argument::argNone;
	x86Op->argB.num = invalidate;
	x86Op->argB.ptrNum = longJump;
	x86Op++;
}
void EMIT_CALL_REG(x86Reg reg1)
{
#ifdef NULLC_OPTIMIZE_X86
	NULLC::regRead[reg1] = true;
#endif
	x86Op->name = o_call;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op++;
}

void EMIT_OP_REG(x86Command op, x86Reg reg1)
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
			KILL_REG(rEAX);
		if(reg1 != rEBX)
			KILL_REG(rEBX);
		if(reg1 != rECX)
			KILL_REG(rECX);
		if(reg1 != rEDX)
			KILL_REG(rEDX);
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
		NULLC::stackUpdate[index] = (unsigned int)(x86Op - x86Base);
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
			KILL_REG(reg1);
			NULLC::reg[reg1] = NULLC::stack[index];
			NULLC::stackTop--;
			NULLC::stack[index].type = x86Argument::argNone;
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
		NULLC::regUpdate[reg1] = (unsigned int)(x86Op - x86Base);
	}
	if(op == o_neg || op == o_not)
		NULLC::reg[reg1].type = x86Argument::argNone;
	NULLC::regRead[reg1] = (op == o_push || op == o_imul || op == o_idiv || op == o_neg || op == o_not || op == o_nop);

#ifdef _DEBUG
	if(op != o_push && op != o_pop && op != o_call && op != o_imul && op < o_setl && op > o_setnz)
		__asm int 3;
#endif

#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argNone;
	x86Op++;
}
void EMIT_OP_FPUREG(x86Command op, x87Reg reg1)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argFPReg;
	x86Op->argA.fpArg = reg1;
	x86Op->argB.type = x86Argument::argNone;
	x86Op++;
}
void EMIT_OP_NUM(x86Command op, unsigned int num)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op == o_push)
	{
		unsigned int index = (++NULLC::stackTop) % NULLC::STACK_STATE_SIZE;
		NULLC::stack[index] = x86Argument(num);
		NULLC::stackRead[index] = false;
		NULLC::stackUpdate[index] = (unsigned int)(x86Op - x86Base);
	}
	if(op == o_int)
		NULLC::regRead[rECX] = true;

#ifdef _DEBUG
	if(op != o_push && op != o_int)
		__asm int 3;
#endif

#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argNumber;
	x86Op->argA.num = num;
	x86Op->argB.type = x86Argument::argNone;
	x86Op++;
}

void EMIT_OP_RPTR(x86Command op, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift)
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
		NULLC::stackUpdate[sIndex] = (unsigned int)(x86Op - x86Base);
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

	}else if(size == sDWORD && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4)){

		if(x86LookBehind && op == o_fstp && size == sDWORD && shift == 0 &&
			x86Op[-1].name == o_sub && x86Op[-1].argA.reg == rESP && x86Op[-1].argB.num == 4 &&
			x86Op[-2].name == o_fld && x86Op[-2].argA.ptrSize == sDWORD)
		{
			x86Instruction &fld = x86Op[-2];
			EMIT_OP_REG_NUM(o_add, rESP, 4);
			EMIT_OP_RPTR(o_push, sDWORD, fld.argA.ptrIndex, fld.argA.ptrMult, fld.argA.ptrBase, fld.argA.ptrNum);
			fld.name = o_none;
			return;
		}
		x86Argument &target = NULLC::stack[(16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE];
		target.type = x86Argument::argNone;
	}else if((op == o_fld || op == o_fild || op == o_fadd || op == o_fsub || op == o_fmul || op == o_fdiv || op == o_fcomp) && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4)){
		if(x86LookBehind && op == o_fld && x86Op[-1].name == o_fstp && x86Op[-1].argA.ptrSize == size && x86Op[-1].argA.ptrBase == base && x86Op[-1].argA.ptrNum == (int)shift)
		{
			x86Op[-1].name = o_fst;
			optiCount++;
			return;
		}
		if(x86LookBehind && (op == o_fadd || op == o_fsub || op == o_fmul || op == o_fdiv) && x86Op[-2].name == o_fstp && shift == 0 && x86Op[-2].argA.ptrNum == 0)
		{
			x86Op[-2].name = o_fst;
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
		NULLC::stackUpdate[target] = (unsigned int)(x86Op - x86Base);

		if(size == sQWORD)
		{
			target = (16 + NULLC::stackTop - (shift >> 2) - 1) % NULLC::STACK_STATE_SIZE;
			NULLC::stack[target].type = op == o_fistp ? x86Argument::argPtrLabel : x86Argument::argFPReg;
			NULLC::stackRead[target] = false;
			NULLC::stackUpdate[target] = (unsigned int)(x86Op - x86Base);
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
		KILL_REG(rEAX); KILL_REG(rEBX); KILL_REG(rECX); KILL_REG(rEDX); KILL_REG(rESI);
		NULLC::InvalidateState();
	}

#ifdef _DEBUG
	if(op != o_push && op != o_pop && op != o_neg && op != o_not && op != o_idiv && op != o_fstp && op != o_fld && op != o_fadd && op != o_fsub && op != o_fmul && op != o_fdiv && op != o_fcomp && op != o_fild && op != o_fistp && op != o_fst && op != o_call && op != o_jmp)
		__asm int 3;
#endif

#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrIndex = index;
	x86Op->argA.ptrMult = mult;
	x86Op->argA.ptrBase = base;
	x86Op->argA.ptrNum = shift;
	x86Op->argB.type = x86Argument::argNone;
	x86Op++;
}
void EMIT_OP_RPTR(x86Command op, x86Size size, x86Reg reg2, unsigned int shift)
{
	EMIT_OP_RPTR(op, size, rNONE, 1, reg2, shift);
}
void EMIT_OP_ADDR(x86Command op, x86Size size, unsigned int addr)
{
	EMIT_OP_RPTR(op, size, rNONE, 1, rNONE, addr);
}

void EMIT_OP_REG_NUM(x86Command op, x86Reg reg1, unsigned int num)
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
					while(++start < x86Op)
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
	}

	if((op == o_add || op == o_sub) && reg1 == rESP)
	{
		x86Instruction &prev = x86Base[NULLC::regUpdate[rESP]];
		if((prev.name == o_add || prev.name == o_sub) && prev.argB.type == x86Argument::argNumber && NULLC::regUpdate[rESP] > NULLC::lastInvalidate)
		{
			x86Instruction *curr = &prev;
			bool safe = true;
			while(++curr < x86Op && safe)
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
		NULLC::regUpdate[rESP] = (unsigned int)(x86Op - x86Base);
	}else if(op == o_mov){
		if(NULLC::reg[reg1].type == x86Argument::argNumber && NULLC::reg[reg1].num == (int)num)
		{
			optiCount++;
			return;
		}
		KILL_REG(reg1);
		NULLC::InvalidateDependand(reg1);
		NULLC::reg[reg1] = x86Argument(num);
		NULLC::regUpdate[reg1] = (unsigned int)(x86Op - x86Base);
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
		__asm int 3;
#endif

#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argNumber;
	x86Op->argB.num = num;
	x86Op++;
}
void EMIT_OP_REG_REG(x86Command op, x86Reg reg1, x86Reg reg2)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op == o_xor && reg1 == reg2)
	{
		KILL_REG(reg1);
		NULLC::InvalidateDependand(reg1);
	}else if(op == o_test && reg1 == reg2 && NULLC::reg[reg1].type == x86Argument::argReg){
		reg1 = reg2 = NULLC::reg[reg1].reg;
	}else{
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
		KILL_REG(reg1);
		NULLC::InvalidateDependand(reg1);

		NULLC::reg[reg1] = x86Argument(reg2);
		NULLC::regUpdate[reg1] = (unsigned int)(x86Op - x86Base);
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
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
	x86Op++;
}
void EMIT_OP_REG_RPTR(x86Command op, x86Reg reg1, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift)
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
		__asm int 3;
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
		KILL_REG(reg1);
		NULLC::InvalidateDependand(reg1);
	}
	if(op == o_mov || op == o_movsx || op == o_lea)
	{
		if(reg1 != base && reg1 != index)
			NULLC::reg[reg1] = newArg;
		else
			NULLC::reg[reg1].type = x86Argument::argNone;
		NULLC::regUpdate[reg1] = (unsigned int)(x86Op - x86Base);
		NULLC::regRead[reg1] = false;
	}else if(op == o_imul){
		NULLC::reg[reg1].type = x86Argument::argNone;
	}else if(op == o_cmp || op == o_or){
		NULLC::regRead[reg1] = true;
	}

	if(size == sDWORD && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4))
		NULLC::stackRead[(16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE] = true;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argPtr;
	x86Op->argB.ptrSize = size;
	x86Op->argB.ptrIndex = index;
	x86Op->argB.ptrMult = mult;
	x86Op->argB.ptrBase = base;
	x86Op->argB.ptrNum = shift;
	x86Op++;
}
void EMIT_OP_REG_RPTR(x86Command op, x86Reg reg1, x86Size size, x86Reg reg2, unsigned int shift)
{
	EMIT_OP_REG_RPTR(op, reg1, size, rNONE, 1, reg2, shift);
}
void EMIT_OP_REG_ADDR(x86Command op, x86Reg reg1, x86Size size, unsigned int addr)
{
	EMIT_OP_REG_RPTR(op, reg1, size, rNONE, 1, rNONE, addr);
}

void EMIT_OP_REG_LABEL(x86Command op, x86Reg reg1, unsigned int labelID, unsigned int shift)
{
#ifdef NULLC_OPTIMIZE_X86
	NULLC::InvalidateDependand(reg1);
	NULLC::reg[reg1].type = x86Argument::argNone;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argPtrLabel;
	x86Op->argB.labelID = labelID;
	x86Op->argB.ptrNum = shift;
	x86Op++;
}
void EMIT_OP_REG_REG_MULT_REG_NUM(x86Command op, x86Reg dst, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift)
{
	EMIT_OP_REG_RPTR(op, dst, size, index, mult, base, shift);
}

void EMIT_OP_RPTR_REG(x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned int shift, x86Reg reg2)
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

	if(NULLC::reg[reg2].type == x86Argument::argNumber)
	{
		EMIT_OP_RPTR_NUM(op, size, index, multiplier, base, shift, NULLC::reg[reg2].num);
		return;
	}else if(NULLC::reg[reg2].type == x86Argument::argReg){
		reg2 = NULLC::reg[reg2].reg;
	}
	x86Argument arg(size, index, multiplier, base, shift);
	if(x86LookBehind &&
		(x86Op[-1].name == o_add || x86Op[-1].name == o_sub) && x86Op[-1].argA.type == x86Argument::argReg && x86Op[-1].argA.reg == reg2 && x86Op[-1].argB.type == x86Argument::argNumber &&
		x86Op[-2].name == o_mov && x86Op[-1].argA.type == x86Argument::argReg && x86Op[-1].argA.reg == reg2 && x86Op[-2].argB == arg)
	{
		x86Command origCmd = x86Op[-1].name;
		int num = x86Op[-1].argB.num;
		x86Op -= 2;
		EMIT_OP_RPTR_NUM(origCmd, size, index, multiplier, base, shift, num);
		EMIT_OP_REG_RPTR(o_mov, reg2, size, index, multiplier, base, shift);
		return;
	}
	if(size == sDWORD && base == rESP && shift < (NULLC::STACK_STATE_SIZE * 4))
	{
		unsigned int index = (16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE;
		x86Argument &target = NULLC::stack[index];
		if(op == o_mov)
		{
			target = x86Argument(reg2);
			NULLC::stackRead[index] = false;
			NULLC::stackUpdate[index] = (unsigned int)(x86Op - x86Base);
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
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrIndex = index;
	x86Op->argA.ptrMult = multiplier;
	x86Op->argA.ptrBase = base;
	x86Op->argA.ptrNum = shift;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
	x86Op++;
}
void EMIT_OP_RPTR_REG(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, x86Reg reg2)
{
	EMIT_OP_RPTR_REG(op, size, rNONE, 1, reg1, shift, reg2);
}
void EMIT_OP_ADDR_REG(x86Command op, x86Size size, unsigned int addr, x86Reg reg2)
{
	EMIT_OP_RPTR_REG(op, size, rNONE, 1, rNONE, addr, reg2);
}

void EMIT_OP_RPTR_NUM(x86Command op, x86Size size, x86Reg index, int multiplier, x86Reg base, unsigned int shift, unsigned int num)
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
		unsigned int index = (16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE;
		x86Argument &target = NULLC::stack[index];
		if(op == o_mov)
		{
			target = x86Argument(num);
			NULLC::stackRead[index] = false;
			NULLC::stackUpdate[index] = (unsigned int)(x86Op - x86Base);
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
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrIndex = index;
	x86Op->argA.ptrMult = multiplier;
	x86Op->argA.ptrBase = base;
	x86Op->argA.ptrNum = shift;
	x86Op->argB.type = x86Argument::argNumber;
	x86Op->argB.num = num;
	x86Op++;
}
void EMIT_OP_RPTR_NUM(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, unsigned int num)
{
	EMIT_OP_RPTR_NUM(op, size, rNONE, 1, reg1, shift, num);
}
void EMIT_OP_ADDR_NUM(x86Command op, x86Size size, unsigned int addr, unsigned int number)
{
	EMIT_OP_RPTR_NUM(op, size, rNONE, 1, rNONE, addr, number);
}

bool EMIT_POP_DOUBLE(x86Reg base, unsigned int address)
{
	x86Instruction &prev = x86Op[-1];
	if(x86LookBehind && prev.name == o_fstp)
	{
		prev.name = o_fst;
		if(base == rNONE)
			EMIT_OP_ADDR(o_fstp, sQWORD, address);
		else
			EMIT_OP_RPTR(o_fstp, sQWORD, base, address);
		optiCount += 3;
		return true;
	}
	return false;
}
void EMIT_REG_READ(x86Reg reg)
{
	(void)reg;
#ifdef NULLC_OPTIMIZE_X86
	EMIT_OP_REG(o_nop, reg);
#endif
}

void OptimizationLookBehind(bool allow)
{
	x86LookBehind = allow;
#ifdef NULLC_OPTIMIZE_X86
	if(!allow)
	{
		KILL_REG(rEAX);KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
		KILL_REG(rESI);
		NULLC::lastInvalidate = (unsigned int)(x86Op - x86Base);
		NULLC::InvalidateState();
		NULLC::regUpdate[rESP] = 0;
	}
#endif
}

static unsigned int paramBase = 0;
static unsigned int aluLabels = LABEL_ALU;

void SetParamBase(unsigned int base)
{
	paramBase = base;
	aluLabels = LABEL_ALU;
}

void SetFunctionList(ExternFuncInfo *list, unsigned int* funcAddresses)
{
	x86Functions = list;
	x86FuncAddr = funcAddresses;
}

void SetContinuePtr(int* continueVar)
{
	x86Continue = continueVar;
}

#ifdef __linux
int nullcJmpTarget;
void LongJumpWrap(sigjmp_buf errorHandler, int code)
{
	int x = 0;
	asm("mov 4(%%ebp), %0":"=r"(x));
	*(int*)((char*)paramBase - 8) = x;
	siglongjmp(errorHandler, code);
}
void (*siglongjmpPtr)() = (void(*)())LongJumpWrap;

void SetLongJmpTarget(sigjmp_buf target)
{
	nullcJmpTarget = (int)(intptr_t)target;
}
#endif

void SetLastInstruction(x86Instruction *pos, x86Instruction *base)
{
	x86Op = pos;
	x86Base = base;
}

x86Instruction* GetLastInstruction()
{
	return x86Op;
}

void SetBinaryCodeBase(const unsigned char* base)
{
	x86BinaryBase = base;
}

unsigned int GetLastALULabel()
{
	return aluLabels;
}

void GenCodeCmdNop(VMCmd cmd)
{
	(void)cmd;
	EMIT_OP(o_nop);
}

void GenCodeCmdPushChar(VMCmd cmd)
{
	EMIT_COMMENT("PUSH char");

	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_REG_ADDR(o_movsx, rEAX, sBYTE, cmd.argument+paramBase);
	else
		EMIT_OP_REG_RPTR(o_movsx, rEAX, sBYTE, rEBP, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushShort(VMCmd cmd)
{
	EMIT_COMMENT("PUSH short");

	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_REG_ADDR(o_movsx, rEAX, sWORD, cmd.argument+paramBase);
	else
		EMIT_OP_REG_RPTR(o_movsx, rEAX, sWORD, rEBP, cmd.argument+paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPushInt(VMCmd cmd)
{
	EMIT_COMMENT("PUSH int");

	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase);
	else
		EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase);
}

void GenCodeCmdPushFloat(VMCmd cmd)
{
	EMIT_COMMENT("PUSH float");

	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR(o_fld, sDWORD, cmd.argument+paramBase);
	else
		EMIT_OP_RPTR(o_fld, sDWORD, rEBP, cmd.argument+paramBase);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdPushDorL(VMCmd cmd)
{
	EMIT_COMMENT("PUSH DorL");

	if(cmd.flag == ADDRESS_ABOLUTE)
	{
		EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase+4);
		EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase);
	}else{
		EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase+4);
		EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase);
	}
}

void GenCodeCmdPushCmplx(VMCmd cmd)
{
	EMIT_COMMENT("PUSH complex");
	if(cmd.helper == 0)
		return;
	if(cmd.helper <= 32)
	{
		unsigned int currShift = cmd.helper;
		while(currShift >= 4)
		{
			currShift -= 4;
			if(cmd.flag == ADDRESS_ABOLUTE)
				EMIT_OP_ADDR(o_push, sDWORD, cmd.argument+paramBase+currShift);
			else
				EMIT_OP_RPTR(o_push, sDWORD, rEBP, cmd.argument+paramBase+currShift);
		}
		assert(currShift == 0);
	}else{
		EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		if(cmd.flag == ADDRESS_ABOLUTE)
			EMIT_OP_REG_NUM(o_mov, rESI, cmd.argument+paramBase);
		else
			EMIT_OP_REG_RPTR(o_lea, rESI, sNONE, rEBP, cmd.argument+paramBase);
		EMIT_OP_REG_REG(o_mov, rEDI, rESP);
		EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
	}
}

void GenCodeCmdPushCharStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH char stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_movsx, rEAX, sBYTE, rEDX, cmd.argument);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdPushShortStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH short stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_movsx, rEAX, sWORD, rEDX, cmd.argument);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdPushIntStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH int stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument);
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdPushFloatStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH float stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fld, sDWORD, rEDX, cmd.argument);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdPushDorLStk(VMCmd cmd)
{
	EMIT_COMMENT(cmd.flag ? "PUSH double stack" : "PUSH long stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument+4);
	EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument);
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdPushCmplxStk(VMCmd cmd)
{
	EMIT_COMMENT("PUSH complex stack");
	EMIT_OP_REG(o_pop, rEDX);

	if(cmd.helper == 0)
		return;
	if(cmd.helper <= 32)
	{
		unsigned int currShift = cmd.helper;
		while(currShift >= 4)
		{
			currShift -= 4;
			EMIT_OP_RPTR(o_push, sDWORD, rEDX, cmd.argument+currShift);
		}
		assert(currShift == 0);
	}else{
		EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		EMIT_OP_REG_RPTR(o_lea, rESI, sNONE, rEDX, cmd.argument);
		EMIT_OP_REG_REG(o_mov, rEDI, rESP);
		EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
	}
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}


void GenCodeCmdPushImmt(VMCmd cmd)
{
	EMIT_COMMENT("PUSHIMMT");
	
	EMIT_OP_NUM(o_push, cmd.argument);
}


void GenCodeCmdMovChar(VMCmd cmd)
{
	EMIT_COMMENT("MOV char");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR_REG(o_mov, sBYTE, cmd.argument+paramBase, rEBX);
	else
		EMIT_OP_RPTR_REG(o_mov, sBYTE, rEBP, cmd.argument+paramBase, rEBX);
	KILL_REG(rEBX);
}

void GenCodeCmdMovShort(VMCmd cmd)
{
	EMIT_COMMENT("MOV short");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR_REG(o_mov, sWORD, cmd.argument+paramBase, rEBX);
	else
		EMIT_OP_RPTR_REG(o_mov, sWORD, rEBP, cmd.argument+paramBase, rEBX);
	KILL_REG(rEBX);
}

void GenCodeCmdMovInt(VMCmd cmd)
{
	EMIT_COMMENT("MOV int");

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR_REG(o_mov, sDWORD, cmd.argument+paramBase, rEBX);
	else
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBP, cmd.argument+paramBase, rEBX);
	KILL_REG(rEBX);
}

void GenCodeCmdMovFloat(VMCmd cmd)
{
	EMIT_COMMENT("MOV float");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR(o_fstp, sDWORD, cmd.argument+paramBase);
	else
		EMIT_OP_RPTR(o_fstp, sDWORD, rEBP, cmd.argument+paramBase);
}

void GenCodeCmdMovDorL(VMCmd cmd)
{
	EMIT_COMMENT("MOV DorL");

#ifdef NULLC_OPTIMIZE_X86
	if(EMIT_POP_DOUBLE(cmd.flag == ADDRESS_ABOLUTE ? rNONE : rEBP, cmd.argument+paramBase))
		return;
#endif

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR_REG(o_mov, sDWORD, cmd.argument+paramBase, rEBX);
	else
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBP, cmd.argument+paramBase, rEBX);
	KILL_REG(rEBX);

	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 4);
	if(cmd.flag == ADDRESS_ABOLUTE)
		EMIT_OP_ADDR_REG(o_mov, sDWORD, cmd.argument+paramBase + 4, rEBX);
	else
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBP, cmd.argument+paramBase + 4, rEBX);
	KILL_REG(rEBX);
}

void GenCodeCmdMovCmplx(VMCmd cmd)
{
	EMIT_COMMENT("MOV complex");
	if(cmd.helper == 0)
		return;
	if(cmd.helper <= 32)
	{
		unsigned int currShift = 0;
		while(currShift < cmd.helper)
		{
			if(cmd.flag == ADDRESS_ABOLUTE)
				EMIT_OP_ADDR(o_pop, sDWORD, cmd.argument+paramBase + currShift);
			else
				EMIT_OP_RPTR(o_pop, sDWORD, rEBP, cmd.argument+paramBase + currShift);
			currShift += 4;
		}
		EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
		assert(currShift == cmd.helper);
	}else{
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		EMIT_OP_REG_REG(o_mov, rESI, rESP);
		if(cmd.flag == ADDRESS_ABOLUTE)
			EMIT_OP_REG_NUM(o_mov, rEDI, cmd.argument+paramBase);
		else
			EMIT_OP_REG_RPTR(o_lea, rEDI, sNONE, rEBP, cmd.argument+paramBase);
		EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
	}
}

void GenCodeCmdMovCharStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV char stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sBYTE, rEDX, cmd.argument, rEBX);
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdMovShortStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV short stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sWORD, rEDX, cmd.argument, rEBX);
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdMovIntStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV int stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rEDX, cmd.argument, rEBX);
	KILL_REG(rEAX);KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);KILL_REG(rESI);
}

void GenCodeCmdMovFloatStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV float stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sDWORD, rEDX, cmd.argument);
	KILL_REG(rEAX);KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);KILL_REG(rESI);
}

void GenCodeCmdMovDorLStk(VMCmd cmd)
{
	EMIT_COMMENT(cmd.flag ? "MOV double stack" : "MOV long stack");

	EMIT_OP_REG(o_pop, rEDX);

	if(cmd.flag)
	{
		EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
		EMIT_OP_RPTR(o_fstp, sQWORD, rEDX, cmd.argument);
	}else{
		EMIT_OP_RPTR(o_pop, sDWORD, rEDX, cmd.argument);
		EMIT_OP_RPTR(o_pop, sDWORD, rEDX, cmd.argument + 4);
		EMIT_OP_REG_NUM(o_sub, rESP, 8);
	}
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdMovCmplxStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV complex stack");
	EMIT_OP_REG(o_pop, rEDX);

	if(cmd.helper == 0)
		return;
	if(cmd.helper <= 32)
	{
		unsigned int currShift = 0;
		while(currShift < cmd.helper)
		{
			EMIT_OP_RPTR(o_pop, sDWORD, rEDX, cmd.argument + currShift);
			currShift += 4;
		}
		EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);
		assert(currShift == cmd.helper);
	}else{
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		EMIT_OP_REG_REG(o_mov, rESI, rESP);
		EMIT_OP_REG_RPTR(o_lea, rEDI, sNONE, rEDX, cmd.argument);
		EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
	}
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdPop(VMCmd cmd)
{
	EMIT_COMMENT("POP");

	EMIT_OP_REG_NUM(o_add, rESP, cmd.argument);
}

int DoubleToInt(double a)
{
	return (int)a;
}
int (*doubleToIntPtr)(double) = DoubleToInt;

void GenCodeCmdDtoI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DTOI");

	EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&doubleToIntPtr);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_REG(o_push, rEAX);
}

long long DoubleToLong(double a)
{
	return (long long)a;
}
long long (*doubleToLongPtr)(double) = DoubleToLong;

void GenCodeCmdDtoL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DTOL");

	EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&doubleToLongPtr);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_REG(o_push, rEDX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdDtoF(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DTOF");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_REG_NUM(o_sub, rESP, 4);
	EMIT_OP_RPTR(o_fstp, sDWORD, rESP, 0);
}

void GenCodeCmdItoD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ITOD");

	EMIT_OP_RPTR(o_fild, sDWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_sub, rESP, 4);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdLtoD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LTOD");

	EMIT_OP_RPTR(o_fild, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdItoL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ITOL");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP(o_cdq);
	EMIT_OP_REG(o_push, rEDX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdLtoI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LTOI");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_NUM(o_add, rESP, 4);
	EMIT_OP_REG(o_push, rEAX);
}


void GenCodeCmdIndex(VMCmd cmd)
{
	EMIT_COMMENT("IMUL int");

	EMIT_OP_REG(o_pop, rEAX);	// Take index
	if(cmd.cmd == cmdIndex)
	{
		EMIT_OP_REG_NUM(o_cmp, rEAX, cmd.argument);
	}else{
		EMIT_OP_REG_RPTR(o_mov, rECX, sDWORD, rESP, 4);	// take size
		EMIT_OP_REG_REG(o_cmp, rEAX, rECX);
	}
	EMIT_OP_LABEL(o_jb, aluLabels, false);
#ifdef __linux
	EMIT_OP_NUM(o_int, 3);
#else
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_NUM(o_int, 3);
#endif
	EMIT_LABEL(aluLabels, false);
	aluLabels++;

	EMIT_OP_REG(o_pop, rEDX);	// Take address

	// Multiply it
	if(cmd.helper == 1)
	{
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rESI, sNONE, rEAX, 1, rEDX, 0);
	}else if(cmd.helper == 2){
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rESI, sNONE, rEAX, 2, rEDX, 0);
	}else if(cmd.helper == 4){
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rESI, sNONE, rEAX, 4, rEDX, 0);
	}else if(cmd.helper == 8){
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rESI, sNONE, rEAX, 8, rEDX, 0);
	}else if(cmd.helper == 16){
		EMIT_OP_REG_NUM(o_shl, rEAX, 4);
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rESI, sNONE, rEAX, 1, rEDX, 0);
	}else{
		EMIT_OP_REG_NUM(o_imul, rEAX, cmd.helper);
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rESI, sNONE, rEAX, 1, rEDX, 0);
	}
	if(cmd.cmd == cmdIndex)
		EMIT_OP_REG(o_push, rESI);
	else
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rESI);
}

void GenCodeCmdCopyDorL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("COPY qword");

	EMIT_OP_REG_RPTR(o_mov, rEDX, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 4);
	EMIT_OP_REG(o_push, rEAX);
	EMIT_OP_REG(o_push, rEDX);
}

void GenCodeCmdCopyI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("COPY dword");

	EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 0);
	EMIT_OP_REG(o_push, rEAX);
}


void GenCodeCmdGetAddr(VMCmd cmd)
{
	EMIT_COMMENT("GETADDR");

	if(cmd.helper)
		EMIT_OP_REG_RPTR(o_lea, rEAX, sNONE, rEBP, cmd.argument + paramBase);
	else
		EMIT_OP_REG_NUM(o_mov, rEAX, cmd.argument + paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdFuncAddr(VMCmd cmd)
{
	(void)cmd;
	assert(!"Unknown command cmdLogOr");
}

void GenCodeCmdSetRange(VMCmd cmd)
{
	unsigned int elCount = x86Op[-1].argA.num;

	EMIT_COMMENT("SETRANGE");

	EMIT_OP_REG(o_pop, rEBX);
	unsigned int typeSizeD[] = { 1, 2, 4, 8, 4, 8 };

	// start address
	EMIT_OP_REG_RPTR(o_lea, rEBX, sNONE, rEBP, paramBase + cmd.argument);
	// end address
	EMIT_OP_REG_RPTR(o_lea, rECX, sNONE, rEBP, paramBase + cmd.argument + (elCount - 1) * typeSizeD[(cmd.helper>>2)&0x00000007]);
	if(cmd.helper == DTYPE_FLOAT)
	{
		EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	}else{
		EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, 0);
		EMIT_OP_REG_RPTR(o_mov, rEDX, sDWORD, rESP, 4);
	}
	EMIT_LABEL(aluLabels);
	EMIT_OP_REG_REG(o_cmp, rEBX, rECX);
	EMIT_OP_LABEL(o_jg, aluLabels + 1);

	switch(cmd.helper)
	{
	case DTYPE_DOUBLE:
	case DTYPE_LONG:
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBX, 4, rEDX);
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBX, 0, rEAX);
		break;
	case DTYPE_FLOAT:
		EMIT_OP_RPTR(o_fst, sDWORD, rEBX, 0);
		break;
	case DTYPE_INT:
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rEBX, 0, rEAX);
		break;
	case DTYPE_SHORT:
		EMIT_OP_RPTR_REG(o_mov, sWORD, rEBX, 0, rEAX);
		break;
	case DTYPE_CHAR:
		EMIT_OP_RPTR_REG(o_mov, sBYTE, rEBX, 0, rEAX);
		break;
	}
	EMIT_OP_REG_NUM(o_add, rEBX, typeSizeD[(cmd.helper>>2)&0x00000007]);
	EMIT_OP_LABEL(o_jmp, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	if(cmd.helper == DTYPE_FLOAT)
		EMIT_OP_FPUREG(o_fstp, rST0);
	aluLabels += 2;
}


void GenCodeCmdJmp(VMCmd cmd)
{
	EMIT_COMMENT("JMP");
	EMIT_OP_LABEL(o_jmp, LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
}


void GenCodeCmdJmpZ(VMCmd cmd)
{
	EMIT_COMMENT("JMPZ int");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_test, rEAX, rEAX);
	EMIT_OP_LABEL(o_jz, LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
}

void GenCodeCmdJmpNZ(VMCmd cmd)
{
	EMIT_COMMENT("JMPNZ int");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_test, rEAX, rEAX);
	EMIT_OP_LABEL(o_jnz, LABEL_GLOBAL | JUMP_NEAR | cmd.argument, true, true);
}

void GenCodeCmdCallEpilog(unsigned int size)
{
	if(size == 0)
		return;
	if(size <= 32)
	{
		unsigned int currShift = 0;
		while(currShift < size)
		{
			EMIT_OP_RPTR(o_pop, sDWORD, rEDI, paramBase + currShift);
			currShift += 4;
		}
		assert(currShift == size);
	}else{
		EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

		EMIT_OP_REG_REG(o_mov, rESI, rESP);
		EMIT_OP_REG_RPTR(o_lea, rEDI, sNONE, rEDI, paramBase);
		EMIT_OP_REG_NUM(o_mov, rECX, size >> 2);
		EMIT_OP(o_rep_movsd);

		EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
		EMIT_OP_REG_NUM(o_add, rESP, size);
	}
}


void GenCodeCmdCallProlog(VMCmd cmd)
{
	if(cmd.helper & bitRetSimple)
	{
		if((asmOperType)(cmd.helper & 0x0FFF) == OTYPE_INT)
		{
			EMIT_OP_REG(o_push, rEAX);
		}else{	// double or long
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEDX);
		}
	}else{
		assert(cmd.helper % 4 == 0);
		if(cmd.helper == 4)
		{
			EMIT_OP_REG(o_push, rEAX);
		}else if(cmd.helper == 8){
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEDX);
		}else if(cmd.helper == 12){
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEDX);
			EMIT_OP_REG(o_push, rECX);
		}else if(cmd.helper == 16){
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEDX);
			EMIT_OP_REG(o_push, rECX);
			EMIT_OP_REG(o_push, rEBX);
		}else if(cmd.helper > 16){
			EMIT_OP_REG_NUM(o_sub, rESP, cmd.helper);

			EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

			EMIT_OP_REG_RPTR(o_lea, rESI, sNONE, rEAX, paramBase);
			EMIT_OP_REG_REG(o_mov, rEDI, rESP);
			EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
			EMIT_OP(o_rep_movsd);

			EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
		}
	}
}

void GenCodeCmdCall(VMCmd cmd)
{
	static unsigned int ret[128];
	EMIT_COMMENT("CALL");

	if(x86Functions[cmd.argument].address != -1)
	{
		GenCodeCmdCallEpilog(x86Functions[cmd.argument].bytesToPop);

		EMIT_OP_ADDR(o_push, sDWORD, paramBase-4);
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-4, rESP);
		EMIT_OP_ADDR(o_call, sNONE, cmd.argument * 8 + (unsigned int)(uintptr_t)x86FuncAddr);	// Index array of function addresses
		EMIT_OP_ADDR(o_pop, sDWORD, paramBase-4);
		
		GenCodeCmdCallProlog(cmd);
	}else{
		unsigned int bytesToPop = x86Functions[cmd.argument].bytesToPop;

		if(x86Functions[cmd.argument].retType == ExternFuncInfo::RETURN_UNKNOWN && x86Functions[cmd.argument].returnShift)
			EMIT_OP_NUM(o_push, (int)(intptr_t)ret);
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-12, rEDI);
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-8, rESP);
		EMIT_OP_ADDR(o_call, sNONE, cmd.argument * 8 + (unsigned int)(uintptr_t)x86FuncAddr);	// Index array of function addresses

		EMIT_OP_REG_ADDR(o_mov, rECX, sDWORD, (int)(intptr_t)x86Continue);
		EMIT_OP_REG_REG(o_test, rECX, rECX);
		EMIT_OP_LABEL(o_jnz, aluLabels);
#ifdef __linux
		// call siglongjmp(target_env, EXCEPTION_STOP_EXECUTION);
		EMIT_OP_NUM(o_push, EXCEPTION_STOP_EXECUTION);
		EMIT_OP_NUM(o_push, nullcJmpTarget);
		EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&siglongjmpPtr);
#else
		EMIT_OP_REG_REG(o_mov, rECX, rESP);	// esp is very likely to contain neither 0 or ~0, so we can distinguish
		EMIT_OP(o_int);						// array out of bounds and function with no return errors from this one
#endif
		EMIT_LABEL(aluLabels);
		aluLabels++;

		EMIT_OP_REG_NUM(o_add, rESP, bytesToPop);
		if(x86Functions[cmd.argument].retType == ExternFuncInfo::RETURN_INT)
		{
			EMIT_OP_REG(o_push, rEAX);
		}else if(x86Functions[cmd.argument].retType == ExternFuncInfo::RETURN_DOUBLE){
			EMIT_OP_REG_NUM(o_sub, rESP, 8);
			EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
		}else if(x86Functions[cmd.argument].retType == ExternFuncInfo::RETURN_LONG){
			EMIT_OP_REG(o_push, rEDX);
			EMIT_OP_REG(o_push, rEAX);
		}else if(x86Functions[cmd.argument].retType == ExternFuncInfo::RETURN_UNKNOWN && x86Functions[cmd.argument].returnShift){
			// adjust new stack top
			EMIT_OP_REG_NUM(o_sub, rESP, x86Functions[cmd.argument].returnShift * 4);
			// copy return value on top of the stack
			EMIT_OP_REG_REG(o_mov, rEBX, rEDI);

			//EMIT_OP_REG_RPTR(o_lea, rESI, sNONE, rEAX, paramBase);
			EMIT_OP_REG_NUM(o_mov, rESI, (int)(intptr_t)ret);	
			EMIT_OP_REG_REG(o_mov, rEDI, rESP);
			EMIT_OP_REG_NUM(o_mov, rECX, x86Functions[cmd.argument].returnShift);
			EMIT_OP(o_rep_movsd);

			EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
		}
	}
}

void GenCodeCmdCallPtr(VMCmd cmd)
{
	EMIT_COMMENT("CALLPTR");

	EMIT_OP_REG_RPTR(o_mov, rECX, sDWORD, rESP, cmd.argument);
	EMIT_OP_REG_RPTR(o_mov, rEAX, sNONE, rECX, 8, rNONE, (unsigned int)(uintptr_t)x86FuncAddr + 4);

	EMIT_OP_REG_REG(o_test, rECX, rECX);
	EMIT_OP_LABEL(o_jnz, aluLabels + 1);
#ifdef __linux
	// call siglongjmp(target_env, EXCEPTION_INVALID_FUNCTION);
	EMIT_OP_NUM(o_push, EXCEPTION_INVALID_FUNCTION);
	EMIT_OP_NUM(o_push, nullcJmpTarget);
	EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&siglongjmpPtr);
#else
	EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
	EMIT_OP_NUM(o_int, 3);
#endif
	EMIT_LABEL(aluLabels + 1);

	assert(cmd.argument >= 4);
	EMIT_OP_REG_REG(o_test, rEAX, rEAX);
	EMIT_OP_LABEL(o_jz, aluLabels);

	// external function call
	{
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-12, rEDI);
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-8, rESP);
		EMIT_OP_RPTR(o_call, sNONE, rECX, 8, rNONE, (unsigned int)(uintptr_t)x86FuncAddr);	// Index array of function addresses
	 
		static int continueLabel = 0;
		EMIT_OP_REG_ADDR(o_mov, rECX, sDWORD, (int)(intptr_t)x86Continue);
		EMIT_OP_REG_REG(o_test, rECX, rECX);
		EMIT_OP_LABEL(o_jnz, aluLabels + 3);
#ifdef __linux
		// call siglongjmp(target_env, EXCEPTION_STOP_EXECUTION);
		EMIT_OP_NUM(o_push, EXCEPTION_STOP_EXECUTION);
		EMIT_OP_NUM(o_push, nullcJmpTarget);
		EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&siglongjmpPtr);
#else
		EMIT_OP_REG_REG(o_mov, rECX, rESP); // esp is very likely to contain neither 0 or ~0, so we can distinguish
		EMIT_OP(o_int);						// array out of bounds and function with no return errors from this one
#endif
		EMIT_LABEL(aluLabels + 3);
	 
		EMIT_OP_REG_NUM(o_add, rESP, cmd.argument + 4);
		if(cmd.helper == (bitRetSimple | OTYPE_INT))
		{
			EMIT_OP_REG(o_push, rEAX);
		}else if(cmd.helper == (bitRetSimple | OTYPE_DOUBLE)){
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_REG(o_push, rEAX);
			EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
		}else if(cmd.helper == (bitRetSimple | OTYPE_LONG)){
			EMIT_OP_REG(o_push, rEDX);
			EMIT_OP_REG(o_push, rEAX);
		}

		EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	}

	// internal function call
	{
		EMIT_LABEL(aluLabels);

		GenCodeCmdCallEpilog(cmd.argument);

		EMIT_OP_REG_NUM(o_add, rESP, 4);
		EMIT_OP_ADDR(o_push, sDWORD, paramBase-4);
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-4, rESP);

		EMIT_OP_RPTR(o_call, sNONE, rECX, 8, rNONE, (unsigned int)(uintptr_t)x86FuncAddr);	// Index array of function addresses
		EMIT_OP_ADDR(o_pop, sDWORD, paramBase-4);

		GenCodeCmdCallProlog(cmd);
	}

	EMIT_LABEL(aluLabels + 2);

	aluLabels += 4;
}

VMCmd	yieldCmd = VMCmd(cmdNop);

#ifdef __linux
void yieldRestoreEIP()
{
	asm("pop %eax"); // remove pushed ebx
	asm("pop %eax"); // remove pushed ebp
	asm("mov %eax, %ebp");
	asm("pop %eax"); // remove pushed eip
	asm("add %0, %%ebx"::"m"(x86BinaryBase):"%ebx");
	asm("mov %ebx, -4(%esp)");
	asm("jmp *-4(%esp)");
}
#else
__declspec(naked) void yieldRestoreEIP()
{
	__asm
	{
		pop eax; // pop return address (we won't return from this function)
		add ebx, x86BinaryBase;
		mov [esp-4], ebx;
		jmp dword ptr [esp-4];
	}
}
#endif

void (*yieldRestorePtr)() = yieldRestoreEIP;

void GenCodeCmdYield(VMCmd cmd)
{
	// If flag is set, jump to saved offset
	if(cmd.flag)
	{
		// Take pointer to closure in hidden argument
		EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rEBP, cmd.argument + paramBase);
		// Read jump offset at 4 byte shift (ExternFuncInfo::Upvalue::next)
		EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rEAX, 4);
		// Test for 0
		EMIT_OP_REG_REG(o_test, rEBX, rEBX);
		// If zero, don't change anything
		EMIT_OP_LABEL(o_jz, aluLabels);

		// Jump to saved position
		EMIT_REG_READ(rEBX);
		EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&yieldRestorePtr);

		EMIT_LABEL(aluLabels);
		aluLabels++;
	}else{
		EMIT_OP(o_nop); // some instruction must be generated so that it is possible to jump to it
		yieldCmd = cmd;
	}
}

#ifdef __linux
void yieldSaveEIP()
{
	asm("push %eax");
	asm("mov 8(%esp), %eax");
	asm("sub %0, %%eax"::"m"(x86BinaryBase):"%eax");
	asm("add $1, %eax");	// jump over ret
	asm("mov %eax, 4(%esi)");
	asm("pop %eax");
}
#else
__declspec(naked) void yieldSaveEIP()
{
	__asm
	{
		push eax; // save eax
		mov eax, [esp+4]; // mov return address to eax
		sub eax, x86BinaryBase;
		add eax, 1;	// jump over ret
		mov [esi+4], eax; // save code address to target variable
		pop eax; // restore eax
		ret;
	}
}
#endif

void (*yieldPtr)() = yieldSaveEIP;

void GenCodeCmdReturn(VMCmd cmd)
{
	EMIT_COMMENT("RET");

	if(cmd.flag & bitRetError)
	{
#ifdef __linux
		// call siglongjmp(target_env, EXCEPTION_FUNCTION_NO_RETURN);
		EMIT_OP_NUM(o_push, EXCEPTION_FUNCTION_NO_RETURN);
		EMIT_OP_NUM(o_push, nullcJmpTarget);
		EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&siglongjmpPtr);
#else
		EMIT_OP_REG_NUM(o_mov, rECX, 0xffffffff);
		EMIT_OP_NUM(o_int, 3);
#endif
		return;
	}
	if(cmd.argument == 0)
	{
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
	}else if(cmd.flag != OTYPE_COMPLEX){
		if(cmd.flag == OTYPE_INT)
		{
			EMIT_OP_REG(o_pop, rEAX);
			EMIT_REG_READ(rEAX);
		}else{
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
			EMIT_REG_READ(rEAX);
			EMIT_REG_READ(rEDX);
		}
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
		if(cmd.helper == 0)
			EMIT_OP_REG_NUM(o_mov, rEBX, cmd.flag);
	}else{
		if(cmd.argument == 4)
		{
			EMIT_OP_REG(o_pop, rEAX);
			EMIT_REG_READ(rEAX);
		}else if(cmd.argument == 8){
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
			EMIT_REG_READ(rEAX);
			EMIT_REG_READ(rEDX);
		}else if(cmd.argument == 12){
			EMIT_OP_REG(o_pop, rECX);
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
			EMIT_REG_READ(rEAX);
			EMIT_REG_READ(rEDX);
			EMIT_REG_READ(rECX);
		}else if(cmd.argument == 16){
			EMIT_OP_REG(o_pop, rEBX);
			EMIT_OP_REG(o_pop, rECX);
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
			EMIT_REG_READ(rEAX);
			EMIT_REG_READ(rEDX);
			EMIT_REG_READ(rECX);
			EMIT_REG_READ(rEBX);
		}else{
			EMIT_OP_REG_REG(o_mov, rEBX, rEDI);
			EMIT_OP_REG_REG(o_mov, rESI, rESP);

			EMIT_OP_REG_RPTR(o_lea, rEDI, sNONE, rEDI, paramBase);
			EMIT_OP_REG_NUM(o_mov, rECX, cmd.argument >> 2);
			EMIT_OP(o_rep_movsd);

			EMIT_OP_REG_REG(o_mov, rEDI, rEBX);

			EMIT_OP_REG_NUM(o_add, rESP, cmd.argument);
		}
		if(cmd.argument > 16)
			EMIT_OP_REG_REG(o_mov, rEAX, rEDI);
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
		if(cmd.helper == 0)
			EMIT_OP_REG_NUM(o_mov, rEBX, 16);
	}
	if(yieldCmd.cmd == cmdYield)
	{
		// Take pointer to closure in hidden argument
		EMIT_OP_REG_RPTR(o_mov, rESI, sDWORD, rEDI, yieldCmd.argument + paramBase);

		// If helper is set, it's yield
		if(yieldCmd.helper)
		{
			// When function is called again, it will continue from instruction after value return.
			EMIT_REG_READ(rESI);
			EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&yieldPtr);
		}else{	// otherwise, it's return
			// When function is called again, start from beginning by reseting  jump offset at 4 byte shift (ExternFuncInfo::Upvalue::next)
			EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESI, 4, 0);
		}
	}
	EMIT_OP(o_ret);
	yieldCmd.cmd = cmdNop;
}


void GenCodeCmdPushVTop(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("PUSHT");

	EMIT_OP_REG(o_push, rEBP);
	EMIT_OP_REG_REG(o_mov, rEBP, rEDI);

	assert(cmd.argument % 16 == 0);
	if(cmd.argument)
		EMIT_OP_REG_NUM(o_add, rEDI, cmd.argument);
	// Clear stack frame
	if(cmd.argument - cmd.helper)
	{
		EMIT_OP_NUM(o_push, cmd.argument - cmd.helper);
		EMIT_OP_NUM(o_push, 0);
		EMIT_OP_REG_RPTR(o_lea, rEAX, sNONE, rNONE, 1, rEBP, cmd.helper + paramBase);
		EMIT_OP_REG(o_push, rEAX);
		EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)memset);
		EMIT_OP_REG(o_call, rECX);
		EMIT_OP_REG_NUM(o_add, rESP, 12);
	}
}

void GenCodeCmdAdd(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ADD int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_add, rEAX, rEDX);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdSub(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SUB int");
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_sub, rEAX, rEDX);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdMul(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MUL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_imul, rEAX, rEDX);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdDiv(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DIV int");
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP(o_cdq);
	EMIT_OP_REG(o_idiv, rEBX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdPow(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("POW int");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)intPow);
	EMIT_CALL_REG(rECX);
#ifdef NULLC_OPTIMIZE_X86
	NULLC::stackRead[(16 + NULLC::stackTop) % NULLC::STACK_STATE_SIZE] = true;
	NULLC::stackRead[(16 + NULLC::stackTop - 1) % NULLC::STACK_STATE_SIZE] = true;
	KILL_REG(rEAX);KILL_REG(rEDX);
	NULLC::InvalidateDependand(rEAX);
	NULLC::InvalidateDependand(rEDX);
#endif
	EMIT_OP_REG_NUM(o_add, rESP, 8);
#ifdef __linux
	EMIT_OP_REG(o_push, rEAX);
#else
	EMIT_OP_REG(o_push, rEDX);
#endif
}

void GenCodeCmdMod(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MOD int");
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP(o_cdq);
	EMIT_OP_REG(o_idiv, rEBX);
	EMIT_OP_REG(o_push, rEDX);
}

void GenCodeCmdLess(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LESS int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_REG(o_cmp, rEDX, rEAX);
	EMIT_OP_REG(o_setl, rECX);
	EMIT_OP_REG(o_push, rECX);
	KILL_REG(rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdGreater(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GREATER int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_REG(o_cmp, rEDX, rEAX);
	EMIT_OP_REG(o_setg, rECX);
	EMIT_OP_REG(o_push, rECX);
	KILL_REG(rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdLEqual(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LEQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_REG(o_cmp, rEDX, rEAX);
	EMIT_OP_REG(o_setle, rECX);
	EMIT_OP_REG(o_push, rECX);
	KILL_REG(rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdGEqual(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GEQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_REG(o_cmp, rEDX, rEAX);
	EMIT_OP_REG(o_setge, rECX);
	EMIT_OP_REG(o_push, rECX);
	KILL_REG(rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdEqual(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("EQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_REG(o_cmp, rEDX, rEAX);
	EMIT_OP_REG(o_sete, rECX);
	EMIT_OP_REG(o_push, rECX);
	KILL_REG(rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdNEqual(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEQUAL int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_REG(o_cmp, rEDX, rEAX);
	EMIT_OP_REG(o_setne, rECX);
	EMIT_OP_REG(o_push, rECX);
	KILL_REG(rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdShl(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SHL int");
	EMIT_OP_REG(o_pop, rECX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_sal, rEAX, rECX);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rECX);
}

void GenCodeCmdShr(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SHR int");
	EMIT_OP_REG(o_pop, rECX);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_REG(o_sar, rEAX, rECX);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rECX);
}

void GenCodeCmdBitAnd(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BAND int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_and, rEAX, rEDX);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdBitOr(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BOR int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_or, rEAX, rEDX);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdBitXor(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BXOR int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_REG_REG(o_xor, rEAX, rEDX);
	EMIT_OP_REG(o_push, rEAX);
	KILL_REG(rEDX);
}

void GenCodeCmdLogAnd(VMCmd cmd)
{
	(void)cmd;
	assert(!"Unknown command cmdLogAnd");
}

void GenCodeCmdLogOr(VMCmd cmd)
{
	(void)cmd;
	assert(!"Unknown command cmdLogOr");
}

void GenCodeCmdLogXor(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LXOR int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEBX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_NUM(o_cmp, rEAX, 0);
	EMIT_OP_REG(o_setne, rECX);
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_REG_NUM(o_cmp, rEBX, 0);
	EMIT_OP_REG(o_setne, rEAX);
	EMIT_OP_REG_REG(o_xor, rEAX, rECX);
	EMIT_OP_REG(o_push, rEAX);
}


void GenCodeCmdAddL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ADD long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_add, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_adc, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdSubL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SUB long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_sub, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_sbb, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdMulL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MUL long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longMul);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdDivL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DIV long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longDiv);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdPowL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("POW long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longPow);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdModL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MOD long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longMod);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdLessL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LESS long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jg, aluLabels);
	EMIT_OP_LABEL(o_jl, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jae, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdGreaterL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GREATER long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jl, aluLabels);
	EMIT_OP_LABEL(o_jg, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jbe, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdLEqualL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LEQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jg, aluLabels);
	EMIT_OP_LABEL(o_jl, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_ja, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdGEqualL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GEQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jl, aluLabels);
	EMIT_OP_LABEL(o_jg, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jb, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdEqualL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("EQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jne, aluLabels);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_jne, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 2;
}

void GenCodeCmdNEqualL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEQUAL long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 4, rEDX);
	EMIT_OP_LABEL(o_jne, aluLabels + 1);
	EMIT_OP_RPTR_REG(o_cmp, sDWORD, rESP, 0, rEAX);
	EMIT_OP_LABEL(o_je, aluLabels);
	EMIT_LABEL(aluLabels + 1);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 2);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 4, 0);
	EMIT_LABEL(aluLabels + 2);
	EMIT_OP_REG(o_pop, rEAX);
	aluLabels += 3;
}

void GenCodeCmdShlL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SHL long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longShl);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdShrL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SHR long");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)longShr);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEDX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}

void GenCodeCmdBitAndL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BAND long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_and, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_and, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdBitOrL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BOR long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_or, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_or, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdBitXorL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BXOR long");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR_REG(o_xor, sDWORD, rESP, 0, rEAX);
	EMIT_OP_RPTR_REG(o_xor, sDWORD, rESP, 4, rEDX);
}

void GenCodeCmdLogAndL(VMCmd cmd)
{
	(void)cmd;
	assert(!"Unknown command cmdLogAndL");
}

void GenCodeCmdLogOrL(VMCmd cmd)
{
	(void)cmd;
	assert(!"Unknown command cmdLogOrL");
}

void GenCodeCmdLogXorL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LXOR long");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 0);
	EMIT_OP_REG_RPTR(o_or, rEBX, sDWORD, rESP, 4);
	EMIT_OP_REG(o_setnz, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 8);
	EMIT_OP_REG_RPTR(o_or, rEBX, sDWORD, rESP, 12);
	EMIT_OP_REG(o_setnz, rECX);
	EMIT_OP_REG_REG(o_xor, rEAX, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
	aluLabels++;
}


void GenCodeCmdAddD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("ADD double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fadd, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 16);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdSubD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("SUB double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fsub, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 16);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdMulD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MUL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fmul, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 16);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdDivD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DIV double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_RPTR(o_fdiv, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 16);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdPowD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("POW double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 16);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(long long)doublePow);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdModD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("MOD double");
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)doubleMod);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdLessD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LESS double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x41);
	EMIT_OP_LABEL(o_jne, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdGreaterD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GREATER double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x05);
	EMIT_OP_LABEL(o_jp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdLEqualD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LEQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x01);
	EMIT_OP_LABEL(o_jne, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdGEqualD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("GEQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x41);
	EMIT_OP_LABEL(o_jp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdEqualD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("EQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}

void GenCodeCmdNEqualD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEQUAL double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jnp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}


void GenCodeCmdNeg(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEG int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_neg, rEAX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdBitNot(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BNOT int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_not, rEAX);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdLogNot(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LNOT int");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, 0, 0);
	EMIT_OP_REG(o_sete, rEAX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
}


void GenCodeCmdNegL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEG long");
	EMIT_OP_RPTR(o_neg, sDWORD, rESP, 0);
	EMIT_OP_RPTR_NUM(o_adc, sDWORD, rESP, 4, 0);
	EMIT_OP_RPTR(o_neg, sDWORD, rESP, 4);
}

void GenCodeCmdBitNotL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("BNOT long");
	EMIT_OP_RPTR(o_not, sDWORD, rESP, 0);
	EMIT_OP_RPTR(o_not, sDWORD, rESP, 4);
}

void GenCodeCmdLogNotL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LNOT long");
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_REG_RPTR(o_mov, rEBX, sDWORD, rESP, 4);
	EMIT_OP_REG_RPTR(o_or, rEBX, sDWORD, rESP, 0);
	EMIT_OP_REG(o_setz, rEAX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 4, rEAX);
	EMIT_OP_REG_NUM(o_add, rESP, 4);
}


void GenCodeCmdNegD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("NEG double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP(o_fchs);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdLogNotD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("LNOT double");
	EMIT_OP(o_fldz);
	EMIT_OP_RPTR(o_fcomp, sQWORD, rESP, 0);
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP(o_fnstsw);
	EMIT_OP_REG_NUM(o_test, rEAX, 0x44);
	EMIT_OP_LABEL(o_jp, aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 1);
	EMIT_OP_LABEL(o_jmp, aluLabels + 1);
	EMIT_LABEL(aluLabels);
	EMIT_OP_RPTR_NUM(o_mov, sDWORD, rESP, 0, 0);
	EMIT_LABEL(aluLabels + 1);
	aluLabels += 2;
}


void GenCodeCmdIncI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("INC int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_NUM(o_add, rEAX, 1);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdIncD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("INC double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP(o_fld1);
	EMIT_OP(o_faddp);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdIncL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("INC long");
	EMIT_OP_RPTR_NUM(o_add, sDWORD, rESP, 0, 1);
	EMIT_OP_RPTR_NUM(o_adc, sDWORD, rESP, 4, 0);
}


void GenCodeCmdDecI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DEC int");
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_NUM(o_sub, rEAX, 1);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdDecD(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DEC double");
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP(o_fld1);
	EMIT_OP(o_fsubp);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fstp, sQWORD, rESP, 0);
}

void GenCodeCmdDecL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DEC long");
	EMIT_OP_RPTR_NUM(o_sub, sDWORD, rESP, 0, 1);
	EMIT_OP_RPTR_NUM(o_sbb, sDWORD, rESP, 4, 0);
}

void (*closureCreateFunc)() = (void(*)())ClosureCreate;

void GenCodeCmdCreateClosure(VMCmd cmd)
{
	EMIT_COMMENT("CREATECLOSURE");

	EMIT_OP_NUM(o_push, cmd.argument);
	EMIT_OP_NUM(o_push, cmd.helper);
	EMIT_OP_REG_RPTR(o_lea, rEBX, sNONE, rEBP, paramBase);
	EMIT_OP_REG(o_push, rEBX);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)closureCreateFunc);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 16);
}

void (*upvaluesCloseFunc)() = (void(*)())CloseUpvalues;

void GenCodeCmdCloseUpvalues(VMCmd cmd)
{
	EMIT_COMMENT("CLOSEUPVALUES");

	EMIT_OP_NUM(o_push, cmd.argument);
	EMIT_OP_NUM(o_push, cmd.flag);
	EMIT_OP_REG_RPTR(o_lea, rEBX, sNONE, rEBP, paramBase);
	EMIT_OP_REG(o_push, rEBX);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)upvaluesCloseFunc);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
}

void GenCodeCmdConvertPtr(VMCmd cmd)
{
	EMIT_COMMENT("CONVERTPTR");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_NUM(o_cmp, rEAX, cmd.argument);
	EMIT_OP_LABEL(o_je, aluLabels);
#ifdef __linux
	EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-16, rEAX);
	// call siglongjmp(target_env, EXCEPTION_CONVERSION_ERROR);
	EMIT_OP_NUM(o_push, EXCEPTION_CONVERSION_ERROR | (cmd.argument << 8));
	EMIT_OP_NUM(o_push, nullcJmpTarget);
	EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&siglongjmpPtr);
#else
	EMIT_OP_REG_NUM(o_mov, rECX, cmd.argument);
	EMIT_OP_NUM(o_int, 3);
#endif
	EMIT_LABEL(aluLabels);
	aluLabels++;
}

ExternTypeInfo* (*getTypeListFunc)() = GetTypeList;

char* checkedCopy(char* top, int typeID, char* ptr, unsigned length)
{
	ExternTypeInfo* typelist = getTypeListFunc();

	NULLCRef r;
	r.ptr = ptr;
	r.typeID = typeID;
	if(ptr >= top && IsPointerUnmanaged(r))
	{
		ExternTypeInfo &type = typelist[typeID];
		if(type.arrSize == ~0u)
		{
			char *copy = (char*)NULLC::AllocObject(typelist[type.subType].size * length);
			memcpy(copy, ptr, typelist[type.subType].size * length);
			ptr = copy;
		}else{
			unsigned int objSize = type.size;
			char *copy = (char*)NULLC::AllocObject(objSize);
			memcpy(copy, ptr, objSize);
			ptr = copy;
		}
	}
	return ptr;
}

char* (*checkedCopyPtr)(char*, int, char*, unsigned) = checkedCopy;

void GenCodeCmdCheckedRet(VMCmd cmd)
{
	// stack top: ptr, size
	EMIT_OP_NUM(o_push, cmd.argument);
	EMIT_OP_REG_RPTR(o_lea, rEBX, sNONE, rEBP, paramBase);
	EMIT_OP_REG(o_push, rEBX);
	EMIT_OP_RPTR(o_call, sDWORD, rNONE, 1, rNONE, (unsigned)(intptr_t)&checkedCopyPtr);
	EMIT_OP_REG_NUM(o_add, rESP, 12);
	EMIT_OP_REG(o_push, rEAX);
}

#endif
