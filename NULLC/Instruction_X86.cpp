#include "Instruction_X86.h"

#include "CodeGenRegVm_X86.h"

namespace
{
	const char* x86RegText[] = { "none", "eax", "ebx", "ecx", "edx", "esp", "edi", "ebp", "esi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
	const char* x64RegText[] = { "none", "rax", "rbx", "rcx", "rdx", "rsp", "rdi", "rbp", "rsi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };

	const char* x86XmmRegText[] = {
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

	const char* x86SizeText[] = { "none", "byte", "word", "dword", "qword" };
	const char* x86XmmSizeText[] = { "none", "byte", "word", "dword", "mmword" };

	const char* x86CmdText[] =
	{
		"",
		"mov", "movsx", "push", "pop", "lea", "cdq", "cqo", "rep movsd", "rep stosb", "rep stosw", "rep stosd", "rep stosq",
		"jmp", "ja", "jae", "jb", "jbe", "je", "jg", "jl", "jne", "jnp", "jp", "jge", "jle", "call", "ret",
		"neg", "add", "adc", "sub", "sbb", "imul", "idiv", "shl", "sal", "sar", "not", "and", "or", "xor", "cmp", "test",
		"setl", "setg", "setle", "setge", "sete", "setne", "setz", "setnz",
		"movss", "movsd", "movd", "movsxd", "cvtss2sd", "cvtsd2ss", "cvttsd2si", "cvtsi2sd", "addsd", "subsd", "mulsd", "divsd", "sqrtsd", "cmpeqsd", "cmpltsd", "cmplesd", "cmpneqsd",
		"int", "label", "use32", "nop", "other",
		"; read_register", "; kill_register", "; set_tracking",

		"mov",
		"neg", "add", "sub", "imul", "idiv", "sal", "sar", "not", "and", "or", "xor", "cmp",
		"cvttsd2si", "cvtsi2sd"
	};
}

int x86Argument::Decode(CodeGenRegVmStateContext &ctx, char *buf, bool x64, bool useMmWord, bool skipSize)
{
	char *curr = buf;

	if(type == argNumber)
	{
		if(ctx.vsAsmStyle)
		{
			curr += sprintf(curr, "%x%s", num, num > 9 ? "h" : "");
		}
		else
		{
			if(uintptr_t(num) == uintptr_t(&ctx.callInstructionPos))
				curr += sprintf(curr, "&ctx.callInstructionPos");
			else if(uintptr_t(num) == uintptr_t(&ctx.dataStackTop))
				curr += sprintf(curr, "&ctx.dataStackTop");
			else if(uintptr_t(num) == uintptr_t(&ctx.callStackTop))
				curr += sprintf(curr, "&ctx.callStackTop");
			else if(uintptr_t(num) == uintptr_t(&ctx.regFileLastTop))
				curr += sprintf(curr, "&ctx.regFileLastTop");
			else if(uintptr_t(num) == uintptr_t(&ctx.regFileLastPtr))
				curr += sprintf(curr, "&ctx.regFileLastPtr");
			else if(uintptr_t(num) == uintptr_t(ctx.dataStackBase))
				curr += sprintf(curr, "ctx.dataStackBase");
			else if(uintptr_t(num) == uintptr_t(ctx.ctx->exRegVmConstants))
				curr += sprintf(curr, "ctx.exRegVmConstants");
			else if(uintptr_t(num) == uintptr_t(&ctx))
				curr += sprintf(curr, "&ctx");
			else
				curr += sprintf(curr, "%d", num);
		}
	}
	else if(type == argReg)
	{
		strcpy(curr, (x64 ? x64RegText : x86RegText)[reg]);
		curr += strlen(curr);
	}
	else if(type == argXmmReg)
	{
		strcpy(curr, x86XmmRegText[xmmArg]);
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
		if(!skipSize)
		{
			strcpy(curr, (useMmWord ? x86XmmSizeText : x86SizeText)[ptrSize]);
			curr += strlen(curr);

			if(ctx.vsAsmStyle)
			{
				*curr++ = ' ';
				*curr++ = 'p';
				*curr++ = 't';
				*curr++ = 'r';
			}

			*curr++ = ' ';
		}

		*curr++ = '[';
		*curr = 0;

		if(!ctx.vsAsmStyle)
		{
			if(ptrIndex != rNONE)
			{
				strcpy(curr, (x64 ? x64RegText : x86RegText)[ptrIndex]);
				curr += strlen(curr);
			}

			if(ptrMult > 1)
				curr += sprintf(curr, "*%d", ptrMult);
		}

		if(ptrBase != rNONE)
		{
			if(ctx.vsAsmStyle)
				curr += sprintf(curr, "%s", (x64 ? x64RegText : x86RegText)[ptrBase]);
			else if(ptrIndex != rNONE)
				curr += sprintf(curr, " + %s", (x64 ? x64RegText : x86RegText)[ptrBase]);
			else
				curr += sprintf(curr, "%s", (x64 ? x64RegText : x86RegText)[ptrBase]);
		}

		if(ctx.vsAsmStyle)
		{
			if(ptrIndex != rNONE)
			{
				if(ptrBase != rNONE)
					*curr++ = '+';

				strcpy(curr, (x64 ? x64RegText : x86RegText)[ptrIndex]);
				curr += strlen(curr);
			}

			if(ptrMult > 1)
				curr += sprintf(curr, "*%d", ptrMult);
		}

		if(ptrIndex == rNONE && ptrBase == rNONE && ctx.vsAsmStyle)
			curr += sprintf(curr, "%x%s", ptrNum, ptrNum > 9 ? "h" : "");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) >= uintptr_t(ctx.tempStackArrayBase) && uintptr_t(ptrNum) < uintptr_t(ctx.tempStackArrayEnd))
			curr += sprintf(curr, "temp+%u", unsigned(ptrNum - uintptr_t(ctx.tempStackArrayBase)));
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) >= uintptr_t(ctx.dataStackBase) && uintptr_t(ptrNum) < uintptr_t(ctx.dataStackEnd))
			curr += sprintf(curr, "globals+%u", unsigned(ptrNum - uintptr_t(ctx.dataStackBase)));
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) >= uintptr_t(ctx.ctx->exRegVmConstants) && uintptr_t(ptrNum) < uintptr_t(ctx.ctx->exRegVmConstantsEnd))
			curr += sprintf(curr, "constants+%u", unsigned(ptrNum - uintptr_t(ctx.ctx->exRegVmConstants)));
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.callInstructionPos))
			curr += sprintf(curr, "&ctx.callInstructionPos");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.dataStackTop))
			curr += sprintf(curr, "&ctx.dataStackTop");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.callStackTop))
			curr += sprintf(curr, "&ctx.callStackTop");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.regFileLastTop))
			curr += sprintf(curr, "&ctx.regFileLastTop");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.regFileLastPtr))
			curr += sprintf(curr, "&ctx.regFileLastPtr");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.callWrap))
			curr += sprintf(curr, "&ctx.callWrap");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.checkedReturnWrap))
			curr += sprintf(curr, "&ctx.checkedReturnWrap");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.convertPtrWrap))
			curr += sprintf(curr, "&ctx.convertPtrWrap");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.errorOutOfBoundsWrap))
			curr += sprintf(curr, "&ctx.errorOutOfBoundsWrap");
		else if(ptrIndex == rNONE && ptrBase == rNONE && uintptr_t(ptrNum) == uintptr_t(&ctx.errorNoReturnWrap))
			curr += sprintf(curr, "&ctx.errorNoReturnWrap");
		else if(ptrIndex == rNONE && ptrBase == rNONE)
			curr += sprintf(curr, "%d", ptrNum);
		else if(ptrNum != 0 && ctx.vsAsmStyle)
			curr += sprintf(curr, "+%x%s", ptrNum, ptrNum > 9 ? "h" : "");
		else if(ptrNum != 0)
			curr += sprintf(curr, "%+d", ptrNum);

		*curr++ = ']';
		*curr = 0;
	}
	else if(type == argImm64)
	{
		if(imm64Arg == uintptr_t(&ctx))
			curr += sprintf(curr, "&ctx");
		else if(imm64Arg == uintptr_t(&ctx.tempStackArrayBase))
			curr += sprintf(curr, "&ctx.tempStackArrayBase");
		else if(imm64Arg == uintptr_t(ctx.tempStackArrayBase))
			curr += sprintf(curr, "ctx.tempStackArrayBase");
		else if(ctx.vsAsmStyle)
			curr += sprintf(curr, "%llXh", (unsigned long long)imm64Arg);
		else
			curr += sprintf(curr, "%lld", (long long)imm64Arg);
	}

	return (int)(curr - buf);
}

