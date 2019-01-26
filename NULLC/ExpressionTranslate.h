#pragma once

#include <stdio.h>

struct ExpressionContext;

struct ExprBase;

struct FunctionData;

struct ExpressionTranslateContext
{
	ExpressionTranslateContext(ExpressionContext &ctx): ctx(ctx)
	{
		file = 0;

		mainName = "main";

		indent = "\t";

		depth = 0;

		skipFunctionDefinitions = false;

		currentFunction = 0;
	}

	ExpressionContext &ctx;

	FILE *file;

	const char *mainName;

	const char *indent;

	unsigned depth;

	bool skipFunctionDefinitions;

	FunctionData *currentFunction;

private:
	ExpressionTranslateContext(const ExpressionTranslateContext&);
	ExpressionTranslateContext& operator=(const ExpressionTranslateContext&);
};

void Translate(ExpressionTranslateContext &ctx, ExprBase *expression);
