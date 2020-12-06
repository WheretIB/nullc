#include "ExpressionEval.h"

#include <math.h>

#include "ExpressionTree.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

namespace
{
	void* ReadVmMemoryPointer(void* address)
	{
		void *result;
		memcpy(&result, address, sizeof(void*));
		return result;
	}

	void WriteVmMemoryPointer(void* address, void *value)
	{
		memcpy(address, &value, sizeof(void*));
	}
}

NULLC_PRINT_FORMAT_CHECK(2, 3) ExprBase* Report(ExpressionEvalContext &ctx, const char *msg, ...)
{
	if(ctx.errorBuf && ctx.errorBufSize)
	{
		va_list args;
		va_start(args, msg);

		vsnprintf(ctx.errorBuf, ctx.errorBufSize, msg, args);

		va_end(args);

		ctx.errorBuf[ctx.errorBufSize - 1] = '\0';
	}

	ctx.errorCritical = false;

	return NULL;
}

NULLC_PRINT_FORMAT_CHECK(2, 3) ExprBase* ReportCritical(ExpressionEvalContext &ctx, const char *msg, ...)
{
	if(ctx.errorBuf && ctx.errorBufSize)
	{
		va_list args;
		va_start(args, msg);

		vsnprintf(ctx.errorBuf, ctx.errorBufSize, msg, args);

		va_end(args);

		ctx.errorBuf[ctx.errorBufSize - 1] = '\0';
	}

	ctx.errorCritical = true;

	return NULL;
}

bool AddInstruction(ExpressionEvalContext &ctx)
{
	if(ctx.instruction < ctx.instructionsLimit)
	{
		ctx.instruction++;
		return true;
	}

	Report(ctx, "ERROR: instruction limit reached");

	return false;
}

ExprPointerLiteral* AllocateTypeStorage(ExpressionEvalContext &ctx, SynBase *source, TypeBase *type)
{
	if(isType<TypeError>(type))
		return NULL;

	for(unsigned i = 0; i < ctx.abandonedMemory.size(); i++)
	{
		ExprPointerLiteral *ptr = ctx.abandonedMemory[i];

		if(ptr->end - ptr->ptr == type->size)
		{
			ptr->type = ctx.ctx.GetReferenceType(type);

			memset(ptr->ptr, 0, unsigned(type->size));

			ctx.abandonedMemory[i] = ctx.abandonedMemory.back();
			ctx.abandonedMemory.pop_back();

			return ptr;
		}
	}

	if(type->size > ctx.variableMemoryLimit)
		return (ExprPointerLiteral*)Report(ctx, "ERROR: single variable memory limit");

	if(ctx.totalMemory + type->size > ctx.totalMemoryLimit)
		return (ExprPointerLiteral*)Report(ctx, "ERROR: total variable memory limit");

	ctx.totalMemory += unsigned(type->size);

	unsigned char *memory = (unsigned char*)ctx.ctx.allocator->alloc(unsigned(type->size));

	memset(memory, 0, unsigned(type->size));

	return new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(source, ctx.ctx.GetReferenceType(type), memory, memory + type->size);
}

void FreeMemoryLiteral(ExpressionEvalContext &ctx, ExprMemoryLiteral *memory)
{
	ctx.abandonedMemory.push_back(memory->ptr);
}

