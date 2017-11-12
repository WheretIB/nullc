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

		showUsers = false;

		displayAsTree = false;
	}

	FILE *file;

	unsigned depth;

	bool showUsers;

	bool displayAsTree;
};

void PrintConstant(InstructionVMGraphContext &ctx, VmConstant *constant);
void PrintInstruction(InstructionVMGraphContext &ctx, VmInstruction *instruction);
void PrintBlock(InstructionVMGraphContext &ctx, VmBlock *block);
void PrintFunction(InstructionVMGraphContext &ctx, VmFunction *function);
void PrintGraph(InstructionVMGraphContext &ctx, VmModule *module);

void DumpGraph(VmModule *module);
