#pragma once

#include "stdafx.h"
#include "Array.h"

struct VariableData;

struct ExprBase;
struct ExprPointerLiteral;

struct ExpressionContext;

struct ExpressionEvalContext
{
	ExpressionEvalContext(ExpressionContext &ctx): ctx(ctx)
	{
		errorBuf = 0;
		errorBufSize = 0;

		globalFrame = 0;

		emulateKnownExternals = false;

		stackDepthLimit = 64;

		variableMemoryLimit = 8 * 1024;

		totalMemory = 0;
		totalMemoryLimit = 64 * 1024;

		instruction = 0;
		instructionsLimit = 64 * 1024;
	}

	ExpressionContext &ctx;

	struct StackVariable
	{
		StackVariable(): variable(0)
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
		StackFrame(): returnValue(0), breakDepth(0), continueDepth(0)
		{
		}

		SmallArray<StackVariable, 32> variables;

		ExprBase *returnValue;

		unsigned breakDepth;
		unsigned continueDepth;
	};

	// Error info
	char *errorBuf;
	unsigned errorBufSize;

	StackFrame *globalFrame;

	SmallArray<StackFrame*, 32> stackFrames;
	unsigned stackDepthLimit;

	bool emulateKnownExternals;

	unsigned variableMemoryLimit;

	unsigned totalMemory;
	unsigned totalMemoryLimit;

	unsigned instruction;
	unsigned instructionsLimit;

private:
	ExpressionEvalContext(const ExpressionEvalContext&);
	ExpressionEvalContext& operator=(const ExpressionEvalContext&);
};

ExprBase* Evaluate(ExpressionEvalContext &ctx, ExprBase *expression);
