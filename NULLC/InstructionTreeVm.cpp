#include "InstructionTreeVm.h"

#include "ExpressionTree.h"
#include "InstructionTreeVmCommon.h"

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
		assert(exprType.structType == value->type.structType);

		return value;
	}

	VmValue* CreateVoid(VmModule *module)
	{
		return new (module->get<VmVoid>()) VmVoid(module->allocator);
	}

	VmBlock* CreateBlock(VmModule *module, SynBase *source, const char *name)
	{
		return new (module->get<VmBlock>()) VmBlock(module->allocator, source, InplaceStr(name), module->currentFunction->nextBlockId++);
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
		case VM_INST_ABORT_NO_RETURN:
			return true;
		default:
			break;
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
		case VM_INST_UNYIELD:
		case VM_INST_CONVERT_POINTER:
		case VM_INST_ABORT_NO_RETURN:
			return true;
		default:
			break;
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
			return true;
		default:
			break;
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
			return true;
		default:
			break;
		}

		return false;
	}

	bool IsLoad(VmInstructionType cmd)
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
			return true;
		default:
			break;
		}

		return false;
	}

	bool IsStore(VmInstructionType cmd)
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
			return true;
		default:
			break;
		}

		return false;
	}

	VmInstruction* CreateInstruction(VmModule *module, SynBase *source, VmType type, VmInstructionType cmd, VmValue *first, VmValue *second, VmValue *third, VmValue *fourth)
	{
		assert(module->currentBlock);

		VmInstruction *inst = new (module->get<VmInstruction>()) VmInstruction(module->allocator, type, source, cmd, module->currentFunction->nextInstructionId++);

		if(first)
			inst->AddArgument(first);

		if(second)
		{
			assert(first);
			inst->AddArgument(second);
		}

		if(third)
		{
			assert(second);
			inst->AddArgument(third);
		}

		if(fourth)
		{
			assert(third);
			inst->AddArgument(fourth);
		}

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

	VmInstructionType GetLoadInstruction(ExpressionContext &ctx, TypeBase *type)
	{
		if(type == ctx.typeBool || type == ctx.typeChar)
			return VM_INST_LOAD_BYTE;

		if(type == ctx.typeShort)
			return VM_INST_LOAD_SHORT;

		if(type == ctx.typeInt)
			return VM_INST_LOAD_INT;

		if(type == ctx.typeFloat)
			return VM_INST_LOAD_FLOAT;

		if(type == ctx.typeDouble)
			return VM_INST_LOAD_DOUBLE;

		if(type == ctx.typeLong)
			return VM_INST_LOAD_LONG;

		if(isType<TypeRef>(type))
			return VM_INST_LOAD_POINTER;

		if(isType<TypeFunction>(type))
			return VM_INST_LOAD_STRUCT;

		if(isType<TypeUnsizedArray>(type))
			return VM_INST_LOAD_STRUCT;

		if(type == ctx.typeAutoRef)
			return VM_INST_LOAD_STRUCT;

		if(type == ctx.typeAutoArray)
			return VM_INST_LOAD_STRUCT;

		if(isType<TypeTypeID>(type) || isType<TypeFunctionID>(type) || isType<TypeEnum>(type))
			return VM_INST_LOAD_INT;

		if(isType<TypeNullptr>(type))
			return VM_INST_LOAD_POINTER;

		assert(type->size != 0);
		assert(type->size % 4 == 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);

		return VM_INST_LOAD_STRUCT;
	}

	VmInstructionType GetStoreInstruction(ExpressionContext &ctx, TypeBase *type)
	{
		if(type == ctx.typeBool || type == ctx.typeChar)
			return VM_INST_STORE_BYTE;

		if(type == ctx.typeShort)
			return VM_INST_STORE_SHORT;

		if(type == ctx.typeInt)
			return VM_INST_STORE_INT;

		if(type == ctx.typeFloat)
			return VM_INST_STORE_FLOAT;

		if(type == ctx.typeDouble)
			return VM_INST_STORE_DOUBLE;

		if(type == ctx.typeLong)
			return VM_INST_STORE_LONG;

		if(isType<TypeRef>(type))
			return VM_INST_STORE_POINTER;

		if(isType<TypeEnum>(type))
			return VM_INST_STORE_INT;

		if(isType<TypeFunction>(type) || isType<TypeUnsizedArray>(type) || type == ctx.typeAutoRef || type == ctx.typeAutoArray)
			return VM_INST_STORE_STRUCT;

		if(isType<TypeTypeID>(type) || isType<TypeFunctionID>(type) || isType<TypeEnum>(type))
			return VM_INST_STORE_INT;

		if(isType<TypeNullptr>(type))
			return VM_INST_STORE_POINTER;

		assert(type->size != 0);
		assert(type->size % 4 == 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);

		return VM_INST_STORE_STRUCT;
	}

	VmType GetLoadResultType(ExpressionContext &ctx, TypeBase *type)
	{
		if(type == ctx.typeBool || type == ctx.typeChar || type == ctx.typeShort || type == ctx.typeInt)
			return VmType::Int;

		if(type == ctx.typeFloat || type == ctx.typeDouble)
			return VmType::Double;

		if(type == ctx.typeLong)
			return VmType::Long;

		if(isType<TypeRef>(type))
			return VmType::Pointer(type);

		if(isType<TypeFunction>(type))
			return VmType::FunctionRef(type);

		if(isType<TypeUnsizedArray>(type))
			return VmType::ArrayRef(type);

		if(type == ctx.typeAutoRef)
			return VmType::AutoRef;

		if(type == ctx.typeAutoArray)
			return VmType::AutoArray;

		if(isType<TypeTypeID>(type) || isType<TypeFunctionID>(type) || isType<TypeEnum>(type))
			return VmType::Int;

		if(isType<TypeNullptr>(type))
			return VmType::Pointer(type);

		assert(type->size != 0);
		assert(type->size % 4 == 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);

		return VmType::Struct(type->size, type);
	}

	VmValue* CreateLoad(ExpressionContext &ctx, VmModule *module, SynBase *source, TypeBase *type, VmValue *address, unsigned offset)
	{
		if(type->size == 0)
			return CreateConstantStruct(ctx.allocator, source, NULL, 0, type);

		return CreateInstruction(module, source, GetLoadResultType(ctx, type), GetLoadInstruction(ctx, type), address, CreateConstantInt(ctx.allocator, source, offset));
	}

	VmValue* CreateStore(ExpressionContext &ctx, VmModule *module, SynBase *source, TypeBase *type, VmValue *address, VmValue *value)
	{
		assert(value->type == GetVmType(ctx, type));

		if(type->size == 0)
			return CreateVoid(module);

		return CreateInstruction(module, source, VmType::Void, GetStoreInstruction(ctx, type), address, CreateConstantInt(ctx.allocator, source, 0), value);
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

	VmValue* CreateAbortNoReturn(VmModule *module, SynBase *source)
	{
		return CreateInstruction(module, source, VmType::Void, VM_INST_ABORT_NO_RETURN);
	}

	VmValue* CreateVariableAddress(VmModule *module, SynBase *source, VariableData *variable, TypeBase *structType)
	{
		(void)source;

		assert(!IsMemberScope(variable->scope));

		VmValue *value = CreateConstantPointer(module->allocator, NULL, 0, variable, structType, true);

		return value;
	}

	VmValue* CreateTypeIndex(VmModule *module, SynBase *source, TypeBase *type)
	{
		return CreateInstruction(module, source, VmType::Int, VM_INST_TYPE_ID, CreateConstantInt(module->allocator, source, type->typeIndex));
	}

	VmValue* CreateFunctionAddress(VmModule *module, SynBase *source, FunctionData *function)
	{
		assert(function->vmFunction);

		return CreateInstruction(module, source, VmType::Int, VM_INST_FUNCTION_ADDRESS, CreateConstantFunction(module->allocator, source, function->vmFunction));
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

	VmValue* CreateBitcast(VmModule *module, SynBase *source, VmType type, VmValue *value)
	{
		return CreateInstruction(module, source, type, VM_INST_BITCAST, value, NULL, NULL, NULL);
	}

	VmConstant* CreateAlloca(ExpressionContext &ctx, VmModule *module, SynBase *source, TypeBase *type, const char *suffix)
	{
		ScopeData *scope = module->currentFunction->function ? module->currentFunction->function->functionScope : ctx.globalScope;

		char *name = (char*)ctx.allocator->alloc(16);
		sprintf(name, "$temp%d_%s", ctx.unnamedVariableCount++, suffix);

		VariableData *variable = new (module->get<VariableData>()) VariableData(ctx.allocator, NULL, scope, type->alignment, type, InplaceStr(name), 0, ctx.uniqueVariableId++);

		variable->isVmAlloca = true;
		variable->offset = ~0u;

		VmConstant *value = CreateConstantPointer(module->allocator, source, 0, variable, ctx.GetReferenceType(variable->type), true);

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
		{
			assert(first);
			inst->AddArgument(second);
		}

		if(third)
		{
			assert(second);
			inst->AddArgument(third);
		}

		if(fourth)
		{
			assert(third);
			inst->AddArgument(fourth);
		}

		for(unsigned i = 0; i < arguments.size(); i++)
			arguments[i]->RemoveUse(inst);

		inst->hasSideEffects = HasSideEffects(cmd);
		inst->hasMemoryAccess = HasMemoryAccess(cmd);

		if(optCount)
			(*optCount)++;
	}

	void ReplaceValue(VmModule *module, VmValue *value, VmValue *original, VmValue *replacement)
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

				ReplaceValue(module, curr, original, replacement);
			}

			// Move replacement block to first position
			VmBlock *block = getType<VmBlock>(replacement);

			if(block != function->firstBlock)
			{
				function->DetachBlock(block);

				block->parent = function;
				block->nextSibling = function->firstBlock;

				function->firstBlock->prevSibling = block;
				function->firstBlock = block;
			}
		}
		else if(VmBlock *block = getType<VmBlock>(value))
		{
			for(VmInstruction *curr = block->firstInstruction; curr; curr = curr->nextSibling)
			{
				assert(curr != original); // Block doesn't use instructions

				ReplaceValue(module, curr, original, replacement);
			}
		}
		else if(VmInstruction *inst = getType<VmInstruction>(value))
		{
			// Can't replace phi instruction argument with a constant
			if(inst->cmd == VM_INST_PHI && isType<VmConstant>(replacement))
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

			// Legalize constant offset in load/store instruction
			switch(inst->cmd)
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
				if(VmConstant *address = getType<VmConstant>(inst->arguments[0]))
				{
					VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

					if(address->container && offset->iValue != 0)
					{
						VmConstant *target = CreateConstantPointer(module->allocator, NULL, address->iValue + offset->iValue, address->container, address->type.structType, true);

						ChangeInstructionTo(module, inst, inst->cmd, target, CreateConstantInt(module->allocator, NULL, 0), inst->arguments.size() == 3 ? inst->arguments[2] : NULL, NULL, NULL);
					}
				}
				break;
			default:
				break;
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
			ReplaceValue(module, users[i], original, replacement);

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

		VmValue *loadAddress = inst->arguments[0];
		VmConstant *loadOffset = getType<VmConstant>(inst->arguments[1]);

		if(VmConstant *address = getType<VmConstant>(loadAddress))
		{
			assert(loadOffset->iValue == 0);

			info.address = address;
		}
		else
		{
			info.pointer = loadAddress;
			info.offset = loadOffset;
		}

		module->loadStoreInfo.push_back(info);
	}

	void AddStoreInfo(VmModule *module, VmInstruction* inst)
	{
		VmValue *storeAddress = inst->arguments[0];
		VmConstant *storeOffset = getType<VmConstant>(inst->arguments[1]);

		if(VmConstant *address = getType<VmConstant>(storeAddress))
		{
			assert(storeOffset->iValue == 0);

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
			if(VmInstruction *ptrArg = getType<VmInstruction>(storeAddress))
			{
				if(ptrArg->cmd == VM_INST_INDEX)
				{
					VmConstant *length = getType<VmConstant>(ptrArg->arguments[0]);
					VmConstant *elemSize = getType<VmConstant>(ptrArg->arguments[1]);

					assert(length && elemSize);

					if(VmConstant *base = getType<VmConstant>(ptrArg->arguments[2]))
					{
						unsigned indexOffset = unsigned(base->iValue);
						unsigned indexSize = length->iValue * elemSize->iValue;

						if(VmConstant *index = getType<VmConstant>(ptrArg->arguments[3]))
						{
							indexOffset += index->iValue * elemSize->iValue;
							indexSize = elemSize->iValue;
						}

						unsigned totalOffset = indexOffset + storeOffset->iValue;

						ClearLoadStoreInfo(module, base->container, totalOffset, indexSize - storeOffset->iValue);
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
		VmValue *loadAddress = inst->arguments[0];
		VmConstant *loadOffset = getType<VmConstant>(inst->arguments[1]);

		if(VmConstant *address = getType<VmConstant>(loadAddress))
		{
			assert(loadOffset->iValue == 0);

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
					VmValue *value = el.storeInst->arguments[2];

					// Can't reuse arguments of a different size
					if(value->type.size != inst->type.size)
						return NULL;

					return value;
				}

				if(el.storeInst && el.address->container == address->container)
				{
					if(VmValue *component = TryExtractConstructElement(el.storeInst->arguments[2], address->iValue, GetAccessSize(inst)))
						return component;
				}
			}
		}
		else if(VmValue *pointer = loadAddress)
		{
			for(unsigned i = 0; i < module->loadStoreInfo.size(); i++)
			{
				VmModule::LoadStoreInfo &el = module->loadStoreInfo[i];

				if(el.address)
					continue;

				if(el.loadInst && el.pointer == pointer && el.offset->iValue == loadOffset->iValue && GetAccessSize(inst) == GetAccessSize(el.loadInst))
					return el.loadInst;
			}
		}

		return NULL;
	}

	bool IsLoad(VmValue *value)
	{
		if(VmInstruction *inst = getType<VmInstruction>(value))
			return IsLoad(inst->cmd);

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

	// New user might not be simple
	hasKnownSimpleUse = false;

	users.push_back(user);
}

void VmValue::RemoveUse(VmValue* user)
{
	for(unsigned i = 0, e = users.count; i < e; i++)
	{
		if(users.data[i] == user)
		{
			users.data[i] = users.back();
			users.pop_back();

			// Might have removed last non-simple use
			hasKnownNonSimpleUse = false;
			break;
		}
	}

	if(users.count == 0 && !hasSideEffects && canBeRemoved)
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
			if(instruction->parent)
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

void VmBlock::DetachInstruction(VmInstruction* instruction)
{
	assert(instruction);
	assert(instruction->parent == this);

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
}

void VmBlock::RemoveInstruction(VmInstruction* instruction)
{
	assert(instruction->users.empty());

	DetachInstruction(instruction);

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

	if(isType<TypeRef>(type))
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

	if(isType<TypeNullptr>(type))
		return VmType::Pointer(type);

	if(isType<TypeArray>(type) || isType<TypeClass>(type))
	{
		assert(type->size % 4 == 0);
		assert(type->size < NULLC_MAX_TYPE_SIZE);

		return VmType::Struct(type->size, type);
	}

	if(isType<TypeEnum>(type))
		return VmType::Int;

	assert(!"unknown type");

	return VmType::Void;
}

void FinalizeAlloca(ExpressionContext &ctx, VmModule *module, VariableData *variable)
{
	unsigned offset = 0;
	ScopeData *scope = AllocateScopeSlot(ctx, module, variable->type, offset);

	variable->offset = offset;

	bool found = false;

	for(unsigned i = 0, e = scope->allVariables.size(); i < e; i++)
	{
		if(scope->allVariables.data[i] == variable)
		{
			found = true;
			break;
		}
	}

	if(!found)
	{
		assert(variable->isVmAlloca);

		scope->variables.push_back(variable);
		scope->allVariables.push_back(variable);
		ctx.variables.push_back(variable);
	}
}

VmValue* CompileVmVoid(ExpressionContext &ctx, VmModule *module, ExprVoid *node)
{
	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmBoolLiteral(ExpressionContext &ctx, VmModule *module, ExprBoolLiteral *node)
{
	return CheckType(ctx, node, CreateConstantInt(module->allocator, node->source, node->value ? 1 : 0));
}

VmValue* CompileVmCharacterLiteral(ExpressionContext &ctx, VmModule *module, ExprCharacterLiteral *node)
{
	return CheckType(ctx, node, CreateConstantInt(module->allocator, node->source, node->value));
}

VmValue* CompileVmStringLiteral(ExpressionContext &ctx, VmModule *module, ExprStringLiteral *node)
{
	unsigned size = node->length + 1;

	// Align to 4
	size = (size + 3) & ~3;

	char *value = (char*)ctx.allocator->alloc(size);
	memset(value, 0, size);

	for(unsigned i = 0; i < node->length; i++)
		value[i] = node->value[i];

	return CheckType(ctx, node, CreateConstantStruct(module->allocator, node->source, value, size, node->type));
}

VmValue* CompileVmIntegerLiteral(ExpressionContext &ctx, VmModule *module, ExprIntegerLiteral *node)
{
	if(node->type == ctx.typeShort)
		return CheckType(ctx, node, CreateConstantInt(module->allocator, node->source, short(node->value)));

	if(node->type == ctx.typeInt)
		return CheckType(ctx, node, CreateConstantInt(module->allocator, node->source, int(node->value)));

	if(node->type == ctx.typeLong)
		return CheckType(ctx, node, CreateConstantLong(module->allocator, node->source, node->value));

	if(isType<TypeEnum>(node->type))
		return CheckType(ctx, node, CreateConstantInt(module->allocator, node->source, int(node->value)));

	assert(!"unknown type");
	return NULL;
}

VmValue* CompileVmRationalLiteral(ExpressionContext &ctx, VmModule *module, ExprRationalLiteral *node)
{
	return CheckType(ctx, node, CreateConstantDouble(module->allocator, node->source, node->value));
}

VmValue* CompileVmTypeLiteral(ExpressionContext &ctx, VmModule *module, ExprTypeLiteral *node)
{
	assert(!isType<TypeFunctionSet>(node->value) && !isType<TypeArgumentSet>(node->value) && !isType<TypeMemberSet>(node->value));

	return CheckType(ctx, node, CreateTypeIndex(module, node->source, node->value));
}

VmValue* CompileVmNullptrLiteral(ExpressionContext &ctx, VmModule *module, ExprNullptrLiteral *node)
{
	return CheckType(ctx, node, CreateConstantPointer(module->allocator, node->source, 0, NULL, node->type, false));
}

VmValue* CompileVmFunctionIndexLiteral(ExpressionContext &ctx, VmModule *module, ExprFunctionIndexLiteral *node)
{
	return CheckType(ctx, node, CreateFunctionAddress(module, node->source, node->function));
}

VmValue* CompileVmPassthrough(ExpressionContext &ctx, VmModule *module, ExprPassthrough *node)
{
	VmValue *value = CompileVm(ctx, module, node->value);

	return CheckType(ctx, node, value);
}

VmValue* CompileVmArray(ExpressionContext &ctx, VmModule *module, ExprArray *node)
{
	assert(isType<TypeArray>(node->type));

	TypeArray *typeArray = getType<TypeArray>(node->type);

	TypeBase *elementType = typeArray->subType;

	if(elementType == ctx.typeBool || elementType == ctx.typeChar || elementType == ctx.typeShort)
	{
		VmValue *storage = CreateAlloca(ctx, module, node->source, typeArray, "arr_lit");

		unsigned i = 0;

		for(ExprBase *value = node->values.head; value; value = value->next)
		{
			VmValue *element = CompileVm(ctx, module, value);

			VmValue *arrayLength = CreateConstantInt(module->allocator, node->source, unsigned(typeArray->length));
			VmValue *elementSize = CreateConstantInt(module->allocator, node->source, unsigned(elementType->size));
			VmValue *index = CreateConstantInt(module->allocator, node->source, i);

			VmValue *address = CreateIndex(module, node->source, arrayLength, elementSize, storage, index, ctx.GetReferenceType(elementType));

			CreateStore(ctx, module, node->source, elementType, address, element);

			i++;
		}

		return CheckType(ctx, node, CreateLoad(ctx, module, node->source, typeArray, storage, 0));
	}

	VmInstruction *inst = new (module->get<VmInstruction>()) VmInstruction(module->allocator, GetVmType(ctx, node->type), node->source, VM_INST_ARRAY, module->currentFunction->nextInstructionId++);

	for(ExprBase *value = node->values.head; value; value = value->next)
	{
		VmValue *element = CompileVm(ctx, module, value);

		if(elementType == ctx.typeFloat)
			element = CreateInstruction(module, node->source, VmType::Int, VM_INST_DOUBLE_TO_FLOAT, element);

		inst->AddArgument(element);
	}

	module->currentBlock->AddInstruction(inst);

	return CheckType(ctx, node, inst);
}

VmValue* CompileVmPreModify(ExpressionContext &ctx, VmModule *module, ExprPreModify *node)
{
	VmValue *address = CompileVm(ctx, module, node->value);

	TypeRef *refType = getType<TypeRef>(node->value->type);

	assert(refType);

	VmValue *value = CreateLoad(ctx, module, node->source, refType->subType, address, 0);

	if(value->type == VmType::Int)
		value = CreateAdd(module, node->source, value, CreateConstantInt(module->allocator, node->source, node->isIncrement ? 1 : -1));
	else if(value->type == VmType::Double)
		value = CreateAdd(module, node->source, value, CreateConstantDouble(module->allocator, node->source, node->isIncrement ? 1.0 : -1.0));
	else if(value->type == VmType::Long)
		value = CreateAdd(module, node->source, value, CreateConstantLong(module->allocator, node->source, node->isIncrement ? 1ll : -1ll));
	else
		assert("!unknown type");

	CreateStore(ctx, module, node->source, refType->subType, address, value);

	return CheckType(ctx, node, value);
}

VmValue* CompileVmPostModify(ExpressionContext &ctx, VmModule *module, ExprPostModify *node)
{
	VmValue *address = CompileVm(ctx, module, node->value);

	TypeRef *refType = getType<TypeRef>(node->value->type);

	assert(refType);

	VmValue *value = CreateLoad(ctx, module, node->source, refType->subType, address, 0);
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

	return CheckType(ctx, node, result);
}

VmValue* CompileVmTypeCast(ExpressionContext &ctx, VmModule *module, ExprTypeCast *node)
{
	VmValue *value = CompileVm(ctx, module, node->value);

	switch(node->category)
	{
	case EXPR_CAST_NUMERICAL:
		if(node->type == ctx.typeBool)
		{
			if(value->type == VmType::Int)
				return CheckType(ctx, node, CreateCompareNotEqual(module, node->source, value, CreateConstantInt(module->allocator, node->source, 0)));

			if(value->type == VmType::Double)
				return CheckType(ctx, node, CreateCompareNotEqual(module, node->source, value, CreateConstantDouble(module->allocator, node->source, 0.0)));

			if(value->type == VmType::Long)
				return CheckType(ctx, node, CreateCompareNotEqual(module, node->source, value, CreateConstantLong(module->allocator, node->source, 0ll)));

			assert(!"unknown type");
		}

		return CheckType(ctx, node, CreateCast(module, node->source, value, GetVmType(ctx, node->type)));
	case EXPR_CAST_PTR_TO_BOOL:
		return CheckType(ctx, node, CreateCompareNotEqual(module, node->source, value, CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false)));
	case EXPR_CAST_UNSIZED_TO_BOOL:
		if(TypeUnsizedArray *unsizedArrType = getType<TypeUnsizedArray>(node->value->type))
		{
			VmValue *ptr = CreateExtract(module, node->source, VmType::Pointer(ctx.GetReferenceType(unsizedArrType->subType)), value, 0);

			return CheckType(ctx, node, CreateCompareNotEqual(module, node->source, ptr, CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false)));
		}
		
		break;
	case EXPR_CAST_FUNCTION_TO_BOOL:
		if(VmValue *index = CreateExtract(module, node->source, VmType::Int, value, sizeof(void*)))
		{
			return CheckType(ctx, node, CreateCompareNotEqual(module, node->source, index, CreateConstantInt(module->allocator, node->source, 0)));
		}

		break;
	case EXPR_CAST_NULL_TO_PTR:
		return CheckType(ctx, node, CreateConstantPointer(module->allocator, node->source, 0, NULL, node->type, false));
	case EXPR_CAST_NULL_TO_AUTO_PTR:
		return CheckType(ctx, node, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateConstantInt(module->allocator, node->source, 0), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), NULL, NULL));
	case EXPR_CAST_NULL_TO_UNSIZED:
		return CheckType(ctx, node, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), CreateConstantInt(module->allocator, node->source, 0), NULL, NULL));
	case EXPR_CAST_NULL_TO_AUTO_ARRAY:
		return CheckType(ctx, node, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateConstantInt(module->allocator, node->source, 0), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), CreateConstantInt(module->allocator, node->source, 0), NULL));
	case EXPR_CAST_NULL_TO_FUNCTION:
		return CheckType(ctx, node, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), CreateConstantInt(module->allocator, node->source, 0), NULL, NULL));
	case EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
		if(TypeRef *refType = getType<TypeRef>(node->value->type))
		{
			TypeArray *arrType = getType<TypeArray>(refType->subType);

			assert(arrType);
			assert(unsigned(arrType->length) == arrType->length);

			return CheckType(ctx, node, CreateConstruct(module, node->source, GetVmType(ctx, node->type), value, CreateConstantInt(module->allocator, node->source, unsigned(arrType->length)), NULL, NULL));
		}

		break;
	case EXPR_CAST_PTR_TO_AUTO_PTR:
		if(TypeRef *refType = getType<TypeRef>(node->value->type))
		{
			TypeClass *classType = getType<TypeClass>(refType->subType);

			VmValue *typeId = NULL;

			if(classType && (classType->extendable || classType->baseClass))
				typeId = CreateLoad(ctx, module, node->source, ctx.typeTypeID, value, 0);
			else
				typeId = CreateTypeIndex(module, node->source, refType->subType);

			return CheckType(ctx, node, CreateConstruct(module, node->source, GetVmType(ctx, node->type), typeId, value, NULL, NULL));
		}
		break;
	case EXPR_CAST_AUTO_PTR_TO_PTR:
		if(TypeRef *refType = getType<TypeRef>(node->type))
		{
			return CheckType(ctx, node, CreateConvertPtr(module, node->source, value, refType->subType, ctx.GetReferenceType(refType->subType)));
		}

		break;
	case EXPR_CAST_UNSIZED_TO_AUTO_ARRAY:
		if(TypeUnsizedArray *unsizedType = getType<TypeUnsizedArray>(node->value->type))
		{
			return CheckType(ctx, node, CreateConstruct(module, node->source, GetVmType(ctx, node->type), CreateTypeIndex(module, node->source, unsizedType->subType), value, NULL, NULL));
		}

		break;
	case EXPR_CAST_REINTERPRET:
		if(node->type == node->value->type)
			return CheckType(ctx, node, value);

		if(isType<TypeUnsizedArray>(node->type) && isType<TypeUnsizedArray>(node->value->type))
		{
			// Try changing the target call type
			if(VmInstruction *inst = getType<VmInstruction>(value))
			{
				if(inst->cmd == VM_INST_CALL)
				{
					inst->type = GetVmType(ctx, node->type);

					return CheckType(ctx, node, value);
				}
			}

			return CheckType(ctx, node, CreateBitcast(module, node->source, GetVmType(ctx, node->type), value));
		}
		else if(isType<TypeRef>(node->type) && isType<TypeRef>(node->value->type))
		{
			// Try changing the target call type
			if(VmInstruction *inst = getType<VmInstruction>(value))
			{
				if(inst->cmd == VM_INST_CALL)
				{
					inst->type = GetVmType(ctx, node->type);

					return CheckType(ctx, node, value);
				}
			}

			return CheckType(ctx, node, CreateBitcast(module, node->source, GetVmType(ctx, node->type), value));
		}
		else if(isType<TypeFunction>(node->type) && isType<TypeFunction>(node->value->type))
		{
			// Try changing the target call type
			if(VmInstruction *inst = getType<VmInstruction>(value))
			{
				if(inst->cmd == VM_INST_CALL)
				{
					inst->type = GetVmType(ctx, node->type);

					return CheckType(ctx, node, value);
				}
			}

			return CheckType(ctx, node, CreateBitcast(module, node->source, GetVmType(ctx, node->type), value));
		}

		return CheckType(ctx, node, value);
	default:
		assert(!"unknown cast");
	}

	return CheckType(ctx, node, value);
}

