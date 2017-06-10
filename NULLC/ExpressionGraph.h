#pragma once

#include <stdio.h>

struct ScopeData;

struct ExprBase;

struct ExpressionGraphContext
{
	ExpressionGraphContext()
	{
		file = 0;

		depth = 0;

		skipImported = false;
		skipFunctionDefinitions = false;
	}

	FILE *file;

	unsigned depth;

	bool skipImported;
	bool skipFunctionDefinitions;
};

void PrintGraph(ExpressionGraphContext &ctx, ScopeData *scope, bool printImported);
void PrintGraph(ExpressionGraphContext &ctx, ExprBase *expression, const char *name);
