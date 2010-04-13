#include "CodeGen_X86.h"
#include "StdLib_X86.h"

#ifdef NULLC_BUILD_X86_JIT

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

	void	InvalidateState()
	{
		stackTop = 0;
		for(unsigned int i = 0; i < STACK_STATE_SIZE; i++)
			stack[i].type = x86Argument::argNone;
		for(unsigned int i = 0; i < 9; i++)
			reg[i].type = x86Argument::argNone;
	}

	void	InvalidateDependand(x86Reg dreg)
	{
		for(unsigned int i = 0; i < NULLC::STACK_STATE_SIZE; i++)
		{
			if(NULLC::stack[i].type == x86Argument::argReg && NULLC::stack[i].reg == dreg)
				NULLC::stack[i].type = x86Argument::argNone;
			if(NULLC::stack[i].type == x86Argument::argPtr && (NULLC::stack[i].ptrBase == dreg || NULLC::stack[i].ptrIndex == dreg))
				NULLC::stack[i].type = x86Argument::argNone;
		}
		for(unsigned int i = 0; i < 9; i++)
		{
			if(reg[i].type == x86Argument::argReg && reg[i].reg == dreg)
				reg[i].type = x86Argument::argNone;
			if(reg[i].type == x86Argument::argPtr && (reg[i].ptrBase == dreg || reg[i].ptrIndex == dreg))
				reg[i].type = x86Argument::argNone;
		}
	}
}
#endif

x86Instruction	*x86Op = NULL, *x86Base = NULL;
ExternFuncInfo	*x86Functions = NULL;
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

void EMIT_LABEL(unsigned int labelID, bool invalidate = true)
{
#ifdef NULLC_OPTIMIZE_X86
	if(invalidate)
		NULLC::InvalidateState();
#else
	(void)invalidate;
#endif
	x86Op->name = o_label;
	x86Op->labelID = labelID;
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
		assert(NULLC::reg[rECX].type == x86Argument::argNumber);
		unsigned int stackEntryCount = NULLC::reg[rECX].num;
		stackEntryCount = stackEntryCount > NULLC::STACK_STATE_SIZE ? NULLC::STACK_STATE_SIZE : stackEntryCount;
		for(unsigned int i = 0; i < stackEntryCount; i++)
			NULLC::stackRead[(16 + NULLC::stackTop - i) % NULLC::STACK_STATE_SIZE] = true;
	}else if(op == o_cdq){
		NULLC::InvalidateDependand(rEDX);
		NULLC::reg[rEDX].type = x86Argument::argNone;
		NULLC::regRead[rEDX] = false;
		NULLC::regUpdate[rEDX] = (unsigned int)(x86Op - x86Base);
	}
#endif
	x86Op->name = op;
	x86Op++;
}
void EMIT_OP_LABEL(x86Command op, unsigned int labelID, bool invalidate = true, bool longJump = false)
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
#else
	(void)invalidate;
	(void)longJump;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argLabel;
	x86Op->argA.labelID = labelID;
	x86Op++;
}
void EMIT_OP_REG(x86Command op, x86Reg reg1)
{
#ifdef NULLC_OPTIMIZE_X86
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
				NULLC::InvalidateDependand(reg1);
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
	NULLC::regRead[reg1] = (op == o_push || op == o_imul || op == o_idiv || op == o_neg || op == o_not);

	if(op != o_push && op != o_pop && op != o_call && op != o_imul && op < o_setl && op > o_setnz)
		__asm int 3;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op++;
}
void EMIT_OP_FPUREG(x86Command op, x87Reg reg1)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argFPReg;
	x86Op->argA.fpArg = reg1;
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

	if(op != o_push && op != o_int)
		__asm int 3;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argNumber;
	x86Op->argA.num = num;
	x86Op++;
}

