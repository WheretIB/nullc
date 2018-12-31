#include "InstructionTreeVm.h"

#include "ExpressionTree.h"
#include "InstructionTreeVmCommon.h"

#define allocate(T) new (module->get<T>()) T

// TODO: VM code generation should use a special pointer type to generate special pointer instructions
#ifdef _M_X64
	#define VM_INST_LOAD_POINTER VM_INST_LOAD_LONG
	#define VM_INST_STORE_POINTER VM_INST_STORE_LONG
#else
	#define VM_INST_LOAD_POINTER VM_INST_LOAD_INT
	#define VM_INST_STORE_POINTER VM_INST_STORE_INT
#endif

namespace
{
	VmValue* CheckType(ExpressionContext &ctx, ExprBase* expr, VmValue *value)
	{
		VmType exprType = GetVmType(ctx, expr->type);

		assert(exprType == value->type);

		return value;
	}

	VmValue* CreateVoid(VmModule *module)
	{
		return allocate(VmVoid)(module->allocator);
	}

	VmBlock* CreateBlock(VmModule *module, SynBase *source, const char *name)
	{
		return allocate(VmBlock)(module->allocator, source, InplaceStr(name), module->currentFunction->nextBlockId++);
	}

	bool IsBlockTerminator(VmInstructionType cmd)
	{
		switch(cmd)
		{
		case VM_INST_JUMP:
		case VM_INST_JUMP_Z:
		case VM_INST_JUMP_NZ:
		case VM_INST_RETURN:
		case VM_INST_YIELD:
		case VM_INST_UNYIELD:
			return true;
		}

		return false;
	}

	bool HasSideEffects(VmInstructionType cmd)
	{
		switch(cmd)
		{
		case VM_INST_STORE_BYTE:
		case VM_INST_STORE_SHORT:
		case VM_INST_STORE_INT:
		case VM_INST_STORE_FLOAT:
		case VM_INST_STORE_DOUBLE:
		case VM_INST_STORE_LONG:
		case VM_INST_STORE_STRUCT:
		case VM_INST_SET_RANGE:
		case VM_INST_JUMP:
		case VM_INST_JUMP_Z:
		case VM_INST_JUMP_NZ:
		case VM_INST_CALL:
		case VM_INST_RETURN:
		case VM_INST_YIELD:
		case VM_INST_CREATE_CLOSURE:
		case VM_INST_CLOSE_UPVALUES:
		case VM_INST_CHECKED_RETURN:
		case VM_INST_UNYIELD:
			return true;
		}

		return false;
	}

	bool HasMemoryWrite(VmInstructionType cmd)
	{
		switch(cmd)
		{
		case VM_INST_STORE_BYTE:
		case VM_INST_STORE_SHORT:
		case VM_INST_STORE_INT:
		case VM_INST_STORE_FLOAT:
		case VM_INST_STORE_DOUBLE:
		case VM_INST_STORE_LONG:
		case VM_INST_STORE_STRUCT:
		case VM_INST_SET_RANGE:
		case VM_INST_CALL:
		case VM_INST_CREATE_CLOSURE:
		case VM_INST_CLOSE_UPVALUES:
			return true;
		}

		return false;
	}

	bool HasMemoryAccess(VmInstructionType cmd)
	{
		switch(cmd)
		{
		case VM_INST_LOAD_BYTE:
		case VM_INST_LOAD_SHORT:
		case VM_INST_LOAD_INT:
		case VM_INST_LOAD_FLOAT:
		case VM_INST_LOAD_DOUBLE:
		case VM_INST_LOAD_LONG:
		case VM_INST_LOAD_STRUCT:
		case VM_INST_STORE_BYTE:
		case VM_INST_STORE_SHORT:
		case VM_INST_STORE_INT:
		case VM_INST_STORE_FLOAT:
		case VM_INST_STORE_DOUBLE:
		case VM_INST_STORE_LONG:
		case VM_INST_STORE_STRUCT:
		case VM_INST_SET_RANGE:
		case VM_INST_CALL:
		case VM_INST_CREATE_CLOSURE:
		case VM_INST_CLOSE_UPVALUES:
			return true;
		}

		return false;
	}

	VmInstruction* CreateInstruction(VmModule *module, SynBase *source, VmType type, VmInstructionType cmd, VmValue *first, VmValue *second, VmValue *third, VmValue *fourth)
	{
		assert(module->currentBlock);

		VmInstruction *inst = allocate(VmInstruction)(module->allocator, type, source, cmd, module->currentFunction->nextInstructionId++);

		if(first)
			inst->AddArgument(first);

		if(second)
			inst->AddArgument(second);

		if(third)
			inst->AddArgument(third);

		if(fourth)
			inst->AddArgument(fourth);

		inst->hasSideEffects = HasSideEffects(inst->cmd);
		inst->hasMemoryAccess = HasMemoryAccess(inst->cmd);

		module->currentBlock->AddInstruction(inst);

		return inst;
	}

	VmInstruction* CreateInstruction(VmModule *module, SynBase *source, VmType type, VmInstructionType cmd)
	{
		return CreateInstruction(module, source, type, cmd, NULL, NULL, NULL, NULL);
	}

	VmInstruction* CreateInstruction(VmModule *module, SynBase *source, VmType type, VmInstructionType cmd, VmValue *first)
	{
		return CreateInstruction(module, source, type, cmd, first, NULL, NULL, NULL);
	}

	VmInstruction* CreateInstruction(VmModule *module, SynBase *source, VmType type, VmInstructionType cmd, VmValue *first, VmValue *second)
	{
		return CreateInstruction(module, source, type, cmd, first, second, NULL, NULL);
	}

	VmInstruction* CreateInstruction(VmModule *module, SynBase *source, VmType type, VmInstructionType cmd, VmValue *first, VmValue *second, VmValue *third)
	{
		return CreateInstruction(module, source, type, cmd, first, second, third, NULL);
	}

	VmValue* CreateLoad(ExpressionContext &ctx, VmModule *module, SynBase *source, TypeBase *type, VmValue *address)
	{
		if(type == ctx.typeBool || type == ctx.typeChar)
			return CreateInstruction(module, source, VmType::Int, VM_INST_LOAD_BYTE, address);

		if(type == ctx.typeShort)
			return CreateInstruction(module, source, VmType::Int, VM_INST_LOAD_SHORT, address);

		if(type == ctx.typeInt)
			return CreateInstruction(module, source, VmType::Int, VM_INST_LOAD_INT, address);

		if(type == ctx.typeFloat)
			return CreateInstruction(module, source, VmType::Double, VM_INST_LOAD_FLOAT, address);

		if(type == ctx.typeDouble)
			return CreateInstruction(module, source, VmType::Double, VM_INST_LOAD_DOUBLE, address);

		if(type == ctx.typeLong)
			return CreateInstruction(module, source, VmType::Long, VM_INST_LOAD_LONG, address);

		if(isType<TypeRef>(type))
			return CreateInstruction(module, source, VmType::Pointer(type), VM_INST_LOAD_POINTER, address);

		if(isType<TypeFunction>(type))
			return CreateInstruction(module, source, VmType::FunctionRef(type), VM_INST_LOAD_STRUCT, address);

		if(isType<TypeUnsizedArray>(type))
			return CreateInstruction(module, source, VmType::ArrayRef(type), VM_INST_LOAD_STRUCT, address);

		if(type == ctx.typeAutoRef)
			return CreateInstruction(module, source, VmType::AutoRef, VM_INST_LOAD_STRUCT, address);

		if(type == ctx.typeAutoArray)
			return CreateInstruction(module, source, VmType::AutoArray, VM_INST_LOAD_STRUCT, address);

		if(isType<TypeTypeID>(type) || isType<TypeFunctionID>(type) || isType<TypeEnum>(type))
			return CreateInstruction(module, source, VmType::Int, VM_INST_LOAD_INT, address);

		if(type->size == 0)
			return CreateConstantInt(module->allocator, source, 0);

		assert(type->size % 4 == 0);
		assert(type->size != 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);

		return CreateInstruction(module, source, VmType::Struct(type->size, type), VM_INST_LOAD_STRUCT, address);
	}

	VmValue* CreateStore(ExpressionContext &ctx, VmModule *module, SynBase *source, TypeBase *type, VmValue *address, VmValue *value)
	{
		assert(value->type == GetVmType(ctx, type));

		if(type == ctx.typeBool || type == ctx.typeChar)
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_BYTE, address, value);