VmValue* CompileVmUnaryOp(ExpressionContext &ctx, VmModule *module, ExprUnaryOp *node)
{
	VmValue *value = CompileVm(ctx, module, node->value);

	VmValue *result = NULL;

	switch(node->op)
	{
	case SYN_UNARY_OP_UNKNOWN:
		break;
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

	return CheckType(ctx, node, result);
}

VmValue* CompileVmBinaryOp(ExpressionContext &ctx, VmModule *module, ExprBinaryOp *node)
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

		return CheckType(ctx, node, phi);
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

		return CheckType(ctx, node, phi);
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
	default:
		break;
	}

	assert(result);

	return CheckType(ctx, node, result);
}

VmValue* CompileVmGetAddress(ExpressionContext &ctx, VmModule *module, ExprGetAddress *node)
{
	return CheckType(ctx, node, CreateVariableAddress(module, node->source, node->variable, ctx.GetReferenceType(node->variable->type)));
}

VmValue* CompileVmDereference(ExpressionContext &ctx, VmModule *module, ExprDereference *node)
{
	VmValue *value = CompileVm(ctx, module, node->value);

	TypeRef *refType = getType<TypeRef>(node->value->type);

	(void)refType;
	assert(refType);
	assert(refType->subType == node->type);

	return CheckType(ctx, node, CreateLoad(ctx, module, node->source, node->type, value, 0));
}

