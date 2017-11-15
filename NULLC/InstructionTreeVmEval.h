#pragma once

#include "stdafx.h"
#include "Array.h"

struct VmValue;
struct VmConstant;
struct VmInstruction;
struct VmBlock;
struct VmFunction;
struct VmModule;

struct ExpressionContext;

struct InstructionVMEvalContext
{
	InstructionVMEvalContext(ExpressionContext &ctx, Allocator *allocator): ctx(ctx), allocator(allocator), stackFrames(allocator), heap(allocator), storageSet(allocator)
	{
		errorBuf = 0;
		errorBufSize = 0;
		hasError = false;

		globalFrame = 0;

		emulateKnownExternals = false;

		stackDepthLimit = 64;

		heapSize = 0;

		frameMemoryLimit = 8 * 1024;

		variableMemoryLimit = 4 * 1024;

		instruction = 0;
		instructionsLimit = 64 * 1024;
	}

	struct Storage
	{
		Storage(Allocator *allocator): index(0u), data(allocator)
		{
		}

		bool Reserve(InstructionVMEvalContext &ctx, unsigned offset, unsigned size);

		unsigned index;

		SmallArray<char, 128> data;
	};

	struct StackFrame
	{
		StackFrame(Allocator *allocator, VmFunction *owner): owner(owner), instructionValues(allocator), allocas(allocator), stack(allocator)
		{
		}

		void AssignRegister(unsigned id, VmConstant *constant);

		VmConstant* ReadRegister(unsigned id);

		VmFunction *owner;

		SmallArray<VmConstant*, 128> instructionValues;

		Storage allocas;
		Storage stack;
	};

	ExpressionContext &ctx;

	// Error info
	char *errorBuf;
	unsigned errorBufSize;
	bool hasError;

	StackFrame *globalFrame;

	SmallArray<StackFrame*, 32> stackFrames;
	unsigned stackDepthLimit;

	Storage heap;
	unsigned heapSize;

	SmallArray<Storage*, 32> storageSet;

	bool emulateKnownExternals;

	unsigned frameMemoryLimit;

	unsigned variableMemoryLimit;

	unsigned instruction;
	unsigned instructionsLimit;

	// Memory pool
	Allocator *allocator;

	template<typename T>
	T* get()
	{
		return (T*)allocator->alloc(sizeof(T));
	}

private:
	InstructionVMEvalContext(const InstructionVMEvalContext&);
	InstructionVMEvalContext& operator=(const InstructionVMEvalContext&);
};

VmConstant* EvaluateInstruction(InstructionVMEvalContext &ctx, VmInstruction *instruction, VmBlock *predecessor, VmBlock **nextBlock);
VmConstant* EvaluateFunction(InstructionVMEvalContext &ctx, VmFunction *function);
VmConstant* EvaluateModule(InstructionVMEvalContext &ctx, VmModule *module);
