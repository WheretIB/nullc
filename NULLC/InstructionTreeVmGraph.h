#pragma once

#include "Output.h"

struct VmValue;
struct VmConstant;
struct VmInstruction;
struct VmBlock;
struct VmFunction;
struct VmModule;

struct InstructionVMGraphContext
{
	InstructionVMGraphContext(OutputContext &output): output(output)
	{
		code = 0;

		depth = 0;

		lastStart = 0;
		lastStartOffset = 0;
		lastEndOffset = 0;

		showTypes = true;
		showFullTypes = false;
		showUsers = false;
		showComments = true;
		showContainers = true;
		showSource = false;
		showAnnotatedSource = false;

		displayAsTree = false;
	}

	const char *code;

	OutputContext &output;

	unsigned depth;

	const char *lastStart;
	unsigned lastStartOffset;
	unsigned lastEndOffset;

	bool showTypes;
	bool showFullTypes;
	bool showUsers;
	bool showComments;
	bool showContainers;
	bool showSource;
	bool showAnnotatedSource;

	bool displayAsTree;

private:
	InstructionVMGraphContext(const InstructionVMGraphContext&);
	InstructionVMGraphContext& operator=(const InstructionVMGraphContext&);
};

void PrintConstant(InstructionVMGraphContext &ctx, VmConstant *constant);
void PrintInstruction(InstructionVMGraphContext &ctx, VmInstruction *instruction);
void PrintBlock(InstructionVMGraphContext &ctx, VmBlock *block);
void PrintFunction(InstructionVMGraphContext &ctx, VmFunction *function);
void PrintGraph(InstructionVMGraphContext &ctx, VmModule *module);

void DumpGraph(VmModule *module);