bool CreateStore(ExpressionEvalContext &ctx, ExprBase *target, ExprBase *value)
{
	// No side-effects while coroutine is skipping to target node
	if(!ctx.stackFrames.empty())
		assert(ctx.stackFrames.back()->targetYield == 0);

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
		memcpy(ptr->ptr, expr->value, unsigned(value->type->size));
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
		if(isType<TypeError>(expr->value))
			return false;

		if(isType<TypeArgumentSet>(expr->value) || isType<TypeMemberSet>(expr->value))
			return false;

		unsigned index = ctx.ctx.GetTypeIndex(expr->value);
		memcpy(ptr->ptr, &index, unsigned(value->type->size));
		return true;
	}

	if(isType<ExprNullptrLiteral>(value))
	{
		memset(ptr->ptr, 0, unsigned(value->type->size));
		return true;
	}

	if(ExprFunctionIndexLiteral *expr = getType<ExprFunctionIndexLiteral>(value))
	{
		unsigned index = expr->function ? ctx.ctx.GetFunctionIndex(expr->function) + 1 : 0;
		memcpy(ptr->ptr, &index, unsigned(value->type->size));
		return true;
	}

	if(ExprFunctionLiteral *expr = getType<ExprFunctionLiteral>(value))
	{
		unsigned index = expr->data ? ctx.ctx.GetFunctionIndex(expr->data) + 1 : 0;
		memcpy(ptr->ptr, &index, sizeof(unsigned));

		if(isType<ExprNullptrLiteral>(expr->context))
			memset(ptr->ptr + 4, 0, sizeof(void*));
		else if(ExprPointerLiteral *context = getType<ExprPointerLiteral>(expr->context))
			memcpy(ptr->ptr + 4, &context->ptr, sizeof(void*));
		else
			return false;

		return true;
	}

	if(ExprPointerLiteral *expr = getType<ExprPointerLiteral>(value))
	{
		TypeRef *ptrType = getType<TypeRef>(expr->type);

		(void)ptrType;
		assert(ptrType);
		assert(expr->ptr + ptrType->subType->size <= expr->end);

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

ExprBase* CreateLoad(ExpressionEvalContext &ctx, ExprBase *target)
{
	// No side-effects while coroutine is skipping to target node
	if(!ctx.stackFrames.empty())
		assert(ctx.stackFrames.back()->targetYield == 0);

	if(isType<ExprNullptrLiteral>(target))
	{
		Report(ctx, "ERROR: load from null pointer");

		return NULL;
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

		return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeChar)
	{
		signed char value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new (ctx.ctx.get<ExprCharacterLiteral>()) ExprCharacterLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeShort)
	{
		short value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeInt)
	{
		int value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeLong)
	{
		long long value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeFloat)
	{
		float value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeDouble)
	{
		double value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(target->source, type, value);
	}

	if(type == ctx.ctx.typeTypeID)
	{
		unsigned index = 0;
		memcpy(&index, ptr->ptr, sizeof(unsigned));

		TypeBase *data = ctx.ctx.types[index];

		return new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(target->source, type, data);
	}

	if(type == ctx.ctx.typeFunctionID)
	{
		unsigned index = 0;
		memcpy(&index, ptr->ptr, sizeof(unsigned));

		FunctionData *data = index != 0 ? ctx.ctx.functions[index - 1] : NULL;

		return new (ctx.ctx.get<ExprFunctionIndexLiteral>()) ExprFunctionIndexLiteral(target->source, type, data);
	}

	if(type == ctx.ctx.typeNullPtr)
	{
		return new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(target->source, type);
	}

	if(isType<TypeFunction>(type))
	{
		unsigned index = 0;
		memcpy(&index, ptr->ptr, sizeof(unsigned));

		FunctionData *data = index != 0 ? ctx.ctx.functions[index - 1] : NULL;

		unsigned char *value = 0;
		memcpy(&value, ptr->ptr + 4, sizeof(value));
		
		if(!data)
		{
			assert(value == NULL);

			return new (ctx.ctx.get<ExprFunctionLiteral>()) ExprFunctionLiteral(target->source, type, NULL, new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(target->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		}

		TypeRef *ptrType = getType<TypeRef>(data->contextType);

		assert(ptrType);

		ExprBase *context = NULL;

		if(value == NULL)
			context = new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(target->source, ptrType);
		else
			context = new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(target->source, ptrType, value, value + ptrType->subType->size);

		return new (ctx.ctx.get<ExprFunctionLiteral>()) ExprFunctionLiteral(target->source, type, data, context);
	}

	if(TypeRef *ptrType = getType<TypeRef>(type))
	{
		unsigned char *value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));
		
		if(value == NULL)
			return new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(target->source, type);

		return new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(target->source, type, value, value + ptrType->subType->size);
	}

	if(TypeEnum *enumType = getType<TypeEnum>(type))
	{
		int value;
		assert(type->size == sizeof(value));
		memcpy(&value, ptr->ptr, unsigned(type->size));

		return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(target->source, type, value);
	}

	ExprPointerLiteral *storage = AllocateTypeStorage(ctx, target->source, type);

	if(!storage)
		return NULL;

	memcpy(storage->ptr, ptr->ptr, unsigned(type->size));

	return new (ctx.ctx.get<ExprMemoryLiteral>()) ExprMemoryLiteral(target->source, type, storage);
}

bool CreateInsert(ExpressionEvalContext &ctx, ExprMemoryLiteral *memory, unsigned offset, ExprBase *value)
{
	assert(memory->ptr->ptr + value->type->size <= memory->ptr->end);

	ExprPointerLiteral addr(memory->source, ctx.ctx.GetReferenceType(value->type), memory->ptr->ptr + offset, memory->ptr->ptr + offset + value->type->size);

	if(!CreateStore(ctx, &addr, value))
		return false;

	return true;
}

ExprBase* CreateExtract(ExpressionEvalContext &ctx, ExprMemoryLiteral *memory, unsigned offset, TypeBase *type)
{
	assert(memory->ptr->ptr + type->size <= memory->ptr->end);

	ExprPointerLiteral addr(memory->source, ctx.ctx.GetReferenceType(type), memory->ptr->ptr + offset, memory->ptr->ptr + offset + type->size);

	return CreateLoad(ctx, &addr);
}

ExprMemoryLiteral* CreateConstruct(ExpressionEvalContext &ctx, TypeBase *type, ExprBase *el0, ExprBase *el1, ExprBase *el2)
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

	ExprMemoryLiteral *memory = new (ctx.ctx.get<ExprMemoryLiteral>()) ExprMemoryLiteral(el0->source, type, storage);

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

ExprPointerLiteral* FindVariableStorage(ExpressionEvalContext &ctx, VariableData *data, bool skipMissing)
{
	if(ctx.stackFrames.empty())
		return (ExprPointerLiteral*)Report(ctx, "ERROR: no stack frame");

	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	for(unsigned i = 0; i < frame->variables.size(); i++)
	{
		ExpressionEvalContext::StackVariable &variable = frame->variables[i];

		if(variable.variable == data)
			return variable.ptr;
	}

	if(ctx.globalFrame)
	{
		for(unsigned i = 0; i < ctx.globalFrame->variables.size(); i++)
		{
			ExpressionEvalContext::StackVariable &variable = ctx.globalFrame->variables[i];

			if(variable.variable == data)
				return variable.ptr;
		}
	}

	if(data->importModule != NULL)
		return (ExprPointerLiteral*)Report(ctx, "ERROR: can't access external variable '%.*s'", FMT_ISTR(data->name->name));

	if(skipMissing)
		return NULL;

	return (ExprPointerLiteral*)Report(ctx, "ERROR: variable '%.*s' not found", FMT_ISTR(data->name->name));
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
	if(isType<ExprNullptrLiteral>(expression))
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

ExprBase* CreateBinaryOp(ExpressionEvalContext &ctx, SynBase *source, ExprBase *lhs, ExprBase *unevaluatedRhs, SynBinaryOpType op)
{
	if(isType<TypeError>(lhs->type) || isType<TypeError>(unevaluatedRhs->type))
		return Report(ctx, "ERROR: encountered an error node");

	assert(lhs->type == unevaluatedRhs->type);

	if((ctx.ctx.IsIntegerType(lhs->type) || isType<TypeEnum>(lhs->type)) && (ctx.ctx.IsIntegerType(unevaluatedRhs->type) || isType<TypeEnum>(unevaluatedRhs->type)))
	{
		long long lhsValue = 0;
		long long rhsValue = 0;

		// Short-circuit behaviour
		if(op == SYN_BINARY_OP_LOGICAL_AND)
		{
			if(TryTakeLong(lhs, lhsValue))
			{
				if(lhsValue == 0)
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, false);

				ExprBase *rhs = Evaluate(ctx, unevaluatedRhs);

				if(!rhs)
					return NULL;

				if(TryTakeLong(rhs, rhsValue))
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, rhsValue != 0);
			}

			return NULL;
		}

		if(op == SYN_BINARY_OP_LOGICAL_OR)
		{
			if(TryTakeLong(lhs, lhsValue))
			{
				if(lhsValue == 1)
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, true);

				ExprBase *rhs = Evaluate(ctx, unevaluatedRhs);

				if(!rhs)
					return NULL;

				if(TryTakeLong(rhs, rhsValue))
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, rhsValue != 0);
			}

			return NULL;
		}

		ExprBase *rhs = Evaluate(ctx, unevaluatedRhs);

		if(!rhs)
			return NULL;

		assert(lhs->type == rhs->type);

		TypeBase *resultType = ctx.ctx.typeInt;

		if(lhs->type == ctx.ctx.typeLong || rhs->type == ctx.ctx.typeLong)
			resultType = ctx.ctx.typeLong;

		if(TryTakeLong(lhs, lhsValue) && TryTakeLong(rhs, rhsValue))
		{
			if(resultType == ctx.ctx.typeInt)
			{
				int lhsValueInt = int(lhsValue);
				int rhsValueInt = int(rhsValue);

				switch(op)
				{
				case SYN_BINARY_OP_ADD:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValueInt + rhsValueInt);
				case SYN_BINARY_OP_SUB:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValueInt - rhsValueInt);
				case SYN_BINARY_OP_MUL:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, int(lhsValueInt * rhsValueInt));
				case SYN_BINARY_OP_DIV:
					if(rhsValueInt == 0)
						return ReportCritical(ctx, "ERROR: division by zero during constant folding");

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValueInt / rhsValueInt);
				case SYN_BINARY_OP_MOD:
					if(rhsValueInt == 0)
						return ReportCritical(ctx, "ERROR: modulus division by zero during constant folding");

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValueInt % rhsValueInt);
				case SYN_BINARY_OP_POW:
					if(rhsValueInt < 0)
						return ReportCritical(ctx, "ERROR: negative power on integer number in exponentiation during constant folding");

					int result, power;

					result = 1;
					power = rhsValueInt;

					while(power)
					{
						if(power & 1)
						{
							result *= lhsValueInt;
							power--;
						}
						lhsValueInt *= lhsValueInt;
						power >>= 1;
					}

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, result);
				case SYN_BINARY_OP_SHL:
					if(rhsValueInt < 0)
						return ReportCritical(ctx, "ERROR: negative shift value");

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValueInt << rhsValueInt);
				case SYN_BINARY_OP_SHR:
					if(rhsValueInt < 0)
						return ReportCritical(ctx, "ERROR: negative shift value");

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValueInt >> rhsValueInt);
				case SYN_BINARY_OP_LESS:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt < rhsValueInt);
				case SYN_BINARY_OP_LESS_EQUAL:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt <= rhsValueInt);
				case SYN_BINARY_OP_GREATER:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt > rhsValueInt);
				case SYN_BINARY_OP_GREATER_EQUAL:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt >= rhsValueInt);
				case SYN_BINARY_OP_EQUAL:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt == rhsValueInt);
				case SYN_BINARY_OP_NOT_EQUAL:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt != rhsValueInt);
				case SYN_BINARY_OP_BIT_AND:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValueInt & rhsValueInt);
				case SYN_BINARY_OP_BIT_OR:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValueInt | rhsValueInt);
				case SYN_BINARY_OP_BIT_XOR:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValueInt ^ rhsValueInt);
				case SYN_BINARY_OP_LOGICAL_XOR:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, !!lhsValueInt != !!rhsValueInt);
				default:
					assert(!"unexpected type");
					break;
				}
			}
			else
			{
				switch(op)
				{
				case SYN_BINARY_OP_ADD:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue + rhsValue);
				case SYN_BINARY_OP_SUB:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue - rhsValue);
				case SYN_BINARY_OP_MUL:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue * rhsValue);
				case SYN_BINARY_OP_DIV:
					if(rhsValue == 0)
						return ReportCritical(ctx, "ERROR: division by zero during constant folding");

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue / rhsValue);
				case SYN_BINARY_OP_MOD:
					if(rhsValue == 0)
						return ReportCritical(ctx, "ERROR: modulus division by zero during constant folding");

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue % rhsValue);
				case SYN_BINARY_OP_POW:
					if(rhsValue < 0)
						return ReportCritical(ctx, "ERROR: negative power on integer number in exponentiation during constant folding");

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

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, result);
				case SYN_BINARY_OP_SHL:
					if(rhsValue < 0)
						return ReportCritical(ctx, "ERROR: negative shift value");

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue << rhsValue);
				case SYN_BINARY_OP_SHR:
					if(rhsValue < 0)
						return ReportCritical(ctx, "ERROR: negative shift value");

					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue >> rhsValue);
				case SYN_BINARY_OP_LESS:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue < rhsValue);
				case SYN_BINARY_OP_LESS_EQUAL:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue <= rhsValue);
				case SYN_BINARY_OP_GREATER:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue > rhsValue);
				case SYN_BINARY_OP_GREATER_EQUAL:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue >= rhsValue);
				case SYN_BINARY_OP_EQUAL:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue == rhsValue);
				case SYN_BINARY_OP_NOT_EQUAL:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue != rhsValue);
				case SYN_BINARY_OP_BIT_AND:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue & rhsValue);
				case SYN_BINARY_OP_BIT_OR:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue | rhsValue);
				case SYN_BINARY_OP_BIT_XOR:
					return new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(source, lhs->type, lhsValue ^ rhsValue);
				case SYN_BINARY_OP_LOGICAL_XOR:
					return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, !!lhsValue != !!rhsValue);
				default:
					assert(!"unexpected type");
					break;
				}
			}
		}

		return Report(ctx, "ERROR: failed to eval binary op");
	}

	ExprBase *rhs = Evaluate(ctx, unevaluatedRhs);

	if(!rhs)
		return NULL;

	if(ctx.ctx.IsFloatingPointType(lhs->type) && ctx.ctx.IsFloatingPointType(rhs->type))
	{
		assert(lhs->type == rhs->type);

		double lhsValue = 0;
		double rhsValue = 0;

		if(TryTakeDouble(lhs, lhsValue) && TryTakeDouble(rhs, rhsValue))
		{
			switch(op)
			{
			case SYN_BINARY_OP_ADD:
				return new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(source, lhs->type, lhsValue + rhsValue);
			case SYN_BINARY_OP_SUB:
				return new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(source, lhs->type, lhsValue - rhsValue);
			case SYN_BINARY_OP_MUL:
				return new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(source, lhs->type, lhsValue * rhsValue);
			case SYN_BINARY_OP_DIV:
				return new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(source, lhs->type, lhsValue / rhsValue);
			case SYN_BINARY_OP_MOD:
				return new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(source, lhs->type, fmod(lhsValue, rhsValue));
			case SYN_BINARY_OP_POW:
				return new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(source, lhs->type, pow(lhsValue, rhsValue));
			case SYN_BINARY_OP_LESS:
				return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue < rhsValue);
			case SYN_BINARY_OP_LESS_EQUAL:
				return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue <= rhsValue);
			case SYN_BINARY_OP_GREATER:
				return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue > rhsValue);
			case SYN_BINARY_OP_GREATER_EQUAL:
				return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue >= rhsValue);
			case SYN_BINARY_OP_EQUAL:
				return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue == rhsValue);
			case SYN_BINARY_OP_NOT_EQUAL:
				return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue != rhsValue);
			default:
				assert(!"unexpected type");
				break;
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
				return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue == rhsValue);

			if(op == SYN_BINARY_OP_NOT_EQUAL)
				return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue != rhsValue);
		}
	}
	else if(isType<TypeRef>(lhs->type) && isType<TypeRef>(rhs->type))
	{
		assert(lhs->type == rhs->type);

		void *lPtr = NULL;

		if(isType<ExprNullptrLiteral>(lhs))
			lPtr = NULL;
		else if(ExprPointerLiteral *value = getType<ExprPointerLiteral>(lhs))
			lPtr = value->ptr;
		else
			assert(!"unknown type");

		void *rPtr = NULL;

		if(isType<ExprNullptrLiteral>(rhs))
			rPtr = NULL;
		else if(ExprPointerLiteral *value = getType<ExprPointerLiteral>(rhs))
			rPtr = value->ptr;
		else
			assert(!"unknown type");

		if(op == SYN_BINARY_OP_EQUAL)
			return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lPtr == rPtr);

		if(op == SYN_BINARY_OP_NOT_EQUAL)
			return new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(source, ctx.ctx.typeBool, lPtr != rPtr);
	}

	return Report(ctx, "ERROR: failed to eval binary op");
}

ExprBase* CheckType(ExprBase* expression, ExprBase *value)
{
	if(isType<TypeError>(expression->type))
		return NULL;

	assert(expression->type == value->type);

	return value;
}

ExprBase* EvaluateVoid(ExpressionEvalContext &ctx, ExprBase *expression)
{
	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateBoolLiteral(ExpressionEvalContext &ctx, ExprBoolLiteral *expression)
{
	return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, expression->type, expression->value));
}

ExprBase* EvaluateCharacterLiteral(ExpressionEvalContext &ctx, ExprCharacterLiteral *expression)
{
	return CheckType(expression, new (ctx.ctx.get<ExprCharacterLiteral>()) ExprCharacterLiteral(expression->source, expression->type, expression->value));
}