void EMIT_OP_ADDR(x86Command op, x86Size size, unsigned int addr)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op == o_push)
	{
		unsigned int index = (++NULLC::stackTop) % NULLC::STACK_STATE_SIZE;
		NULLC::stack[index] = x86Argument(size, addr);
		NULLC::stackRead[index] = false;
		NULLC::stackUpdate[index] = (unsigned int)(x86Op - x86Base);
	}
	if(op == o_pop)
	{
		unsigned int index = (NULLC::stackTop) % NULLC::STACK_STATE_SIZE;
		if(NULLC::stack[index].type == x86Argument::argReg || NULLC::stack[index].type == x86Argument::argNumber)
		{
			x86Op->name = o_mov;
			x86Op->argA.type = x86Argument::argPtr;
			x86Op->argA.ptrSize = size;
			x86Op->argA.ptrNum = addr;
			x86Op->argB = NULLC::stack[index];
			x86Op++;
			EMIT_OP_REG_NUM(o_add, rESP, 4);
			return;
		}
		NULLC::stackTop--;
		NULLC::stack[index].type = x86Argument::argNone;
	}
	if(op != o_push && op != o_pop && op != o_fstp && op != o_fld && op != o_fild && op != o_fadd && op != o_fsub && op != o_fmul && op != o_fdiv && op != o_fcomp)
		__asm int 3;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrNum = addr;
	x86Op++;
}
void EMIT_OP_RPTR(x86Command op, x86Size size, x86Reg reg2, unsigned int shift)
{
#ifdef NULLC_OPTIMIZE_X86
	if(NULLC::reg[reg2].type == x86Argument::argReg)
		reg2 = NULLC::reg[reg2].reg;
	if(op == o_push)
	{
		unsigned int index  = (++NULLC::stackTop) % NULLC::STACK_STATE_SIZE;
		NULLC::stack[index] = x86Argument(size, reg2, shift);
		NULLC::stackRead[index] = false;
		NULLC::stackUpdate[index] = (unsigned int)(x86Op - x86Base);
	}else if(op == o_pop){

		unsigned int index = (NULLC::stackTop) % NULLC::STACK_STATE_SIZE;
		if(NULLC::stack[index].type == x86Argument::argReg || NULLC::stack[index].type == x86Argument::argNumber)
		{
			x86Op->name = o_mov;
			x86Op->argA.type = x86Argument::argPtr;
			x86Op->argA.ptrSize = size;
			x86Op->argA.ptrBase = reg2;
			x86Op->argA.ptrNum = shift;
			x86Op->argB = NULLC::stack[index];
			x86Op++;
			NULLC::regRead[reg2] = true;
			EMIT_OP_REG_NUM(o_add, rESP, 4);
			return;
		}
		NULLC::stackTop--;
		NULLC::stack[index].type = x86Argument::argNone;

	}else if(size == sDWORD && reg2 == rESP && shift < (NULLC::STACK_STATE_SIZE * 4)){
		x86Argument &target = NULLC::stack[(16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE];
		target.type = x86Argument::argNone;
	}else if((op == o_fld || op == o_fild || op == o_fadd || op == o_fsub || op == o_fmul || op == o_fdiv || op == o_fcomp) && reg2 == rESP && shift < (NULLC::STACK_STATE_SIZE * 4)){
		if(x86LookBehind && op == o_fld && x86Op[-1].name == o_fstp && x86Op[-1].argA.ptrSize == size && x86Op[-1].argA.ptrBase == reg2 && x86Op[-1].argA.ptrNum == (int)shift)
		{
			x86Op[-1].name = o_fst;
			optiCount++;
			return;
		}
		
		unsigned int index = (16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE;

		if(NULLC::stack[index].type == x86Argument::argPtr)
		{
			if(NULLC::stack[index].ptrBase == rNONE)
			{
				EMIT_OP_ADDR(op, size, NULLC::stack[index].ptrNum);
				return;
			}
			reg2 = NULLC::stack[index].ptrBase;
			shift = NULLC::stack[index].ptrNum;
		}else{
			NULLC::stackRead[index] = true;
			if(size == sQWORD)
			{
				unsigned int index2 = (16 + NULLC::stackTop - (shift >> 2) - 1) % NULLC::STACK_STATE_SIZE;
				NULLC::stackRead[index2] = true;
			}
		}
	}else if((op == o_fstp || op == o_fst) && size == sQWORD && reg2 == rESP && shift < (NULLC::STACK_STATE_SIZE * 4)){
		unsigned int target = (16 + NULLC::stackTop - (shift >> 2)) % NULLC::STACK_STATE_SIZE;
		NULLC::stack[target].type = x86Argument::argFPReg;
		NULLC::stackRead[target] = false;
		NULLC::stackUpdate[target] = (unsigned int)(x86Op - x86Base);

		target = (16 + NULLC::stackTop - (shift >> 2) - 1) % NULLC::STACK_STATE_SIZE;
		NULLC::stack[target].type = x86Argument::argFPReg;
		NULLC::stackRead[target] = false;
		NULLC::stackUpdate[target] = (unsigned int)(x86Op - x86Base);
	}
		
	NULLC::regRead[reg2] = true;

	if(op != o_push && op != o_pop && op != o_neg && op != o_not && op != o_idiv && op != o_fstp && op != o_fld && op != o_fadd && op != o_fsub && op != o_fmul && op != o_fdiv && op != o_fcomp && op != o_fild && op != o_fistp && op != o_fst)
		__asm int 3;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrBase = reg2;
	x86Op->argA.ptrNum = shift;
	x86Op++;
}
void EMIT_OP_REG_NUM(x86Command op, x86Reg reg1, unsigned int num)
{
#ifdef NULLC_OPTIMIZE_X86
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
		KILL_REG(reg1);
		NULLC::InvalidateDependand(reg1);
		NULLC::reg[reg1] = x86Argument(num);
		NULLC::regUpdate[reg1] = (unsigned int)(x86Op - x86Base);
		NULLC::regRead[reg1] = false;
	}else{
		NULLC::InvalidateDependand(reg1);
		NULLC::reg[reg1].type = x86Argument::argNone;
	}

	if(op != o_add && op != o_sub && op != o_mov && op != o_test && op != o_imul && op != o_shl && op != o_cmp)
		__asm int 3;

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
	if(op == o_mov)
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
	}else if(op == o_add && NULLC::reg[reg2].type == x86Argument::argNumber){
		EMIT_OP_REG_NUM(op, reg1, NULLC::reg[reg2].num);
		return;
	}else if(op == o_add && NULLC::reg[reg1].type == x86Argument::argNumber){
		EMIT_OP_REG_NUM(op, reg2, NULLC::reg[reg1].num);
		EMIT_OP_REG_REG(o_mov, reg1, reg2);
		return;
	}else if(op == o_cmp){
		NULLC::regRead[reg1] = true;
	}else if(op == o_imul){
		if(NULLC::reg[reg2].type == x86Argument::argPtr)
		{
			EMIT_OP_REG_RPTR(o_imul, reg1, sDWORD, NULLC::reg[reg2].ptrIndex, NULLC::reg[reg2].ptrMult, NULLC::reg[reg2].ptrBase, NULLC::reg[reg2].ptrNum);
			return;
		}
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
void EMIT_OP_REG_ADDR(x86Command op, x86Reg reg1, x86Size size, unsigned int addr)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op == o_mov)
	{
		KILL_REG(reg1);
		NULLC::InvalidateDependand(reg1);

		NULLC::reg[reg1] = x86Argument(size, addr);
		NULLC::regUpdate[reg1] = (unsigned int)(x86Op - x86Base);
		NULLC::regRead[reg1] = false;
	}else{
		NULLC::reg[reg1].type = x86Argument::argNone;
	}
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = reg1;
	x86Op->argB.type = x86Argument::argPtr;
	x86Op->argB.ptrSize = size;
	x86Op->argB.ptrNum = addr;
	x86Op++;
}
void EMIT_OP_REG_RPTR(x86Command op, x86Reg reg1, x86Size size, x86Reg index, unsigned int mult, x86Reg base, unsigned int shift)
{
#ifdef NULLC_OPTIMIZE_X86
	if(op != o_mov && op != o_lea && op != o_movsx && op != o_or && op != o_imul)
		__asm int 3;

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
	if(op == o_mov || op == o_movsx || op == o_lea)
	{
		KILL_REG(reg1);
		NULLC::InvalidateDependand(reg1);
	}
	if(op == o_mov || op == o_movsx)
	{
		NULLC::reg[reg1] = x86Argument(size, index, mult, base, shift);
		NULLC::regUpdate[reg1] = (unsigned int)(x86Op - x86Base);
		NULLC::regRead[reg1] = false;
	}else if(op == o_lea || op == o_imul){
		NULLC::reg[reg1].type = x86Argument::argNone;
	}
	NULLC::regRead[base] = true;
	NULLC::regRead[index] = true;

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
#ifdef NULLC_OPTIMIZE_X86
	if(NULLC::reg[base].type == x86Argument::argNumber)
	{
		shift += NULLC::reg[base].num;
		base = rNONE;
	}
	NULLC::regRead[index] = true;
	NULLC::regRead[base] = true;

	KILL_REG(dst);
	NULLC::reg[dst] = x86Argument(size, index, mult, base, shift);
	NULLC::regRead[dst] = false;
	NULLC::regUpdate[dst] = (unsigned int)(x86Op - x86Base);
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argReg;
	x86Op->argA.reg = dst;
	x86Op->argB.type = x86Argument::argPtr;
	x86Op->argB.ptrSize = size;
	x86Op->argB.ptrIndex = index;
	x86Op->argB.ptrMult = mult;
	x86Op->argB.ptrBase = base;
	x86Op->argB.ptrNum = shift;
	x86Op++;
}
void EMIT_OP_ADDR_NUM(x86Command op, x86Size size, unsigned int addr, unsigned int number)
{
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrNum = addr;
	x86Op->argB.type = x86Argument::argNumber;
	x86Op->argB.num = number;
	x86Op++;
}
void EMIT_OP_ADDR_REG(x86Command op, x86Size size, unsigned int addr, x86Reg reg2)
{
#ifdef NULLC_OPTIMIZE_X86
	if(NULLC::reg[reg2].type == x86Argument::argNumber)
	{
		EMIT_OP_ADDR_NUM(op, size, addr, NULLC::reg[reg2].num);
		return;
	}else if(NULLC::reg[reg2].type == x86Argument::argReg){
		reg2 = NULLC::reg[reg2].reg;
	}
	NULLC::regRead[reg2] = true;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrNum = addr;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
	x86Op++;
}

