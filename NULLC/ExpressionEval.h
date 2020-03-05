#pragma once

#include "stdafx.h"
#include "Array.h"

struct FunctionData;
struct VariableData;

struct ExprBase;
struct ExprPointerLiteral;

struct ExpressionContext;

struct ExpressionEvalContext
{
	ExpressionEvalContext(ExpressionContext &ctx, Allocator *allocator): ctx(ctx), stackFrames(allocator), abandonedMemory(allocator)
	{
		errorBuf = 0;
		errorBufSize = 0;
		errorCritical = false;

		globalFrame = 0;

		emulateKnownExternals = false;

		stackDepthLimit = 64;

		variableMemoryLimit = 8 * 1024;

		totalMemory = 0;
		totalMemoryLimit = 64 * 1024;

		instruction = 0;
		instructionsLimit = 64 * 1024;

		expressionDepth = 0;
		expressionDepthLimit = 2048;
	}

	ExpressionContext &ctx;

	struct StackVariable
	{
		StackVariable(): variable(0), ptr(0)
		{
		}

		StackVariable(VariableData *variable, ExprPointerLiteral *ptr): variable(variable), ptr(ptr)
		{
		}

		VariableData *variable;
		ExprPointerLiteral *ptr;
	};

	struct StackFrame
	{
		StackFrame(Allocator *allocator, FunctionData *owner): owner(owner), variables(allocator), returnValue(0), targetYield(0), breakDepth(0), continueDepth(0)
		{
		}

		FunctionData *owner;

		SmallArray<StackVariable, 32> variables;

		ExprBase *returnValue;

		unsigned targetYield;

		unsigned breakDepth;
		unsigned continueDepth;
	};

	// Error info
	char *errorBuf;
	unsigned errorBufSize;
	bool errorCritical; // If error wasn't a limitation of the evaluation engine, but was an error in the program itself

	StackFrame *globalFrame;

	SmallArray<StackFrame*, 32> stackFrames;
	unsigned stackDepthLimit;

	bool emulateKnownExternals;

	SmallArray<ExprPointerLiteral*, 32> abandonedMemory;

	unsigned variableMemoryLimit;

	unsigned totalMemory;
	unsigned totalMemoryLimit;

	unsigned instruction;
	unsigned instructionsLimit;

	unsigned expressionDepth;
	unsigned expressionDepthLimit;

private:
	ExpressionEvalContext(const ExpressionEvalContext&);
	ExpressionEvalContext& operator=(const ExpressionEvalContext&);
};

ExprBase* Evaluate(ExpressionEvalContext &ctx, ExprBase *expression);
bool EvaluateToBuffer(ExpressionEvalContext &ctx, ExprBase *expression, char *resultBuf, unsigned resultBufSize);

bool TestEvaluation(ExpressionContext &ctx, ExprBase *expression, char *resultBuf, unsigned resultBufSize, char *errorBuf, unsigned errorBufSize);