ExprBase* EvaluateStringLiteral(ExpressionEvalContext &ctx, ExprStringLiteral *expression)
{
	return CheckType(expression, new (ctx.ctx.get<ExprStringLiteral>()) ExprStringLiteral(expression->source, expression->type, expression->value, expression->length));
}

ExprBase* EvaluateIntegerLiteral(ExpressionEvalContext &ctx, ExprIntegerLiteral *expression)
{
	return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, expression->type, expression->value));
}

ExprBase* EvaluateRationalLiteral(ExpressionEvalContext &ctx, ExprRationalLiteral *expression)
{
	return CheckType(expression, new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(expression->source, expression->type, expression->value));
}

ExprBase* EvaluateTypeLiteral(ExpressionEvalContext &ctx, ExprTypeLiteral *expression)
{
	return CheckType(expression, new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(expression->source, expression->type, expression->value));
}

ExprBase* EvaluateNullptrLiteral(ExpressionEvalContext &ctx, ExprNullptrLiteral *expression)
{
	return CheckType(expression, new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, expression->type));
}

ExprBase* EvaluateFunctionIndexLiteral(ExpressionEvalContext &ctx, ExprFunctionIndexLiteral *expression)
{
	return CheckType(expression, new (ctx.ctx.get<ExprFunctionIndexLiteral>()) ExprFunctionIndexLiteral(expression->source, expression->type, expression->function));
}

ExprBase* EvaluateArray(ExpressionEvalContext &ctx, ExprArray *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

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

		ExprPointerLiteral *target = new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(arrayType->subType), targetPtr, targetPtr + arrayType->subType->size);

		if(!CreateStore(ctx, target, element))
			return NULL;

		offset += unsigned(arrayType->subType->size);
	}

	ExprBase *load = CreateLoad(ctx, storage);

	if(!load)
		return NULL;

	return CheckType(expression, load);
}

ExprBase* EvaluatePreModify(ExpressionEvalContext &ctx, ExprPreModify *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *ptr = Evaluate(ctx, expression->value);

	if(!ptr)
		return NULL;

	ExprBase *value = CreateLoad(ctx, ptr);

	if(!value)
		return NULL;

	ExprBase *modified = CreateBinaryOp(ctx, expression->source, value, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, value->type, 1), expression->isIncrement ? SYN_BINARY_OP_ADD : SYN_BINARY_OP_SUB);

	if(!modified)
		return NULL;

	if(!CreateStore(ctx, ptr, modified))
		return NULL;

	return CheckType(expression, modified);
}

ExprBase* EvaluatePostModify(ExpressionEvalContext &ctx, ExprPostModify *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *ptr = Evaluate(ctx, expression->value);

	if(!ptr)
		return NULL;

	ExprBase *value = CreateLoad(ctx, ptr);

	if(!value)
		return NULL;

	ExprBase *modified = CreateBinaryOp(ctx, expression->source, value, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, value->type, 1), expression->isIncrement ? SYN_BINARY_OP_ADD : SYN_BINARY_OP_SUB);

	if(!modified)
		return NULL;

	if(!CreateStore(ctx, ptr, modified))
		return NULL;

	return CheckType(expression, value);
}

ExprBase* EvaluateCast(ExpressionEvalContext &ctx, ExprTypeCast *expression)
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

			if(ExprRationalLiteral *expr = getType<ExprRationalLiteral>(value))
			{
				if(expression->type == ctx.ctx.typeBool)
					return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, ctx.ctx.typeBool, expr->value != 0.0));

				if(expression->type == ctx.ctx.typeChar)
					return CheckType(expression, new (ctx.ctx.get<ExprCharacterLiteral>()) ExprCharacterLiteral(expression->source, ctx.ctx.typeChar, (char)expr->value));

				if(expression->type == ctx.ctx.typeShort)
					return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeShort, (short)expr->value));

				if(expression->type == ctx.ctx.typeInt)
					return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, (int)expr->value));

				if(expression->type == ctx.ctx.typeLong)
					return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeLong, (long long)expr->value));
			}
			else if(TryTakeLong(value, result))
			{
				if(expression->type == ctx.ctx.typeBool)
					return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, ctx.ctx.typeBool, result != 0));

				if(expression->type == ctx.ctx.typeChar)
					return CheckType(expression, new (ctx.ctx.get<ExprCharacterLiteral>()) ExprCharacterLiteral(expression->source, ctx.ctx.typeChar, (char)result));

				if(expression->type == ctx.ctx.typeShort)
					return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeShort, (short)result));

				if(expression->type == ctx.ctx.typeInt)
					return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, (int)result));

				if(expression->type == ctx.ctx.typeLong)
					return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeLong, result));
			}
		}
		else if(ctx.ctx.IsFloatingPointType(expression->type))
		{
			double result = 0.0;

			if(TryTakeDouble(value, result))
			{
				if(expression->type == ctx.ctx.typeFloat)
					return CheckType(expression, new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(expression->source, ctx.ctx.typeFloat, (float)result));

				if(expression->type == ctx.ctx.typeDouble)
					return CheckType(expression, new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(expression->source, ctx.ctx.typeDouble, result));
			}
		}
		break;
	case EXPR_CAST_PTR_TO_BOOL:
		return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, ctx.ctx.typeBool, !isType<ExprNullptrLiteral>(value)));
	case EXPR_CAST_UNSIZED_TO_BOOL:
		{
			ExprMemoryLiteral *memLiteral = getType<ExprMemoryLiteral>(value);

			ExprBase *ptr = CreateExtract(ctx, memLiteral, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

			return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, ctx.ctx.typeBool, !isType<ExprNullptrLiteral>(ptr)));
		}
		break;
	case EXPR_CAST_FUNCTION_TO_BOOL:
		{
			ExprFunctionLiteral *funcLiteral = getType<ExprFunctionLiteral>(value);

			return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, ctx.ctx.typeBool, funcLiteral->data != NULL));
		}
		break;
	case EXPR_CAST_NULL_TO_PTR:
		return CheckType(expression, new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, expression->type));
	case EXPR_CAST_NULL_TO_AUTO_PTR:
		{
			ExprBase *typeId = new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, ctx.ctx.typeVoid);
			ExprBase *ptr = new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

			ExprBase *result = CreateConstruct(ctx, expression->type, typeId, ptr, NULL);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_NULL_TO_UNSIZED:
		{
			ExprBase *ptr = new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));
			ExprBase *size = new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, 0);

			ExprBase *result = CreateConstruct(ctx, expression->type, ptr, size, NULL);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_NULL_TO_AUTO_ARRAY:
		{
			ExprBase *typeId = new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, ctx.ctx.typeVoid);
			ExprBase *ptr = new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));
			ExprBase *length = new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, 0);

			ExprBase *result = CreateConstruct(ctx, expression->type, typeId, ptr, length);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_NULL_TO_FUNCTION:
		{
			ExprBase *context = new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

			ExprFunctionLiteral *result = new (ctx.ctx.get<ExprFunctionLiteral>()) ExprFunctionLiteral(expression->source, expression->type, NULL, context);

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

			ExprBase *result = CreateConstruct(ctx, expression->type, value, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, arrType->length), NULL);

			if(!result)
				return NULL;

			return CheckType(expression, result);
		}
		break;
	case EXPR_CAST_PTR_TO_AUTO_PTR:
		{
			TypeRef *refType = getType<TypeRef>(value->type);

			assert(refType);

			TypeClass *classType = getType<TypeClass>(refType->subType);

			ExprBase *typeId = NULL;

			if(classType && (classType->extendable || classType->baseClass))
			{
				if(isType<ExprNullptrLiteral>(value))
					return Report(ctx, "ERROR: null pointer access");

				ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(value);

				assert(ptr);
				assert(ptr->end - ptr->ptr >= 4);

				typeId = CreateLoad(ctx, new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeTypeID), ptr->ptr, ptr->ptr + 4));
			}
			else
			{
				typeId = new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, refType->subType);
			}

			ExprBase *result = CreateConstruct(ctx, expression->type, typeId, value, NULL);

			if(!result)
				return NULL;

			return CheckType(expression, result);
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

			ExprBase *typeId = new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, arrType->subType);
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

			if(isType<TypeError>(typeLiteral->value))
				return NULL;

			if(isType<TypeArgumentSet>(typeLiteral->value) || isType<TypeMemberSet>(typeLiteral->value))
				return NULL;

			unsigned index = ctx.ctx.GetTypeIndex(typeLiteral->value);

			return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, index));
		}
		else if(isType<TypeRef>(expression->type) && isType<TypeRef>(value->type))
		{
			TypeRef *refType = getType<TypeRef>(expression->type);

			if(isType<ExprNullptrLiteral>(value))
				return CheckType(expression, new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, expression->type));
			
			if(ExprPointerLiteral *tmp = getType<ExprPointerLiteral>(value))
			{
				(void)refType;
				assert(uintptr_t(tmp->end - tmp->ptr) >= uintptr_t(refType->subType->size));

				return CheckType(expression, new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, expression->type, tmp->ptr, tmp->end));
			}
		}
		else if(isType<TypeUnsizedArray>(expression->type) && isType<TypeUnsizedArray>(value->type))
		{
			ExprMemoryLiteral *memLiteral = getType<ExprMemoryLiteral>(value);

			return CheckType(expression, new (ctx.ctx.get<ExprMemoryLiteral>()) ExprMemoryLiteral(expression->source, expression->type, memLiteral->ptr));
		}
		else if(isType<TypeFunction>(expression->type) && isType<TypeFunction>(value->type))
		{
			ExprFunctionLiteral *funcLiteral = getType<ExprFunctionLiteral>(value);

			return CheckType(expression, new (ctx.ctx.get<ExprFunctionLiteral>()) ExprFunctionLiteral(expression->source, expression->type, funcLiteral->data, funcLiteral->context));
		}
		else if(isType<TypeInt>(expression->type) && isType<TypeEnum>(value->type))
		{
			ExprIntegerLiteral *intLiteral = getType<ExprIntegerLiteral>(value);

			return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, expression->type, intLiteral->value));
		}
		else if(isType<TypeEnum>(expression->type) && isType<TypeInt>(value->type))
		{
			ExprIntegerLiteral *intLiteral = getType<ExprIntegerLiteral>(value);

			return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, expression->type, intLiteral->value));
		}
		break;
	}

	return Report(ctx, "ERROR: failed to cast '%.*s' to '%.*s'", FMT_ISTR(value->type->name), FMT_ISTR(expression->type->name));
}

