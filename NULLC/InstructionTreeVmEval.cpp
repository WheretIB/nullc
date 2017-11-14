#include "InstructionTreeVmEval.h"

#include <math.h>

#include "ExpressionTree.h"
#include "TypeTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin
#define allocate(T) new (ctx.get<T>()) T

typedef InstructionVMEvalContext Eval;

const unsigned memoryStorageBits = 16u;
const unsigned memoryOffsetMask = (1u << memoryStorageBits) - 1;
const unsigned memoryStorageMask = ~0u & ~memoryOffsetMask;

namespace
{
	unsigned GetFunctionIndex(VmModule *module, VmFunction *function)
	{
		unsigned index = ~0u;

		for(unsigned i = 0, e = module->functions.size(); i < e; i++)
		{
			if(module->functions[i] == function)
			{
				index = i;
				break;
			}
		}

		assert(index != ~0u);

		return index;
	}

	int GetIntPow(int number, int power)
	{
		if(power < 0)
			return number == 1 ? 1 : (number == -1 ? (power & 1 ? -1 : 1) : 0);

		int result = 1;
		while(power)
		{
			if(power & 1)
			{
				result *= number;
				power--;
			}
			number *= number;
			power >>= 1;
		}
		return result;
	}

	long long GetLongPow(long long number, long long power)
	{
		if(power < 0)
			return number == 1 ? 1 : (number == -1 ? (power & 1 ? -1 : 1) : 0);

		long long result = 1;
		while(power)
		{
			if(power & 1)
			{
				result *= number;
				power--;
			}
			number *= number;
			power >>= 1;
		}
		return result;
	}

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

	bool HasReport(Eval &ctx)
	{
		return *ctx.errorBuf != 0;
	}
}

bool Eval::Storage::Reserve(Eval &ctx, unsigned offset, unsigned size)
{
	unsigned oldSize = data.size();
	unsigned newSize = offset + size;

	if(newSize >= ctx.frameMemoryLimit)
		return false;

	if(newSize > oldSize)
	{
		data.resize(newSize + 32);

		memset(&data[oldSize], 0, data.size() - oldSize);
	}

	return true;
}

void Eval::StackFrame::AssignRegister(unsigned id, VmConstant *constant)
{
	assert(id != 0);

	unsigned oldSize = instructionValues.size();
	unsigned newSize = id;

	if(newSize > oldSize)
	{
		instructionValues.resize(newSize + 32);

		memset(&instructionValues[oldSize], 0, (instructionValues.size() - oldSize) * sizeof(instructionValues[0]));
	}

	instructionValues[id - 1] = constant;
}

VmConstant* Eval::StackFrame::ReadRegister(unsigned id)
{
	assert(id != 0);

	if(id - 1 < instructionValues.size())
		return instructionValues[id - 1];

	return 0;
}

VmConstant* LoadFrameByte(Eval &ctx, Eval::Storage *storage, unsigned offset)
{
	char value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	return CreateConstantInt(ctx.allocator, value);
}

VmConstant* LoadFrameShort(Eval &ctx, Eval::Storage *storage, unsigned offset)
{
	short value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	return CreateConstantInt(ctx.allocator, value);
}

VmConstant* LoadFrameInt(Eval &ctx, Eval::Storage *storage, unsigned offset, VmType type)
{
	int value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	if(type.type == VM_TYPE_POINTER)
		return CreateConstantPointer(ctx.allocator, value, NULL, type.structType, false);

	return CreateConstantInt(ctx.allocator, value);
}

VmConstant* LoadFrameFloat(Eval &ctx, Eval::Storage *storage, unsigned offset)
{
	float value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	return CreateConstantDouble(ctx.allocator, value);
}

VmConstant* LoadFrameDouble(Eval &ctx, Eval::Storage *storage, unsigned offset)
{
	double value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	return CreateConstantDouble(ctx.allocator, value);
}

VmConstant* LoadFrameLong(Eval &ctx, Eval::Storage *storage, unsigned offset, VmType type)
{
	long long value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	if(type.type == VM_TYPE_POINTER)
	{
		assert(unsigned(value) == value);

		return CreateConstantPointer(ctx.allocator, unsigned(value), NULL, type.structType, false);
	}

	return CreateConstantLong(ctx.allocator, value);
}

unsigned GetAllocaAddress(Eval &ctx, VariableData *container)
{
	Eval::StackFrame *frame = ctx.stackFrames.back();

	unsigned offset = 8;

	for(unsigned i = 0; i < frame->owner->allocas.size(); i++)
	{
		VariableData *data = frame->owner->allocas[i];

		if(container == data)
			return offset;

		offset += unsigned(data->type->size);
	}

	return 0;
}

