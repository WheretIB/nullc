#pragma once

#include <stdio.h>

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
		file = 0;

		currentFunction = 0;
		currentBlock = 0;

		lastStart = 0;
		lastStartOffset = 0;
		lastEndOffset = 0;

		showSource = false;
		showAnnotatedSource = false;
	}

	ExpressionContext &ctx;

	FastVector<SynBase*> locations;
	FastVector<VMCmd> cmds;

	FILE *file;

	const char *lastStart;
	unsigned lastStartOffset;
	unsigned lastEndOffset;

	bool showSource;
	bool showAnnotatedSource;

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
void PrintInstructions(InstructionVMLowerContext &ctx, const char *code);
