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
		file = 0;

		depth = 0;

		showTypes = true;
		showFullTypes = false;
		showUsers = false;
		showComments = true;
		showContainers = true;

		displayAsTree = false;
	}

	FILE *file;

	unsigned depth;

	bool showTypes;
	bool showFullTypes;
	bool showUsers;
	bool showComments;
	bool showContainers;

	bool displayAsTree;
};

void PrintConstant(InstructionVMGraphContext &ctx, VmConstant *constant);
void PrintInstruction(InstructionVMGraphContext &ctx, VmInstruction *instruction);
void PrintBlock(InstructionVMGraphContext &ctx, VmBlock *block);
void PrintFunction(InstructionVMGraphContext &ctx, VmFunction *function);
void PrintGraph(InstructionVMGraphContext &ctx, VmModule *module);

void DumpGraph(VmModule *module);
