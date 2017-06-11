#include "ExpressionEval.h"

#include <math.h>

#include "ExpressionTree.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

typedef ExpressionEvalContext Eval;

ExprBase* Report(Eval &ctx, const char *msg, ...)
{
	if(ctx.errorBuf && ctx.errorBufSize)
	{
		va_list args;
		va_start(args, msg);

		vsnprintf(ctx.errorBuf, ctx.errorBufSize, msg, args);

		va_end(args);

		ctx.errorBuf[ctx.errorBufSize - 1] = '\0';
	}

	return NULL;
}

bool AddInstruction(Eval &ctx)
{
	if(ctx.instruction < ctx.instructionsLimit)
	{
		ctx.instruction++;
		return true;
	}

	return false;
}

unsigned GetTypeIndex(Eval &ctx, TypeBase *type)
{
	unsigned index = ~0u;

	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
	{
		if(ctx.ctx.types[i] == type)
		{
			index = i;
			break;
		}
	}

	assert(index != ~0u);

	return index;
}

unsigned GetFunctionIndex(Eval &ctx, FunctionData *data)
{
	unsigned index = ~0u;

	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
	{
		if(ctx.ctx.functions[i] == data)
		{
			index = i;
			break;
		}
	}

	assert(index != ~0u);

	return index;
}

ExprPointerLiteral* AllocateTypeStorage(Eval &ctx, SynBase *source, TypeBase *type)
{
	if(type->size > ctx.variableMemoryLimit)
		return (ExprPointerLiteral*)Report(ctx, "ERROR: single variable memory limit");

	if(ctx.totalMemory + type->size > ctx.totalMemoryLimit)
		return (ExprPointerLiteral*)Report(ctx, "ERROR: total variable memory limit");

	ctx.totalMemory += unsigned(type->size);

	unsigned char *memory = new unsigned char[unsigned(type->size)];

	memset(memory, 0, unsigned(type->size));

	return new ExprPointerLiteral(source, ctx.ctx.GetReferenceType(type), memory, memory + type->size);
}

bool CreateStore(Eval &ctx, ExprBase *target, ExprBase *value)
{
	if(isType<ExprNullptrLiteral>(target))
	{
		Report(ctx, "ERROR: store to null pointer");

		return false;
	}

	ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(target);

	assert(ptr);
	assert(ptr->ptr + value->type->size <= ptr->end);

	if(ExprBoolLiteral *expr = getType<ExprBoolLiteral>(value))
	{
		memcpy(ptr->ptr, &expr->value, unsigned(value->type->size));
		return true;
	}

	if(ExprCharacterLiteral *expr = getType<ExprCharacterLiteral>(value))
	{
		memcpy(ptr->ptr, &expr->value, unsigned(value->type->size));
		return true;
	}

	if(ExprStringLiteral *expr = getType<ExprStringLiteral>(value))
	{
		memcpy(ptr->ptr, &expr->value, unsigned(value->type->size));
		return true;
	}

	if(ExprIntegerLiteral *expr = getType<ExprIntegerLiteral>(value))
	{
		memcpy(ptr->ptr, &expr->value, unsigned(value->type->size));
		return true;
	}

	if(ExprRationalLiteral *expr = getType<ExprRationalLiteral>(value))
	{
		if(expr->type == ctx.ctx.typeFloat)
		{
			float tmp = float(expr->value);
			memcpy(ptr->ptr, &tmp, unsigned(value->type->size));
			return true;
		}

		memcpy(ptr->ptr, &expr->value, unsigned(value->type->size));
		return true;
	}

	if(ExprTypeLiteral *expr = getType<ExprTypeLiteral>(value))
	{
		unsigned index = GetTypeIndex(ctx, expr->value);
		memcpy(ptr->ptr, &index, unsigned(value->type->size));
		return true;
	}

	if(ExprNullptrLiteral *expr = getType<ExprNullptrLiteral>(value))
	{
		memset(ptr->ptr, 0, unsigned(value->type->size));
		return true;
	}

	if(ExprFunctionLiteral *expr = getType<ExprFunctionLiteral>(value))
	{
		unsigned index = expr->data ? GetFunctionIndex(ctx, expr->data) + 1 : 0;
		memcpy(ptr->ptr, &index, sizeof(unsigned));

		if(ExprNullptrLiteral *value = getType<ExprNullptrLiteral>(expr->context))
			memset(ptr->ptr + 4, 0, sizeof(void*));
		else if(ExprPointerLiteral *value = getType<ExprPointerLiteral>(expr->context))
			memcpy(ptr->ptr + 4, &value->ptr, sizeof(void*));
		else
			return false;

		return true;
	}

	if(ExprPointerLiteral *expr = getType<ExprPointerLiteral>(value))
	{
		TypeRef *ptrType = getType<TypeRef>(expr->type);

		assert(ptrType);
		assert(expr->ptr + ptrType->subType->size == expr->end);

		memcpy(ptr->ptr, &expr->ptr, unsigned(value->type->size));
		return true;
	}

	if(ExprMemoryLiteral *expr = getType<ExprMemoryLiteral>(value))
	{
		memcpy(ptr->ptr, expr->ptr->ptr, unsigned(value->type->size));
		return true;
	}

	Report(ctx, "ERROR: unknown store type");

	return false;
}

ExprBase* CreateLoad(Eval &ctx, ExprBase *target)
{
	if(isType<ExprNullptrLiteral>(target))
	{
		Report(ctx, "ERROR: load from null pointer");

		return false;
	}

	ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(target);

	assert(ptr);

	TypeRef *refType = getType<TypeRef>(target->type);

	assert(refType);

	TypeBase *type = refType->subType;

	assert(ptr->ptr + type->size <= ptr->end);

	if(type == ctx.ctx.typeBool)
	{
		bool value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new ExprBoolLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeChar)
	{
		unsigned char value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new ExprCharacterLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeShort)
	{
		short value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new ExprIntegerLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeInt)
	{
		int value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new ExprIntegerLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeLong)
	{
		long long value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new ExprIntegerLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeFloat)
	{
		float value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new ExprRationalLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeDouble)
	{
		double value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new ExprRationalLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeTypeID)
	{
		unsigned index = 0;
		memcpy(&index, ptr->ptr, sizeof(unsigned));

		TypeBase *data = ctx.ctx.types[index];

		return new ExprTypeLiteral(target->source, type, data);
	}

	if(TypeFunction *functionType = getType<TypeFunction>(type))
	{
		unsigned index = 0;
		memcpy(&index, ptr->ptr, sizeof(unsigned));

		FunctionData *data = index != 0 ? ctx.ctx.functions[index - 1] : NULL;

		unsigned char *value = 0;
		memcpy(&value, ptr->ptr + 4, sizeof(value));
		
		if(!data)
		{
			assert(value == NULL);

			return new ExprFunctionLiteral(target->source, type, NULL, new ExprNullptrLiteral(target->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		}

		TypeRef *ptrType = getType<TypeRef>(data->contextType);

		assert(ptrType);

		ExprBase *context = NULL;

		if(value == NULL)
			context = new ExprNullptrLiteral(target->source, ptrType);
		else
			context = new ExprPointerLiteral(target->source, ptrType, value, value + ptrType->subType->size);

		return new ExprFunctionLiteral(target->source, type, data, context);
	}

	if(TypeRef *ptrType = getType<TypeRef>(type))
	{
		unsigned char *value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));
		
		if(value == NULL)
			return new ExprNullptrLiteral(target->source, type);

		return new ExprPointerLiteral(target->source, type, value, value + ptrType->subType->size);
	}

	ExprPointerLiteral *storage = AllocateTypeStorage(ctx, target->source, type);

	if(!storage)
		return NULL;

	memcpy(storage->ptr, ptr->ptr, unsigned(type->size));

	return new ExprMemoryLiteral(target->source, type, storage);
}

