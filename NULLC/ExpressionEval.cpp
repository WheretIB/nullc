#include "ExpressionEval.h"

#include "ExpressionTree.h"

bool TryTakeLong(ExprBase *expression, long long &result)
{
	if(ExprBoolLiteral *expr = getType<ExprBoolLiteral>(expression))
	{
		result = expr->value ? 1 : 0;
		return true;
	}

	if(ExprCharacterLiteral *expr = getType<ExprCharacterLiteral>(expression))
	{
		result = expr->value;
		return true;
	}

	if(ExprIntegerLiteral *expr = getType<ExprIntegerLiteral>(expression))
	{
		result = expr->value;
		return true;
	}

	if(ExprRationalLiteral *expr = getType<ExprRationalLiteral>(expression))
	{
		result = (long long)expr->value;
		return true;
	}

	return false;
}

bool TryTakeDouble(ExprBase *expression, double &result)
{
	if(ExprBoolLiteral *expr = getType<ExprBoolLiteral>(expression))
	{
		result = expr->value ? 1.0 : 0.0;
		return true;
	}

	if(ExprCharacterLiteral *expr = getType<ExprCharacterLiteral>(expression))
	{
		result = (double)expr->value;
		return true;
	}

	if(ExprIntegerLiteral *expr = getType<ExprIntegerLiteral>(expression))
	{
		result = (double)expr->value;
		return true;
	}

	if(ExprRationalLiteral *expr = getType<ExprRationalLiteral>(expression))
	{
		result = expr->value;
		return true;
	}

	return false;
}

ExprBase* EvaluateCast(ExpressionContext &ctx, ExprTypeCast *expression)
{
	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	if(ctx.IsIntegerType(expression->type))
	{
		long long result = 0;

		if(TryTakeLong(value, result))
		{
			if(expression->type == ctx.typeBool)
				return new ExprBoolLiteral(expression->source, ctx.typeBool, result != 0);

			if(expression->type == ctx.typeChar)
			{
				if((char)result != result)
					ctx.Stop(expression->source->pos, "ERROR: truncation of value '%lld' while casting to 'char'", result);

				return new ExprCharacterLiteral(expression->source, ctx.typeChar, (char)result);
			}

			if(expression->type == ctx.typeShort)
			{
				if((short)result != result)
					ctx.Stop(expression->source->pos, "ERROR: truncation of value '%lld' while casting to 'short'", result);

				return new ExprIntegerLiteral(expression->source, ctx.typeShort, (short)result);
			}

			if(expression->type == ctx.typeInt)
			{
				if((int)result != result)
					ctx.Stop(expression->source->pos, "ERROR: truncation of value '%lld' while casting to 'int'", result);

				return new ExprIntegerLiteral(expression->source, ctx.typeInt, (int)result);
			}

			if(expression->type == ctx.typeLong)
				return new ExprIntegerLiteral(expression->source, ctx.typeLong, result);
		}
	}
	else if(ctx.IsFloatingPointType(expression->type))
	{
		double result = 0.0;

		if(TryTakeDouble(value, result))
		{
			if(expression->type == ctx.typeFloat)
				return new ExprRationalLiteral(expression->source, ctx.typeFloat, (float)result);

			if(expression->type == ctx.typeDouble)
				return new ExprRationalLiteral(expression->source, ctx.typeDouble, result);
		}
	}

	return NULL;
}

ExprBase* EvaluateUnaryOp(ExpressionContext &ctx, ExprUnaryOp *expression)
{
	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	if(value->type == ctx.typeBool)
	{
		if(ExprBoolLiteral *expr = getType<ExprBoolLiteral>(value))
		{
			if(expression->op == SYN_UNARY_OP_LOGICAL_NOT)
				return new ExprBoolLiteral(expression->source, expression->type, !expr->value);
		}
	}
	else if(ctx.IsIntegerType(value->type))
	{
		long long result = 0;

		if(TryTakeLong(value, result))
		{
			switch(expression->op)
			{
			case SYN_UNARY_OP_PLUS:
				return new ExprIntegerLiteral(expression->source, expression->type, result);
			case SYN_UNARY_OP_NEGATE:
				return new ExprIntegerLiteral(expression->source, expression->type, -result);
			case SYN_UNARY_OP_BIT_NOT:
				return new ExprIntegerLiteral(expression->source, expression->type, ~result);
			case SYN_UNARY_OP_LOGICAL_NOT:
				return new ExprIntegerLiteral(expression->source, expression->type, !result);
			}
		}
	}
	else if(ctx.IsFloatingPointType(value->type))
	{
		double result = 0.0;

		if(TryTakeDouble(value, result))
		{
			switch(expression->op)
			{
			case SYN_UNARY_OP_PLUS:
				return new ExprRationalLiteral(expression->source, expression->type, result);
			case SYN_UNARY_OP_NEGATE:
				return new ExprRationalLiteral(expression->source, expression->type, -result);
			}
		}
	}

	return NULL;
}