ExprBase* EvaluateUnaryOp(ExpressionEvalContext &ctx, ExprUnaryOp *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

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
				return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, expression->type, !expr->value));
		}
		else if(ExprIntegerLiteral *expr = getType<ExprIntegerLiteral>(value))
		{
			if(expression->op == SYN_UNARY_OP_LOGICAL_NOT)
				return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, expression->type, !expr->value));
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
				return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, expression->type, result));
			case SYN_UNARY_OP_NEGATE:
				return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, expression->type, -result));
			case SYN_UNARY_OP_BIT_NOT:
				return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, expression->type, ~result));
			case SYN_UNARY_OP_LOGICAL_NOT:
				return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, expression->type, !result));
			default:
				assert(!"unknown unary operation");
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
				return CheckType(expression, new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(expression->source, expression->type, result));
			case SYN_UNARY_OP_NEGATE:
				return CheckType(expression, new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(expression->source, expression->type, -result));
			case SYN_UNARY_OP_BIT_NOT:
			case SYN_UNARY_OP_LOGICAL_NOT:
				return NULL;
			default:
				assert(!"unknown unary operation");
			}
		}
	}
	else if(isType<TypeRef>(value->type))
	{
		void *lPtr = NULL;

		if(isType<ExprNullptrLiteral>(value))
			lPtr = NULL;
		else if(ExprPointerLiteral *tmp = getType<ExprPointerLiteral>(value))
			lPtr = tmp->ptr;
		else
			assert(!"unknown type");

		if(expression->op == SYN_UNARY_OP_LOGICAL_NOT)
			return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, ctx.ctx.typeBool, !lPtr));
	}
	else if(value->type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral *memLiteral = getType<ExprMemoryLiteral>(value);

		void *lPtr = 0;
		if(!TryTakePointer(CreateExtract(ctx, memLiteral, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), lPtr))
			return Report(ctx, "ERROR: failed to evaluate auto ref value");

		return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, ctx.ctx.typeBool, !lPtr));

	}

	return Report(ctx, "ERROR: failed to eval unary op");
}

ExprBase* EvaluateBinaryOp(ExpressionEvalContext &ctx, ExprBinaryOp *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *lhs = Evaluate(ctx, expression->lhs);

	if(!lhs)
		return NULL;

	// rhs remain unevaluated
	ExprBase *result = CreateBinaryOp(ctx, expression->source, lhs, expression->rhs, expression->op);

	if(!result)
		return result;

	return CheckType(expression, result);
}

ExprBase* EvaluateGetAddress(ExpressionEvalContext &ctx, ExprGetAddress *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	ExprPointerLiteral *ptr = FindVariableStorage(ctx, expression->variable->variable, false);

	if(!ptr)
		return NULL;

	return CheckType(expression, ptr);
}

ExprBase* EvaluateDereference(ExpressionEvalContext &ctx, ExprDereference *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

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

ExprBase* EvaluateConditional(ExpressionEvalContext &ctx, ExprConditional *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

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

ExprBase* EvaluateAssignment(ExpressionEvalContext &ctx, ExprAssignment *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

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

ExprBase* EvaluateMemberAccess(ExpressionEvalContext &ctx, ExprMemberAccess *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	if(isType<ExprNullptrLiteral>(value))
		return Report(ctx, "ERROR: member access of null pointer");

	ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(value);

	assert(ptr);
	assert(ptr->ptr + expression->member->variable->offset + expression->member->variable->type->size <= ptr->end);

	unsigned char *targetPtr = ptr->ptr + expression->member->variable->offset;

	ExprPointerLiteral *shifted = new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(expression->member->variable->type), targetPtr, targetPtr + expression->member->variable->type->size);

	return CheckType(expression, shifted);
}

ExprBase* EvaluateArrayIndex(ExpressionEvalContext &ctx, ExprArrayIndex *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

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
			return ReportCritical(ctx, "ERROR: array index out of bounds");

		ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(value);

		assert(ptr);

		unsigned char *targetPtr = ptr->ptr + result * arrayType->subType->size;

		ExprPointerLiteral *shifted = new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(arrayType->subType), targetPtr, targetPtr + arrayType->subType->size);

		return CheckType(expression, shifted);
	}

	TypeRef *refType = getType<TypeRef>(value->type);

	assert(refType);

	TypeArray *arrayType = getType<TypeArray>(refType->subType);

	assert(arrayType);

	if(isType<ExprNullptrLiteral>(value))
		return Report(ctx, "ERROR: array index of a null array");

	if(result < 0 || result >= arrayType->length)
		return ReportCritical(ctx, "ERROR: array index out of bounds");

	ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(value);

	assert(ptr);
	assert(ptr->ptr + result * arrayType->subType->size + arrayType->subType->size <= ptr->end);

	unsigned char *targetPtr = ptr->ptr + result * arrayType->subType->size;

	ExprPointerLiteral *shifted = new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(arrayType->subType), targetPtr, targetPtr + arrayType->subType->size);

	return CheckType(expression, shifted);
}