bool CreateInsert(Eval &ctx, ExprMemoryLiteral *memory, unsigned offset, ExprBase *value)
{
	assert(memory->ptr->ptr + value->type->size <= memory->ptr->end);

	ExprPointerLiteral addr(memory->source, ctx.ctx.GetReferenceType(value->type), memory->ptr->ptr + offset, memory->ptr->ptr + offset + value->type->size);

	if(!CreateStore(ctx, &addr, value))
		return false;

	return true;
}

ExprBase* CreateExtract(Eval &ctx, ExprMemoryLiteral *memory, unsigned offset, TypeBase *type)
{
	assert(memory->ptr->ptr + type->size <= memory->ptr->end);

	ExprPointerLiteral addr(memory->source, ctx.ctx.GetReferenceType(type), memory->ptr->ptr + offset, memory->ptr->ptr + offset + type->size);

	return CreateLoad(ctx, &addr);
}

ExprMemoryLiteral* CreateConstruct(Eval &ctx, TypeBase *type, ExprBase *el0, ExprBase *el1, ExprBase *el2)
{
	long long size = 0;
	
	if(el0)
		size += el0->type->size;

	if(el1)
		size += el1->type->size;

	if(el2)
		size += el2->type->size;

	assert(type->size == size);

	ExprPointerLiteral *storage = AllocateTypeStorage(ctx, el0->source, type);

	if(!storage)
		return NULL;

	ExprMemoryLiteral *memory = new ExprMemoryLiteral(el0->source, type, storage);

	unsigned offset = 0;

	if(el0 && !CreateInsert(ctx, memory, offset, el0))
		return NULL;
	else if(el0)
		offset += unsigned(el0->type->size);

	if(el1 && !CreateInsert(ctx, memory, offset, el1))
		return NULL;
	else if(el1)
		offset += unsigned(el1->type->size);

	if(el2 && !CreateInsert(ctx, memory, offset, el2))
		return NULL;
	else if(el2)
		offset += unsigned(el2->type->size);

	return memory;
}

ExprPointerLiteral* FindVariableStorage(Eval &ctx, VariableData *data)
{
	if(ctx.stackFrames.empty())
		return (ExprPointerLiteral*)Report(ctx, "ERROR: no stack frame");

	Eval::StackFrame *frame = ctx.stackFrames.back();

	for(unsigned i = 0; i < frame->variables.size(); i++)
	{
		Eval::StackVariable &variable = frame->variables[i];

		if(variable.variable == data)
			return variable.ptr;
	}

	for(unsigned i = 0; i < ctx.globalFrame->variables.size(); i++)
	{
		Eval::StackVariable &variable = ctx.globalFrame->variables[i];

		if(variable.variable == data)
			return variable.ptr;
	}

	return (ExprPointerLiteral*)Report(ctx, "ERROR: variable '%.*s' not found", FMT_ISTR(data->name));
}

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

bool TryTakeTypeId(ExprBase *expression, TypeBase* &result)
{
	if(ExprTypeLiteral *expr = getType<ExprTypeLiteral>(expression))
	{
		result = expr->value;
		return true;
	}

	return false;
}

bool TryTakePointer(ExprBase *expression, void* &result)
{
	if(ExprNullptrLiteral *expr = getType<ExprNullptrLiteral>(expression))
	{
		result = 0;
		return true;
	}
	else if(ExprPointerLiteral *expr = getType<ExprPointerLiteral>(expression))
	{
		result = expr->ptr;
		return true;
	}

	return false;
}

ExprBase* CreateBinaryOp(Eval &ctx, SynBase *source, ExprBase *lhs, ExprBase *rhs, SynBinaryOpType op)
{
	assert(lhs->type == rhs->type);

	if((ctx.ctx.IsIntegerType(lhs->type) || isType<TypeEnum>(lhs->type)) && (ctx.ctx.IsIntegerType(rhs->type) || isType<TypeEnum>(rhs->type)))
	{
		long long lhsValue = 0;
		long long rhsValue = 0;

		if(TryTakeLong(lhs, lhsValue) && TryTakeLong(rhs, rhsValue))
		{
			switch(op)
			{
			case SYN_BINARY_OP_ADD:
				return new ExprIntegerLiteral(source, lhs->type, lhsValue + rhsValue);
			case SYN_BINARY_OP_SUB:
				return new ExprIntegerLiteral(source, lhs->type, lhsValue - rhsValue);
			case SYN_BINARY_OP_MUL:
				return new ExprIntegerLiteral(source, lhs->type, lhsValue * rhsValue);
			case SYN_BINARY_OP_DIV:
				if(rhsValue == 0)
					ctx.ctx.Stop(source->pos, "ERROR: division by zero during constant folding");

				return new ExprIntegerLiteral(source, lhs->type, lhsValue / rhsValue);
			case SYN_BINARY_OP_MOD:
				if(rhsValue == 0)
					ctx.ctx.Stop(source->pos, "ERROR: modulus division by zero during constant folding");

				return new ExprIntegerLiteral(source, lhs->type, lhsValue % rhsValue);
			case SYN_BINARY_OP_POW:
				if(rhsValue < 0)
					ctx.ctx.Stop(source->pos, "ERROR: negative power on integer number in exponentiation during constant folding");

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

				return new ExprIntegerLiteral(source, lhs->type, result);
			case SYN_BINARY_OP_SHL:
				if(rhsValue < 0)
					ctx.ctx.Stop(source->pos, "ERROR: negative shift value");

				return new ExprIntegerLiteral(source, lhs->type, lhsValue << rhsValue);
			case SYN_BINARY_OP_SHR:
				if(rhsValue < 0)
					ctx.ctx.Stop(source->pos, "ERROR: negative shift value");

				return new ExprIntegerLiteral(source, lhs->type, lhsValue >> rhsValue);
			case SYN_BINARY_OP_LESS:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue < rhsValue);
			case SYN_BINARY_OP_LESS_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue <= rhsValue);
			case SYN_BINARY_OP_GREATER:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue > rhsValue);
			case SYN_BINARY_OP_GREATER_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue >= rhsValue);
			case SYN_BINARY_OP_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue == rhsValue);
			case SYN_BINARY_OP_NOT_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue != rhsValue);
			case SYN_BINARY_OP_BIT_AND:
				return new ExprIntegerLiteral(source, lhs->type, lhsValue & rhsValue);
			case SYN_BINARY_OP_BIT_OR:
				return new ExprIntegerLiteral(source, lhs->type, lhsValue | rhsValue);
			case SYN_BINARY_OP_BIT_XOR:
				return new ExprIntegerLiteral(source, lhs->type, lhsValue ^ rhsValue);
			case SYN_BINARY_OP_LOGICAL_AND:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue && rhsValue);
			case SYN_BINARY_OP_LOGICAL_OR:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue || rhsValue);
			case SYN_BINARY_OP_LOGICAL_XOR:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, !!lhsValue != !!rhsValue);
			}
		}
	}
	if(ctx.ctx.IsFloatingPointType(lhs->type) && ctx.ctx.IsFloatingPointType(rhs->type))
	{
		double lhsValue = 0;
		double rhsValue = 0;

		if(TryTakeDouble(lhs, lhsValue) && TryTakeDouble(rhs, rhsValue))
		{
			switch(op)
			{
			case SYN_BINARY_OP_ADD:
				return new ExprRationalLiteral(source, lhs->type, lhsValue + rhsValue);
			case SYN_BINARY_OP_SUB:
				return new ExprRationalLiteral(source, lhs->type, lhsValue - rhsValue);
			case SYN_BINARY_OP_MUL:
				return new ExprRationalLiteral(source, lhs->type, lhsValue * rhsValue);
			case SYN_BINARY_OP_DIV:
				return new ExprRationalLiteral(source, lhs->type, lhsValue / rhsValue);
			case SYN_BINARY_OP_MOD:
				return new ExprRationalLiteral(source, lhs->type, fmod(lhsValue, rhsValue));
			case SYN_BINARY_OP_POW:
				return new ExprRationalLiteral(source, lhs->type, pow(lhsValue, rhsValue));
			case SYN_BINARY_OP_LESS:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue < rhsValue);
			case SYN_BINARY_OP_LESS_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue <= rhsValue);
			case SYN_BINARY_OP_GREATER:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue > rhsValue);
			case SYN_BINARY_OP_GREATER_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue >= rhsValue);
			case SYN_BINARY_OP_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue == rhsValue);
			case SYN_BINARY_OP_NOT_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue != rhsValue);
			}
		}
	}
	else if(lhs->type == ctx.ctx.typeTypeID && rhs->type == ctx.ctx.typeTypeID)
	{
		TypeBase *lhsValue = NULL;
		TypeBase *rhsValue = NULL;

		if(TryTakeTypeId(lhs, lhsValue) && TryTakeTypeId(rhs, rhsValue))
		{
			if(op == SYN_BINARY_OP_EQUAL)
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue == rhsValue);

			if(op == SYN_BINARY_OP_NOT_EQUAL)
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue != rhsValue);
		}
	}
	else if(isType<TypeRef>(lhs->type) && isType<TypeRef>(rhs->type))
	{
		assert(lhs->type == rhs->type);

		void *lPtr = NULL;

		if(ExprNullptrLiteral *value = getType<ExprNullptrLiteral>(lhs))
			lPtr = NULL;
		else if(ExprPointerLiteral *value = getType<ExprPointerLiteral>(lhs))
			lPtr = value->ptr;
		else
			assert(!"unknown type");

		void *rPtr = NULL;

		if(ExprNullptrLiteral *value = getType<ExprNullptrLiteral>(rhs))
			rPtr = NULL;
		else if(ExprPointerLiteral *value = getType<ExprPointerLiteral>(rhs))
			rPtr = value->ptr;
		else
			assert(!"unknown type");

		if(op == SYN_BINARY_OP_EQUAL)
			return new ExprBoolLiteral(source, ctx.ctx.typeBool, lPtr == rPtr);

		if(op == SYN_BINARY_OP_NOT_EQUAL)
			return new ExprBoolLiteral(source, ctx.ctx.typeBool, lPtr != rPtr);
	}

	return Report(ctx, "ERROR: failed to eval binary op");
}

