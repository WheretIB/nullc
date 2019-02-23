#pragma once

#include "Output.h"

struct VmLoweredModule;
struct InstructionVmFinalizeContext;

struct InstructionVmLowerGraphContext
{
	InstructionVmLowerGraphContext(OutputContext &output): output(output)
	{
		code = 0;

		lastStart = 0;
		lastStartOffset = 0;
		lastEndOffset = 0;

		showSource = false;
		showAnnotatedSource = false;
	}

	const char *code;

	OutputContext &output;

	const char *lastStart;
	unsigned lastStartOffset;
	unsigned lastEndOffset;

	bool showSource;
	bool showAnnotatedSource;

private:
	InstructionVmLowerGraphContext(const InstructionVmLowerGraphContext&);
	InstructionVmLowerGraphContext& operator=(const InstructionVmLowerGraphContext&);
};

void PrintGraph(InstructionVmLowerGraphContext &ctx, VmLoweredModule *lowModule);
void PrintInstructions(InstructionVmLowerGraphContext &ctx, InstructionVmFinalizeContext &lowerCtx, const char *code);

void DumpGraph(VmLoweredModule *lowModule);
