#include "InstructionTreeVmEval.h"

#include <math.h>

#include "ExpressionTree.h"
#include "TypeTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

const unsigned memoryStorageBits = 16u;
const unsigned memoryOffsetMask = (1u << memoryStorageBits) - 1;
const unsigned memoryStorageMask = ~0u & ~memoryOffsetMask;

namespace
{
	int GetIntPow(int number, int power)
	{
		if(power < 0)
			return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

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
			return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

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

	long long StrToLong(const char* p)
	{
		while(*p && *p <= ' ')
			p++;

		bool negative = *p == '-';
		if(negative)
			p++;

		unsigned long long res = 0;
		while(unsigned(*p - '0') < 10)
			res = res * 10 + unsigned(*p++ - '0');

		return res * (negative ? -1 : 1);
	}

	NULLC_PRINT_FORMAT_CHECK(2, 3) ExprBase* Report(InstructionVMEvalContext &ctx, const char *msg, ...)
	{
		// Do not replace previous error
		if(ctx.hasError)
			return NULL;

		if(ctx.errorBuf && ctx.errorBufSize)
		{
			va_list args;
			va_start(args, msg);

			vsnprintf(ctx.errorBuf, ctx.errorBufSize, msg, args);

			va_end(args);

			ctx.errorBuf[ctx.errorBufSize - 1] = '\0';
		}

		ctx.hasError = true;

		return NULL;
	}

	intptr_t ReadVmMemoryPointer(void* address)
	{
		intptr_t result;
		memcpy(&result, address, sizeof(intptr_t));
		return result;
	}

	void WriteVmMemoryPointer(void* address, intptr_t value)
	{
		memcpy(address, &value, sizeof(intptr_t));
	}
}

bool InstructionVMEvalContext::Storage::Reserve(InstructionVMEvalContext &ctx, unsigned offset, unsigned size)
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

void InstructionVMEvalContext::StackFrame::AssignRegister(unsigned id, VmConstant *constant)
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

VmConstant* InstructionVMEvalContext::StackFrame::ReadRegister(unsigned id)
{
	assert(id != 0);

	if(id - 1 < instructionValues.size())
		return instructionValues[id - 1];

	return 0;
}

VmConstant* LoadFrameByte(InstructionVMEvalContext &ctx, InstructionVMEvalContext::Storage *storage, unsigned offset)
{
	char value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	return CreateConstantInt(ctx.allocator, NULL, value);
}

VmConstant* LoadFrameShort(InstructionVMEvalContext &ctx, InstructionVMEvalContext::Storage *storage, unsigned offset)
{
	short value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	return CreateConstantInt(ctx.allocator, NULL, value);
}

VmConstant* LoadFrameInt(InstructionVMEvalContext &ctx, InstructionVMEvalContext::Storage *storage, unsigned offset, VmType type)
{
	int value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	if(type.type == VM_TYPE_POINTER)
		return CreateConstantPointer(ctx.allocator, NULL, value, NULL, type.structType, false);

	return CreateConstantInt(ctx.allocator, NULL, value);
}

VmConstant* LoadFrameFloat(InstructionVMEvalContext &ctx, InstructionVMEvalContext::Storage *storage, unsigned offset)
{
	float value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	return CreateConstantDouble(ctx.allocator, NULL, value);
}

VmConstant* LoadFrameDouble(InstructionVMEvalContext &ctx, InstructionVMEvalContext::Storage *storage, unsigned offset)
{
	double value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	return CreateConstantDouble(ctx.allocator, NULL, value);
}

VmConstant* LoadFrameLong(InstructionVMEvalContext &ctx, InstructionVMEvalContext::Storage *storage, unsigned offset, VmType type)
{
	long long value = 0;
	if(!storage->Reserve(ctx, offset, sizeof(value)))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(&value, storage->data.data + offset, sizeof(value));

	if(type.type == VM_TYPE_POINTER)
	{
		assert(unsigned(value) == value);

		return CreateConstantPointer(ctx.allocator, NULL, unsigned(value), NULL, type.structType, false);
	}

	return CreateConstantLong(ctx.allocator, NULL, value);
}

VmConstant* LoadFrameStruct(InstructionVMEvalContext &ctx, InstructionVMEvalContext::Storage *storage, unsigned offset, VmType type)
{
	unsigned size = type.size;

	char *value = (char*)ctx.allocator->alloc(size);
	memset(value, 0, size);

	if(!storage->Reserve(ctx, offset, size))
		return (VmConstant*)Report(ctx, "ERROR: out of stack space");

	memcpy(value, storage->data.data + offset, size);

	VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, type, NULL);

	result->sValue = value;

	return result;
}

VmConstant* LoadFramePointer(InstructionVMEvalContext &ctx, InstructionVMEvalContext::Storage *storage, unsigned offset, VmType type)
{
	if(sizeof(void*) == 4)
		return LoadFrameInt(ctx, storage, offset, type);

	return LoadFrameLong(ctx, storage, offset, type);
}

VmConstant* ExtractValue(InstructionVMEvalContext &ctx, VmConstant *value, unsigned offset, VmType type)
{
	assert(value->sValue);
	assert(offset + type.size <= value->type.size);

	const char *source = value->sValue + offset;

	if(type == VmType::Int)
	{
		int tmp = 0;
		memcpy(&tmp, source, sizeof(tmp));
		return CreateConstantInt(ctx.allocator, NULL, tmp);
	}
	else if(type == VmType::Double)
	{
		double tmp = 0;
		memcpy(&tmp, source, sizeof(tmp));
		return CreateConstantDouble(ctx.allocator, NULL, tmp);
	}
	else if(type == VmType::Long)
	{
		long long tmp = 0;
		memcpy(&tmp, source, sizeof(tmp));
		return CreateConstantLong(ctx.allocator, NULL, tmp);
	}
	else if(type.type == VM_TYPE_POINTER)
	{
		unsigned long long pointer = 0;

		if(sizeof(void*) == 4)
		{
			unsigned tmp;
			memcpy(&tmp, source, sizeof(void*));
			pointer = tmp;
		}
		else
		{
			memcpy(&pointer, source, sizeof(void*));
		}

		assert(unsigned(pointer) == pointer);

		return CreateConstantPointer(ctx.allocator, NULL, unsigned(pointer), NULL, type.structType, false);
	}
	else
	{
		char *tmp = (char*)ctx.allocator->alloc(type.size);
		memcpy(tmp, source, type.size);

		VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, type, NULL);

		result->sValue = tmp;

		return result;
	}
}

