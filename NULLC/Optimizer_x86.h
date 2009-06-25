#pragma once
#include "stdafx.h"

enum Command_Hash
{
	o_none,
	o_mov,
	o_movsx,
	o_push,
	o_pop,
	o_lea,
	o_xchg,
	o_cdq,
	o_rep_movsd,

	o_jmp,
	o_ja,
	o_jae,
	o_jb,
	o_jbe,
	o_jc,
	o_je,
	o_jz,
	o_jg,
	o_jl,
	o_jne,
	o_jnp,
	o_jnz,
	o_jp,
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
	o_shr,
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
	o_dd,
	o_label,
	o_other,
};

struct Argument
{
	// Argument type
	enum Type{ none, number, eax, ebx, ecx, edx, edi, esi, reg, ptr, label };

	char	begin, size;
	Type	type;
};

class Command
{
public:
	Command_Hash Name;
	std::string* strName;	// pointer to command in text form
	Argument	argA, argB, argC;
};

class Optimizer_x86
{
public:
	std::vector<Command>*		HashListing(const char* pListing, int strSize);
	std::vector<std::string>*	Optimize();
private:
	void HashListing(const char*);
	void OptimizePushPop();
};