		if(type == ctx.typeShort)
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_SHORT, address, value);

		if(type == ctx.typeInt)
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_INT, address, value);

		if(type == ctx.typeFloat)
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_FLOAT, address, value);

		if(type == ctx.typeDouble)
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_DOUBLE, address, value);

		if(type == ctx.typeLong)
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_LONG, address, value);

		if(isType<TypeRef>(type))
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_POINTER, address, value);

		if(isType<TypeEnum>(type))
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_INT, address, value);

		if(isType<TypeFunction>(type) || isType<TypeUnsizedArray>(type) || type == ctx.typeAutoRef || type == ctx.typeAutoArray)
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_STRUCT, address, value);

		if(isType<TypeTypeID>(type) || isType<TypeFunctionID>(type) || isType<TypeEnum>(type))
			return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_INT, address, value);

		if(type->size == 0)
			return CreateVoid(module);

		assert(type->size % 4 == 0);
		assert(type->size != 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);
		assert(value->type.type == VM_TYPE_STRUCT);

		return CreateInstruction(module, source, VmType::Void, VM_INST_STORE_STRUCT, address, value);
	}

	VmValue* CreateCast(VmModule *module, SynBase *source, VmValue *value, VmType target)
	{
		if(target == value->type)
			return value;

		if(target == VmType::Int)
		{
			if(value->type == VmType::Double)
				return CreateInstruction(module, source, target, VM_INST_DOUBLE_TO_INT, value);

			if(value->type == VmType::Long)
				return CreateInstruction(module, source, target, VM_INST_LONG_TO_INT, value);
		}
		else if(target == VmType::Double)
		{
			if(value->type == VmType::Int)
				return CreateInstruction(module, source, target, VM_INST_INT_TO_DOUBLE, value);

			if(value->type == VmType::Long)
				return CreateInstruction(module, source, target, VM_INST_LONG_TO_DOUBLE, value);
		}
		else if(target == VmType::Long)
		{
			if(value->type == VmType::Int)
				return CreateInstruction(module, source, target, VM_INST_INT_TO_LONG, value);

			if(value->type == VmType::Double)
				return CreateInstruction(module, source, target, VM_INST_DOUBLE_TO_LONG, value);
		}

		assert(!"unknown cast");

		return CreateVoid(module);
	}

	VmValue* CreateIndex(VmModule *module, SynBase *source, VmValue *arrayLength, VmValue *elementSize, VmValue *value, VmValue *index, TypeBase *structType)
	{
		assert(arrayLength->type == VmType::Int);
		assert(elementSize->type == VmType::Int);
		assert(value->type.type == VM_TYPE_POINTER);
		assert(index->type == VmType::Int);

		return CreateInstruction(module, source, VmType::Pointer(structType), VM_INST_INDEX, arrayLength, elementSize, value, index);
	}

	VmValue* CreateIndexUnsized(VmModule *module, SynBase *source, VmValue *elementSize, VmValue *value, VmValue *index, TypeBase *structType)
	{
		assert(value->type.type == VM_TYPE_ARRAY_REF);
		assert(elementSize->type == VmType::Int);
		assert(index->type == VmType::Int);

		return CreateInstruction(module, source, VmType::Pointer(structType), VM_INST_INDEX_UNSIZED, elementSize, value, index);
	}

	VmValue* CreateMemberAccess(VmModule *module, SynBase *source, VmValue *ptr, VmValue *shift, TypeBase *structType, InplaceStr name)
	{
		assert(ptr->type.type == VM_TYPE_POINTER);
		assert(shift->type == VmType::Int);

		VmInstruction *inst = CreateInstruction(module, source, VmType::Pointer(structType), VM_INST_ADD, ptr, shift);

		inst->comment = name;

		return inst;
	}

	VmValue* CreateAdd(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_ADD, lhs, rhs);
	}

	VmValue* CreateSub(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_SUB, lhs, rhs);
	}

	VmValue* CreateMul(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_MUL, lhs, rhs);
	}

	VmValue* CreateDiv(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_DIV, lhs, rhs);
	}

	VmValue* CreatePow(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_POW, lhs, rhs);
	}

	VmValue* CreateMod(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_MOD, lhs, rhs);
	}

	VmValue* CreateCompareLess(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, VmType::Int, VM_INST_LESS, lhs, rhs);
	}

	VmValue* CreateCompareGreater(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, VmType::Int, VM_INST_GREATER, lhs, rhs);
	}

	VmValue* CreateCompareLessEqual(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, VmType::Int, VM_INST_LESS_EQUAL, lhs, rhs);
	}

	VmValue* CreateCompareGreaterEqual(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, VmType::Int, VM_INST_GREATER_EQUAL, lhs, rhs);
	}

	VmValue* CreateCompareEqual(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long || lhs->type.type == VM_TYPE_POINTER);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, VmType::Int, VM_INST_EQUAL, lhs, rhs);
	}

	VmValue* CreateCompareNotEqual(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Double || lhs->type == VmType::Long || lhs->type.type == VM_TYPE_POINTER);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, VmType::Int, VM_INST_NOT_EQUAL, lhs, rhs);
	}

	VmValue* CreateShl(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_SHL, lhs, rhs);
	}

	VmValue* CreateShr(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_SHR, lhs, rhs);
	}

	VmValue* CreateAnd(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_BIT_AND, lhs, rhs);
	}

	VmValue* CreateOr(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_BIT_OR, lhs, rhs);
	}

	VmValue* CreateXor(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, lhs->type, VM_INST_BIT_XOR, lhs, rhs);
	}

	VmValue* CreateLogicalXor(VmModule *module, SynBase *source, VmValue *lhs, VmValue *rhs)
	{
		assert(lhs->type == VmType::Int || lhs->type == VmType::Long);
		assert(lhs->type == rhs->type);

		return CreateInstruction(module, source, VmType::Int, VM_INST_LOG_XOR, lhs, rhs);
	}

	VmValue* CreateNeg(VmModule *module, SynBase *source, VmValue *value)
	{
		assert(value->type == VmType::Int || value->type == VmType::Double || value->type == VmType::Long);

		return CreateInstruction(module, source, value->type, VM_INST_NEG, value);
	}

	VmValue* CreateNot(VmModule *module, SynBase *source, VmValue *value)
	{
		assert(value->type == VmType::Int || value->type == VmType::Long);

		return CreateInstruction(module, source, value->type, VM_INST_BIT_NOT, value);
	}

	VmValue* CreateLogicalNot(VmModule *module, SynBase *source, VmValue *value)
	{
		assert(value->type == VmType::Int || value->type == VmType::Long || value->type.type == VM_TYPE_POINTER);

		return CreateInstruction(module, source, VmType::Int, VM_INST_LOG_NOT, value);
	}

	VmValue* CreateJump(VmModule *module, SynBase *source, VmValue *label)
	{
		assert(label->type == VmType::Block);

		return CreateInstruction(module, source, VmType::Void, VM_INST_JUMP, label);
	}

	VmValue* CreateJumpZero(VmModule *module, SynBase *source, VmValue *value, VmValue *trueLabel, VmValue *falseLabel)
	{
		assert(value->type == VmType::Int);
		assert(trueLabel->type == VmType::Block);
		assert(falseLabel->type == VmType::Block);

		return CreateInstruction(module, source, VmType::Void, VM_INST_JUMP_Z, value, trueLabel, falseLabel);
	}

	VmValue* CreateJumpNotZero(VmModule *module, SynBase *source, VmValue *value, VmValue *trueLabel, VmValue *falseLabel)
	{
		assert(value->type == VmType::Int);
		assert(trueLabel->type == VmType::Block);
		assert(falseLabel->type == VmType::Block);

		return CreateInstruction(module, source, VmType::Void, VM_INST_JUMP_NZ, value, trueLabel, falseLabel);
	}

	VmValue* CreateReturn(VmModule *module, SynBase *source)
	{
		return CreateInstruction(module, source, VmType::Void, VM_INST_RETURN);
	}

	VmValue* CreateReturn(VmModule *module, SynBase *source, VmValue *value)
	{
		return CreateInstruction(module, source, VmType::Void, VM_INST_RETURN, value);
	}

	VmValue* CreateYield(VmModule *module, SynBase *source)
	{
		return CreateInstruction(module, source, VmType::Void, VM_INST_YIELD);
	}

	VmValue* CreateYield(VmModule *module, SynBase *source, VmValue *value)
	{
		return CreateInstruction(module, source, VmType::Void, VM_INST_YIELD, value);
	}

	VmValue* CreateVariableAddress(VmModule *module, SynBase *source, VariableData *variable, TypeBase *structType)
	{
		assert(!IsMemberScope(variable->scope));

		VmValue *value = CreateConstantPointer(module->allocator, source, 0, variable, structType, true);

		return value;
	}

	VmValue* CreateTypeIndex(VmModule *module, SynBase *source, TypeBase *type)
	{
		return CreateInstruction(module, source, VmType::Int, VM_INST_TYPE_ID, CreateConstantInt(module->allocator, source, type->typeIndex));
	}

	VmValue* CreateFunctionAddress(VmModule *module, SynBase *source, FunctionData *function)
	{
		return CreateInstruction(module, source, VmType::Int, VM_INST_FUNCTION_ADDRESS, CreateConstantInt(module->allocator, source, function->functionIndex));
	}

	VmValue* CreateConvertPtr(VmModule *module, SynBase *source, VmValue *ptr, TypeBase *type, TypeBase *structType)
	{
		return CreateInstruction(module, source, VmType::Pointer(structType), VM_INST_CONVERT_POINTER, ptr, CreateConstantInt(module->allocator, source, type->typeIndex));
	}

	VmValue* CreateConstruct(VmModule *module, SynBase *source, VmType type, VmValue *el0, VmValue *el1, VmValue *el2, VmValue *el3)
	{
		unsigned size = el0->type.size;

		if(el1)
			size += el1->type.size;

		if(el2)
			size += el2->type.size;

		if(el3)
			size += el3->type.size;

		assert(type.size == size);

		return CreateInstruction(module, source, type, VM_INST_CONSTRUCT, el0, el1, el2, el3);
	}

	VmValue* CreateExtract(VmModule *module, SynBase *source, VmType type, VmValue *value, unsigned offset)
	{
		assert(offset + type.size <= value->type.size);

		return CreateInstruction(module, source, type, VM_INST_EXTRACT, value, CreateConstantInt(module->allocator, source, offset));
	}

	VmValue* CreateLoadImmediate(VmModule *module, SynBase *source, VmConstant *value)
	{
		return CreateInstruction(module, source, value->type, VM_INST_LOAD_IMMEDIATE, value);
	}

	VmValue* CreatePhi(VmModule *module, SynBase *source, VmInstruction *valueA, VmInstruction *valueB)
	{
		assert(valueA);
		assert(valueB);
		assert(valueA->type == valueB->type);

		return CreateInstruction(module, source, valueA->type, VM_INST_PHI, valueA, valueA->parent, valueB, valueB->parent);
	}

	VmValue* CreateAlloca(ExpressionContext &ctx, VmModule *module, SynBase *source, TypeBase *type, const char *suffix)
	{
		char *name = (char*)ctx.allocator->alloc(16);
		sprintf(name, "$temp%d_%s", ctx.unnamedVariableCount++, suffix);

		VariableData *variable = allocate(VariableData)(ctx.allocator, NULL, NULL, type->alignment, type, InplaceStr(name), 0, 0);

		VmValue *value = CreateConstantPointer(module->allocator, source, 0, variable, ctx.GetReferenceType(variable->type), true);

		module->currentFunction->allocas.push_back(variable);

		return value;
	}

	ScopeData* AllocateScopeSlot(ExpressionContext &ctx, VmModule *module, TypeBase *type, unsigned &offset)
	{
		FunctionData *function = module->currentFunction->function;

		ScopeData *scope = NULL;
		offset = 0;

		if(function)
		{
			scope = function->functionScope;

			function->stackSize += GetAlignmentOffset(function->stackSize, type->alignment);

			offset = unsigned(function->stackSize);

			function->stackSize += type->size;
		}
		else
		{
			scope = ctx.globalScope;

			scope->dataSize += GetAlignmentOffset(scope->dataSize, type->alignment);

			offset = unsigned(scope->dataSize);

			scope->dataSize += type->size; // TODO: alignment
		}

		assert(scope);

		return scope;
	}

	void FinalizeAlloca(ExpressionContext &ctx, VmModule *module, VariableData *variable)
	{
		unsigned offset = 0;
		ScopeData *scope = AllocateScopeSlot(ctx, module, variable->type, offset);

		variable->offset = offset;

		scope->variables.push_back(variable);

		ctx.variables.push_back(variable);
	}

	void ChangeInstructionTo(VmModule *module, VmInstruction *inst, VmInstructionType cmd, VmValue *first, VmValue *second, VmValue *third, VmValue *fourth, unsigned *optCount)
	{
		inst->cmd = cmd;

		SmallArray<VmValue*, 128> arguments(module->allocator);
		arguments.reserve(inst->arguments.size());
		arguments.push_back(inst->arguments.data, inst->arguments.size());

		inst->arguments.clear();

		if(first)
			inst->AddArgument(first);

		if(second)
			inst->AddArgument(second);

		if(third)
			inst->AddArgument(third);

		if(fourth)
			inst->AddArgument(fourth);

		for(unsigned i = 0; i < arguments.size(); i++)
			arguments[i]->RemoveUse(inst);

		inst->hasSideEffects = HasSideEffects(cmd);
		inst->hasMemoryAccess = HasMemoryAccess(cmd);

		if(optCount)
			(*optCount)++;
	}

	void ReplaceValue(VmValue *value, VmValue *original, VmValue *replacement)
	{
		assert(original);
		assert(replacement);

		if(VmFunction *function = getType<VmFunction>(value))
		{
			if(original == function->firstBlock)
			{
				replacement->AddUse(function);
				original->RemoveUse(function);
			}

			for(VmBlock *curr = function->firstBlock; curr; curr = curr->nextSibling)
			{
				assert(curr != original || curr == function->firstBlock); // Function can only use first block

				ReplaceValue(curr, original, replacement);
			}
		}
		else if(VmBlock *block = getType<VmBlock>(value))
		{
			for(VmInstruction *curr = block->firstInstruction; curr; curr = curr->nextSibling)
			{
				assert(curr != original); // Block doesn't use instructions

				ReplaceValue(curr, original, replacement);
			}
		}
		else if(VmInstruction *inst = getType<VmInstruction>(value))
		{
			if(inst->cmd == VM_INST_PHI)
				return;

			for(unsigned i = 0; i < inst->arguments.size(); i++)
			{
				if(inst->arguments[i] == original)
				{
					replacement->AddUse(inst);

					inst->arguments[i] = replacement;

					original->RemoveUse(inst);
				}
			}
		}
		else
		{
			assert(!"unknown type");
		}
	}

	void ReplaceValueUsersWith(VmModule *module, VmValue *original, VmValue *replacement, unsigned *optCount)
	{
		SmallArray<VmValue*, 256> users(module->allocator);
		users.reserve(original->users.size());
		users.push_back(original->users.data, original->users.size());

		for(unsigned i = 0; i < users.size(); i++)
			ReplaceValue(users[i], original, replacement);

		if(VmBlock *block = getType<VmBlock>(original))
		{
			VmFunction *function = block->parent;

			for(unsigned i = 0; i < function->restoreBlocks.size(); i++)
			{
				if(original == function->restoreBlocks[i])
					function->restoreBlocks[i] = getType<VmBlock>(replacement);
			}
		}

		if(optCount)
			(*optCount)++;
	}

	void ClearLoadStoreInfo(VmModule *module)
	{
		module->loadStoreInfo.clear();
	}

	void ClearLoadStoreInfoAliasing(VmModule *module)
	{
		for(unsigned i = 0; i < module->loadStoreInfo.size();)
		{
			VmModule::LoadStoreInfo &el = module->loadStoreInfo[i];

			if(el.address && el.address->container && !HasAddressTaken(el.address->container))
			{
				i++;
				continue;
			}

			module->loadStoreInfo[i] = module->loadStoreInfo.back();
			module->loadStoreInfo.pop_back();
		}
	}

	void ClearLoadStoreInfoGlobal(VmModule *module)
	{
		for(unsigned i = 0; i < module->loadStoreInfo.size();)
		{
			VmModule::LoadStoreInfo &el = module->loadStoreInfo[i];

			if(el.address && el.address->container && IsGlobalScope(el.address->container->scope))
			{
				module->loadStoreInfo[i] = module->loadStoreInfo.back();
				module->loadStoreInfo.pop_back();
			}
			else
			{
				i++;
				continue;
			}
		}
	}

	void ClearLoadStoreInfo(VmModule *module, VariableData *container, unsigned storeOffset, unsigned storeSize)
	{
		assert(storeSize != 0);

		for(unsigned i = 0; i < module->loadStoreInfo.size();)
		{
			VmModule::LoadStoreInfo &el = module->loadStoreInfo[i];

			// Any opaque pointer might be clobbered
			if(el.pointer)
			{
				if(!HasAddressTaken(container))
				{
					i++;
					continue;
				}

				module->loadStoreInfo[i] = module->loadStoreInfo.back();
				module->loadStoreInfo.pop_back();
				continue;
			}

			unsigned otherOffset = unsigned(el.address->iValue);
			unsigned otherSize = GetAccessSize(el.loadInst ? el.loadInst : el.storeInst);

			assert(otherSize != 0);

			// (a+aw >= b) && (a <= b+bw)
			if(container == el.address->container && storeOffset + storeSize - 1 >= otherOffset && storeOffset <= otherOffset + otherSize - 1)
			{
				module->loadStoreInfo[i] = module->loadStoreInfo.back();
				module->loadStoreInfo.pop_back();
				continue;
			}

			i++;
		}
	}

	void AddLoadInfo(VmModule *module, VmInstruction* inst)
	{
		VmModule::LoadStoreInfo info;

		info.loadInst = inst;

		if(VmConstant *address = getType<VmConstant>(inst->arguments[0]))
			info.address = address;
		else
			info.pointer = inst->arguments[0];

		module->loadStoreInfo.push_back(info);
	}

	void AddStoreInfo(VmModule *module, VmInstruction* inst)
	{
		if(VmConstant *address = getType<VmConstant>(inst->arguments[0]))
		{
			VmModule::LoadStoreInfo info;

			info.storeInst = inst;

			info.address = address;

			// Remove previous loads and stores to this address range
			ClearLoadStoreInfo(module, address->container, unsigned(address->iValue), GetAccessSize(inst));

			module->loadStoreInfo.push_back(info);
		}
		else
		{
			// Check for index const const, const, ptr instruction, it might be possible to reduce the invalidation range
			if(VmInstruction *ptrArg = getType<VmInstruction>(inst->arguments[0]))
			{
				if(ptrArg->cmd == VM_INST_INDEX)
				{
					VmConstant *length = getType<VmConstant>(ptrArg->arguments[0]);
					VmConstant *elemSize = getType<VmConstant>(ptrArg->arguments[1]);

					assert(length && elemSize);

					if(VmConstant *base = getType<VmConstant>(ptrArg->arguments[2]))
					{
						unsigned storeOffset = unsigned(base->iValue);
						unsigned storeSize = length->iValue * elemSize->iValue;

						if(VmConstant *index = getType<VmConstant>(ptrArg->arguments[3]))
						{
							storeOffset += index->iValue * elemSize->iValue;
							storeSize = elemSize->iValue;
						}

						ClearLoadStoreInfo(module, base->container, storeOffset, storeSize);
						return;
					}
				}
			}

			ClearLoadStoreInfoAliasing(module);
		}
	}

	VmValue* TryExtractConstructElement(VmValue* value, unsigned offset, unsigned size)
	{
		VmInstruction *inst = getType<VmInstruction>(value);

		if(inst && (inst->cmd == VM_INST_CONSTRUCT || inst->cmd == VM_INST_ARRAY))
		{
			unsigned pos = 0;

			for(unsigned k = 0; k < inst->arguments.size(); k++)
			{
				VmValue *component = inst->arguments[k];

				if(pos == offset && size == component->type.size)
					return component;

				if(offset >= pos && offset + size <= pos + component->type.size)
					return TryExtractConstructElement(component, offset - pos, size);

				pos += component->type.size;
			}
		}

		return NULL;
	}

	VmValue* GetLoadStoreInfo(VmModule *module, VmInstruction* inst)
	{
		if(VmConstant *address = getType<VmConstant>(inst->arguments[0]))
		{
			for(unsigned i = 0; i < module->loadStoreInfo.size(); i++)
			{
				VmModule::LoadStoreInfo &el = module->loadStoreInfo[i];

				if(el.pointer)
					continue;

				// Reuse previous load
				if(el.loadInst && *el.address == *address && GetAccessSize(inst) == GetAccessSize(el.loadInst))
					return el.loadInst;

				// Reuse store argument
				if(el.storeInst && *el.address == *address && GetAccessSize(inst) == GetAccessSize(el.storeInst))
				{
					VmValue *value = el.storeInst->arguments[1];

					// Can't reuse arguments of a different size
					if(value->type.size != inst->type.size)
						return NULL;

					return value;
				}

				if(el.storeInst && el.address->container == address->container)
				{
					if(VmValue *component = TryExtractConstructElement(el.storeInst->arguments[1], address->iValue, GetAccessSize(inst)))
						return component;
				}
			}
		}
		else if(VmValue *pointer = inst->arguments[0])
		{
			for(unsigned i = 0; i < module->loadStoreInfo.size(); i++)
			{
				VmModule::LoadStoreInfo &el = module->loadStoreInfo[i];

				if(el.address)
					continue;

				if(el.loadInst && el.pointer == pointer && GetAccessSize(inst) == GetAccessSize(el.loadInst))
					return el.loadInst;
			}
		}

		return NULL;
	}

	bool IsLoad(VmValue *value)
	{
		if(VmInstruction *inst = getType<VmInstruction>(value))
		{
			switch(inst->cmd)
			{
			case VM_INST_LOAD_BYTE:
			case VM_INST_LOAD_SHORT:
			case VM_INST_LOAD_INT:
			case VM_INST_LOAD_FLOAT:
			case VM_INST_LOAD_DOUBLE:
			case VM_INST_LOAD_LONG:
			case VM_INST_LOAD_STRUCT:
				return true;
			}
		}

		return false;
	}

	TypeBase* GetBaseType(ExpressionContext &ctx, VmType type)
	{
		if(type == VmType::Void)
			return ctx.typeVoid;
		else if(type == VmType::Int)
			return ctx.typeInt;
		else if(type == VmType::Double)
			return ctx.typeDouble;
		else if(type == VmType::Long)
			return ctx.typeLong;
		else if(type.type == VM_TYPE_POINTER)
			return type.structType;
		else if(type.type == VM_TYPE_FUNCTION_REF)
			return type.structType;
		else if(type.type == VM_TYPE_ARRAY_REF)
			return type.structType;
		else if(type == VmType::AutoRef)
			return ctx.typeAutoRef;
		else if(type == VmType::AutoArray)
			return ctx.typeAutoArray;
		else if(type.type == VM_TYPE_STRUCT)
			return type.structType;
		else
			assert(!"unknown type");

		return NULL;
	}
}

