#pragma once

#include "Array.h"
#include "InstructionSet.h"

struct ExpressionContext;

struct SynBase;

struct VmBlock;
struct VmFunction;
struct VmModule;

struct InstructionVMLowerContext
{
	InstructionVMLowerContext(ExpressionContext &ctx, Allocator *allocator): ctx(ctx), fixupPoints(allocator)
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
	InstructionVMLowerContext(const InstructionVMLowerContext&);
	InstructionVMLowerContext& operator=(const InstructionVMLowerContext&);
};

void LowerModule(InstructionVMLowerContext &ctx, VmModule *module);
