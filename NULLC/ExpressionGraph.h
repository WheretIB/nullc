#pragma once

#include <stdio.h>

struct ExprBase;

struct ExpressionGraphContext
{
	ExpressionGraphContext()
	{
		file = 0;

		depth = 0;

		skipFunctionDefinitions = false;
	}

	FILE *file;

	unsigned depth;

	bool skipFunctionDefinitions;
};

void PrintGraph(ExpressionGraphContext &ctx, ExprBase *expression, const char *name);