const VmType VmType::Void = VmType(VM_TYPE_VOID, 0, NULL);
const VmType VmType::Int = VmType(VM_TYPE_INT, 4, NULL);
const VmType VmType::Double = VmType(VM_TYPE_DOUBLE, 8, NULL);
const VmType VmType::Long = VmType(VM_TYPE_LONG, 8, NULL);
const VmType VmType::Block = VmType(VM_TYPE_BLOCK, 4, NULL);
const VmType VmType::Function = VmType(VM_TYPE_FUNCTION, 4, NULL);
const VmType VmType::AutoRef = VmType(VM_TYPE_AUTO_REF, 4 + NULLC_PTR_SIZE, NULL); // type + ptr
const VmType VmType::AutoArray = VmType(VM_TYPE_AUTO_ARRAY, 4 + NULLC_PTR_SIZE + 4, NULL); // type + ptr + length

void VmValue::AddUse(VmValue* user)
{
	// Can't use empty values
	assert(type != VmType::Void);

	users.push_back(user);
}

void VmValue::RemoveUse(VmValue* user)
{
	for(unsigned i = 0; i < users.size(); i++)
	{
		if(users[i] == user)
		{
			users[i] = users.back();
			users.pop_back();
			break;
		}
	}

	if(users.empty() && !hasSideEffects && canBeRemoved)
	{
		if(VmConstant *constant = getType<VmConstant>(this))
		{
			if(VariableData *container = constant->container)
			{
				bool found = false;

				for(unsigned i = 0; i < container->users.size(); i++)
				{
					if(container->users[i] == constant)
					{
						found = true;

						container->users[i] = container->users.back();
						container->users.pop_back();
						break;
					}
				}

				assert(found);
			}
		}
		else if(VmInstruction *instruction = getType<VmInstruction>(this))
		{
			instruction->parent->RemoveInstruction(instruction);
		}
		else if(VmBlock *block = getType<VmBlock>(this))
		{
			// Remove all block instructions
			while(block->lastInstruction)
				block->RemoveInstruction(block->lastInstruction);
		}
		else if(VmFunction *function = getType<VmFunction>(this))
		{
			// Do not remove functions
		}
		else
		{
			assert(!"unknown type");
		}
	}
}

void VmInstruction::AddArgument(VmValue *argument)
{
	assert(argument);
	assert(argument->type != VmType::Void);

	arguments.push_back(argument);

	argument->AddUse(this);
}

void VmBlock::AddInstruction(VmInstruction* instruction)
{
	assert(instruction);
	assert(instruction->parent == NULL);
	assert(instruction->prevSibling == NULL);
	assert(instruction->nextSibling == NULL);

	instruction->parent = this;

	if(!firstInstruction)
	{
		assert(!insertPoint);

		firstInstruction = lastInstruction = instruction;
	}
	else
	{
		assert(insertPoint);

		if(insertPoint->nextSibling)
			insertPoint->nextSibling->prevSibling = instruction;

		instruction->nextSibling = insertPoint->nextSibling;

		insertPoint->nextSibling = instruction;
		instruction->prevSibling = insertPoint;

		if(insertPoint == lastInstruction)
			lastInstruction = instruction;
	}

	insertPoint = instruction;
}

void VmBlock::RemoveInstruction(VmInstruction* instruction)
{
	assert(instruction);
	assert(instruction->parent == this);
	assert(instruction->users.empty());

	if(instruction == firstInstruction)
		firstInstruction = instruction->nextSibling;

	if(instruction == lastInstruction)
		lastInstruction = instruction->prevSibling;

	if(instruction->prevSibling)
		instruction->prevSibling->nextSibling = instruction->nextSibling;
	if(instruction->nextSibling)
		instruction->nextSibling->prevSibling = instruction->prevSibling;

	instruction->parent = NULL;
	instruction->prevSibling = NULL;
	instruction->nextSibling = NULL;

	for(unsigned i = 0; i < instruction->arguments.size(); i++)
		instruction->arguments[i]->RemoveUse(instruction);
}

void VmFunction::AddBlock(VmBlock* block)
{
	assert(block);
	assert(block->parent == NULL);
	assert(block->prevSibling == NULL);
	assert(block->nextSibling == NULL);

	block->parent = this;

	if(!firstBlock)
	{
		firstBlock = lastBlock = block;
	}
	else
	{
		lastBlock->nextSibling = block;
		block->prevSibling = lastBlock;
		lastBlock = block;
	}
}

void VmFunction::DetachBlock(VmBlock *block)
{
	assert(block);
	assert(block->parent == this);

	if(block == firstBlock)
		firstBlock = block->nextSibling;

	if(block == lastBlock)
		lastBlock = block->prevSibling;

	if(block->prevSibling)
		block->prevSibling->nextSibling = block->nextSibling;
	if(block->nextSibling)
		block->nextSibling->prevSibling = block->prevSibling;

	block->parent = NULL;
	block->prevSibling = NULL;
	block->nextSibling = NULL;
}

void VmFunction::RemoveBlock(VmBlock* block)
{
	assert(block->users.empty());

	DetachBlock(block);

	while(block->lastInstruction)
		block->RemoveInstruction(block->lastInstruction);
}

void VmFunction::MoveEntryBlockToStart()
{
	if(!firstBlock)
		return;

	// Function must start from the first block
	VmBlock *entryBlock = NULL;

	for(VmBlock *curr = firstBlock; curr; curr = curr->nextSibling)
	{
		for(unsigned i = 0; i < curr->users.size(); i++)
		{
			if(curr->users[i] == this)
			{
				entryBlock = curr;
				break;
			}
		}

		if(entryBlock)
			break;
	}

	assert(entryBlock);

	if(firstBlock != entryBlock)
	{
		// Detach entry block
		DetachBlock(entryBlock);

		// Re-attach to front
		if(!firstBlock)
		{
			firstBlock = lastBlock = entryBlock;
		}
		else
		{
			firstBlock->prevSibling = entryBlock;
			entryBlock->nextSibling = firstBlock;
			firstBlock = entryBlock;
		}

		entryBlock->parent = this;
	}
}

VmType GetVmType(ExpressionContext &ctx, TypeBase *type)
{
	if(type == ctx.typeVoid)
		return VmType::Void;

	if(type == ctx.typeBool || type == ctx.typeChar || type == ctx.typeShort || type == ctx.typeInt)
		return VmType::Int;

	if(type == ctx.typeLong)
		return VmType::Long;

	if(type == ctx.typeFloat || type == ctx.typeDouble)
		return VmType::Double;

	if(isType<TypeRef>(type) || type == ctx.typeNullPtr)
		return VmType::Pointer(type);

	if(isType<TypeFunction>(type))
		return VmType::FunctionRef(type);

	if(isType<TypeUnsizedArray>(type))
		return VmType::ArrayRef(type);

	if(isType<TypeAutoRef>(type))
		return VmType::AutoRef;

	if(isType<TypeAutoArray>(type))
		return VmType::AutoArray;

	if(isType<TypeTypeID>(type))
		return VmType::Int;

	if(isType<TypeFunctionID>(type))
		return VmType::Int;

	if(isType<TypeArray>(type) || isType<TypeClass>(type))
	{
		if(isType<TypeClass>(type) && type->size == 0)
			return VmType::Int;

		assert(type->size % 4 == 0);
		assert(type->size != 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);

		return VmType::Struct(type->size, type);
	}

	if(isType<TypeEnum>(type))
		return VmType::Int;

	assert(!"unknown type");

	return VmType::Void;
}