unsigned GetStorageIndex(Eval &ctx, Eval::Storage *storage)
{
	if(storage->index != 0)
		return storage->index;

	ctx.storageSet.push_back(storage);
	storage->index = ctx.storageSet.size();

	return storage->index;
}

void CopyConstantRaw(Eval &ctx, char *dst, unsigned dstSize, VmConstant *src, unsigned storeSize)
{
	Eval::StackFrame *frame = ctx.stackFrames.back();

	assert(dstSize >= storeSize);

	if(src->type == VmType::Int)
	{
		if(storeSize == 1)
		{
			char tmp = (char)src->iValue;
			memcpy(dst, &tmp, storeSize);
		}
		else if(storeSize == 2)
		{
			short tmp = (short)src->iValue;
			memcpy(dst, &tmp, storeSize);
		}
		else if(storeSize == 4)
		{
			memcpy(dst, &src->iValue, storeSize);
		}
		else
		{
			assert(!"invalid store size");
		}
	}
	else if(src->type == VmType::Double)
	{
		if(storeSize == 4)
		{
			float tmp = (float)src->dValue;
			memcpy(dst, &tmp, storeSize);
		}
		else if(storeSize == 8)
		{
			memcpy(dst, &src->dValue, storeSize);
		}
		else
		{
			assert(!"invalid store size");
		}
	}
	else if(src->type == VmType::Long)
	{
		assert(src->type.size == 8);

		memcpy(dst, &src->lValue, src->type.size);
	}
	else if(src->type.type == VM_TYPE_POINTER)
	{
		assert(src->type.size == sizeof(void*));

		unsigned long long pointer = 0;

		if(src->container)
		{
			if(unsigned offset = GetAllocaAddress(ctx, src->container))
			{
				pointer = src->iValue + offset;
				assert((pointer & memoryOffsetMask) == pointer);
				pointer |= GetStorageIndex(ctx, &frame->allocas) << memoryStorageBits;
			}
			else if(IsGlobalScope(src->container->scope))
			{
				pointer = src->iValue + src->container->offset;
				assert((pointer & memoryOffsetMask) == pointer);
				pointer |= GetStorageIndex(ctx, &ctx.globalFrame->stack) << memoryStorageBits;
			}
			else
			{
				pointer = src->iValue + src->container->offset;
				assert((pointer & memoryOffsetMask) == pointer);
				pointer |= GetStorageIndex(ctx, &frame->stack) << memoryStorageBits;
			}
		}
		else
		{
			pointer = src->iValue;

			if(pointer == 0)
			{
				assert((pointer & memoryOffsetMask) == pointer);
				pointer |= GetStorageIndex(ctx, &ctx.heap) << memoryStorageBits;
			}
		}

		memcpy(dst, &pointer, src->type.size);
	}
	else if(src->sValue)
	{
		memcpy(dst, src->sValue, src->type.size);
	}
	else if(src->fValue)
	{
		assert(src->type.size == 4);

		unsigned index = GetFunctionIndex(ctx.module, src->fValue);

		memcpy(dst, &index, src->type.size);
	}
	else
	{
		assert(!"unknown constant type");
	}
}

VmConstant* EvaluateOperand(Eval &ctx, VmValue *value)
{
	Eval::StackFrame *frame = ctx.stackFrames.back();

	if(VmConstant *constant = getType<VmConstant>(value))
		return constant;

	if(VmInstruction *instruction = getType<VmInstruction>(value))
		return frame->ReadRegister(instruction->uniqueId);

	if(VmBlock *block = getType<VmBlock>(value))
	{
		VmConstant *result = allocate(VmConstant)(ctx.allocator, VmType::Block);

		result->bValue = block;

		return result;
	}

	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmConstant *result = allocate(VmConstant)(ctx.allocator, VmType::Function);

		result->fValue = function;

		return result;
	}

	assert(!"unknown type");

	return NULL;
}

Eval::Storage* FindTarget(Eval &ctx, VmConstant *value, unsigned &base)
{
	Eval::StackFrame *frame = ctx.stackFrames.back();

	Eval::Storage *target = NULL;

	base = 0;

	if(VariableData *variable = value->container)
	{
		if(variable->imported)
			return NULL;

		if(unsigned address = GetAllocaAddress(ctx, variable))
		{
			target = &frame->allocas;
			base = address;
		}
		else if(IsGlobalScope(variable->scope))
		{
			target = &ctx.globalFrame->stack;
			base = variable->offset;
		}
		else
		{
			target = &frame->stack;
			base = variable->offset;
		}
	}
	else if(unsigned storageIndex = (value->iValue & memoryStorageMask) >> memoryStorageBits)
	{
		target = ctx.storageSet[storageIndex - 1];
	}

	return target;
}