ExprBase* CheckType(ExprBase* expression, ExprBase *value)
{
	assert(expression->type == value->type);

	return value;
}

ExprBase* EvaluateArray(Eval &ctx, ExprArray *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, expression->type);

	if(!storage)
		return NULL;

	TypeArray *arrayType = getType<TypeArray>(expression->type);

	assert(arrayType);

	unsigned offset = 0;

	for(ExprBase *value = expression->values.head; value; value = value->next)
	{
		ExprBase *element = Evaluate(ctx, value);

		if(!element)
			return NULL;

		assert(storage->ptr + offset + arrayType->subType->size <= storage->end);

		unsigned char *targetPtr = storage->ptr + offset;

		ExprPointerLiteral *target = new ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(arrayType->subType), targetPtr, targetPtr + arrayType->subType->size);

		CreateStore(ctx, target, element);

		offset += unsigned(arrayType->subType->size);
	}

	ExprBase *load = CreateLoad(ctx, storage);

	if(!load)
		return NULL;

	return CheckType(expression, load);
}

ExprBase* EvaluatePreModify(Eval &ctx, ExprPreModify *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *ptr = Evaluate(ctx, expression->value);

	if(!ptr)
		return NULL;

	ExprBase *value = CreateLoad(ctx, ptr);

	if(!value)
		return NULL;

	ExprBase *modified = CreateBinaryOp(ctx, expression->source, value, new ExprIntegerLiteral(expression->source, value->type, 1), expression->isIncrement ? SYN_BINARY_OP_ADD : SYN_BINARY_OP_SUB);

	if(!modified)
		return NULL;

	if(!CreateStore(ctx, ptr, modified))
		return NULL;

	return CheckType(expression, modified);
}

ExprBase* EvaluatePostModify(Eval &ctx, ExprPostModify *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *ptr = Evaluate(ctx, expression->value);

	if(!ptr)
		return NULL;

	ExprBase *value = CreateLoad(ctx, ptr);

	if(!value)
		return NULL;

	ExprBase *modified = CreateBinaryOp(ctx, expression->source, value, new ExprIntegerLiteral(expression->source, value->type, 1), expression->isIncrement ? SYN_BINARY_OP_ADD : SYN_BINARY_OP_SUB);

	if(!modified)
		return NULL;

	if(!CreateStore(ctx, ptr, modified))
		return NULL;

	return CheckType(expression, value);
}