unsigned GetAllocaAddress(InstructionVMEvalContext &ctx, VariableData *container)
{
	InstructionVMEvalContext::StackFrame *frame = ctx.stackFrames.back();

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

unsigned GetStorageIndex(InstructionVMEvalContext &ctx, InstructionVMEvalContext::Storage *storage)
{
	if(storage->index != 0)
		return storage->index;

	ctx.storageSet.push_back(storage);
	storage->index = ctx.storageSet.size();

	return storage->index;
}

void CopyConstantRaw(InstructionVMEvalContext &ctx, char *dst, unsigned dstSize, VmConstant *src, unsigned storeSize)
{
	InstructionVMEvalContext::StackFrame *frame = ctx.stackFrames.back();

	(void)dstSize;
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

		if(VariableData *variable = src->container)
		{
			if(variable->importModule)
			{
				Report(ctx, "ERROR: can't access imported variable");
				return;
			}

			if(unsigned offset = GetAllocaAddress(ctx, variable))
			{
				pointer = src->iValue + offset;
				assert((pointer & memoryOffsetMask) == pointer);
				pointer |= GetStorageIndex(ctx, &frame->allocas) << memoryStorageBits;
			}
			else if(IsGlobalScope(variable->scope))
			{
				pointer = src->iValue + variable->offset;
				assert((pointer & memoryOffsetMask) == pointer);
				pointer |= GetStorageIndex(ctx, &ctx.globalFrame->stack) << memoryStorageBits;
			}
			else
			{
				pointer = src->iValue + variable->offset;
				assert((pointer & memoryOffsetMask) == pointer);
				pointer |= GetStorageIndex(ctx, &frame->stack) << memoryStorageBits;
			}
		}
		else
		{
			pointer = src->iValue;
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

		unsigned index = ctx.ctx.GetFunctionIndex(src->fValue->function);

		memcpy(dst, &index, src->type.size);
	}
	else
	{
		assert(!"unknown constant type");
	}
}

VmConstant* EvaluateOperand(InstructionVMEvalContext &ctx, VmValue *value)
{
	InstructionVMEvalContext::StackFrame *frame = ctx.stackFrames.back();

	if(VmConstant *constant = getType<VmConstant>(value))
		return constant;

	if(VmInstruction *instruction = getType<VmInstruction>(value))
		return frame->ReadRegister(instruction->uniqueId);

	if(VmBlock *block = getType<VmBlock>(value))
		return CreateConstantBlock(ctx.allocator, block->source, block);

	if(VmFunction *function = getType<VmFunction>(value))
		return CreateConstantFunction(ctx.allocator, function->source, function);

	assert(!"unknown type");

	return NULL;
}

InstructionVMEvalContext::Storage* FindTarget(InstructionVMEvalContext &ctx, VmConstant *value, unsigned &base)
{
	InstructionVMEvalContext::StackFrame *frame = ctx.stackFrames.back();

	InstructionVMEvalContext::Storage *target = NULL;

	base = 0;

	if(VariableData *variable = value->container)
	{
		if(variable->importModule)
		{
			Report(ctx, "ERROR: can't access imported variable");
			return NULL;
		}

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

	if(!target)
	{
		Report(ctx, "ERROR: null pointer access");
		return NULL;
	}

	if(target->expired)
	{
		Report(ctx, "ERROR: dead memory access");
		return NULL;
	}

	return target;
}

char* GetPointerDataPtr(InstructionVMEvalContext &ctx, unsigned vmPointer)
{
	if(unsigned storageIndex = (vmPointer & memoryStorageMask) >> memoryStorageBits)
	{
		InstructionVMEvalContext::Storage *storage = ctx.storageSet[storageIndex - 1];

		return storage->data.data + (vmPointer & memoryOffsetMask);
	}

	return NULL;
}

VmConstant* LoadFrameValue(InstructionVMEvalContext &ctx, VmConstant *pointer, VmConstant *offset, VmType type, unsigned loadSize)
{
	unsigned base = 0;

	if(InstructionVMEvalContext::Storage *target = FindTarget(ctx, pointer, base))
	{
		if(ctx.printExecution)
		{
			InplaceStr functionName = target->functionOwner && target->functionOwner->function? target->functionOwner->function->name->name : InplaceStr("---");

			printf("      LoadFrameValue %.*s [%s] @ %04x + %02x [%02x]\n", FMT_ISTR(functionName), target->tag, base, pointer->iValue & memoryOffsetMask, loadSize);
		}

		unsigned location = (pointer->iValue & memoryOffsetMask) + base;

		if(offset)
			location += offset->iValue;

		if(type == VmType::Int && loadSize == 1)
			return LoadFrameByte(ctx, target, location);

		if(type == VmType::Int && loadSize == 2)
			return LoadFrameShort(ctx, target, location);

		if((type == VmType::Int || type.type == VM_TYPE_POINTER) && loadSize == 4)
			return LoadFrameInt(ctx, target, location, type);

		if(type == VmType::Double && loadSize == 4)
			return LoadFrameFloat(ctx, target, location);

		if(type == VmType::Double && loadSize == 8)
			return LoadFrameDouble(ctx, target, location);

		if((type == VmType::Long || type.type == VM_TYPE_POINTER) && loadSize == 8)
			return LoadFrameLong(ctx, target, location, type);

		return LoadFrameStruct(ctx, target, location, type);
	}

	assert(ctx.hasError);

	return NULL;
}

bool StoreFrameValue(InstructionVMEvalContext &ctx, VmConstant *pointer, int offset, VmConstant *value, unsigned storeSize)
{
	unsigned base = 0;

	if(InstructionVMEvalContext::Storage *target = FindTarget(ctx, pointer, base))
	{
		if(ctx.printExecution)
		{
			InplaceStr functionName = target->functionOwner && target->functionOwner->function ? target->functionOwner->function->name->name : InplaceStr("---");

			printf("      StoreFrameValue %.*s [%s] @ %04x + %02x [%02x]\n", FMT_ISTR(functionName), target->tag, base, pointer->iValue & memoryOffsetMask, storeSize);
		}

		unsigned location = (pointer->iValue & memoryOffsetMask) + base;

		location += offset;

		if(!target->Reserve(ctx, location, value->type.size))
		{
			Report(ctx, "ERROR: out of stack space");
			return false;
		}

		CopyConstantRaw(ctx, target->data.data + location, target->data.size() - location, value, storeSize);

		return true;
	}

	assert(ctx.hasError);

	return false;
}

VmConstant* AllocateHeapObject(InstructionVMEvalContext &ctx, TypeBase *target)
{
	unsigned offset = ctx.heapSize;

	ctx.heap.Reserve(ctx, offset, unsigned(target->size));
	ctx.heapSize += unsigned(target->size);

	VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(target)), NULL);

	result->iValue = offset;
	assert(int(result->iValue & memoryOffsetMask) == result->iValue);
	result->iValue |= GetStorageIndex(ctx, &ctx.heap) << memoryStorageBits;

	return result;
}

VmConstant* AllocateHeapArray(InstructionVMEvalContext &ctx, TypeBase *target, unsigned count)
{
	unsigned offset = ctx.heapSize;

	ctx.heap.Reserve(ctx, offset, unsigned(target->size) * count);
	ctx.heapSize += unsigned(target->size) * count;

	unsigned pointer = 0;

	pointer = offset;
	assert(unsigned(pointer & memoryOffsetMask) == pointer);
	pointer |= GetStorageIndex(ctx, &ctx.heap) << memoryStorageBits;

	VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, VmType::ArrayRef(ctx.ctx.GetUnsizedArrayType(target)), NULL);

	char *storage = (char*)ctx.allocator->alloc(result->type.size);

	if(sizeof(void*) == 4)
	{
		memcpy(storage, &pointer, 4);
	}
	else
	{
		unsigned long long tmp = pointer;
		memcpy(storage, &tmp, 8);
	}

	memcpy(storage + sizeof(void*), &count, 4);

	result->sValue = storage;

	return result;
}

