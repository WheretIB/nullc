#pragma once

#include "CodeGen_X86.h"

struct RegVmCmd;
struct RegVmRegister;

struct ExternFuncInfo;
struct ExternTypeInfo;
struct ExternMemberInfo;
struct ExternLocalInfo;

struct CodeGenRegVmCallStackEntry
{
	unsigned instruction;
};

struct CodeGenRegVmContext;

struct CodeGenRegVmStateContext
{
	CodeGenRegVmStateContext()
	{
		ctx = NULL;

		dataStackBase = NULL;
		dataStackTop = NULL;
		dataStackEnd = NULL;

		callStackBase = NULL;
		callStackTop = NULL;
		callStackEnd = NULL;

		regFileArrayBase = NULL;
		regFileLastPtr = NULL;
		regFileLastTop = NULL;
		regFileArrayEnd = NULL;

		tempStackArrayBase = NULL;
		tempStackArrayEnd = NULL;

		callInstructionPos = 0;

		instAddress = NULL;

		codeLaunchHeader = NULL;

		callWrap = NULL;
		checkedReturnWrap = NULL;
		convertPtrWrap = NULL;

		x64PowWrap = NULL;
		x64PowdWrap = NULL;
		x64ModdWrap = NULL;
		x64PowlWrap = NULL;

		x86PowWrap = NULL;
		x86PowdWrap = NULL;
		x86ModdWrap = NULL;
		x86MullWrap = NULL;
		x86DivlWrap = NULL;
		x86PowlWrap = NULL;
		x86ModlWrap = NULL;
		x86LtodWrap = NULL;
		x86DtolWrap = NULL;
		x86ShllWrap = NULL;
		x86ShrlWrap = NULL;

		vsAsmStyle = false;
	}

	CodeGenRegVmContext *ctx;

	char *dataStackBase;
	char *dataStackTop;
	char *dataStackEnd;

	CodeGenRegVmCallStackEntry *callStackBase;
	CodeGenRegVmCallStackEntry *callStackTop;
	CodeGenRegVmCallStackEntry *callStackEnd;

	RegVmRegister	*regFileArrayBase;
	RegVmRegister	*regFileLastPtr;
	RegVmRegister	*regFileLastTop;
	RegVmRegister	*regFileArrayEnd;

	unsigned		*tempStackArrayBase;
	unsigned		*tempStackArrayEnd;

	unsigned callInstructionPos;

	unsigned char **instAddress;

	unsigned char *codeLaunchHeader;

	void (*callWrap)(CodeGenRegVmStateContext *vmState, unsigned functionId);
	void (*checkedReturnWrap)(CodeGenRegVmStateContext *vmState, unsigned microcodePos);
	void (*convertPtrWrap)(CodeGenRegVmStateContext *vmState, unsigned targetTypeId, unsigned sourceTypeId);

	void (*x64PowWrap)(CodeGenRegVmStateContext *vmState, uintptr_t cmdValue);
	void (*x64PowdWrap)(CodeGenRegVmStateContext *vmState, uintptr_t cmdValue);
	void (*x64ModdWrap)(CodeGenRegVmStateContext *vmState, uintptr_t cmdValue);
	void (*x64PowlWrap)(CodeGenRegVmStateContext *vmState, uintptr_t cmdValue);

	void (*x86PowWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86PowdWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86ModdWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86MullWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86DivlWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86PowlWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86ModlWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86LtodWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86DtolWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86ShllWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);
	void (*x86ShrlWrap)(CodeGenRegVmStateContext *vmState, unsigned cmdValueA, unsigned cmdValueB);

	bool vsAsmStyle;
};

class ExecutorX86;

struct CodeGenRegVmContext
{
	CodeGenRegVmContext()
	{
		x86rvm = NULL;

		labelCount = 0;

		exFunctions = NULL;
		exTypes = NULL;
		exTypeExtra = NULL;
		exLocals = NULL;
		exRegVmConstants = NULL;
		exRegVmConstantsEnd = NULL;
		exSymbols = NULL;

		vmState = NULL;

		currInstructionPos = 0;

		x86FuncAddr = NULL;
		x86Continue = NULL;

		nullcJmpTarget = 0;
		siglongjmpPtr = NULL;
	}

	CodeGenGenericContext ctx;

	ExecutorX86 *x86rvm;

	unsigned labelCount;

	ExternFuncInfo *exFunctions;
	ExternTypeInfo *exTypes;
	ExternMemberInfo *exTypeExtra;
	ExternLocalInfo *exLocals;
	unsigned *exRegVmConstants;
	unsigned *exRegVmConstantsEnd;
	char *exSymbols;

	CodeGenRegVmStateContext *vmState;

	unsigned currInstructionPos;

	unsigned *x86FuncAddr; // unused?
	int *x86Continue; // unused?

	uintptr_t nullcJmpTarget; // unused?
	void (*siglongjmpPtr)(); // unused?
};

void GenCodeCmdNop(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLoadByte(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLoadWord(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLoadDword(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLoadLong(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLoadFloat(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLoadDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLoadImm(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLoadImmLong(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLoadImmDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdStoreByte(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdStoreWord(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdStoreDword(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdStoreLong(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdStoreFloat(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdStoreDouble(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdCombinedd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdBreakupdd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdMov(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdMovMult(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdDtoi(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdDtol(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdDtof(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdItod(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLtod(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdItol(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLtoi(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdIndex(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdGetAddr(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdSetRange(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdMemCopy(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdJmp(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdJmpz(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdJmpnz(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdCall(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdCallPtr(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdReturn(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdAddImm(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdAdd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdSub(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdMul(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdDiv(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdPow(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdMod(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLess(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdGreater(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLequal(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdGequal(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdEqual(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdNequal(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdShl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdShr(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdBitAnd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdBitOr(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdBitXor(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdAddImml(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdAddl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdSubl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdMull(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdDivl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdPowl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdModl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLessl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdGreaterl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLequall(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdGequall(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdEquall(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdNequall(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdShll(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdShrl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdBitAndl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdBitOrl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdBitXorl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdAddd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdSubd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdMuld(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdDivd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdAddf(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdSubf(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdMulf(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdDivf(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdPowd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdModd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLessd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdGreaterd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLequald(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdGequald(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdEquald(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdNequald(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdNeg(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdNegl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdNegd(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdBitNot(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdBitNotl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLogNot(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdLogNotl(CodeGenRegVmContext &ctx, RegVmCmd cmd);
void GenCodeCmdConvertPtr(CodeGenRegVmContext &ctx, RegVmCmd cmd);