VmConstant* EvaluateInstruction(Eval &ctx, VmInstruction *instruction, VmBlock *predecessor, VmBlock **nextBlock)
{
	ctx.instruction++;

	if(ctx.instruction >= ctx.instructionsLimit)
		return (VmConstant*)Report(ctx, "ERROR: instruction limit reached");

	SmallArray<VmConstant*, 8> arguments(ctx.allocator);

	for(unsigned i = 0; i < instruction->arguments.size(); i++)
		arguments.push_back(EvaluateOperand(ctx, instruction->arguments[i]));

	switch(instruction->cmd)
	{
	case VM_INST_LOAD_BYTE:
		{
			unsigned base = 0;

			if(Eval::Storage *target = FindTarget(ctx, arguments[0], base))
				return LoadFrameByte(ctx, target, (arguments[0]->iValue & memoryOffsetMask) + base);
		}

		return (VmConstant*)Report(ctx, "ERROR: heap memory access");
	case VM_INST_LOAD_SHORT:
		{
			unsigned base = 0;

			if(Eval::Storage *target = FindTarget(ctx, arguments[0], base))
				return LoadFrameShort(ctx, target, (arguments[0]->iValue & memoryOffsetMask) + base);
		}

		return (VmConstant*)Report(ctx, "ERROR: heap memory access");
	case VM_INST_LOAD_INT:
		{
			unsigned base = 0;

			if(Eval::Storage *target = FindTarget(ctx, arguments[0], base))
				return LoadFrameInt(ctx, target, (arguments[0]->iValue & memoryOffsetMask) + base, instruction->type);
		}

		return (VmConstant*)Report(ctx, "ERROR: heap memory access");
	case VM_INST_LOAD_FLOAT:
		{
			unsigned base = 0;

			if(Eval::Storage *target = FindTarget(ctx, arguments[0], base))
				return LoadFrameFloat(ctx, target, (arguments[0]->iValue & memoryOffsetMask) + base);
		}

		return (VmConstant*)Report(ctx, "ERROR: heap memory access");
	case VM_INST_LOAD_DOUBLE:
		{
			unsigned base = 0;

			if(Eval::Storage *target = FindTarget(ctx, arguments[0], base))
				return LoadFrameDouble(ctx, target, (arguments[0]->iValue & memoryOffsetMask) + base);
		}

		return (VmConstant*)Report(ctx, "ERROR: heap memory access");
	case VM_INST_LOAD_LONG:
		{
			unsigned base = 0;

			if(Eval::Storage *target = FindTarget(ctx, arguments[0], base))
				return LoadFrameLong(ctx, target, (arguments[0]->iValue & memoryOffsetMask) + base, instruction->type);
		}

		return (VmConstant*)Report(ctx, "ERROR: heap memory access");
	case VM_INST_LOAD_STRUCT:
		{
			unsigned base = 0;

			if(Eval::Storage *target = FindTarget(ctx, arguments[0], base))
			{
				unsigned offset = (arguments[0]->iValue & memoryOffsetMask) + base;

				unsigned size = instruction->type.size;

				char *value = (char*)ctx.allocator->alloc(size);
				memset(value, 0, size);

				if(!target->Reserve(ctx, offset, size))
					return (VmConstant*)Report(ctx, "ERROR: out of stack space");

				memcpy(value, target->data.data + offset, size);

				VmConstant *result = allocate(VmConstant)(ctx.allocator, instruction->type);

				result->sValue = value;

				return result;
			}
		}

		return (VmConstant*)Report(ctx, "ERROR: heap memory access");
	case VM_INST_LOAD_IMMEDIATE:
		return arguments[0];
	case VM_INST_STORE_BYTE:
	case VM_INST_STORE_SHORT:
	case VM_INST_STORE_INT:
	case VM_INST_STORE_FLOAT:
	case VM_INST_STORE_DOUBLE:
	case VM_INST_STORE_LONG:
	case VM_INST_STORE_STRUCT:
		{
			unsigned base = 0;

			if(Eval::Storage *target = FindTarget(ctx, arguments[0], base))
			{
				unsigned offset = (arguments[0]->iValue & memoryOffsetMask) + base;

				if(!target->Reserve(ctx, offset, arguments[1]->type.size))
					return (VmConstant*)Report(ctx, "ERROR: out of stack space");

				CopyConstantRaw(ctx, target->data.data + offset, target->data.size() - offset, arguments[1], GetAccessSize(instruction));
			}
			else
			{
				Report(ctx, "ERROR: heap memory access");
			}
		}

		return NULL;
	case VM_INST_DOUBLE_TO_INT:
		return CreateConstantInt(ctx.allocator, int(arguments[0]->dValue));
	case VM_INST_DOUBLE_TO_LONG:
		return CreateConstantLong(ctx.allocator, (long long)(arguments[0]->dValue));
	case VM_INST_INT_TO_DOUBLE:
		return CreateConstantDouble(ctx.allocator, double(arguments[0]->iValue));
	case VM_INST_LONG_TO_DOUBLE:
		return CreateConstantDouble(ctx.allocator, double(arguments[0]->lValue));
	case VM_INST_INT_TO_LONG:
		return CreateConstantLong(ctx.allocator, (long long)(arguments[0]->iValue));
	case VM_INST_LONG_TO_INT:
		return CreateConstantInt(ctx.allocator, int(arguments[0]->lValue));
	case VM_INST_INDEX:
		{
			VmConstant *arrayLength = arguments[0];
			VmConstant *elementSize = arguments[1];
			VmConstant *value = arguments[2];
			VmConstant *index = arguments[3];

			assert(arrayLength->type == VmType::Int);
			assert(elementSize->type == VmType::Int);
			assert(value->type.type == VM_TYPE_POINTER);
			assert(index->type == VmType::Int);

			if(unsigned(index->iValue) >= unsigned(arrayLength->iValue))
				return (VmConstant*)Report(ctx, "ERROR: array index out of bounds");

			return CreateConstantPointer(ctx.allocator, value->iValue + index->iValue * elementSize->iValue, value->container, instruction->type.structType, false);
		}

		break;
	case VM_INST_INDEX_UNSIZED:
		{
			VmConstant *elementSize = arguments[0];
			VmConstant *value = arguments[1];
			VmConstant *index = arguments[2];

			assert(value->type.type == VM_TYPE_ARRAY_REF && value->sValue);
			assert(elementSize->type == VmType::Int);
			assert(index->type == VmType::Int);

			unsigned long long pointer = 0;
			memcpy(&pointer, value->sValue + 0, sizeof(void*));

			unsigned length = 0;
			memcpy(&length, value->sValue + sizeof(void*), 4);

			if(unsigned(index->iValue) >= length)
				return (VmConstant*)Report(ctx, "ERROR: array index out of bounds");

			assert(unsigned(pointer) == pointer);

			return CreateConstantPointer(ctx.allocator, unsigned(pointer) + index->iValue * elementSize->iValue, NULL, instruction->type.structType, false);
		}

		break;
	case VM_INST_FUNCTION_ADDRESS:
		break;
	case VM_INST_TYPE_ID:
		return arguments[0];
	case VM_INST_SET_RANGE:
		break;
	case VM_INST_JUMP:
		assert(arguments[0]->type == VmType::Block && arguments[0]->bValue);

		*nextBlock = arguments[0]->bValue;

		return NULL;
	case VM_INST_JUMP_Z:
		assert(arguments[0]->type == VmType::Int);
		assert(arguments[1]->type == VmType::Block && arguments[1]->bValue);
		assert(arguments[2]->type == VmType::Block && arguments[2]->bValue);

		*nextBlock = arguments[0]->iValue == 0 ? arguments[1]->bValue : arguments[2]->bValue;

		return NULL;
	case VM_INST_JUMP_NZ:
		assert(arguments[0]->type == VmType::Int);
		assert(arguments[1]->type == VmType::Block && arguments[1]->bValue);
		assert(arguments[2]->type == VmType::Block && arguments[2]->bValue);

		*nextBlock = arguments[0]->iValue != 0 ? arguments[1]->bValue : arguments[2]->bValue;

		return NULL;
	case VM_INST_CALL:
		{
			assert(arguments[0]->type.type == VM_TYPE_FUNCTION_REF);

			unsigned functionIndex = 0;
			memcpy(&functionIndex, arguments[0]->sValue + 0, 4);

			if(functionIndex >= ctx.module->functions.size())
				return (VmConstant*)Report(ctx, "ERROR: invalid function index");

			VmFunction *function = ctx.module->functions[functionIndex];

			unsigned long long context = 0;
			memcpy(&context, arguments[0]->sValue + 4, sizeof(void*));

			Eval::StackFrame *calleeFrame = allocate(Eval::StackFrame)(ctx.allocator, function);

			if(ctx.stackFrames.size() >= ctx.stackDepthLimit)
				return (VmConstant*)Report(ctx, "ERROR: stack depth limit");

			unsigned offset = 0;

			if(!calleeFrame->stack.Reserve(ctx, offset, sizeof(void*)))
				return (VmConstant*)Report(ctx, "ERROR: out of stack space");

			memcpy(calleeFrame->stack.data.data + offset, &context, sizeof(void*));
			offset += sizeof(void*);

			for(unsigned i = 1; i < arguments.size(); i++)
			{
				VmConstant *argument = arguments[i];

				ArgumentData &original = function->function->arguments[i - 1];

				if(original.type->size == 0)
					continue;

				if(original.type == ctx.ctx.typeFloat)
					return (VmConstant*)Report(ctx, "ERROR: unsupported float argument");

				unsigned argumentSize = argument->type.size;

				if(!calleeFrame->stack.Reserve(ctx, offset, argumentSize))
					return (VmConstant*)Report(ctx, "ERROR: out of stack space");

				CopyConstantRaw(ctx, calleeFrame->stack.data.data + offset, calleeFrame->stack.data.size() - offset, argument, argumentSize);

				offset += argumentSize > 4 ? argumentSize : 4;
			}

			ctx.stackFrames.push_back(calleeFrame);

			VmConstant *result = EvaluateFunction(ctx, function);

			ctx.stackFrames.pop_back();

			return result;
		}
		break;
	case VM_INST_RETURN:
		if(arguments.empty())
			return CreateConstantVoid(ctx.allocator);

		return arguments[0];
	case VM_INST_YIELD:
		return (VmConstant*)Report(ctx, "ERROR: yield is not supported");
	case VM_INST_ADD:
		if(arguments[0]->type.type == VM_TYPE_POINTER || arguments[1]->type.type == VM_TYPE_POINTER)
		{
			// Both arguments can't be based on an offset
			assert(!(arguments[0]->container && arguments[1]->container));

			return CreateConstantPointer(ctx.allocator, arguments[0]->iValue + arguments[1]->iValue, arguments[0]->container ? arguments[0]->container : arguments[1]->container, instruction->type.structType, false);
		}
		else
		{
			assert(arguments[0]->type == arguments[1]->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, arguments[0]->iValue + arguments[1]->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantDouble(ctx.allocator, arguments[0]->dValue + arguments[1]->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, arguments[0]->lValue + arguments[1]->lValue);
		}
		break;
	case VM_INST_SUB:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue - arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, arguments[0]->dValue - arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, arguments[0]->lValue - arguments[1]->lValue);

		break;
	case VM_INST_MUL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue * arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, arguments[0]->dValue * arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, arguments[0]->lValue * arguments[1]->lValue);

		break;
	case VM_INST_DIV:
		assert(arguments[0]->type == arguments[1]->type);

		if(IsConstantZero(arguments[1]))
			return (VmConstant*)Report(ctx, "ERROR: division by zero");

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue / arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, arguments[0]->dValue / arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, arguments[0]->lValue / arguments[1]->lValue);

		break;
	case VM_INST_POW:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, GetIntPow(arguments[0]->iValue, arguments[1]->iValue));
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, pow(arguments[0]->dValue, arguments[1]->dValue));
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, GetLongPow(arguments[0]->lValue, arguments[1]->lValue));

		break;
	case VM_INST_MOD:
		assert(arguments[0]->type == arguments[1]->type);

		if(IsConstantZero(arguments[1]))
			return (VmConstant*)Report(ctx, "ERROR: division by zero");

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue % arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, fmod(arguments[0]->dValue, arguments[1]->dValue));
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, arguments[0]->lValue % arguments[1]->lValue);

		break;
	case VM_INST_LESS:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue < arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, arguments[0]->dValue < arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, arguments[0]->lValue < arguments[1]->lValue);

		break;
	case VM_INST_GREATER:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue > arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, arguments[0]->dValue > arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, arguments[0]->lValue > arguments[1]->lValue);

		break;
	case VM_INST_LESS_EQUAL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue <= arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, arguments[0]->dValue <= arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, arguments[0]->lValue <= arguments[1]->lValue);

		break;
	case VM_INST_GREATER_EQUAL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue >= arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, arguments[0]->dValue >= arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, arguments[0]->lValue >= arguments[1]->lValue);

		break;
	case VM_INST_EQUAL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue == arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, arguments[0]->dValue == arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, arguments[0]->lValue == arguments[1]->lValue);
		else if(arguments[0]->type.type == VM_TYPE_POINTER)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue == arguments[1]->iValue && arguments[0]->container == arguments[1]->container);

		break;
	case VM_INST_NOT_EQUAL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue != arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, arguments[0]->dValue != arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, arguments[0]->lValue != arguments[1]->lValue);
		else if(arguments[0]->type.type == VM_TYPE_POINTER)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue != arguments[1]->iValue || arguments[0]->container != arguments[1]->container);

		break;
	case VM_INST_SHL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue << arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, arguments[0]->lValue << arguments[1]->lValue);
		break;
	case VM_INST_SHR:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue >> arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, arguments[0]->lValue >> arguments[1]->lValue);
		break;
	case VM_INST_BIT_AND:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue & arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, arguments[0]->lValue & arguments[1]->lValue);
		break;
	case VM_INST_BIT_OR:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue | arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, arguments[0]->lValue | arguments[1]->lValue);
		break;
	case VM_INST_BIT_XOR:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, arguments[0]->iValue ^ arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, arguments[0]->lValue ^ arguments[1]->lValue);
		break;
	case VM_INST_LOG_XOR:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, (arguments[0]->iValue != 0) != (arguments[1]->iValue != 0));
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, (arguments[0]->lValue != 0) != (arguments[1]->lValue != 0));
		break;
	case VM_INST_NEG:
		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, -arguments[0]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, -arguments[0]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, -arguments[0]->lValue);
		break;
	case VM_INST_BIT_NOT:
		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, ~arguments[0]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, ~arguments[0]->lValue);
		break;
	case VM_INST_LOG_NOT:
		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, !arguments[0]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, !arguments[0]->lValue);

		if(arguments[0]->type.type == VM_TYPE_POINTER)
		{
			unsigned storageIndex = (arguments[0]->iValue & memoryStorageMask) >> memoryStorageBits;

			return CreateConstantInt(ctx.allocator, arguments[0]->iValue == 0 || (ctx.heap.index == storageIndex && (arguments[0]->iValue & memoryOffsetMask) == 0));
		}
		break;
	case VM_INST_CREATE_CLOSURE:
		break;
	case VM_INST_CLOSE_UPVALUES:
		break;
	case VM_INST_CONVERT_POINTER:
		{
			VmConstant *value = arguments[0];
			VmConstant *typeID = arguments[1];

			assert(value->type.type == VM_TYPE_AUTO_REF && value->sValue);
			assert(typeID->type == VmType::Int);

			unsigned valueTypeID = 0;
			memcpy(&valueTypeID, value->sValue + 0, 4);

			return (VmConstant*)Report(ctx, "ERROR: pointer convertion unsupported");
		}
		break;
	case VM_INST_CHECKED_RETURN:
		break;
	case VM_INST_CONSTRUCT:
		{
			unsigned size = instruction->type.size;

			char *value = (char*)ctx.allocator->alloc(size);
			memset(value, 0, size);

			unsigned offset = 0;

			for(unsigned i = 0; i < arguments.size(); i++)
			{
				VmConstant *argument = arguments[i];

				unsigned argumentSize = argument->type.size;

				CopyConstantRaw(ctx, value + offset, size - offset, argument, argumentSize);

				offset += argumentSize > 4 ? argumentSize : 4;
			}

			VmConstant *result = allocate(VmConstant)(ctx.allocator, instruction->type);

			result->sValue = value;

			return result;
		}
		break;
	case VM_INST_ARRAY:
		{
			unsigned size = instruction->type.size;

			char *value = (char*)ctx.allocator->alloc(size);
			memset(value, 0, size);

			unsigned offset = 0;

			TypeArray *arrayType = getType<TypeArray>(instruction->type.structType);

			assert(arrayType);

			unsigned elementSize = unsigned(arrayType->subType->size);

			for(unsigned i = 0; i < arguments.size(); i++)
			{
				VmConstant *argument = arguments[i];

				CopyConstantRaw(ctx, value + offset, size - offset, argument, elementSize);

				offset += elementSize;
			}

			VmConstant *result = allocate(VmConstant)(ctx.allocator, instruction->type);

			result->sValue = value;

			return result;
		}
		break;
	case VM_INST_EXTRACT:
		{
			unsigned size = instruction->type.size;

			assert(arguments[0]->sValue);
			assert(arguments[1]->type == VmType::Int);
			assert(arguments[1]->iValue + size <= arguments[0]->type.size);

			const char *source = arguments[0]->sValue + arguments[1]->iValue;

			if(instruction->type == VmType::Int)
			{
				int value = 0;
				memcpy(&value, source, sizeof(value));
				return CreateConstantInt(ctx.allocator, value);
			}
			else if(instruction->type == VmType::Double)
			{
				double value = 0;
				memcpy(&value, source, sizeof(value));
				return CreateConstantDouble(ctx.allocator, value);
			}
			else if(instruction->type == VmType::Long)
			{
				long long value = 0;
				memcpy(&value, source, sizeof(value));
				return CreateConstantLong(ctx.allocator, value);
			}
			else if(instruction->type.type == VM_TYPE_POINTER)
			{
				unsigned long long pointer = 0;
				memcpy(&pointer, source, sizeof(void*));

				assert(unsigned(pointer) == pointer);

				return CreateConstantPointer(ctx.allocator, unsigned(pointer), NULL, instruction->type.structType, false);
			}
			else
			{
				char *value = (char*)ctx.allocator->alloc(size);
				memcpy(value, source, size);

				VmConstant *result = allocate(VmConstant)(ctx.allocator, instruction->type);

				result->sValue = value;

				return result;
			}
		}
		break;
	case VM_INST_PHI:
		{
			VmConstant *valueA = arguments[0];
			VmConstant *parentA = arguments[1];
			VmConstant *valueB = arguments[2];
			VmConstant *parentB = arguments[3];

			assert(parentA->type == VmType::Block && parentA->bValue);
			assert(parentB->type == VmType::Block && parentB->bValue);

			if(parentA->bValue == predecessor)
				return valueA;

			if(parentB->bValue == predecessor)
				return valueB;

			assert(!"phi instruction can't handle the predecessor");
		}
		break;
	default:
		assert(!"unknown instruction");
	}

	return (VmConstant*)Report(ctx, "ERROR: unsupported instruction kind");
}

