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
};

static const char* x86RegText[] = { "none", "eax", "ebx", "ecx", "edx", "esp", "edi", "ebp", "esi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
static const char* x64RegText[] = { "none", "rax", "rbx", "rcx", "rdx", "rsp", "rdi", "rbp", "rsi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };

enum x87Reg
{
	rST0,
	rST1,
	rST2,
	rST3,
	rST4,
	rST5,
	rST6,
	rST7
};

static const char* x87RegText[] = {
	"st0",
	"st1",
	"st2",
	"st3",
	"st4",
	"st5",
	"st6",
	"st7"
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
};

static const char* x86XmmRegText[] = {
	"xmm0",
	"xmm1",
	"xmm2",
	"xmm3",
	"xmm4",
	"xmm5",
	"xmm6",
	"xmm7",
	"xmm8",
	"xmm9",
	"xmm10",
	"xmm11",
	"xmm12",
	"xmm13",
	"xmm14",
	"xmm15"
};

enum x86Size{ sNONE, sBYTE, sWORD, sDWORD, sQWORD };
static const char* x86SizeText[] = { "none", "byte", "word", "dword", "qword" };

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
	o_rep_movsd,
	o_rep_stosd,

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

	o_movss,
	o_movsd,
	o_cvtss2sd,
	o_cvtsd2ss,
	o_cvttsd2si,
	o_cvtsi2sd,

	o_int,
	o_label,
	o_use32,
	o_nop,
	o_other,

	o_mov64,
	o_movsxd,

	o_add64,
	o_sub64,

	o_cvttsd2si64,

	// Aliases
	o_jc = o_jb,
	o_jz = o_je,
	o_jnz = o_jne
};

static const char* x86CmdText[] = 
{	"", "mov", "movsx", "push", "pop", "lea", "cdq", "rep movsd", "rep stosd",
	"jmp", "ja", "jae", "jb", "jbe", "je", "jg", "jl", "jne", "jnp", "jp", "jge", "jle", "call", "ret",
	"fld", "fild", "fistp", "fst", "fstp", "fnstsw", "fstcw", "fldcw",
	"neg", "add", "adc", "sub", "sbb", "imul", "idiv", "shl", "sal", "sar", "not", "and", "or", "xor", "cmp", "test",
	"setl", "setg", "setle", "setge", "sete", "setne", "setz", "setnz",
	"fadd", "faddp", "fmul", "fmulp", "fsub", "fsubr", "fsubp", "fsubrp", "fdiv", "fdivr", "fdivrp", "fchs", "fprem", "fcomp", "fldz", "fld1", "fsincos", "fptan", "fsqrt", "frndint",
	"movss", "movsd", "cvtss2sd", "cvtsd2ss", "cvttsd2si", "cvtsi2sd",
	"int", "label", "use32", "nop", "other",

	"mov", "movsxd",
	"add", "sub",
	"cvttsd2si64"
};

struct x86Argument
{
	// Argument type
	enum ArgType
	{
		argNone,
		
		argNumber,
		argReg,
		argFpReg,
		argXmmReg,
		argPtr,
		argPtrLabel,
		argLabel,
		argImm64
	};

	// no argument
	x86Argument()
	{
	}

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
		type = argFpReg;
		fpArg = fpReg;
	}
	// sse register
	explicit x86Argument(x86XmmReg xmmReg)
	{
		Empty();
		type = argXmmReg;
		xmmArg = xmmReg;
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
	// long immediate number
	explicit x86Argument(long long Num)
	{
		Empty();
		type = argImm64;
		imm64Arg = Num;
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
		x86Reg reg;				// Used only when type == argReg
		int num;				// Used only when type == argNumber
		x87Reg fpArg;			// Used only when type == argFpReg
		x86XmmReg xmmArg;		// Used only when type == argXmmReg
		unsigned labelID;		// Used only when type == argLabel or argPtrLabel
		x86Size	ptrSize;		// Used only when type == argPtr
		uintptr_t imm64Arg;		// Used only when type == argImm64
	};

	x86Reg	ptrBase, ptrIndex;
	int		ptrMult;
	int		ptrNum;

	int	Decode(char *buf, bool x64)
	{
		char *curr = buf;

		if(type == argNumber)
		{
			curr += sprintf(curr, "%d", num);
		}
		else if(type == argReg)
		{
			strcpy(curr, (x64 ? x64RegText : x86RegText)[reg]);
			curr += strlen(curr);
		}
		else if(type == argFpReg)
		{
			strcpy(curr, x87RegText[fpArg]);
			curr += strlen(curr);
		}
		else if(type == argXmmReg)
		{
			strcpy(curr, x86XmmRegText[fpArg]);
			curr += strlen(curr);
		}
		else if(type == argLabel)
		{
			curr += sprintf(curr, "'0x%x'", labelID);
		}
		else if(type == argPtrLabel)
		{
			curr += sprintf(curr, "['0x%x'+%d]", labelID, ptrNum);
		}
		else if(type == argPtr)
		{
			strcpy(curr, x86SizeText[ptrSize]);
			curr += strlen(curr);

			*curr++ = ' ';
			*curr++ = '[';
			*curr = 0;

			if(ptrIndex != rNONE)
			{
				strcpy(curr, (x64 ? x64RegText : x86RegText)[ptrIndex]);
				curr += strlen(curr);
			}

			if(ptrMult > 1)
				curr += sprintf(curr, "*%d", ptrMult);

			if(ptrBase != rNONE)
			{
				if(ptrIndex != rNONE)
					curr += sprintf(curr, " + %s", (x64 ? x64RegText : x86RegText)[ptrBase]);
				else
					curr += sprintf(curr, "%s", (x64 ? x64RegText : x86RegText)[ptrBase]);
			}

			if(ptrIndex == rNONE && ptrBase == rNONE)
				curr += sprintf(curr, "%d", ptrNum);
			else if(ptrNum != 0)
				curr += sprintf(curr, "%+d", ptrNum);

			*curr++ = ']';
			*curr = 0;
		}
		else if(type == argImm64)
		{
			curr += sprintf(curr, "%lld", imm64Arg);
		}

		return (int)(curr - buf);
	}
};

const int INST_COMMENT = 1;

struct x86Instruction
{
	x86Instruction() : name(o_none)
	{
	}

	explicit x86Instruction(unsigned labelID) : labelID(labelID), name(o_label)
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
	int	Decode(char *buf)
	{
		char *curr = buf;

		if(name == o_label)
		{
			curr += sprintf(curr, "0x%p:", (void*)(intptr_t)labelID);
		}
		else if(name == o_other)
		{
			strcpy(curr, "  ; ");
			curr += strlen(curr);

			strcpy(curr, comment);
			curr += strlen(curr);
		}
		else
		{
			strcpy(curr, x86CmdText[name]);
			curr += strlen(curr);
		}

		if(name != o_none)
		{
			if(argA.type != x86Argument::argNone)
			{
				curr += sprintf(curr, " ");
				curr += argA.Decode(curr, name >= o_mov64 || argA.type == x86Argument::argPtr);
			}
			if(argB.type != x86Argument::argNone)
			{
				curr += sprintf(curr, ", ");
				curr += argB.Decode(curr, name >= o_mov64 || argB.type == x86Argument::argPtr);
			}
		}

		return (int)(curr-buf);
	}
};
