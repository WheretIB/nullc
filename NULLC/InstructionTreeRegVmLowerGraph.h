#pragma once

#include "Output.h"

struct RegVmLoweredModule;
struct InstructionRegVmFinalizeContext;

struct InstructionRegVmLowerGraphContext
{
	InstructionRegVmLowerGraphContext(OutputContext &output): output(output)
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
	InstructionRegVmLowerGraphContext(const InstructionRegVmLowerGraphContext&);
	InstructionRegVmLowerGraphContext& operator=(const InstructionRegVmLowerGraphContext&);
};

void PrintGraph(InstructionRegVmLowerGraphContext &ctx, RegVmLoweredModule *lowModule);

void DumpGraph(RegVmLoweredModule *lowModule);
