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
	InstructionVMEvalContext(ExpressionContext &ctx, Allocator *allocator): ctx(ctx), allocator(allocator), stackFrames(allocator)
	{
		module = 0;

		errorBuf = 0;
		errorBufSize = 0;

		globalFrame = 0;

		emulateKnownExternals = false;

		stackDepthLimit = 64;

		frameMemoryLimit = 8 * 1024;

		instruction = 0;
		instructionsLimit = 64 * 1024;
	}

	struct Storage
	{
		Storage(Allocator *allocator): data(allocator)
		{
		}

		bool Reserve(InstructionVMEvalContext &ctx, unsigned offset, unsigned size);

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

	VmModule *module;

	// Error info
	char *errorBuf;
	unsigned errorBufSize;

	StackFrame *globalFrame;

	SmallArray<StackFrame*, 32> stackFrames;
	unsigned stackDepthLimit;

	bool emulateKnownExternals;

	unsigned frameMemoryLimit;

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
