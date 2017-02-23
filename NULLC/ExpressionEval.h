#pragma once

struct ExprBase;

struct ExpressionContext;

ExprBase* Evaluate(ExpressionContext &ctx, ExprBase *expression);
