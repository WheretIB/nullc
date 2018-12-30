#pragma once

#include <stdio.h>

struct VmValue;
struct VmConstant;
struct VmInstruction;
struct VmBlock;
struct VmFunction;
struct VmModule;

struct InstructionVMGraphContext
{
	InstructionVMGraphContext()
	{
		code = 0;

		file = 0;

		depth = 0;

		lastStart = 0;

		showTypes = true;
		showFullTypes = false;
		showUsers = false;
		showComments = true;
		showContainers = true;
		showSource = false;

		displayAsTree = false;
	}

	const char *code;

	FILE *file;

	unsigned depth;

	const char *lastStart;

	bool showTypes;
	bool showFullTypes;
	bool showUsers;
	bool showComments;
	bool showContainers;
	bool showSource;

	bool displayAsTree;
};

void PrintConstant(InstructionVMGraphContext &ctx, VmConstant *constant);
void PrintInstruction(InstructionVMGraphContext &ctx, VmInstruction *instruction);
void PrintBlock(InstructionVMGraphContext &ctx, VmBlock *block);
void PrintFunction(InstructionVMGraphContext &ctx, VmFunction *function);
void PrintGraph(InstructionVMGraphContext &ctx, VmModule *module);

void DumpGraph(VmModule *module);