ExprBase* EvaluateCast(Eval &ctx, ExprTypeCast *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	switch(expression->category)
	{
	case EXPR_CAST_NUMERICAL:
		if(ctx.ctx.IsIntegerType(expression->type))
		{
			long long result = 0;

			if(TryTakeLong(value, result))
			{
				if(expression->type == ctx.ctx.typeBool)
					return CheckType(expression, new ExprBoolLiteral(expression->source, ctx.ctx.typeBool, result != 0));

				if(expression->type == ctx.ctx.typeChar)
					return CheckType(expression, new ExprCharacterLiteral(expression->source, ctx.ctx.typeChar, (char)result));

				if(expression->type == ctx.ctx.typeShort)
					return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeShort, (short)result));

				if(expression->type == ctx.ctx.typeInt)
					return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, (int)result));

				if(expression->type == ctx.ctx.typeLong)
					return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeLong, result));
			}
		}
		else if(ctx.ctx.IsFloatingPointType(expression->type))
		{
			double result = 0.0;

			if(TryTakeDouble(value, result))
			{
				if(expression->type == ctx.ctx.typeFloat)
					return CheckType(expression, new ExprRationalLiteral(expression->source, ctx.ctx.typeFloat, (float)result));

				if(expression->type == ctx.ctx.typeDouble)
					return CheckType(expression, new ExprRationalLiteral(expression->source, ctx.ctx.typeDouble, result));
			}
		}
		break;
	case EXPR_CAST_PTR_TO_BOOL:
		return CheckType(expression, new ExprBoolLiteral(expression->source, ctx.ctx.typeBool, !isType<ExprNullptrLiteral>(value)));
	case EXPR_CAST_UNSIZED_TO_BOOL:
		{
			ExprMemoryLiteral *memLiteral = getType<ExprMemoryLiteral>(value);

			ExprBase *ptr = CreateExtract(ctx, memLiteral, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

			return CheckType(expression, new ExprBoolLiteral(expression->source, ctx.ctx.typeBool, !isType<ExprNullptrLiteral>(ptr)));
		}
		break;
	case EXPR_CAST_FUNCTION_TO_BOOL:
		{
			ExprFunctionLiteral *funcLiteral = getType<ExprFunctionLiteral>(value);

			return CheckType(expression, new ExprBoolLiteral(expression->source, ctx.ctx.typeBool, funcLiteral->data != NULL));
		}
		break;
	case EXPR_CAST_NULL_TO_PTR:
		return CheckType(expression, new ExprNullptrLiteral(expression->source, expression->type));
	case EXPR_CAST_NULL_TO_AUTO_PTR:
		{
			ExprBase *typeId = new ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, ctx.ctx.typeVoid);
			ExprBase *ptr = new ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

			ExprBase *result = CreateConstruct(ctx, expression->type, typeId, ptr, NULL);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_NULL_TO_UNSIZED:
		{
			ExprBase *ptr = new ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));
			ExprBase *size = new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, 0);

			ExprBase *result = CreateConstruct(ctx, expression->type, ptr, size, NULL);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_NULL_TO_AUTO_ARRAY:
		{
			ExprBase *typeId = new ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, ctx.ctx.typeVoid);
			ExprBase *ptr = new ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));
			ExprBase *length = new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, 0);

			ExprBase *result = CreateConstruct(ctx, expression->type, typeId, ptr, length);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_NULL_TO_FUNCTION:
		{
			ExprBase *context = new ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

			ExprFunctionLiteral *result = new ExprFunctionLiteral(expression->source, expression->type, NULL, context);

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_ARRAY_TO_UNSIZED:
		{
			TypeArray *arrType = getType<TypeArray>(value->type);

			assert(arrType);
			assert(unsigned(arrType->length) == arrType->length);

			ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, value->type);

			if(!storage)
				return NULL;

			if(!CreateStore(ctx, storage, value))
				return NULL;

			ExprBase *result = CreateConstruct(ctx, expression->type, storage, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, arrType->length), NULL);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
		{
			TypeRef *refType = getType<TypeRef>(value->type);

			assert(refType);

			TypeArray *arrType = getType<TypeArray>(refType->subType);

			assert(arrType);
			assert(unsigned(arrType->length) == arrType->length);

			ExprBase *result = CreateConstruct(ctx, expression->type, value, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, arrType->length), NULL);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_ARRAY_PTR_TO_UNSIZED_PTR:
		{
			TypeRef *refType = getType<TypeRef>(value->type);

			assert(refType);

			TypeArray *arrType = getType<TypeArray>(refType->subType);

			assert(arrType);
			assert(unsigned(arrType->length) == arrType->length);

			ExprBase *result = CreateConstruct(ctx, ctx.ctx.GetUnsizedArrayType(arrType->subType), value, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, arrType->length), NULL);

			if(!result)
				return NULL;

			ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, ctx.ctx.GetUnsizedArrayType(arrType->subType));

			if(!storage)
				return NULL;

			if(!CreateStore(ctx, storage, result))
				return NULL;

			return CheckType(expression, storage);
		}
		break;
	case EXPR_CAST_PTR_TO_AUTO_PTR:
		{
			TypeRef *refType = getType<TypeRef>(value->type);

			assert(refType);

			ExprBase *typeId = new ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, refType->subType);

			ExprBase *result = CreateConstruct(ctx, expression->type, typeId, value, NULL);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_ANY_TO_PTR:
		{
			ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, value->type);

			if(!storage)
				return NULL;

			if(!CreateStore(ctx, storage, value))
				return NULL;

			return CheckType(expression, storage);
		}
		break;
	case EXPR_CAST_AUTO_PTR_TO_PTR:
		{
			TypeRef *refType = getType<TypeRef>(expression->type);

			assert(refType);

			ExprMemoryLiteral *memLiteral = getType<ExprMemoryLiteral>(value);

			ExprTypeLiteral *typeId = getType<ExprTypeLiteral>(CreateExtract(ctx, memLiteral, 0, ctx.ctx.typeTypeID));

			if(typeId->value != refType->subType)
				return Report(ctx, "ERROR: failed to cast '%.*s' to '%.*s'", FMT_ISTR(value->type->name), FMT_ISTR(expression->type->name));

			ExprBase *ptr = CreateExtract(ctx, memLiteral, 4, refType);

			if(!ptr)
				return NULL;

			return CheckType(expression, ptr);
		}
		break;
	case EXPR_CAST_UNSIZED_TO_AUTO_ARRAY:
		{
			TypeUnsizedArray *arrType = getType<TypeUnsizedArray>(value->type);

			assert(arrType);

			ExprMemoryLiteral *memLiteral = getType<ExprMemoryLiteral>(value);

			ExprBase *typeId = new ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, arrType->subType);
			ExprBase *ptr = CreateExtract(ctx, memLiteral, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));
			ExprBase *length = CreateExtract(ctx, memLiteral, sizeof(void*), ctx.ctx.typeInt);

			ExprBase *result = CreateConstruct(ctx, expression->type, typeId, ptr, length);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_REINTERPRET:
		if(expression->type == ctx.ctx.typeInt && value->type == ctx.ctx.typeTypeID)
		{
			ExprTypeLiteral *typeLiteral = getType<ExprTypeLiteral>(value);

			unsigned index = GetTypeIndex(ctx, typeLiteral->value);

			return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, index));
		}
		else if(isType<TypeRef>(expression->type) && isType<TypeRef>(value->type))
		{
			TypeRef *refType = getType<TypeRef>(expression->type);

			if(ExprNullptrLiteral *tmp = getType<ExprNullptrLiteral>(value))
				return CheckType(expression, new ExprNullptrLiteral(expression->source, expression->type));
			
			if(ExprPointerLiteral *tmp = getType<ExprPointerLiteral>(value))
			{
				assert(uintptr_t(tmp->end - tmp->ptr) >= refType->subType->size);

				return CheckType(expression, new ExprPointerLiteral(expression->source, expression->type, tmp->ptr, tmp->end));
			}
		}
		else if(isType<TypeUnsizedArray>(expression->type) && isType<TypeUnsizedArray>(value->type))
		{
			ExprMemoryLiteral *memLiteral = getType<ExprMemoryLiteral>(value);

			return CheckType(expression, new ExprMemoryLiteral(expression->source, expression->type, memLiteral->ptr));
		}
		else if(isType<TypeFunction>(expression->type) && isType<TypeFunction>(value->type))
		{
			ExprFunctionLiteral *funcLiteral = getType<ExprFunctionLiteral>(value);

			return CheckType(expression, new ExprFunctionLiteral(expression->source, expression->type, funcLiteral->data, funcLiteral->context));
		}
		break;
	}

	return Report(ctx, "ERROR: failed to cast '%.*s' to '%.*s'", FMT_ISTR(value->type->name), FMT_ISTR(expression->type->name));
}