VmValue* CompileVmUnboxing(ExpressionContext &ctx, VmModule *module, ExprUnboxing *node)
{
	VmValue *value = CompileVm(ctx, module, node->value);

	return CheckType(ctx, node, value);
}

VmValue* CompileVmConditional(ExpressionContext &ctx, VmModule *module, ExprConditional *node)
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

	return CheckType(ctx, node, phi);
}

VmValue* CompileVmAssignment(ExpressionContext &ctx, VmModule *module, ExprAssignment *node)
{
	TypeRef *refType = getType<TypeRef>(node->lhs->type);

	(void)refType;
	assert(refType);
	assert(refType->subType == node->rhs->type);

	VmValue *address = CompileVm(ctx, module, node->lhs);

	VmValue *initializer = CompileVm(ctx, module, node->rhs);

	CreateStore(ctx, module, node->source, node->rhs->type, address, initializer);

	return CheckType(ctx, node, initializer);
}

VmValue* CompileVmMemberAccess(ExpressionContext &ctx, VmModule *module, ExprMemberAccess *node)
{
	VmValue *value = CompileVm(ctx, module, node->value);

	assert(isType<TypeRef>(node->value->type));

	VmValue *offset = CreateConstantInt(module->allocator, node->source, node->member->offset);

	return CheckType(ctx, node, CreateMemberAccess(module, node->source, value, offset, ctx.GetReferenceType(node->member->type), node->member->name));
}