unsigned GetArgumentOffset(FunctionData *data, unsigned argument)
{
	// Start at context
	unsigned offset = sizeof(void*);

	for(unsigned i = 0; i < argument; i++)
	{
		unsigned size = unsigned(data->arguments[i].type->size);

		if(size != 0 && size < 4)
			size = 4;

		offset += size;
	}

	return offset;
}

VmConstant* GetArgumentValue(Eval &ctx, FunctionData *data, unsigned argument)
{
	Eval::StackFrame *frame = ctx.stackFrames.back();

	VmType type = GetVmType(ctx.ctx, data->arguments[argument].type);

	if(type == VmType::Int)
		return LoadFrameInt(ctx, &frame->stack, GetArgumentOffset(data, argument), type);

	return (VmConstant*)Report(ctx, "ERROR: failed to load function '%.*s' argument '%.*s'", FMT_ISTR(data->name), FMT_ISTR(data->arguments[argument].name));
}

VmConstant* EvaluateKnownExternalFunction(Eval &ctx, FunctionData *function)
{
	if(function->name == InplaceStr("assert") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeInt)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		if(value->iValue == 0)
			return (VmConstant*)Report(ctx, "ERROR: assertion failed");

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name == InplaceStr("bool") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeBool)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		return CreateConstantInt(ctx.allocator, value->iValue != 0);
	}
	else if(function->name == InplaceStr("char") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeChar)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		return CreateConstantInt(ctx.allocator, char(value->iValue));
	}
	else if(function->name == InplaceStr("short") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeShort)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		return CreateConstantInt(ctx.allocator, short(value->iValue));
	}
	else if(function->name == InplaceStr("int") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeInt)
	{
		return GetArgumentValue(ctx, function, 0);
	}
	else if(function->name == InplaceStr("long") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeLong)
	{
		return GetArgumentValue(ctx, function, 0);
	}
	else if(function->name == InplaceStr("float") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeFloat)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		return CreateConstantDouble(ctx.allocator, float(value->dValue));
	}
	else if(function->name == InplaceStr("double") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeDouble)
	{
		return GetArgumentValue(ctx, function, 0);
	}
	else if(function->name == InplaceStr("__newS"))
	{
		VmConstant *size = GetArgumentValue(ctx, function, 0);

		if(!size)
			return NULL;

		VmConstant *type = GetArgumentValue(ctx, function, 1);

		if(!type)
			return NULL;

		TypeBase *target = ctx.ctx.types[unsigned(type->iValue)];

		assert(target->size == size->iValue);

		unsigned offset = ctx.heapSize;

		ctx.heap.Reserve(ctx, offset, size->iValue);
		ctx.heapSize += size->iValue;

		VmConstant *result = allocate(VmConstant)(ctx.allocator, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(target)));

		result->iValue = offset;
		assert(int(result->iValue & memoryOffsetMask) == result->iValue);
		result->iValue |= GetStorageIndex(ctx, &ctx.heap) << memoryStorageBits;

		return result;
	}
	else if(function->name == InplaceStr("__newA"))
	{
		VmConstant *size = GetArgumentValue(ctx, function, 0);

		if(!size)
			return NULL;

		VmConstant *count = GetArgumentValue(ctx, function, 1);

		if(!count)
			return NULL;

		VmConstant *type = GetArgumentValue(ctx, function, 2);

		if(!type)
			return NULL;

		TypeBase *target = ctx.ctx.types[unsigned(type->iValue)];

		assert(target->size == size->iValue);

		if(unsigned(size->iValue * count->iValue) > ctx.variableMemoryLimit)
			return (VmConstant*)Report(ctx, "ERROR: single variable memory limit");

		unsigned offset = ctx.heapSize;

		ctx.heap.Reserve(ctx, offset, size->iValue * count->iValue);
		ctx.heapSize += size->iValue * count->iValue;

		unsigned pointer = 0;

		pointer = offset;
		assert(int(pointer & memoryOffsetMask) == pointer);
		pointer |= GetStorageIndex(ctx, &ctx.heap) << memoryStorageBits;

		VmConstant *result = allocate(VmConstant)(ctx.allocator, VmType::ArrayRef(ctx.ctx.GetUnsizedArrayType(target)));

		char *storage = (char*)ctx.allocator->alloc(NULLC_PTR_SIZE + 4);

		if(sizeof(void*) == 4)
		{
			memcpy(storage, &pointer, 4);
		}
		else
		{
			unsigned long long tmp = pointer;
			memcpy(storage, &tmp, 8);
		}

		memcpy(storage + sizeof(void*), &count->iValue, 4);

		result->sValue = storage;

		return result;
	}

	return NULL;
}