VmConstant* EvaluateInstruction(InstructionVMEvalContext &ctx, VmInstruction *instruction, VmBlock *predecessor, VmBlock **nextBlock)
{
	ctx.instruction++;

	if(ctx.instruction >= ctx.instructionsLimit)
		return (VmConstant*)Report(ctx, "ERROR: instruction limit reached");

	SmallArray<VmConstant*, 8> arguments(ctx.allocator);

	for(unsigned i = 0; i < instruction->arguments.size(); i++)
		arguments.push_back(EvaluateOperand(ctx, instruction->arguments[i]));

	if(ctx.printExecution)
		printf("    EvaluateInstruction %d = %s\n", instruction->uniqueId, GetInstructionName(instruction));

	switch(instruction->cmd)
	{
	case VM_INST_LOAD_BYTE:
	case VM_INST_LOAD_SHORT:
	case VM_INST_LOAD_INT:
	case VM_INST_LOAD_FLOAT:
	case VM_INST_LOAD_DOUBLE:
	case VM_INST_LOAD_LONG:
	case VM_INST_LOAD_STRUCT:
		return LoadFrameValue(ctx, arguments[0], arguments[1], instruction->type, GetAccessSize(instruction));
	case VM_INST_LOAD_IMMEDIATE:
		return arguments[0];
	case VM_INST_STORE_BYTE:
	case VM_INST_STORE_SHORT:
	case VM_INST_STORE_INT:
	case VM_INST_STORE_FLOAT:
	case VM_INST_STORE_DOUBLE:
	case VM_INST_STORE_LONG:
	case VM_INST_STORE_STRUCT:
		StoreFrameValue(ctx, arguments[0], arguments[1]->iValue, arguments[2], GetAccessSize(instruction));

		return NULL;
	case VM_INST_DOUBLE_TO_INT:
		return CreateConstantInt(ctx.allocator, NULL, int(arguments[0]->dValue));
	case VM_INST_DOUBLE_TO_LONG:
		return CreateConstantLong(ctx.allocator, NULL, (long long)(arguments[0]->dValue));
	case VM_INST_DOUBLE_TO_FLOAT:
		{
			float source = float(arguments[0]->dValue);

			int target = 0;
			memcpy(&target, &source, sizeof(float));

			return CreateConstantInt(ctx.allocator, NULL, target);
		}
	case VM_INST_INT_TO_DOUBLE:
		return CreateConstantDouble(ctx.allocator, NULL, double(arguments[0]->iValue));
	case VM_INST_LONG_TO_DOUBLE:
		return CreateConstantDouble(ctx.allocator, NULL, double(arguments[0]->lValue));
	case VM_INST_INT_TO_LONG:
		return CreateConstantLong(ctx.allocator, NULL, (long long)(arguments[0]->iValue));
	case VM_INST_LONG_TO_INT:
		return CreateConstantInt(ctx.allocator, NULL, int(arguments[0]->lValue));
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

			return CreateConstantPointer(ctx.allocator, NULL, value->iValue + index->iValue * elementSize->iValue, value->container, instruction->type.structType, false);
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

			return CreateConstantPointer(ctx.allocator, NULL, unsigned(pointer) + index->iValue * elementSize->iValue, NULL, instruction->type.structType, false);
		}

		break;
	case VM_INST_FUNCTION_ADDRESS:
		return arguments[0];
	case VM_INST_TYPE_ID:
		return arguments[0];
	case VM_INST_SET_RANGE:
		{
			VmConstant *address = arguments[0];
			VmConstant *count = arguments[1];
			VmConstant *initializer = arguments[2];
			VmConstant *elementSize = arguments[3];

			unsigned base = 0;

			if(InstructionVMEvalContext::Storage *target = FindTarget(ctx, address, base))
			{
				for(int i = 0; i < count->iValue; i++)
				{
					unsigned location = (address->iValue & memoryOffsetMask) + base;

					location += i * elementSize->iValue;

					if(!target->Reserve(ctx, location, initializer->type.size))
					{
						Report(ctx, "ERROR: out of stack space");
						return NULL;
					}

					CopyConstantRaw(ctx, target->data.data + location, target->data.size() - location, initializer, elementSize->iValue);
				}
			}

			return NULL;
		}
		break;
	case VM_INST_MEM_COPY:
	{
		TypeRef *refType = getType<TypeRef>(arguments[2]->type.structType);

		VmConstant *srcValue = LoadFrameValue(ctx, arguments[2], arguments[3], GetVmType(ctx.ctx, refType->subType), arguments[4]->iValue);

		StoreFrameValue(ctx, arguments[0], arguments[1]->iValue, srcValue, arguments[4]->iValue);

		return NULL;
	}
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
			VmFunction *function = NULL;
			void *context = NULL;
			unsigned startArgument = ~0u;

			if(arguments[0]->type.type == VM_TYPE_FUNCTION_REF)
			{
				unsigned functionIndex = 0;
				memcpy(&functionIndex, arguments[0]->sValue + sizeof(void*), 4);

				if(functionIndex >= ctx.ctx.functions.size())
					return (VmConstant*)Report(ctx, "ERROR: invalid function index");

				if(functionIndex == 0)
					return (VmConstant*)Report(ctx, "ERROR: null function pointer call");

				function = ctx.ctx.functions[functionIndex]->vmFunction;

				memcpy(&context, arguments[0]->sValue, sizeof(void*));

				startArgument = 2;
			}
			else
			{
				assert(arguments[1]->fValue);

				function = arguments[1]->fValue;

				CopyConstantRaw(ctx, (char*)&context, sizeof(void*), arguments[0], sizeof(void*));

				startArgument = 3;
			}

			InstructionVMEvalContext::StackFrame *calleeFrame = new (ctx.get<InstructionVMEvalContext::StackFrame>()) InstructionVMEvalContext::StackFrame(ctx.allocator, function);

			if(ctx.stackFrames.size() >= ctx.stackDepthLimit)
				return (VmConstant*)Report(ctx, "ERROR: stack depth limit");

			unsigned offset = 0;

			for(unsigned i = startArgument; i < arguments.size(); i++)
			{
				VmConstant *argument = arguments[i];

				if(argument->isReference)
					argument = LoadFrameValue(ctx, argument, NULL, argument->type, argument->type.size);

				ArgumentData &original = function->function->arguments[i - startArgument];

				if(original.type->size == 0)
				{
					offset += 4;
					continue;
				}

				unsigned argumentSize = argument->type.size;

				if(!calleeFrame->stack.Reserve(ctx, offset, argumentSize))
					return (VmConstant*)Report(ctx, "ERROR: out of stack space");

				CopyConstantRaw(ctx, calleeFrame->stack.data.data + offset, calleeFrame->stack.data.size() - offset, argument, argumentSize);

				offset += argumentSize > 4 ? argumentSize : 4;
			}

			if(!calleeFrame->stack.Reserve(ctx, offset, sizeof(void*)))
				return (VmConstant*)Report(ctx, "ERROR: out of stack space");

			memcpy(calleeFrame->stack.data.data + offset, &context, sizeof(void*));
			offset += sizeof(void*);

			ctx.stackFrames.push_back(calleeFrame);

			VmConstant *result = EvaluateFunction(ctx, function);

			calleeFrame->stack.expired = true;
			calleeFrame->allocas.expired = true;

			ctx.stackFrames.pop_back();

			if(result)
			{
				VmConstant *target = arguments[startArgument - 1];

				if(target->isReference)
					StoreFrameValue(ctx, target, 0, result, target->type.size);
			}

			return result;
		}
		break;
	case VM_INST_RETURN:
		if(arguments.empty())
			return CreateConstantVoid(ctx.allocator);

		if(VmConstant *source = arguments[0])
		{
			if(source->isReference)
				return LoadFrameValue(ctx, source, NULL, source->type, source->type.size);
		}

		return arguments[0];
	case VM_INST_YIELD:
		if(arguments.empty())
			return CreateConstantVoid(ctx.allocator);

		return arguments[0];
	case VM_INST_ADD:
		if(arguments[0]->type.type == VM_TYPE_POINTER || arguments[1]->type.type == VM_TYPE_POINTER)
		{
			// Both arguments can't be based on an offset
			assert(!(arguments[0]->container && arguments[1]->container));

			return CreateConstantPointer(ctx.allocator, NULL, arguments[0]->iValue + arguments[1]->iValue, arguments[0]->container ? arguments[0]->container : arguments[1]->container, instruction->type.structType, false);
		}
		else
		{
			assert(arguments[0]->type == arguments[1]->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue + arguments[1]->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantDouble(ctx.allocator, NULL, arguments[0]->dValue + arguments[1]->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue + arguments[1]->lValue);
		}
		break;
	case VM_INST_SUB:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue - arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, NULL, arguments[0]->dValue - arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue - arguments[1]->lValue);

		break;
	case VM_INST_MUL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue * arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, NULL, arguments[0]->dValue * arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue * arguments[1]->lValue);

		break;
	case VM_INST_DIV:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type != VmType::Double && IsConstantZero(arguments[1]))
			return (VmConstant*)Report(ctx, "ERROR: division by zero");

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue / arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, NULL, arguments[0]->dValue / arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue / arguments[1]->lValue);

		break;
	case VM_INST_POW:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, GetIntPow(arguments[0]->iValue, arguments[1]->iValue));
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, NULL, pow(arguments[0]->dValue, arguments[1]->dValue));
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, GetLongPow(arguments[0]->lValue, arguments[1]->lValue));

		break;
	case VM_INST_MOD:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type != VmType::Double && IsConstantZero(arguments[1]))
			return (VmConstant*)Report(ctx, "ERROR: division by zero");

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue % arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, NULL, fmod(arguments[0]->dValue, arguments[1]->dValue));
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue % arguments[1]->lValue);

		break;
	case VM_INST_LESS:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue < arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue < arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue < arguments[1]->lValue);

		break;
	case VM_INST_GREATER:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue > arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue > arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue > arguments[1]->lValue);

		break;
	case VM_INST_LESS_EQUAL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue <= arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue <= arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue <= arguments[1]->lValue);

		break;
	case VM_INST_GREATER_EQUAL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue >= arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue >= arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue >= arguments[1]->lValue);

		break;
	case VM_INST_EQUAL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue == arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue == arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue == arguments[1]->lValue);
		else if(arguments[0]->type.type == VM_TYPE_POINTER)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue == arguments[1]->iValue && arguments[0]->container == arguments[1]->container);

		break;
	case VM_INST_NOT_EQUAL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue != arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue != arguments[1]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue != arguments[1]->lValue);
		else if(arguments[0]->type.type == VM_TYPE_POINTER)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue != arguments[1]->iValue || arguments[0]->container != arguments[1]->container);

		break;
	case VM_INST_SHL:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue << arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue << arguments[1]->lValue);
		break;
	case VM_INST_SHR:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue >> arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue >> arguments[1]->lValue);
		break;
	case VM_INST_BIT_AND:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue & arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue & arguments[1]->lValue);
		break;
	case VM_INST_BIT_OR:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue | arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue | arguments[1]->lValue);
		break;
	case VM_INST_BIT_XOR:
		assert(arguments[0]->type == arguments[1]->type);

		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue ^ arguments[1]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue ^ arguments[1]->lValue);
		break;
	case VM_INST_ADD_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			if(arguments[0]->type.type == VM_TYPE_POINTER || rhs->type.type == VM_TYPE_POINTER)
			{
				// Both arguments can't be based on an offset
				assert(!(arguments[0]->container && rhs->container));

				return CreateConstantPointer(ctx.allocator, NULL, arguments[0]->iValue + rhs->iValue, arguments[0]->container ? arguments[0]->container : rhs->container, instruction->type.structType, false);
			}
			else
			{
				assert(arguments[0]->type == rhs->type);

				if(arguments[0]->type == VmType::Int)
					return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue + rhs->iValue);
				else if(arguments[0]->type == VmType::Double)
					return CreateConstantDouble(ctx.allocator, NULL, arguments[0]->dValue + rhs->dValue);
				else if(arguments[0]->type == VmType::Long)
					return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue + rhs->lValue);
			}
		}
		else
		{
			return NULL;
		}
		break;
	case VM_INST_SUB_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue - rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantDouble(ctx.allocator, NULL, arguments[0]->dValue - rhs->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue - rhs->lValue);
		}
		else
		{
			return NULL;
		}
		break;
	case VM_INST_MUL_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue * rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantDouble(ctx.allocator, NULL, arguments[0]->dValue * rhs->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue * rhs->lValue);
		}
		else
		{
			return NULL;
		}
		break;
	case VM_INST_DIV_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type != VmType::Double && IsConstantZero(rhs))
				return (VmConstant*)Report(ctx, "ERROR: division by zero");

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue / rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantDouble(ctx.allocator, NULL, arguments[0]->dValue / rhs->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue / rhs->lValue);
		}
		else
		{
			return NULL;
		}
		break;
	case VM_INST_POW_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, GetIntPow(arguments[0]->iValue, rhs->iValue));
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantDouble(ctx.allocator, NULL, pow(arguments[0]->dValue, rhs->dValue));
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, GetLongPow(arguments[0]->lValue, rhs->lValue));
		}
		else
		{
			return NULL;
		}

		break;
	case VM_INST_MOD_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type != VmType::Double && IsConstantZero(rhs))
				return (VmConstant*)Report(ctx, "ERROR: division by zero");

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue % rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantDouble(ctx.allocator, NULL, fmod(arguments[0]->dValue, rhs->dValue));
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue % rhs->lValue);
		}
		else
		{
			return NULL;
		}

		break;
	case VM_INST_LESS_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], arguments[0]->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue < rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue < rhs->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue < rhs->lValue);
		}
		else
		{
			return NULL;
		}

		break;
	case VM_INST_GREATER_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], arguments[0]->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue > rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue > rhs->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue > rhs->lValue);
		}
		else
		{
			return NULL;
		}

		break;
	case VM_INST_LESS_EQUAL_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], arguments[0]->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue <= rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue <= rhs->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue <= rhs->lValue);
		}
		else
		{
			return NULL;
		}

		break;
	case VM_INST_GREATER_EQUAL_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], arguments[0]->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue >= rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue >= rhs->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue >= rhs->lValue);
		}
		else
		{
			return NULL;
		}

		break;
	case VM_INST_EQUAL_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], arguments[0]->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue == rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue == rhs->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue == rhs->lValue);
			else if(arguments[0]->type.type == VM_TYPE_POINTER)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue == rhs->iValue && arguments[0]->container == rhs->container);
		}
		else
		{
			return NULL;
		}

		break;
	case VM_INST_NOT_EQUAL_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], arguments[0]->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue != rhs->iValue);
			else if(arguments[0]->type == VmType::Double)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->dValue != rhs->dValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->lValue != rhs->lValue);
			else if(arguments[0]->type.type == VM_TYPE_POINTER)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue != rhs->iValue || arguments[0]->container != rhs->container);
		}
		else
		{
			return NULL;
		}

		break;
	case VM_INST_SHL_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue << rhs->iValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue << rhs->lValue);
		}
		else
		{
			return NULL;
		}
		break;
	case VM_INST_SHR_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue >> rhs->iValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue >> rhs->lValue);
		}
		else
		{
			return NULL;
		}
		break;
	case VM_INST_BIT_AND_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue & rhs->iValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue & rhs->lValue);
		}
		else
		{
			return NULL;
		}
		break;
	case VM_INST_BIT_OR_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue | rhs->iValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue | rhs->lValue);
		}
		else
		{
			return NULL;
		}
		break;
	case VM_INST_BIT_XOR_LOAD:
		if(VmConstant *rhs = LoadFrameValue(ctx, arguments[1], arguments[2], instruction->type, GetAccessSize(instruction)))
		{
			assert(arguments[0]->type == rhs->type);

			if(arguments[0]->type == VmType::Int)
				return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue ^ rhs->iValue);
			else if(arguments[0]->type == VmType::Long)
				return CreateConstantLong(ctx.allocator, NULL, arguments[0]->lValue ^ rhs->lValue);
		}
		else
		{
			return NULL;
		}
		break;
	case VM_INST_NEG:
		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, -arguments[0]->iValue);
		else if(arguments[0]->type == VmType::Double)
			return CreateConstantDouble(ctx.allocator, NULL, -arguments[0]->dValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, -arguments[0]->lValue);
		break;
	case VM_INST_BIT_NOT:
		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, ~arguments[0]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantLong(ctx.allocator, NULL, ~arguments[0]->lValue);
		break;
	case VM_INST_LOG_NOT:
		if(arguments[0]->type == VmType::Int)
			return CreateConstantInt(ctx.allocator, NULL, !arguments[0]->iValue);
		else if(arguments[0]->type == VmType::Long)
			return CreateConstantInt(ctx.allocator, NULL, !arguments[0]->lValue);

		if(arguments[0]->type.type == VM_TYPE_POINTER)
			return CreateConstantInt(ctx.allocator, NULL, arguments[0]->iValue == 0 && arguments[0]->container == NULL);
		break;
	case VM_INST_CONVERT_POINTER:
		{
			VmConstant *value = arguments[0];
			VmConstant *typeID = arguments[1]; // target

			assert(value->type.type == VM_TYPE_AUTO_REF && value->sValue);
			assert(typeID->type == VmType::Int);

			VmConstant *valueTypeID = ExtractValue(ctx, value, 0, VmType::Int);
			VmConstant *valuePtr = ExtractValue(ctx, value, 4, instruction->type);

			if(valueTypeID->iValue == typeID->iValue)
				return valuePtr;

			if(unsigned(valueTypeID->iValue) >= ctx.ctx.types.size())
				return (VmConstant*)Report(ctx, "ERROR: invalid type index");

			TypeBase *sourceType = ctx.ctx.types[valueTypeID->iValue];
			TypeBase *targetType = ctx.ctx.types[typeID->iValue];

			if(TypeClass *classType = getType<TypeClass>(sourceType))
			{
				while(classType->baseClass)
				{
					classType = classType->baseClass;

					if(classType == targetType)
						return valuePtr;
				}
			}

			return (VmConstant*)Report(ctx, "ERROR: cannot convert from %.*s ref to %.*s ref", FMT_ISTR(sourceType->name), FMT_ISTR(targetType->name));
		}
		break;
	case VM_INST_ABORT_NO_RETURN:
		return (VmConstant*)Report(ctx, "ERROR: function didn't return a value");
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

			VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, instruction->type, NULL);

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

			VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, instruction->type, NULL);

			result->sValue = value;

			return result;
		}
		break;
	case VM_INST_EXTRACT:
		{
			assert(arguments[1]->type == VmType::Int);

			return ExtractValue(ctx, arguments[0], arguments[1]->iValue, instruction->type);
		}
		break;
	case VM_INST_UNYIELD:
		{
			InstructionVMEvalContext::StackFrame *frame = ctx.stackFrames.back();

			*nextBlock = frame->owner->restoreBlocks[arguments[0]->iValue];

			return NULL;
		}
		break;
	case VM_INST_PHI:
		{
			for(unsigned i = 0; i < arguments.size(); i += 2)
			{
				VmConstant *value = arguments[i];
				VmConstant *edge = arguments[i + 1];

				assert(edge->type == VmType::Block && edge->bValue);

				if(edge->bValue == predecessor)
					return value;
			}

			assert(!"phi instruction can't handle the predecessor");
		}
		break;
	case VM_INST_BITCAST:
	case VM_INST_MOV:
		{
			VmConstant *value = arguments[0];

			if(value->type.type == VM_TYPE_ARRAY_REF && instruction->type.type == VM_TYPE_ARRAY_REF)
				return value;

			if(value->type.type == VM_TYPE_POINTER && instruction->type.type == VM_TYPE_POINTER)
				return value;

			if(value->type.type == VM_TYPE_FUNCTION_REF && instruction->type.type == VM_TYPE_FUNCTION_REF)
				return value;

			if(value->type.type == VM_TYPE_FUNCTION && instruction->type.type == VM_TYPE_INT)
				return CreateConstantInt(ctx.allocator, NULL, value->fValue->function->functionIndex);

			if(value->type.type == VM_TYPE_STRUCT)
				return ExtractValue(ctx, value, 0, instruction->type);

			if(instruction->type.type == VM_TYPE_STRUCT)
			{
				char *storage = (char*)ctx.allocator->alloc(instruction->type.size);

				CopyConstantRaw(ctx, storage, instruction->type.size, value, value->type.size);

				VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, instruction->type, NULL);

				result->sValue = storage;

				return result;
			}

			if(value->type == instruction->type)
				return value;

			assert(!"unsupported bitcast");
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
	unsigned offset = 0;

	for(unsigned i = 0; i < argument; i++)
	{
		unsigned size = unsigned(data->arguments[i].type->size);

		if(size != 0 && size < 4)
			size = 4;

		offset += size;
	}

	return offset;
}