ExprBase* EvaluateUnaryOp(Eval &ctx, ExprUnaryOp *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	if(value->type == ctx.ctx.typeBool)
	{
		if(ExprBoolLiteral *expr = getType<ExprBoolLiteral>(value))
		{
			if(expression->op == SYN_UNARY_OP_LOGICAL_NOT)
				return CheckType(expression, new ExprBoolLiteral(expression->source, expression->type, !expr->value));
		}
	}
	else if(ctx.ctx.IsIntegerType(value->type))
	{
		long long result = 0;

		if(TryTakeLong(value, result))
		{
			switch(expression->op)
			{
			case SYN_UNARY_OP_PLUS:
				return CheckType(expression, new ExprIntegerLiteral(expression->source, expression->type, result));
			case SYN_UNARY_OP_NEGATE:
				return CheckType(expression, new ExprIntegerLiteral(expression->source, expression->type, -result));
			case SYN_UNARY_OP_BIT_NOT:
				return CheckType(expression, new ExprIntegerLiteral(expression->source, expression->type, ~result));
			case SYN_UNARY_OP_LOGICAL_NOT:
				return CheckType(expression, new ExprIntegerLiteral(expression->source, expression->type, !result));
			}
		}
	}
	else if(ctx.ctx.IsFloatingPointType(value->type))
	{
		double result = 0.0;

		if(TryTakeDouble(value, result))
		{
			switch(expression->op)
			{
			case SYN_UNARY_OP_PLUS:
				return CheckType(expression, new ExprRationalLiteral(expression->source, expression->type, result));
			case SYN_UNARY_OP_NEGATE:
				return CheckType(expression, new ExprRationalLiteral(expression->source, expression->type, -result));
			}
		}
	}
	else if(isType<TypeRef>(value->type))
	{
		void *lPtr = NULL;

		if(ExprNullptrLiteral *tmp = getType<ExprNullptrLiteral>(value))
			lPtr = NULL;
		else if(ExprPointerLiteral *tmp = getType<ExprPointerLiteral>(value))
			lPtr = tmp->ptr;
		else
			assert(!"unknown type");

		if(expression->op == SYN_UNARY_OP_LOGICAL_NOT)
			return CheckType(expression, new ExprBoolLiteral(expression->source, ctx.ctx.typeBool, !lPtr));
	}
	else if(value->type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral *memLiteral = getType<ExprMemoryLiteral>(value);

		void *lPtr = 0;
		if(!TryTakePointer(CreateExtract(ctx, memLiteral, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), lPtr))
			return Report(ctx, "ERROR: failed to evaluate auto ref value");

		return CheckType(expression, new ExprBoolLiteral(expression->source, ctx.ctx.typeBool, !lPtr));

	}

	return Report(ctx, "ERROR: failed to eval unary op");
}

ExprBase* EvaluateBinaryOp(Eval &ctx, ExprBinaryOp *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *lhs = Evaluate(ctx, expression->lhs);
	ExprBase *rhs = Evaluate(ctx, expression->rhs);

	if(!lhs || !rhs)
		return NULL;

	ExprBase *result = CreateBinaryOp(ctx, expression->source, lhs, rhs, expression->op);

	if(!result)
		return result;

	return CheckType(expression, result);
}

ExprBase* EvaluateGetAddress(Eval &ctx, ExprGetAddress *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprPointerLiteral *ptr = FindVariableStorage(ctx, expression->variable);

	if(!ptr)
		return NULL;

	return CheckType(expression, ptr);
}

ExprBase* EvaluateDereference(Eval &ctx, ExprDereference *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *ptr = Evaluate(ctx, expression->value);

	if(!ptr)
		return NULL;

	ExprBase *value = CreateLoad(ctx, ptr);

	if(!value)
		return NULL;

	return CheckType(expression, value);
}

ExprBase* EvaluateConditional(Eval &ctx, ExprConditional *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *condition = Evaluate(ctx, expression->condition);

	if(!condition)
		return NULL;

	long long result;
	if(!TryTakeLong(condition, result))
		return Report(ctx, "ERROR: failed to evaluate ternary operator condition");

	ExprBase *value = Evaluate(ctx, result ? expression->trueBlock : expression->falseBlock);

	if(!value)
		return NULL;

	return CheckType(expression, value);
}

ExprBase* EvaluateAssignment(Eval &ctx, ExprAssignment *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *lhs = Evaluate(ctx, expression->lhs);
	ExprBase *rhs = Evaluate(ctx, expression->rhs);

	if(!lhs || !rhs)
		return NULL;

	if(!CreateStore(ctx, lhs, rhs))
		return NULL;

	return CheckType(expression, rhs);
}

ExprBase* EvaluateMemberAccess(Eval &ctx, ExprMemberAccess *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	if(isType<ExprNullptrLiteral>(value))
		return Report(ctx, "ERROR: member access of null pointer");

	ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(value);

	assert(ptr);
	assert(ptr->ptr + expression->member->offset + expression->member->type->size <= ptr->end);

	unsigned char *targetPtr = ptr->ptr + expression->member->offset;

	ExprPointerLiteral *shifted = new ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(expression->member->type), targetPtr, targetPtr + expression->member->type->size);

	return CheckType(expression, shifted);
}

ExprBase* EvaluateArrayIndex(Eval &ctx, ExprArrayIndex *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	ExprBase *index = Evaluate(ctx, expression->index);

	if(!index)
		return NULL;

	long long result;
	if(!TryTakeLong(index, result))
		return Report(ctx, "ERROR: failed to evaluate array index");

	if(TypeUnsizedArray *arrayType = getType<TypeUnsizedArray>(value->type))
	{
		ExprMemoryLiteral *memory = getType<ExprMemoryLiteral>(value);

		assert(memory);

		ExprBase *value = CreateExtract(ctx, memory, 0, ctx.ctx.GetReferenceType(arrayType->subType));

		if(!value)
			return NULL;

		if(isType<ExprNullptrLiteral>(value))
			return Report(ctx, "ERROR: array index of a null array");

		ExprIntegerLiteral *size = getType<ExprIntegerLiteral>(CreateExtract(ctx, memory, sizeof(void*), ctx.ctx.typeInt));

		if(!size)
			return NULL;

		if(result < 0 || result >= size->value)
			return Report(ctx, "ERROR: array index out of bounds");

		ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(value);

		assert(ptr);

		unsigned char *targetPtr = ptr->ptr + result * arrayType->subType->size;

		ExprPointerLiteral *shifted = new ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(arrayType->subType), targetPtr, targetPtr + arrayType->subType->size);

		return CheckType(expression, shifted);
	}

	TypeRef *refType = getType<TypeRef>(value->type);

	assert(refType);

	TypeArray *arrayType = getType<TypeArray>(refType->subType);

	assert(arrayType);

	if(isType<ExprNullptrLiteral>(value))
		return Report(ctx, "ERROR: array index of a null array");

	if(result < 0 || result >= arrayType->length)
		return Report(ctx, "ERROR: array index out of bounds");

	ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(value);

	assert(ptr);
	assert(ptr->ptr + result * arrayType->subType->size + arrayType->subType->size <= ptr->end);

	unsigned char *targetPtr = ptr->ptr + result * arrayType->subType->size;

	ExprPointerLiteral *shifted = new ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(arrayType->subType), targetPtr, targetPtr + arrayType->subType->size);

	return CheckType(expression, shifted);
}