VmValue* CompileVmArrayIndex(ExpressionContext &ctx, VmModule *module, ExprArrayIndex *node)
{
	VmValue *value = CompileVm(ctx, module, node->value);
	VmValue *index = CompileVm(ctx, module, node->index);

	if(TypeUnsizedArray *arrayType = getType<TypeUnsizedArray>(node->value->type))
	{
		VmValue *elementSize = CreateConstantInt(module->allocator, node->source, unsigned(arrayType->subType->size));

		return CheckType(ctx, node, CreateIndexUnsized(module, node->source, elementSize, value, index, ctx.GetReferenceType(arrayType->subType)));
	}

	TypeRef *refType = getType<TypeRef>(node->value->type);

	assert(refType);

	TypeArray *arrayType = getType<TypeArray>(refType->subType);

	assert(arrayType);
	assert(unsigned(arrayType->subType->size) == arrayType->subType->size);

	VmValue *arrayLength = CreateConstantInt(module->allocator, node->source, unsigned(arrayType->length));
	VmValue *elementSize = CreateConstantInt(module->allocator, node->source, unsigned(arrayType->subType->size));

	return CheckType(ctx, node, CreateIndex(module, node->source, arrayLength, elementSize, value, index, ctx.GetReferenceType(arrayType->subType)));
}

VmValue* CompileVmReturn(ExpressionContext &ctx, VmModule *module, ExprReturn *node)
{
	VmValue *value = CompileVm(ctx, module, node->value);

	if(node->coroutineStateUpdate)
		CompileVm(ctx, module, node->coroutineStateUpdate);

	if(node->closures)
		CompileVm(ctx, module, node->closures);

	if(node->value->type == ctx.typeVoid)
		return CheckType(ctx, node, CreateReturn(module, node->source));

	return CheckType(ctx, node, CreateReturn(module, node->source, value));
}

VmValue* CompileVmYield(ExpressionContext &ctx, VmModule *module, ExprYield *node)
{
	VmValue *value = CompileVm(ctx, module, node->value);

	if(node->coroutineStateUpdate)
		CompileVm(ctx, module, node->coroutineStateUpdate);

	if(node->closures)
		CompileVm(ctx, module, node->closures);

	VmBlock *block = module->currentFunction->restoreBlocks[++module->currentFunction->nextRestoreBlock];

	VmValue *result = node->value->type == ctx.typeVoid ? CreateYield(module, node->source) : CreateYield(module, node->source, value);

	module->currentFunction->AddBlock(block);
	module->currentBlock = block;

	return CheckType(ctx, node, result);
}

