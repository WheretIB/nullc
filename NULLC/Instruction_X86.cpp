#include "Instruction_X86.h"

#include "CodeGenRegVm_X86.h"

int x86Argument::Decode(CodeGenRegVmStateContext &ctx, char *buf, bool x64, bool useMmWord, bool skipSize)
{
	char *curr = buf;

	if(type == argNumber)
	{
		if(ctx.vsAsmStyle)
			curr += sprintf(curr, "%x%s", num, num > 9 ? "h" : "");
		else
			curr += sprintf(curr, "%d", num);
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
		else if(ptrIndex == rNONE && ptrBase == rNONE && ptrNum >= uintptr_t(ctx.tempStackArrayBase) && ptrNum < uintptr_t(ctx.tempStackArrayEnd))
			curr += sprintf(curr, "vmState.tempStackArrayBase+%d", unsigned(ptrNum - uintptr_t(ctx.tempStackArrayBase)));
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
			curr += sprintf(curr, "&vmState");
		else if(imm64Arg == uintptr_t(&ctx.tempStackArrayBase))
			curr += sprintf(curr, "&vmState.tempStackArrayBase");
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