void EMIT_OP_RPTR_REG(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, x86Reg reg2)
{
#ifdef NULLC_OPTIMIZE_X86
	if(NULLC::reg[reg1].type == x86Argument::argReg)
		reg1 = NULLC::reg[reg1].reg;

	if(NULLC::reg[reg2].type == x86Argument::argNumber)
	{
		EMIT_OP_RPTR_NUM(op, size, reg1, shift, NULLC::reg[reg2].num);
		return;
	}else if(NULLC::reg[reg2].type == x86Argument::argReg){
		reg2 = NULLC::reg[reg2].reg;
	}
	if(size == sDWORD && reg1 == rESP && shift < (NULLC::STACK_STATE_SIZE * 4))
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
	NULLC::regRead[reg1] = true;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrBase = reg1;
	x86Op->argA.ptrNum = shift;
	x86Op->argB.type = x86Argument::argReg;
	x86Op->argB.reg = reg2;
	x86Op++;
}

void EMIT_OP_RPTR_NUM(x86Command op, x86Size size, x86Reg reg1, unsigned int shift, unsigned int num)
{
#ifdef NULLC_OPTIMIZE_X86
	if(NULLC::reg[reg1].type == x86Argument::argReg)
		reg1 = NULLC::reg[reg1].reg;

	if(size == sDWORD && reg1 == rESP && shift < (NULLC::STACK_STATE_SIZE * 4))
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
	NULLC::regRead[reg1] = true;
#endif
	x86Op->name = op;
	x86Op->argA.type = x86Argument::argPtr;
	x86Op->argA.ptrSize = size;
	x86Op->argA.ptrBase = reg1;
	x86Op->argA.ptrNum = shift;
	x86Op->argB.type = x86Argument::argNumber;
	x86Op->argB.num = num;
	x86Op++;
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

void OptimizationLookBehind(bool allow)
{
	x86LookBehind = allow;
#ifdef NULLC_OPTIMIZE_X86
	if(!allow)
	{
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

void SetFunctionList(ExternFuncInfo *list)
{
	x86Functions = list;
}

void SetContinuePtr(int* continueVar)
{
	x86Continue = continueVar;
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
			EMIT_OP_REG_RPTR(o_lea, rESI, sDWORD, rEBP, cmd.argument+paramBase);
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

		EMIT_OP_REG_RPTR(o_lea, rESI, sDWORD, rEDX, cmd.argument);
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
			EMIT_OP_REG_RPTR(o_lea, rEDI, sDWORD, rEBP, cmd.argument+paramBase);
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
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
}

void GenCodeCmdMovFloatStk(VMCmd cmd)
{
	EMIT_COMMENT("MOV float stack");

	EMIT_OP_REG(o_pop, rEDX);
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fstp, sDWORD, rEDX, cmd.argument);
	KILL_REG(rEBX);KILL_REG(rECX);KILL_REG(rEDX);
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
		EMIT_OP_REG_RPTR(o_lea, rEDI, sDWORD, rEDX, cmd.argument);
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


void GenCodeCmdDtoI(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DTOI");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_REG_NUM(o_sub, rESP, 4);
	EMIT_OP_RPTR(o_fistp, sDWORD, rESP, 0);
}

void GenCodeCmdDtoL(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("DTOL");

	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
	EMIT_OP_RPTR(o_fistp, sQWORD, rESP, 0);
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
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels, false);
	aluLabels++;

	EMIT_OP_REG(o_pop, rEBX);	// Take address

	// Multiply it
	if(cmd.helper == 1)
	{
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rEBX, sNONE, rEAX, 1, rEBX, 0);
	}else if(cmd.helper == 2){
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rEBX, sNONE, rEAX, 2, rEBX, 0);
	}else if(cmd.helper == 4){
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rEBX, sNONE, rEAX, 4, rEBX, 0);
	}else if(cmd.helper == 8){
		EMIT_OP_REG_REG_MULT_REG_NUM(o_lea, rEBX, sNONE, rEAX, 8, rEBX, 0);
	}else if(cmd.helper == 16){
		EMIT_OP_REG_NUM(o_shl, rEAX, 4);
		EMIT_OP_REG_REG(o_add, rEBX, rEAX);
	}else{
		EMIT_OP_REG_NUM(o_imul, rEAX, cmd.helper);
		EMIT_OP_REG_REG(o_add, rEBX, rEAX);
	}
	if(cmd.cmd == cmdIndex)
		EMIT_OP_REG(o_push, rEBX);
	else
		EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEBX);
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
		EMIT_OP_REG_RPTR(o_lea, rEAX, sDWORD, rEBP, cmd.argument + paramBase);
	else
		EMIT_OP_REG_NUM(o_mov, rEAX, cmd.argument + paramBase);
	EMIT_OP_REG(o_push, rEAX);
}

