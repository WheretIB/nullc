#include "Instruction_X86.h"

#include "CodeGenRegVm_X86.h"

int x86Argument::Decode(CodeGenRegVmStateContext &ctx, char *buf, bool x64)
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
		if(imm64Arg == uintptr_t(&ctx))
			curr += sprintf(curr, "&vmState");
		else if(imm64Arg == uintptr_t(&ctx.tempStackArrayBase))
			curr += sprintf(curr, "&vmState.tempStackArrayBase");
		else
			curr += sprintf(curr, "%lld", imm64Arg);
	}

	return (int)(curr - buf);
}

int	x86Instruction::Decode(CodeGenRegVmStateContext &ctx, char *buf)
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
			curr += argA.Decode(ctx, curr, name >= o_mov64 || argA.type == x86Argument::argPtr);
		}
		if(argB.type != x86Argument::argNone)
		{
			curr += sprintf(curr, ", ");
			curr += argB.Decode(ctx, curr, name >= o_mov64 || argB.type == x86Argument::argPtr);
		}
	}

	return (int)(curr-buf);
}
