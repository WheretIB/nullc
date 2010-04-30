#pragma once

#include "stdafx.h"

enum x86Reg{ rNONE, rEAX, rEBX, rECX, rEDX, rESP, rEDI, rEBP, rESI, };
static const char* x86RegText[] = { "none", "eax", "ebx", "ecx", "edx", "esp", "edi", "ebp", "esi" };

enum x87Reg{ rST0, rST1, rST2, rST3, rST4, rST5, rST6, rST7, };
static const char* x87RegText[] = { "st0", "st1", "st2", "st3", "st4", "st5", "st6", "st7" };

enum x86Size{ sNONE, sBYTE, sWORD, sDWORD, sQWORD, };
static const char* x86SizeText[] = { "none", "byte", "word", "dword", "qword" };

enum x86Cond{ condO, condNO, condB, condC, condNAE, condAE, condNB, condNC, condE, condZ, condNE, condNZ,
				condBE, condNA, condA, condNBE, condS, condNS, condP, condPE, condNP, condPO,
				condL, condNGE, condGE, condNL, condLE, condNG, condG, condNLE };

const int rAX = rEAX;
const int rAL = rEAX;
const int rBX = rEBX;
const int rBL = rEBX;

enum x86Command
{
	o_none,
	o_mov,
	o_movsx,
	o_push,
	o_pop,
	o_lea,
	o_cdq,
	o_rep_movsd,

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

	o_fld,
	o_fild,
	o_fistp,
	o_fst,
	o_fstp,
	o_fnstsw,
	o_fstcw,
	o_fldcw,

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

	o_fadd,
	o_faddp,
	o_fmul,
	o_fmulp,
	o_fsub,
	o_fsubr,
	o_fsubp,
	o_fsubrp,
	o_fdiv,
	o_fdivr,
	o_fdivrp,
	o_fchs,
	o_fprem,
	o_fcomp,
	o_fldz,
	o_fld1,
	o_fsincos,
	o_fptan,
	o_fsqrt,
	o_frndint,

	o_int,
	o_label,
	o_use32,
	o_nop,
	o_other,
};
#define o_jc o_jb
#define o_jz o_je
#define o_jnz o_jne

static const char* x86CmdText[] = 
{	"", "mov", "movsx", "push", "pop", "lea", "cdq", "rep movsd",
	"jmp", "ja", "jae", "jb", "jbe", "je", "jg", "jl", "jne", "jnp", "jp", "jge", "jle", "call", "ret",
	"fld", "fild", "fistp", "fst", "fstp", "fnstsw", "fstcw", "fldcw",
	"neg", "add", "adc", "sub", "sbb", "imul", "idiv", "shl", "sal", "sar", "not", "and", "or", "xor", "cmp", "test",
	"setl", "setg", "setle", "setge", "sete", "setne", "setz", "setnz",
	"fadd", "faddp", "fmul", "fmulp", "fsub", "fsubr", "fsubp", "fsubrp", "fdiv", "fdivr", "fdivrp", "fchs", "fprem", "fcomp", "fldz", "fld1", "fsincos", "fptan", "fsqrt", "frndint",
	"int", "dd", "label", "use32", "nop", "other"
};

struct x86Argument
{
	// Argument type
	enum ArgType{ argNone, argNumber, argReg, argFPReg, argPtr, argPtrLabel, argLabel };

	// no argument
	x86Argument(){ }

	// immediate number
	explicit x86Argument(int Num)
	{
		Empty();
		type = argNumber;
		num = Num;
	}
	// register
	explicit x86Argument(x86Reg Register)
	{
		Empty();
		type = argReg;
		reg = Register;
	}
	// fp register
	explicit x86Argument(x87Reg fpReg)
	{
		Empty();
		type = argFPReg;
		fpArg = fpReg;
	}
	// size [num]
	x86Argument(x86Size Size, unsigned int Num)
	{
		Empty();
		type = argPtr;
		ptrSize = Size; ptrMult = 1; ptrNum = Num;
	}
	// size [register + num]
	x86Argument(x86Size Size, x86Reg RegA, unsigned int Num)
	{
		Empty();
		type = argPtr;
		ptrSize = Size; ptrBase = RegA; ptrMult = 1; ptrNum = Num;
	}
	// size [register + register + num]
	x86Argument(x86Size Size, x86Reg RegA, x86Reg RegB, unsigned int Num)
	{
		Empty();
		type = argPtr;
		ptrSize = Size; ptrBase = RegB; ptrMult = 1; ptrIndex = RegA; ptrNum = Num;
	}
	// size [register * multiplier + register + num]
	x86Argument(x86Size Size, x86Reg RegA, unsigned int Mult, x86Reg RegB, unsigned int Num)
	{
		Empty();
		type = argPtr;
		ptrSize = Size; ptrBase = RegB; ptrMult = Mult; ptrIndex = RegA; ptrNum = Num;
	}

