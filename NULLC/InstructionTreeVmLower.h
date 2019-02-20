#pragma once

#include "Array.h"
#include "InstructionSet.h"

struct ExpressionContext;

struct SynBase;

struct VmConstant;
struct VmInstruction;
struct VmBlock;
struct VmFunction;
struct VmModule;

struct VmLoweredBlock;
struct VmLoweredFunction;
struct VmLoweredModule;

struct VmLoweredInstruction
{
	VmLoweredInstruction(SynBase *location, InstructionCode cmd, VmConstant *flag, VmConstant *helper, VmConstant *argument): location(location), cmd(cmd), flag(flag), helper(helper), argument(argument)
	{
		parent = NULL;

		prevSibling = NULL;
		nextSibling = NULL;

		stackDepthBefore = 0;
		stackDepthAfter = 0;
	}

	SynBase *location;

	InstructionCode cmd;
	VmConstant *flag;
	VmConstant *helper;
	VmConstant *argument;

	VmLoweredBlock *parent;

	VmLoweredInstruction *prevSibling;
	VmLoweredInstruction *nextSibling;

	unsigned stackDepthBefore;
	unsigned stackDepthAfter;
};

struct VmLoweredBlock
{
	VmLoweredBlock(VmBlock *vmBlock): vmBlock(vmBlock)
	{
		firstInstruction = NULL;
		lastInstruction = NULL;

		stackDepth = 0;
	}

	void AddInstruction(ExpressionContext &ctx, VmLoweredInstruction* instruction);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, VmConstant *argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, VmBlock *argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, VmConstant *helper, VmConstant *argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned short helper, VmConstant *argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned short helper, unsigned argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, VmConstant *flag, VmConstant *helper, VmConstant *argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned char flag, unsigned short helper, unsigned argument);
	void AddInstruction(ExpressionContext &ctx, SynBase *location, InstructionCode cmd, unsigned char flag, unsigned short helper, VmConstant *argument);

	void AttachInstructionAt(VmLoweredInstruction* instruction, VmLoweredInstruction* insertPoint);
	void DetachInstruction(VmLoweredInstruction* instruction);
	void RemoveInstruction(VmLoweredInstruction* instruction);

	VmBlock *vmBlock;

	VmLoweredInstruction *firstInstruction;
	VmLoweredInstruction *lastInstruction;

	unsigned stackDepth;
};

struct VmLoweredFunction
{
	VmLoweredFunction(Allocator *allocator, VmFunction *vmFunction): vmFunction(vmFunction), blocks(allocator)
	{
	}

	VmFunction *vmFunction;

	SmallArray<VmLoweredBlock*, 16> blocks;
};

struct VmLoweredModule
{
	VmLoweredModule(Allocator *allocator, VmModule *vmModule): vmModule(vmModule), functions(allocator)
	{
		removedSpilledRegisters = 0;
	}

	VmModule *vmModule;

	SmallArray<VmLoweredFunction*, 16> functions;

	unsigned removedSpilledRegisters;
};

bool IsBlockTerminator(VmLoweredInstruction *lowInstruction);
bool HasMemoryWrite(VmLoweredInstruction *lowInstruction);
bool HasMemoryAccess(VmLoweredInstruction *lowInstruction);

VmLoweredModule* LowerModule(ExpressionContext &ctx, VmModule *module);

void OptimizeTemporaryRegisterSpills(VmLoweredModule *lowModule);

void FinalizeRegisterSpills(ExpressionContext &ctx, VmLoweredModule *lowModule);

struct InstructionVmFinalizeContext
{
	InstructionVmFinalizeContext(ExpressionContext &ctx, Allocator *allocator): ctx(ctx), fixupPoints(allocator)
	{
		currentFunction = 0;
		currentBlock = 0;
	}

	ExpressionContext &ctx;

	FastVector<SynBase*> locations;
	FastVector<VMCmd> cmds;

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
	InstructionVmFinalizeContext(const InstructionVmFinalizeContext&);
	InstructionVmFinalizeContext& operator=(const InstructionVmFinalizeContext&);
};

void FinalizeModule(InstructionVmFinalizeContext &ctx, VmLoweredModule *lowModule);