int	x86Instruction::Decode(CodeGenRegVmStateContext &ctx, char *buf)
{
	char *curr = buf;

	if(ctx.vsAsmStyle)
		*curr++ = ' ';

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

		if(ctx.vsAsmStyle)
		{
			for(unsigned width = (unsigned)strlen(x86CmdText[name]); width < 11; width++)
				*curr++ = ' ';
		}
	}

	if(name != o_none)
	{
		if(argA.type != x86Argument::argNone)
		{
			*curr++ = ' ';

			bool usex64 = name >= o_mov64 || name == o_movsxd || ((argA.type == x86Argument::argPtr || name == o_call || name == o_push || name == o_pop) && sizeof(void*) == 8) || (argB.type == x86Argument::argPtr && argB.ptrSize == sQWORD && name != o_cvttsd2si);
			bool useMmWord = ctx.vsAsmStyle && argB.type == x86Argument::argXmmReg;

			curr += argA.Decode(ctx, curr, usex64, useMmWord, name == o_lea);
		}
		if(argB.type != x86Argument::argNone)
		{
			*curr++ = ',';

			if(!ctx.vsAsmStyle)
				*curr++ = ' ';

			bool usex64 = name >= o_mov64 || (argB.type == x86Argument::argPtr && sizeof(void*) == 8) || name == o_movsxd || (argA.type == x86Argument::argPtr && argA.ptrSize == sQWORD);
			bool useMmWord = ctx.vsAsmStyle && (argA.type == x86Argument::argXmmReg || name == o_cvttsd2si || name == o_cvttsd2si64);

			curr += argB.Decode(ctx, curr, usex64, useMmWord, name == o_lea);
		}
	}

	if(ctx.vsAsmStyle)
	{
		*curr++ = ' ';
		*curr++ = ' ';
		*curr = 0;
	}

	return (int)(curr-buf);
}