	void Empty()
	{
		memset(this, 0, sizeof(x86Argument));
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
		x86Reg	reg;				// Used only when type == argReg
		int		num;				// Used only when type == argNumber
		x87Reg	fpArg;				// Used only when type == argFPReg
		unsigned int	labelID;	// Used only when type == argLabel or argPtrLabel
		x86Size	ptrSize;			// Used only when type == argPtr
	};

	x86Reg	ptrBase, ptrIndex;
	int		ptrMult;
	int		ptrNum;

	int	Decode(char *buf)
	{
		char *curr = buf;
		if(type == argNumber)
			curr += sprintf(curr, "%d", num);
		else if(type == argReg)
			curr += sprintf(curr, "%s", x86RegText[reg]);
		else if(type == argFPReg)
			curr += sprintf(curr, "%s", x87RegText[fpArg]);
		else if(type == argLabel)
			curr += sprintf(curr, "'0x%p'", (int*)(intptr_t)labelID);
		else if(type == argPtrLabel)
			curr += sprintf(curr, "['0x%p'+%d]", (int*)(intptr_t)labelID, ptrNum);
		else if(type == argPtr){
			curr += sprintf(curr, "%s [", x86SizeText[ptrSize]);
			if(ptrIndex != rNONE)
				curr += sprintf(curr, "%s", x86RegText[ptrIndex]);
			if(ptrMult > 1)
				curr += sprintf(curr, "*%d", ptrMult);
			if(ptrBase != rNONE)
			{
				if(ptrIndex != rNONE)
					curr += sprintf(curr, " + %s", x86RegText[ptrBase]);
				else
					curr += sprintf(curr, "%s", x86RegText[ptrBase]);
			}
			if(ptrIndex == rNONE && ptrBase == rNONE)
				curr += sprintf(curr, "%d", ptrNum);
			else if(ptrNum != 0)
				curr += sprintf(curr, "%+d", ptrNum);
			curr += sprintf(curr, "]");
		}

		return (int)(curr-buf);
	}
};

const int INST_COMMENT = 1;

struct x86Instruction
{
	x86Instruction(){ name = o_none; }
	explicit x86Instruction(unsigned int LabelID){ name = o_label; labelID = LabelID; }
	explicit x86Instruction(x86Command Name){ name = Name; }
	x86Instruction(x86Command Name, const x86Argument& a){ name = Name; argA = a; }
	x86Instruction(x86Command Name, const x86Argument& a, const x86Argument& b){ name = Name; argA = a; argB = b; }

	x86Command	name;
	unsigned int	instID;
	x86Argument	argA, argB;

#ifdef NULLC_LOG_FILES
	union
	{
		unsigned int	labelID;
		const char		*comment;
	};
#else
	unsigned int	labelID;
#endif

	// returns string length
	int	Decode(char *buf)
	{
		char *curr = buf;
		if(name == o_label)
			curr += sprintf(curr, "0x%p:", (int*)(intptr_t)labelID);
#ifdef NULLC_LOG_FILES
		else if(name == o_other)
			curr += sprintf(curr, "  ; %s", comment);
#endif
		else
			curr += sprintf(curr, "%s", x86CmdText[name]);
		if(name != o_none)
		{
			if(argA.type != x86Argument::argNone)
			{
				curr += sprintf(curr, " ");
				curr += argA.Decode(curr);
			}
			if(argB.type != x86Argument::argNone)
			{
				curr += sprintf(curr, ", ");
				curr += argB.Decode(curr);
			}
		}

		return (int)(curr-buf);
	}
};