void GenCodeCmdFuncAddr(VMCmd cmd)
{
	EMIT_COMMENT("FUNCADDR");

	if(x86Functions[cmd.argument].funcPtr == NULL)
	{
		EMIT_OP_REG_LABEL(o_lea, rEAX, LABEL_GLOBAL + x86Functions[cmd.argument].address, 0);
		EMIT_OP_REG(o_push, rEAX);
	}else{
		EMIT_OP_NUM(o_push, (int)(intptr_t)x86Functions[cmd.argument].funcPtr);
	}
}

void GenCodeCmdSetRange(VMCmd cmd)
{
	unsigned int elCount = x86Op[-1].argA.num;

	EMIT_COMMENT("SETRANGE");

	EMIT_OP_REG(o_pop, rEBX);
	unsigned int typeSizeD[] = { 1, 2, 4, 8, 4, 8 };

	// start address
	EMIT_OP_REG_RPTR(o_lea, rEBX, sDWORD, rEBP, paramBase + cmd.argument);
	// end address
	EMIT_OP_REG_RPTR(o_lea, rECX, sDWORD, rEBP, paramBase + cmd.argument + (elCount - 1) * typeSizeD[(cmd.helper>>2)&0x00000007]);
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
		EMIT_OP_REG_RPTR(o_lea, rEDI, sDWORD, rEDI, paramBase);
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

			EMIT_OP_REG_RPTR(o_lea, rESI, sDWORD, rEAX, paramBase);
			EMIT_OP_REG_REG(o_mov, rEDI, rESP);
			EMIT_OP_REG_NUM(o_mov, rECX, cmd.helper >> 2);
			EMIT_OP(o_rep_movsd);

			EMIT_OP_REG_REG(o_mov, rEDI, rEBX);
		}
	}
}