VmValue* CompileVmVariableDefinition(ExpressionContext &ctx, VmModule *module, ExprVariableDefinition *node)
{
	if(node->initializer)
		CompileVm(ctx, module, node->initializer);

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmArraySetup(ExpressionContext &ctx, VmModule *module, ExprArraySetup *node)
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

	CreateStore(ctx, module, node->source, ctx.typeInt, offsetPtr, CreateConstantInt(module->allocator, node->source, 0));

	CreateJump(module, node->source, conditionBlock);

	module->currentFunction->AddBlock(conditionBlock);
	module->currentBlock = conditionBlock;

	// Offset will move in element size steps, so it will reach the full size of the array
	assert(int(arrayType->length * arrayType->subType->size) == arrayType->length * arrayType->subType->size);

	// While offset is less than array size
	VmValue* condition = CreateCompareLess(module, node->source, CreateLoad(ctx, module, node->source, ctx.typeInt, offsetPtr, 0), CreateConstantInt(module->allocator, node->source, int(arrayType->length * arrayType->subType->size)));

	CreateJumpNotZero(module, node->source, condition, bodyBlock, exitBlock);

	module->currentFunction->AddBlock(bodyBlock);
	module->currentBlock = bodyBlock;

	VmValue *offset = CreateLoad(ctx, module, node->source, ctx.typeInt, offsetPtr, 0);

	CreateStore(ctx, module, node->source, arrayType->subType, CreateMemberAccess(module, node->source, address, offset, ctx.GetReferenceType(arrayType->subType), InplaceStr()), initializer);
	CreateStore(ctx, module, node->source, ctx.typeInt, offsetPtr, CreateAdd(module, node->source, offset, CreateConstantInt(module->allocator, node->source, int(arrayType->subType->size))));

	CreateJump(module, node->source, conditionBlock);

	module->currentFunction->AddBlock(exitBlock);
	module->currentBlock = exitBlock;

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmVariableDefinitions(ExpressionContext &ctx, VmModule *module, ExprVariableDefinitions *node)
{
	for(ExprVariableDefinition *value = node->definitions.head; value; value = getType<ExprVariableDefinition>(value->next))
		CompileVm(ctx, module, value);

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmVariableAccess(ExpressionContext &ctx, VmModule *module, ExprVariableAccess *node)
{
	VmValue *address = CreateVariableAddress(module, node->source, node->variable, ctx.GetReferenceType(node->variable->type));

	VmValue *value = CreateLoad(ctx, module, node->source, node->variable->type, address, 0);

	value->comment = node->variable->name;

	return CheckType(ctx, node, value);
}

VmValue* CompileVmFunctionContextAccess(ExpressionContext &ctx, VmModule *module, ExprFunctionContextAccess *node)
{
	TypeRef *refType = getType<TypeRef>(node->function->contextType);

	assert(refType);

	TypeClass *classType = getType<TypeClass>(refType->subType);

	assert(classType);

	VmValue *value = NULL;

	if(classType->size == 0)
	{
		value = CreateConstantPointer(module->allocator, node->source, 0, NULL, node->type, false);
	}
	else
	{
		VmValue *address = CreateVariableAddress(module, node->source, node->function->contextVariable, ctx.GetReferenceType(node->function->contextVariable->type));

		value = CreateLoad(ctx, module, node->source, node->function->contextVariable->type, address, 0);

		value->comment = node->function->contextVariable->name;
	}

	return CheckType(ctx, node, value);
}

VmValue* CompileVmFunctionDefinition(ExpressionContext &ctx, VmModule *module, ExprFunctionDefinition *node)
{
	VmFunction *function = node->function->vmFunction;

	if(module->skipFunctionDefinitions)
		return CheckType(ctx, node, CreateConstruct(module, node->source, VmType::FunctionRef(node->function->type), CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false), function, NULL, NULL));

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

	CreateAbortNoReturn(module, node->source);

	// Restore state
	module->currentFunction = currentFunction;
	module->currentBlock = currentBlock;

	module->skipFunctionDefinitions = false;

	return CreateVoid(module);
}

VmValue* CompileVmGenericFunctionPrototype(ExpressionContext &ctx, VmModule *module, ExprGenericFunctionPrototype *node)
{
	for(ExprBase *expr = node->contextVariables.head; expr; expr = expr->next)
		CompileVm(ctx, module, expr);

	return CreateVoid(module);
}

VmValue* CompileVmFunctionAccess(ExpressionContext &ctx, VmModule *module, ExprFunctionAccess *node)
{
	assert(node->function->vmFunction);

	VmValue *context = node->context ? CompileVm(ctx, module, node->context) : CreateConstantPointer(module->allocator, node->source, 0, NULL, ctx.typeNullPtr, false);

	VmValue *funcRef = CreateConstruct(module, node->source, VmType::FunctionRef(node->function->type), context, node->function->vmFunction, NULL, NULL);

	return CheckType(ctx, node, funcRef);
}

VmValue* CompileVmFunctionCall(ExpressionContext &ctx, VmModule *module, ExprFunctionCall *node)
{
	VmValue *function = CompileVm(ctx, module, node->function);

	assert(module->currentBlock);

	VmInstruction *inst = new (module->get<VmInstruction>()) VmInstruction(module->allocator, GetVmType(ctx, node->type), node->source, VM_INST_CALL, module->currentFunction->nextInstructionId++);

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

	return CheckType(ctx, node, inst);
}

VmValue* CompileVmAliasDefinition(ExpressionContext &ctx, VmModule *module, ExprAliasDefinition *node)
{
	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmClassPrototype(ExpressionContext &ctx, VmModule *module, ExprClassPrototype *node)
{
	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmGenericClassPrototype(ExpressionContext &ctx, VmModule *module, ExprGenericClassPrototype *node)
{
	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmClassDefinition(ExpressionContext &ctx, VmModule *module, ExprClassDefinition *node)
{
	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmEnumDefinition(ExpressionContext &ctx, VmModule *module, ExprEnumDefinition *node)
{
	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmIfElse(ExpressionContext &ctx, VmModule *module, ExprIfElse *node)
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

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmFor(ExpressionContext &ctx, VmModule *module, ExprFor *node)
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

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmWhile(ExpressionContext &ctx, VmModule *module, ExprWhile *node)
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

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmDoWhile(ExpressionContext &ctx, VmModule *module, ExprDoWhile *node)
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

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmSwitch(ExpressionContext &ctx, VmModule *module, ExprSwitch *node)
{
	CompileVm(ctx, module, node->condition);

	SmallArray<VmBlock*, 16> conditionBlocks(module->allocator);
	SmallArray<VmBlock*, 16> caseBlocks(module->allocator);

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

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmBreak(ExpressionContext &ctx, VmModule *module, ExprBreak *node)
{
	if(node->closures)
		CompileVm(ctx, module, node->closures);

	VmBlock *target = module->loopInfo[module->loopInfo.size() - node->depth].breakBlock;

	CreateJump(module, node->source, target);

	return CheckType(ctx, node, CreateVoid(module));
}

VmBlock* GetLoopContinueBlock(VmModule *module, unsigned depth)
{
	unsigned pos = module->loopInfo.size();

	for(unsigned i = 0; i < depth; i++)
	{
		pos--;

		while(!module->loopInfo[pos].continueBlock)
			pos--;
	}

	return module->loopInfo[pos].continueBlock;
}

VmValue* CompileVmContinue(ExpressionContext &ctx, VmModule *module, ExprContinue *node)
{
	if(node->closures)
		CompileVm(ctx, module, node->closures);

	VmBlock *target = GetLoopContinueBlock(module, node->depth);

	CreateJump(module, node->source, target);

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmBlock(ExpressionContext &ctx, VmModule *module, ExprBlock *node)
{
	for(ExprBase *value = node->expressions.head; value; value = value->next)
		CompileVm(ctx, module, value);

	if(node->closures)
		CompileVm(ctx, module, node->closures);

	return CheckType(ctx, node, CreateVoid(module));
}

VmValue* CompileVmSequence(ExpressionContext &ctx, VmModule *module, ExprSequence *node)
{
	VmValue *result = CreateVoid(module);

	for(ExprBase *value = node->expressions.head; value; value = value->next)
		result = CompileVm(ctx, module, value);

	return CheckType(ctx, node, result);
}

VmValue* CompileVm(ExpressionContext &ctx, VmModule *module, ExprBase *expression)
{
	if(ExprVoid *node = getType<ExprVoid>(expression))
		return CompileVmVoid(ctx, module, node);

	if(ExprBoolLiteral *node = getType<ExprBoolLiteral>(expression))
		return CompileVmBoolLiteral(ctx, module, node);

	if(ExprCharacterLiteral *node = getType<ExprCharacterLiteral>(expression))
		return CompileVmCharacterLiteral(ctx, module, node);

	if(ExprStringLiteral *node = getType<ExprStringLiteral>(expression))
		return CompileVmStringLiteral(ctx, module, node);

	if(ExprIntegerLiteral *node = getType<ExprIntegerLiteral>(expression))
		return CompileVmIntegerLiteral(ctx, module, node);

	if(ExprRationalLiteral *node = getType<ExprRationalLiteral>(expression))
		return CompileVmRationalLiteral(ctx, module, node);

	if(ExprTypeLiteral *node = getType<ExprTypeLiteral>(expression))
		return CompileVmTypeLiteral(ctx, module, node);

	if(ExprNullptrLiteral *node = getType<ExprNullptrLiteral>(expression))
		return CompileVmNullptrLiteral(ctx, module, node);

	if(ExprFunctionIndexLiteral *node = getType<ExprFunctionIndexLiteral>(expression))
		return CompileVmFunctionIndexLiteral(ctx, module, node);

	if(ExprPassthrough *node = getType<ExprPassthrough>(expression))
		return CompileVmPassthrough(ctx, module, node);

	if(ExprArray *node = getType<ExprArray>(expression))
		return CompileVmArray(ctx, module, node);

	if(ExprPreModify *node = getType<ExprPreModify>(expression))
		return CompileVmPreModify(ctx, module, node);

	if(ExprPostModify *node = getType<ExprPostModify>(expression))
		return CompileVmPostModify(ctx, module, node);	

	if(ExprTypeCast *node = getType<ExprTypeCast>(expression))
		return CompileVmTypeCast(ctx, module, node);

	if(ExprUnaryOp *node = getType<ExprUnaryOp>(expression))
		return CompileVmUnaryOp(ctx, module, node);

	if(ExprBinaryOp *node = getType<ExprBinaryOp>(expression))
		return CompileVmBinaryOp(ctx, module, node);

	if(ExprGetAddress *node = getType<ExprGetAddress>(expression))
		return CompileVmGetAddress(ctx, module, node);

	if(ExprDereference *node = getType<ExprDereference>(expression))
		return CompileVmDereference(ctx, module, node);

	if(ExprUnboxing *node = getType<ExprUnboxing>(expression))
		return CompileVmUnboxing(ctx, module, node);

	if(ExprConditional *node = getType<ExprConditional>(expression))
		return CompileVmConditional(ctx, module, node);

	if(ExprAssignment *node = getType<ExprAssignment>(expression))
		return CompileVmAssignment(ctx, module, node);

	if(ExprMemberAccess *node = getType<ExprMemberAccess>(expression))
		return CompileVmMemberAccess(ctx, module, node);

	if(ExprArrayIndex *node = getType<ExprArrayIndex>(expression))
		return CompileVmArrayIndex(ctx, module, node);

	if(ExprReturn *node = getType<ExprReturn>(expression))
		return CompileVmReturn(ctx, module, node);

	if(ExprYield *node = getType<ExprYield>(expression))
		return CompileVmYield(ctx, module, node);

	if(ExprVariableDefinition *node = getType<ExprVariableDefinition>(expression))
		return CompileVmVariableDefinition(ctx, module, node);

	if(ExprArraySetup *node = getType<ExprArraySetup>(expression))
		return CompileVmArraySetup(ctx, module, node);

	if(ExprVariableDefinitions *node = getType<ExprVariableDefinitions>(expression))
		return CompileVmVariableDefinitions(ctx, module, node);

	if(ExprVariableAccess *node = getType<ExprVariableAccess>(expression))
		return CompileVmVariableAccess(ctx, module, node);

	if(ExprFunctionContextAccess *node = getType<ExprFunctionContextAccess>(expression))
		return CompileVmFunctionContextAccess(ctx, module, node);

	if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(expression))
		return CompileVmFunctionDefinition(ctx, module, node);

	if(ExprGenericFunctionPrototype *node = getType<ExprGenericFunctionPrototype>(expression))
		return CompileVmGenericFunctionPrototype(ctx, module, node);

	if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(expression))
		return CompileVmFunctionAccess(ctx, module, node);

	if(ExprFunctionCall *node = getType<ExprFunctionCall>(expression))
		return CompileVmFunctionCall(ctx, module, node);

	if(ExprAliasDefinition *node = getType<ExprAliasDefinition>(expression))
		return CompileVmAliasDefinition(ctx, module, node);

	if(ExprClassPrototype *node = getType<ExprClassPrototype>(expression))
		return CompileVmClassPrototype(ctx, module, node);

	if(ExprGenericClassPrototype *node = getType<ExprGenericClassPrototype>(expression))
		return CompileVmGenericClassPrototype(ctx, module, node);

	if(ExprClassDefinition *node = getType<ExprClassDefinition>(expression))
		return CompileVmClassDefinition(ctx, module, node);

	if(ExprEnumDefinition *node = getType<ExprEnumDefinition>(expression))
		return CompileVmEnumDefinition(ctx, module, node);

	if(ExprIfElse *node = getType<ExprIfElse>(expression))
		return CompileVmIfElse(ctx, module, node);

	if(ExprFor *node = getType<ExprFor>(expression))
		return CompileVmFor(ctx, module, node);

	if(ExprWhile *node = getType<ExprWhile>(expression))
		return CompileVmWhile(ctx, module, node);

	if(ExprDoWhile *node = getType<ExprDoWhile>(expression))
		return CompileVmDoWhile(ctx, module, node);

	if(ExprSwitch *node = getType<ExprSwitch>(expression))
		return CompileVmSwitch(ctx, module, node);

	if(ExprBreak *node = getType<ExprBreak>(expression))
		return CompileVmBreak(ctx, module, node);

	if(ExprContinue *node = getType<ExprContinue>(expression))
		return CompileVmContinue(ctx, module, node);

	if(ExprBlock *node = getType<ExprBlock>(expression))
		return CompileVmBlock(ctx, module, node);

	if(ExprSequence *node = getType<ExprSequence>(expression))
		return CompileVmSequence(ctx, module, node);

	if(!expression)
		return NULL;

	assert(!"unknown type");

	return NULL;
}

VmModule* CompileVm(ExpressionContext &ctx, ExprBase *expression, const char *code)
{
	if(ExprModule *node = getType<ExprModule>(expression))
	{
		VmModule *module = new (ctx.get<VmModule>()) VmModule(ctx.allocator, code);

		// Generate global function
		VmFunction *global = new (module->get<VmFunction>()) VmFunction(module->allocator, VmType::Void, node->source, NULL, node->moduleScope, VmType::Void);

		for(unsigned k = 0; k < global->scope->allVariables.size(); k++)
		{
			VariableData *variable = global->scope->allVariables[k];

			if(variable->isAlloca)
				global->allocas.push_back(variable);
		}

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

			// Skip prototypes that will have an implementation later
			if(function->isPrototype && function->implementation)
				continue;

			if(function->vmFunction)
				continue;

			VmFunction *vmFunction = new (module->get<VmFunction>()) VmFunction(module->allocator, GetVmType(ctx, ctx.typeFunctionID), function->source, function, function->functionScope, GetVmType(ctx, function->type->returnType));

			for(unsigned k = 0; k < vmFunction->scope->allVariables.size(); k++)
			{
				VariableData *variable = vmFunction->scope->allVariables[k];

				if(variable->isAlloca)
					vmFunction->allocas.push_back(variable);
			}

			function->vmFunction = vmFunction;

			module->functions.push_back(vmFunction);
		}

		for(unsigned i = 0; i < ctx.functions.size(); i++)
		{
			FunctionData *function = ctx.functions[i];

			if(function->isPrototype && function->implementation)
				function->vmFunction = function->implementation->vmFunction;
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
			// Try to place address offset into the load/store instruction
		case VM_INST_LOAD_BYTE:
		case VM_INST_LOAD_SHORT:
		case VM_INST_LOAD_INT:
		case VM_INST_LOAD_FLOAT:
		case VM_INST_LOAD_DOUBLE:
		case VM_INST_LOAD_LONG:
		case VM_INST_LOAD_STRUCT:
			if(VmInstruction *address = getType<VmInstruction>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				if(address->cmd == VM_INST_ADD)
				{
					VmConstant *lhsAsConstant = getType<VmConstant>(address->arguments[0]);
					VmConstant *rhsAsConstant = getType<VmConstant>(address->arguments[1]);

					if(lhsAsConstant && !lhsAsConstant->container && !rhsAsConstant)
					{
						VmConstant *totalOffset = CreateConstantInt(ctx.allocator, NULL, offset->iValue + lhsAsConstant->iValue);

						ChangeInstructionTo(module, inst, inst->cmd, address->arguments[1], totalOffset, NULL, NULL, &module->peepholeOptimizations);
					}
					else if(rhsAsConstant && !rhsAsConstant->container && !lhsAsConstant)
					{
						VmConstant *totalOffset = CreateConstantInt(ctx.allocator, NULL, offset->iValue + rhsAsConstant->iValue);

						ChangeInstructionTo(module, inst, inst->cmd, address->arguments[0], totalOffset, NULL, NULL, &module->peepholeOptimizations);
					}
				}
			}
			break;
		case VM_INST_STORE_BYTE:
		case VM_INST_STORE_SHORT:
		case VM_INST_STORE_INT:
		case VM_INST_STORE_FLOAT:
		case VM_INST_STORE_DOUBLE:
		case VM_INST_STORE_LONG:
		case VM_INST_STORE_STRUCT:
			if(VmInstruction *address = getType<VmInstruction>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				if(address->cmd == VM_INST_ADD)
				{
					VmConstant *lhsAsConstant = getType<VmConstant>(address->arguments[0]);
					VmConstant *rhsAsConstant = getType<VmConstant>(address->arguments[1]);

					if(lhsAsConstant && !lhsAsConstant->container && !rhsAsConstant)
					{
						VmConstant *totalOffset = CreateConstantInt(ctx.allocator, NULL, offset->iValue + lhsAsConstant->iValue);

						ChangeInstructionTo(module, inst, inst->cmd, address->arguments[1], totalOffset, inst->arguments[2], NULL, &module->peepholeOptimizations);
					}
					else if(rhsAsConstant && !rhsAsConstant->container && !lhsAsConstant)
					{
						VmConstant *totalOffset = CreateConstantInt(ctx.allocator, NULL, offset->iValue + rhsAsConstant->iValue);

						ChangeInstructionTo(module, inst, inst->cmd, address->arguments[0], totalOffset, inst->arguments[2], NULL, &module->peepholeOptimizations);
					}
				}
			}
			break;
		case VM_INST_EXTRACT:
			if(VmInstruction *load = getType<VmInstruction>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				if(IsLoad(load))
				{
					VmValue *address = load->arguments[0];
					VmConstant *addressOffset = getType<VmConstant>(load->arguments[1]);

					VmConstant *addressAsConstant = getType<VmConstant>(address);

					if (addressAsConstant && addressAsConstant->container)
					{
						assert(addressOffset->iValue == 0);

						VmConstant *shiftedTarget = CreateConstantPointer(module->allocator, NULL, addressAsConstant->iValue + offset->iValue, addressAsConstant->container, addressAsConstant->type.structType, true);

						ChangeInstructionTo(module, inst, GetLoadInstruction(ctx, GetBaseType(ctx, inst->type)), shiftedTarget, CreateConstantInt(module->allocator, NULL, 0), NULL, NULL, &module->peepholeOptimizations);
					}
					else
					{
						VmConstant *totalOffset = CreateConstantInt(ctx.allocator, NULL, offset->iValue + addressOffset->iValue);

						ChangeInstructionTo(module, inst, GetLoadInstruction(ctx, GetBaseType(ctx, inst->type)), address, totalOffset, NULL, NULL, &module->peepholeOptimizations);
					}
				}
			}
			break;
		default:
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
		default:
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

			if(inst->parent)
				inst->parent->RemoveInstruction(inst);
		}
		else if(inst->cmd == VM_INST_JUMP_Z || inst->cmd == VM_INST_JUMP_NZ)
		{
			// Remove conditional branches with constant condition
			if(VmConstant *condition = getType<VmConstant>(inst->arguments[0]))
			{
				if(inst->cmd == VM_INST_JUMP_Z)
					ChangeInstructionTo(module, inst, VM_INST_JUMP, condition->iValue == 0 ? inst->arguments[1] : inst->arguments[2], NULL, NULL, NULL, &module->deadCodeEliminations);
				else
					ChangeInstructionTo(module, inst, VM_INST_JUMP, condition->iValue == 0 ? inst->arguments[2] : inst->arguments[1], NULL, NULL, NULL, &module->deadCodeEliminations);
			}
		}
		else if(inst->cmd == VM_INST_PHI)
		{
			// Remove incoming branches that are never executed (phi instruction is the only user)
			for(unsigned i = 0; i < inst->arguments.size();)
			{
				VmValue *value = inst->arguments[i];
				VmValue *edge = inst->arguments[i + 1];

				if(edge->users.size() == 1 && edge->users[0] == inst)
				{
					value->RemoveUse(inst);
					edge->RemoveUse(inst);

					inst->arguments[i] = inst->arguments[inst->arguments.size() - 2];
					inst->arguments[i + 1] = inst->arguments[inst->arguments.size() - 1];

					inst->arguments.pop_back();
					inst->arguments.pop_back();
				}
				else
				{
					i += 2;
				}
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
				if(IsBlockTerminator(inst->cmd))
				{
					while(curr->lastInstruction && curr->lastInstruction != inst)
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
					// Fixup phi instructions
					for(VmInstruction *inst = next->firstInstruction; inst;)
					{
						VmInstruction *nextInst = inst->nextSibling;

						if(inst->cmd == VM_INST_PHI)
						{
							// Check that phi only expects us to come from the block we are merging with
							assert(inst->arguments.size() == 2 && inst->arguments[1] == curr);

							ReplaceValueUsersWith(module, inst, inst->arguments[0], &module->controlFlowSimplifications);
						}

						inst = nextInst;
					}

					// Steal target block instructions
					for(VmInstruction *inst = next->firstInstruction; inst; inst = inst->nextSibling)
						inst->parent = curr;

					if(curr->lastInstruction)
						curr->lastInstruction->nextSibling = next->firstInstruction;

					if(next->firstInstruction)
						next->firstInstruction->prevSibling = curr->lastInstruction;

					curr->lastInstruction = next->lastInstruction;

					next->firstInstruction = next->lastInstruction = NULL;

					// Remove branch
					if(currLastInst->parent)
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

							ReplaceValue(module, terminator, terminator->arguments[0], target);

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

		// Remove allocas that are only used by stores
		for(unsigned i = 0; i < function->allocas.size(); i++)
		{
			VariableData *variable = function->allocas[i];

			bool nonStoreUse = false;

			for(unsigned k = 0; k < variable->users.size(); k++)
			{
				VmConstant *user = variable->users[k];

				for(unsigned l = 0; l < user->users.size(); l++)
				{
					if(VmInstruction *inst = getType<VmInstruction>(user->users[l]))
					{
						if(inst->cmd >= VM_INST_STORE_BYTE && inst->cmd <= VM_INST_STORE_STRUCT && inst->arguments[0] == user)
							continue;

						nonStoreUse = true;
						break;
					}
					else
					{
						assert(!"invalid constant use");
					}

					if(nonStoreUse)
						break;
				}

				if(nonStoreUse)
					break;
			}

			if(!nonStoreUse)
			{
				SmallArray<VmInstruction*, 32> deadStores;

				for(unsigned k = 0; k < variable->users.size(); k++)
				{
					VmConstant *user = variable->users[k];

					for(unsigned l = 0; l < user->users.size(); l++)
					{
						if(VmInstruction *inst = getType<VmInstruction>(user->users[l]))
							deadStores.push_back(inst);
					}
				}

				for(unsigned k = 0; k < deadStores.size(); k++)
				{
					VmInstruction *inst = deadStores[k];

					if(inst->parent)
						inst->parent->RemoveInstruction(inst);

					module->loadStorePropagations++;
				}
			}
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
				if(VmValue* value = GetLoadStoreInfo(module, curr))
				{
					if(VmConstant* constant = getType<VmConstant>(value))
						ReplaceValueUsersWith(module, curr, CreateConstantInt(module->allocator, value->source, (int)(char)(constant->iValue)), &module->loadStorePropagations);
				}

				AddLoadInfo(module, curr);
				break;
			case VM_INST_LOAD_SHORT:
				if(VmValue* value = GetLoadStoreInfo(module, curr))
				{
					if(VmConstant* constant = getType<VmConstant>(value))
						ReplaceValueUsersWith(module, curr, CreateConstantInt(module->allocator, value->source, (int)(short)(constant->iValue)), &module->loadStorePropagations);
				}

				AddLoadInfo(module, curr);
				break;
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

						value = CreateBitcast(module, curr->source, curr->type, value);

						block->insertPoint = block->lastInstruction;

						module->currentBlock = NULL;

						ReplaceValueUsersWith(module, curr, value, &module->loadStorePropagations);
					}
					else
					{
						ReplaceValueUsersWith(module, curr, value, &module->loadStorePropagations);
					}

					break;
				}

				AddLoadInfo(module, curr);
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
			case VM_INST_CALL:
				ClearLoadStoreInfoAliasing(module);
				ClearLoadStoreInfoGlobal(module);
				break;
			default:
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

					VmConstant *prevAddressAsConst = getType<VmConstant>(prev->arguments[0]);
					VmConstant *prevOffset = getType<VmConstant>(prev->arguments[1]);

					VmConstant *currAddressAsConst = getType<VmConstant>(curr->arguments[0]);
					VmConstant *currOffset = getType<VmConstant>(curr->arguments[1]);

					if(currAddressAsConst && prevAddressAsConst)
						same = *currAddressAsConst == *prevAddressAsConst && prevOffset->iValue == currOffset->iValue;
					else
						same = prev->arguments[0] == curr->arguments[0] && prevOffset->iValue == currOffset->iValue;

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

			if(curr->cmd >= VM_INST_LOAD_INT && curr->cmd <= VM_INST_LOAD_STRUCT)
			{
				VmValue *loadAddress = curr->arguments[0];
				VmConstant *loadOffset = getType<VmConstant>(curr->arguments[1]);

				// Walk up until a memory write is reached
				VmInstruction *prev = curr->prevSibling;

				while(prev && !HasMemoryAccess(prev->cmd))
					prev = prev->prevSibling;

				if(prev && (prev->cmd >= VM_INST_STORE_INT && prev->cmd <= VM_INST_STORE_STRUCT))
				{
					VmValue *storeAddress = prev->arguments[0];
					VmConstant *storeOffset = getType<VmConstant>(prev->arguments[1]);
					VmValue *storeValue = prev->arguments[2];

					if(GetAccessSize(prev) == GetAccessSize(curr) && storeAddress == loadAddress && storeOffset->iValue == loadOffset->iValue && curr->type.size == storeValue->type.size)
					{
						if(curr->type != storeValue->type)
						{
							module->currentBlock = block;

							block->insertPoint = curr->prevSibling;

							storeValue = CreateBitcast(module, curr->source, curr->type, storeValue);

							block->insertPoint = block->lastInstruction;

							module->currentBlock = NULL;

							ReplaceValueUsersWith(module, curr, storeValue, &module->loadStorePropagations);
						}
						else
						{
							ReplaceValueUsersWith(module, curr, storeValue, &module->loadStorePropagations);
						}
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
		for(VmInstruction *curr = block->firstInstruction; curr;)
		{
			VmInstruction *next = curr->nextSibling;

			if(curr->hasSideEffects || curr->hasMemoryAccess)
			{
				curr = next;
				continue;
			}

			VmInstruction *prev = curr->prevSibling;

			unsigned distance = 0;

			while(prev && distance < 64)
			{
				if(prev->cmd == curr->cmd && prev->arguments.count == curr->arguments.count)
				{
					bool same = true;

					for(unsigned i = 0, e = curr->arguments.count; i < e; i++)
					{
						VmValue *currArg = curr->arguments.data[i];
						VmValue *prevArg = prev->arguments.data[i];

						VmConstant *currArgAsConst = currArg->typeID == VmConstant::myTypeID ? static_cast<VmConstant*>(currArg) : NULL;
						VmConstant *prevArgAsConst = prevArg->typeID == VmConstant::myTypeID ? static_cast<VmConstant*>(prevArg) : NULL;

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

				prev = prev->prevSibling;
				distance++;
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
		if(curr->cmd == VM_INST_CALL || IsLoad(curr))
		{
			// If call/load instuction has even a single user, store it's result in a temporary register so that the store will be lowered at the correct place
			if(curr->users.empty())
				continue;

			if(curr->users.size() == 1)
			{
				if(VmInstruction *inst = getType<VmInstruction>(curr->users[0]))
				{
					if(curr->nextSibling == inst)
					{
						// But if the single user is the next instruction that itself will be lowered at the correct place, result can still be forwarded without a temporary
						if(inst->type == VmType::Void)
							continue;

						if(inst->cmd == VM_INST_CALL)
							continue;

						if(IsLoad(inst))
							continue;
					}
				}
			}
		}
		else
		{
			if(curr->users.size() <= 1)
				continue;
		}

		if(curr->type == VmType::Block)
			continue;

		if(curr->cmd == VM_INST_CONSTRUCT && (curr->type.type == VM_TYPE_FUNCTION_REF || curr->type.type == VM_TYPE_ARRAY_REF))
			continue;

		if(curr->cmd == VM_INST_FUNCTION_ADDRESS || curr->cmd == VM_INST_TYPE_ID)
			continue;

		TypeBase *type = GetBaseType(ctx, curr->type);

		VmConstant *address = CreateAlloca(ctx, module, curr->source, type, "reg");

		address->container->isVmRegSpill = true;

		block->insertPoint = curr;

		curr->canBeRemoved = false;

		ReplaceValueUsersWith(module, curr, CreateLoad(ctx, module, curr->source, type, address, 0), NULL);

		curr->canBeRemoved = true;

		block->insertPoint = curr;

		CreateStore(ctx, module, curr->source, type, address, curr);

		block->insertPoint = block->lastInstruction;

		// Skip generated load and store instructions
		curr = curr->nextSibling;
		curr = curr->nextSibling;
	}

	module->currentBlock = NULL;
}

void LegalizeVmInstructions(ExpressionContext &ctx, VmModule *module, VmBlock *block)
{
	for(VmInstruction *curr = block->firstInstruction; curr; curr = curr->nextSibling)
	{
		if(curr->cmd == VM_INST_EXTRACT)
		{
			VmValue *target = curr->arguments[0];
			VmConstant *offset = getType<VmConstant>(curr->arguments[1]);

			VmInstruction *targetAsInst = getType<VmInstruction>(target);

			if(targetAsInst && targetAsInst->cmd == VM_INST_CONSTRUCT)
			{
				bool replaced = false;

				int pos = 0;

				for(unsigned i = 0; i < targetAsInst->arguments.size(); i++)
				{
					VmValue *argument = targetAsInst->arguments[i];

					if(offset->iValue == pos && argument->type.size == curr->type.size)
					{
						if(VmFunction *function = getType<VmFunction>(argument))
						{
							module->currentBlock = block;

							block->insertPoint = curr;

							argument = CreateBitcast(module, curr->source, VmType::Int, CreateFunctionAddress(module, curr->source, function->function));

							block->insertPoint = block->lastInstruction;

							module->currentBlock = NULL;
						}

						ReplaceValueUsersWith(module, curr, argument, NULL);

						replaced = curr->users.empty();
						break;
					}

					pos += argument->type.size;
				}

				if(replaced)
					continue;
			}

			VmConstant *address = CreateAlloca(ctx, module, curr->source, GetBaseType(ctx, target->type), "construct");

			FinalizeAlloca(ctx, module, address->container);

			module->currentBlock = block;

			block->insertPoint = curr;

			CreateStore(ctx, module, curr->source, GetBaseType(ctx, target->type), address, target);

			VmConstant *shiftAddress = CreateConstantPointer(module->allocator, curr->source, offset->iValue, address->container, ctx.GetReferenceType(GetBaseType(ctx, curr->type)), true);

			ReplaceValueUsersWith(module, curr, CreateLoad(ctx, module, curr->source, GetBaseType(ctx, curr->type), shiftAddress, 0), NULL);

			block->insertPoint = block->lastInstruction;

			module->currentBlock = NULL;
		}
	}
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

		VmConstant *address = CreateAlloca(ctx, module, curr->source, type, "reg");

		address->container->isVmRegSpill = true;

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

		ReplaceValueUsersWith(module, curr, CreateLoad(ctx, module, curr->source, type, address, 0), NULL);

		block->insertPoint = block->lastInstruction;

		module->currentBlock = NULL;
	}
}

void RunLegalizeVm(ExpressionContext &ctx, VmModule *module, VmValue* value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		module->currentFunction = function;

		// Legal code doesn't contain dead instructions
		RunDeadCodeElimiation(ctx, module, function);

		for(VmBlock *curr = function->firstBlock; curr; curr = curr->nextSibling)
			RunLegalizeVm(ctx, module, curr);

		module->currentFunction = NULL;
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		LegalizeVmInstructions(ctx, module, block);

		LegalizeVmPhiStorage(ctx, module, block);

		LegalizeVmRegisterUsage(ctx, module, block);
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