ExprBase* EvaluateReturn(ExpressionEvalContext &ctx, ExprReturn *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	if(frame->targetYield)
		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));

	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	if(ctx.stackFrames.empty())
		return Report(ctx, "ERROR: no stack frame to return from");

	frame->returnValue = value;

	if(expression->coroutineStateUpdate)
	{
		if(!Evaluate(ctx, expression->coroutineStateUpdate))
			return NULL;
	}

	if(expression->closures)
	{
		if(!Evaluate(ctx, expression->closures))
			return NULL;
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateYield(ExpressionEvalContext &ctx, ExprYield *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	// Check if we reached target yield
	if(frame->targetYield == expression->order)
	{
		frame->targetYield = 0;

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}

	if(frame->targetYield)
		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));

	ExprBase *value = Evaluate(ctx, expression->value);

	if(!value)
		return NULL;

	if(ctx.stackFrames.empty())
		return Report(ctx, "ERROR: no stack frame to return from");

	frame->returnValue = value;

	if(expression->coroutineStateUpdate)
	{
		if(!Evaluate(ctx, expression->coroutineStateUpdate))
			return NULL;
	}

	if(expression->closures)
	{
		if(!Evaluate(ctx, expression->closures))
			return NULL;
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateVariableDefinition(ExpressionEvalContext &ctx, ExprVariableDefinition *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	if(FindVariableStorage(ctx, expression->variable->variable, true) == NULL)
	{
		TypeBase *type = expression->variable->variable->type;

		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, type);

		if(!storage)
			return NULL;

		frame->variables.push_back(ExpressionEvalContext::StackVariable(expression->variable->variable, storage));
	}

	if(!frame->targetYield)
	{
		if(expression->initializer)
		{
			if(!Evaluate(ctx, expression->initializer))
				return NULL;
		}
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateZeroInitialize(ExpressionEvalContext &ctx, ExprZeroInitialize *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	TypeRef *refType = getType<TypeRef>(expression->address->type);

	assert(refType);

	ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(Evaluate(ctx, expression->address));

	if(!ptr)
		return NULL;

	assert(ptr->ptr);
	assert(uintptr_t(ptr->end - ptr->ptr) == unsigned(refType->subType->size));

	memset(ptr->ptr, 0, unsigned(refType->subType->size));

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateArraySetup(ExpressionEvalContext &ctx, ExprArraySetup *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	TypeRef *refType = getType<TypeRef>(expression->lhs->type);

	assert(refType);

	TypeArray *arrayType = getType<TypeArray>(refType->subType);

	assert(arrayType);

	ExprBase *initializer = Evaluate(ctx, expression->initializer);

	if(!initializer)
		return NULL;

	ExprPointerLiteral *ptr = getType<ExprPointerLiteral>(Evaluate(ctx, expression->lhs));

	if(!ptr)
		return NULL;

	for(unsigned i = 0; i < unsigned(arrayType->length); i++)
	{
		if(!AddInstruction(ctx))
			return NULL;

		assert(ptr);
		assert(ptr->ptr + i * arrayType->subType->size + arrayType->subType->size <= ptr->end);

		unsigned char *targetPtr = ptr->ptr + i * arrayType->subType->size;

		ExprPointerLiteral *shifted = new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(arrayType->subType), targetPtr, targetPtr + arrayType->subType->size);

		if(!CreateStore(ctx, shifted, initializer))
			return NULL;
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateVariableDefinitions(ExpressionEvalContext &ctx, ExprVariableDefinitions *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	for(ExprBase *definition = expression->definitions.head; definition; definition = definition->next)
	{
		if(!Evaluate(ctx, definition))
			return NULL;
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateVariableAccess(ExpressionEvalContext &ctx, ExprVariableAccess *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	ExprPointerLiteral *ptr = FindVariableStorage(ctx, expression->variable, false);

	if(!ptr)
		return NULL;

	ExprBase *value = CreateLoad(ctx, ptr);

	if(!value)
		return NULL;

	return CheckType(expression, value);
}

ExprBase* EvaluateFunctionContextAccess(ExpressionEvalContext &ctx, ExprFunctionContextAccess *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	ExprPointerLiteral *ptr = FindVariableStorage(ctx, expression->contextVariable, false);

	if(!ptr)
		return NULL;

	ExprBase *value = NULL;

	TypeRef *refType = getType<TypeRef>(expression->function->contextType);

	assert(refType);

	TypeClass *classType = getType<TypeClass>(refType->subType);

	assert(classType);

	if(classType->members.empty())
		value = new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, expression->function->contextType);
	else
		value = CreateLoad(ctx, ptr);

	if(!value)
		return NULL;

	return CheckType(expression, value);
}

ExprBase* EvaluateFunctionDefinition(ExpressionEvalContext &ctx, ExprFunctionDefinition *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *context = new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

	return CheckType(expression, new (ctx.ctx.get<ExprFunctionLiteral>()) ExprFunctionLiteral(expression->source, expression->function->type, expression->function, context));
}

ExprBase* EvaluateGenericFunctionPrototype(ExpressionEvalContext &ctx, ExprGenericFunctionPrototype *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	for(ExprBase *expr = expression->contextVariables.head; expr; expr = expr->next)
	{
		if(!Evaluate(ctx, expr))
			return NULL;
	}

	return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);
}

ExprBase* EvaluateFunction(ExpressionEvalContext &ctx, ExprFunctionDefinition *expression, ExprBase *context, ArrayView<ExprBase*> arguments)
{
	if(!AddInstruction(ctx))
		return NULL;

	if(ctx.stackFrames.size() >= ctx.stackDepthLimit)
		return Report(ctx, "ERROR: stack depth limit");

	ctx.stackFrames.push_back(new (ctx.ctx.get<ExpressionEvalContext::StackFrame>()) ExpressionEvalContext::StackFrame(ctx.ctx.allocator, expression->function));

	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	if(ExprVariableDefinition *curr = expression->contextArgument)
	{
		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, curr->source, curr->variable->variable->type);

		if(!storage)
			return NULL;

		frame->variables.push_back(ExpressionEvalContext::StackVariable(curr->variable->variable, storage));

		if(!CreateStore(ctx, storage, context))
			return NULL;
	}

	unsigned pos = 0;

	for(ExprVariableDefinition *curr = expression->arguments.head; curr; curr = getType<ExprVariableDefinition>(curr->next))
	{
		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, curr->source, curr->variable->variable->type);

		if(!storage)
			return NULL;

		frame->variables.push_back(ExpressionEvalContext::StackVariable(curr->variable->variable, storage));

		if(!CreateStore(ctx, storage, arguments[pos]))
			return NULL;

		pos++;
	}

	if(expression->coroutineStateRead)
	{
		if(ExprIntegerLiteral *jmpOffset = getType<ExprIntegerLiteral>(Evaluate(ctx, expression->coroutineStateRead)))
		{
			frame->targetYield = unsigned(jmpOffset->value);
		}
		else
		{
			return NULL;
		}
	}

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		if(!Evaluate(ctx, value))
			return NULL;

		assert(frame->breakDepth == 0 && frame->continueDepth == 0);

		if(frame->returnValue)
			break;
	}

	ExprBase *result = frame->returnValue;

	if(!result)
		return ReportCritical(ctx, "ERROR: function didn't return a value");

	ctx.stackFrames.pop_back();

	return result;
}

ExprBase* EvaluateFunctionAccess(ExpressionEvalContext &ctx, ExprFunctionAccess *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *context = Evaluate(ctx, expression->context);

	if(!context)
		return NULL;

	FunctionData *function = expression->function;

	if(function->implementation)
		function = function->implementation;

	return CheckType(expression, new (ctx.ctx.get<ExprFunctionLiteral>()) ExprFunctionLiteral(expression->source, function->type, function, context));
}

ExprBase* EvaluateKnownExternalFunctionCall(ExpressionEvalContext &ctx, ExprFunctionCall *expression, ExprFunctionLiteral *ptr, ArrayView<ExprBase*> arguments)
{
	if(ptr->data->name->name == InplaceStr("assert") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeInt)
	{
		long long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		if(value == 0)
			return Report(ctx, "ERROR: Assertion failed");

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("assert") && arguments.size() == 2 && arguments[0]->type == ctx.ctx.typeInt && arguments[1]->type == ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeChar))
	{
		long long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		ExprMemoryLiteral *memory = getType<ExprMemoryLiteral>(arguments[1]);

		ExprPointerLiteral *str = getType<ExprPointerLiteral>(CreateExtract(ctx, memory, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeChar)));
		ExprIntegerLiteral *length = getType<ExprIntegerLiteral>(CreateExtract(ctx, memory, sizeof(void*), ctx.ctx.typeInt));

		if(!str)
			return Report(ctx, "ERROR: null pointer access");

		assert(length);

		if(value == 0)
			return Report(ctx, "ERROR: %.*s", int(length->value), str->ptr);

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("bool") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeBool)
	{
		long long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new (ctx.ctx.get<ExprBoolLiteral>()) ExprBoolLiteral(expression->source, ctx.ctx.typeBool, value != 0));
	}
	else if(ptr->data->name->name == InplaceStr("char") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeChar)
	{
		long long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeChar, char(value)));
	}
	else if(ptr->data->name->name == InplaceStr("short") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeShort)
	{
		long long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeShort, short(value)));
	}
	else if(ptr->data->name->name == InplaceStr("int") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeInt)
	{
		long long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, int(value)));
	}
	else if(ptr->data->name->name == InplaceStr("long") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeLong)
	{
		long long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeLong, value));
	}
	else if(ptr->data->name->name == InplaceStr("float") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeFloat)
	{
		double value;
		if(!TryTakeDouble(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(expression->source, ctx.ctx.typeFloat, value));
	}
	else if(ptr->data->name->name == InplaceStr("double") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeDouble)
	{
		double value;
		if(!TryTakeDouble(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new (ctx.ctx.get<ExprRationalLiteral>()) ExprRationalLiteral(expression->source, ctx.ctx.typeDouble, value));
	}
	else if(ptr->data->name->name == InplaceStr("bool::bool") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeBool)
	{
		if(!CreateStore(ctx, ptr->context, arguments[0]))
			return NULL;

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("char::char") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeChar)
	{
		if(!CreateStore(ctx, ptr->context, arguments[0]))
			return NULL;

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("short::short") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeShort)
	{
		if(!CreateStore(ctx, ptr->context, arguments[0]))
			return NULL;

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("int::int") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeInt)
	{
		if(!CreateStore(ctx, ptr->context, arguments[0]))
			return NULL;

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("long::long") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeLong)
	{
		if(!CreateStore(ctx, ptr->context, arguments[0]))
			return NULL;

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("float::float") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeFloat)
	{
		if(!CreateStore(ctx, ptr->context, arguments[0]))
			return NULL;

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("double::double") && ptr->context && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeDouble)
	{
		if(!CreateStore(ctx, ptr->context, arguments[0]))
			return NULL;

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("__newS"))
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

		return CheckType(expression, new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid), storage->ptr, storage->end));
	}
	else if(ptr->data->name->name == InplaceStr("__newA"))
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

		if(target->size * unsigned(count) > ctx.variableMemoryLimit)
			return Report(ctx, "ERROR: single variable memory limit");

		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, ctx.ctx.GetArrayType(target, count));

		if(!storage)
			return NULL;

		ExprBase *result = CreateConstruct(ctx, ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeInt), storage, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, count), NULL);

		if(!result)
			return NULL;

		return CheckType(expression, result);
	}
	else if(ptr->data->name->name == InplaceStr("__rcomp"))
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

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, lPtr == rPtr));
	}
	else if(ptr->data->name->name == InplaceStr("__rncomp"))
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

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, lPtr != rPtr));
	}
	else if(ptr->data->name->name == InplaceStr("__pcomp"))
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

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, a->data == b->data && aContext == bContext));
	}
	else if(ptr->data->name->name == InplaceStr("__pncomp"))
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

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, a->data != b->data || aContext != bContext));
	}
	else if(ptr->data->name->name == InplaceStr("__acomp"))
	{
		ExprMemoryLiteral *a = getType<ExprMemoryLiteral>(arguments[0]);
		ExprMemoryLiteral *b = getType<ExprMemoryLiteral>(arguments[1]);

		assert(a && b);
		assert(a->type->size == b->type->size);

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, memcmp(a->ptr->ptr, b->ptr->ptr, unsigned(a->type->size)) == 0));
	}
	else if(ptr->data->name->name == InplaceStr("__ancomp"))
	{
		ExprMemoryLiteral *a = getType<ExprMemoryLiteral>(arguments[0]);
		ExprMemoryLiteral *b = getType<ExprMemoryLiteral>(arguments[1]);

		assert(a && b);
		assert(a->type->size == b->type->size);

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, memcmp(a->ptr->ptr, b->ptr->ptr, unsigned(a->type->size)) != 0));
	}
	else if(ptr->data->name->name == InplaceStr("__typeCount"))
	{
		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, ctx.ctx.types.size()));
	}
	else if(ptr->data->name->name == InplaceStr("__redirect") || ptr->data->name->name == InplaceStr("__redirect_ptr"))
	{
		ExprMemoryLiteral *autoRef = getType<ExprMemoryLiteral>(arguments[0]);
		ExprPointerLiteral *tableRef = getType<ExprPointerLiteral>(arguments[1]);

		if(!tableRef)
			return Report(ctx, "ERROR: null pointer access");

		ExprTypeLiteral *typeID = getType<ExprTypeLiteral>(CreateExtract(ctx, autoRef, 0, ctx.ctx.typeTypeID));

		assert(typeID);

		unsigned typeIndex = ctx.ctx.GetTypeIndex(typeID->value);

		ExprBase *context = CreateExtract(ctx, autoRef, 4, ctx.ctx.GetReferenceType(ctx.ctx.types[typeIndex]));

		assert(context);

		ExprBase *tableRefLoad = CreateLoad(ctx, tableRef);

		if(!tableRefLoad)
			return NULL;

		ExprMemoryLiteral *table = getType<ExprMemoryLiteral>(tableRefLoad);

		ExprPointerLiteral *tableArray = getType<ExprPointerLiteral>(CreateExtract(ctx, table, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeFunctionID)));
		ExprIntegerLiteral *tableSize = getType<ExprIntegerLiteral>(CreateExtract(ctx, table, sizeof(void*), ctx.ctx.typeInt));

		assert(tableArray && tableSize);

		if(typeIndex >= tableSize->value)
			return Report(ctx, "ERROR: type index is out of bounds of redirection table");

		unsigned char *targetPtr = tableArray->ptr + typeIndex * ctx.ctx.typeTypeID->size;

		unsigned index = 0;
		memcpy(&index, targetPtr, sizeof(unsigned));

		FunctionData *data = index != 0 ? ctx.ctx.functions[index - 1] : NULL;

		if(!data)
		{
			if(ptr->data->name->name == InplaceStr("__redirect_ptr"))
				return CheckType(expression, new (ctx.ctx.get<ExprFunctionLiteral>()) ExprFunctionLiteral(expression->source, expression->type, NULL, new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.types[typeIndex]))));

			return Report(ctx, "ERROR: type '%.*s' doesn't implement method", FMT_ISTR(ctx.ctx.types[typeIndex]->name));
		}

		return CheckType(expression, new (ctx.ctx.get<ExprFunctionLiteral>()) ExprFunctionLiteral(expression->source, expression->type, data, context));
	}
	else if(ptr->data->name->name == InplaceStr("duplicate") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral *object = getType<ExprMemoryLiteral>(arguments[0]);

		assert(object);

		ExprTypeLiteral *ptrTypeID = getType<ExprTypeLiteral>(CreateExtract(ctx, object, 0, ctx.ctx.typeTypeID));
		ExprPointerLiteral *ptrPtr = getType<ExprPointerLiteral>(CreateExtract(ctx, object, 4, ctx.ctx.GetReferenceType(ptrTypeID->value)));

		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, ctx.ctx.typeAutoRef);

		if(!storage)
			return NULL;

		ExprMemoryLiteral *result = new (ctx.ctx.get<ExprMemoryLiteral>()) ExprMemoryLiteral(expression->source, ctx.ctx.typeAutoRef, storage);

		CreateInsert(ctx, result, 0, new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, ptrTypeID->value));

		if(!ptrPtr)
		{
			CreateInsert(ctx, result, 4, new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ptrTypeID->value)));

			return CheckType(expression, result);
		}

		ExprPointerLiteral *resultPtr = AllocateTypeStorage(ctx, expression->source, ptrTypeID->value);

		if(!resultPtr)
			return NULL;

		CreateInsert(ctx, result, 4, resultPtr);

		ExprBase *ptrPtrLoad = CreateLoad(ctx, ptrPtr);

		if(!ptrPtrLoad)
			return NULL;

		if(!CreateStore(ctx, resultPtr, ptrPtrLoad))
			return NULL;

		return CheckType(expression, result);
	}
	else if(ptr->data->name->name == InplaceStr("duplicate") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeAutoArray)
	{
		ExprMemoryLiteral *arr = getType<ExprMemoryLiteral>(arguments[0]);

		assert(arr);

		ExprTypeLiteral *arrTypeID = getType<ExprTypeLiteral>(CreateExtract(ctx, arr, 0, ctx.ctx.typeTypeID));
		ExprIntegerLiteral *arrLen = getType<ExprIntegerLiteral>(CreateExtract(ctx, arr, 4 + sizeof(void*), ctx.ctx.typeInt));
		ExprPointerLiteral *arrPtr = getType<ExprPointerLiteral>(CreateExtract(ctx, arr, 4, ctx.ctx.GetReferenceType(ctx.ctx.GetArrayType(arrTypeID->value, arrLen->value))));

		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, ctx.ctx.typeAutoArray);

		if(!storage)
			return NULL;

		ExprMemoryLiteral *result = new (ctx.ctx.get<ExprMemoryLiteral>()) ExprMemoryLiteral(expression->source, ctx.ctx.typeAutoArray, storage);

		CreateInsert(ctx, result, 0, new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, arrTypeID->value));
		CreateInsert(ctx, result, 4 + sizeof(void*), new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, arrLen->value));

		if(!arrPtr)
		{
			CreateInsert(ctx, result, 4, new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));

			return CheckType(expression, result);
		}

		ExprPointerLiteral *resultPtr = AllocateTypeStorage(ctx, expression->source, ctx.ctx.GetArrayType(arrTypeID->value, arrLen->value));

		if(!resultPtr)
			return NULL;

		CreateInsert(ctx, result, 4, resultPtr);

		ExprBase *ptrPtrLoad = CreateLoad(ctx, arrPtr);

		if(!ptrPtrLoad)
			return NULL;

		if(!CreateStore(ctx, resultPtr, ptrPtrLoad))
			return NULL;

		return CheckType(expression, result);
	}
	else if(ptr->data->name->name == InplaceStr("typeid") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral *reference = getType<ExprMemoryLiteral>(arguments[0]);

		assert(reference);

		ExprTypeLiteral *typeID = getType<ExprTypeLiteral>(CreateExtract(ctx, reference, 0, ctx.ctx.typeTypeID));

		return CheckType(expression, typeID);
	}
	else if(ptr->data->name->name == InplaceStr("auto_array") && arguments.size() == 2 && arguments[0]->type == ctx.ctx.typeTypeID && arguments[1]->type == ctx.ctx.typeInt)
	{
		ExprTypeLiteral *type = getType<ExprTypeLiteral>(arguments[0]);
		ExprIntegerLiteral *count = getType<ExprIntegerLiteral>(arguments[1]);

		assert(type && count);

		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, ctx.ctx.typeAutoArray);

		if(!storage)
			return NULL;

		ExprMemoryLiteral *result = new (ctx.ctx.get<ExprMemoryLiteral>()) ExprMemoryLiteral(expression->source, ctx.ctx.typeAutoArray, storage);

		CreateInsert(ctx, result, 0, new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, type->value));

		ExprPointerLiteral *resultPtr = AllocateTypeStorage(ctx, expression->source, ctx.ctx.GetArrayType(type->value, count->value));

		if(!resultPtr)
			return NULL;

		CreateInsert(ctx, result, 4, resultPtr);
		CreateInsert(ctx, result, 4 + sizeof(void*), new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, count->value));

		return CheckType(expression, result);
	}
	else if(ptr->data->name->name == InplaceStr("array_copy") && arguments.size() == 2 && arguments[0]->type == ctx.ctx.typeAutoArray && arguments[1]->type == ctx.ctx.typeAutoArray)
	{
		ExprMemoryLiteral *dst = getType<ExprMemoryLiteral>(arguments[0]);
		ExprMemoryLiteral *src = getType<ExprMemoryLiteral>(arguments[1]);

		assert(dst && src);

		ExprTypeLiteral *dstTypeID = getType<ExprTypeLiteral>(CreateExtract(ctx, dst, 0, ctx.ctx.typeTypeID));
		ExprPointerLiteral *dstPtr = getType<ExprPointerLiteral>(CreateExtract(ctx, dst, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		ExprIntegerLiteral *dstLen = getType<ExprIntegerLiteral>(CreateExtract(ctx, dst, 4 + sizeof(void*), ctx.ctx.typeInt));

		ExprTypeLiteral *srcTypeID = getType<ExprTypeLiteral>(CreateExtract(ctx, src, 0, ctx.ctx.typeTypeID));
		ExprPointerLiteral *srcPtr = getType<ExprPointerLiteral>(CreateExtract(ctx, src, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		ExprIntegerLiteral *srcLen = getType<ExprIntegerLiteral>(CreateExtract(ctx, src, 4 + sizeof(void*), ctx.ctx.typeInt));

		if(!dstPtr && !srcPtr)
			return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));

		if(!srcPtr || dstPtr->ptr == srcPtr->ptr)
			return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));

		if(dstTypeID->value != srcTypeID->value)
			return Report(ctx, "ERROR: destination element type '%.*s' doesn't match source element type '%.*s'", FMT_ISTR(dstTypeID->value->name), FMT_ISTR(srcTypeID->value->name));

		if(dstLen->value < srcLen->value)
			return Report(ctx, "ERROR: destination array size '%d' is smaller than source array size '%d'", unsigned(dstLen->value), unsigned(srcLen->value));

		memcpy(dstPtr->ptr, srcPtr->ptr, unsigned(dstTypeID->value->size * srcLen->value));

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("[]") && arguments.size() == 2 && arguments[0]->type == ctx.ctx.GetReferenceType(ctx.ctx.typeAutoArray) && arguments[1]->type == ctx.ctx.typeInt)
	{
		// Get arguments
		ExprPointerLiteral *arrPtrArg = getType<ExprPointerLiteral>(arguments[0]);

		if(!arrPtrArg)
			return Report(ctx, "ERROR: null pointer access");

		ExprIntegerLiteral *indexArg = getType<ExprIntegerLiteral>(arguments[1]);

		assert(indexArg);

		ExprBase *arrPtrLoad = CreateLoad(ctx, arrPtrArg);

		if(!arrPtrLoad)
			return NULL;

		ExprMemoryLiteral *arr = getType<ExprMemoryLiteral>(arrPtrLoad);

		assert(arr);

		// Check index
		ExprIntegerLiteral *arrLen = getType<ExprIntegerLiteral>(CreateExtract(ctx, arr, 4 + sizeof(void*), ctx.ctx.typeInt));

		if(unsigned(indexArg->value) >= arrLen->value)
			return Report(ctx, "ERROR: array index out of bounds");

		// Load auto[] array type and pointer members
		ExprTypeLiteral *arrTypeID = getType<ExprTypeLiteral>(CreateExtract(ctx, arr, 0, ctx.ctx.typeTypeID));
		ExprPointerLiteral *arrPtr = getType<ExprPointerLiteral>(CreateExtract(ctx, arr, 4, ctx.ctx.GetReferenceType(ctx.ctx.GetArrayType(arrTypeID->value, arrLen->value))));

		// Create storage for result
		ExprPointerLiteral *storage = AllocateTypeStorage(ctx, expression->source, ctx.ctx.typeAutoRef);

		if(!storage)
			return NULL;

		// Create result in that storage
		ExprMemoryLiteral *result = new (ctx.ctx.get<ExprMemoryLiteral>()) ExprMemoryLiteral(expression->source, ctx.ctx.typeAutoRef, storage);

		// Save typeid
		CreateInsert(ctx, result, 0, new (ctx.ctx.get<ExprTypeLiteral>()) ExprTypeLiteral(expression->source, ctx.ctx.typeTypeID, arrTypeID->value));

		// Save pointer to array element
		assert(arrPtr->ptr + indexArg->value * arrTypeID->value->size + arrTypeID->value->size <= arrPtr->end);

		unsigned char *targetPtr = arrPtr->ptr + indexArg->value * arrTypeID->value->size;

		ExprPointerLiteral *shifted = new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(arrTypeID->value), targetPtr, targetPtr + arrTypeID->value->size);

		CreateInsert(ctx, result, 4, shifted);

		return CheckType(expression, result);
	}
	else if(ptr->data->name->name == InplaceStr("__assertCoroutine") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral *functionPtr = getType<ExprMemoryLiteral>(arguments[0]);

		assert(functionPtr);

		ExprTypeLiteral *ptrTypeID = getType<ExprTypeLiteral>(CreateExtract(ctx, functionPtr, 0, ctx.ctx.typeTypeID));

		if(!isType<TypeFunction>(ptrTypeID->value))
			return Report(ctx, "ERROR: '%.*s' is not a function'", FMT_ISTR(ptrTypeID->value->name));

		ExprPointerLiteral *ptrPtr = getType<ExprPointerLiteral>(CreateExtract(ctx, functionPtr, 4, ctx.ctx.GetReferenceType(ptrTypeID->value)));

		assert(ptrPtr);

		ExprFunctionLiteral *function = getType<ExprFunctionLiteral>(CreateLoad(ctx, ptrPtr));

		if(!function->data)
			return Report(ctx, "ERROR: function is not a coroutine'");

		if(!function->data->coroutine)
			return Report(ctx, "ERROR: '%.*s' is not a coroutine'", FMT_ISTR(function->data->name->name));

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}
	else if(ptr->data->name->name == InplaceStr("isCoroutineReset") && arguments.size() == 1 && arguments[0]->type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral *functionPtr = getType<ExprMemoryLiteral>(arguments[0]);

		assert(functionPtr);

		ExprTypeLiteral *ptrTypeID = getType<ExprTypeLiteral>(CreateExtract(ctx, functionPtr, 0, ctx.ctx.typeTypeID));

		if(!isType<TypeFunction>(ptrTypeID->value))
			return Report(ctx, "ERROR: '%.*s' is not a function'", FMT_ISTR(ptrTypeID->value->name));

		ExprPointerLiteral *ptrPtr = getType<ExprPointerLiteral>(CreateExtract(ctx, functionPtr, 4, ctx.ctx.GetReferenceType(ptrTypeID->value)));

		assert(ptrPtr);

		ExprFunctionLiteral *function = getType<ExprFunctionLiteral>(CreateLoad(ctx, ptrPtr));

		if(!function->data->coroutine)
			return Report(ctx, "ERROR: '%.*s' is not a coroutine'", FMT_ISTR(function->data->name->name));

		ExprBase *contextLoad = CreateLoad(ctx, function->context);

		if(!contextLoad)
			return NULL;

		ExprMemoryLiteral *context = getType<ExprMemoryLiteral>(contextLoad);

		// TODO: remove this check, all coroutines must have a context
		if(!context)
			return Report(ctx, "ERROR: '%.*s' coroutine has no context'", FMT_ISTR(function->data->name->name));

		ExprIntegerLiteral *jmpOffset = getType<ExprIntegerLiteral>(CreateExtract(ctx, context, 0, ctx.ctx.typeInt));

		return CheckType(expression, new (ctx.ctx.get<ExprIntegerLiteral>()) ExprIntegerLiteral(expression->source, ctx.ctx.typeInt, jmpOffset->value == 0));
	}
	else if(ptr->data->name->name == InplaceStr("assert_derived_from_base") && arguments.size() == 2 && arguments[0]->type == ctx.ctx.GetReferenceType(ctx.ctx.typeVoid) && arguments[1]->type == ctx.ctx.typeTypeID)
	{
		ExprPointerLiteral *object = getType<ExprPointerLiteral>(arguments[0]);
		ExprTypeLiteral *base = getType<ExprTypeLiteral>(arguments[1]);

		if(!object)
			return CheckType(expression, new (ctx.ctx.get<ExprNullptrLiteral>()) ExprNullptrLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));

		assert(uintptr_t(object->end - object->ptr) >= sizeof(unsigned));

		ExprTypeLiteral *derived = getType<ExprTypeLiteral>(CreateLoad(ctx, new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeTypeID), object->ptr, object->end)));

		assert(derived);

		TypeBase *curr = derived->value;

		while(curr)
		{
			if(curr == base->value)
				return new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(curr), object->ptr, object->ptr + curr->size);

			if(TypeClass *classType = getType<TypeClass>(curr))
				curr = classType->baseClass;
			else
				curr = NULL;
		}

		return Report(ctx, "ERROR: cannot convert from '%.*s' to '%.*s'", FMT_ISTR(derived->value->name), FMT_ISTR(base->value->name));
	}
	else if(ptr->data->name->name == InplaceStr("__closeUpvalue") && arguments.size() == 4)
	{
		ExprPointerLiteral *upvalueListLocation = getType<ExprPointerLiteral>(arguments[0]);
		ExprPointerLiteral *variableLocation = getType<ExprPointerLiteral>(arguments[1]);
		ExprIntegerLiteral *offsetToCopy = getType<ExprIntegerLiteral>(arguments[2]);
		ExprIntegerLiteral *copySize = getType<ExprIntegerLiteral>(arguments[3]);

		assert(upvalueListLocation);
		assert(variableLocation);
		assert(offsetToCopy);
		assert(copySize);

		ExprBase *upvalueListHeadBase = CreateLoad(ctx, upvalueListLocation);

		// Nothing to close if the list is empty
		if(getType<ExprNullptrLiteral>(upvalueListHeadBase))
			return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));

		ExprPointerLiteral *upvalueListHead = getType<ExprPointerLiteral>(upvalueListHeadBase);

		assert(upvalueListHead);

		struct Upvalue
		{
			void *target;
			Upvalue *next;
		};

		Upvalue *upvalue = (Upvalue*)upvalueListHead->ptr;

		assert(upvalue);

		while (upvalue && ReadVmMemoryPointer(&upvalue->target) == variableLocation->ptr)
		{
			Upvalue *next = (Upvalue*)ReadVmMemoryPointer(&upvalue->next);

			unsigned char *copy = (unsigned char*)upvalue + offsetToCopy->value;
			memcpy(copy, variableLocation->ptr, unsigned(copySize->value));
			WriteVmMemoryPointer(&upvalue->target, copy);
			WriteVmMemoryPointer(&upvalue->next, NULL);

			upvalue = next;
		}

		if(!CreateStore(ctx, upvalueListLocation, new (ctx.ctx.get<ExprPointerLiteral>()) ExprPointerLiteral(expression->source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid), (unsigned char*)upvalue, (unsigned char*)upvalue + NULLC_PTR_SIZE)))
			return NULL;

		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}

	return NULL;
}