void GenCodeCmdCall(VMCmd cmd)
{
	EMIT_COMMENT("CALL");

	if(x86Functions[cmd.argument].address != -1)
	{
		GenCodeCmdCallEpilog(x86Functions[cmd.argument].bytesToPop);

		EMIT_OP_ADDR(o_push, sDWORD, paramBase-4);
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-4, rESP);
		EMIT_OP_LABEL(o_call, LABEL_GLOBAL | JUMP_NEAR | x86Functions[cmd.argument].address);
		EMIT_OP_ADDR(o_pop, sDWORD, paramBase-4);
		
		GenCodeCmdCallProlog(cmd);
	}else{
		unsigned int bytesToPop = x86Functions[cmd.argument].bytesToPop;

		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-8, rESP);
		EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)x86Functions[cmd.argument].funcPtr);
		EMIT_OP_REG(o_call, rECX);

		static int continueLabel = 0;
		EMIT_OP_REG_ADDR(o_mov, rECX, sDWORD, (int)(intptr_t)x86Continue);
		EMIT_OP_REG_REG(o_test, rECX, rECX);
		EMIT_OP_LABEL(o_jnz, aluLabels);
		EMIT_OP_REG_REG(o_mov, rECX, rESP);	// esp is very likely to contain neither 0 or ~0, so we can distinguish
		EMIT_OP(o_int);						// array out of bounds and function with no return errors from this one
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
		}
	}
}

