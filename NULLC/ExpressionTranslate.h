#pragma once

#include <stdio.h>

#include "Array.h"
#include "ExpressionTree.h"

struct FunctionData;

struct ExpressionTranslateContext
{
	ExpressionTranslateContext(ExpressionContext &ctx, Allocator *allocator): ctx(ctx), allocator(allocator), loopIdStack(allocator)
	{
		file = 0;

		mainName = "main";

		indent = "\t";

		errorPos = 0;
		errorBuf = 0;
		errorBufSize = 0;

		depth = 0;

		nextLoopId = 1;
		nextReturnValueId = 1;

		skipFunctionDefinitions = false;

		currentFunction = 0;
	}

	ExpressionContext &ctx;

	FILE *file;

	const char *mainName;

	const char *indent;

	const char *errorPos;
	char *errorBuf;
	unsigned errorBufSize;

	unsigned depth;

	unsigned nextLoopId;
	unsigned nextReturnValueId;

	SmallArray<unsigned, 32> loopIdStack;

	bool skipFunctionDefinitions;

	FunctionData *currentFunction;

	// Memory pool
	Allocator *allocator;

	template<typename T>
	T* get()
	{
		return (T*)allocator->alloc(sizeof(T));
	}

private:
	ExpressionTranslateContext(const ExpressionTranslateContext&);
	ExpressionTranslateContext& operator=(const ExpressionTranslateContext&);
};

void Translate(ExpressionTranslateContext &ctx, ExprBase *expression);
bool TranslateModule(ExpressionTranslateContext &ctx, ExprModule *expression, SmallArray<const char*, 32> &dependencies);