VmValue* CompileVm(ExpressionContext &ctx, VmModule *module, ExprBase *expression)
{
	if(ExprVoid *node = getType<ExprVoid>(expression))
	{
		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprBoolLiteral *node = getType<ExprBoolLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateConstantInt(module->allocator, node->source, node->value ? 1 : 0));
	}
	else if(ExprCharacterLiteral *node = getType<ExprCharacterLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateConstantInt(module->allocator, node->source, node->value));
	}
	else if(ExprStringLiteral *node = getType<ExprStringLiteral>(expression))
	{
		unsigned size = node->length + 1;

		// Align to 4
		size = (size + 3) & ~3;

		char *value = (char*)ctx.allocator->alloc(size);
		memset(value, 0, size);

		for(unsigned i = 0; i < node->length; i++)
			value[i] = node->value[i];

		return CheckType(ctx, expression, CreateConstantStruct(module->allocator, node->source, value, size, node->type));
	}
	else if(ExprIntegerLiteral *node = getType<ExprIntegerLiteral>(expression))
	{
		if(node->type == ctx.typeShort)
			return CheckType(ctx, expression, CreateConstantInt(module->allocator, node->source, short(node->value)));

		if(node->type == ctx.typeInt)
			return CheckType(ctx, expression, CreateConstantInt(module->allocator, node->source, int(node->value)));

		if(node->type == ctx.typeLong)
			return CheckType(ctx, expression, CreateConstantLong(module->allocator, node->source, node->value));

		if(isType<TypeEnum>(node->type))
			return CheckType(ctx, expression, CreateConstantInt(module->allocator, node->source, int(node->value)));

		assert(!"unknown type");
	}
	else if(ExprRationalLiteral *node = getType<ExprRationalLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateConstantDouble(module->allocator, node->source, node->value));
	}
	else if(ExprTypeLiteral *node = getType<ExprTypeLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateTypeIndex(module, node->source, node->value));
	}
	else if(ExprNullptrLiteral *node = getType<ExprNullptrLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateConstantPointer(module->allocator, node->source, 0, NULL, node->type, false));
	}
	else if(ExprFunctionIndexLiteral *node = getType<ExprFunctionIndexLiteral>(expression))
	{
		return CheckType(ctx, expression, CreateFunctionAddress(module, node->source, node->function));
	}
	else if(ExprPassthrough *node = getType<ExprPassthrough>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		return CheckType(ctx, expression, value);
	}
	else if(ExprArray *node = getType<ExprArray>(expression))
	{
		VmInstruction *inst = allocate(VmInstruction)(module->allocator, GetVmType(ctx, node->type), node->source, VM_INST_ARRAY, module->currentFunction->nextInstructionId++);

		for(ExprBase *value = node->values.head; value; value = value->next)
			inst->AddArgument(CompileVm(ctx, module, value));

		module->currentBlock->AddInstruction(inst);

		return CheckType(ctx, expression, inst);
	}
	else if(ExprPreModify *node = getType<ExprPreModify>(expression))
	{
		VmValue *address = CompileVm(ctx, module, node->value);

		TypeRef *refType = getType<TypeRef>(node->value->type);

		assert(refType);

		VmValue *value = CreateLoad(ctx, module, node->source, refType->subType, address);

		if(value->type == VmType::Int)
			value = CreateAdd(module, node->source, value, CreateConstantInt(module->allocator, node->source, node->isIncrement ? 1 : -1));
		else if(value->type == VmType::Double)
			value = CreateAdd(module, node->source, value, CreateConstantDouble(module->allocator, node->source, node->isIncrement ? 1.0 : -1.0));
		else if(value->type == VmType::Long)
			value = CreateAdd(module, node->source, value, CreateConstantLong(module->allocator, node->source, node->isIncrement ? 1ll : -1ll));
		else
			assert("!unknown type");

		CreateStore(ctx, module, node->source, refType->subType, address, value);

		return CheckType(ctx, expression, value);

	}
	else if(ExprPostModify *node = getType<ExprPostModify>(expression))
	{
		VmValue *address = CompileVm(ctx, module, node->value);

		TypeRef *refType = getType<TypeRef>(node->value->type);

		assert(refType);

		VmValue *value = CreateLoad(ctx, module, node->source, refType->subType, address);
		VmValue *result = value;

		if(value->type == VmType::Int)
			value = CreateAdd(module, node->source, value, CreateConstantInt(module->allocator, node->source, node->isIncrement ? 1 : -1));
		else if(value->type == VmType::Double)
			value = CreateAdd(module, node->source, value, CreateConstantDouble(module->allocator, node->source, node->isIncrement ? 1.0 : -1.0));
		else if(value->type == VmType::Long)
			value = CreateAdd(module, node->source, value, CreateConstantLong(module->allocator, node->source, node->isIncrement ? 1ll : -1ll));
		else
			assert("!unknown type");

		CreateStore(ctx, module, node->source, refType->subType, address, value);

		return CheckType(ctx, expression, result);
	}
	else if(ExprTypeCast *node = getType<ExprTypeCast>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		switch(node->category)
		{
		case EXPR_CAST_NUMERICAL:
			return CheckType(ctx, expression, CreateCast(module, node->source, value, GetVmType(ctx, node->type)));
		case EXPR_CAST_PTR_TO_BOOL:
			return CheckType(ctx, expression, CreateCompareNotEqual(module, node->source, value, CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false)));
		case EXPR_CAST_UNSIZED_TO_BOOL:
			{
				TypeUnsizedArray *unsizedArrType = getType<TypeUnsizedArray>(node->value->type);

				assert(unsizedArrType);

				VmValue *ptr = CreateExtract(module, node->source, VmType::Pointer(ctx.GetReferenceType(unsizedArrType->subType)), value, 0);

				return CheckType(ctx, expression, CreateCompareNotEqual(module, node->source, ptr, CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false)));
			}
			break;
		case EXPR_CAST_FUNCTION_TO_BOOL:
			{
				VmValue *index = CreateExtract(module, node->source, VmType::Int, value, sizeof(void*));

				return CheckType(ctx, expression, CreateCompareNotEqual(module, node->source, index, CreateConstantInt(module->allocator, node->source, 0)));
			}
			break;
		case EXPR_CAST_NULL_TO_PTR:
			return CheckType(ctx, expression, CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false));
		case EXPR_CAST_NULL_TO_AUTO_PTR:
			return CheckType(ctx, expression, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateConstantInt(module->allocator, node->source, 0), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), NULL, NULL));
		case EXPR_CAST_NULL_TO_UNSIZED:
			return CheckType(ctx, expression, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), CreateConstantInt(module->allocator, node->source, 0), NULL, NULL));
		case EXPR_CAST_NULL_TO_AUTO_ARRAY:
			return CheckType(ctx, expression, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateConstantInt(module->allocator, node->source, 0), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), CreateConstantInt(module->allocator, node->source, 0), NULL));
		case EXPR_CAST_NULL_TO_FUNCTION:
			return CheckType(ctx, expression, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), CreateConstantInt(module->allocator, node->source, 0), NULL, NULL));
		case EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
			{
				TypeRef *refType = getType<TypeRef>(node->value->type);

				assert(refType);

				TypeArray *arrType = getType<TypeArray>(refType->subType);

				assert(arrType);
				assert(unsigned(arrType->length) == arrType->length);

				return CheckType(ctx, expression, CreateConstruct(module, node->source, GetVmType(ctx, node->type), value, CreateConstantInt(module->allocator, node->source, unsigned(arrType->length)), NULL, NULL));
			}
			break;
		case EXPR_CAST_ARRAY_PTR_TO_UNSIZED_PTR:
			{
				TypeRef *refType = getType<TypeRef>(node->value->type);

				assert(refType);

				TypeArray *arrType = getType<TypeArray>(refType->subType);

				assert(arrType);
				assert(unsigned(arrType->length) == arrType->length);

				TypeRef *targetRefType = getType<TypeRef>(node->type);

				assert(targetRefType);

				VmValue *address = CreateAlloca(ctx, module, node->source, targetRefType->subType, "arr_ptr");

				CreateStore(ctx, module, node->source, targetRefType->subType, address, CreateConstruct(module, node->source, GetVmType(ctx, targetRefType->subType), address, CreateConstantInt(module->allocator, node->source, unsigned(arrType->length)), NULL, NULL));

				return CheckType(ctx, expression, address);
			}
			break;
		case EXPR_CAST_PTR_TO_AUTO_PTR:
			{
				TypeRef *refType = getType<TypeRef>(node->value->type);

				assert(refType);

				TypeClass *classType = getType<TypeClass>(refType->subType);

				VmValue *typeId = NULL;

				if(classType && (classType->extendable || classType->baseClass))
					typeId = CreateLoad(ctx, module, node->source, ctx.typeTypeID, value);
				else
					typeId = CreateTypeIndex(module, node->source, refType->subType);

				return CheckType(ctx, expression, CreateConstruct(module, node->source, GetVmType(ctx, node->type), typeId, value, NULL, NULL));
			}
			break;
		case EXPR_CAST_ANY_TO_PTR:
			{
				VmValue *address = CreateAlloca(ctx, module, node->source, node->value->type, "lit");

				CreateStore(ctx, module, node->source, node->value->type, address, value);

				return CheckType(ctx, expression, address);
			}
			break;
		case EXPR_CAST_AUTO_PTR_TO_PTR:
			{
				TypeRef *refType = getType<TypeRef>(node->type);

				assert(refType);

				return CheckType(ctx, expression, CreateConvertPtr(module, node->source, value, refType->subType, ctx.GetReferenceType(refType->subType)));
			}
		case EXPR_CAST_UNSIZED_TO_AUTO_ARRAY:
			{
				TypeUnsizedArray *unsizedType = getType<TypeUnsizedArray>(node->value->type);

				assert(unsizedType);

				return CheckType(ctx, expression, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateTypeIndex(module, node->source, unsizedType->subType), value, NULL, NULL));
			}
		case EXPR_CAST_DERIVED_TO_BASE:
			{
				VmValue *address = CreateAlloca(ctx, module, node->source, node->value->type, "derived");

				CreateStore(ctx, module, node->source, node->value->type, address, value);

				VmValue *result = CreateLoad(ctx, module, node->source, node->type, address);

				return CheckType(ctx, expression, result);
			}
		case EXPR_CAST_REINTERPRET:
			return CheckType(ctx, expression, value);
		default:
			assert(!"unknown cast");
		}

		return CheckType(ctx, expression, value);
	}
	else if(ExprUnaryOp *node = getType<ExprUnaryOp>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		VmValue *result = NULL;

		switch(node->op)
		{
		case SYN_UNARY_OP_PLUS:
			result = value;
			break;
		case SYN_UNARY_OP_NEGATE:
			result = CreateNeg(module, node->source, value);
			break;
		case SYN_UNARY_OP_BIT_NOT:
			result = CreateNot(module, node->source, value);
			break;
		case SYN_UNARY_OP_LOGICAL_NOT:
			if(value->type == VmType::AutoRef)
			{
				result = CreateLogicalNot(module, node->source, CreateExtract(module, node->source, VmType::Pointer(ctx.GetReferenceType(ctx.typeVoid)), value, 4));
			}
			else
			{
				result = CreateLogicalNot(module, node->source, value);
			}
			break;
		}

		assert(result);

		return CheckType(ctx, expression, result);
	}
	else if(ExprBinaryOp *node = getType<ExprBinaryOp>(expression))
	{
		VmValue *lhs = CompileVm(ctx, module, node->lhs);

		if(node->op == SYN_BINARY_OP_LOGICAL_AND)
		{
			VmBlock *checkRhsBlock = CreateBlock(module, node->source, "land_check_rhs");
			VmBlock *storeOneBlock = CreateBlock(module, node->source, "land_store_1");
			VmBlock *storeZeroBlock = CreateBlock(module, node->source, "land_store_0");
			VmBlock *exitBlock = CreateBlock(module, node->source, "land_exit");

			CreateJumpZero(module, node->source, lhs, storeZeroBlock, checkRhsBlock);

			module->currentFunction->AddBlock(checkRhsBlock);
			module->currentBlock = checkRhsBlock;

			VmValue *rhs = CompileVm(ctx, module, node->rhs);

			CreateJumpZero(module, node->source, rhs, storeZeroBlock, storeOneBlock);

			module->currentFunction->AddBlock(storeOneBlock);
			module->currentBlock = storeOneBlock;

			VmValue *trueValue = CreateLoadImmediate(module, node->source, CreateConstantInt(module->allocator, node->source, 1));

			CreateJump(module, node->source, exitBlock);

			module->currentFunction->AddBlock(storeZeroBlock);
			module->currentBlock = storeZeroBlock;

			VmValue *falseValue = CreateLoadImmediate(module, node->source, CreateConstantInt(module->allocator, node->source, 0));

			CreateJump(module, node->source, exitBlock);

			module->currentFunction->AddBlock(exitBlock);
			module->currentBlock = exitBlock;

			VmValue *phi = CreatePhi(module, node->source, getType<VmInstruction>(trueValue), getType<VmInstruction>(falseValue));

			return CheckType(ctx, expression, phi);
		}

		if(node->op == SYN_BINARY_OP_LOGICAL_OR)
		{
			VmBlock *checkRhsBlock = CreateBlock(module, node->source, "lor_check_rhs");
			VmBlock *storeOneBlock = CreateBlock(module, node->source, "lor_store_1");
			VmBlock *storeZeroBlock = CreateBlock(module, node->source, "lor_store_0");
			VmBlock *exitBlock = CreateBlock(module, node->source, "lor_exit");

			CreateJumpNotZero(module, node->source, lhs, storeOneBlock, checkRhsBlock);

			module->currentFunction->AddBlock(checkRhsBlock);
			module->currentBlock = checkRhsBlock;

			VmValue *rhs = CompileVm(ctx, module, node->rhs);

			CreateJumpNotZero(module, node->source, rhs, storeOneBlock, storeZeroBlock);

			module->currentFunction->AddBlock(storeOneBlock);
			module->currentBlock = storeOneBlock;

			VmValue *trueValue = CreateLoadImmediate(module, node->source, CreateConstantInt(module->allocator, node->source, 1));

			CreateJump(module, node->source, exitBlock);

			module->currentFunction->AddBlock(storeZeroBlock);
			module->currentBlock = storeZeroBlock;

			VmValue *falseValue = CreateLoadImmediate(module, node->source, CreateConstantInt(module->allocator, node->source, 0));

			CreateJump(module, node->source, exitBlock);

			module->currentFunction->AddBlock(exitBlock);
			module->currentBlock = exitBlock;

			VmValue *phi = CreatePhi(module, node->source, getType<VmInstruction>(trueValue), getType<VmInstruction>(falseValue));

			return CheckType(ctx, expression, phi);
		}

		VmValue *rhs = CompileVm(ctx, module, node->rhs);

		VmValue *result = NULL;

		switch(node->op)
		{
		case SYN_BINARY_OP_ADD:
			result = CreateAdd(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_SUB:
			result = CreateSub(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_MUL:
			result = CreateMul(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_DIV:
			result = CreateDiv(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_MOD:
			result = CreateMod(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_POW:
			result = CreatePow(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_SHL:
			result = CreateShl(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_SHR:
			result = CreateShr(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_LESS:
			result = CreateCompareLess(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_LESS_EQUAL:
			result = CreateCompareLessEqual(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_GREATER:
			result = CreateCompareGreater(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_GREATER_EQUAL:
			result = CreateCompareGreaterEqual(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_EQUAL:
			result = CreateCompareEqual(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_NOT_EQUAL:
			result = CreateCompareNotEqual(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_BIT_AND:
			result = CreateAnd(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_BIT_OR:
			result = CreateOr(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_BIT_XOR:
			result = CreateXor(module, node->source, lhs, rhs);
			break;
		case SYN_BINARY_OP_LOGICAL_XOR:
			result = CreateLogicalXor(module, node->source, lhs, rhs);
			break;
		}

		assert(result);

		return CheckType(ctx, expression, result);
	}
	else if(ExprGetAddress *node = getType<ExprGetAddress>(expression))
	{
		return CheckType(ctx, expression, CreateVariableAddress(module, node->source, node->variable, ctx.GetReferenceType(node->variable->type)));
	}
	else if(ExprDereference *node = getType<ExprDereference>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		TypeRef *refType = getType<TypeRef>(node->value->type);

		assert(refType);
		assert(refType->subType == node->type);

		return CheckType(ctx, expression, CreateLoad(ctx, module, node->source, node->type, value));
	}
	else if(ExprUnboxing *node = getType<ExprUnboxing>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		return CheckType(ctx, expression, value);
	}
	else if(ExprConditional *node = getType<ExprConditional>(expression))
	{
		VmValue* condition = CompileVm(ctx, module, node->condition);

		VmBlock *trueBlock = CreateBlock(module, node->source, "if_true");
		VmBlock *falseBlock = CreateBlock(module, node->source, "if_false");
		VmBlock *exitBlock = CreateBlock(module, node->source, "if_exit");

		CreateJumpNotZero(module, node->source, condition, trueBlock, falseBlock);

		module->currentFunction->AddBlock(trueBlock);
		module->currentBlock = trueBlock;

		VmValue *trueValue = CompileVm(ctx, module, node->trueBlock);

		if(VmConstant *constant = getType<VmConstant>(trueValue))
			trueValue = CreateLoadImmediate(module, node->source, constant);

		CreateJump(module, node->source, exitBlock);

		module->currentFunction->AddBlock(falseBlock);
		module->currentBlock = falseBlock;

		VmValue *falseValue = CompileVm(ctx, module, node->falseBlock);

		if(VmConstant *constant = getType<VmConstant>(falseValue))
			falseValue = CreateLoadImmediate(module, node->source, constant);

		CreateJump(module, node->source, exitBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		VmValue *phi = CreatePhi(module, node->source, getType<VmInstruction>(trueValue), getType<VmInstruction>(falseValue));

		return CheckType(ctx, expression, phi);
	}
	else if(ExprAssignment *node = getType<ExprAssignment>(expression))
	{
		TypeRef *refType = getType<TypeRef>(node->lhs->type);

		assert(refType);
		assert(refType->subType == node->rhs->type);

		VmValue *address = CompileVm(ctx, module, node->lhs);

		VmValue *initializer = CompileVm(ctx, module, node->rhs);

		CreateStore(ctx, module, node->source, node->rhs->type, address, initializer);

		return CheckType(ctx, expression, initializer);
	}
	else if(ExprMemberAccess *node = getType<ExprMemberAccess>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		assert(isType<TypeRef>(node->value->type));

		VmValue *offset = CreateConstantInt(module->allocator, node->source, node->member->offset);

		return CheckType(ctx, expression, CreateMemberAccess(module, node->source, value, offset, ctx.GetReferenceType(node->member->type), node->member->name));
	}
	else if(ExprArrayIndex *node = getType<ExprArrayIndex>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);
		VmValue *index = CompileVm(ctx, module, node->index);

		if(TypeUnsizedArray *arrayType = getType<TypeUnsizedArray>(node->value->type))
		{
			VmValue *elementSize = CreateConstantInt(module->allocator, node->source, unsigned(arrayType->subType->size));

			return CheckType(ctx, expression, CreateIndexUnsized(module, node->source, elementSize, value, index, ctx.GetReferenceType(arrayType->subType)));
		}

		TypeRef *refType = getType<TypeRef>(node->value->type);

		assert(refType);

		TypeArray *arrayType = getType<TypeArray>(refType->subType);

		assert(arrayType);
		assert(unsigned(arrayType->subType->size) == arrayType->subType->size);

		VmValue *arrayLength = CreateConstantInt(module->allocator, node->source, unsigned(arrayType->length));
		VmValue *elementSize = CreateConstantInt(module->allocator, node->source, unsigned(arrayType->subType->size));

		return CheckType(ctx, expression, CreateIndex(module, node->source, arrayLength, elementSize, value, index, ctx.GetReferenceType(arrayType->subType)));
	}
	else if(ExprReturn *node = getType<ExprReturn>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		if(node->coroutineStateUpdate)
			CompileVm(ctx, module, node->coroutineStateUpdate);

		for(ExprBase *expr = node->closures.head; expr; expr = expr->next)
			CompileVm(ctx, module, expr);

		if(node->value->type == ctx.typeVoid)
			return CheckType(ctx, expression, CreateReturn(module, node->source));

		return CheckType(ctx, expression, CreateReturn(module, node->source, value));
	}
	else if(ExprYield *node = getType<ExprYield>(expression))
	{
		VmValue *value = CompileVm(ctx, module, node->value);

		if(node->coroutineStateUpdate)
			CompileVm(ctx, module, node->coroutineStateUpdate);

		for(ExprBase *expr = node->closures.head; expr; expr = expr->next)
			CompileVm(ctx, module, expr);

		VmBlock *block = module->currentFunction->restoreBlocks[++module->currentFunction->nextRestoreBlock];

		VmValue *result = node->value->type == ctx.typeVoid ? CreateYield(module, node->source) : CreateYield(module, node->source, value);

		module->currentFunction->AddBlock(block);
		module->currentBlock = block;

		return CheckType(ctx, expression, result);
	}
	else if(ExprVariableDefinition *node = getType<ExprVariableDefinition>(expression))
	{
		if(node->initializer)
			CompileVm(ctx, module, node->initializer);

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprArraySetup *node = getType<ExprArraySetup>(expression))
	{
		TypeRef *refType = getType<TypeRef>(node->lhs->type);

		assert(refType);

		TypeArray *arrayType = getType<TypeArray>(refType->subType);

		assert(arrayType);

		VmValue *initializer = CompileVm(ctx, module, node->initializer);

		VmValue *address = CompileVm(ctx, module, node->lhs);

		// TODO: use cmdSetRange for supported types

		VmValue *offsetPtr = CreateAlloca(ctx, module, node->source, ctx.typeInt, "arr_it");

		VmBlock *conditionBlock = CreateBlock(module, node->source, "arr_setup_cond");
		VmBlock *bodyBlock = CreateBlock(module, node->source, "arr_setup_body");
		VmBlock *exitBlock = CreateBlock(module, node->source, "arr_setup_exit");

		CreateJump(module, node->source, conditionBlock);

		module->currentFunction->AddBlock(conditionBlock);
		module->currentBlock = conditionBlock;

		// Offset will move in element size steps, so it will reach the full size of the array
		assert(int(arrayType->length * arrayType->subType->size) == arrayType->length * arrayType->subType->size);

		// While offset is less than array size
		VmValue* condition = CreateCompareLess(module, node->source, CreateLoad(ctx, module, node->source, ctx.typeInt, offsetPtr), CreateConstantInt(module->allocator, node->source, int(arrayType->length * arrayType->subType->size)));

		CreateJumpNotZero(module, node->source, condition, bodyBlock, exitBlock);

		module->currentFunction->AddBlock(bodyBlock);
		module->currentBlock = bodyBlock;

		VmValue *offset = CreateLoad(ctx, module, node->source, ctx.typeInt, offsetPtr);

		CreateStore(ctx, module, node->source, arrayType->subType, CreateMemberAccess(module, node->source, address, offset, ctx.GetReferenceType(arrayType->subType), InplaceStr()), initializer);
		CreateStore(ctx, module, node->source, ctx.typeInt, offsetPtr, CreateAdd(module, node->source, offset, CreateConstantInt(module->allocator, node->source, int(arrayType->subType->size))));

		CreateJump(module, node->source, conditionBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprVariableDefinitions *node = getType<ExprVariableDefinitions>(expression))
	{
		for(ExprVariableDefinition *value = node->definitions.head; value; value = getType<ExprVariableDefinition>(value->next))
			CompileVm(ctx, module, value);

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprVariableAccess *node = getType<ExprVariableAccess>(expression))
	{
		VmValue *address = CreateVariableAddress(module, node->source, node->variable, ctx.GetReferenceType(node->variable->type));

		VmValue *value = CreateLoad(ctx, module, node->source, node->variable->type, address);

		value->comment = node->variable->name;

		return CheckType(ctx, expression, value);
	}
	else if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(expression))
	{
		VmFunction *function = node->function->vmFunction;

		if(module->skipFunctionDefinitions)
			return CheckType(ctx, expression, CreateConstruct(module, node->source, VmType::FunctionRef(node->function->type), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), function, NULL, NULL));

		if(node->function->isPrototype)
			return CreateVoid(module);

		module->skipFunctionDefinitions = true;

		// Store state
		VmFunction *currentFunction = module->currentFunction;
		VmBlock *currentBlock = module->currentBlock;

		// Switch to new function
		module->currentFunction = function;

		VmBlock *block = CreateBlock(module, node->source, "start");

		module->currentFunction->AddBlock(block);
		module->currentBlock = block;
		block->AddUse(function);

		if(node->function->coroutine)
		{
			VmValue *state = CompileVm(ctx, module, node->coroutineStateRead);

			VmInstruction *inst = CreateInstruction(module, node->source, VmType::Void, VM_INST_UNYIELD, state, NULL, NULL, NULL);

			{
				VmBlock *block = CreateBlock(module, node->source, "co_start");

				inst->AddArgument(block);

				module->currentFunction->AddBlock(block);
				module->currentBlock = block;

				function->restoreBlocks.push_back(block);
			}

			for(unsigned i = 0; i < node->function->yieldCount; i++)
			{
				VmBlock *block = CreateBlock(module, node->source, "restore");

				inst->AddArgument(block);

				function->restoreBlocks.push_back(block);
			}
		}

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			CompileVm(ctx, module, value);

		// Restore state
		module->currentFunction = currentFunction;
		module->currentBlock = currentBlock;

		module->skipFunctionDefinitions = false;

		return CreateVoid(module);
	}
	else if(ExprGenericFunctionPrototype *node = getType<ExprGenericFunctionPrototype>(expression))
	{
		for(ExprBase *expr = node->contextVariables.head; expr; expr = expr->next)
			CompileVm(ctx, module, expr);

		return CreateVoid(module);
	}
	else if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(expression))
	{
		assert(node->function->vmFunction);

		VmValue *context = node->context ? CompileVm(ctx, module, node->context) : CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false);

		VmValue *funcRef = CreateConstruct(module, node->source, VmType::FunctionRef(node->function->type), context, node->function->vmFunction, NULL, NULL);

		return CheckType(ctx, expression, funcRef);
	}
	else if(ExprFunctionCall *node = getType<ExprFunctionCall>(expression))
	{
		VmValue *function = CompileVm(ctx, module, node->function);

		assert(module->currentBlock);

		VmInstruction *inst = allocate(VmInstruction)(module->allocator, GetVmType(ctx, node->type), node->source, VM_INST_CALL, module->currentFunction->nextInstructionId++);

		unsigned argCount = 1;

		for(ExprBase *value = node->arguments.head; value; value = value->next)
			argCount++;

		inst->arguments.reserve(argCount);

		inst->AddArgument(function);

		for(ExprBase *value = node->arguments.head; value; value = value->next)
		{
			VmValue *argument = CompileVm(ctx, module, value);

			assert(argument->type != VmType::Void);

			if(value->type == ctx.typeFloat)
				argument = CreateInstruction(module, node->source, VmType::Int, VM_INST_DOUBLE_TO_FLOAT, argument);

			inst->AddArgument(argument);
		}

		inst->hasSideEffects = HasSideEffects(inst->cmd);
		inst->hasMemoryAccess = HasMemoryAccess(inst->cmd);

		module->currentBlock->AddInstruction(inst);

		return CheckType(ctx, expression, inst);
	}
	else if(ExprAliasDefinition *node = getType<ExprAliasDefinition>(expression))
	{
		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprClassPrototype *node = getType<ExprClassPrototype>(expression))
	{
		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprGenericClassPrototype *node = getType<ExprGenericClassPrototype>(expression))
	{
		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprClassDefinition *node = getType<ExprClassDefinition>(expression))
	{
		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprEnumDefinition *node = getType<ExprEnumDefinition>(expression))
	{
		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprIfElse *node = getType<ExprIfElse>(expression))
	{
		VmValue* condition = CompileVm(ctx, module, node->condition);

		VmBlock *trueBlock = CreateBlock(module, node->source, "if_true");
		VmBlock *falseBlock = CreateBlock(module, node->source, "if_false");
		VmBlock *exitBlock = CreateBlock(module, node->source, "if_exit");

		if(node->falseBlock)
			CreateJumpNotZero(module, node->source, condition, trueBlock, falseBlock);
		else
			CreateJumpNotZero(module, node->source, condition, trueBlock, exitBlock);

		module->currentFunction->AddBlock(trueBlock);
		module->currentBlock = trueBlock;

		CompileVm(ctx, module, node->trueBlock);

		CreateJump(module, node->source, exitBlock);

		if(node->falseBlock)
		{
			module->currentFunction->AddBlock(falseBlock);
			module->currentBlock = falseBlock;

			CompileVm(ctx, module, node->falseBlock);

			CreateJump(module, node->source, exitBlock);
		}

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprFor *node = getType<ExprFor>(expression))
	{
		CompileVm(ctx, module, node->initializer);

		VmBlock *conditionBlock = CreateBlock(module, node->source, "for_cond");
		VmBlock *bodyBlock = CreateBlock(module, node->source, "for_body");
		VmBlock *iterationBlock = CreateBlock(module, node->source, "for_iter");
		VmBlock *exitBlock = CreateBlock(module, node->source, "for_exit");

		module->loopInfo.push_back(VmModule::LoopInfo(exitBlock, iterationBlock));

		CreateJump(module, node->source, conditionBlock);

		module->currentFunction->AddBlock(conditionBlock);
		module->currentBlock = conditionBlock;

		VmValue* condition = CompileVm(ctx, module, node->condition);

		CreateJumpNotZero(module, node->source, condition, bodyBlock, exitBlock);

		module->currentFunction->AddBlock(bodyBlock);
		module->currentBlock = bodyBlock;

		CompileVm(ctx, module, node->body);

		CreateJump(module, node->source, iterationBlock);

		module->currentFunction->AddBlock(iterationBlock);
		module->currentBlock = iterationBlock;

		CompileVm(ctx, module, node->increment);

		CreateJump(module, node->source, conditionBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		module->loopInfo.pop_back();

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprWhile *node = getType<ExprWhile>(expression))
	{
		VmBlock *conditionBlock = CreateBlock(module, node->source, "while_cond");
		VmBlock *bodyBlock = CreateBlock(module, node->source, "while_body");
		VmBlock *exitBlock = CreateBlock(module, node->source, "while_exit");

		module->loopInfo.push_back(VmModule::LoopInfo(exitBlock, conditionBlock));

		CreateJump(module, node->source, conditionBlock);

		module->currentFunction->AddBlock(conditionBlock);
		module->currentBlock = conditionBlock;

		VmValue* condition = CompileVm(ctx, module, node->condition);

		CreateJumpNotZero(module, node->source, condition, bodyBlock, exitBlock);

		module->currentFunction->AddBlock(bodyBlock);
		module->currentBlock = bodyBlock;

		CompileVm(ctx, module, node->body);

		CreateJump(module, node->source, conditionBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		module->loopInfo.pop_back();

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprDoWhile *node = getType<ExprDoWhile>(expression))
	{
		VmBlock *bodyBlock = CreateBlock(module, node->source, "do_body");
		VmBlock *condBlock = CreateBlock(module, node->source, "do_cond");
		VmBlock *exitBlock = CreateBlock(module, node->source, "do_exit");

		CreateJump(module, node->source, bodyBlock);

		module->currentFunction->AddBlock(bodyBlock);
		module->currentBlock = bodyBlock;

		module->loopInfo.push_back(VmModule::LoopInfo(exitBlock, condBlock));

		CompileVm(ctx, module, node->body);

		CreateJump(module, node->source, condBlock);

		module->currentFunction->AddBlock(condBlock);
		module->currentBlock = condBlock;

		VmValue* condition = CompileVm(ctx, module, node->condition);

		CreateJumpNotZero(module, node->source, condition, bodyBlock, exitBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		module->loopInfo.pop_back();

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprSwitch *node = getType<ExprSwitch>(expression))
	{
		CompileVm(ctx, module, node->condition);

		SmallArray<VmBlock*, 64> conditionBlocks(module->allocator);
		SmallArray<VmBlock*, 64> caseBlocks(module->allocator);

		// Generate blocks for all cases
		for(ExprBase *curr = node->cases.head; curr; curr = curr->next)
			conditionBlocks.push_back(CreateBlock(module, node->source, "switch_case"));

		// Generate blocks for all cases
		for(ExprBase *curr = node->blocks.head; curr; curr = curr->next)
			caseBlocks.push_back(CreateBlock(module, node->source, "case_block"));

		VmBlock *defaultBlock = CreateBlock(module, node->source, "default_block");
		VmBlock *exitBlock = CreateBlock(module, node->source, "switch_exit");

		CreateJump(module, node->source, conditionBlocks.empty() ? defaultBlock : conditionBlocks[0]);

		unsigned i;

		// Generate code for all conditions
		i = 0;
		for(ExprBase *curr = node->cases.head; curr; curr = curr->next, i++)
		{
			module->currentFunction->AddBlock(conditionBlocks[i]);
			module->currentBlock = conditionBlocks[i];

			VmValue *condition = CompileVm(ctx, module, curr);

			CreateJumpNotZero(module, node->source, condition, caseBlocks[i], curr->next ? conditionBlocks[i + 1] : defaultBlock);
		}

		module->loopInfo.push_back(VmModule::LoopInfo(exitBlock, NULL));

		// Generate code for all cases
		i = 0;
		for(ExprBase *curr = node->blocks.head; curr; curr = curr->next, i++)
		{
			module->currentFunction->AddBlock(caseBlocks[i]);
			module->currentBlock = caseBlocks[i];

			CompileVm(ctx, module, curr);

			CreateJump(module, node->source, curr->next ? caseBlocks[i + 1] : defaultBlock);
		}

		// Create default block
		module->currentFunction->AddBlock(defaultBlock);
		module->currentBlock = defaultBlock;

		if(node->defaultBlock)
			CompileVm(ctx, module, node->defaultBlock);
		CreateJump(module, node->source, exitBlock);

		module->currentFunction->AddBlock(exitBlock);
		module->currentBlock = exitBlock;

		module->loopInfo.pop_back();

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprBreak *node = getType<ExprBreak>(expression))
	{
		for(ExprBase *expr = node->closures.head; expr; expr = expr->next)
			CompileVm(ctx, module, expr);

		VmBlock *target = module->loopInfo[module->loopInfo.size() - node->depth].breakBlock;

		CreateJump(module, node->source, target);

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprContinue *node = getType<ExprContinue>(expression))
	{
		for(ExprBase *expr = node->closures.head; expr; expr = expr->next)
			CompileVm(ctx, module, expr);

		VmBlock *target = module->loopInfo[module->loopInfo.size() - node->depth].continueBlock;

		CreateJump(module, node->source, target);

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprBlock *node = getType<ExprBlock>(expression))
	{
		for(ExprBase *value = node->expressions.head; value; value = value->next)
			CompileVm(ctx, module, value);

		for(ExprBase *expr = node->closures.head; expr; expr = expr->next)
			CompileVm(ctx, module, expr);

		return CheckType(ctx, expression, CreateVoid(module));
	}
	else if(ExprSequence *node = getType<ExprSequence>(expression))
	{
		VmValue *result = CreateVoid(module);

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			result = CompileVm(ctx, module, value);

		return CheckType(ctx, expression, result);
	}
	else if(!expression)
	{
		return NULL;
	}
	else
	{
		assert(!"unknown type");
	}

	return NULL;
}

VmModule* CompileVm(ExpressionContext &ctx, ExprBase *expression)
{
	if(ExprModule *node = getType<ExprModule>(expression))
	{
		VmModule *module = new (ctx.get<VmModule>()) VmModule(ctx.allocator, ctx.code);

		// Generate global function
		VmFunction *global = allocate(VmFunction)(module->allocator, VmType::Void, node->source, NULL, node->moduleScope, VmType::Void);

		// Generate type indexes
		for(unsigned i = 0; i < ctx.types.size(); i++)
			ctx.types[i]->typeIndex = i;

		// Generate function indexes
		for(unsigned i = 0; i < ctx.functions.size(); i++)
			ctx.functions[i]->functionIndex = i;

		// Generate VmFunction object for each function
		for(unsigned i = 0; i < ctx.functions.size(); i++)
		{
			FunctionData *function = ctx.functions[i];

			if(ctx.IsGenericFunction(function))
				continue;

			if(function->vmFunction)
				continue;

			VmFunction *vmFunction = allocate(VmFunction)(module->allocator, GetVmType(ctx, ctx.typeFunctionID), function->source, function, function->functionScope, GetVmType(ctx, function->type->returnType));

			function->vmFunction = vmFunction;

			if(FunctionData *implementation = function->implementation)
				implementation->vmFunction = vmFunction;

			module->functions.push_back(vmFunction);
		}

		for(unsigned i = 0; i < node->definitions.size(); i++)
			CompileVm(ctx, module, node->definitions[i]);

		module->skipFunctionDefinitions = true;

		// Setup global function
		module->currentFunction = global;

		VmBlock *block = CreateBlock(module, node->source, "start");

		global->AddBlock(block);
		module->currentBlock = block;
		block->AddUse(global);

		for(ExprBase *value = node->setup.head; value; value = value->next)
			CompileVm(ctx, module, value);

		for(ExprBase *value = node->expressions.head; value; value = value->next)
			CompileVm(ctx, module, value);

		module->functions.push_back(global);

		module->currentFunction = NULL;
		module->currentBlock = NULL;

		return module;
	}

	return NULL;
}

void RunPeepholeOptimizations(ExpressionContext &ctx, VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunPeepholeOptimizations(ctx, module, curr);
			curr = next;
		}
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		VmInstruction *curr = block->firstInstruction;

		while(curr)
		{
			VmInstruction *next = curr->nextSibling;
			RunPeepholeOptimizations(ctx, module, curr);
			curr = next;
		}
	}
	else if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		switch(inst->cmd)
		{
		case VM_INST_ADD:
			if(IsConstantZero(inst->arguments[0])) // 0 + x, all types
				ReplaceValueUsersWith(module, inst, inst->arguments[1], &module->peepholeOptimizations);
			else if(IsConstantZero(inst->arguments[1])) // x + 0, all types
				ReplaceValueUsersWith(module, inst, inst->arguments[0], &module->peepholeOptimizations);
			break;
		case VM_INST_SUB:
			if(DoesConstantIntegerMatch(inst->arguments[0], 0)) // 0 - x, integer types
				ChangeInstructionTo(module, inst, VM_INST_NEG, inst->arguments[1], NULL, NULL, NULL, &module->peepholeOptimizations);
			else if(IsConstantZero(inst->arguments[1])) // x - 0, all types
				ReplaceValueUsersWith(module, inst, inst->arguments[0], &module->peepholeOptimizations);
			break;
		case VM_INST_MUL:
			if(IsConstantZero(inst->arguments[0]) || IsConstantZero(inst->arguments[1])) // 0 * x or x * 0, all types
			{
				if(inst->type == VmType::Int)
					ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, 0), &module->peepholeOptimizations);
				else if(inst->type == VmType::Double)
					ReplaceValueUsersWith(module, inst, CreateConstantDouble(module->allocator, inst->source, 0), &module->peepholeOptimizations);
				else if(inst->type == VmType::Long)
					ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, 0), &module->peepholeOptimizations);
			}
			else if(IsConstantOne(inst->arguments[0])) // 1 * x, all types
			{
				ReplaceValueUsersWith(module, inst, inst->arguments[1], &module->peepholeOptimizations);
			}
			else if(IsConstantOne(inst->arguments[1])) // x * 1, all types
			{
				ReplaceValueUsersWith(module, inst, inst->arguments[0], &module->peepholeOptimizations);
			}
			break;
		case VM_INST_INDEX_UNSIZED:
			// Try to replace unsized array index with an array index if the type[] is a construct expression
			if(VmInstruction *objectConstruct = getType<VmInstruction>(inst->arguments[1]))
			{
				if(objectConstruct->cmd == VM_INST_ARRAY && isType<VmConstant>(objectConstruct->arguments[1]))
					ChangeInstructionTo(module, inst, VM_INST_INDEX, objectConstruct->arguments[1], inst->arguments[0], objectConstruct->arguments[0], inst->arguments[2], &module->peepholeOptimizations);
			}
			break;
		case VM_INST_CONVERT_POINTER:
			// Try to replace with a pointer value if auto ref is a construct expression
			if(VmInstruction *objectConstruct = getType<VmInstruction>(inst->arguments[0]))
			{
				if(objectConstruct->cmd == VM_INST_CONSTRUCT)
				{
					VmInstruction *typeidConstruct = getType<VmInstruction>(objectConstruct->arguments[0]);

					VmInstruction *typeidConvert = getType<VmInstruction>(inst->arguments[1]);

					if(typeidConstruct && typeidConstruct->cmd == VM_INST_TYPE_ID && typeidConvert && typeidConvert->cmd == VM_INST_TYPE_ID)
					{
						VmConstant *typeIndexConstruct = getType<VmConstant>(typeidConstruct->arguments[0]);
						VmConstant *typeIndexConvert = getType<VmConstant>(typeidConvert->arguments[0]);

						if(typeIndexConstruct && typeIndexConvert && typeIndexConstruct->iValue == typeIndexConvert->iValue)
							ReplaceValueUsersWith(module, inst, objectConstruct->arguments[1], &module->peepholeOptimizations);
					}
				}
			}
			break;
		case VM_INST_LESS:
			if((inst->arguments[0]->type == VmType::Int || inst->arguments[0]->type == VmType::Long) && inst->arguments[0] == inst->arguments[1])
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, 0), &module->peepholeOptimizations);
			break;
		case VM_INST_GREATER:
			if((inst->arguments[0]->type == VmType::Int || inst->arguments[0]->type == VmType::Long) && inst->arguments[0] == inst->arguments[1])
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, 0), &module->peepholeOptimizations);
			break;
		case VM_INST_LESS_EQUAL:
			if((inst->arguments[0]->type == VmType::Int || inst->arguments[0]->type == VmType::Long) && inst->arguments[0] == inst->arguments[1])
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, 1), &module->peepholeOptimizations);
			break;
		case VM_INST_GREATER_EQUAL:
			if((inst->arguments[0]->type == VmType::Int || inst->arguments[0]->type == VmType::Long) && inst->arguments[0] == inst->arguments[1])
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, 1), &module->peepholeOptimizations);
			break;
		case VM_INST_EQUAL:
			if((inst->arguments[0]->type == VmType::Int || inst->arguments[0]->type == VmType::Long) && inst->arguments[0] == inst->arguments[1])
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, 1), &module->peepholeOptimizations);
			break;
		case VM_INST_NOT_EQUAL:
			if((inst->arguments[0]->type == VmType::Int || inst->arguments[0]->type == VmType::Long) && inst->arguments[0] == inst->arguments[1])
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, 0), &module->peepholeOptimizations);
			break;
		}
	}
}

