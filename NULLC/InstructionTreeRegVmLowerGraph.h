#pragma once

#include "InstructionTreeRegVmLower.h"
#include "Output.h"

struct ExternFuncInfo;
struct ExpressionContext;
struct VmConstant;
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

void PrintInstruction(OutputContext &ctx, char *constantData, ExternFuncInfo *functionData, char *symbolData, RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, unsigned argument, VmConstant *constant);

void PrintGraph(InstructionRegVmLowerGraphContext &ctx, RegVmLoweredModule *lowModule);

void DumpGraph(RegVmLoweredModule *lowModule);