ExprBase* EvaluateReturn(Eval &ctx, ExprReturn *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	if(ctx.stackFrames.empty())
		return Report(ctx, "ERROR: no stack frame to return from");

	Eval::StackFrame *frame = ctx.stackFrames.back();

	frame->returnValue = value;

	return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateVariableDefinition(Eval &ctx, ExprVariableDefinition *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	Eval::StackFrame *frame = ctx.stackFrames.back();

	TypeBase *type = expression->variable->type;

	ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, type);

	if(!storage)
		return NULL;

	frame->variables.push_back(Eval::StackVariable(expression->variable, storage));

	if(expression->initializer)
	{
		if(!Evaluate(ctx, expression->initializer))
			return NULL;
	}

	return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateArraySetup(Eval &ctx, ExprArraySetup *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	TypeArray *arrayType = getType<TypeArray>(expression->variable->type);

	assert(arrayType);

	ExprBase *initializer = Evaluate(ctx, expression->initializer);

	if(!initializer)
		return NULL;

	ExprPointerLiteral *ptr = FindVariableStorage(ctx, expression->variable);

	if(!ptr)
		return NULL;

	for(unsigned i = 0; i < unsigned(arrayType->length); i++)
	{
		if(!AddInstruction(ctx))
			return NULL;

		assert(ptr);
		assert(ptr->ptr + i * arrayType->subType->size + arrayType->subType->size <= ptr->end);

		unsigned char *targetPtr = ptr->ptr + i * arrayType->subType->size;

		ExprPointerLiteral *shifted = new ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(arrayType->subType), targetPtr, targetPtr + arrayType->subType->size);

		if(!CreateStore(ctx, shifted, initializer))
			return NULL;
	}

	return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateVariableDefinitions(Eval &ctx, ExprVariableDefinitions *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	for(ExprVariableDefinition *definition = expression->definitions.head; definition; definition = getType<ExprVariableDefinition>(definition->next))
	{
		if(!Evaluate(ctx, definition))
			return NULL;
	}

	return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateVariableAccess(Eval &ctx, ExprVariableAccess *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprPointerLiteral *ptr = FindVariableStorage(ctx, expression->variable);

	if(!ptr)
		return NULL;

	ExprBase *value = CreateLoad(ctx, ptr);

	if(!value)
		return NULL;

	return CheckType(expression, value);
}

ExprBase* EvaluateFunctionDefinition(Eval &ctx, ExprFunctionDefinition *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *context = new ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

	return CheckType(expression, new ExprFunctionLiteral(expression->source, expression->function->type, expression->function, context));
}

ExprBase* EvaluateFunction(Eval &ctx, ExprFunctionDefinition *expression, ExprBase *context, SmallArray<ExprBase*, 32> &arguments)
{
	if(!AddInstruction(ctx))
		return NULL;

	if(ctx.stackFrames.size() >= ctx.stackDepthLimit)
		return Report(ctx, "ERROR: stack depth limit");

	ctx.stackFrames.push_back(new Eval::StackFrame());

	Eval::StackFrame *frame = ctx.stackFrames.back();

	if(ExprVariableDefinition *curr = expression->context)
	{
		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, curr->source, curr->variable->type);

		if(!storage)
			return NULL;

		frame->variables.push_back(Eval::StackVariable(curr->variable, storage));

		if(!CreateStore(ctx, storage, context))
			return NULL;
	}

	unsigned pos = 0;

	for(ExprVariableDefinition *curr = expression->arguments.head; curr; curr = getType<ExprVariableDefinition>(curr->next))
	{
		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, curr->source, curr->variable->type);

		if(!storage)
			return NULL;

		frame->variables.push_back(Eval::StackVariable(curr->variable, storage));

		if(!CreateStore(ctx, storage, arguments[pos]))
			return NULL;

		pos++;
	}

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		if(!Evaluate(ctx, value))
			return NULL;

		if(ctx.stackFrames.back()->returnValue)
			break;
	}

	ExprBase *result = ctx.stackFrames.back()->returnValue;

	if(!result && expression->function->type->returnType == ctx.ctx.typeVoid)
		result = new ExprVoid(expression->source, ctx.ctx.typeVoid);

	ctx.stackFrames.pop_back();

	return result;
}

ExprBase* EvaluateFunctionAccess(Eval &ctx, ExprFunctionAccess *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *context = Evaluate(ctx, expression->context);

	if(!context)
		return NULL;

	return CheckType(expression, new ExprFunctionLiteral(expression->source, expression->function->type, expression->function, context));
}