void RunConstantPropagation(ExpressionContext &ctx, VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunConstantPropagation(ctx, module, curr);
			curr = next;
		}
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		VmInstruction *curr = block->firstInstruction;

		while(curr)
		{
			VmInstruction *next = curr->nextSibling;
			RunConstantPropagation(ctx, module, curr);
			curr = next;
		}
	}
	else if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		if(inst->type != VmType::Int && inst->type != VmType::Double && inst->type != VmType::Long && inst->type.type != VM_TYPE_POINTER)
			return;

		SmallArray<VmConstant*, 32> consts(module->allocator);

		for(unsigned i = 0; i < inst->arguments.size(); i++)
		{
			VmConstant *constant = getType<VmConstant>(inst->arguments[i]);

			if(!constant)
				return;

			consts.push_back(constant);
		}

		switch(inst->cmd)
		{
		case VM_INST_LOAD_IMMEDIATE:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantDouble(module->allocator, inst->source, consts[0]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue), &module->constantPropagations);
			break;
		case VM_INST_DOUBLE_TO_INT:
			ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, int(consts[0]->dValue)), &module->constantPropagations);
			break;
		case VM_INST_DOUBLE_TO_LONG:
			ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, (long long)(consts[0]->dValue)), &module->constantPropagations);
			break;
		case VM_INST_INT_TO_DOUBLE:
			ReplaceValueUsersWith(module, inst, CreateConstantDouble(module->allocator, inst->source, double(consts[0]->iValue)), &module->constantPropagations);
			break;
		case VM_INST_LONG_TO_DOUBLE:
			ReplaceValueUsersWith(module, inst, CreateConstantDouble(module->allocator, inst->source, double(consts[0]->lValue)), &module->constantPropagations);
			break;
		case VM_INST_INT_TO_LONG:
			ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, (long long)(consts[0]->iValue)), &module->constantPropagations);
			break;
		case VM_INST_LONG_TO_INT:
			ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, int(consts[0]->lValue)), &module->constantPropagations);
			break;
		case VM_INST_ADD:
			if(inst->type.type == VM_TYPE_POINTER)
			{
				// Both arguments can't be based on an offset
				assert(!(consts[0]->container && consts[1]->container));

				ReplaceValueUsersWith(module, inst, CreateConstantPointer(module->allocator, inst->source, consts[0]->iValue + consts[1]->iValue, consts[0]->container ? consts[0]->container : consts[1]->container, inst->type.structType, true), &module->constantPropagations);
			}
			else
			{
				if(inst->type == VmType::Int)
					ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue + consts[1]->iValue), &module->constantPropagations);
				else if(inst->type == VmType::Double)
					ReplaceValueUsersWith(module, inst, CreateConstantDouble(module->allocator, inst->source, consts[0]->dValue + consts[1]->dValue), &module->constantPropagations);
				else if(inst->type == VmType::Long)
					ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue + consts[1]->lValue), &module->constantPropagations);
			}
			break;
		case VM_INST_SUB:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue - consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantDouble(module->allocator, inst->source, consts[0]->dValue - consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue - consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_MUL:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue * consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantDouble(module->allocator, inst->source, consts[0]->dValue * consts[1]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue * consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_DIV:
			if(!IsConstantZero(consts[1]))
			{
				if(inst->type == VmType::Int)
					ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue / consts[1]->iValue), &module->constantPropagations);
				else if(inst->type == VmType::Double)
					ReplaceValueUsersWith(module, inst, CreateConstantDouble(module->allocator, inst->source, consts[0]->dValue / consts[1]->dValue), &module->constantPropagations);
				else if(inst->type == VmType::Long)
					ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue / consts[1]->lValue), &module->constantPropagations);
			}
			break;
		case VM_INST_MOD:
			if(!IsConstantZero(consts[1]))
			{
				if(inst->type == VmType::Int)
					ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue % consts[1]->iValue), &module->constantPropagations);
				else if(inst->type == VmType::Long)
					ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue % consts[1]->lValue), &module->constantPropagations);
			}
			break;
		case VM_INST_LESS:
			if(consts[0]->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue < consts[1]->iValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->dValue < consts[1]->dValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->lValue < consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_GREATER:
			if(consts[0]->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue > consts[1]->iValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->dValue > consts[1]->dValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->lValue > consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_LESS_EQUAL:
			if(consts[0]->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue <= consts[1]->iValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->dValue <= consts[1]->dValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->lValue <= consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_GREATER_EQUAL:
			if(consts[0]->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue >= consts[1]->iValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->dValue >= consts[1]->dValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->lValue >= consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_EQUAL:
			if(consts[0]->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue == consts[1]->iValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->dValue == consts[1]->dValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->lValue == consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_NOT_EQUAL:
			if(consts[0]->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue != consts[1]->iValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->dValue != consts[1]->dValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->lValue != consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_SHL:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue << consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue << consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_SHR:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue >> consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue >> consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_BIT_AND:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue & consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue & consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_BIT_OR:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue | consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue | consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_BIT_XOR:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, consts[0]->iValue ^ consts[1]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, consts[0]->lValue ^ consts[1]->lValue), &module->constantPropagations);
			break;
		case VM_INST_LOG_XOR:
			if(consts[0]->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, (consts[0]->iValue != 0) != (consts[1]->iValue != 0)), &module->constantPropagations);
			else if(consts[0]->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, (consts[0]->lValue != 0) != (consts[1]->lValue != 0)), &module->constantPropagations);
			break;
		case VM_INST_NEG:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, -consts[0]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Double)
				ReplaceValueUsersWith(module, inst, CreateConstantDouble(module->allocator, inst->source, -consts[0]->dValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, -consts[0]->lValue), &module->constantPropagations);
			break;
		case VM_INST_BIT_NOT:
			if(inst->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, ~consts[0]->iValue), &module->constantPropagations);
			else if(inst->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, ~consts[0]->lValue), &module->constantPropagations);
			break;
		case VM_INST_LOG_NOT:
			if(consts[0]->type == VmType::Int)
				ReplaceValueUsersWith(module, inst, CreateConstantInt(module->allocator, inst->source, !consts[0]->iValue), &module->constantPropagations);
			else if(consts[0]->type == VmType::Long)
				ReplaceValueUsersWith(module, inst, CreateConstantLong(module->allocator, inst->source, !consts[0]->lValue), &module->constantPropagations);
			break;
		case VM_INST_INDEX:
			{
				unsigned arrayLength = consts[0]->iValue;
				unsigned elementSize = consts[1]->iValue;

				unsigned ptr = consts[2]->iValue;
				unsigned index = consts[3]->iValue;

				if(index < arrayLength)
					ReplaceValueUsersWith(module, inst, CreateConstantPointer(module->allocator, inst->source, ptr + elementSize * index, consts[2]->container, inst->type.structType, true), &module->constantPropagations);
			}
			break;
		}
	}
}