ExprBase* EvaluateBinaryOp(ExpressionContext &ctx, ExprBinaryOp *expression)
{
	ExprBase *lhs = Evaluate(ctx, expression->lhs);
	ExprBase *rhs = Evaluate(ctx, expression->rhs);

	if(!lhs || !rhs)
		return NULL;

	if(ctx.IsIntegerType(lhs->type) && ctx.IsIntegerType(rhs->type))
	{
		long long lhsValue = 0;
		long long rhsValue = 0;

		if(TryTakeLong(lhs, lhsValue) && TryTakeLong(rhs, rhsValue))
		{
			switch(expression->op)
			{
			case SYN_BINARY_OP_ADD:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue + rhsValue);
			case SYN_BINARY_OP_SUB:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue - rhsValue);
			case SYN_BINARY_OP_MUL:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue * rhsValue);
			case SYN_BINARY_OP_DIV:
				if(rhsValue == 0)
					ctx.Stop(expression->source->pos, "ERROR: division by zero during constant folding");

				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue / rhsValue);
			case SYN_BINARY_OP_MOD:
				if(rhsValue == 0)
					ctx.Stop(expression->source->pos, "ERROR: modulus division by zero during constant folding");

				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue % rhsValue);
			case SYN_BINARY_OP_POW:
				if(rhsValue < 0)
					ctx.Stop(expression->source->pos, "ERROR: negative power on integer number in exponentiation during constant folding");

				long long result, power;

				result = 1;
				power = rhsValue;

				while(power)
				{
					if(power & 1)
					{
						result *= lhsValue;
						power--;
					}
					lhsValue *= lhsValue;
					power >>= 1;
				}

				return new ExprIntegerLiteral(expression->source, expression->type, result);
			case SYN_BINARY_OP_SHL:
				if(rhsValue < 0)
					ctx.Stop(expression->source->pos, "ERROR: negative shift value");

				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue << rhsValue);
			case SYN_BINARY_OP_SHR:
				if(rhsValue < 0)
					ctx.Stop(expression->source->pos, "ERROR: negative shift value");

				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue >> rhsValue);
			case SYN_BINARY_OP_LESS:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue < rhsValue);
			case SYN_BINARY_OP_LESS_EQUAL:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue <= rhsValue);
			case SYN_BINARY_OP_GREATER:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue > rhsValue);
			case SYN_BINARY_OP_GREATER_EQUAL:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue >= rhsValue);
			case SYN_BINARY_OP_EQUAL:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue == rhsValue);
			case SYN_BINARY_OP_NOT_EQUAL:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue != rhsValue);
			case SYN_BINARY_OP_BIT_AND:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue & rhsValue);
			case SYN_BINARY_OP_BIT_OR:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue | rhsValue);
			case SYN_BINARY_OP_BIT_XOR:
				return new ExprIntegerLiteral(expression->source, expression->type, lhsValue ^ rhsValue);
			}
		}
	}
	else if(lhs->type == ctx.typeTypeID && rhs->type == ctx.typeTypeID)
	{
		if(expression->op == SYN_BINARY_OP_EQUAL)
			return new ExprBoolLiteral(expression->source, ctx.typeBool, lhs->type == rhs->type);

		if(expression->op == SYN_BINARY_OP_NOT_EQUAL)
			return new ExprBoolLiteral(expression->source, ctx.typeBool, lhs->type != rhs->type);
	}

	return NULL;
}

ExprBase* Evaluate(ExpressionContext &ctx, ExprBase *expression)
{
	if(ExprVoid *expr = getType<ExprVoid>(expression))
		return expr;

	if(ExprBoolLiteral *expr = getType<ExprBoolLiteral>(expression))
		return expr;

	if(ExprCharacterLiteral *expr = getType<ExprCharacterLiteral>(expression))
		return expr;

	if(ExprStringLiteral *expr = getType<ExprStringLiteral>(expression))
		return expr;

	if(ExprIntegerLiteral *expr = getType<ExprIntegerLiteral>(expression))
		return expr;

	if(ExprRationalLiteral *expr = getType<ExprRationalLiteral>(expression))
		return expr;

	if(ExprTypeLiteral *expr = getType<ExprTypeLiteral>(expression))
		return expr;

	if(ExprNullptrLiteral *expr = getType<ExprNullptrLiteral>(expression))
		return expr;

	if(ExprTypeCast *expr = getType<ExprTypeCast>(expression))
		return EvaluateCast(ctx, expr);

	if(ExprUnaryOp *expr = getType<ExprUnaryOp>(expression))
		return EvaluateUnaryOp(ctx, expr);

	if(ExprBinaryOp *expr = getType<ExprBinaryOp>(expression))
		return EvaluateBinaryOp(ctx, expr);

	return NULL;
}
