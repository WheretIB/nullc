#pragma once

#include <stdio.h>

struct ExpressionContext;

struct ExprBase;

struct ExpressionTranslateContext
{
	ExpressionTranslateContext(ExpressionContext &ctx): ctx(ctx)
	{
		file = 0;

		mainName = "main";

		indent = "\t";

		depth = 0;

		skipFunctionDefinitions = false;
		isInGlobalCode = false;
	}

	ExpressionContext &ctx;

	FILE *file;

	const char *mainName;

	const char *indent;

	unsigned depth;

	bool skipFunctionDefinitions;
	bool isInGlobalCode;

private:
	ExpressionTranslateContext(const ExpressionTranslateContext&);
	ExpressionTranslateContext& operator=(const ExpressionTranslateContext&);
};

void Translate(ExpressionTranslateContext &ctx, ExprBase *expression);