void RunDeadCodeElimiation(ExpressionContext &ctx, VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunDeadCodeElimiation(ctx, module, curr);
			curr = next;
		}
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		if(block->users.empty())
		{
			module->deadCodeEliminations++;

			block->parent->RemoveBlock(block);
		}
		else
		{
			VmInstruction *curr = block->firstInstruction;

			while(curr)
			{
				VmInstruction *next = curr->nextSibling;
				RunDeadCodeElimiation(ctx, module, curr);
				curr = next;
			}
		}
	}
	else if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		if(inst->users.empty() && !inst->hasSideEffects && inst->canBeRemoved)
		{
			module->deadCodeEliminations++;

			inst->parent->RemoveInstruction(inst);
		}
		else if(inst->cmd == VM_INST_JUMP_Z || inst->cmd == VM_INST_JUMP_NZ)
		{
			if(VmConstant *condition = getType<VmConstant>(inst->arguments[0]))
			{
				if(inst->cmd == VM_INST_JUMP_Z)
					ChangeInstructionTo(module, inst, VM_INST_JUMP, condition->iValue == 0 ? inst->arguments[1] : inst->arguments[2], NULL, NULL, NULL, &module->deadCodeEliminations);
				else
					ChangeInstructionTo(module, inst, VM_INST_JUMP, condition->iValue == 0 ? inst->arguments[2] : inst->arguments[1], NULL, NULL, NULL, &module->deadCodeEliminations);
			}
		}
	}
}

