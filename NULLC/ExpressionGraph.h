#pragma once

#include "Output.h"

struct ScopeData;

struct ExpressionContext;
struct ExprBase;

struct ExpressionGraphContext
{
	ExpressionGraphContext(ExpressionContext &ctx, OutputContext &output): ctx(ctx), output(output)
	{
		depth = 0;

		skipImported = false;
		skipFunctionDefinitions = false;
	}

	ExpressionContext &ctx;

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
