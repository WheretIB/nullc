#pragma once

struct ExpressionContext;
struct ExprBase;
struct ExprModule;

struct LlvmModule
{
};

LlvmModule* CompileLlvm(ExpressionContext &exprCtx, ExprModule *expression, const char *code);