void GenCodeCmdCallPtr(VMCmd cmd)
{
	EMIT_COMMENT("CALLPTR");

	assert(cmd.argument >= 4);
	EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, cmd.argument-4, ~0u);
	EMIT_OP_LABEL(o_jne, aluLabels);

	// external function call
	{
		EMIT_OP_REG_RPTR(o_mov, rEAX, sDWORD, rESP, cmd.argument);
		EMIT_OP_REG(o_call, rEAX);
	 
		static int continueLabel = 0;
		EMIT_OP_REG_ADDR(o_mov, rECX, sDWORD, (int)(intptr_t)x86Continue);
		EMIT_OP_REG_REG(o_test, rECX, rECX);
		EMIT_OP_LABEL(o_jnz, aluLabels + 3);
		EMIT_OP_REG_REG(o_mov, rECX, rESP); // esp is very likely to contain neither 0 or ~0, so we can distinguish
		EMIT_OP(o_int);						// array out of bounds and function with no return errors from this one
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

		EMIT_OP_REG(o_pop, rEAX);
		EMIT_OP_ADDR(o_push, sDWORD, paramBase-4);
		EMIT_OP_ADDR_REG(o_mov, sDWORD, paramBase-4, rESP);

		EMIT_OP_REG_REG(o_test, rEAX, rEAX);
		EMIT_OP_LABEL(o_jnz, aluLabels + 1);
		EMIT_OP_REG_NUM(o_mov, rECX, 0xDEADBEEF);
		EMIT_OP_NUM(o_int, 3);
		EMIT_LABEL(aluLabels + 1);

		EMIT_OP_REG(o_call, rEAX);
		EMIT_OP_ADDR(o_pop, sDWORD, paramBase-4);

		GenCodeCmdCallProlog(cmd);
	}

	EMIT_LABEL(aluLabels + 2);

	aluLabels += 4;
}

