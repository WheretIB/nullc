#pragma once

#include "Array.h"
#include "InstructionTreeRegVm.h"

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

struct RegVmLoweredInstruction
{
	RegVmLoweredInstruction(Allocator *allocator, SynBase *location, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, VmConstant *argument): location(location), code(code), rA(rA), rB(rB), rC(rC), argument(argument), preKillRegisters(allocator), postKillRegisters(allocator)
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

	SmallArray<unsigned char, 8> preKillRegisters;
	SmallArray<unsigned char, 8> postKillRegisters;
};

struct RegVmLoweredBlock
{
	RegVmLoweredBlock(Allocator *allocator, RegVmLoweredFunction *parent, VmBlock *vmBlock): parent(parent), vmBlock(vmBlock), entryRegisters(allocator), reservedRegisters(allocator), exitRegisters(allocator)
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

	RegVmLoweredFunction *parent;

	VmBlock *vmBlock;

	RegVmLoweredInstruction *firstInstruction;
	RegVmLoweredInstruction *lastInstruction;

	SmallArray<unsigned char, 16> entryRegisters;
	SmallArray<unsigned char, 16> reservedRegisters;
	SmallArray<unsigned char, 16> exitRegisters;
};

struct RegVmLoweredFunction
{
	RegVmLoweredFunction(Allocator *allocator, RegVmLoweredModule *parent, VmFunction *vmFunction): parent(parent), vmFunction(vmFunction), blocks(allocator), delayedFreedRegisters(allocator), freedRegisters(allocator), constantRegisters(allocator), killedRegisters(allocator), colorRegisters(allocator)
	{
		registerUsers.fill(0);

		nextRegister = rvrrCount;

		hasRegisterOverflow = false;
		registerOverflowLocation = NULL;
	}

	unsigned char GetRegister();
	void FreeRegister(unsigned char reg);

	void CompleteUse(VmValue *value);
	unsigned char GetRegister(VmValue *value);
	void GetRegisters(SmallArray<unsigned char, 32> &result, VmValue *value);
	unsigned char AllocateRegister(VmValue *value, unsigned index = 0u, bool freeDelayed = true);

	unsigned char GetRegisterForConstant();

	void FreeConstantRegisters();
	void FreeDelayedRegisters(RegVmLoweredBlock *lowBlock);

	bool TransferRegisterTo(VmValue *value, unsigned char reg);

	RegVmLoweredModule *parent;

	VmFunction *vmFunction;

	SmallArray<RegVmLoweredBlock*, 16> blocks;

	FixedArray<unsigned short, 256> registerUsers;

	unsigned char nextRegister;
	SmallArray<unsigned char, 16> delayedFreedRegisters;
	SmallArray<unsigned char, 16> freedRegisters;

	SmallArray<unsigned char, 16> constantRegisters;

	SmallArray<unsigned char, 16> killedRegisters;

	SmallArray<VmInstruction*, 16> colorRegisters;

	// TODO: register spills
	bool hasRegisterOverflow;
	VmInstruction *registerOverflowLocation;
};

struct RegVmLoweredModule
{
	RegVmLoweredModule(Allocator *allocator, VmModule *vmModule): allocator(allocator), vmModule(vmModule), functions(allocator), constants(allocator)
	{
	}

	// Returns index + 1 or 0 if not found
	unsigned FindConstant(unsigned value);
	unsigned FindConstant(unsigned value1, unsigned value2);

	Allocator *allocator;

	VmModule *vmModule;

	SmallArray<RegVmLoweredFunction*, 32> functions;

	SmallArray<unsigned, 256> constants;
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
	FastVector<unsigned> constants;
	FastVector<unsigned char> regKillInfo;

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