VmConstant* GetArgumentValue(InstructionVMEvalContext &ctx, FunctionData *data, unsigned argument)
{
	InstructionVMEvalContext::StackFrame *frame = ctx.stackFrames.back();

	TypeBase *argumentType = data->arguments[argument].type;

	VmType type = GetVmType(ctx.ctx, argumentType);

	unsigned offset = GetArgumentOffset(data, argument);

	if(type == VmType::Int || (type.type == VM_TYPE_POINTER && sizeof(void*) == 4))
		return LoadFrameInt(ctx, &frame->stack, offset, type);

	if(type == VmType::Double && argumentType == ctx.ctx.typeFloat)
		return LoadFrameFloat(ctx, &frame->stack, offset);

	if(type == VmType::Double && argumentType == ctx.ctx.typeDouble)
		return LoadFrameDouble(ctx, &frame->stack, offset);

	if(type == VmType::Long || (type.type == VM_TYPE_POINTER && sizeof(void*) == 8))
		return LoadFrameLong(ctx, &frame->stack, offset, type);
	
	return LoadFrameStruct(ctx, &frame->stack, offset, type);
}

VmConstant* GetContextValue(InstructionVMEvalContext &ctx, FunctionData *data)
{
	InstructionVMEvalContext::StackFrame *frame = ctx.stackFrames.back();

	TypeBase *contextType = data->contextType;

	VmType type = GetVmType(ctx.ctx, contextType);

	unsigned offset = GetArgumentOffset(data, data->arguments.size());

	return LoadFramePointer(ctx, &frame->stack, offset, type);
}