VmConstant* EvaluateFunction(Eval &ctx, VmFunction *function)
{
	Eval::StackFrame *frame = ctx.stackFrames.back();

	VmBlock *predecessorBlock = NULL;
	VmBlock *currentBlock = function->firstBlock;

	if(function->function && !function->function->declaration)
	{
		if(ctx.emulateKnownExternals && ctx.ctx.GetFunctionIndex(function->function) < ctx.ctx.baseModuleFunctionCount)
		{
			if(VmConstant *result = EvaluateKnownExternalFunction(ctx, function->function))
				return result;
		}

		return (VmConstant*)Report(ctx, "ERROR: function '%.*s' has no source", FMT_ISTR(function->function->name));
	}

	while(currentBlock)
	{
		if(!currentBlock->firstInstruction)
			break;

		for(VmInstruction *instruction = currentBlock->firstInstruction; instruction; instruction = instruction->nextSibling)
		{
			VmBlock *nextBlock = NULL;

			VmConstant *result = EvaluateInstruction(ctx, instruction, predecessorBlock, &nextBlock);

			if(HasReport(ctx))
				return NULL;

			frame->AssignRegister(instruction->uniqueId, result);

			if(instruction->cmd == VM_INST_RETURN || instruction->cmd == VM_INST_YIELD)
				return result;

			if(nextBlock)
			{
				predecessorBlock = currentBlock;
				currentBlock = nextBlock;
				break;
			}

			if(instruction == currentBlock->lastInstruction)
				currentBlock = NULL;
		}
	}

	return NULL;
}

VmConstant* EvaluateModule(Eval &ctx, VmModule *module)
{
	ctx.module = module;

	VmFunction *global = module->functions.tail;

	ctx.heap.Reserve(ctx, 0, 4096);
	ctx.heapSize += 4096;

	ctx.globalFrame = allocate(Eval::StackFrame)(ctx.allocator, global);
	ctx.stackFrames.push_back(ctx.globalFrame);

	VmConstant *result = EvaluateFunction(ctx, global);

	if(HasReport(ctx))
		return NULL;

	ctx.stackFrames.pop_back();

	assert(ctx.stackFrames.empty());

	return result;
}