ExprBase* EvaluateFunctionCall(Eval &ctx, ExprFunctionCall *expression)
{
	if(!AddInstruction(ctx))
		return NULL;
	
	ExprBase *function = Evaluate(ctx, expression->function);

	if(!function)
		return NULL;

	SmallArray<ExprBase*, 32> arguments;

	for(ExprBase *curr = expression->arguments.head; curr; curr = curr->next)
	{
		ExprBase *value = Evaluate(ctx, curr);

		if(!value)
			return NULL;

		arguments.push_back(value);
	}

	ExprFunctionLiteral *ptr = getType<ExprFunctionLiteral>(function);

	if(!ptr->data)
		return Report(ctx, "ERROR: null function pointer call");

	if(!ptr->data->declaration)
	{
		if(ctx.emulateKnownExternals && GetFunctionIndex(ctx, ptr->data) < ctx.ctx.baseModuleFunctionCount)
		{
			if(ptr->data->name == InplaceStr("assert") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeInt)
			{
				long long value;
				if(!TryTakeLong(arguments[0], value))
					return Report(ctx, "ERROR: failed to evaluate value");

				if(value == 0)
					return Report(ctx, "ERROR: Assertion failed");

				return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
			}
			else if(ptr->data->name == InplaceStr("assert") && arguments.size() == 2 && arguments[0]->type == ctx.ctx.typeInt && arguments[1]->type == ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeChar))
			{
				long long value;
				if(!TryTakeLong(arguments[0], value))
					return Report(ctx, "ERROR: failed to evaluate value");

				ExprMemoryLiteral *memory = getType<ExprMemoryLiteral>(arguments[1]);

				ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(CreateExtract(ctx, memory, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeChar)));
				ExprIntegerLiteral *length = getType<ExprIntegerLiteral>(CreateExtract(ctx, memory, sizeof(void*), ctx.ctx.typeInt));

				if(!ptr)
					return Report(ctx, "ERROR: null pointer access");

				assert(length);

				if(value == 0)
					return Report(ctx, "ERROR: %.*s", length->value, ptr->ptr);

				return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
			}
			else if(ptr->data->name == InplaceStr("bool") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeBool)
			{
				long long value;
				if(!TryTakeLong(arguments[0], value))
					return Report(ctx, "ERROR: failed to evaluate value");

				return CheckType(expression, new ExprBoolLiteral(expression->source, ctx.ctx.typeBool, value != 0));
			}
			else if(ptr->data->name == InplaceStr("char") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeChar)
			{
				long long value;
				if(!TryTakeLong(arguments[0], value))
					return Report(ctx, "ERROR: failed to evaluate value");

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeChar, char(value)));
			}
			else if(ptr->data->name == InplaceStr("short") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeShort)
			{
				long long value;
				if(!TryTakeLong(arguments[0], value))
					return Report(ctx, "ERROR: failed to evaluate value");

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeShort, short(value)));
			}
			else if(ptr->data->name == InplaceStr("int") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeInt)
			{
				long long value;
				if(!TryTakeLong(arguments[0], value))
					return Report(ctx, "ERROR: failed to evaluate value");

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, int(value)));
			}
			else if(ptr->data->name == InplaceStr("long") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeLong)
			{
				long long value;
				if(!TryTakeLong(arguments[0], value))
					return Report(ctx, "ERROR: failed to evaluate value");

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeLong, value));
			}
			else if(ptr->data->name == InplaceStr("float") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeFloat)
			{
				double value;
				if(!TryTakeDouble(arguments[0], value))
					return Report(ctx, "ERROR: failed to evaluate value");

				return CheckType(expression, new ExprRationalLiteral(expression->source, ctx.ctx.typeFloat, value));
			}
			else if(ptr->data->name == InplaceStr("double") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeDouble)
			{
				double value;
				if(!TryTakeDouble(arguments[0], value))
					return Report(ctx, "ERROR: failed to evaluate value");

				return CheckType(expression, new ExprRationalLiteral(expression->source, ctx.ctx.typeDouble, value));
			}
			else if(ptr->data->name == InplaceStr("bool::bool") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeBool)
			{
				if(!CreateStore(ctx, ptr->context, arguments[0]))
					return NULL;

				return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
			}
			else if(ptr->data->name == InplaceStr("char::char") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeChar)
			{
				if(!CreateStore(ctx, ptr->context, arguments[0]))
					return NULL;

				return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
			}
			else if(ptr->data->name == InplaceStr("short::short") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeShort)
			{
				if(!CreateStore(ctx, ptr->context, arguments[0]))
					return NULL;

				return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
			}
			else if(ptr->data->name == InplaceStr("int::int") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeInt)
			{
				if(!CreateStore(ctx, ptr->context, arguments[0]))
					return NULL;

				return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
			}
			else if(ptr->data->name == InplaceStr("long::long") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeLong)
			{
				if(!CreateStore(ctx, ptr->context, arguments[0]))
					return NULL;

				return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
			}
			else if(ptr->data->name == InplaceStr("float::float") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeFloat)
			{
				if(!CreateStore(ctx, ptr->context, arguments[0]))
					return NULL;

				return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
			}
			else if(ptr->data->name == InplaceStr("double::double") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeDouble)
			{
				if(!CreateStore(ctx, ptr->context, arguments[0]))
					return NULL;

				return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
			}
			else if(ptr->data->name == InplaceStr("__newS"))
			{
				long long size;
				if(!TryTakeLong(arguments[0], size))
					return Report(ctx, "ERROR: failed to evaluate type size");

				long long type;
				if(!TryTakeLong(arguments[1], type))
					return Report(ctx, "ERROR: failed to evaluate type ID");

				TypeBase *target = ctx.ctx.types[unsigned(type)];

				assert(target->size == size);

				ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, target);

				if(!storage)
					return NULL;

				return CheckType(expression, new ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid), storage->ptr, storage->end));
			}
			else if(ptr->data->name == InplaceStr("__newA"))
			{
				long long size;
				if(!TryTakeLong(arguments[0], size))
					return Report(ctx, "ERROR: failed to evaluate type size");

				long long count;
				if(!TryTakeLong(arguments[1], count))
					return Report(ctx, "ERROR: failed to evaluate element count");

				long long type;
				if(!TryTakeLong(arguments[2], type))
					return Report(ctx, "ERROR: failed to evaluate type ID");

				TypeBase *target = ctx.ctx.types[unsigned(type)];

				assert(target->size == size);

				if(target->size * count > ctx.variableMemoryLimit)
					return Report(ctx, "ERROR: single variable memory limit");

				ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, ctx.ctx.GetArrayType(target, count));

				if(!storage)
					return NULL;

				ExprBase *result = CreateConstruct(ctx, ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeInt), storage, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, count), NULL);

				if(!result)
					return NULL;

				return CheckType(expression, result);
			}
			else if(ptr->data->name == InplaceStr("__rcomp"))
			{
				ExprMemoryLiteral *a = getType<ExprMemoryLiteral>(arguments[0]);
				ExprMemoryLiteral *b = getType<ExprMemoryLiteral>(arguments[1]);

				assert(a && b);

				void *lPtr = 0;
				if(!TryTakePointer(CreateExtract(ctx, a, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), lPtr))
					return Report(ctx, "ERROR: failed to evaluate first argument");

				void *rPtr = 0;
				if(!TryTakePointer(CreateExtract(ctx, b, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), rPtr))
					return Report(ctx, "ERROR: failed to evaluate second argument");

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, lPtr == rPtr));
			}
			else if(ptr->data->name == InplaceStr("__rncomp"))
			{
				ExprMemoryLiteral *a = getType<ExprMemoryLiteral>(arguments[0]);
				ExprMemoryLiteral *b = getType<ExprMemoryLiteral>(arguments[1]);

				assert(a && b);

				void *lPtr = 0;
				if(!TryTakePointer(CreateExtract(ctx, a, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), lPtr))
					return Report(ctx, "ERROR: failed to evaluate first argument");

				void *rPtr = 0;
				if(!TryTakePointer(CreateExtract(ctx, b, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), rPtr))
					return Report(ctx, "ERROR: failed to evaluate second argument");

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, lPtr != rPtr));
			}
			else if(ptr->data->name == InplaceStr("__pcomp"))
			{
				ExprFunctionLiteral *a = getType<ExprFunctionLiteral>(arguments[0]);
				ExprFunctionLiteral *b = getType<ExprFunctionLiteral>(arguments[1]);

				assert(a && b);

				void *aContext = 0;
				if(a->context && !TryTakePointer(a->context, aContext))
					return Report(ctx, "ERROR: failed to evaluate first argument");

				void *bContext = 0;
				if(b->context && !TryTakePointer(b->context, bContext))
					return Report(ctx, "ERROR: failed to evaluate second argument");

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, a->data == b->data && aContext == bContext));
			}
			else if(ptr->data->name == InplaceStr("__pncomp"))
			{
				ExprFunctionLiteral *a = getType<ExprFunctionLiteral>(arguments[0]);
				ExprFunctionLiteral *b = getType<ExprFunctionLiteral>(arguments[1]);

				assert(a && b);

				void *aContext = 0;
				if(a->context && !TryTakePointer(a->context, aContext))
					return Report(ctx, "ERROR: failed to evaluate first argument");

				void *bContext = 0;
				if(b->context && !TryTakePointer(b->context, bContext))
					return Report(ctx, "ERROR: failed to evaluate second argument");

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, a->data != b->data || aContext != bContext));
			}
			else if(ptr->data->name == InplaceStr("__pcomp") || ptr->data->name == InplaceStr("__acomp"))
			{
				ExprMemoryLiteral *a = getType<ExprMemoryLiteral>(arguments[0]);
				ExprMemoryLiteral *b = getType<ExprMemoryLiteral>(arguments[1]);

				assert(a && b);
				assert(a->type->size == b->type->size);

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, memcmp(a->ptr->ptr, b->ptr->ptr, unsigned(a->type->size)) == 0));
			}
			else if(ptr->data->name == InplaceStr("__pncomp") || ptr->data->name == InplaceStr("__ancomp"))
			{
				ExprMemoryLiteral *a = getType<ExprMemoryLiteral>(arguments[0]);
				ExprMemoryLiteral *b = getType<ExprMemoryLiteral>(arguments[1]);

				assert(a && b);
				assert(a->type->size == b->type->size);

				return CheckType(expression, new ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, memcmp(a->ptr->ptr, b->ptr->ptr, unsigned(a->type->size)) != 0));
			}
		}

		return Report(ctx, "ERROR: function '%.*s' has no source", FMT_ISTR(ptr->data->name));
	}

	ExprFunctionDefinition *declaration = getType<ExprFunctionDefinition>(ptr->data->declaration);

	assert(declaration);

	ExprBase *call = EvaluateFunction(ctx, declaration, ptr->context, arguments);

	if(!call)
		return NULL;

	return CheckType(expression, call);
}