ExprBase* EvaluateFunctionCall(ExpressionEvalContext &ctx, ExprFunctionCall *expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return NULL;
	
	ExprBase *function = Evaluate(ctx, expression->function);

	if(!function)
		return NULL;

	SmallArray<ExprBase*, 8> arguments(ctx.ctx.allocator);

	for(ExprBase *curr = expression->arguments.head; curr; curr = curr->next)
	{
		ExprBase *value = Evaluate(ctx, curr);

		if(!value)
			return NULL;

		arguments.push_back(value);
	}

	ExprFunctionLiteral *ptr = getType<ExprFunctionLiteral>(function);

	if(!ptr)
		return NULL;

	if(!ptr->data)
		return Report(ctx, "ERROR: null function pointer call");

	if(!ptr->data->declaration)
	{
		if(ctx.emulateKnownExternals && ctx.ctx.GetFunctionIndex(ptr->data) < ctx.ctx.baseModuleFunctionCount)
		{
			if(ExprBase *result = EvaluateKnownExternalFunctionCall(ctx, expression, ptr, arguments))
				return result;
		}

		return Report(ctx, "ERROR: function '%.*s' has no source", FMT_ISTR(ptr->data->name->name));
	}

	if(ptr->data->isPrototype)
		return Report(ctx, "ERROR: function '%.*s' has no source", FMT_ISTR(ptr->data->name->name));

	ExprFunctionDefinition *declaration = getType<ExprFunctionDefinition>(ptr->data->declaration);

	assert(declaration);

	if(declaration->arguments.size() != arguments.size())
		return NULL;

	ExprBase *call = EvaluateFunction(ctx, declaration, ptr->context, arguments);

	if(!call)
		return NULL;

	return CheckType(expression, call);
}

