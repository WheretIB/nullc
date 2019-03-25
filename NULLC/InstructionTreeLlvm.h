#pragma once

struct ExpressionContext;
struct ExprModule;

struct LlvmModule
{
	LlvmModule(): moduleData(0), moduleSize(0)
	{
	}

	char *moduleData;
	unsigned moduleSize;
};

LlvmModule* CompileLlvm(ExpressionContext &exprCtx, ExprModule *expression);