void GenCodeCmdReturn(VMCmd cmd)
{
	EMIT_COMMENT("RET");

	if(cmd.flag & bitRetError)
	{
		EMIT_OP_REG_NUM(o_mov, rECX, 0xffffffff);
		EMIT_OP_NUM(o_int, 3);
		return;
	}
	if(cmd.argument == 0)
	{
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
		EMIT_OP(o_ret);
		return;
	}
	if(cmd.flag != OTYPE_COMPLEX)
	{
		if(cmd.flag == OTYPE_INT)
		{
			EMIT_OP_REG(o_pop, rEAX);
		}else{
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}
		EMIT_OP_REG_REG(o_mov, rEDI, rEBP);
		EMIT_OP_REG(o_pop, rEBP);
		if(cmd.helper == 0)
			EMIT_OP_REG_NUM(o_mov, rEBX, cmd.flag);
	}else{
		if(cmd.argument == 4)
		{
			EMIT_OP_REG(o_pop, rEAX);
		}else if(cmd.argument == 8){
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}else if(cmd.argument == 12){
			EMIT_OP_REG(o_pop, rECX);
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}else if(cmd.argument == 16){
			EMIT_OP_REG(o_pop, rEBX);
			EMIT_OP_REG(o_pop, rECX);
			EMIT_OP_REG(o_pop, rEDX);
			EMIT_OP_REG(o_pop, rEAX);
		}else{
			EMIT_OP_REG_REG(o_mov, rEBX, rEDI);
			EMIT_OP_REG_REG(o_mov, rESI, rESP);

			EMIT_OP_REG_RPTR(o_lea, rEDI, sDWORD, rEDI, paramBase);
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
	EMIT_OP(o_ret);
}


void GenCodeCmdPushVTop(VMCmd cmd)
{
	(void)cmd;
	EMIT_COMMENT("PUSHT");

	EMIT_OP_REG(o_push, rEBP);
	EMIT_OP_REG_REG(o_mov, rEBP, rEDI);

	if(cmd.argument)
		EMIT_OP_REG_NUM(o_add, rEDI, (cmd.argument & 0xfffffff0) + 16);
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
	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG(o_pop, rEBX);
#ifdef NULLC_OPTIMIZE_X86
	NULLC::regRead[rEAX] = true;
	NULLC::regRead[rEBX] = true;
#endif
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)intPow);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG(o_push, rEDX);
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
	EMIT_OP_REG_REG(o_xor, rEAX, rEAX);
	EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, 0, 0);
	EMIT_OP_REG(o_setne, rEAX);
	EMIT_OP_REG_REG(o_xor, rECX, rECX);
	EMIT_OP_RPTR_NUM(o_cmp, sDWORD, rESP, 4, 0);
	EMIT_OP_REG(o_setne, rECX);
	EMIT_OP_REG_REG(o_xor, rEAX, rECX);
	EMIT_OP_REG(o_pop, rECX);
	EMIT_OP_RPTR_REG(o_mov, sDWORD, rESP, 0, rEAX);
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
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 0);
	EMIT_OP_RPTR(o_fld, sQWORD, rESP, 8);
	EMIT_OP_REG_NUM(o_add, rESP, 16);
	EMIT_OP(o_fprem);
	EMIT_OP_FPUREG(o_fstp, rST1);
	EMIT_OP_REG_NUM(o_sub, rESP, 8);
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

void (*closureCreateFunc)() = NULL;
void SetClosureCreateFunc(void (*f)())
{
	closureCreateFunc = f;
}

void GenCodeCmdCreateClosure(VMCmd cmd)
{
	EMIT_COMMENT("CREATECLOSURE");

	EMIT_OP_NUM(o_push, cmd.argument);
	EMIT_OP_NUM(o_push, cmd.helper);
	EMIT_OP_REG_RPTR(o_lea, rEBX, sDWORD, rEBP, paramBase);
	EMIT_OP_REG(o_push, rEBX);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)closureCreateFunc);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 16);
}

void (*upvaluesCloseFunc)() = NULL;
void SetUpvaluesCloseFunc(void (*f)())
{
	upvaluesCloseFunc = f;
}

void GenCodeCmdCloseUpvalues(VMCmd cmd)
{
	EMIT_COMMENT("CLOSEUPVALUES");

	EMIT_OP_NUM(o_push, cmd.argument);
	EMIT_OP_REG_RPTR(o_lea, rEBX, sDWORD, rEBP, paramBase);
	EMIT_OP_REG(o_push, rEBX);
	EMIT_OP_REG_NUM(o_mov, rECX, (int)(intptr_t)upvaluesCloseFunc);
	EMIT_OP_REG(o_call, rECX);
	EMIT_OP_REG_NUM(o_add, rESP, 8);
}

void GenCodeCmdConvertPtr(VMCmd cmd)
{
	EMIT_COMMENT("CONVERTPTR");

	EMIT_OP_REG(o_pop, rEAX);
	EMIT_OP_REG_NUM(o_cmp, rEAX, cmd.argument);
	EMIT_OP_LABEL(o_je, aluLabels);
	EMIT_OP_REG_NUM(o_mov, rECX, cmd.argument);
	EMIT_OP_NUM(o_int, 3);
	EMIT_LABEL(aluLabels);
	aluLabels++;
}

#endif