ExprBase* EvaluateIfElse(ExpressionEvalContext &ctx, ExprIfElse *expression)
{
	if(ctx.stackFrames.back()->targetYield)
		return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));

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

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateFor(ExpressionEvalContext &ctx, ExprFor *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	if(!frame->targetYield)
	{
		if(!Evaluate(ctx, expression->initializer))
			return NULL;
	}

	for(;;)
	{
		if(!AddInstruction(ctx))
			return NULL;

		if(!frame->targetYield)
		{
			if(!expression->condition)
				return NULL;

			ExprBase *condition = Evaluate(ctx, expression->condition);

			if(!condition)
				return NULL;

			long long result;
			if(!TryTakeLong(condition, result))
				return Report(ctx, "ERROR: failed to evaluate 'for' condition");

			if(!result)
				break;
		}

		if(!Evaluate(ctx, expression->body))
			return NULL;

		// On break, decrease depth and exit
		if(frame->breakDepth)
		{
			frame->breakDepth--;
			break;
		}

		// On continue, decrease depth and proceed to next iteration, unless it's a multi-level continue
		if(frame->continueDepth)
		{
			frame->continueDepth--;

			if(frame->continueDepth)
				break;
		}

		if(frame->returnValue)
			break;

		if(!frame->targetYield)
		{
			if(!Evaluate(ctx, expression->increment))
				return NULL;
		}
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateWhile(ExpressionEvalContext &ctx, ExprWhile *expression)
{
	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	for(;;)
	{
		if(!AddInstruction(ctx))
			return NULL;

		if(!frame->targetYield)
		{
			ExprBase *condition = Evaluate(ctx, expression->condition);

			if(!condition)
				return NULL;

			long long result;
			if(!TryTakeLong(condition, result))
				return Report(ctx, "ERROR: failed to evaluate 'while' condition");

			if(!result)
				break;
		}

		if(!Evaluate(ctx, expression->body))
			return NULL;

		// On break, decrease depth and exit
		if(frame->breakDepth)
		{
			frame->breakDepth--;
			break;
		}

		// On continue, decrease depth and proceed to next iteration, unless it's a multi-level continue
		if(frame->continueDepth)
		{
			frame->continueDepth--;

			if(frame->continueDepth)
				break;
		}

		if(frame->returnValue)
			break;
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateDoWhile(ExpressionEvalContext &ctx, ExprDoWhile *expression)
{
	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	for(;;)
	{
		if(!AddInstruction(ctx))
			return NULL;

		if(!Evaluate(ctx, expression->body))
			return NULL;

		// On break, decrease depth and exit
		if(frame->breakDepth)
		{
			frame->breakDepth--;
			break;
		}

		// On continue, decrease depth and proceed to next iteration, unless it's a multi-level continue
		if(frame->continueDepth)
		{
			frame->continueDepth--;

			if(frame->continueDepth)
				break;
		}

		if(frame->returnValue)
			break;

		if(!frame->targetYield)
		{
			ExprBase *condition = Evaluate(ctx, expression->condition);

			if(!condition)
				return NULL;

			long long result;
			if(!TryTakeLong(condition, result))
				return Report(ctx, "ERROR: failed to evaluate 'do' condition");

			if(!result)
				break;
		}
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateSwitch(ExpressionEvalContext &ctx, ExprSwitch *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	if(frame->targetYield)
		return Report(ctx, "ERROR: can't yield back into a switch statement");

	ExprBase *condition = Evaluate(ctx, expression->condition);

	if(!condition)
		return NULL;

	bool matched = false;

	for(ExprBase *currCase = expression->cases.head, *currBlock = expression->blocks.head; currCase && currBlock; currCase = currCase->next, currBlock = currBlock->next)
	{
		if(!AddInstruction(ctx))
			return NULL;

		if(!matched)
		{
			ExprBase *value = Evaluate(ctx, currCase);

			if(!value)
				return NULL;

			long long result;
			if(!TryTakeLong(value, result))
				return Report(ctx, "ERROR: failed to evaluate 'case' value");

			// Try next case
			if(!result)
				continue;

			matched = true;
		}

		if(!Evaluate(ctx, currBlock))
			return NULL;

		// On break, decrease depth and exit
		if(frame->breakDepth)
		{
			frame->breakDepth--;

			return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
		}
	}

	if(expression->defaultBlock)
	{
		if(!Evaluate(ctx, expression->defaultBlock))
			return NULL;

		// On break, decrease depth and exit
		if(frame->breakDepth)
		{
			frame->breakDepth--;

			return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
		}
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateBreak(ExpressionEvalContext &ctx, ExprBreak *expression)
{
	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	if(!frame->targetYield)
	{
		assert(frame->breakDepth == 0);

		frame->breakDepth = expression->depth;
	}

	if(expression->closures)
	{
		if(!Evaluate(ctx, expression->closures))
			return NULL;
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateContinue(ExpressionEvalContext &ctx, ExprContinue *expression)
{
	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	if(!frame->targetYield)
	{
		assert(frame->continueDepth == 0);

		frame->continueDepth = expression->depth;
	}

	if(expression->closures)
	{
		if(!Evaluate(ctx, expression->closures))
			return NULL;
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateBlock(ExpressionEvalContext &ctx, ExprBlock *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		if(!Evaluate(ctx, value))
			return NULL;

		if(frame->continueDepth || frame->breakDepth || frame->returnValue)
			return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
	}

	if(expression->closures)
	{
		if(!Evaluate(ctx, expression->closures))
			return NULL;
	}

	return CheckType(expression, new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid));
}

ExprBase* EvaluateSequence(ExpressionEvalContext &ctx, ExprSequence *expression)
{
	if(!AddInstruction(ctx))
		return NULL;

	ExprBase *result = new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	for(unsigned i = 0; i < expression->expressions.size(); i++)
	{
		result = Evaluate(ctx, expression->expressions[i]);

		if(!result)
			return NULL;
	}

	if(!ctx.stackFrames.empty() && ctx.stackFrames.back()->targetYield)
		return new (ctx.ctx.get<ExprVoid>()) ExprVoid(expression->source, ctx.ctx.typeVoid);

	return CheckType(expression, result);
}

ExprBase* EvaluateModule(ExpressionEvalContext &ctx, ExprModule *expression)
{
	ctx.globalFrame = new (ctx.ctx.get<ExpressionEvalContext::StackFrame>()) ExpressionEvalContext::StackFrame(ctx.ctx.allocator, NULL);
	ctx.stackFrames.push_back(ctx.globalFrame);

	ExpressionEvalContext::StackFrame *frame = ctx.stackFrames.back();

	for(ExprBase *value = expression->setup.head; value; value = value->next)
	{
		if(!Evaluate(ctx, value))
			return NULL;
	}

	for(ExprBase *value = expression->expressions.head; value; value = value->next)
	{
		if(!Evaluate(ctx, value))
			return NULL;

		assert(frame->breakDepth == 0 && frame->continueDepth == 0);

		if(frame->returnValue)
			return frame->returnValue;
	}

	ctx.stackFrames.pop_back();

	assert(ctx.stackFrames.empty());

	return NULL;
}

ExprBase* Evaluate(ExpressionEvalContext &ctx, ExprBase *expression)
{
	if(isType<TypeError>(expression->type))
		return NULL;

	ctx.expressionDepth++;

	if(ctx.expressionDepth > ctx.expressionDepthLimit)
		return Report(ctx, "ERROR: expression depth limit reached");

	ExprBase *result = NULL;

	switch(expression->typeID)
	{
	case ExprError::myTypeID:
		result = Report(ctx, "ERROR: invalid expression");
		break;
	case ExprErrorTypeMemberAccess::myTypeID:
		result = Report(ctx, "ERROR: invalid expression");
		break;
	case ExprVoid::myTypeID:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprBoolLiteral::myTypeID:
		result = EvaluateBoolLiteral(ctx, (ExprBoolLiteral*)expression);
		break;
	case ExprCharacterLiteral::myTypeID:
		result = EvaluateCharacterLiteral(ctx, (ExprCharacterLiteral*)expression);
		break;
	case ExprStringLiteral::myTypeID:
		result = EvaluateStringLiteral(ctx, (ExprStringLiteral*)expression);
		break;
	case ExprIntegerLiteral::myTypeID:
		result = EvaluateIntegerLiteral(ctx, (ExprIntegerLiteral*)expression);
		break;
	case ExprRationalLiteral::myTypeID:
		result = EvaluateRationalLiteral(ctx, (ExprRationalLiteral*)expression);
		break;
	case ExprTypeLiteral::myTypeID:
		result = EvaluateTypeLiteral(ctx, (ExprTypeLiteral*)expression);
		break;
	case ExprNullptrLiteral::myTypeID:
		result = EvaluateNullptrLiteral(ctx, (ExprNullptrLiteral*)expression);
		break;
	case ExprFunctionIndexLiteral::myTypeID:
		result = EvaluateFunctionIndexLiteral(ctx, (ExprFunctionIndexLiteral*)expression);
		break;
	case ExprPassthrough::myTypeID:
		result = Evaluate(ctx, ((ExprPassthrough*)expression)->value);
		break;
	case ExprArray::myTypeID:
		result = EvaluateArray(ctx, (ExprArray*)expression);
		break;
	case ExprPreModify::myTypeID:
		result = EvaluatePreModify(ctx, (ExprPreModify*)expression);
		break;
	case ExprPostModify::myTypeID:
		result = EvaluatePostModify(ctx, (ExprPostModify*)expression);
		break;
	case ExprTypeCast::myTypeID:
		result = EvaluateCast(ctx, (ExprTypeCast*)expression);
		break;
	case ExprUnaryOp::myTypeID:
		result = EvaluateUnaryOp(ctx, (ExprUnaryOp*)expression);
		break;
	case ExprBinaryOp::myTypeID:
		result = EvaluateBinaryOp(ctx, (ExprBinaryOp*)expression);
		break;
	case ExprGetAddress::myTypeID:
		result = EvaluateGetAddress(ctx, (ExprGetAddress*)expression);
		break;
	case ExprDereference::myTypeID:
		result = EvaluateDereference(ctx, (ExprDereference*)expression);
		break;
	case ExprUnboxing::myTypeID:
		result = Evaluate(ctx, ((ExprUnboxing*)expression)->value);
		break;
	case ExprConditional::myTypeID:
		result = EvaluateConditional(ctx, (ExprConditional*)expression);
		break;
	case ExprAssignment::myTypeID:
		result = EvaluateAssignment(ctx, (ExprAssignment*)expression);
		break;
	case ExprMemberAccess::myTypeID:
		result = EvaluateMemberAccess(ctx, (ExprMemberAccess*)expression);
		break;
	case ExprArrayIndex::myTypeID:
		result = EvaluateArrayIndex(ctx, (ExprArrayIndex*)expression);
		break;
	case ExprReturn::myTypeID:
		result = EvaluateReturn(ctx, (ExprReturn*)expression);
		break;
	case ExprYield::myTypeID:
		result = EvaluateYield(ctx, (ExprYield*)expression);
		break;
	case ExprVariableDefinition::myTypeID:
		result = EvaluateVariableDefinition(ctx, (ExprVariableDefinition*)expression);
		break;
	case ExprZeroInitialize::myTypeID:
		result = EvaluateZeroInitialize(ctx, (ExprZeroInitialize*)expression);
		break;
	case ExprArraySetup::myTypeID:
		result = EvaluateArraySetup(ctx, (ExprArraySetup*)expression);
		break;
	case ExprVariableDefinitions::myTypeID:
		result = EvaluateVariableDefinitions(ctx, (ExprVariableDefinitions*)expression);
		break;
	case ExprVariableAccess::myTypeID:
		result = EvaluateVariableAccess(ctx, (ExprVariableAccess*)expression);
		break;
	case ExprFunctionContextAccess::myTypeID:
		result = EvaluateFunctionContextAccess(ctx, (ExprFunctionContextAccess*)expression);
		break;
	case ExprFunctionDefinition::myTypeID:
		result = EvaluateFunctionDefinition(ctx, (ExprFunctionDefinition*)expression);
		break;
	case ExprGenericFunctionPrototype::myTypeID:
		result = EvaluateGenericFunctionPrototype(ctx, (ExprGenericFunctionPrototype*)expression);
		break;
	case ExprFunctionAccess::myTypeID:
		result = EvaluateFunctionAccess(ctx, (ExprFunctionAccess*)expression);
		break;
	case ExprFunctionOverloadSet::myTypeID:
		result = NULL;
		break;
	case ExprShortFunctionOverloadSet::myTypeID:
		result = NULL;
		break;
	case ExprFunctionCall::myTypeID:
		result = EvaluateFunctionCall(ctx, (ExprFunctionCall*)expression);
		break;
	case ExprAliasDefinition::myTypeID:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprClassPrototype::myTypeID:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprGenericClassPrototype::myTypeID:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprClassDefinition::myTypeID:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprEnumDefinition::myTypeID:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprIfElse::myTypeID:
		result = EvaluateIfElse(ctx, (ExprIfElse*)expression);
		break;
	case ExprFor::myTypeID:
		result = EvaluateFor(ctx, (ExprFor*)expression);
		break;
	case ExprWhile::myTypeID:
		result = EvaluateWhile(ctx, (ExprWhile*)expression);
		break;
	case ExprDoWhile::myTypeID:
		result = EvaluateDoWhile(ctx, (ExprDoWhile*)expression);
		break;
	case ExprSwitch::myTypeID:
		result = EvaluateSwitch(ctx, (ExprSwitch*)expression);
		break;
	case ExprBreak::myTypeID:
		result = EvaluateBreak(ctx, (ExprBreak*)expression);
		break;
	case ExprContinue::myTypeID:
		result = EvaluateContinue(ctx, (ExprContinue*)expression);
		break;
	case ExprBlock::myTypeID:
		result = EvaluateBlock(ctx, (ExprBlock*)expression);
		break;
	case ExprSequence::myTypeID:
		result = EvaluateSequence(ctx, (ExprSequence*)expression);
		break;
	case ExprModule::myTypeID:
		result = EvaluateModule(ctx, (ExprModule*)expression);
		break;
	default:
		assert(!"unknown type");
		break;
	}

	ctx.expressionDepth--;

	return result;
}

bool EvaluateToBuffer(ExpressionEvalContext &ctx, ExprBase *expression, char *resultBuf, unsigned resultBufSize)
{
	if(ExprBase *value = Evaluate(ctx, expression))
	{
		if(ExprBoolLiteral *result = getType<ExprBoolLiteral>(value))
		{
			NULLC::SafeSprintf(resultBuf, resultBufSize, "%d", result->value ? 1 : 0);
		}
		else if(ExprCharacterLiteral *result = getType<ExprCharacterLiteral>(value))
		{
			NULLC::SafeSprintf(resultBuf, resultBufSize, "%d", result->value);
		}
		else if(ExprIntegerLiteral *result = getType<ExprIntegerLiteral>(value))
		{
			if(result->type == ctx.ctx.typeLong)
				NULLC::SafeSprintf(resultBuf, resultBufSize, "%lldL", result->value);
			else
				NULLC::SafeSprintf(resultBuf, resultBufSize, "%d", int(result->value));
		}
		else if(ExprRationalLiteral *result = getType<ExprRationalLiteral>(value))
		{
			NULLC::SafeSprintf(resultBuf, resultBufSize, "%f", result->value);
		}
		else
		{
			NULLC::SafeSprintf(resultBuf, resultBufSize, "unknown");
		}

		return true;
	}

	return false;
}

bool TestEvaluation(ExpressionContext &ctx, ExprBase *expression, char *resultBuf, unsigned resultBufSize, char *errorBuf, unsigned errorBufSize)
{
	if(!expression)
		return false;

	ExpressionEvalContext evalCtx(ctx, ctx.allocator);

	evalCtx.errorBuf = errorBuf;
	evalCtx.errorBufSize = errorBufSize;

	evalCtx.emulateKnownExternals = true;

	return EvaluateToBuffer(evalCtx, expression, resultBuf, resultBufSize);
}