VmConstant* EvaluateKnownExternalFunction(InstructionVMEvalContext &ctx, FunctionData *function)
{
	if(function->name->name == InplaceStr("assert") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeInt)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		if(value->iValue == 0)
			return (VmConstant*)Report(ctx, "ERROR: assertion failed");

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("bool") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeBool)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		return CreateConstantInt(ctx.allocator, NULL, value->iValue != 0);
	}
	else if(function->name->name == InplaceStr("char") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeChar)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		return CreateConstantInt(ctx.allocator, NULL, char(value->iValue));
	}
	else if(function->name->name == InplaceStr("short") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeShort)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		return CreateConstantInt(ctx.allocator, NULL, short(value->iValue));
	}
	else if(function->name->name == InplaceStr("int") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeInt)
	{
		return GetArgumentValue(ctx, function, 0);
	}
	else if(function->name->name == InplaceStr("long") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeLong)
	{
		return GetArgumentValue(ctx, function, 0);
	}
	else if(function->name->name == InplaceStr("float") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeFloat)
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		return CreateConstantDouble(ctx.allocator, NULL, float(value->dValue));
	}
	else if(function->name->name == InplaceStr("double") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeDouble)
	{
		return GetArgumentValue(ctx, function, 0);
	}
	else if(function->name->name == InplaceStr("bool::bool") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeBool)
	{
		VmConstant *context = GetContextValue(ctx, function);
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!context || !value)
			return NULL;

		if(!StoreFrameValue(ctx, context, 0, value, 1))
			return NULL;

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("char::char") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeChar)
	{
		VmConstant *context = GetContextValue(ctx, function);
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!context || !value)
			return NULL;

		if(!StoreFrameValue(ctx, context, 0, value, 1))
			return NULL;

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("short::short") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeShort)
	{
		VmConstant *context = GetContextValue(ctx, function);
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!context || !value)
			return NULL;

		if(!StoreFrameValue(ctx, context, 0, value, 2))
			return NULL;

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("int::int") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeInt)
	{
		VmConstant *context = GetContextValue(ctx, function);
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!context || !value)
			return NULL;

		if(!StoreFrameValue(ctx, context, 0, value, 4))
			return NULL;

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("long::long") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeLong)
	{
		VmConstant *context = GetContextValue(ctx, function);
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!context || !value)
			return NULL;

		if(!StoreFrameValue(ctx, context, 0, value, 8))
			return NULL;

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("float::float") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeFloat)
	{
		VmConstant *context = GetContextValue(ctx, function);
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!context || !value)
			return NULL;

		if(!StoreFrameValue(ctx, context, 0, value, 4))
			return NULL;

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("double::double") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeDouble)
	{
		VmConstant *context = GetContextValue(ctx, function);
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!context || !value)
			return NULL;

		if(!StoreFrameValue(ctx, context, 0, value, 8))
			return NULL;

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("__newS"))
	{
		VmConstant *size = GetArgumentValue(ctx, function, 0);

		if(!size)
			return NULL;

		VmConstant *type = GetArgumentValue(ctx, function, 1);

		if(!type)
			return NULL;

		TypeBase *target = ctx.ctx.types[unsigned(type->iValue)];

		assert(target->size == size->iValue);

		return AllocateHeapObject(ctx, target);
	}
	else if(function->name->name == InplaceStr("__newA"))
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

		return AllocateHeapArray(ctx, target, count->iValue);
	}
	else if(function->name->name == InplaceStr("__rcomp") || function->name->name == InplaceStr("__rncomp"))
	{
		VmConstant *a = GetArgumentValue(ctx, function, 0);

		if(!a)
			return NULL;

		VmConstant *b = GetArgumentValue(ctx, function, 1);

		if(!b)
			return NULL;

		a = ExtractValue(ctx, a, 4, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		b = ExtractValue(ctx, b, 4, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));

		assert(a && b);

		return CreateConstantInt(ctx.allocator, NULL, function->name->name == InplaceStr("__rcomp") ? a->iValue == b->iValue : a->iValue != b->iValue);
	}
	else if(function->name->name == InplaceStr("__pcomp") || function->name->name == InplaceStr("__pncomp"))
	{
		VmConstant *a = GetArgumentValue(ctx, function, 0);

		if(!a)
			return NULL;

		VmConstant *b = GetArgumentValue(ctx, function, 1);

		if(!b)
			return NULL;

		int order = memcmp(a->sValue, b->sValue, NULLC_PTR_SIZE + 4);

		return CreateConstantInt(ctx.allocator, NULL, function->name->name == InplaceStr("__pcomp") ? order == 0 : order != 0);
	}
	else if(function->name->name == InplaceStr("__acomp") || function->name->name == InplaceStr("__ancomp"))
	{
		VmConstant *a = GetArgumentValue(ctx, function, 0);

		if(!a)
			return NULL;

		VmConstant *b = GetArgumentValue(ctx, function, 1);

		if(!b)
			return NULL;

		int order = memcmp(a->sValue, b->sValue, NULLC_PTR_SIZE + 4);

		return CreateConstantInt(ctx.allocator, NULL, function->name->name == InplaceStr("__acomp") ? order == 0 : order != 0);
	}
	else if(function->name->name == InplaceStr("__typeCount"))
	{
		return CreateConstantInt(ctx.allocator, NULL, ctx.ctx.types.size());
	}
	else if(function->name->name == InplaceStr("__redirect") || function->name->name == InplaceStr("__redirect_ptr"))
	{
		VmConstant *autoRef = GetArgumentValue(ctx, function, 0);

		if(!autoRef)
			return NULL;

		VmConstant *tableRef = GetArgumentValue(ctx, function, 1);

		if(!tableRef)
			return NULL;

		VmConstant *typeID = ExtractValue(ctx, autoRef, 0, GetVmType(ctx.ctx, ctx.ctx.typeTypeID));

		assert(typeID);

		unsigned typeIndex = unsigned(typeID->iValue);

		VmConstant *context = ExtractValue(ctx, autoRef, 4, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.types[typeIndex])));

		assert(context);

		VmConstant *table = LoadFrameValue(ctx, tableRef, NULL, VmType::ArrayRef(ctx.ctx.typeFunctionID), NULLC_PTR_SIZE + 4);

		if(!table)
			return NULL;

		VmConstant *tableArray = ExtractValue(ctx, table, 0, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.typeFunctionID)));
		VmConstant *tableSize = ExtractValue(ctx, table, sizeof(void*), GetVmType(ctx.ctx, ctx.ctx.typeInt));

		assert(tableArray && tableSize);

		if(typeIndex >= unsigned(tableSize->iValue))
			return (VmConstant*)Report(ctx, "ERROR: type index is out of bounds of redirection table");

		VmConstant *indexLocation = CreateConstantPointer(ctx.allocator, NULL, unsigned(tableArray->iValue + typeIndex * ctx.ctx.typeTypeID->size), tableArray->container, ctx.ctx.GetReferenceType(ctx.ctx.typeFunctionID), false);

		VmConstant *index = LoadFrameValue(ctx, indexLocation, NULL, VmType::Int, 4);

		if(!index)
			return NULL;

		if(index->iValue == 0)
		{
			if(function->name->name == InplaceStr("__redirect"))
				return (VmConstant*)Report(ctx, "ERROR: type '%.*s' doesn't implement method", FMT_ISTR(ctx.ctx.types[typeIndex]->name));

			context = CreateConstantPointer(ctx.allocator, NULL, 0, NULL, ctx.ctx.GetReferenceType(ctx.ctx.types[typeIndex]), false);
		}

		VmType resultType = GetVmType(ctx.ctx, function->type->returnType);

		char *value = (char*)ctx.allocator->alloc(resultType.size);

		CopyConstantRaw(ctx, value + 0, resultType.size, context, context->type.size);
		CopyConstantRaw(ctx, value + sizeof(void*), resultType.size - sizeof(void*), CreateConstantInt(ctx.allocator, NULL, index->iValue), 4);

		VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, resultType, NULL);

		result->sValue = value;

		return result;
	}
	else if(function->name->name == InplaceStr("duplicate") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeAutoRef)
	{
		VmConstant *ptr = GetArgumentValue(ctx, function, 0);

		if(!ptr)
			return NULL;

		VmConstant *ptrTypeID = ExtractValue(ctx, ptr, 0, GetVmType(ctx.ctx, ctx.ctx.typeTypeID));

		TypeBase *targetType = ctx.ctx.types[ptrTypeID->iValue];

		VmConstant *ptrPtr = ExtractValue(ctx, ptr, 4, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(targetType)));

		VmType storageType = GetVmType(ctx.ctx, ctx.ctx.typeAutoRef);

		char *storageValue = (char*)ctx.allocator->alloc(storageType.size);

		CopyConstantRaw(ctx, storageValue + 0, storageType.size, ptrTypeID, ptrTypeID->type.size);

		if(!ptrPtr->iValue)
		{
			VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, storageType, NULL);

			result->sValue = storageValue;

			return result;
		}

		VmConstant *resultPtr = AllocateHeapObject(ctx, targetType);

		CopyConstantRaw(ctx, storageValue + 4, storageType.size - 4, resultPtr, resultPtr->type.size);

		if(targetType->size != 0)
		{
			VmConstant *copy = LoadFrameValue(ctx, ptrPtr, NULL, GetVmType(ctx.ctx, targetType), unsigned(targetType->size));

			if(!copy)
				return NULL;

			StoreFrameValue(ctx, resultPtr, 0, copy, unsigned(targetType->size));
		}

		VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, storageType, NULL);

		result->sValue = storageValue;

		return result;
	}
	else if(function->name->name == InplaceStr("typeid") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeAutoRef)
	{
		VmConstant *reference = GetArgumentValue(ctx, function, 0);

		if(!reference)
			return NULL;

		return ExtractValue(ctx, reference, 0, GetVmType(ctx.ctx, ctx.ctx.typeTypeID));
	}
	else if(function->name->name == InplaceStr("auto_array") && function->arguments.size() == 2 && function->arguments[0].type == ctx.ctx.typeTypeID && function->arguments[1].type == ctx.ctx.typeInt)
	{
		VmConstant *type = GetArgumentValue(ctx, function, 0);

		if(!type)
			return NULL;

		VmConstant *count = GetArgumentValue(ctx, function, 1);

		if(!count)
			return NULL;

		VmConstant *result = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, VmType::AutoArray, NULL);

		char *storage = (char*)ctx.allocator->alloc(result->type.size);
		
		result->sValue = storage;

		CopyConstantRaw(ctx, storage + 0, result->type.size, type, type->type.size);

		VmConstant *resultPtr = AllocateHeapObject(ctx, ctx.ctx.GetArrayType(ctx.ctx.types[type->iValue], count->iValue));
		
		CopyConstantRaw(ctx, storage + 4, result->type.size - 4, resultPtr, resultPtr->type.size);

		CopyConstantRaw(ctx, storage + 4 + sizeof(void*), result->type.size - 4 - sizeof(void*), count, count->type.size);

		return result;
	}
	else if(function->name->name == InplaceStr("array_copy") && function->arguments.size() == 2 && function->arguments[0].type == ctx.ctx.typeAutoArray && function->arguments[1].type == ctx.ctx.typeAutoArray)
	{
		VmConstant *dst = GetArgumentValue(ctx, function, 0);

		if(!dst)
			return NULL;

		VmConstant *src = GetArgumentValue(ctx, function, 1);

		if(!src)
			return NULL;

		VmConstant *dstTypeID = ExtractValue(ctx, dst, 0, GetVmType(ctx.ctx, ctx.ctx.typeTypeID));
		VmConstant *dstPtr = ExtractValue(ctx, dst, 4, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		VmConstant *dstLen = ExtractValue(ctx, dst, 4 + sizeof(void*), GetVmType(ctx.ctx, ctx.ctx.typeInt));

		VmConstant *srcTypeID = ExtractValue(ctx, src, 0, GetVmType(ctx.ctx, ctx.ctx.typeTypeID));
		VmConstant *srcPtr = ExtractValue(ctx, src, 4, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		VmConstant *srcLen = ExtractValue(ctx, src, 4 + sizeof(void*), GetVmType(ctx.ctx, ctx.ctx.typeInt));

		if(!dstPtr->iValue && !srcPtr->iValue)
			return CreateConstantVoid(ctx.allocator);

		if(!srcPtr->iValue || dstPtr->iValue == srcPtr->iValue)
			return CreateConstantVoid(ctx.allocator);

		if(dstTypeID->iValue != srcTypeID->iValue)
			return (VmConstant*)Report(ctx, "ERROR: destination element type '%.*s' doesn't match source element type '%.*s'", FMT_ISTR(ctx.ctx.types[dstTypeID->iValue]->name), FMT_ISTR(ctx.ctx.types[srcTypeID->iValue]->name));

		if(dstLen->iValue < srcLen->iValue)
			return (VmConstant*)Report(ctx, "ERROR: destination array size '%d' is smaller than source array size '%d'", unsigned(dstLen->iValue), unsigned(srcLen->iValue));

		TypeBase *arrayType = ctx.ctx.GetArrayType(ctx.ctx.types[srcTypeID->iValue], srcLen->iValue);

		VmConstant *copy = LoadFrameValue(ctx, srcPtr, NULL, GetVmType(ctx.ctx, arrayType), unsigned(arrayType->size));

		if(!copy)
			return NULL;

		StoreFrameValue(ctx, dstPtr, 0, copy, unsigned(arrayType->size));

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("__assertCoroutine") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeAutoRef)
	{
		VmConstant *ptr = GetArgumentValue(ctx, function, 0);

		if(!ptr)
			return NULL;

		VmConstant *ptrTypeID = ExtractValue(ctx, ptr, 0, GetVmType(ctx.ctx, ctx.ctx.typeTypeID));

		TypeBase *targetType = ctx.ctx.types[ptrTypeID->iValue];

		if(!isType<TypeFunction>(targetType))
			return (VmConstant*)Report(ctx, "ERROR: '%.*s' is not a function'", FMT_ISTR(targetType->name));

		VmConstant *ptrPtr = ExtractValue(ctx, ptr, 4, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(targetType)));

		VmConstant *functionRef = LoadFrameValue(ctx, ptrPtr, NULL, GetVmType(ctx.ctx, targetType), unsigned(targetType->size));

		if(!functionRef)
			return NULL;

		VmConstant *functionIndex = ExtractValue(ctx, functionRef, sizeof(void*), GetVmType(ctx.ctx, ctx.ctx.typeFunctionID));

		if(unsigned(functionIndex->iValue) >= ctx.ctx.functions.size())
			return (VmConstant*)Report(ctx, "ERROR: invalid function index");

		FunctionData *functionValue = ctx.ctx.functions[functionIndex->iValue];

		if(!functionValue->coroutine)
			return (VmConstant*)Report(ctx, "ERROR: '%.*s' is not a coroutine'", FMT_ISTR(functionValue->name->name));

		return CreateConstantVoid(ctx.allocator);
	}
	else if(function->name->name == InplaceStr("isCoroutineReset") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.typeAutoRef)
	{
		VmConstant *ptr = GetArgumentValue(ctx, function, 0);

		if(!ptr)
			return NULL;

		VmConstant *ptrTypeID = ExtractValue(ctx, ptr, 0, GetVmType(ctx.ctx, ctx.ctx.typeTypeID));

		TypeBase *targetType = ctx.ctx.types[ptrTypeID->iValue];

		if(!isType<TypeFunction>(targetType))
			return (VmConstant*)Report(ctx, "ERROR: '%.*s' is not a function'", FMT_ISTR(targetType->name));

		VmConstant *ptrPtr = ExtractValue(ctx, ptr, 4, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(targetType)));

		VmConstant *functionRef = LoadFrameValue(ctx, ptrPtr, NULL, GetVmType(ctx.ctx, targetType), unsigned(targetType->size));

		if(!functionRef)
			return NULL;

		VmConstant *functionIndex = ExtractValue(ctx, functionRef, sizeof(void*), GetVmType(ctx.ctx, ctx.ctx.typeFunctionID));

		if(unsigned(functionIndex->iValue) >= ctx.ctx.functions.size())
			return (VmConstant*)Report(ctx, "ERROR: invalid function index");

		FunctionData *functionValue = ctx.ctx.functions[functionIndex->iValue];

		if(!functionValue->coroutine)
			return (VmConstant*)Report(ctx, "ERROR: '%.*s' is not a coroutine'", FMT_ISTR(functionValue->name->name));

		VmConstant *functionContextPtr = ExtractValue(ctx, functionRef, 0, GetVmType(ctx.ctx, functionValue->contextType));

		if(!functionContextPtr->iValue)
			return (VmConstant*)Report(ctx, "ERROR: null pointer access");

		TypeRef *refType = getType<TypeRef>(functionValue->contextType);

		VmType contextType = GetVmType(ctx.ctx, refType->subType);

		VmConstant *functionContext = LoadFrameValue(ctx, functionContextPtr, NULL, contextType, contextType.size);

		if(!functionContext)
			return NULL;

		VmConstant *jmpOffset = ExtractValue(ctx, functionContext, 0, GetVmType(ctx.ctx, ctx.ctx.typeInt));

		return CreateConstantInt(ctx.allocator, NULL, jmpOffset->iValue == 0);
	}
	else if(function->name->name == InplaceStr("assert_derived_from_base") && function->arguments.size() == 2 && function->arguments[0].type == ctx.ctx.GetReferenceType(ctx.ctx.typeVoid) && function->arguments[1].type == ctx.ctx.typeTypeID)
	{
		VmConstant *ptr = GetArgumentValue(ctx, function, 0);

		if(!ptr)
			return NULL;

		VmConstant *base = GetArgumentValue(ctx, function, 1);

		if(!base)
			return NULL;

		if(!ptr->iValue)
			return ptr;

		VmConstant *derived = LoadFrameValue(ctx, ptr, NULL, GetVmType(ctx.ctx, ctx.ctx.typeTypeID), unsigned(ctx.ctx.typeTypeID->size));

		if(!derived)
			return NULL;

		TypeBase *curr = ctx.ctx.types[derived->iValue];

		while(curr)
		{
			if(curr == ctx.ctx.types[base->iValue])
				return ptr;

			if(TypeClass *classType = getType<TypeClass>(curr))
				curr = classType->baseClass;
			else
				curr = NULL;
		}

		return (VmConstant*)Report(ctx, "ERROR: cannot convert from '%.*s' to '%.*s'", FMT_ISTR(ctx.ctx.types[derived->iValue]->name), FMT_ISTR(ctx.ctx.types[base->iValue]->name));
	}
	else if(function->name->name == InplaceStr("int::str") && function->arguments.size() == 0)
	{
		VmConstant *context = GetContextValue(ctx, function);

		if(!context)
			return NULL;

		VmConstant *value = LoadFrameValue(ctx, context, NULL, VmType::Int, 4);

		if(!value)
			return NULL;

		char buf[32];
		sprintf(buf, "%d", value->iValue);

		unsigned length = unsigned(strlen(buf) + 1);

		VmConstant *result = AllocateHeapArray(ctx, ctx.ctx.typeChar, length);

		VmConstant *pointer = ExtractValue(ctx, result, 0, VmType::Pointer(ctx.ctx.typeChar));

		StoreFrameValue(ctx, pointer, 0, CreateConstantStruct(ctx.allocator, NULL, buf, (length + 3) & ~3, ctx.ctx.GetArrayType(ctx.ctx.typeChar, length)), length);

		return result;
	}
	else if(function->name->name == InplaceStr("long::str") && function->arguments.size() == 0)
	{
		VmConstant *context = GetContextValue(ctx, function);

		if(!context)
			return NULL;

		VmConstant *value = LoadFrameValue(ctx, context, NULL, VmType::Long, 8);

		if(!value)
			return NULL;

		char buf[32];
		sprintf(buf, "%lld", value->lValue);

		unsigned length = unsigned(strlen(buf) + 1);

		VmConstant *result = AllocateHeapArray(ctx, ctx.ctx.typeChar, length);

		VmConstant *pointer = ExtractValue(ctx, result, 0, VmType::Pointer(ctx.ctx.typeChar));

		StoreFrameValue(ctx, pointer, 0, CreateConstantStruct(ctx.allocator, NULL, buf, (length + 3) & ~3, ctx.ctx.GetArrayType(ctx.ctx.typeChar, length)), length);

		return result;
	}
	else if(function->name->name == InplaceStr("int") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeChar))
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		VmConstant *valuePtr = ExtractValue(ctx, value, 0, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.typeChar)));
		VmConstant *valueLen = ExtractValue(ctx, value, sizeof(void*), GetVmType(ctx.ctx, ctx.ctx.typeInt));

		if(valueLen->iValue == 0 || valueLen->iValue >= 32)
			return CreateConstantInt(ctx.allocator, NULL, 0);

		VmConstant *valueBuf = LoadFrameValue(ctx, valuePtr, NULL, GetVmType(ctx.ctx, ctx.ctx.GetArrayType(ctx.ctx.typeChar, valueLen->iValue)), valueLen->iValue);

		if(!valueBuf)
			return NULL;

		char buf[32];
		strcpy(buf, valueBuf->sValue);

		return CreateConstantInt(ctx.allocator, NULL, strtol(buf, 0, 10));
	}
	else if(function->name->name == InplaceStr("long") && function->arguments.size() == 1 && function->arguments[0].type == ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeChar))
	{
		VmConstant *value = GetArgumentValue(ctx, function, 0);

		if(!value)
			return NULL;

		VmConstant *valuePtr = ExtractValue(ctx, value, 0, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.typeChar)));
		VmConstant *valueLen = ExtractValue(ctx, value, sizeof(void*), GetVmType(ctx.ctx, ctx.ctx.typeInt));

		if(valueLen->iValue == 0 || valueLen->iValue >= 32)
			return CreateConstantLong(ctx.allocator, NULL, 0);

		VmConstant *valueBuf = LoadFrameValue(ctx, valuePtr, NULL, GetVmType(ctx.ctx, ctx.ctx.GetArrayType(ctx.ctx.typeChar, valueLen->iValue)), valueLen->iValue);

		if(!valueBuf)
			return NULL;

		char buf[32];
		strcpy(buf, valueBuf->sValue);

		return CreateConstantLong(ctx.allocator, NULL, StrToLong(buf));
	}
	else if((function->name->name == InplaceStr("==") || function->name->name == InplaceStr("!=")) && function->arguments.size() == 2 && function->arguments[0].type == ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeChar) && function->arguments[1].type == ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeChar))
	{
		VmConstant *lhs = GetArgumentValue(ctx, function, 0);

		if(!lhs)
			return NULL;

		VmConstant *rhs = GetArgumentValue(ctx, function, 1);

		if(!rhs)
			return NULL;

		VmConstant *lhsPtr = ExtractValue(ctx, lhs, 0, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.typeChar)));
		VmConstant *lhsLen = ExtractValue(ctx, lhs, sizeof(void*), GetVmType(ctx.ctx, ctx.ctx.typeInt));

		VmConstant *rhsPtr = ExtractValue(ctx, rhs, 0, GetVmType(ctx.ctx, ctx.ctx.GetReferenceType(ctx.ctx.typeChar)));
		VmConstant *rhsLen = ExtractValue(ctx, rhs, sizeof(void*), GetVmType(ctx.ctx, ctx.ctx.typeInt));

		if(lhsLen->iValue != rhsLen->iValue)
			return CreateConstantInt(ctx.allocator, NULL, function->name->name == InplaceStr("==") ? 0 : 1);

		VmConstant *lhsBuf = LoadFrameValue(ctx, lhsPtr, NULL, GetVmType(ctx.ctx, ctx.ctx.GetArrayType(ctx.ctx.typeChar, lhsLen->iValue)), lhsLen->iValue);

		if(!lhsBuf)
			return NULL;

		VmConstant *rhsBuf = LoadFrameValue(ctx, rhsPtr, NULL, GetVmType(ctx.ctx, ctx.ctx.GetArrayType(ctx.ctx.typeChar, rhsLen->iValue)), rhsLen->iValue);

		if(!rhsBuf)
			return NULL;

		int order = memcmp(lhsBuf->sValue, rhsBuf->sValue, lhsLen->iValue);

		return CreateConstantInt(ctx.allocator, NULL, function->name->name == InplaceStr("==") ? order == 0 : order != 0);
	}
	else if(function->name->name == InplaceStr("__closeUpvalue"))
	{
		VmConstant *upvalueListLocation = GetArgumentValue(ctx, function, 0);

		if(!upvalueListLocation)
			return NULL;

		VmConstant *variableLocation = GetArgumentValue(ctx, function, 1);

		if(!variableLocation)
			return NULL;

		VmConstant *offsetToCopy = GetArgumentValue(ctx, function, 2);

		if(!offsetToCopy)
			return NULL;

		VmConstant *copySize = GetArgumentValue(ctx, function, 3);

		if(!copySize)
			return NULL;

		VmConstant *upvalueListHead = LoadFrameValue(ctx, upvalueListLocation, NULL, VmType::Pointer(ctx.ctx.typeVoid), NULLC_PTR_SIZE);

		if(!upvalueListHead)
			return NULL;

		char *upvalueListHeadData = GetPointerDataPtr(ctx, upvalueListHead->iValue);

		if(!upvalueListHeadData)
			return CreateConstantVoid(ctx.allocator);

		struct Upvalue
		{
			intptr_t target;
			intptr_t next;
		};

		int upvalueVmPtr = upvalueListHead->iValue;
		Upvalue *upvalueDataPtr = (Upvalue*)upvalueListHeadData;

		while(upvalueDataPtr && ReadVmMemoryPointer(&upvalueDataPtr->target) == variableLocation->iValue)
		{
			int nextVmPtr = (int)ReadVmMemoryPointer(&upvalueDataPtr->next);
			Upvalue *nextDataPtr = (Upvalue*)GetPointerDataPtr(ctx, nextVmPtr);

			char *variableLocationData = GetPointerDataPtr(ctx, variableLocation->iValue);

			char *copy = (char*)upvalueDataPtr + offsetToCopy->iValue;
			memcpy(copy, variableLocationData, unsigned(copySize->iValue));
			WriteVmMemoryPointer(&upvalueDataPtr->target, upvalueVmPtr + offsetToCopy->iValue);
			WriteVmMemoryPointer(&upvalueDataPtr->next, 0);

			upvalueVmPtr = nextVmPtr;
			upvalueDataPtr = nextDataPtr;
		}

		StoreFrameValue(ctx, upvalueListLocation, 0, CreateConstantPointer(ctx.allocator, NULL, upvalueVmPtr, NULL, ctx.ctx.typeVoid, false), NULLC_PTR_SIZE);

		return CreateConstantVoid(ctx.allocator);
	}

	return NULL;
}

