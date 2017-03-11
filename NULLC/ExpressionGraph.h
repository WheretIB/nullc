#pragma once

#include <stdio.h>

struct ExprBase;

struct ExpressionGraphContext
{
	ExpressionGraphContext()
	{
		file = 0;

		depth = 0;
	}

	FILE *file;

	unsigned depth;
};

void PrintGraph(ExpressionGraphContext &ctx, ExprBase *expression, const char *name);
