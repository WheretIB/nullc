#pragma once

#include <stdio.h>

#include "Array.h"
#include "ExpressionTree.h"

struct FunctionData;

struct ExpressionTranslateContext
{
	ExpressionTranslateContext(ExpressionContext &ctx): ctx(ctx), loopIdStack(ctx.allocator)
	{
		file = 0;

		mainName = "main";

		indent = "\t";

		depth = 0;

		nextLoopId = 1;

		skipFunctionDefinitions = false;

		currentFunction = 0;
	}

	ExpressionContext &ctx;

	FILE *file;

	const char *mainName;

	const char *indent;

	unsigned depth;

	unsigned nextLoopId;

	SmallArray<unsigned, 32> loopIdStack;

	bool skipFunctionDefinitions;

	FunctionData *currentFunction;

private:
	ExpressionTranslateContext(const ExpressionTranslateContext&);
	ExpressionTranslateContext& operator=(const ExpressionTranslateContext&);
};

void Translate(ExpressionTranslateContext &ctx, ExprBase *expression);
