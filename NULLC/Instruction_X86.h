#pragma once

#include "stdafx.h"

enum x86Reg
{
	rNONE,

	rEAX,
	rEBX,
	rECX,
	rEDX,
	rESP,
	rEDI,
	rEBP,
	rESI,

	rRAX = rEAX,
	rRBX = rEBX,
	rRCX = rECX,
	rRDX = rEDX,
	rRSP = rESP,
	rRDI = rEDI,
	rRBP = rEBP,
	rRSI = rESI,
	rR8,
	rR9,
	rR10,
	rR11,
	rR12,
	rR13,
	rR14,
	rR15,

	rRegCount
};

enum x86XmmReg
{
	rXMM0,
	rXMM1,
	rXMM2,
	rXMM3,
	rXMM4,
	rXMM5,
	rXMM6,
	rXMM7,
	rXMM8,
	rXMM9,
	rXMM10,
	rXMM11,
	rXMM12,
	rXMM13,
	rXMM14,
	rXMM15,

	rXmmRegCount
};

enum x86Size{ sNONE, sBYTE, sWORD, sDWORD, sQWORD };

enum x86Cond{ condO, condNO, condB, condC, condNAE, condAE, condNB, condNC, condE, condZ, condNE, condNZ,
				condBE, condNA, condA, condNBE, condS, condNS, condP, condPE, condNP, condPO,
				condL, condNGE, condGE, condNL, condLE, condNG, condG, condNLE };

const unsigned int JUMP_NEAR = (unsigned int)(1u << 31u);

// jump ID markers for assembly printout
const unsigned int LABEL_GLOBAL = 1 << 30;

enum x86Command
{
	o_none,

	o_mov,
	o_movsx,
	o_push,
	o_pop,
	o_lea,
	o_cdq,
	o_cqo,
	o_rep_movsd,
	o_rep_stosb,
	o_rep_stosw,
	o_rep_stosd,
	o_rep_stosq,

	o_jmp,
	o_ja,
	o_jae,
	o_jb,
	o_jbe,
	o_je,
	o_jg,
	o_jl,
	o_jne,
	o_jnp,
	o_jp,
	o_jge,
	o_jle,
	o_call,
	o_ret,

	o_neg,
	o_add,
	o_adc,
	o_sub,
	o_sbb,
	o_imul,
	o_idiv,
	o_shl,
	o_sal,
	o_sar,
	o_not,
	o_and,
	o_or,
	o_xor,
	o_cmp,
	o_test,

	o_setl,
	o_setg,
	o_setle,
	o_setge,
	o_sete,
	o_setne,
	o_setz,
	o_setnz,

	o_movss,
	o_movsd,
	o_movd,
	o_movsxd,
	o_cvtss2sd,
	o_cvtsd2ss,
	o_cvttsd2si,
	o_cvtsi2sd,
	o_addsd,
	o_subsd,
	o_mulsd,
	o_divsd,
	o_sqrtsd,
	o_cmpeqsd,
	o_cmpltsd,
	o_cmplesd,
	o_cmpneqsd,

	o_int,
	o_label,
	o_use32,
	o_nop,
	o_other,

	o_read_register,
	o_kill_register,
	o_set_tracking,

	o_mov64,

	o_neg64,
	o_add64,
	o_sub64,
	o_imul64,
	o_idiv64,
	o_sal64,
	o_sar64,
	o_not64,
	o_and64,
	o_or64,
	o_xor64,
	o_cmp64,

	o_cvttsd2si64,
	o_cvtsi2sd64,

	// Aliases
	o_jc = o_jb,
	o_jz = o_je,
	o_jnz = o_jne
};

struct CodeGenRegVmStateContext;

struct x86Argument
{
	// Argument type
	enum ArgType
	{
		argNone,
		
		argNumber,
		argReg,
		argXmmReg,
		argPtr,
		argPtrLabel,
		argLabel,
		argImm64
	};

	// no argument
	x86Argument()
	{
		this->type = argNone;
		this->imm64Arg = 0;
		this->ptrBase = rNONE;
		this->ptrIndex = rNONE;
		this->ptrMult = 0;
		this->ptrNum = 0;
	}

	// immediate number
	explicit x86Argument(int num)
	{
		this->type = argNumber;
		this->num = num;
		this->ptrBase = rNONE;
		this->ptrIndex = rNONE;
		this->ptrMult = 0;
		this->ptrNum = 0;
	}