VmConstant* EvaluateFunction(InstructionVMEvalContext &ctx, VmFunction *function)
{
	InstructionVMEvalContext::StackFrame *frame = ctx.stackFrames.back();

	VmBlock *predecessorBlock = NULL;
	VmBlock *currentBlock = function->firstBlock;

	if(function->function && !function->function->declaration)
	{
		bool isBaseModuleFunction = ctx.ctx.GetFunctionIndex(function->function) < ctx.ctx.baseModuleFunctionCount;

		if(ctx.emulateKnownExternals && isBaseModuleFunction)
		{
			if(VmConstant *result = EvaluateKnownExternalFunction(ctx, function->function))
				return result;

			return (VmConstant*)Report(ctx, "ERROR: function '%.*s' has no source", FMT_ISTR(function->function->name->name));
		}

		return (VmConstant*)Report(ctx, "ERROR: imported function has no source");
	}

	if(!function->firstBlock)
		return (VmConstant*)Report(ctx, "ERROR: function implementation was not found");

	if(ctx.printExecution)
	{
		if(function->function)
			printf("EvaluateFunction %.*s\n", FMT_ISTR(function->function->name->name));
		else
			printf("EvaluateFunction 'global'\n");
	}

	while(currentBlock)
	{
		if(ctx.printExecution)
			printf("  EvaluateFunction block %.*s\n", FMT_ISTR(currentBlock->name));

		if(!currentBlock->firstInstruction)
			return (VmConstant*)Report(ctx, "ERROR: function didn't return a value");

		for(VmInstruction *instruction = currentBlock->firstInstruction; instruction; instruction = instruction->nextSibling)
		{
			VmBlock *nextBlock = NULL;

			VmConstant *result = EvaluateInstruction(ctx, instruction, predecessorBlock, &nextBlock);

			if(ctx.hasError)
				return NULL;

			frame->AssignRegister(instruction->uniqueId, result);

			if(instruction->cmd == VM_INST_RETURN || instruction->cmd == VM_INST_YIELD)
			{
				if(result && result->container)
				{
					assert(result->type.type == VM_TYPE_POINTER);

					VmConstant *clean = new (ctx.get<VmConstant>()) VmConstant(ctx.allocator, result->type, NULL);

					CopyConstantRaw(ctx, (char*)&clean->iValue, clean->type.size, result, result->type.size);

					return clean;
				}

				return result;
			}

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

VmConstant* EvaluateModule(InstructionVMEvalContext &ctx, VmModule *module)
{
	VmFunction *global = module->functions.tail;

	ctx.hasError = false;

	ctx.heap.Reserve(ctx, 0, 4096);
	ctx.heapSize += 4096;

	ctx.globalFrame = new (ctx.get<InstructionVMEvalContext::StackFrame>()) InstructionVMEvalContext::StackFrame(ctx.allocator, global);
	ctx.stackFrames.push_back(ctx.globalFrame);

	VmConstant *result = EvaluateFunction(ctx, global);

	if(ctx.hasError)
		return NULL;

	ctx.stackFrames.pop_back();

	assert(ctx.stackFrames.empty());

	return result;
}

bool EvaluateToBuffer(InstructionVMEvalContext &ctx, VmModule *module, char *resultBuf, unsigned resultBufSize)
{
	if(VmConstant *value = EvaluateModule(ctx, module))
	{
		if(value->type == VmType::Int)
		{
			NULLC::SafeSprintf(resultBuf, resultBufSize, "%d", value->iValue);
		}
		else if(value->type == VmType::Double)
		{
			NULLC::SafeSprintf(resultBuf, resultBufSize, "%f", value->dValue);
		}
		else if(value->type == VmType::Long)
		{
			NULLC::SafeSprintf(resultBuf, resultBufSize, "%lldL", value->lValue);
		}
		else
		{
			NULLC::SafeSprintf(resultBuf, resultBufSize, "unknown");
		}

		return true;
	}

	return false;
}

bool TestEvaluation(ExpressionContext &ctx, VmModule *module, char *resultBuf, unsigned resultBufSize, char *errorBuf, unsigned errorBufSize)
{
	InstructionVMEvalContext evalCtx(ctx, ctx.allocator);

	evalCtx.errorBuf = errorBuf;
	evalCtx.errorBufSize = errorBufSize;

	evalCtx.emulateKnownExternals = true;

	return EvaluateToBuffer(evalCtx, module, resultBuf, resultBufSize);
}