void RunControlFlowOptimization(ExpressionContext &ctx, VmModule *module, VmValue *value)
{
	(void)ctx;

	if(VmFunction *function = getType<VmFunction>(value))
	{
		for(VmBlock *curr = function->firstBlock; curr; curr = curr->nextSibling)
		{
			// Remove any instructions after a branch
			for(VmInstruction *inst = curr->firstInstruction; inst; inst = inst->nextSibling)
			{
				if(inst->cmd == VM_INST_JUMP || inst->cmd == VM_INST_JUMP_Z || inst->cmd == VM_INST_JUMP_NZ || inst->cmd == VM_INST_RETURN)
				{
					while(curr->lastInstruction != inst)
					{
						module->controlFlowSimplifications++;

						curr->RemoveInstruction(curr->lastInstruction);
					}
					break;
				}
			}

			// Merge together blocks if a block A ends with a branch to block B and block B only incoming blocks is block A
			VmInstruction *currLastInst = curr->lastInstruction;

			if(currLastInst && currLastInst->cmd == VM_INST_JUMP)
			{
				VmBlock *next = curr->nextSibling;

				if(next && currLastInst->arguments[0] == next && next->users.size() == 1 && next->users[0] == currLastInst)
				{
					// Steal target block instructions
					for(VmInstruction *inst = next->firstInstruction; inst; inst = inst->nextSibling)
						inst->parent = curr;

					curr->lastInstruction->nextSibling = next->firstInstruction;

					if(next->firstInstruction)
						next->firstInstruction->prevSibling = curr->lastInstruction;

					curr->lastInstruction = next->lastInstruction;

					next->firstInstruction = next->lastInstruction = NULL;

					// Remove branch
					curr->RemoveInstruction(currLastInst);

					module->controlFlowSimplifications++;
				}
			}

			// Reverse conditional branch so that the false block jump will jump further than the true block jump
			if(currLastInst && (currLastInst->cmd == VM_INST_JUMP_Z || currLastInst->cmd == VM_INST_JUMP_NZ))
			{
				VmBlock *next = curr->nextSibling;

				if(currLastInst->arguments[2] == next)
				{
					VmValue *trueBlock = currLastInst->arguments[1];
					VmValue *falseBlock = currLastInst->arguments[2];

					currLastInst->cmd = currLastInst->cmd == VM_INST_JUMP_Z ? VM_INST_JUMP_NZ : VM_INST_JUMP_Z;

					currLastInst->arguments[1] = falseBlock;
					currLastInst->arguments[2] = trueBlock;

					module->controlFlowSimplifications++;
				}
			}

			// If block contains a single conditional jump, and the condition is a phi node with constant arguments, try to redirect predecessor to our target
			if(currLastInst && (currLastInst->cmd == VM_INST_JUMP_Z || currLastInst->cmd == VM_INST_JUMP_NZ))
			{
				VmInstruction *phi = getType<VmInstruction>(currLastInst->arguments[0]);
				VmValue *trueBlock = currLastInst->arguments[1];
				VmValue *falseBlock = currLastInst->arguments[2];

				if(phi && phi->cmd == VM_INST_PHI && phi == currLastInst->prevSibling && phi == curr->firstInstruction)
				{
					for(unsigned i = 0; i < phi->arguments.size(); i += 2)
					{
						VmInstruction *value = getType<VmInstruction>(phi->arguments[i]);
						VmBlock *edge = getType<VmBlock>(phi->arguments[i + 1]);

						if(value->cmd != VM_INST_LOAD_IMMEDIATE)
							continue;

						VmConstant *condition = getType<VmConstant>(value->arguments[0]);

						VmValue *target = condition->iValue != 0 ? (currLastInst->cmd == VM_INST_JUMP_NZ ? trueBlock : falseBlock) : (currLastInst->cmd == VM_INST_JUMP_NZ ? falseBlock : trueBlock);

						VmInstruction *terminator = edge->lastInstruction;

						if(terminator->cmd == VM_INST_JUMP)
						{
							assert(terminator->arguments[0] == curr);

							ReplaceValue(terminator, terminator->arguments[0], target);

							module->controlFlowSimplifications++;
						}
					}
				}
			}

			// If block contains a return, and the return value is a phi node with constant arguments, try to return directly from predecessors
			if(currLastInst && currLastInst->cmd == VM_INST_RETURN && !currLastInst->arguments.empty())
			{
				VmInstruction *phi = getType<VmInstruction>(currLastInst->arguments[0]);

				if(phi && phi->cmd == VM_INST_PHI && phi == currLastInst->prevSibling && phi == curr->firstInstruction)
				{
					for(unsigned i = 0; i < phi->arguments.size(); i += 2)
					{
						VmInstruction *value = getType<VmInstruction>(phi->arguments[i]);
						VmBlock *edge = getType<VmBlock>(phi->arguments[i + 1]);

						VmInstruction *terminator = edge->lastInstruction;

						if(terminator->cmd == VM_INST_JUMP)
						{
							assert(terminator->arguments[0] == curr);

							ChangeInstructionTo(module, terminator, VM_INST_RETURN, value, 0, 0, 0, &module->controlFlowSimplifications);
						}
					}
				}
			}

			// Reverse conditional jump with unconditional if both targets are the same
			if(currLastInst && (currLastInst->cmd == VM_INST_JUMP_Z || currLastInst->cmd == VM_INST_JUMP_NZ))
			{
				VmValue *trueBlock = currLastInst->arguments[1];
				VmValue *falseBlock = currLastInst->arguments[2];

				if(trueBlock == falseBlock)
					ChangeInstructionTo(module, currLastInst, VM_INST_JUMP, trueBlock, 0, 0, 0, &module->controlFlowSimplifications);
			}

			// Remove coroutine unyield that only contains a single target
			if(currLastInst && currLastInst->cmd == VM_INST_UNYIELD && currLastInst->arguments.size() == 2)
			{
				VmValue *targetBlock = currLastInst->arguments[1];

				ChangeInstructionTo(module, currLastInst, VM_INST_JUMP, targetBlock, 0, 0, 0, &module->controlFlowSimplifications);
			}
		}

		for(VmBlock *curr = function->firstBlock; curr;)
		{
			VmBlock *next = curr->nextSibling;

			if(curr->firstInstruction && curr->firstInstruction == curr->lastInstruction && curr->firstInstruction->cmd == VM_INST_JUMP)
			{
				// Remove blocks that only contain an unconditional branch to some other block
				VmBlock *target = getType<VmBlock>(curr->firstInstruction->arguments[0]);

				assert(target);

				ReplaceValueUsersWith(module, curr, target, &module->controlFlowSimplifications);
			}

			if(curr->users.empty())
			{
				// Remove unused blocks
				function->RemoveBlock(curr);

				module->controlFlowSimplifications++;
			}

			curr = next;
		}
	}
}