	explicit x86Argument(unsigned num)
	{
		this->type = argNumber;
		this->num = num;
		this->ptrBase = rNONE;
		this->ptrIndex = rNONE;
		this->ptrMult = 0;
		this->ptrNum = 0;
	}

	// register
	explicit x86Argument(x86Reg reg)
	{
		this->type = argReg;
		this->reg = reg;
		this->ptrBase = rNONE;
		this->ptrIndex = rNONE;
		this->ptrMult = 0;
		this->ptrNum = 0;
	}

	// sse register
	explicit x86Argument(x86XmmReg xmmReg)
	{
		this->type = argXmmReg;
		this->xmmArg = xmmReg;
		this->ptrBase = rNONE;
		this->ptrIndex = rNONE;
		this->ptrMult = 0;
		this->ptrNum = 0;
	}

	// size [num]
	x86Argument(x86Size size, unsigned num)
	{
		this->type = argPtr;
		this->ptrSize = size;
		this->ptrBase = rNONE;
		this->ptrIndex = rNONE;
		this->ptrMult = 1;
		this->ptrNum = num;
	}

	// size [register + num]
	x86Argument(x86Size size, x86Reg regA, unsigned num)
	{
		this->type = argPtr;
		this->ptrSize = size;
		this->ptrBase = regA;
		this->ptrIndex = rNONE;
		this->ptrMult = 1;
		this->ptrNum = num;
	}

	// size [register + register + num]
	x86Argument(x86Size size, x86Reg regA, x86Reg regB, unsigned num)
	{
		this->type = argPtr;
		this->ptrSize = size;
		this->ptrBase = regB;
		this->ptrIndex = regA;
		this->ptrMult = 1;
		this->ptrNum = num;
	}

	// size [register * multiplier + register + num]
	x86Argument(x86Size size, x86Reg regA, unsigned mult, x86Reg regB, unsigned num)
	{
		this->type = argPtr;
		this->ptrSize = size;
		this->ptrBase = regB;
		this->ptrIndex = regA;
		this->ptrMult = mult;
		this->ptrNum = num;
	}

	// long immediate number
	explicit x86Argument(long long num)
	{
		this->type = argImm64;
		this->imm64Arg = num;
		this->ptrBase = rNONE;
		this->ptrIndex = rNONE;
		this->ptrMult = 0;
		this->ptrNum = 0;
	}

	// long immediate number
	explicit x86Argument(unsigned long long num)
	{
		this->type = argImm64;
		this->imm64Arg = num;
		this->ptrBase = rNONE;
		this->ptrIndex = rNONE;
		this->ptrMult = 0;
		this->ptrNum = 0;
	}

	bool operator==(const x86Argument& r)
	{
		if(type != r.type || reg != r.reg)
			return false;

		if(ptrSize != r.ptrSize || ptrBase != r.ptrBase || ptrIndex != r.ptrIndex || ptrMult != r.ptrMult || ptrNum != r.ptrNum)
			return false;

		return true;
	}

	ArgType	type;

	union
	{
		x86Reg reg;				// Used only when type == argReg
		int num;				// Used only when type == argNumber
		x86XmmReg xmmArg;		// Used only when type == argXmmReg
		unsigned labelID;		// Used only when type == argLabel or argPtrLabel
		x86Size	ptrSize;		// Used only when type == argPtr
		unsigned long long imm64Arg;		// Used only when type == argImm64
	};

	x86Reg	ptrBase;
	x86Reg	ptrIndex;
	int		ptrMult;
	int		ptrNum;

	int	Decode(CodeGenRegVmStateContext &ctx, char *buf, unsigned bufSize, bool x64, bool useMmWord, bool skipSize);
};

struct x86Instruction
{
	x86Instruction() : name(o_none)
	{
	}

	explicit x86Instruction(unsigned labelID) : name(o_label), labelID(labelID)
	{
	}

	explicit x86Instruction(x86Command name): name(name)
	{
	}

	x86Instruction(x86Command name, const x86Argument& a) : name(name)
	{
		argA = a;
	}

	x86Instruction(x86Command name, const x86Argument& a, const x86Argument& b) : name(name)
	{
		argA = a;
		argB = b;
	}

	x86Command name;
	unsigned instID;
	x86Argument	argA, argB;

	union
	{
		unsigned int	labelID;
		const char		*comment;
	};

	// returns string length
	int	Decode(CodeGenRegVmStateContext &ctx, char *buf, unsigned bufSize);
};
