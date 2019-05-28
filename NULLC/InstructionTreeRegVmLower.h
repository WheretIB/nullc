#pragma once

#include "Array.h"

struct ExpressionContext;

struct SynBase;

struct VmValue;
struct VmConstant;
struct VmInstruction;
struct VmBlock;
struct VmFunction;
struct VmModule;

struct RegVmLoweredBlock;
struct RegVmLoweredFunction;
struct RegVmLoweredModule;

enum RegVmInstructionCode
{
	rviNop,

	rviLoadByte,
	rviLoadWord,
	rviLoadDword,
	rviLoadQword,
	rviLoadFloat,

	rviLoadImm,
	rviLoadImmHigh,

	rviStoreByte,
	rviStoreWord,
	rviStoreDword,
	rviStoreQword,
	rviStoreFloat,

	rviCombinedd,
	rviMov,

	rviDtoi,
	rviDtol,
	rviDtof,
	rviItod,
	rviLtod,
	rviItol,
	rviLtoi,

	rviIndex,

	rviGetAddr,

	rviSetRange,

	rviJmp,
	rviJmpz,
	rviJmpnz,

	rviPush,
	rviPushq,
	rviPushImm,
	rviPushImmq,

	rviPop,
	rviPopq,

	rviCall,
	rviCallPtr,

	rviReturn,

	rviPushvtop,

	rviAdd,
	rviSub,
	rviMul,
	rviDiv,
	rviPow,
	rviMod,

	rviLess,
	rviGreater,
	rviLequal,
	rviGequal,
	rviEqual,
	rviNequal,

	rviShl,
	rviShr,
	
	rviBitAnd,
	rviBitOr,
	rviBitXor,

	rviLogXor,

	rviAddl,
	rviSubl,
	rviMull,
	rviDivl,
	rviPowl,
	rviModl,

	rviLessl,
	rviGreaterl,
	rviLequall,
	rviGequall,
	rviEquall,
	rviNequall,

	rviShll,
	rviShrl,

	rviBitAndl,
	rviBitOrl,
	rviBitXorl,

	rviLogXorl,

	rviAddd,
	rviSubd,
	rviMuld,
	rviDivd,
	rviPowd,
	rviModd,

	rviLessd,
	rviGreaterd,
	rviLequald,
	rviGequald,
	rviEquald,
	rviNequald,

	rviNeg,
	rviNegl,
	rviNegd,

	rviBitNot,
	rviBitNotl,

	rviLogNot,
	rviLogNotl,

	rviConvertPtr,

	rviCheckRet,

	// Temporary instructions, no execution
	rviFuncAddr,
	rviTypeid,
};

const char* GetInstructionName(RegVmInstructionCode code);

enum RegVmSetRangeType
{
	rvsrDouble,
	rvsrFloat,
	rvsrLong,
	rvsrInt,
	rvsrShort,
	rvsrChar,
};

enum RegVmReturnType
{
	rvrVoid,
	rvrDouble,
	rvrLong,
	rvrInt,
	rvrStruct,
	rvrError,
};

#define rvrrGlobals 0
#define rvrrFrame 1

#define rvrrCount 2

struct RegVmCmd
{
	RegVmCmd(): code(0), rA(0), rB(0), rC(0), argument(0)
	{
	}

	RegVmCmd(RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, unsigned argument): code((unsigned char)code), rA(rA), rB(rB), rC(rC), argument(argument)
	{
	}

	unsigned char code;
	unsigned char rA;
	unsigned char rB;
	unsigned char rC;
	unsigned argument;
};

struct RegVmLoweredInstruction
{
	RegVmLoweredInstruction(SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmConstant *argument): location(location), code(code), rA(rA), rB(rB), rC(rC), argument(argument)
	{
		parent = NULL;

		prevSibling = NULL;
		nextSibling = NULL;
	}

	SynBase *location;

	RegVmInstructionCode code;
	unsigned char rA;
	unsigned char rB;
	unsigned char rC;
	VmConstant *argument;

	RegVmLoweredBlock *parent;

	RegVmLoweredInstruction *prevSibling;
	RegVmLoweredInstruction *nextSibling;
};

struct RegVmLoweredBlock
{
	RegVmLoweredBlock(VmBlock *vmBlock): vmBlock(vmBlock)
	{
		firstInstruction = NULL;
		lastInstruction = NULL;
	}

	void AddInstruction(ExpressionContext &ctx, RegVmLoweredInstruction* instruction);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmConstant *argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, unsigned argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmBlock *argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmFunction *argument);

	VmBlock *vmBlock;

	RegVmLoweredInstruction *firstInstruction;
	RegVmLoweredInstruction *lastInstruction;
};

struct RegVmLoweredFunction
{
	RegVmLoweredFunction(Allocator *allocator, VmFunction *vmFunction): vmFunction(vmFunction), blocks(allocator)
	{
		registerUsers.fill(0);

		nextRegister = rvrrCount;
	}

	unsigned char GetRegister();
	void FreeRegister(unsigned char reg);

	void CompleteUse(VmValue *value);
	unsigned char GetRegister(VmValue *value);
	void GetRegisters(SmallArray<unsigned char, 8> &result, VmValue *value);
	unsigned char AllocateRegister(VmValue *value, bool additional = false);

	unsigned char GetRegisterForConstant();

	void FreeConstantRegisters();
	void FreeDelayedRegisters();

	bool TransferRegisterTo(VmValue *value, unsigned char reg);

	VmFunction *vmFunction;

	SmallArray<RegVmLoweredBlock*, 16> blocks;

	FixedArray<unsigned short, 256> registerUsers;

	unsigned char nextRegister;
	SmallArray<unsigned char, 16> delayedFreedRegisters;
	SmallArray<unsigned char, 16> freedRegisters;

	SmallArray<unsigned char, 16> constantRegisters;

	// TODO: register spills
};

struct RegVmLoweredModule
{
	RegVmLoweredModule(Allocator *allocator, VmModule *vmModule): allocator(allocator), vmModule(vmModule), functions(allocator)
	{
	}

	Allocator *allocator;

	VmModule *vmModule;

	SmallArray<RegVmLoweredFunction*, 16> functions;
};

RegVmLoweredModule* RegVmLowerModule(ExpressionContext &ctx, VmModule *module);

struct InstructionRegVmFinalizeContext
{
	InstructionRegVmFinalizeContext(ExpressionContext &ctx, Allocator *allocator): ctx(ctx), fixupPoints(allocator)
	{
		currentFunction = 0;
		currentBlock = 0;
	}

	ExpressionContext &ctx;

	FastVector<SynBase*> locations;
	FastVector<RegVmCmd> cmds;

	struct FixupPoint
	{
		FixupPoint(): cmdIndex(0), target(0)
		{
		}

		FixupPoint(unsigned cmdIndex, VmBlock *target): cmdIndex(cmdIndex), target(target)
		{
		}

		unsigned cmdIndex;
		VmBlock *target;
	};

	SmallArray<FixupPoint, 32> fixupPoints;

	VmFunction *currentFunction;
	VmBlock *currentBlock;

private:
	InstructionRegVmFinalizeContext(const InstructionRegVmFinalizeContext&);
	InstructionRegVmFinalizeContext& operator=(const InstructionRegVmFinalizeContext&);
};

void RegVmFinalizeModule(InstructionRegVmFinalizeContext &ctx, RegVmLoweredModule *lowModule);