void RunLoadStorePropagation(ExpressionContext &ctx, VmModule *module, VmValue *value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		module->currentFunction = function;

		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunLoadStorePropagation(ctx, module, curr);
			curr = next;
		}

		module->currentFunction = NULL;
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		// Handle loads and stores to constant global or frame addresses
		ClearLoadStoreInfo(module);

		for(VmInstruction *curr = block->firstInstruction; curr;)
		{
			VmInstruction *next = curr->nextSibling;

			switch(curr->cmd)
			{
			case VM_INST_LOAD_BYTE:
			case VM_INST_LOAD_SHORT:
			case VM_INST_LOAD_INT:
			case VM_INST_LOAD_FLOAT:
			case VM_INST_LOAD_DOUBLE:
			case VM_INST_LOAD_LONG:
			case VM_INST_LOAD_STRUCT:
				if(VmValue* value = GetLoadStoreInfo(module, curr))
				{
					if(curr->type != value->type)
					{
						assert(curr->type.size == value->type.size);

						module->currentBlock = block;

						block->insertPoint = curr->prevSibling;

						value = CreateInstruction(module, curr->source, curr->type, VM_INST_BITCAST, value, NULL, NULL, NULL);

						block->insertPoint = block->lastInstruction;

						module->currentBlock = NULL;

						ReplaceValueUsersWith(module, curr, value, &module->loadStorePropagations);
					}
					else
					{
						ReplaceValueUsersWith(module, curr, value, &module->loadStorePropagations);
					}
				}
				else
				{
					AddLoadInfo(module, curr);
				}
				break;
			case VM_INST_STORE_BYTE:
			case VM_INST_STORE_SHORT:
			case VM_INST_STORE_INT:
			case VM_INST_STORE_FLOAT:
			case VM_INST_STORE_DOUBLE:
			case VM_INST_STORE_LONG:
			case VM_INST_STORE_STRUCT:
				AddStoreInfo(module, curr);
				break;
			case VM_INST_SET_RANGE:
			case VM_INST_YIELD:
			case VM_INST_CLOSE_UPVALUES:
				ClearLoadStoreInfoAliasing(module);
				break;
			case VM_INST_CALL:
				ClearLoadStoreInfoAliasing(module);
				ClearLoadStoreInfoGlobal(module);
				break;
			}

			curr = next;
		}

		// Handle consecutive stores to the same address
		for(VmInstruction *curr = block->firstInstruction; curr; curr = curr->nextSibling)
		{
			if(curr->cmd >= VM_INST_STORE_BYTE && curr->cmd <= VM_INST_STORE_STRUCT)
			{
				// Walk up until a memory write is reached
				VmInstruction *prev = curr->prevSibling;

				while(prev && !HasMemoryAccess(prev->cmd))
					prev = prev->prevSibling;

				if(prev && prev->cmd == curr->cmd)
				{
					bool same = false;

					VmConstant *prevArgAsConst = getType<VmConstant>(prev->arguments[0]);
					VmConstant *currArgAsConst = getType<VmConstant>(curr->arguments[0]);

					if(currArgAsConst && prevArgAsConst)
						same = *currArgAsConst == *prevArgAsConst;
					else
						same = prev->arguments[0] == curr->arguments[0];

					if(same)
					{
						block->RemoveInstruction(prev);

						module->loadStorePropagations++;
					}
				}
			}
		}

		// Handle immediate loads from the same address as a store
		for(VmInstruction *curr = block->firstInstruction; curr;)
		{
			VmInstruction *next = curr->nextSibling;

			if(curr->cmd >= VM_INST_LOAD_BYTE && curr->cmd <= VM_INST_LOAD_STRUCT)
			{
				// Walk up until a memory write is reached
				VmInstruction *prev = curr->prevSibling;

				while(prev && !HasMemoryAccess(prev->cmd))
					prev = prev->prevSibling;

				if(prev && (prev->cmd >= VM_INST_STORE_BYTE && prev->cmd <= VM_INST_STORE_STRUCT) && GetAccessSize(prev) == GetAccessSize(curr) && prev->arguments[0] == curr->arguments[0] && curr->type.size == prev->arguments[1]->type.size)
				{
					VmValue* value = prev->arguments[1];

					if(curr->type != value->type)
					{
						assert(curr->type.size == value->type.size);

						module->currentBlock = block;

						block->insertPoint = curr->prevSibling;

						value = CreateInstruction(module, curr->source, curr->type, VM_INST_BITCAST, value, NULL, NULL, NULL);

						block->insertPoint = block->lastInstruction;

						module->currentBlock = NULL;

						ReplaceValueUsersWith(module, curr, value, &module->loadStorePropagations);
					}
					else
					{
						ReplaceValueUsersWith(module, curr, value, &module->loadStorePropagations);
					}
				}
			}

			curr = next;
		}
	}
}

void RunCommonSubexpressionElimination(ExpressionContext &ctx, VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		VmBlock *curr = function->firstBlock;

		while(curr)
		{
			VmBlock *next = curr->nextSibling;
			RunCommonSubexpressionElimination(ctx, module, curr);
			curr = next;
		}
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		VmInstruction *start = block->firstInstruction;

		for(VmInstruction *curr = block->firstInstruction; curr;)
		{
			VmInstruction *next = curr->nextSibling;

			if(curr->hasSideEffects || curr->hasMemoryAccess)
			{
				curr = next;
				continue;
			}

			VmInstruction *prev = start;

			while(prev != curr)
			{
				if(prev->cmd == curr->cmd && prev->arguments.size() == curr->arguments.size())
				{
					bool same = true;

					for(unsigned i = 0; i < curr->arguments.size(); i++)
					{
						VmValue *currArg = curr->arguments[i];
						VmValue *prevArg = prev->arguments[i];

						VmConstant *currArgAsConst = getType<VmConstant>(currArg);
						VmConstant *prevArgAsConst = getType<VmConstant>(prevArg);

						if(currArgAsConst && prevArgAsConst)
						{
							if(!(*currArgAsConst == *prevArgAsConst))
								same = false;
						}
						else if(currArg != prevArg)
						{
							same = false;
						}
					}

					if(same)
					{
						ReplaceValueUsersWith(module, curr, prev, &module->commonSubexprEliminations);
						break;
					}
				}

				prev = prev->nextSibling;
			}

			curr = next;
		}
	}
}

void RunCreateAllocaStorage(ExpressionContext &ctx, VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		module->currentFunction = function;

		for(unsigned i = 0; i < function->allocas.size(); i++)
		{
			VariableData *variable = function->allocas[i];

			if(variable->users.empty())
				continue;

			FinalizeAlloca(ctx, module, variable);
		}

		module->currentFunction = NULL;
	}
}

void LegalizeVmRegisterUsage(ExpressionContext &ctx, VmModule *module, VmBlock *block)
{
	module->currentBlock = block;

	// Replace non-trivial instructions that have multiple uses with stack variables
	for(VmInstruction *curr = block->firstInstruction; curr; curr = curr->nextSibling)
	{
		if(curr->users.size() <= 1)
			continue;

		if(curr->type == VmType::Block)
			continue;

		if(IsLoad(curr))
			continue;

		if(curr->cmd == VM_INST_CONSTRUCT && (curr->type.type == VM_TYPE_FUNCTION_REF || curr->type.type == VM_TYPE_ARRAY_REF))
			continue;

		TypeBase *type = GetBaseType(ctx, curr->type);

		VmValue *address = CreateAlloca(ctx, module, curr->source, type, "reg");

		block->insertPoint = curr;

		curr->canBeRemoved = false;

		ReplaceValueUsersWith(module, curr, CreateLoad(ctx, module, curr->source, type, address), NULL);

		curr->canBeRemoved = true;

		block->insertPoint = curr;

		CreateStore(ctx, module, curr->source, type, address, curr);

		block->insertPoint = block->lastInstruction;
	}

	module->currentBlock = NULL;
}

void LegalizeVmPhiStorage(ExpressionContext &ctx, VmModule *module, VmBlock *block)
{
	// Alias phi argument registers to the same storage
	for(VmInstruction *curr = block->firstInstruction; curr; curr = curr->nextSibling)
	{
		if(curr->cmd != VM_INST_PHI)
			continue;

		// Can't have any instructions before phi
		assert(curr->prevSibling == NULL || curr->prevSibling->cmd == VM_INST_PHI);

		TypeBase *type = GetBaseType(ctx, curr->type);

		VmValue *address = CreateAlloca(ctx, module, curr->source, type, "reg");

		for(unsigned i = 0; i < curr->arguments.size(); i += 2)
		{
			VmInstruction *value = getType<VmInstruction>(curr->arguments[i]);
			VmBlock *edge = getType<VmBlock>(curr->arguments[i + 1]);

			module->currentBlock = edge;

			edge->insertPoint = value;

			CreateStore(ctx, module, value->source, GetBaseType(ctx, value->type), address, value);

			edge->insertPoint = edge->lastInstruction;

			module->currentBlock = NULL;
		}

		module->currentBlock = block;

		block->insertPoint = curr;

		ReplaceValueUsersWith(module, curr, CreateLoad(ctx, module, curr->source, type, address), NULL);

		block->insertPoint = block->lastInstruction;

		module->currentBlock = NULL;
	}
}

void RunLegalizeVm(ExpressionContext &ctx, VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		module->currentFunction = function;

		for(VmBlock *curr = function->firstBlock; curr; curr = curr->nextSibling)
			RunLegalizeVm(ctx, module, curr);

		module->currentFunction = NULL;
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		LegalizeVmRegisterUsage(ctx, module, block);

		LegalizeVmPhiStorage(ctx, module, block);

		// Check that constructs that require a temporary
		// TODO
	}
}

void RunVmPass(ExpressionContext &ctx, VmModule *module, VmPassType type)
{
	for(VmFunction *value = module->functions.head; value; value = value->next)
	{
		switch(type)
		{
		case VM_PASS_OPT_PEEPHOLE:
			RunPeepholeOptimizations(ctx, module, value);
			break;
		case VM_PASS_OPT_CONSTANT_PROPAGATION:
			RunConstantPropagation(ctx, module, value);
			break;
		case VM_PASS_OPT_DEAD_CODE_ELIMINATION:
			RunDeadCodeElimiation(ctx, module, value);
			break;
		case VM_PASS_OPT_CONTROL_FLOW_SIPLIFICATION:
			RunControlFlowOptimization(ctx, module, value);
			break;
		case VM_PASS_OPT_LOAD_STORE_PROPAGATION:
			RunLoadStorePropagation(ctx, module, value);
			break;
		case VM_PASS_OPT_COMMON_SUBEXPRESSION_ELIMINATION:
			RunCommonSubexpressionElimination(ctx, module, value);
			break;
		case VM_PASS_CREATE_ALLOCA_STORAGE:
			RunCreateAllocaStorage(ctx, module, value);
			break;
		case VM_PASS_LEGALIZE_VM:
			RunLegalizeVm(ctx, module, value);
			break;
		}

		// Preserve entry block order for execution
		value->MoveEntryBlockToStart();
	}
}