ExprBase* EvaluateIfElse(Eval &ctx, ExprIfElse *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *condition = Evaluate(ctx, expression->condition);

	if(!condition)
		return NULL;

	long long result;
	if(!TryTakeLong(condition, result))
		return Report(ctx, "ERROR: failed to evaluate 'if' condition");

	if(result)
	{
		if(!Evaluate(ctx, expression->trueBlock))
			return NULL;
	}
	else if(expression->falseBlock)
	{
		if(!Evaluate(ctx, expression->falseBlock))
			return NULL;
	}

	return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateFor(Eval &ctx, ExprFor *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	if(!Evaluate(ctx, expression->initializer))
		return NULL;

	for(;;)
	{
		if(!AddInstruction(ctx))
			return NULL;

		ExprBase *condition = Evaluate(ctx, expression->condition);

		if(!condition)
			return NULL;

		long long result;
		if(!TryTakeLong(condition, result))
			return Report(ctx, "ERROR: failed to evaluate 'for' condition");

		if(!result)
			break;

		if(!Evaluate(ctx, expression->body))
			return NULL;

		if(ctx.stackFrames.back()->returnValue)
			break;

		if(!Evaluate(ctx, expression->increment))
			return NULL;
	}

	return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateWhile(Eval &ctx, ExprWhile *expression)
{
	for(;;)
	{
		if(!AddInstruction(ctx))
			return NULL;

		ExprBase *condition = Evaluate(ctx, expression->condition);

		if(!condition)
			return NULL;

		long long result;
		if(!TryTakeLong(condition, result))
			return Report(ctx, "ERROR: failed to evaluate 'while' condition");

		if(!result)
			break;

		if(!Evaluate(ctx, expression->body))
			return NULL;

		if(ctx.stackFrames.back()->returnValue)
			break;
	}

	return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateDoWhile(Eval &ctx, ExprDoWhile *expression)
{
	for(;;)
	{
		if(!AddInstruction(ctx))
			return NULL;

		if(!Evaluate(ctx, expression->body))
			return NULL;

		if(ctx.stackFrames.back()->returnValue)
			break;

		ExprBase *condition = Evaluate(ctx, expression->condition);

		if(!condition)
			return NULL;

		long long result;
		if(!TryTakeLong(condition, result))
			return Report(ctx, "ERROR: failed to evaluate 'do' condition");

		if(!result)
			break;
	}

	return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateBlock(Eval &ctx, ExprBlock *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		if(!Evaluate(ctx, value))
			return NULL;

		if(ctx.stackFrames.back()->returnValue)
			break;
	}

	return CheckType(expression, new ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateSequence(Eval &ctx, ExprSequence *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *result = new ExprVoid(expression->source, ctx.ctx.typeVoid);

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		result = Evaluate(ctx, value);

		if(!result)
			return NULL;
	}

	return CheckType(expression, result);
}

ExprBase* EvaluateModule(Eval &ctx, ExprModule *expression)
{
	ctx.globalFrame = new Eval::StackFrame();
	ctx.stackFrames.push_back(ctx.globalFrame);

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		if(!Evaluate(ctx, value))
			return NULL;

		if(ctx.stackFrames.back()->returnValue)
			return ctx.stackFrames.back()->returnValue;
	}

	ctx.stackFrames.pop_back();

	assert(ctx.stackFrames.empty());

	return NULL;
}

ExprBase* Evaluate(Eval &ctx, ExprBase *expression)
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

	if(ExprArray *expr = getType<ExprArray>(expression))
		return EvaluateArray(ctx, expr);

	if(ExprPreModify *expr = getType<ExprPreModify>(expression))
		return EvaluatePreModify(ctx, expr);

	if(ExprPostModify *expr = getType<ExprPostModify>(expression))
		return EvaluatePostModify(ctx, expr);

	if(ExprTypeCast *expr = getType<ExprTypeCast>(expression))
		return EvaluateCast(ctx, expr);

	if(ExprUnaryOp *expr = getType<ExprUnaryOp>(expression))
		return EvaluateUnaryOp(ctx, expr);

	if(ExprBinaryOp *expr = getType<ExprBinaryOp>(expression))
		return EvaluateBinaryOp(ctx, expr);

	if(ExprGetAddress *expr = getType<ExprGetAddress>(expression))
		return EvaluateGetAddress(ctx, expr);

	if(ExprDereference *expr = getType<ExprDereference>(expression))
		return EvaluateDereference(ctx, expr);

	if(ExprConditional *expr = getType<ExprConditional>(expression))
		return EvaluateConditional(ctx, expr);

	if(ExprAssignment *expr = getType<ExprAssignment>(expression))
		return EvaluateAssignment(ctx, expr);

	if(ExprMemberAccess *expr = getType<ExprMemberAccess>(expression))
		return EvaluateMemberAccess(ctx, expr);

	if(ExprArrayIndex *expr = getType<ExprArrayIndex>(expression))
		return EvaluateArrayIndex(ctx, expr);

	if(ExprReturn *expr = getType<ExprReturn>(expression))
		return EvaluateReturn(ctx, expr);

	if(ExprYield *expr = getType<ExprYield>(expression))
		return Report(ctx, "ERROR: 'yield' is not supported");

	if(ExprVariableDefinition *expr = getType<ExprVariableDefinition>(expression))
		return EvaluateVariableDefinition(ctx, expr);

	if(ExprArraySetup *expr = getType<ExprArraySetup>(expression))
		return EvaluateArraySetup(ctx, expr);

	if(ExprVariableDefinitions *expr = getType<ExprVariableDefinitions>(expression))
		return EvaluateVariableDefinitions(ctx, expr);

	if(ExprVariableAccess *expr = getType<ExprVariableAccess>(expression))
		return EvaluateVariableAccess(ctx, expr);

	if(ExprFunctionDefinition *expr = getType<ExprFunctionDefinition>(expression))
		return EvaluateFunctionDefinition(ctx, expr);

	if(ExprGenericFunctionPrototype *expr = getType<ExprGenericFunctionPrototype>(expression))
		return new ExprVoid(expr->source, ctx.ctx.typeVoid);

	if(ExprFunctionAccess *expr = getType<ExprFunctionAccess>(expression))
		return EvaluateFunctionAccess(ctx, expr);

	if(ExprFunctionOverloadSet *expr = getType<ExprFunctionOverloadSet>(expression))
		assert(!"miscompiled tree");

	if(ExprFunctionCall *expr = getType<ExprFunctionCall>(expression))
		return EvaluateFunctionCall(ctx, expr);

	if(ExprAliasDefinition *expr = getType<ExprAliasDefinition>(expression))
		return CheckType(expression, new ExprVoid(expr->source, ctx.ctx.typeVoid));

	if(ExprGenericClassPrototype *expr = getType<ExprGenericClassPrototype>(expression))
		return CheckType(expression, new ExprVoid(expr->source, ctx.ctx.typeVoid));

	if(ExprClassDefinition *expr = getType<ExprClassDefinition>(expression))
		return CheckType(expression, new ExprVoid(expr->source, ctx.ctx.typeVoid));

	if(ExprEnumDefinition *expr = getType<ExprEnumDefinition>(expression))
		return CheckType(expression, new ExprVoid(expr->source, ctx.ctx.typeVoid));

	if(ExprIfElse *expr = getType<ExprIfElse>(expression))
		return EvaluateIfElse(ctx, expr);

	if(ExprFor *expr = getType<ExprFor>(expression))
		return EvaluateFor(ctx, expr);

	if(ExprWhile *expr = getType<ExprWhile>(expression))
		return EvaluateWhile(ctx, expr);

	if(ExprDoWhile *expr = getType<ExprDoWhile>(expression))
		return EvaluateDoWhile(ctx, expr);

	if(ExprBlock *expr = getType<ExprBlock>(expression))
		return EvaluateBlock(ctx, expr);

	if(ExprSequence *expr = getType<ExprSequence>(expression))
		return EvaluateSequence(ctx, expr);

	if(ExprModule *expr = getType<ExprModule>(expression))
		return EvaluateModule(ctx, expr);

	return Report(ctx, "ERROR: unknown expression type");
}
