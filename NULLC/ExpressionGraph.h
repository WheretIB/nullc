#pragma once

#include "Output.h"

struct ScopeData;

struct ExprBase;

struct ExpressionGraphContext
{
	ExpressionGraphContext(OutputContext &output): output(output)
	{
		depth = 0;

		skipImported = false;
		skipFunctionDefinitions = false;
	}

	OutputContext &output;

	unsigned depth;

	bool skipImported;
	bool skipFunctionDefinitions;

private:
	ExpressionGraphContext(const ExpressionGraphContext&);
	ExpressionGraphContext& operator=(const ExpressionGraphContext&);
};

void PrintGraph(ExpressionGraphContext &ctx, ScopeData *scope, bool printImported);
void PrintGraph(ExpressionGraphContext &ctx, ExprBase *expression, const char *name);

void DumpGraph(ExprBase *tree);
void DumpGraph(ScopeData *scope);
